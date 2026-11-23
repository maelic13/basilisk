# BAS-O13

**BAS-O13 — C.2.8's matched whole-surface fits of corpora A and B from Stockfish 16's tables**

- Date / owner / category: registered 2026-11-23 (C.2.8.1), before any fit;
  agent; measurement, fit layer. The maintainer runs the fits (C.2.8.3).
- Question: what do `fit.py`'s four stages make of the surface from
  C.2.5's seed, on corpus A and on corpus B at the same K? And do the two
  fits differ beyond what the instrument resolves?
- Inputs:
  - Corpus A (`tools/texel/data/corpusA/rows`, train `32CD3D63…`) and
    corpus B (`corpusB/rows`, train `B456BCE4…`);
  - the surface `surface_v1.tsv`;
  - the seed `seed_sf16.py --scale 0.35`.
- Schedule and anchors: `analysis/c281_fit_registration_2026-11-23.md`
  §2. K is fitted once on A's validation at the seed and frozen for both
  arms.
- Reads (C.2.8.3's fit report):
  - realized losses (`--loss --init`) by phase band, on validation and on
    unread train rows;
  - each stage's stop line;
  - per-table movement against the seed, the endgame-passer group and
    E6's signs;
  - king-safety and king-PST movement (C-56);
  - `bench 13`; the deterministic floors;
  - the magnitude reads: the lazy-checkpoint share, the median score
    change by band, and marginal piece values by phase;
  - for B against A, BAS-E60's condition.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-23

- K at the seed lies in [1.90, 2.10]: 0.8.
- **The A fit's realized validation loss** lies in [0.0958, 0.0964] at
  80%, point 0.0961. It improves on the header's 0.09913 by at least 2.5%:
  0.85.
- Every stage stops by its convergence rule, with no exit 3: 0.85.
- **Magnitude:**
  - the knight's, bishop's and rook's endgame (phase 0–7) marginal values
    rise 10% to 40% over the header's: 0.8;
  - the lazy-checkpoint share of validation rows lies in 12% to 15%: 0.75.
- **`bench 13`:** it moves by at most 10% from 4,777,412 (0.7), and grows
  by more than 5% (0.35).
- **Floors and vetoes:** the deterministic floors pass (0.85); C.2.7's
  bracket veto passes (0.6, since KNN-KP swings by 6 positions between
  nearby budgets).
- **B against A:** BAS-E60's condition fails, and B is null at fit level,
  0.75. B changes 0.28% of train labels, symmetrically (C.2.4).
- Falsifiers:
  - a loss outside [0.0950, 0.0970], which means the pipeline or the seed
    is not what is registered;
  - the endgame values not rising, which means Correction 2's mechanism
    is wrong.

**Stop rule:** one run per corpus on the registered inputs. A run that
exits 3 or fails a guard is reported, and its schedule is amended here
before a re-run, never quietly re-run.

#### RESULT — append after exposure

- **Run 1, 2026-11-23** (`tools/results/c283/`, source `bdaa95a`; the
  report is `analysis/c283_fit_report_2026-11-23.md`). Both fits
  completed, and every stage stopped by its own rule.
  - K 1.960321.
  - Realized validation loss: A **0.09648236** and B 0.09650363, against
    the header's 0.09914295 at the same K (−2.67%).
  - Lazy share 12.60%.
  - Endgame knight, bishop and rook values +25%, +20% and +21%.
  - `bench 13` A 3,392,955 (−29.0%), B 3,868,741.
  - The floors pass.
  - BAS-E60's condition fails: on B's labels, B's fit is worse than A's
    by 2.0e-5.
- **Not baked: a protocol defect.** `fit.py`'s surface froze 242 slots
  under 200 rows of support counted on a 30,000-row sample. Scaled to the
  full train split, only 4 of them fall under 200. 204 of the 242 are PST
  entries. They stayed at the seed while their tables re-levelled, up to
  −215 cp. This stranded steps such as a 112 cp jump between a pawn on
  f6 and on f7. BAS-O12, the source of the loss interval, had no such
  freeze and reached 0.09622, so the interval's miss is this defect.
- **Score:**
  - hits: K, the 2.5% gain, no exit 3, the endgame values, the lazy
    share, the floors, BAS-E60's condition;
  - misses: the loss interval and `bench` within 10%;
  - the bracket veto is not yet read;
  - no falsifier triggered.
- **By the stop rule** the schedule is amended in the registration before
  a re-run. The report's §3 gives the options.

#### AMENDMENT 1 — 2026-11-23, after run 1 and before run 2

The registration's §6 (`analysis/c281_fit_registration_2026-11-23.md`).
The maintainer accepted it in full the same day.
- **The support rule:** support is counted on every row the linear
  stages read, at 200 rows, and the rank is read on 300,000 rows. A fit
  stops when a support-frozen slot sits in an unanchored table that
  still has a free member.
- **By count** (`tools/results/c283/amend1/`): on the split's 3,547,892
  rows, two slots are under 200, `MobQMg[0]` and `MobQEg[0]` at 10 rows.
  The RESULT's "only 4" was a scaled estimate. So 1,023 linear slots are
  free in run 2, against 783 in run 1 and 1,025 in BAS-O12.
- **Run 2:** one run per corpus from the tool change's commit, A then B
  with A's K. Everything else is as registered.
- **Controls,** not predictions: the seed's hash, K 1.960321, the seed's
  loss 0.10122069, the seed's bench 4,777,412, and one derived surface
  for both arms.

**The frozen predictions stand,** and run 1's score stays.
- The loss interval [0.0958, 0.0964] is not moved. It came from BAS-O12's
  full surface, and run 2 is its first fair test.
- The other items were exposed by run 1 on nearly the same fit. They are
  read again and earn no calibration credit.

**Predictions for run 2, made after run 1's exposure:**
- A's realized loss is at most 0.09632, point 0.0962: 0.8. That recovers
  at least 1.6e-4 of run 1's 2.6e-4 gap to BAS-O12.
- The levels return toward BAS-O12's: the eg rook table's mean shift
  from the seed, over its 64 entries, exceeds +100 cp (run 1 +52 on its
  free entries, BAS-O12 +180): 0.65.
- `MobQMg[1]` and `MobQEg[1]` end at 0, on the held entry: 0.75.
- `bench 13` falls by 10% to 40% from 4,777,412: 0.6. It grows by more
  than 5%: 0.1. This replaces "within 10%", whose model was wrong: one
  count is not a measure of the tree.
- BAS-E60's condition fails again: 0.9. Its label-specific part is under
  1e-5: 0.9.
- **Falsifier of the amendment:** a loss above 0.09640. The freed slots
  would then explain under a third of the gap, and the gap is explained
  before any bake. The candidates are the 0.35 seed, the king-safety
  stage before the linear one, and K.
- The registered falsifiers stand.

#### RESULT — run 2, 2026-11-23 (under Amendment 1)

- Report: `analysis/c283_run2_report_2026-11-23.md`; records in
  `tools/results/c283/A2/` and `B2/` (source `fef59ab`).
- **The controls reproduce:**
  - the seed `C73430E6…`;
  - K 1.960321;
  - the seed's losses 0.10122069 (A) and 0.10108955 (B);
  - the seed's bench 4,777,412;
  - one derived surface, `E37D5983…`, with only `MobQMg[0]` and
    `MobQEg[0]` frozen and 1,023 linear slots free.
- Every stage stopped by its own rule.
- **A's realized validation loss is 0.09612177,** 3.04% below the
  header's. It is 3.6e-4 below run 1 and 1.0e-4 below BAS-O12's H arm.
- **Levels:** the eg rook +185, the eg queen +166, the eg knight +94, the
  mg pawn −137, in line with BAS-O12's free fits.
- **Endgame values:** knight +38%, bishop +29%, rook +30%. The lazy share
  is 13.89%.
- **`bench 13`:** A2 3,103,977 (−35.0%); B2 3,825,925.
- **The floors:** A2's baked build fails KR-K at 60,000 nodes (23 of 24),
  and at 40,000 and 50,000. The cause is a coupling, which Amendment 2
  repaired on the head (`cd4e453`, BAS-D24). Over the repair, A2 converts
  KQ-K and KR-K 24 of 24 at every budget from 40,000 to 120,000.
- **Score:**
  - frozen items: the loss interval hits on its first fair test; the
    floors miss before the repair;
  - Amendment 1's predictions: all seven hit;
  - the falsifiers did not trigger;
  - the bracket veto is read with BAS-E59.
