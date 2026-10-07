# BAS-D06

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D06 — shallow-depth node cost, step 5.14** (16 suite positions, Hash 64
every arm, fresh process per depth, 2026-08-25;
`analysis/step514_shallow_cost.md`). Cumulative nodes to reach each depth,
Basilisk against SF search driving our own evaluator:

| depth | 1 | 2 | 3 | **4** | 6 | 8 | 11 |
|---|---:|---:|---:|---:|---:|---:|---:|
| ratio | 1.50× | 3.12× | 3.41× | **4.38×** | 3.45× | 3.18× | 1.99× |

**Extended to depth 19 by BAS-D07, which corrects the conclusion below: the excess does NOT keep decaying — it plateaus at ~1.9× and deep branching is equal, not better for us.**

The excess is not startup overhead: it **rises** to a peak at depth 4 and then
decays, which with our better branching ratio (BAS-D05) describes a search that
pays a large penalty in a narrow band and then grows more slowly. Interior and
quiescence carry the **same** ratio at every depth (4.69×/4.05× at depth 4,
1.95×/2.05× at depth 11) and converge together.

*Conditional lesson.* One cause, not two — a multiplier applied above the
subtree shows up identically in interior and quiescence, and it rules out both
"qsearch is expensive" and "interior pruning is weak" as separate diagnoses.
The target band is **depths 2–6**, where our shallow pruning lives (razoring
`<= 3`; futility, LMP and history pruning `<= 6`). Phase 5 has never tested that
band: clusters 5.4 and 5.6 judged candidates on depth reached at 300,000 nodes,
a metric dominated by deep search that a shallow-band saving barely moves.
BAS-D04's "no depth" verdict on history pruning is consistent with this and is
not contradicted — it was measured against the wrong band.

*Caveat.* Absolute node counts are never comparable across engines; the shape of
the ratio is. The engine-agnostic evidence of a real deficit remains the
equal-time 15.6 against 25.2 plies (BAS-O01/O03).

*Disposition.* Diagnostic complete, no candidate proposed. Whether to open a
cluster against the shallow band is part of the pending budget decision.
