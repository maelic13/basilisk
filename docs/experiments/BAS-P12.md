# BAS-P12

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P12 |
| Experiment and conditions | **Frozen before the run, A.7.4 pooled-PGO NPS baseline of 1.10.1.** `tools/run_a74_nps_baseline.ps1` (committed at `f334a09`): four final-PGO Release builds of revision `3e5294be` by `build_test.ps1` from a clean worktree, each required to reproduce bench 14,978,465; then, behind the harness idle-host guard, `nps_ab.ps1` at `bench 13`, 16 alternating rounds × 3 repeats, one pinned physical core at High priority: (1) a self pair over all four builds; (2) builds 1–2 against builds 3–4. Transcript and both logs in `tools/results/a74-nps/`. Wiring proven 2026-10-05: a smoke run built at the fingerprint and its token self pair (1 round, depth 6) failed as it should, stopping the launcher with exit 1; a real 6-round self pair on that build read +0.16% [−0.14, +0.47], `SELF-PAIR OK`, 4.10M NPS. Smoke artifacts deleted. |
| Result / disposition | **COMPLETE (2026-10-05, 23:16–23:30): baseline 4.127M NPS** (pooled median of 64 round values over the four builds; range 4.113M–4.147M). Builds SHA-256 `D3ABD5FB…`, `4F085E49…`, `46A9E9E9…`, `E68B9561…`, all clean `3e5294be` at bench 14,978,465; idle-host guard passed before building and before measuring. Self pair **+0.11%** [−0.14, +0.25], `SELF-PAIR OK`, A faster in 10/16 rounds; per-build medians 4.121M–4.129M. Builds 1–2 against 3–4 **+0.04%** [−0.32, +0.30], A faster in 7/16. Fingerprints identical throughout, so every delta is pure speed. Calibration: all three predictions held (self pair inside ±0.30%; pools inside ±0.6%; baseline inside 3.9–4.3M, +0.6% above the central 4.10M). |
| Conditional lesson and retry trigger | **Prediction (frozen):** the self pair reads within ±0.30% with its interval covering 0 (0.9); builds 1–2 against 3–4 within ±0.6%, the identical-source PGO spread (0.85); the pooled median is **4.10M NPS**, range 3.9–4.3M (0.8). Reading: the pooled median over all four builds is the 1.10.1 baseline B.7 and B.9 compare against; a failed self pair voids the run as an instrument fault, not a result. |
| Source | PLAN A.7.4 |
