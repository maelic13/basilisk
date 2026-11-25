#pragma once

// The rebuilt evaluation's king family: the pawn shelter and storm, the king
// danger index from the attack maps, its conversion to a score, and the
// king's flank.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone when BASILISK_EVAL_REBUILD is defined. Include it nowhere else.

#include "eval_rb_pieces.h"

namespace rb {

// Structure constants of king danger: below the threshold the index costs
// nothing; above it the middlegame pays its square over 4096 and the endgame
// a sixteenth. The flank attack's square and the shelter enter over 8.
inline constexpr int KING_DANGER_THRESHOLD = 100;

template<Color Us>
Score king_terms(const Board& b, PawnEntry& e, const Maps& m) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    constexpr Bitboard CAMP = Us == WHITE ? ~(rank_bb(5) | rank_bb(6) | rank_bb(7))
                                          : ~(rank_bb(0) | rank_bb(1) | rank_bb(2));
    const EvalParams& p = g_eval_params;
    const Square ksq = b.king_square(Us);
    const Bitboard flank = KING_FLANK[file_of(ksq)];

    Score score = king_pawns<Us>(b, e);

    // Squares they attack that we defend at most once, by our king or queen.
    const Bitboard weak = m.attacked_by[Them][ALL_PIECES] & ~m.attacked_by2[Us]
                        & (~m.attacked_by[Us][ALL_PIECES] | m.attacked_by[Us][KING] | m.attacked_by[Us][QUEEN]);

    // Their safe checks next move: from squares we do not attack, or weak
    // squares they attack twice. A check by a stronger piece from a square
    // is not counted again for a weaker one there.
    Bitboard safe = ~b.occupancy_bb(Them);
    safe &= ~m.attacked_by[Us][ALL_PIECES] | (weak & m.attacked_by2[Them]);
    const Bitboard rook_lines = rook_attacks(ksq, b.all_pieces() ^ b.piece_bb(Us, QUEEN));
    const Bitboard bishop_lines = bishop_attacks(ksq, b.all_pieces() ^ b.piece_bb(Us, QUEEN));

    int danger = 0;
    Bitboard unsafe_checks = 0;
    const Bitboard rook_checks = rook_lines & m.attacked_by[Them][ROOK] & safe;
    if (rook_checks)
        danger += p.safe_check[ROOK * 2 + more_than_one(rook_checks)];
    else
        unsafe_checks |= rook_lines & m.attacked_by[Them][ROOK];

    const Bitboard queen_checks = (rook_lines | bishop_lines) & m.attacked_by[Them][QUEEN] & safe
                                & ~(m.attacked_by[Us][QUEEN] | rook_checks);
    if (queen_checks)
        danger += p.safe_check[QUEEN * 2 + more_than_one(queen_checks)];

    const Bitboard bishop_checks = bishop_lines & m.attacked_by[Them][BISHOP] & safe & ~queen_checks;
    if (bishop_checks)
        danger += p.safe_check[BISHOP * 2 + more_than_one(bishop_checks)];
    else
        unsafe_checks |= bishop_lines & m.attacked_by[Them][BISHOP];

    const Bitboard knight_checks = KnightAttacks[ksq] & m.attacked_by[Them][KNIGHT];
    if (knight_checks & safe)
        danger += p.safe_check[KNIGHT * 2 + more_than_one(knight_checks & safe)];
    else
        unsafe_checks |= knight_checks;

    // Their attacks in our king's flank, twice-attacked squares counted
    // again, and our defence there.
    const Bitboard flank_attacked = m.attacked_by[Them][ALL_PIECES] & flank & CAMP;
    const int flank_attack = popcount(flank_attacked) + popcount(flank_attacked & m.attacked_by2[Them]);
    const int flank_defence = popcount(m.attacked_by[Us][ALL_PIECES] & flank & CAMP);

    danger += m.king_attackers_count[Them] * m.king_attackers_weight[Them]
            + p.kd_weak_ring * popcount(m.king_ring[Us] & weak)
            + p.kd_unsafe_checks * popcount(unsafe_checks)
            + p.kd_blockers * popcount(m.blockers_for_king[Us])
            + p.kd_king_attacks * m.king_attacks_count[Them]
            + p.kd_flank_attack * flank_attack * flank_attack / 8
            + (m.mobility[Them].mg - m.mobility[Us].mg)
            - p.kd_no_queen * !b.piece_bb(Them, QUEEN)
            - p.kd_knight_defender * bool(m.attacked_by[Us][KNIGHT] & m.attacked_by[Us][KING])
            - p.kd_shelter * score.mg / 8
            - p.kd_flank_defence * flank_defence
            + p.kd_offset;

    if (danger > KING_DANGER_THRESHOLD) {
        score -= Score{danger * danger / 4096, danger / 16};
        RB_REST(KING_DANGER, -SIGN<Us> * (danger * danger / 4096), -SIGN<Us> * (danger / 16));
    }

    // A king on a flank without pawns.
    if (!(pieces(b, PAWN) & flank)) {
        score -= Score{p.pawnless_flank_mg, p.pawnless_flank_eg};
        RB_TR2(PawnlessFlank, 0, -SIGN<Us>);
    }

    // Pressure on the flank, which may move toward the king.
    score -= Score{p.flank_attacks_mg, p.flank_attacks_eg} * flank_attack;
    RB_TR2(FlankAttacks, 0, -SIGN<Us> * flank_attack);
    return score;
}

} // namespace rb
