# C.2.10 family order from games and screens: registration (2026-11-24)

- State / class: **registered, no game played and no screen read.** `R3`.
- Owner: agent (research). The launcher and the builds are implementation;
  the game reads are the maintainer's; the screen is a short agent read.
- Ledger: BAS-O14 (the family meter), BAS-M24 (the frontier pair),
  BAS-E62 (the screen).
- Head: the accepted refit, `bench 13` 3,359,329.

## 1. Question

Which evaluation cluster opens first, and does the order C.3, C.4, C.5,
C.6 stand?
- C.3 mobility, space and threats;
- C.4 king safety, phase and tempo;
- C.5 pawns and passers;
- C.6 material, imbalance and pieces.

C.0 put mobility and space first from a static screen on the 1.11.0 head.
Three things have changed since: the whole surface is refitted, the screen
is a tool, and a family can now be switched off on both sides of the
meter. So the order is decided again, this time from games.

## 2. Instruments

### 2.1 The family meter (BAS-O14)

- Binary: `tools/test_engines/oracle-c29-e56f1e1-evalablate.exe`
  (`CAA8B70B…`), Stockfish `9587eeeb`'s search. Both arms are this binary.
- Arms: `Use Basilisk HCE=false` against `=true`, at one
  `EvalAblationMask`. E(mask) is the first arm's Elo over the second.
- A meter is M(mask) = E(0) − E(mask): the part of the same-search deficit
  that goes away when that family is off on both sides.
- Reads, 3,000 games each (1,500 cycles):

  | Read | Mask | Off on both sides | Use |
  |---|---:|---|---|
  | `e0` | 0 | nothing | the deficit on the refitted head |
  | `m7` | 7 | space, mobility, threats | C.3's meter |
  | `m8` | 8 | king danger, shelter, storm | C.4's meter; a family |
  | `m48` | 48 | pawn structure, passers | C.5's meter |
  | `m64` | 64 | the piece terms | C.6's meter; a family |
  | `m127` | 127 | all seven | the families' joint share |
  | `m2` | 2 | mobility | family |
  | `m1` | 1 | space | family |
  | `m4` | 4 | threats | family |
  | `m16` | 16 | pawn structure | family |
  | `m32` | 32 | passers | family |

- Conditions: BAS-O08's. Colosseum CLI, two-engine gauntlet, UHO
  `UHO_Lichess_4852_v1.epd` in random order, `3+0.03`, margin 20 ms, 1T,
  Hash 64, concurrency 14, no adjudication, no tablebases, zero permitted
  faults.
- **One seed, 210001, for all eleven reads.** They then play the same
  openings in the same order, and what the openings add is shared by every
  E. Intervals are still computed as if the reads were independent.
- Three reads go beyond PLAN's "by family" (`m7`, `m48`, `m127`), 9,000
  games:
  - The order is between clusters. A cluster's meter from the sum of its
    families carries E(0)'s error once per family and assumes they add. A
    direct read compares two E(mask) values, at about ±15.
  - `m127` says how much of the deficit the seven families hold together.
    The rest is in what is never masked: material, the tables, the
    imbalance, the phase, tempo, scaling and endgame knowledge.

### 2.2 The frontier pair's baseline (BAS-M24)

- The head against Stockfish 16 with `Use NNUE=false`, a fixed match of
  3,000 games, `3+0.03`.
- Head: a fresh final PGO PEXT build of the branch's head, `bench 13`
  3,359,329. Stockfish 16:
  `D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe`
  (`041396A4…`, BAS-M18's), at its defaults otherwise.
- Conditions: the meter's, as a match with an external second arm and the
  wrapper's 1% fault allowance for it. Seed 210002.
- This is the protocol every later frontier pair repeats (PLAN, the
  cluster shape's step 7).

### 2.3 The family screen (BAS-E62)

- `tools/diag/family_screen.py`, `collect` then `screen`, on the head's
  tuner.
- Rows: corpus A's validation split, 60,000 positions, both donors
  (`donor-sf9587-stock.exe` and `donor-sf16-stock.exe`).
- Read both ways, by phase band, with the tool's game bootstrap.
- Evidence: `tools/results/c210-screen/`.

## 3. What a meter does and does not say

A meter is the donor family's worth inside the donor's evaluation, less
Basilisk's family's worth inside Basilisk's, in `9587eeeb`'s search. It
orders families. It does not predict a port's gate, which is played in
Basilisk's own search after a refit.

Confounds, with the direction each pushes:
- **Level.** Neither side is refitted after a family is removed. The
  donor's mobility is positive at typical counts for bishops, rooks and
  queens, so removing it also lowers those pieces' values. Basilisk's
  mobility tables are mean-anchored and hold less level. This raises
  mobility's meter, and the pawn family's less so. It is not separable in
  games; the screen fits its own coefficients and does not carry it.
- **Speed.** A masked bit skips the work on Basilisk's side for space,
  threats, king, pawns, passers and pieces (`src/eval.cpp`,
  `src/eval_pawns.h`); mobility's sweep stays. The donor's side keeps
  every computation. So Basilisk's arm gains speed under a mask, which
  raises the meter. Control: nodes a second per arm from the games,
  reported beside every read. A Basilisk arm more than 5% faster than at
  mask 0 is flagged on that read.
- **Not additive.** Families were fitted together. `m7`, `m48` and `m127`
  measure the joint effect directly; the single meters are not summed for
  any decision.
- **What is never masked.** C.4's phase and tempo and C.6's material and
  imbalance are outside every bit. C.4's and C.6's meters are therefore
  lower bounds on those clusters.
- **The donor.** The meter's evaluation is `9587eeeb`'s, and the seed
  donor is `sf_16`'s. They are within about 8 Elo in either search
  (BAS-O09).
- **Elementary mates.** BAS-O08 found 16 clamped KR-K and KBB-K draws,
  about 3 Elo. The count is reported per read.
- **Saturation.** An arm outside 20% to 80% is a bound, not a number
  (PROCESS, *Matched ablation*, rule 4).

Wire checks, before any game of a read:
- the binary's hash;
- for a mask other than 0, `bench 16 1 8 default depth` differs from mask
  0's count under both values of `Use Basilisk HCE`; a count that does not
  move refuses the read;
- Colosseum's advertised-option check refuses an engine without
  `EvalAblationMask` (rule 1).

## 4. The decision rule, frozen

1. **Clusters are compared by E(mask) directly.** Cluster X explains more
   than cluster Y when E(X) is below E(Y).
2. **The present order stands unless games contradict it.** A later
   cluster moves ahead of an earlier one only when its E is lower by more
   than the 95% interval of the difference (the two reads' errors in
   quadrature, about 15 Elo).
3. **A tie in games goes to the screen.** The donor-into-Basilisk gain at
   phase 18–24, then over all rows, breaks it only when the two families'
   intervals do not overlap. Otherwise the present order stands.
4. **Constraints kept:** the phase, the endgame king danger and tempo
   change together; the endgame cluster follows pawns and pieces.
5. **`NO_CHANGE` (C-110).** A family closes without a donor form when
   both hold: its single meter's 95% interval includes zero or lies below
   it, and its donor-into-Basilisk gain includes zero in every band. Its
   values still ride every refit.
6. **Inside C.3** the three single meters are inputs to C.3.1. This leaf
   does not order them.
7. **A saturated or flagged read does not move the order.**
8. **The programme's check.** If the seven families together explain
   under 40 Elo (E(0) − E(127) below 40), the premise that family ports
   carry the recovery is contradicted. The order then goes back to the
   maintainer before C.3 opens, with the unmasked parts as candidates.

A negative meter means the deficit is not in that family. It does not
mean Basilisk's form is the better one; the screen's other direction
answers that.

## 5. Predictions, frozen before any read

Elo, with an 80% band.

| Quantity | Point | 80% band |
|---|---:|---|
| E(0) | +160 | +130 to +190 |
| Frontier pair, head − Stockfish 16 HCE | −140 | −170 to −110 |
| M(7), C.3 | +55 | +25 to +90 |
| M(8), king | +20 | −5 to +45 |
| M(48), C.5 | +12 | −15 to +40 |
| M(64), pieces | +5 | −12 to +22 |
| M(2), mobility | +35 | +10 to +65 |
| M(1), space | +12 | −5 to +30 |
| M(4), threats | +8 | −12 to +28 |
| M(16), pawn structure | +8 | −12 to +28 |
| M(32), passers | +5 | −18 to +28 |
| E(127), the unmasked remainder | +70 | +35 to +110 |

- C.3's is the largest cluster meter: 0.7.
- The rule leaves the order unchanged: 0.75.
- **Mobility's single meter exceeds space's: 0.75.** This is against
  C.0's static ranking. Basilisk's space weights are 0, 1 and −1 after
  the refit, so space's meter is about what the term is worth to the
  donor, and that has always been small in games.
- At least three of the seven single meters include zero: 0.8.
- The screen, at phase 18–24, donor into Basilisk:
  - space still has the largest gain: 0.6;
  - space's gain is at least 25 (1e-4 nats; it was 47.0): 0.6;
  - the passers' gain over all rows includes zero: 0.7.
- The screen, Basilisk into the donor: at least one family's gain
  excludes zero: 0.8.

Falsifiers:
- **E(0) above +205.** The refit, +85 in Basilisk's own search, then
  bought under 15 in the donor's search. The fit would be specific to
  Basilisk's search, and every later refit's gate reads with that.
- **The frontier pair worse than −185.** The same finding at the target.
- **E(0) − E(127) below 40:** rule 8.

Not a falsifier, but explained before the order is taken: a single meter
above the cluster read that contains it by more than the interval (M(2)
above M(7), or M(16) or M(32) above M(48), by more than 21). The other
members would then be worth more to Basilisk than to the donor, and their
single reads must show it.

## 6. Stop rule and order of play

- One run per read at 3,000 games, never extended or replaced. An
  interrupted read resumes from its checkpoint.
- Order: `e0`, `m7`, `m8`, `m48`, `m64`, the frontier pair, `m127`, `m2`,
  `m1`, `m4`, `m16`, `m32`. The reads that decide the order come first.
- Twelve reads, 36,000 games, about nine hours at 46 minutes a read.
- The screen is read after this packet is committed and before or beside
  the games. Neither is used to change the other's registration.

## 7. The launcher (implementation's)

`tools/run_c210_reads.py`, in `run_c0_reads.py`'s form:
- `--read <name>` and `--read all`; `all` plays the order above, skips a
  finished read and stops at the first failure; `--dry-run` for each.
- The meter reads: both engines the pinned oracle, the first arm
  `Use Basilisk HCE=false`, the second `=true` and fixed at 1500, both
  with `EvalAblationMask=<mask>`; 1,500 cycles; seed 210001; results in
  `tools/results/c210-<read>/`.
- The wire checks of §3, as refusals.
- The frontier pair: match mode, the head build with its bench and
  revision expected, Stockfish 16 as the external arm with
  `Use NNUE=false`; 3,000 games; seed 210002.
- The head build: `-Suffix c210-head`, pinned by hash and revision.
- One reader that writes a table from the finished reads: Elo and 95%
  interval by BAS-O08's method, the pentanomial, each arm's nodes a
  second and nodes a move, the clamped-draw count, and M(mask) with its
  interval. It refuses a read whose recount disagrees.

## 8. Results

Appended after exposure.

### 8.1 The screen (BAS-E62), read 2026-11-24

The table is in BAS-E62. Against this packet:
- **The tiebreak now points at king safety.** At phase 18–24 its gain,
  16.2 [8.5, 24.1], is clear of space's 3.4 [−0.4, 7.8] and mobility's
  2.1 [−0.8, 5.3]. It touches threats' 4.7 [0.5, 8.3], and overlaps it by
  0.3 against `9587eeeb`. Over all rows it is clear of
  the passers' 5.4 [3.4, 7.6]. By rule 3, a tie in games between C.4 and
  an earlier cluster puts C.4 first.
- **C-110's static half** (rule 5) holds for space, mobility, pawn
  structure, knights, rooks and queens: the donor's gain includes zero
  in every band. Each closes `NO_CHANGE` if its game meter also includes
  zero. It does not hold for king safety, threats, the passers or the
  bishops.
- **Three of the four screen predictions missed** (§5). They rested on
  C.0's residuals surviving the refit, and so do this packet's game
  predictions for M(7), M(2) and M(1). Those stay frozen and are scored
  as written. Read today they would be lower.
- One control is owed before the fall from 47.0 is called the refit's:
  the same tool and sample with the header before the refit.

### 8.2 The screen's control, 2026-11-24

BAS-E62's CONTROL block has the reads. In short:
- On corpus A's rows space never read: 5.7 at phase 18–24 on the header
  before the refit. The refit did not remove it.
- On C.0's gate-game rows it reads 50.8 before the refit and 40.4
  [32.7, 49.1] with the head. Mobility goes 29.6 to 13.4 [9.7, 17.4],
  and king safety 11.8 to 10.8 [7.3, 14.9].
- The form, the code, an intercept and the ply were each ruled out. The
  difference is the sample: UHO gate games at `3+0.03` against self-play
  at 8,000 nodes from a start pool.
- §8.1's "read today they would be lower" is withdrawn. On the rows the
  games are played from, C.0's static case stands after the refit.

## 9. Amendment 1 — 2026-11-24, after the screen and before any game

The registered screen sample was the fit's rows. The tiebreak is about
games, and the two samples disagree on the first family. So the screen
may decide less than registered, never more.

- **Rule 3, amended.** A tie in games is broken only when both samples
  agree: corpus A's validation rows (BAS-E62's read) and C.0's gate-game
  rows with the head's evaluation (BAS-E62's control). Otherwise the
  present order stands.
- **Rule 5, amended.** C-110's static half needs the donor's gain to
  include zero in every band on both samples.
- Nothing else changes: the games, their order, the predictions and
  rules 1, 2, 4, 6, 7 and 8.

Where the samples stand, for the rule's use:
- **C.3 against C.4: they disagree.** Corpus A puts king safety ahead at
  phase 18–24; the gate games put space ahead. A tie in games keeps C.3
  first.
- **C.4 against C.5: they agree.** King safety is clear of the pawns and
  of the passers over all rows on both. A tie keeps C.4 ahead.
- **C-110's static half** now holds only for the queens' and the
  rooks' terms. Space, mobility and pawn structure exclude zero on the
  gate games.

## 10. The games — read 2026-11-25

Twelve reads of 3,000 games, each complete, with no fault and no time
loss. The full table is BAS-O14's; the frontier pair is BAS-M24's.

| Quantity | Predicted | Read (95%) | |
|---|---:|---|---|
| E(0) | +160 | +146.2 [+137.5, +155.1] | in band |
| Frontier pair | −140 | −120.0 [−129.2, −111.0] | in band |
| M(7), mobility, space, threats | +55 | −52.0 ± 13.5 | missed |
| M(8), king | +20 | +43.1 ± 13.2 | in band |
| M(48), pawns and passers | +12 | +94.9 ± 13.4 | missed |
| M(64), pieces | +5 | −37.7 ± 13.1 | missed |
| M(2), mobility | +35 | −46.0 ± 13.3 | missed |
| M(1), space | +12 | −2.1 ± 12.5 | in band |
| M(4), threats | +8 | −9.9 ± 12.9 | in band |
| M(16), pawn structure | +8 | −33.2 ± 13.1 | missed |
| M(32), passers | +5 | +125.4 ± 12.9 | missed |
| E(127) | +70 | −16.6 [−27.4, −5.8] | missed |

- No falsifier fired. The four ranked predictions all missed (BAS-O14's
  calibration).
- §3's confounds, as they read:
  - **Speed.** Basilisk's arm is within about 4% of its mask-0 speed on
    every order read. Only `m127` is flagged.
  - **Level.** The two mobility terms carry the same level (slope on
    own material 0.019 and 0.021), so it explains no sign.
  - **Saturation.** Every arm is inside 20% to 80%.
  - **Not additive.** The singles sum to +39.6 and the clusters to
    +48.3, against +162.8 together.
- The passers' single meter is 30.5 above its cluster's, over §5's 21.
  Pawn structure's −33.2 is the explanation §5 named.

## 11. The rule applied

1. E by cluster: C.5 +51.3, C.4 +103.1, C.6 +184.0, C.3 +198.2.
2. Each move is by more than its interval:
   - C.5 ahead of C.4, by 51.8 ± 14.2;
   - C.4 ahead of C.3, by 95.1 ± 14.2;
   - C.6 ahead of C.3, by 14.2 against 14.1: a tenth of an Elo.

   C.6 does not pass C.4; its E is 80.9 higher.
3. No tie reaches the screen.
4. The constraints hold. The phase, tempo and the endgame king danger
   stay together in the king cluster, and the endgame cluster still
   follows pawns and pieces.
5. C-110 closes the donor's queen terms: the pieces' meter lies below
   zero and their gain includes zero in every band on both samples. The
   rook terms sit on the edge ([0.0, 2.0] at phase 10–17 on gate games)
   and are left to their cluster's screen. No whole family meets both
   halves.
6. C.3's singles go to its research: mobility −46.0, space −2.1,
   threats −9.9.
7. No order read is saturated or flagged.
8. E(0) − E(127) is 162.8. The families hold the whole same-search
   deficit, and the premise stands.

**The order: pawns and passers, king safety, material and pieces,
mobility, space and threats.** They are renumbered C.3, C.4, C.5 and
C.6 in that order; HISTORY has the map.

## 12. What the order does not say

- **A meter ranks; it does not size a port** (§3). The singles sum to a
  quarter of the joint read.
- **The passers' +125.4 has three readings**, and the games do not
  separate them:
  - the donor's form is worth more;
  - Basilisk's passer value sits partly in its endgame pawn table (152
    to 270 on the seventh rank), which bit 5 leaves in and which cannot
    see a blocker or a king;
  - Basilisk's own passer terms cost it in play.
- Static evidence, after the fact and not registered
  (`tools/results/c210-close/`, corpus A's validation rows):
  - Removing its own passer terms costs the donor 379.9 and Basilisk
    103.1 (1e-4 nats); for the king, 261.1 and 53.2.
  - The donor's passer term adds 5.4 to Basilisk's whole score, and
    14.0 at phase 0–9 (BAS-E62).
  - So the donor leans on its term far more than Basilisk does on its
    own, while Basilisk's whole score already holds most of what the
    donor's term predicts.
- **The read that separates them** is each evaluation with and without
  its own passer terms: masks 0 and 32 at one `Use Basilisk HCE`, both
  values. It opens the first cluster's research.
- **Third place rests on a tenth of an Elo.** Both clusters read below
  zero, so it decides little.
- **The screen's donor gain did not rank the games.** Its leading
  family on gate games, space, reads −2.1 ± 12.5. C.0's prior is
  refuted.
