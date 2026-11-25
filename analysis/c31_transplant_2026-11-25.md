# C.3.1 — The transplant read: the donor's passers and king safety on Basilisk's base

- State / class: research, `R3`; the instrument is `READY_FOR_IMPLEMENTATION`
  (§9). No engine source changes.
- Owner / date: agent, 2026-11-25, registered before any game. Experiment
  BAS-O15.
- Decision it serves: the maintainer accepted the evaluation's rebuild in
  the donor's form on 2026-11-25 (PLAN C.3), then asked whether the two
  families that hold the deficit would be enough.

## 1. Question

BAS-O14 put the same-search deficit in two families: passers +125.4 and
king safety +43.1. Every other family read zero or below.

- If the donor's passers and king safety, set on Basilisk's own
  evaluation, come close to the donor's whole evaluation, only those two
  are ported. Basilisk keeps its own mobility, pawn structure, piece
  terms, space, threats and tables. That is about half the port and a
  more original engine.
- If they do not, the whole rebuild goes ahead as accepted.

Building the two first cannot answer this cleanly. They are about half
the port, and a hybrid that read low would not say whether the forms or
the fit failed. The oracle already holds both evaluations, so the idea is
tested there, at the donor's values, before anything is built.

## 2. The instrument

The C.2.9 oracle (`9587eeeb`'s search, both evaluations, the family
masks of `tools/oracle/oracle-eval-ablate.patch`) gains a third
evaluation, the transplant T(S), for a family set S in the mask's bit
order:

    T(S) = bridge(B with its families S removed) + X(S)

- **B** is Basilisk's evaluation at the accepted head, through the
  bridge as today: centipawns times `PawnValueEg` / 100, then the
  bridge's clamp.
- **X(S)** is the donor's contribution from the families in S, taken
  from one full, unmasked donor evaluation of the same position:
  - t = the sum over the families in S of the donor's White-minus-Black
    `Score`, as the ablation patch already names them (`kingTerms`,
    `passedTerms` and the others);
  - X = (s_mg · mg(t) · ph + s_eg · eg(t) · (128 − ph) · sf / 64) / 128,
    with `ph` that evaluation's game phase and `sf` the scale factor it
    applies;
  - then the donor's own rule-50 damping, and the side to move's sign.
- A family the donor's evaluation never computes for a position (a
  specialised endgame, a lazy exit before it) contributes zero.
- s_mg and s_eg are §3's two scales.

Matched cost: with `EvalMatchedCost=true` every evaluation call computes
both evaluations whichever value it returns. All arms of these reads set
it, so no arm is faster for evaluating less.

The options, by proposed name (implementation may rename and records
it): `EvalTransplantMask` (0 to 127, default 0), `EvalTransplantMgScale`
and `EvalTransplantEgScale` (in 1/1024, defaults 697 and 2022),
`EvalMatchedCost` (default false). A transplant mask other than 0 needs
`Use Basilisk HCE=true` and `EvalAblationMask=0`; any other combination
is refused, never ignored.

## 3. The two scales, frozen

One donor unit is not one Basilisk unit, and the ratio changes with the
phase. The rule, fixed before the values were read:

- Fit each evaluation's K on game results as a linear function of the
  phase: logit = (k_mg · p/24 + k_eg · (1 − p/24)) · score.
- The scale of a donor unit in bridged Basilisk units is
  K_donor / (100 · K_Basilisk), read at phase 24 and at phase 0.
- The sample is C.0's gate-game rows with the head's score (243,662
  rows), because the reads are played from such positions.

| Sample | s_mg | s_eg |
|---|---|---|
| Gate-game rows (registered) | 0.681, **697/1024** | 1.975, **2022/1024** |
| Corpus A's validation rows (for comparison) | 0.581 | 1.848 |

Script and log: `tools/results/c210-close/transplant_scale.py`.

## 4. Zero-game checks, before any game

Implementation's, each mechanical:

1. **The old instrument is unchanged.** With the transplant off and
   matched cost off, `bench 16 1 8 default depth` gives the existing
   binary's node counts at masks 0 and 127 under both values of
   `Use Basilisk HCE`.
2. **Matched cost changes time only.** With it on and the transplant
   off, the same four node counts.
3. **The wires are live.** A transplant mask of 40 moves the node count
   against mask 0; so does each scale against its default; so does mask
   127 against mask 40.
4. **X is the donor's own terms.** A diagnostic prints B, mg(t), eg(t),
   `ph`, `sf`, X and T for a position. On at least 10,000 game positions
   mg(t) and eg(t) equal the donor's own trace rows for the families in
   S, to the trace's printed precision, for S = 40 and S = 127.
5. **Colour symmetry.** T of a position equals T of its colour-flipped
   mirror on the same positions.
6. **A refused combination is refused**: a transplant mask with
   `Use Basilisk HCE=false`, and one with a non-zero `EvalAblationMask`.

## 5. The reads

One binary, pinned by hash. Colosseum CLI, two-engine reads, 1,500
cycles (3,000 games), UHO `UHO_Lichess_4852_v1.epd` in random order, seed
`210003` for every read, `3+0.03`, margin 20 ms, 1T, Hash 64, concurrency
14, no adjudication, no tablebases, zero permitted faults,
`EvalMatchedCost=true` on both arms.

| Read | First arm | Second arm | Elo of the first arm |
|---|---|---|---|
| `t40-b` | T(40): passers and king safety transplanted | Basilisk's evaluation | g, the gain |
| `d-t40` | the donor's evaluation | T(40) | s, the shortfall |
| `d-t127` | the donor's evaluation | T(127): all seven transplanted | c, the control |

Played in that order, each once, never extended or replaced. About 35
minutes a read.

## 6. Confounds, with their direction

- **Unfitted.** The donor's values meet Basilisk's other terms without a
  refit. A real port is refitted. This lowers g.
- **Scale.** Two numbers from one fit; corpus A gives 15% and 6% less.
- **Phase and scaling.** X rides the donor's phase and scale factors,
  not Basilisk's.
- **Tables.** Basilisk's endgame pawn table still pays up to 270 on the
  seventh rank, so a passer is counted in the table and in X. Direction
  unknown.
- **Inputs.** The donor's king danger reads the donor's own attack maps
  and mobility. A port of the two families needs those inputs too.
- **Host.** The search is the donor's, not Basilisk's.
- **Matched cost.** Every arm is slower than in BAS-O14, so g + s is not
  that entry's +146.2.

The control answers the first four together: T(127) carries the same
mechanics with nothing left out.

## 7. The decision rule, frozen

Point estimates decide; intervals are reported beside them.

1. **Two families suffice** when s ≤ 30 and g's interval excludes zero.
   Then passers and king safety are ported with their inputs, the other
   families stay Basilisk's, and the whole rebuild is not built.
2. **The whole** when g ≤ 40, or g is under 40% of g + s. The rebuild
   goes ahead as accepted.
3. **Staged** otherwise. The port builds king safety and passers first,
   and the maintainer decides on the remainder with the control in hand.
4. **The control qualifies 2 and 3.** If c is above 40, the transplant
   itself or Basilisk's base costs more than the rule resolves. The
   narrow port is then *not shown*, not refuted, and the rebuild stays
   whole as accepted.
5. A read with an arm outside 20% to 80%, or with one arm more than 5%
   faster than the other, decides nothing and returns to the maintainer.

## 8. Predictions, frozen before any game

Elo, with an 80% band.

| Quantity | Point | 80% band |
|---|---:|---|
| g, T(40) over Basilisk's | +75 | +30 to +120 |
| s, the donor's over T(40) | +55 | +15 to +100 |
| c, the donor's over T(127) | +15 | −20 to +50 |

- Rule 1 (two families suffice): 0.35. Rule 2 (the whole): 0.25. Rule 3
  (staged): 0.40.
- The control passes (c ≤ 40): 0.8.
- Basis: BAS-O14's passers and king safety sum to +168 against a lead of
  +146, but the singles did not add (+40 against +163 together), and the
  transplant is unfitted.
- **Falsifier of the instrument:** g below zero with c above 60. The
  mechanics are then at fault and the reads decide nothing.

## 9. Handoff to implementation

- **Goal.** The transplant and matched-cost options in the oracle, a
  launcher for §5's three reads, and their table.
- **Files.**
  - a new patch beside `tools/oracle/oracle-eval-ablate.patch`, applied
    after it by `tools/oracle/build_oracle.ps1`;
  - the new binary in `tools/test_engines/`, built around the accepted
    head's `src/`, its hash pinned in the launcher;
  - `tools/run_c31_reads.py`, on `tools/run_c210_reads.py`'s pattern:
    `--read <name|all>`, `--dry-run`, the hash check, §4's wire checks
    before a read, the launch-record check that each arm carries its
    options, finished reads never replaced, an interrupted read resumed
    by the same command;
  - the table: Elo with its 95% interval, pentanomial, nodes a second
    and nodes a move by arm, clamped elementary-mate draws, the flags of
    rule 5.
- **Semantics.** §2 exactly. Where §2 is silent, the donor's stock
  behaviour stays.
- **Invariants.** §4's six checks. No file under `src/` changes; the
  engine's `bench 13` stays 3,359,329.
- **Non-goals.** No change to the existing masks or their reads. No
  transplant at the level of a single term. No scale other than §3's as
  a default. No game played by the agent: the reads are the
  maintainer's.
- **Cheap qualification.** §4, a dry run of each read, and a smoke of a
  few games under other arm names, read for its exit only.
- **Report back.** The binary's hash, the node counts of checks 1 to 3,
  the largest gap in check 4, and the single command for the reads.

## 10. A correction to this session's static claim

Earlier on 2026-11-25 I recorded that Basilisk's score predicts results
better than the donor's. That holds only with one K over all rows. The
donor's scale differs by phase (K 0.43 a pawn at phase 24, 1.10 at phase
0, on gate games; Basilisk's is nearly flat), and with K linear in the
phase the donor leads:

| Sample, donor | One K: donor − Basilisk | K by phase: donor − Basilisk |
|---|---|---|
| Gate games, `9587eeeb` | +10.7 | −44.3 |
| Gate games, `sf_16` | +23.0 | −25.1 |
| Corpus A, `9587eeeb` | +42.5 | −43.7 |
| Corpus A, `sf_16` | +50.0 | −37.1 |

In 1e-4 nats; negative means the donor predicts better. So results do
see the donor's lead once each score is calibrated by phase. What still
stands: the screen's per-family ranking did not match the games (space
led it and reads −2.1 in play). The fit's method for the port is
therefore more open than I said, and it stays C.3.1's question.

## 11. The maintainer's directions for the rest of C.3.1 (2026-11-25)

- Ideas and forms may come from the donor. Every value and constant that
  ships is fitted as Basilisk's own. The donor's values appear only in a
  diagnostic arm, as here.
- Basilisk's own forms return where they give strength, each on its own
  read, as explicit steps in PLAN (C.3.6).
- The pull request waits for C.3.

## Return 1 — §2's scale factor is not colour symmetric (2026-11-25)

**Premise returned.** §2: X uses "`sf` the scale factor it applies"; §4
check 5: "T of a position equals T of its colour-flipped mirror". Both
cannot hold. When the donor's whole endgame score after `winnable()` is
exactly zero, `strongSide = eg > VALUE_DRAW ? WHITE : BLACK` names Black in
the position and in its mirror alike, so `sf` is computed for different
real sides. The donor's own value never shows it, since it multiplies that
`sf` by zero; X multiplies it by eg(t), which is not zero.

**Evidence.** The instrument as §2 states it
(`tools/oracle/oracle-eval-transplant.patch` after the ablation patch,
around `e56f1e1`'s `src/`, a development build of SHA-256
`CC5F167F…C4EF6E`, kept at `tools/results/c31-checks/`), checked by
`tools/diag/c31_checks.py` on 12,119 positions, every 21st row of C.0's
gate-game positions (`positions.csv`, SHA-256 `1308BD0E…455494`), none in
check:

| Check | Result |
|---|---|
| 1, old instrument | 293,089 / 289,609 (HCE true, masks 0 / 127); 322,185 / 384,959 (HCE false): the C.2.9 oracle's counts exactly |
| 2, matched cost | the same four counts |
| 3, wires | mask 0: 293,089; mask 40: 261,621; 40 with mg scale 1024: 246,746; 40 with eg scale 1024: 285,494; 127: 292,447 |
| 4, donor's terms | 48,476 cells at S = 40 and 242,380 at S = 127, all within the trace's 0.005; largest gap 0.00495 of a pawn; X and T recomputed from the line's fields agree everywhere |
| 5, symmetry | **fails**: 99 positions at S = 40, 104 at S = 127 |
| 6, refusals | both combinations refused at `isready` and at `go`; the valid one, reached through an invalid order, accepted |

In every asymmetric position only `sf`, and so X, differs; B, mg(t),
eg(t) and `ph` are symmetric everywhere, and every one of them has the
donor's whole endgame score at 0.00 in its trace. That score is zero in
956 of the 12,119 positions (7.9%), mostly drawish endgames where
`winnable()`'s complexity cancels it. The X gap: median 17 and 28 internal
units at S = 40 and 127 (0.08 and 0.14 of a pawn), 25 and 29 positions at
50 or more, largest 522 and 608 (2.5 and 3.0 pawns). Breakdown:
`tools/results/c31-checks/symmetry_breakdown.json`.

**What was built and is kept.** The patch: the four options under §2's
names and defaults (`EvalTransplantMask` 0 to 127,
`EvalTransplantMgScale` and `EvalTransplantEgScale` 0 to 4096,
`EvalMatchedCost`); the donor's families recorded from one full evaluation
where it computes them; X in one exact 64-bit expression with one division
truncating toward zero, then the side to move's sign and the donor's
rule-50 damping, with no evaluation grain and no clamp on T, since §2 lists
neither and the donor's own value is not clamped (a T in the mate range
stops the engine; the sample's largest |X| is 4,736 and |T| 9,999); the
refusal of a transplant with `Use Basilisk HCE=false` or a non-zero
`EvalAblationMask` at `isready` and at `go`, whatever order the options
arrive in; and a `transplant` command printing B, mg(t), eg(t), `ph`,
`sf`, X, T and every family for the current position, with no contempt,
as the donor's trace has none. In search the donor's full evaluation runs
with the search's contempt, as stock, which moves only its lazy exits.
`tools/diag/c31_checks.py` runs the six checks. The launcher and the pinned
binary wait for the amendment, because the patch may change.

**Options the implementer sees.**

1. Keep §2 and accept the asymmetry: T is not colour symmetric in about
   0.8% of game positions. Check 5 is amended to exclude positions whose
   donor endgame score is zero.
2. Where the donor's whole endgame score is zero, so its `sf` scales
   nothing, take `sf` for the side eg(t) favours, by the donor's same rule.
   Elsewhere X is §2's exactly, in 92% of these positions. T is symmetric
   everywhere. A small refactor lets `winnable()`'s factor be computed for
   a named side.
3. Always take `sf` for the side eg(t) favours. Symmetric, but it departs
   from the factor the donor applies wherever eg(t) and the whole endgame
   score differ in sign.
4. Where the donor's whole endgame score is zero, use the normal factor,
   64. Symmetric and simple, but it unscales the drawish endgames, opposite
   bishops among them, that the donor would scale.

The implementer's view: option 2, because it changes X only where the
donor's own factor multiplies nothing and makes check 5 hold as written.
The choice and any change to §8's predictions belong to Amendment 1.

## Amendment 1 — the scale factor where the donor's scales nothing (2026-11-25)

- **Option chosen and why.** Return 1's option 2, chosen by the maintainer
  on 2026-11-25. It changes X only where the donor's own factor multiplies
  a zero endgame score, so it is §2's factor wherever the donor applies
  one, and it makes check 5 hold as written.
- **Predictions affected.** None. §8 stands as registered, and the change
  is clerical: no game had been played, and it touches X only in the
  game positions whose donor endgame score is exactly zero, 378 of the
  12,119 (3.1%), mostly drawish endgames. Return 1's 956 (7.9%) counted the
  trace's printed 0.00, which also holds scores under half a hundredth of a
  pawn; X is §2's exactly in the other 96.9%.
- **Handoff clause replaced.** §2, "with `ph` that evaluation's game phase
  and `sf` the scale factor it applies", now reads: "with `ph` that
  evaluation's game phase and `sf` the scale factor it applies; where that
  evaluation's whole endgame score after `winnable()` is zero, so that its
  factor scales nothing, `sf` is the factor the same rule gives the side
  eg(t) favours (Black when eg(t) is zero too, where the term vanishes)".
  §4 is unchanged.

## Implementation record

- **Commits.** `4156385` (the patch and the checks as §2 stood), `ded4c19`
  (Return 1), `3d2d8f0` (the factor by Amendment 1), and the commit that
  adds the launcher, the table and this record.
- **Binary.** `tools/test_engines/oracle-c31-3d2d8f0-transplant.exe`,
  SHA-256 ECF6C8BD71BB011899F0AFCBF5431C2100D500C8DE74A18BA11CD87FC37F1C4D,
  from `tools/oracle/build_oracle.ps1 -BasiliskRevision 3d2d8f0 -Label
  c31-3d2d8f0-transplant -ExtraPatch oracle-eval-ablate.patch,
  oracle-eval-transplant.patch`. `src/` is byte-identical to `e56f1e1`'s,
  so `bench 13` stays 3,359,329. Conformance 471,519 positions, 0
  mismatches; oracle bench 1,017,388, as the C.2.9 oracle's.
- **The six checks** on that binary (`tools/diag/c31_checks.py`, record
  `tools/results/c31-checks/checks.json`):

  | Check | Result |
  |---|---|
  | 1 | HCE true: 293,089 at mask 0, 289,609 at 127; HCE false: 322,185 and 384,959; the C.2.9 oracle's counts |
  | 2 | the same four with matched cost on |
  | 3 | mask 0: 293,089; mask 40: 272,892; 40 with mg scale 1024: 256,981; 40 with eg scale 1024: 293,365; 127: 269,110 |
  | 4 | 12,119 positions; 48,476 cells at S = 40, 242,380 at S = 127; largest gap 0.00495 of a pawn, under the trace's 0.005; X and T recomputed from each line agree; `sf` is the donor's wherever its endgame score is not zero (378 positions where it is) |
  | 5 | no asymmetric position at S = 40 or 127 |
  | 6 | refused at `isready` and at `go` for both combinations; the valid one accepted |

  Check 3's counts differ from Return 1's because the bench reaches
  positions whose donor endgame score is zero.
- **Launcher.** `tools/run_c31_reads.py`, on `run_c210_reads.py`'s
  pattern. Before any read it checks the hash, requires the checks' record
  to be a full PASS for this hash, and reruns checks 1, 2, 3 and 6. Every
  arm states all six evaluation options. After a dry run it refuses a
  launch record in which an arm does not carry exactly its options; a
  record with one option changed was refused. A finished read is refused
  with exit 2. All three dry runs pass, and so does a smoke of four games a
  read under other arm names (`tools/results/c31-smoke-*`).
- **Table.** `tools/diag/c31_table.py`, with `test_c31_table.py`: c210's
  recount checks, Elo with its 95% interval, pentanomial, nodes a second
  and nodes a move by arm, clamped draws, and rule 5's two flags.
- **Deviations from the handoff.** None beyond Amendment 1. Where §2 is
  silent: X is not grained, T is not clamped (a T in the mate range stops
  the engine), and in search the donor's full evaluation keeps the
  search's contempt, as stock.
- **What the reviewer must check.** The patch against §2 as amended; the
  binary's hash and the six checks reproduced; the launch records' options.

## Result and the rule applied (2026-11-25)

Three reads of 3,000 games, no fault (BAS-O15; `tools/results/c31-table.md`).

| Read | Quantity | Elo [95%] | Predicted | Nodes a second |
|---|---|---|---|---|
| `t40-b` | g | −1.6 [−10.7, +7.5] | +75, +30 to +120 | T(40) 2.15M; Basilisk's 2.05M |
| `d-t40` | s | +122.1 [+113.1, +131.3] | +55, +15 to +100 | T(40) 2.10M; the donor's 2.02M |
| `d-t127` | c | +172.9 [+162.8, +183.4] | +15, −20 to +50 | T(127) 2.13M; the donor's 1.93M (flagged) |

- **The falsifier of §8 fired:** g below zero with c above 60. The
  control's speed flag favours the arm that lost, so it does not
  explain c.
- **§7 applied.** Rule 4 governs: the control is above 40, the narrow
  port is not shown, and the rebuild stays whole. Rule 2's condition
  also holds, but with the control failed g measures the instrument,
  not a two-family port.
- **What the reads do not say:** that the donor's passers and king
  safety, fitted beside Basilisk's other terms, would fail. They say
  this instrument cannot put the donor's terms on Basilisk's base.
- **Calibration.** None of three predictions inside its band. The
  wrong part of the model is §3: scales taken from each whole score's
  calibration by phase do not set a term's weight against the material
  and tables beside it (BAS-O15, *Prediction calibration*).
