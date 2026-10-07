# BAS-P08

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P08 |
| Experiment and conditions | Windows ARM64 Clang PGO selected `llvm-profdata` from global `PATH`. |
| Result / disposition | **Correctness/tooling repair in 1.9.3; search unchanged, bench 11,941,440.** |
| Conditional lesson and retry trigger | Tool identity is part of reproducibility. Validate compiler/profdata compatibility for every asset rather than treating a successful compile as a PGO proof. |
| Source | `CHANGELOG.md` 1.9.3 |
