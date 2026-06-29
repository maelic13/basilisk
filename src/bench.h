#pragma once

// bench [depth] [repeats] [threads] — deterministic search fingerprint over a
// fixed 40-position suite, plus geomean-EBF / median / top-share diagnostics
// and (for repeats > 1) a best-of-N NPS reading. Kept identical in structure to
// sibling engine Rarog's bench harness.
void run_bench(int depth = 13, int repeats = 1, int threads = 1);
