/// Independent pseudo-legal move generator — TEST ORACLE ONLY.
///
/// This is a SECOND, deliberately separate implementation of move generation:
/// the differential tests assert that (pseudo-legal filtered by is_legal)
/// produces exactly the same move set as the engine's real b.gen_legal(), and
/// that both reproduce published perft counts. Two implementations that share
/// code cannot catch a bug in the code they share — the point is precisely
/// that this one is written differently (std::vector instead of MoveList, no
/// staging, no legality fast paths).
///
/// 8.6.2b: previously these were Board:: member functions living in
/// src/Board.cpp — ~300 lines of test-only code inside the production
/// translation unit, called from nowhere in the engine. Moving them here makes
/// the production/test boundary explicit and shrinks Board.cpp accordingly.
///
/// DO NOT "de-duplicate" this against b.gen_legal(). The duplication IS the test.
/// Keeping it in sync by hand is the accepted cost of having an oracle.

#pragma once

#include "board.h"

#include <vector>

namespace test_oracle {


// Castling-rights helpers. Deliberately re-stated here rather than exported
// from Board.cpp: the oracle's whole value is being an independent
// implementation, so it does not borrow the engine's internals (8.6.2b).
inline bool has_piece_on(const Board& b, Square sq, Color c, PieceType pt) {
    return (b.pieces[c][pt] & sq_bb(sq)) != 0;
}

inline bool can_castle_kingside(const Board& b, Color us) {
    const Rank back = us == WHITE ? RANK_1 : RANK_8;
    const Square king_from = make_square(FILE_E, back);
    const Square rook_from = make_square(FILE_H, back);
    const int right = us == WHITE ? WK_CASTLE : BK_CASTLE;
    return (b.castling_rights & right)
        && has_piece_on(b, king_from, us, KING)
        && has_piece_on(b, rook_from, us, ROOK);
}

inline bool can_castle_queenside(const Board& b, Color us) {
    const Rank back = us == WHITE ? RANK_1 : RANK_8;
    const Square king_from = make_square(FILE_E, back);
    const Square rook_from = make_square(FILE_A, back);
    const int right = us == WHITE ? WQ_CASTLE : BQ_CASTLE;
    return (b.castling_rights & right)
        && has_piece_on(b, king_from, us, KING)
        && has_piece_on(b, rook_from, us, ROOK);
}

inline void add_promotions(std::vector<Move>& moves, Square from, Square to) {
    moves.push_back(make_promotion(from, to, QUEEN));
    moves.push_back(make_promotion(from, to, ROOK));
    moves.push_back(make_promotion(from, to, BISHOP));
    moves.push_back(make_promotion(from, to, KNIGHT));
}

inline void gen_pseudo_legal(const Board& b, std::vector<Move>& moves) {
    // Data-member aliases so the body below is the original code verbatim
    // (8.6.2b move-out); Board's state is public by design.
    const Color&    side_to_move    = b.side_to_move;
    const Bitboard& all_occ         = b.all_occ;
    const auto&     occupancy       = b.occupancy;
    const auto&     pieces          = b.pieces;
    const Square&   ep_sq           = b.ep_sq;
    const auto&     king_sq         = b.king_sq;
    const int&      castling_rights = b.castling_rights;
    (void)king_sq; (void)castling_rights;

    Color us   = side_to_move;
    Color them = ~us;
    Bitboard friendly = occupancy[us];
    Bitboard enemy    = occupancy[them];
    Bitboard empty    = ~all_occ;

    // ---- Pawns ----
    {
        Bitboard pawns = pieces[us][PAWN];
        Bitboard promo_rank = (us == WHITE) ? BB_RANKS[RANK_7] : BB_RANKS[RANK_2];
        Bitboard push1 = (us == WHITE) ? shift<NORTH>(pawns & ~promo_rank)
                                       : shift<SOUTH>(pawns & ~promo_rank);
        push1 &= empty;
        Bitboard push2 = (us == WHITE) ? (shift<NORTH>(push1) & empty & BB_RANKS[RANK_4])
                                       : (shift<SOUTH>(push1) & empty & BB_RANKS[RANK_5]);

        // Single push
        Bitboard tmp = push1;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 8 : to + 8;
            moves.push_back(::make_move(Square(from), Square(to)));
        }
        // Double push
        tmp = push2;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 16 : to + 16;
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // Pawn promotions (from promotion rank)
        Bitboard promo_pawns = pawns & promo_rank;
        if (promo_pawns) {
            Bitboard promo_push = (us == WHITE) ? shift<NORTH>(promo_pawns)
                                                : shift<SOUTH>(promo_pawns);
            promo_push &= empty;
            tmp = promo_push;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 8 : to + 8;
                add_promotions(moves, Square(from), Square(to));
            }

            // Promotion captures
            Bitboard pcap_e = (us == WHITE) ? shift<NORTH_EAST>(promo_pawns)
                                            : shift<SOUTH_EAST>(promo_pawns);
            Bitboard pcap_w = (us == WHITE) ? shift<NORTH_WEST>(promo_pawns)
                                            : shift<SOUTH_WEST>(promo_pawns);
            pcap_e &= enemy;
            pcap_w &= enemy;
            tmp = pcap_e;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 9 : to + 7;
                add_promotions(moves, Square(from), Square(to));
            }
            tmp = pcap_w;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 7 : to + 9;
                add_promotions(moves, Square(from), Square(to));
            }
        }

        // Normal captures
        Bitboard cap_e = (us == WHITE) ? shift<NORTH_EAST>(pawns & ~promo_rank)
                                       : shift<SOUTH_EAST>(pawns & ~promo_rank);
        Bitboard cap_w = (us == WHITE) ? shift<NORTH_WEST>(pawns & ~promo_rank)
                                       : shift<SOUTH_WEST>(pawns & ~promo_rank);
        cap_e &= enemy;
        cap_w &= enemy;
        tmp = cap_e;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 9 : to + 7;
            moves.push_back(::make_move(Square(from), Square(to)));
        }
        tmp = cap_w;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 7 : to + 9;
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // En passant
        if (ep_sq != SQ_NONE) {
            Bitboard ep_attackers = PawnAttacks[them][ep_sq] & pawns;
            tmp = ep_attackers;
            while (tmp) {
                int from = pop_lsb(tmp);
                moves.push_back(make_ep(Square(from), ep_sq));
            }
        }
    }

    // ---- Knights ----
    {
        Bitboard knights = pieces[us][KNIGHT];
        while (knights) {
            int from = pop_lsb(knights);
            Bitboard att = KnightAttacks[from] & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- Bishops ----
    {
        Bitboard bishops = pieces[us][BISHOP];
        while (bishops) {
            int from = pop_lsb(bishops);
            Bitboard att = bishop_attacks(Square(from), all_occ) & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- Rooks ----
    {
        Bitboard rooks = pieces[us][ROOK];
        while (rooks) {
            int from = pop_lsb(rooks);
            Bitboard att = rook_attacks(Square(from), all_occ) & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- Queens ----
    {
        Bitboard queens = pieces[us][QUEEN];
        while (queens) {
            int from = pop_lsb(queens);
            Bitboard att = queen_attacks(Square(from), all_occ) & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- King ----
    {
        int from = king_sq[us];
        Bitboard att = KingAttacks[from] & ~friendly;
        while (att) {
            int to = pop_lsb(att);
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // Castling
        if (us == WHITE) {
            if (can_castle_kingside(b, WHITE)
                && !(all_occ & ((sq_bb(F1) | sq_bb(G1))))
                && !b.is_square_attacked(E1, BLACK)
                && !b.is_square_attacked(F1, BLACK)
                && !b.is_square_attacked(G1, BLACK)) {
                moves.push_back(make_castling(E1, G1));
            }
            if (can_castle_queenside(b, WHITE)
                && !(all_occ & (sq_bb(B1) | sq_bb(C1) | sq_bb(D1)))
                && !b.is_square_attacked(E1, BLACK)
                && !b.is_square_attacked(D1, BLACK)
                && !b.is_square_attacked(C1, BLACK)) {
                moves.push_back(make_castling(E1, C1));
            }
        } else {
            if (can_castle_kingside(b, BLACK)
                && !(all_occ & (sq_bb(F8) | sq_bb(G8)))
                && !b.is_square_attacked(E8, WHITE)
                && !b.is_square_attacked(F8, WHITE)
                && !b.is_square_attacked(G8, WHITE)) {
                moves.push_back(make_castling(E8, G8));
            }
            if (can_castle_queenside(b, BLACK)
                && !(all_occ & (sq_bb(B8) | sq_bb(C8) | sq_bb(D8)))
                && !b.is_square_attacked(E8, WHITE)
                && !b.is_square_attacked(D8, WHITE)
                && !b.is_square_attacked(C8, WHITE)) {
                moves.push_back(make_castling(E8, C8));
            }
        }
    }
}

inline void gen_pseudo_legal_captures(const Board& b, std::vector<Move>& moves) {
    // Data-member aliases so the body below is the original code verbatim
    // (8.6.2b move-out); Board's state is public by design.
    const Color&    side_to_move    = b.side_to_move;
    const Bitboard& all_occ         = b.all_occ;
    const auto&     occupancy       = b.occupancy;
    const auto&     pieces          = b.pieces;
    const Square&   ep_sq           = b.ep_sq;
    const auto&     king_sq         = b.king_sq;
    const int&      castling_rights = b.castling_rights;
    (void)king_sq; (void)castling_rights;

    Color us   = side_to_move;
    Color them = ~us;
    Bitboard enemy    = occupancy[them];

    // ---- Pawn captures & promotions ----
    {
        Bitboard pawns = pieces[us][PAWN];
        Bitboard promo_rank = (us == WHITE) ? BB_RANKS[RANK_7] : BB_RANKS[RANK_2];

        // Promotion pushes (queen only for now — queen captures ordering in qsearch)
        Bitboard promo_push = (us == WHITE) ? shift<NORTH>(pawns & promo_rank) & ~all_occ
                                            : shift<SOUTH>(pawns & promo_rank) & ~all_occ;
        Bitboard tmp = promo_push;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 8 : to + 8;
            add_promotions(moves, Square(from), Square(to));
        }

        // Promotion captures
        Bitboard promo_pawns = pawns & promo_rank;
        if (promo_pawns) {
            Bitboard pcap_e = (us == WHITE) ? shift<NORTH_EAST>(promo_pawns)
                                            : shift<SOUTH_EAST>(promo_pawns);
            Bitboard pcap_w = (us == WHITE) ? shift<NORTH_WEST>(promo_pawns)
                                            : shift<SOUTH_WEST>(promo_pawns);
            pcap_e &= enemy;
            pcap_w &= enemy;
            tmp = pcap_e;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 9 : to + 7;
                add_promotions(moves, Square(from), Square(to));
            }
            tmp = pcap_w;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 7 : to + 9;
                add_promotions(moves, Square(from), Square(to));
            }
        }

        // Normal pawn captures (not promo rank)
        Bitboard cap_e = (us == WHITE) ? shift<NORTH_EAST>(pawns & ~promo_rank)
                                       : shift<SOUTH_EAST>(pawns & ~promo_rank);
        Bitboard cap_w = (us == WHITE) ? shift<NORTH_WEST>(pawns & ~promo_rank)
                                       : shift<SOUTH_WEST>(pawns & ~promo_rank);
        cap_e &= enemy;
        cap_w &= enemy;
        tmp = cap_e;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 9 : to + 7;
            moves.push_back(::make_move(Square(from), Square(to)));
        }
        tmp = cap_w;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 7 : to + 9;
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // En passant
        if (ep_sq != SQ_NONE) {
            Bitboard ep_atk = PawnAttacks[them][ep_sq] & pawns;
            tmp = ep_atk;
            while (tmp) {
                int from = pop_lsb(tmp);
                moves.push_back(make_ep(Square(from), ep_sq));
            }
        }
    }

    // ---- Other pieces: captures only ----
    auto gen_piece_caps = [&](PieceType pt) {
        Bitboard pcs = pieces[us][pt];
        while (pcs) {
            int from = pop_lsb(pcs);
            Bitboard att;
            switch (pt) {
                case KNIGHT: att = KnightAttacks[from]; break;
                case BISHOP: att = bishop_attacks(Square(from), all_occ); break;
                case ROOK:   att = rook_attacks(Square(from), all_occ); break;
                case QUEEN:  att = queen_attacks(Square(from), all_occ); break;
                case KING:   att = KingAttacks[from]; break;
                default:     att = 0; break;
            }
            att &= enemy;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    };

    gen_piece_caps(KNIGHT);
    gen_piece_caps(BISHOP);
    gen_piece_caps(ROOK);
    gen_piece_caps(QUEEN);
    gen_piece_caps(KING);
}
} // namespace test_oracle
