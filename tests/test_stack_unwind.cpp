/// The selectivity core's per-ply stack across whole searches:
/// twenty fixed positions searched to depth 8 leave the caller's board as it
/// was, answer with a legal best move, ponder move and line, and repeat
/// exactly in a fresh searcher; at Threads 4 the pool's answer is legal.
///
/// In builds with node checks (sanitizer and other non-NDEBUG builds, and
/// the Diag flavour) every node entry also verifies that the record's
/// per-node fields start at their defaults, so a field a search fails to
/// reset aborts this test.

#include "attacks.h"
#include "bench.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "search_root.h"
#include "search_thread_pool.h"
#include "search_worker.h"
#include "test_harness.h"
#include "tt.h"
#include "zobrist.h"

#include <atomic>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

static constexpr int POSITIONS = 20;
static constexpr int DEPTH = 8;

struct BoardImage {
    Key key;
    Key pawn_key;
    Bitboard occupied;
    Color turn;
    int rule50;
    size_t history;
};

static BoardImage image(const Board& b) {
    return {b.position_key(), b.pawn_key_value(), b.all_pieces(), b.turn(),
            b.rule50_count(), b.history_size()};
}

static bool same(const BoardImage& a, const BoardImage& b) {
    return a.key == b.key && a.pawn_key == b.pawn_key && a.occupied == b.occupied
        && a.turn == b.turn && a.rule50 == b.rule50 && a.history == b.history;
}

// Every move of the line is legal where it is played; the ponder move, when
// there is one, is legal after the best move.
static bool legal_answer(const Board& root, const SearchResult& r) {
    if (!is_legal_move_on_board(root, r.bestmove))
        return false;
    if (!r.pv.empty() && r.pv.front() != r.bestmove)
        return false;
    Board line = root;
    for (Move m : r.pv) {
        if (!is_legal_move_on_board(line, m))
            return false;
        line.make_move(m);
    }
    if (r.pondermove != MOVE_NONE) {
        Board after = root;
        after.make_move(r.bestmove);
        if (!is_legal_move_on_board(after, r.pondermove))
            return false;
    }
    return true;
}

static SearchResult fresh_search(const Board& b) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    auto searcher = std::make_unique<Searcher>(tt, stop);
    SearchLimits lim;
    lim.depth = DEPTH;
    return searcher->search(b, lim);
}

static void test_single_thread() {
    int unchanged = 0, legal = 0, repeated = 0;
    for (int i = 0; i < POSITIONS; ++i) {
        Board b;
        b.set_fen(std::string(bench_fens()[static_cast<size_t>(i)]));
        const BoardImage before = image(b);

        const SearchResult first = fresh_search(b);
        unchanged += same(before, image(b));
        legal += legal_answer(b, first);

        const SearchResult again = fresh_search(b);
        repeated += again.nodes == first.nodes && again.bestmove == first.bestmove
                 && again.pv == first.pv && again.score == first.score;
    }
    begin_section("1 thread: the caller's board is unchanged");
    EXPECT_EQ(unchanged, POSITIONS);
    end_section();
    begin_section("1 thread: best move, line and ponder move are legal");
    EXPECT_EQ(legal, POSITIONS);
    end_section();
    begin_section("1 thread: a fresh repeat matches in nodes, score and line");
    EXPECT_EQ(repeated, POSITIONS);
    end_section();
}

static void test_four_threads() {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    SearchThreadPool pool(tt, stop);
    const int threads = pool.resize_threads(4);

    int unchanged = 0, legal = 0;
    for (int i = 0; i < POSITIONS; ++i) {
        Board b;
        b.set_fen(std::string(bench_fens()[static_cast<size_t>(i)]));
        const BoardImage before = image(b);
        pool.clear();
        SearchLimits lim;
        lim.depth = DEPTH;
        const SearchResult r = pool.search(b, lim, threads);
        unchanged += same(before, image(b));
        legal += legal_answer(b, r);
    }
    begin_section("Threads 4: the caller's board is unchanged");
    EXPECT_EQ(unchanged, POSITIONS);
    end_section();
    begin_section("Threads 4: the pool's best move, line and ponder move are legal");
    EXPECT_EQ(legal, POSITIONS);
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Core stack unwind\n");
    std::printf("=================\n");
    test_single_thread();
    test_four_threads();
    return harness_summary();
}
