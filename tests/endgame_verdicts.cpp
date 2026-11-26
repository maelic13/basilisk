/// The evaluator's hard verdicts read against Syzygy WDL.
///
///     endgame_verdicts --tables PATH [--threads N] [--random N] [--seed S]
///                      [--estimates] [--max-rate R] [--dump FILE] CLASS...
///
/// --dump writes every judged position (site index, FEN, the side to move's
/// WDL, the side the claim is read for: w or b) to FILE, one tab-separated
/// line each. It writes every position, judged or not, with "-" for the WDL and then the endgame layer's step, the
/// score it read and its verdict.
///
/// --tables fixtures reads the bundled fixtures (CI); --golden leaves out what
/// varies between runs (times, examples, the search check), so the output
/// can be compared with tests/data/endgame_verdicts_golden_rebuild.txt.
///
/// A hard verdict is a value the evaluator states rather than estimates: a
/// recognised draw, a known-win floor, a scale at its floor. For each material
/// class (KBPvK, KRPvKR, ...; White is the first side) this enumerates every
/// legal position, or draws --random N of them, keeps the quiet ones (the side
/// to move not in check, with a move, and no capture or promotion among its
/// moves), attributes each to the evaluator site that fired, and probes the
/// tables where a site fired. A site is wrong where the tables contradict its
/// claim:
///   - a draw claim (the search's dead-position rule, KNNK, the drawn KPK, the
///     wrong-corner KBP-K, the pawnless scale of 0) where either side wins;
///   - a win claim (a score at or above KNOWN_WIN) where the side it favours
///     does not win;
///   - a scale claim (the pawnless 4 and 14, KRP-KR's draw rules and floor,
///     the KRPP-KRP table) where the side it scales down wins.
/// Wins and losses are the tables' under the fifty-move rule; a cursed win or
/// blessed loss is a draw and is counted beside. A site wrong in more than
/// --max-rate of its positions (0.005, frozen in PLAN B.9.3) is a defect; the
/// exit status is 1 when any is, 0 when none, 2 on a harness error.
///
/// Exhaustive runs use the board's symmetry: without pawns the white king
/// stands on the a1-d1-d4 triangle, with pawns on files a to d. Each position
/// is read with each side to move. --estimates also probes the positions no
/// site claims and counts those scored at 300 or more for a side the tables
/// call drawn (F6, the same-coloured bishops).

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "move.h"
#include "search.h"
#include "tt.h"
#include "syzygy.h"
#include "syzygy_fixture.h"
#include "zobrist.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

// --dump: every judged position's site, FEN and the side to move's WDL, one
// line each, for reading a site's errors by its own conditions.
FILE* g_dump = nullptr;
using EvalRebuildRecordOrNone = EvalRebuildRecord;
constexpr bool REBUILT_ARM = true;
std::mutex g_dump_mutex;

enum Claim { DRAW, WIN, SCALE, ESTIMATE };

struct SiteDef {
    const char* name;
    Claim claim;
};

enum Site {
    S_DEAD, S_KNNK, S_KPK_DRAW, S_WRONG_BISHOP, S_PAWNLESS_0,
    S_KPK_WIN, S_TIER_Q, S_TIER_R, S_MINOR_MATE, S_KNOWN_WIN,
    S_PAWNLESS_4, S_PAWNLESS_14,
    S_KRPKR_1, S_KRPKR_2, S_KRPKR_3, S_KRPKR_4, S_KRPKR_5, S_KRPKR_FLOOR,
    S_KRPPKRP_FLOOR,
    S_ESTIMATE,
    SITE_NB
};

constexpr std::array<SiteDef, SITE_NB> SITES{{
    {"dead position (search)", DRAW}, {"KNNK draw", DRAW}, {"KPK draw", DRAW},
    {"wrong-bishop KBP-K draw", DRAW}, {"pawnless scale 0", DRAW},
    {"KPK win", WIN}, {"bare king, queen tier", WIN}, {"bare king, rook tier", WIN},
    {"bare king, minor mate", WIN}, {"other known win", WIN},
    {"pawnless scale 4", SCALE}, {"pawnless scale 14", SCALE},
    {"KRP-KR draw rule 1 (Philidor)", SCALE}, {"KRP-KR draw rule 2 (sixth, from behind)", SCALE},
    {"KRP-KR draw rule 3 (king on the square)", SCALE}, {"KRP-KR draw rule 4 (a7 rook pawn)", SCALE},
    {"KRP-KR draw rule 5 (blockade)", SCALE}, {"KRP-KR other scale under the floor", SCALE},
    {"KRPP-KRP table under the floor", SCALE},
    {"estimate (no site)", ESTIMATE},
}};

// A wrong position kept for the search check: its FEN, the side to move's
// WDL, and whether the claim's side is to move.
struct Wrong {
    std::string fen;
    int stm_wdl = 0;
    bool strong_to_move = false;
};

constexpr size_t SAMPLE = 200;   // wrong positions kept per site for the search check

struct Tally {
    int64_t quiet = 0;
    std::array<int64_t, SITE_NB> positions{}, wrong{}, cursed{}, unprobed{};
    // Of a draw claim's wrong positions, those White (the class's first side) wins.
    std::array<int64_t, SITE_NB> white_wins{};
    std::array<std::vector<std::string>, SITE_NB> examples;
    std::array<std::vector<Wrong>, SITE_NB> sample;

    void add(const Tally& o) {
        quiet += o.quiet;
        for (size_t s = 0; s < SITE_NB; ++s) {
            positions[s] += o.positions[s];
            wrong[s] += o.wrong[s];
            cursed[s] += o.cursed[s];
            unprobed[s] += o.unprobed[s];
            white_wins[s] += o.white_wins[s];
            for (const auto& e : o.examples[s])
                if (examples[s].size() < 3)
                    examples[s].push_back(e);
            for (const auto& w : o.sample[s])
                if (sample[s].size() < SAMPLE)
                    sample[s].push_back(w);
        }
    }
};

struct Piece2 {
    Color color;
    PieceType type;
};

struct ClassSpec {
    std::string name;
    std::vector<Piece2> pieces;  // kings first
    bool pawns = false;
};

bool parse_class(const std::string& name, ClassSpec& out) {
    const auto v = name.find('v');
    if (name.empty() || name[0] != 'K' || v == std::string::npos || v + 1 >= name.size()
        || name[v + 1] != 'K')
        return false;
    out = ClassSpec{name, {{WHITE, KING}, {BLACK, KING}}, false};
    auto add = [&](Color c, const std::string& part) {
        for (char ch : part) {
            PieceType pt;
            switch (ch) {
            case 'Q': pt = QUEEN; break;
            case 'R': pt = ROOK; break;
            case 'B': pt = BISHOP; break;
            case 'N': pt = KNIGHT; break;
            case 'P': pt = PAWN; out.pawns = true; break;
            default: return false;
            }
            out.pieces.push_back({c, pt});
        }
        return true;
    };
    return add(WHITE, name.substr(1, v - 1)) && add(BLACK, name.substr(v + 2));
}

char piece_char(Piece2 p) {
    static const char* letters = "PNBRQK";
    const char c = letters[int(p.type) - int(PAWN)];
    return p.color == WHITE ? c : char(c - 'A' + 'a');
}

std::string fen_of(const ClassSpec& spec, const std::array<int, 8>& squares, Color stm) {
    char grid[64] = {};
    for (size_t i = 0; i < spec.pieces.size(); ++i)
        grid[squares[i]] = piece_char(spec.pieces[i]);
    std::string fen;
    for (int r = 7; r >= 0; --r) {
        int run = 0;
        for (int f = 0; f < 8; ++f) {
            const char c = grid[r * 8 + f];
            if (!c) { ++run; continue; }
            if (run) { fen += char('0' + run); run = 0; }
            fen += c;
        }
        if (run) fen += char('0' + run);
        if (r) fen += '/';
    }
    fen += stm == WHITE ? " w - - 0 1" : " b - - 0 1";
    return fen;
}

bool quiet(const Board& b) {
    if (b.is_in_check())
        return false;
    MoveList legal;
    b.gen_legal(legal);
    if (legal.size() == 0)
        return false;
    for (Move m : legal)
        if (move_type(m) == PROMOTION || move_type(m) == EN_PASSANT || b.piece_on(to_sq(m)) != NO_PIECE)
            return false;
    return true;
}

bool lone(const Board& b, Color c) {
    return b.occupancy_bb(c) == sq_bb(b.king_square(c));
}

// The site that fired, attributed in the order the evaluator decides, and
// the side whose claim is read: the side a win favours, the side a scale
// scales down. The rebuilt evaluation adds its tempo after the endgame layer,
// so the served score's sign is not the side the layer scaled: on that arm
// the layer's own step is read from the evaluator's record (`rec`), its
// verdict, whether it scaled, and the score it read.
constexpr int REBUILD_LAYER_SCALED = 1;  // EvalRebuildRecord::layer
Site classify(Evaluator& ev, const Board& b, int& stm_score, Color& strong,
              [[maybe_unused]] EvalRebuildRecordOrNone& rec) {
    stm_score = ev.evaluate_undamped(b);
    const int white_score = b.turn() == WHITE ? stm_score : -stm_score;
    strong = white_score >= 0 ? WHITE : BLACK;
    if (b.is_insufficient_material())
        return S_DEAD;
    ev.rebuild_record(b, rec);
    const bool verdict = rec.verdict != 0;
    const bool layer_scaled = rec.layer == REBUILD_LAYER_SCALED;
    const Color layer_strong = rec.layer_input > 0 ? WHITE : BLACK;
    const int pawns = popcount(b.piece_bb(WHITE, PAWN) | b.piece_bb(BLACK, PAWN));
    auto pieces = [&](Color c) {
        return b.piece_bb(c, KNIGHT) | b.piece_bb(c, BISHOP) | b.piece_bb(c, ROOK) | b.piece_bb(c, QUEEN);
    };
    if (is_knnk_draw(b))
        return S_KNNK;
    if (pawns == 1 && !pieces(WHITE) && !pieces(BLACK)
        && (lone(b, WHITE) || lone(b, BLACK))) {
        strong = b.piece_bb(WHITE, PAWN) ? WHITE : BLACK;
        return verdict ? S_KPK_DRAW : S_KPK_WIN;
    }
    if (std::abs(stm_score) >= KNOWN_WIN) {
        const Color weak = ~strong;
        if (!lone(b, weak))
            return S_KNOWN_WIN;
        if (b.piece_bb(strong, QUEEN))
            return S_TIER_Q;
        if (b.piece_bb(strong, ROOK))
            return S_TIER_R;
        return S_MINOR_MATE;
    }
    if (verdict) {
        for (Color c : {WHITE, BLACK})
            if (lone(b, ~c) && b.piece_bb(c, BISHOP) && b.piece_bb(c, PAWN)) {
                strong = c;
                return S_WRONG_BISHOP;
            }
        return S_PAWNLESS_0;
    }
    if (layer_scaled && (lone(b, WHITE) || lone(b, BLACK) || !b.piece_bb(WHITE, PAWN) || !b.piece_bb(BLACK, PAWN))) {
        strong = layer_strong;
        const int scale = pawnless_scale(b, strong);
        if (scale == 4)
            return S_PAWNLESS_4;
        if (scale == 14)
            return S_PAWNLESS_14;
    }
    const Bitboard minors_queens = b.piece_bb(WHITE, KNIGHT) | b.piece_bb(WHITE, BISHOP)
                                 | b.piece_bb(WHITE, QUEEN) | b.piece_bb(BLACK, KNIGHT)
                                 | b.piece_bb(BLACK, BISHOP) | b.piece_bb(BLACK, QUEEN);
    if (layer_scaled && !minors_queens && popcount(b.piece_bb(WHITE, ROOK)) == 1
        && popcount(b.piece_bb(BLACK, ROOK)) == 1) {
        const int wp = popcount(b.piece_bb(WHITE, PAWN)), bp = popcount(b.piece_bb(BLACK, PAWN));
        const Color pawn_side = wp >= bp ? WHITE : BLACK;
        const int sp = std::max(wp, bp), dp = std::min(wp, bp);
        if (sp == 1 && dp == 0) {
            int rule = 0;
            const int scale = krpkr_scale_unfloored(b, pawn_side, rule);
            strong = pawn_side;
            if (rule)
                return Site(S_KRPKR_1 + rule - 1);
            if (scale >= 0 && scale < 24)
                return S_KRPKR_FLOOR;
        } else if (sp == 2 && dp == 1) {
            const int scale = krppkrp_scale_unfloored(b, pawn_side);
            strong = pawn_side;
            if (scale >= 0 && scale < 24)
                return S_KRPPKRP_FLOOR;
        }
    }
    return S_ESTIMATE;
}

struct Reader {
    Reader(const ClassSpec& s, bool e) : spec(s), estimates(e) {}
    const ClassSpec& spec;
    bool estimates;
    std::unique_ptr<Evaluator> ev = std::make_unique<Evaluator>();
    std::mt19937_64 rng{0x5eed};
    Tally tally;

    void read(const std::array<int, 8>& squares) {
        for (Color stm : {WHITE, BLACK}) {
            const std::string fen = fen_of(spec, squares, stm);
            Board b;
            if (!b.try_set_fen(fen, /*validate_legal_position=*/true) || !quiet(b))
                continue;
            ++tally.quiet;
            int score = 0;
            Color strong = WHITE;
            EvalRebuildRecordOrNone rec;
            const auto site = size_t(classify(*ev, b, score, strong, rec));
            ++tally.positions[site];
            const Claim claim = SITES[site].claim;
            // The dump holds every position, with the layer's step, so that
            // a difference between two dumps can be split by its cause.
            if (g_dump) {
                std::scoped_lock lock(g_dump_mutex);
                std::fprintf(g_dump, "%zu\t%s\t-\t%c\t%d\t%d\t%d\n", site, fen.c_str(),
                             strong == WHITE ? 'w' : 'b', rec.layer, rec.layer_input, rec.verdict);
            }
            if (claim == ESTIMATE && (!estimates || std::abs(score) < 300))
                continue;
            const auto wdl = Syzygy::probe_wdl(b, 7, true);
            if (!wdl) {
                ++tally.unprobed[site];
                continue;
            }
            const int stm_wdl = int(*wdl);
            const int strong_wdl = strong == b.turn() ? stm_wdl : -stm_wdl;
            bool wrong = false;
            switch (claim) {
            case DRAW:     wrong = std::abs(stm_wdl) == 2; break;
            case WIN:      wrong = strong_wdl != 2; break;
            case SCALE:    wrong = strong_wdl == 2; break;
            case ESTIMATE: wrong = stm_wdl == 0; break;
            }
            if (std::abs(stm_wdl) == 1)
                ++tally.cursed[site];
            if (g_dump && claim != ESTIMATE && !REBUILT_ARM) {
                std::scoped_lock lock(g_dump_mutex);
                std::fprintf(g_dump, "%zu\t%s\t%d\t%c\n", site, fen.c_str(), stm_wdl,
                             strong == WHITE ? 'w' : 'b');
            }
            if (wrong) {
                ++tally.wrong[site];
                if (claim == DRAW && (b.turn() == WHITE ? stm_wdl : -stm_wdl) == 2)
                    ++tally.white_wins[site];
                // A reservoir per thread, so the sample spans the enumeration.
                auto& kept = tally.sample[site];
                const Wrong w{fen, stm_wdl, strong == b.turn()};
                if (kept.size() < SAMPLE)
                    kept.push_back(w);
                else if (const auto k = rng() % uint64_t(tally.wrong[site]); k < SAMPLE)
                    kept[k] = w;
                if (tally.examples[site].size() < 3)
                    tally.examples[site].push_back(fen + "  eval " + std::to_string(score)
                                                   + " wdl " + std::to_string(stm_wdl));
            }
        }
    }
};

// Whether a depth-8 search without tables agrees with the tables where the
// site did not: it finds the win the site denied, or no longer claims the win
// the site claimed. Returns the count that agree.
int search_check(const std::vector<Wrong>& sample, Claim claim, int threads) {
    std::atomic<size_t> next{0};
    std::atomic<int> agree{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < std::min<int>(threads, int(sample.size())); ++t) {
        pool.emplace_back([&] {
            for (size_t k; (k = next.fetch_add(1)) < sample.size();) {
                const Wrong& w = sample[k];
                Board b;
                b.set_fen(w.fen);
                TranspositionTable tt(4);
                std::atomic_bool stop{false};
                SearchLimits limits;
                limits.depth = 8;
                limits.nodes = 2'000'000;
                auto searcher = std::make_unique<Searcher>(tt, stop);
                const int score = searcher->search(b, limits).score;   // the side to move's
                bool ok = false;
                if (claim == WIN) {
                    const int strong_score = w.strong_to_move ? score : -score;
                    ok = strong_score < 150;
                } else {
                    // The tables name a winner: the side to move when its WDL is 2.
                    const int winner_score = w.stm_wdl > 0 ? score : -score;
                    ok = winner_score >= 150;
                }
                agree += ok;
            }
        });
    }
    for (auto& t : pool)
        t.join();
    return agree.load();
}

const std::array<int, 10> TRIANGLE{0, 1, 2, 3, 9, 10, 11, 18, 19, 27};  // a1 b1 c1 d1 b2 c2 d2 c3 d3 d4

bool square_ok(const ClassSpec& spec, size_t i, int sq) {
    if (spec.pieces[i].type == PAWN)
        return sq >= 8 && sq < 56;
    return true;
}

// Every placement of pieces[from..], squares strictly increasing within a run
// of identical pieces so that each set is placed once.
template<class F>
void place(const ClassSpec& spec, size_t i, std::array<int, 8>& squares, Bitboard used, F&& visit) {
    if (i == spec.pieces.size()) {
        visit(squares);
        return;
    }
    const bool same_as_previous = i > 2 && spec.pieces[i].color == spec.pieces[i - 1].color
                                && spec.pieces[i].type == spec.pieces[i - 1].type;
    for (int sq = same_as_previous ? squares[i - 1] + 1 : 0; sq < 64; ++sq) {
        if ((used & sq_bb(Square(sq))) || !square_ok(spec, i, sq))
            continue;
        squares[i] = sq;
        place(spec, i + 1, squares, used | sq_bb(Square(sq)), visit);
    }
}

Tally run_exhaustive(const ClassSpec& spec, int threads, bool estimates) {
    std::vector<std::pair<int, int>> items;
    if (spec.pawns) {
        for (int wk = 0; wk < 64; ++wk)
            if ((wk & 7) <= 3)
                for (int bk = 0; bk < 64; ++bk)
                    items.emplace_back(wk, bk);
    } else {
        for (int wk : TRIANGLE)
            for (int bk = 0; bk < 64; ++bk)
                items.emplace_back(wk, bk);
    }
    std::atomic<size_t> next{0};
    std::mutex mutex;
    Tally total;
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&] {
            Reader reader(spec, estimates);
            for (size_t k; (k = next.fetch_add(1)) < items.size();) {
                const auto [wk, bk] = items[k];
                if (wk == bk)
                    continue;
                std::array<int, 8> squares{};
                squares[0] = wk;
                squares[1] = bk;
                place(spec, 2, squares, sq_bb(Square(wk)) | sq_bb(Square(bk)),
                      [&](const std::array<int, 8>& s) { reader.read(s); });
            }
            std::scoped_lock lock(mutex);
            total.add(reader.tally);
        });
    }
    for (auto& t : pool)
        t.join();
    return total;
}

Tally run_random(const ClassSpec& spec, int threads, bool estimates, int64_t target, uint64_t seed) {
    std::atomic<int64_t> quiet_seen{0};
    std::mutex mutex;
    Tally total;
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            Reader reader(spec, estimates);
            std::mt19937_64 rng(seed * 1000003u + uint64_t(t));
            // The top six bits of the engine's output: mt19937_64's sequence is
            // fixed by the standard, a distribution's is not (libstdc++ and
            // libc++ draw differently), and the golden sample must match on both.
            const auto square = [](std::mt19937_64& g) { return int(g() >> 58); };
            while (quiet_seen.load(std::memory_order_relaxed) < target) {
                std::array<int, 8> squares{};
                Bitboard used = 0;
                bool ok = true;
                for (size_t i = 0; i < spec.pieces.size() && ok; ++i) {
                    int sq = square(rng);
                    for (int tries = 0; ((used & sq_bb(Square(sq))) || !square_ok(spec, i, sq)) && tries < 64; ++tries)
                        sq = square(rng);
                    ok = !(used & sq_bb(Square(sq))) && square_ok(spec, i, sq);
                    squares[i] = sq;
                    used |= sq_bb(Square(sq));
                }
                if (!ok)
                    continue;
                const int64_t before = reader.tally.quiet;
                reader.read(squares);
                quiet_seen.fetch_add(reader.tally.quiet - before, std::memory_order_relaxed);
            }
            std::scoped_lock lock(mutex);
            total.add(reader.tally);
        });
    }
    for (auto& t : pool)
        t.join();
    return total;
}

}  // namespace

int main(int argc, char** argv) {
    std::string tables;
    int threads = int(std::max(1u, std::thread::hardware_concurrency()));
    int64_t random = 0;
    uint64_t seed = 20261116;
    bool estimates = false;
    bool golden = false;
    double max_rate = 0.005;
    std::vector<std::string> classes;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "%s needs a value\n", a.c_str()); std::exit(2); }
            return argv[++i];
        };
        if (a == "--tables") tables = value();
        else if (a == "--threads") threads = std::max(1, std::atoi(value().c_str()));
        else if (a == "--random") random = std::atoll(value().c_str());
        else if (a == "--seed") seed = std::strtoull(value().c_str(), nullptr, 10);
        else if (a == "--estimates") estimates = true;
        else if (a == "--golden") golden = true;
        else if (a == "--max-rate") max_rate = std::atof(value().c_str());
        else if (a == "--dump") {
            g_dump = std::fopen(value().c_str(), "w");
            if (!g_dump) { std::fprintf(stderr, "HARNESS ERROR: cannot write the dump\n"); return 2; }
        }
        else classes.push_back(a);
    }
    if (tables.empty() || classes.empty()) {
        std::fprintf(stderr, "usage: endgame_verdicts --tables PATH [--threads N] [--random N]"
                             " [--seed S] [--estimates] [--max-rate R] CLASS...\n");
        return 2;
    }

    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();
    if (tables == "fixtures") {
#ifdef _WIN32
        const char separator = ';';
#else
        const char separator = ':';
#endif
        tables = syzygy_fixture_path().string() + separator + kpk_fixture_path().string()
               + separator + verdict_fixture_path().string();
    }
    if (!Syzygy::init(tables) || !Syzygy::enabled()) {
        std::fprintf(stderr, "HARNESS ERROR: no tables at %s\n", tables.c_str());
        return 2;
    }

    bool defect = false;
    for (const std::string& name : classes) {
        ClassSpec spec;
        if (!parse_class(name, spec) || spec.pieces.size() > 8) {
            std::fprintf(stderr, "HARNESS ERROR: cannot read class %s\n", name.c_str());
            return 2;
        }
        if (int(spec.pieces.size()) > Syzygy::largest()) {
            std::fprintf(stderr, "HARNESS ERROR: %s has %zu men; the tables go to %d\n",
                         name.c_str(), spec.pieces.size(), Syzygy::largest());
            return 2;
        }
        const auto start = std::chrono::steady_clock::now();
        const Tally t = random > 0 ? run_random(spec, threads, estimates, random, seed)
                                   : run_exhaustive(spec, threads, estimates);
        // A golden run prints what is the same on every run and machine: no
        // time, no examples, no search check (its sample depends on the threads).
        const double secs = golden ? 0.0
            : std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::printf("%s: %lld quiet positions (%s), %.1f s\n", name.c_str(), static_cast<long long>(t.quiet),
                    random > 0 ? "random" : "exhaustive under symmetry", secs);
        for (size_t s = 0; s < SITE_NB; ++s) {
            if (!t.positions[s])
                continue;
            const bool judged = SITES[s].claim != ESTIMATE;
            const double rate = t.positions[s] ? double(t.wrong[s]) / double(t.positions[s]) : 0.0;
            const bool bad = judged && rate > max_rate;
            defect |= bad;
            if (t.unprobed[s] && SITES[s].claim != ESTIMATE) {
                std::fprintf(stderr, "HARNESS ERROR: %s %s: %lld positions unprobed\n", name.c_str(),
                             SITES[s].name, static_cast<long long>(t.unprobed[s]));
                return 2;
            }
            std::printf("  %-42s %12lld  wrong %10lld (%.4f%%)  cursed %lld%s\n", SITES[s].name,
                        static_cast<long long>(t.positions[s]), static_cast<long long>(t.wrong[s]),
                        100.0 * rate, static_cast<long long>(t.cursed[s]),
                        bad ? "  DEFECT" : (judged ? "" : (estimates ? "  (scored >= 300, drawn)" : "")));
            if (SITES[s].claim == DRAW && t.wrong[s])
                std::printf("      of the wrong, White (the first side) wins %lld, Black %lld\n",
                            static_cast<long long>(t.white_wins[s]),
                            static_cast<long long>(t.wrong[s] - t.white_wins[s]));
            if (golden)
                continue;
            if (judged && !t.sample[s].empty())
                std::printf("      a depth-8 search without tables agrees with the tables on %d of %zu sampled\n",
                            search_check(t.sample[s], SITES[s].claim, threads), t.sample[s].size());
            for (const auto& e : t.examples[s])
                std::printf("      %s\n", e.c_str());
        }
        std::fflush(stdout);
    }
    return defect ? 1 : 0;
}
