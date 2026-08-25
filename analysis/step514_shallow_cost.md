# Step 5.14 — shallow-depth node cost

**Date:** 2026-08-25. **Conditions:** 16 suite positions, Hash 64 on every arm,
fresh process per depth, cumulative `go depth N` node counts. Oracle arm is
`Use Basilisk HCE=true`, so only the search differs. Engine unchanged.

## The cost curve

| depth | Basilisk | oracle | ratio |
|---:|---:|---:|---:|
| 1 | 1,718 | 1,148 | 1.50× |
| 2 | 6,008 | 1,923 | 3.12× |
| 3 | 11,514 | 3,377 | 3.41× |
| **4** | **29,482** | **6,732** | **4.38×** |
| 6 | 88,653 | 25,683 | 3.45× |
| 8 | 311,338 | 97,930 | 3.18× |
| 11 | 1,170,224 | 588,190 | 1.99× |

The excess is **not** a fixed startup overhead: it *rises* from 1.50× at depth 1
to a peak of **4.38× at depth 4**, then decays to 1.99× by depth 11. Combined
with our better branching ratio — 1.692 against 1.894 (BAS-D05) — the picture is
a search that pays a large penalty in a narrow band and then grows more slowly
than the reference.

## Both components carry the same excess

| depth | interior ratio | qsearch ratio |
|---:|---:|---:|
| 4 | 4.69× | 4.05× |
| 6 | 3.31× | 3.68× |
| 8 | 3.06× | 3.37× |
| 11 | 1.95× | 2.05× |

Interior and quiescence are inflated by the **same** factor at every depth and
converge together. This rules out two separate causes: it is not "qsearch is
expensive" plus "interior pruning is weak". One cause expresses itself in both,
which is what a node-count multiplier at the top of a subtree does.

It also confirms 5.5's finding from the other direction — our qsearch share
(30.8%) is *lower* than the reference's, so qsearch is not disproportionate; it
is simply carried along by whatever inflates the tree above it.

## Where the target is

**Depths 2 to 6**, where the ratio runs 3.1× to 4.4×. That band is exactly where
our shallow pruning lives: razoring (`depth <= 3`), futility, late-move pruning
and history pruning (all `depth <= 6`), with reverse futility spanning `<= 9`.

This is a different target from anything Phase 5 has tested. Clusters 5.4, 5.6
and their candidates were judged on **depth reached at 300,000 nodes**, a metric
dominated by deep search, which a shallow-band saving barely moves. BAS-D04's
conclusion that reviving history pruning "buys no depth" is consistent with that
and is not contradicted here — but it was measured against the wrong band.

## What this does not claim

Absolute node counts are not comparable across engines. The finding rests on
each engine's own cost curve and on the *shape* of the ratio, which is
comparable. The engine-agnostic evidence that a real deficit exists remains the
equal-time tournament result: 15.6 plies against 25.2 (BAS-O01/O03).

No candidate is proposed here. 5.14 was registered as a diagnostic that names a
cause or closes, and it has named one: a 3–4× node multiplier concentrated in
depths 2–6, present in interior and quiescence alike.

## Disposition

Recorded as **BAS-D06**. Whether to open a cluster against the shallow band is
part of the pending budget decision, now taken alongside the maintainer's
2026-08-25 decision to unfreeze the HCE and run 5.9 as a maturity program.

---

## Revision, same day — measured to depth 19

Stopping at depth 11 was a blind spot; games reach 15–25 plies and the ratio was
still moving. Extended:

| depth | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| ratio | 1.99× | 1.78× | 2.06× | 2.06× | 1.75× | 1.95× | 1.92× | 1.86× | 1.80× |

| segment | Basilisk | oracle |
|---|---:|---:|
| b(4–11) | 1.692 | 1.894 |
| b(11–19) | **1.570** | **1.590** |
| b(11–19) per-position median | 1.548 | 1.558 |

**The ratio plateaus at ~1.8–2.0× and does not close.** Deep branching is
**equal**, not better for us — the shallow-segment advantage was an artifact of
that band and does not describe asymptotic behaviour. The crossover projected
from the 4–11 data does not exist.

**BAS-O04's 12-ply gap is also corrected.** It was a mean over a distribution
containing forced mates at depth 100 and 245; 10 of 105 positions reached ≥100.
The median gap is **4.00 plies** (13 against 17).

**Open discrepancy.** A flat 1.9× cost at b≈1.55 predicts ~1.5 plies against a
measured median of 4.0; ~2.5 plies is unexplained. Recorded as open. A cluster
against the shallow band should wait until it is settled.
