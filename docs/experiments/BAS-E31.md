# BAS-E31

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E31 — 5.9.18: randomised conversion floors added to CTest**
(2026-08-31). The existing endgame suite gated a handful of hand-picked EPD
positions, which is precisely why it stayed green throughout the period KBNK
converted 13% of random positions. It tests *evaluation direction*, not whether
the engine can finish.

Four randomised per-family floors, fixed-seed LCG so the position set is
identical every run and a failure is reproducible. Floors set well below
measured rates — they are floors like WAC's, not rate assertions, so ordinary
search churn cannot trip them:

| family | measured | floor |
|---|---:|---:|
| `KQ-K` | 12/12 | **12** (deterministic; must stay perfect) |
| `KR-K` | 12/12 | **12** (deterministic; must stay perfect) |
| `KBB-K` | 3/12 | 1 |
| `KBN-K` | 14/16 | 10 |

Runtime 48s, against 2m53s for a first version that used a fixed depth 18.

*Three defects in that first version, all found by running it:* it generated
**same-coloured bishop pairs**, which are a genuine draw, so 7/16 was measuring
the generator rather than the engine; depth 18 made the suite far too slow; and
a fixed depth conflated knowledge with search effort — KBB-K scored 2/12 at
depth 10 *and* at depth 14. Switching to a node limit matched the measurement
instrument.

**Standing instruction recorded in the test itself:** raise each floor when the
matching conversion work lands. A floor left at an old rate silently stops
protecting the improvement that replaced it.
