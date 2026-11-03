// nstats.h -- counters of the network's work (08/10/2026), ONLY with -DTRIUMV_NSTATS.
// They serve to compare how much network work every engine does PER NODE: evaluations, incremental updates of the
// accumulators, refreshes from the finny table, weight rows read. An identical copy of this file goes into the source of
// the reference engine used for the comparison, with the macros at the same points of its accumulator code.
// At the end of the process, if the environment variable NSTATS_FILE is set, it appends to the file a line "key value" per
// counter. The nodes are taken from the workload (uci_workload.py), which reads them from the engine.
// Without TRIUMV_NSTATS the macros do nothing: identical generated code.
#pragma once

#ifdef TRIUMV_NSTATS
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace NStats {
enum Key {
    EVAL,        // AccumulatorStack::evaluate: network evaluations going through the accumulators
    INC,         // update_accumulator_incremental: one incremental step, one side
    INC_BOTH,    // update_accumulator_incremental_both: one incremental step, both sides
    REFRESH,     // update_accumulator_refresh_cache: one refresh from the finny table, one side
    HYBRID,      // update_accumulator_hybrid: one hybrid update, one side
    COMBINED,    // apply_combined: calls (one side)
    PSQ_ROWS,    // positional rows (added + removed) read by apply_combined
    THR_ROWS,    // threat rows (added + removed) read by apply_combined
    // X2 (08/10/2026): HalfKA rows read by the rebuilds from the finny table. Only in our engine: keys at the
    // end, the copy in the comparison source does not have them and does not need them.
    HYB_OLD_ROWS,  // hybrid, old entry (previous position)
    HYB_NEW_ROWS,  // hybrid, new entry
    REF_ROWS,      // refresh, entry
    // (Counters of the graft blocks GRAFT_*, PREL_*, REF_PST_*: removed on 10/10/2026 with the blocks.)
    REF_PAWN_HIT,   // refresh with a hit of the "pe" cache (PawnPair, PassedPawns for free)
    REF_PAWN_ROWS,  // refresh with a miss: rows of the pawn blocks added (the four families)
    NKEYS
};
inline const char* const kName[NKEYS] = {"EVAL", "INC", "INC_BOTH", "REFRESH", "HYBRID", "COMBINED", "PSQ_ROWS",
                                         "THR_ROWS", "HYB_OLD_ROWS", "HYB_NEW_ROWS", "REF_ROWS",
                                         "REF_PAWN_HIT", "REF_PAWN_ROWS"};
inline std::uint64_t c[NKEYS];
inline void dump() {
    const char* f = std::getenv("NSTATS_FILE");
    if (!f) return;
    if (std::FILE* o = std::fopen(f, "a")) {
        for (int k = 0; k < NKEYS; k++)
            std::fprintf(o, "%s %llu\n", kName[k], (unsigned long long) c[k]);
        std::fprintf(o, "END\n");
        std::fclose(o);
    }
}
inline const int reg = (std::atexit(dump), 0);
}  // namespace NStats
#define NSTAT(k) (++NStats::c[NStats::k])
#define NSTATV(k, v) (NStats::c[NStats::k] += std::uint64_t(v))
#else
#define NSTAT(k) ((void) 0)
#define NSTATV(k, v) ((void) 0)
#endif
