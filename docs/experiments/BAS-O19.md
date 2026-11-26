# BAS-O19

**BAS-O19 — C.5.2's first cycle, pulled forward into C.3.6: the shipped evaluation refitted on corpus C, the rebuilt engine's own games**

- Date / owner / category: registered 2026-11-26, before corpus C
  exists; agent-run overnight with the maintainer's leave; data/tuning,
  fit layer.
- Source revision: the shipped evaluation (`bench 13` 3,880,333, played
  hash fa8739a3fc205e63) at the C.3.6.2 commit, mask 0.
- Corpus C: corpus A's recipe with only the engine changed. That is
  `tools/datagen.ps1`, 10 slices of 100,000 games from corpus A's book
  slices (`tools/texel/data/corpusA/books/slice00..09.epd`) at corpus
  A's seeds, 8,000 nodes a move, Hash 16, 1T, no adjudication, with the
  shipped PGO PEXT build as engine. Rows by `tools/texel/extract.py` at
  corpus A's settings and split rule, with corpus A's audits.
  Asserted before the fit: C's validation start set equals A's.
- Research question: does the shipped evaluation, refitted on the
  current engine's games, predict them better, and does that refit play
  better (BAS-E66)?
- Fit: C.3.4's method (stages `kingsafety,linear,kingsafety,polish`,
  `lr` 0.5, `epochs` 20,000, `patience` 50, `min_delta` 1e-6, no
  regularisation, the knob bounds of `surface_rebuild_v1.tsv`) from the
  shipped vector. K is linear in the phase, fitted once on C's
  validation at the shipped vector and frozen. The material gauge is
  re-levelled at the bake.
- Reported beside it: the shipped vector's loss on C at C's K pair and
  at C.3.4's pair (`K_eg` 3.026687, `K_mg` 0.970080).
- Interactions: every evaluation slot; the search's frozen margins read
  the score's scale, which the frozen K protects; the endgame layer is
  outside the fit.
- Competing hypotheses: the shipped vector is already near C's optimum,
  since the same function played both corpora; or C's stronger play
  moves the piece values and the king-safety knobs, and the refit gains.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- The refit lowers C's validation loss against the shipped vector at C's
  frozen K by 0.5%, with an 80% band of 0.1% to 1.5%. C.3.4's fit from
  the donor's seeds moved 2.03%.
- C's K pair is within 10% of C.3.4's in both halves: 0.7.
- Marginal piece values within ±15% of the shipped vector in every phase
  band: 0.85. No knob ends on a bound: 0.7.

#### Registered rule

- Stop before games if a piece value leaves the ±15% band in any phase
  band, or if a knob ends on a bound after one widened re-run. Either
  is a consultation with research (the packet's Amendment 1).
- Otherwise R goes to BAS-E66's pre-game checks. One run, never
  extended or re-run on the same corpus and commit.

#### RESULT — append after exposure
