# BAS-O14

**BAS-O14 — C.2.10: the matched evaluation ablation by family on the refitted head**

- Date / owner / category: registered 2026-11-24, before any game;
  maintainer-run, agent-read; measurement.
- Binary: `tools/test_engines/oracle-c29-e56f1e1-evalablate.exe`,
  SHA-256 `CAA8B70B…`, Stockfish `9587eeeb`'s search around the head's
  evaluation, played against itself with `Use Basilisk HCE=false` and
  `=true` at one `EvalAblationMask`.
- Research question: how much of the same-search evaluation deficit does
  each family, and each cluster, explain? M(mask) = E(0) − E(mask).
- Reads: masks 0, 7, 8, 48, 64, 127, 2, 1, 4, 16 and 32, in that order,
  3,000 games each.
- Registration, confounds, the decision rule, predictions and
  falsifiers: `analysis/c210_family_order_2026-11-24.md`, frozen in the
  commit that adds this file.
- Registered design and stop rule: no gate; one fixed run per read,
  never extended or re-run on the same binary.
- Full conditions: `python tools/run_c210_reads.py --read all`; Colosseum
  CLI, two-engine gauntlet, 1,500 cycles, UHO `UHO_Lichess_4852_v1.epd`
  in random order, seed `210001` for every read, `3+0.03`, margin 20 ms,
  1T, Hash 64, concurrency 14, no adjudication, no tablebases, zero
  permitted faults.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-24

Elo, with an 80% band.
- E(0): +160, +130 to +190.
- M(7), mobility, space and threats: +55, +25 to +90.
- M(8), king: +20, −5 to +45.
- M(48), pawns and passers: +12, −15 to +40.
- M(64), pieces: +5, −12 to +22.
- M(2), mobility: +35, +10 to +65.
- M(1), space: +12, −5 to +30.
- M(4), threats: +8, −12 to +28.
- M(16), pawn structure: +8, −12 to +28.
- M(32), passers: +5, −18 to +28.
- E(127): +70, +35 to +110.
- C.3's is the largest cluster meter: 0.7. The frozen rule leaves the
  order unchanged: 0.75. Mobility's single meter exceeds space's: 0.75.
  At least three single meters include zero: 0.8.
- Falsifiers: E(0) above +205; E(0) − E(127) below 40.

#### RESULT — append after exposure
