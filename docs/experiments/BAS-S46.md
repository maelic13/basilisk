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

- **Read 2026-11-04** (`tools/results/b36-20261104/xc_suite_d14.json`
  `80a8bae8…`):
  - The build is `build/b3-on-diag/basilisk.exe` (`a3615da0…`) at
    `183e0bd`, `bench 13` 5,369,042.
  - 106 positions at depth 14, Hash 64, each from clear tables. Every
    position reported, and the suite's accounting identities hold.

  | Table | Reads | Mean other-colour share | Other colour alone | Both | Own colour alone | Per-position median (range) |
  |---|---:|---:|---:|---:|---:|---|
  | Continuation history | 205,036,663 | **16.3%** | 8.8% | 18.9% | 72.3% | 11.2% (0.0–25.8%) |
  | Pawn history | 31,028,486 | **11.3%** | 6.2% | 13.1% | 80.7% | 8.1% (0.0–29.9%) |
  | Capture history | 18,524,983 | **7.8%** | 1.2% | 23.8% | 75.0% | 6.6% (0.0–38.6%) |
  | Continuation correction | 16,269,653 | **3.1%** | 1.8% | 3.5% | 94.7% | 3.5% (0.0–13.1%) |

- **The rule: every table reads above 2% on the mean other-colour share,
  so B.3.6 proceeds to step 2**, the colour-indexed build, registered
  separately before it is built.
  - The continuation and pawn histories are also above 2% on reads
    trained by the other colour alone.
  - The competing explanation, that the colours rarely meet in one
    entry, is refuted: about one read in six of the continuation history
    draws on the other side's training, and one in eleven draws on
    nothing else.
  - This is a lower bound, because each position starts from clear
    tables and a game carries them over from move to move.
- **Predictions:**
  - **X1 hit:** the read sends the leaf to step 2.
  - **X2 missed:** continuation 16.3% and pawn 11.3%, against at least 20%.
  - **X3 missed:** capture 7.8%.
  - **X4 missed:** correction 3.1%.
  - Every magnitude was overestimated. The direction held, and the
    ordering (continuation > pawn > capture > correction) matches how
    freely each table's index lets the colours meet.
- Disposition: step 1 done; step 2 next.

#### STEP 2 REGISTRATION (2026-11-04, before any step-2 code or game)

- **The maintainer's decision (2026-11-04):** step 2 runs now; it is not
  deferred to Phase H.
- **The form, after the step-2 investigation:**
  - A continuation-history row is read at 1 to 6 plies back, both
    parities. A colourless row then mixes "White moved Nf3, and two plies
    later White plays X" with "Black moved Nf3, and one ply later White
    plays X". Colour on the leaf alone does not separate them; the row
    needs it too. This is the donor's `[Piece][to]` row and `[Piece][to]`
    leaf.
  - The pawn history takes colour on the moving piece, and the capture
    history on the attacker; the victim's colour is implied. Both are the
    donor's form.
  - The continuation correction is read only at 2 and 4 plies back, so
    its row's colour is always the side to move. Colour on the row alone
    separates it. The donor colours the leaf as well, but half of those
    cells could never be reached here.
  - The coloured index is `colour × 7 + type`, 14 slots, where the donor
    uses 16.
- **Build:** a CMake option, `COLOUR_HISTORY`, defaulting OFF, defines
  `BASILISK_COLOUR_HISTORY`.
  - OFF keeps every table and `bench 13` exact (ON arm 5,369,042, OFF arm
    11,880,564).
  - Memory with it ON: continuation 1.6 → 6.4 MB, pawn 7.3 → 14.7 MB,
    correction 0.4 → 0.8 MB, capture 6 → 12 KB.
- **Planted check:** step 1's counter must read 0% other-colour share in
  every table on the coloured Diag build. A non-zero read is an
  implementation defect.
- **Reads before the gate, recorded, not deciding:** on `suite_v2` at
  depth 14, coloured against colourless (both with Cluster 2 on):
  - the first-move cutoff rate, `fail_high_first / fail_highs`;
  - the mean cutoff index, `fail_high_index_sum / fail_highs`;
  - `bench 13`;
  - the memory above.
- **Gate:** SPRT `[0,3]` normalized, at `3+0.03`, 1T, Hash 64, UHO paired,
  no adjudication, tablebases off. Two clean final-PGO Release builds of
  one revision, colour-indexed against colourless, both with Cluster 2
  on. Seed 46001.
  - **Cap 16,000 pairs.** The drift model at an expected +4 Elo, about
    +6.4 nElo, with σ ≈ 245 per pair (BAS-S43), puts the expected stop
    near 12,000 pairs.
  - The speed cost of the larger tables is inside the gate. A pooled NPS
    read is the maintainer's to schedule on an idle host.
  - **H1:** `COLOUR_HISTORY` defaults ON, and the fingerprint record moves.
  - **H0:** the colourless form stays, and it goes to B.3's departures
    table with this reading.
  - A cap without a decision goes to the maintainer.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| X5 | The planted check reads 0% on all four tables | 95% | any table above 0% |
| X6 | The colour-indexed build raises the first-move cutoff rate on the suite | 60% | equal or lower |
| X7 | The SPRT accepts H1 | 55% | H0 |

#### STEP 2 RESULT BEFORE THE GATE (2026-11-04)

- **Built:** `70e978b`, the `COLOUR_HISTORY` switch, default OFF.
  - With it OFF, every flavour of both Cluster 2 arms reproduces its
    fingerprint: Release, Diag, Tune and Ablate, ON 5,369,042 and OFF
    11,880,564.
  - With it ON, `bench 13` reads **5,361,058**.
  - Release CTest passes 26/26 on the coloured build and on both
    switch-OFF arms, the strength floors included.
  - The coloured sanitizer build passes `test_history_core`,
    `test_move_picker_core`, `test_tt`, `test_b3_proof`, `test_search`,
    `test_engine_ponder` and `test_invariants`.
- **The planted check** (`tools/results/b36-20261104/col_suite_d14.json`
  `bc53b0a5…`, the coloured Diag build on `suite_v2` at depth 14):
  - The capture, continuation and pawn histories read **0.0000%**
    other-colour share, with no read of an entry the other colour
    trained.
  - The continuation correction read **0.59%** (60,258 reads trained by
    the other colour alone, 99,077 by both).
  - A probe that left the correction's sentinel row out of the counter
    (a throwaway build, not committed) read **0.0000%** on 16,323,619
    reads.
  - So the residual is the sentinel, `row[NO_PIECE_TYPE][A1]`. Records
    with no move two or four plies back point to it, and the update trains
    those rows whichever colour moved. That behaviour predates this step,
    and the donor's `NO_PIECE` sentinel shares it.
  - The colour index itself is exact.
- **Cutoff reads, suite at depth 14** (each position from clear tables),
  colourless → coloured:
  - first-move cutoff rate **0.8743 → 0.8731**;
  - mean cutoff index **0.2102 → 0.2158**;
  - nodes **15,836,468 → 16,766,365** (+5.9%).
  
  From clear tables each entry learns from half the data. A game carries
  the tables over from move to move, so these reads are a lower bound on
  what the split earns. They are recorded, not deciding.
- **Memory:** continuation 1,568 → 6,272 KiB, pawn 7,168 → 14,336 KiB,
  correction 392 → 784 KiB, capture 6 → 12 KiB. `test_history_core`
  checks every size on both builds.
- **The gate's arms:** `basilisk-b36-colour-on-pext-pgo.exe`
  (`0E2C93E2…`, 5,361,058) against `basilisk-b36-colour-off-pext-pgo.exe`
  (`21CF9C56…`, 5,369,042).
  - Both are clean final-PGO builds at `70e978b`, Cluster 2 on through
    the default.
  - `tools/run_b36_gate.ps1` pins them, and its dry run is accepted:
    `[0,3]` normalized, 16,000 pairs, `3+0.03`, seed 46001.
  - Maintainer-run.
- **Predictions so far:**
  - **X5 missed as written:** the correction read 0.59%, not 0%. The
    probe places all of it in the sentinel row and none in the colour
    index.
  - **X6 missed:** the first-move cutoff rate fell slightly, from clear
    tables.
  - X7 waits for the gate.

