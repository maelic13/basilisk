#pragma once
#ifndef SEE_H
#define SEE_H

#include "defs.h"
#include "threads.h"

// Piece values for SEE
// 04/10/2026: values in the units of the search margins (pawn 208), the same as unit_value in
// search/09_history.inc. Before 100/320/330/500/900.
// 21/10/2026 (agent SEE): constexpr, so that the release can check the parameters SeeVal* against it (01_params.inc).
static constexpr int see_piece_values[12] = {
    208, 781, 825, 1276, 2538, 20000,  // P N B R Q K
    208, 781, 825, 1276, 2538, 20000   // p n b r q k
};

// Piece values (28/10/2026): base SeeValP..SeeValQ plus an offset per type and per expert (C2ValP..C2ValQ, search/
// 01_params.inc). The development builds read the copies g_see_val and g_c2_val_off, synced at setoption; the release
// reads the constant tables here (identical values, static_assert in 01_params.inc).
static constexpr int see_val_offsets[5][8] = {   // C2ValP..C2ValQ x experts 0..7 (unbaked 28/10/2026: all 0)
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}};
#ifndef TRIUMV_RELEASE
extern int g_see_val[12];          // SeeValP..SeeValQ for both colours, kings 20000
#endif
// Material value of every piece code in the thread's current position (base + the offset of the owner's expert;
// kings and codes 12..15, i.e. "no piece" & 15, are 0). Refreshed when the expert pair changes (see.cpp).
const int* piece_values_mat(const ThreadData& td);

// Pawn structure in the value of a pawn (28/10/2026, author: "valuta se la struttura pedoni deve dire la sua sul valore
// dei pedoni"). Removing pawns from 64,204 positions of 8.1 games (match_moe/piece_values/piece_values_all.py) the final
// network gives an isolated or a doubled pawn about 20% less than the average pawn, a supported one 14% more, a passed
// pawn on its 5th-6th rank about 20% more and a pawn on the 7th rank twice as much. The value of a pawn taken (the
// victim of the exchange evaluation, of the futility and quiescence margins and of the capture order) is corrected by
// its own status: PawnIsoAdj, PawnDblAdj, PawnSupAdj, PawnPassAdj (passed, 5th-6th rank), PawnR7Adj (7th rank), SEE
// units, all 0 = the old values. Squares a8 = 0 (row 0 = 8th rank).
static constexpr int see_pawn_adj[5] = {};   // the release values (static_assert in 01_params.inc)
#ifdef TRIUMV_RELEASE
static constexpr const int (&g_pawn_adj)[5] = see_pawn_adj;
static constexpr bool g_pawn_adj_on = false;
#else
extern int  g_pawn_adj[5];       // copies of PawnIsoAdj..PawnR7Adj, synced at setoption
extern bool g_pawn_adj_on;       // any of them not 0
#endif
extern U64 pawn_attacks[2][64];
static inline int pawn_struct_adj(const ThreadData& td, int sq, int code) {
    if (!g_pawn_adj_on || (code != P && code != p))
        return 0;
    const bool white_pawn = code == P;
    const int row = sq >> 3, file = sq & 7;
    const U64 own = td.bitboards[code], enemy = td.bitboards[white_pawn ? p : P];
    const U64 file_a = 0x0101010101010101ULL;
    const U64 this_file = file_a << file;
    const U64 side_files = (file > 0 ? file_a << (file - 1) : 0) | (file < 7 ? file_a << (file + 1) : 0);
    const U64 front = white_pawn ? ((1ULL << (row * 8)) - 1) : (row < 7 ? ~((1ULL << ((row + 1) * 8)) - 1) : 0);
    const int rel_rank = white_pawn ? 8 - row : row + 1;
    const bool isolated = !(own & side_files);
    const bool doubled  = (own & this_file & ~(1ULL << sq)) != 0;
    const bool supported = (pawn_attacks[white_pawn ? 1 : 0][sq] & own) != 0;   // own pawns that defend sq
    const bool passed = !(enemy & front & (this_file | side_files));
    return isolated * g_pawn_adj[0] + doubled * g_pawn_adj[1] + supported * g_pawn_adj[2] +
           (passed && (rel_rank == 5 || rel_rank == 6)) * g_pawn_adj[3] + (rel_rank == 7) * g_pawn_adj[4];
}

// Static Exchange Evaluation for thread-local state
int td_see(ThreadData& td, int move);

// F-008: THRESHOLD SEE with early exit (SF see_ge style). Returns 1 if
// td_see(move) >= threshold, 0 otherwise - without simulating the full exchange.
int td_see_ge(ThreadData& td, int move, int threshold);

#endif
