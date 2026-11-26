#pragma once

// The rebuilt evaluation's threats, passed pawns and space.
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone. Include it nowhere else.

#include "eval_rb_king.h"

namespace rb {

// Space counts only while this much non-pawn material is on the board.
inline constexpr int SPACE_THRESHOLD = 12222;

template<Color Us>
Score threat_terms(const Board& b, const Maps& m) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    constexpr Bitboard RELATIVE_RANK_3 = Us == WHITE ? rank_bb(2) : rank_bb(5);
    const EvalParams& p = g_eval_params;
    [[maybe_unused]] constexpr int SG = SIGN<Us>;
    Score score;

    const Bitboard non_pawn_enemies = b.occupancy_bb(Them) & ~pieces(b, PAWN);
    // Squares they hold strongly: by a pawn, or twice where we attack once.
    const Bitboard strongly_protected = m.attacked_by[Them][PAWN]
                                      | (m.attacked_by2[Them] & ~m.attacked_by2[Us]);
    const Bitboard defended = non_pawn_enemies & strongly_protected;
    const Bitboard weak = b.occupancy_bb(Them) & ~strongly_protected & m.attacked_by[Us][ALL_PIECES];

    if (defended | weak) {
        for (Bitboard bb = (defended | weak) & (m.attacked_by[Us][KNIGHT] | m.attacked_by[Us][BISHOP]); bb; ) {
            const PieceType pt = type_of(b.piece_on(pop_lsb(bb)));
            score += Score{p.threat_by_minor_mg[pt], p.threat_by_minor_eg[pt]};
            RB_TR2(ThreatByMinor, pt, SG);
        }
        for (Bitboard bb = weak & m.attacked_by[Us][ROOK]; bb; ) {
            const PieceType pt = type_of(b.piece_on(pop_lsb(bb)));
            score += Score{p.threat_by_rook_mg[pt], p.threat_by_rook_eg[pt]};
            RB_TR2(ThreatByRook, pt, SG);
        }
        if (weak & m.attacked_by[Us][KING]) {
            score += Score{p.threat_by_king_mg, p.threat_by_king_eg};
            RB_TR2(ThreatByKing, 0, SG);
        }

        const Bitboard hanging = ~m.attacked_by[Them][ALL_PIECES] | (non_pawn_enemies & m.attacked_by2[Us]);
        score += Score{p.hanging_mg, p.hanging_eg} * popcount(weak & hanging);
        RB_TR2(Hanging, 0, SG * popcount(weak & hanging));

        // A weak piece their queen alone protects.
        score += Score{p.weak_queen_protection_mg, p.weak_queen_protection_eg}
               * popcount(weak & m.attacked_by[Them][QUEEN]);
        RB_TR2(WeakQueenProtection, 0, SG * popcount(weak & m.attacked_by[Them][QUEEN]));
    }

    // Squares their pieces could use that we contest.
    const Bitboard restricted = m.attacked_by[Them][ALL_PIECES] & ~strongly_protected
                              & m.attacked_by[Us][ALL_PIECES];
    score += Score{p.restricted_piece_mg, p.restricted_piece_eg} * popcount(restricted);
    RB_TR2(RestrictedPiece, 0, SG * popcount(restricted));

    // Our relatively safe pawns attacking their pieces, now or after a push;
    // or Basilisk's own, any pawn of ours attacking a piece, by its type.
    Bitboard safe = ~m.attacked_by[Them][ALL_PIECES] | m.attacked_by[Us][ALL_PIECES];
    if constexpr (own_form(OWN_PAWN_THREAT)) {
        const Bitboard hit = m.attacked_by[Us][PAWN] & b.occupancy_bb(Them);
        const int minors = popcount(hit & (pieces(b, KNIGHT) | pieces(b, BISHOP)));
        const int rooks = popcount(hit & pieces(b, ROOK));
        const int queens = popcount(hit & pieces(b, QUEEN));
        score += Score{p.own_threat_minor_mg, p.own_threat_minor_eg} * minors
               + Score{p.own_threat_rook_mg, p.own_threat_rook_eg} * rooks
               + Score{p.own_threat_queen_mg, p.own_threat_queen_eg} * queens;
        RB_TR2(OwnThreatMinor, 0, SG * minors);
        RB_TR2(OwnThreatRook, 0, SG * rooks);
        RB_TR2(OwnThreatQueen, 0, SG * queens);
    } else {
        const Bitboard by_safe_pawn = pawn_attacks<Us>(b.piece_bb(Us, PAWN) & safe) & non_pawn_enemies;
        score += Score{p.threat_by_safe_pawn_mg, p.threat_by_safe_pawn_eg} * popcount(by_safe_pawn);
        RB_TR2(ThreatBySafePawn, 0, SG * popcount(by_safe_pawn));
    }

    Bitboard pushes = shift_up<Us>(b.piece_bb(Us, PAWN)) & ~b.all_pieces();
    pushes |= shift_up<Us>(pushes & RELATIVE_RANK_3) & ~b.all_pieces();
    pushes &= ~m.attacked_by[Them][PAWN] & safe;
    score += Score{p.threat_by_pawn_push_mg, p.threat_by_pawn_push_eg}
           * popcount(pawn_attacks<Us>(pushes) & non_pawn_enemies);
    RB_TR2(ThreatByPawnPush, 0, SG * popcount(pawn_attacks<Us>(pushes) & non_pawn_enemies));

    // Next-move threats against their only queen, doubled when it is the
    // only queen on the board.
    if (count(b, Them, QUEEN) == 1) {
        const int factor = 1 + (popcount(pieces(b, QUEEN)) == 1);
        const Square s = Square(lsb(b.piece_bb(Them, QUEEN)));
        safe = m.mobility_area[Us] & ~b.piece_bb(Us, PAWN) & ~strongly_protected;

        const Bitboard knights = m.attacked_by[Us][KNIGHT] & KnightAttacks[s];
        score += Score{p.knight_on_queen_mg, p.knight_on_queen_eg} * (popcount(knights & safe) * factor);
        RB_TR2(KnightOnQueen, 0, SG * popcount(knights & safe) * factor);

        const Bitboard sliders = (m.attacked_by[Us][BISHOP] & bishop_attacks(s, b.all_pieces()))
                               | (m.attacked_by[Us][ROOK] & rook_attacks(s, b.all_pieces()));
        score += Score{p.slider_on_queen_mg, p.slider_on_queen_eg}
               * (popcount(sliders & safe & m.attacked_by2[Us]) * factor);
        RB_TR2(SliderOnQueen, 0, SG * popcount(sliders & safe & m.attacked_by2[Us]) * factor);
    }
    return score;
}

// The passer tables' index: relative rank 3 to 6 (the fourth to the seventh)
// with a king distance 0 to 5, or with the free-path case 0 to 7.
inline constexpr int passer_distance_index(int r, int d) { return (r - 3) * 6 + d; }
inline constexpr int passer_path_index(int r, int path) { return (r - 3) * 8 + path; }

template<Color Us>
Score passer_terms(const Board& b, const PawnEntry& e, const Maps& m) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    const EvalParams& p = g_eval_params;
    [[maybe_unused]] constexpr int SG = SIGN<Us>;
    auto king_proximity = [&](Color c, Square s) { return std::min(distance(b.king_square(c), s), 5); };

    Bitboard passers = e.passed[Us];
    const Bitboard blocked = passers & shift_down<Us>(b.piece_bb(Them, PAWN));
    if (blocked) {
        // A blocked candidate counts only with a helper able to lever.
        const Bitboard helpers = shift_up<Us>(b.piece_bb(Us, PAWN)) & ~b.occupancy_bb(Them)
                               & (~m.attacked_by2[Them] | m.attacked_by[Us][ALL_PIECES]);
        passers &= ~blocked | shift<WEST>(helpers) | shift<EAST>(helpers);
    }

    Score score;
    while (passers) {
        const Square s = Square(pop_lsb(passers));
        const int r = relative_rank(Us, s);
        Score bonus{p.passed_rank_mg[r], p.passed_rank_eg[r]};
        RB_TR2(PassedRank, r, SG);

        if (r > RANK_3) {
            const Square block = Square(int(s) + int(pawn_push(Us)));
            const int theirs = passer_distance_index(r, king_proximity(Them, block));
            const int ours = passer_distance_index(r, king_proximity(Us, block));
            bonus.eg += p.passed_their_king_eg[theirs] - p.passed_our_king_eg[ours];
            TR_EG(PassedTheirKingEg, theirs, SG);
            TR_EG(PassedOurKingEg, ours, -SG);
            if (r != RANK_7) {
                const int next = passer_distance_index(r, king_proximity(Us, Square(int(block) + int(pawn_push(Us)))));
                bonus.eg -= p.passed_our_king_next_eg[next];
                TR_EG(PassedOurKingNextEg, next, -SG);
            }

            if (!(b.all_pieces() & sq_bb(block))) {
                const Bitboard to_queen = forward_file(Us, s);
                Bitboard unsafe = passed_pawn_span(Us, s);
                const Bitboard behind = forward_file(Them, s) & (pieces(b, ROOK) | pieces(b, QUEEN));
                if (!(b.occupancy_bb(Them) & behind))
                    unsafe &= m.attacked_by[Them][ALL_PIECES];

                // The path: unattacked, the queening line free, the block
                // square free, or none; then whether we hold the block square.
                const int path = !unsafe ? 0 : !(unsafe & to_queen) ? 1 : !(unsafe & sq_bb(block)) ? 2 : 3;
                const bool held = (b.occupancy_bb(Us) & behind) || (m.attacked_by[Us][ALL_PIECES] & sq_bb(block));
                const int i = passer_path_index(r, path * 2 + held);
                bonus += Score{p.passed_path_mg[i], p.passed_path_eg[i]};
                RB_TR2(PassedPath, i, SG);
            }
        }
        score += bonus - Score{p.passed_file_mg, p.passed_file_eg} * edge_distance(file_of(s));
        RB_TR2(PassedFile, 0, -SG * edge_distance(file_of(s)));
    }

    // Basilisk's rook behind a passer: each rook, ours or theirs, on the file
    // of one of our passers and behind the rearmost of them, from our side.
    if constexpr (own_form(OWN_ROOK_BEHIND_PASSER)) {
        auto behind = [&](Square r) {
            const Bitboard on_file = e.passed[Us] & file_bb_of(r);
            if (!on_file)
                return false;
            return Us == WHITE ? rank_of(r) < rank_of(Square(lsb(on_file)))
                               : rank_of(r) > rank_of(Square(msb(on_file)));
        };
        int ours = 0, theirs = 0;
        for (Bitboard bb = b.piece_bb(Us, ROOK); bb; )
            ours += behind(Square(pop_lsb(bb)));
        for (Bitboard bb = b.piece_bb(Them, ROOK); bb; )
            theirs += behind(Square(pop_lsb(bb)));
        score += Score{p.own_rook_behind_passer_mg, p.own_rook_behind_passer_eg} * ours
               + Score{p.own_enemy_rook_passer_mg, p.own_enemy_rook_passer_eg} * theirs;
        RB_TR2(OwnRookBehindPasser, 0, SG * ours);
        RB_TR2(OwnEnemyRookPasser, 0, SG * theirs);
    }
    return score;
}

template<Color Us>
Score space_terms(const Board& b, const PawnEntry& e, const Maps& m) {
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    if constexpr (own_form(OWN_SPACE)) {
        // Basilisk's three middlegame terms, at any material: the safe
        // squares on files c to f and our ranks 2 to 4 (no pawn of ours, none
        // of theirs attacking), alone, times our pieces bar pawns and the
        // king, and times our pawns with any piece directly ahead.
        constexpr Bitboard OWN_SPACE_MASK = Us == WHITE ? CENTER_FILES & (rank_bb(1) | rank_bb(2) | rank_bb(3))
                                                        : CENTER_FILES & (rank_bb(6) | rank_bb(5) | rank_bb(4));
        const EvalParams& p = g_eval_params;
        const int squares = popcount(OWN_SPACE_MASK & ~b.piece_bb(Us, PAWN) & ~e.attacks[Them]);
        const int pieces_bar_pawns = popcount(b.occupancy_bb(Us)) - count(b, Us, PAWN) - 1;
        const int blocked = popcount(b.piece_bb(Us, PAWN) & shift_down<Us>(b.all_pieces()));
        TR_MG(OwnSpaceMg, 0, SIGN<Us> * squares);
        TR_MG(OwnSpacePieceMg, 0, SIGN<Us> * squares * pieces_bar_pawns);
        TR_MG(OwnSpaceBlockedMg, 0, SIGN<Us> * squares * blocked);
        return Score{p.own_space_mg * squares + p.own_space_piece_mg * squares * pieces_bar_pawns
                     + p.own_space_blocked_mg * squares * blocked, 0};
    }
    if (non_pawn_material(b) < SPACE_THRESHOLD)
        return {};
    constexpr Bitboard SPACE_MASK = Us == WHITE ? CENTER_FILES & (rank_bb(1) | rank_bb(2) | rank_bb(3))
                                                : CENTER_FILES & (rank_bb(6) | rank_bb(5) | rank_bb(4));
    const EvalParams& p = g_eval_params;

    // Our safe central squares, counted again when at most three squares
    // behind one of our pawns and unattacked.
    const Bitboard safe = SPACE_MASK & ~b.piece_bb(Us, PAWN) & ~m.attacked_by[Them][PAWN];
    Bitboard behind = b.piece_bb(Us, PAWN);
    behind |= shift_down<Us>(behind);
    behind |= shift_down<Us>(shift_down<Us>(behind));

    const int bonus = popcount(safe) + popcount(behind & safe & ~m.attacked_by[Them][ALL_PIECES]);
    const int weight = popcount(b.occupancy_bb(Us)) - 3 + std::min(e.blocked_count, 9);
    // The weight in 256ths: 16 is the reference's sixteenth, exactly.
    RB_REST(SPACE, SIGN<Us> * (p.space_weight * bonus * weight * weight / 256), 0);
    return Score{p.space_weight * bonus * weight * weight / 256, 0};
}

} // namespace rb
