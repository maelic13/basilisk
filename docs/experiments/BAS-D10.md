# BAS-D10

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D10 — `singularQuietLMR` implemented; the reference's magnitude is wrong
for us by 8×** (2026-08-30, step 5.7.2).

*The semantics were nearly misread, and the misreading would have shipped dead
code.* The flag is reset **once per node**, not per move, and set when the TT
move proves singular — so it relaxes LMR for **every later move at that node**,
not for the extended move itself. The extended move is the TT move, ordered
first, so `searched < 2` blocks LMR from ever reaching it. "Reduce the extended
move less" is unimplementable; "reduce its siblings less because the position is
sharp" is the mechanism.

*Plumbing verified before magnitude:* at `LmrSingularQuiet = 0` the bench is
**13,981,020**, matching the head **exactly**, so the mechanism is inert when
disabled and fires only through the intended path.

*Magnitude swept with `sweep.py`* (107 positions, 300k nodes, Hash 64, paired):

| value | mean depth | paired Δ | better / worse |
|---:|---:|---:|---|
| 0 | 21.30 | — | — |
| 128 | 21.38 | +0.084 | 32 / 24 |
| 256 | 21.41 | +0.112 | 26 / 32 |
| **401** | **21.42** | **+0.121** | **32 / 23** |
| **1024** (the reference's `r -= 1`) | 21.08 | **−0.215** | 26 / 33 |

**Porting the reference's constant would have been a regression**, and a large
one — a full ply doubled the bench (13.98M → 29.47M) and broke a mate-distance
test. Our LMR mean reduction is 2.337 plies and our singular gate is one ply
earlier than the reference's, so a full-ply relaxation removes ~43% of the
reduction at every singular node. Shipped at **401**; 128/256/401 are within
noise of each other, so this is a plausible operating point, not a tuned one.

*Instrument note.* `run_bench()` takes no `SearchParams` and therefore ignores
UCI options entirely — a parameter sweep driven by `bench` returns the identical
node count for every value, which is what it did here before the mistake was
caught. This is correct for a fingerprint and useless for a sweep. `sweep.py`
sets options on a real `go nodes` search and is the instrument for this.
