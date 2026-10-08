# BAS-E43

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E43 — 6.0.g census: KBNK does not occur in search trees from real
positions** (2026-09-03). First use of the 6.0.g instrument, at depth 13 on the
frozen `suite_v1.epd`, one thread per root. Reported because it changes how the
6.2 gate must be read, not because it decides any candidate.

| | full suite, 107 roots | non-endgame roots only, 86 |
|---|---:|---:|
| nodes / evaluations | 31.6M / 14.3M | 30.5M / 14.1M |
| <=7-men evaluations | 234,930 (1.638% of evals) | 39,853 (0.283%) |
| **KBNK** | 112,463 | **0** |
| KXK | 161,406 | 3 |
| KPKP / KRPPKRP | 1,056 / 352 | 364 / 352 |

**The full-suite figure is an artifact and must not be quoted.** `suite_v1.epd`
is the frozen Phase-5.2 differential suite, and 21 of its 107 roots are already
seven men or fewer -- 13 three-man, 7 four-man, 1 five-man. Those roots *are*
bare-king endgames, so they produced essentially every KBNK evaluation counted.
Restricting to the 86 roots with eight or more men, KBNK occurs **zero times**
in 30.5M nodes and 14.1M evaluations, and the whole <=7-man band falls to
0.283% of evaluations.

*Consequence.* KBN-K cannot reach the score sheet through search-tree
influence; it contributes only when the position is actually reached on the
board. The 6.1 evaluation change is therefore invisible to ordinary play by
construction, which reframes 6.2 from a strength gate into a non-regression
gate. It also retires the 6.1.f magnitude concern in practice: a 31,660-point
static score that never appears in a real tree cannot propagate into one.

*What this does not say.* Eighty-six roots is a small corpus and a
search-differential suite is not a random sample of played positions in either
direction, and this is depth 13 only, where deeper searches liquidate further.
The defensible claim is zero on this corpus, not zero in chess. Nor does it
devalue 6.1: when KBN-K is reached, conversion against a fifty-move draw is a
whole point, and BAS-E41 moved that from 95/198 to 144/198.
