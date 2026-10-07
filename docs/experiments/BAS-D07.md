# BAS-D07

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D07 — deep-segment branching; corrects BAS-D05 and BAS-O04** (16 suite
positions, depths 11–19, Hash 64 every arm, 2026-08-25). Measuring only to depth
11 was a blind spot: games reach 15–25 plies and the ratio was still moving.

| depth | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Basilisk / oracle | 1.99× | 1.78× | 2.06× | 2.06× | 1.75× | 1.95× | 1.92× | 1.86× | 1.80× |

| segment | Basilisk | oracle |
|---|---:|---:|
| b(4–11) | 1.692 | 1.894 |
| **b(11–19)** | **1.570** | **1.590** |
| b(11–19) per-position median | 1.548 | 1.558 |

**Correction 1 — deep branching is EQUAL, not better for us.** BAS-D05
concluded "our per-ply growth is better than the reference's" from the 4–11
segment alone. Over 11–19, which is where games actually run, the two are
indistinguishable. The shallow advantage was a shallow-band artifact and does
not describe our asymptotic behaviour. **The ratio therefore plateaus at
~1.8–2.0× and never closes; the crossover projected from BAS-D05 does not
happen.**

**Correction 2 — BAS-O04's 12.07-ply gap is an outlier artifact.** It was a mean
over a distribution containing forced mates that ran to depth 100 on our side
and 245 on the oracle's; 10 of 105 positions hit depth ≥100. The **median gap is
4.00 plies** (Basilisk 13, oracle 17), not 12.07. Every prior statement of "12
plies shallower at equal nodes" overstates by roughly three times and is
corrected here. The paired-delta comparisons used elsewhere were never affected,
because pairing is robust to exactly this; only the absolute means were.

*Open and stated as such.* A flat ~1.9× node cost at b≈1.55 predicts a gap of
about **1.5 plies**, but the measured median gap is **4.0**. The two measures do
not reconcile and roughly 2.5 plies is unexplained. Candidate causes not yet
separated: the 16-position summed branching sample is dominated by its most
expensive positions and may not represent the 107-position median; and the two
engines may differ in when an iteration is reported complete. This is recorded
as an open discrepancy rather than resolved by choosing whichever number suits.

*Consequence.* The real deficit is smaller than recorded and is a **persistent
multiplier**, not a shallow-band transient. Since deep branching is equal, any
node saving achieved by depth ~11 propagates unchanged through the playing
range — which keeps the shallow band worth attacking, but for a different reason
than BAS-D06 gave.
