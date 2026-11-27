# BAS-O20

**BAS-O20 — C.3.6: the own-forms screen repeated on corpus C, from the refit vector R**

- Date / owner / category: registered 2026-11-26, before corpus C
  exists; agent-run overnight; evaluation/HCE, fit layer.
- Base: BAS-O19's refit vector R, on corpus C, at BAS-O19's frozen K
  pair. The commit is the C.3.6.2 commit reviewed in C.3.6.3.
- Research question: BAS-O18's question on the rebuilt engine's own
  games, which are the fair population for its forms (the packet's §9
  and §13). It also replicates BAS-O18 on a second corpus.
- Arms: BAS-O18's (N, C1, C2, the bits alone, U), then P_C, the passing
  bits fitted together, when two or more pass. The execution is BAS-O18's
  Amendment 1. After R5 (2026-11-26, before any arm) the bits are
  nineteen and U is the nineteen: 23 arms before P_C.
- Measure: BAS-O18's, on C's validation rows, at C's frozen K.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- Method gates pass: 0.85.
- Candidates passing: 3 of 20, 80% band 0 to 7.
- At least half of the forms that pass on A also pass on C: 0.5.
- Δ on C is smaller than on A for the replacing forms, on average: 0.6.
  A's population was steered by the retired evaluation, so its own forms
  should look better there.

#### Registered rule

- δ_C = 2.0e-5 × L_C(R) / 0.09274536, and both gates scaled the same way.
- Gates first, with BAS-O18's gate 1 fallback; then a candidate passes
  at Δ ≥ δ_C with its bootstrap lower bound above 0. All fits end before
  any game starts.
- If corpus C becomes the base (BAS-E66 kept R), this screen chooses
  BAS-E67's candidate. Otherwise BAS-O18 does, and this screen is an
  observation.

#### AMENDMENT 1 — 2026-11-27, before corpus C exists

- The packet's R7, after BAS-O18's gate 1 failed at the shipped vector:
  the null arm runs alone first (label `C`, from R) and gate 1 is read
  before any other arm is launched. If it passes, the arms run under `C`
  from R. If it fails, the fallback's null N2 (from N's final vector) is
  label `C2`. If N2 passes, every other arm runs under `C2` from N's
  vector and is read against N2. If N2 fails, the forms line on corpus C
  stops.
- `OPENBLAS_NUM_THREADS=2` for every arm, as in BAS-O18's run note.
- R8: every arm, the null and the controls included, is fitted `--stages
  linear,polish`, so no king-safety knob moves from R's value, which is
  the shipped value.

#### RESULT — append after exposure

2026-11-27, HEAD `704b957`, from R (`tools/results/c36-refit-R/05-releveled.txt`)
at C's frozen K (3.188600, 1.115073), stages `linear,polish`, OPENBLAS 2
threads. Report: `tools/results/c36-screen-C/report.json`, log
`tools/results/c36/reportC.log`. The loss scale is L_C(R)/0.09274536 =
0.08880492/0.09274536 = 0.957513, so δ_C = 1.915e-5.

- **Gate 1** (the null alone, first): N from R changes 0 of 1,244 active
  slots, so |L(N) − L(R)| = 0, within the limit of 9.58e-6. The arms ran
  under `C` from R.
- **Gate 2:** C1 costs 7.01e-5 and C2 costs 8.70e-5, both above δ_C.
  Pass.
- **Candidates:** one passes, **b12 `PAWN_CLASS`**, with Δ +4.10e-5 and
  interval [+3.37e-6, +8.05e-5]. One pass, so no P_C.

| arm | Δ on C | interval | Δ on A3 |
|---|---|---|---|
| b00 PHASE | −1.03e-5 | [−6.77e-5, +4.82e-5] | +5.78e-5 PASS |
| b01 TEMPO | −7.01e-5 | [−1.06e-4, −3.21e-5] | −8.65e-5 |
| b02 OCB | −4.20e-4 | [−4.77e-4, −3.62e-4] | −3.26e-4 |
| b03 SPACE | −1.43e-5 | [−3.61e-5, +9.13e-6] | +8.28e-6 |
| b04 MOBILITY | −3.26e-5 | [−8.59e-5, +1.95e-5] | −2.62e-5 |
| b05 KING_ZONE | −2.18e-5 | [−4.92e-5, +5.12e-6] | −1.04e-5 |
| b06 BAD_BISHOP | −2.91e-5 | [−6.38e-5, +6.09e-6] | −3.51e-5 |
| b07 OUTPOST | −6.55e-5 | [−1.06e-4, −2.55e-5] | −3.65e-5 |
| b08 MINOR_BEHIND_PAWN | −2.62e-5 | [−4.95e-5, −1.44e-6] | −2.05e-5 |
| b09 ROOK_QUEEN_FILE | +6.89e-6 | [−3.95e-6, +1.80e-5] | −1.10e-5 |
| b10 QUEEN_INFILTRATION | −1.27e-5 | [−2.97e-5, +5.08e-6] | −4.85e-6 |
| b11 PAWN_THREAT | −1.47e-6 | [−1.19e-5, +8.66e-6] | −1.09e-6 |
| b12 PAWN_CLASS | +4.10e-5 | [+3.37e-6, +8.05e-5] PASS | −3.07e-5 |
| b13 | 0 | — | +1.36e-6 |
| b14 | +1.27e-5 | [−6.12e-6, +3.14e-5] | +1.55e-5 |
| b16 | 0 | — | −5.22e-6 |
| b17 | +1.79e-5 | [−7.10e-6, +4.30e-5] | +1.68e-5 |
| b18 | 0 | — | −1.06e-7 |
| b19 | +2.05e-6 | [−1.62e-5, +2.10e-5] | +1.90e-5 |
| U | −5.83e-4 | [−7.02e-4, −4.53e-4] | −4.45e-4 |

- **b13 `MAJORITY` and b16 `CONNECTED_ROOKS` fail plainly:** no gain
  above the stop rule from R (Δ 0). Each restored epoch 0 in both stages,
  so its rows score exactly as R's (played hash fb1ed67763bb0dac).
  These forms occur often, and they read nothing on A either (+1.4e-6 and
  −5.2e-6), so they are not R6's class. Their only retry is §9's.
- **b18 `TRAPPED_BISHOP`** stays under R6, as on A: it almost never
  occurs on quiet rows. It also restored epoch 0, and does not pass.
- **Every arm** carries HEAD `704b957`, and every one finished. No
  arm's surface moved a king-safety slot.

**Calibration.**
- Method gates pass, 0.85: yes.
- Candidates passing, 3 (80% band 0 to 7): 1, inside the band.
- At least half of the forms that pass on A also pass on C, 0.5: no.
  b00 passes on A and reads −1.03e-5 here.
- Δ on C is smaller than on A for the replacing forms on average, 0.6:
  yes, a mean of −5.05e-5 against −4.02e-5 over bits 0 to 12.

Research's reading (2026-11-27): "The two passes are not distinguishable
from chance. Thirty-eight form reads at a one-sided 2.5% tail give up to
about one false pass by chance alone; the screens returned two, each
marginal, and neither replicates: pooled over both corpora b12 reads
+1.6e-6 (z 0.1) and b00 +2.3e-5 (z 1.1). The expectation for BAS-E67 is H0
or the cap. It is read because the rule registers it and because the
screen is a weak predictor of games (BAS-O14), not because the static
evidence supports the form."

The pooled figures are inverse-variance means of the two screens' Δ, each
sd taken as its interval's width over 3.92. They describe; they decide
nothing.

- On both corpora, five replacing forms are worse than the shipped forms:
  OCB (pooled z −17.9), TEMPO (−5.9), OUTPOST (−3.6), BAD_BISHOP (−3.0) and
  MINOR_BEHIND_PAWN (−2.9).
- Two added forms read small and positive on both corpora, below δ each
  time: b17 ROOK_CLOSED_FILE (pooled z 2.1) and b14 ISLANDS (pooled z 2.1).
  Neither passes as registered.

**What this screen decides.** If BAS-E66 keeps R, corpus C is the base
and this screen chooses BAS-E67's candidate: b12 alone. Otherwise BAS-O18
chooses (b00 alone, on the shipped base), and this screen is an
observation.

#### REVIEW NOTE — 2026-11-27 (research; the packet's Amendment 2)

The readings stand.

- **b14 and b17 are the forms' own gain.** With the form's slots set to
  0 and the arm's other slots kept, the loss is worse than R's
  (0.08881850 and 0.08885202 against 0.08880492). The fitted values
  repeat corpus A's: `ROOK_CLOSED_FILE` −29/−8 here and −30/−3 there,
  `ISLANDS` −3/+19 and +2/+21 (mg/eg, units).
- **b12.** Its two readings differ by 7.2e-5 (z 2.7), which 19 forms
  give about one time in eight.
- **BAS-E67.** Withdrawn by the maintainer on 2026-11-27, before any
  game, on research's recommendation. b12 was not read in games.
