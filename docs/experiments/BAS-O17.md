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

##### Run 1 (`fb6f3b3`, `tools/results/c34-fit-onek/` and `tools/results/c34-fit-kphase/`)

Both arms converged cleanly by patience with exact played-hash identity across final, relevelled and baked vectors (`header_restored: true`; rank `1,220` of `1,220`; `16` sparse `Connected*` cells support-frozen in mean-anchored tables):

- **`one-k`**: `K = 2.002698`; validation MSE `0.09639910` → `0.09299295` (`−3.53%`), or `0.09390130` under `k-phase`'s `K(phase)`. Stages: KS1 pass 17 (`0.09529109`), linear epoch 4,051 (`0.0930378`), KS3 pass 15 (`0.09280965`), polish epoch 1,003 (`0.0929688`). Lazy share `14.67%` (`28,944 / 197,277`) → `13.70%` (`27,021 / 197,277`). Played hash 01b088354446dcbb (seed e87063a94a5a10bc); `bench_baked = 3,556,275` (`−4.3%`). Relevelled material (`P, N, B, R, Q`): `MgVal [77, 632, 679, 991, 2362]`, `EgVal [302, 1052, 1113, 1772, 3087]`. Marginal piece values (`tools/texel/piece_values.py`, `phase 0-7 / 8-15 / 16-24`): P `189 / 146 / 116` (seed `138 / 134 / 146`), N `570 / 589 / 448` (`457 / 546 / 563`), B `599 / 616 / 497` (`483 / 572 / 607`), R `972 / 929 / 712` (`767 / 835 / 810`), Q `1076 / 1264 / 1442` (`910 / 1116 / 1423`).
- **`k-phase`**: `K_eg = 3.026687, K_mg = 0.970080`; validation MSE `0.09466759` → `0.09276149` (`−2.01%`), or `0.09445043` under `K = 2.002698` (beats `one-k` in every phase band at each band's own K). Stages: KS1 pass 17 (`0.09415106`), linear epoch 1,770 (`0.0928207`), KS3 pass 18 (`0.09262661`), polish epoch 843 (`0.092756`). Lazy share `14.67%` → `13.95%` (`27,523 / 197,277`). Played hash c081097b94ea6882; `bench_baked = 3,928,657` (`+5.7%`). Relevelled material: `MgVal [131, 848, 921, 1337, 2559]`, `EgVal [227, 834, 877, 1411, 2683]`. Marginal piece values (`phase 0-7 / 8-15 / 16-24`): P `150 / 140 / 159`, N `454 / 550 / 586`, B `472 / 576 / 644`, R `780 / 848 / 875`, Q `946 / 1194 / 1548` (all within −2.3% to +8.9% of seed).
- **`test_eval` on baked binary**: `33` failures on each arm, matching the donor-seed parity checks expected to move; all structural, symmetry, cache, lazy, clamp and verdict checks pass.
- **Knob bounds hit (triggers Amendment 7 re-run)**:
  - `one-k` (4 on bound): `ImbOurs[17]` (`−483`, LO), `ImbOurs[18]` (`−467`, LO), `ImbOurs[19]` (`−734`, LO), `KdFlankDefence[0]` (`10`, HI); near bound: `ImbOurs[20]` (`564 / 594`), `KdKnightDefender[0]` (`186 / 200`).
  - `k-phase` (4 on bound): `ImbOurs[19]` (`466`, HI), `KdKnightDefender[0]` (`200`, HI), `KdFlankDefence[0]` (`10`, HI), and `KdBlockers[0]` (`0`, LO — physical non-negativity floor: a blocker shielding our king from a slider does not increase safety, so it is not widened below zero).
- **Separate non-blocking observation**: `ThreatByMinorMg[6]`, `ThreatByMinorEg[6]`, `ThreatByRookMg[6]`, `ThreatByRookEg[6]` (the enemy king slot, `seed = 0`, 368–728 rows of train support from bishop/rook x-rays through a pinned enemy queen) moved to `+638..+902` on `one-k` and `+465..+569` (for the top-moving `ThreatByRookEg/Mg[6]`, `ThreatByMinorMg[6]`) on `k-phase`. Left unchanged in Run 2 so Run 2 isolates knob bounds alone; flagged to maintainer separately.
- **Run 1 prediction calibration (prospective section above unchanged)**: Both losses, marginal piece-value shifts (`+18%` to `+37%` in `phase 0-7`, `−12%` to `−21%` for P/N/B/R in `phase 16-24` under `one-k`; within `−2.3%` to `+8.9%` across all cells under `k-phase`), convergence and played-hash identity all landed inside the registered 80% bands. Disposing Run 1 is deferred pending Run 2 with widened knob bounds; conversion floors and paired games were not yet run.

##### Amendment 1 — Run 2 (widened knob bounds, 2026-11-25)

- **Trigger**: Amendment 7, point 5 ("A knob that ends on a bound is widened and the fit repeated") after Run 1's four bound hits (`ImbOurs[17..19]`, `KdFlankDefence`, `KdKnightDefender`).
- **Source & surface**: tooling commit `2e77b9a` (`src/` unchanged from `7d68e25`; `tools/texel/surface_rebuild_v1.tsv` SHA-256 `B4486569…`; Run 1 artifacts preserved in `c34-fit-onek/` and `c34-fit-kphase/`).
- **Widened bounds (`tools/texel/surface.py`)**:
  - `RB_IMBALANCE_REACH`: `600` → `1500` (`step = 300`, `ImbOurs[0..20]` and `ImbTheirs[0..14]`);
  - `KdKnightDefender[0]`: `[0, 200]` → `[0, 500]` (`step = 50`);
  - `KdFlankDefence[0]`: `[0, 10]` → `[0, 40]` (`step = 4`);
  - `KdBlockers[0]` held at `[0, 200]` (zero is a physical floor).
- **Outputs**: `tools/results/c34-fit-onek-r2/` and `tools/results/c34-fit-kphase-r2/`.
- **Stop rule**: if any imbalance knob reaches its `±1500` bound in Run 2, treat `Q × {N, B, R}` as near-collinear with `PstMgQueen` / phase and return to research before playing games; otherwise proceed to the deterministic conversion floors (`KBNK`, `KQK`, `KRK`, `KBBK` at 60k nodes), `KQ` vs two minors re-read, and the 2,000-game paired runs against the seed arm.
- **Command**: `python tools/run_c34_fits.py --arm all`
