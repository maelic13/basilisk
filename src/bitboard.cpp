#include "bitboard.h"
#include <algorithm>
#include <cmath>

Bitboard BB_SQUARES[SQUARE_NB];
Bitboard BB_FILES[FILE_NB];
Bitboard BB_RANKS[RANK_NB];
Bitboard BB_ADJACENT_FILES[FILE_NB];
Bitboard BB_FORWARD_RANKS[NCOLORS][RANK_NB];
Bitboard BB_PASSED_PAWN_MASK[NCOLORS][SQUARE_NB];
Bitboard BB_BETWEEN[SQUARE_NB][SQUARE_NB];
Bitboard BB_LINE[SQUARE_NB][SQUARE_NB];
int      KING_DIST[SQUARE_NB][SQUARE_NB];

void init_bitboards() {
    for (int s = 0; s < 64; s++)
        BB_SQUARES[s] = 1ULL << s;

    for (int f = 0; f < 8; f++)
        BB_FILES[f] = 0x0101010101010101ULL << f;

    for (int r = 0; r < 8; r++)
        BB_RANKS[r] = 0xFFULL << (r * 8);

    for (int f = 0; f < 8; f++)
        BB_ADJACENT_FILES[f] = (f > 0 ? BB_FILES[f-1] : 0)
                              | (f < 7 ? BB_FILES[f+1] : 0);

    // Forward ranks: ranks strictly in front of rank r
    for (int r = 0; r < 8; r++) {
        BB_FORWARD_RANKS[WHITE][r] = 0;
        for (int r2 = r + 1; r2 < 8; r2++)
            BB_FORWARD_RANKS[WHITE][r] |= BB_RANKS[r2];

        BB_FORWARD_RANKS[BLACK][r] = 0;
        for (int r2 = 0; r2 < r; r2++)
            BB_FORWARD_RANKS[BLACK][r] |= BB_RANKS[r2];
    }

    // Passed-pawn masks (file + adjacent files, forward ranks only)
    for (int s = 0; s < 64; s++) {
        int f = file_of(Square(s));
        int r = rank_of(Square(s));
        Bitboard fileMask = BB_FILES[f] | BB_ADJACENT_FILES[f];
        BB_PASSED_PAWN_MASK[WHITE][s] = fileMask & BB_FORWARD_RANKS[WHITE][r];
        BB_PASSED_PAWN_MASK[BLACK][s] = fileMask & BB_FORWARD_RANKS[BLACK][r];
    }

    // Between and line tables via ray stepping
    const int DR[8] = {1, -1, 0, 0, 1,  1, -1, -1};
    const int DF[8] = {0,  0, 1,-1, 1, -1,  1, -1};

    for (int a = 0; a < 64; a++) {
        for (int b = 0; b < 64; b++) {
            BB_BETWEEN[a][b] = 0;
            BB_LINE[a][b]    = 0;
        }

        int ar = rank_of(Square(a)), af = file_of(Square(a));

        for (int dir = 0; dir < 8; dir++) {
            int r = ar + DR[dir], f = af + DF[dir];
            Bitboard ray = 0;

            while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                int b = r * 8 + f;
                ray |= 1ULL << b;
                r += DR[dir];
                f += DF[dir];
            }

            // BB_LINE[a][b] = full ray through a in this direction
            // We'll fix up BB_LINE and BB_BETWEEN properly below
            (void)ray;
        }
    }

    // Proper between/line: two squares on same rank/file/diagonal
    for (int a = 0; a < 64; a++) {
        for (int b = 0; b < 64; b++) {
            if (a == b) continue;
            int ar = rank_of(Square(a)), af = file_of(Square(a));
            int br = rank_of(Square(b)), bf = file_of(Square(b));
            int dr = br - ar, df = bf - af;

            bool aligned = (dr == 0) || (df == 0) || (std::abs(dr) == std::abs(df));
            if (!aligned) continue;

            int sr = (dr > 0) - (dr < 0);
            int sf = (df > 0) - (df < 0);

            // Walk from a toward b (exclusive both ends = between)
            Bitboard between = 0;
            int r = ar + sr, f = af + sf;
            while (r != br || f != bf) {
                between |= 1ULL << (r * 8 + f);
                r += sr; f += sf;
            }
            BB_BETWEEN[a][b] = between;

            // Full line: extend ray both ways from a
            Bitboard line = 1ULL << a;
            r = ar + sr; f = af + sf;
            while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                line |= 1ULL << (r * 8 + f);
                r += sr; f += sf;
            }
            r = ar - sr; f = af - sf;
            while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                line |= 1ULL << (r * 8 + f);
                r -= sr; f -= sf;
            }
            BB_LINE[a][b] = line;
        }
    }

    // Chebyshev (king) distance
    for (int a = 0; a < 64; a++) {
        for (int b = 0; b < 64; b++) {
            KING_DIST[a][b] = std::max(
                std::abs(file_of(Square(a)) - file_of(Square(b))),
                std::abs(rank_of(Square(a)) - rank_of(Square(b)))
            );
        }
    }
}
