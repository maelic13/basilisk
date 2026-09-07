#pragma once

#include "types.h"
#include "move.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
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

// Dense partial-key cluster (8.5.D1). Each entry is an 8-byte record word plus
// a 2-byte move — 10 bytes, half the old 16-byte full-key/XOR slot, so a
// 32-byte cluster holds the same 3 entries and equal hash fits ~2x the clusters
// (and entries).
//
// Publication model (BAS-C05). The VALIDATED RECORD — partial key, score,
// static eval, depth, bound flag and age — is exactly 64 bits, so it is
// published and consumed as ONE atomic word. A reader therefore cannot pair a
// key from one publication with a bound from another: the tear is not unlikely,
// it is unrepresentable. That matters because two search paths turn this record
// straight into a value — the depth-preferred cutoff at `search.cpp:1589` and
// the singular multicut at `search.cpp:1967`.
//
// `move16` is deliberately OUTSIDE that guarantee; there are no spare bits, and
// it is the one field every consumer already validates before use (MovePicker
// checks piece/colour/is_legal, ponder_from_tt checks legality, qsearch only
// compares it against generated legal moves). A move observed from a different
// publication can only mis-order a node, never produce a wrong bound. See
// DESIGN.md section 3.
//
// No memory ordering is needed for the record: atomicity of the single word is
// the entire requirement, and the only cross-field pairing left (word/move) is
// the one carrying no guarantee. Relaxed keeps the scan cheap on ARM64 too —
// the previous `key16 ^ fold16(payload)` tag cost 3.91% NPS at identical nodes
// precisely because it put work between the load and the reject.
[[nodiscard]] inline constexpr uint64_t tt_pack(uint16_t key16, int score, int static_eval,
                                                int depth, uint8_t flag_age) noexcept {
    const auto score16 = static_cast<uint16_t>(static_cast<int16_t>(score));
    const auto eval16  = static_cast<uint16_t>(static_cast<int16_t>(static_eval));
    const auto depth8  = static_cast<uint8_t>(
        static_cast<int8_t>(std::clamp(depth, -1, 127)));

    return uint64_t(key16)
         | (uint64_t(score16)  << 16)
         | (uint64_t(eval16)   << 32)
         | (uint64_t(depth8)   << 48)
         | (uint64_t(flag_age) << 56);
}

// move16 is filled by the caller from the slot's separate field, or left 0.
[[nodiscard]] inline constexpr TTEntry tt_unpack(uint64_t word) noexcept {
    TTEntry e{};
    e.key16       = static_cast<uint16_t>(word & 0xFFFFu);
    e.score       = static_cast<int16_t>((word >> 16) & 0xFFFFu);
    e.static_eval = static_cast<int16_t>((word >> 32) & 0xFFFFu);
    e.move16      = 0;
    e.depth       = static_cast<int8_t>((word >> 48) & 0xFFu);
    e.flag_age    = static_cast<uint8_t>((word >> 56) & 0xFFu);
    return e;
}

struct alignas(32) TTCluster {
    std::atomic<uint64_t> words[3];   // validated records, 8-byte aligned (offsets 0/8/16)
    std::atomic<uint16_t> moves[3];   // best moves, NOT covered by the guarantee (24/26/28)

    TTCluster() noexcept {
        for (int i = 0; i < 3; ++i) { words[i].store(0); moves[i].store(0); }
    }
    TTCluster(const TTCluster&) = delete;
    TTCluster& operator=(const TTCluster&) = delete;

    [[nodiscard]] uint64_t load_word(int index) const noexcept {
        return words[index].load(std::memory_order_relaxed);
    }

    [[nodiscard]] uint16_t load_move(int index) const noexcept {
        return moves[index].load(std::memory_order_relaxed);
    }

    void store(int index, uint64_t word, uint16_t move16) noexcept {
        moves[index].store(move16, std::memory_order_relaxed);
        words[index].store(word, std::memory_order_relaxed);
    }

    void clear(int index) noexcept {
        words[index].store(0, std::memory_order_relaxed);
        moves[index].store(0, std::memory_order_relaxed);
    }
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

// BAS-C05: the validated record must round-trip through ONE 64-bit word. These
// are the structural form of the coherence guarantee — widening any field so it
// no longer fits (or quietly moving one out of the word) must be a build
// failure, not a torn read discovered by a game gate. Extremes are chosen at
// the real limits: the deepest mate band, the INF_EVAL sentinel, the depth=-1
// qsearch sentinel and a full age byte.
namespace tt_layout_check {
inline constexpr uint64_t kWord = tt_pack(0xBEEF, -(32000 + 128), 32001, -1, 0xFC);
static_assert(tt_unpack(kWord).key16       == 0xBEEF, "key16 must survive the word");
static_assert(tt_unpack(kWord).score       == -(32000 + 128), "score must survive the word");
static_assert(tt_unpack(kWord).static_eval == 32001, "INF_EVAL must survive the word");
static_assert(tt_unpack(kWord).depth       == -1, "the depth=-1 sentinel must survive the word");
static_assert(tt_unpack(kWord).flag_age    == 0xFC, "flag/age must survive the word");
static_assert(tt_unpack(0).key16 == 0 && (tt_unpack(0).flag_age & 3) == TT_NONE,
              "an empty slot must decode to key16 0 and TT_NONE so probe_copy rejects it");
}  // namespace tt_layout_check

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
        clusters_.reset();
        clusters_ = std::make_unique<TTCluster[]>(power);
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
                clusters_[i].clear(j);
            }
        }
        age_.store(0, std::memory_order_relaxed);
    }

    void new_search() {
        const uint8_t age = age_.load(std::memory_order_relaxed);
        age_.store((age + 4) & 0xFC, std::memory_order_relaxed);
    }

    [[nodiscard]] bool probe_copy(Key key, TTEntry& out) const {
        const TTCluster& cluster = clusters_[key & mask_];
        const uint16_t want = static_cast<uint16_t>(key >> 48);

        for (int i = 0; i < 3; ++i) {
            const uint64_t word = cluster.load_word(i);
            if (static_cast<uint16_t>(word) != want)   // rejects on the loaded word itself
                continue;
            TTEntry e = tt_unpack(word);
            if ((e.flag_age & 3) != TT_NONE) {   // reject an empty slot that hashes to want==0
                e.move16 = cluster.load_move(i);   // unvalidated by contract; callers check it
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
    // than evicting a different position (9.3c telemetry — the caller
    // accumulates it into DiagCounters; the return is free to ignore and the
    // stored data is identical either way).
    bool store(Key key, int depth, int score, TTFlag flag, Move m, int ply, int static_eval) {
        TTCluster& cluster = clusters_[key & mask_];
        const uint16_t want = static_cast<uint16_t>(key >> 48);
        const uint8_t age = age_.load(std::memory_order_relaxed);

        int replace_idx = 0;
        TTEntry replace_entry{};
        bool have_replace = false;
        bool same_key = false;

        for (int i = 0; i < 3; i++) {
            // One load now carries the key AND the depth/flag/age that
            // entry_quality needs, so the replacement decision is made on a
            // single coherent publication rather than a possibly-torn pair.
            TTEntry old_entry = tt_unpack(cluster.load_word(i));

            if (old_entry.key16 == want && (old_entry.flag_age & 3) != TT_NONE) {
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
            m = move_from_tt(cluster.load_move(replace_idx));

        cluster.store(replace_idx,
                      tt_pack(want, score_to_tt(score, ply),
                              static_eval == INF_EVAL ? INF_EVAL : static_eval,
                              depth, static_cast<uint8_t>(age | uint8_t(flag))),
                      static_cast<uint16_t>(move_to_tt(m)));
        return same_key;
    }

    [[nodiscard]] int hashfull() const {
        int count = 0;
        size_t sample = std::min(size_t(334), cluster_count_);
        const uint8_t age = age_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < sample; i++) {
            for (int j = 0; j < 3; ++j) {
                // Deliberately unkeyed: this is an occupancy estimate for the
                // UCI info string, not a record any search decision consumes.
                TTEntry e = tt_unpack(clusters_[i].load_word(j));
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
    std::unique_ptr<TTCluster[]> clusters_;
    size_t cluster_count_ = 0;
    size_t mask_ = 0;
    std::atomic<uint8_t> age_{0};

    static int entry_quality(const TTEntry& e, uint8_t age) {
        if ((e.flag_age & 3) == TT_NONE)
            return -100000;

        int age_delta = int(age - (e.flag_age & 0xFC)) & 0xFC; // 0, 4, 8, ...
        return int(e.depth) - age_delta / 2 + (((e.flag_age & 3) == TT_EXACT) ? 2 : 0);
    }
};
