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

#include "board.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <chrono>
#include <cmath>
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
    double      median_ops_per_sec;
    double      mad_ops_per_sec;   // median absolute deviation (dispersion)
    uint64_t    ops_per_iter;      // work quantum, for context

    double ops_per_sec() const { return median_ops_per_sec; }
};

// Warm-up, then take SAMPLES timed runs and report the median throughput plus
// the median absolute deviation (a robust dispersion measure, unlike best-of-N
// which hides variance). The workload returns the op count it performed; it
// must NOT copy or allocate the working set inside the timed region.
static constexpr std::size_t SAMPLES = 11;

template <typename F>
static BenchResult benchmark(const char* label, int iterations, int warmups, F workload) {
    for (int i = 0; i < warmups; ++i)
        do_not_optimize(workload());

    std::vector<double> rate(SAMPLES);
    uint64_t ops_per_iter = 0;
    for (std::size_t s = 0; s < SAMPLES; ++s) {
        uint64_t ops = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i)
            ops += workload();
        auto t1 = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(t1 - t0).count();
        rate[s] = static_cast<double>(ops) / elapsed;
        ops_per_iter = ops / static_cast<uint64_t>(iterations);
    }

    auto median_of = [](std::vector<double> v) {
        std::sort(v.begin(), v.end());
        return v[v.size() / 2];
    };
    const double med = median_of(rate);
    std::vector<double> dev(SAMPLES);
    for (std::size_t s = 0; s < SAMPLES; ++s) dev[s] = std::fabs(rate[s] - med);
    const double mad = median_of(dev);

    return { label, med, mad, ops_per_iter };
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

// Takes the working set by REFERENCE and restores it via unmake, so no Board
// is copied inside the timed region (the by-value version copied the whole
// vector on every call, which dominated the measurement).
static uint64_t make_unmake_workload(std::vector<Board>& boards) {
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

// Replaces the old trivial cached `is_in_check()` read with a real workload:
// evaluate see_ge (the pruning-critical static exchange) over every legal
// capture — a hot search operation with genuine branching and X-ray recompute.
static uint64_t see_workload(const std::vector<Board>& boards) {
    uint64_t ops = 0;
    MoveList caps;
    for (const Board& b : boards) {
        caps.count = 0;
        b.gen_legal_captures(caps);
        for (Move m : caps) {
            do_not_optimize(b.see_ge(m, 0));
            ++ops;
        }
    }
    return ops;
}

static uint64_t game_simulation_workload(std::vector<Board>& boards) {
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

    auto see   = benchmark("see_ge captures", 20000, 200,
        [&]() { return see_workload(boards); });

    Board start;
    start.set_fen(BENCHMARK_POSITIONS[0].fen);
    auto pft   = benchmark("perft(4) startpos", 30, 3,
        [&]() { return perft(start, 4); });

    auto sim   = benchmark("game simulation",  300, 10,
        [&]() { return game_simulation_workload(boards); });

    // ---- print results ----

    const BenchResult* results[] = { &legal, &caps, &mku, &see, &pft, &sim };

    std::printf("\n");
    std::printf("Board representation performance (%d positions, median of %zu)\n",
                N_POSITIONS, SAMPLES);
    std::printf("%s\n", std::string(72, '-').c_str());
    for (const BenchResult* r : results) {
        const double madpct = r->median_ops_per_sec > 0
            ? 100.0 * r->mad_ops_per_sec / r->median_ops_per_sec : 0.0;
        std::printf("%-20s %14.0f ops/s  +/- %5.1f%%  (%llu ops/iter)\n",
            r->label, r->median_ops_per_sec, madpct,
            (unsigned long long)r->ops_per_iter);
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
    ASSERT_MIN(see,   1'000.0,  "see_ge captures");
    ASSERT_MIN(pft,     200.0,  "perft(4)");
    ASSERT_MIN(sim,      50.0,  "game simulation");

#undef ASSERT_MIN

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
