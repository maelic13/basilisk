/// Board representation performance benchmark.
///
/// Mirrors the beast/hydra/whitespine pattern: measure throughput for the
/// operations that dominate a chess-engine search loop, assert generous
/// minimums that should hold even in a debug build, and print a formatted
/// results table so release-mode numbers are easy to read at a glance.
///
/// Run from the repo root:
///   cmake --preset release && cmake --build --preset release --target board_performance_test
///   ./build/release/board_performance_test
///
/// Or via CTest:
///   ctest --test-dir build/release -R board_performance --output-on-failure

#include "Board.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

// ---------------------------------------------------------------------------
// Benchmark positions — same set used by beast / hydra / whitespine
// ---------------------------------------------------------------------------

struct Position {
    const char* label;
    const char* fen;
};

static constexpr Position BENCHMARK_POSITIONS[] = {
    { "startpos",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" },
    { "kiwipete",
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" },
    { "midgame",
      "rnbq1k1r/pppp1ppp/4pn2/8/1b1PP3/2N2N2/PPP2PPP/R1BQKB1R w KQ - 2 5" },
    { "endgame",
      "8/2p5/3p4/KP5r/8/8/8/7k w - - 0 1" },
    { "in-check",
      "rnbqkb1r/pppp1ppp/5n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 3 3" },
};

static constexpr int N_POSITIONS =
    static_cast<int>(sizeof(BENCHMARK_POSITIONS) / sizeof(BENCHMARK_POSITIONS[0]));

// ---------------------------------------------------------------------------
// Helper — collect legal moves into a MoveList
// ---------------------------------------------------------------------------

static MoveList legal_moves_list(const Board& b) {
    MoveList ml;
    b.gen_legal(ml);
    return ml;
}

static MoveList legal_captures_list(const Board& b) {
    MoveList ml;
    b.gen_legal_captures(ml);
    return ml;
}

// ---------------------------------------------------------------------------
// Perft — needed by the benchmark, not built into Board
// ---------------------------------------------------------------------------

static uint64_t perft(Board& b, int depth) {
    if (depth == 0) return 1;

    MoveList ml;
    b.gen_legal(ml);

    if (depth == 1) return static_cast<uint64_t>(ml.size());

    uint64_t nodes = 0;
    for (Move m : ml) {
        b.make_move(m);
        nodes += perft(b, depth - 1);
        b.unmake_move(m);
    }
    return nodes;
}

// ---------------------------------------------------------------------------
// Benchmark harness — identical logic to beast / whitespine
// ---------------------------------------------------------------------------

// Prevent dead-code elimination of computed values (GCC/Clang)
#if defined(__GNUC__) || defined(__clang__)
template<typename T>
inline void do_not_optimize(T const& val) {
    asm volatile("" : : "r,m"(val) : "memory");
}
#else
// MSVC: volatile read/write is sufficient
template<typename T>
inline void do_not_optimize(T const& val) { (void)val; }
#endif

struct BenchResult {
    const char* label;
    uint64_t    ops;
    double      secs;

    double ops_per_sec() const { return static_cast<double>(ops) / secs; }
};

template <typename F>
static BenchResult benchmark(const char* label, int iterations, int warmups, F workload) {
    for (int i = 0; i < warmups; ++i)
        workload();

    double   best_secs = 1e18;
    uint64_t best_ops  = 0;

    for (int run = 0; run < 3; ++run) {
        uint64_t ops = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i)
            ops += workload();
        auto t1 = std::chrono::steady_clock::now();
        double elapsed =
            std::chrono::duration<double>(t1 - t0).count();
        if (elapsed < best_secs) {
            best_secs = elapsed;
            best_ops  = ops;
        }
    }

    return { label, best_ops, best_secs };
}

// ---------------------------------------------------------------------------
// Workloads
// ---------------------------------------------------------------------------

static uint64_t legal_moves_workload(const std::vector<Board>& boards) {
    uint64_t total = 0;
    for (const Board& b : boards)
        total += static_cast<uint64_t>(legal_moves_list(b).size());
    return total;
}

static uint64_t capture_gen_workload(const std::vector<Board>& boards) {
    uint64_t total = 0;
    for (const Board& b : boards)
        total += static_cast<uint64_t>(legal_captures_list(b).size());
    return total;
}

static uint64_t make_unmake_workload(std::vector<Board> boards) {
    uint64_t ops = 0;
    MoveList moves;
    for (Board& b : boards) {
        moves.count = 0;
        b.gen_legal(moves);
        for (Move m : moves) {
            b.make_move(m);
            b.unmake_move(m);
            ++ops;
        }
    }
    return ops;
}

static uint64_t check_detection_workload(const std::vector<Board>& boards) {
    uint64_t ops = 0;
    for (const Board& b : boards) {
        do_not_optimize(b.is_in_check());
        ++ops;
    }
    return ops;
}

static uint64_t game_simulation_workload(std::vector<Board> boards) {
    uint64_t ops = 0;
    MoveList outer, inner;
    for (Board& b : boards) {
        outer.count = 0;
        b.gen_legal(outer);
        for (Move m : outer) {
            b.make_move(m);
            inner.count = 0;
            b.gen_legal(inner);
            ops += static_cast<uint64_t>(inner.count);
            b.unmake_move(m);
        }
    }
    return ops;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // One-time engine init (magic bitboards, Zobrist keys)
    init_bitboards();
    init_attacks();
    Zobrist::init();

    // Sanity-check perft at each depth to pinpoint any move generation bug
    {
        const uint64_t expected[] = { 0, 20, 400, 8902, 197281 };
        bool perft_ok = true;
        for (int d = 1; d <= 4; ++d) {
            Board b;
            uint64_t nodes = perft(b, d);
            if (nodes != expected[d]) {
                std::fprintf(stderr,
                    "perft(%d) = %llu, expected %llu\n",
                    d, (unsigned long long)nodes, (unsigned long long)expected[d]);
                perft_ok = false;
            }
        }
        if (!perft_ok) return EXIT_FAILURE;
    }

    std::vector<Board> boards;
    boards.reserve(N_POSITIONS);
    for (const auto& pos : BENCHMARK_POSITIONS) {
        Board b;
        b.set_fen(pos.fen);
        boards.push_back(b);
    }

    // ---- run benchmarks ----

    auto legal = benchmark("legal moves",    5000, 50,
        [&]() { return legal_moves_workload(boards); });

    auto caps  = benchmark("captures",       10000, 100,
        [&]() { return capture_gen_workload(boards); });

    auto mku   = benchmark("make/unmake",    2000, 20,
        [&]() { return make_unmake_workload(boards); });

    auto chk   = benchmark("check detection", 500000, 5000,
        [&]() { return check_detection_workload(boards); });

    Board start;
    auto pft   = benchmark("perft(4) startpos", 30, 3,
        [&]() -> uint64_t {
            Board b;
            return perft(b, 4);
        });

    auto sim   = benchmark("game simulation",  300, 10,
        [&]() { return game_simulation_workload(boards); });

    // ---- print results ----

    const BenchResult* results[] = { &legal, &caps, &mku, &chk, &pft, &sim };

    std::printf("\n");
    std::printf("Board representation performance (%d positions)\n", N_POSITIONS);
    std::printf("%s\n", std::string(65, '-').c_str());
    for (const BenchResult* r : results) {
        std::printf("%-22s %12.0f ops/s  (%9llu ops in %.3fs)\n",
            r->label,
            r->ops_per_sec(),
            (unsigned long long)r->ops,
            r->secs);
    }
    std::printf("\n");

    // ---- assert generous minimums ----
    // These are conservative enough to pass even in a debug build;
    // release-mode numbers will be 10-100× higher.

    bool ok = true;

#define ASSERT_MIN(result, min, name)                                       \
    do {                                                                    \
        if ((result).ops_per_sec() < (min)) {                              \
            std::fprintf(stderr, "FAIL: %s too slow: %.0f ops/s (min %g)\n", \
                (name), (result).ops_per_sec(), (double)(min));            \
            ok = false;                                                     \
        }                                                                   \
    } while (0)

    ASSERT_MIN(legal, 1'000.0,  "legal movegen");
    ASSERT_MIN(caps,  1'000.0,  "capture gen");
    ASSERT_MIN(mku,     100.0,  "make/unmake");
    ASSERT_MIN(chk,   5'000.0,  "check detection");
    ASSERT_MIN(pft,     200.0,  "perft(4)");
    ASSERT_MIN(sim,      50.0,  "game simulation");

#undef ASSERT_MIN

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
