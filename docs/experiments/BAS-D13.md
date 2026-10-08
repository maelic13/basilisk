# BAS-D13

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D13 — 5.7.4 REFUTED: the reference's verification search is neutral, and
its apparent gain was an outlier artifact** (2026-08-30). Implemented behind an
inert knob, measured on both instruments, reverted. No games spent.

*Reach first.* The `tt_score >= beta` branch fires **25,124** times on the
107-position suite at depth 12 — **0.1775% of interior nodes**, comparable to
the 3-ply stack 5.7.3 examined.

*Depth at equal nodes, verify(1) minus current(0):*

| | value |
|---|---|
| mean | **+0.383** |
| **median** | **+0.000** |
| better / worse / equal | **27 / 25 / 55** |
| largest swings | **+11, +11, +9** — all trivial pawn/king endgames |

**The mean is an artifact.** Three positions such as `8/8/8/4k3/8/4P3/8/4K3` and
`7k/8/7K/7P/8/8/8/8` carry it: shallow-material endgames where depth is cheap and
a small pruning gain compounds into double-digit plies. The median is zero and
the better/worse split is a coin flip. Excluding mate runaways does not help
(+0.433) because these are not mate runaways.

*WAC:* **138 vs 137** — one position, noise, both far above the floor of 130.

**Disposition: keep our negative extension.** No broad gain by either
instrument, and the reference's form costs an additional search at every firing.
This is a design decision taken on evidence rather than a gap left unclosed:
ours post-dates `9587eeeb`, and nothing here argues for trading it.

*Method note — BAS-D11's lesson generalised.* 5.7.3 was caught by WAC after the
depth sweep endorsed it; 5.7.4 was caught by the **median and the better/worse
split** after the depth *mean* endorsed it. The recurring failure is trusting a
single aggregate. For search work: report mean **and** median **and** the
directional split, and clear WAC as well. Both refutations came free.

*Kept:* the `sing_ttbeta` counter, since the branch still fires and its reach is
the evidence base for anything revisiting it.
