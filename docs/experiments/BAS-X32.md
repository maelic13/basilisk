# BAS-X32

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X32 |
| Rarog evidence | **Harness for the programme.** Colosseum CLI and fastchess agree within their intervals on the same arms (RAR-M60, RAR-M61; Colosseum's own spread over three same-seed runs was as wide as the gap between instruments). A tune of 15 slots × 30 games per iteration runs about **1.65×** the games per hour of 14 × 32, mostly from wave packing (RAR-M62). The drift model **≈ 8.3e-6 × (Elo1 − Elo0) × (true nElo − midpoint) per game** fits three completed `[0,3]` gates within 1% (RAR-M10); it sizes caps prospectively and is extrapolation outside about ±6 nElo. |
| Basilisk implication | Adopt Colosseum as the main path with the shared guards, the 15 × 30 tune shape and the drift model as the sizing prior until Basilisk's own gates calibrate one. Never run the two engines' pinned harnesses at once. |
| PLAN coverage | A.3; PLAN §4 |
