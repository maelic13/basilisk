# BAS-E04

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 5. Evaluation and data experiments -->

| Field | Value |
|---|---|
| ID | BAS-E04 |
| Experiment and conditions | Successive on-policy self-play refresh cycles with phase balancing and joint linear/king-safety refit. |
| Result / disposition | **Accepted:** +21.02, +19.51, +18.29 and +15.32 Elo; the next cycle washed at +1.37 ± 5.21 and was discarded. |
| Conditional lesson and retry trigger | On-policy refresh paid repeatedly until this line saturated. Retry only after a material policy, representation or evaluator change—not by extending the same HCE loop. |
| Source | `CHANGELOG.md` 1.8.0; legacy plan at `8dc0a24^` |
