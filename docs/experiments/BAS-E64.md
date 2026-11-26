# BAS-E64

**BAS-E64 — C.3.4: `k-phase` Run 2 fitted rebuilt evaluation against the rebuilt seed**

- Date / owner / calibration category: registered 2026-11-26, before any
  game; maintainer-run, agent-read; evaluation/HCE.
- Arms: clean final-PGO PEXT Release builds (`EVAL_REBUILD=ON`, Clang
  23.1.5, dirty diff `clean`):
  - Candidate (`Basilisk k-phase fit`):
    `tools/test_engines/basilisk-c34-kphase-r2-pext-pgo.exe`, revision
    `59de7211e7f528456716f3dc5bd6eff1e7e4a235` (`59de721`), SHA-256
    `AC206511DF91F19048BE29BCBACD8EA1C29AB50D07983A9239A7A952662E2BB1`,
    `bench 13` = `3,880,333` (`+4.4%`), played hash fa8739a3fc205e63.
    Source diff vs `7d68e25`: `src/eval_rb_params.h` replaced with
    `tools/results/c34-fit-kphase-r2/eval_rb_params.h` (BAS-O17 Run 2
    `k-phase`, fitted on Corpus A under linear `K_eg = 3.026687`,
    `K_mg = 0.970080` with material gauge re-levelling).
  - Baseline (`Basilisk rebuilt seed`, fixed at 1500):
    `tools/test_engines/basilisk-c34-on-pext-pgo.exe`, revision
    `7d68e25b3174da34cdfe00035bc85a15bf0bd505` (`7d68e25`), SHA-256
    `325B660438FF44F4B8EB650114CA4D774A7AB6A0E11E817BB081BAEAC264AAF9`,
    `bench 13` = `3,717,775`, played hash e87063a94a5a10bc.
- Research question: does fitting all free slots of the rebuilt surface on
  Basilisk's own Corpus A games under phase-linear `K(phase)` preserve
  within 10 Elo of the donor's game-tuned seed weights while giving Basilisk
  its own evaluation values, and does it outperform `one-k` (`BAS-E63`)?
- Hypothesis / competing hypotheses (`analysis/c31_rebuild_2026-11-25.md`
  §11): fitting with `K_eg = 3.026687, K_mg = 0.970080` achieves lower
  validation MSE than `one-k` in every phase band while keeping marginal
  piece values within `−2.7%` to `+8.9%` of the seed in every band and
  avoiding frozen search-margin distortion. Competing hypothesis: static
  Corpus A regression still trades away subtle tactical interactions that
  only game-based tuning (SPSA) recovers (`H2`).
- Cheapest prior falsifiers / pre-game checks (passed 2026-11-26):
  - Zero artificial knob bound hits in BAS-O17 Run 2 (only `KdBlockers[0] = 0`
    at its physical non-negativity floor);
  - `test_endgames --conversion` (`60,000` nodes/move, `59de721`): `24/24`
    on `KQ-K` (11.1 plies), `KR-K` (21.3), `KBB-K` (23.9) and `KBN-K` (48.2);
  - `tests/endgames.epd`: `190/193` pass (only the 3 `SEED_SERVED` `KQ`
    vs two minors assertions differ: `+293, +358, +236` vs seed
    `+213, +333, +118`, moving `QBB` cleanly above `+150`).
- Full conditions: Colosseum CLI gauntlet (`tools/run_c34_fit_reads.py`),
  two-engine match, `1,000` cycles (`2,000` games), UHO
  `UHO_Lichess_4852_v1.epd` in random order, paired colour-reversed openings,
  shared seed `210006` (same 1,000 openings as `BAS-E63`), `3+0.03`, margin
  `20 ms`, `1T`, `Hash 64`, concurrency `14`, no adjudication, no
  tablebases, `--max-engine-faults 0`.
- Registered stop rule (`analysis/c31_rebuild_2026-11-25.md` §11): fixed
  2,000-game read, never extended or replaced. Paired with `BAS-E63`, the
  stronger fitted arm advances to the C.3.4 lazy-margin check and C.3.5
  SPRT gate; if neither `BAS-E63` nor `BAS-E64` stays within `10 Elo` of
  the seed arm (`−10` or better), C.3.4 returns to research.
- Command: `python tools/run_c34_fit_reads.py --read e64` (or `--read all`).

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- `k-phase` fit minus rebuilt seed: point `+0 Elo`, 80% band `−15` to `+15` Elo.
- Probability `k-phase` stays within `10 Elo` of the seed (`>= −10`): `0.80`;
  probability `k-phase` beats `one-k` (`BAS-E64 > BAS-E63`): `0.70`.
- Most likely failure mode: overfitting static Corpus A regressions on
  rare king-safety/passer/imbalance interactions (`ImbOurs[19]: −134 -> +878`,
  `SafeCheck[9]: 1897 -> 948`) sheds strength in dynamic search trees (`H2`).
- Falsification criteria: falling below `−15 Elo` against the seed triggers
  investigation of static-vs-game tuning gap; trailing `BAS-E63` would
  indicate that preserving donor phase balance on static MSE does not map
  to stronger play in Basilisk's search.

#### RESULT — append after exposure

- 2026-11-26. Complete: 2,000 games (`tools/results/c34-e64/`, PGN
  SHA-256 `ED1965A9…992CEBC9`), zero engine faults, zero time losses,
  recount agrees.
- **Basilisk `k-phase` fit minus Basilisk rebuilt seed: −9.6 Elo [−19.6, +0.4]**
  (95% pentanomial CI; trinomial `[−21.3, +2.1]`), score 48.6%
  (972.5 / 2,000), 564 wins, 817 draws, 619 losses, pentanomial
  [57, 250, 422, 233, 38].
- Speed and depth: `k-phase` 1.97M nodes/s (139,627 nodes/move, 70.7
  ms/move, mean non-mate depth 16.42, median 15) against rebuilt seed
  1.96M nodes/s (138,563 nodes/move, 70.7 ms/move, mean non-mate depth
  16.30, median 15), speed ratio 1.005 (unflagged). Terminations: 1,183
  checkmate, 686 threefold, 120 insufficient material, 7 fifty-move, 4
  stalemate.
- **Disposition:** `accepted` as C.3.4's fitted arm (`−9.6` clears §11's
  `>= −10` Elo return threshold and beats [BAS-E63](BAS-E63.md) by `+16.2`
  Elo on the same 1,000 UHO openings; baked into `src/eval_rb_params.h`
  at `5128e6e` and advanced to C.3.5).

#### PREDICTION CALIBRATION — preserve the original prediction above

- Observed `−9.6 [−19.6, +0.4]` against predicted `+0` (80% band `−15` to
  `+15`, `P(>= −10) = 0.80`, `P(> BAS-E63) = 0.70`): inside the 80%
  interval, `k-phase > one-k` confirmed (`+16.2`), and consistent with `H2`
  (single static regression on Corpus A sits ~10 Elo behind years of
  Stockfish game SPSA on sharp tactical weights).
- Objective follow-up: C.6 SPSA and refitting on rebuilt-engine games after
  C.3.5 ships.
