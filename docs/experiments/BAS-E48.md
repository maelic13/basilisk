# BAS-E48

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E48 - 6.3.a: no king-to-passed-pawn approach signature exists in
Basilisk's own failures** (2026-09-03). Frozen as
`analysis/passed_pawn_king_approach_v1.md`.

For every clean-win root the strong side's most advanced passer was located and
the geometry a king-approach term would act on was measured: `own_dist`,
`foe_dist`, and `race = own_dist - foe_dist`, negative when the strong king is
closer. Aggregated over 288 roots at 60,000 nodes there is a weak apparent
trend, 82.2% conversion at king distance 0-1 falling to 56.8% at distance 5 --
but it is non-monotone, recovering to 70.0% at 6-7.

**Conditioning on family destroys it.** Within-family comparison of race < 0
against race > 0 gives +25.0, +20.0, +9.2, +9.1, 0.0, -3.5, -7.1, -11.9, -27.1
and -35.7 percentage points across the ten families with enough roots. Four
favour the closer king, six the further one; mean -2.2pp, median -1.8pp. The
aggregate trend was family composition -- the families whose roots place the
king far from the passer are also the ones Basilisk is worst at, for unrelated
reasons. At 200,000 nodes the trend is absent even in aggregate.

The deficit is real but material-specific: KBP-K 15 positions behind, KNN-KP
13, KQ-KR 13, KBN-K 12 (pre-6.1 head, since addressed), KQ-KRP 10, KBPP-KB 9.
Those are now distributed across 6.8.b/6.8.c and 6.10.b/6.10.c. A general
king-approach term touches none of
them, and adding one anyway would be importing a reference constant, which is
precisely what 6.3.a forbids.

*Retry trigger:* a king-approach signature that survives conditioning on family
-- consistent sign across families and a mean within-family delta beyond noise.
Aggregate correlation over a mixed family set is not sufficient, and is the
specific error this entry exists to prevent.
