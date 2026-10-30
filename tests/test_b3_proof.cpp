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

#include <atomic>
#include <cstdio>
#include <map>
#include <memory>
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

static Run search(const std::string& fen, int depth, int ext_budget_cap = 0) {
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
        // Where the cap moved the tree, the counter saw it truncate.
        if (kCounters && kBudgetApplied && full.result.nodes != capped.result.nodes)
            EXPECT(counter(capped, "ext_budget_bound") > counter(full, "ext_budget_bound"));
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

    return harness_summary();
}
