#pragma once

// The squares the side not to move attacks, graded by the cheapest attacker
// class, for move ordering: a quiet move that takes a piece off a square a
// lesser piece attacks is an escape, and one that steps onto such a square
// walks into a threat. Computed on demand from the attack tables; the board
// keeps no state for it.

#include "board.h"
#include "types.h"

struct Threats {
    Bitboard by_pawn  = 0;  // pawn attacks
    Bitboard by_minor = 0;  // knight and bishop attacks, plus by_pawn
    Bitboard by_rook  = 0;  // rook attacks, plus by_minor
    Bitboard all      = 0;  // every attack, queen and king included
};

// The attacks of ~us. Sliders see through the full occupancy.
[[nodiscard]] Threats threats_against(const Board& b, Color us);
