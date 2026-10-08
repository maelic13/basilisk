# BAS-M08

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M08 |
| Experiment and conditions | A.7.1 local replay of the Super Rating Tournament PGN: 42-engine Colosseum round robin, 172,200 games, 200 per pair, `3+0.03`, 1T, UHO, no adjudication. PGN 175,960,452 bytes, SHA-256 `4e87a36a030dfc696c9328f9f34f60b94784a5303af4c8be92d2db6b2c05103c`. Header census used `tools/diag/pgn_census.py`; conversion used A.5.5's `conversion_audit.py` with the six BAS-X34 anchors, 12 persistent plies, 3 material units and lone-minor exclusion. |
| Result / disposition | Basilisk 1.10.0: Houdini 3 **15-26-159 (14.00%, -315.35 logistic Elo)**; Critter 1.6a **25-35-140 (21.25%, -227.56)**; Fritz 16 **25-45-130 (23.75%, -202.63)**; Rybka 4.1 **44-38-118 (31.50%, -134.95)**. Each record has 100 games per colour and zero unfinished. Against Critter, Fritz, HIARCS 14, Houdini 1.5a, Rybka 4 and Shredder 12, **20 draws and 8 losses after a persistent piece-up advantage in 1,200 games: 16.67 / 6.67 per 1,000**, 28 total thrown (23.33/1,000). |
| Conditional lesson and retry trigger | This replaces the unsourced 2026-09-04 HISTORY figures with direct records. The head-to-head Elo values are ordinary logistic summaries of 200 games, not paired nElo or acceptance gates. The conversion mix moved from the imported 1.9.3 result (17.5/5.0) but total throws did not materially improve; conversion remains diagnostic. Repeat from the next checkpoint tournament rather than carrying these counts across a changed engine. |
| Source | `tools/results/a71/pool-census.json`; `tools/results/a71/conversion-1.10.0-six.json`; PLAN A.7.1 |
