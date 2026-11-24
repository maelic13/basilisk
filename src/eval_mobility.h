#pragma once

// Mobility and space: the mobility tables' accumulation, which the attack
// producer's sweep calls for every piece it sweeps, and the three space terms.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_common.h"

// Per-count mobility: one-hot table indexed by the mobility-square count over
// the mobility area. Called by the sweep, piece by piece, into the producer's
// mobility sum.
template <PieceType PT>
static inline void mobility_term(const EvalParams& p, int mob, int sign, EvalScore& s) {
    int& mg = s.mg;
    int& eg = s.eg;
    if constexpr (PT == KNIGHT) {
        mg += sign * p.mob_n_mg[mob]; TR_MG(MobNMg, mob, sign);
        eg += sign * p.mob_n_eg[mob]; TR_EG(MobNEg, mob, sign);
    } else if constexpr (PT == BISHOP) {
        mg += sign * p.mob_b_mg[mob]; TR_MG(MobBMg, mob, sign);
        eg += sign * p.mob_b_eg[mob]; TR_EG(MobBEg, mob, sign);
    } else if constexpr (PT == ROOK) {
        mg += sign * p.mob_r_mg[mob]; TR_MG(MobRMg, mob, sign);
        eg += sign * p.mob_r_eg[mob]; TR_EG(MobREg, mob, sign);
    } else {  // QUEEN
        mg += sign * p.mob_q_mg[mob]; TR_MG(MobQMg, mob, sign);
        eg += sign * p.mob_q_eg[mob]; TR_EG(MobQEg, mob, sign);
    }
}

// The mobility and space family: the sweep's mobility sum and the space terms.
static EvalScore mobility_space(const Board& b, const EvalScore& mobility,
                                const Bitboard (&pawn_atk)[NCOLORS]) {
    const EvalParams& p = g_eval_params;
    const bool no_mobility = eval_ablated(EVAL_ABLATE_MOBILITY);
    int mg = no_mobility ? 0 : mobility.mg, eg = no_mobility ? 0 : mobility.eg;

    // ---- Space evaluation (center control in middlegame) -------------------
    if (!eval_ablated(EVAL_ABLATE_SPACE)) {
        const Bitboard center_files =
            BB_FILES[FILE_C] | BB_FILES[FILE_D] | BB_FILES[FILE_E] | BB_FILES[FILE_F];
        const Bitboard white_space_ranks =
            BB_RANKS[RANK_2] | BB_RANKS[RANK_3] | BB_RANKS[RANK_4];
        const Bitboard black_space_ranks =
            BB_RANKS[RANK_5] | BB_RANKS[RANK_6] | BB_RANKS[RANK_7];

        Bitboard wspace = (center_files & white_space_ranks) & ~b.piece_bb(WHITE, PAWN) & ~pawn_atk[BLACK];
        Bitboard bspace = (center_files & black_space_ranks) & ~b.piece_bb(BLACK, PAWN) & ~pawn_atk[WHITE];
        int wsp = popcount(wspace), bsp = popcount(bspace);
        int space_count = wsp - bsp;
        mg += space_count * p.space_mg;
        TR_MG(SpaceMg, 0, space_count);

        // ---- Step 3.6 space refinement (traced) ----
        Bitboard wp = b.piece_bb(WHITE, PAWN);
        Bitboard bp = b.piece_bb(BLACK, PAWN);

        // Space weighted by own non-pawn piece count (SF weights by piece count).
        int wpc = popcount(b.occupancy_bb(WHITE)) - popcount(wp) - 1;
        int bpc = popcount(b.occupancy_bb(BLACK)) - popcount(bp) - 1;
        int piece_w = wsp * wpc - bsp * bpc;
        mg += piece_w * p.space_piece_mg;
        TR_MG(SpacePieceMg, 0, piece_w);

        // Space weighted by own blocked-pawn count (a piece directly ahead).
        int wblk = popcount(wp & (b.all_pieces() >> 8));
        int bblk = popcount(bp & (b.all_pieces() << 8));
        int blocked_w = wsp * wblk - bsp * bblk;
        mg += blocked_w * p.space_blocked_mg;
        TR_MG(SpaceBlockedMg, 0, blocked_w);
    }
    return {mg, eg};
}
