#pragma once

// The rebuilt evaluation's attack maps and piece families: the pins the
// families read, the mobility area, the king rings, the four piece
// functions that fill the attack maps while they score, and mobility.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone. Include it nowhere else.

#include "eval_rb_pawns.h"

namespace rb {

inline constexpr int ALL_PIECES = 0;  // attacked_by[c][ALL_PIECES]: every attack of c

// What the families after the start read, filled in the reference's order:
// init_maps() for both colours, then the piece functions, which add each
// piece's attacks as they go.
struct Maps {
    Bitboard blockers_for_king[NCOLORS];   // pieces of either colour shielding c's king from a slider
    Bitboard mobility_area[NCOLORS];
    Bitboard attacked_by[NCOLORS][PIECE_TYPE_NB];
    Bitboard attacked_by2[NCOLORS];        // attacked at least twice, x-rays included
    Bitboard king_ring[NCOLORS];
    int      king_attackers_count[NCOLORS];   // c's pieces attacking the enemy king ring
    int      king_attackers_weight[NCOLORS];
    int      king_attacks_count[NCOLORS];     // c's attacks on squares next to the enemy king
    Score    mobility[NCOLORS];
    // Basilisk's own forms' inputs, filled only when their bit is compiled in:
    // its mobility area (only our own pinned pieces of the shielding set
    // excluded) and its king zone (the king, its neighbours and those one
    // rank forward).
    Bitboard own_mobility_area[NCOLORS];
    Bitboard own_king_zone[NCOLORS];
};

// Our pieces pinned to our king, as Basilisk's own mobility area reads them:
// alone on the line between the king and an enemy slider aligned with it.
inline Bitboard own_pinned(const Board& b, Color us) {
    const Color them = ~us;
    const Square ksq = b.king_square(us);
    Bitboard snipers = (rook_attacks(ksq, 0) & (b.piece_bb(them, ROOK) | b.piece_bb(them, QUEEN)))
                     | (bishop_attacks(ksq, 0) & (b.piece_bb(them, BISHOP) | b.piece_bb(them, QUEEN)));
    Bitboard pinned = 0;
    while (snipers) {
        const Bitboard between = BB_BETWEEN[ksq][pop_lsb(snipers)] & b.all_pieces();
        if (between && !more_than_one(between))
            pinned |= between & b.occupancy_bb(us);
    }
    return pinned;
}

// A piece's attacks on the plain occupancy: no x-ray and no pin line.
template<PieceType Pt>
inline Bitboard plain_attacks(const Board& b, Square s) {
    if constexpr (Pt == KNIGHT) return KnightAttacks[s];
    else if constexpr (Pt == BISHOP) return bishop_attacks(s, b.all_pieces());
    else if constexpr (Pt == ROOK) return rook_attacks(s, b.all_pieces());
    else return queen_attacks(s, b.all_pieces());
}

// The pieces of either colour that alone stand between s and one of
// `sliders` aligned with it (removing one would expose s).
inline Bitboard slider_blockers(const Board& b, Bitboard sliders, Square s) {
    const Bitboard snipers = ((rook_attacks(s, 0) & (pieces(b, QUEEN) | pieces(b, ROOK)))
                            | (bishop_attacks(s, 0) & (pieces(b, QUEEN) | pieces(b, BISHOP)))) & sliders;
    const Bitboard occupancy = b.all_pieces() ^ snipers;
    Bitboard blockers = 0;
    for (Bitboard sn = snipers; sn; ) {
        const Bitboard between = BB_BETWEEN[s][pop_lsb(sn)] & occupancy;
        if (between && !more_than_one(between))
            blockers |= between;
    }
    return blockers;
}

template<Color Us>
void init_maps(const Board& b, const PawnEntry& e, Maps& m) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    constexpr Bitboard LOW_RANKS = Us == WHITE ? rank_bb(1) | rank_bb(2) : rank_bb(6) | rank_bb(5);
    const Square ksq = b.king_square(Us);
    const Bitboard our_pawns = b.piece_bb(Us, PAWN);
    const Bitboard double_by_pawn = pawn_double_attacks<Us>(our_pawns);

    // Blocked pawns and those on their first two ranks, our king and queen,
    // the pieces shielding our king, and the squares their pawns attack are
    // outside our mobility area.
    const Bitboard immobile = our_pawns & (shift_down<Us>(b.all_pieces()) | LOW_RANKS);
    m.mobility_area[Us] = ~(immobile | b.piece_bb(Us, KING) | b.piece_bb(Us, QUEEN)
                            | m.blockers_for_king[Us] | e.attacks[Them]);

    m.attacked_by[Us][KING] = KingAttacks[ksq];
    m.attacked_by[Us][PAWN] = e.attacks[Us];
    m.attacked_by[Us][ALL_PIECES] = m.attacked_by[Us][KING] | m.attacked_by[Us][PAWN];
    m.attacked_by2[Us] = double_by_pawn | (m.attacked_by[Us][KING] & m.attacked_by[Us][PAWN]);

    // The king ring: the king's neighbourhood, moved in from the edge, less
    // the squares two of our pawns defend.
    const Square centre = make_square(File(std::clamp(int(file_of(ksq)), int(FILE_B), int(FILE_G))),
                                      Rank(std::clamp(int(rank_of(ksq)), int(RANK_2), int(RANK_7))));
    m.king_ring[Us] = KingAttacks[centre] | sq_bb(centre);
    m.king_attackers_count[Them] = popcount(m.king_ring[Us] & e.attacks[Them]);
    m.king_attacks_count[Them] = m.king_attackers_weight[Them] = 0;
    m.king_ring[Us] &= ~double_by_pawn;

    if constexpr (own_form(OWN_MOBILITY))
        m.own_mobility_area[Us] = ~(immobile | b.piece_bb(Us, KING) | b.piece_bb(Us, QUEEN)
                                    | own_pinned(b, Us) | e.attacks[Them]);
    if constexpr (own_form(OWN_KING_ZONE))
        m.own_king_zone[Us] = KingAttacks[ksq] | sq_bb(ksq) | shift_up<Us>(KingAttacks[ksq]);
}

// One side's pieces of one type: their terms, their mobility, and their
// attacks added to the maps.
template<Color Us, PieceType Pt>
Score piece_terms(const Board& b, const PawnEntry& e, Maps& m) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    constexpr Bitboard OUTPOST_RANKS = Us == WHITE ? rank_bb(3) | rank_bb(4) | rank_bb(5)
                                                   : rank_bb(4) | rank_bb(3) | rank_bb(2);
    const EvalParams& p = g_eval_params;
    const Square ksq = b.king_square(Us);
    [[maybe_unused]] constexpr int SG = SIGN<Us>;
    Score score;
    m.attacked_by[Us][Pt] = 0;

    for (Bitboard bb = b.piece_bb(Us, Pt); bb; ) {
        const Square s = Square(pop_lsb(bb));

        // Attacks, through our queens for the bishops and rooks, and through
        // our other rooks for a rook; a pinned piece keeps the pin's line.
        Bitboard att = Pt == BISHOP ? bishop_attacks(s, b.all_pieces() ^ pieces(b, QUEEN))
                     : Pt == ROOK   ? rook_attacks(s, b.all_pieces() ^ pieces(b, QUEEN) ^ b.piece_bb(Us, ROOK))
                     : Pt == QUEEN  ? queen_attacks(s, b.all_pieces())
                                    : KnightAttacks[s];
        if (m.blockers_for_king[Us] & sq_bb(s))
            att &= BB_LINE[ksq][s];

        m.attacked_by2[Us] |= m.attacked_by[Us][ALL_PIECES] & att;
        m.attacked_by[Us][Pt] |= att;
        m.attacked_by[Us][ALL_PIECES] |= att;

        if (att & m.king_ring[Them]) {
            m.king_attackers_count[Us]++;
            m.king_attackers_weight[Us] += p.king_attack_weight[Pt];
            m.king_attacks_count[Us] += popcount(att & m.attacked_by[Them][KING]);
        } else if (!own_form(OWN_KING_ZONE) && Pt == ROOK && (file_bb_of(s) & m.king_ring[Them])) {
            score += Score{p.rook_on_king_ring_mg, p.rook_on_king_ring_eg};
            RB_TR2(RookOnKingRing, 0, SG);
        } else if (!own_form(OWN_KING_ZONE) && Pt == BISHOP
                   && (bishop_attacks(s, pieces(b, PAWN)) & m.king_ring[Them])) {
            score += Score{p.bishop_on_king_ring_mg, p.bishop_on_king_ring_eg};
            RB_TR2(BishopOnKingRing, 0, SG);
        }
        // Basilisk's own king-zone pressure: a minor or a rook whose plain
        // attacks reach the enemy king's zone, whether or not king danger
        // counts it as an attacker.
        if constexpr (own_form(OWN_KING_ZONE) && Pt != QUEEN) {
            if (plain_attacks<Pt>(b, s) & m.own_king_zone[Them]) {
                if constexpr (Pt == ROOK) {
                    score += Score{p.own_rook_king_ring_mg, p.own_rook_king_ring_eg};
                    RB_TR2(OwnRookKingRing, 0, SG);
                } else {
                    score += Score{p.own_minor_king_ring_mg, p.own_minor_king_ring_eg};
                    RB_TR2(OwnMinorKingRing, 0, SG);
                }
            }
        }

        // The mobility count: the classical form's attack set and area, or
        // Basilisk's own (plain attacks, its own area). The maps are the
        // classical form's either way, and so is the trapped rook's count.
        const int mob_classical = popcount(att & m.mobility_area[Us]);
        const int mob = own_form(OWN_MOBILITY) ? popcount(plain_attacks<Pt>(b, s) & m.own_mobility_area[Us])
                                               : mob_classical;
        if constexpr (Pt == KNIGHT)      { m.mobility[Us] += Score{p.mob_n_mg[mob], p.mob_n_eg[mob]}; RB_TR2(MobN, mob, SG); }
        else if constexpr (Pt == BISHOP) { m.mobility[Us] += Score{p.mob_b_mg[mob], p.mob_b_eg[mob]}; RB_TR2(MobB, mob, SG); }
        else if constexpr (Pt == ROOK)   { m.mobility[Us] += Score{p.mob_r_mg[mob], p.mob_r_eg[mob]}; RB_TR2(MobR, mob, SG); }
        else                             { m.mobility[Us] += Score{p.mob_q_mg[mob], p.mob_q_eg[mob]}; RB_TR2(MobQ, mob, SG); }

        if constexpr (Pt == BISHOP || Pt == KNIGHT) {
            // An outpost: a square on ranks 4 to 6 our pawns defend and theirs
            // can never attack. A knight on a side outpost with nothing to
            // attack and at most one target on its wing is a bad one.
            // Basilisk's own outpost form replaces the knight's.
            const Bitboard outposts = OUTPOST_RANKS & m.attacked_by[Us][PAWN] & ~e.attacks_span[Them];
            const Bitboard targets = b.occupancy_bb(Them) & ~pieces(b, PAWN);
            const Bitboard wing = (sq_bb(s) & QUEEN_SIDE) ? QUEEN_SIDE : KING_SIDE;
            constexpr bool classical_knight_outpost = !(own_form(OWN_OUTPOST) && Pt == KNIGHT);
            if (classical_knight_outpost && Pt == KNIGHT && (outposts & sq_bb(s) & ~CENTER_FILES)
                && !(att & targets) && !more_than_one(targets & wing)) {
                score += Score{p.bad_outpost_mg, p.bad_outpost_eg};
                RB_TR2(BadOutpost, 0, SG);
            } else if (classical_knight_outpost && (outposts & sq_bb(s))) {
                score += Pt == BISHOP ? Score{p.outpost_mg[1], p.outpost_eg[1]}
                                      : Score{p.outpost_mg[0], p.outpost_eg[0]};
                RB_TR2(Outpost, Pt == BISHOP ? 1 : 0, SG);
            } else if (classical_knight_outpost && Pt == KNIGHT && (outposts & att & ~b.occupancy_bb(Us))) {
                score += Score{p.reachable_outpost_mg, p.reachable_outpost_eg};
                RB_TR2(ReachableOutpost, 0, SG);
            }
            if constexpr (own_form(OWN_OUTPOST) && Pt == KNIGHT) {
                // Basilisk's: on the fifth rank or beyond, defended by our pawn
                // and attacked by none of theirs now; and each square it could
                // hop to that our pawns defend and theirs do not attack.
                constexpr Bitboard ADVANCED = Us == WHITE ? rank_bb(4) | rank_bb(5) | rank_bb(6)
                                                          : rank_bb(3) | rank_bb(2) | rank_bb(1);
                if (relative_rank(Us, s) >= RANK_5 && (PawnAttacks[Them][s] & b.piece_bb(Us, PAWN))
                    && !(PawnAttacks[Us][s] & b.piece_bb(Them, PAWN))) {
                    score += Score{p.own_knight_outpost_mg, p.own_knight_outpost_eg};
                    RB_TR2(OwnKnightOutpost, 0, SG);
                }
                const Bitboard squares = e.attacks[Us] & ~e.attacks[Them] & ADVANCED;
                const int reach = popcount(KnightAttacks[s] & squares & ~b.occupancy_bb(Us));
                score += Score{p.own_reachable_outpost_mg, p.own_reachable_outpost_eg} * reach;
                RB_TR2(OwnReachableOutpost, 0, SG * reach);
            }

            // Shielded by a pawn directly in front: any pawn, or Basilisk's,
            // our own.
            if constexpr (own_form(OWN_MINOR_BEHIND_PAWN)) {
                if (shift_down<Us>(b.piece_bb(Us, PAWN)) & sq_bb(s)) {
                    score += Score{p.own_minor_behind_pawn_mg, p.own_minor_behind_pawn_eg};
                    RB_TR2(OwnMinorBehindPawn, 0, SG);
                }
            } else if (shift_down<Us>(pieces(b, PAWN)) & sq_bb(s)) {
                score += Score{p.minor_behind_pawn_mg, p.minor_behind_pawn_eg};
                RB_TR2(MinorBehindPawn, 0, SG);
            }

            // Far from our king.
            const int i = Pt == BISHOP ? 1 : 0;
            score -= Score{p.king_protector_mg[i], p.king_protector_eg[i]} * distance(ksq, s);
            RB_TR2(KingProtector, i, -SG * distance(ksq, s));

            if constexpr (Pt == BISHOP) {
                // Our pawns on the bishop's colour, more so behind blocked
                // centre pawns and less so with the bishop outside the chain;
                // or Basilisk's own, the plain count.
                const Bitboard blocked = b.piece_bb(Us, PAWN) & shift_down<Us>(b.all_pieces());
                const Bitboard colour = (DARK_SQUARES & sq_bb(s)) ? DARK_SQUARES : ~DARK_SQUARES;
                const int same_colour = popcount(b.piece_bb(Us, PAWN) & colour);
                if constexpr (own_form(OWN_BAD_BISHOP)) {
                    score += Score{p.own_bad_bishop_mg, p.own_bad_bishop_eg} * same_colour;
                    RB_TR2(OwnBadBishop, 0, SG * same_colour);
                } else {
                    const int bad = same_colour * (!(m.attacked_by[Us][PAWN] & sq_bb(s)) + popcount(blocked & CENTER_FILES));
                    score -= Score{p.bishop_pawns_mg, p.bishop_pawns_eg} * bad;
                    RB_TR2(BishopPawns, 0, -SG * bad);
                }

                // Basilisk's trapped bishop: no square to go to, before the
                // piece-count phase reaches its middle.
                if constexpr (own_form(OWN_TRAPPED_BISHOP)) {
                    if (piece_count_phase(b) < TOTAL_PHASE / 2
                        && !(bishop_attacks(s, b.all_pieces()) & ~b.occupancy_bb(Us))) {
                        score += Score{p.own_trapped_bishop_mg, p.own_trapped_bishop_eg};
                        RB_TR2(OwnTrappedBishop, 0, SG);
                    }
                }

                // Their pawns on its diagonals, through everything.
                const int xrayed = popcount(bishop_attacks(s, 0) & b.piece_bb(Them, PAWN));
                score -= Score{p.bishop_xray_pawns_mg, p.bishop_xray_pawns_eg} * xrayed;
                RB_TR2(BishopXrayPawns, 0, -SG * xrayed);

                // A long diagonal that sees both centre squares past the pawns.
                if (more_than_one(bishop_attacks(s, pieces(b, PAWN)) & CENTER)) {
                    score += Score{p.long_diagonal_bishop_mg, p.long_diagonal_bishop_eg};
                    RB_TR2(LongDiagonalBishop, 0, SG);
                }
            }
        }

        if constexpr (Pt == ROOK) {
            if constexpr (own_form(OWN_ROOK_QUEEN_FILE)) {
                if (file_bb_of(s) & b.piece_bb(Them, QUEEN)) {
                    score += Score{p.own_rook_queen_file_mg, p.own_rook_queen_file_eg};
                    RB_TR2(OwnRookQueenFile, 0, SG);
                }
            } else if (file_bb_of(s) & pieces(b, QUEEN)) {
                score += Score{p.rook_on_queen_file_mg, p.rook_on_queen_file_eg};
                RB_TR2(RookOnQueenFile, 0, SG);
            }
            if constexpr (own_form(OWN_ROOK_CLOSED_FILE)) {
                if ((b.piece_bb(Us, PAWN) & file_bb_of(s)) && (b.piece_bb(Them, PAWN) & file_bb_of(s))) {
                    score += Score{p.own_rook_closed_mg, p.own_rook_closed_eg};
                    RB_TR2(OwnRookClosed, 0, SG);
                }
            }

            if (!(b.piece_bb(Us, PAWN) & file_bb_of(s))) {
                const int open = !(b.piece_bb(Them, PAWN) & file_bb_of(s));
                score += Score{p.rook_on_file_mg[open], p.rook_on_file_eg[open]};
                RB_TR2(RookOnFile, open, SG);
            } else if (mob_classical <= 3) {
                // Trapped by its own king, worse when the king cannot castle.
                const int kf = file_of(ksq);
                if ((kf < FILE_E) == (int(file_of(s)) < kf)) {
                    const int rights = b.castling() & (Us == WHITE ? (WK_CASTLE | WQ_CASTLE)
                                                                   : (BK_CASTLE | BQ_CASTLE));
                    score -= Score{p.trapped_rook_mg, p.trapped_rook_eg} * (1 + !rights);
                    RB_TR2(TrappedRook, 0, -SG * (1 + !rights));
                }
            }
        }

        if constexpr (Pt == QUEEN) {
            // A relative pin or a discovered attack against the queen.
            if (slider_blockers(b, b.piece_bb(Them, ROOK) | b.piece_bb(Them, BISHOP), s)) {
                score -= Score{p.weak_queen_mg, p.weak_queen_eg};
                RB_TR2(WeakQueen, 0, -SG);
            }

            // On a weak square in their camp: one their pawns can never
            // attack, or Basilisk's, one no pawn of theirs attacks now.
            if constexpr (own_form(OWN_QUEEN_INFILTRATION)) {
                if (relative_rank(Us, s) > RANK_4 && !(e.attacks[Them] & sq_bb(s))) {
                    score += Score{p.own_queen_infiltration_mg, p.own_queen_infiltration_eg};
                    RB_TR2(OwnQueenInfiltration, 0, SG);
                }
            } else if (relative_rank(Us, s) > RANK_4 && (~e.attacks_span[Them] & sq_bb(s))) {
                score += Score{p.queen_infiltration_mg, p.queen_infiltration_eg};
                RB_TR2(QueenInfiltration, 0, SG);
            }
        }
    }

    // Basilisk's connected rooks: one rook's plain attacks reach the other.
    if constexpr (own_form(OWN_CONNECTED_ROOKS) && Pt == ROOK) {
        const Bitboard rooks = b.piece_bb(Us, ROOK);
        bool connected = false;
        for (Bitboard bb = rooks; bb && !connected; ) {
            const Square s = Square(pop_lsb(bb));
            connected = (rook_attacks(s, b.all_pieces()) & rooks & ~sq_bb(s)) != 0;
        }
        if (connected) {
            score += Score{p.own_connected_rooks_mg, p.own_connected_rooks_eg};
            RB_TR2(OwnConnectedRooks, 0, SG);
        }
    }
    return score;
}

} // namespace rb
