#pragma once

#include "move.h"
#include "types.h"

namespace Zobrist {
    extern Key PieceKeys[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
    extern Key SideKey;
    extern Key CastlingKeys[16];
    extern Key EpKeys[FILE_NB];

    void init();

    // Cuckoo tables (Cluster 2's upcoming-repetition test, Marcel van
    // Kervinck's method in Stockfish's form): for every move of a non-pawn
    // piece between two squares on an empty board, the key
    // piece(from) ^ piece(to) ^ side and the move from the lower square,
    // each in one of its two slots. init() builds them and asserts all
    // CUCKOO_MOVES placed.
    inline constexpr int CUCKOO_SIZE  = 8192;
    inline constexpr int CUCKOO_MOVES = 3668;
    extern Key  Cuckoo[CUCKOO_SIZE];
    extern Move CuckooMove[CUCKOO_SIZE];
    inline int cuckoo_h1(Key h) { return int(h & 0x1fff); }
    inline int cuckoo_h2(Key h) { return int((h >> 16) & 0x1fff); }
}
