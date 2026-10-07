# BAS-P07

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P07 |
| Experiment and conditions | `origin/arm_fix` aligned the TT allocation to 64-byte cache lines. |
| Result / disposition | **Rejected hypothesis:** no AArch64 evidence; a 32-byte-aligned 32-byte cluster cannot straddle a 128-byte boundary, and Rarog found no material Apple 4T false-sharing case. |
| Conditional lesson and retry trigger | Close the wrapper in Plan 5.3. Retain target-native cache/atomic measurement, ISA contracts and emitted-prefetch verification rather than benchmarking the invalid geometry. |
| Source | `PLAN.md` 5.3; branch `origin/arm_fix`, later tag `archive/arm_fix` (`67a987b`), removed 2026-09-28: this row is the record |
