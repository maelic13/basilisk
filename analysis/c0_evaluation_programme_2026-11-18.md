# C.0 — Evaluation programme investigation

- State / class: `CLOSED` 2026-11-18, `R3`
- Owner / date: C.0, opened 2026-11-18 on the 1.11.0 head (`bench 13`
  4,777,412)
- Decision needed: the donor evaluation; whether the same-search deficit is
  values or structure; the cluster order and budget; the corpus, refit and
  label protocol; frozen handoffs for C.1 and the first family.

This packet is written in the order the work is done. Sections 1 to 4 were
written and committed before any measurement returned, so section 4 is the
frozen record; results and calibration are appended below it and never
edited into it.

## 1. Questions

1. **Donor.** Is the evaluation donor classical Stockfish `9587eeeb`
   (`SF_classical`) or Stockfish 16's classical evaluation (`sf_16`,
   `68e1e9b`)? PLAN's default is `sf_16` unless the comparison says
   otherwise.
2. **Size.** How large is the same-search evaluation deficit on this head?
   BAS-O02 read +232.8 ± 32 on 400 games before Phase 5.9's refits, B.10's
   table repair and the verdict repairs.
3. **Values or structure.** BAS-E07 says the deficit is the values of
   features Basilisk already has and that static fitting is exhausted. The
   family clusters C.3 to C.6 assume the donor's conditioning carries it.
   Both cannot be the plan's premise.
4. **Order and budget.** Which family goes first, and what does C expect
   to recover?
5. **Protocol.** Which corpus, which labels and which refit rules can still
   correct a seed error (B.10's lesson)?

## 2. Known evidence

- **The deficit meter.** BAS-O02: Stockfish's own evaluation over
  Basilisk's inside `9587eeeb`'s search, 264-106-30, about +232.8 ± 32.
  One binary, one option.
- **Fitting history.** BAS-E04: four on-policy refresh cycles paid +21.0,
  +19.5, +18.3 and +15.3, and the fifth washed at +1.37 ± 5.21. Phase 5.9:
  sixteen donor terms fitted on a mislabelled corpus lost 77.92 and were
  later deleted after reading inert three times; the two refits of existing
  surfaces (king-danger funnel +2.64, all 768 table entries +9.52) were the
  phase's whole gain. Holdout loss has inverted against Elo twice.
- **Since BAS-O02.** The evaluation gained by repair, not by fit: B.10's
  piece-square orientation (+18.3 ± on its gate), A.13's two truth repairs,
  A.16's removal of the inert winnable term, B.9.5's verdicts. The search
  changed completely: +387 by its cluster gates, G(0) from +312.6 to −70.7
  (BAS-O06). BAS-E04's retry trigger (a material policy change) has fired.
- **Form differences, read not measured**
  (`analysis/code_audit_2026-10-27.md` §6.3): king danger is middlegame
  only and short of the donor's inputs (E1); the phase gives middlegame
  terms a 25–33% share in endings where the donor gives 3–13% (E2); the
  scalers multiply the whole tapered score (E3); passers carry rank five
  times with an unweighted king race (E4); tempo tapers away (E5); five
  fitted signs oppose the donor's (E6); the lazy cut is one stage at 700
  (E8). About ten small donor terms are absent.
- **Rarog's worked example** (priors only): its family units returned about
  +105 and its speed work +42 against a 328 deficit; the king-danger map
  carried 92% of its king residual; material, imbalance and piece rows
  carried no signal; refitting donor coefficients failed where the index's
  structure differed (RAR-E73); the remainder sat in positions the search
  visits, not in quiet game positions (C-124).
- **The two candidate donors differ in more than values.** `9587eeeb`'s
  evaluation adds the search's contempt score, adds a tempo of 28 and damps
  by `(100 − rule50)/100`; `sf_16`'s has no contempt and no tempo, lazy
  thresholds of 3622 and 1962 against 1400 and 1300, a space threshold of
  11551 against 12222, pawn values of 126/208 against 124/206, and is
  damped by `(200 − rule50)/214` outside the classical function. Knight to
  queen values and the phase limits are equal.
- **The evaluation has not moved since the oracle was built.** `git diff
  4d2742f HEAD -- src/eval.cpp` is comments only, so
  `tools/test_engines/oracle-b111-4d2742f.exe` carries the head's
  evaluation.

## 3. Hypotheses

- **H-values (BAS-E07).** The donor's terms and Basilisk's are close enough
  in form; the gap is the values, and those were fitted on a static
  objective that cannot see what search realises.
- **H-structure (PLAN's cluster premise).** The donor's conditioning (the
  king-danger index, the phase, passer geometry, threat gating) holds
  information no refit of Basilisk's forms can express.
- **H-distribution (Rarog C-124, this packet's addition).** Both evaluators
  are about equally good on quiet positions from games, and the gap is in
  the positions a search visits: unbalanced, loose, off the game
  distribution. Then a Texel corpus of quiet game positions is fitted to
  the wrong population whatever the term set, which would explain why
  BAS-E04's line saturated while 232.8 stood.

These are not exclusive. The instruments below are chosen to separate them:
a static loss that already favours Basilisk on game positions refutes the
reading "fit harder on the same data"; a donor family whose terms add
outcome information beyond Basilisk's whole score points at structure in
that family; a deficit that opens only off the game distribution points at
the corpus.

## 4. Registered instruments and predictions — frozen 2026-11-18

Frozen before any result. The measurement agents were still running when
this section was committed; nothing from them had been read.

### 4.1 BAS-E58 — static screens on the head's own games

**Corpus.** Positions from the five most recent `3+0.03` game sets played
by the head or its immediate parents: `tools/results/b95-gate` (8,000
games), `b87-gate` (8,000), `b8-gate` (3,970), `b111-g0-equal` (3,000) and
`b111-g0-odds` (3,000). Games with a normal result and no forfeit, crash or
illegal move. Candidates are positions from ply 9 where the side to move is
not in check and neither the move just played nor the next one is a capture
or promotion; up to 10 per game, uniformly, seed `20261118 + game index`;
duplicates by the first four FEN fields dropped. Each row keeps its game,
ply, result, material counts and Basilisk's phase (0–24).

**Evaluators**, all white-relative:
- `B`: `basilisk-texel --dump-eval`, the full evaluation with the lazy path
  off, centipawns.
- `D95`: the oracle with `Use Basilisk HCE=false`, `eval`: the term table
  (thirteen terms, Total MG and EG) and the total.
- `D16`: `stockfish-16-x64-bmi2-classical-pgo.exe` with `Use NNUE=false`,
  the same table.

**Loss.** `p = 1/(1+exp(−K·s))`, cross-entropy against the result
{0, ½, 1}, K fitted once per evaluator on the rows all three evaluate.
Intervals by bootstrap over whole games; z by a sandwich estimator
clustered on the game.

**Reads.**
- A1: K, cross-entropy and Brier per evaluator, overall and by phase band,
  men band and queens.
- A2: the two-evaluator ensemble `σ(a·B + c·D)` and its gain over each
  member.
- A3: the donor family screen: Basilisk's score as a fixed offset, the
  donor's thirteen terms (MG weighted by phase/24, EG by the rest) as
  features; joint coefficients in units of the donor's own weight, and
  each family's gain alone.
- A4: calibration by phase band.
- A5: activation, `--feature-support`, per parameter group.
- A6: the values screen: `--tune all` with `phase911.ps1`'s options on
  four fifths of the games, read on the held-out fifth, beside the
  ensemble's gain on the same rows. Nothing is baked.
- The scale between the donors: the slope of `D16` on `D95`.

**Predictions.**

| # | Prediction | P |
|---|---|---:|
| P1a | `B`'s cross-entropy is no worse than `D95`'s | 0.60 |
| P1b | `B`'s cross-entropy is no worse than `D16`'s | 0.55 |
| P1c | The three lie within 2% of each other | 0.75 |
| P2a | `B + D95` beats `B` alone, interval excluding 0 | 0.85 |
| P2b | `B + D95` beats `D95` alone, interval excluding 0 | 0.85 |
| P2c | The gain over `B` is 0.3% to 2% of `B`'s cross-entropy | 0.65 |
| P3a | King safety MG: z ≥ 3, positive | 0.70 |
| P3b | Passed EG: z ≥ 3, positive | 0.65 |
| P3c | Threats (either half): z ≥ 3, positive | 0.60 |
| P3d | Winnable EG: z ≥ 3 | 0.60 |
| P3e | Mobility (either half): z ≥ 3, positive | 0.50 |
| P3f | Space MG at phase 18–24: z ≥ 3, positive | 0.40 |
| P3g | At most two of Material, Imbalance, Knights, Bishops, Rooks, Queens reach abs(z) ≥ 3 | 0.60 |
| P3h | By gain alone the order opens King safety, Passed, Threats | 0.35 |
| P4a | `B`'s best K varies by more than 15% across the five phase bands | 0.70 |
| P4b | `D95`'s spread is within 5 points of `B`'s (a property of game length, not of Basilisk) | 0.55 |
| P4c | At six men or fewer `B` trails `D95` by a wider relative margin than overall | 0.65 |
| P5 | Beyond the pawn tables' first and last ranks, at least 15 scalars have under 0.1% support | 0.60 |
| P6a | The refit improves held-out loss by under 0.5% | 0.70 |
| P6b | The refit's gain is smaller than the `B + D95` ensemble's on the same rows | 0.65 |
| P7 | The slope of `D16` on `D95` is between 0.93 and 1.10 | 0.80 |

**What each outcome means.**
- P1 holding (the donor is no better statically on Basilisk's own game
  positions, though it is worth a three-figure Elo in play) says the
  deficit is not visible to a WDL loss on quiet game positions. Fit loss
  then cannot rank the clusters, and H-distribution becomes the lead for
  C.2.2: the follow-up screen compares the same evaluators on positions
  off the game line.
- P1 failing by more than 2% says the deficit is statically visible, the
  family screen ranks the clusters directly, and H-values can be tested by
  fit.
- A3 names the families where the donor's terms carry outcome information
  that Basilisk's whole score lacks. A family with a significant positive
  coefficient is a structure candidate; a family with none is a refit at
  most.
- A6 is weak either way: about 185,000 training rows against 1,178
  scalars and against the 3.5M-row corpora earlier fits used, with the
  tuner's travel bound (T1) in force. A gain above 1.5% would say the
  values have drifted off their optimum on the new search's positions.

**Falsifiers.** A parse check that fails (term rows not summing to the
Total row) voids A3. An ensemble that cannot beat its members at all (P2a
and P2b both failing) would say the two evaluators are the same function
up to scale, which the code audit contradicts: check the pipeline first.

### 4.2 BAS-O08 — the same-search evaluation deficit on the head

**Question.** BAS-O02 on this head (Rarog audit C-102): how many Elo is
`9587eeeb`'s evaluation worth over Basilisk's inside `9587eeeb`'s search?

**Instrument.** One binary, `oracle-b111-4d2742f.exe` (`8ADEE9AE…`), two
option sets: `Use Basilisk HCE=false` against `=true`. A two-engine
gauntlet, 1,500 cycles of one opening played twice with colours reversed,
3,000 games, UHO in random order, `3+0.03`, 1T, Hash 64, concurrency 14,
no adjudication, no tablebases, zero faults. **E(0)** is the
`false` arm's rating over the `true` arm fixed at 1500.

**Confounds.** The `true` arm pays the bridge (2.07% a node, BAS-O07) and
its evaluator's own cost; the meter is at equal time, as BAS-O02 was. The
run's per-arm nodes per second are recorded. If they differ by more than
15%, an equal-node companion read is registered before C's budget row is
rewritten (Rarog audit C-95).

**Prediction.** E(0) = **+190**, 80% band **+135 to +245**. P(E(0) below
BAS-O02's 232.8) = 0.75. Basis: about +30 to +50 of evaluation gains since
BAS-O02 (Phase 5.9's refits, B.10, the truth repairs), read through a
different search.

**Falsifiers.** Below +100 or above +290, check the instrument before
reading: the option must move the oracle's `bench` node count, and both
arms' game meters must look like one search.

**Use.** E(0) replaces 232.8 in PLAN §1's budget row.

### 4.3 BAS-O09 — the donor comparison, each evaluation in both hosts

**Instrument.** Four binaries from pinned Stockfish sources and committed
patches (`tools/donor/`), each pair built with identical flags, no PGO:
- `sf16-stock`: `sf_16` playing its classical evaluation;
- `sf16-eval9587`: `sf_16`'s search with `9587eeeb`'s whole evaluation;
- `sf9587-stock`: `9587eeeb`;
- `sf9587-eval16`: `9587eeeb`'s search with `sf_16`'s whole classical
  evaluation.

"The whole evaluation" is everything `Eval::evaluate` computes: terms, lazy
thresholds, winnable, scale factors, tempo and rule-50 damp as that
revision has them, with its pawn values, tables and endgame functions.
One exception: contempt is computed by `9587eeeb`'s search, so it stays
with that host and is added to whichever evaluation plays there; `sf_16`'s
host has none.

**Port acceptance.** Each hybrid's raw evaluation equals its origin's on
at least 200,000 game positions and 30,000 low-material positions with
zero mismatches; the stock `9587eeeb` build reproduces the commit's bench
of 4,746,616; no search file differs between a hybrid and its host's
stock build; the recipe rebuilds each binary from the patch.

**Design.** Two gauntlets of two engines, hybrid against stock, the form
and conditions of 4.2: 3,000 games a host. Let `a16` be `sf_16`'s
evaluation's advantage in `sf_16`'s search and `a95` its advantage in
`9587eeeb`'s search; `A = (a16 + a95)/2` is the evaluation's quality
averaged over the hosts and `H = (a16 − a95)/2` the hosts' binding to
their own evaluations.

**Decision rule.** `sf_16` is the donor unless `9587eeeb`'s evaluation is
ahead in both hosts and `A`'s 95% interval excludes zero. This is PLAN's
"unless that comparison says otherwise" with "says" read as a result the
games can distinguish from noise: under no difference at all, two signs
agree half the time. Opposite signs are recorded as host-bound values: the
default donor stands, the other is the second seed column, and no "better
evaluation" is claimed.

**Prediction.** `a16` = **+15**, 80% band −10 to +45. `a95` = **−10**,
80% band −40 to +20. P(opposite signs) = 0.50; P(`sf_16` ahead in both) =
0.30; P(`9587eeeb` ahead in both) = 0.20; P(the rule moves the donor to
`9587eeeb`) = 0.08. Basis: each host's margins were tuned around its own
evaluation's scale, tempo and lazy thresholds, and `sf_16`'s classical
values were last tuned with the network on, for the lopsided positions the
classical function was still called in.

**Use.** If the donor is `sf_16`, the meter against the new donor is
E(0) + `a95`: the same search, `sf_16`'s evaluation over Basilisk's.

## 5. Results and calibration

Appended after exposure, in the order the evidence arrived.

### 5.1 BAS-O09's ports: accepted 2026-11-18

Two agents ported in parallel, one a host, and each also built the other
host's stock engine as its private reference. The acceptance below was run
afterwards on engines rebuilt from the committed patches by
`tools/donor/build_donor.py`, each hybrid against the *other* agent's stock
build, on positions neither agent had used (`b87-gate`'s first 800 games
and the verifier's own low-material generator, seeds 7 and 8).

| Engine | Stockfish | Bench | SHA-256 |
|---|---|---:|---|
| `donor-sf16-stock` | `sf_16` | 3,684,832 | `C85B7F82…` |
| `donor-sf16-eval9587` | `sf_16` | 2,360,513 | `C870CD69…` |
| `donor-sf9587-stock` | `9587eeeb` | 4,746,616 | `2F2FE66B…` |
| `donor-sf9587-eval16` | `9587eeeb` | 4,572,257 | `AF84AAFB…` |

- `donor-sf9587-stock` reproduces the commit's own bench signature, and
  `donor-sf16-stock` the count of the classical build already in
  `D:/chess/engines` (`bench 16 1 13 default depth classical`).
- `sf9587-eval16` against `sf16-stock`: 98,019 game positions and 40,000
  low-material ones, 122,320 compared, **0 mismatches**; 99.1% and 96.1%
  differ from the host's own evaluation.
- `sf16-eval9587` against `sf9587-stock`: 122,326 compared, **0
  mismatches**; 99.1% and 96.0% differ from the host's.
- The verifier fails when it should: two stock engines of different hosts
  read 1,594 mismatches of 1,659 (exit 1), and a hybrid checked against a
  control equal to its reference fails the control (exit 1).
- No search file differs between a hybrid and its host, with one declared
  exception. `sf_16`'s lazy cut reads the thread's root score
  (`abs(bestValue) * 5/4` is added to both thresholds), an input its
  search supplies as `9587eeeb`'s supplies contempt. In `sf9587-eval16`
  the root loop's local `bestValue` therefore became the thread member,
  one declaration line in `search.cpp`. The wire is live: with the term
  forced to zero the bench reads 4,824,576, wired 4,572,257.
- Differences found between the two evaluations, beyond about 450 moved
  constants. Only in `9587eeeb`: `RookOnQueenFile`, `QueenInfiltration`,
  `BadOutpost`, a scalar `BishopPawns`, tempo, contempt. Only in `sf_16`:
  `UncontestedOutpost`, `RookOnClosedFile`, `DoubledEarly`, `KingOnFile`,
  `BishopPawns` by file, outpost squares in front of pawns,
  `WeakUnopposed` off the edge files, a two-phase imbalance, a longer
  passer king-proximity ladder, `4 * !pawnsOnBothFlanks` in the scale
  factor.

### 5.2 BAS-E58: the static screens

**Corpus as realised.** 25,970 games, all with normal termination;
254,486 rows from 25,950 games after 4,918 duplicates. `D95` was read from
a plain `9587eeeb` build, because the oracle prints no term table with its
own evaluation; its totals equal the oracle's on 3,000 rows. Term rows sum
to the Total row within 0.03 pawns. Scripts, logs and `dataset.csv`
(`cfee7385…`) are in `tools/results/c0-screen/`.

**A defect in the registered loss, found on the first read.** 9,603 rows
carry one of Basilisk's known-win overrides, about ±30,000 cp. A logistic
saturates on them, so the handful that were not won set Basilisk's K for
every row: K fell from 0.00606 to 0.00428, and the first pass's tables
(Basilisk 5.8% behind `D95`) measured that, not the evaluators. Everything
below is the re-analysis (`c0_reanalysis.py`): override rows and the
donors' specialised-endgame rows are read apart, the 243,108 regular rows
keep each evaluator's own K, and the family screen gives Basilisk's score
a free coefficient where the registration fixed it as an offset. The
predictions are scored against the re-analysis and marked where the change
matters.

**The override rows.** The favoured side won 95.8% at three or four men
and all but two rows above. Every unconverted row is in the two
`b111-g0` sets, none in the three self-play gates: the *oracle*
(Stockfish's search on Basilisk's evaluation) failed to mate with KR-K in
22 games, KBN-K in 15, KBB-K in 7, KQ-K in 3 and KRN-K or KRB-K in 4, 51
of 6,000. That is a property of the instrument, not of the head, and it
costs the `true` arm of BAS-O08 about 3 Elo.

**Regular rows, one K each.**

| Evaluator | K | Cross-entropy | Brier |
|---|---:|---:|---:|
| `B` | 0.006058 per cp | 0.52958 | 0.05784 |
| `D95` | 0.7651 per printed pawn | 0.52597 | 0.05518 |
| `D16` | 1.1772 per printed pawn | 0.52720 | 0.05563 |

- `B` trails `D95` by 36.0e-4 nats [28.2, 44.5], 0.68%, and `D16` by
  23.8e-4 [15.7, 32.7], 0.45%.
- `D95` leads `D16` by 12.3e-4 [9.5, 15.1]: on these positions
  `9587eeeb`'s evaluation is the better predictor of the two.
- Scale: `D16` is 1.017 of `D95` in internal units (1.035 by the K
  route), correlation 0.989. One Basilisk centipawn is worth 1.631
  Stockfish units by outcome, so a donor seed converts at **0.613**, not at
  the nominal 0.485 or B.0's magnitude ratio 0.557.
- `B` and `D95` correlate 0.929 and disagree on the sign in 11.6% of rows.

**By cohort, each evaluator at the cohort's own K** (1e-4 nats; the
ensemble is fitted inside the cohort):

| Cohort | Rows | `B` − `D95` [95%] | Ensemble over `B` | over `D95` | Weights `B`, `D95` |
|---|---:|---|---:|---:|---|
| phase 0–4 | 51,616 | 143.7 [128.9, 162.6] | 157.4 | 13.7 | 0.24, 0.79 |
| phase 5–9 | 65,953 | 94.9 [82.1, 109.3] | 119.7 | 24.8 | 0.33, 0.70 |
| phase 10–14 | 43,841 | −3.0 [−24.7, 13.6] | 59.2 | 62.3 | 0.55, 0.52 |
| phase 15–19 | 23,461 | 16.6 [−10.2, 43.6] | 93.0 | 76.4 | 0.54, 0.56 |
| phase 20–24 | 58,237 | 109.7 [92.3, 126.9] | 126.1 | 16.4 | 0.32, 0.77 |
| men ≤ 6 | 19,500 | 109.5 [91.3, 129.2] | 113.3 | 3.9 | 0.16, 0.85 |
| men 7–12 | 63,553 | 93.5 [78.8, 109.3] | 121.2 | 27.7 | 0.35, 0.69 |
| men 13–22 | 89,210 | 13.0 [−3.9, 28.8] | 87.4 | 74.4 | 0.53, 0.54 |
| men 23–32 | 70,845 | 98.0 [85.3, 111.4] | 114.0 | 16.1 | 0.33, 0.76 |

The deficit has a shape. In the middlegame the two evaluators are equal
and each holds as much the other lacks. At both ends the donor is ahead by
1.5 to 3.4% and Basilisk adds little to it. Weighted by rows the
per-cohort gap is about 84e-4, 1.6%; one global K hides half of it,
because Basilisk's K varies 1.43-fold across the phase bands and the
donor's 1.97-fold. Texel fitting bought Basilisk the flatter calibration,
not more information.

**The family screen** (a donor family's MG and EG rows added to
Basilisk's score, gain in 1e-4 nats over Basilisk at its best K; `D95`,
then `D16` where it differs):

| Band | Families, by gain |
|---|---|
| phase 18–24 (70,543 rows) | Space 47.0 [38, 57] / 50.8; Mobility 29.6 [24, 35]; Pawns 13.9 / 10.0; King safety 11.1 / 11.8; Threats 4.4; all others under 4, Knights 6.0 in `D16` |
| phase 10–17 (54,996) | King safety 15.2 [11, 19] / 15.9; Pawns 9.4 / 7.8; Imbalance 7.3; Knights 3.4 / 7.0; Bishops 3.7 / 4.7; Mobility 3.8; Material 3.3; Threats 1.7; Passed 1.3 |
| phase 0–9 (117,569) | Winnable 14.1 [12, 16] / 14.6; Pawns 11.1 / 11.6; Material 7.9; Passed 1.8; all others under 1.6 |
| all (243,108) | Space 9.0; Pawns 8.2; Material 7.0; King safety 5.0; Mobility 3.5; Imbalance 3.0; Winnable 2.8; Threats 1.5; Bishops 1.4; Knights 1.3; Queens 0.3; Passed 0.1; Rooks 0.0 |

- In the joint fit every donor family is significant (z from 3 to 60)
  while Basilisk's own weight falls to 0.44: with thirteen donor terms
  beside it the fit rebuilds the donor, so the joint z separates nothing.
  The gains alone are the read.
- The white-side intercept at phase 20–24 is +0.075 logit for `B` and
  +0.035 for `D95`: Basilisk under-reads the advantage the unbalanced
  openings give White, which is space and activity.

**Activation.** Of 1,159 traced scalars, 58 never fire and all are
unreachable by construction (pawn tables' first and last ranks, rank 0 and
7 slots, king and empty piece slots) except six `ImbTheir` slots (C-20).
Under 0.1%: five knight corner squares, one queen and one king square,
queen mobility at 0, 1, 2 and 26, `Trapped[0]`. No live term is dead on
this corpus.

**The values screen.** `--tune all`, `--l2 1e-6 --epochs 100`, 203,601
training rows; the tuner's loss is Brier with K fitted on the held-out
rows. Held-out Brier fell from 0.055492 to **0.054062** at epoch 16, 2.58%,
and stood at 0.054086 at epoch 100. 131 scalars moved, by at most 4, which
is the Adam travel bound at that epoch (T1), not convergence. In the same
unit on the same rows, with K fitted on the training rows:

| Model | Held-out Brier | Against `B` |
|---|---:|---:|
| `B` | 0.055492 | |
| `B` refitted | 0.054062 | −2.58% |
| `D95` | 0.052930 | −4.62% |
| `D16` | 0.053254 | −4.03% |
| `B + D95` | 0.051538 | −7.13% |

- A bounded refit of the existing terms closes **56%** of the static gap
  to the donor. The donor's terms beside Basilisk's close 2.8 times what
  the refit does.
- Direction of the refit: every endgame passer term down (`PassFreeEg`,
  `PassSafeEg` −3, `PassedPathSafeEg`, `PassedKingBlockEg` −4), pawn
  weaknesses heavier (`Isolated` −2/−3, `Doubled` −3/−2, `PawnIslands`
  −3/−3), imbalance slots up, `MinorKingRingMg` and `WeakQueenProtMg` −3.
  The passer terms were fitted while the pawn table was rank-mirrored
  (B.10); the flip then paid the advanced pawn twice. This is the
  compensation C.2.8 expects, seen before the corpus exists.

### 5.3 Calibration of BAS-E58

| # | Prediction | P | Outcome |
|---|---|---:|---|
| P1a | `B` no worse than `D95` | 0.60 | **Failed**: 0.68% worse, interval excluding 0 |
| P1b | `B` no worse than `D16` | 0.55 | **Failed**: 0.45% worse |
| P1c | The three within 2% | 0.75 | Held (it fails on the unseparated rows, 5.8%) |
| P2a | `B + D95` beats `B` | 0.85 | Held: 95.0e-4 |
| P2b | `B + D95` beats `D95` | 0.85 | Held: 59.0e-4 |
| P2c | Gain over `B` 0.3–2% | 0.65 | Held: 1.8% |
| P3a–f | King safety, Passed, Threats, Winnable, Mobility, Space reach z ≥ 3 | 0.40–0.70 | Held, all six, and uninformative: every family does |
| P3g | At most two of the six material and piece rows reach z ≥ 3 | 0.60 | **Failed**: all six do |
| P3h | Gain alone opens King safety, Passed, Threats | 0.35 | **Failed**: Space, Pawns, Material; Passed and Threats are last |
| P4a | `B`'s K varies over 15% by phase | 0.70 | Held: 1.43-fold |
| P4b | `D95`'s spread within 5 points of `B`'s | 0.55 | **Failed**: 1.97-fold, far wider |
| P4c | A wider margin at six men or fewer | 0.65 | Held: 2.9% |
| P5 | At least 15 scalars under 0.1% beyond the pawn ranks | 0.60 | Held in count (about 50), empty in meaning: unreachable indices |
| P6a | The refit gains under 0.5% | 0.70 | **Failed**: 2.58% |
| P6b | The refit gains less than the ensemble | 0.65 | Held: 0.00143 against 0.00395 |
| P7 | `D16` on `D95` slope 0.93–1.10 | 0.80 | Held: 1.017 |

What the model had wrong:
- **The deficit is visible statically.** I expected a Texel-fitted
  evaluator to match an outcome-tuned one on its own games' quiet
  positions and the gap to live elsewhere. It is visible there, at both
  ends of the game. H-distribution is not needed to explain a static gap,
  and is still untested for the part of the gap that is not static.
- **The values are not at their optimum.** I took BAS-E04's wash as
  evidence the surface sat at its static optimum. It sat there for the old
  search's positions and the mirrored tables.
- **Where the donor's information is.** I ranked the families by how much
  their value depends on search (king safety, passers, threats). The
  static screen ranks by what Basilisk lacks outright: space, which it
  does not evaluate, then mobility, pawn structure and winnable. Passers
  and threats, where the forms are closest, add nothing.
- **The instrument.** The joint z was the wrong statistic and the fixed
  offset the wrong model; both were replaced after exposure, which is why
  the scoring above says so.

### 5.4 What the static layer decides, and what it cannot

Evidence layer: loss on game results. It ranks and explains; BAS-X02 and
Phase 5.9 forbid reading it as Elo.

- **BAS-E07's "static fitting is exhausted" is refuted on this head.** A
  bounded refit recovers 2.6% of held-out Brier and more than half the
  static gap to the donor. Inference: C.2.8's refit is a strength
  candidate in its own right, not a formality before the families.
- **BAS-E07's "the missing terms cannot carry it" stands for the forms
  nearest the donor's** (passers, threats, rooks, queens) and fails for
  space, mobility, king safety, pawn structure and winnable, where the
  donor's terms carry outcome information Basilisk's whole score lacks.
- So the answer to values against structure is **both, by family**:

| Family | Static read | Consequence |
|---|---|---|
| Space | The largest signal, full board only; Basilisk's space weights are zero | Structure; it joins mobility's cluster, which gives space its owner (C-119) |
| Mobility | Second on the full board | Structure: the area and the tables |
| King safety | First in the middlegame, present early | Structure, as planned (C.3) |
| Pawn structure | 8 to 14 in every band, and the refit moves it | Values first; forms (C-12, C-16, doubled) after the refit shows what is left |
| Winnable and scaling | First in endings | Structure (C.7.3); the phase and scalers (E2, E3) sit behind the same band |
| Imbalance, knights, bishops | 3 to 7 in the middlegame | Screen each term before building it (C-118 holds) |
| Passers | Nothing beyond Basilisk's; the refit lowers every endgame passer term | Values, and the one definitional repair (D2); no form port on this evidence |
| Threats | 1.5 to 4.4 | No static case; kept in its cluster for the search-realised part the screen cannot see |
| Rooks, queens | Nothing | No form work on this evidence |

- **The corpus contract gains two rules** (C.2.1, C.2.2): override and
  specialised-endgame rows take no part in K or in the loss, and K is
  reported per phase band beside the global one.
- **H-distribution stays open.** The part of the meter this layer cannot
  see is tested where its answer is used: C.2.2's first read compares the
  three evaluators on positions one move off the game line against a
  strong reference, beside the same game positions under the same
  reference.

### 5.5 BAS-O08: the same-search meter

Run by the maintainer 2026-11-18, `tools/results/c0-meter`: 3,000 games,
no faults, no time losses. `tools/diag/colosseum_recount.py` recounts the
same pentanomial from the PGN (exit 0).

- `9587eeeb`'s evaluation over Basilisk's inside `9587eeeb`'s search:
  1940-785-275, 77.75%, **E(0) = +217.3 [+207.1, +228.0]** (pentanomial
  [5, 35, 299, 612, 549]); as a rating +216.8 ± 14.9.
- Speed. The Basilisk arm ran 2.50M nodes a second against 2.85M: 12.3%
  slower, or 14.0% counted from the slower arm. It also spent more time a
  move (70.7 ms against 65.5), so it searched 5.5% fewer nodes a move
  (177,143 against 186,902) and 0.4 ply less (17.62 against 18.02 without
  mate scores). The registered threshold for an equal-node companion is
  15%; it is not reached by either count and none is registered. E(0) is
  an equal-time figure and contains whatever that speed is worth. This
  packet does not convert it.
- The bridge costs 2.07% a node (BAS-O07), so about ten of the twelve
  points are the evaluator's own cost inside this search: an input for
  C.12, at this evidence layer only.
- Sixteen of the Basilisk arm's draws are elementary mates it could not
  play (§5.8). Counted as wins they would make E(0) +214.6.

| Frozen | Outcome |
|---|---|
| E(0) = +190, 80% band +135 to +245 | Held: +217.3, 27 above the point |
| P(E(0) below 232.8) = 0.75 | Held |
| Falsifiers: below +100 or above +290 | Not reached; the wire check passed at launch |

What the model had wrong: I took +30 to +50 of evaluation gains since
BAS-O02 off its 232.8. The meter moved by 15.5 and BAS-O02's own interval
is ±32, so nothing can be said about how much of those gains this search
sees. The usable result is the new baseline: +217.3 ± 10.5, on this
binary, is what every later re-read of the meter is compared with.

### 5.6 BAS-O09: the donor comparison

Run by the maintainer 2026-11-18, `tools/results/c0-donor-sf16` and
`c0-donor-9587`: 3,000 games each, no faults, no time losses, recounts
agree (exit 0).

| Host | Hybrid against stock | W-D-L | Hybrid's Elo [95%] | Pentanomial |
|---|---|---|---|---|
| `sf_16`'s search | `9587eeeb`'s evaluation | 784-1464-752 | +3.7 [−3.5, +10.9] | [33, 355, 696, 379, 37] |
| `9587eeeb`'s search | `sf_16`'s evaluation | 826-1334-840 | −1.6 [−9.4, +6.1] | [56, 374, 648, 372, 50] |

- In the registered terms, `a16` = **−3.7** [−10.9, +3.5] and `a95` =
  **−1.6** [−9.4, +6.1]: `sf_16`'s evaluation is nominally behind in both
  searches.
- `A` = **−2.7** [−8.0, +2.6], the evaluation's quality averaged over the
  hosts. `H` = **−1.0** [−6.3, +4.3], the hosts' binding to their own
  evaluations.
- `9587eeeb`'s evaluation is about 2% faster a second in either host
  (2.80M against 2.75M, 2.88M against 2.82M).
- One host effect was found, outside the margins (§5.8): `sf_16`'s
  evaluation in `9587eeeb`'s search cannot mate with a rook, and drew
  seven such endings. That is about 0.8 Elo of `a95`.

**Decision by the frozen rule: `sf_16` stays the donor.** `9587eeeb`'s
evaluation is ahead in both hosts, but `A`'s interval includes zero, so
the comparison does not say otherwise. No better evaluation is claimed.
What the games do say is narrower and firm: in play the two evaluations
are equal within about 8 Elo in either search, and neither search favours
its own evaluation.

| Frozen | Outcome |
|---|---|
| `a16` = +15, 80% band −10 to +45 | Held in the band: −3.7, the sign missed |
| `a95` = −10, 80% band −40 to +20 | Held: −1.6 |
| P(opposite signs) = 0.50 | The same sign |
| P(`9587eeeb` ahead in both) = 0.20 | This outcome, nominally |
| P(the rule moves the donor) = 0.08 | It did not |

What the model had wrong: I expected each search to favour the evaluation
it was tuned around, by about 12 Elo (`H`). There is no binding at this
control: the two evaluations differ by 1.7% in scale, and a search's
margins do not notice that. PLAN's caution that a host favours its own
evaluation can only be about scale, which B.0 measured for Basilisk's.
And five releases of tuning the classical evaluation beside a network
neither helped nor hurt it as a classical evaluation.

Consequences:
- **The seed.** C.2.5's piece-square seed stays `sf_16`'s, as decided
  2026-11-18. `9587eeeb`'s values are the second seed column.
- **The meter against the donor** is E(0) + `a95` = **+215.7**
  [+202.7, +228.7]. The oracle's own evaluation stays a valid stand-in for
  the donor's: the two differ by −1.6 ± 7.8 in that search.
- **Forms.** Where the two evaluations differ in form, the family's
  research reads both and the family screen is the tiebreak for a term.
  Today `sf_16`'s outpost forms read twice `9587eeeb`'s on the knight row
  (7.0 against 3.4), and `9587eeeb`'s pawn row reads 13.9 against 10.0 on
  the full board.
- **Tempo** exists only in `9587eeeb`'s evaluation, so C.3's tempo
  question takes its form (a constant added after scaling) as the only
  donor form.
- **Rule-50 damping.** `sf_16` damps specialised endings with everything
  else; that is the form that failed in the older search. Basilisk's
  curve leaves a known-win tier's base undamped (`damp_rule50`,
  `eval.h`) and stays.

### 5.7 Where the meter's advantage is realised (exploratory)

Not registered. The one guess written before the read was that phase 10
and above would carry more than half (0.70) and phase 20–24 about a
quarter. Script and output: `tools/results/c0-screen/c0_game_shape.py`,
`c0_game_shape.json`; its arguments are the calibration run and engine
(`tools/results/c0-donor-9587`, "9587 search 9587 eval"), the meter run
and engine (`tools/results/c0-meter`, "Oracle 9587 eval") and the output
file.

**Method.** The focal engine is the meter's `9587eeeb`-evaluation arm.
Its own search score at each of its moves becomes an expected score
through a logistic fitted, per phase band, on the same search and
evaluation playing an equal opponent (BAS-O09's `9587eeeb` host). The
change between two of its consecutive moves is credited to the phase of
the earlier move, and the last step to the result. Over a game the
credits sum to the result minus the expected score at its first move, so
the band means decompose the meter's excess score, 0.266 a game. Between
equals every band should read zero; the calibration run is the control.
The calibration engine plays with contempt 24 and the oracle with 0, so
four calibrations were run (bands, bands with an offset, interpolated,
one global curve). Intervals are a bootstrap over opening pairs.

| Basilisk phase | Focal moves | Credit a game [95%] | Share | Range over the four calibrations | Control, worst case |
|---|---:|---|---:|---|---:|
| 20–24 | 54,552 | 0.171 [0.163, 0.179] | 64% | 62–64% | +0.024 |
| 15–19 | 20,891 | 0.022 [0.017, 0.027] | 8% | 8–9% | −0.011 |
| 10–14 | 28,825 | 0.031 [0.027, 0.036] | 12% | 12% | −0.009 |
| 5–9 | 41,345 | 0.022 [0.018, 0.026] | 8% | 8–10% | −0.014 |
| 0–4 | 50,444 | 0.020 [0.016, 0.024] | 7% | 7% | −0.010 |

- **Two thirds of the meter is realised while nearly every piece is on
  the board.** In the family screen's bands: phase 18–24 holds 68–69%,
  10–17 15–16%, 0–9 16%.
- It is not a ceiling effect. Among games that enter a band undecided
  (expected score 0.3 to 0.7), the focal arm gains 0.187 ± 0.008 a game
  inside phase 20–24 against 0.051, 0.073, 0.066 and 0.044 in the later
  bands; per hundred moves 1.04 against 0.64, 0.56, 0.31 and 0.09. The
  control reads within ±0.024 on the same statistic.
- Limits. A mistake shows in the score some plies after it is made, which
  moves credit later, not earlier. UHO openings start unbalanced, so the
  first band is also where an opening advantage is kept or given back.
  Every gate in this project plays the same book. It is one search, one
  clock, one thread (TM-5).

What it changes: the static screen said the evaluators are level at phase
10–19 and Basilisk trails at both ends. The games say which end pays: the
full board. The endgame bands hold 48% of the quiet rows and 16% of the
meter.

### 5.8 Elementary mates that were drawn

`tools/results/c0-screen/c0_bare_king_draws.py` lists drawn games that
ended with an elementary mate against a bare king on the board.

| Run | Engine holding the mate | Fifty-move draws | Stalemates on the move that bared the king |
|---|---|---:|---:|
| Meter | Oracle, Basilisk's evaluation | **16** (KR-K 15, KBB-K 1) | 0 |
| Meter | Oracle, `9587eeeb`'s evaluation | 0 | 1 |
| `9587eeeb` host | `sf_16`'s evaluation | **7** (KR-K 4, KBN-K 2, KRB-K 1) | 0 |
| `9587eeeb` host | its own evaluation | 0 | 1 |
| `sf_16` host | either evaluation | 0 | 5 |

- **The oracle bridge clamps Basilisk's known-win scores.** In the games
  inspected the oracle reports 4853 cp, unchanged, for all hundred plies:
  9,999 of Stockfish's units, one under its `VALUE_KNOWN_WIN`, where the
  bridge clamps by design (`hybrid_eval.cpp:74` at the `oracle/hybrid`
  tag). On `8/8/8/6K1/8/8/4k3/2R5 w` it reads 4853 at depth 20 and never
  a mate. A flat score has no gradient to mate along. Basilisk's own
  search reads the tiers above that value and converted every such row in
  three gates (§5.2). This is the instrument, not the evaluation: 2.7 Elo
  of E(0), and about 3 Elo in favour of the head in B.11.1's two G(0)
  reads (51 of 6,000 games).
- **`sf_16`'s evaluation cannot mate in `9587eeeb`'s search.** On the
  same position `sf9587-eval16` reads 4961 cp at depth 22 where
  `9587eeeb`, `sf_16` and `sf16-eval9587` each find a mate in 17 to 19.
  `sf_16` damps specialised endings by the halfmove clock and `9587eeeb`
  does not: the KR-K score there is 11,412 at any clock, and `sf_16`'s is
  under 10,000 from clock 13 (6,399 at clock 80). The port is as
  registered: the evaluation is `sf_16`'s on 122,320 positions, damping
  included.
- **One cause fits both**, inferred from the code and the probes, not
  isolated: `9587eeeb`'s search returns a static score from a shallow
  node unless it is at or above `VALUE_KNOWN_WIN` (`search.cpp:821`, "do
  not return unproven wins"). The clamped score and the damped one both
  sit below it, so the won ending is cut off as an ordinary good position
  and the mate is never searched.

## 6. Programme shape so far — provisional until the game reads return

Written 2026-11-18 with the static layer in hand and BAS-O08 and BAS-O09
prepared but not played. Nothing here is a handoff yet. The meter sets the
budget, the donor read sets the seed column, and both can move the order.

### 6.1 Order

1. **C.1**, the restructure, unchanged. Its module boundaries follow the
   clusters below, so space is built beside mobility.
2. **C.2**, with C.2.8's refit of the current surface now a strength
   candidate and the programme's first game test of the values reading.
   The registration is C.2.8's; the prior from this packet is a gain in
   the +15 to +40 range, on the evidence that a bounded fit already moves
   held-out Brier 2.6% and that the endgame passer terms carry B.10's
   compensation.
3. **C.3**, king safety with the phase and tempo, first among the
   families. The reason is structural and the static layer does not
   overturn it: the phase re-weights every MG and EG pair, so every later
   refit must read the final one. King safety is also the leading family
   in the middlegame band, and the endgame band's deficit (2.2 to 3.4%)
   sits behind the phase and the scalers.
4. **C.4**, renamed in effect to mobility, space and threats. Space gets
   its owner here (C-119): it shares the mobility area and the pawn attack
   spans, and with mobility it is the largest static signal on the full
   board (47 and 30 against king safety's 11).
5. **C.5**, pawns and passers. Pawn-structure forms are decided on what
   the refit leaves. Passers get the definitional repair D2 and their
   values; no form is ported unless the re-read shows a residual.
6. **C.6**, material, imbalance and pieces, screen first (C-118): a term
   is built only if its gain alone excludes zero on the head of that day.
   Today's candidates are imbalance, the outpost forms (`sf_16`'s reads
   twice `9587eeeb`'s on knights) and the bishop terms. Rook and queen
   rows show nothing.
7. **C.7**, endgames. Winnable and scaling have static support (first in
   endings); the order inside C.7 is its own leaf's.

**A decision point is added after C.2.8's gate.** The family screen is
re-read on the refitted head (C-84). A family whose gain alone has fallen
to an interval that includes zero closes `NO_CHANGE` without a port. That
is the structural stop rule (C-110), stated with its statistic (C-101).

### 6.2 Shared inputs for C.1

One producer, computed once after the lazy checkpoint and read by every
family module: the attack maps (`attacked_by[colour][piece]`, `attacked`,
`attacked2`), the mobility area, the king ring and zone, the blockers and
pins, and the pawn sets the donor's space and outposts read (pawn attack
spans, squares behind own pawns). The passer set stays in the pawn cache.
Consumers: mobility, space, threats, king danger, passer path safety,
outposts. The lazy checkpoint stays where it is, before the producer.

### 6.3 Protocol rules this investigation adds

- Override rows (Basilisk's known-win scores) and the donors'
  specialised-endgame rows take no part in fitting K or in any loss
  (C.2.1). The first pass here was wrong by a factor of eight until they
  were separated.
- K is reported per phase band beside the global K (C.2.1, C-92).
- A donor seed converts at 0.613 Basilisk centipawns per Stockfish unit,
  the outcome-equivalent ratio on 243,108 rows (C.2.5).
- The travel bound was binding at the best epoch (T1): the bounded fit
  here is a floor on what a converged fit recovers.
- The screen is an instrument, not a one-off: `c0_reanalysis.py` and the
  dataset builder in `tools/results/c0-screen/` are the prototype of a
  committed tool, owed before the first re-read (C.2.1 or a leaf of its
  own; the maintainer's choice when C.2 opens).
- Recent gate games are a corpus that costs nothing: 25,970 games gave
  254,486 rows at the deployment clock. C.2.2 decides whether they serve
  as validation beside corpus A.
- H-distribution is C.2.2's first read (§5.4).

### 6.4 Still owed by C.0 after the reads

The donor decision and its consequences for C.2.5's seed; the budget row;
the family map refreshed on this head; the frozen handoffs for C.1 and for
the first family; the PLAN edits for C.4's contents and the decision point.

## 7. Decisions

Written 2026-11-18 after the game reads. This section supersedes §6 where
they differ; §6 stays as the record of what was provisional.

### 7.1 The five questions

1. **Donor: `sf_16`'s classical evaluation**, by the frozen rule, with
   `9587eeeb`'s as the second seed column. In play they are equal within
   8 Elo in either search (§5.6).
2. **Size: +217.3 [+207.1, +228.0]** against `9587eeeb`'s evaluation and
   +215.7 against the donor's, in `9587eeeb`'s search at `3+0.03`.
3. **Values or structure: both, by family** (§5.4), and the games add
   where: two thirds of the deficit is realised on the full board (§5.7).
   BAS-E07's "fitting is exhausted" is refuted on this head, and its "the
   missing terms cannot carry it" holds only for the forms nearest the
   donor's.
4. **Order and budget:** §7.3 and §7.4.
5. **Protocol:** §7.5.

### 7.2 The family map on the 1.11.0 head

It starts from the code audit's inventory (§6.3 there, read at `fe20f09`).
Since then A.16 removed the inert winnable term and the hanging-piece
loop, A.13 repaired the pawnless scaler, B.10 flipped seven piece-square
tables, and the imbalance gained a cache. Static gains are §5.2's, in
1e-4 nats over Basilisk's score at its best K.

| Family | Basilisk on the head | Donor form that differs | Static gain alone | Reading | Cluster |
|---|---|---|---|---|---|
| Space | Three linear middlegame counts; two weights 0, one −2 against the donor's sign (E6); no material gate, nothing for squares behind own pawns (E8) | A count weighted by the square of the piece count, gated by non-pawn material | 47.0 at phase 18–24 (50.8 for `sf_16`), 9.0 over all rows: the largest | Structure; absent in effect | With mobility |
| Mobility | The donor's area on plain occupancy; no x-rays, no pin line; tables near their linear seeds | X-rays through queens and rooks; the pin line | 29.6 at 18–24 | Structure | With space |
| Threats | The donor's forms; safe-pawn threat flat; no slider or knight on queen | Graded safe-pawn threat; two queen threats | 4.4, 1.7, 1.5 | No static case; each term screened before it is built | With mobility |
| King danger | Middlegame only; bishop and queen attacker units fitted 0; plain-occupancy inputs (E1) | The donor's index, its endgame share, x-ray inputs | 15.2 at 10–17, 11.1 at 18–24 | Structure | C.3 |
| Shelter, storm | Wing kings only, linear, no castling alternative (C-15) | By file and rank, with the castled alternative | Inside the king row | Structure | C.3 |
| Phase | Piece counts, 24 steps; middlegame share 25–33% in simplified endings against the donor's 3–13% (E2) | Non-pawn material between two limits | Not a family row | Changes with the endgame king danger and tempo, never alone | C.3 |
| Tempo | 22, middlegame only, tapered and scaled; absent on the lazy path (E5) | `9587eeeb`: 28 after scaling; `sf_16`: none | Not separable statically | Decided in C.3 | C.3 |
| Pawn structure | Backward, doubled and supported forms differ (C-12, C-16); islands and majority are Basilisk's own | `sf_16` adds `DoubledEarly`; weak lever | 8.2 to 13.9 in every band; the refit moves it | Values first; forms on what the refit leaves | C.5 |
| Passers | Rank carried five times (E4); path term not gated on the stop square (D2) | King proximity by rank; a longer ladder in `sf_16` | 0.1 over all rows | Values and D2; no form port on this evidence | C.5 |
| Material, tables | PeSTO-derived tables in their correct orientation since B.10, never fitted there | `sf_16`'s tables are the seed (C.2.5) | 7.0 over all rows, 7.9 at 0–9 | Values | C.2 |
| Imbalance | Quadratic with a linear slot; six `ImbTheir` slots never fire; four bishop-pair carriers (C-20) | Two-phase in `sf_16` | 7.3 at 10–17 | Screen first | C.6 |
| Knights, bishops | Knight outposts on ranks 5–8 (C-10); no bishop outpost; linear bishop pawns | `sf_16`: uncontested outposts, bishop pawns by file | Knights 3.4 (`sf_16` 7.0), bishops 3.7 (4.7) at 10–17 | Screen each term | C.6 |
| Rooks, queens | Close to the donor; closed file, seventh rank and connected rooks are Basilisk's own | `sf_16`: rook on a closed file | 0.0 and 0.3 | Nothing to build | — |
| Winnable | Removed by A.16, where it was inert | The donor's complexity term | 14.1 at 0–9, first in endings | Structure, with its own instrument (T5) | C.7.3 |
| Scale factors | Applied to the whole tapered score (E3); no pawn-count scale | Endgame half only; pawn count; `sf_16` adds one for pawns on one flank | Not a family row | See below | C.7 |
| Lazy cut | One stage at 700, never refit (C-98) | Two stages; `sf_16` adds the root score | — | A margin block after every unit | Every unit, C.12 |

- **The endgame bands are not one family's.** At phase 0–9 the single
  families gain at most 14 each, while Basilisk trails the donor by 95 to
  144 and the ensemble all but replaces Basilisk's score (weights 0.24 to
  0.33 against 0.70 to 0.79). Inference: that gap is in how the endgame
  score is put together (values across families, the taper, scaling on
  the endgame half), which no additive family row can show. It is also
  the smaller part of the meter (16%).
- **The zero groups** (A.2.3's 23). Thirteen are gone: the seven
  complexity groups, `HangPen` and the five inert king-danger inputs left
  with A.16. Ten remain. `SpaceMg` and `SpacePieceMg` go to the mobility
  and space cluster. The seven fitted-zero halves (`PassFreeMg`,
  `DoubledMg`, `ConnectedEg`, `RookQueenFileEg`, `MinorBehindPawnEg`,
  `KingProtectorNEg`, `KingProtectorBEg`) are values: the screening refit
  already moved `DoubledMg` to −3 and two others off zero. `ProxBase` is
  not inert.
- **E6's opposite signs.** In the screening refit `KingProtectorNMg`
  crossed to the donor's sign (+1 to −1) and the space weights moved its
  way (0, 0, −2 to 1, 2, −1). `RookKingRing` did not move and
  `RestrictedEg` moved away (−4 to −5). The directional screen before
  C.2.8's bake reads them again on a converged fit.
- **Activation.** No live term is dead on game positions (§5.2).

### 7.3 Order by expected value

**Proposed, for the maintainer's sign-off: the mobility and space cluster
goes before king safety.** PLAN's order stands until then.

1. C.1, the restructure.
2. C.2, with C.2.8's refit as the first strength candidate.
3. The family screen re-read on the refitted head (the decision point).
4. **Mobility, space and threats** (today's C.4).
5. **King safety, phase and tempo** (today's C.3).
6. Pawns and passers (C.5).
7. Material, imbalance and pieces, screen first (C.6).
8. Endgames (C.7).

Why:
- Two thirds of the meter is realised at phase 18–24 (§5.7).
- In that band the donor's space term carries 47 and its mobility 30,
  against king safety's 11 (§5.2). Space is the one family Basilisk does
  not have in any working form.
- Space is small to build, so it is also the cheapest test of whether the
  static screen can rank clusters at all.

The counter-argument, which is PLAN's of 2026-11-05: the phase re-weights
every middlegame and endgame pair, so families fitted before it are
fitted again after it. That costs one refit, which every cluster runs
anyway. It does not make the earlier cluster wrong: the donor's space is
gated by material, not by the phase. King safety also has a case the
static layer cannot see, since a danger term pays through the attacks a
search then finds.

What would change it:
- The re-read after C.2.8. If space and mobility no longer lead the
  full-board band on the refitted head, the screen of that day orders the
  families.
- The first family's gate. If it pays under +10, the static screen is not
  a ranking instrument; the rest of the order reverts to PLAN's and to
  the code audit's ranking.

Constraints kept: the phase, the endgame king danger and tempo change
together; the endgame cluster follows pawns and pieces; a family whose
gain alone includes zero on the refitted head closes `NO_CHANGE` without
a port (C-110, with its statistic).

### 7.4 Budget

| | Was | Now |
|---|---|---|
| Measured deficit | 232.8 ± 32 (BAS-O02, 400 games) | **217.3** [207.1, 228.0] (BAS-O08); 215.7 against the donor |
| Planned recovery | 60–120 | **80–140** |

Basis for the band, as priors that can be wrong: C.2.8's refit +15 to
+40; mobility and space +20 to +50; king safety with the phase +15 to
+40; pawns +5 to +15; pieces 0 to +10; endgames +5 to +15; later refits
and the joint fit +5 to +20. They are not additive, so the band is below
their sum. Rarog's units recovered 32% of its deficit (C-124); this band
is 37 to 65% of Basilisk's, on the evidence that the values are off their
optimum and one family is missing outright. Speed stays its own row.

The first two gates test it: a C.2.8 gate under +10, or a first family
under +10, puts the expectation in the band's lower half.

### 7.5 Corpus, refit and label protocol

What PLAN's C.2 leaves already hold stands. This investigation adds:

**Corpus.**
- Rows carrying a known-win override or a draw verdict are tagged at
  extraction and take no part in K or in any loss. They are reported as
  their own table, by conversion (C.2.1). Left in, they set K: 0.00606
  became 0.00428 here.
- The gate games are a corpus that costs nothing: 25,970 games gave
  254,486 rows at the deployment clock. They are spent as a test set for
  the questions §5.2 asked of them; they can serve as validation
  (C.2.2).
- Games played by the oracle are not fitting data: its Basilisk arm draws
  won endings (§5.8).
- Composition. 48% of quiet game rows are at phase 9 or below, where 16%
  of the meter is. Equal phase reservoirs (C-27) weight the endings
  further. C.2.2 registers the composition with that in view.
- **H-distribution is C.2.2's first read**: the three evaluators on
  positions one move off the game line, beside the same game positions,
  against one strong reference. It decides whether corpus A needs
  positions a search visits and a game does not.

**Refit.**
- K is fitted once on validation, over regular rows, and frozen (C-49).
  Every fit report also gives K by phase band. Basilisk's varies
  1.43-fold over the bands and the donor's 1.97-fold, so a spread is not
  a defect by itself.
- Held-out loss is reported by phase band, not only overall: the deficit
  has a shape and a fit can close one end while opening the other.
- A fit whose best epoch is its last, or whose travel reaches the Adam
  bound, fails (T1). The screening fit here stopped at epoch 16 with 131
  scalars at the bound, so its 2.58% is a floor. It is also selected on
  the held-out fifth, so it is a screen, not a test result.
- **How a fit can still correct a seed error** (B.10's input). Before
  every bake, a screening fit from the same start with the anchor off for
  the groups under test. For each group it reports the mean signed
  movement and the share of members moving one way. A group that moves as
  a whole is read before the anchored fit runs, and the anchor is sized
  so that such a group can travel as far as a free fit moved Rarog's
  tables (C-66). The first such group is already visible: every endgame
  passer term moved down together (§5.2), which is the compensation the
  mirrored pawn table left behind.
- The fit reads the played function, the lazy path included (C-5); the
  screen here read the full score only.

**Labels.**
- The game result, exactly 0, ½ or 1. No blended targets (C-67).
- Tablebase relabelling only as corpus B's matched arm (C.2.4), never in
  a strength gate.
- Self-play by the head converts its known wins, so its labels carry no
  conversion noise there; natural termination still multiplies mates
  (C-56), and the tree read is registered with corpus A.

**The seed scale.** A donor value converts at 0.613 Basilisk centipawns a
Stockfish unit by outcome, against B.0's 0.557 by magnitude. C.2.5
decides between them with the fixed-K screen (C-23); they differ by 10%,
which a refit absorbs for a table and does not for a threshold.

### 7.6 Instruments: what was built, and who owns the rest

| Input | Disposition |
|---|---|
| Donor-direction residual screen (C-79 to C-88) | Built here as a prototype: the family screen by band, `c0_reanalysis.py`. A committed tool is owed before its first re-read |
| The same-search meter (C-102) | BAS-O08: +217.3 |
| Calibration by phase (C-92) | Read here; in every fit report from C.2.1 |
| Term inventory by firing share (C-93) | Read here; the untraced remainder is split in C.1 (C-90) |
| Played and full scores both dumped (C-89) | Not built; C.2.1's extractor |
| Static score against the engine's own re-search (C-91) | Not built; it is the reference in C.2.2's H-distribution read |
| Concept residual screen (TE-1) | Not built. Donor-free, so it can re-read a family after the donor's form is in; with the committed screen tool |
| Fixed-depth tree read (C-96), a refit grows the tree (C-72) | Each cluster's qualification |
| Magnitude contract (C-97), fixed-K table screen (C-23) | C.2.8's registration |
| Phase regime check (TE-2) | C.3's research |
| Rules for every unit (C-95, C-98, C-99, C-107 to C-110, C-101) | The cluster contract, §8.2 |
| Screen before building pieces (C-118) | Holds: weak signals on imbalance, knights and bishops, none on rooks and queens |
| Space has no owner (C-119, E8) | The mobility and space cluster |
| Transient material, remaining game length (C-77, C-78) | C.2.2 |
| The remainder in search-visited positions (C-124) | H-distribution, C.2.2 |
| Regime (TM-5) | Every number here is `3+0.03`, one thread, UHO; the meter is `9587eeeb`'s search and the screens are the head's own games |

## 8. Handoffs

### 8.1 C.1 — evaluation restructure: `READY_FOR_IMPLEMENTATION`

**Goal.** `src/eval.cpp` (2,117 lines; `Evaluator::evaluate_full` is
1,156 to 2,050) becomes family modules around one shared-input producer.
No served score changes.

**Modules**, following the clusters so that each later cluster edits one
place:

| Module | Holds today's | Cluster |
|---|---|---|
| Material | Material and tables, the phase, the imbalance and its cache | C.2, C.3, C.6 |
| Pawns | The pawn cache, the passer terms inside and outside it, rook behind a passer, king proximity and path | C.5 |
| Pieces | Bishop pair, rook and knight terms, the small positional terms, the trapped bishop | C.6 |
| Attack producer | Attack maps, king zones, blockers, the cached slider attacks, the mobility area | Shared |
| Mobility and space | The mobility tables' accumulation, the three space terms | Mobility and space |
| Threats | Pawn threats and the threats package | Mobility and space |
| King | The danger index and its table, shelter and storm | C.3 |
| Endgame | `apply_endgame`, the KPK table, the drives, the rook-ending scales, the mate drive | C.7 |
| `evaluate_full` | The order of calls, the lazy checkpoint, tempo, the taper, the audit | C.3 for tempo |

**The producer.** One struct on the stack, filled once after the lazy
checkpoint and read by every module: `attacked_by[colour][piece]`,
`attacked`, `attacked2`, `king_zone`, `blockers_for_king`, the slider
attack cache, the mobility area per colour, the pawn attacks and passed
sets from the pawn cache, and the two king-attacker accumulators
(`attack_units`, `n_attackers`). The knight and slider sweep stays one
pass: it builds the maps, counts mobility and counts king attackers
together, as it does today. C.1 moves inputs; it adds none. Inputs a
donor form needs (x-rays, the pin line, pawn attack spans, squares behind
own pawns) arrive with their cluster.

**What must stay true.**
- Every served score is identical, on the lazy path and the full one.
- The three reads of the running sum keep the terms before them: the
  lazy checkpoint (`eval.cpp:1345`) reads the cheap block only; the mate
  drive (`:1983`) reads everything but tempo and itself; the final taper
  (`:2029`) reads all. The order around the lazy return is documented
  where it is enforced (C-6).
- `apply_endgame` runs on both paths, and the draw verdict is set on
  both.
- The lazy audit of the tune and diagnostic builds stays
  behaviour-identical.
- `EvalTrace` means what it means now: every `TR_MG` and `TR_EG` slot
  stays with its term, `--verify` is exact, the reconstruction test
  passes. The untraced remainder is reported by source (C-90); the mate
  drive is in it today, by its own comment.
- The pawn cache is a function of the pawns alone and the imbalance cache
  of the ten counts alone (C-105).
- No parameter is renamed, added or removed, and no value changes.

**Proof, cheapest first.**
1. `bench 13` reads 4,777,412.
2. A score hash over at least 250,000 positions is equal before and
   after, for the served function and for the traced one (C-104).
   `tools/results/c0-screen/dataset.csv` (254,486 rows, `cfee7385…`) is
   on this machine; a committed position source is the implementer's
   choice.
3. The existing suites: `test_eval` (the symmetry sweep included),
   `test_endgames`, the verdict instruments, the tuner's `--verify`.
4. New tests: cache purity (C-105), and one that fails when a term is
   moved across the lazy checkpoint.
5. Pooled NPS within noise. Build the pools, then stop and ask the
   maintainer for an idle machine; do not start the read.

**Files that list the evaluator's sources**, all of which follow the
split: `CMakeLists.txt` (the engine library at line 229, the tuner target
at 706), `tools/oracle/build_oracle.ps1:82`, and
`tools/diag/profile_regions.json:24` with its test.

**Falsifiers.** Any bench or hash difference is a defect in the
restructure, repaired, never accepted. If separate translation units cost
speed beyond noise, the modules become headers of one unit; the split is
never traded for speed or speed for the split.

**Out of scope.** Any term, value or margin; the tuner beyond the
remainder's report; the oracle bridge's clamp (§5.8).

### 8.2 The first family — the cluster contract

This freezes what C.0 knows for the first family's research step. Which
family that is waits on §7.3's sign-off; both cases are given.

**Common to every family cluster.** It opens as research, implementation,
refit and gate. Its research step delivers the design PLAN's *Cluster
shape* asks for, and:
- reads the family screen on that day's head, by band, for both donor
  evaluations, and names each term it will build with its gain alone;
- classes each change as a correctness repair, a definitional repair or a
  new form (C-107), and compares each with the incumbent (C-108);
- seeds a new term at 0 behind the lazy gate with a post-refit falsifier
  (C-109);
- registers the gate with its equal-node companion for a failed
  equal-time gate (C-95), the lazy-margin block (C-98), and the NPS and
  `10+0.1` reads (C-99);
- states the stop: a term whose gain alone includes zero is not built
  (C-110).
Its gate is an SPRT at `3+0.03`, `[0,3]`, or `[3,10]` where the screen's
gain is large; fit loss never accepts (BAS-X02).

**If the mobility and space cluster is first** (proposed).
- Measured defect: at phase 18–24 the donor's space term adds 47.0
  [38, 57] and its mobility 29.6 [24, 35] to Basilisk's whole score; that
  band holds 68–69% of the meter. Basilisk's space weights are 0, 0 and
  −2, and the screening refit moved all three the donor's way.
- Leading hypothesis: the missing information is the donor's *form* of
  space (squares behind own pawns, weight by the square of the piece
  count, the material gate), which three linear counts cannot express.
- Competing: (a) the same information is in the piece-square tables once
  they are refitted in their right orientation (C.2.8), and space reads
  nothing afterwards; (b) the gain is the unbalanced book's, and a
  balanced book would not show it; (c) mobility's gain is the area's
  values, not the x-rays.
- First reads, cheapest first: the family screen on the refitted head,
  which tests (a); the same screen on the positions from a balanced
  book's games if any exist, which tests (b); for mobility, the donor's
  tables on Basilisk's area against the donor's own, which tests (c).
- Scope: the mobility area and its tables, x-rays and the pin line, the
  donor's space, and of the threat terms only those whose screen excludes
  zero; the flat safe-pawn threat is a definitional repair. E6's
  `space_blocked_mg` sign is settled by the new form, not patched.
- Shared inputs it adds to the producer: the squares behind own pawns,
  the blocked-pawn count, x-ray attack sets.
- Falsifier: after the refit, space's and mobility's fitted weights
  within noise of zero, or a gate under +10. Then §7.3's second condition
  applies.

**If king safety, the phase and tempo stay first** (PLAN's order).
PLAN's C.3 text is the contract. C.0 adds:
- Measured: the donor's king row adds 15.2 [11, 19] at phase 10–17 and
  11.1 at 18–24. Those bands hold 15–16% and 68–69% of the meter.
- The phase: the endgame bands' static gap (95 to 144) is not one
  family's (§7.2), and they hold 16% of the meter. First read: Basilisk's
  middlegame and endgame totals re-tapered with the donor's phase,
  offline, by band. If the endgame gap does not close, the phase is not
  its cause and the scalers (C.7) are next.
- Tempo: only `9587eeeb` has one. Its arm is the constant after scaling
  against today's tapered 22.
- Rule-50 damping is not part of this cluster's port (§5.6).

### 8.3 PLAN edits

Applied with this leaf, as its record: C.0 closed; the budget row; the
donor and the values-or-structure answer in Phase C's introduction; C.1's
register row; a few lines of C.0 inputs under C.1, C.2.1, C.2.2, C.2.5,
C.2.8, C.3 and C.4, where C.0 also places space.

Proposed, not applied, each needing the maintainer's sign-off:
1. The mobility and space cluster before king safety, renumbered in
   execution order (§7.3).
2. C.4 renamed "Mobility, space and threats", which gives space its owner
   (C-119).
3. A leaf in C.2 for the committed family screen, before C.2.7; C.2.6 is
   free. `I1`: `c0_reanalysis.py` and the dataset builder are its
   prototype.
4. A decision point after C.2.8's gate: the screen re-read on the
   refitted head, which orders the families and closes any with no
   signal.
5. The oracle's known-win clamp. My position: leave the binary as it is
   through Phase C, so every re-read of the meter stays comparable, and
   report the count of such draws and the figure without them each time.
   A repair needs a mapping of Basilisk's tiers into Stockfish's value
   bands, and then a new baseline.

## 9. Amendment 1 — sign-off and the reworked phase (2026-11-18)

**Decision.** The maintainer accepted the donor and §8.3's proposals, and
set Phase C's goal: a frontier-class hand-crafted evaluation, as close to
Stockfish 16 HCE as possible and past it if it can be done. PLAN was
reworked the same day.

### 9.1 Numbering

What §7 and §8 call "today's C.4" is now **C.3**, Mobility, space and
threats. "Today's C.3" is **C.4**, King safety, phase and tempo. New
leaves:
- C.2.9, the family screen and the matched evaluation ablation as
  committed tools;
- C.2.10, the family order from games and screens, which is §8.3's
  decision point;
- C.3.1 to C.3.5, the first family's research, implementation, review,
  refit and gate;
- C.8.1, the residual investigation. The refit cycles are C.8.2.

### 9.2 Why `sf_16`, when `9587eeeb`'s read ahead (the maintainer's question)

Every measured lean is toward `9587eeeb`'s evaluation: 3.7 and 1.6 Elo
ahead in the two searches, a better static predictor by 0.23% of
cross-entropy, about 2% faster. None of it is a reason to expect more
from `sf_16`'s, and this packet claims no better potential for it.

It stays the donor for three reasons, none of them strength:
- **The games cannot tell the two apart.** Averaged over the hosts the
  difference is 2.7 Elo with an interval of −8.0 to +2.6. Two reads
  agreeing in sign happens half the time with no difference at all. The
  rule was frozen before the games so that a default would not move on
  that.
- **The label decides little.** Every value is refitted on Basilisk's own
  games, and where the two differ in form the family's research reads
  both and the screen picks per term. `9587eeeb`'s tempo, for one, is the
  only donor form of tempo there is.
- **`sf_16`'s is the target's evaluation.** Stockfish 16 HCE is the
  engine to catch, so the same-search meter against its evaluation says
  directly how much of the gap is evaluation.

What the choice does fix is the piece-square seed, which C.2.8 refits
freely. If a term's form ever turns on this choice alone, the family
screen and a game decide it, not the label.

### 9.3 What the rework adds

The goal is about 195 Elo and the expectation from the listed clusters is
80 to 140 (§7.4). The rework does not raise the expectation by assertion;
it removes the ways the phase could miss without noticing.

| Addition | The failure it guards against |
|---|---|
| The family order from games, C.2.10 | §7.3's order rests on games for *where* the deficit is and on a static screen for *which family*. A matched ablation measures the family in games, the layer that decides, before the first family is built |
| The donor fixture, stage 4 of the cluster shape | A donor form built with a wrong definition and then fitted around: Phase 5.9's sixteen terms, Rarog's king index (RAR-E73) |
| The union rule and the screen read both ways | Replacing a Basilisk term that carries information the donor lacks. Statically the two together beat the donor by 59.0e-4 nats where the donor beats Basilisk by 36.0e-4 (§5.2), so an evaluation better than the donor's is in reach of the two term sets |
| The frontier pair after every accepted cluster | Gates that add up on paper while the gap to the target does not close |
| The residual investigation, C.8.1 | Ending the phase because the list ended. It names what is left and whether it is evaluation at all |

**Registered for C.11's freeze**, head to head against Stockfish 16 HCE
at `3+0.03`, one thread: the gap is 100 or less with probability 0.55, 50
or less with 0.25, and Basilisk is ahead with 0.08. Phase D's clock and
thread work adds to it before the target is read at E.

### 9.4 Amendments to §8.1, C.1's handoff

Found while reworking the phase: the oracle's build names
`src/eval.cpp`, in `tools/oracle/build_oracle.ps1` and in the build
script inside the `oracle/hybrid` tag, which cannot be edited. Every
later read of the meter rebuilds the oracle on a new head.

1. **One translation unit.** `src/eval.cpp` stays the evaluator's only
   translation unit, and the family modules are headers it includes.
   The oracle's recipe, the tuner target and the engine library then
   need no change, and no inlining decision can move. This replaces
   §8.1's fallback: separate translation units are not tried.
   `tools/diag/profile_regions.json` gains the new file names.
2. **A sixth proof.** The oracle rebuilt from the restructured commit
   passes its conformance test and reads 266,667 nodes at `bench 16 1 8
   default depth` with `Use Basilisk HCE=true`, as
   `oracle-b111-4d2742f` does.
3. **Switch points.** Each family's contribution enters the sums through
   one call, so that C.2.9's mask can skip it. C.1 adds no mask.

## 10. C.1 implementation record (2026-11-19)

C.1 is `LOCAL_QUALIFIED`: proofs 1 to 5 pass (§8.1 and §9.4's sixth).
The pooled NPS read, proof 6, is owed; the pools are built.

### 10.1 Commits (branch `c0-c1`)

| Commit | What |
|---|---|
| `914f06a` | The evaluation digest, `tests/eval_hash.cpp`: targets `eval_hash` (the engine library) and `eval_hash_trace` (the TEXEL build, which compiles `src/eval.cpp` as the tuner does) |
| `4cd986e` | The split |
| `e7794bf` | The cache-purity and lazy-checkpoint tests |
| `7a20849` | `tools/diag/profile_regions.json` and its test |
| `a1c24c6` | The untraced remainder recorded by source, in TEXEL_TRACE builds only |
| `c0f0209` | Its report, `eval_hash_trace FILE --rest` |

### 10.2 The layout

`src/eval.cpp` is still the evaluator's only translation unit. It holds
the parameters, `init_eval_tables`, the caches' lifetime, `reconstruct`,
the tune-only loader and `evaluate_full`; the families are headers it alone
includes, and none of the files that list the evaluator's sources changed
(`CMakeLists.txt`'s engine library and tuner, `tools/oracle/build_oracle.ps1`).

| Header | Holds | Entry into the sum |
|---|---|---|
| `eval_common.h` | The trace storage and macros, the phase constants, `forward_square`, `EvalScore`, `taper` | — |
| `eval_material.h` | Material and tables, the phase, the imbalance and its cache | `material` |
| `eval_pawns.h` | The pawn cache (`pawn_probe`), the passer terms inside and outside it, rook behind a passer, king proximity and path | `pawns_cheap`, `pawns_full` |
| `eval_pieces.h` | Bishop pair, rook and knight terms, the small positional terms, the trapped bishop | `pieces_cheap`, `pieces_full` |
| `eval_attacks.h` | `AttackInfo`, the producer, and `fill_attack_info`, its one sweep | — (inputs) |
| `eval_mobility.h` | The mobility tables' accumulation (`mobility_term`, called by the sweep), the three space terms | `mobility_space` |
| `eval_threats.h` | Pawn threats and the threats package | `threats` |
| `eval_king.h` | The danger index and its table, shelter and storm | `king` |
| `eval_endgame.h` | `apply_endgame`, the KPK table, the drives, the rook-ending scales, the tune-build census, the mate drive | `mate_drive`; `apply_endgame` on both paths |

`evaluate_full` keeps the order of calls, the lazy checkpoint, the tempo,
the taper and the lazy audit. The order of the sum and its three reads are
documented where they are enforced, above the cheap block: the lazy
checkpoint reads the cheap block alone, the mate drive every family without
the tempo or itself, the final taper everything. The producer is one
`AttackInfo` on the stack, filled once after the checkpoint; `slider_att`
is still not cleared, as before. It holds what §8.1 lists and nothing more:
`mob_area` per colour, a loop local before, is now kept beside the maps.

### 10.3 Proofs

The "before" readings were taken at `914f06a`, whose `src/` is the C.1
starting head's (`496248d`); the "after" readings at `4cd986e`, and bench
and the served digest again at `c0f0209`. The digests are FNV-1a over one
warm evaluator reading `tools/results/c0-screen/dataset.csv` (254,486 rows,
sha256 cfee73850fa863e0…) in file order, as a search would, so the pawn and
imbalance caches are on the path.

| Proof | Before | After |
|---|---|---|
| `bench 13` | 4,777,412 | 4,777,412 at every commit |
| Served score, `evaluate()` | 45f5998c772e3145 | 45f5998c772e3145 |
| Draw verdict, `draw_verdict()` | 77730f705f8fdfe4 | 77730f705f8fdfe4 |
| Traced score (TEXEL build) | 9d24145c5bdec126 | 9d24145c5bdec126 |
| Trace: the phase and every `TR_MG`/`TR_EG` slot | 3d5f7b895b663db2 | 3d5f7b895b663db2 |
| Lazy audit (Diag build): served digest with the audit on | equal to the served | equal to the served |
| Lazy audit counters: fires, sign flips, crossings, Σ\|Δ\|, max \|Δ\| | 20,597, 0, 476, 4,977,647, 746 | the same |
| Tuner `--verify tools/texel/test_positions.csv` | 7 of 7 exact | 7 of 7 exact |

- **Suites.** Release CTest 59 of 59 on the split. `test_eval` 126 of 126
  with the new tests, the symmetry sweep included. The sanitizer (`debug`)
  CTest 56 of 56 with the new tests, the two `strength`-labelled tests
  left out there as they are slow under the sanitizers; both passed in the
  release CTest.
- **The oracle.** Rebuilt from `c0f0209` by `tools/oracle/build_oracle.ps1`:
  `tools/test_engines/oracle-c1-c0f0209.exe`, sha256 B2AB5A9AC04C5B12…,
  conformance 471,519 positions with 0 mismatches, `bench 16 1 10 default
  depth` 875,469 nodes and `bench 16 1 8 default depth` 266,667 with `Use
  Basilisk HCE=true`, both as `oracle-b111-4d2742f` reads.
- **The digest is live.** With the pieces' cheap entry moved after the lazy
  checkpoint, bench read 3,449,705 and the served digest e94597b3413b8ace.

### 10.4 Tests added, and the breaks that fail them

In `tests/test_eval.cpp`, over the bench and WAC positions, each also
after four and eight seeded random plies:

- **Pawn cache purity.** The same pawns with every other piece removed, or
  with the kings moved, are read on a warm evaluator after the full
  position and before it (4,044 pairs); each must read as on an empty
  cache.
- **Imbalance cache purity.** The file mirror (the same ten counts, every
  piece moved, every bishop on the other colour) and the position less one
  piece (a count changed), read the same way (4,044 pairs).
- **The lazy checkpoint.** Positions the lazy path certainly serves (one
  side keeps 1,500 cp of pieces, the other only king and pawns, never more
  than two pawns apart). Every parameter group is perturbed in turn, by an
  amount that differs by slot: each of the 50 cheap-block groups must move
  a lazy-served score, and no other group may.

| Deliberate break (never committed) | Result |
|---|---|
| The pawn cache adds a knight count | Pawn purity fails (34 of 4,044 stale) |
| The imbalance cache adds the white bishops on dark squares | Imbalance purity fails (497 stale), the symmetry tests with it |
| The pieces' cheap entry moved after the checkpoint | Lazy test fails: the ten bishop-pair, rook and knight groups moved nothing |
| The tempo moved before the checkpoint | Lazy test fails: `Tempo` moved 1,557 lazy-served scores |

Each cache test also fails on the other's break: both compare a warm
evaluator with a cold one end to end.

### 10.5 The untraced remainder by source (C-90)

`rest` is the served score less the trace's reconstruction. The TEXEL build
now records, beside the trace, each untraced addition to the running sum
where it is made, the sum the final taper read, that taper, and
`apply_endgame`'s result (`EvalRestTrace`, `g_rest`); `EvalTrace` is
unchanged. `eval_hash_trace FILE --rest` decomposes `rest` exactly. On the
dataset above:

| Source | Positions nonzero | Σ\|value\| | max \|value\| |
|---|---|---|---|
| `rest`, the total | 223,813 | 222,586,045 | 28,779 |
| The passer king-proximity product's rank half, tapered | 98,014 | 601,955 | 67 |
| The mate drive, tapered | 18,928 | 876,167 | 71 |
| The taper's rounding of the two | 51,159 | 52,030 | 2 |
| `apply_endgame`: overrides and scales | 17,376 | 221,891,229 | 28,700 |
| Rule-50 damping | 209,034 | 2,564,206 | 7,334 |
| Unattributed: the running sum beyond the traced and frozen terms | 0 | 0 | 0 |

The unattributed line is the check that every other term is traced: it
reads zero on every position, and the tool exits 1 when it does not.
Almost all of `rest` is `apply_endgame`'s known-win and bare-king overrides
on 6.8% of the rows. The damping is not new to the fit: the tuner's
`linear_delta_scale` already carries the rule-50 curve.

### 10.6 Deviations from the handoff, and readings of it

- **Two entries for two families.** Pawns and pieces have terms on both
  sides of the lazy checkpoint, so each has a cheap entry and a full one;
  a mask skips the family by skipping both. Moving a term across the
  checkpoint to give them one entry is what C.1 must not do.
- **Two additions moved inside the full block.** The mobility sum is
  accumulated by the sweep into the producer and enters the sum with the
  space terms; the trapped bishop enters with the pieces' full entry,
  before space. Both stay between the lazy checkpoint and the mate drive,
  so no read sees a different set of terms; the digests show the sums
  unchanged.
- **The pawn cache's member function** `Evaluator::eval_pawns` became the
  free `pawn_probe`, which takes the cache and the probe counters by
  reference, and the imbalance cache is passed into `material`. The
  private declaration left `eval.h`; nothing public changed.
- **The remainder report** lives in the digest instrument, not the tuner:
  the tuner is untouched.
- **The digest instrument** is new. Its traced twin is one more target that
  names `src/eval.cpp`, the same single translation unit.

### 10.7 Owed

The pooled NPS read, in the no-regression form: pools
`tools/test_engines/c1-base.pool.json` (four PGO builds of `496248d`) and
`c1-head.pool.json` (four of the C.1 head), both reading 4,777,412.
