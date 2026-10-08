# BAS-X31

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X31 |
| Rarog evidence | **Tunes in blocks.** RAR-S75 ran 5,000 iterations; its last 1,100 were worth **+4.43 ± 2.90** (RAR-S77). Restarting all 82 coordinates from its endpoint with a fresh gain schedule gained **+13.1 ± 5.4**, carried by coordinates the first run had barely moved (RAR-S78): the limit was the decayed gain, not the games. Rule 7c followed (blocks of 2,000 × 30, a movement stop rule, at most three blocks), amended so the count saves only unattended compute (RAR-S82). Categorical switches were settled by 2,000-game paired runs on one tune build and never tuned: refusing mate-range residuals to the correction histories cost **−15.1** (RAR-S74 b). A converted seed clamp read −7.64 ± 9.72 and was reverted to donor seeds as SPSA coordinates (RAR-S74 g). |
| Basilisk implication | Basilisk's earlier SPSA doctrine (a 5,000-iteration floor, `spsa.ps1`'s "PLAN gate 11") predates this evidence and is superseded by rule 7c on the main path. Keep categorical choices out of the SPSA surface. |
| PLAN coverage | PLAN rule 7c; A.3.2; B.2.2–B.2.3 |
