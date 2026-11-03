// sstats.h -- search counters per depth (03/10/2026), ONLY with -DTRIUMV_SSTATS.
// They serve to compare the decisions of our search with those of a reference engine, which receives an identical copy
// of this file. One thread, one position per process: at the end of the process, if the environment
// variable SSTATS_FILE is set, it appends to the file a line "key depth value" for every non-zero
// counter. The depth is that of the node at ENTRY (before IIR, extensions, internal depth--).
// Without TRIUMV_SSTATS the SSTAT/SSTATV macros do nothing: identical generated code (bench unchanged).
// Scripts: match_moe/search_stats_vs_*.py (comparison), match_moe/prune_grid.py (option trials).
#pragma once

#ifdef TRIUMV_SSTATS
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace SStats {
enum Key {
    NODES,      // entries into the main search (depth > 0)
    QNODES,     // entries into quiescence
    LOOP,       // nodes that reach the move loop
    RAZOR, RFP, NMP_TRY, NMP_CUT, PROBCUT,
    LEGAL,      // moves considered in the loop (before the pruning)
    MADE,       // moves actually played (after the pruning of the loop)
    MADE_Q, MADE_C, MADE_CHK, // ... of which quiet (not check), captures/promotions (not check), checks
    LMR_N,      // moves searched with the LMR branch (after the first)
    LMR_SUM,    // sum of the positive reductions applied (plies)
    LMR_NEG,    // negative reductions (extensions from LMR)
    LMR_RES,    // full-depth re-searches after a reduced fail high
    RED_Q, RED_C, RED_CHK,    // moves actually reduced (reduction > 0) per category
    PR_LMP, PR_FUT, PR_SEE, PR_HIST, PR_CAPFUT, // moves pruned in the loop, per rule
    SING_TRY, EXT1, EXT2, EXT3, EXTNEG, MULTICUT,
    FH,         // fail highs in the loop
    FH_FIRST,   // fail highs at the first move played
    FH_MOVESUM, // sum of the order number of the move that fails high
    FH_HASTT,   // fail highs in nodes with a TT move
    FH_TT,      // fail highs made by the TT move
    FH_CAP,     // fail highs made by a capture/promotion
    FH_QUIET,   // fail highs made by a quiet move
    // Triumviratus only: why the quiet futility does not fire (quiet moves from the 2nd move on)
    QCONS, FUT_PV, FUT_INCHECK, FUT_DEEP, FUT_EVALUP, FUT_TRY, FUT_PDSUM, FUT_GAPSUM,
    TT_HIT, TT_MOVE,   // main nodes (ply > 0) with a TT entry found / with a TT move
    FH_FIRST_CAP,      // fail highs made by a capture played as the first move
    NKEYS
};
inline const char* const kName[NKEYS] = {
    "NODES", "QNODES", "LOOP", "RAZOR", "RFP", "NMP_TRY", "NMP_CUT", "PROBCUT", "LEGAL", "MADE", "MADE_Q", "MADE_C",
    "MADE_CHK", "LMR_N", "LMR_SUM", "LMR_NEG", "LMR_RES", "RED_Q", "RED_C", "RED_CHK", "PR_LMP", "PR_FUT", "PR_SEE",
    "PR_HIST", "PR_CAPFUT", "SING_TRY", "EXT1", "EXT2", "EXT3", "EXTNEG", "MULTICUT", "FH", "FH_FIRST",
    "FH_MOVESUM", "FH_HASTT", "FH_TT", "FH_CAP", "FH_QUIET",
    "QCONS", "FUT_PV", "FUT_INCHECK", "FUT_DEEP", "FUT_EVALUP", "FUT_TRY", "FUT_PDSUM", "FUT_GAPSUM",
    "TT_HIT", "TT_MOVE", "FH_FIRST_CAP"};
inline std::uint64_t c[NKEYS][64];
inline void add(int k, int d, std::int64_t v = 1) { c[k][d < 0 ? 0 : d > 63 ? 63 : d] += std::uint64_t(v); }
inline void dump() {
    const char* f = std::getenv("SSTATS_FILE");
    if (!f) return;
    if (std::FILE* o = std::fopen(f, "a")) {
        for (int k = 0; k < NKEYS; k++)
            for (int d = 0; d < 64; d++)
                if (c[k][d]) std::fprintf(o, "%s %d %llu\n", kName[k], d, (unsigned long long) c[k][d]);
        std::fclose(o);
    }
}
inline const int reg = (std::atexit(dump), 0);
}  // namespace SStats
#define SSTAT(k, d) SStats::add(SStats::k, (d))
#define SSTATV(k, d, v) SStats::add(SStats::k, (d), (v))
#else
#define SSTAT(k, d) ((void) 0)
#define SSTATV(k, d, v) ((void) 0)
#endif
