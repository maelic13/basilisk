# BAS-O18

**BAS-O18 — C.3.6: the own-forms screen, twenty of Basilisk's forms each fitted against a null refit of the shipped evaluation on corpus A**

- Date / owner / category: registered 2026-11-26, before any arm is
  built or fitted; maintainer-run, agent-read; evaluation/HCE, fit layer.
- Baseline source revision: the shipped evaluation after C.3.5
  (`bench 13` 3,880,333, played hash fa8739a3fc205e63, `S` = 620).
  Candidate source revision: C.3.6.2's commit, named here before the run.
- Binary: the traced tuner and release build that `tools/texel/fit.py`
  builds from that commit with `EVAL_OWN_FORMS` set per arm.
- Research question: does each of Basilisk's displaced forms, fitted,
  predict held-out results better than the shipped evaluation refitted
  the same way? The screen chooses candidates for SPRTs; it accepts
  nothing.
- Hypothesis / mechanism, interactions and competing hypotheses: the
  packet, `analysis/c36_own_forms_2026-11-26.md` §2 to §4 and §13.
- Arms (packet §6): N (null refit of the shipped vector), C1 and C2
  (positive controls: `Hanging`, `RookOnFile` fixed at 0), the twenty
  bits of §4 alone, and U (all twenty, diagnostic only).
- Inputs: corpus A (`tools/texel/data/corpusA/rows`): train 3,547,892
  rows, validation 197,277 rows used (BAS-O17's SHA-256s); surface
  `tools/texel/surface_rebuild_v1.tsv` extended by C.3.6.2 per arm.
- Fit: C.3.4's (`tools/run_c34_fits.py` common arguments: stages
  `kingsafety,linear,kingsafety,polish`, `lr` 0.5, `epochs` 20,000,
  `patience` 50, `min_delta` 1e-6, `l2` 0, `smooth` 0, `ks_positions`
  100,000, `rank_positions` 300,000, `min_support` 200), from the shipped
  vector plus each form's seeds (packet §5), K frozen at `K_eg` 3.026687,
  `K_mg` 0.970080.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- The packet's §11, which is this entry's prediction.

#### Registered decision rule

- Δ = L(N) − L(arm), the paired per-row squared error at the frozen K on
  the tuner's validation rows; 95% interval from a bootstrap over
  `game_id`, 2,000 resamples, fixed seed.
- **Method gates first.** |L(N) − L(shipped)| ≤ 1.0e-5, and at least one
  of C1, C2 has −Δ > 2.0e-5. Either failing returns C.3.6 to research
  before any candidate is read.
- **A candidate passes** when Δ ≥ 2.0e-5 and the lower bound is above 0.
  Passing candidates go to SPRTs in order of Δ (packet §7), each
  registered in its own entry before its first game.
- One run per arm, never extended or re-run on the same commit. A knob
  that ends on a bound is widened and that arm re-run, as at BAS-O17.

#### AMENDMENT 1 — 2026-11-26, before any arm

The packet's Amendment 1. On this screen it changes three things only;
the design on corpus A is otherwise as registered.
- **Execution:** arms run in parallel, each in its own local clone (or a
  no-bake mode), one build directory per mask, each with its own
  `--expect-bench`; one fit's memory is measured before the parallel
  count is chosen.
- **Gate 1 fallback:** if |L(N) − L(shipped)| > 1.0e-5, one further null
  refit N2 from N. If |L(N2) − L(N)| ≤ 1.0e-5, every arm restarts from N.
  Otherwise the forms line stops and returns to research.
- **Arm P_A:** the passing bits fitted together, when two or more pass,
  read by the same Δ. It feeds BAS-E67's candidate rule if corpus A is
  the base.
- δ's reason, corrected: it is just above the fit's path noise (1.6e-5
  between `k-phase` Run 1 and Run 2). It has no Elo anchor.
- **Arms, after R5** (the packet's Amendment 1): `ROOK_SEVENTH` is
  withdrawn, so nineteen candidate arms, and U is the nineteen: 23 arms
  before P_A.
- **The bootstrap** draws with numpy (`np.random.default_rng(360001)`),
  in chunks of 50 resamples. That changes which resamples a given seed
  draws, which is acceptable only because no arm had been read. Every arm
  draws the same games against N, so the comparison stays paired.
- **Gate 1 fallback, the command:** `python tools/run_c36_screen.py screen
  --label A2 --corpus tools/texel/data/corpusA/rows --k-phase
  3.026687,0.970080 --init tools/results/c36-screen-A/N/fit/04-polish.txt
  --arms N`; then |L(N2) − L(N)| is read from the two arms' `arm.json`.
- **Launcher:** `python tools/run_c36_screen.py screen --label A --corpus
  tools/texel/data/corpusA/rows --k-phase 3.026687,0.970080`, then
  `report` with the same arguments.

#### RUN NOTE — 2026-11-27, before any arm is read

- **The rank step stalled.** numpy's OpenBLAS (24 threads a process) spun
  against corpus C's datagen: N's `surface.py rank` took 22:38 to 02:36 UTC
  where an idle machine takes about a minute. C1 and C2 were stopped in that
  same step at 02:50 UTC, before any fit stage or loss existed. Their
  directories are kept at `tools/results/c36/aborted/A-C{1,2}-blas24/`.
  Their relaunch is therefore not a second run of a read arm.
- **Every other arm, here and on BAS-O20, runs with
  `OPENBLAS_NUM_THREADS=2`.** That is the environment only, with no code
  change, and Fable agreed to it before any arm used it. N's rank, rerun at
  two threads, wrote a byte-identical `surface.tsv` (SHA-256 `85521c25…dbe44f`
  both ways; rank 1220 of 1220, smallest relative singular value 0.00392).
  The rank step feeds the fit only through that derived surface. N stands as
  run.
- Each arm's own rank line is still read: an arm short of its free slots, or
  one that needs a slot fixed, goes to Fable before it is fitted.
- **Gate 1 at the shipped vector failed.** N went from 0.09274536 to
  0.09272609, a difference of 1.927e-5 against the 1.0e-5 limit. 22 arms
  had been launched from the shipped vector. C1, C2, b00 and b01 were
  stopped in their check or rank steps, before any fit stage, and are kept
  as `tools/results/c36/aborted/A-*-shippedinit/`. The other 18 never
  started. The fallback N2 runs as registered. The frozen prediction gave
  gate 1 a 0.9 chance: this miss is noted for calibration, and the
  prediction is not rewritten.
- **R7** (the packet's Amendment 1): if N2 passes, the restarted screen is
  label `A2` as a whole. Its null arm is N2 (init N's polish vector).
  C1, C2, the 19 bits, U and P_A run under `A2` from that same init, and
  `report --label A2` reads gate 1 as |L(N) − L(N2)| and Δ against N2.
  Label `A` keeps N alone.
- **R8** (the packet's Amendment 1), which supersedes the restart from N:
  - N ended with KdOffset on its cap (200; the seed was 99), after a
    king-safety walk the quiet rows do not identify.
  - The fit becomes `--stages linear,polish`, every other setting as
    registered (`51745f6`), so every kingsafety-class slot keeps its init
    value.
  - The screen restarts from the shipped vector as label `A3`, its null
    arm alone first, with gate 1 as registered and R7's fallback.
  - N (`A`) and N2 (`A2`) are method observations, not arms.

#### RESULT — append after exposure

Read 2026-11-27 on label `A3` (R8: `--stages linear,polish` from the
shipped vector, HEAD `879863a`, `OPENBLAS_NUM_THREADS=2`). Every arm's
`run.json` carries `879863a`, ran `linear,polish` and moved no
kingsafety-class slot, and every arm's rank was full. Report:
`tools/results/c36-screen-A3/report.json` (log
`tools/results/c36/reportA3.log`), bootstrap seed 360001, 2,000
resamples.

- **Gate 1 passes:** L(shipped) 0.09274536, L(N) 0.09274177, difference
  3.59e-6 against 1.0e-5.
- **Gate 2 passes:** C1 (`Hanging` at 0) costs 4.46e-5 (CI 1.62e-5 to
  7.23e-5), and C2 (`RookOnFile` at 0) costs 1.14e-4 (CI 7.27e-5 to
  1.56e-4).
- **One candidate passes: b00 `OWN_PHASE`**, Δ +5.78e-5, CI [+1.4e-7,
  +1.19e-4]. Its lower bound is only just above 0. With one passer there
  is no P_A.

| arm | form | Δ | 95% CI |
|---|---|---|---|
| b00 | OWN_PHASE | +5.78e-5 | [+1.4e-7, +1.19e-4] PASS |
| b19 | ROOK_BEHIND_PASSER | +1.90e-5 | [−1.4e-7, +3.72e-5] |
| b17 | ROOK_CLOSED_FILE | +1.68e-5 | [−3.7e-6, +3.89e-5] |
| b14 | ISLANDS | +1.55e-5 | [−3.9e-6, +3.39e-5] |
| b03 | SPACE | +8.3e-6 | [−1.75e-5, +3.46e-5] |
| b13 | MAJORITY | +1.4e-6 | [−1.16e-5, +1.51e-5] |
| b18 | TRAPPED_BISHOP | −1.1e-7 | [−2.6e-7, +3.6e-8] (R6: not readable by this screen, 0.06% active) |
| b11 | PAWN_THREAT | −1.1e-6 | [−9.3e-6, +6.8e-6] (R6: not readable by this screen, 3.6% active) |
| b10 | QUEEN_INFILTRATION | −4.9e-6 | [−2.47e-5, +1.46e-5] |
| b16 | CONNECTED_ROOKS | −5.2e-6 | [−9.9e-6, −7.8e-7] |
| b05 | KING_ZONE | −1.04e-5 | [−3.45e-5, +1.37e-5] (king-safety knobs frozen, R8) |
| b09 | ROOK_QUEEN_FILE | −1.10e-5 | [−2.13e-5, −1.2e-6] |
| b08 | MINOR_BEHIND_PAWN | −2.05e-5 | [−3.96e-5, +1.7e-7] |
| b04 | MOBILITY | −2.62e-5 | [−7.12e-5, +2.11e-5] |
| b12 | PAWN_CLASS | −3.07e-5 | [−6.61e-5, +3.8e-6] |
| b06 | BAD_BISHOP | −3.51e-5 | [−6.26e-5, −7.8e-6] |
| b07 | OUTPOST | −3.65e-5 | [−7.17e-5, −7.7e-7] |
| b01 | TEMPO | −8.65e-5 | [−1.23e-4, −5.03e-5] |
| b02 | OCB | −3.26e-4 | [−3.84e-4, −2.68e-4] |
| U | all nineteen | −4.45e-4 | [−5.58e-4, −3.30e-4] (diagnostic) |

- **OWN_PHASE's lower bound.** The bound's own Monte Carlo error is about
  2e-6 (2,000 resamples, bootstrap sd about 3.0e-5), so +1.4e-7 is a pass
  inside that error. It is recorded as a marginal pass. It is not a K
  artefact: both arms get the same K on every row, since the phase comes
  from the rows file, and K was fitted at the shipped vector, which if
  anything favours the null. Its Δ is a lower estimate: its linear stage
  stopped at its final epoch (974), still descending.
- **The stop rule.** In 14 of the 23 arms the linear stage ended at its
  final epoch (C1, C2, U, b00 to b05, b08, b10, b14, b17, b19). Min-delta
  1e-6 over 50 epochs stopped them while they were still descending at
  about 2e-8 an epoch. The rule is the same for every arm and understates
  a candidate's Δ, so it cannot create a pass. At that rate no failing
  replacing form could close its gap in under 1,000 epochs (b10 1,250;
  b08 2,000; b04 2,300; b01 5,300).
- **The near misses.** b19, b17 and b14 are within the fit's path noise
  of δ: rounding at the polish stage moved arms by up to 1.3e-5 either
  way. They fail as registered, and §9's retry for them is BAS-O20.
- **The singles roughly add up.** They sum to about −4.75e-4, against U's
  −4.45e-4.

- **Calibration against §11.**
  - Gate 1 at the shipped vector, as first registered (four stages),
    missed its 0.9. It passed on A3 only after R8.
  - Both controls held (0.85 and 0.6).
  - Passing forms: 1 against 3 of nineteen, inside the 80% band of 0 to
    7.
  - The largest Δ came from a replacing form, not an added one: a miss
    of the 0.6.
  - "U's Δ below the sum of the passing bits' Δ" (0.7) held:
    −4.45e-4 < +5.78e-5.
  - The registered most-likely failure mode is close to what happened:
    one form passes, and only just.
- **Next:** b00 is corpus A's forms candidate, and goes to BAS-E67 if
  corpus A stays the base (BAS-E66 not kept). Otherwise BAS-O20 on corpus
  C chooses.
