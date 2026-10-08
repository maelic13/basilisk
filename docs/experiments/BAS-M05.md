# BAS-M05

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M05 |
| Experiment and conditions | Resignation threshold replay against historical score streams. |
| Result / disposition | `400/3` one-sided was too aggressive for the engine's score scale; `600/3` one-sided became the shared `strength-v1` profile. |
| Conditional lesson and retry trigger | Adjudication scores are engine-scale dependent. Recalibrate after a material score-scale change such as NNUE integration. |
| Source | legacy plan at `8dc0a24^`; `PLAN.md` §2 |
