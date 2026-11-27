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
