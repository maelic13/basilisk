# BAS-P13

| Field | Value |
|---|---|
| ID | BAS-P13 |
| Experiment and conditions | **Frozen before the run, PLAN A.8.9's pooled-PGO NPS read: the A.8.7–A.8.9 head against 1.10.1.** Candidate: two final-PGO Release builds of `d1ad178` (MultiPV at default 1, with A.8.7's pool line and A.8.8's info formatting) by `tools/build_test.ps1 -Suffix a89-nps-pgo1/2 -Flavor Release`, each required clean at bench 14,978,465. Baseline: BAS-P12's builds 1–2 of 1.10.1 (`tools/test_engines/basilisk-a74-nps-pgo1/2-pext-pgo.exe`), unchanged. Behind the harness idle-host guard, `nps_ab.ps1` at `bench 13`, 16 alternating rounds × 3 repeats, one pinned physical core at High priority: (1) a self pair over the two candidate builds; (2) candidate (arm A) against baseline (arm B). Logs in `tools/results/a89-nps/`. The engine changes since 1.10.1 add per-iteration root work and one short-circuited test per node at the hash-table store; nothing else in the per-node path moved. |
| Result / disposition | registered |
| Conditional lesson and retry trigger | **Prediction (frozen):** the self pair reads within ±0.30% with its interval covering 0 (0.9); the candidate against 1.10.1 reads **0.0%**, inside ±0.6%, the identical-source PGO spread BAS-P12 measured (0.85). **Decision rule:** a median delta of −0.6% or better accepts A.8.9's NPS condition; below −0.6%, A.8.9 returns to find the per-node cost before anything else lands. A failed self pair voids the run as an instrument fault. |
| Source | PLAN A.8.9 |
