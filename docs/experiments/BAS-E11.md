# BAS-E11

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E11 — 5.9.6 REJECTED at −77.92 Elo; no measured mechanism explains it**
(2026-08-26). `basilisk-5.9.5-cand-pext-pgo` (`fd048497a5`, bench 18,228,447)
against `basilisk-1.9.3-baseline-pext-pgo` (`16eff201`, clean, bench
11,941,440). `3+0.03`, 1T, Hash 64 both arms, UHO, concurrency 14.

**Elo −77.92 ±15.32, nElo −99.71 ±18.94, LOS 0.00%, 1,292 games, LLR −2.95 →
H0.** Ptnml [117, 199, 219, 74, 37], PairsRatio 0.35. The loss is broad and
consistent, not a few catastrophic games.

Every hypothesis was then tested **without further games**. None of them, alone
or summed, accounts for the result.

| hypothesis | measurement | verdict |
|---|---|---|
| NPS regression | new-term code costs **4.1%** (3.358M → 3.221M) at *identical* bench nodes | real, ~2–4 Elo |
| tree growth | bench nodes +52.6% at fixed depth; time-to-depth **+60%** | real, maybe 10–15 Elo |
| depth at equal nodes | **−0.196 ply** at 1M, 107 paired positions | real, small |
| static eval quality | holdout **0.0656034** vs baseline 0.0703086 — **6.7% better** | refuted as cause |
| tactical ability | WAC@12: **49 fails baseline, 48 candidate** | refuted as cause |
| lazy-eval divergence | 0 sign flips both; crossings 2.191% → **1.365%** (better); mean abs delta 157.8 → 181.1 | refuted as cause |
| score-scale drift | `K` 1.41868 → 1.47613, **4.1% compression** | real, few Elo |

*The residual is the finding.* Identified costs sum to perhaps 15–20 Elo against
a measured 78. **The fitted values are simply worse in play while being better on
the corpus.** This is BAS-X02's lesson at roughly four times the magnitude —
there, Stockfish distillation improved holdout 4.9% and lost 17.11 Elo in Rarog;
here 6.7% better holdout costs 77.92.

*CORRECTION (2026-08-26, same day). The primary cause was misdiagnosed below.*
The `beast_sf_*` corpus is **Stockfish-distillation labelled**, not self-play
WDL. `import_beast.py` imports `FEN<TAB>target` where the target is a Stockfish
expected score, and the data confirms it: **427 distinct target values** in
200,000 rows, continuous on [0,1], with only ~18% at the discrete 0 / 0.5 / 1 a
game result would produce. 5.9.4 therefore fitted 348 coefficients to reproduce
**Stockfish's static evaluation**, and the "holdout loss" it improved by 6.7%
measures *agreement with Stockfish*, not agreement with winning.

**Our own ledger already priced this: BAS-X02 — Stockfish distillation improved
holdout 4.9% and lost −17.11 Elo in Rarog.** That is the recorded failure mode,
and 5.9.4 walked into it at four times the scale. I described this corpus as
"Basilisk self-play WDL" throughout 5.9 without once checking the label format.
The quiet-filter account below remains true and independently verified — it
explains the mate-drive canary (BAS-E10) and the missing endings (BAS-E14) —
but it is the **secondary** factor, not the primary one.

*The secondary reason, and it matches 5.9.5's canary failure exactly.* The
corpus is also **quiet-filtered and off-policy**. Quiet filtering removes positions
with live attacks on the king — precisely where king safety governs — so the
objective cannot see whether king-safety values are right where they matter, and
the fit is free to set them wrongly at no measured cost. BAS-E10 found the same
blindness for mating positions, which the corpus lacks entirely. Two independent
manifestations of one defect: **the corpus does not contain the position classes
these terms exist to price.**

*A structural blindness worth recording separately.* The tuner builds with
`TEXEL_TRACE`, which **disables the lazy-eval skip** (`eval.cpp`, `#ifndef
TEXEL_TRACE`). It therefore fits the full evaluation and is constitutionally
unable to see that every centipawn it adds to the skipped block widens the error
`LAZY_MARGIN = 700` silently accepts. Measured here as harmless — sign flips
stayed at zero — but it is a real gap between what is fitted and what is played.

*Retry trigger.* Do not re-gate any fit produced from a quiet-filtered
off-policy corpus. 5.9.11 must regenerate on-policy **and** carry the position
classes the terms govern — sharp king attacks and mating material — before
5.9.12 refits.
