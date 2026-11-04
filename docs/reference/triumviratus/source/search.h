#pragma once
#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"
#include "movegen.h"

// FIX Linux/GCC (2026-07-12): <cmath> MUST be included BEFORE the macro
// `infinity` below. On libstdc++ (Debian/Ubuntu g++), <cmath> includes in
// cascade the TR1 templates (bessel/elliptic/zeta) that use literally
// `std::numeric_limits<_Tp>::infinity()` - the preprocessor blindly replaces
// ANY occurrence of the token "infinity", even inside the
// system headers, corrupting those calls if <cmath> is
// (re-)included AFTER our macro. Pre-including it here exploits the
// include guards of <cmath>: the later inclusions (e.g. threads.cpp)
// become no-ops and never see the raw token again. MSVC does not have
// this problem (no libstdc++ TR1 headers), so the Windows build
// does not show it - but the fix is harmless and free there too.
#include <cmath>

// Score bounds for mating scores.
// NOTE: these MUST stay within the signed 16-bit range, because the
// transposition table packs the score into 16 bits (see tt.h). With the old
// values (49000/50000) every mate score overflowed int16 and was corrupted
// when stored/read from the TT (mate values must stay well under 32767). The gap mate_value - mate_score (1000) exceeds max_ply, so
// real (non-mate) evaluations never reach the mate band.
#define infinity 32000
#define mate_value 31000
#define mate_score 30000

// max ply that we can reach within a search.
// 🔴 2026-09-07: was 64. Beyond this ply td_negamax (:7569) and td_quiescence
// (:6489) do NOT search any more: they return the static eval, silently. At 10s and
// 1 thread the seldepth stays below 30, but the SMP measurements of 07/09 (10 s, 16-40
// threads) reached **seldepth 42-54**, and at long TC with many threads the
// cap is really reached - precisely in the forcing lines, where stopping costs
// most.
// Cost: the ply-indexed arrays of ThreadData are [max_ply + 8]; the only
// quadratic one is pv_table, 20 KB -> 72 KB. On a ThreadData of 21.25 MB that is
// 0.24%. The derived constants (TB_VALUE_WIN in syzygy.cpp, the corr_max bands in
// threads.cpp) are written as functions of max_ply and stay correct by themselves:
// the minimum TB score is TB_VALUE_WIN - max_ply = mate_score - 2*max_ply, which is
// exactly the edge CorrTBGuard uses.
// Unchanged constraint: mate_value - mate_score = 1000 must exceed max_ply.
#define max_ply 128

// MVV LVA [attacker][victim]
extern int mvv_lva[12][12];

// killer moves [id][ply]
extern int killer_moves[2][max_ply];

// history moves [piece][square]
extern int history_moves[12][64];

// PV length [ply]
extern int pv_length[max_ply];

// PV table [ply][ply]
extern int pv_table[max_ply][max_ply];

// follow PV & score PV move
extern int follow_pv, score_pv;

// Single-threaded search
extern void search_position(int depth);

#endif
