# BAS-E28

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E28 — 5.9.17: KBNK conversion measured directly, and nearly doubled**
(2026-08-31).

*Baseline, and it is far worse than the game statistic suggested.* Random legal
KBNK positions, engine playing **both** sides at 60,000 nodes:

| outcome | n=40 |
|---|---:|
| **mated** | **8 (20.0%)** |
| fifty-move draw | 25 (62.5%) |
| other / stalemate | 7 (17.5%) |

BAS-E27's 0.549 game result understated the defect, because positions arising in
games are often already part-converted. **When it does mate, the median is 53
plies against a 100-ply limit** — so the technique is not slow, it fails to
find the plan at all.

*Cause.* `kbnk_score` was `KNOWN_WIN + (7 − corner_dist) × 250 + (8 −
king_dist) × 30` — **both terms price only the two kings.** Nothing rewarded
bringing the knight to bear, and in KBNK the knight is what removes the weak
king's escape squares. The search had no gradient toward the piece the mate
depends on.

*Sweep* (n=30, then the two survivors re-run at n=120 on identical positions):

| `kbnk_knight_prox` | mated (n=30) | mated (n=120) |
|---:|---:|---:|
| **0** | 23.3% | **11.7%** (14) |
| **20** | 33.3% | **21.7%** (26) |
| 60 | 23.3% | — |
| 150 | 20.0% | — |

**n=30 could not resolve it** — 7 mates against 10 is barely one standard
error, and I did not treat it as a result. At n=120 on the same positions, 14
against 26 is roughly **3 SE**, and conversion nearly doubles. Shipped at 20.

*What this does NOT claim.* **21.7% is still bad.** A correct KBNK
implementation converts near 100%. This fixes a missing gradient term, not the
technique: our corner drive is a coarse Chebyshev distance where the standard
formulation uses a corner-distance table. The class remains open work, and the
Elo available from finishing it is larger than what this step banks.

*Correctness.* CTest 12/12, mate-drive canary intact, KNNK draws intact, WAC
137/300, tuner reconstruction exact. Bench unchanged at 12,709,666 — the bench
suite contains no KBNK position, which is itself worth knowing.
