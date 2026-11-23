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
