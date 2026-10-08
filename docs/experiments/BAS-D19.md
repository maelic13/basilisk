# BAS-D19

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D19 — A.2.3 parameter liveness probe: observation** (2026-09-28;
registered before exposure in `analysis/parameter_inventory_v1.md`, `3702de1`).
Every `search_params.h` coordinate alone at each range endpoint, 107
positions of `suite_v1.epd` at depth 14, TUNE binary reproducing
14,978,465; an arm is live when any `kv` counter moves. The baseline pair is
identical; 38 coordinates move the tree. Null by construction:
`QuietSeeDepth`/`Coeff`, `QsearchCheckCap`, `PostLmrHistScale` (gates at 0).
**Capture futility is dead at the default LMR table**: `lmr_depth` never falls
below 1, so `CapFutDepth` 1 never fires (the registered prediction that it
would fire was wrong). At `LmrBase=150` it fires (46 counters; 41.8M against
35.7M nodes with it off), so
an `LmrBase` tune reactivates a mechanism reverted at -2.78. `DoubleExtMax`
16 is a live bound that does not bind on the suite (null at 200), and
`HistPruneCoeff` is live, not near-dead (+5.2% nodes at 28000). Lesson: equal
completed depth (`sweep.py`, BAS-D11/D15) cannot show behavioural identity;
compare every counter. Retry: after any LMR-table change, re-read the
capture-futility gate. Disposition: observation.
