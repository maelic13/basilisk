/// Cluster 3, the quiescence the engine plays: its arithmetic read directly,
/// and its mechanisms driven through whole searches.
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
#include "search_b3.h"
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
    bool seed_pv = false;            // the seeded entry carries the PV mark
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
                 INF_SCORE, r.seed_pv);
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
    EXPECT_EQ(counter(with_table, "qs_evasion_fail_high_first"), 1);
    EXPECT_EQ(counter(with_table, "qs_evasion_tt_cuts"), 1);
    EXPECT_EQ(counter(with_table, "qs_evasion_moves"), 1);
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
    EXPECT(pruned.result.nodes < searched.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(pruned, "qs_evasion_moves"), 1);
        EXPECT_EQ(counter(pruned, "qs_evasion_quiets_pruned"), 3);
    }
    end_section();

    begin_section("evasions: with the switch at 0 every evasion is searched");
    if constexpr (kCounters) {
        EXPECT_EQ(counter(searched, "qs_evasion_moves"), 4);
        EXPECT_EQ(counter(searched, "qs_evasion_quiets_pruned"), 0);
    }
    end_section();

    begin_section("evasions: a quiet table move is the one quiet searched");
    if constexpr (kCounters) {
        EXPECT_EQ(counter(one_quiet, "qs_evasion_moves"), 2);
        EXPECT_EQ(counter(one_quiet, "qs_evasion_quiets_pruned"), 2);
    }
    end_section();

    begin_section("evasions: the rule leaves the node's value where it was here");
    EXPECT_EQ(pruned.result.score, searched.result.score);
    end_section();
}

// The arm stores an evasion node at depth 0 with no static evaluation and the
// move that raised alpha.
static void test_exit_store_in_check() {
    Board child;
    child.set_fen(knight_takes_rook().fen);
    child.make_move(legal_move(child, A5, E5));
    const Run run = read_node(knight_takes_rook());
    begin_section("evasions: the exit store in check");
    EXPECT(run.node_stored);
    EXPECT_EQ(int(run.node_entry.depth), 0);
    EXPECT_EQ(run.node_entry.static_eval, INF_SCORE);
    EXPECT_EQ(move_from_tt(run.node_entry.move16), legal_move(child, F7, E5));
    EXPECT(run.node_entry.bound() == TT_ALPHA);
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

// ---- The gain rules ---------------------------------------------------------------

// The static evaluation a node reads from fresh tables, at its own clock.
static int node_eval(const NodeRead& r) {
    Board b;
    b.set_fen(r.fen);
    b.make_move(legal_move(b, r.from, r.to));
    Evaluator evaluator;
    return evaluator.evaluate(b);
}

// A window that makes `alpha` the node's lower edge, one wide.
static void set_node_alpha(NodeRead& r, int alpha) {
    r.root_alpha = -alpha - 1;
    r.root_beta = -alpha;
}

// Black's queen takes the knight on d5 and White's pawn recaptures: the node
// stands a queen down, and with alpha at +200 the 900 credited, on the seed's
// margin, leaves the recapture below alpha.
static void test_a_recapture_is_exempt() {
    NodeRead r;
    r.fen = "3qk3/8/8/3N4/4P3/8/8/6K1 b - - 0 1";
    r.from = D8;
    r.to = D5;
    set_node_alpha(r, 200);
    const Run exempt = read_node(r);
    r.set = [](SearchParams& p) { p.b4_qs_recapture = 0; };
    const Run judged = read_node(r);
    begin_section("gain rules: a queen recapture below alpha is searched, and pruned without the exemption");
    EXPECT(exempt.result.nodes > judged.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(exempt, "qs_futility_pruned"), 0);
        EXPECT_EQ(counter(exempt, "qs_recapture_exempt"), 1);
        EXPECT_EQ(counter(exempt, "qs_gain_tested"), 0);
        EXPECT(counter(exempt, "qs_searched") >= 1);
        EXPECT_EQ(counter(judged, "qs_futility_pruned"), 1);
        EXPECT_EQ(counter(judged, "qs_recapture_exempt"), 0);
        EXPECT_EQ(counter(judged, "qs_gain_tested"), 1);
        EXPECT_EQ(counter(judged, "qs_searched"), 0);
    }
    end_section();
}

// After ...Kg8 White has three captures of loose pawns. None checks or
// recaptures, and Black has no capture after any of them, so the count rule
// reaches the third.
static NodeRead three_loose_pawns() {
    NodeRead r;
    r.fen = "7k/8/p7/2p5/4p3/1N6/6B1/R5K1 b - - 0 1";
    r.from = H8;
    r.to = G8;
    return r;
}

static void test_the_count_rule() {
    // The widest futility base, so that only the count rule can stop the third.
    NodeRead on = three_loose_pawns();
    on.set = [](SearchParams& p) { p.b4_qs_fut_base = 220; };
    const Run limited = read_node(on);
    NodeRead off = three_loose_pawns();
    off.set = [](SearchParams& p) {
        p.b4_qs_fut_base = 220;
        p.b4_qs_move_count = 64;
    };
    const Run unlimited = read_node(off);
    begin_section("gain rules: the third non-checking capture is skipped at 2 and searched at 64");
    EXPECT(limited.result.nodes < unlimited.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(limited, "qs_count_pruned"), 1);
        EXPECT_EQ(counter(limited, "qs_gain_tested"), 3);
        EXPECT_EQ(counter(limited, "qs_searched"), 2);
        EXPECT_EQ(counter(unlimited, "qs_count_pruned"), 0);
        EXPECT_EQ(counter(unlimited, "qs_searched"), 3);
    }
    end_section();

    // A quiet table move takes no count slot: the same two captures are searched.
    Board child;
    child.set_fen(three_loose_pawns().fen);
    child.make_move(legal_move(child, H8, G8));
    NodeRead quiet = on;
    quiet.seed = true;
    quiet.table_move = legal_move(child, G1, G2);
    const Run with_quiet = read_node(quiet);
    begin_section("gain rules: a quiet table move is neither searched nor counted");
    EXPECT_EQ(with_quiet.result.nodes, limited.result.nodes);
    end_section();
}

// exd6 en passant: the node's alpha is set either side of the pawn it credits.
// One below the rule's sum the capture is searched; at the sum it is pruned.
static void test_en_passant_is_credited_a_pawn() {
    NodeRead r;
    r.fen = "7k/3p4/8/4P3/8/8/8/6K1 b - - 0 1";
    r.from = D7;
    r.to = D5;
    const SearchParams defaults;
    const int sum = node_eval(r) + defaults.b4_qs_fut_base + PIECE_VALUE[PAWN];
    NodeRead below = r;
    set_node_alpha(below, sum - 1);
    NodeRead at = r;
    set_node_alpha(at, sum);
    const Run searched = read_node(below);
    const Run pruned = read_node(at);
    begin_section("gain rules: en passant is credited a pawn");
    EXPECT(searched.result.nodes > pruned.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(searched, "qs_futility_pruned"), 0);
        EXPECT_EQ(counter(pruned, "qs_futility_pruned"), 1);
    }
    end_section();
}

// After ...Kg8 a knight takes a loose rook. At the sum on the search's values
// the capture is pruned; the evaluation's material credits it 38 more, so the
// same alpha lets it through. En passant on vector 1 is credited its pawn, 88.
static void test_the_gain_vector() {
    NodeRead r;
    r.fen = "7k/8/8/8/8/2r5/8/1N4K1 b - - 0 1";
    r.from = H8;
    r.to = G8;
    const SearchParams defaults;
    set_node_alpha(r, node_eval(r) + defaults.b4_qs_fut_base + PIECE_VALUE[ROOK]);
    const Run search_values = read_node(r);
    r.set = [](SearchParams& p) { p.b4_gain_values = 1; };
    const Run hce_values = read_node(r);
    begin_section("gain rules: the gain vector moves a rook's credit from 500 to 538");
    EXPECT_EQ(defaults.b4_gain_values, 0);
    EXPECT_EQ(b4::gain_value(ROOK, 0), 500);
    EXPECT_EQ(b4::gain_value(ROOK, 1), 538);
    EXPECT(hce_values.result.nodes > search_values.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(search_values, "qs_futility_pruned"), 1);
        EXPECT_EQ(counter(hce_values, "qs_futility_pruned"), 0);
    }
    end_section();

    NodeRead ep;
    ep.fen = "7k/3p4/8/4P3/8/8/8/6K1 b - - 0 1";
    ep.from = D7;
    ep.to = D5;
    set_node_alpha(ep, node_eval(ep) + defaults.b4_qs_fut_base + HCE_PIECE_VALUE[PAWN]);
    const Run ep_search_values = read_node(ep);
    ep.set = [](SearchParams& p) { p.b4_gain_values = 1; };
    const Run ep_hce_values = read_node(ep);
    begin_section("gain rules: en passant is credited the vector's pawn");
    if constexpr (kCounters) {
        EXPECT_EQ(counter(ep_search_values, "qs_futility_pruned"), 0);
        EXPECT_EQ(counter(ep_hce_values, "qs_futility_pruned"), 1);
    }
    end_section();
}

// Black's bare king stands next to a loose bishop in a won KBB-K, a known
// loss by the static eval. The capture ends the ending in a draw, and it is
// no recapture, so only the band's guard keeps the gain rules from reading it
// as a bishop against that eval. (The wrong-bishop draws of test_endgames end
// in a recapture, which the arm exempts either way.)
static void test_the_known_loss_band_stands_down() {
    NodeRead r;
    r.fen = "8/7K/8/8/8/7B/8/3kB3 w - - 0 1";
    r.from = H7;
    r.to = H8;
    set_node_alpha(r, -5000);
    const SearchParams defaults;
    const Run run = read_node(r);
    begin_section("gain rules: a capture in the known-loss band is searched");
    EXPECT(node_eval(r) + defaults.b4_qs_fut_base <= -KNOWN_WIN);
    // The node holds at least the draw, so the root fails low on it.
    EXPECT(run.result.score <= 5000);
    if constexpr (kCounters)
        EXPECT_EQ(counter(run, "qs_futility_pruned"), 0);
    end_section();
}

// The delta rule is off by default on the arm and returns with its switch;
// the late rule and the ply cap are gone from the arm's tree.
static void test_the_rules_the_arm_removes() {
    if constexpr (!kCounters)
        return;
    NodeRead r;
    r.fen = "3qk3/8/8/3N4/4P3/8/8/6K1 b - - 0 1";
    r.from = D8;
    r.to = D5;
    set_node_alpha(r, 1000);
    const Run plain = read_node(r);
    r.set = [](SearchParams& p) { p.b4_qs_delta = 1; };
    const Run delta = read_node(r);
    begin_section("gain rules: the delta rule fires only behind its switch on the arm");
    EXPECT_EQ(counter(delta, "qs_delta_pruned"), 1);
    EXPECT_EQ(counter(plain, "qs_delta_pruned"), 0);
    end_section();

    NodeRead tree;
    tree.fen = std::string(bench_fens()[3]);
    Board b;
    b.set_fen(tree.fen);
    MoveList legal;
    b.gen_legal(legal);
    tree.from = Square(from_sq(legal[0]));
    tree.to = Square(to_sq(legal[0]));
    tree.depth = 9;
    const Run run = read_node(tree);
    begin_section("gain rules: the arm's tree holds no late rule and no ply cap");
    EXPECT(counter(run, "qs_nodes") > 0);
    EXPECT_EQ(counter(run, "qs_late_pruned"), 0);
    EXPECT_EQ(counter(run, "qs_cap_hits"), 0);
    end_section();
}

// ---- The blends ---------------------------------------------------------------------

static void test_blend_arithmetic() {
    begin_section("blends: the arithmetic at the seeds");
    EXPECT_EQ(b4::fail_high_value(1000, 0, 441), 441 * 1000 / 1024);
    EXPECT_EQ(b4::fail_high_value(1000, 0, 462), 462 * 1000 / 1024);
    EXPECT_EQ(b4::fail_high_value(300, 100, 441), (441 * 300 + 583 * 100) / 1024);
    EXPECT_EQ(b4::fail_high_value(300, 100, 462), (462 * 300 + 562 * 100) / 1024);
    EXPECT_EQ(b4::fail_high_value(-100, -300, 441), (441 * -100 + 583 * -300) / 1024);
    end_section();

    begin_section("blends: a weight of 1024 is the identity");
    for (int v : {-5000, -1, 0, 7, 901, 9000})
        EXPECT_EQ(b4::fail_high_value(v, v - 37, 1024), v);
    end_section();

    begin_section("blends: a decisive value is untouched");
    const int mate = MATE_SCORE - 9;
    EXPECT_EQ(b4::fail_high_value(mate, 100, 441), mate);
    EXPECT_EQ(b4::fail_high_value(tablebaseValue - 5, 100, 462), tablebaseValue - 5);
    EXPECT_EQ(b4::fail_high_value(tablebaseWinInMaxPly, 100, 462), tablebaseWinInMaxPly);
    end_section();

    begin_section("blends: a value at or below beta is untouched");
    EXPECT_EQ(b4::fail_high_value(100, 100, 441), 100);
    EXPECT_EQ(b4::fail_high_value(99, 100, 462), 99);
    end_section();

    begin_section("blends: a returned fail high is at or above beta for every weight in range");
    bool holds = true, below_value = true;
    for (int weight = 256; weight <= 1024; ++weight)
        for (int beta : {-9973, -1001, -3, -2, -1, 0, 1, 2, 3, 77, 1024, 9973})
            for (int gap : {1, 2, 3, 5, 100, 1023, 1024, 1025, 7000}) {
                const int v = b4::fail_high_value(beta + gap, beta, weight);
                holds = holds && v >= beta;
                below_value = below_value && v <= beta + gap;
            }
    EXPECT(holds);
    EXPECT(below_value);
    end_section();
}

// The node stands pat 100 above beta. The root reads the node's value, and
// the table's lower bound is the value returned, on the arm the blend.
static void test_stand_pat_returns_and_stores_the_blend() {
    NodeRead r;
    r.fen = "3qk3/8/8/8/4P3/8/8/6K1 b - - 0 1";
    r.from = D8;
    r.to = D7;
    const int eval = node_eval(r);
    const int beta = eval - 100;
    set_node_alpha(r, beta - 1);
    const Run run = read_node(r);
    const SearchParams defaults;
    const int expected = (defaults.b4_qs_stand_pat_blend * eval
                          + (1024 - defaults.b4_qs_stand_pat_blend) * beta) / 1024;
    begin_section("blends: a stand-pat cut returns and stores the value");
    EXPECT_EQ(run.result.score, -expected);
    EXPECT(run.node_stored);
    EXPECT(run.node_entry.bound() == TT_BETA);
    EXPECT_EQ(TranspositionTable::score_from_tt(run.node_entry.score, 1, 0), expected);
    end_section();
}

// ---- The clauses the fingerprint alone bound ------------------------------------
//
// Each section here fails on the mutant B.4.4's review named for its clause.

// Rd8+ leaves Black Kh7 and Qxd8, which gives the queen for a rook to the rook
// behind. Once Kh7, from the table, holds the node the capture is skipped.
static void test_a_losing_evasion_capture_is_skipped() {
    NodeRead r;
    r.fen = "6k1/5pp1/7p/q7/8/8/3R1PPP/3R2K1 w - - 0 1";
    r.from = D2;
    r.to = D8;
    Board child;
    child.set_fen(r.fen);
    child.make_move(legal_move(child, D2, D8));
    r.seed = true;
    r.table_move = legal_move(child, G8, H7);
    const Run pruned = read_node(r);
    r.set = evasions_all;
    const Run searched = read_node(r);
    begin_section("evasions: a capture that loses material is skipped once the node holds");
    EXPECT(pruned.result.nodes < searched.result.nodes);
    if constexpr (kCounters) {
        EXPECT(counter(pruned, "qs_evasion_see_pruned") >= 1);
        EXPECT_EQ(counter(searched, "qs_evasion_see_pruned"), 0);
    }
    end_section();
}

// An evasion node keeps the table's PV mark at its fail-high store and at its
// exit store.
static void test_the_evasion_stores_keep_the_pv_mark() {
    NodeRead high = back_rank_check();
    Board child;
    child.set_fen(high.fen);
    child.make_move(legal_move(child, D1, D8));
    high.seed = true;
    high.seed_pv = true;
    high.table_move = legal_move(child, G8, H7);
    const Run fail_high = read_node(high);

    NodeRead low = knight_takes_rook();
    Board exit_child;
    exit_child.set_fen(low.fen);
    exit_child.make_move(legal_move(exit_child, A5, E5));
    low.seed = true;
    low.seed_pv = true;
    low.table_move = legal_move(exit_child, F7, E5);
    const Run exit = read_node(low);

    begin_section("evasions: both stores keep the PV mark");
    EXPECT(fail_high.node_entry.bound() == TT_BETA);
    EXPECT(fail_high.node_entry.is_pv());
    EXPECT(exit.node_entry.bound() == TT_ALPHA);
    EXPECT(exit.node_entry.is_pv());
    end_section();
}

// A null move or the start of the line leaves no square to recapture on, even
// for a capture landing where the sentinel's bits point.
static void test_a_sentinel_is_no_previous_move() {
    begin_section("gain rules: a null move or no move has no recapture");
    EXPECT(b4::is_recapture(make_move(E4, D5), make_move(D8, D5)));
    EXPECT(!b4::is_recapture(make_move(E4, D5), make_move(D8, D7)));
    EXPECT(!b4::is_recapture(make_move(E4, to_sq(MOVE_NULL)), MOVE_NULL));
    EXPECT(!b4::is_recapture(make_move(E4, to_sq(MOVE_NONE)), MOVE_NONE));
    end_section();
}

// Rxa7 takes a loose pawn far below alpha. With the king on h7 it checks, so
// the gain rules pass it by; with the king on h8 futility prunes it.
static void test_a_checking_capture_is_exempt() {
    NodeRead check;
    check.fen = "6k1/p7/8/8/8/8/8/R5K1 b - - 0 1";
    check.from = G8;
    check.to = H7;
    NodeRead quiet = check;
    quiet.to = H8;
    set_node_alpha(check, node_eval(check) + 500);
    set_node_alpha(quiet, node_eval(quiet) + 500);
    const Run exempt = read_node(check);
    const Run judged = read_node(quiet);
    begin_section("gain rules: a checking capture below alpha is searched");
    EXPECT(exempt.result.nodes > judged.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(exempt, "qs_gain_tested"), 0);
        EXPECT_EQ(counter(exempt, "qs_futility_pruned"), 0);
        EXPECT(counter(exempt, "qs_searched") >= 1);
        EXPECT_EQ(counter(judged, "qs_gain_tested"), 1);
        EXPECT_EQ(counter(judged, "qs_futility_pruned"), 1);
        EXPECT_EQ(counter(judged, "qs_searched"), 0);
    }
    end_section();
}

// Rxa7+ is exempt from the gain rules but gives the rook for a pawn to the
// knight, so the plain SEE every capture must clear stops it.
static void test_the_plain_see_binds_an_exempt_capture() {
    NodeRead r;
    r.fen = "6k1/p7/2n5/8/8/8/8/R5K1 b - - 0 1";
    r.from = G8;
    r.to = H7;
    const Run pruned = read_node(r);
    r.set = [](SearchParams& p) { p.b4_qs_see_margin = -1000; };
    const Run searched = read_node(r);
    begin_section("gain rules: a losing check is stopped by the plain SEE");
    EXPECT(pruned.result.nodes < searched.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(pruned, "qs_see_pruned"), 1);
        EXPECT_EQ(counter(pruned, "qs_searched"), 0);
        EXPECT(counter(searched, "qs_searched") >= 1);
    }
    end_section();
}

// Nxd5 trades knights: its SEE is level, and it is credited a knight. With
// alpha 100 over the futility base the credit clears alpha and the exchange
// does not, so the alpha-relative SEE prunes it, and the base it leaves is the
// node's bound. One under the base it is searched.
static void test_the_alpha_relative_see() {
    NodeRead r;
    r.fen = "7k/8/4p3/3n4/8/2N5/8/6K1 b - - 0 1";
    r.from = H8;
    r.to = G8;
    const SearchParams defaults;
    const int base = node_eval(r) + defaults.b4_qs_fut_base;
    NodeRead above = r;
    set_node_alpha(above, base + 100);
    NodeRead below = r;
    set_node_alpha(below, base - 1);
    const Run pruned = read_node(above);
    const Run searched = read_node(below);
    begin_section("gain rules: the alpha-relative SEE prunes a level exchange and leaves the base");
    EXPECT(pruned.result.nodes < searched.result.nodes);
    EXPECT(pruned.node_entry.bound() == TT_ALPHA);
    EXPECT_EQ(TranspositionTable::score_from_tt(pruned.node_entry.score, 1, 0), base);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(pruned, "qs_see_pruned"), 1);
        EXPECT_EQ(counter(pruned, "qs_searched"), 0);
        EXPECT_EQ(counter(pruned, "qs_futility_pruned"), 0);
        EXPECT(counter(searched, "qs_searched") >= 1);
    }
    end_section();
}

// Qxd5 and Rxb5 lose material and are pruned, then gxh5 takes a loose pawn. The
// count rule counts delivered captures, so the third is past it though nothing
// has been searched.
static void test_the_count_rule_counts_pruned_captures() {
    NodeRead r;
    r.fen = "7k/8/p3p3/1n1r3p/6P1/8/8/1R1Q2K1 b - - 0 1";
    r.from = H8;
    r.to = G7;
    const Run limited = read_node(r);
    r.set = [](SearchParams& p) { p.b4_qs_move_count = 64; };
    const Run unlimited = read_node(r);
    begin_section("gain rules: a pruned capture takes its place in the count");
    EXPECT(limited.result.nodes < unlimited.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(limited, "qs_count_pruned"), 1);
        EXPECT_EQ(counter(limited, "qs_see_pruned"), 2);
        EXPECT_EQ(counter(limited, "qs_searched"), 0);
        EXPECT_EQ(counter(unlimited, "qs_count_pruned"), 0);
        EXPECT(counter(unlimited, "qs_searched") >= 1);
    }
    end_section();
}

// Nxc3 is credited a rook, and alpha lies 20 over the sum, so futility prunes
// it. The sum, above the stand-pat, is the node's upper bound.
static void test_futility_leaves_its_sum() {
    NodeRead r;
    r.fen = "7k/8/8/8/8/2r5/8/1N4K1 b - - 0 1";
    r.from = H8;
    r.to = G8;
    const SearchParams defaults;
    const int sum = node_eval(r) + defaults.b4_qs_fut_base + PIECE_VALUE[ROOK];
    set_node_alpha(r, sum + 20);
    const Run run = read_node(r);
    begin_section("gain rules: futility leaves its sum as the node's bound");
    EXPECT(run.node_entry.bound() == TT_ALPHA);
    EXPECT_EQ(TranspositionTable::score_from_tt(run.node_entry.score, 1, 0), sum);
    if constexpr (kCounters)
        EXPECT_EQ(counter(run, "qs_futility_pruned"), 1);
    end_section();
}

// Rxd3 wins the queen from a stand-pat 300 under beta. The node returns and
// stores the value pulled toward beta; a blend of 1024 reads the value itself.
static void test_a_capture_fail_high_returns_and_stores_the_blend() {
    NodeRead r;
    r.fen = "k7/pp6/8/8/3q4/8/PP6/3R2K1 b - - 0 1";
    r.from = D4;
    r.to = D3;
    const int beta = node_eval(r) + 300;
    set_node_alpha(r, beta - 1);
    Board child;
    child.set_fen(r.fen);
    child.make_move(legal_move(child, D4, D3));
    const Run blended = read_node(r);
    r.set = [](SearchParams& p) { p.b4_qs_fail_high_blend = 1024; };
    const Run plain = read_node(r);
    const SearchParams defaults;
    const int value = -plain.result.score;
    const int expected = b4::fail_high_value(value, beta, defaults.b4_qs_fail_high_blend);
    begin_section("blends: a capture fail high returns and stores the value");
    EXPECT(value > beta + 100);
    EXPECT_EQ(blended.result.score, -expected);
    EXPECT(blended.node_stored);
    EXPECT(blended.node_entry.bound() == TT_BETA);
    EXPECT_EQ(move_from_tt(blended.node_entry.move16), legal_move(child, D1, D3));
    EXPECT_EQ(TranspositionTable::score_from_tt(blended.node_entry.score, 1, 0), expected);
    end_section();
}

// ---- Stalemate ----------------------------------------------------------------------

// Rxh3 takes Black's last piece and leaves a bare king with a blocked pawn and
// no legal move. Quiescence reads the draw; with the switch off it stands pat.
static void test_stalemate() {
    NodeRead r;
    r.fen = "k7/p1K5/P7/8/8/7r/8/7R w - - 0 1";
    r.from = H1;
    r.to = H3;
    const int eval = node_eval(r);
    const Run on = read_node(r);
    r.set = [](SearchParams& p) { p.b4_stalemate = 0; };
    const Run off = read_node(r);
    begin_section("stalemate: a blocked bare king reads 0 with the switch on, the static eval off");
    EXPECT(eval != 0);
    EXPECT_EQ(on.result.score, 0);
    EXPECT_EQ(off.result.score, -eval);
    if constexpr (kCounters)
        EXPECT_EQ(counter(on, "qs_stalemates"), 1);
    end_section();

    // The same stalemate after a pawn fell: the donor's condition asks for a
    // piece, so quiescence stands pat.
    NodeRead pawn;
    pawn.fen = "k7/p1K5/P7/8/8/7p/8/7R w - - 0 1";
    pawn.from = H1;
    pawn.to = H3;
    const Run after_pawn = read_node(pawn);
    begin_section("stalemate: the test asks for a captured piece");
    EXPECT_EQ(after_pawn.result.score, -node_eval(pawn));
    end_section();

    // The draw is exact: above a window that ends below it, no blend moves it.
    // The node's beta of -200 lies between the static eval and the draw.
    NodeRead narrow = r;
    narrow.set = nullptr;
    set_node_alpha(narrow, -201);
    const Run exact = read_node(narrow);
    begin_section("stalemate: the draw takes no blend");
    EXPECT(eval < -200);
    EXPECT_EQ(exact.result.score, 0);
    end_section();
}

// ---- The fail-low store ---------------------------------------------------------

// exd5 recaptures the queen and is searched as a recapture, but with alpha at
// +3000 it fails low. The arm's one form stores no move after a fail low; with
// the switch at 0 it stores the best-scored searched move, here exd5.
static void test_fail_low_store_in_quiescence() {
    NodeRead r;
    r.fen = "3qk3/8/8/3N4/4P3/8/8/6K1 b - - 0 1";
    r.from = D8;
    r.to = D5;
    set_node_alpha(r, 3000);
    Board child;
    child.set_fen(r.fen);
    child.make_move(legal_move(child, D8, D5));
    const Move recapture = legal_move(child, E4, D5);
    const SearchParams defaults;

    r.set = [](SearchParams& p) { p.b3_fail_low_store = 1; };
    const Run none = read_node(r);
    r.set = [](SearchParams& p) { p.b3_fail_low_store = 0; };
    const Run scored = read_node(r);
    begin_section("fail-low store: no move at 1, the best-scored move at 0");
    EXPECT_EQ(defaults.b3_fail_low_store, 1);
    EXPECT(none.node_stored && scored.node_stored);
    EXPECT(none.node_entry.bound() == TT_ALPHA);
    EXPECT(scored.node_entry.bound() == TT_ALPHA);
    EXPECT_EQ(move_from_tt(none.node_entry.move16), MOVE_NONE);
    EXPECT_EQ(move_from_tt(scored.node_entry.move16), recapture);
    end_section();

    // In check the arm's form is the same: Re5+ Nxe5 is the best-scored evasion.
    NodeRead check;
    check.fen = "4k3/5n2/8/R7/8/8/8/K7 w - - 0 1";
    check.from = A5;
    check.to = E5;
    set_node_alpha(check, 5000);
    Board in_check;
    in_check.set_fen(check.fen);
    in_check.make_move(legal_move(in_check, A5, E5));
    check.set = [](SearchParams& p) { p.b3_fail_low_store = 1; };
    const Run none_in_check = read_node(check);
    check.set = [](SearchParams& p) { p.b3_fail_low_store = 0; };
    const Run scored_in_check = read_node(check);
    begin_section("fail-low store: the same form at an evasion node");
    EXPECT_EQ(move_from_tt(none_in_check.node_entry.move16), MOVE_NONE);
    EXPECT_EQ(move_from_tt(scored_in_check.node_entry.move16),
              legal_move(in_check, F7, E5));
    end_section();
}

// ---- Reverse futility's cap -------------------------------------------------------

static void test_rfp_seek_mate_gate() {
    begin_section("reverse futility: the cap binds past it, under seek_mate alone");
    EXPECT(b4::rfp_past_seek_mate_cap(true, 6, 5));
    EXPECT(b4::rfp_past_seek_mate_cap(true, 10, 5));
    EXPECT(!b4::rfp_past_seek_mate_cap(true, 5, 5));
    EXPECT(!b4::rfp_past_seek_mate_cap(true, 1, 5));
    EXPECT(!b4::rfp_past_seek_mate_cap(false, 6, 5));
    EXPECT(!b4::rfp_past_seek_mate_cap(false, 19, 5));
    end_section();

    begin_section("reverse futility: the cap reads its own bar, not the singular half's");
    const SearchParams p;
    auto capped = [&](int root_depth, int root_score, int depth) {
        return b4::rfp_past_seek_mate_cap(
            b3::seek_mate(root_depth, root_score, p.b4_rfp_seek_mate_depth, p.b3_seek_mate_score),
            depth, p.b4_rfp_seek_mate_cap);
    };
    EXPECT_EQ(p.b4_rfp_seek_mate_depth, 16);
    EXPECT_EQ(p.b3_seek_mate_depth, 12);
    EXPECT_EQ(p.b3_seek_mate_score, 564);
    EXPECT_EQ(p.b4_rfp_seek_mate_cap, 5);
    for (int root_depth : {16, 24}) {
        EXPECT(capped(root_depth, 564, 6));
        EXPECT(capped(root_depth, -564, 6));
        EXPECT(capped(root_depth, MATE_SCORE - 9, 6));
        EXPECT(capped(root_depth, -(MATE_SCORE - 9), 6));
        EXPECT(!capped(root_depth, 563, 6));     // not decided enough
        EXPECT(!capped(root_depth, -563, 6));
        EXPECT(!capped(root_depth, 564, 5));     // at the cap
    }
    EXPECT(!capped(15, 564, 6));                 // an early iteration
    EXPECT(!capped(15, MATE_SCORE - 9, 6));
    EXPECT(!capped(12, 564, 6));                 // the singular half's bar
    EXPECT(!capped(12, MATE_SCORE - 9, 6));
    end_section();

    begin_section("reverse futility: the default leaves the cap off");
    EXPECT_EQ(p.b4_rfp_seek_mate, 0);
    end_section();
}

// A won KR-K: the root's iterations pass the cap's bar and no mate is in reach,
// so the cap fires on the arm with its switch on and nowhere else.
static Run search_krk(int depth, void (*set)(SearchParams&)) {
    NodeRead r;
    r.fen = "8/8/8/4k3/8/8/8/4K2R w - - 0 1";
    r.from = E1;
    r.to = E2;
    r.depth = depth;
    r.set = set;
    return read_node(r);
}

static void cap_on(SearchParams& p) { p.b4_rfp_seek_mate = 1; }

static void test_rfp_seek_mate_live() {
    const Run deep_default = search_krk(18, nullptr);
    const Run deep_on = search_krk(18, cap_on);
    const Run shallow_default = search_krk(15, nullptr);
    const Run shallow_on = search_krk(15, cap_on);

    begin_section("reverse futility: from its bar the cap moves the tree");
    EXPECT(deep_on.result.nodes != deep_default.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(deep_default, "rfp_seekmate_capped"), 0);
        EXPECT(counter(deep_on, "rfp_seekmate_capped") > 0);
    }
    end_section();

    begin_section("reverse futility: below its bar the cap changes nothing");
    EXPECT_EQ(shallow_on.result.nodes, shallow_default.result.nodes);
    if constexpr (kCounters)
        EXPECT_EQ(counter(shallow_on, "rfp_seekmate_capped"), 0);
    end_section();

    // The singular half's bar moved out of reach: the cap fires as before, and
    // the switch alone separates the two trees.
    const Run singular_far_on = search_krk(18, [](SearchParams& p) {
        p.b4_rfp_seek_mate = 1;
        p.b3_seek_mate_depth = 64;
    });
    const Run singular_far_off = search_krk(18, [](SearchParams& p) {
        p.b3_seek_mate_depth = 64;
    });
    begin_section("reverse futility: the cap does not read the singular half's bar");
    EXPECT(singular_far_on.result.nodes != singular_far_off.result.nodes);
    if constexpr (kCounters)
        EXPECT(counter(singular_far_on, "rfp_seekmate_capped") > 0);
    end_section();

    const Run bar_far = search_krk(18, [](SearchParams& p) {
        p.b4_rfp_seek_mate = 1;
        p.b4_rfp_seek_mate_depth = 64;
    });
    begin_section("reverse futility: with its bar out of reach the tree is the default's");
    EXPECT_EQ(bar_far.result.nodes, deep_default.result.nodes);
    if constexpr (kCounters)
        EXPECT_EQ(counter(bar_far, "rfp_seekmate_capped"), 0);
    end_section();
}

// ---- Quiet checks -----------------------------------------------------------------

// After ...a6 White has one quiet check, Rd8 mate: a depth-1 search can see it
// only from quiescence.
static void test_a_quiet_mate_at_the_first_ply() {
    NodeRead r;
    r.fen = "6k1/p4ppp/8/8/8/8/8/3R2K1 b - - 0 1";
    r.from = A7;
    r.to = A6;
    const Run none = read_node(r);
    r.set = [](SearchParams& p) { p.b4_quiet_checks = 6; };
    const Run six = read_node(r);
    begin_section("quiet checks: a quiet mate at the first ply is found at 6 and not at 0");
    EXPECT_EQ(SearchParams{}.b4_quiet_checks, 0);
    EXPECT(!is_decisive(none.result.score));
    EXPECT_EQ(six.result.score, -(MATE_SCORE - 2));
    if constexpr (kCounters) {
        EXPECT_EQ(counter(none, "qs_quiet_checks"), 0);
        EXPECT_EQ(counter(six, "qs_quiet_checks"), 1);
    }
    end_section();
}

static void test_which_quiet_checks_are_searched() {
    // Two quiet checks from d1. Each leaves further checks after the king
    // moves, at a ply the rule does not reach.
    NodeRead two;
    two.fen = "7k/p7/8/8/8/8/8/1K1R4 b - - 0 1";
    two.from = A7;
    two.to = A6;
    two.set = [](SearchParams& p) { p.b4_quiet_checks = 6; };
    const Run first_ply = read_node(two);
    two.set = [](SearchParams& p) { p.b4_quiet_checks = 1; };
    const Run limited = read_node(two);

    // Rd8 is White's only check, and the rook on a8 takes it.
    NodeRead hanging;
    hanging.fen = "r5k1/p4ppp/8/8/8/8/8/3R2K1 b - - 0 1";
    hanging.from = A7;
    hanging.to = A6;
    const Run off = read_node(hanging);
    hanging.set = [](SearchParams& p) { p.b4_quiet_checks = 6; };
    const Run on = read_node(hanging);

    begin_section("quiet checks: the first ply alone, up to the limit, none that loses SEE");
    EXPECT(first_ply.result.nodes > limited.result.nodes);
    EXPECT_EQ(on.result.nodes, off.result.nodes);
    if constexpr (kCounters) {
        EXPECT_EQ(counter(first_ply, "qs_quiet_checks"), 2);
        EXPECT_EQ(counter(limited, "qs_quiet_checks"), 1);
        EXPECT_EQ(counter(on, "qs_quiet_checks"), 0);
    }
    end_section();
}

// ---- The SEE vector -----------------------------------------------------------

// The seam is a tune build's alone, and a search reads it from its limits: the
// tree moves with the vector.
static void test_the_see_vector_reaches_the_search() {
#ifdef BASILISK_TUNE
    const char* const kiwipete =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    const SearchResult search_values = search_fen(kiwipete, 8);
    const SearchResult hce_values = search_fen(kiwipete, 8, [](SearchParams& p) {
        p.b4_see_values = 1;
    });
    begin_section("SEE vector: a search reads it");
    EXPECT_EQ(SearchParams{}.b4_see_values, 0);
    EXPECT(hce_values.nodes != search_values.nodes);
    end_section();
#endif
}

// ---- The tune surface -----------------------------------------------------------

static void test_options_are_advertised() {
#ifdef BASILISK_TUNE
    const std::string options = Parameters::uci_options();
    const SearchParams compiled;
    begin_section("options: a tune build advertises Cluster 3's table");
#define B4_ADVERTISED(field, uci, def, lo, hi)                                              \
    EXPECT(options.find("option name " #uci " type spin default "                           \
                        + std::to_string(compiled.field) + " min ") != std::string::npos);
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

    std::printf("Cluster 3\n");
    std::printf("=========\n");

    test_options_are_advertised();
    test_table_move_leads_the_evasions();
    test_quiet_table_move_outside_check();
    test_a_holding_evasion_ends_the_quiets();
    test_exit_store_in_check();
    test_a_lost_node_searches_every_evasion();
    test_mate_distance_under_the_rule();
    test_a_recapture_is_exempt();
    test_the_count_rule();
    test_en_passant_is_credited_a_pawn();
    test_the_gain_vector();
    test_the_known_loss_band_stands_down();
    test_the_rules_the_arm_removes();
    test_blend_arithmetic();
    test_stand_pat_returns_and_stores_the_blend();
    test_stalemate();
    test_fail_low_store_in_quiescence();
    test_rfp_seek_mate_gate();
    test_rfp_seek_mate_live();
    test_the_see_vector_reaches_the_search();
    test_a_quiet_mate_at_the_first_ply();
    test_which_quiet_checks_are_searched();
    test_a_losing_evasion_capture_is_skipped();
    test_the_evasion_stores_keep_the_pv_mark();
    test_a_sentinel_is_no_previous_move();
    test_a_checking_capture_is_exempt();
    test_the_plain_see_binds_an_exempt_capture();
    test_the_alpha_relative_see();
    test_the_count_rule_counts_pruned_captures();
    test_futility_leaves_its_sum();
    test_a_capture_fail_high_returns_and_stores_the_blend();

    return harness_summary();
}
