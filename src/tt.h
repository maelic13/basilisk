#pragma once

#include "constants.h"
#include "large_pages.h"
#include "types.h"
#include "move.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <thread>
#include <utility>
#include <vector>
#if defined(_MSC_VER)
#include <xmmintrin.h>
#endif

// The bound a stored score holds. Scoped, so a bound never compares with an
// integer by accident; the TT_* names stay as its values.
enum class TTFlag : uint8_t { None = 0, Exact = 1, Alpha = 2, Beta = 3 };
inline constexpr TTFlag TT_NONE  = TTFlag::None;
inline constexpr TTFlag TT_EXACT = TTFlag::Exact;
inline constexpr TTFlag TT_ALPHA = TTFlag::Alpha;
inline constexpr TTFlag TT_BETA  = TTFlag::Beta;

// The flag_age byte: bits 0-1 hold the bound, the rest the age (generation).
// new_search() advances the age by TT_AGE_STEP, so the bound bits are never
// touched. A kernel that persists more in this byte takes its bits from the
// age field and changes these values together.
// The selectivity core persists the tt_pv flag in bit 2, leaving five age bits
// (a cycle of 32 searches).
inline constexpr uint8_t TT_BOUND_MASK = 0x03;
inline constexpr uint8_t TT_PV_MASK    = 0x04;
inline constexpr uint8_t TT_AGE_MASK   = 0xF8;
inline constexpr uint8_t TT_AGE_STEP   = 0x08;
// Stored depths carry this offset, so a payload of zero (an empty slot) decodes
// below every real depth and occupancy is a non-zero depth byte. An entry that
// holds only a static evaluation is stored at TT_DEPTH_UNSEARCHED with no bound.
inline constexpr int TT_DEPTH_OFFSET = 2;
inline constexpr int TT_DEPTH_UNSEARCHED = -1;

// Decoded TT entry. The table stores entries in a compact atomic format; this
// type is the stable interface used by search and tests.
struct TTEntry {
    uint16_t key16;
    int16_t  score;
    int16_t  static_eval;
    uint16_t move16;
    int8_t   depth;      // signed: allows sentinels at depth=-1
    uint8_t  flag_age;   // bound in the low bits, age above (TT_BOUND_MASK, TT_AGE_MASK)

    [[nodiscard]] TTFlag  bound() const noexcept { return TTFlag(flag_age & TT_BOUND_MASK); }
    [[nodiscard]] uint8_t age() const noexcept { return uint8_t(flag_age & TT_AGE_MASK); }
    [[nodiscard]] bool    is_pv() const noexcept { return (flag_age & TT_PV_MASK) != 0; }
};

// Dense partial-key cluster. Each entry is a 16-bit key fragment plus an
// 8-byte payload word (score16|eval16|move16|depth8|flag_age8), 10 bytes, so a
// 32-byte cluster holds 3 entries and a given Hash buys twice the entries of
// a full-key slot.
//
// Lock-free model (Stockfish's): the payload words stay 8-byte aligned so each
// is an atomic load/store; the key16 fragment is a separate 2-byte atomic.
// Single-thread search is race-free.
//
// A KNOWN DEFECT, not a harmless race (BAS-C05, decided 2026-09-07). The key and
// the payload are two publications, so under SMP a reader can match its key
// against one store and consume the score/depth/bound of another position's
// store. Nothing downstream catches it: negamax's TT cutoff returns the stored
// score as the node value on a depth-and-bound match with no verification, and
// its singular multi-cut returns a bound derived from it. The worst case is a
// foreign mate score reaching the root. An earlier claim here that the
// mismatch was "harmless, bounds validated" was false and is what BAS-C05
// corrected.
//
// It is tolerated until a repair costs no strength; both so far cost more than
// the defect. Binding the pair with a `key16 ^ fold16(payload)` tag measured
// -3.91% NPS at identical nodes; packing the whole validated record into one
// atomic word measured -1.22% (about -2.4 Elo at BAS-P01's ratio). Coherence
// cannot be free at this density: key+score+eval+depth+flag+move needs 80 bits
// and the word holds 64, so the repair needs a different layout. Stockfish
// ships this same tolerance in this same structure.
struct alignas(32) TTCluster {
    std::atomic<uint64_t> data[3]{};  // payload words, 8-byte aligned (offsets 0/8/16)
    std::atomic<uint16_t> key16[3]{}; // partial keys (offsets 24/26/28)

    // Value-initialised: a construction is plain stores of zero, which the
    // table's clear runs on several threads.
    TTCluster() noexcept = default;
    TTCluster(const TTCluster&) = delete;
    TTCluster& operator=(const TTCluster&) = delete;
};

// The density contract the comment above and resize()'s
// `bytes / sizeof(TTCluster)` both depend on. Widening any field (say key16 to
// uint32_t) would silently push the cluster to 64 bytes and halve the entries
// the user's Hash buys while every comment still claimed otherwise; make it a
// build failure instead.
static_assert(sizeof(TTCluster) == 32,
              "TTCluster must stay 32 bytes (3 entries/cluster; resize() divides Hash by it)");
static_assert(alignof(TTCluster) == 32, "TTCluster must stay 32-byte aligned");

class TranspositionTable {
public:
    static constexpr size_t kMaxReservableClusters = (size_t{1} << 57) / sizeof(TTCluster);
    // The ceiling resize() enforces, in clusters: every platform's address
    // space. Tests lower it to make a refusal deterministic at any size; it
    // is read only by resize(), never during a search.
    static inline size_t reservable_clusters = kMaxReservableClusters;

    explicit TranspositionTable(size_t mb = 64) { resize(mb); }
    ~TranspositionTable() { large_pages::release(clusters_); }
    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    // Size the table from the byte budget the user asked for. The cluster
    // count floors to a power of two (the index is a mask), so the allocation
    // is always <= the budget and never less than half of it. Closing that gap
    // needs full-budget (multiply-hi) indexing, which is deferred. Returns
    // false when the requested table cannot be allocated: the table then
    // keeps its previous size, cleared, so the engine stays usable. The table
    // is on large pages where the LargePages option and the system allow
    // (src/large_pages.h), and `threads` workers construct it.
    bool resize(size_t mb, int threads = 1) {
        size_t bytes = mb * 1024 * 1024;
        size_t count = bytes / sizeof(TTCluster);
        size_t power = 1;
        while (power * 2 <= count) power *= 2;

        // Free the old table before allocating the new one: holding both
        // across the allocation made growing 8 -> 16 GB transiently need
        // about 24 GB at `setoption Hash` time, mid-game. No entries are
        // carried across a resize, so dropping first costs nothing.
        const size_t previous = cluster_count_;
        bool allocated = true;
        large_pages::release(clusters_);
        clusters_ = nullptr;
        cluster_count_ = 0;
        // A table above every platform's virtual address space (2^57 bytes,
        // x86-64 with five-level paging) is refused here, without asking the
        // allocator: no platform could reserve it.
        if (power <= reservable_clusters)
            clusters_ = static_cast<TTCluster*>(large_pages::allocate(power * sizeof(TTCluster)));
        if (!clusters_) {
            // Not a smaller table: zero-filling the largest one the system
            // would commit could exhaust memory. The previous size was
            // allocated a moment ago; a first table has none to return to.
            if (previous == 0)
                throw std::bad_alloc();
            clusters_ = static_cast<TTCluster*>(large_pages::allocate(previous * sizeof(TTCluster)));
            if (!clusters_)
                throw std::bad_alloc();
            power = previous;
            allocated = false;
        }
        cluster_count_ = power;
        mask_ = power - 1;
        clear(threads);
        return allocated;
    }

    // Allocated size in bytes, the `Hash` contract under test: allocated <=
    // requested budget, and > budget/2 given the power-of-two floor.
    [[nodiscard]] size_t allocated_bytes() const noexcept {
        return cluster_count_ * sizeof(TTCluster);
    }

    // Every cluster constructed afresh (empty), split over `threads` workers
    // as the donor's clear is, each touching its own share; run only while no
    // search does. A worker the system will not start leaves its share to
    // this thread.
    void clear(int threads = 1) {
        const size_t n = cluster_count_;
        auto fill = [this](size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i)
                ::new (&clusters_[i]) TTCluster;
        };
        const size_t workers = std::clamp<size_t>(size_t(std::max(threads, 1)), 1,
                                                   std::max<size_t>(n / kClearChunk, 1));
        std::vector<std::thread> helpers;
        for (size_t t = 1; t < workers; ++t) {
            const size_t begin = n * t / workers;
            const size_t end = n * (t + 1) / workers;
            try {
                helpers.emplace_back(fill, begin, end);
            } catch (...) {
                fill(begin, end);
            }
        }
        fill(0, n / workers);
        for (std::thread& helper : helpers)
            helper.join();
        age_.store(0, std::memory_order_relaxed);
    }

    void new_search() {
        const uint8_t age = age_.load(std::memory_order_relaxed);
        age_.store(uint8_t((age + TT_AGE_STEP) & TT_AGE_MASK), std::memory_order_relaxed);
    }

    [[nodiscard]] bool probe_copy(Key key, TTEntry& out) const {
        const TTCluster& cluster = clusters_[key & mask_];
        const uint16_t want = static_cast<uint16_t>(key >> 48);

        for (int i = 0; i < 3; ++i) {
            if (cluster.key16[i].load(std::memory_order_acquire) != want)
                continue;
            const uint64_t data = cluster.data[i].load(std::memory_order_relaxed);
            TTEntry e = unpack_entry(data);
            if (occupied(data)) {   // reject an empty slot that hashes to want==0
                e.key16 = want;
                out = e;
                return true;
            }
        }

        out = TTEntry{};
        return false;
    }

    void prefetch(Key key) const {
        if (!clusters_)
            return;
        const void* addr = &clusters_[key & mask_];
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
    // than evicting a different position. The caller accumulates it into its
    // telemetry; the return is free to ignore and the stored data is
    // identical either way.
    bool store(Key key, int depth, int score, TTFlag flag, Move m, int ply, int static_eval,
               bool pv = false) {
        TTCluster& cluster = clusters_[key & mask_];
        const uint16_t want = static_cast<uint16_t>(key >> 48);
        const uint8_t age = age_.load(std::memory_order_relaxed);

        int replace_idx = 0;
        TTEntry replace_entry{};
        uint64_t replace_data = 0;
        bool have_replace = false;
        bool same_key = false;

        for (int i = 0; i < 3; i++) {
            const uint16_t old_key16 = cluster.key16[i].load(std::memory_order_relaxed);
            const uint64_t old_data = cluster.data[i].load(std::memory_order_relaxed);
            TTEntry old_entry = unpack_entry(old_data);
            old_entry.key16 = old_key16;

            if (old_key16 == want && occupied(old_data)) {
                if (flag != TT_EXACT && depth < old_entry.depth - 3
                    && old_entry.age() == age)
                    return true;   // declined, but it WAS our own key

                same_key = true;
                replace_idx = i;
                replace_entry = old_entry;
                replace_data = old_data;
                have_replace = true;
                break;
            }

            if (!have_replace
                || entry_quality(old_entry, old_data, age)
                       < entry_quality(replace_entry, replace_data, age)) {
                replace_idx = i;
                replace_entry = old_entry;
                replace_data = old_data;
                have_replace = true;
            }
        }

        // Preserve the existing best move on a MOVE_NONE store into the same key.
        if (m == MOVE_NONE && same_key)
            m = move_from_tt(replace_entry.move16);

        const uint8_t pv_bit = pv ? TT_PV_MASK : uint8_t(0);
        const int stored_score = score == VALUE_NONE ? VALUE_NONE : score_to_tt(score, ply);
        const uint64_t data = pack_entry(stored_score, static_eval, m, depth,
                                         static_cast<uint8_t>(age | pv_bit | std::to_underlying(flag)));

        // Publish the payload before the key fragment so a concurrent reader
        // that matches key16 sees at least this store's payload. The release
        // here pairs with the acquire on the key16 load in probe_copy and is
        // what enforces "payload before key" on the ARM targets we ship, where
        // store-store reordering could otherwise publish a key ahead of its
        // payload; x86 stores are already ordered, so it costs nothing there.
        // Defence in depth, not a fix for an observed bug: a mismatched pair is
        // the accepted risk above.
        cluster.data[replace_idx].store(data, std::memory_order_relaxed);
        cluster.key16[replace_idx].store(want, std::memory_order_release);
        return same_key;
    }

    // Lowers this position's entry by one ply of depth and changes nothing
    // else in it (Cluster 2's bound-mismatch penalty). An entry at depth 0
    // or below is left alone, so the slot stays occupied and searched. The
    // read and the write are two publications, as any store's are (BAS-C05).
    void penalize(Key key) {
        TTCluster& cluster = clusters_[key & mask_];
        const uint16_t want = static_cast<uint16_t>(key >> 48);
        for (int i = 0; i < 3; ++i) {
            if (cluster.key16[i].load(std::memory_order_relaxed) != want)
                continue;
            const uint64_t data = cluster.data[i].load(std::memory_order_relaxed);
            if (!occupied(data))
                continue;
            if (unpack_entry(data).depth > 0)
                cluster.data[i].store(data - (uint64_t(1) << 48), std::memory_order_relaxed);
            return;
        }
    }

    [[nodiscard]] int hashfull() const {
        int count = 0;
        size_t sample = std::min(size_t(334), cluster_count_);
        const uint8_t age = age_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < sample; i++) {
            for (int j = 0; j < 3; ++j) {
                const uint64_t data = clusters_[i].data[j].load(std::memory_order_relaxed);
                if (occupied(data) && unpack_entry(data).age() == age)
                    count++;
            }
        }
        const int slots = static_cast<int>(sample * 3);
        return slots == 0 ? 0 : count * 1000 / slots;
    }

    // Decisive scores (mates and tablebase results) are stored relative to
    // the node, not the root, so their distance survives a transposition.
    static constexpr int DECISIVE = tablebaseWinInMaxPly;
    static int score_to_tt(int score, int ply) {
        if (score >=  DECISIVE) return score + ply;
        if (score <= -DECISIVE) return score - ply;
        return score;
    }

    // A decisive score the rule-50 clock may spoil before it is realised (a
    // mate, or a tablebase result, further from this node than the plies left
    // on the clock) is downgraded to the strongest non-decisive score; an
    // evaluation is never changed by the clock. An entry without a score
    // reads back as VALUE_NONE.
    static int score_from_tt(int score, int ply, int halfmove_clock = 0) {
        if (score == VALUE_NONE)
            return VALUE_NONE;
        if (score >= DECISIVE) {
            const int left = 100 - halfmove_clock;
            if (score >= MATE_SCORE - MAX_PLY ? MATE_SCORE - score > left
                                              : tablebaseValue - score > left)
                return DECISIVE - 1;
            return score - ply;
        }
        if (score <= -DECISIVE) {
            const int left = 100 - halfmove_clock;
            if (score <= -(MATE_SCORE - MAX_PLY) ? MATE_SCORE + score > left
                                                 : tablebaseValue + score > left)
                return -(DECISIVE - 1);
            return score + ply;
        }
        return score;
    }

private:
    // A worker clears at least this many clusters (2 MiB), so a small table
    // is cleared on one thread.
    static constexpr size_t kClearChunk = (size_t{2} << 20) / sizeof(TTCluster);
    TTCluster* clusters_ = nullptr;
    size_t cluster_count_ = 0;
    size_t mask_ = 0;
    std::atomic<uint8_t> age_{0};

    static uint64_t pack_entry(int score, int static_eval, Move move, int depth, uint8_t flag_age) {
        const auto score16 = static_cast<uint16_t>(static_cast<int16_t>(score));
        const auto eval16  = static_cast<uint16_t>(static_cast<int16_t>(static_eval));
        const auto move16  = static_cast<uint16_t>(move_to_tt(move));
        const auto depth8  = static_cast<uint8_t>(static_cast<int8_t>(
            std::clamp(depth, TT_DEPTH_UNSEARCHED, 127 - TT_DEPTH_OFFSET) + TT_DEPTH_OFFSET));

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
        e.depth       = static_cast<int8_t>(static_cast<int8_t>((data >> 48) & 0xFFu)
                                            - TT_DEPTH_OFFSET);
        e.flag_age    = static_cast<uint8_t>((data >> 56) & 0xFFu);
        return e;
    }

    // Whether a payload word holds an entry. Without the depth offset an entry
    // is told from an empty slot by its bound; with it, by its depth byte,
    // which also admits an evaluation-only entry that has no bound.
    static bool occupied(uint64_t data) {
        if constexpr (TT_DEPTH_OFFSET > 0)
            return ((data >> 48) & 0xFFu) != 0;
        else
            return ((data >> 56) & TT_BOUND_MASK) != 0;
    }

    // Replacement quality: depth, less two per generation of age, plus two
    // for an exact bound. An empty slot loses to anything.
    static int entry_quality(const TTEntry& e, uint64_t data, uint8_t age) {
        if (!occupied(data))
            return -100000;

        const int age_delta = int(uint8_t(age - e.age()) & TT_AGE_MASK); // generations * TT_AGE_STEP
        return int(e.depth) - (age_delta / TT_AGE_STEP) * 2 + (e.bound() == TT_EXACT ? 2 : 0);
    }
};
