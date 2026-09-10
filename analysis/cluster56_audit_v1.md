# Cluster 5.6 audit — main selectivity

**Date:** 2026-08-13. **Verdict: no candidate.** One genuine structural defect
found and characterised; reviving it measures as depth-neutral, and the 5.4.4
precedent says a tree that shrinks without gaining depth loses Elo. Recorded
with a retry trigger rather than gated. Engine unchanged, bench 11,941,440,
CTest 12/12.

## 1. History pruning — a real defect, but not a candidate

### The defect

The live condition is `hist < -hist_prune_coeff * depth`, where `hist` sums six
**bounded** history channels:

| Channel | Bound |
|---|---:|
| main | 16,384 |
| continuation ply-1 | 16,384 |
| continuation ply-2 | 16,384 |
| continuation ply-4 (halved) | 8,192 |
| pawn | 16,384 |
| low-ply | 8,192 |
| **maximum \|hist\|** | **81,920** |

The threshold scales with depth; the signal does not. At `coeff = 14004`:

| Depth | Threshold | Fraction of max needed |
|---:|---:|---:|
| 1 | 14,004 | 17% |
| 3 | 42,012 | 51% |
| 5 | 70,020 | 85% |
| **6** | **84,024** | **103% — provably unsatisfiable** |

At depth 6 the condition can never fire. At depth 5 it needs 85% of theoretical
maximum negative on every channel simultaneously. The mechanism is live only at
depths 1–2 in practice, which is why it fires **142 times in 5,355,599** quiet
moves that reach the test — 0.003%.

This is the tail of the `hcefinal` SPSA: it set `HistPruneCoeff = 14004` while
re-scaling the history space, leaving the threshold stranded above the
distribution it is meant to cut.

### Why it is still not a candidate

The population is genuinely there — loosening the coefficient would activate a
real mechanism:

| Threshold | Fires | Share of tested |
|---|---:|---:|
| current (`coeff × depth`) | 142 | 0.003% |
| `coeff/2` | 95,418 | 1.78% |
| `coeff/4` | 234,235 | 4.37% |
| `coeff/8` | 467,647 | 8.73% |

But paired depth at a fixed 300,000 nodes over `suite_v1.epd` is **flat**:

| Coefficient | Δ paired ply |
|---|---:|
| 7002 (`/2`) | −0.019 |
| 3501 (`/4`) | +0.037 |
| 1750 (`/8`) | −0.019 |

All inside noise. The mechanism would prune 4–9% of tested quiets and buy no
depth — and BAS-S16 established, at −3.48 ±3.32 Elo, that a tree which shrinks
without gaining depth costs strength. Gating this would be spending three hours
to re-learn that lesson.

Redundancy explains it: move-count pruning already fires 22.2M times against
15.1M interior nodes, so the late quiets history pruning would catch are mostly
gone before it is consulted, and those that remain sit at depth ≤ 6 behind heavy
LMR — cheap subtrees whose removal saves little.

**Retry trigger.** Re-open only if move-count pruning is restructured such that
the surviving quiet population changes materially, or if a future diagnostic
shows the pruned moves carry *quality* cost rather than node cost. Not on a new
coefficient alone.

## 2. ProbCut — correctly rare, not broken

The second flagged anomaly, at 0.4% of interior nodes, is **not** a defect.
It is gated on `depth >= 5` plus a capture whose SEE clears
`pc_beta - static_eval`, and when it does fire it succeeds **56,311 of 84,469
times — a 67% hit rate**. A mechanism with that success rate is well targeted,
not stranded; it is simply asking a demanding question. No work.

## 3. The remaining selectivity population is healthy

| Mechanism | Fires | Share of interior |
|---|---:|---:|
| move-count pruning (per move) | 22,211,538 | 146.7% |
| reverse futility | 2,148,693 | 14.2% |
| futility | 1,032,317 | 6.8% |
| razoring | 872,839 | 5.8% |
| SEE pruning | 667,957 | 4.4% |
| null-move cuts | 317,065 | 2.1% |
| ProbCut | 56,311 | 0.4% |
| history pruning | 142 | 0.001% |

Only history pruning is anomalous, and it is characterised above.

## 4. A measurement bug found and fixed

The first run of the reachability probe produced an arithmetically impossible
series — `coeff/4` firing more often than `coeff/8`, when a looser threshold
must fire at least as often. Cause: `print_diag` built its machine-readable
line into `char buf[256]`, and the grown line exceeded it. `snprintf` truncates
silently and always loses the **tail** field, so the corruption scaled with the
counter magnitudes.

The buffer is now 512 and the history probe has its own line. Worth recording
as a pattern: the bug was caught only because the series had an invariant that
made the corruption *visibly* impossible. A truncated counter with no such
invariant would have been believed.

## 5. Conclusion, and the budget clause

Cluster 5.6 closes with **no candidate**.

PLAN's honesty clause named 5.5 and 5.6. **Both have now closed empty.** The
clause says to re-open whether the remaining Phase-5 budget is better spent
going to NNUE, and explicitly not to argue around it. That question is now
live and belongs to the maintainer.
