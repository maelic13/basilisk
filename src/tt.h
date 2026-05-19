#pragma once

#include "types.h"
#include "move.h"
#include <vector>
#include <cstring>
#include <algorithm>

enum TTFlag : uint8_t { TT_NONE=0, TT_EXACT=1, TT_ALPHA=2, TT_BETA=3 };

// 10 bytes per entry
struct TTEntry {
    uint16_t key16;
    int16_t  score;
    int16_t  static_eval;
    uint16_t move16;
    int8_t   depth;      // signed: allows sentinels at depth=-1
    uint8_t  flag_age;   // bits 0-1: TTFlag, bits 2-7: age (generation)
};

// 3 entries per cluster = 30 bytes + 2 padding = 32 bytes (one cache line half)
struct alignas(32) TTCluster {
    TTEntry entries[3];
    uint8_t  padding[2];
};

class TranspositionTable {
public:
    static constexpr int MATE_SCORE = 32000;
    static constexpr int MAX_PLY    = 128;
    static constexpr int INF_EVAL   = 32001;

    explicit TranspositionTable(size_t mb = 64) { resize(mb); }

    void resize(size_t mb) {
        size_t bytes   = mb * 1024 * 1024;
        size_t count   = bytes / sizeof(TTCluster);
        // Round down to power of 2
        size_t power   = 1;
        while (power * 2 <= count) power *= 2;
        clusters_.assign(power, TTCluster{});
        mask_ = power - 1;
        age_  = 0;
    }

    void clear() {
        std::fill(clusters_.begin(), clusters_.end(), TTCluster{});
        age_ = 0;
    }

    void new_search() { age_ = (age_ + 4) & 0xFC; } // upper 6 bits cycle through ages

    // Returns pointer to the best entry in the cluster; found=true if key matched.
    TTEntry* probe(Key key, bool& found) const {
        TTCluster* cluster = const_cast<TTCluster*>(&clusters_[key & mask_]);
        uint16_t key16 = uint16_t(key >> 48);

        for (int i = 0; i < 3; i++) {
            if (cluster->entries[i].key16 == key16) {
                found = (cluster->entries[i].flag_age & 3) != TT_NONE;
                return &cluster->entries[i];
            }
        }
        found = false;
        // Return weakest slot for potential overwrite
        return weakest_slot(cluster);
    }

    void store(Key key, int depth, int score, TTFlag flag, Move m, int ply, int static_eval) {
        TTCluster* cluster = &clusters_[key & mask_];
        uint16_t key16 = uint16_t(key >> 48);

        // Check for key match first (always update same position)
        TTEntry* replace = nullptr;
        for (int i = 0; i < 3; i++) {
            TTEntry* e = &cluster->entries[i];
            if (e->key16 == key16) {
                // Same position: only update if new info is better
                if (flag != TT_EXACT && depth < e->depth - 3
                    && (e->flag_age & 0xFC) == age_)
                    return;
                replace = e;
                break;
            }
            // Track weakest entry as fallback
            if (!replace || entry_quality(e) < entry_quality(replace))
                replace = e;
        }

        // Update move only if we have one, or if replacing a different position
        if (m == MOVE_NONE && replace->key16 == key16)
            m = move_from_tt(replace->move16);

        replace->key16       = key16;
        replace->score       = (int16_t)score_to_tt(score, ply);
        replace->static_eval = (int16_t)(static_eval == INF_EVAL ? INF_EVAL : static_eval);
        replace->move16      = (uint16_t)move_to_tt(m);
        replace->depth       = (int8_t)std::clamp(depth, -1, 127);
        replace->flag_age    = (uint8_t)(age_ | uint8_t(flag));
    }

    int hashfull() const {
        int count = 0;
        size_t sample = std::min(size_t(334), clusters_.size());
        for (size_t i = 0; i < sample; i++)
            for (int j = 0; j < 3; j++)
                if ((clusters_[i].entries[j].flag_age & 3) != TT_NONE)
                    count++;
        return count; // permille (334 clusters * 3 entries = 1002 samples ≈ 1000)
    }

    // Score adjustments for mate scores stored relative to ply
    static int score_to_tt(int score, int ply) {
        if (score >=  MATE_SCORE - MAX_PLY) return score + ply;
        if (score <= -MATE_SCORE + MAX_PLY) return score - ply;
        return score;
    }

    static int score_from_tt(int score, int ply) {
        if (score >=  MATE_SCORE - MAX_PLY) return score - ply;
        if (score <= -MATE_SCORE + MAX_PLY) return score + ply;
        return score;
    }

private:
    std::vector<TTCluster> clusters_;
    size_t  mask_ = 0;
    uint8_t age_  = 0;

    // Entry quality for replacement: lower = more replaceable.
    // Penalise old entries and reward deep entries.
    int entry_quality(const TTEntry* e) const {
        int age_delta = int(age_ - (e->flag_age & 0xFC)) & 0xFC; // 0, 4, 8, ...
        return int(e->depth) - age_delta / 2;
    }

    TTEntry* weakest_slot(TTCluster* cluster) const {
        TTEntry* weakest = &cluster->entries[0];
        for (int i = 1; i < 3; i++)
            if (entry_quality(&cluster->entries[i]) < entry_quality(weakest))
                weakest = &cluster->entries[i];
        return weakest;
    }
};
