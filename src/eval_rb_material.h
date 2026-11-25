#pragma once

// The rebuilt evaluation's start: material and the piece-square tables, the
// material imbalance with its cache, and the game phase.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone when BASILISK_EVAL_REBUILD is defined. Include it nowhere else.

#include "eval_rb_common.h"

// Material plus piece-square value by piece and square, from White's side: a
// black piece's entry is negative, so a position's sum needs no sign. Baked
// from g_eval_params by init_eval_tables().
struct PieceSquareValue {
    int mg, eg;
};
static PieceSquareValue PIECE_SQUARE[PIECE_NB][SQUARE_NB];

namespace rb {

// The phase from both sides' non-pawn material: PHASE_MIDGAME at or above
// MIDGAME_LIMIT, 0 at or below ENDGAME_LIMIT, linear and truncated between.
inline int game_phase(const Board& b) {
    const int npm = std::clamp(non_pawn_material(b), ENDGAME_LIMIT, MIDGAME_LIMIT);
    return (npm - ENDGAME_LIMIT) * PHASE_MIDGAME / (MIDGAME_LIMIT - ENDGAME_LIMIT);
}

// One side's second-degree imbalance polynomial. `ours` and `theirs` are the
// piece counts: the bishop pair, then pawns, knights, bishops, rooks, queens.
inline int imbalance_polynomial(const int ours[6], const int theirs[6], const EvalParams& p) {
    int bonus = 0;
    for (int i = 0; i < 6; i++) {
        if (!ours[i])
            continue;
        int v = p.imb_ours[i * (i + 1) / 2 + i] * ours[i];
        for (int j = 0; j < i; j++)
            v += p.imb_ours[i * (i + 1) / 2 + j] * ours[j]
               + p.imb_theirs[i * (i - 1) / 2 + j] * theirs[j];
        bonus += ours[i] * v;
    }
    return bonus;
}

// The imbalance, added to both halves. A function of the ten piece counts
// and the parameters alone, so the caller's table caches it under those
// counts and the parameters' generation.
inline int imbalance(const Board& b, ImbalanceEntry (&table)[1 << IMBALANCE_TABLE_BITS],
                     int& generation, int tables_generation) {
    int counts[NCOLORS][6];
    for (int c = 0; c < NCOLORS; c++) {
        counts[c][0] = count(b, Color(c), BISHOP) > 1;
        counts[c][1] = count(b, Color(c), PAWN);
        counts[c][2] = count(b, Color(c), KNIGHT);
        counts[c][3] = count(b, Color(c), BISHOP);
        counts[c][4] = count(b, Color(c), ROOK);
        counts[c][5] = count(b, Color(c), QUEEN);
    }
    if (generation != tables_generation) {
        std::memset(table, 0, sizeof(table));
        generation = tables_generation;
    }
    uint64_t key = 1;  // 0 is an empty slot
    for (int c = 0; c < NCOLORS; c++)
        for (int i = 1; i < 6; i++)
            key = key << 6 | static_cast<uint64_t>(counts[c][i]);
    ImbalanceEntry& entry = table[(key * 0x9E3779B97F4A7C15ull) >> (64 - IMBALANCE_TABLE_BITS)];
    if (entry.key != key) {
        entry.key = key;
        const EvalParams& p = g_eval_params;
        entry.value = (imbalance_polynomial(counts[WHITE], counts[BLACK], p)
                     - imbalance_polynomial(counts[BLACK], counts[WHITE], p)) / 16;
    }
    return entry.value;
}

// Material and the tables.
inline Score material(const Board& b) {
    Score s;
    for (Bitboard bb = b.all_pieces(); bb; ) {
        const int sq = pop_lsb(bb);
        const PieceSquareValue& v = PIECE_SQUARE[b.piece_on(Square(sq))][sq];
        s.mg += v.mg;
        s.eg += v.eg;
    }
    return s;
}

} // namespace rb
