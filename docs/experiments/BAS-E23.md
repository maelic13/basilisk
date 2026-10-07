# BAS-E23

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E23 — 5.9.13 ACCEPTED: +9.52 Elo from unfreezing the piece-square tables**
(2026-08-29). `basilisk-5912-full-pext-pgo` (`e7e6b61211`, bench 20,005,943)
against `basilisk-5914-ks-pext-pgo` (`a534b1ab87`, bench 12,844,350). `3+0.03`,
1T, Hash 64 both arms, UHO.

**Elo +9.52 ±4.66, nElo +14.58 ±7.14, LOS 100.00%, LLR 2.95 → H1 accepted** in
**9,092 games / 1h42m**. Ptnml [189, 1026, 1903, 1203, 225], PairsRatio 1.18.
No time forfeits. The fastest and largest acceptance of the phase.

*Phase 5.9 cumulative: **+2.64** (BAS-E21, king safety) and **+9.52** here, so
roughly **+12 Elo** against the head the phase started from.*

**Holdout loss is not merely noisy as a predictor — it has now inverted twice.**

| change | holdout | Elo |
|---|---:|---:|
| 5.9.4 refit on distilled labels | **−6.2%** | **−77.92** |
| 5.9.12 full-surface refit | **−0.43%** | **+9.52** |

A 14× smaller holdout improvement produced an 87-Elo swing in the opposite
direction. Any future reasoning that leans on a holdout delta to predict, rank
or justify a candidate should be treated as unsupported. Only the gate decides.

*What actually produced the phase's gains.* Both accepted changes came from
**fitting surfaces we already had, with the right instrument, on correct data**:

- the capped non-linear king-danger funnel, reachable only by coordinate
  descent, which a linear fit had nothing to say about (BAS-E21);
- the 768 piece-square tables, frozen since **Phase 4.7** and never touched by
  any fit in this phase until 5.9.12.

**Neither gain came from new evaluation features.** The sixteen terms added at
5.9.1/5.9.2 were measured inert three separate times — BAS-E08, then again with
the PSTs free (BAS-E22), with the fit itself zeroing 12 of their 20 values. The
phase's lesson is that our evaluation was **mis-calibrated, not
under-featured**, and that the expensive part was discovering which surfaces had
never been fitted at all.

*Immediate consequences.*

1. The refuted terms should be **removed** — a separate simplification gate
   (`-Mode simplify`), never bundled with a gain.
2. Coverage now stands at 1,173 of 1,190 fittable parameters. The 7 `winnable`
   params are the only reachable set still unfitted, and they need a
   finite-difference instrument; they are independent of everything in 5.9.12
   and can be done at any time.
3. A third fit iteration is untested. The two-iteration run moved holdout
   0.0791382 → 0.0787951; whether a third buys anything is a ~40-minute
   question, and given the table above, only a gate could answer it.
