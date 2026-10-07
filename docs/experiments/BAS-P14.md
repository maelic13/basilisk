# BAS-P14

| Field | Value |
|---|---|
| ID | BAS-P14 |
| Experiment and conditions | **Frozen before the run, PLAN A.8.12's pooled-PGO NPS read: the C++23 idiom pass against the head before it.** Candidate: two final-PGO Release builds of the A.8.12 commit by `tools/build_test.ps1 -Suffix a812-nps-pgo1/2 -Flavor Release`, each clean at bench 14,978,465. Baseline: BAS-P13's candidate builds (`basilisk-a89-nps-pgo1/2`, source `d1ad178`); A.8.10 and A.8.11 between them touch only argument handling and the tablebase extension's start rule, outside the bench's path. Behind the idle-host guard, `nps_ab.ps1` at `bench 13`, 16 alternating rounds × 3 repeats, one pinned physical core: (1) a self pair over the candidate builds; (2) candidate (arm A) against baseline (arm B). Logs in `tools/results/a812-nps/`. The pass changes no per-node code: `std::format` replaces output-side string building, `[[nodiscard]]` and `std::unreachable` are compile-time. |
| Result / disposition | registered |
| Conditional lesson and retry trigger | **Prediction (frozen):** the self pair reads within ±0.30% with its interval covering 0 (0.9); the candidate against the baseline reads **0.0%**, inside ±0.6% (0.85). **Decision rule:** −0.6% or better accepts A.8.12's NPS condition; below it, the pass returns to find the cost. A failed self pair voids the run. |
| Source | PLAN A.8.12 |
