# BAS-X36

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X36 |
| Rarog evidence | **One longer tune near an optimum** (Rarog's live repository at `ec6f5b5`, 2026-10-27, read under A.12's exception; its PLAN rule 7c, amended 2026-10-20). Where a pilot (RAR-S101) and an earlier block read the surface as near its optimum with small gradients, RAR-S104 ran one tune of 82 coordinates, 4,000 iterations × 30 games, with perturbations twice the block step and the gain ratio halved (`r-end` 0.0016 against 0.0031). Its derivation: the per-iteration move at a given result is unchanged, the noise travel grows by √2 while the signal grows four times, so a full-step move needs a gradient of about 0.75 Elo per step against about 3 for a standard block: 2.8 times the resolution for 2.2 times the games. A restarted block re-inflates the early random walk near an optimum, while one run keeps averaging as its gain decays. The run completed with 0 faults and settled in its first half. Its baked theta passed C.10.5's gate RAR-S105 (`[0,3]`, H1), and C.10 closed at **+4.5 ± 2.9 Elo** at `3+0.03`. |
| Basilisk implication | Rule 7c's blocks stay the default for seeds far from their optimum; a registration may choose the single run when a zero-game curvature sweep and an earlier block read the surface as near its optimum, stating the perturbation, the gain ratio and the resolution they buy. The movement unit stays the block's step, and a run cut short is not theta. Rarog's resolution figures are its harness's and are re-derived for any Basilisk registration. |
| PLAN coverage | PLAN rule 7c; A.12 |
