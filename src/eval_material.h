#pragma once

// Material and the piece-square tables, the game phase, and the material
// imbalance with its cache.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_common.h"

// Runtime tables baked from g_eval_params in init_eval_tables().
// Material plus piece-square value by piece and square, from White's side: a
// black piece's entry is negative, so a position's sum needs no sign.
struct PieceSquareValue {
    int mg, eg;
};
static PieceSquareValue PIECE_SQUARE[PIECE_NB][SQUARE_NB];

// `ic[colour]`: the bishop pair, then pawns, knights, bishops, rooks and queens.
static int material_imbalance(const int ic[NCOLORS][6], const EvalParams& p) {
    int imbalance = 0;
    for (int i = 0; i < 6; i++) {
        int lin_cnt = ic[WHITE][i] - ic[BLACK][i];
        imbalance += p.imb_linear[i] * lin_cnt;
        TR_BOTH(ImbLinear, i, lin_cnt);
        for (int j = 0; j <= i; j++) {
            int t = i * (i + 1) / 2 + j;
            int our_cnt   = ic[WHITE][i] * ic[WHITE][j] - ic[BLACK][i] * ic[BLACK][j];
            int their_cnt = ic[WHITE][i] * ic[BLACK][j] - ic[BLACK][i] * ic[WHITE][j];
            imbalance += p.imb_our[t]   * our_cnt;   TR_BOTH(ImbOur,   t, our_cnt);
            imbalance += p.imb_their[t] * their_cnt; TR_BOTH(ImbTheir, t, their_cnt);
        }
    }
    return imbalance;
}

// The material family: also sets the phase every later read tapers with.
// The imbalance cache is the caller's: a function of the ten piece counts
// and of the parameters' generation alone.
static EvalScore material(const Board& b,
                          [[maybe_unused]] ImbalanceEntry (&imbalance_table)[1 << IMBALANCE_TABLE_BITS],
                          [[maybe_unused]] int& imbalance_generation,
                          [[maybe_unused]] int tables_generation, int& phase) {
    const EvalParams& p = g_eval_params;
    int mg = 0, eg = 0;
    phase = 0;

    // ---- Material and PST ----
#ifdef TEXEL_TRACE
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = b.piece_bb(c, pt);
            phase += popcount(bb) * PHASE_W[pt];
            while (bb) {
                int sq = pop_lsb(bb);
                const PieceSquareValue& v = PIECE_SQUARE[make_piece(Color(c), PieceType(pt))][sq];
                mg += v.mg;
                eg += v.eg;
                TR_MG(MgVal, pt, sign);
                TR_EG(EgVal, pt, sign);
                int pst_sq = (c == WHITE) ? sq : (sq ^ 56);
                TR_PST_MG(pt, pst_sq, sign);
                TR_PST_EG(pt, pst_sq, sign);
            }
        }
    }
#else
    for (int pt = PAWN; pt <= KING; pt++)
        phase += (popcount(b.piece_bb(WHITE, pt)) + popcount(b.piece_bb(BLACK, pt))) * PHASE_W[pt];
    for (Bitboard bb = b.all_pieces(); bb; ) {
        const int sq = pop_lsb(bb);
        const PieceSquareValue& v = PIECE_SQUARE[b.piece_on(Square(sq))][sq];
        mg += v.mg;
        eg += v.eg;
    }
#endif
    phase = std::min(phase, TOTAL_PHASE);
#ifdef TEXEL_TRACE
    g_trace.phase = phase;
#endif

    // ---- Material imbalance (Step 3.8; SF-style quadratic) ----------------
    // Linear in its coefficients (feature = piece-count products), so traced
    // normally and fit by the linear tuner. Added equally to mg and eg (a
    // non-tapered material correction).
    {
        int ic[NCOLORS][6];
        for (int c = 0; c < NCOLORS; c++) {
            ic[c][0] = (popcount(b.piece_bb(c, BISHOP)) >= 2) ? 1 : 0;  // bishop pair
            ic[c][1] = popcount(b.piece_bb(c, PAWN));
            ic[c][2] = popcount(b.piece_bb(c, KNIGHT));
            ic[c][3] = popcount(b.piece_bb(c, BISHOP));
            ic[c][4] = popcount(b.piece_bb(c, ROOK));
            ic[c][5] = popcount(b.piece_bb(c, QUEEN));
        }
#ifdef TEXEL_TRACE
        // The trace records the features, so it reads them every time.
        const int imbalance = material_imbalance(ic, p);
#else
        if (imbalance_generation != tables_generation) {
            std::memset(imbalance_table, 0, sizeof(imbalance_table));
            imbalance_generation = tables_generation;
        }
        uint64_t key = 1;  // 0 is an empty slot
        for (int c = 0; c < NCOLORS; c++)
            for (int i = 1; i < 6; i++)
                key = key << 6 | static_cast<uint64_t>(ic[c][i]);
        ImbalanceEntry& entry = imbalance_table[(key * 0x9E3779B97F4A7C15ull) >> (64 - IMBALANCE_TABLE_BITS)];
        if (entry.key != key) {
            entry.key = key;
            entry.value = material_imbalance(ic, p);
        }
        const int imbalance = entry.value;
#endif
        mg += imbalance;
        eg += imbalance;
    }
    return {mg, eg};
}
