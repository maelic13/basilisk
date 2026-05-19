#pragma once

#include "types.h"
#include <string>

// ---- 32-bit move encoding --------------------------------------------------
// bits  0-5:  from square
// bits  6-11: to square
// bits 12-13: move type
// bits 14-15: promo piece type - KNIGHT offset (0=N,1=B,2=R,3=Q)
using Move = int;

enum MoveType : int {
    NORMAL     = 0,
    PROMOTION  = 1,
    EN_PASSANT = 2,
    CASTLING   = 3
};

static constexpr Move MOVE_NONE = 0;
static constexpr Move MOVE_NULL = 65;  // null-move sentinel

inline constexpr Move make_move(Square from, Square to) {
    return int(from) | (int(to) << 6);
}
inline constexpr Move make_promotion(Square from, Square to, PieceType promo) {
    return int(from) | (int(to) << 6) | (PROMOTION << 12)
         | ((int(promo) - int(KNIGHT)) << 14);
}
inline constexpr Move make_ep(Square from, Square to) {
    return int(from) | (int(to) << 6) | (EN_PASSANT << 12);
}
inline constexpr Move make_castling(Square from, Square to) {
    return int(from) | (int(to) << 6) | (CASTLING << 12);
}

inline constexpr Square   from_sq(Move m)    { return Square(m & 0x3F); }
inline constexpr Square   to_sq(Move m)      { return Square((m >> 6) & 0x3F); }
inline constexpr MoveType move_type(Move m)  { return MoveType((m >> 12) & 3); }
inline constexpr PieceType promo_type(Move m){ return PieceType(((m >> 14) & 3) + KNIGHT); }

// TT stores lower 16 bits (sufficient — top bits are unused)
inline constexpr int  move_to_tt(Move m)   { return m & 0xFFFF; }
inline constexpr Move move_from_tt(int m16){ return m16; }

std::string move_to_uci(Move m);
