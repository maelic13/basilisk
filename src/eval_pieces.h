#pragma once

// Pieces: the bishop pair, the rook and knight terms, the small positional
// terms and the trapped bishop.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_attacks.h"
#include "eval_endgame.h"  // EG_DARK_SQUARES

// The pieces family's cheap entry, before the lazy checkpoint.
static void pieces_cheap(const Board& b, EvalScore& sum) {
    const EvalParams& p = g_eval_params;
    int& mg = sum.mg;
    int& eg = sum.eg;

    // ---- Bishop pair ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        if (more_than_one(b.piece_bb(c, BISHOP))) {
            mg += sign * p.bp_mg; TR_MG(BpMg, 0, sign);
            eg += sign * p.bp_eg; TR_EG(BpEg, 0, sign);
        }
    }

    // ---- Rook bonuses ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard rooks = b.piece_bb(c, ROOK);
        while (rooks) {
            int sq = pop_lsb(rooks);
            int f = file_of(Square(sq));

            bool no_own_pawn   = !(b.piece_bb(us, PAWN)   & BB_FILES[f]);
            bool no_their_pawn = !(b.piece_bb(them, PAWN) & BB_FILES[f]);

            if (no_own_pawn && no_their_pawn) {
                mg += sign * p.rook_open_mg; TR_MG(RookOpenMg, 0, sign);
                eg += sign * p.rook_open_eg; TR_EG(RookOpenEg, 0, sign);
            } else if (no_own_pawn) {
                mg += sign * p.rook_semi_mg; TR_MG(RookSemiMg, 0, sign);
                eg += sign * p.rook_semi_eg; TR_EG(RookSemiEg, 0, sign);
            }

            // Rook on 7th rank (relative)
            if (relative_rank(us, Square(sq)) == RANK_7) {
                mg += sign * p.rook_7th_mg; TR_MG(Rook7thMg, 0, sign);
                eg += sign * p.rook_7th_eg; TR_EG(Rook7thEg, 0, sign);
            }
        }
    }

    // ---- Knight bonuses ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard knights = b.piece_bb(c, KNIGHT);
        while (knights) {
            int sq = pop_lsb(knights);
            Rank r = relative_rank(us, Square(sq));
            if (r >= RANK_5) {
                if (PawnAttacks[them][sq] & b.piece_bb(us, PAWN)) {
                    if (!(PawnAttacks[us][sq] & b.piece_bb(them, PAWN))) {
                        mg += sign * p.knight_outpost_mg; TR_MG(KnightOutpostMg, 0, sign);
                        eg += sign * p.knight_outpost_eg; TR_EG(KnightOutpostEg, 0, sign);
                    }
                }
            }
        }
    }
}

// The pieces family's full entry, after the lazy checkpoint: the terms that
// read the attack maps.
static void pieces_full(const Board& b, const AttackInfo& ai, int phase, EvalScore& sum) {
    const EvalParams& p = g_eval_params;
    const auto& pawn_atk   = ai.pawn_atk;
    const auto& king_zone  = ai.king_zone;
    const auto& slider_att = ai.slider_att;
    int& mg = sum.mg;
    int& eg = sum.eg;

    // ---- Small positional terms (Step 3.7) ---------------------------------
    // Uses the Step-3.0 attack maps + king_zone; traced for the fit.
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c), them = ~us;
        Bitboard our_pawns   = b.piece_bb(us, PAWN);
        Bitboard their_pawns = b.piece_bb(them, PAWN);
        Bitboard ring_them   = king_zone[them];
        Bitboard advanced = (us == WHITE)
            ? (BB_RANKS[RANK_5] | BB_RANKS[RANK_6] | BB_RANKS[RANK_7])
            : (BB_RANKS[RANK_2] | BB_RANKS[RANK_3] | BB_RANKS[RANK_4]);
        Bitboard outpost_sqs = pawn_atk[us] & ~pawn_atk[them] & advanced;

        // Bishop pair scaled by own pawn count.
        if (more_than_one(b.piece_bb(us, BISHOP))) {
            int np = popcount(our_pawns);
            mg += sign * np * p.bishop_pair_pawns_mg; TR_MG(BishopPairPawnsMg, 0, sign * np);
            eg += sign * np * p.bishop_pair_pawns_eg; TR_EG(BishopPairPawnsEg, 0, sign * np);
        }

        // Bishops: bad bishop, king-ring pressure, x-ray pawns, long diagonal.
        // (The outpost bonus lives in the cheap block beside the knight's.)
        Bitboard bishops = b.piece_bb(us, BISHOP);
        while (bishops) {
            Square bsq = Square(pop_lsb(bishops));
            Bitboard cmask = (sq_bb(bsq) & EG_DARK_SQUARES) ? EG_DARK_SQUARES : ~EG_DARK_SQUARES;
            int bp_on_col = popcount(our_pawns & cmask);
            mg += sign * bp_on_col * p.bad_bishop_mg; TR_MG(BadBishopMg, 0, sign * bp_on_col);
            eg += sign * bp_on_col * p.bad_bishop_eg; TR_EG(BadBishopEg, 0, sign * bp_on_col);
            if (slider_att[bsq] & ring_them) {
                mg += sign * p.minor_king_ring_mg; TR_MG(MinorKingRingMg, 0, sign);
                eg += sign * p.minor_king_ring_eg; TR_EG(MinorKingRingEg, 0, sign);
            }
            // Step 3.9: minor behind a friendly pawn; king-protector distance.
            Square b_ahead = forward_square(us, bsq);
            if (b_ahead != SQ_NONE && (our_pawns & sq_bb(b_ahead))) {
                mg += sign * p.minor_behind_pawn_mg; TR_MG(MinorBehindPawnMg, 0, sign);
                eg += sign * p.minor_behind_pawn_eg; TR_EG(MinorBehindPawnEg, 0, sign);
            }
            int b_kd = KING_DIST[b.king_square(us)][bsq];
            mg += sign * b_kd * p.king_protector_b_mg; TR_MG(KingProtectorBMg, 0, sign * b_kd);
            eg += sign * b_kd * p.king_protector_b_eg; TR_EG(KingProtectorBEg, 0, sign * b_kd);
        }

        // Knights: reachable outpost, king-ring pressure.
        Bitboard knights = b.piece_bb(us, KNIGHT);
        while (knights) {
            Square nsq = Square(pop_lsb(knights));
            int reach = popcount(KnightAttacks[nsq] & outpost_sqs & ~b.occupancy_bb(us));
            mg += sign * reach * p.reachable_outpost_mg; TR_MG(ReachableOutpostMg, 0, sign * reach);
            eg += sign * reach * p.reachable_outpost_eg; TR_EG(ReachableOutpostEg, 0, sign * reach);
            if (KnightAttacks[nsq] & ring_them) {
                mg += sign * p.minor_king_ring_mg; TR_MG(MinorKingRingMg, 0, sign);
                eg += sign * p.minor_king_ring_eg; TR_EG(MinorKingRingEg, 0, sign);
            }
            // Step 3.9: minor behind a friendly pawn; king-protector distance.
            Square n_ahead = forward_square(us, nsq);
            if (n_ahead != SQ_NONE && (our_pawns & sq_bb(n_ahead))) {
                mg += sign * p.minor_behind_pawn_mg; TR_MG(MinorBehindPawnMg, 0, sign);
                eg += sign * p.minor_behind_pawn_eg; TR_EG(MinorBehindPawnEg, 0, sign);
            }
            int n_kd = KING_DIST[b.king_square(us)][nsq];
            mg += sign * n_kd * p.king_protector_n_mg; TR_MG(KingProtectorNMg, 0, sign * n_kd);
            eg += sign * n_kd * p.king_protector_n_eg; TR_EG(KingProtectorNEg, 0, sign * n_kd);
        }

        // Queen infiltration (Step 3.9): our queen safely deep in the enemy half.
        Bitboard queens_us = b.piece_bb(us, QUEEN);
        while (queens_us) {
            Square qsq = Square(pop_lsb(queens_us));
            if (relative_rank(us, qsq) >= RANK_5 && !(pawn_atk[them] & sq_bb(qsq))) {
                mg += sign * p.queen_infiltration_mg; TR_MG(QueenInfiltrationMg, 0, sign);
                eg += sign * p.queen_infiltration_eg; TR_EG(QueenInfiltrationEg, 0, sign);
            }
        }

        // Rooks: king-ring pressure, closed file, enemy-queen file.
        Bitboard rooks  = b.piece_bb(us, ROOK);
        Bitboard q_them = b.piece_bb(them, QUEEN);
        while (rooks) {
            Square rsq = Square(pop_lsb(rooks));
            int f = file_of(rsq);
            if (slider_att[rsq] & ring_them) {
                mg += sign * p.rook_king_ring_mg; TR_MG(RookKingRingMg, 0, sign);
                eg += sign * p.rook_king_ring_eg; TR_EG(RookKingRingEg, 0, sign);
            }
            if ((our_pawns & BB_FILES[f]) && (their_pawns & BB_FILES[f])) {
                mg += sign * p.rook_closed_mg; TR_MG(RookClosedMg, 0, sign);
                eg += sign * p.rook_closed_eg; TR_EG(RookClosedEg, 0, sign);
            }
            if (q_them & BB_FILES[f]) {
                mg += sign * p.rook_queen_file_mg; TR_MG(RookQueenFileMg, 0, sign);
                eg += sign * p.rook_queen_file_eg; TR_EG(RookQueenFileEg, 0, sign);
            }
        }

        // Connected rooks (one rook defends the other along an open line).
        if (more_than_one(b.piece_bb(us, ROOK))) {
            Bitboard rks = b.piece_bb(us, ROOK);
            bool connected = false;
            Bitboard tmp = rks;
            while (tmp) {
                Square r = Square(pop_lsb(tmp));
                if (slider_att[r] & rks & ~sq_bb(r)) { connected = true; break; }
            }
            if (connected) {
                mg += sign * p.connected_rooks_mg; TR_MG(ConnectedRooksMg, 0, sign);
                eg += sign * p.connected_rooks_eg; TR_EG(ConnectedRooksEg, 0, sign);
            }
        }

    }

    // ---- Trapped bishop detection ------------------------------------------
    if (phase < TOTAL_PHASE / 2) {
        for (int c = 0; c < NCOLORS; c++) {
            int sign = (c == WHITE) ? 1 : -1;
            Bitboard bbs = b.piece_bb(c, BISHOP);
            while (bbs) {
                Square bsq = Square(pop_lsb(bbs));
                Bitboard moves = slider_att[bsq] & ~b.occupancy_bb(c);
                if (moves == 0) {
                    mg -= sign * p.trapped_mg; TR_MG(TrappedMg, 0, -sign);
                    eg -= sign * p.trapped_eg; TR_EG(TrappedEg, 0, -sign);
                }
            }
        }
    }
}
