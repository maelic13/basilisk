# BAS-X11

Two different experiments were registered under this ID before the
ledger was split; both are kept, in ledger order.

<!-- part 1 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-X11 - current standing, 12,000-game Colosseum round robin** (2026-09-04,
maintainer-run). Conditions: 3s+30ms, two games per pair, parallel 10, no draw
or resign adjudication, UHO_Lichess_4852_v1 openings, **tablebases off**.

**Correction (same day): the Elo column in that run was NOT computed.** Ratings
were frozen and maintainer-estimated; only Perf was calculated. The figures
below are therefore Perf, and the estimated-Elo column is omitted rather than
quoted. The head-to-head values further down are unaffected, being derived
directly from W-D-L.

| engine | Perf | avg nps | avg depth |
|---|---:|---:|---:|
| Basilisk 1.10.0dev | 2959 | 2.3M | 14.2 |
| Basilisk 1.9.3 | 2949 | 2.2M | 14.1 |
| Rarog 2.4.0dev | 2934 | 2.0M | 14.6 |
| Rarog 2.3.2 | 2873 | 2.1M | 14.7 |

Perf in a four-engine pool is itself a weak statistic: it is a rating implied by
the score against opponents whose own ratings are unsettled, so it should be
read as ordering within this pool and not as a strength estimate.

Head-to-head over 2,000 games each: 1.10.0dev over 1.9.3 **+16.0 +/- 15.2**;
1.10.0dev over Rarog 2.4.0dev +28.4 +/- 15.3; 1.9.3 over Rarog 2.4.0dev
+32.1 +/- 15.3.

*Read conservatively.* The dev-over-release margin barely excludes zero at
2,000 games, so the accumulated Phase 5-6 work is probably positive and not
firmly established by this run. Note also that 1.9.3 scores nominally BETTER
against Rarog 2.4.0dev than 1.10.0dev does, by 3.7 Elo -- comfortably inside
the interval, and a caution against reading the pool table as a clean ordering.

*Three things it does establish.* First, the strength metric is TB-OFF, which
resolves the Phase-6 scope question: endgame failures the truth harness finds are
not artifacts of a TB-blind instrument, because the games are equally blind.
Second, **510 of 12,000 games (4.25%) end by the fifty-move rule**, the
game-level signature of conversion failure, bounding the Phase 6 prize; the
overall drawn share is 33.0%. Third, **Rarog reaches greater depth on fewer
nodes** -- 14.6 ply at 2.0M nps against Basilisk's 14.2 at 2.3M -- which
corroborates BAS-X09's search-coordination deficit as still present rather than
closed.

<!-- part 2 of 2: from docs/EXPERIMENTS.md, 8b. Cross-engine evidence imported from Manta -->

| Field | Value |
|---|---|
| ID | BAS-X11 |
| Manta evidence | `MAN-E05` (endgame conversion grading, −16.32 Elo) and `MAN-E07` (nonlinear material imbalance, −7.00) — two faithful reference-family evaluation concepts with hand-reasoned coefficients, about **−23 Elo between them**. Manta's conclusion: adopting reference concepts with reasoned constants "reproduces its structure without its calibration". |
| Basilisk implication | A direct warning against adding reference terms with hand-set constants one by one. Manta's answer was to land dependency-complete structure on deterministic evidence, then promote it through one whole-surface fit and one gate; current Plan 8 adopts that discipline without importing its terms or values. |
| Coverage | 8.0, 8.7–8.9 |
