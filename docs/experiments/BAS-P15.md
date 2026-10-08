# BAS-P15

| Field | Value |
|---|---|
| ID | BAS-P15 |
| Experiment and conditions | **Frozen before the run, PLAN A.8.20's pooled-PGO NPS baseline of the A.8 head (maintainer-run).** `tools/run_a820_nps_baseline.ps1`: four final-PGO Release builds of `9c93d157` (A.8.17's engine source, unchanged on `dev` since) by `build_test.ps1` from a clean worktree, each required to reproduce bench 14,978,465. Then, behind the harness idle-host guard, `nps_ab.ps1` at `bench 13`, 16 alternating rounds × 3 repeats, one pinned physical core at High priority: (1) a self pair over all four builds; (2) builds 1–2 against builds 3–4. BAS-P12's recipe exactly. Results in `tools/results/a820-nps/`. It replaces BAS-P12 as B.1's NPS reference. |
| Result / disposition | registered |
| Conditional lesson and retry trigger | **Prediction (frozen):** the self pair reads within ±0.30% with its interval covering 0 (0.9); the build pools within ±0.6% (0.85); the pooled median is **4.17M NPS**, range 4.10–4.25M (0.75). That is BAS-P12's 4.127M plus about the +1.1% BAS-P13 read after A.8.9; A.8.14 and A.8.17 add no per-node work. **Reading:** the pooled median is the A.8 head's baseline; a failed self pair voids the run as an instrument fault. |
| Source | PLAN A.8.20; `analysis/search_programme_2026-10-06.md` §12.4 |
