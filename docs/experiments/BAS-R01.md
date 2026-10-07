# BAS-R01

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 4. Root search, time management and SMP -->

| Field | Value |
|---|---|
| ID | BAS-R01 |
| Experiment and conditions | Start the move clock at receipt of `go`, including GUI-to-worker dispatch latency. |
| Result / disposition | **Retained non-regression, +2.95 ± 6.74 Elo** versus 1.7.0. |
| Conditional lesson and retry trigger | At bullet TC with concurrent engines, dispatch latency was material to safety. Recheck on materially different UCI scheduling architectures. |
| Source | `CHANGELOG.md` 1.8.0 |
