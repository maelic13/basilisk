# BAS-P33

**BAS-P33 — the counter read's null redesigned: copies of every base build, decided over builds**

- **Registration.** 2026-11-14, B.7.9, after BAS-P28 failed its null rule and before
  this read (maintainer: "fix the instrument now"). Agent; maintainer-run, elevated.
  BAS-P28's verdict stands as NOT ACCEPTED; this is a new read of a new design, not a
  re-scoring of that one.
- **The defect.** BAS-P28's null compared three runs of one copy with three of its
  source, by Welch over runs (two degrees of freedom each). Runs scatter about 0.3%,
  with an occasional run 2–3% off (BAS-P28's `p25-cand` build 3, cycle 2; BAS-P32's
  `b76-base` build 3, cycle 3). The contrasts take each build's median of its three
  runs and decide over builds, which absorbs such a run. The null did not, so it was
  wider (0.76%) than the instrument the contrasts use.
- **The design.** `tools/run_b164_pmc.ps1 -NullCopies 4` (the new default): a
  byte-identical copy of each of the base pool's four builds, run beside it in every
  cycle; the null is `copy` against the base pool, each build a median of its runs,
  Welch over builds: the contrast's own estimator. `tools/diag/pmc_read.py` already
  decides such a null over builds; `test_null_over_copied_builds` covers it.
- **Arms.** BAS-P28's pools (`b164-p25-base`, `b164-p25-cand`) and their copies,
  three cycles, CPU 2. Then the same design on BAS-P30's pools (`b76-base`,
  `b76-head`), which counts only if this read accepts the instrument.
- **Acceptance rule, BAS-P28's, unchanged:** (a) the null's cycles-per-node interval
  holds 0 with a half-width of at most 0.5%, and (b) `p25`'s interval lies below 0 and
  overlaps BAS-P25's [−2.63%, −1.97%].
- **Prediction, frozen.** The null's half-width 0.2%–0.4%, holding 0 (probability
  0.8); `p25` about −2.2%, inside [−2.8%, −1.7%] (0.8); the B.7.6 read about −2.6%
  cycles per node, overlapping BAS-P30's −3.1% in time (0.7).
- **Commands** (elevated, idle host, about eight minutes in all):
  `pwsh -NoProfile -File tools/run_b164_pmc.ps1 -Name b33 -Label "BAS-P33 counters"`, then
  `pwsh -NoProfile -File tools/run_b164_pmc.ps1 -BasePool tools/test_engines/b76-base.pool.json
  -CandPool tools/test_engines/b76-head.pool.json -ExpectNodes 4247965 -Name b79b -Contrast
  b76 -Label "B.7.6 set counters"`.
- **Status.** Registered; dry runs accepted.
