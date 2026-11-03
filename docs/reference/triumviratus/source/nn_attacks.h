// nn_attacks.h -- The engine's attacks as seen by the network (07/10/2026, single board v2).
//
// The network enumerates the active threats in the refresh (FullThreats::append_active_indices) on our board, in
// our numbering (a8 = 0): it therefore uses OUR attack tables (attacks.cpp, magic.cpp), not those of the
// network. Here only the declarations, identical to those of attacks.h and magic.h, without including defs.h (its macros
// must not enter the network's units).

#ifndef NN_ATTACKS_H_INCLUDED
#define NN_ATTACKS_H_INCLUDED

extern unsigned long long knight_attacks[64];
extern unsigned long long get_bishop_attacks(int square, unsigned long long occupancy);
extern unsigned long long get_rook_attacks(int square, unsigned long long occupancy);
extern unsigned long long get_queen_attacks(int square, unsigned long long occupancy);   // AA1 (10/10/2026)

#endif  // NN_ATTACKS_H_INCLUDED
