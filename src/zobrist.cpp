#include "zobrist.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace Zobrist {

Key PieceKeys[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
Key SideKey;
Key CastlingKeys[16];
Key EpKeys[FILE_NB];
Key  Cuckoo[CUCKOO_SIZE];
Move CuckooMove[CUCKOO_SIZE];

// Whether a piece of this type moves between the squares on an empty board
// (geometry alone, so the tables need no attack tables at start-up).
static bool reaches(int pt, int a, int b) {
    const int df = std::abs((a & 7) - (b & 7)), dr = std::abs((a >> 3) - (b >> 3));
    const bool line = df == 0 || dr == 0, diag = df == dr;
    switch (pt) {
        case KNIGHT: return (df == 1 && dr == 2) || (df == 2 && dr == 1);
        case BISHOP: return diag;
        case ROOK:   return line;
        case QUEEN:  return line || diag;
        case KING:   return df <= 1 && dr <= 1;
        default:     return false;
    }
}

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

    for (int i = 0; i < CUCKOO_SIZE; i++) {
        Cuckoo[i] = 0;
        CuckooMove[i] = MOVE_NONE;
    }
    [[maybe_unused]] int count = 0;
    for (int c = 0; c < NCOLORS; c++)
        for (int pt = KNIGHT; pt <= KING; pt++)
            for (int s1 = 0; s1 < SQUARE_NB; s1++)
                for (int s2 = s1 + 1; s2 < SQUARE_NB; s2++) {
                    if (!reaches(pt, s1, s2))
                        continue;
                    Move move = make_move(Square(s1), Square(s2));
                    Key key = PieceKeys[c][pt][s1] ^ PieceKeys[c][pt][s2] ^ SideKey;
                    int i = cuckoo_h1(key);
                    while (true) {
                        std::swap(Cuckoo[i], key);
                        std::swap(CuckooMove[i], move);
                        if (move == MOVE_NONE)
                            break;
                        i = (i == cuckoo_h1(key)) ? cuckoo_h2(key) : cuckoo_h1(key);
                    }
                    count++;
                }
    assert(count == CUCKOO_MOVES);
}

} // namespace Zobrist
