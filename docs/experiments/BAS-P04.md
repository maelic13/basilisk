# BAS-P04

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P04 |
| Experiment and conditions | Per-node `CheckInfo`, check-hinted `make_move`, and pin-sharing candidates. |
| Result / disposition | **Rejected:** roughly −1.8% to −2.7% for check caching, −0.16% for pin sharing. |
| Conditional lesson and retry trigger | Caching is not free when computation is lazy or consumers are sparse. Retry only after profile evidence shows changed reuse. |
| Source | `CHANGELOG.md` 1.9.1 |
