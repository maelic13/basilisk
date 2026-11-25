# BAS-O17

**BAS-O17 — C.3.4: matched static fits of the rebuilt evaluation on corpus A (`one-k` and `k-phase`)**

- Date / owner / category: registered 2026-11-25, before any fit run;
  maintainer-run, agent-read; measurement, fit layer.
- Source revision: `663e381` (`src/` unchanged from `7d68e25`; `EVAL_REBUILD=ON`,
  `bench 13` = 3,717,775; seed played hash e87063a94a5a10bc; conversion `S = 620`).
- Inputs:
  - Corpus A (`tools/texel/data/corpusA/rows`, `basilisk-texel-rows-v1`):
    3,547,892 used train rows (SHA-256 `32CD3D63…`), 197,277 used
    validation rows (SHA-256 `851B6FF8…`);
  - surface `tools/texel/surface_rebuild_v1.tsv` (`1,450` slots: `129` fixed,
    `59` free `kingsafety` knobs, `1,262` free `linear` → `1,246` after
    support freeze of 16 sparse `Connected*` cells in mean-anchored
    tables; `26` mean anchors; rank `1,220` of `1,220` on 300,000 rows);
  - seed vector: `src/eval_rb_params.h` (`9587eeeb` constants in units).
- Knob-bound review (Amendment 7, point 5): all 59 `kingsafety` knobs in
  `surface_rebuild_v1.tsv` reviewed and accepted as registered:
  - `ImbOurs[0..20]` (21) and `ImbTheirs[0..14]` (15): `[seed − 600, seed + 600]`,
    first step `120`;
  - `KingAttackWeight[2..5]` (4): `[0, 200]`, first step `20` (seeds `81, 52, 44, 10`);
  - `SafeCheck[4..11]` (8): `[0, max(2 × seed, 100)]`, first step `10%` of range;
  - `Kd*` (10): `KdWeakRing` `[0, 400]`, `KdUnsafeChecks` `[0, 300]`,
    `KdBlockers` `[0, 200]`, `KdKingAttacks` `[0, 150]`, `KdFlankAttack`
    `[0, 8]`, `KdNoQueen` `[0, 1800]`, `KdKnightDefender` `[0, 200]`,
    `KdShelter` `[0, 16]`, `KdFlankDefence` `[0, 10]`, `KdOffset` `[−200, 200]`;
  - `SpaceWeight[0]` (1): `[0, 64]`, first step `6` (seed `16`).
  Any knob ending on a bound after fitting triggers widening and re-running
  that arm.
- Material gauge re-levelling (Amendment 7): at bake, each piece PST's
  live-square mean (`8..55` for pawns, `0..63` for knight..queen, rounded)
  is added to `MgVal[1..5]` / `EgVal[1..5]` and subtracted from its live
  squares, verified by identical `--played-hash` before and after.
- Schedule (`tools/texel/fit.py` via `tools/run_c34_fits.py --arm all`):
  `kingsafety,linear,kingsafety,polish`, `lr=0.5` (≈ 0.30 cp/step),
  `epochs=20000`, `patience=50`, `min_delta=1e-6`, `l2=0`, `smooth=0`,
  `max_positions=0`, `ks_positions=100000`, `rank_positions=300000`,
  `min_support=200`.
- Arms:
  - `one-k` (`tools/results/c34-fit-onek/`): single constant `K` fitted on
    validation at the seed (`K = 2.002698`, seed loss `0.09639910`).
  - `k-phase` (`tools/results/c34-fit-kphase/`): linear-in-phase
    `K(phase) = K_eg + (K_mg − K_eg) × phase / 24`, fitted once on
    validation at the seed (`K_eg = 3.026687`, `K_mg = 0.970080`, seed
    loss `0.09466759`).
- Pre-bake reads and next step: validation loss under both K metrics,
  marginal piece values (`tools/texel/piece_values.py`), lazy-checkpoint
  share, knob-bound check, deterministic floors (`KBNK`, `KQK`, `KRK`,
  `KBBK` at 60k nodes), and `KQ` vs two minors re-read; then 2,000-game
  paired runs of each fitted arm against the seed arm
  (`analysis/c31_rebuild_2026-11-25.md` §11: return to research if
  neither stays within 10 Elo of the seed arm).
- Command: `python tools/run_c34_fits.py --arm all`.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-25

- Seed calibration reproduces `K = 2.002698` (loss `0.09639910`) for
  `one-k` and `K_eg ≈ 3.027`, `K_mg ≈ 0.970` (loss `0.09466759`) for
  `k-phase`.
- Both arms converge by patience (no exit 3) and hold exact `--played-hash`
  identity across material re-levelling and bake.
- `one-k` reduces validation MSE under single `K` by 2.0% to 3.5% (80%
  band), shifting endgame marginal piece values up by +15% to +40% and
  middlegame values down by −15% to −35% to compress the 3.1× phase-K
  gap.
- `k-phase` reduces validation MSE under linear `K(phase)` by 0.8% to
  2.2% (80% band), keeping marginal piece values within ±15% of the seed
  in every phase band.
- Both arms pass all four deterministic conversion floors (`KBNK`, `KQK`,
  `KRK`, `KBBK` at 60,000 nodes).
- Subsequent paired-game prior (for the 2,000-game runs that follow):
  `k-phase` beats `one-k` (probability 0.65), because keeping the donor's
  phase balance avoids distorting the frozen search margins and lazy
  exits.

#### RESULT — append after exposure
