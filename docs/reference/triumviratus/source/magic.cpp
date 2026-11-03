// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
#include "defs.h"
#include "attacks.h"
#include "magic.h"
#if defined(USE_PEXT)
#include <immintrin.h>   // _pext_u64 (BMI2): sliding-attack index without a multiplication
#endif

// The search for magic numbers (find_magic_number, init_magic_numbers, from BBC) was never called: the magic
// numbers are constants in the code. Removed on 28/10/2026 with the 32-bit generator it used (random.cpp).

// init slider piece's attack tables
void init_sliders_attacks(int bishop)
{
    for (int square = 0; square < 64; square++)
    {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);
        int occupancy_indices = (1 << relevant_bits_count);

        for (int index = 0; index < occupancy_indices; index++)
        {
            if (bishop)
            {
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
#if defined(USE_PEXT)
                int magic_index = (int)_pext_u64(occupancy, bishop_masks[square]);
#else
                int magic_index = (int)((occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]));
#endif
                bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
            }
            else
            {
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
#if defined(USE_PEXT)
                int magic_index = (int)_pext_u64(occupancy, rook_masks[square]);
#else
                int magic_index = (int)((occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]));
#endif
                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
            }
        }
    }
}

// The old macro names (DualMagic, 08/10/2026) still select the same branches.
#if defined(TRIUMV_NO_DUALHQ) && !defined(TRIUMV_NO_SLIDERS)
    #define TRIUMV_NO_SLIDERS
#endif
#if defined(TRIUMV_VERIFY_DUALHQ) && !defined(TRIUMV_VERIFY_SLIDERS)
    #define TRIUMV_VERIFY_SLIDERS
#endif
#if defined(USE_AVX2) && !defined(TRIUMV_NO_SLIDERS)
// Sliding attacks from the bit geometry (sliders.h, block 7 of 8.1, 18/10/2026), bishop and rook in one computation:
// four lines in one register with vplzcntq on AVX-512, hyperbola quintessence plus a 512-byte rank table on AVX2,
// instead of the PEXT tables (2.25 MB, one random read per attack). -DTRIUMV_VERIFY_SLIDERS compares every attack
// with the tables (abort at the first difference); -DTRIUMV_NO_SLIDERS goes back to the tables.
#include "sliders.h"
#include <cstdio>
#include <cstdlib>
static U64 pext_bishop_attacks(int square, U64 occupancy);
static U64 pext_rook_attacks(int square, U64 occupancy);
static inline SliderPair slider_pair(int square, U64 occupancy) {
// AVX-512: obstruction difference (-0.98% / -1.20% cycles per node, middlegame / endgame, against the previous
// attacks); AVX2: diagonals in a vector register, rook from general registers (hqs, -0.23% middlegame).
// -DSLIDERS_FORCE_HQ: the three-lane hyperbola quintessence everywhere (measured +0.16 to +0.35% on AVX2).
#if defined(SLIDERS_HAVE_OD)
    const SliderPair p = sliders::pair_od(square, occupancy);
#elif defined(SLIDERS_FORCE_HQ)
    const SliderPair p = sliders::pair_hq(square, occupancy);
#else
    const SliderPair p = sliders::pair_hqs(square, occupancy);
#endif
#ifdef TRIUMV_VERIFY_SLIDERS
    if (pair_bishop(p) != pext_bishop_attacks(square, occupancy) || pair_rook(p) != pext_rook_attacks(square, occupancy)) {
        printf("info string SLIDERS WRONG: square %d occ %llx\n", square, (unsigned long long)occupancy);
        fflush(stdout);
        abort();
    }
#endif
    return p;
}
U64 get_bishop_attacks(int square, U64 occupancy) { return pair_bishop(slider_pair(square, occupancy)); }
U64 get_rook_attacks(int square, U64 occupancy) { return pair_rook(slider_pair(square, occupancy)); }
U64 get_queen_attacks(int square, U64 occupancy) {
    const SliderPair p = slider_pair(square, occupancy);
    return pair_bishop(p) | pair_rook(p);
}
// AA1 (10/10/2026): the bishop/rook pair of one computation, in an xmm register (magic.h).
SliderPair get_slider_pair(int square, U64 occupancy) { return slider_pair(square, occupancy); }
#define TRIUMV_AA1_PAIR_DONE
// The table-based functions remain, under another name, for verification.
#define get_bishop_attacks pext_bishop_attacks
#define get_rook_attacks pext_rook_attacks
#define get_queen_attacks pext_queen_attacks
static U64 pext_queen_attacks(int square, U64 occupancy);
#endif

// get bishop attacks
U64 get_bishop_attacks(int square, U64 occupancy)
{
#if defined(USE_PEXT)
    return bishop_attacks[square][(int)_pext_u64(occupancy, bishop_masks[square])];
#else
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];
    return bishop_attacks[square][occupancy];
#endif
}

// get rook attacks
U64 get_rook_attacks(int square, U64 occupancy)
{
#if defined(USE_PEXT)
    return rook_attacks[square][(int)_pext_u64(occupancy, rook_masks[square])];
#else
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];
    return rook_attacks[square][occupancy];
#endif
}

// get queen attacks
U64 get_queen_attacks(int square, U64 occupancy)
{
#if defined(USE_PEXT)
    return bishop_attacks[square][(int)_pext_u64(occupancy, bishop_masks[square])]
         | rook_attacks[square][(int)_pext_u64(occupancy, rook_masks[square])];
#else
    U64 queen_attacks = 0ULL;
    U64 bishop_occupancy = occupancy;
    U64 rook_occupancy = occupancy;

    bishop_occupancy &= bishop_masks[square];
    bishop_occupancy *= bishop_magic_numbers[square];
    bishop_occupancy >>= 64 - bishop_relevant_bits[square];
    queen_attacks = bishop_attacks[square][bishop_occupancy];

    rook_occupancy &= rook_masks[square];
    rook_occupancy *= rook_magic_numbers[square];
    rook_occupancy >>= 64 - rook_relevant_bits[square];
    queen_attacks |= rook_attacks[square][rook_occupancy];

    return queen_attacks;
#endif
}

#ifndef TRIUMV_AA1_PAIR_DONE
// AA1 without DualMagic (builds without AVX2 or -DTRIUMV_NO_DUALHQ): the two tables, as before.
SliderPair get_slider_pair(int square, U64 occupancy)
{
    const U64 b = get_bishop_attacks(square, occupancy), r = get_rook_attacks(square, occupancy);
#if defined(__x86_64__) || defined(_M_X64)
    return _mm_set_epi64x((long long)r, (long long)b);
#else
    return SliderPair{b, r};
#endif
}
#endif
