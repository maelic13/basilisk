#pragma once

#include "types.h"

// ---- Bit intrinsics --------------------------------------------------------
#ifdef _MSC_VER
#  include <intrin.h>
inline int lsb(Bitboard b) { unsigned long idx; _BitScanForward64(&idx, b); return (int)idx; }
inline int msb(Bitboard b) { unsigned long idx; _BitScanReverse64(&idx, b); return (int)idx; }
#else
inline int lsb(Bitboard b) { return __builtin_ctzll(b); }
inline int msb(Bitboard b) { return 63 - __builtin_clzll(b); }
#endif

inline int  pop_lsb(Bitboard& b)  { int s = lsb(b); b &= b - 1; return s; }
inline int  popcount(Bitboard b)  { return __builtin_popcountll(b); }
inline bool more_than_one(Bitboard b) { return b & (b - 1); }

inline Bitboard sq_bb(Square s) { return 1ULL << s; }
inline Bitboard sq_bb(int    s) { return 1ULL << s; }

// ---- Precomputed tables ----------------------------------------------------
extern Bitboard BB_SQUARES[SQUARE_NB];
extern Bitboard BB_FILES[FILE_NB];
extern Bitboard BB_RANKS[RANK_NB];
extern Bitboard BB_ADJACENT_FILES[FILE_NB];
extern Bitboard BB_FORWARD_RANKS[NCOLORS][RANK_NB];
extern Bitboard BB_PASSED_PAWN_MASK[NCOLORS][SQUARE_NB];
extern Bitboard BB_BETWEEN[SQUARE_NB][SQUARE_NB];
extern Bitboard BB_LINE[SQUARE_NB][SQUARE_NB];
extern int      KING_DIST[SQUARE_NB][SQUARE_NB];

// ---- Directional shift (compile-time direction, no wrap) -------------------
template<Direction D>
constexpr Bitboard shift(Bitboard b) {
    constexpr Bitboard notA = ~0x0101010101010101ULL;
    constexpr Bitboard notH = ~0x8080808080808080ULL;
    if constexpr (D == NORTH)      return b << 8;
    if constexpr (D == SOUTH)      return b >> 8;
    if constexpr (D == EAST)       return (b & notH) << 1;
    if constexpr (D == WEST)       return (b & notA) >> 1;
    if constexpr (D == NORTH_EAST) return (b & notH) << 9;
    if constexpr (D == NORTH_WEST) return (b & notA) << 7;
    if constexpr (D == SOUTH_EAST) return (b & notH) >> 7;
    if constexpr (D == SOUTH_WEST) return (b & notA) >> 9;
    return b;
}

// Call once at startup
void init_bitboards();
