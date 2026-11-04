# BAS-S46

**BAS-S46 — B.3.6 step 1: how much of what the search reads from the colourless histories the other colour trained**

- **Registration.** Created 2026-11-04 at B.3.6's opening, before the
  read; agent-run, zero-game; search, instrument layer. From PLAN B.3.6 and
  its S2 card (B.3's inputs).
- **Subject.** The B.3 head with Cluster 2 on (`bench 13` 5,369,042). Four
  tables index a move by piece type with no colour:
  - the capture history (`history_core.h`, `CaptureHistory`);
  - the continuation history (`ContinuationHistory`, its rows and leaves);
  - the pawn history (`PawnHistoryBucket`);
  - the continuation correction (`ContinuationCorrection`).
  
  White's NxB on f6 and Black's share one entry, as do a reply after
  White's Nf3 and one after Black's. The donor indexes by `Piece`.
- **Research question.** When the search reads one of these entries for
  the side to move, how much of the training behind it came from the
  other colour?
- **Mechanism, if it matters.** Ordering, statScore, continuation-history
  pruning, LMR depth, capture futility and the correction read the other
  side's experience as their own. Where the colours' patterns differ, the
  entry is a blend that fits neither.
- **Competing explanation.** The colours rarely meet in one entry, because
  the position fixes which squares each side uses and the pawn bucket
  separates structures. In that case the shared index costs nothing, and
  the donor's form only doubles the memory.

#### INSTRUMENT

- **The counter** is in diagnostic builds only (`src/history_shadow.h`;
  every flavour's `bench 13` is unchanged, ON 5,369,042 and OFF
  11,880,564).
  - A shadow cell per table entry counts the updates White made and the
    updates Black made, at every write site.
  - At every read site, for the side to move, it records:
    - a read of a trained entry;
    - whether the other colour alone trained it, or both did;
    - the other colour's share of the entry's updates.
  - The read sites: the picker's capture and quiet scores, quiescence's
    capture order, `cont_hist_score`, capture futility,
    continuation-history pruning, the LMR statScore, and the correction.
  - The cells clear with the tables; the read totals are per search.
  - `test_history_core` checks the accounting on a planted sequence and
    that the cell indices span each table once.
- **The read.** `python tools/diag/run_suite.py --engine
  build/b3-on-diag/basilisk.exe --depth 14 --hash 64` on `suite_v2` (106
  positions, Threads 1), the Diag build of the B.3 head. This is B.3.0's
  counter read, with the four `xc_*` counter groups summed over the
  suite.
- **The measures, per table:**
  - the mean other-colour share, `xc_*_other_ppm / xc_*_reads / 10^6`;
  - the share of reads trained by the other colour alone,
    `xc_*_other_only / xc_*_reads`;
  - the share trained by both, `xc_*_mixed / xc_*_reads`.
- **Limitation, stated before the read:** each suite position starts from
  clear tables. Within a game the tables carry over from move to move,
  and the colours' training accumulates further, so this read is a lower
  bound on what a game sees.

#### RULE (frozen before the read)

- **NO_CHANGE**, closing B.3.6 without games, if in every one of the four
  tables both measures are under 2%:
  - the mean other-colour share;
  - the share of reads trained by the other colour alone.
- **Otherwise** B.3.6 proceeds to its step 2, the colour-indexed build (the
  donor's index by `Piece`). It is registered separately before it is
  built, read on first-move cutoffs, mean cutoff index, `bench` and
  memory, and gated by SPRT `[0,3]`.

#### PRE-REGISTERED PREDICTION — freeze before exposure

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| X1 | The rule sends B.3.6 to step 2: some table reads at least 2% | 90% | every table under 2% on both measures |
| X2 | The continuation and pawn histories read a mean other-colour share of at least 20% | 75% | either under 20% |
| X3 | The capture history reads at least 20% | 65% | under 20% |
| X4 | The continuation correction reads at least 20% | 55% | under 20% |

#### RESULT — append after exposure
