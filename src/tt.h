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

struct TTSlot {
    std::atomic<uint64_t> key_xor_data;
    std::atomic<uint64_t> data;

    TTSlot() noexcept : key_xor_data(0), data(0) {}
    TTSlot(const TTSlot&) = delete;
    TTSlot& operator=(const TTSlot&) = delete;
};

// 3 entries per cluster. Atomic slots make the cluster 64 bytes on common
// 64-bit targets, which keeps one cluster on one cache line.
struct alignas(64) TTCluster {
    TTSlot entries[3];
};

class TranspositionTable {
public:
    static constexpr int MATE_SCORE = 32000;
    static constexpr int MAX_PLY    = 128;
    static constexpr int INF_EVAL   = 32001;

    explicit TranspositionTable(size_t mb = 64) { resize(mb); }

    void resize(size_t mb) {
        size_t bytes = mb * 1024 * 1024;
        size_t count = bytes / sizeof(TTCluster);
        size_t power = 1;
        while (power * 2 <= count) power *= 2;

        clusters_ = std::make_unique<TTCluster[]>(power);
        cluster_count_ = power;
        mask_ = power - 1;
        age_.store(0, std::memory_order_relaxed);
    }

    void clear() {
        for (size_t i = 0; i < cluster_count_; ++i) {
            for (TTSlot& slot : clusters_[i].entries) {
                slot.data.store(0, std::memory_order_relaxed);
                slot.key_xor_data.store(0, std::memory_order_relaxed);
            }
        }
        age_.store(0, std::memory_order_relaxed);
    }

    void new_search() {
        const uint8_t age = age_.load(std::memory_order_relaxed);
        age_.store((age + 4) & 0xFC, std::memory_order_relaxed);
    }

    bool probe_copy(Key key, TTEntry& out) const {
        const TTCluster& cluster = clusters_[key & mask_];

        for (const TTSlot& slot : cluster.entries) {
            const uint64_t data = slot.data.load(std::memory_order_relaxed);
            const uint64_t stored_key = slot.key_xor_data.load(std::memory_order_relaxed) ^ data;
            if (stored_key == key) {
                TTEntry e = unpack_entry(key, data);
                if ((e.flag_age & 3) != TT_NONE) {
                    out = e;
                    return true;
                }
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

    void store(Key key, int depth, int score, TTFlag flag, Move m, int ply, int static_eval) {
        TTCluster& cluster = clusters_[key & mask_];
        const uint8_t age = age_.load(std::memory_order_relaxed);

        int replace_idx = 0;
        TTEntry replace_entry{};
        bool have_replace = false;

        for (int i = 0; i < 3; i++) {
            TTSlot& slot = cluster.entries[i];
            const uint64_t old_data = slot.data.load(std::memory_order_relaxed);
            const uint64_t old_key = slot.key_xor_data.load(std::memory_order_relaxed) ^ old_data;
            TTEntry old_entry = unpack_entry(old_key, old_data);

            if (old_key == key && (old_entry.flag_age & 3) != TT_NONE) {
                if (flag != TT_EXACT && depth < old_entry.depth - 3
                    && (old_entry.flag_age & 0xFC) == age)
                    return;

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

        if (m == MOVE_NONE
            && (cluster.entries[replace_idx].key_xor_data.load(std::memory_order_relaxed)
                ^ cluster.entries[replace_idx].data.load(std::memory_order_relaxed)) == key)
            m = move_from_tt(replace_entry.move16);

        const uint64_t data = pack_entry(score_to_tt(score, ply),
                                         static_eval == INF_EVAL ? INF_EVAL : static_eval,
                                         m, depth, static_cast<uint8_t>(age | uint8_t(flag)));

        TTSlot& slot = cluster.entries[replace_idx];
        slot.data.store(data, std::memory_order_relaxed);
        slot.key_xor_data.store(key ^ data, std::memory_order_relaxed);
    }

    int hashfull() const {
        int count = 0;
        size_t sample = std::min(size_t(334), cluster_count_);
        const uint8_t age = age_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < sample; i++) {
            for (const TTSlot& slot : clusters_[i].entries) {
                const uint64_t data = slot.data.load(std::memory_order_relaxed);
                TTEntry e = unpack_entry(slot.key_xor_data.load(std::memory_order_relaxed) ^ data,
                                         data);
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

    static TTEntry unpack_entry(Key key, uint64_t data) {
        TTEntry e{};
        e.key16       = static_cast<uint16_t>(key >> 48);
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
