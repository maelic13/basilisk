# C.2.8.1 the fit and gate registration (2026-11-23)

- State / class: closed, `R2`. Registered before any C.2.8 fit or game.
  BAS-O13 holds the fits; BAS-E59 and BAS-E60 hold the gates.
  Amendment 1 (§6, 2026-11-23), after run 1, changes the support rule
  and registers run 2.
- Owner / date: agent, PLAN C.2.8.1, on Opus at the maintainer's choice.
- Inputs:
  - BAS-O12 with Corrections 1 and 2;
  - C.2.4's packet §6 (B's gate is conditional);
  - C.2.5's production rule (the seed);
  - C.2.7's bracket veto (audit §6).

**Decision.** Fit corpus A, then corpus B, with `fit.py`'s four stages
from Stockfish 16's tables at 0.35, under one frozen K.
- Gate the A fit against the head by SPRT `[0,3]`: BAS-E59, the refit's
  strength.
- Run the label gate, BAS-E60, only if the B fit predicts B's labels
  better than the A fit by more than the instrument resolves.
- The magnitude contract is read and reported, not enforced. Its reads
  explain; the SPRT decides.
- The main counter-argument is a per-phase K. It keeps scores at today's
  magnitudes, so no search margin tuned to them changes meaning. It is
  registered as the retry.

## 1. The magnitude contract (C-97, C-98)

**The measured situation** (BAS-O12, Correction 2):
- A whole-surface fit at one frozen K makes every piece worth 12% to 35%
  more in endgames. Middlegame values move −20% (the pawn) to +4%.
- The header's own K is 2.33 in endgames against 2.01 overall, and the
  fit answers by steepening the endgame's scores.
- 27% more validation rows reach the lazy checkpoint.

**Options:**
1. **One frozen K; `LAZY_MARGIN` unchanged; the contract read and reported.**
   - The protocol as C.2.1 built it.
   - C.0's +15 to +40 prior and its 2.58% Brier screen came from a fit of
     this kind.
   - The re-valuation encodes something true in chess: material converts
     more surely as the board empties.
2. **A K per phase band in the loss.** Scores keep today's magnitudes, so
   no search margin changes meaning.
   - It changes the tuner's loss and C.2.1's contract.
   - It throws away the cross-phase signal option 1 keeps: whether to
     trade into an ending.
3. **Option 1 plus a lazy-margin refit in the same candidate.** Two causal
   axes in one gate (C-57), so it is refused.

**Chosen: option 1.**
- The ±20 cp bound applies to an evaluation unit added on top of a fitted
  surface. A whole-surface refit whose prior is tens of Elo moves scores
  by more than that by design.
- So the contract is read on this candidate, not enforced. These reads
  explain a result and do not accept one:
  - the share of validation rows at the lazy checkpoint;
  - the median served-score change by phase band;
  - the marginal piece values by phase (`piece_value_read.py`'s method).
- **Retries,** in order, if BAS-E59 rejects or its equal-node companion
  (C-95) disagrees with it:
  - option 2, the per-phase K;
  - a lazy-margin refit (C-98, owned by C.10);
  - each as its own registered gate.

## 2. The schedule (8.7.b)

| Stage | Settings | Validated by |
|---|---|---|
| king safety | `fit.py`'s defaults (`--ks-positions 100000`) | C.2.1; it must stop converged |
| linear | `--lr 0.3 --epochs 20000 --patience 50 --min-delta 1e-6` | BAS-O12: four starts converged at 2,700 to 3,200 epochs, and the travel bound (6,000) never binds |
| king safety | as before | — |
| polish | rate 0.075 (a quarter), patience 100, the same tolerance and cap | `fit.py`'s rule |

- Every stage must stop by its own convergence rule. An exit 3 stops the
  run, and it is reported, never relaxed.
- **Anchors:** `--l2 0 --smooth 0`. The only anchors are the surface's:
  - material held (C-19);
  - the two king tables and the eight mobility tables mean-anchored.

  This is the looseness PLAN asks for: BAS-O12's tables moved 72 to 78 cp
  an entry on average, with no L2.
- **Corpus and K:**
  - Corpus A's train split, every row (3,547,892 used).
  - K is fitted once on A's validation at the seed and frozen for both
    arms. The B arm reuses A's K, so the arms differ only in labels
    (C.2.4's packet §6).

## 3. The seed

C.2.5's production rule, unchanged: `seed_sf16.py --scale 0.35` from the
header at the fit's root, one start. `fit.py` must seed from that file
rather than from the compiled header (C.2.8.2).

## 4. The gates

- **BAS-E59, the refit:** the A fit's baked build against the head, at
  BAS-S67's conditions.
  - `[0,3]` nElo, 3+0.03 1T, Hash 64, concurrency 15, paired reversed UHO
    openings;
  - no adjudication, tablebases off, a fault cap of zero, a cap of 4,000
    pairs.
  - One causal axis: the fitted surface, labels A.
- **BAS-E60, the label contract:** the B fit's build against the A fit's,
  under the same conditions. It runs only if, on B's validation labels at
  the frozen K:
  - the B fit's realized loss is below the A fit's by at least 2e-5,
    BAS-O12's measured resolution;
  - and the 95% interval of the paired difference, bootstrapped over
    validation games, excludes zero.

  Otherwise corpus A stands and B is recorded null at fit level, with
  C.2.4's retry trigger.
- **Before any bake** (C.2.8.3's report), all against the seed:
  - the king-safety and king-PST movement (C-56);
  - `bench 13`, where growth over 5% is a finding the gate must explain;
  - the deterministic floors (KBNK, KQK, KRK, KBBK), which must pass;
  - the per-table movement, and the endgame-passer group and E6's signs
    (C.0);
  - the magnitude reads of §1.
- **With the gate:** C.2.7's bracket veto. The candidate is measured at
  21,000, 64,000 and 190,000 nodes against `tools/results/c274/head`.
- **After selection:** corpus A's test split is claimed once
  (`claim_test.py`) and reported for the accepted vector.
- **When refitted tables land:** the PeSTO disclosure is retired and the
  similarity screen re-run (maintainer, 2026-10-27).

## 5. What C.2.8.2 implements

`tools/texel/fit.py` gains:
- `--init FILE`, a seed vector whose root is the header;
- `--K K`, to freeze a given K instead of fitting one;
- `--min-delta`, passed to the linear and polish stages;
- per-stage settings, so the registered schedule runs as registered;
- the lazy-checkpoint count in its loss records.

The magnitude reads become a committed tool, `tools/texel/piece_values.py`
(from `tools/results/c25/piece_value_read.py`), with tests, and the
contract's guards stay. Non-goals: no change to the tuner's loss, the
surface or any engine source.

## 6. Amendment 1: the support rule and run 2 (2026-11-23)

- State / class: recorded, `R2`, after run 1 and before any re-run. It
  answers the fit report's return
  (`analysis/c283_fit_report_2026-11-23.md` §3) under BAS-O13's stop rule.
- Owner / date: agent, on Fable at the maintainer's choice.
- **Accepted by the maintainer in full, 2026-11-23:** the third part of
  BAS-E60's condition (§6.3) and the queen-mobility floor (§6.2)
  included.
- New reads, in `tools/results/c283/amend1/` (ignored), by the tuner in
  `build/c283/texel` (the restored header; `surface.py check` passes):
  - `support_full.py`, `.json` and `.log`: every slot's support on the
    whole train split;
  - `rank_trial.py` and `.log`: the rank on larger samples.

**Decision.** Count support on every row the linear stages read, keep the
threshold at 200 rows, and re-run both fits.
- By count, two slots stay frozen, not four, and no PST entry is near the
  threshold.
- A fit fails when a support-frozen slot sits in an unanchored table that
  still has a free member.
- The rank sample grows to 300,000 rows. At 30,000 it fails under the new
  freeze.
- BAS-E60's condition is re-read on run 2, with a label-specific part
  added.
- BAS-O13's predictions are not re-frozen. Run 2 is the loss interval's
  first fair test.
- The main counter-argument is the report's option 2: hold a frozen entry
  relative to its table. It is the general repair, and it is machinery
  for a case this corpus does not have. It is registered as the retry.

### 6.1 The new reads

**Support on the whole split** (`support_full.py`). The dense matrix of
the split is 33 GB, so the split was dumped in 26 chunks
(`--dump-coeffs`) and each chunk counted.
- Controls: the first 30,000 rows reproduce run 1's 242 slots and their
  counts, and the rows sum to the fit's 3,547,892.
- **Two free linear slots have under 200 rows:** `MobQMg[0]` and
  `MobQEg[0]`, 10 rows each.
- Queen mobility 1 has 278 rows (mg) and 224 (eg). It was absent from the
  sample, so the report's scaled estimate counted four slots.
- The other 240 slots that run 1 froze have 224 rows or more. The 242
  have a median of 12,378.
- The rarest PST entries: the knight on h8, 1,457 rows; the mg king on
  a8, 1,923.

**The rank sample** (`rank_trial.py`). With the whole split's freeze,
1,023 slots are free.
- On the first 30,000 rows `rank` fails, 1,012 of 1,013: queen mobility 1
  has no row there.
- On 100,000, 200,000 and 300,000 rows it passes, 1,013 of 1,013, with a
  smallest relative singular value of 0.002. The rarest free slot has 4,
  7 and 15 rows in those samples.

**The stranding, from the vectors.** The report's table reproduces: the
153 non-king PST entries run 1 held moved 0, while the rest of their
tables moved −215 (mg pawn) to +52 (eg rook).
- In BAS-O12's full-surface fits the same entries moved by their own
  data. In the endgame tables they moved with the rest: eg rook +171
  against +180, eg knight +104 against +80 (S613). In two tables they
  moved apart from it: mg rook +14 against −53. So a gap between these
  entries and their table is not by itself a stranding.
- **The freeze also changed the levels (inference).** Run 1's free
  entries re-levelled unlike BAS-O12's: eg rook +52 against +180, eg
  queen +33 against +150, eg knight +19 against +80. A piece-times-pawn
  product re-values a piece on a held square, and a table level cannot.
  So run 1 carried more of the re-valuation in the products.

**A second tie: the tuner's monotone chains.** A support-frozen slot is
held at the seed's value. That is sound only where its neighbours cannot
move away from it. Two structures tie a slot to its neighbours:
- a free table level, the report's finding;
- a non-decreasing chain (`enforce_chain` in `tuner.cpp`): the mobility
  tables, the passer ranks and the pawn-threat chain. A held entry
  bounds the free entries on both sides of it.

Run 1 held 11 of each queen-mobility table's 28 entries (0 to 4 and 22
to 27), so the 17 between them could move only between two held values.
The report did not read this.

**Run 1's label read, taken apart** (the report's §5 and its logs):

| Realized loss, K 1.960321 | A's labels | B's labels |
|---|---:|---:|
| A fit | 0.09648236 | 0.09632117 |
| B fit | 0.09650363 | 0.09634110 |
| B fit's advantage | −2.13e-5 | −1.99e-5 |

The B fit is worse on both label sets by about 2.1e-5: a slightly worse
vector, by its path, whatever the labels. What belongs to the labels is
the difference of the two advantages: **+1.3e-6**.

### 6.2 The support rule

**Chosen: the report's option 1 by exact count, with a guard.**
- **Support** is the number of rows whose coefficient for the slot is
  nonzero (`--dump-coeffs`'s rows and definition), counted on every row
  the linear stages read: the whole train split at `--max-positions 0`.
- **The threshold stays 200 rows** (C-52). Counted on 30,000 rows it
  meant about 23,650 rows of the split, 118 times what C-52 asks.
- **The rank** is read on a sample, 300,000 rows for run 2, over the
  slots the count left free.
- **The guard:** a fit stops before any stage when a support-frozen slot
  belongs to a table that still has a free linear member and no mean
  anchor. A table is a registry group of more than one slot.
  - Such a table's level is free, or is held only weakly by another
    term. The ten non-king PSTs are the measured case.
  - The test reads the manifest alone and errs toward stopping. A stop
    costs a returned leaf, and a missed one cost run 1.
- **A support-frozen slot in a mean-anchored table is allowed** and
  recorded. Its table's level is held, so it is not stranded.

**The two slots that stay frozen.** `MobQMg[0]` and `MobQEg[0]` are held
at the seed's 0, the bottom of a monotone chain. So no queen-mobility
entry can end below 0.
- BAS-O12's free fits put entries 0 and 1 on the −200 clamp, mg and eg,
  and eg entries 2 and 3 at −133 and −16 (S613). Run 2 will stop them at
  0.
- That is accepted. The floor is the seed's own lowest value, the table
  stays monotone, and the rows concerned are 0.2% of the split.
- It is also the safer value (inference, not measured). Queen mobility 1
  is what a queen on its home square counts before its own pawns move:
  the bishop's square alone (`src/eval_attacks.h`, the mobility area).
  The corpus starts at book exits, so its 224 to 278 such rows are
  mostly trapped queens. At −200, the first pawn move from the starting
  position would swing the score by about 230 cp.
- The report reads both queen-mobility tables and names the entries
  standing on the held one. The entry's definition belongs to C.3.

**Refused:**
- **Scaling the threshold to the sample** (about 1.7 rows of 30,000). A
  slot at the threshold is absent from such a sample 18% of the time.
  Queen mobility 1 was: the scaled rule freezes it, and the count does
  not. The count costs one pass over the split.
- **Holding a frozen entry relative to its table** (option 2). A tied
  entry enters the gradient and the anchor projection: a tuner change
  with its own tests, for no slot of this corpus. Retry trigger: the
  guard stops a fit.
- **Anchoring the PST levels** (option 3). It changes C-19's gauge and
  the tracked surface, which §5 excludes. With material held as well,
  the levels that carry the endgame re-valuation §1 chose to keep could
  not move. Retry trigger: C.6's screen of the level against the
  products (C-118), or a guard stop that option 2 cannot serve.
- **Freeing every slot**, BAS-O12's surface. It puts queen mobility 0 on
  a clamp from 10 rows, and entry 1 with it.

### 6.3 B's re-run and BAS-E60's condition

**Chosen: B is re-run with A, and the condition is re-read on run 2. Run
2's read is the verdict.**
- BAS-E60's arms are baked builds, and only run 2's vectors can be baked.
- The rule must not depend on the outcome. Had run 1's read passed, B
  would be re-run; so a failed read is not taken from it either.
- The cost is one more fit, about half an hour.
- Run 1's read stays on the record as exposure, and BAS-E60's prediction
  for run 2 is restated with it.

**The condition gains a third part.** Run 1 exposed a confound (§6.1):
the registered quantity is the sum of a path difference between two
vectors and the labels' own effect, and the first was 15 times the
second.
- A path difference of run 1's size with the other sign would pass both
  registered parts with no label effect. The bootstrap does not guard
  against it: it resamples validation games, not fits.
- **The label-specific part:** the B fit's advantage on B's labels, less
  its advantage on A's labels, is at least 2e-5.
  - Only relabelled rows carry it (604 of 209,444 validation rows), so
    the path difference cancels.
- All three parts must hold. The registered two are read and reported as
  registered.
- The addition can stop a gate and cannot start one, so it rescues
  nothing. On run 1 every part fails.
- The counter-argument: the condition changes after exposure. It only
  tightens, and without it the gate can test a fit's luck instead of the
  labels (C-57).

**Against re-running B at all:** run 1's read was matched, and its
label-specific part is 15 times under the bar. Freeing 240 rare slots
will not multiply it by 15. That is the prediction (0.9), not a reason
to take a verdict from vectors that will never play.

### 6.4 The predictions

**Chosen: BAS-O13's frozen list is not re-frozen, and run 1's score
stays.**
- **The loss interval stands.** It came from BAS-O12's full surface,
  which is run 2's surface plus two slots. Run 2 is its first fair test,
  and the test of the report's explanation of run 1's miss. Moving the
  interval now would turn that explanation into a prediction it never
  was.
- **The items run 1 hit** stand and are read again. Run 1 exposed them on
  nearly the same fit, so a second hit earns no calibration credit.
- **K at the seed** is now a control: the seed and A's validation are
  unchanged, so it must reproduce 1.960321.
- **Two items are restated for run 2,** marked as made after run 1:
  `bench 13`, whose model was wrong, and BAS-E60's condition, which was
  exposed.
- **New predictions** test this amendment's own claims: the loss
  recovers, the levels return toward BAS-O12's, and queen mobility stops
  at the held entry. BAS-O13 holds the list.

**What was wrong in the model:**
- The registration took the tracked surface for the fit's surface. Its
  numbers came from tuner runs with no freeze, and nobody ran `fit.py`'s
  own surface step on corpus A before registering. The tool change's dry
  run now does (§6.5).
- One `bench 13` count was taken for a measure of the tree. Two fits
  within 2 cp an entry differ by 14% (the report's §4).

### 6.5 The tool change (C.2.8.2's driver)

Specified here and implemented in its own step. Non-goals: the tracked
surface, the tuner's loss, clamps and chains, the schedule, the seed and
the K rule.

**`surface.py rank`:**
- It takes two row sets, named apart: the support rows and the rank
  sample.
- **Support is an exact count** over the support rows, by
  `--dump-coeffs`'s rows and definition. `--feature-support` is not that
  count: it reads raw traces on every row, so it counts left-out rows,
  and a middlegame slot in a pure endgame.
- The count streams, since the dense matrix of the split is 33 GB. A
  counting mode in the tuner and chunked dumps are both acceptable, and
  a counting mode changes no loss.
- Freezing uses that count. Each frozen slot's reason gives its count
  and the number of rows counted.
- The rank is read on the sample, over the slots the count left free. A
  free slot with no row in the sample fails `rank` by name, as a sample
  too small, apart from an identity.
- The guard of §6.2 fails `rank`, names every offending slot and its
  table, and writes no manifest.
- Every support-frozen slot is printed, with its count and whether its
  table is anchored.

**`fit.py`:**
- The support rows are the linear stages' rows (`--max-positions`).
  `--rank-positions` stays the rank sample, and its default becomes
  300,000.
- `run.json` records the rows counted, the threshold, the frozen slots
  with their counts, and the number of free linear slots.
- A `rank` failure stops the run before K is fitted, as any failed step
  does.
- Its docstring and the HANDBOOK's §6 and §9 say what is counted on
  which rows. The HANDBOOK's §8 gains two traps:
  - support counted on a sample is a different threshold;
  - two fits of nearly the same data differ by about 2e-5 by their path
    alone, so matched arms are compared on each other's labels.

**Tests,** each failing on its known-bad input:
- The streamed count equals `count_nonzero` of the dense dump, slot for
  slot, on `test_positions.csv` (with the tuner).
- Run 1's defect: a slot under the threshold in the sample and over it
  in the support rows stays free, and the reverse freezes.
- A free slot absent from the sample fails as a sample too small. A
  duplicated column still fails as a null direction.
- The guard: a support-frozen entry fails by name in an unanchored table
  with a free member. It passes in a mean-anchored table, as a table's
  only slot, and where every other member is fixed.
- `fit.py`'s `rank` command carries the linear stages' row count as the
  support rows.

**CI's texel job** runs all of them with `BASILISK_TEXEL` set, so the
tuner test runs there and is not skipped. The job names its test files
one by one, so a new file is added to that list.

**Qualification on corpus A,** a dry run of the surface step with no
fit, before hand-over:
- every slot's support equals `support_full.json`'s, all 1,159;
- exactly `MobQMg[0]` and `MobQEg[0]` freeze, 1,023 slots stay free, and
  the rank is 1,013 of 1,013 on 300,000 rows;
- with the support rows cut to 30,000 it finds run 1's 242 slots, and
  the guard fails on them: run 1's surface is the known-bad input;
- corpus B's train split gives the same manifest, byte for byte.

### 6.6 Run 2

- **Source:** the tool change's commit. The header and the engine are
  unchanged, so the seed's bench is 4,777,412.
- **Settings:** §2's schedule and §3's seed, unchanged. Support at 200
  rows on the whole split, and `--rank-positions 300000`.
- **Order:** A, then B with A's K, in new run directories. Run 1's stay.
- **Controls,** which must reproduce or the reading stops:
  - the seed's hash, K 1.960321 and the seed's loss 0.10122069, as in
    run 1;
  - the dry run's derived surface, the same file in A and B.
- **Reads:** §4's list again, and:
  - the frozen slots and the guard's result;
  - the 153 entries run 1 stranded, each table's against its rest;
  - each table's level shift beside run 1's and BAS-O12's;
  - both queen-mobility tables;
  - the three parts of BAS-E60's condition.
- **Stop rule:** BAS-O13's, unchanged. One run per corpus; a failed
  guard, rank or stage is reported and amended here before another run.

### 6.7 Findings for later leaves (not acted on)

- **Queen mobility 0 and 1 have no usable data** (§6.2). Count 1 is the
  home queen's, and the corpus holds 10 and 224 to 278 rows, mostly
  trapped queens. A free fit puts both on the −200 clamp. Owner: C.3,
  whose research settles the two entries. PLAN's C.3 entry carries it.
- **The level against the products moves with the surface (inference;
  §6.1).** The eg rook table rose 180 cp with every entry free and 52
  with 242 slots held, and the products carried the rest. Run 2 reads
  the split first. Owner: C.6 (C-118's screen). PLAN's C.6 entry carries
  it.
- **A fit's path is worth about 2e-5** (§6.3). Two fits of nearly the
  same data differed by that much on both label sets. Any later
  matched-fit comparison reads its arms on each other's labels. Owner:
  the leaf that registers the comparison; the HANDBOOK's trap (§6.5).
- **One `bench 13` count does not measure a refit's tree** (§6.4). C-56's
  5% growth flag stays registered, but a single count neither raises nor
  clears it. Owner: C.2.8.4's reads.
- **`--feature-support` and the surface count different things** (§6.5):
  raw traces on every row, against coefficients on the rows a fit uses.
  Owner: the tool change's documentation.
- **Run 1's term-level findings** (the report's §7) wait for run 2's
  report, which reads them again on a valid surface.

## 7. Amendment 2: the top tiers stop borrowing the fitted score (2026-11-23)

- State / class: recorded, `R2`, after run 2's report
  (`analysis/c283_run2_report_2026-11-23.md` §6) and before any bake.
  Under the maintainer's authority of the same night: a defect found in
  C.2.8 is fixed by the implementing session and the work continues.
- Owner / date: agent, on Fable.

**Decision.** Do not bake A2 on this head, and do not relax the floor.
Give the queen and rook tiers the donor's bare-king score in place of the
winner's ordinary score, on the head, for both arms. A2 is not re-fitted.
- The main counter-argument: it is an engine change made because a
  candidate failed a floor. It is answered by where the change sits: the
  floor reads a coupling, not the fit (§7.1), every later refit would
  meet it again, and both arms of BAS-E59 carry the repair.

### 7.1 The defect

- **Measured** (the report's §6): A2's baked build converts KR-K 23 of 24
  at 60,000 nodes and 22 of 24 at 50,000. On
  `4K3/8/8/8/8/5k2/8/1R6 w` it finds no mate in 100 plies at 40,000 to
  60,000 nodes, and mates in 29 to 47 at 70,000 and above.
- **The same position is fragile everywhere:** the head takes 27 to 51
  plies across budgets, and B2 passes with 63.
- **The cause is a coupling** (`src/eval_endgame.h`, the top tiers). A
  bare king against a queen or rook scores its tier plus the winner's
  ordinary evaluation, so the piece-square and mobility tables are the
  only mating gradient. `mate_drive` adds 5 and 4 cp a step under them.
  - Those rows are overrides, and the fit leaves every override out. So
    the tables are fitted on other positions and then used here.
  - The gradient is not missing. From the start to a near-mate
    (`7k/8/5K2/8/8/8/8/1R6`) the score rises 162 on the header, 212 on A2
    and 207 on B2 (`--dump-eval`). It has no shape the technique needs,
    and it changes with every refit.
- **Competing explanation, refuted:** the tier's cap flattening A2's
  larger scores. A2's KR-K scores are 26,908 to 27,120 in a tier 2,599
  wide.

### 7.2 The mechanism

In the two top tiers, `ordinary` is replaced by the donor's
`Endgame<KXK>` form (classical Stockfish, `sf_16`'s `endgame.cpp`):
the winner's material, a push of the bare king to the edge, and a pull
of the kings together.
- **Material:** the frozen `EG_MAT` constants and the pawn count, as
  `kxk_score` reads them. No fitted slot.
- **The drive:** the donor's two tables, converted at 100 to its endgame
  pawn. Its span stays under one pawn (87 cp against 100), so it orders
  positions of equal material and never outranks a pawn.
- **Unchanged:** the tier bases, the caps, the order of the tiers, the
  minor-piece mates and `mate_drive`.

**Invariants:**
- More material never scores lower inside a tier, and a promotion to a
  queen still outranks one to a rook.
- No term of a top-tier score reads `g_eval_params`. The test: the same
  position scores the same under two different parameter sets.
- A top-tier score stays inside its tier.
- The fit is untouched: `--verify` passes, and A2's realized loss on
  validation reproduces 0.09612177 on the repaired tuner.

**Interactions:** the lazy path runs `apply_endgame` too and needs no
term the lazy block lacks. The rule-50 damping keeps the tier bases.

### 7.3 Qualification and the gate

Deterministic, with no game gate of its own. The change reorders scores
only inside a tier, so no decision outside a bare-king ending moves, and
the conversion instrument measures those endings directly.
- **The grid:** `test_endgames --conversion` for KQ-K and KR-K at 40,000,
  50,000, 60,000, 70,000, 80,000, 100,000 and 120,000 nodes: 24 of 24
  everywhere, on the repaired head and with A2 baked on it.
- **Independence:** the two builds give the same ply list for KQ-K and
  KR-K at every budget.
- **The other floors:** all four families at the registered 60,000 nodes,
  `test_endgames`, `test_search`, `test_eval`, and the sanitizer job for
  the touched file.
- **`bench 13`:** recorded for the repaired head. If it moves, the
  fingerprint record moves with it, and BAS-E59's baseline is rebuilt on
  the repaired head, as its registration says.
- **Predictions, frozen now:** the grid passes, 0.8; the ply lists are
  identical, 0.95.
- **Falsifier:** any KR-K or KQ-K failure in the grid. The coupling is
  then not the cause, the leaf returns here, and nothing is baked.

### 7.4 What follows

- The repair is its own engine commit with its tests, reviewed against
  this section before the bake.
- Then run 2's pre-bake reads are repeated on A2 over the repaired head,
  A2 is baked, and C.2.8.4 runs as registered. BAS-E59 keeps one causal
  axis: both arms carry the repair.
- **Refused:** baking B2 because it passed (BAS-E60's condition failed,
  and 63 plies is the same fragility); a larger node budget or a longer
  mate budget (relaxing the floor); constraining the fit's tables to
  serve this ending.
- **For C.7:** the pawnless classes that still use `mate_drive`'s 5 and 4
  (KQ-KR and its kin) borrow the fitted score the same way.
