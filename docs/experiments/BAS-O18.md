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

#### RESULT — append after exposure
