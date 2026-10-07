# BAS-D08

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D08 — per-iteration cost; there is no shallow target** (same runs,
2026-08-25). Cumulative counts hide where the cost is. Differencing them gives
the cost of each iteration on its own:

| depth | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| per-iteration ratio | 1.56× | 2.56× | 2.05× | 1.31× | 2.37× | 1.85× | 1.75× | 1.69× |

Our **depth-19 iteration itself** costs about 1.7× theirs. The cumulative ratio
sits near 1.9× because the per-iteration ratio is near 1.9× at every depth — not
because a shallow overhead is carried forward.

**Depths ≤6 are 0.205% of a depth-19 search.** Removing that entire band
outright would change nothing.

*Correction.* BAS-D06 and BAS-D07 both concluded that depths 2–6 were the place
to attack — first because the excess peaked there, then because a saving there
would "propagate unchanged". **Both readings were wrong.** The depth-4 peak of
4.38× is an artifact of cumulative accounting: at shallow depths the early
iterations are most of the total, so their ratio dominates it. The propagation
argument was worse — it inferred a mechanism from a constant ratio when the
constancy has the simpler explanation that every iteration costs the same
multiple.

*Disposition.* **5.14 yields no localized target.** The deficit is a uniform
per-iteration cost multiplier of roughly 1.9× at all depths, with no band where
work would pay disproportionately. Any attack has to make the whole search
cheaper per unit depth, which is what clusters 5.4–5.6 already failed to do. The
~2.5-ply discrepancy from BAS-D07 remains open and is now the only concrete
unexplained quantity left in this line of enquiry.
