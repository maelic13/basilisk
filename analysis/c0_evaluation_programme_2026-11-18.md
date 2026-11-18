# C.0 — Evaluation programme investigation

- State / class: `RESEARCH`, `R3`
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
