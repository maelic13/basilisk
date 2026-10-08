# BAS-O04

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search-oracle observations (Basilisk's own) -->

| Field | Value |
|---|---|
| ID | BAS-O04 |
| Experiment | **Width attribution.** Mean completed depth over `suite_v1.epd` at a fixed 300,000 nodes, three arms, holding one side constant at a time. Run 2026-08-13 during the cluster-5.4 re-audit. |
| Result | Basilisk native **20.80** (EBF 1.834); SF search + **our** eval **32.87** (1.468); SF search + SF eval **33.38** (1.459). Attribution: **search +12.07 ply = 95.9%**, evaluation +0.51 ply = 4.1%. **CORRECTED 2026-08-13** — those arms ran at unequal hash (Basilisk defaults to 64, the oracle to 16) and mixed two estimators. Re-measured with every arm at Hash 64 and one estimator: Basilisk 21.47, oracle 32.88, control 33.07 — **search 98.4%, evaluation 1.6%**. Conclusion unchanged and slightly strengthened. The EBF figures in this row are superseded by BAS-D05. The evaluation arm's paired split was 36 better / 42 worse. |
| Conditional lesson | Tree width is a **search-policy** property, not a symptom of evaluation quality — an evaluator +232.8 Elo stronger (BAS-O02) buys half a ply and is not even consistently deeper. This **refutes** the working hypothesis recorded when cluster 5.4 closed, which is withdrawn. Combined with BAS-S16 it says the reference is narrow because its decisions are better informed at the point of pruning, not because its margins are more aggressive: pruning the same decisions harder is blindness, and games price it as such. |
