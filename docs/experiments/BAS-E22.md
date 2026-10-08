# BAS-E22

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E22 — 5.9.12 full-surface fit: the PSTs move, and the added terms are
refuted** (2026-08-28/29, `--tune texel` 1,116 params + `--tune-kingsafety` 57,
two iterations, arm C corpus). The run completed in ~42 minutes — the
king-safety stage was ~20 min per iteration, not the 90 I estimated — and
survived a Windows-update reboot that destroyed only the console output. The
final bake is verified landed: re-baking `5912_ks_it2.txt` reports **0 changed**.

| | value |
|---|---|
| holdout | 0.0791382 → **0.0787951**, −0.43% |
| bench | 12,844,350 → **20,005,943**, +55.8% |
| paired depth vs the 5.9.14 head | **−0.037 ply** (27 deeper / 31 shallower / 49 equal) |
| CTest | 12/12 |
| mate canary | on=**35** (5.9.14 head 37, pre-5.9.14 baseline 29) |
| `K` score-scale audit | 1.94497 → **1.9107**, +1.8% scale expansion |

*Score-scale audit passes.* `K` moved 1.8%, against 5.6% at 5.9.4. Our
centipawn margins are effectively 1.8% *less* aggressive than calibrated —
small, recorded, not repaired. Bench is again a poor proxy: +55.8% nodes costs
**no depth at all** at equal nodes, exactly as BAS-E09 established.

**THE BAS-E08 ABLATION IS SETTLED, AND IT REFUTES THE FROZEN-PST EXPLANATION.**

BAS-E16 argued the 5.9.1/5.9.2 terms measured inert because they compete with a
piece-square table frozen since Phase 4.7 that already holds their geometric
signal. 5.9.12 unfroze it. Zeroing the twenty term values on the finished
surface gives holdout **0.0787873** against **0.0787951** with them active —
**removing them is very slightly BETTER**, and the difference is inside noise
either way.

The fit's own verdict is the same: **12 of the 20 values are exactly 0**, with
`trapped_rook`, `threat_safe_pawn` and `bishop_outpost` all zero in both phases.

So the terms are genuinely redundant, not shackled. The BAS-E16 hypothesis was
reasonable and is now dead. **Disposition: remove them**, per PLAN 5.9.12's
stated rule. That is a separate simplification, not part of this gate —
bundling "refit 768 PSTs" with "delete 20 terms" would confound two changes.
Removing them also recovers the NPS the zero-guards cannot reclaim while eight
of the values are non-zero.

*What 5.9.12 is worth on its own.* A −0.43% holdout improvement from unfreezing
the largest remaining surface, at no depth cost. Holdout has repeatedly failed
to predict Elo here, so this is a candidate for a gate and nothing more.

*Winnable remains outside all of this, and can stay outside.* Its 7 parameters
are independent of both the gradient set and the king-safety descent, so they
can be fitted later without redoing any of 5.9.12 — a finite-difference
instrument added at any point applies on top of the current state.
