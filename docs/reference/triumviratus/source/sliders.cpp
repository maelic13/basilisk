// Triumviratus, a UCI chess engine.
// Copyright (C) 2026 Francesco Torsello
// SPDX-License-Identifier: GPL-3.0-or-later (see COPYING)

// sliders.cpp -- tables of sliders.h, built once at startup (block 7 of 8.1, 18/10/2026).

#include "sliders.h"

namespace sliders {

Lines        g_lines[64];
LinesHQ      g_linesHQ[64];
#if defined(SLIDERS_HAVE_HQ)
LinesBishop  g_linesB[64];
#endif
std::uint8_t g_rank8[8][64];

void init() {
    for (int sq = 0; sq < 64; sq++) {
        const int r = sq >> 3, f = sq & 7;
        u64       line[4] = {0, 0, 0, 0};
        for (int s = 0; s < 64; s++) {
            if (s == sq) continue;
            const int r2 = s >> 3, f2 = s & 7;
            const u64 b  = 1ULL << s;
            if (r2 - f2 == r - f) line[0] |= b;   // diagonal
            if (f2 == f) line[1] |= b;            // file
            if (r2 + f2 == r + f) line[2] |= b;   // antidiagonal
            if (r2 == r) line[3] |= b;            // rank
        }
        const u64 below = (1ULL << sq) - 1;
        for (int k = 0; k < 4; k++) {
            g_lines[sq].line[k]  = line[k];
            g_lines[sq].below[k] = line[k] & below;
        }
        LinesHQ& H = g_linesHQ[sq];
        for (int k = 0; k < 4; k++) {
            H.line[k]    = k < 3 ? line[k] : 0;
            H.twoS[k]    = k < 3 ? 2ULL << sq : 0;          // 2 << 63 wraps to 0: the formula holds modulo 2^64
            H.twoSRev[k] = k < 3 ? 2ULL << (sq ^ 56) : 0;   // twice bswap(1 << sq)
            H.pad[k]     = 0;
        }
#if defined(SLIDERS_HAVE_HQ)
        LinesBishop& B = g_linesB[sq];
        B.line[0] = line[0];
        B.line[1] = line[2];
        B.twoS[0] = B.twoS[1] = 2ULL << sq;
        B.twoSRev[0] = B.twoSRev[1] = 2ULL << (sq ^ 56);
        B.file = line[1];
        B.pad  = 0;
#endif
    }
    // Rank table: a slider on file f, the six inner squares of its rank given by occ6 (files b..g), the outer two
    // always reached when nothing stands between.
    for (int f = 0; f < 8; f++)
        for (int occ6 = 0; occ6 < 64; occ6++) {
            const unsigned occ8 = unsigned(occ6) << 1;
            unsigned       att  = 0;
            for (int g = f + 1; g < 8; g++) {
                att |= 1u << g;
                if (occ8 & (1u << g)) break;
            }
            for (int g = f - 1; g >= 0; g--) {
                att |= 1u << g;
                if (occ8 & (1u << g)) break;
            }
            g_rank8[f][occ6] = std::uint8_t(att);
        }
}

}  // namespace sliders
