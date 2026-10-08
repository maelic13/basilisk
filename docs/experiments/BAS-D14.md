# BAS-D14

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D14 — 5.7.5 singular gate depth: the two instruments measure the same
trade-off from opposite ends, and WAC-at-fixed-depth is biased** (2026-08-30).
`singular_min_depth` parameterised (default 5, unchanged) and swept.

| gate | depth mean | median | better/worse | WAC @6 |
|---:|---:|---:|---|---:|
| 4 | −0.121 | +0.0 | 18 / 38 | **162** |
| **5 (current)** | — | — | — | 137 |
| 6 (the reference) | +0.308 | +0.0 | 33 / 23 | 131 |
| 7 | +0.318 | +0.0 | 40 / 24 | **130** (at the floor) |

Lowering the gate runs more singular searches: worse depth, dramatically better
WAC. Raising it does the reverse. One trade-off, read from both ends.

**WAC at a fixed shallow depth is structurally biased toward more extension, and
this is the measurement that exposes it.** At depth 6, a gate of 4 lets singular
fire at depths 4–6 instead of 5–6, so critical lines are extended and more
tactics resolve *within the fixed depth*. That is a mechanical consequence of
the protocol, not evidence of strength. The +25 WAC positions at gate 4 are
largely this effect.

*This refines BAS-D11 rather than overturning it.* WAC remains a valid **floor**
— a candidate that drops below 130 has broken something. It is **not** a fair
comparator between settings that differ in how much they extend, because it
rewards extension mechanically. 5.7.3's WAC drop was partly this bias too; the
conclusion there still holds because it broke the floor outright, which is a
floor question rather than a comparison.

**Disposition: gate stays at 5.** Neither instrument can decide a trade-off it
is biased on, and nothing here justifies games. `singular_min_depth` is kept as
a tunable — unlike the refuted knobs of 5.7.3/5.7.4, its alternatives are not
measured worse, merely undecided, which makes it legitimate SPSA material later.
