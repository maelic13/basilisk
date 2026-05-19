#pragma once

#include <cstdint>
#include <cassert>
#include <string>

using Bitboard = uint64_t;
using Key      = uint64_t;

static constexpr Bitboard BB_EMPTY = 0ULL;
static constexpr Bitboard BB_ALL   = ~0ULL;

// ---- Colors ----------------------------------------------------------------
enum Color : int { WHITE = 0, BLACK = 1, NCOLORS = 2 };
inline constexpr Color operator~(Color c) noexcept { return Color(c ^ 1); }

// ---- Piece types -----------------------------------------------------------
enum PieceType : int {
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6,
    PIECE_TYPE_NB = 7
};

// Piece encoding: color*8 + type.  NO_PIECE = 0.
enum Piece : int {
    NO_PIECE  = 0,
    W_PAWN=1, W_KNIGHT=2, W_BISHOP=3, W_ROOK=4, W_QUEEN=5, W_KING=6,
    B_PAWN=9, B_KNIGHT=10, B_BISHOP=11, B_ROOK=12, B_QUEEN=13, B_KING=14,
    PIECE_NB  = 15
};

inline constexpr Piece     make_piece(Color c, PieceType t) { return Piece((c << 3) | t); }
inline constexpr PieceType type_of(Piece p)                 { return PieceType(p & 7); }
inline constexpr Color     color_of(Piece p)                 { return Color(p >> 3); }

// ---- Squares ---------------------------------------------------------------
enum Square : int {
    A1=0,B1,C1,D1,E1,F1,G1,H1,
    A2,B2,C2,D2,E2,F2,G2,H2,
    A3,B3,C3,D3,E3,F3,G3,H3,
    A4,B4,C4,D4,E4,F4,G4,H4,
    A5,B5,C5,D5,E5,F5,G5,H5,
    A6,B6,C6,D6,E6,F6,G6,H6,
    A7,B7,C7,D7,E7,F7,G7,H7,
    A8,B8,C8,D8,E8,F8,G8,H8,
    SQ_NONE = 64, SQUARE_NB = 64
};

enum File : int { FILE_A=0,FILE_B,FILE_C,FILE_D,FILE_E,FILE_F,FILE_G,FILE_H, FILE_NB=8 };
enum Rank : int { RANK_1=0,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8, RANK_NB=8 };

enum Direction : int {
    NORTH=8, SOUTH=-8, EAST=1, WEST=-1,
    NORTH_EAST=9, NORTH_WEST=7,
    SOUTH_EAST=-7, SOUTH_WEST=-9
};

enum CastlingRights : int {
    NO_CASTLING  = 0,
    WK_CASTLE    = 1,
    WQ_CASTLE    = 2,
    BK_CASTLE    = 4,
    BQ_CASTLE    = 8,
    ALL_CASTLING = 15
};

inline constexpr File   file_of(Square s)            { return File(s & 7); }
inline constexpr Rank   rank_of(Square s)            { return Rank(s >> 3); }
inline constexpr Square make_square(File f, Rank r)  { return Square((r << 3) | f); }
inline constexpr Square flip_rank(Square s)          { return Square(s ^ 56); }

inline constexpr Rank relative_rank(Color c, Rank r) {
    return c == WHITE ? r : Rank(7 - r);
}
inline constexpr Rank relative_rank(Color c, Square s) {
    return relative_rank(c, rank_of(s));
}
inline constexpr Square relative_square(Color c, Square s) {
    return c == WHITE ? s : flip_rank(s);
}
inline constexpr Direction pawn_push(Color c) {
    return c == WHITE ? NORTH : SOUTH;
}
