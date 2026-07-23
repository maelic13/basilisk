#pragma once

#include "types.h"
#include "bitboard.h"

// Precomputed non-sliding attack tables
extern Bitboard KnightAttacks[SQUARE_NB];
extern Bitboard KingAttacks[SQUARE_NB];
extern Bitboard PawnAttacks[NCOLORS][SQUARE_NB];

// Sliding attacks via magic bitboards
Bitboard bishop_attacks(Square sq, Bitboard occ);
Bitboard rook_attacks(Square sq, Bitboard occ);

inline Bitboard queen_attacks(Square sq, Bitboard occ) {
    return bishop_attacks(sq, occ) | rook_attacks(sq, occ);
}

// Must be called once at startup (after init_bitboards)
void init_attacks();

#if !defined(USE_PEXT)
// 8.7.10: squares whose baked magic failed verification and fell back to the
// live search during the last init_attacks(). 0 in a healthy build; the
// baked_magics_cover_every_square test asserts it. (PEXT tiers have no magics.)
extern int g_baked_magic_fallbacks;
#endif
