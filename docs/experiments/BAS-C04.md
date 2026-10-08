# BAS-C04

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C04 |
| Experiment or failure mode | Helper/private clocks and fixed-depth inheritance did not match deployed multi-thread semantics. |
| Disposition | Repaired in 1.9.2 with aggregate accounting and zero-forfeit tests. |
| Conditional lesson / coverage | UCI timing and SMP cannot be validated only at 1T. Repeat the full matrix after root, stop or pool changes. |
| Source | `CHANGELOG.md` 1.9.2 |
