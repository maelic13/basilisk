# BAS-M02

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M02 |
| Experiment and conditions | Identical-binary null testing after harness changes. |
| Result / disposition | The old symmetric `[-3,+3]` setup had zero expected LLR drift at equality; current policy is fixed-N 30k at 1T and 10k at 4T, requiring the full 95% nElo CI inside ±5. |
| Conditional lesson and retry trigger | Equivalence needs a calibration design, not an ordinary gain SPRT. Repeat after runner, scheduler, topology or adjudication changes. |
| Source | `PLAN.md` §2 |
