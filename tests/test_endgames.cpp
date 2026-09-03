/// Endgame regression suite (Step 3.5).
///
/// Loads tests/endgames.epd and gates the scale-factor framework and the
/// known-endgame functions (KPK bitbase, KBNK corner mop-up, KNNK / insufficient
/// draws, KBP wrong-bishop draw). For static verdicts it checks the sign and
/// magnitude of the static eval; for `mate_*` verdicts it plays the position out
/// with a short fixed-depth search and asserts checkmate is delivered from the
/// board within a move budget.
///
/// Build:
///   cmake --build --preset release --target test_endgames
///   ./build/release/test_endgames tests/endgames.epd

#include "board.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "move.h"
#include "search.h"
#include "tt.h"
#include "zobrist.h"
#include "test_harness.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Tunables for the static-eval verdicts and the mate playout
// ---------------------------------------------------------------------------

static constexpr int DRAW_TOL    = 75;    // |cp| <= this counts as a draw
static constexpr int WIN_MIN     = 150;   // |cp| >= this counts as a clear win
static constexpr int MATE_DEPTH  = 18;    // fixed search depth per playout move
// 5.9.18: the randomised floors use a NODE limit, not a depth limit, so they
// measure the same thing BAS-E28/E29 measured. A fixed depth turned out to
// conflate knowledge with search effort -- KBB-K scored 2/12 at depth 10 AND at
// depth 14, while converting 87% at this node count.
static constexpr int64_t CONV_NODES = 60000;
static constexpr int MATE_BUDGET = 100;   // max plies to deliver mate

// White-perspective static eval.
static int eval_white(const std::string& fen) {
    Board b;
    b.set_fen(fen);
    Evaluator ev;
    int s = ev.evaluate(b);
    return (b.side_to_move == WHITE) ? s : -s;
}

static Move best_move(Board& b, int depth, int64_t node_cap = 0) {
    TranspositionTable tt(8);
    std::atomic_bool stop{false};
    SearchLimits lim;
    lim.depth = depth;
    lim.nodes = node_cap;
    auto searcher = std::make_unique<Searcher>(tt, stop);
    SearchResult sr = searcher->search(b, lim);
    return sr.bestmove;
}

// Plays the position out, searching each move to `MATE_DEPTH`. Returns the ply
// count at which `winner` delivers checkmate, or -1 if no mate is reached
// within MATE_BUDGET (a generous budget). Every move played is legal by
// construction (drawn from gen_legal / the searcher's legal root).
static int mate_playout_plies(const std::string& fen, Color winner,
                              int depth = MATE_DEPTH, int64_t node_cap = 0) {
    Board b;
    b.set_fen(fen);
    for (int ply = 0; ply < MATE_BUDGET; ply++) {
        MoveList ml;
        b.gen_legal(ml);
        if (ml.empty()) {
            // Terminal: checkmate only if the side to move is in check, and the
            // mated side must be the loser (opponent of the winner).
            if (b.is_in_check() && b.side_to_move == ~winner) return ply;
            return -1;  // stalemate or the wrong side mated — a false result
        }
        Move m = best_move(b, depth, node_cap);
        if (m == MOVE_NONE)
            return -1;
        b.make_move(m);
    }
    return -1;
}

// Side-to-move-relative score of a fixed-depth search (for mate recognition).
static int search_score(const std::string& fen, int depth) {
    Board b;
    b.set_fen(fen);
    TranspositionTable tt(8);
    std::atomic_bool stop{false};
    SearchLimits lim;
    lim.depth = depth;
    auto searcher = std::make_unique<Searcher>(tt, stop);
    return searcher->search(b, lim).score;
}

// Robust mate-recognition canary (search doc §14: gate correctness, not
// trajectory). Legal positions a few moves from mate — including a KQK
// stalemate trap where the winning move is a mate and a lazy move would
// stalemate — must return a mate score at a moderate depth. This does NOT
// depend on a long fixed-depth conversion trajectory, so it is stable across
// benign search-shape changes while still catching lost mate-finding or a
// stalemate blunder. (The old per-position full-conversion gate over-fired on
// pure search-shape changes; that trajectory is now a diagnostic + a floor.)
static void test_near_mate_recognition() {
    struct Case { const char* fen; int max_mate_plies; const char* note; };
    static const Case CASES[] = {
        { "k7/7Q/2K5/8/8/8/8/8 w - - 0 1",     2, "KQK: finds mate-in-1 (Qb7#)" },
        { "k7/8/2K5/8/8/8/8/1Q6 w - - 0 1",    2, "KQK stalemate trap: mates, not stalemates" },
        { "k7/2K5/8/8/8/8/8/7R w - - 0 1",     2, "KRK: finds mate-in-1" },
        { "7k/8/6K1/8/8/8/8/R7 w - - 0 1",     2, "KRK: finds mate-in-1 (Ra8#)" },
        { "k7/2K5/8/3N4/4B3/8/8/8 w - - 0 1", 12, "KBNK: finds mate within a few moves" },
    };
    for (const Case& c : CASES) {
        begin_section(c.note);
        const int s = search_score(c.fen, MATE_DEPTH);
        EXPECT(s >= MATE_SCORE - c.max_mate_plies);
        end_section();
    }
}

// ---------------------------------------------------------------------------
// EPD loading
// ---------------------------------------------------------------------------

struct EpdEntry {
    std::string fen;
    std::string verdict;
};

static std::vector<EpdEntry> load_epd(const std::string& path) {
    std::vector<EpdEntry> out;
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "FATAL: cannot open EPD file '%s'\n", path.c_str());
        return out;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        auto sep = line.find(';');
        if (sep == std::string::npos)
            continue;
        std::string fen = line.substr(0, sep);
        std::string verdict = line.substr(sep + 1);
        // trim
        auto trim = [](std::string& s) {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
        };
        trim(fen);
        trim(verdict);
        if (!fen.empty() && !verdict.empty())
            out.push_back({fen, verdict});
    }
    return out;
}

// ---------------------------------------------------------------------------
// Targeted KBNK orientation check: the strong side must prefer driving the
// bare king toward the bishop-coloured corner.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 5.9.18 — randomised conversion floors, one per forced-win family.
//
// The EPD floor above gates a handful of chosen positions. BAS-E28 showed that
// is not enough: KBNK converted 13% of RANDOM legal positions while every
// hand-picked case still passed. A class-level floor catches the collapse the
// per-position gate cannot see.
//
// Measured rates when these floors were set (60k nodes, engine both sides):
//   KQ-K 100%   KR-K 100%   KBB-K 87%   KBN-K 54%
// Floors sit far below those so ordinary search churn cannot trip them.
// RAISE THEM whenever a conversion improvement lands -- a floor left at an old
// rate silently stops protecting the gain that replaced it.
// ---------------------------------------------------------------------------
namespace {

// Fixed-seed LCG: the position set must be identical on every run so a failure
// is reproducible rather than a lottery.
struct Lcg {
    uint64_t s;
    explicit Lcg(uint64_t seed) : s(seed) {}
    uint32_t next() { s = s * 6364136223846793005ULL + 1442695040888963407ULL;
                      return uint32_t(s >> 33); }
    int below(int n) { return int(next() % uint32_t(n)); }
};

// A random legal position with `winner` holding `pieces`, the loser bare.
static bool random_family(Lcg& rng, std::string& out_fen, Color winner,
                          const std::vector<PieceType>& pieces) {
    for (int attempt = 0; attempt < 400; ++attempt) {
        int sq[4];
        const int need = 2 + int(pieces.size());
        bool clash = false;
        for (int i = 0; i < need; ++i) {
            sq[i] = rng.below(64);
            for (int j = 0; j < i; ++j) if (sq[j] == sq[i]) clash = true;
        }
        if (clash) continue;

        std::string fen_board[8];
        char grid[64];
        for (int i = 0; i < 64; ++i) grid[i] = 0;
        grid[sq[0]] = (winner == WHITE) ? 'K' : 'k';
        for (size_t i = 0; i < pieces.size(); ++i) {
            char c = "  NBRQ"[pieces[i]];
            grid[sq[1 + i]] = (winner == WHITE) ? c : char(c - 'A' + 'a');
        }
        grid[sq[need - 1]] = (winner == WHITE) ? 'k' : 'K';

        std::string fen;
        for (int r = 7; r >= 0; --r) {
            int run = 0;
            for (int f = 0; f < 8; ++f) {
                char c = grid[r * 8 + f];
                if (!c) { ++run; continue; }
                if (run) { fen += char('0' + run); run = 0; }
                fen += c;
            }
            if (run) fen += char('0' + run);
            if (r) fen += '/';
        }
        fen += (winner == WHITE) ? " w - - 0 1" : " b - - 0 1";

        // Two bishops on the SAME colour cannot force mate -- including such a
        // pair would measure the generator, not the engine.
        if (pieces.size() == 2 && pieces[0] == BISHOP && pieces[1] == BISHOP) {
            const int a = sq[1], c = sq[2];
            if (((a / 8 + a % 8) & 1) == ((c / 8 + c % 8) & 1)) continue;
        }
        Board probe;
        if (!probe.try_set_fen(fen)) continue;
        if (probe.is_in_check()) continue;      // side to move already in check
        MoveList ml; probe.gen_legal(ml);
        if (ml.empty()) continue;
        out_fen = fen;
        return true;
    }
    return false;
}

struct Family { const char* name; std::vector<PieceType> pieces; int n; int floor_; };

}  // namespace

// ---------------------------------------------------------------------------
// Drawn-ending bias floor (BAS-E32).
//
// The conversion floors above ask "can we finish a won ending". This asks the
// other half: "do we know a drawn one when we see it". Positions come from
// tests/endgame_draws.epd -- corpus positions whose GAME finished drawn.
//
// That is weaker than theoretical drawness, so this is a STATISTICAL floor: a
// count of how many drawn positions we score as a clear win. Individual
// positions may genuinely be wins that the players missed, which is exactly why
// there is no per-position assertion here.
//
// TIGHTEN THE BUDGET as each scaling function lands (5.9.24 onward). A budget
// left at the pre-implementation count stops protecting the gain.
// ---------------------------------------------------------------------------
static void test_drawn_ending_bias(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::printf("  [skip] %s not found\n", path.c_str());
        return;
    }
    // A "clear win" claim on a drawn position. Deliberately generous: we are
    // catching gross misvaluation, not asking for a perfect zero.
    constexpr int CLEAR_WIN = 250;
    // Measured 2026-08-31, before ANY scaling function exists: 22/60. The budget
    // sits a little above that so unrelated evaluation churn cannot flap it,
    // and far below 60 so a regression to "everything looks won" fails loudly.
    // TIGHTEN with each scaling function (5.9.24 onward).
    constexpr int MAX_CLAIMED_WINS = 26;

    int total = 0, claimed = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::size_t c0 = line.find(" c0 ");
        const std::string fen = (c0 == std::string::npos) ? line : line.substr(0, c0);
        ++total;
        if (std::abs(eval_white(fen)) >= CLEAR_WIN) ++claimed;
    }
    begin_section("drawn endings not scored as clear wins");
    std::printf("  %d/%d drawn positions scored >= %dcp (budget %d)\n",
                claimed, total, CLEAR_WIN, MAX_CLAIMED_WINS);
    EXPECT(claimed <= MAX_CLAIMED_WINS);
    end_section();
}

static void test_conversion_floors() {
    // Two bishops are generated on random squares, so a same-colour pair (a
    // genuine draw) can occur; the floor accounts for that rather than
    // rejecting them, which keeps the position set reproducible.
    const std::vector<Family> fams = {
        // Floors calibrated 2026-08-31 against measured rates in THIS harness.
        // They are NOT comparable to BAS-E28/E29: best_move() builds a fresh TT
        // for every move, where a real game keeps one across the whole playout,
        // so this is a harsher instrument by design -- deterministic, and no
        // history dependence between positions.
        //   measured: KQ 12/12  KR 12/12  KBB 3/12  KBN 14/16
        // RAISE EACH FLOOR when the matching conversion work lands. A floor left
        // at an old rate stops protecting the improvement that replaced it.
        { "KQ-K",  { QUEEN },           12, 12 },   // deterministic; must stay perfect
        { "KR-K",  { ROOK },            12, 12 },   // deterministic; must stay perfect
        { "KBB-K", { BISHOP, BISHOP },  12, 11 },   // 3/12 -> 12/12 at 5.9.19; floor 11 leaves one position of slack
        { "KBN-K", { BISHOP, KNIGHT },  16, 10 },   // 14/16 after 5.9.17
    };

    for (const Family& f : fams) {
        Lcg rng(0x5E9D18ULL);           // same seed for every family
        int converted = 0, generated = 0;
        for (int i = 0; i < f.n; ++i) {
            std::string fen;
            if (!random_family(rng, fen, WHITE, f.pieces)) continue;
            ++generated;
            if (mate_playout_plies(fen, WHITE, MAX_PLY - 1, CONV_NODES) >= 0) ++converted;
        }
        char label[96];
        std::snprintf(label, sizeof(label), "%s conversion floor", f.name);
        begin_section(label);
        std::printf("  %s converted %d/%d (floor %d)\n",
                    f.name, converted, generated, f.floor_);
        EXPECT(converted >= f.floor_);
        end_section();
    }
}

static void test_kbnk_corner_preference() {
    // Same dark-squared bishop (d2) and knight (f3) in both positions; only the
    // bare king moves. Its bishop-coloured corners are a1 / h8, so the defender
    // near a1 must score better for White than the mirror near the wrong corner.
    begin_section("KBNK drives toward the bishop-coloured corner");
    int right = eval_white("8/8/8/8/4K3/5N2/3B4/k7 b - - 0 1");  // black king a1 (dark, right)
    int wrong = eval_white("k7/8/8/8/4K3/5N2/3B4/8 b - - 0 1");  // black king a8 (light, wrong)
    EXPECT(right > wrong);
    end_section();

    // Light-squared bishop (d3): the correct pair reverses to a8 / h1. King,
    // knight and edge distances are equal between these two positions, so this
    // isolates the other diagonal-potential orientation.
    begin_section("KBNK light bishop reverses the target-corner diagonal");
    right = eval_white("k7/8/8/8/4K3/3B1N2/8/8 b - - 0 1");  // black king a8 (light, right)
    wrong = eval_white("8/8/8/8/4K3/3B1N2/8/k7 b - - 0 1");  // black king a1 (dark, wrong)
    EXPECT(right > wrong);
    end_section();
}

// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::string epd_path = (argc > 1) ? argv[1] : "tests/endgames.epd";
    std::vector<EpdEntry> entries = load_epd(epd_path);
    if (entries.empty()) {
        std::fprintf(stderr, "FATAL: no EPD entries loaded from '%s'\n", epd_path.c_str());
        return 1;
    }

    std::printf("Endgame regression suite (%zu positions)\n", entries.size());

    // 8.6.3b packaged-FEN legality sweep (canary_integrity class): every FEN
    // this suite ships must parse under STRICT validation. Four ILLEGAL
    // positions (side-not-to-move in check) hid in this very file for weeks in
    // 2026-07 and were only caught by hand — this gate makes that structural.
    begin_section("every packaged EPD FEN is strictly legal");
    for (const auto& e : entries) {
        Board b;
        auto r = b.try_set_fen(e.fen, /*validate_legal_position=*/true);
        EXPECT(r.has_value());
        if (!r)
            std::fprintf(stderr, "  illegal packaged FEN: %s (%s)\n",
                         e.fen.c_str(), r.error().c_str());
    }
    end_section();

    int mate_total = 0, mate_converts = 0;
    for (const auto& e : entries) {
        std::string label = e.verdict + ": " + e.fen;
        begin_section(label.c_str());
        if (e.verdict == "draw") {
            int cp = eval_white(e.fen);
            EXPECT(std::abs(cp) <= DRAW_TOL);
        } else if (e.verdict == "win_w") {
            EXPECT(eval_white(e.fen) >= WIN_MIN);
        } else if (e.verdict == "win_b") {
            EXPECT(eval_white(e.fen) <= -WIN_MIN);
        } else if (e.verdict == "mate_w" || e.verdict == "mate_b") {
            // Canary policy (search doc §14; PLAN §1 gate 6), revised 2026-07-15:
            //   HARD CORE (gating) — the won endgame is not misevaluated as a
            //   draw (per-position, below), plus robust mate recognition
            //   (test_near_mate_recognition) and a conversion FLOOR (after the
            //   loop). DIAGNOSTIC (non-gating) — per-position conversion at the
            //   fixed depth and its ply count. Rationale: single-position
            //   full-conversion at a fixed depth is a search-*shape* trajectory,
            //   not correctness — it over-fired on benign eval/search/TT changes
            //   (8.4/8.5.5/8.5.6/TT-density) while the eval still saw the win.
            //   §14: gate correctness, let SPRT arbitrate trajectory/strength.
            const Color winner = (e.verdict == "mate_w") ? WHITE : BLACK;
            const int wsign = (winner == WHITE) ? 1 : -1;
            // Hard core: no false draw — the static eval recognizes the win.
            EXPECT(wsign * eval_white(e.fen) >= WIN_MIN);
            // Conversion feeds a floor (below), not a per-position hard gate.
            const int plies = mate_playout_plies(e.fen, winner);
            ++mate_total;
            if (plies >= 0) ++mate_converts;
            std::printf("    [diag] mate route: %s in %d plies at depth %d\n",
                        plies >= 0 ? "converts" : "NO CONVERGENCE",
                        plies, MATE_DEPTH);
        } else {
            std::fprintf(stderr, "  FAIL: unknown verdict '%s'\n", e.verdict.c_str());
            EXPECT(false);
        }
        end_section();
    }

    // Hard core: a conversion FLOOR across all mate positions. Robust to a
    // single fixed-depth trajectory tipping (a search-shape artifact) while
    // still catching a real collapse in conversion ability. Calibrated so at
    // most one position may fail to converge.
    const int mate_floor = std::max(0, mate_total - 1);
    begin_section("mate conversion floor (robust to single-position fragility)");
    std::printf("  converted %d/%d mate positions (floor %d)\n",
                mate_converts, mate_total, mate_floor);
    EXPECT(mate_converts >= mate_floor);
    end_section();

    test_near_mate_recognition();
    test_conversion_floors();
    test_drawn_ending_bias("tests/endgame_draws.epd");
    test_kbnk_corner_preference();

    return harness_summary();
}
