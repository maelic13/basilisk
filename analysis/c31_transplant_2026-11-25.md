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
