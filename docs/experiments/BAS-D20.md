# BAS-D20

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search programme investigation (B.0) -->

**BAS-D20 — the deficit is a constant ×4 node multiplier, not per-ply
growth** (observation). Branching 4→14 on the 103 positions common to all
arms: Basilisk **1.767** (per-position median 1.697), oracle **1.757**
(1.773), Stockfish 19 1.549. Nodes Basilisk/oracle **3.50× at depth 4, 3.98×
at 8, 3.70× at 14**; against Stockfish 19 2.67× rising to 9.95×. At 300,000
nodes the median completed depth is 14 against the oracle's 18 (gap 4, as
BAS-D07; mean 5.5 without mate runaways), best-move agreement 66/105. WAC at
100,000 nodes **204 / 241 / 261** (Basilisk / oracle / Stockfish 19; median
completed depth 10 / 16 / 17; 50 oracle-only, 13 Basilisk-only), at 400,000
**242 / 272 / 278**, at fixed depth 10 Basilisk solves 224 first against the
oracle's 212. Canaries 77/77, 0 regressions. Scale (10,039 positions):
mean |static| Basilisk **167.9 cp**, classical oracle HCE 301.3 internal
units, Stockfish 19 search-facing 595.8: ratios **0.282** (per-position
median 0.293) and **0.557** (unit conversion 0.485). Residual at 50,000 nodes
(1,997 positions): Basilisk mean |search − static| **92.3 cp** (median 55),
the oracle's search on the same evaluation 124.9 (median 71), against a mean
|static| of 164.6. Depth-14 counters on suite_v2: quiescence share 35.5%,
check extensions 15.6% of interior nodes, RFP cuts 14.2%, null conversion
49.6%, history pruning 590 of 11.3M tested, LMR applied 36.4% with mean 2.47
plies and re-search 1.67%, 56.5% of applied reductions at the ceiling,
`tt_pv` nodes 0.57%, first-move cutoff 89.1%. *Lesson.* BAS-D05–D08's 1.9×
was a 16-position reading; the whole-suite multiplier is about 4 from depth 4
on, with equal growth. Stockfish-sized margins cannot be converted by one
scalar here: the HCE's error is over half its own magnitude. Disposition:
observation; the numbers are the B.2.2 baselines (packet §10).
