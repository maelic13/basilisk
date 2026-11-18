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

Appended after exposure.
