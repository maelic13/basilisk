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
- **Launcher:** `python tools/run_c36_screen.py screen --label A --corpus
  tools/texel/data/corpusA/rows --k-phase 3.026687,0.970080`, then
  `report` with the same arguments.

#### RESULT — append after exposure
