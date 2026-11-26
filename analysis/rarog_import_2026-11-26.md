# Rarog import, 2026-11-26: evaluation candidates beside the donor's form

- State / class: delivered; the source of the PLAN changes of 2026-11-26, `R2`
- Owner / date: agent, 2026-11-26, from a research note the maintainer
  supplied in the session (Rarog's RAR-E75, dated 2026-11-26). No Rarog
  repository or commit was read; the numbers below are the note's.
- Decision taken: maintainer, 2026-11-26: every candidate is recorded as an
  idea to research in its own Phase C step, where it is implemented and
  tested or dropped. Nothing is accepted here.

**Recommendation.** Research the pawn race (B) with C.4.5's promotion
races first among these: it is the one term neither donor has, and its
screen was the largest. The rest are cheap reads before any code.

Rarog's verdicts are priors: its evaluation is its own, and the note's
donor column is `sf_16`'s, while Basilisk's rebuilt evaluation is
`9587eeeb`'s, refitted in C.3.4 and under Basilisk's endgame layer
(`analysis/c31_rebuild_2026-11-25.md` §4). Expect Basilisk's residuals
between the note's `sf_11` and `sf_16` columns, and read them on
Basilisk's own evaluation before assuming either.

## 1. The note's instrument

- 194,444 quiet positions from one engine's self-play at 8,000 nodes,
  labelled by game result; six men or fewer relabelled by Syzygy.
- Three static evaluations of each row: Rarog's, `sf_11`'s and `sf_16`'s.
- Calibration: the logistic slope K fitted inside a class of positions,
  over K on all rows. Above 1, the class is under-scaled: an advantage
  there decides more games than the same number elsewhere.
- Screen: p = sigmoid(k * eval + side-to-move terms + w * feature),
  fitted on half the games and read on the other half. Gain is the change
  in held-out cross-entropy, in percent; negative is better. A random
  feature gains +0.001; a whole second evaluation about −1.1 to −1.2.
  Game-clustered bootstrap for every interval.
- Limits: cross-entropy on results is not Elo; quiet rows cannot see
  tactical terms; with heavy material such a corpus favours the
  evaluation that played the games, so only light-material findings are
  quoted.

## 2. Finding 1: scaling by ending type (→ C.4.2)

K in class over K overall, `sf_16`, 95% intervals:

| Class | Ratio |
|---|---|
| Pawn endings | 1.75 [1.64, 1.88] |
| One bishop each, opposite colours, nothing else | 1.68 [1.54, 1.83] |
| One knight each / same-colour bishops | 1.35 [1.27, 1.43] |
| Rook endings | 1.30 [1.25, 1.37] |
| Other minor endings | 1.45 [1.40, 1.50] |
| Queen endings | 1.02 [0.95, 1.08] |
| Non-pawn material 25–44 | 0.84 [0.82, 0.86] |
| Non-pawn material 45+ | 0.63 [0.60, 0.66] |

Most of it is one slope by phase (light material about 1.3–1.45, heavy
0.6–0.85). Beside a phase-dependent slope the residuals are:

| Evaluation | Pawn endings | Pure opposite bishops | Rook endings | Held-out gain |
|---|---|---|---|---|
| `sf_16` | ×1.12 ± 0.07 | ×1.14 ± 0.08 | ×0.95 ± 0.03 | −0.020 [−0.036, −0.003] |
| `sf_11` | ×1.19 | ×1.33 | ×0.97 | −0.058 |
| Rarog (no donor pawn-ending term) | ×1.28 | ×0.60 | ×1.01 | −0.090 |

For Basilisk: the fit already sets middlegame and endgame values apart,
so most of a phase slope should be absorbed. The read is the same ratio
per class on the accepted head, corpus A's held-out half: minutes, no
games. A phase slope that survives the fit is a fit question before it
is a term.

## 3. Candidate A: a scale by ending type (→ C.4.3)

- Form: one multiplier on the final score per class, chosen by non-pawn
  material alone: no pieces; one bishop each on opposite colours and
  nothing else; rooks only, if the fit still wants it. Applied after the
  specialised endgames and scale factors, never to mate, tablebase or
  recognised-draw scores.
- Effect: trade decisions into and out of those endings; no ordering
  changes inside a class.
- Interactions named by the note: the donor's `51 * !non_pawn_material`
  in `winnable()` and its opposite-bishop factors, so residuals are
  small; passed-pawn terms scale with it; a capture that enters a class
  moves the static evaluation, which the static-evaluation margins read;
  tablebases remove most of it at seven men or fewer.
- Interactions in Basilisk: C.3.6 reads Basilisk's own opposite-bishop
  scale against the donor's on the same class; the correction-history
  tables are keyed on pawns and non-pawn material, the inputs that pick
  the class (C.4.2's E7); the endgame layer's scales sit on the same
  classes (`src/eval_rb.h`, `general_scale_factor`).
- Invariants: colour-flip symmetry; draws stay 0; the score stays below
  the tablebase range; the evaluation cache keys on the material that
  picks the class.
- Kill rule: on tablebase positions from Basilisk's own games, the scaled
  evaluation raises drawn-position overclaims by more than won positions
  gain. Then gate.

## 4. Candidate B: the pawn race (→ C.4.5)

- Form, pawn endings only: for each side, its passed pawn nearest to
  queening that cannot be stopped: a clear path, and either the enemy
  king outside its square counting the move, or its own king guarding
  every remaining square of the path while the pawn cannot simply be
  taken. Count plies to queen (a second-rank pawn steps twice; the side
  to move is one ply ahead). The term is the difference between the
  sides, in ranks, times one weight. Neither `sf_16` nor `9587eeeb` has
  one.
- Evidence on `sf_16`: −0.087 [−0.125, −0.050] alone, about +100 cp a
  rank; beside the pawn-ending scale −0.034 [−0.064, −0.004], about +83.
  Active on 1.1% of rows. Fitted beside A, never before.
- Interactions: the search resolves a race two plies a rank; tables
  resolve it at seven men or fewer; it reads both kings and the side to
  move, so it cannot live in the pawn entry as keyed now.
- Kill rule: on pawn endings of seven men or fewer from Basilisk's games,
  its sign does not agree with Syzygy clearly more often than the
  evaluation without it.

## 5. Candidate C: a rook on the seventh in the endgame (→ C.5.1.1)

- Form: an endgame-half bonus for a rook on its seventh rank while an
  enemy pawn is on that rank or the enemy king is on its back rank. No
  middlegame weight.
- Evidence on `sf_16`: −0.015 [−0.027, −0.004]; endgame +39 ± 16,
  middlegame −38 ± 28 (unresolved). The condition is not resolved against
  a plain rook on the seventh. Small: the note reads it inside a sweep.
- Basilisk's rebuilt arm has no such term (`src/eval_rb_pieces.h`).

## 6. Candidate D: several pieces attacked at once (→ C.5.1.1)

- Form: count a side's pieces attacked by a lesser piece (a minor by a
  pawn; a rook by a pawn or minor; the queen by a pawn, minor or rook).
  A bonus to the attacker for each beyond the first, since one move
  saves one piece. Variant: split by side to move.
- Evidence: none either way; it occurs on 0.3% of quiet rows, which
  cannot judge it.
- The read it needs is the search's own leaves: how often two or more
  occur where the static evaluation decides a node, and the static
  evaluation's error against a deeper search there, with and without the
  term. Kill rule: negligible activation, or an error that does not fall.
- Basilisk's threats (`src/eval_rb_threats.h`) score each threatened
  piece; none counts them.

## 7. Screened without gain on the note's instrument (→ C.5.1.1)

- Rook mobility counted through pieces, only pawns blocking: +0.007 on
  `sf_16`.
- An offset per uneven piece configuration beside the evaluation's own
  imbalance: +0.018 on `sf_16`.
- A separate multiplier for each of 52 piece configurations: on the
  donors it only rediscovers the slope by phase.

## 8. The note's order, and where it lands

1. The slope by phase (Finding 1): C.4.2.
2. A beside it (C.4.3), then B beside A (C.4.5).
3. D's leaf read: C.5.1.1.
4. C only inside a sweep: C.5.1.1 decides.

The note expects small gains: its three-class scale is about a seventh of
what a whole second evaluation adds on its instrument.
