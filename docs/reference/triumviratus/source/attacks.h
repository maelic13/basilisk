// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
#pragma once
#ifndef ATTACKS_H
#define ATTACKS_H

#include "defs.h"

extern const U64 not_a_file;
extern const U64 not_h_file;
extern const U64 not_hg_file;
extern const U64 not_ab_file;

// bishop relevant occupancy bit count for every square on board
extern const int bishop_relevant_bits[64];

// rook relevant occupancy bit count for every square on board
extern const int rook_relevant_bits[64];

// rook magic numbers
extern U64 rook_magic_numbers[64];

// bishop magic numbers
extern U64 bishop_magic_numbers[64];

extern U64 pawn_attacks[2][64];
extern U64 knight_attacks[64];
extern U64 king_attacks[64];
extern U64 bishop_masks[64];
extern U64 rook_masks[64];
// (PACKED SLIDER TABLES: tried and REMOVED on 10/09/2026.
//  The form below uses a FIXED stride per square, 2.25 MB in all, where
//  a layout that packs every square to its exact size fits in 841 KB.
//  Really compacted: rook_attacks_store had dropped to 800 KB and bishop to 41 KB,
//  exactly their sizes, with the bench signature unchanged.
//  RESULT: NEUTRAL, 5,647.5 +- 11.6 cycles/node against 5,624.0 +- 23.1, and the L2
//  misses identical to the hundredth (86.92 against 86.98 per node).
//  🔑 The reasoning error: the cache holds only the lines TOUCHED, and the
//  unused padding occupied none of them. The set really read was
//  already of the same size, so there was no pressure to remove. In exchange
//  the vector of pointers added a dependent load to every query.
//  A big table does not cost as much as it is declared, but as much as is read from it.)
extern U64 bishop_attacks[64][512];
extern U64 rook_attacks[64][4096];

extern U64 mask_pawn_attacks(int side, int square);
extern U64 mask_knight_attacks(int square);
extern U64 mask_king_attacks(int square);
extern U64 mask_bishop_attacks(int square);
extern U64 mask_rook_attacks(int square);
extern U64 bishop_attacks_on_the_fly(int square, U64 block);
extern U64 rook_attacks_on_the_fly(int square, U64 block);
extern void init_leapers_attacks();
extern U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask);

#endif
