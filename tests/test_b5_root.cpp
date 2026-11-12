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
#include "search_b5.h"
#include "search_worker.h"
#include "test_harness.h"
#include "tt.h"
#include "zobrist.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
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

// ---- The record ---------------------------------------------------------------

static void test_the_record_arithmetic() {
    begin_section("the record's weights and blends");
    // The share of this search in the move's effort, in 32nds, between 12 and 24.
    EXPECT_EQ(b5::ema_weights(1000, 0).average, 24);       // the first search: 64000/2003, clamped
    EXPECT_EQ(b5::ema_weights(1, 1000).average, 12);       // a sliver: clamped from below
    EXPECT_EQ(b5::ema_weights(1000, 1000).average, 12);    // 64000/5000
    EXPECT_EQ(b5::ema_weights(1000, 500).average, 18);     // 64000/3500
    EXPECT_EQ(b5::ema_weights(1000, 400).average, 20);     // 64000/3200
    // The mean square's weight stops at 16.
    EXPECT_EQ(b5::ema_weights(1000, 500).mean_sq, 16);
    EXPECT_EQ(b5::ema_weights(1000, 1000).mean_sq, 12);

    EXPECT_EQ(b5::blend_average(kRootScoreNone, -37, 24), -37);
    EXPECT_EQ(b5::blend_average(0, 100, 24), 75);
    EXPECT_EQ(b5::blend_average(0, -100, 24), -75);
    EXPECT_EQ(b5::blend_average(100, 0, 12), 62);          // 2000/32 truncated

    EXPECT_EQ(b5::blend_mean_sq(kRootMeanSqNone, 100, 16), 10000);
    EXPECT_EQ(b5::blend_mean_sq(kRootMeanSqNone, -100, 16), -10000);
    EXPECT_EQ(b5::blend_mean_sq(10000, -100, 16), 0);
    EXPECT_EQ(b5::blend_mean_sq(0, 20000, 16), 200000000);  // no overflow in the won tier

    // By score, then the last score; the none score last, in the order it had.
    std::vector<RootMove> moves(4);
    moves[0].move = Move(1);
    moves[1].move = Move(2);
    moves[1].score = 5;
    moves[1].previous_score = 1;
    moves[2].move = Move(3);
    moves[3].move = Move(4);
    moves[3].score = 5;
    moves[3].previous_score = 2;
    std::stable_sort(moves.begin(), moves.end(), b5::root_move_before);
    EXPECT(moves[0].move == Move(4));
    EXPECT(moves[1].move == Move(2));
    EXPECT(moves[2].move == Move(1));
    EXPECT(moves[3].move == Move(3));
    end_section();
}

// The arm's root takes its first move from its own line once an iteration
// completed, whatever the table holds: rewriting the root entry's move at every
// depth line, its score and bound kept, leaves the arm's tree as it was, and
// moves the OFF arm's.
static int64_t nodes_with_a_foreign_root_move(const std::string& fen, int depth, bool rewrite) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    Board b;
    b.set_fen(fen);
    const Key key = b.position_key();
    MoveList legal;
    b.gen_legal(legal);
    auto info = [&](const std::string& line) {
        if (!rewrite || line.rfind("info depth", 0) != 0)
            return;
        TTEntry entry{};
        if (!tt.probe_copy(key, entry))
            return;
        // The legal move furthest down the generation order that is not the entry's.
        Move other = MOVE_NONE;
        for (Move m : legal)
            if (m != move_from_tt(entry.move16))
                other = m;
        tt.store(key, entry.depth, TranspositionTable::score_from_tt(entry.score, 0, 0), entry.bound(),
                 other, 0, entry.static_eval, entry.is_pv());
    };
    auto searcher = std::make_unique<Searcher>(tt, stop, info);
    SearchLimits lim;
    lim.depth = depth;
    return searcher->search(b, lim).nodes;
}

static void test_the_root_move_comes_from_the_record() {
    begin_section("the root's first move comes from the record");
    const char* const fen = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3";
    const int64_t plain = nodes_with_a_foreign_root_move(fen, 10, false);
    const int64_t foreign = nodes_with_a_foreign_root_move(fen, 10, true);
    EXPECT_EQ(foreign, plain);
    end_section();
}

// ---- The window ---------------------------------------------------------------

static void test_the_window_arithmetic() {
    begin_section("the window's width, growth and bounds");
    // The first window at the seeds: a level score, and half a pawn to ten pawns.
    const auto first_delta = [](int64_t score, int thread = 0) {
        return b5::width_delta(b5::first_width(1410, thread, score * score, 2874));
    };
    EXPECT_EQ(b5::first_width(1410, 0, 0, 2874), 1410);
    EXPECT_EQ(first_delta(0), 2);
    EXPECT_EQ(first_delta(50), 3);
    EXPECT_EQ(first_delta(100), 5);
    EXPECT_EQ(first_delta(150), 10);
    EXPECT_EQ(first_delta(300), 33);
    EXPECT_EQ(first_delta(1000), 350);
    EXPECT_EQ(b5::first_width(1410, 0, -100 * 100, 2874), b5::first_width(1410, 0, 100 * 100, 2874));
    // After failures at a level score.
    int64_t level = b5::first_width(1410, 0, 0, 2874);
    for (const int expected : {2, 2, 3, 4, 5, 7, 10, 13, 18, 24}) {
        EXPECT_EQ(b5::width_delta(level), expected);
        level = b5::grow_width(level, 47);
    }
    // The thread's offset, in thousandths, wrapping at the eighth.
    const int helpers[] = {2, 2, 3, 3, 3, 4, 4};
    for (int thread = 1; thread <= 7; ++thread)
        EXPECT_EQ(first_delta(0, thread), helpers[thread - 1]);
    EXPECT_EQ(b5::first_width(1410, 3, 0, 2874), 1410 + 3 * 282);
    EXPECT_EQ(first_delta(0, 8), first_delta(0));
    // The cap, and the won tier and an empty record, which are open.
    EXPECT_EQ(b5::first_width(40000, 0, 0, 12000), 40000);
    EXPECT_EQ(b5::first_width(500, 0, 20000LL * 20000, 1000), b5::kMaxWindowWidth);
    EXPECT_EQ(b5::first_width(500, 0, kRootMeanSqNone, 12000), b5::kMaxWindowWidth);
    EXPECT_EQ(b5::grow_width(b5::kMaxWindowWidth, 96), b5::kMaxWindowWidth);
    EXPECT_EQ(b5::grow_width(b5::kMaxWindowWidth - 1, 96), b5::kMaxWindowWidth);
    EXPECT_EQ(b5::width_delta(b5::kMaxWindowWidth), b5::kMaxWindowDelta);
    const b5::RootWindow won = b5::first_window(20000, b5::width_delta(b5::first_width(1410, 0, 20000LL * 20000, 2874)));
    EXPECT_EQ(won.alpha, -INF_SCORE);
    EXPECT_EQ(won.beta, INF_SCORE);
    const b5::RootWindow unseen = b5::first_window(kRootScoreNone, b5::width_delta(b5::first_width(1410, 0, kRootMeanSqNone, 2874)));
    EXPECT_EQ(unseen.alpha, -INF_SCORE);
    EXPECT_EQ(unseen.beta, INF_SCORE);

    const b5::RootWindow first = b5::first_window(50, 5);
    EXPECT_EQ(first.alpha, 45);
    EXPECT_EQ(first.beta, 55);
    // A fail low pulls beta to the old alpha and alpha below the score.
    const b5::RootWindow low = b5::after_fail_low(first, 30, 7);
    EXPECT_EQ(low.alpha, 23);
    EXPECT_EQ(low.beta, 45);
    EXPECT_EQ(b5::after_fail_low(first, -INF_SCORE + 3, 7).alpha, -INF_SCORE);
    // A fail high raises alpha to the old beta less the width, and beta above the score.
    const b5::RootWindow high = b5::after_fail_high(first, 80, 7);
    EXPECT_EQ(high.alpha, 48);
    EXPECT_EQ(high.beta, 87);
    EXPECT_EQ(b5::after_fail_high(first, 80, 100).alpha, 45);   // alpha never falls
    EXPECT_EQ(b5::after_fail_high(first, INF_SCORE - 3, 7).beta, INF_SCORE);

    // LMR's term: at a PV node 577 per unit of the root's width, whatever the
    // node's window; at a non-PV node, whose window is one unit, the donor's unit
    // in ours.
    EXPECT_EQ(b5::lmr_window_term(true, 38, 38), 577);
    EXPECT_EQ(b5::lmr_window_term(true, 2, 2), 577);
    EXPECT_EQ(b5::lmr_window_term(true, 1, 1), 577);
    EXPECT_EQ(b5::lmr_window_term(true, 1, 4), 144);
    EXPECT_EQ(b5::lmr_window_term(false, 1, 1), 163);
    EXPECT_EQ(b5::lmr_window_term(false, 1, 2), 81);
    EXPECT_EQ(b5::lmr_window_term(false, 1, 38), 4);
    end_section();
}

// Nothing but the width's growth ends the loop, so on the whole registered surface
// it has to keep growing: strictly at every failure until it holds the cap, within
// a bounded number of failures, its whole-unit distance never falling and never
// standing still for long. Corners and a grid between them, at a level score.
static void test_the_width_grows_everywhere_on_the_ranges() {
    begin_section("the width grows everywhere on the registered ranges");
    const int bases[] = {500, 501, 750, 1000, 1410, 2000, 2820, 4230, 5640, 9870, 19000, 30000, 39999, 40000};
    const int divisors[] = {1000, 2874, 12000};
    int points = 0, worst_failures = 0, worst_repeat = 0, corner_failures = 0, corner_repeat = 0;
    bool strict = true, never_falls = true;
    for (const int base : bases) {
        for (int growth = 32; growth <= 96; ++growth) {
            for (const int divisor : divisors) {
                for (int thread = 0; thread <= 8; ++thread) {
                    ++points;
                    int64_t width = b5::first_width(base, thread, 0, divisor);
                    int failures = 0, repeat = 1, longest = 1;
                    while (width < b5::kMaxWindowWidth && failures < 1000) {
                        const int64_t next = b5::grow_width(width, growth);
                        strict = strict && next > width;
                        const int delta = b5::width_delta(width), next_delta = b5::width_delta(next);
                        never_falls = never_falls && next_delta >= delta;
                        repeat = next_delta == delta ? repeat + 1 : 1;
                        longest = std::max(longest, repeat);
                        width = next;
                        ++failures;
                    }
                    worst_failures = std::max(worst_failures, failures);
                    worst_repeat = std::max(worst_repeat, longest);
                    if (base == 500 && growth == 32 && thread == 0) {
                        corner_failures = failures;
                        corner_repeat = longest;
                    }
                }
            }
        }
    }
    EXPECT_EQ(points, 14 * 65 * 3 * 9);
    EXPECT(strict);
    EXPECT(never_falls);
    EXPECT(worst_failures <= 60);
    EXPECT(worst_repeat <= 4);
    // The worst corner, base 500 at growth 32: 53 failures, and one unit four times.
    EXPECT_EQ(corner_failures, 53);
    EXPECT_EQ(worst_repeat, 4);
    EXPECT_EQ(corner_repeat, 4);
    end_section();
}

static void test_the_adjusted_depth() {
    begin_section("the fail-high depth reduction");
    // Off, the iteration's depth whatever has failed.
    EXPECT_EQ(b5::adjusted_depth(14, 0, false), 14);
    EXPECT_EQ(b5::adjusted_depth(14, 5, false), 14);
    // On, a ply less per fail high, at least one ply.
    EXPECT_EQ(b5::adjusted_depth(14, 0, true), 14);
    EXPECT_EQ(b5::adjusted_depth(14, 1, true), 13);
    EXPECT_EQ(b5::adjusted_depth(14, 5, true), 9);
    EXPECT_EQ(b5::adjusted_depth(5, 4, true), 1);
    EXPECT_EQ(b5::adjusted_depth(5, 5, true), 1);
    EXPECT_EQ(b5::adjusted_depth(5, 9, true), 1);
    end_section();
}

// ---- The negative-reduction cap ----------------------------------------------

static void test_the_ceiling() {
    begin_section("the negative-reduction ceiling");
    // Unbounded at 0, whatever the line holds.
    EXPECT_EQ(b5::negext_ceiling(true, 10, 0, 0), 12);
    EXPECT_EQ(b5::negext_ceiling(true, 10, 7, 0), 12);
    // A limit stops the extra plies once the line holds that many.
    EXPECT_EQ(b5::negext_ceiling(true, 10, 0, 1), 12);
    EXPECT_EQ(b5::negext_ceiling(true, 10, 1, 1), 10);
    EXPECT_EQ(b5::negext_ceiling(true, 10, 1, 2), 12);
    EXPECT_EQ(b5::negext_ceiling(true, 10, 2, 2), 10);
    EXPECT_EQ(b5::negext_ceiling(true, 10, 5, 2), 10);
    // Negative reductions switched off is the full depth at any setting.
    EXPECT_EQ(b5::negext_ceiling(false, 10, 0, 0), 10);
    EXPECT_EQ(b5::negext_ceiling(false, 10, 0, 2), 10);
    end_section();
}

static std::vector<std::string> suite_positions(const char* path, size_t count) {
    std::vector<std::string> all;
    std::ifstream in(path);
    for (std::string line; std::getline(in, line);) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream fields(line);
        std::string board, side, castling, ep;
        if (fields >> board >> side >> castling >> ep)
            all.push_back(board + " " + side + " " + castling + " " + ep + " 0 1");
    }
    // Spread over the suite, which is ordered.
    std::vector<std::string> picked;
    for (size_t k = 0; k < count && !all.empty(); ++k)
        picked.push_back(all[k * all.size() / count]);
    return picked;
}

// Every node starts clean, whichever way it was entered: the node checks are live
// on a Diag build and abort on a stale line, so finishing is the proof, and the
// counters show the entry paths were walked.
static void test_every_child_starts_clean(const char* suite) {
    begin_section("a line's negative reductions, on every entry path");
    if (kCounters) {
        const std::vector<std::string> positions = suite_positions(suite, 20);
        EXPECT_EQ(positions.size(), size_t(20));
        int64_t nulls = 0, probcuts = 0, singulars = 0, drops = 0, extended = 0;
        for (const std::string& fen : positions) {
            SearchLimits lim;
            lim.depth = 8;
            const Run run = search_limits(fen, lim);
            nulls += counter(run, "null_tries");
            probcuts += counter(run, "probcut_tries");
            singulars += counter(run, "sing_tries");
            drops += counter(run, "qs_nodes");
            extended += counter(run, "negext_total");
        }
        EXPECT(nulls > 0);
        EXPECT(probcuts > 0);
        EXPECT(singulars > 0);
        EXPECT(drops > 0);
        EXPECT(extended > 0);
    }
    end_section();
}

// With a limit, no line goes past it.
static void test_the_limit_holds_on_the_line(const char* suite) {
    begin_section("a line holds no more than its limit");
    if (kCounters) {
        int64_t unlimited = 0, at_one = 0, chained_at_one = 0, at_two = 0, chained_at_two = 0;
        for (const std::string& fen : suite_positions(suite, 20)) {
            SearchLimits lim;
            lim.depth = 10;
            lim.params.b5_neg_ext_max = 0;
            const Run open = search_limits(fen, lim);
            unlimited += counter(open, "negext_chain1") + counter(open, "negext_chain2p");
            lim.params.b5_neg_ext_max = 1;
            const Run one = search_limits(fen, lim);
            at_one += counter(one, "negext_total");
            chained_at_one += counter(one, "negext_chain1") + counter(one, "negext_chain2p");
            lim.params.b5_neg_ext_max = 2;
            const Run two = search_limits(fen, lim);
            at_two += counter(two, "negext_chain1");
            chained_at_two += counter(two, "negext_chain2p");
        }
        EXPECT(unlimited > 0);      // the limit has something to bind
        EXPECT(at_one > 0);
        EXPECT_EQ(chained_at_one, 0);
        EXPECT(at_two > 0);
        EXPECT_EQ(chained_at_two, 0);
    }
    end_section();
}

// ---- MultiPV on the record -----------------------------------------------------

// Every line searches under its own window, from depth 4 on, and only line 1's
// search reaches the table: a later line searched the root without the best moves,
// so its result is not the root's.
static void test_the_lines_of_a_multipv_search() {
    begin_section("MultiPV lines on the record");
    const std::string fen = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3";
    // A queen for nothing: line 1 cannot be overtaken, so its move is the search's.
    const std::string queen = "4k3/8/8/3q4/4P3/8/8/4K3 w - - 0 1";
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    std::vector<std::string> lines;
    std::map<std::string, int64_t> diag;
    const std::regex token("([a-z_0-9]+)=(-?[0-9]+)");
    auto info = [&](const std::string& line) {
        lines.push_back(line);
        const std::string prefix = "info string diag kv ";
        if (line.compare(0, prefix.size(), prefix) == 0)
            for (std::sregex_iterator it(line.begin(), line.end(), token), end; it != end; ++it)
                diag[(*it)[1].str()] = std::stoll((*it)[2].str());
    };
    auto searcher = std::make_unique<Searcher>(tt, stop, info);
    Board b;
    b.set_fen(fen);
    SearchLimits lim;
    lim.depth = 10;
    lim.multipv = 3;
    lim.diag = true;
    const SearchResult result = searcher->search(b, lim);
    const auto pv_of = [&](int line_number, int depth) {
        const std::string head = "info depth " + std::to_string(depth) + " ";
        const std::string tag = " multipv " + std::to_string(line_number) + " ";
        for (const std::string& line : lines)
            if (line.rfind(head, 0) == 0 && line.find(tag) != std::string::npos && line.find("bound") == std::string::npos)
                return line.substr(line.find(" pv ") + 4, 4);
        return std::string();
    };
    EXPECT(result.bestmove != MOVE_NONE);
    EXPECT_STR(pv_of(1, 10), move_to_uci(result.bestmove));
    EXPECT(!pv_of(3, 10).empty());
    if (kCounters)
        EXPECT_EQ(diag["asp_windows"], 3 * (10 - 3));
    TranspositionTable queen_tt(16);
    auto queen_searcher = std::make_unique<Searcher>(queen_tt, stop);
    Board queen_board;
    queen_board.set_fen(queen);
    SearchLimits queen_lim;
    queen_lim.depth = 8;
    queen_lim.multipv = 3;
    const SearchResult taken = queen_searcher->search(queen_board, queen_lim);
    EXPECT_STR(move_to_uci(taken.bestmove), "e4d5");
    TTEntry entry{};
    EXPECT(queen_tt.probe_copy(queen_board.position_key(), entry));
    EXPECT_STR(move_to_uci(move_from_tt(entry.move16)), "e4d5");
    end_section();
}

// ---- The forgotten mate --------------------------------------------------------

static void test_a_proven_mate_stands() {
    begin_section("a proven mate stands");
    const int mate_in_8 = MATE_SCORE - 15;
    EXPECT(!b5::forgotten_mate(kRootScoreNone, 100));
    EXPECT(!b5::forgotten_mate(100, 50));               // no mate to lose
    EXPECT(!b5::forgotten_mate(mate_in_8, mate_in_8));
    EXPECT(!b5::forgotten_mate(mate_in_8, MATE_SCORE - 3));   // a shorter one
    EXPECT(b5::forgotten_mate(mate_in_8, 2607));
    EXPECT(b5::forgotten_mate(mate_in_8, MATE_SCORE - 19));   // a longer one
    EXPECT(b5::forgotten_mate(-mate_in_8, -2607));      // being mated, likewise
    // At depth 15 the search proves this mate, and at depth 16 it reads 2607.
    SearchLimits lim;
    lim.depth = 16;
    const Run run = search_limits("r1bq1r2/pp4k1/4p2p/3pPp1Q/3N1R1P/2PB4/6P1/6K1 w - - 0 1", lim);
    EXPECT_EQ(run.result.score, mate_in_8);
    EXPECT_STR(move_to_uci(run.result.bestmove), "f4g4");
    // Every line from the proof on says so.
    int after_the_proof = 0;
    for (const std::string& line : run.lines) {
        if (line.rfind("info depth 16 ", 0) == 0 && line.find("bound") == std::string::npos) {
            ++after_the_proof;
            EXPECT(line.find(" score mate 8 ") != std::string::npos);
        }
    }
    EXPECT_EQ(after_the_proof, 1);
    end_section();
}

// A proved mate keeps the open window: no iteration after it has a window.
static void test_a_mate_score_keeps_the_open_window() {
    begin_section("a mate score keeps the open window");
    SearchLimits lim;
    lim.depth = 10;
    const Run run = search_limits("6k1/8/5K2/8/8/8/8/R7 w - - 0 1", lim);
    EXPECT_EQ(run.result.score, MATE_SCORE - 3);
    // The iteration that first read the mate is the last with a window.
    int first_mate = 0;
    for (const std::string& line : run.lines) {
        if (first_mate == 0 && line.rfind("info depth", 0) == 0 && line.find(" score mate ") != std::string::npos)
            first_mate = std::stoi(line.substr(11));
    }
    EXPECT(first_mate > 0);
    EXPECT(first_mate < 10);
    if (kCounters)
        EXPECT_EQ(counter(run, "asp_windows"), std::max(0, first_mate - 3));
    end_section();
}

// Each failed window prints its bound, the line that failed high on a lower
// bound, and every printed score is inside what the search found.
static void test_the_bound_lines() {
    begin_section("the bound lines");
    SearchLimits lim;
    lim.depth = 14;
    lim.bound_line_after = 0.0;
    const Run run = search_limits("r1bq1rk1/pp2bppp/2n1pn2/2pp4/3P1B2/2PBPN2/PP1N1PPP/R2QK2R w KQ - 0 9", lim);
    int lower = 0, upper = 0, exact = 0;
    for (const std::string& line : run.lines) {
        if (line.rfind("info depth", 0) != 0)
            continue;
        if (line.find(" lowerbound ") != std::string::npos) ++lower;
        else if (line.find(" upperbound ") != std::string::npos) ++upper;
        else ++exact;
        EXPECT(line.find(" pv ") != std::string::npos);
    }
    EXPECT(lower + upper > 0);
    EXPECT_EQ(exact, 14);
    if (kCounters) {
        EXPECT_EQ(lower, counter(run, "asp_fail_high"));
        EXPECT_EQ(upper, counter(run, "asp_fail_low"));
    }
    end_section();
}

// The arm has no give-up hatch, and in a won ending its window is open.
static void test_no_give_up_on_the_arm() {
    begin_section("no give-up window on the arm");
    SearchLimits lim;
    lim.depth = 20;
    const Run run = search_limits("8/8/8/8/3k4/8/3KBN2/8 w - - 0 1", lim);
    EXPECT(run.result.bestmove != MOVE_NONE);
    if (kCounters) {
        EXPECT_EQ(counter(run, "asp_giveup"), 0);
        EXPECT_EQ(counter(run, "asp_nodes_giveup"), 0);
    }
    end_section();
}

// ---- Root reduction -------------------------------------------------------------

// One root search's trace, as records of what a root move's searches were.
struct RootEvent {
    std::string event, move;
    int depth = 0, alpha = 0, beta = 0, reduction = 0, result = 0;
};

static int trace_field(const std::string& line, const std::string& name) {
    const size_t at = line.find(" " + name + "=");
    return at == std::string::npos ? 0 : std::stoi(line.substr(at + name.size() + 2));
}

static std::string trace_text(const std::string& line, const std::string& name) {
    const size_t at = line.find(" " + name + "=");
    if (at == std::string::npos)
        return "";
    const size_t begin = at + name.size() + 2;
    return line.substr(begin, line.find(' ', begin) - begin);
}

[[maybe_unused]] static std::vector<RootEvent> root_trace(const std::string& fen, int depth) {
    SearchLimits lim;
    lim.depth = depth;
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    lim.decision_trace = true;
    lim.trace_root = true;
#endif
    const Run run = search_limits(fen, lim);
    std::vector<RootEvent> events;
    for (const std::string& line : run.lines) {
        if (line.rfind("info string trace record", 0) != 0)
            continue;
        RootEvent e;
        e.event = trace_text(line, "event");
        e.move = trace_text(line, "move");
        e.depth = trace_field(line, "depth");
        e.alpha = trace_field(line, "alpha");
        e.beta = trace_field(line, "beta");
        e.reduction = trace_field(line, "reduction");
        e.result = trace_field(line, "result");
        events.push_back(e);
    }
    return events;
}

// A reduced root move that fails high is searched again at its full depth and
// then with the full window. The trace reads each root move's searches in order.
static void test_a_reduced_root_move_that_fails_high() {
    begin_section("a reduced root move that fails high");
    if (kCounters) {
        // The whole root: a second move is hardly ever reduced, since it has the
        // smallest move count and the ply a PV node adds back.
        int chains = 0;
        for (std::string_view fen : bench_fens()) {
            const std::vector<RootEvent> events = root_trace(std::string(fen), 14);
            for (size_t k = 0; k + 2 < events.size(); ++k) {
                const RootEvent& reduced = events[k];
                const RootEvent& again = events[k + 1];
                const RootEvent& full = events[k + 2];
                if (reduced.event != "lmr_reduction" || reduced.reduction <= 0
                    || again.event != "lmr_research" || full.event != "root_pv_research"
                    || again.move != reduced.move || full.move != reduced.move)
                    continue;
                ++chains;
                // The re-search is at the move's full depth, the depth the reduced
                // one was short of, give or take the ply a margin adds or takes.
                EXPECT(again.reduction >= reduced.reduction - 1);
                EXPECT(again.reduction <= reduced.reduction + 1);
                // It failed high, and only then the window opens to the node's.
                EXPECT(again.result > again.alpha);
                EXPECT(full.alpha == again.alpha);
                EXPECT(full.beta > full.alpha + 1 || full.beta == again.beta);
            }
        }
        EXPECT(chains > 0);
    }
    end_section();
}

// Reduced root moves are counted and are searched again no more often than they
// are reduced.
static void test_root_reduction_counters() {
    begin_section("root reduction fires");
    if (kCounters) {
        SearchLimits lim;
        lim.depth = 12;
        const Run run = search_limits("r1bq1rk1/pp2bppp/2n1pn2/2pp4/3P1B2/2PBPN2/PP1N1PPP/R2QK2R w KQ - 0 9", lim);
        EXPECT(counter(run, "root_lmr_reduced") > 0);
        EXPECT(counter(run, "root_lmr_researched") <= counter(run, "root_lmr_reduced"));
    }
    end_section();
}

int main(int argc, char** argv) {
    const char* const suite = argc > 1 ? argv[1] : "tools/diag/suite_v2.epd";
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
    test_the_record_arithmetic();
    test_the_root_move_comes_from_the_record();
    test_the_window_arithmetic();
    test_the_width_grows_everywhere_on_the_ranges();
    test_the_adjusted_depth();
    test_the_ceiling();
    test_every_child_starts_clean(suite);
    test_the_limit_holds_on_the_line(suite);
    test_the_lines_of_a_multipv_search();
    test_a_proven_mate_stands();
    test_a_mate_score_keeps_the_open_window();
    test_the_bound_lines();
    test_no_give_up_on_the_arm();
    test_a_reduced_root_move_that_fails_high();
    test_root_reduction_counters();

    return harness_summary();
}
