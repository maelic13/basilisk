#include "zobrist.h"

namespace Zobrist {

Key PieceKeys[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
Key SideKey;
Key CastlingKeys[16];
Key EpKeys[FILE_NB];

static uint64_t sm64_state_z;
static uint64_t sm64_next_z() {
    uint64_t z = (sm64_state_z += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void init() {
    sm64_state_z = 0xDEADBEEFCAFEBABEULL;

    for (int c = 0; c < NCOLORS; c++)
        for (int pt = 0; pt < PIECE_TYPE_NB; pt++)
            for (int sq = 0; sq < SQUARE_NB; sq++)
                PieceKeys[c][pt][sq] = sm64_next_z();

    SideKey = sm64_next_z();

    for (int i = 0; i < 16; i++)
        CastlingKeys[i] = sm64_next_z();

    for (int f = 0; f < FILE_NB; f++)
        EpKeys[f] = sm64_next_z();
}

} // namespace Zobrist
