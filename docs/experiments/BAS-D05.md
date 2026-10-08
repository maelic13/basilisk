# BAS-D05

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D05 — consecutive-depth branching; the leading diagnosis is overturned**
(16 suite positions, depths 4–11, **Hash 64 on every arm**, 2026-08-13;
`analysis/manta_import_v1.md`). Method imported from Manta `MAN-S23`: branching
is the ratio between consecutive depths, because a single-depth
`nodes^(1/depth)` estimate folds in the fixed cost of the first plies. Every EBF
figure previously recorded here used that folded estimator.

| | Basilisk | SF search + our eval |
|---|---:|---:|
| b(4–11) aggregate | **1.692** | 1.894 |
| per-position median | 1.699 | 1.899 |
| nodes at depth 4 | 29,482 | 6,732 (**4.38×**) |
| nodes at depth 11 | 1,170,224 | 588,190 (1.99×) |

**Our per-ply growth is BETTER than the reference search's.** The deficit is a
constant factor — 4.4× at depth 4 decaying to 2.0× by depth 11, which is what a
better ratio does to a worse starting point.

*Conditional lesson.* Phase 5 spent three clusters on the premise that our tree
is too wide per ply, and every attempt to cut harder failed (BAS-S13/S14
neutral, BAS-S16 −3.48 Elo, BAS-D04 no depth). Those failures now share one
explanation: **the growth rate was never the deficit**, so cutting harder could
not help. What is deficient is what a shallow subtree costs us. The 12-ply
equal-node gap and the ~9.6-ply equal-time gap (BAS-O01/O03, engine-agnostic)
both stand; only their attribution changes. Absolute node counts are never
comparable across engines, but both count interior and quiescence nodes, so the
ratio is sound in kind.

*Disposition.* Supersedes the EBF framing in BAS-O03/O04. Registered as PLAN
5.14.
