/// Cluster 2 (the B3_PROOF arm): its arithmetic read directly, and its
/// mechanisms driven through whole searches. Every section binds both arms:
/// where the arms differ, the OFF arm's expectation is the legacy kernel's.
///
/// In Diag and Tune builds the counter sections also read the searches'
/// `Diag` lines through the info callback; release builds compile the
/// counters out and skip them.

#include "attacks.h"
#include "bench.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "search_b3.h"
#include "search_worker.h"
#include "test_harness.h"
#include "tt.h"
#include "zobrist.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <map>
#include <memory>
#include <random>
#include <regex>
#include <string>
#include <vector>

// ---- The per-line extension budget -------------------------------------

static void test_budget_arithmetic() {
    begin_section("budget: full, truncated and spent");
    bool truncated = true;
    EXPECT_EQ(b3::budget_extension(2, 0, 13, truncated), 2);
    EXPECT(!truncated);
    EXPECT_EQ(b3::budget_extension(3, 10, 13, truncated), 3);
    EXPECT(!truncated);
    EXPECT_EQ(b3::budget_extension(2, 12, 13, truncated), 1);
    EXPECT(truncated);
    EXPECT_EQ(b3::budget_extension(1, 13, 13, truncated), 0);
    EXPECT(truncated);
    EXPECT_EQ(b3::budget_extension(3, 15, 13, truncated), 0);
    EXPECT(truncated);
    end_section();

    begin_section("budget: no positive extension is touched");
    EXPECT_EQ(b3::budget_extension(0, 13, 13, truncated), 0);
    EXPECT(!truncated);
    EXPECT_EQ(b3::budget_extension(-3, 20, 13, truncated), -3);
    EXPECT(!truncated);
    end_section();
}

// ---- The previous-PV predicate -------------------------------------------

static void test_follow_pv_predicate() {
    const Move a = make_move(E2, E4), b = make_move(E7, E5), c = make_move(G1, F3);
    const Move pv[3] = {a, b, c};
    begin_section("follow_pv: the root always, a child only along the previous PV");
    EXPECT(b3::follow_pv(true, false, 0, MOVE_NONE, pv, 0));
    EXPECT(b3::follow_pv(false, true, 1, a, pv, 3));
    EXPECT(b3::follow_pv(false, true, 2, b, pv, 3));
    EXPECT(b3::follow_pv(false, true, 3, c, pv, 3));
    end_section();
    begin_section("follow_pv: false off the line, past its end, under a null move");
    EXPECT(!b3::follow_pv(false, false, 1, a, pv, 3));   // parent off the PV
    EXPECT(!b3::follow_pv(false, true, 1, b, pv, 3));    // another move
    EXPECT(!b3::follow_pv(false, true, 4, a, pv, 3));    // past the line
    EXPECT(!b3::follow_pv(false, true, 1, a, pv, 0));    // no previous PV
    EXPECT(!b3::follow_pv(false, true, 1, MOVE_NULL, pv, 3));
    end_section();
}

// ---- The singular extension's arithmetic ---------------------------------

static void test_singular_margin() {
    // The seeds: 126/63 = 2 per ply, plus 19/63 near a PV off it.
    begin_section("singular: the bound at depths 6, 10 and 20, with and without the PV term");
    EXPECT_EQ(b3::singular_beta(500, 6, false, 126, 19), 500 - 12);
    EXPECT_EQ(b3::singular_beta(500, 10, false, 126, 19), 500 - 20);
    EXPECT_EQ(b3::singular_beta(500, 20, false, 126, 19), 500 - 40);
    EXPECT_EQ(b3::singular_beta(500, 6, true, 126, 19), 500 - 145 * 6 / 63);
    EXPECT_EQ(b3::singular_beta(500, 10, true, 126, 19), 500 - 145 * 10 / 63);
    EXPECT_EQ(b3::singular_beta(500, 20, true, 126, 19), 500 - 145 * 20 / 63);
    // The donor's 17 (0.26 per ply) is reachable and divides as the donor's.
    EXPECT_EQ(b3::singular_beta(0, 20, false, 17, 0), -(17 * 20 / 63));
    EXPECT_EQ(b3::singular_beta(-300, 7, true, 13, 0), -300 - 13 * 7 / 63);
    end_section();
}

static void test_graded_extension() {
    begin_section("singular: the extension lies in [1, 3] and grows with the gap");
    bool ok = true;
    for (int dbl = -60; dbl <= 120; dbl += 7)
        for (int trp = -60; trp <= 200; trp += 11) {
            int prev = 0;
            // value runs from just below the bound to far below it.
            for (int gap = 1; gap <= 400; ++gap) {
                const int e = b3::graded_extension(100 - gap, 100, dbl, trp);
                ok = ok && e >= 1 && e <= 3 && e >= prev;
                prev = e;
            }
        }
    EXPECT(ok);
    EXPECT_EQ(b3::graded_extension(99, 100, 10, 20), 1);
    EXPECT_EQ(b3::graded_extension(85, 100, 10, 20), 2);
    EXPECT_EQ(b3::graded_extension(70, 100, 10, 20), 3);
    end_section();
}

static void test_multicut_and_negative() {
    begin_section("singular: multi-cut admission under both rules, never decisive");
    // The donor's rule reads the value, the alternative the bound.
    EXPECT(b3::multicut_admits(0, 120, 90, 100, false));
    EXPECT(!b3::multicut_admits(0, 95, 90, 100, false));
    EXPECT(!b3::multicut_admits(1, 120, 90, 100, false));
    EXPECT(b3::multicut_admits(1, 110, 105, 100, false));
    EXPECT(!b3::multicut_admits(0, 31000, 90, 100, true));
    EXPECT(!b3::multicut_admits(1, 31000, 105, 100, true));
    end_section();
    begin_section("singular: the negative extension at a fail-high TT score or a cut node");
    EXPECT(b3::negative_extension(100, 100, false));
    EXPECT(b3::negative_extension(50, 100, true));      // a cut node, TT score below beta
    EXPECT(!b3::negative_extension(50, 100, false));
    end_section();
}

static void test_shuffling_truth_table() {
    // e1-e2, then e2-e1 by the same side two plies later: the next e1-e2
    // shuffles.
    const Move m = make_move(E1, E2), prev2 = make_move(E2, E1), prev4 = make_move(E1, E2);
    begin_section("singular: is_shuffling's truth table");
    EXPECT(b3::is_shuffling(false, 10, 6, 20, m, prev2, prev4));
    EXPECT(!b3::is_shuffling(true, 10, 6, 20, m, prev2, prev4));    // capture stage
    EXPECT(!b3::is_shuffling(false, 9, 6, 20, m, prev2, prev4));    // rule 50 under 10
    EXPECT(!b3::is_shuffling(false, 10, 5, 20, m, prev2, prev4));   // a recent null move
    EXPECT(!b3::is_shuffling(false, 10, 6, 19, m, prev2, prev4));   // ply under 20
    EXPECT(!b3::is_shuffling(false, 10, 6, 20, make_move(E1, D1), make_move(D1, E1), prev4));
    EXPECT(!b3::is_shuffling(false, 10, 6, 20, m, prev2, make_move(E1, D2)));
    EXPECT(!b3::is_shuffling(false, 10, 6, 20, make_move(D2, E2), prev2, prev4));
    end_section();
}

static void test_seek_mate_gate() {
    begin_section("singular: seek_mate stands down at root depth >= 16 with |score| >= 564");
    EXPECT(b3::seek_mate(16, 564, 16, 564));
    EXPECT(b3::seek_mate(20, -564, 16, 564));
    EXPECT(b3::seek_mate(16, 31000, 16, 564));
    EXPECT(!b3::seek_mate(15, 31000, 16, 564));
    EXPECT(!b3::seek_mate(30, 563, 16, 564));
    EXPECT(!b3::seek_mate(30, -563, 16, 564));
    end_section();
}

// ---- Searches ------------------------------------------------------------

struct Run {
    SearchResult result;
    std::map<std::string, long long> diag;
};

static Run search(const std::string& fen, int depth, int ext_budget_cap = 0,
                  void (*set)(SearchParams&) = nullptr) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    Run run;
    const std::regex token("([a-z_0-9]+)=(-?[0-9]+)");
    auto info = [&](const std::string& line) {
        const std::string prefix = "info string diag kv ";
        if (line.compare(0, prefix.size(), prefix) != 0)
            return;
        for (std::sregex_iterator it(line.begin(), line.end(), token), end; it != end; ++it)
            run.diag[(*it)[1].str()] = std::stoll((*it)[2].str());
    };
    auto searcher = std::make_unique<Searcher>(tt, stop, info);
    Board b;
    b.set_fen(fen);
    SearchLimits lim;
    lim.depth = depth;
    lim.diag = true;
    lim.ext_budget_cap = ext_budget_cap;
    if (set)
        set(lim.params);
    run.result = searcher->search(b, lim);
    return run;
}

// A counter the search printed; 0 when it printed none.
static long long counter(const Run& run, const std::string& name) {
    const auto it = run.diag.find(name);
    return it == run.diag.end() ? 0 : it->second;
}

static constexpr bool kCounters =
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    true;
#else
    false;
#endif

// The arm whose singular extension the budget bounds: the B3_PROOF arm's
// graded extension.
static constexpr bool kBudgetApplied = kB3Proof;

// A cap far below the root depth truncates extensions where the budget is
// applied and changes the tree; elsewhere the cap is ignored.
static void test_budget_bound_is_live() {
    int changed = 0;
    int legal = 0;
    for (int i = 0; i < 8; ++i) {
        const std::string fen(bench_fens()[static_cast<size_t>(i)]);
        const Run full = search(fen, 10);
        const Run capped = search(fen, 10, 1);
        changed += full.result.nodes != capped.result.nodes;
        Board b;
        b.set_fen(fen);
        legal += is_legal_move_on_board(b, capped.result.bestmove);
        // Where the cap moved the tree, the counter saw it truncate. (The
        // two runs are different trees, so their counts are not compared.)
        if (kCounters && kBudgetApplied && full.result.nodes != capped.result.nodes)
            EXPECT(counter(capped, "ext_budget_bound") > 0);
    }
    begin_section("budget: a forced cap moves the tree where the budget is applied");
    if constexpr (kBudgetApplied)
        EXPECT(changed > 0);
    else
        EXPECT_EQ(changed, 0);
    EXPECT_EQ(legal, 8);
    end_section();
}

// The previous PV is followed from the second iteration on, the root alone
// at the first; the counter reads it in Diag and Tune builds.
static void test_follow_pv_in_search() {
    if constexpr (!kCounters)
        return;
    const std::string fen(bench_fens()[0]);
    const Run one = search(fen, 1);
    const Run four = search(fen, 4);
    begin_section("follow_pv: the root alone at iteration 1, the line after it");
    if constexpr (kB3Proof) {
        EXPECT_EQ(counter(one, "follow_pv_nodes"), 1);
        EXPECT(counter(four, "follow_pv_nodes") > 4);
        EXPECT(counter(four, "follow_pv_nodes") < counter(four, "interior_nodes"));
    } else {
        EXPECT_EQ(counter(one, "follow_pv_nodes"), 0);
        EXPECT_EQ(counter(four, "follow_pv_nodes"), 0);
    }
    end_section();
}

// ---- Correction and the fresh tables (ticket 7) ----------------------------

// The donor's fills land in every table they name; the default clear leaves
// every entry at zero.
static void test_history_fill_values() {
    auto t = std::make_unique<CoreHistoryTables>();
    begin_section("fills: clear(true) writes the donor's values, clear() zero");
    t->clear(true);
    EXPECT_EQ(int(t->main.entry[WHITE][0][1]), -5);
    EXPECT_EQ(int(t->capture.entry[0][0][0]), -742);
    EXPECT_EQ(int(t->continuation->row[1][1][KNIGHT][20].entry[QUEEN][63]), -586);
    EXPECT_EQ(int(t->pawn->bucket[0].entry[0][0]), -1338);
    EXPECT_EQ(int(t->pawn_corr[BLACK][7]), -5);
    EXPECT_EQ(int(t->minor_corr[WHITE][9]), -5);
    EXPECT_EQ(int(t->nonpawn_corr[WHITE][BLACK][11]), -5);
    EXPECT_EQ(int(t->cont_corr->row[ROOK][5].entry[PAWN][40]), 5);
    EXPECT_EQ(int(t->low_ply.entry[0][0][0]), 0);
    EXPECT_EQ(t->tt_move.value, 0);
    t->clear();
    EXPECT_EQ(int(t->main.entry[WHITE][0][1]), 0);
    EXPECT_EQ(int(t->capture.entry[0][0][0]), 0);
    EXPECT_EQ(int(t->continuation->row[1][1][KNIGHT][20].entry[QUEEN][63]), 0);
    EXPECT_EQ(int(t->pawn->bucket[0].entry[0][0]), 0);
    EXPECT_EQ(int(t->pawn_corr[BLACK][7]), 0);
    EXPECT_EQ(int(t->cont_corr->row[ROOK][5].entry[PAWN][40]), 0);
    end_section();
}

// Each switch moves the tree on the B3_PROOF arm and nothing on the OFF arm:
// the fills from the first search, decisive training in a won ending, and
// training under exclusion in a middlegame where singular searches run.
static void test_correction_switches_live() {
    const std::string kqk = "8/8/8/4k3/8/8/8/4K2Q w - - 0 1";
    const std::string mid(bench_fens()[1]);
    const long long fills0 = search(mid, 10).result.nodes;
    const long long fills1 = search(mid, 10, 0, [](SearchParams& p) {
        p.core_history_fills = 1; }).result.nodes;
    const long long dec1 = search(kqk, 14).result.nodes;
    const long long dec0 = search(kqk, 14, 0, [](SearchParams& p) {
        p.b3_corr_decisive = 0; }).result.nodes;
    const long long exc1 = search(mid, 12).result.nodes;
    const long long exc0 = search(mid, 12, 0, [](SearchParams& p) {
        p.b3_corr_excluded = 0; }).result.nodes;
    begin_section("correction: CoreHistoryFills, B3CorrDecisive, B3CorrExcluded move the ON arm only");
    if constexpr (kB3Proof) {
        EXPECT(fills0 != fills1);
        EXPECT(dec0 != dec1);
        EXPECT(exc0 != exc1);
    } else {
        EXPECT_EQ(fills0, fills1);
        EXPECT_EQ(dec0, dec1);
        EXPECT_EQ(exc0, exc1);
    }
    end_section();
}

// Amendment 4's guard: an exclusion node whose raw static eval is 0 (the
// wrong-bishop corner the recogniser draws) trains no correction, so the
// ON arm keeps the draw with exclusion training on (the unguarded tree read
// 259 here) and off; the OFF arm's tree does not see the switch.
static void test_excluded_training_spares_recognised_draws() {
    const std::string fen = "7k/8/6KP/8/8/8/8/5B2 w - - 0 1";
    auto run = [&](int excluded) {
        TranspositionTable tt(8);
        std::atomic_bool stop{false};
        auto searcher = std::make_unique<Searcher>(tt, stop);
        Board b;
        b.set_fen(fen);
        SearchLimits lim;
        lim.depth = 12;
        lim.nodes = 200000;
        lim.params.b3_corr_excluded = excluded;
        return searcher->search(b, lim);
    };
    const SearchResult on = run(1);
    const SearchResult off = run(0);
    begin_section("correction: exclusion training spares a recognised draw (KBP-K wrong bishop, depth 12)");
    EXPECT_EQ(on.depth, 12);
    EXPECT_EQ(off.depth, 12);
    if constexpr (kB3Proof) {
        EXPECT(std::abs(on.score) <= 75);
        EXPECT(std::abs(off.score) <= 75);
    } else {
        EXPECT_EQ(on.nodes, off.nodes);
        EXPECT_EQ(on.score, off.score);
    }
    end_section();
}

// ---- Upcoming repetition (the cuckoo tables) ----------------------------

// Every move of a non-pawn piece between two squares on an empty board, by
// the attack tables, is in the cuckoo tables under its key, in one of its
// two slots, as the move from the lower square.
static void test_cuckoo_tables_complete() {
    int found = 0, missing = 0;
    for (int c = 0; c < NCOLORS; c++)
        for (int pt = KNIGHT; pt <= KING; pt++)
            for (int s1 = 0; s1 < SQUARE_NB; s1++) {
                const Square sq = Square(s1);
                const Bitboard reach = pt == KNIGHT ? KnightAttacks[sq]
                                     : pt == BISHOP ? bishop_attacks(sq, 0)
                                     : pt == ROOK   ? rook_attacks(sq, 0)
                                     : pt == QUEEN  ? queen_attacks(sq, 0)
                                                    : KingAttacks[sq];
                for (int s2 = s1 + 1; s2 < SQUARE_NB; s2++) {
                    if (!(reach & sq_bb(Square(s2))))
                        continue;
                    const Key key = Zobrist::PieceKeys[c][pt][s1]
                                  ^ Zobrist::PieceKeys[c][pt][s2] ^ Zobrist::SideKey;
                    const Move m = make_move(sq, Square(s2));
                    const int h1 = Zobrist::cuckoo_h1(key), h2 = Zobrist::cuckoo_h2(key);
                    if ((Zobrist::Cuckoo[h1] == key && Zobrist::CuckooMove[h1] == m)
                        || (Zobrist::Cuckoo[h2] == key && Zobrist::CuckooMove[h2] == m))
                        ++found;
                    else
                        ++missing;
                }
            }
    int filled = 0;
    for (int i = 0; i < Zobrist::CUCKOO_SIZE; i++)
        filled += Zobrist::CuckooMove[i] != MOVE_NONE;
    begin_section("cuckoo: every reversible move's key is found");
    EXPECT_EQ(found, Zobrist::CUCKOO_MOVES);
    EXPECT_EQ(missing, 0);
    EXPECT_EQ(filled, Zobrist::CUCKOO_MOVES);
    end_section();
}

// The slow reference: make every legal reversible move of the side to move
// (a non-pawn piece to an empty square, no castling) and look for its key
// i plies back for odd i in [3, end], end = min(rule50, plies since a null
// move, the history); within search_ply a draw, otherwise only a position
// already met again at least four plies before it.
static bool upcoming_repetition_slow(Board& b, const std::vector<Key>& keys, int search_ply) {
    const int n = static_cast<int>(keys.size()) - 1;
    const int end = std::min({b.rule50_count(), b.plies_since_null(), n});
    auto back = [&](int d) { return keys[static_cast<size_t>(n - d)]; };
    MoveList legal;
    b.gen_legal(legal);
    for (Move m : legal) {
        const Piece pc = b.piece_on(from_sq(m));
        if (move_type(m) != NORMAL || type_of(pc) == PAWN || b.piece_on(to_sq(m)) != NO_PIECE)
            continue;
        b.make_move(m);
        const Key reached = b.position_key();
        b.unmake_move(m);
        for (int i = 3; i <= end; i += 2) {
            if (back(i) != reached)
                continue;
            if (search_ply > i)
                return true;
            for (int k = i + 4; k <= end; k += 2)
                if (back(k) == back(i))
                    return true;
        }
    }
    return false;
}

// Random walks that shuffle pieces back and forth, so repetitions are
// common: the cuckoo test never reports a repetition the reference does not
// find, at search plies inside and outside the walk's reach.
static void test_cuckoo_no_false_positive() {
    const char* seeds[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "4k3/8/8/3q4/8/2N5/1B6/4K2R w K - 0 1",
        "6k1/5pp1/7p/8/8/2B5/5PPP/3R2K1 w - - 0 1",
    };
    std::mt19937_64 rng(0xC0C0C0ULL);
    int positions = 0, positives = 0, false_positives = 0, false_negatives = 0;
    for (int w = 0; positions < 10000; ++w) {
        Board b;
        b.set_fen(seeds[w % 5]);
        std::vector<Key> keys{b.position_key()};
        std::vector<Move> played;
        const int plies = 4 + static_cast<int>(rng() % 36);
        for (int p = 0; p < plies && positions < 10000; ++p) {
            MoveList legal;
            b.gen_legal(legal);
            if (legal.empty())
                break;
            // Prefer undoing this side's move from two plies back, then a
            // reversible move, then anything.
            Move pick = MOVE_NONE;
            if (played.size() >= 2 && rng() % 2 == 0) {
                const Move old = played[played.size() - 2];
                const Move back = make_move(to_sq(old), from_sq(old));
                for (Move m : legal)
                    if (m == back)
                        pick = m;
            }
            if (pick == MOVE_NONE && rng() % 5 != 0) {
                MoveList rev;
                for (Move m : legal)
                    if (move_type(m) == NORMAL && type_of(b.piece_on(from_sq(m))) != PAWN
                        && b.piece_on(to_sq(m)) == NO_PIECE)
                        rev.push(m);
                if (!rev.empty())
                    pick = rev[static_cast<int>(rng() % static_cast<uint64_t>(rev.size()))];
            }
            if (pick == MOVE_NONE)
                pick = legal[static_cast<int>(rng() % static_cast<uint64_t>(legal.size()))];
            b.make_move(pick);
            played.push_back(pick);
            keys.push_back(b.position_key());
            ++positions;
            for (int search_ply : {0, 2, 5, 64}) {
                const bool fast = b.upcoming_repetition(search_ply);
                const bool slow = upcoming_repetition_slow(b, keys, search_ply);
                positives += fast;
                false_positives += fast && !slow;
                false_negatives += slow && !fast;
            }
        }
    }
    std::printf("%d positions, %d positive reads, %d missed by the cuckoo ", positions,
                positives, false_negatives);
    begin_section("cuckoo: no false positive against the slow reference (10,000 positions)");
    EXPECT_EQ(positions, 10000);
    EXPECT(positives > 500);
    EXPECT_EQ(false_positives, 0);
    end_section();
}

// The terminal test (BAS-S12's trigger, amendment 5): White is lost by
// material and its king and knight hold Black's king to h8/h7. After the
// pre-root list Black's only move h8h7 lets White play f8f7 to a position
// seen twice. The root, searched under a null window at the draw, asks
// whether h8h7 beats a draw: White's node gets (-1, 0), and the cutoff
// returns the draw there, so the ON arm's root fails low at exactly 0;
// without it the root fails high on the win. One below, White's alpha is 0
// and the switch changes nothing.
static SearchResult search_windowed(const Board& b, int depth, int root_alpha, int root_beta,
                                    int cuckoo) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    auto searcher = std::make_unique<Searcher>(tt, stop);
    SearchLimits lim;
    lim.depth = depth;
    lim.root_alpha = root_alpha;
    lim.root_beta = root_beta;
    lim.root_moves = {make_move(H8, H7)};
    lim.params.b3_cuckoo = cuckoo;
    return searcher->search(b, lim);
}

static void test_cuckoo_terminal() {
    Board b;
    b.set_fen("rb6/p1p2K1k/p1p1p3/p1p1p3/P1P1P1N1/8/8/8 b - - 0 1");
    for (Move m : {make_move(H7, H8), make_move(F7, F8), make_move(H8, H7), make_move(F8, F7),
                   make_move(H7, H8), make_move(F7, F8)})
        b.make_move(m);
    Board child = b;
    child.make_move(make_move(H8, H7));
    begin_section("cuckoo: the root's null window at the draw reads 0 through the cutoff (depths 1-3)");
    EXPECT(child.upcoming_repetition(1));
    for (int depth = 1; depth <= 3; ++depth) {
        const SearchResult at_draw = search_windowed(b, depth, 0, 1, 1);
        const SearchResult at_draw_off = search_windowed(b, depth, 0, 1, 0);
        const SearchResult below = search_windowed(b, depth, -1, 0, 1);
        const SearchResult below_off = search_windowed(b, depth, -1, 0, 0);
        if constexpr (kB3Proof)
            EXPECT_EQ(at_draw.score, 0);
        else
            EXPECT_EQ(at_draw.nodes, at_draw_off.nodes);
        EXPECT(at_draw_off.score > 0);
        EXPECT_EQ(below.score, below_off.score);
        EXPECT_EQ(below.nodes, below_off.nodes);
    }
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Cluster 2 (%s arm)\n", kB3Proof ? "B3_PROOF" : "OFF");
    std::printf("==================\n");

    test_budget_arithmetic();
    test_follow_pv_predicate();
    test_budget_bound_is_live();
    test_follow_pv_in_search();
    test_singular_margin();
    test_graded_extension();
    test_multicut_and_negative();
    test_shuffling_truth_table();
    test_seek_mate_gate();
    test_history_fill_values();
    test_correction_switches_live();
    test_excluded_training_spares_recognised_draws();
    test_cuckoo_tables_complete();
    test_cuckoo_no_false_positive();
    test_cuckoo_terminal();

    return harness_summary();
}
