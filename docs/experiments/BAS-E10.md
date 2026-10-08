# BAS-E10

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E10 — 5.9.5 king-safety coordinate descent; the table reshape breaks
mating and is deferred** (2026-08-26, `--tune-kingsafety`, 43 knobs, 1,520,109
train / 79,891 holdout, coordinate descent from step 8, K = 1.49757).

Converged in 83 passes; best holdout restored from **pass 75**, after which
holdout rose — the overfit onset was caught by the restore, not by luck.
Holdout **0.0658991 → 0.0652499**, −0.99%.

*What it changed, and it is not what the early probe suggested.* A 2-pass probe
on 100k positions appeared to raise the safety table uniformly, and that reading
was recorded here as a direction before it converged. It was wrong. The
converged fit made the table sharply **convex** — low end roughly halved, high
end up ~45%, crossover near index 15:

| index | 2 | 6 | 10 | 12 | 15 | 17 | 20 | 22 | 24 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| old | 6 | 55 | 111 | 161 | 192 | 203 | 302 | 316 | 407 |
| new | 0 | 28 | 68 | 108 | 188 | 263 | 369 | 451 | 597 |

Small attacks are priced far lower, large attacks far higher. A linear fit
cannot express that reshaping, which is the case for using this instrument
rather than Texel — independent of whether the result ships.

*Two dead knobs found.* `ks_unit[ROOK]` and `ks_unit[QUEEN]` were both **0** —
rook and queen attacks contributed nothing to the attack-unit count. Both fitted
to 2. That is a standing coverage gap in king safety, not a tuning artifact.

*The canary failure, and its real cause.* Baking all 34 changed knobs fails
`test_eval`'s "mate-drive drives the pawnless defender to the edge": the edge
preference collapses from **29cp to 4cp** against a `>20` threshold. Bisection
isolated it to `safety_table` alone — reverting only the table restores 29cp and
passes 77/77 with all eleven scalar changes in place.

The cause is **corpus coverage, not the fitter**. The training corpus is
quiet-filtered self-play WDL and carries essentially no forced-mate positions,
so the objective has no signal for mating behaviour and freely halved the
low-end table. In the canary position the mate-drive's own designed contribution
is only ~15cp (`5 × lk_center`; the king-distance term cancels between the two
FENs) — the remaining ~14cp was king safety incidentally topping it up. **The
canary has therefore been passing partly by accident**, and the fit removed the
accidental part. That is worth stating plainly: a threshold met by a mechanism
plus an unrelated contribution is not a measurement of the mechanism.

*Disposition.* Variant A shipped — the eleven scalar knobs, `safety_table`
reverted. The table reshape is deferred to **5.9.12**, whose corpus (5.9.11)
must include mating and near-mating material so the objective has signal there.

*Cost.* Bench **15,655,764 → 18,228,447**, +16%. Paired depth at equal nodes is
**−0.196 ply at 1M** against the accepted head, statistically unchanged from
5.9.4 alone (−0.168). The king-safety scalars cost nothing measurable, and the
bench rise is once again a poor proxy — see BAS-E09.

*Score scale moved at 5.9.4, and this is the first record of it.* `K` fitted
**1.41868** at the start of 5.9.4 and **1.49757** at the start of 5.9.5, both on
the same holdout. Since each is fitted before its run, that is a ~5.6%
**compression of the eval scale caused by the 5.9.4 refit**. Our futility,
razoring and delta-pruning margins are centipawn constants calibrated to the old
scale and are now effectively ~5.6% more aggressive; BAS-M05 records the resign
threshold as engine-scale dependent for the same reason. Not repaired here — it
is registered as a known consequence carried into 5.9.6's verdict, and it is
exactly the failure mode 5.9.12's mandatory score-scale audit exists to catch.
