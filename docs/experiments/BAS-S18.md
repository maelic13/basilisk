# BAS-S18

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search programme investigation (B.0) -->

| Field | Value |
|---|---|
| ID | BAS-S18 |
| Experiment and conditions | **Registered, not yet run (B.0.2, maintainer).** The oracle's move-loop pruning family in Elo: `oracle-1.10.1-ablate.exe` (SHA-256 `0E5155CC…8644`) with `AblationMask=32` and `Use Basilisk HCE=true` against Basilisk 1.10.1 (the BAS-O05 arm), the BAS-O05 recipe at 1,000 cycles (2,000 games), equal time `3+0.03`, Hash 64 both, no adjudication; the oracle fixed at 1500, G(32) = 1500 − Basilisk's rating. **Prediction (frozen 2026-10-06):** G(32) = +110, 80% interval [+40, +180], so the family explains about 200 of BAS-O05's 312.6; confidence moderate. |
| Result / disposition | **Prepared 2026-10-08, not yet run.** `tools/run_b02_oracle_g32.ps1`: BAS-O05's launcher with `oracle-1.10.1-ablate.exe` (SHA-256 pinned), `Use Basilisk HCE=true` and `AblationMask=32` on the oracle, 1,000 cycles (2,000 games), seed 20102, concurrency 14, results in `tools/results/b02-oracle-g32/`. Dry run accepted (`tools/results/colosseum_gauntlet_b02-oracle-g32_20261008_195201.*`, dry-run JSON SHA-256 `A2E8CBEC…42FE`). The mask is live: startpos `go depth 14` at Hash 64 searches 445,620 nodes at mask 32 against 131,089 at mask 0. **Reading rule:** G(0) − G(32) is the Elo the oracle's move-loop pruning explains against Basilisk. G(32) ≥ +250 says the family explains under 60 Elo and BAS-D21's node attribution does not carry Elo, which lowers the B.2 prediction P2 below +20 and re-opens the cluster order before B.2.1; G(32) ≤ 0 contradicts the node ratio and points at the instrument first. |
| Conditional lesson and retry trigger | PROCESS's matched-ablation instrument, used once for the family B.0 names; NPS cancels in the difference, not in the absolute figure. Retry: none. |
| Source | PLAN B.0.2; packet §11 |
