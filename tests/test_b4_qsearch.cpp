/// Cluster 3 (the B4_QSEARCH arm): its arithmetic read directly, and its
/// mechanisms driven through whole searches. Every section binds both arms:
/// where the arms differ, the OFF arm's expectation is today's quiescence.
///
/// A mechanism is read at a quiescence node by a one-ply search of one root
/// move under a fixed root window: the child is the node. In Diag and Tune
/// builds the counter sections read the search's `Diag` lines through the
/// info callback; release builds compile the counters out and skip them.

#include "bench.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "parameters.h"
#include "search_b4.h"
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
    bool node_stored = false;   // the table holds the node after the search
    TTEntry node_entry{};
};

static long long counter(const Run& run, const std::string& name) {
    const auto it = run.diag.find(name);
    return it == run.diag.end() ? 0 : it->second;
}

// A square-to-square move of the position `fen`, found among the legal moves
// so its type is right.
static Move legal_move(const Board& b, Square from, Square to) {
    MoveList legal;
    b.gen_legal(legal);
    for (Move m : legal)
        if (from_sq(m) == from && to_sq(m) == to)
            return m;
    return MOVE_NONE;
}

struct NodeRead {
    std::string fen;                 // the root; its one move leads to the node
    Square from = A1, to = A1;       // the root move
    int root_alpha = -INF_SCORE + 1; // the node's window is the negation
    int root_beta = INF_SCORE - 1;
    bool seed = false;               // store `table_move` for the node first
    Move table_move = MOVE_NONE;
    void (*set)(SearchParams&) = nullptr;
    int depth = 1;
};

// The search of one root move. A seeded entry holds a move and no bound, so
// the node reads the move and takes no cutoff from it.
static Run read_node(const NodeRead& r) {
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
    b.set_fen(r.fen);
    const Move root_move = legal_move(b, r.from, r.to);
    EXPECT(root_move != MOVE_NONE);
    if (r.seed) {
        Board child = b;
        child.make_move(root_move);
        tt.store(child.position_key(), TT_DEPTH_UNSEARCHED, VALUE_NONE, TT_NONE, r.table_move, 1,
                 INF_SCORE, false);
    }
    SearchLimits lim;
    lim.depth = r.depth;
    lim.diag = true;
    lim.root_moves = {root_move};
    lim.root_alpha = r.root_alpha;
    lim.root_beta = r.root_beta;
    if (r.set)
        r.set(lim.params);
    run.result = searcher->search(b, lim);
    Board child = b;
    child.make_move(root_move);
    run.node_stored = tt.probe_copy(child.position_key(), run.node_entry);
    return run;
}

// A whole search from `fen` under one setting.
static SearchResult search_fen(const std::string& fen, int depth,
                               void (*set)(SearchParams&) = nullptr) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    auto searcher = std::make_unique<Searcher>(tt, stop);
    Board b;
    b.set_fen(fen);
    SearchLimits lim;
    lim.depth = depth;
    if (set)
        set(lim.params);
    return searcher->search(b, lim);
}

static void evasions_all(SearchParams& p) { p.b4_evasion_prune = 0; }

// ---- Order --------------------------------------------------------------------

// Rd8+ leaves Black two evasions: the capture Rxd8, which every order but the
// table's tries first, and the quiet Kh7. Under a window so low that any
// evasion holds, the node fails high on the first one it searches.
static const char* const kBackRank = "r5k1/5pp1/7p/8/8/8/5PPP/3R2K1 w - - 0 1";

static NodeRead back_rank_check() {
    NodeRead r;
    r.fen = kBackRank;
    r.from = D1;
    r.to = D8;
    r.root_alpha = 1999;
    r.root_beta = 2000;
    return r;
}

static void test_table_move_leads_the_evasions() {
    if constexpr (!kCounters)
        return;
    Board child;
    child.set_fen(kBackRank);
    child.make_move(legal_move(child, D1, D8));
    const Move quiet = legal_move(child, G8, H7);
    const Move capture = legal_move(child, A8, D8);
    EXPECT(quiet != MOVE_NONE && capture != MOVE_NONE);

    NodeRead seeded = back_rank_check();
    seeded.seed = true;
    seeded.table_move = quiet;
    const Run with_table = read_node(seeded);
    const Run without_table = read_node(back_rank_check());

    begin_section("order: a table move that is a legal evasion is searched first");
    EXPECT_EQ(counter(with_table, "qs_evasion_nodes"), 1);
    EXPECT_EQ(counter(with_table, "qs_evasion_tt_move"), 1);
    EXPECT_EQ(counter(with_table, "qs_evasion_fail_highs"), 1);
    if constexpr (kB4Qsearch) {
        EXPECT_EQ(counter(with_table, "qs_evasion_fail_high_first"), 1);
        EXPECT_EQ(counter(with_table, "qs_evasion_tt_cuts"), 1);
        EXPECT_EQ(counter(with_table, "qs_evasion_moves"), 1);
    }
    end_section();

    begin_section("order: without one the node takes the picker's first evasion");
    EXPECT_EQ(counter(without_table, "qs_evasion_tt_move"), 0);
    EXPECT_EQ(counter(without_table, "qs_evasion_tt_cuts"), 0);
    EXPECT_EQ(counter(without_table, "qs_evasion_fail_highs"), 1);
    end_section();

    // A move that does not answer the check is validated away, not played.
    NodeRead stale = back_rank_check();
    stale.seed = true;
    stale.table_move = make_move(A8, A1);
    const Run with_stale = read_node(stale);
    begin_section("order: a table move that is no legal evasion is never played");
    EXPECT_EQ(counter(with_stale, "qs_evasion_tt_move"), 0);
    EXPECT_EQ(counter(with_stale, "qs_evasion_fail_highs"), 1);
    EXPECT_EQ(with_stale.result.nodes, without_table.result.nodes);
    end_section();
}

// Outside check a quiet table move changes nothing: quiescence neither
// searches nor counts it. The node after Qd2 is quiet and holds captures.
static void test_quiet_table_move_outside_check() {
    const std::string fen(bench_fens()[1]);
    Board b;
    b.set_fen(fen);
    MoveList legal;
    b.gen_legal(legal);
    // Any root move that gives no check, and a quiet reply to it.
    Move root_move = MOVE_NONE, reply = MOVE_NONE;
    for (Move m : legal) {
        Board child = b;
        child.make_move(m);
        if (child.is_in_check())
            continue;
        MoveList quiets;
        child.gen_legal_quiets(quiets);
        if (!quiets.empty()) {
            root_move = m;
            reply = quiets[0];
            break;
        }
    }
    EXPECT(root_move != MOVE_NONE);
    NodeRead plain;
    plain.fen = fen;
    plain.from = Square(from_sq(root_move));
    plain.to = Square(to_sq(root_move));
    plain.seed = true;
    plain.table_move = MOVE_NONE;
    NodeRead quiet = plain;
    quiet.table_move = reply;
    const Run a = read_node(plain);
    const Run c = read_node(quiet);
    begin_section("order: a quiet table move outside check changes nothing");
    EXPECT_EQ(a.result.nodes, c.result.nodes);
    EXPECT_EQ(a.result.score, c.result.score);
    end_section();
}

// ---- Evasions -------------------------------------------------------------------

// Re5+ loses the rook to Nxe5; the king has three quiet evasions besides. The
// capture holds, so the arm searches no quiet after it.
static NodeRead knight_takes_rook() {
    NodeRead r;
    r.fen = "4k3/5n2/8/R7/8/8/8/K7 w - - 0 1";
    r.from = A5;
    r.to = E5;
    return r;
}

static void test_a_holding_evasion_ends_the_quiets() {
    Board child;
    child.set_fen(knight_takes_rook().fen);
    child.make_move(legal_move(child, A5, E5));

    const Run pruned = read_node(knight_takes_rook());
    NodeRead everything = knight_takes_rook();
    everything.set = evasions_all;
    const Run searched = read_node(everything);
    NodeRead quiet_first = knight_takes_rook();
    quiet_first.seed = true;
    quiet_first.table_move = legal_move(child, E8, D7);
    const Run one_quiet = read_node(quiet_first);

    begin_section("evasions: a holding capture and three quiets search no quiet (arm)");
    if constexpr (kB4Qsearch) {
        EXPECT(pruned.result.nodes < searched.result.nodes);
        if constexpr (kCounters) {
            EXPECT_EQ(counter(pruned, "qs_evasion_moves"), 1);
            EXPECT_EQ(counter(pruned, "qs_evasion_quiets_pruned"), 3);
        }
    } else {
        EXPECT_EQ(pruned.result.nodes, searched.result.nodes);
    }
    end_section();

    begin_section("evasions: with the switch at 0 every evasion is searched");
    if constexpr (kCounters) {
        EXPECT_EQ(counter(searched, "qs_evasion_moves"), 4);
        EXPECT_EQ(counter(searched, "qs_evasion_quiets_pruned"), 0);
    }
    end_section();

    begin_section("evasions: a quiet table move is the one quiet searched");
    if constexpr (kB4Qsearch && kCounters) {
        EXPECT_EQ(counter(one_quiet, "qs_evasion_moves"), 2);
        EXPECT_EQ(counter(one_quiet, "qs_evasion_quiets_pruned"), 2);
    }
    end_section();

    begin_section("evasions: the rule leaves the node's value where it was here");
    EXPECT_EQ(pruned.result.score, searched.result.score);
    end_section();
}

// The arm stores an evasion node at depth 0 with no static evaluation and the
// move that raised alpha; the OFF arm stores none.
static void test_exit_store_in_check() {
    Board child;
    child.set_fen(knight_takes_rook().fen);
    child.make_move(legal_move(child, A5, E5));
    const Run run = read_node(knight_takes_rook());
    begin_section("evasions: the exit store in check");
    if constexpr (kB4Qsearch) {
        EXPECT(run.node_stored);
        EXPECT_EQ(int(run.node_entry.depth), 0);
        EXPECT_EQ(run.node_entry.static_eval, INF_SCORE);
        EXPECT_EQ(move_from_tt(run.node_entry.move16), legal_move(child, F7, E5));
        EXPECT(run.node_entry.bound() == TT_ALPHA);
    } else {
        EXPECT(!run.node_stored);
    }
    end_section();
}

// Ra8+ leaves two evasions, Nc8 and Ng8, and each loses to the rook's capture
// with mate, so the best stays a loss and no evasion may be skipped: a mate
// score is only as good as its proof.
static void test_a_lost_node_searches_every_evasion() {
    NodeRead lost;
    lost.fen = "7k/4n1pp/8/3B4/8/8/8/R5K1 w - - 0 1";
    lost.from = A1;
    lost.to = A8;
    const Run pruned = read_node(lost);
    lost.set = evasions_all;
    const Run searched = read_node(lost);
    begin_section("evasions: a node whose every evasion loses searches all of them");
    EXPECT(pruned.result.score >= MATE_SCORE - MAX_PLY);
    EXPECT_EQ(pruned.result.score, searched.result.score);
    EXPECT_EQ(pruned.result.nodes, searched.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(pruned, "qs_evasion_quiets_pruned"), 0);
        EXPECT_EQ(counter(pruned, "qs_evasion_see_pruned"), 0);
    }
    end_section();

    // Without the bishop Ng8 holds: Rxg8+ Kxg8. With Nc8 searched first the
    // node's best is a loss when Ng8 comes, so Ng8 is searched and the root
    // reads no mate.
    NodeRead one_holds;
    one_holds.fen = "7k/4n1pp/8/8/8/8/8/R5K1 w - - 0 1";
    one_holds.from = A1;
    one_holds.to = A8;
    Board child;
    child.set_fen(one_holds.fen);
    child.make_move(legal_move(child, A1, A8));
    one_holds.seed = true;
    one_holds.table_move = legal_move(child, E7, C8);
    const Run saved = read_node(one_holds);
    begin_section("evasions: a loss searched first does not end the node's evasions");
    EXPECT(std::abs(saved.result.score) < KNOWN_WIN);
    end_section();
}

// The mate-recognition fixtures of test_endgames and test_search's KQK
// shortest mate read the same distance with the rule on and off.
static void test_mate_distance_under_the_rule() {
    static const char* const FENS[] = {
        "k7/7Q/2K5/8/8/8/8/8 w - - 0 1",
        "k7/8/2K5/8/8/8/8/1Q6 w - - 0 1",
        "k7/2K5/8/8/8/8/8/7R w - - 0 1",
        "7k/8/6K1/8/8/8/8/R7 w - - 0 1",
        "k7/2K5/8/3N4/4B3/8/8/8 w - - 0 1",
        "4K3/2Q5/6k1/8/8/8/8/8 w - - 0 1",
    };
    begin_section("evasions: a mate reads the same distance with the rule on and off");
    for (const char* fen : FENS) {
        const SearchResult on = search_fen(fen, 18);
        const SearchResult off = search_fen(fen, 18, evasions_all);
        EXPECT(on.score >= MATE_SCORE - MAX_PLY);
        EXPECT_EQ(on.score, off.score);
    }
    end_section();
}

// ---- The tune surface -----------------------------------------------------------

static void test_options_are_the_arms_alone() {
#ifdef BASILISK_TUNE
    const std::string options = Parameters::uci_options();
    const SearchParams compiled;
    begin_section("options: a tune build advertises Cluster 3's table on the arm alone");
#define B4_ADVERTISED(field, uci, def, lo, hi)                                              \
    EXPECT_EQ(options.find("option name " #uci " type spin default "                        \
                           + std::to_string(compiled.field) + " min ") != std::string::npos, \
              kB4Qsearch);
    BASILISK_B4_PARAMS(B4_ADVERTISED, B4_ADVERTISED, B4_ADVERTISED)
#undef B4_ADVERTISED
    end_section();
#endif
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Cluster 3 (%s arm)\n", kB4Qsearch ? "B4_QSEARCH" : "OFF");
    std::printf("==================\n");

    test_options_are_the_arms_alone();
    test_table_move_leads_the_evasions();
    test_quiet_table_move_outside_check();
    test_a_holding_evasion_ends_the_quiets();
    test_exit_store_in_check();
    test_a_lost_node_searches_every_evasion();
    test_mate_distance_under_the_rule();

    return harness_summary();
}
