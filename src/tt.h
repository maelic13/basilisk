#pragma once

#include "types.h"
#include "move.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#if defined(_MSC_VER)
#include <xmmintrin.h>
#endif

enum TTFlag : uint8_t { TT_NONE=0, TT_EXACT=1, TT_ALPHA=2, TT_BETA=3 };

// Decoded TT entry. The table stores entries in a compact atomic format; this
// type is the stable interface used by search and tests.
struct TTEntry {
    uint16_t key16;
    int16_t  score;
    int16_t  static_eval;
    uint16_t move16;
    int8_t   depth;      // signed: allows sentinels at depth=-1
    uint8_t  flag_age;   // bits 0-1: TTFlag, bits 2-7: age (generation)
};

// Dense partial-key cluster (8.5.D1). Each entry is a 16-bit key fragment plus
// an 8-byte payload word (score16|eval16|move16|depth8|flag_age8) — 10 bytes,
// half the old 16-byte full-key/XOR slot, so a 32-byte cluster holds the same
// 3 entries and equal hash fits ~2x the clusters (and entries).
//
// Lock-free model (SF-style): the payload words stay 8-byte aligned so each is
// an atomic load/store; the key16 fragment is a separate 2-byte atomic. A read
// can only "tear" across the key16/payload pair, and a mismatched pair is
// harmless — the 1/65536 partial-key collision (or a stale pair under SMP) is
// caught downstream (illegal tt_move rejected by is_legal; bounds validated).
// Single-thread search is race-free. Old scheme detected torn writes; this one
// tolerates the rare harmless race instead, the price of the density.
struct alignas(32) TTCluster {
    std::atomic<uint64_t> data[3];    // payload words, 8-byte aligned (offsets 0/8/16)
    std::atomic<uint16_t> key16[3];   // partial keys (offsets 24/26/28)

    TTCluster() noexcept {
        for (int i = 0; i < 3; ++i) { data[i].store(0); key16[i].store(0); }
    }
    TTCluster(const TTCluster&) = delete;
    TTCluster& operator=(const TTCluster&) = delete;
};

// 8.6.2a: pin the density contract that the comment above AND resize()'s
// `bytes / sizeof(TTCluster)` math both depend on. Widening any field (say
// key16 -> uint32_t) would silently push the cluster to 64 bytes, halving the
// entries the user's Hash buys while every comment still claimed otherwise —
// the latent form of the sibling-engine bug where a table was sized in another
// structure's unit. Make it a build failure instead of a silent regression.
static_assert(sizeof(TTCluster) == 32,
              "TTCluster must stay 32 bytes (3 entries/cluster; resize() divides Hash by it)");
static_assert(alignof(TTCluster) == 32, "TTCluster must stay 32-byte aligned");

// Apple Silicon has 128-byte data-cache lines. Keep the logical 32-byte
// clusters (and therefore probing/replacement behaviour) unchanged, but group
// four adjacent clusters in one cache-line-aligned allocation unit. This
// prevents the first cluster from inheriting a weaker allocator alignment and
// makes every group of four clusters line-exact on Apple ARM64.
#if defined(__APPLE__) && defined(__aarch64__)
inline constexpr size_t TT_STORAGE_ALIGNMENT = 128;
inline constexpr size_t TT_CLUSTERS_PER_BLOCK = 4;
#else
inline constexpr size_t TT_STORAGE_ALIGNMENT = 32;
inline constexpr size_t TT_CLUSTERS_PER_BLOCK = 1;
#endif

struct alignas(TT_STORAGE_ALIGNMENT) TTStorageBlock {
    TTCluster clusters[TT_CLUSTERS_PER_BLOCK];
};

static_assert(sizeof(TTStorageBlock) == TT_STORAGE_ALIGNMENT,
              "TT storage blocks must occupy exactly one target cache line");
static_assert(alignof(TTStorageBlock) == TT_STORAGE_ALIGNMENT,
              "TT storage blocks must use the target cache-line alignment");

class TranspositionTable {
public:
    static constexpr int MATE_SCORE = 32000;
    static constexpr int MAX_PLY    = 128;
    static constexpr int INF_EVAL   = 32001;

    explicit TranspositionTable(size_t mb = 64) { resize(mb); }

    // Size the table from the byte budget the user asked for. The cluster
    // count floors to a power of two (the index is a mask), so the allocation
    // is always <= the budget and never less than half of it. Closing that gap
    // needs full-budget (multiply-hi) indexing — deferred, PLAN section 6.
    void resize(size_t mb) {
        size_t bytes = mb * 1024 * 1024;
        size_t count = bytes / sizeof(TTCluster);
        size_t power = 1;
        while (power * 2 <= count) power *= 2;

        // Free the old table BEFORE allocating the new one (8.6.2a). Plain
        // assignment keeps both alive across make_unique, so growing 8 -> 16 GB
        // transiently needed ~24 GB — at `setoption Hash` time, i.e. mid-game.
        // No entries are ever carried across a resize, so dropping first costs
        // nothing. (make_unique value-initializes, so the new table is clear.)
        blocks_.reset();
        const size_t block_count =
            std::max<size_t>(1, power / TT_CLUSTERS_PER_BLOCK);
        blocks_ = std::make_unique<TTStorageBlock[]>(block_count);
        cluster_count_ = power;
        mask_ = power - 1;
        age_.store(0, std::memory_order_relaxed);
    }

    // Allocated size in bytes — the `Hash` contract under test (8.6.2a):
    // allocated <= requested budget, and > budget/2 given the pow2 floor.
    [[nodiscard]] size_t allocated_bytes() const noexcept {
        return cluster_count_ * sizeof(TTCluster);
    }

    void clear() {
        for (size_t i = 0; i < cluster_count_; ++i) {
            for (int j = 0; j < 3; ++j) {
                cluster_at(i).data[j].store(0, std::memory_order_relaxed);
                cluster_at(i).key16[j].store(0, std::memory_order_relaxed);
            }
        }
        age_.store(0, std::memory_order_relaxed);
    }

    void new_search() {
        const uint8_t age = age_.load(std::memory_order_relaxed);
        age_.store((age + 4) & 0xFC, std::memory_order_relaxed);
    }

    [[nodiscard]] bool probe_copy(Key key, TTEntry& out) const {
        const TTCluster& cluster = cluster_at(key & mask_);
        const uint16_t want = static_cast<uint16_t>(key >> 48);

        for (int i = 0; i < 3; ++i) {
            if (cluster.key16[i].load(std::memory_order_acquire) != want)
                continue;
            const uint64_t data = cluster.data[i].load(std::memory_order_relaxed);
            TTEntry e = unpack_entry(data);
            if ((e.flag_age & 3) != TT_NONE) {   // reject an empty slot that hashes to want==0
                e.key16 = want;
                out = e;
                return true;
            }
        }

        out = TTEntry{};
        return false;
    }

    void prefetch(Key key) const {
        if (!blocks_)
            return;
        const void* addr = &cluster_at(key & mask_);
#if defined(_MSC_VER)
        _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(addr, 0, 3);
#else
        (void)addr;
#endif
    }

    // Returns true when the store landed on a slot that ALREADY held this
    // position's key (an update of our own or another thread's entry) rather
    // than evicting a different position (9.3c telemetry — the caller
    // accumulates it into DiagCounters; the return is free to ignore and the
    // stored data is identical either way).
    bool store(Key key, int depth, int score, TTFlag flag, Move m, int ply, int static_eval) {
        TTCluster& cluster = cluster_at(key & mask_);
        const uint16_t want = static_cast<uint16_t>(key >> 48);
        const uint8_t age = age_.load(std::memory_order_relaxed);

        int replace_idx = 0;
        TTEntry replace_entry{};
        bool have_replace = false;
        bool same_key = false;

        for (int i = 0; i < 3; i++) {
            const uint16_t old_key16 = cluster.key16[i].load(std::memory_order_relaxed);
            TTEntry old_entry = unpack_entry(cluster.data[i].load(std::memory_order_relaxed));
            old_entry.key16 = old_key16;

            if (old_key16 == want && (old_entry.flag_age & 3) != TT_NONE) {
                if (flag != TT_EXACT && depth < old_entry.depth - 3
                    && (old_entry.flag_age & 0xFC) == age)
                    return true;   // declined, but it WAS our own key

                same_key = true;
                replace_idx = i;
                replace_entry = old_entry;
                have_replace = true;
                break;
            }

            if (!have_replace || entry_quality(old_entry, age) < entry_quality(replace_entry, age)) {
                replace_idx = i;
                replace_entry = old_entry;
                have_replace = true;
            }
        }

        // Preserve the existing best move on a MOVE_NONE store into the same key.
        if (m == MOVE_NONE && replace_entry.key16 == want
            && (replace_entry.flag_age & 3) != TT_NONE)
            m = move_from_tt(replace_entry.move16);

        const uint64_t data = pack_entry(score_to_tt(score, ply),
                                         static_eval == INF_EVAL ? INF_EVAL : static_eval,
                                         m, depth, static_cast<uint8_t>(age | uint8_t(flag)));

        // Publish the payload before the key fragment so a concurrent reader
        // that matches key16 sees at least this store's payload (relaxed is
        // enough for single-thread; the SMP race is harmless as noted above).
        cluster.data[replace_idx].store(data, std::memory_order_relaxed);
        // Release, paired with the acquire on the key16 load in probe_copy
        // (8.6.2b / C9): this is what actually enforces "payload published
        // before key". Under the previous all-relaxed scheme that ordering was
        // only a comment - free on x86, where stores are already ordered, but on
        // the ARM targets we ship (Apple Silicon, ARM64 Windows) store-store
        // reordering could publish a key ahead of its payload, making a
        // mismatched pair more reachable than the 1/65536 collision figure
        // suggests. Still self-correcting downstream, so this is defence in
        // depth rather than a fix for an observed bug; it costs nothing on x86.
        cluster.key16[replace_idx].store(want, std::memory_order_release);
        return same_key;
    }

    [[nodiscard]] int hashfull() const {
        int count = 0;
        size_t sample = std::min(size_t(334), cluster_count_);
        const uint8_t age = age_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < sample; i++) {
            for (int j = 0; j < 3; ++j) {
                TTEntry e = unpack_entry(cluster_at(i).data[j].load(std::memory_order_relaxed));
                if ((e.flag_age & 3) != TT_NONE && (e.flag_age & 0xFC) == age)
                    count++;
            }
        }
        const int slots = static_cast<int>(sample * 3);
        return slots == 0 ? 0 : count * 1000 / slots;
    }

    // Score adjustments for mate scores stored relative to ply
    static int score_to_tt(int score, int ply) {
        if (score >=  MATE_SCORE - MAX_PLY) return score + ply;
        if (score <= -MATE_SCORE + MAX_PLY) return score - ply;
        return score;
    }

    static int score_from_tt(int score, int ply, int halfmove_clock = 0) {
        if (score >=  MATE_SCORE - MAX_PLY) return score - ply;
        if (score <= -MATE_SCORE + MAX_PLY) return score + ply;
        if (halfmove_clock >= 100)
            return 0;
        return score;
    }

private:
    std::unique_ptr<TTStorageBlock[]> blocks_;
    size_t cluster_count_ = 0;
    size_t mask_ = 0;
    std::atomic<uint8_t> age_{0};

    [[nodiscard]] TTCluster& cluster_at(size_t index) noexcept {
        return blocks_[index / TT_CLUSTERS_PER_BLOCK]
            .clusters[index % TT_CLUSTERS_PER_BLOCK];
    }

    [[nodiscard]] const TTCluster& cluster_at(size_t index) const noexcept {
        return blocks_[index / TT_CLUSTERS_PER_BLOCK]
            .clusters[index % TT_CLUSTERS_PER_BLOCK];
    }

    static uint64_t pack_entry(int score, int static_eval, Move move, int depth, uint8_t flag_age) {
        const auto score16 = static_cast<uint16_t>(static_cast<int16_t>(score));
        const auto eval16  = static_cast<uint16_t>(static_cast<int16_t>(static_eval));
        const auto move16  = static_cast<uint16_t>(move_to_tt(move));
        const auto depth8  = static_cast<uint8_t>(
            static_cast<int8_t>(std::clamp(depth, -1, 127)));

        return uint64_t(score16)
             | (uint64_t(eval16) << 16)
             | (uint64_t(move16) << 32)
             | (uint64_t(depth8) << 48)
             | (uint64_t(flag_age) << 56);
    }

    static TTEntry unpack_entry(uint64_t data) {
        TTEntry e{};
        e.key16       = 0;  // filled from the stored key16 fragment by the caller
        e.score       = static_cast<int16_t>(data & 0xFFFFu);
        e.static_eval = static_cast<int16_t>((data >> 16) & 0xFFFFu);
        e.move16      = static_cast<uint16_t>((data >> 32) & 0xFFFFu);
        e.depth       = static_cast<int8_t>((data >> 48) & 0xFFu);
        e.flag_age    = static_cast<uint8_t>((data >> 56) & 0xFFu);
        return e;
    }

    static int entry_quality(const TTEntry& e, uint8_t age) {
        if ((e.flag_age & 3) == TT_NONE)
            return -100000;

        int age_delta = int(age - (e.flag_age & 0xFC)) & 0xFC; // 0, 4, 8, ...
        return int(e.depth) - age_delta / 2 + (((e.flag_age & 3) == TT_EXACT) ? 2 : 0);
    }
};
