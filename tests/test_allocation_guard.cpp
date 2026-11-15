/// Per-node search code never allocates (agents/implementation.md, *Per-node
/// code*).
///
/// A counting global operator new watches whole searches of the same
/// positions at two depths, at Threads 1 and 4. Search setup and each
/// iteration may allocate a little: the root's records, the reported line.
/// Nothing per node may, so going deeper may add only a small fixed number of
/// allocations per extra iteration while the node count grows by more than an
/// order of magnitude. An allocation at every node, or at one node in a
/// few hundred, breaks that bound.
///
/// The counter is process-wide, so this binary runs its searches one after
/// another and holds no other test.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "search_thread_pool.h"
#include "test_harness.h"
#include "tt.h"
#include "zobrist.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

namespace {

std::atomic<uint64_t> g_allocations{0};

void* counted_malloc(std::size_t size) {
    g_allocations.fetch_add(1, std::memory_order_relaxed);
    return std::malloc(size ? size : 1);
}

void* counted_aligned(std::size_t size, std::align_val_t alignment) {
    g_allocations.fetch_add(1, std::memory_order_relaxed);
    const auto a = static_cast<std::size_t>(alignment);
    const std::size_t rounded = ((size ? size : 1) + a - 1) / a * a;
#ifdef _WIN32
    return _aligned_malloc(rounded, a);
#else
    return std::aligned_alloc(a, rounded);
#endif
}

void aligned_release(void* p) noexcept {
#ifdef _WIN32
    _aligned_free(p);
#else
    std::free(p);
#endif
}

}  // namespace

// Every replaceable allocation form counts once; the releases only forward.
void* operator new(std::size_t size) {
    if (void* p = counted_malloc(size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    if (void* p = counted_malloc(size)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return counted_malloc(size); }
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept { return counted_malloc(size); }
void* operator new(std::size_t size, std::align_val_t alignment) {
    if (void* p = counted_aligned(size, alignment)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    if (void* p = counted_aligned(size, alignment)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    return counted_aligned(size, alignment);
}
void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    return counted_aligned(size, alignment);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { aligned_release(p); }
void operator delete[](void* p, std::align_val_t) noexcept { aligned_release(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { aligned_release(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { aligned_release(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { aligned_release(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { aligned_release(p); }

namespace {

// Plies between the two searches; each extra ply is one more iteration.
constexpr int SHALLOW_DEPTH = 5;
constexpr int DEEP_DEPTH = 13;

// Allocations one iteration may add, per searching thread. Measured at most
// 15 at Threads 1 and 17 at Threads 4 over these positions (2026-11-15, six
// runs; the helpers' timing varies the second). The bound is nearly twice
// that and still catches an allocation at one node in about 650 in the least
// sensitive case (the start position at Threads 1, 88,410 deep nodes).
constexpr uint64_t ALLOCATIONS_PER_ITERATION_PER_THREAD = 32;

// The deep search must be much larger than the shallow one, or a per-node
// allocation could hide inside the budget.
constexpr int64_t MIN_NODE_GROWTH = 10;

// Bench positions of different kinds: the start, two middlegames, an ending.
constexpr const char* POSITIONS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r2qr1k1/p4ppp/1pn1bn2/2b1p3/4P3/1BN1BN2/PPP2PPP/R2QR1K1 b - - 6 10",
    "1kr2r2/1p1n1pp1/4p1p1/p2p2P1/3P3P/6P1/PPP3B1/1K1R1R2 b - - 0 10",
    "1R6/5k2/3ppp2/4p3/P7/1P4P1/2P2r2/1K6 w - - 0 16",
};

struct Measured {
    uint64_t allocations;
    int64_t nodes;
};

// One search from a cleared state, so an earlier search's table cannot shrink
// the tree. The board is built and the state cleared before the window opens.
Measured measured_search(SearchThreadPool& pool, TranspositionTable& tt, std::atomic_bool& stop,
                         const char* fen, int depth, int threads) {
    Board board;
    board.set_fen(fen);
    SearchLimits limits;
    limits.depth = depth;
    tt.clear();
    pool.clear();
    stop.store(false, std::memory_order_release);
    const uint64_t before = g_allocations.load(std::memory_order_relaxed);
    const SearchResult result = pool.search(board, limits, threads);
    const uint64_t allocations = g_allocations.load(std::memory_order_relaxed) - before;
    return {allocations, result.nodes};
}

void test_allocations_grow_per_iteration_never_per_node() {
    std::atomic_bool stop{false};
    TranspositionTable tt(16);
    SearchThreadPool pool(tt, stop);
    for (const int threads : {1, 4}) {
        const int active = pool.ensure_threads(threads);
        begin_section(threads == 1 ? "allocations: Threads 1, per iteration only"
                                   : "allocations: Threads 4, per iteration only");
        EXPECT_EQ(active, threads);
        for (const char* fen : POSITIONS) {
            // The first search pays one-time set-up (lazy tables, the helpers'
            // first jobs); it is not what this test is about.
            measured_search(pool, tt, stop, fen, SHALLOW_DEPTH, threads);
            const Measured shallow = measured_search(pool, tt, stop, fen, SHALLOW_DEPTH, threads);
            const Measured deep = measured_search(pool, tt, stop, fen, DEEP_DEPTH, threads);
            const uint64_t grown = deep.allocations > shallow.allocations
                                       ? deep.allocations - shallow.allocations : 0;
            const uint64_t budget = ALLOCATIONS_PER_ITERATION_PER_THREAD
                                  * uint64_t(DEEP_DEPTH - SHALLOW_DEPTH) * uint64_t(threads);
            std::printf("\n    depth %d: %llu allocations in %lld nodes; depth %d: %llu in %lld "
                        "(grown %llu of %llu) [%s]",
                        SHALLOW_DEPTH, static_cast<unsigned long long>(shallow.allocations),
                        static_cast<long long>(shallow.nodes), DEEP_DEPTH,
                        static_cast<unsigned long long>(deep.allocations),
                        static_cast<long long>(deep.nodes), static_cast<unsigned long long>(grown),
                        static_cast<unsigned long long>(budget), fen);
            // Too small a deep search could hide a per-node allocation.
            EXPECT(deep.nodes >= MIN_NODE_GROWTH * shallow.nodes);
            EXPECT(grown <= budget);
        }
        std::printf("\n  %-52s ", "");
        end_section();
    }
}

}  // namespace

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Allocation guard\n");
    std::printf("%s\n", std::string(62, '=').c_str());
    test_allocations_grow_per_iteration_never_per_node();
    return harness_summary();
}
