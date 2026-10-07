# BAS-D16

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D16 — 5.8.2 aspiration instrumentation, and 5.8.3/5.8.4 REFUTED**
(2026-08-30). Nothing had ever counted the aspiration path, so none of cluster
E's candidates could be sized before implementing them.

*Measured*, 107 positions at depth 14 — **events per window, not percentages
of windows**:

| | count | per window |
|---|---:|---:|
| root iterations using a window | 1,066 | — |
| fail-low events | 544 | **0.51** |
| fail-high events | 910 | **0.85** |
| total re-searches | 1,463 | **1.37** |
| `delta >= 900` give-up | 9 | 0.008 |

**More than one full root re-search per iteration on average**, and the give-up
hatch does fire — rarely, but it is not dead code.

*Then the two window candidates, swept at 300k nodes:*

| config | re-searches | depth mean | median | better / worse |
|---|---:|---:|---:|---|
| current | 1,305 | — | — | — |
| **5.8.3** fail-low narrows beta | **1,342** | +0.131 | +0.0 | **18 / 19** |
| **5.8.4** delta growth /4 (ref-like) | 1,419 | **−0.243** | +0.0 | 20 / 33 |
| both | 1,515 | −0.009 | +0.0 | 21 / 25 |

**5.8.3 refutes its own rationale.** The argument was that pulling beta to the
midpoint makes the fail-low re-search cheaper. It makes re-searches **more
frequent** — 1,305 to 1,342 — because a tighter window simply fails again.
The depth split is 18/19, no direction, so the +0.131 mean is the BAS-D13
pattern again.

**5.8.4 is clearly worse.** The reference's slower growth (`delta/4 + 5` against
our `delta/2`) costs 0.243 ply on a 20/33 split and adds re-searches. Our faster
escalation is the better trade here.
