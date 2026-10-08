# BAS-O03

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search-oracle observations (Basilisk's own) -->

| Field | Value |
|---|---|
| ID | BAS-O03 |
| Experiment | **Mechanism.** Average completed depth and effective branching factor at equal time, from the same tournament. |
| Result | Control 29.7 plies / EBF 1.51; oracle 25.2 / 1.61; **Basilisk 15.6 / 2.20**; Rarog 15.5 / 2.21. Basilisk searched *more* nodes per move than the oracle and finished **9.6 plies shallower**. |
| Conditional lesson | The gap is tree shape, not speed: our effective branching factor is ~2.20 against ~1.61. This is the single most actionable number the experiment produced and it points squarely at ordering, reductions and selectivity — i.e. cluster 5.4 first. It also matches durable lesson 5 in reverse: our tree is not too small, it is too wide. |
