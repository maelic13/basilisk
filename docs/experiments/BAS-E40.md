# BAS-E40

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E40 — 6.1.d: both bishop-dependent KBNK remedies stay closed, and the
reason is now stronger than their refutations** (2026-09-03). No new games were
run; this entry registers the closure and its retry triggers.

BAS-E36 diagnosed the KBNK failures as *stuck*, not slow, and traced the
mechanism correctly: every term in `kbnk_score` is a function of the weak king,
our king or the knight, so **nothing depends on the bishop**, all bishop moves
score exactly alike, and the potential has a flat maximum that is not mate. The
natural inference was to add a bishop-dependent term. Both attempts failed:

| arm | mechanism | verdict | decisive evidence |
|---|---|---|---|
| BAS-E36 Arm B | bishop proximity to the weak king, weight 300 | REFUTED | conversion 94 -> 88 is within one SE, but piece lost before ply 10 went **0 -> 2**; pulling a long-range piece next to a bare king gets it captured |
| BAS-E36 Arm C | weak king's escape-square count, weight 400 | REFUTED | KBN-K conversion **14/16 -> 9/16**, about 3.8 SE, failing the deterministic CTest floor; rejected without spending the 198-position run |
