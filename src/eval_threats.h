#pragma once

// Threats: pawn threats and the threats package.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_attacks.h"

static EvalScore threats(const Board& b, const AttackInfo& ai) {
    const EvalParams& p = g_eval_params;
    const auto& pawn_atk    = ai.pawn_atk;
    const auto& attacked_by = ai.attacked_by;
    const auto& attacked    = ai.attacked;
    const auto& attacked2   = ai.attacked2;
    int mg = 0, eg = 0;

    // ---- Pawn threats to enemy pieces ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        Bitboard threats = pawn_atk[c] & b.occupancy_bb(them);
        while (threats) {
            int sq = pop_lsb(threats);
            PieceType pt = type_of(b.piece_on(sq));
            switch (pt) {
                case KNIGHT: case BISHOP:
                    mg += sign * p.threat_minor_mg; TR_MG(ThreatMinorMg, 0, sign);
                    eg += sign * p.threat_minor_eg; TR_EG(ThreatMinorEg, 0, sign);
                    break;
                case ROOK:
                    mg += sign * p.threat_rook_mg; TR_MG(ThreatRookMg, 0, sign);
                    eg += sign * p.threat_rook_eg; TR_EG(ThreatRookEg, 0, sign);
                    break;
                case QUEEN:
                    mg += sign * p.threat_queen_mg; TR_MG(ThreatQueenMg, 0, sign);
                    eg += sign * p.threat_queen_eg; TR_EG(ThreatQueenEg, 0, sign);
                    break;
                default: break;
            }
        }
    }

    // ---- Threats package (Step 3.1; SF-style) -------------------------------
    // Uses the Step 3.0 attack maps, beside the flat pawn-threat term above.
    for (int c = 0; c < NCOLORS; c++) {
        int sign   = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;

        Bitboard nonpawn_enemies = b.occupancy_bb(them) & ~b.piece_bb(them, PAWN);
        // Squares the enemy holds firmly: defended by a pawn, or attacked twice by
        // them and not twice by us.
        Bitboard strongly_protected =
            attacked_by[them][PAWN] | (attacked2[them] & ~attacked2[us]);
        Bitboard defended = nonpawn_enemies & strongly_protected;
        // Enemy pieces we attack that are not firmly held.
        Bitboard weak = b.occupancy_bb(them) & ~strongly_protected & attacked[us];

        // Threats by a minor (knight or bishop) on weak or defended targets.
        Bitboard minor_att = attacked_by[us][KNIGHT] | attacked_by[us][BISHOP];
        Bitboard tb = (defended | weak) & minor_att;
        while (tb) {
            PieceType pt = type_of(b.piece_on(pop_lsb(tb)));
            mg += sign * p.threat_by_minor_mg[pt]; TR_MG(ThreatByMinorMg, pt, sign);
            eg += sign * p.threat_by_minor_eg[pt]; TR_EG(ThreatByMinorEg, pt, sign);
        }

        // Threats by a rook on weak targets.
        tb = weak & attacked_by[us][ROOK];
        while (tb) {
            PieceType pt = type_of(b.piece_on(pop_lsb(tb)));
            mg += sign * p.threat_by_rook_mg[pt]; TR_MG(ThreatByRookMg, pt, sign);
            eg += sign * p.threat_by_rook_eg[pt]; TR_EG(ThreatByRookEg, pt, sign);
        }

        // Threat by king on a weak target.
        if (weak & attacked_by[us][KING]) {
            mg += sign * p.threat_by_king_mg; TR_MG(ThreatByKingMg, 0, sign);
            eg += sign * p.threat_by_king_eg; TR_EG(ThreatByKingEg, 0, sign);
        }

        // Hanging: weak pieces with no enemy defender, or non-pawns we hit twice.
        Bitboard hanging = weak & (~attacked[them] | (nonpawn_enemies & attacked2[us]));
        int hang_cnt = popcount(hanging);
        mg += sign * hang_cnt * p.threat_hanging_mg; TR_MG(ThreatHangingMg, 0, sign * hang_cnt);
        eg += sign * hang_cnt * p.threat_hanging_eg; TR_EG(ThreatHangingEg, 0, sign * hang_cnt);

        // Weak piece whose only defender is the enemy queen.
        int wq_cnt = popcount(weak & attacked_by[them][QUEEN]);
        mg += sign * wq_cnt * p.weak_queen_prot_mg; TR_MG(WeakQueenProtMg, 0, sign * wq_cnt);
        eg += sign * wq_cnt * p.weak_queen_prot_eg; TR_EG(WeakQueenProtEg, 0, sign * wq_cnt);

        // Restricted: squares the enemy attacks that we also attack and they do not
        // firmly hold (limits their piece mobility).
        int restr_cnt = popcount(attacked[them] & ~strongly_protected & attacked[us]);
        mg += sign * restr_cnt * p.restricted_mg; TR_MG(RestrictedMg, 0, sign * restr_cnt);
        eg += sign * restr_cnt * p.restricted_eg; TR_EG(RestrictedEg, 0, sign * restr_cnt);

        // Threat by a safe pawn push: squares our pawns can (double-)push to that
        // are safe, whose resulting pawn attacks would hit an enemy non-pawn.
        Bitboard empty = ~b.all_pieces();
        Bitboard safe  = ~attacked[them] | attacked[us];
        Bitboard push, push_att;
        if (us == WHITE) {
            push  = shift<NORTH>(b.piece_bb(us, PAWN)) & empty;
            push |= shift<NORTH>(push & BB_RANKS[RANK_3]) & empty;
            push &= ~attacked_by[them][PAWN] & safe;
            push_att = shift<NORTH_WEST>(push) | shift<NORTH_EAST>(push);
        } else {
            push  = shift<SOUTH>(b.piece_bb(us, PAWN)) & empty;
            push |= shift<SOUTH>(push & BB_RANKS[RANK_6]) & empty;
            push &= ~attacked_by[them][PAWN] & safe;
            push_att = shift<SOUTH_WEST>(push) | shift<SOUTH_EAST>(push);
        }
        int push_cnt = popcount(push_att & nonpawn_enemies);
        mg += sign * push_cnt * p.threat_push_mg; TR_MG(ThreatPushMg, 0, sign * push_cnt);
        eg += sign * push_cnt * p.threat_push_eg; TR_EG(ThreatPushEg, 0, sign * push_cnt);
    }
    return {mg, eg};
}
