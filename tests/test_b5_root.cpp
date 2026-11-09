/// Cluster 4, the root and the aspiration window: its arithmetic read directly,
/// its mechanisms driven through whole searches; and the ply cap, on both arms.
///
/// In Diag and Tune builds the counter sections read the search's `Diag` lines
/// through the info callback; release builds compile the counters out and skip
/// them.

#include "bench.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "parameters.h"
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

static constexpr bool kCounters =
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    true;
#else
    false;
#endif

// ---- The harness ------------------------------------------------------------

struct Run {
    SearchResult result;
    std::map<std::string, long long> diag;
    std::vector<std::string> lines;
};

static long long counter(const Run& run, const std::string& name) {
    const auto it = run.diag.find(name);
    return it == run.diag.end() ? 0 : it->second;
}

// A whole search, its options set by `limits`, reading the Diag lines.
static Run search_limits(const std::string& fen, SearchLimits lim) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    Run run;
    const std::regex token("([a-z_0-9]+)=(-?[0-9]+)");
    auto info = [&](const std::string& line) {
        run.lines.push_back(line);
        const std::string prefix = "info string diag kv ";
        if (line.compare(0, prefix.size(), prefix) != 0)
            return;
        for (std::sregex_iterator it(line.begin(), line.end(), token), end; it != end; ++it)
            run.diag[(*it)[1].str()] = std::stoll((*it)[2].str());
    };
    auto searcher = std::make_unique<Searcher>(tt, stop, info);
    Board b;
    b.set_fen(fen);
    lim.diag = true;
    run.result = searcher->search(b, lim);
    return run;
}

static Move legal_move(const Board& b, Square from, Square to) {
    MoveList legal;
    b.gen_legal(legal);
    for (Move m : legal)
        if (from_sq(m) == from && to_sq(m) == to)
            return m;
    return MOVE_NONE;
}

// ---- The ply cap --------------------------------------------------------------

// The position after `moves` from `fen`.
static Board after(const std::string& fen, const std::vector<std::pair<Square, Square>>& moves) {
    Board b;
    b.set_fen(fen);
    for (const auto& [from, to] : moves) {
        const Move m = legal_move(b, from, to);
        EXPECT(m != MOVE_NONE);
        b.make_move(m);
    }
    return b;
}

// One root move at depth 1 under a lowered cap.
static Run capped(const std::string& fen, Square from, Square to, int cap) {
    Board b;
    b.set_fen(fen);
    SearchLimits lim;
    lim.depth = 1;
    lim.ply_cap = cap;
    lim.root_moves = {legal_move(b, from, to)};
    return search_limits(fen, lim);
}

// Cap 1: the root's child is the node at the cap, in negamax.
static void test_the_cap_in_the_search() {
    begin_section("the ply cap in the search");
    Evaluator evaluator;
    const char* const fen = "k7/1pb5/2N5/8/8/8/8/1R2K3 w - - 0 1";

    // Ra1+: the side to move at the cap is in check, and the node reads 0 where
    // the static evaluation would read a clear score for it.
    const Run check = capped(fen, B1, A1, 1);
    EXPECT_EQ(check.result.score, 0);
    Board at_cap = after(fen, {{B1, A1}});
    EXPECT(at_cap.is_in_check());
    EXPECT(evaluator.evaluate(at_cap) != 0);

    // Rc1: not in check, the node reads its static evaluation.
    const Run quiet = capped(fen, B1, C1, 1);
    at_cap = after(fen, {{B1, C1}});
    EXPECT(!at_cap.is_in_check());
    const int quiet_eval = evaluator.evaluate(at_cap);
    EXPECT(quiet_eval != 0);
    EXPECT_EQ(quiet.result.score, -quiet_eval);

    if (kCounters) {
        EXPECT_EQ(counter(check, "ply_cap_hits"), 1);
        EXPECT_EQ(counter(check, "ply_cap_in_check"), 1);
        EXPECT_EQ(counter(quiet, "ply_cap_hits"), 1);
        EXPECT_EQ(counter(quiet, "ply_cap_in_check"), 0);
    }
    end_section();
}

// Cap 2: the root's child drops to quiescence, where its one evasion leads to
// the node at the cap. After Ra1+ Black's only move is Ba5, which checks the king
// on e1 and leaves the king on f1 alone.
static void test_the_cap_in_quiescence() {
    begin_section("the ply cap in quiescence");
    Evaluator evaluator;

    const char* const cross = "k7/1pb5/2N5/8/8/8/8/1R2K3 w - - 0 1";
    const Run check = capped(cross, B1, A1, 2);
    Board at_cap = after(cross, {{B1, A1}, {C7, A5}});
    EXPECT(at_cap.is_in_check());
    EXPECT(evaluator.evaluate(at_cap) != 0);
    EXPECT_EQ(check.result.score, 0);

    const char* const plain = "k7/1pb5/2N5/8/8/8/8/1R3K2 w - - 0 1";
    const Run quiet = capped(plain, B1, A1, 2);
    at_cap = after(plain, {{B1, A1}, {C7, A5}});
    EXPECT(!at_cap.is_in_check());
    const int quiet_eval = evaluator.evaluate(at_cap);
    EXPECT(quiet_eval != 0);
    EXPECT_EQ(quiet.result.score, quiet_eval);

    if (kCounters) {
        EXPECT_EQ(counter(check, "qs_ply_cap_hits"), 1);
        EXPECT_EQ(counter(check, "qs_ply_cap_in_check"), 1);
        EXPECT_EQ(counter(quiet, "qs_ply_cap_hits"), 1);
        EXPECT_EQ(counter(quiet, "qs_ply_cap_in_check"), 0);
    }
    end_section();
}

// The draw test precedes the cap: the fifty-move draw at the cap reads 0, in
// check and out of it, in a position whose evaluation is far from 0.
static void test_a_draw_at_the_cap() {
    begin_section("a draw at the ply cap");
    Evaluator evaluator;
    const char* const fen = "7k/8/8/8/8/8/8/R3K3 w - - 99 120";

    const Run quiet = capped(fen, A1, A2, 1);
    Board at_cap = after(fen, {{A1, A2}});
    EXPECT(!at_cap.is_in_check());
    EXPECT(at_cap.is_draw(1));
    EXPECT_EQ(quiet.result.score, 0);

    const Run check = capped(fen, A1, A8, 1);
    at_cap = after(fen, {{A1, A8}});
    EXPECT(at_cap.is_in_check());
    EXPECT(at_cap.is_draw(1));
    EXPECT_EQ(check.result.score, 0);

    // Off the fifty-move boundary the same quiet move reads the evaluation,
    // so the 0 above is the draw and not the position.
    const char* const fresh = "7k/8/8/8/8/8/8/R3K3 w - - 0 120";
    const Run live = capped(fresh, A1, A2, 1);
    at_cap = after(fresh, {{A1, A2}});
    const int live_eval = evaluator.evaluate(at_cap);
    EXPECT(live_eval != 0);
    EXPECT_EQ(live.result.score, -live_eval);

    if (kCounters) {
        EXPECT_EQ(counter(quiet, "ply_cap_hits"), 0);
        EXPECT_EQ(counter(check, "ply_cap_hits"), 0);
        EXPECT_EQ(counter(live, "ply_cap_hits"), 1);
    }
    end_section();
}

// A lowered cap truncates a real search and nothing past it reads a stale line:
// the result is a legal move with a line no longer than the cap.
static void test_a_search_under_a_low_cap() {
    begin_section("a whole search under a low cap");
    SearchLimits lim;
    lim.depth = 8;
    lim.ply_cap = 3;
    const Run run = search_limits("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", lim);
    EXPECT(run.result.bestmove != MOVE_NONE);
    EXPECT(run.result.pv.size() <= 3);
    if (kCounters)
        EXPECT(counter(run, "ply_cap_hits") + counter(run, "qs_ply_cap_hits") > 0);
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Cluster 4\n");
    std::printf("=========\n");

    test_the_cap_in_the_search();
    test_the_cap_in_quiescence();
    test_a_draw_at_the_cap();
    test_a_search_under_a_low_cap();

    return harness_summary();
}
