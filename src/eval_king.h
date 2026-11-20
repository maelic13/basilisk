#pragma once

// King safety: the danger index and its table, the pawn shelter and storm.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_attacks.h"

static void king(const Board& b, const AttackInfo& ai, EvalScore& sum) {
    const EvalParams& p = g_eval_params;
    const auto& attacked_by       = ai.attacked_by;
    const auto& attacked          = ai.attacked;
    const auto& attacked2         = ai.attacked2;
    const auto& blockers_for_king = ai.blockers_for_king;
    const auto& attack_units      = ai.attack_units;
    const auto& n_attackers       = ai.n_attackers;
    int& mg = sum.mg;  // every king term is middlegame-only

    // ---- King safety v2 (finalization) -------------------------------------
    // safety_table[units] is a one-hot lookup. Piece-attack pressure
    // (attack_units / n_attackers) was gathered in the attack-map sweep; here we
    // add the Step-3.2 danger inputs, apply the no-queen relief and open-file
    // penalty, and look up the table. The new
    // unit weights and the existing ks_unit/coord/open-file/no-queen knobs are
    // composite (they shape the index), so only SafetyTable carries a trace; they
    // are tuned by the Phase-4.3 finite-difference path.
    for (int c = 0; c < NCOLORS; c++) {
        int sign   = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Square ksq = b.king_square(us);
        File   kf  = file_of(ksq);

        int units = attack_units[c];
        if (n_attackers[c] >= 2) units += p.ks_coord_bonus;
        if (n_attackers[c] == 0) units = 0;

        // Safe checks: enemy checking squares it attacks and we do not hold.
        Bitboard safe_sq = ~b.occupancy_bb(them) & ~attacked[us];
        for (int pt : {KNIGHT, BISHOP, ROOK, QUEEN}) {
            Bitboard checks = b.check_squares(PieceType(pt), them)
                            & attacked_by[them][pt] & safe_sq;
            if (checks)
                units += p.ks_safe_check[pt] * popcount(checks);
        }

        // Weak king-ring squares: enemy-attacked, not held twice, no pawn cover.
        Bitboard ring = KingAttacks[ksq];
        Bitboard weak_ring = ring & attacked[them] & ~attacked2[us] & ~attacked_by[us][PAWN];
        units += p.ks_weak_ring * popcount(weak_ring);

        // Own pieces pinned in front of the king (a pinned defender doesn't defend).
        units += p.ks_king_blockers * popcount(blockers_for_king[c]);

        // Central king with castling rights gone.
        const int our_castle_mask = (us == WHITE) ? 3 : 12;
        if (kf >= FILE_C && kf <= FILE_F && !(b.castling() & our_castle_mask))
            units += p.ks_central_king;

        // Pawn-shelter exposure folded into the danger funnel (open files near king).
        int shelter_danger = 0;
        for (int df = -1; df <= 1; df++) {
            int f = int(kf) + df;
            if (f < FILE_A || f > FILE_H) continue;
            if (!(b.piece_bb(us, PAWN) & BB_FILES[f])) shelter_danger++;
        }
        units += p.ks_shelter_storm * shelter_danger;

        // No-enemy-queen relief (exposed scalar; was frozen 2/3, retuned since).
        if (!b.piece_bb(them, QUEEN))
            units = units * p.ks_noqueen_num / p.ks_noqueen_den;

        units = std::min(units, 24);
        if (units < 0) units = 0;
        mg -= sign * p.safety_table[units];
        TR_MG(SafetyTable, units, -sign);
    }

    // ---- King pawn shelter -------------------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Square ksq = b.king_square(c);
        File   kf  = file_of(ksq);
        Rank   kr  = rank_of(ksq);

        if (kf <= FILE_C || kf >= FILE_F) {
            for (int df = -1; df <= 1; df++) {
                int f = kf + df;
                if (f < FILE_A || f > FILE_H) continue;
                Bitboard file_pawns = b.piece_bb(us, PAWN) & BB_FILES[f];

                if (us == WHITE) {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[WHITE][kr];
                    if (!in_front) {
                        if (df == 0) { mg -= sign * p.shelter_missing_center; TR_MG(ShelterMissingCenter, 0, -sign); }
                        else         { mg -= sign * p.shelter_missing_flank;  TR_MG(ShelterMissingFlank,  0, -sign); }
                    } else {
                        Rank pawn_rank = rank_of(Square(lsb(in_front)));
                        int dist = pawn_rank - kr;
                        if (dist == 1)      { mg += sign * p.shelter_close1; TR_MG(ShelterClose1, 0, sign); }
                        else if (dist == 2) { mg += sign * p.shelter_close2; TR_MG(ShelterClose2, 0, sign); }
                    }
                } else {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[BLACK][kr];
                    if (!in_front) {
                        if (df == 0) { mg -= sign * p.shelter_missing_center; TR_MG(ShelterMissingCenter, 0, -sign); }
                        else         { mg -= sign * p.shelter_missing_flank;  TR_MG(ShelterMissingFlank,  0, -sign); }
                    } else {
                        Rank pawn_rank = rank_of(Square(msb(in_front)));
                        int dist = kr - pawn_rank;
                        if (dist == 1)      { mg += sign * p.shelter_close1; TR_MG(ShelterClose1, 0, sign); }
                        else if (dist == 2) { mg += sign * p.shelter_close2; TR_MG(ShelterClose2, 0, sign); }
                    }
                }
            }
        }

        Bitboard storm_files = 0;
        for (int df = -1; df <= 1; df++) {
            int f = kf + df;
            if (f >= FILE_A && f <= FILE_H)
                storm_files |= BB_FILES[f];
        }

        Bitboard storm = b.piece_bb(~us, PAWN) & storm_files;
        while (storm) {
            Square psq = Square(pop_lsb(storm));
            int rel_r = relative_rank(~us, psq);
            if (rel_r >= 3) {
                if (file_of(psq) == kf) {
                    mg -= sign * rel_r * p.storm_weight_kf;  TR_MG(StormWeightKf,  0, -sign * rel_r);
                } else {
                    mg -= sign * rel_r * p.storm_weight_adj; TR_MG(StormWeightAdj, 0, -sign * rel_r);
                }
            }
        }
    }
}
