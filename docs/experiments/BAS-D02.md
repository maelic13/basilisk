# BAS-D02

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

| Field | Value |
|---|---|
| ID | BAS-D02 |
| Measurement | **Where the width is.** LMR gate accounting; the identity `eligible = applied + clamped_zero + Σ blocked` holds exactly on every run. |
| Result | Of eligible moves only **36.1%** are reduced; **16.2%** pass every gate and then compute a reduction of **zero**; mean reduction when applied is 2.354 plies; and the **re-search rate is 1.744%**. |
| Conditional lesson | Reductions are far too conservative. A re-search rate near 1.7% means reductions almost never need undoing, which is the signature of under-reduction rather than of a well-tuned policy — a healthy LMR pays for its depth with visibly more re-searches. Combined with a sixth of eligible moves being reduced by zero, this is where the tree width is created. Points directly at the reduction/re-search contract, cluster **5.4.3**. |
