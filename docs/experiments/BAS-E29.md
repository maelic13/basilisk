# BAS-E29

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E29 — 5.9.17 continued: KBNK conversion 13.0% → 54.5%, and WHY the old
drive failed** (2026-08-31). Matched comparison, identical 200 random legal KBNK
positions, engine playing both sides at 60,000 nodes:

| outcome | pre-5.9.17 | after |
|---|---:|---:|
| **mated** | **26 (13.0%)** | **109 (54.5%)** |
| fifty-move draw | 144 (72.0%) | 62 (31.0%) |
| other / stalemate | 30 (15.0%) | 29 (14.5%) |

**A 4.2× improvement, roughly 12 SE.**

**The mechanism is the finding, and it generalises beyond this ending.** The old
drive was `(7 − chebyshev_corner) × 250 + (8 − king_dist) × 30`. Two defects:

1. **Chebyshev distance has large plateaus** — a whole L-shaped band of squares
   shares one value — so over most of the board the potential was flat and
   there was nothing for the search to follow. Manhattan distance has the same
   minimum with far fewer ties.
2. **The gradient was smaller than our own pruning margins.** A king-distance
   term of **30cp per step** sits far below futility, razoring and RFP margins
   of roughly 100–500cp, so the search *pruned away the very moves that made
   progress*. This is why scaling every weight up together helped, even though
   `apply_endgame` returns `kbnk_score` as an override and a pure rescaling
   should be behaviour-neutral: it is not the ratios that matter, it is whether
   the differences survive the pruning thresholds.

*Weight sweep, n=60 screening:*

| corner / edge / king / knight | mated |
|---|---:|
| 250 / 0 / 30 / 20 | 18.3% |
| 200 / 200 / 100 / 40 | 21.7% |
| 400 / 500 / 100 / 100 | 38.3% |
| 500 / 600 / 150 / 150 | 48.3% |
| **800 / 900 / 220 / 220** | **66.7%** |
| 900 / 1000 / 250 / 250 | 63.3% |
| 1000 / 1100 / 280 / 280 | 63.3% |

The plateau is real and the ceiling is hard: at 1000/1100/280/280 the maximum
evaluation reaches **31,780** against a mate boundary of **MATE_SCORE −
MAX_PLY = 31,872**. Shipped at **800/900/220/220**, whose maximum is 27,420 —
verified at **+18,056** on a textbook position, comfortably clear.

*n=60 over-estimated it at 66.7%; the honest number is the n=200 figure of
54.5%.* The first sixty positions of the fixed seed are easier than average, and
the screening sweep is a ranking instrument, not a measurement.

**Still not solved, and the remaining failures are two different problems.**
31% still reach the fifty-move rule — technique too slow. **14.5% end in
stalemate or otherwise, essentially unchanged from 15.0%** — nothing here
addressed stalemate avoidance, which is a distinct defect and the obvious next
target.

*Correctness.* CTest 12/12, mate-drive canary intact, all three KNNK draw
assertions intact, the KNK non-trigger assertion intact, WAC 137/300, tuner
reconstruction exact on 10,000 positions. Bench unchanged at 12,709,666 — the
bench suite still contains no KBNK position.
