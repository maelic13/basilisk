#pragma once

// The rebuilt evaluation's pawn family and the king's pawn shelter: the
// pawn-structure score with what the later families read from it (passers,
// pawn attacks and their span, blocked pawns), cached by pawn structure, and
// the shelter and storm, cached in the same entry by king square and
// castling rights.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone. Include it nowhere else.

#include "eval_rb_common.h"

namespace rb {

// The connected-pawn table's index: relative rank (1 to 6), whether the pawn
// has a phalanx neighbour, whether it is opposed, and its supporters (0 to 2).
inline constexpr int connected_index(int r, bool phalanx, bool opposed, int support) {
    return ((r - 1) * 4 + int(phalanx) * 2 + int(opposed)) * 3 + support;
}

// One side's pawn structure, White's sign applied by the caller. Fills the
// entry's passers, pawn attacks and attack span for Us, and adds Us's
// blocked pawns to its count.
template<Color Us>
Score pawn_structure(const Board& b, PawnEntry& e) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    const EvalParams& p = g_eval_params;
    const Bitboard ours = b.piece_bb(Us, PAWN), theirs = b.piece_bb(Them, PAWN);
    const Bitboard double_attack_them = pawn_double_attacks<Them>(theirs);

    e.passed[Us] = 0;
    e.attacks[Us] = e.attacks_span[Us] = pawn_attacks<Us>(ours);
    e.blocked_count += popcount(shift_up<Us>(ours) & (theirs | double_attack_them));

    Score score;
    for (Bitboard bb = ours; bb; ) {
        const Square s = Square(pop_lsb(bb));
        const int r = relative_rank(Us, s);
        const Square up = Square(int(s) + int(pawn_push(Us)));
        const Square down = Square(int(s) - int(pawn_push(Us)));

        const Bitboard opposed    = theirs & forward_file(Us, s);
        const Bitboard blocked    = theirs & sq_bb(up);
        const Bitboard stoppers   = theirs & passed_pawn_span(Us, s);
        const Bitboard lever      = theirs & PawnAttacks[Us][s];
        const Bitboard lever_push = theirs & PawnAttacks[Us][up];
        const bool     doubled    = ours & sq_bb(down);
        const Bitboard neighbours = ours & adjacent_files(s);
        const Bitboard phalanx    = neighbours & rank_bb_of(s);
        const Bitboard support    = neighbours & rank_bb_of(down);

        // Backward: behind every pawn of ours on the adjacent files, and
        // unable to advance safely.
        const bool backward = !(neighbours & forward_ranks(Them, up)) && (lever_push | blocked);

        if (!backward && !blocked)
            e.attacks_span[Us] |= pawn_attack_span(Us, s);

        // Passed: no stoppers but levers; or only push-levers, outnumbered
        // by the phalanx; or one blocker on the fifth rank or beyond that a
        // supported push can lever. The passer family refines it.
        bool passed = !(stoppers ^ lever)
                   || (!(stoppers ^ lever_push) && popcount(phalanx) >= popcount(lever_push))
                   || (stoppers == blocked && r >= RANK_5
                       && (shift_up<Us>(support) & ~(theirs | double_attack_them)));
        passed &= !(forward_file(Us, s) & ours);
        if (passed)
            e.passed[Us] |= sq_bb(s);

        [[maybe_unused]] constexpr int SG = SIGN<Us>;
        // Basilisk's own classification replaces the classical doubled and
        // backward scores; the backward flag above still shapes the span.
        constexpr bool CLASSICAL = !own_form(OWN_PAWN_CLASS);
        if (support | phalanx) {
            const int i = connected_index(r, phalanx != 0, opposed != 0, popcount(support));
            score += Score{p.connected_mg[i], p.connected_eg[i]};
            RB_TR2(Connected, i, SG);
        } else if (!neighbours) {
            if (CLASSICAL && opposed && (ours & forward_file(Them, s)) && !(theirs & adjacent_files(s))) {
                score -= Score{p.doubled_mg, p.doubled_eg};
                RB_TR2(Doubled, 0, -SG);
            } else {
                score -= Score{p.isolated_mg, p.isolated_eg};
                RB_TR2(Isolated, 0, -SG);
                if (CLASSICAL && !opposed) {
                    score -= Score{p.weak_unopposed_mg, p.weak_unopposed_eg};
                    RB_TR2(WeakUnopposed, 0, -SG);
                }
            }
        } else if (CLASSICAL && backward) {
            score -= Score{p.backward_mg, p.backward_eg};
            RB_TR2(Backward, 0, -SG);
            if (!opposed) {
                score -= Score{p.weak_unopposed_mg, p.weak_unopposed_eg};
                RB_TR2(WeakUnopposed, 0, -SG);
            }
        }

        if constexpr (!CLASSICAL) {
            // Backward: no pawn of ours strictly behind on an adjacent file,
            // and its stop square attacked by one of theirs; a phalanx
            // neighbour does not prevent it. Doubled: another pawn of ours
            // on its file, counted for each.
            const bool own_backward = !(ours & pawn_attack_span(Them, s)) && lever_push;
            if (own_backward) {
                score += Score{p.own_backward_mg, p.own_backward_eg};
                RB_TR2(OwnBackward, 0, SG);
            }
            if (more_than_one(ours & file_bb_of(s))) {
                score += Score{p.own_doubled_mg, p.own_doubled_eg};
                RB_TR2(OwnDoubled, 0, SG);
            }
            if ((!neighbours || own_backward) && !opposed) {
                score -= Score{p.weak_unopposed_mg, p.weak_unopposed_eg};
                RB_TR2(WeakUnopposed, 0, -SG);
            }
        }

        if (!support) {
            if (CLASSICAL && doubled) {
                score -= Score{p.doubled_mg, p.doubled_eg};
                RB_TR2(Doubled, 0, -SG);
            }
            if (more_than_one(lever)) {
                score -= Score{p.weak_lever_mg, p.weak_lever_eg};
                RB_TR2(WeakLever, 0, -SG);
            }
        }

        if (blocked && r > RANK_4) {
            score += Score{p.blocked_pawn_mg[r - RANK_5], p.blocked_pawn_eg[r - RANK_5]};
            RB_TR2(BlockedPawn, r - RANK_5, SG);
        }
    }

    // Basilisk's majority: more of our pawns than theirs on a wing (files a
    // to d, e to h), each wing once.
    if constexpr (own_form(OWN_MAJORITY)) {
        for (const Bitboard wing : {QUEEN_SIDE, KING_SIDE}) {
            if (popcount(ours & wing) > popcount(theirs & wing)) {
                score += Score{p.own_pawn_majority_mg, p.own_pawn_majority_eg};
                RB_TR2(OwnPawnMajority, 0, SIGN<Us>);
            }
        }
    }
    // Basilisk's islands: our groups of adjacent files holding pawns.
    if constexpr (own_form(OWN_ISLANDS)) {
        int islands = 0;
        bool previous = false;
        for (int f = 0; f < 8; f++) {
            const bool has = (ours & file_bb(f)) != 0;
            islands += has && !previous;
            previous = has;
        }
        score += Score{p.own_pawn_islands_mg, p.own_pawn_islands_eg} * islands;
        RB_TR2(OwnPawnIslands, 0, SIGN<Us> * islands);
    }
    return score;
}

// The pawn entry for b's pawns: probed, or computed and stored. Its key is
// never 0 for a real structure, so an empty slot answers for none. A traced
// build always recomputes, and counts nothing: it has no cache to size.
inline PawnEntry& pawn_probe(const Board& b, PawnEntry (&table)[PAWN_TABLE_SIZE],
                             [[maybe_unused]] int64_t& probes, [[maybe_unused]] int64_t& hits) {
    const Key key = b.pawn_key_value() ^ eval_ablation_key() ^ 0x5D1C4F0A7E3B9286ULL;
    PawnEntry& e = table[key & (PAWN_TABLE_SIZE - 1)];
    if constexpr (!TRACED) {
        DIAG_COUNT(++probes);
        if (e.key == key) {
            DIAG_COUNT(++hits);
            return e;
        }
    }
    e.key = key;
    e.blocked_count = 0;
    const Score w = pawn_structure<WHITE>(b, e);
    const Score k = pawn_structure<BLACK>(b, e);
    e.score[WHITE][0] = w.mg; e.score[WHITE][1] = w.eg;
    e.score[BLACK][0] = k.mg; e.score[BLACK][1] = k.eg;
    e.king_square[WHITE] = e.king_square[BLACK] = SQ_NONE;
    return e;
}

// The shelter of our pawns and the storm of theirs for a king on ksq. Trace
// writes its coefficients, for the square the king's term chose.
template<Color Us, bool Trace = false>
Score shelter(const Board& b, const PawnEntry& e, Square ksq) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    const EvalParams& p = g_eval_params;
    const Bitboard not_behind = pieces(b, PAWN) & ~forward_ranks(Them, ksq);
    const Bitboard ours = not_behind & b.piece_bb(Us, PAWN) & ~e.attacks[Them];
    const Bitboard theirs = not_behind & b.piece_bb(Them, PAWN);

    // Every shelter starts at the same constant, which cancels between the
    // two kings: a structure constant, not a slot.
    Score bonus{5, 5};
    const int center = std::clamp(int(file_of(ksq)), int(FILE_B), int(FILE_G));
    for (int f = center - 1; f <= center + 1; f++) {
        const Bitboard our_file = ours & file_bb(f), their_file = theirs & file_bb(f);
        const int our_rank = our_file ? int(relative_rank(Us, frontmost(Them, our_file))) : 0;
        const int their_rank = their_file ? int(relative_rank(Us, frontmost(Them, their_file))) : 0;
        const int d = edge_distance(f);
        bonus.mg += p.shelter_strength[d * 7 + our_rank];
        if (our_rank && our_rank == their_rank - 1)
            bonus -= Score{p.blocked_storm_mg[their_rank], p.blocked_storm_eg[their_rank]};
        else
            bonus.mg -= p.unblocked_storm[d * 7 + their_rank];
        if constexpr (Trace) {
            TR_MG(ShelterStrength, d * 7 + our_rank, SIGN<Us>);
            if (our_rank && our_rank == their_rank - 1)
                RB_TR2(BlockedStorm, their_rank, -SIGN<Us>);
            else
                TR_MG(UnblockedStorm, d * 7 + their_rank, -SIGN<Us>);
        }
    }
    return bonus;
}

// The king's pawn term: the best shelter over the king's square and the
// squares it may still castle to, by middlegame value, less the distance to
// its nearest pawn in the endgame. Cached in the pawn entry under the king
// square and castling rights.
template<Color Us>
Score king_pawns(const Board& b, PawnEntry& e) {
    const Square ksq = b.king_square(Us);
    const int rights = b.castling() & (Us == WHITE ? (WK_CASTLE | WQ_CASTLE) : (BK_CASTLE | BQ_CASTLE));
    if (e.king_square[Us] == ksq && e.castling[Us] == rights && !TRACED)
        return Score{e.king_safety[Us][0], e.king_safety[Us][1]};

    const EvalParams& p = g_eval_params;
    Score best = shelter<Us>(b, e, ksq);
    [[maybe_unused]] Square chosen = ksq;
    if (rights & (Us == WHITE ? WK_CASTLE : BK_CASTLE)) {
        const Score s = shelter<Us>(b, e, relative_square(Us, G1));
        if (s.mg > best.mg) {
            best = s;
            chosen = relative_square(Us, G1);
        }
    }
    if (rights & (Us == WHITE ? WQ_CASTLE : BQ_CASTLE)) {
        const Score s = shelter<Us>(b, e, relative_square(Us, C1));
        if (s.mg > best.mg) {
            best = s;
            chosen = relative_square(Us, C1);
        }
    }
    if constexpr (TRACED)
        shelter<Us, true>(b, e, chosen);

    Bitboard pawns = b.piece_bb(Us, PAWN);
    int min_distance = 6;
    if (pawns & KingAttacks[ksq])
        min_distance = 1;
    else
        while (pawns)
            min_distance = std::min(min_distance, distance(ksq, Square(pop_lsb(pawns))));
    best.eg -= p.king_pawn_distance_eg * min_distance;
    TR_EG(KingPawnDistanceEg, 0, -SIGN<Us> * min_distance);

    e.king_square[Us] = ksq;
    e.castling[Us] = rights;
    e.king_safety[Us][0] = best.mg;
    e.king_safety[Us][1] = best.eg;
    return best;
}

} // namespace rb
