# BAS-C01

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C01 |
| Experiment or failure mode | Brittle fixed-depth endgame conversion canaries rejected benign eval/search/TT changes. |
| Disposition | Replaced with eval recognition, tolerant conversion floors and near-mate checks. |
| Conditional lesson / coverage | A correctness test should assert the invariant, not one search trajectory. Keep tactical strength diagnostics separate. |
| Source | legacy plan at `8dc0a24^` |
