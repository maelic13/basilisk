#pragma once

// Pawns: the pawn cache, the passer terms inside and outside it, rook behind
// a passer, king proximity and the promotion path.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone, which the engine library, the Texel tuner and the oracle build all
// compile as one file. Include it nowhere else.

#include "eval_attacks.h"

// ---- Pawn structure evaluation (cached) ------------------------------------
// A function of the pawns alone: the pawn key is their Zobrist key, so a
// cached entry can only ever answer for its own pawn structure.
static void pawn_probe(const Board& b,
                       [[maybe_unused]] PawnEntry (&pawn_table)[PAWN_TABLE_SIZE],
                       [[maybe_unused]] int64_t& pawn_probes,
                       [[maybe_unused]] int64_t& pawn_hits, PawnEntry& out) {
    Bitboard (&passed)[NCOLORS]  = out.passed;
    Bitboard (&attacks)[NCOLORS] = out.attacks;
#ifndef TEXEL_TRACE
    // Cache lookup: skip recomputation if pawn structure matches.
    Key pkey = b.pawn_key_value();
    PawnEntry& pe = pawn_table[pkey & (PAWN_TABLE_SIZE - 1)];

    DIAG_COUNT(++pawn_probes);
    if (pe.key == pkey) {
        DIAG_COUNT(++pawn_hits);
        out = pe;
        return;
    }
#endif

    const EvalParams& p = g_eval_params;
    int mg = 0, eg = 0;

    for (int c = 0; c < NCOLORS; c++) {
        Color us   = Color(c);
        Color them = ~us;
        int sign   = (us == WHITE) ? 1 : -1;

        Bitboard our_pawns   = b.piece_bb(us, PAWN);
        Bitboard their_pawns = b.piece_bb(them, PAWN);

        // Pawn attack map
        Bitboard pawn_atk = 0;
        Bitboard tmp = our_pawns;
        while (tmp) {
            int sq = pop_lsb(tmp);
            pawn_atk |= PawnAttacks[us][sq];
        }
        attacks[c] = pawn_atk;

        passed[c] = 0;
        tmp = our_pawns;
        while (tmp) {
            int sq = pop_lsb(tmp);
            int f = file_of(Square(sq));
            int r = rank_of(Square(sq));

            // Passed pawn: no enemy pawns in front on same or adjacent files
            if (!(BB_PASSED_PAWN_MASK[us][sq] & their_pawns)) {
                passed[c] |= sq_bb(Square(sq));
                int rel_r = (us == WHITE) ? r : 7 - r;
                mg += sign * p.passed_mg[rel_r]; TR_MG(PassedMg, rel_r, sign);
                eg += sign * p.passed_eg[rel_r]; TR_EG(PassedEg, rel_r, sign);
            }

            Bitboard file_bb = BB_FILES[f];
            Bitboard adj_bb  = BB_ADJACENT_FILES[f];

            // Doubled pawns
            if (more_than_one(our_pawns & file_bb)) {
                mg += sign * p.doubled_mg; TR_MG(DoubledMg, 0, sign);
                eg += sign * p.doubled_eg; TR_EG(DoubledEg, 0, sign);
            }

            // Isolated pawns
            if (!(our_pawns & adj_bb)) {
                mg += sign * p.isolated_mg; TR_MG(IsolatedMg, 0, sign);
                eg += sign * p.isolated_eg; TR_EG(IsolatedEg, 0, sign);
            }

            // Connected pawns (supported by another pawn)
            if (PawnAttacks[them][sq] & our_pawns) {
                mg += sign * p.connected_mg; TR_MG(ConnectedMg, 0, sign);
                eg += sign * p.connected_eg; TR_EG(ConnectedEg, 0, sign);
            }

            // Backward pawn
            bool backward = false;
            if (!(our_pawns & BB_PASSED_PAWN_MASK[them][sq] & adj_bb)) {
                int stop_sq = (us == WHITE) ? sq + 8 : sq - 8;
                if (stop_sq >= 0 && stop_sq < 64) {
                    if (PawnAttacks[us][stop_sq] & their_pawns) {
                        backward = true;
                        mg += sign * p.backward_mg; TR_MG(BackwardMg, 0, sign);
                        eg += sign * p.backward_eg; TR_EG(BackwardEg, 0, sign);
                    }
                }
            }

            // ---- Step 3.4 pawn-structure refinement (pawn-only, seeded 0) ----
            int rel_r = (us == WHITE) ? r : 7 - r;
            // Connected/phalanx bonus by rank (supported by, or side-by-side with, a friendly pawn).
            bool supported = (PawnAttacks[them][sq] & our_pawns) != 0;
            bool phalanx   = (our_pawns & adj_bb & BB_RANKS[r]) != 0;
            if (supported || phalanx) {
                mg += sign * p.connected_rank_mg[rel_r]; TR_MG(ConnectedRankMg, rel_r, sign);
                eg += sign * p.connected_rank_eg[rel_r]; TR_EG(ConnectedRankEg, rel_r, sign);
            }
            // Weak (isolated or backward) pawn on a half-open file (unopposed).
            bool isolated_p = !(our_pawns & adj_bb);
            if ((isolated_p || backward) && !(their_pawns & file_bb)) {
                mg += sign * p.weak_unopposed_mg; TR_MG(WeakUnopposedMg, 0, sign);
                eg += sign * p.weak_unopposed_eg; TR_EG(WeakUnopposedEg, 0, sign);
            }
            // Rammed: own pawn directly blocked by an enemy pawn, on relative rank 5 / 6.
            {
                int ahead = (us == WHITE) ? sq + 8 : sq - 8;
                if (ahead >= 0 && ahead < 64 && (their_pawns & sq_bb(Square(ahead)))) {
                    if (rel_r == 4) {
                        mg += sign * p.blocked_pawn_mg[0]; TR_MG(BlockedPawnMg, 0, sign);
                        eg += sign * p.blocked_pawn_eg[0]; TR_EG(BlockedPawnEg, 0, sign);
                    } else if (rel_r == 5) {
                        mg += sign * p.blocked_pawn_mg[1]; TR_MG(BlockedPawnMg, 1, sign);
                        eg += sign * p.blocked_pawn_eg[1]; TR_EG(BlockedPawnEg, 1, sign);
                    }
                }
            }
        }

        // Pawn majorities (queenside files a-d, kingside e-h) - breakthrough potential.
        {
            const Bitboard qs = BB_FILES[FILE_A] | BB_FILES[FILE_B] | BB_FILES[FILE_C] | BB_FILES[FILE_D];
            const Bitboard ks = BB_FILES[FILE_E] | BB_FILES[FILE_F] | BB_FILES[FILE_G] | BB_FILES[FILE_H];
            if (popcount(our_pawns & qs) > popcount(their_pawns & qs)) {
                mg += sign * p.pawn_majority_mg; TR_MG(PawnMajorityMg, 0, sign);
                eg += sign * p.pawn_majority_eg; TR_EG(PawnMajorityEg, 0, sign);
            }
            if (popcount(our_pawns & ks) > popcount(their_pawns & ks)) {
                mg += sign * p.pawn_majority_mg; TR_MG(PawnMajorityMg, 0, sign);
                eg += sign * p.pawn_majority_eg; TR_EG(PawnMajorityEg, 0, sign);
            }
        }

        // Pawn islands: disconnected own-pawn file groups (Step 3.9, seeded 0).
        {
            int islands = 0;
            bool prev = false;
            for (int f = FILE_A; f <= FILE_H; f++) {
                bool has = (our_pawns & BB_FILES[f]) != 0;
                if (has && !prev) islands++;
                prev = has;
            }
            mg += sign * islands * p.pawn_islands_mg; TR_MG(PawnIslandsMg, 0, sign * islands);
            eg += sign * islands * p.pawn_islands_eg; TR_EG(PawnIslandsEg, 0, sign * islands);
        }
    }

#ifndef TEXEL_TRACE
    pe.key            = pkey;
    pe.mg             = mg;
    pe.eg             = eg;
    pe.passed[WHITE]  = passed[WHITE];
    pe.passed[BLACK]  = passed[BLACK];
    pe.attacks[WHITE] = attacks[WHITE];
    pe.attacks[BLACK] = attacks[BLACK];
#endif

    out.key = b.pawn_key_value();
    out.mg  = mg;
    out.eg  = eg;
}

// The pawns family's cheap entry, before the lazy checkpoint: the cached
// structure score and the passer terms that read non-pawn occupancy.
static EvalScore pawns_cheap(const Board& b, const PawnEntry& pawns) {
    const EvalParams& p = g_eval_params;
    const auto& passed = pawns.passed;
    int mg = pawns.mg, eg = pawns.eg;

    // ---- Dynamic passed-pawn terms (non-pawn occupancy; outside pawn hash) --
    for (int c = 0; c < NCOLORS; c++) {
        Color us   = Color(c);
        Color them = ~us;
        int sign   = (us == WHITE) ? 1 : -1;
        Bitboard pp = passed[c];
        while (pp) {
            Square psq = Square(pop_lsb(pp));
            Square stop = forward_square(us, psq);
            if (stop == SQ_NONE || (b.all_pieces() & sq_bb(stop)))
                continue;

            int rel_r = relative_rank(us, psq);
            mg += sign * (rel_r * p.pass_free_mg); TR_MG(PassFreeMg, 0, sign * rel_r);
            eg += sign * (rel_r * p.pass_free_eg); TR_EG(PassFreeEg, 0, sign * rel_r);

            if (!b.is_attacked_by(stop, b.all_pieces(), them)) {
                eg += sign * (rel_r * p.pass_safe_eg); TR_EG(PassSafeEg, 0, sign * rel_r);
            }
        }
    }
    return {mg, eg};
}

// The pawns family's full entry, after the lazy checkpoint: the terms that
// read the attack maps.
static EvalScore pawns_full(const Board& b, const AttackInfo& ai) {
    const EvalParams& p = g_eval_params;
    const auto& passed   = ai.passed;
    const auto& attacked = ai.attacked;
    int mg = 0, eg = 0;

    // ---- Rook behind passed pawn -------------------------------------------
    // Two independent passes: the enemy-rook penalty must not nest inside the
    // friendly-rook loop, or it would only fire when WE also have a rook on
    // the passer file (and fire once per friendly rook when stacked).
    // "Behind" stays purely geometric (blockers between rook and pawn are not
    // examined), matching the friendly-rook term. Audit hce_analysis 2.
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Color them = ~us;

        // Friendly rook behind own passer: reward per rook.
        Bitboard rooks = b.piece_bb(us, ROOK);
        while (rooks) {
            Square rsq = Square(pop_lsb(rooks));
            int f = file_of(rsq);
            Bitboard file_passers = passed[us] & BB_FILES[f];
            if (!file_passers) continue;
            const bool behind = (us == WHITE)
                ? rank_of(rsq) < rank_of(Square(lsb(file_passers)))
                : rank_of(rsq) > rank_of(Square(msb(file_passers)));
            if (behind) {
                mg += sign * p.rook_behind_passer_mg; TR_MG(RookBehindPasserMg, 0, sign);
                eg += sign * p.rook_behind_passer_eg; TR_EG(RookBehindPasserEg, 0, sign);
            }
        }

        // Enemy rook behind own passer: penalty per enemy rook, independent
        // of any friendly rook on the file.
        Bitboard enemy_rooks = b.piece_bb(them, ROOK);
        while (enemy_rooks) {
            Square er = Square(pop_lsb(enemy_rooks));
            int f = file_of(er);
            Bitboard file_passers = passed[us] & BB_FILES[f];
            if (!file_passers) continue;
            const bool behind = (us == WHITE)
                ? rank_of(er) < rank_of(Square(lsb(file_passers)))
                : rank_of(er) > rank_of(Square(msb(file_passers)));
            if (behind) {
                mg -= sign * p.enemy_rook_passer_mg; TR_MG(EnemyRookPasserMg, 0, -sign);
                eg -= sign * p.enemy_rook_passer_eg; TR_EG(EnemyRookPasserEg, 0, -sign);
            }
        }
    }

    // ---- Passed pawn king proximity + promotion-path safety (endgame) ------
    // The king-proximity term is unchanged; the Step-3.4 promotion-path terms
    // (path-safe / block-defended / king-to-block) are seeded 0 and use the
    // attack maps (so they live here, outside the pawn cache).
    for (int c = 0; c < NCOLORS; c++) {
        Color us = Color(c);
        Color them = ~us;
        int sign = (us == WHITE) ? 1 : -1;
        Bitboard pp = passed[us];
        while (pp) {
            Square psq = Square(pop_lsb(pp));
            int rel_r = (us == WHITE) ? rank_of(psq) : (RANK_8 - rank_of(psq));
            int own_dist = KING_DIST[b.king_square(us)][psq];
            int opp_dist = KING_DIST[b.king_square(them)][psq];
            eg += sign * (opp_dist - own_dist) * (p.prox_base + rel_r);
            TR_EG(ProxBase, 0, sign * (opp_dist - own_dist));
            // rel_r * (opp_dist-own_dist) is a frozen data contribution captured in rest
            TR_REST_EG(PASSER_RANK, sign * (opp_dist - own_dist) * rel_r);

            // Promotion-path safety (Step 3.4, seeded 0).
            Bitboard path = BB_PASSED_PAWN_MASK[us][psq] & BB_FILES[file_of(psq)];
            if (!(path & attacked[them])) {
                eg += sign * rel_r * p.passed_path_safe_eg;
                TR_EG(PassedPathSafeEg, 0, sign * rel_r);
            }
            Square block = forward_square(us, psq);
            if (block != SQ_NONE) {
                if (attacked[us] & sq_bb(block)) {
                    eg += sign * p.passed_block_defended_eg;
                    TR_EG(PassedBlockDefendedEg, 0, sign);
                }
                int own_bd = KING_DIST[b.king_square(us)][block];
                int opp_bd = KING_DIST[b.king_square(them)][block];
                eg += sign * (opp_bd - own_bd) * p.passed_king_block_eg;
                TR_EG(PassedKingBlockEg, 0, sign * (opp_bd - own_bd));
            }

        }
    }
    return {mg, eg};
}
