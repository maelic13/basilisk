// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
#pragma once
#ifndef MAGIC_H
#define MAGIC_H

#include "defs.h"

extern void init_sliders_attacks(int bishop);
// ⛔ 04/10/2026 - TRIED AND REMOVED as inline in this header: with the PGO + ThinLTO build +1.27% instructions and
// +0.77% cycles per node (xperf, 6 rounds, identical nodes). The compiler already inlined them where it pays; forcing them everywhere
// bloats the hot code. They stay in magic.cpp.
// ⛔ Night of 04/10/2026 - TRIED AND REMOVED the PER-LINE tables (rank with shift, file and diagonals with PEXT on <= 6
// bits: 4 x 64 x 64 x 8 = 128 KB instead of the 2.25 MB of the fancy tables): two small reads instead of one big one,
// identical nodes, but cycles +0.55% and instructions +0.33% (xperf, 6 rounds, PGO). The big tables do not miss the cache
// as much as it seems: the rows really read are few and hot (same lesson as on 10/09 with the packed tables).
extern U64 get_bishop_attacks(int square, U64 occupancy);
extern U64 get_rook_attacks(int square, U64 occupancy);
extern U64 get_queen_attacks(int square, U64 occupancy);

// AA1 (10/10/2026, speed, same attacks): bishop AND rook from the same square with the same occupancy in ONE
// call. With DualMagic (P4, 08/10) every get_bishop_attacks / get_rook_attacks already computes both attacks and
// throws one away: whoever asked for both for the same square (SEE, check squares, king legality, network threats) paid
// the same computation twice, out of line in magic.cpp. Counters (bench 15 on fens30): 9.8 computations per node, ~4.4
// duplicates. The pair comes back in an xmm register (Windows x64 ABI: __m128i in the return register), not in memory like
// the pointer-based get_both_attacks of P4b (08/10, discarded). Lane 0 = bishop, lane 1 = rook.
#if defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
typedef __m128i SliderPair;
static inline U64 pair_bishop(SliderPair v) { return (U64)_mm_cvtsi128_si64(v); }
static inline U64 pair_rook(SliderPair v) { return (U64)_mm_cvtsi128_si64(_mm_unpackhi_epi64(v, v)); }
#else
struct SliderPair { U64 b, r; };
static inline U64 pair_bishop(SliderPair v) { return v.b; }
static inline U64 pair_rook(SliderPair v) { return v.r; }
#endif
extern SliderPair get_slider_pair(int square, U64 occupancy);

#endif
