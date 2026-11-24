# BAS-E62

**BAS-E62 — C.2.10: the family screen both ways on the refitted head**

- Date / owner / category: registered 2026-11-24, before any read;
  agent; measurement, static layer.
- Tool: `tools/diag/family_screen.py`, `collect` then `screen`, with the
  head's tuner.
- Inputs: corpus A's validation split
  (`tools/texel/data/corpusA/rows/validation.csv`), 60,000 positions;
  donors `donor-sf9587-stock.exe` and `donor-sf16-stock.exe`.
- Research question: per family and phase band, what does the donor's
  family add to Basilisk's whole score, and what does Basilisk's add to
  the donor's? It is the tiebreak for what games cannot resolve, and one
  half of C-110's `NO_CHANGE` test.
- Registration, its place in the decision rule and predictions:
  `analysis/c210_family_order_2026-11-24.md` §2.3, §4 and §5.
- Stop rule: one read on the registered inputs. A refused join or a
  donor table that does not sum is reported, never worked around.
- Evidence: `tools/results/c210-screen/`.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-24

Gains in 1e-4 nats, donor into Basilisk, at phase 18–24:
- space still has the largest gain: 0.6;
- space's gain is at least 25 (BAS-E58 read 47.0 on the 1.11.0 head):
  0.6.

Over all rows:
- the passers' gain includes zero: 0.7.

Basilisk into the donor:
- at least one family's gain excludes zero: 0.8.

A static gain is not Elo. The games of BAS-O14 decide the order.

#### RESULT — append after exposure

- **Read 2026-11-24** (`tools/results/c210-screen/report.json`; the
  head's tuner `C4FC9F38…`, collect seed 20261124, 300 bootstrap
  repetitions). 56,311 regular rows; 3,436 override rows and 3,614
  specialised-endgame rows are set aside.
- **Donor into Basilisk,** `sf_16` (`9587eeeb` agrees within 2
  everywhere), gain in 1e-4 nats with its 95% interval:

  | Family | All rows | Phase 0–9 | Phase 10–17 | Phase 18–24 |
  |---|---|---|---|---|
  | King safety | **12.1** [8.7, 15.5] | 2.9 [0.9, 4.7] | **29.4** [17.5, 38.8] | **16.2** [8.5, 24.1] |
  | Passed | 5.4 [3.4, 7.6] | **14.0** [8.1, 19.9] | 0.8 [−1.0, 2.8] | 2.4 [−0.6, 5.9] |
  | Winnable | 2.2 [1.1, 3.4] | **10.0** [6.9, 13.2] | 2.5 [−0.6, 5.5] | 1.4 [−0.5, 3.3] |
  | Threats | 1.0 [−0.1, 2.2] | 0.1 [−0.3, 0.6] | 2.0 [−1.0, 4.4] | 4.7 [0.5, 8.3] |
  | Space | 0.5 [−0.3, 1.2] | — | 0.0 [−0.3, 0.4] | 3.4 [−0.4, 7.8] |
  | Mobility | 0.8 [−0.1, 1.7] | 0.5 [−0.3, 1.5] | 3.0 [−0.0, 6.5] | 2.1 [−0.8, 5.3] |
  | Material | 2.3 [0.5, 4.1] | 0.3 [−0.4, 1.1] | 3.3 [−0.8, 6.9] | 1.5 [−1.2, 4.2] |
  | Bishops | 1.4 [0.1, 2.6] | 0.8 [−0.5, 2.0] | 2.8 [−0.9, 5.8] | 2.0 [−0.5, 4.7] |

  Pawns, knights, rooks, queens and the imbalance include zero in every
  band.
- **Basilisk into the donor,** `sf_16`, over all rows: material 20.0
  [14.9, 24.4], passed 19.9 [16.4, 23.8], tempo 14.3 [10.5, 18.1], the
  tables 5.0, king 4.5, pawns 3.8, threats 1.5, all excluding zero.
  Against `9587eeeb`, which has a tempo, Basilisk's tempo adds 2.2.
- **What it says:**
  - **The static case for space and mobility is gone on the refitted
    head.** At phase 18–24 the donor's space adds 3.4 and its mobility
    2.1, both including zero, against BAS-E58's 47.0 and 29.6.
  - **King safety is the largest donor gain in every band but the
    endgame,** and at phase 18–24 its interval is clear of space's and
    mobility's. It touches threats': clear by 0.2 against `sf_16`,
    overlapping by 0.3 against `9587eeeb`.
  - In the endgame band the passers and the winnable term lead, as in
    C.0.
  - Basilisk's passers, material and tables carry what the donor's
    whole score lacks: the union's case, again.
- **Not yet shown: that the refit is the cause.** BAS-E58 was a
  prototype on another sample. The control is this tool on this sample
  with the header before the refit; it is run before the comparison
  with BAS-E58 is relied on.
- Disposition: COMPLETE, a static read. It is the tiebreak and one half
  of C-110's test (the packet's §4); BAS-O14's games decide the order.

#### PREDICTION CALIBRATION — preserve the original prediction above

- Space the largest gain at phase 18–24 (0.6): **miss.** King safety
  leads at 16.2; space is 3.4.
- Space's gain at least 25 (0.6): **miss,** 3.4 with zero inside.
- The passers' gain over all rows includes zero (0.7): **miss,** 5.4
  [3.4, 7.6].
- A Basilisk family's gain into the donor excludes zero (0.8): hit.
- What the model had wrong: it carried C.0's family residuals across the
  refit. A whole-surface fit moves the tables and the mobility values,
  and a screen of one family against the whole score sees what they now
  hold. C.0 named this as its first competing reading.
- Conditional lesson: a static ranking is of one header. It is read
  again after every refit before it orders anything.

#### CONTROL — 2026-11-24, after the read (not registered)

Asked: is the fall from BAS-E58's 47.0 the refit's doing? Evidence and
scripts: `tools/results/c210-screen-control/`.

- **The registered control: the header before the refit, same rows.**
  The tuner is built from the pre-refit header; `sample.csv` and both
  donors' arrays are identical to the read's.
  - Expected before it ran: space at least 25 and mobility at least 15
    at phase 18–24, 0.7 each. **Both missed:** space 5.7 [0.9, 11.2],
    mobility 7.1 [1.8, 13.2].
  - So the refit is not the cause. On these rows space never read.
- **Not the form** (`offset_check.py`). BAS-E58's final form also fitted
  Basilisk's coefficient. With a fixed offset instead, space reads 0.7
  on these rows.
- **Not the code** (`c0_sample_by_ply.py`). The same script gives this
  tool's figures on corpus A, and on C.0's rows it gives 50.8, 29.6 and
  11.8, BAS-E58's `sf_16` figures exactly.
- **Not White's start** (`intercept_check.py`). With an intercept in
  base and model, space still adds 46.0 on C.0's rows.
- **Not the first moves.** By ply on C.0's rows, space adds 53.1 at
  9–20, 61.4 at 21–40 and 31.4 at 41–60.
- **It is the sample.** C.0's rows are UHO gate games at `3+0.03`.
  Corpus A is self-play at 8,000 nodes from a phase-balanced start pool.
- **C.0's rows with the head's evaluation** (`c0_sample_head.py`;
  241,887 regular rows, 25,949 games; the old header's scores reproduce
  C.0's on all 244,883 regular rows). Donor `sf_16` into Basilisk, the
  header before the refit → the head, with the head's 95% interval:

  | Family | All rows | Phase 0–9 | Phase 10–17 | Phase 18–24 |
  |---|---|---|---|---|
  | Space | 9.6 → 12.1 [9.8, 14.6] | — | 0.5 → 0.7 | 50.8 → **40.4** [32.7, 49.1] |
  | Mobility | 3.5 → 4.5 [3.3, 5.9] | 0.5 → 0.6 | 3.9 → 4.5 | 29.6 → **13.4** [9.7, 17.4] |
  | King safety | 5.2 → 7.7 [6.2, 9.3] | 0.8 → 1.1 | 15.9 → **19.4** [14.2, 24.6] | 11.8 → 10.8 [7.3, 14.9] |
  | Winnable | 2.7 → 9.9 [8.5, 11.5] | 14.8 → **20.3** [17.7, 23.2] | 0.0 → 0.7 | 3.1 → 1.1 |
  | Pawns | 6.8 → 3.0 [1.9, 4.1] | 11.8 → 4.5 | 7.8 → 5.4 | 10.0 → 2.3 [0.8, 4.1] |
  | Passed | 0.2 → 2.3 [1.5, 3.2] | 1.5 → 5.8 | 1.6 → 0.4 | 1.3 → 0.7 |
  | Threats | 1.4 → 0.1 | 0.2 → 0.1 | 1.8 → 0.9 | 4.1 → 0.1 |
  | Imbalance | 2.6 → 4.5 [2.9, 5.8] | 0.8 → 2.6 | 6.7 → 9.9 [5.7, 14.2] | 3.3 → 5.4 |

  The head's own loss on these rows is 48.0e-4 below the old header's.
- **Corrections to the RESULT above:**
  - "The static case for space and mobility is gone" holds on corpus A
    only. On gate games the donor's space still adds 40.4 at phase
    18–24 after the refit, and it leads there, clear of mobility and of
    king safety.
  - The calibration's cause was wrong. The model did not fail by
    carrying C.0's residuals across the refit; the registered rows were
    not C.0's kind of rows. The three misses stand as scored.
- **Findings:**
  - A static ranking belongs to its sample as well as to its header.
    The two samples rank the first family differently.
  - Corpus A carries almost none of what the donor's space predicts in
    gate games. Whether the starts or the search depth make the
    difference is not separated. A space term fitted on corpus A alone
    may be fitted to nothing; that is C.3.1's first question.
