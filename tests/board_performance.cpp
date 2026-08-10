/// Board representation performance benchmark.
///
/// Measures throughput for the operations that dominate a chess-engine search
/// loop. The timed region must contain board work and nothing else: no heap
/// allocation, no container copy, no dead code.
///
/// Two profiles:
///   cross-engine-board-v1  (default) — 150 ms warm-up, 11 x 150 ms samples,
///                          median +/- MAD. Comparable across engines that
///                          implement the same contract and work counts.
///   legacy-board-a-v1      — the historical fixed-iteration schedule with the
///                          same 11-sample median/MAD estimator. Kept so older
///                          Basilisk numbers stay interpretable; never mix the
///                          two profiles in one comparison.
///
/// Run from the repo root:
///   cmake --preset release-pext && cmake --build --preset release-pext --target board_performance_test
///   ./build/release-pext/board_performance_test.exe [--profile legacy-board-a-v1] [--preflight-only]

#include "board.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

// ---------------------------------------------------------------------------
// Benchmark corpus — the cross-engine-board-v1 five positions, in order
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

// Frozen work quanta. The benchmark refuses to time anything that does not
// perform exactly this much work, so "same benchmark" is enforced rather than
// assumed. Order matches the results table.
static constexpr uint64_t EXPECTED_LEGAL_MOVES   = 128;
static constexpr uint64_t EXPECTED_CAPTURES      = 10;
static constexpr uint64_t EXPECTED_MAKE_UNMAKE   = 128;
static constexpr uint64_t EXPECTED_SEE           = 10;
static constexpr uint64_t EXPECTED_PERFT4        = 197281;
static constexpr uint64_t EXPECTED_SIMULATION    = 4597;

// ---------------------------------------------------------------------------
// Dead-code-elimination barriers
// ---------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
template<typename T>
inline void do_not_optimize(T const& val) {
    asm volatile("" : : "r,m"(val) : "memory");
}
// Large aggregates (MoveList is 256 slots) cannot go through an "r" constraint.
// Feeding the address with a memory clobber keeps the stores to the list alive
// without copying it, which is what a by-value barrier would have cost.
inline void keep_alive(const void* p) {
    asm volatile("" : : "r"(p) : "memory");
}
#else
template<typename T>
inline void do_not_optimize(T const& val) { (void)val; }
inline void keep_alive(const void* p) { (void)p; }
#endif

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
// Workloads
//
// Every list is a reused stack MoveList reset with `count = 0`. Nothing here
// returns a MoveList by value: the previous version did, which put a 1 KB
// aggregate copy inside the timed region of the two generation workloads and
// measured the copy as if it were move generation.
// ---------------------------------------------------------------------------

static uint64_t legal_moves_workload(const std::vector<Board>& boards, MoveList& ml) {
    uint64_t total = 0;
    for (const Board& b : boards) {
        ml.count = 0;
        b.gen_legal(ml);
        total += static_cast<uint64_t>(ml.size());
        keep_alive(&ml);
    }
    return total;
}

static uint64_t capture_gen_workload(const std::vector<Board>& boards, MoveList& ml) {
    uint64_t total = 0;
    for (const Board& b : boards) {
        ml.count = 0;
        b.gen_legal_captures(ml);
        total += static_cast<uint64_t>(ml.size());
        keep_alive(&ml);
    }
    return total;
}

static uint64_t make_unmake_workload(std::vector<Board>& boards, MoveList& ml) {
    uint64_t ops = 0;
    for (Board& b : boards) {
        ml.count = 0;
        b.gen_legal(ml);
        for (Move m : ml) {
            b.make_move(m);
            do_not_optimize(b.all_occ);
            b.unmake_move(m);
            ++ops;
        }
    }
    return ops;
}

// Threshold SEE (`see_ge(m, 0)`) over every legal capture — the pruning-hot
// form, not the full-value variant used only for move ordering.
static uint64_t see_workload(const std::vector<Board>& boards, MoveList& ml) {
    uint64_t ops = 0;
    for (const Board& b : boards) {
        ml.count = 0;
        b.gen_legal_captures(ml);
        for (Move m : ml) {
            do_not_optimize(b.see_ge(m, 0));
            ++ops;
        }
    }
    return ops;
}

static uint64_t game_simulation_workload(std::vector<Board>& boards,
                                         MoveList& outer, MoveList& inner) {
    uint64_t ops = 0;
    for (Board& b : boards) {
        outer.count = 0;
        b.gen_legal(outer);
        for (Move m : outer) {
            b.make_move(m);
            inner.count = 0;
            b.gen_legal(inner);
            ops += static_cast<uint64_t>(inner.count);
            keep_alive(&inner);
            b.unmake_move(m);
        }
    }
    return ops;
}

// ---------------------------------------------------------------------------
// Harness
// ---------------------------------------------------------------------------

static constexpr std::size_t SAMPLES = 11;
static constexpr auto WARMUP_TIME = std::chrono::milliseconds(150);
static constexpr auto SAMPLE_TIME = std::chrono::milliseconds(150);

struct BenchResult {
    const char* label;
    const char* unit;
    double      median_ops_per_sec;
    double      mad_ops_per_sec;
    uint64_t    ops_per_iter;
    uint64_t    iterations;

    double ops_per_sec() const { return median_ops_per_sec; }
};

static double median_of(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

static BenchResult summarize(const char* label, const char* unit,
                             std::vector<double>& rate,
                             uint64_t ops_per_iter, uint64_t iterations) {
    const double med = median_of(rate);
    std::vector<double> dev(rate.size());
    for (std::size_t s = 0; s < rate.size(); ++s) dev[s] = std::fabs(rate[s] - med);
    return { label, unit, med, median_of(dev), ops_per_iter, iterations };
}

// Batch size is calibrated so the clock is read roughly once per millisecond.
// A steady_clock read costs tens of nanoseconds; the 10-op workloads run an
// iteration in ~100 ns, so checking the deadline once per iteration would put
// a double-digit percentage of clock overhead inside the timed region and
// publish it as board throughput.
static constexpr double BATCH_TARGET_SECONDS = 0.001;
static constexpr uint64_t MAX_BATCH = 1000000;

template <typename F>
static uint64_t calibrate_batch(F workload) {
    const auto c0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 32; ++i) do_not_optimize(workload());
    const auto c1 = std::chrono::steady_clock::now();
    const double per_iter = std::chrono::duration<double>(c1 - c0).count() / 32.0;
    if (per_iter <= 0) return MAX_BATCH;
    const double batch = BATCH_TARGET_SECONDS / per_iter;
    if (batch <= 1.0) return 1;
    if (batch >= static_cast<double>(MAX_BATCH)) return MAX_BATCH;
    return static_cast<uint64_t>(batch);
}

// cross-engine-board-v1: fixed wall-clock per sample, so every workload gets
// the same measurement budget regardless of how fast it is.
template <typename F>
static BenchResult benchmark_timed(const char* label, const char* unit,
                                   uint64_t expected_ops, F workload) {
    const auto warm0 = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - warm0 < WARMUP_TIME)
        do_not_optimize(workload());

    const uint64_t batch = calibrate_batch(workload);

    std::vector<double> rate(SAMPLES);
    uint64_t iterations = 0;
    for (std::size_t s = 0; s < SAMPLES; ++s) {
        uint64_t ops = 0;
        uint64_t iters = 0;
        const auto t0 = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - t0 < SAMPLE_TIME) {
            for (uint64_t i = 0; i < batch; ++i) ops += workload();
            iters += batch;
        }
        const auto t1 = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(t1 - t0).count();
        rate[s] = static_cast<double>(ops) / elapsed;
        iterations += iters;
        if (ops != expected_ops * iters) {
            std::fprintf(stderr, "FAIL: %s performed %llu ops, expected %llu\n",
                label, (unsigned long long)ops,
                (unsigned long long)(expected_ops * iters));
            std::exit(EXIT_FAILURE);
        }
    }
    return summarize(label, unit, rate, expected_ops, iterations);
}

// legacy-board-a-v1: the historical fixed-iteration schedule.
template <typename F>
static BenchResult benchmark_fixed(const char* label, const char* unit,
                                   int iterations, int warmups,
                                   uint64_t expected_ops, F workload) {
    for (int i = 0; i < warmups; ++i)
        do_not_optimize(workload());

    std::vector<double> rate(SAMPLES);
    for (std::size_t s = 0; s < SAMPLES; ++s) {
        uint64_t ops = 0;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i)
            ops += workload();
        const auto t1 = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(t1 - t0).count();
        rate[s] = static_cast<double>(ops) / elapsed;
    }
    return summarize(label, unit, rate, expected_ops,
                     static_cast<uint64_t>(iterations) * SAMPLES);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    bool legacy = false;
    bool preflight_only = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--preflight-only") == 0) {
            preflight_only = true;
        } else if (std::strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            ++i;
            if (std::strcmp(argv[i], "legacy-board-a-v1") == 0) legacy = true;
            else if (std::strcmp(argv[i], "cross-engine-board-v1") != 0) {
                std::fprintf(stderr, "unknown profile: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
        } else {
            std::fprintf(stderr,
                "usage: board_performance_test [--profile cross-engine-board-v1|"
                "legacy-board-a-v1] [--preflight-only]\n");
            return EXIT_FAILURE;
        }
    }

    init_bitboards();
    init_attacks();
    Zobrist::init();

    std::vector<Board> boards;
    boards.reserve(N_POSITIONS);
    for (const auto& pos : BENCHMARK_POSITIONS) {
        Board b;
        b.set_fen(pos.fen);
        boards.push_back(b);
    }

    // Reused scratch lists, created once outside every timed region.
    MoveList scratch, outer, inner;

    Board start;
    start.set_fen(BENCHMARK_POSITIONS[0].fen);

    // ---- preflight: prove the work quanta before timing anything ----
    {
        struct Check { const char* name; uint64_t got; uint64_t want; };
        const Check checks[] = {
            { "legal moves",       legal_moves_workload(boards, scratch),        EXPECTED_LEGAL_MOVES },
            { "legal captures",    capture_gen_workload(boards, scratch),        EXPECTED_CAPTURES },
            { "make/unmake",       make_unmake_workload(boards, scratch),        EXPECTED_MAKE_UNMAKE },
            { "threshold SEE",     see_workload(boards, scratch),                EXPECTED_SEE },
            { "perft(4) startpos", perft(start, 4),                              EXPECTED_PERFT4 },
            { "two-ply simulation",game_simulation_workload(boards, outer, inner),EXPECTED_SIMULATION },
        };
        bool ok = true;
        for (const Check& c : checks) {
            if (c.got != c.want) {
                std::fprintf(stderr, "work mismatch for %s: expected %llu, received %llu\n",
                    c.name, (unsigned long long)c.want, (unsigned long long)c.got);
                ok = false;
            }
        }
        if (!ok) return EXIT_FAILURE;
    }

    std::printf("\nBasilisk board benchmark\n");
    std::printf("profile: %s\n", legacy ? "legacy-board-a-v1" : "cross-engine-board-v1");
    std::printf("positions: %d\n", N_POSITIONS);
    std::printf("samples: %s\n", legacy
        ? "11 fixed-work samples (median +/- MAD)"
        : "11 x 150 ms after a 150 ms warm-up (median +/- MAD)");
    std::printf("preflight: PASS\n");
    if (preflight_only) return EXIT_SUCCESS;

    std::vector<BenchResult> results;
    if (legacy) {
        results.push_back(benchmark_fixed("legal moves", "moves", 5000, 50,
            EXPECTED_LEGAL_MOVES, [&]() { return legal_moves_workload(boards, scratch); }));
        results.push_back(benchmark_fixed("legal captures", "moves", 10000, 100,
            EXPECTED_CAPTURES, [&]() { return capture_gen_workload(boards, scratch); }));
        results.push_back(benchmark_fixed("make/unmake", "moves", 2000, 20,
            EXPECTED_MAKE_UNMAKE, [&]() { return make_unmake_workload(boards, scratch); }));
        results.push_back(benchmark_fixed("threshold SEE", "captures", 20000, 200,
            EXPECTED_SEE, [&]() { return see_workload(boards, scratch); }));
        results.push_back(benchmark_fixed("perft(4) startpos", "nodes", 30, 3,
            EXPECTED_PERFT4, [&]() { return perft(start, 4); }));
        results.push_back(benchmark_fixed("two-ply simulation", "moves", 300, 10,
            EXPECTED_SIMULATION, [&]() { return game_simulation_workload(boards, outer, inner); }));
    } else {
        results.push_back(benchmark_timed("legal moves", "moves",
            EXPECTED_LEGAL_MOVES, [&]() { return legal_moves_workload(boards, scratch); }));
        results.push_back(benchmark_timed("legal captures", "moves",
            EXPECTED_CAPTURES, [&]() { return capture_gen_workload(boards, scratch); }));
        results.push_back(benchmark_timed("make/unmake", "moves",
            EXPECTED_MAKE_UNMAKE, [&]() { return make_unmake_workload(boards, scratch); }));
        results.push_back(benchmark_timed("threshold SEE", "captures",
            EXPECTED_SEE, [&]() { return see_workload(boards, scratch); }));
        results.push_back(benchmark_timed("perft(4) startpos", "nodes",
            EXPECTED_PERFT4, [&]() { return perft(start, 4); }));
        results.push_back(benchmark_timed("two-ply simulation", "moves",
            EXPECTED_SIMULATION, [&]() { return game_simulation_workload(boards, outer, inner); }));
    }

    std::printf("\n%-22s %15s %15s %10s %12s %12s\n",
        "workload", "estimate ops/s", "MAD ops/s", "MAD %", "ops/iter", "total iters");
    for (const BenchResult& r : results) {
        const double madpct = r.median_ops_per_sec > 0
            ? 100.0 * r.mad_ops_per_sec / r.median_ops_per_sec : 0.0;
        std::printf("%-22s %15.0f %15.0f %9.2f%% %12llu %12llu %s\n",
            r.label, r.median_ops_per_sec, r.mad_ops_per_sec, madpct,
            (unsigned long long)r.ops_per_iter,
            (unsigned long long)r.iterations, r.unit);
    }
    std::printf("\n");

    // ---- generous floors, meaningful even in a debug build ----
    bool ok = true;
    const double floors[] = { 1000.0, 1000.0, 100.0, 1000.0, 200.0, 50.0 };
    for (std::size_t i = 0; i < results.size(); ++i) {
        if (results[i].ops_per_sec() < floors[i]) {
            std::fprintf(stderr, "FAIL: %s too slow: %.0f ops/s (min %g)\n",
                results[i].label, results[i].ops_per_sec(), floors[i]);
            ok = false;
        }
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
