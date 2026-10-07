# BAS-O01

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search-oracle observations (Basilisk's own) -->

| Field | Value |
|---|---|
| ID | BAS-O01 |
| Experiment | **Search isolated.** Stockfish `9587eeeb` search driving Basilisk's own unmodified 1.9.3 HCE, against native Basilisk 1.9.3. Only the search differs. |
| Result | **302-88-10, 86.5%, ≈ +322.7 ±36 Elo.** The oracle won while running *fewer* nodes per move (170k vs 226k) and lower NPS (2.4M vs 2.9M). |
| Conditional lesson | Basilisk's dominant deficit is search coordination, and it is not a throughput artifact — the oracle was handicapped on speed and still won by a wide margin. Far larger than the ~+196 Rarog measured, partly because our direct-link adapter costs ~14% where their DLL cost ~37%. Sizes the Phase-5 search track. |
