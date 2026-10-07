# BAS-D03

<!-- part 1 of 2: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D03 is superseded.** Qsearch share moved **30.8% → 35.1%** (suite-wide,
depth 12, same protocol — a first single-position reading of 35.0% was
discarded as non-comparable before this was concluded). BAS-D03 concluded "ours
is *smaller* than the reference's 36–37%; not a width source". **We are now
inside that band.** The conclusion no longer follows; qsearch share is a matched
quantity, not a favourable outlier. Nothing in 5.7 depends on it, but any future
argument citing BAS-D03's *smaller* must use this row instead.

*Not re-measured, and why that is defensible:* BAS-O01/O04's oracle attribution
(+322.7 Elo search, 95.9%/4.1% split) holds **Basilisk's evaluation constant on
both arms** — the hybrid runs SF's search against our eval versus our search
against our eval. An evaluation improvement lifts both arms, so the *search*
contrast is structurally insensitive to it. Re-measuring costs 2,400 games and a
rebuilt bridge to move a 322-Elo finding by at most a few Elo. Deferred with the
reason recorded; re-open if a decision ever turns on the exact split.

<!-- part 2 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D03 — qsearch share, all three arms** (`suite_v1.epd`, fixed 300,000
nodes, 2026-08-13). Basilisk native **30.8%**; SF search + Basilisk HCE
**36.1%**; SF search + SF HCE **37.0%**. Our qsearch is *smaller* than the
reference's, which spends a larger share of its nodes there while still
reaching 12 more plies. **Qsearch is not a source of wasted width** and the
hypothesis that it might be is closed. Measured by adding a behaviour-neutral
qsearch counter to the vendored Stockfish on a **derived branch `hybrid-diag`** (tag `oracle/hybrid-diag`, `324ace4`);
the frozen `hybrid` oracle stays at `01df815` with its tournament binary
untouched.

**Differential at equal nodes.** Given the same 300,000-node budget, Basilisk
reaches mean depth **20.80** and the oracle **32.87** — **+12.07 plies** on
identical evaluation. This reproduces BAS-O03's time-based EBF finding under a
node budget, so it is not a throughput artifact in any form.

**What this does not yet say.** The counters localize the width; they do not
prove that reducing more would gain Elo, and they credit no specific change.
Durable lesson 5 cuts both ways — a smaller tree can also be worse. Cluster 5.4
must gate any reduction change on games, and prune recall (would a pruned move
have been best?) is not yet instrumented, so "reduce more" remains a hypothesis
with a mechanism, not a finding.
