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

// A cap far below the root depth truncates extensions on the B3_PROOF arm
// and changes the tree; the OFF arm has no budget and ignores it.
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
        if (kCounters && kB3Proof && full.result.nodes != capped.result.nodes)
            EXPECT(counter(capped, "ext_budget_bound") > counter(full, "ext_budget_bound"));
    }
    begin_section("budget: a forced cap moves the tree on the B3_PROOF arm only");
    if constexpr (kB3Proof)
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

    return harness_summary();
}
