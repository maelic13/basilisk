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

#### AMENDMENT 1 — 2026-11-27, before corpus C exists

- The packet's R8. BAS-O18's null refit on corpus A walked the
  king-safety knobs (KdOffset 99 to its cap of 200) for almost no loss, a
  walk that would happen on either corpus. So R is fitted `--stages
  linear,polish` from the shipped vector, every king-safety knob stays at
  its shipped value, and every other setting is as registered. BAS-E66
  plays that R.
- Clerical: "no knob ends on a bound", in the rule and in the prediction,
  no longer applies, since no knob is fitted. The prediction is not
  rewritten.

#### RESULT — append after exposure

2026-11-27. Corpus C, then R, at HEAD `704b957`.

**Corpus C.** Ten slices of 100,000 games, every one ending by the rules
with 0 faults. The engine is the shipped PGO PEXT build (`B8D7A8EE…`),
and each slice's book is byte-identical to corpus A's, at A's seed. The
extractor is unchanged since `2193afb`, and its audit has A's kind:

| | A | C |
|---|---|---|
| rejected games | 0 | 0 |
| no candidate position | 4,023 | 3,608 |
| candidates | 6,016,141 | 6,078,148 |
| quiet rejected | 1,673,189 | 1,714,661 |
| duplicates | 20,976 | 20,988 |
| validation games with rows, W / D / L | 18,676 / 15,220 / 15,108 | 18,735 / 15,837 / 14,488 |
| tablebase mislabel rate, slice 00, 20,000 games | 2.54% | 1.96% |

- **The splits.** The split is a function of the start position alone,
  and the books are identical. No start lies in different splits of the
  two corpora: the overlap is 0 for all six cross-split pairs. So C's
  validation start assignment equals A's.
  - The row-level start sets differ only by sampling: 49,004 in A,
    49,060 in C, 48,299 in both
    (`tools/results/c36/corpusC_startsets.json`).
- **`--verify`** is exact on all 197,304 rows of C's validation, at the
  seed and at the bake.

**R** (`tools/results/c36-refit-R/run.json`). Stages `linear,polish` (the
amendment), OPENBLAS 2 threads, in 3.5 minutes. The table's losses are
on C's validation.

| | value | prediction |
|---|---|---|
| C's K pair at the shipped vector | K_eg 3.188600, K_mg 1.115073 | within 10% of C.3.4's pair (3.026687, 0.970080) in both halves, 0.7: missed in mg (+5.3%, +14.9%) |
| loss at C's K, shipped / R | 0.08892371 / 0.08880492 | |
| R's change | −1.19e-4, −0.134% | −0.5%, 80% band −0.1% to −1.5%: inside, near its small end |
| loss at C.3.4's K, shipped / R | 0.08902715 / 0.08888252 | |
| marginal piece values, every cell (`piece_values.py`, base against R) | within ±3% | within ±15%, 0.85: met |

- **The bake.**
  - Played hash fb1ed67763bb0dac is identical at the final, the
    re-levelled and the baked vector.
  - The re-levelling shifts are MgPawn +9, EgPawn +6, and every other
    shift within ±5.
  - `bench 13` 3,525,148 (from 3,880,333); `test_eval` 47/47.
- **The king-safety knobs.** None moved. KdBlockers sits on its lower
  bound at 0, its shipped value.

The registered rule is met: no piece value is outside the band, and no
fitted knob is on a bound. R goes to BAS-E66's pre-game checks.

#### REVIEW NOTE — 2026-11-27 (research; the packet's Amendment 2)

The result stands. Two additions:

- **What R did not fit.** The amendment's stage holds 59 knobs: 22 of
  king danger, 36 of the material imbalance and `SpaceWeight`. R is a
  refit of the 1,244 free linear slots, and those 59 keep C.3.4's
  values.
- **R by phase.** Paired against the shipped vector on C's validation,
  with a bootstrap over games, R is better in every phase group from 2
  to 24 and worse at phases 0 and 1: −2.60e-4, interval [−4.13e-4,
  −1.08e-4], on 17,473 rows. It is worse there at its own best K too.
  By class, it lost in pawns-only endings, one-minor endings and the
  uneven phase-2 class, and gained in rook against rook and opposite
  bishops. The packet's Amendment 2 has both tables; the recipe and the
  outputs are in `tools/results/c36/review/`.
