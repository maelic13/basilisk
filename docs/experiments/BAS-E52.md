# BAS-E52

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E52 - 6.4.a: the pruning-margin hypothesis is refuted, and refuted
backwards** (2026-09-04). Three arms differing only in the starting fifty-move
counter -- 0, 25, 50 -- on the frozen cohort's four bare-king mate families at
60,000 nodes, same binary. Syzygy ignores the halfmove clock, so theory labels
are unaffected; what changes is how much `damp_rule50` shrinks the engine's own
evaluation.

The prediction was that KQ-K, KR-K and KBB-K would degrade most, because their
`kxk_score` king and corner steps fall to 224 and 187 at clock 50, below the
243 razoring margin, while `kbnk_score`'s diagonal step is still 1422.

| metric | kxk families | KBN-K |
|---|---|---|
| DTZ progress, clock 0 -> 50 | 0.5962 -> 0.5946 (**-0.0016**) | 0.6357 -> 0.4900 (**-0.1457**) |
| conversion, clock 0 -> 50 | 100% -> 88.9% | 85.7% -> 28.6% |

**The exact opposite.** The families whose steps cross the margin show no
measurable move-quality degradation at all; the family whose gradient never
approaches the margin degrades by far the most. Whatever damping does to
endgame play, it is not the pruning-margin erosion the static audit
hypothesised, and the code comment's concern -- that a weight must clear 243 to
survive pruning -- appears not to bind in practice.

*What the numbers do and do not support.* DTZ progress is a per-move rate, so
each family serves as its own control across clocks and the comparison is
clean. **The conversion row is not**: the eligibility cut equalises feasibility,
not slack. At budget 50, KQ-K roots have a median DTZ of 10 and therefore 40
halfmoves of slack, while KBN-K's median is 50 and its slack is zero, so any
imperfection fails. Cross-family conversion differences are largely that, and
should not be read as a damping effect. A slack-matched comparison is not
possible on this cohort because KBN-K's DTZ distribution barely overlaps the
others'.

*Leading explanation, not established.* The within-family DTZ-progress collapse
in KBN-K is consistent with BAS-E35's observation that the engine "is not
blundering... it is running out of clock and then behaving indifferently,
correctly, in a position whose result is already settled". Damping drives the
evaluation toward zero as the counter climbs, so the engine stops distinguishing
winning continuations precisely where the technique is longest. That is the
damping curve working as designed, not a defect, and it bites hardest in the
family with the least slack.

*Disposition.* No change to `damp_rule50` is recommended. The static audit's
other finding stands independently and is not touched by this result: the
release build's compiled KBNK default is validated by nothing, because
`set_kbnk_drive_weights` is `#ifdef BASILISK_TUNE`.

*Instrument note.* The probe added `--start-halfmove-clock` to
`endgame_truth.py`, verified inert at clock 0, and a guard in the summarizer
that refuses to report conversion for a family with fewer than ten eligible
roots -- added after a first design at clocks 0/50/80 left KBN-K with one.
