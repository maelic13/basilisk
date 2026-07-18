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
            for (int j = 0; j < 3; ++j) {
                clusters_[i].data[j].store(0, std::memory_order_relaxed);
                clusters_[i].key16[j].store(0, std::memory_order_relaxed);
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
        const uint16_t want = static_cast<uint16_t>(key >> 48);

        for (int i = 0; i < 3; ++i) {
            if (cluster.key16[i].load(std::memory_order_relaxed) != want)
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
        const uint16_t want = static_cast<uint16_t>(key >> 48);
        const uint8_t age = age_.load(std::memory_order_relaxed);

        int replace_idx = 0;
        TTEntry replace_entry{};
        bool have_replace = false;

        for (int i = 0; i < 3; i++) {
            const uint16_t old_key16 = cluster.key16[i].load(std::memory_order_relaxed);
            TTEntry old_entry = unpack_entry(cluster.data[i].load(std::memory_order_relaxed));
            old_entry.key16 = old_key16;

            if (old_key16 == want && (old_entry.flag_age & 3) != TT_NONE) {
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
        cluster.key16[replace_idx].store(want, std::memory_order_relaxed);
    }

    int hashfull() const {
        int count = 0;
        size_t sample = std::min(size_t(334), cluster_count_);
        const uint8_t age = age_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < sample; i++) {
            for (int j = 0; j < 3; ++j) {
                TTEntry e = unpack_entry(clusters_[i].data[j].load(std::memory_order_relaxed));
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
