// cstats.h -- precision of the evaluation corrections (06/10/2026, report docs/audit_8.0/N_CORREZIONI.md 4.1),
// ONLY with -DTRIUMV_CORRSTATS. At the point where the search learns the corrections (end of node, 13_search.inc) the
// residuals are accumulated, per network expert x fifty-move band x bound type,
//   r0 = best - network eval (without correction)   and   r1 = best - corrected eval:
// count, sum, sum of squares. From here: how much of the error variance the corrections explain (1 - q1/q0) and the
// mean residual per expert and per band (if it is ~0 in every band, a per-expert table has nothing to learn).
// The absolute number is distorted by the bounds (censored residual); the comparison between variants on the same set is not.
// At the end of the process, if the environment variable CSTATS_FILE is set, it appends to the file one line per non-empty cell:
// "expert band bound n s0 q0 s1 q1". Script: match_moe/corrstats.py.
// Without TRIUMV_CORRSTATS the CSTAT macro does nothing: identical generated code (bench unchanged).
#pragma once

#ifdef TRIUMV_CORRSTATS
#include <cstdio>
#include <cstdlib>

namespace CStats {
inline double n[4][3][3], s0[4][3][3], q0[4][3][3], s1[4][3][3], q1[4][3][3];   // [esperto][fascia 50][limite]
inline void add(int ph, int fb, int kind, int r0, int r1) {
    n[ph][fb][kind] += 1;
    s0[ph][fb][kind] += r0;
    q0[ph][fb][kind] += double(r0) * r0;
    s1[ph][fb][kind] += r1;
    q1[ph][fb][kind] += double(r1) * r1;
}
inline void dump() {
    const char* f = std::getenv("CSTATS_FILE");
    if (!f) return;
    if (std::FILE* o = std::fopen(f, "a")) {
        for (int p = 0; p < 4; p++)
            for (int b = 0; b < 3; b++)
                for (int k = 0; k < 3; k++)
                    if (n[p][b][k])
                        std::fprintf(o, "%d %d %d %.0f %.0f %.0f %.0f %.0f\n", p, b, k, n[p][b][k], s0[p][b][k],
                                     q0[p][b][k], s1[p][b][k], q1[p][b][k]);
        std::fclose(o);
    }
}
inline const int reg = (std::atexit(dump), 0);
}  // namespace CStats
#define CSTAT(ph, fb, kind, r0, r1) CStats::add((ph), (fb), (kind), (r0), (r1))
#else
#define CSTAT(ph, fb, kind, r0, r1) ((void) 0)
#endif
