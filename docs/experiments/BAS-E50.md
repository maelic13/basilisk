# BAS-E50

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E50 - 6.0.c ceilings regenerated; the hard residue is 13 positions, not
75** (2026-09-03). No new games. `endgame_ceilings.py` re-run against the
corrected 6.0.b arms with the identical two binaries, so the instrument fix is
the only delta.

| | v1 (contaminated) | v2 (corrected) |
|---|---:|---:|
| accepted converted | 293/480 | 361/480 |
| reference converted | 389/480 | 466/480 |
| attained 60k reference ceiling | 389 | **466** |
| demonstrated solvable union | 405 | **467** |
| both / accepted-only / reference-only / neither | 277 / 16 / 112 / 75 | 360 / 1 / 106 / **13** |

Seven family ceilings were understated: KBPP-KB 6 to 23, KRP-KR 9 to 24,
KRP-KB 9 to 24, KPP-K 12 to 24, KBP-KN 15 to 24, KBP-KB 16 to 23, KBP-K 22 to
24. Aggregate understatement 77 positions.

**The paired matrix is the more useful correction.** The old `neither` bucket
of 75 read as a set of positions neither engine could convert -- a plausible
proxy for genuinely hard theory. It was mostly two aborted games. Of 480 clean
wins, 467 are demonstrably convertible by at least one engine at 60,000 nodes,
so the residual hard set is **13**. Any reasoning that treated the 75 as a
difficulty floor was reasoning about the instrument.

These ceilings are acceptance targets for later steps, so v1's values were not
merely wrong but lenient in exactly the families 6.8 and 6.10 may implement. v1 is kept
as the historical artifact and superseded; the generator's defaults now point
at the corrected inputs and a v2 output so a regeneration cannot restore them.
Supersedes the ceiling figures quoted in BAS-E08.
