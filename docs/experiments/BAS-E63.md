# BAS-E63

**BAS-E63 — C.3.4: `one-k` Run 2 fitted rebuilt evaluation against the rebuilt seed**

- Date / owner / calibration category: registered 2026-11-26, before any
  game; maintainer-run, agent-read; evaluation/HCE.
- Arms: clean final-PGO PEXT Release builds (`EVAL_REBUILD=ON`, Clang
  23.1.5, dirty diff `clean`):
  - Candidate (`Basilisk one-k fit`):
    `tools/test_engines/basilisk-c34-onek-r2-pext-pgo.exe`, revision
    `902c681e5f6004ebbe1affee71ca657ec0278259` (`902c681`), SHA-256
    `F1D0178043493B8C9DB4972B9FA1BD0122B4E94C0BEA70A6DF5446EE5657AD72`,
    `bench 13` = `3,882,274` (`+4.4%`), played hash bb6c6b079c61a6d2.
    Source diff vs `7d68e25`: `src/eval_rb_params.h` replaced with
    `tools/results/c34-fit-onek-r2/eval_rb_params.h` (BAS-O17 Run 2
    `one-k`, fitted on Corpus A under single `K = 2.002698` with material
    gauge re-levelling).
  - Baseline (`Basilisk rebuilt seed`, fixed at 1500):
    `tools/test_engines/basilisk-c34-on-pext-pgo.exe`, revision
    `7d68e25b3174da34cdfe00035bc85a15bf0bd505` (`7d68e25`), SHA-256
    `325B660438FF44F4B8EB650114CA4D774A7AB6A0E11E817BB081BAEAC264AAF9`,
    `bench 13` = `3,717,775`, played hash e87063a94a5a10bc.
- Research question: does fitting all free slots of the rebuilt surface on
  Basilisk's own Corpus A games under a single constant `K = 2.002698` keep
  within 10 Elo of the donor's seed weights in Basilisk's search, and how
  does it compare against `k-phase` (`BAS-E64`)?
- Hypothesis / competing hypotheses (`analysis/c31_rebuild_2026-11-25.md`
  §11, BAS-O15 control): fitting to Corpus A gives Basilisk its own values,
  but compressing the seed's `3.1×` phase slope (`K_eg = 3.027`,
  `K_mg = 0.970`) into a single `K` raises endgame marginal piece values
  (`phase 0-7`, `+16%` to `+38%`) and lowers middlegame P/N/B/R values
  (`phase 16-24`, `−14%` to `−22%`), risking miscalibration against fixed
  search margins and lazy thresholds (`14.67% -> 13.73%`). Competing
  hypothesis: search adapts cleanly to the `one-k` scale and corpus-specific
  weights offset the phase-K compression.
- Cheapest prior falsifiers / pre-game checks (passed 2026-11-26):
  - Zero artificial knob bound hits in BAS-O17 Run 2;
  - `test_endgames --conversion` (`60,000` nodes/move, `902c681`): `24/24`
    on `KQ-K` (11.1 plies), `KR-K` (21.3), `KBB-K` (23.9) and `KBN-K` (48.2);
  - `tests/endgames.epd`: `190/193` pass (only the 3 `SEED_SERVED` `KQ`
    vs two minors assertions differ: `+255, +327, +151` vs seed
    `+213, +333, +118`).
- Full conditions: Colosseum CLI gauntlet (`tools/run_c34_fit_reads.py`),
  two-engine match, `1,000` cycles (`2,000` games), UHO
  `UHO_Lichess_4852_v1.epd` in random order, paired colour-reversed openings,
  shared seed `210006` (same 1,000 openings as `BAS-E64`), `3+0.03`, margin
  `20 ms`, `1T`, `Hash 64`, concurrency `14`, no adjudication, no
  tablebases, `--max-engine-faults 0`.
- Registered stop rule (`analysis/c31_rebuild_2026-11-25.md` §11): fixed
  2,000-game read, never extended or replaced. Paired with `BAS-E64`, the
  stronger fitted arm advances to the C.3.4 lazy-margin check and C.3.5
  SPRT gate; if neither `BAS-E63` nor `BAS-E64` stays within `10 Elo` of
  the seed arm (`−10` or better), C.3.4 returns to research.
- Command: `python tools/run_c34_fit_reads.py --read e63` (or `--read all`).

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- `one-k` fit minus rebuilt seed: point `−8 Elo`, 80% band `−25` to `+10` Elo.
- Probability `one-k` stays within `10 Elo` of the seed (`>= −10`): `0.55`;
  probability `one-k` beats the seed (`> 0`): `0.25`.
- Most likely failure mode: single `K` rescales endgame values up and
  middlegame minor/rook values down across frozen search margins and lazy
  exits (`14.67% -> 13.73%`), costing strength relative to `k-phase`.
- Falsification criteria: falling below `−25 Elo` confirms phase-K
  distortion harms play in Basilisk's search; beating the seed by `> +10 Elo`
  falsifies the concern that static corpus fits lose donor game-tuned synergy.

#### RESULT — append after exposure

- 2026-11-26. Complete: 2,000 games (`tools/results/c34-e63/`, PGN
  SHA-256 `BDE51A55…75C517AF`), zero engine faults, zero time losses,
  recount agrees.
- **Basilisk `one-k` fit minus Basilisk rebuilt seed: −25.8 Elo [−35.7, −15.9]**
  (95% pentanomial CI; trinomial `[−37.5, −14.1]`), score 46.3%
  (926 / 2,000), 512 wins, 828 draws, 660 losses, pentanomial
  [62, 287, 420, 199, 32].
- Speed and depth: `one-k` 1.97M nodes/s (139,455 nodes/move, 70.9
  ms/move, mean non-mate depth 16.50, median 15) against rebuilt seed
  1.95M nodes/s (138,099 nodes/move, 70.6 ms/move, mean non-mate depth
  16.33, median 15), speed ratio 1.010 (unflagged). Terminations: 1,172
  checkmate, 689 threefold, 119 insufficient material, 17 fifty-move, 3
  stalemate.
- **Disposition:** `rejected` (misses the `>= −10` threshold and trails
  [BAS-E64](BAS-E64.md) by 16.2 Elo on the same 1,000 UHO openings).

#### PREDICTION CALIBRATION — preserve the original prediction above

- Observed `−25.8 [−35.7, −15.9]` against predicted `−8` (80% band `−25`
  to `+10`, falsifier below `−25`): direction correct, magnitude slightly
  worse than the 80% lower bound, confirming that compressing the 3.1×
  phase slope of `K(phase)` under a single `K` harms play across frozen
  search margins.
- Closed for C.3.4; no retry needed once `k-phase` is selected.
