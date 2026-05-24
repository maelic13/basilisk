/// Search tests.
///
/// Covers: legal move at depth 1, mate-in-1 detection (×2), free queen capture,
/// depth/nodes limits, near-zero score at startpos, node counter.
///
/// Build:
///   cmake --build --preset release --target test_search
///   ./build/release/test_search

#include "Board.h"
#include "EngineCommand.h"
#include "attacks.h"
#include "bitboard.h"
#include "eval.h"
#include "move.h"
#include "search.h"
#include "tt.h"
#include "zobrist.h"
#include "test_harness.h"

#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: run_search
// ---------------------------------------------------------------------------

struct RunResult {
    SearchResult sr;
    std::string  uci;
};

static RunResult run_search(const char* fen, int depth,
                            int64_t nodes = 0, int movetime = 0) {
    TranspositionTable tt(4);
    std::atomic_bool stop{false};

    Board b;
    b.set_fen(fen);

    SearchLimits lim;
    lim.depth    = depth;
    lim.nodes    = nodes;
    lim.movetime = movetime;

    auto searcher = std::make_unique<Searcher>(tt, stop);
    SearchResult sr = searcher->search(b, lim);

    RunResult rr;
    rr.sr  = sr;
    rr.uci = (sr.bestmove != MOVE_NONE) ? move_to_uci(sr.bestmove) : "0000";
    return rr;
}

static bool is_legal_bestmove(const char* fen, Move move) {
    if (move == MOVE_NONE)
        return false;

    Board b;
    b.set_fen(fen);
    MoveList legal;
    b.gen_legal(legal);
    for (Move candidate : legal)
        if (candidate == move)
            return true;
    return false;
}

static int mate_in_from_score(int score) {
    return (MATE_SCORE - std::abs(score) + 1) / 2;
}

static bool apply_legal_uci(Board& board, const std::string& uci) {
    MoveList legal;
    board.gen_legal(legal);
    for (Move move : legal) {
        if (move_to_uci(move) == uci) {
            board.make_move(move);
            return true;
        }
    }
    return false;
}

static bool info_pv_is_legal(const char* fen, const std::string& line) {
    const std::string marker = " pv ";
    const size_t pv_pos = line.find(marker);
    if (pv_pos == std::string::npos)
        return true;

    Board board;
    board.set_fen(fen);

    std::istringstream iss(line.substr(pv_pos + marker.size()));
    std::string token;
    while (iss >> token) {
        if (!apply_legal_uci(board, token))
            return false;
    }
    return true;
}

static std::vector<std::string> collect_info_lines(const char* fen, int depth) {
    TranspositionTable tt(4);
    std::atomic_bool stop{false};
    std::vector<std::string> lines;

    Board board;
    board.set_fen(fen);

    SearchLimits limits;
    limits.depth = depth;

    auto searcher = std::make_unique<Searcher>(tt, stop, [&](const std::string& info) {
        lines.push_back(info);
    });
    (void)searcher->search(board, limits);
    return lines;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_returns_legal_move() {
    // The engine must return a legal move for the starting position at depth 1.
    begin_section("startpos depth 1: returns valid move");
    auto rr = run_search("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1);
    EXPECT(rr.sr.bestmove != MOVE_NONE);
    end_section();

    // Verify the returned move is actually legal
    begin_section("startpos depth 1: move is legal");
    Board b;
    b.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    MoveList ml;
    b.gen_legal(ml);
    auto rr2 = run_search("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1);
    bool found = false;
    for (int i = 0; i < ml.size(); i++) {
        if (ml[i] == rr2.sr.bestmove) { found = true; break; }
    }
    EXPECT(found);
    end_section();

    begin_section("startpos depth 1: nodes > 0");
    EXPECT(rr.sr.nodes > 0);
    end_section();
}

static void test_mate_in_one() {
    // Position 1: "6k1/5ppp/8/8/8/8/5PPP/1R4K1 w - - 0 1"
    // White Rb1 can mate with Rb8#.  Black king on g8 is trapped by the rook
    // on the back rank and by its own pawns on f7/g7/h7.
    {
        const char* fen = "6k1/5ppp/8/8/8/8/5PPP/1R4K1 w - - 0 1";

        begin_section("mate-in-1 #1: finds Rb8#");
        auto rr = run_search(fen, 3);
        EXPECT_STR(rr.uci, "b1b8");
        end_section();

        begin_section("mate-in-1 #1: score >= MATE_SCORE - 10");
        EXPECT(rr.sr.score >= MATE_SCORE - 10);
        end_section();
    }

    // Position 2: "7k/5K2/6Q1/8/8/8/8/8 w - - 0 1"
    // White Qg6 / Kf7 vs Black Kh8.  There are multiple mating moves (Qh5, Qh6,
    // Qh7, etc.) because any queen move to the h-file (with Kf7 covering g7/g8)
    // delivers mate.  We just verify the engine finds *a* mate-in-1.
    {
        const char* fen = "7k/5K2/6Q1/8/8/8/8/8 w - - 0 1";

        begin_section("mate-in-1 #2: score >= MATE_SCORE - 10");
        auto rr = run_search(fen, 3);
        EXPECT(rr.sr.score >= MATE_SCORE - 10);
        end_section();

        begin_section("mate-in-1 #2: returns a move");
        EXPECT(rr.sr.bestmove != MOVE_NONE);
        end_section();
    }
}

static void test_free_queen_capture() {
    // "4k3/8/8/3q4/8/8/Q7/4K3 w - - 0 1"
    // White Qa2 can take the hanging black queen on d5 (Qxd5 = a2d5).
    // At depth 2 this should be clearly the best move.
    const char* fen = "4k3/8/8/3q4/8/8/Q7/4K3 w - - 0 1";

    begin_section("free queen capture: Qxd5 at depth 2");
    auto rr = run_search(fen, 2);
    EXPECT_STR(rr.uci, "a2d5");
    end_section();

    begin_section("free queen capture: large positive score");
    auto rr2 = run_search(fen, 2);
    EXPECT(rr2.sr.score > 500);
    end_section();
}

static void test_depth_limit() {
    // With depth=1 the result should not exceed depth 1
    begin_section("depth limit: result.depth == 1");
    auto rr = run_search("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1);
    EXPECT(rr.sr.depth >= 1);
    EXPECT(rr.sr.depth <= 2); // allow aspiration window iteration
    end_section();
}

static void test_nodes_limit() {
    // With a small node limit the search should still return a move
    begin_section("nodes limit: returns move with limit 100");
    auto rr = run_search(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        MAX_SEARCH_DEPTH, /*nodes=*/100);
    EXPECT(rr.sr.bestmove != MOVE_NONE);
    end_section();

    begin_section("nodes limit: nodes <= limit + safety margin");
    // Allow significant overshoot due to check-stop frequency (~1024-node granularity)
    EXPECT(rr.sr.nodes <= 2048);
    end_section();
}

static void test_score_near_zero_startpos() {
    // The starting position is roughly equal; at depth 5 the score should be
    // well within ±100 centipawns of 0.
    begin_section("startpos depth 5: |score| < 100");
    auto rr = run_search("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5);
    EXPECT(rr.sr.score >= -100 && rr.sr.score <= 100);
    end_section();
}

static void test_node_counter() {
    // After any search the node counter must be positive
    begin_section("node counter: > 0 after depth-2 search");
    auto rr = run_search("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2);
    EXPECT(rr.sr.nodes > 0);
    end_section();
}

static void test_only_move() {
    // Position where there is exactly one legal move.
    // "4k3/8/8/8/8/8/4Q3/4K3 b - - 0 1" — black king on e8, white queen on e2
    // and king on e1.  Black has very limited moves; any legal move is the only
    // choice.
    begin_section("only legal move: Ke8 to d7/c6 (not 0000)");
    auto rr = run_search("4k3/8/8/8/8/8/4Q3/4K3 b - - 0 1", 1);
    EXPECT(rr.sr.bestmove != MOVE_NONE);
    end_section();
}

static void test_black_to_move() {
    // Engine should work correctly when black is to move
    begin_section("black to move: returns legal move at depth 1");
    auto rr = run_search("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 1);
    EXPECT(rr.sr.bestmove != MOVE_NONE);
    EXPECT(rr.sr.nodes > 0);
    end_section();

    // Black has a free queen to take.
    // Use a position where only the black queen (not the king) can capture:
    // "4k3/8/2Q5/3q4/8/8/8/4K3 b - - 0 1" — White Qc6 vs Black Qd5/Ke8.
    // d5→c6 is one diagonal step — a legal queen move.
    // Ke8 is 2 files + 2 ranks from c6 — cannot capture it.
    begin_section("black: free queen capture at depth 2");
    auto rr2 = run_search("4k3/8/2Q5/3q4/8/8/8/4K3 b - - 0 1", 2);
    EXPECT_STR(rr2.uci, "d5c6");
    end_section();
}

static void test_shortest_mate_not_first_mate() {
    // KQK endgame where a longer checking mate is visible before the shorter
    // quiet mating net. The engine used to stop at depth 11 with Qc7-f4,
    // reporting mate in 8, instead of continuing to find Qc7-e5.
    const char* fen = "4K3/2Q5/6k1/8/8/8/8/8 w - - 0 1";

    begin_section("mate-distance: continues past first longer mate");
    auto rr = run_search(fen, 18);
    EXPECT_EQ(rr.sr.depth, 18);
    end_section();

    begin_section("mate-distance: returns a legal mating move");
    EXPECT(is_legal_bestmove(fen, rr.sr.bestmove));
    end_section();

    begin_section("mate-distance: resolves mate in 5 or better");
    EXPECT(rr.sr.score >= MATE_SCORE - 9);
    EXPECT(mate_in_from_score(rr.sr.score) <= 5);
    end_section();
}

static void test_search_result_sanitizer() {
    static constexpr const char* FEN =
        "8/6K1/8/8/8/p7/P7/1k6 b - - 4 71";

    Board board;
    board.set_fen(FEN);

    SearchResult bad;
    bad.bestmove = make_move(A3, A2);
    bad.pondermove = make_move(A2, A1);
    bad.depth = 12;
    bad.score = 1000;

    SearchResult safe = sanitize_search_result(board, bad);

    begin_section("search-result sanitizer: replaces illegal bestmove");
    EXPECT(safe.bestmove != MOVE_NONE);
    EXPECT(safe.bestmove != bad.bestmove);
    EXPECT(is_legal_bestmove(FEN, safe.bestmove));
    end_section();

    begin_section("search-result sanitizer: clears ponder after replacement");
    EXPECT(safe.pondermove == MOVE_NONE);
    end_section();
}

static void test_tournament_infraction_positions() {
    {
        static constexpr const char* FEN =
            "8/6K1/8/8/8/p7/P7/1k6 b - - 4 71";
        begin_section("rules-infraction final position #1: legal bestmove");
        auto rr = run_search(FEN, 10);
        EXPECT(is_legal_bestmove(FEN, rr.sr.bestmove));
        end_section();
    }

    {
        static constexpr const char* FEN =
            "8/8/8/K3R3/3Q4/8/6p1/2k4q w - - 26 91";
        begin_section("rules-infraction final position #2: legal bestmove");
        auto rr = run_search(FEN, 6);
        EXPECT(is_legal_bestmove(FEN, rr.sr.bestmove));
        end_section();
    }
}

static void test_info_pv_lines_are_legal() {
    static constexpr const char* FEN =
        "8/6K1/8/8/8/p7/P7/1k6 b - - 4 71";

    std::vector<std::string> lines = collect_info_lines(FEN, 10);

    begin_section("uci info: emits at least one pv line");
    bool saw_pv = false;
    for (const std::string& line : lines)
        saw_pv = saw_pv || line.find(" pv ") != std::string::npos;
    EXPECT(saw_pv);
    end_section();

    begin_section("uci info: pv lines remain legal from the root");
    for (const std::string& line : lines)
        EXPECT(info_pv_is_legal(FEN, line));
    end_section();
}

static void test_thread_pool_search() {
    static constexpr const char* FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    TranspositionTable tt(8);
    std::atomic_bool stop{false};
    SearchThreadPool pool(tt, stop);
    const int threads = pool.ensure_threads(2);

    begin_section("thread pool: creates at least one worker slot");
    EXPECT(threads >= 1);
    end_section();

    Board board;
    board.set_fen(FEN);
    SearchLimits limits;
    limits.depth = 5;

    stop.store(false, std::memory_order_release);
    SearchResult result = pool.search(board, limits, threads);

    begin_section("thread pool: returns legal move");
    EXPECT(is_legal_bestmove(FEN, result.bestmove));
    end_section();

    begin_section("thread pool: reports searched nodes");
    EXPECT(result.nodes > 0);
    end_section();

    board.set_fen(FEN);
    stop.store(false, std::memory_order_release);
    SearchResult second = pool.search(board, limits, threads);

    begin_section("thread pool: repeated search after helper cancellation works");
    EXPECT(is_legal_bestmove(FEN, second.bestmove));
    EXPECT(second.nodes > 0);
    end_section();
}

static void test_command_queue_priority() {
    EngineCommandQueue queue;
    queue.push(EngineCommand{EngineCommandType::Go, "depth 1", nullptr, 1});
    queue.push_priority(EngineCommand{EngineCommandType::Quit, {}, nullptr, 2});

    begin_section("command queue: priority command is popped first");
    EngineCommand first = queue.wait_pop();
    EXPECT(first.type == EngineCommandType::Quit);
    EXPECT_EQ(static_cast<int>(first.epoch), 2);
    end_section();

    begin_section("command queue: older command remains queued");
    EngineCommand second = queue.wait_pop();
    EXPECT(second.type == EngineCommandType::Go);
    EXPECT_EQ(static_cast<int>(second.epoch), 1);
    end_section();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Search tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nLegal move\n");
    test_returns_legal_move();

    std::printf("\nMate in 1\n");
    test_mate_in_one();

    std::printf("\nMate distance\n");
    test_shortest_mate_not_first_mate();

    std::printf("\nIllegal move hardening\n");
    test_search_result_sanitizer();
    test_tournament_infraction_positions();
    test_info_pv_lines_are_legal();

    std::printf("\nFree queen capture\n");
    test_free_queen_capture();

    std::printf("\nDepth limit\n");
    test_depth_limit();

    std::printf("\nNodes limit\n");
    test_nodes_limit();

    std::printf("\nScore near zero\n");
    test_score_near_zero_startpos();

    std::printf("\nNode counter\n");
    test_node_counter();

    std::printf("\nOnly move\n");
    test_only_move();

    std::printf("\nBlack to move\n");
    test_black_to_move();

    std::printf("\nThread pool\n");
    test_thread_pool_search();

    std::printf("\nCommand queue\n");
    test_command_queue_priority();

    return harness_summary();
}
