#pragma once

#include "types.h"
#include <cstdint>
#include <string>

// ---- 16-bit move encoding (8.6.10a) -----------------------------------------
// bits  0-5:  from square
// bits  6-11: to square
// bits 12-13: move type
// bits 14-15: promo piece type - KNIGHT offset (0=N,1=B,2=R,3=Q)
// The encoding always fit 16 bits (the TT has stored move16 forever); the
// carrier type now matches it. uint16_t, not int16_t: a queen promotion sets
// bit 15. Halves MoveList, the PV table, countermove and killer storage.
using Move = std::uint16_t;

enum MoveType : int {
    NORMAL     = 0,
    PROMOTION  = 1,
    EN_PASSANT = 2,
    CASTLING   = 3
};

static constexpr Move MOVE_NONE = 0;
static constexpr Move MOVE_NULL = 65;  // null-move sentinel

inline constexpr Move make_move(Square from, Square to) {
    return static_cast<Move>(int(from) | (int(to) << 6));
}
inline constexpr Move make_promotion(Square from, Square to, PieceType promo) {
    return static_cast<Move>(int(from) | (int(to) << 6) | (PROMOTION << 12)
         | ((int(promo) - int(KNIGHT)) << 14));
}
inline constexpr Move make_ep(Square from, Square to) {
    return static_cast<Move>(int(from) | (int(to) << 6) | (EN_PASSANT << 12));
}
inline constexpr Move make_castling(Square from, Square to) {
    return static_cast<Move>(int(from) | (int(to) << 6) | (CASTLING << 12));
}

inline constexpr Square   from_sq(Move m)    { return Square(m & 0x3F); }
inline constexpr Square   to_sq(Move m)      { return Square((m >> 6) & 0x3F); }
inline constexpr MoveType move_type(Move m)  { return MoveType((m >> 12) & 3); }
inline constexpr PieceType promo_type(Move m){ return PieceType(((m >> 14) & 3) + KNIGHT); }

// The TT's move16 and Move are now the same width; these remain as the
// documented conversion points (and would absorb any future re-widening).
inline constexpr int  move_to_tt(Move m)   { return m; }
inline constexpr Move move_from_tt(int m16){ return static_cast<Move>(m16 & 0xFFFF); }

std::string move_to_uci(Move m);
