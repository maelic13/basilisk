#pragma once

#include "types.h"

namespace Zobrist {
    extern Key PieceKeys[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
    extern Key SideKey;
    extern Key CastlingKeys[16];
    extern Key EpKeys[FILE_NB];

    void init();
}
