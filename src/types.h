#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <string>

// ---- Platform contract (8.6.2a, 2026-07-20) --------------------------------
// Basilisk is 64-bit-only, and always has been in practice: the bitboard
// primitives call the unconditional 64-bit builtins (no 32-bit SWAR fallback
// was ever written), every shipped asset is x86-64 or aarch64, and the TT's
// `mb * 1024 * 1024` byte math would overflow a 32-bit size_t at Hash >= 4096.
// Making the assumption explicit turns a would-be silent miscompile into a
// build failure, and retroactively proves the `Key & (SIZE - 1)` table indices
// lossless. There is no 32-bit code path to maintain — this is the contract,
// not a portability aspiration.
static_assert(sizeof(std::size_t) >= 8, "Basilisk requires a 64-bit target (x86-64 or aarch64)");
static_assert(sizeof(void*) >= 8, "Basilisk requires 64-bit pointers");

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
inline constexpr Square flip_file(Square s)          { return Square(s ^ 7); }

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

// ---- Square arithmetic ------------------------------------------------------
// 8.6.2b: square/direction maths used to be written `Square(int(s) + off)` at 22
// sites. The enums stay plain `enum : int` deliberately — bitboard and index
// maths wants the implicit int conversion, and enum class would ADD casts rather
// than remove them — but the offset arithmetic deserves to read as arithmetic.
// These are the Stockfish-style incremental operators; all constexpr, so codegen
// is unchanged. Results are NOT range-checked: an offset that leaves the board is
// caught by sq_bb()'s debug assert at the point of use, which is where the
// meaningful context is.
inline constexpr Square operator+(Square s, int d) noexcept { return Square(int(s) + d); }
inline constexpr Square operator-(Square s, int d) noexcept { return Square(int(s) - d); }
inline constexpr Square operator+(Square s, Direction d) noexcept { return Square(int(s) + int(d)); }
inline constexpr Square operator-(Square s, Direction d) noexcept { return Square(int(s) - int(d)); }
inline constexpr Square& operator+=(Square& s, Direction d) noexcept { return s = s + d; }
inline constexpr Square& operator-=(Square& s, Direction d) noexcept { return s = s - d; }
inline constexpr Direction operator-(Square a, Square b) noexcept { return Direction(int(a) - int(b)); }
