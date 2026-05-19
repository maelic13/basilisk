#pragma once

#include "types.h"
#include "move.h"
#include <vector>
#include <cstring>
#include <algorithm>

enum TTFlag : uint8_t { TT_NONE=0, TT_EXACT=1, TT_ALPHA=2, TT_BETA=3 };

struct TTEntry {
    uint16_t key16;
    int16_t  score;
    int16_t  static_eval;
    uint16_t move16;
    uint8_t  depth;
    uint8_t  flag_age;   // bits 0-1: TTFlag, bits 2-7: age
};

class TranspositionTable {
public:
    static constexpr int MATE_SCORE = 32000;
    static constexpr int MAX_PLY    = 128;
    static constexpr int INF_EVAL   = 32001;

    explicit TranspositionTable(size_t mb = 64) { resize(mb); }

    void resize(size_t mb) {
        size_t bytes  = mb * 1024 * 1024;
        size_t count  = bytes / sizeof(TTEntry);
        // Round down to power of 2
        size_t power  = 1;
        while (power * 2 <= count) power *= 2;
        table_.assign(power, TTEntry{});
        mask_ = power - 1;
        age_  = 0;
    }

    void clear() {
        std::fill(table_.begin(), table_.end(), TTEntry{});
        age_ = 0;
    }

    void new_search() { age_ = (age_ + 4) & 0xFC; } // upper 6 bits are age

    TTEntry* probe(Key key, bool& found) const {
        TTEntry* e = const_cast<TTEntry*>(&table_[key & mask_]);
        found = (e->key16 == uint16_t(key >> 48));
        return e;
    }

    void store(Key key, int depth, int score, TTFlag flag, Move m, int ply, int static_eval) {
        TTEntry* e = &table_[key & mask_];
        uint16_t key16 = uint16_t(key >> 48);
        uint8_t  entry_age = e->flag_age & 0xFC;

        // Replacement: replace if: different position, or lower depth, or different age
        bool replace = (e->key16 != key16)
                    || (entry_age != age_)
                    || (flag == TT_EXACT)
                    || (depth + 2 >= e->depth);
        if (!replace) return;

        e->key16       = key16;
        e->score       = (int16_t)score_to_tt(score, ply);
        e->static_eval = (int16_t)(static_eval == INF_EVAL ? INF_EVAL : static_eval);
        e->move16      = (uint16_t)move_to_tt(m);
        e->depth       = (uint8_t)std::max(0, std::min(depth, 255));
        e->flag_age    = (uint8_t)(age_ | uint8_t(flag));
    }

    int hashfull() const {
        int count = 0;
        for (size_t i = 0; i < std::min(size_t(1000), table_.size()); i++)
            if ((table_[i].flag_age & 3) != TT_NONE) count++;
        return count; // permille
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
    std::vector<TTEntry> table_;
    size_t mask_ = 0;
    uint8_t age_ = 0;
};
