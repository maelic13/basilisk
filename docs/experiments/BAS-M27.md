# BAS-M27

**BAS-M27 — C.3.5: frontier pair, baked `k-phase` rebuilt evaluation against Stockfish 16 HCE (`3,000` games)**

- Date / owner / calibration category: registered 2026-11-26, before any
  game; maintainer-run, agent-read; measurement.
- Arms (`0374cfd`, launcher `tools/run_c35_gate.py`, `4a9c07d`):
  - Candidate (`Basilisk C.3 rebuild`):
    `tools/test_engines/basilisk-c35-cand-pext-pgo.exe`,
    `EVAL_REBUILD=ON`, SHA-256
    `DF1FAA70DFBBC30417CF238514F647822D749C979D3F98342231391BA787A473`,
    `bench 13` = `3,880,333`.
  - Baseline (`Stockfish 16 HCE`):
    `D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe`, SHA-256
    `041396A4B14EC25BAE0B57F90E917A17E7FCEB1FBBAB80859EE2EC3774D25442`,
    `Use NNUE=false` (BAS-M18 / BAS-M24 baseline).
- Research question (`analysis/c31_rebuild_2026-11-25.md` §12): how much of
  BAS-M24's `−120.0 Elo` head-to-head gap to Stockfish 16 HCE does the
  baked C.3 rebuilt evaluation close?
- Full conditions: Colosseum CLI external-Arm-B fixed match (`c35-frontier/`),
  `3,000` paired games, `3+0.03`, margin `20 ms`, `1T`, `Hash 64`, UHO
  `UHO_Lichess_4852_v1.epd` random order, concurrency `14`, seed `350003`,
  no adjudication, no tablebases, wrapper `1%` fault allowance on SF16, `0`
  on Basilisk.
- Command: `python tools/run_c35_gate.py --run frontier` (or `--run all`).

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- `Basilisk C.3 rebuild` minus `Stockfish 16 HCE` (from `c31_rebuild` §12):
  point `−40 Elo`, 80% band `−75` to `−10`.
- Basis: `BAS-M24` read `−120.0` on the head; `BAS-M25` gained `+99.8` on
  the head at seed (`+42.4` behind `9587eeeb` in `9587eeeb`'s search in
  `BAS-O16`) and `BAS-E64` read `−9.6` for `k-phase` vs seed. Non-additivity
  against SF16 HCE (BAS-M24's `2.76M` vs `1.97M` nps and deeper search)
  leaves roughly `−40 Elo` remaining for search/C.3.6/SPSA.
- Falsification criteria: worse than `−90 Elo` means the ~+90 Elo
  self-play gain failed to transfer appreciably against the frontier
  target.

#### RESULT — recorded 2026-11-26

- **Status (`tools/results/c35-frontier/`):** `completed`, **`3,000` games**
  (`1,500` pairs), `0` engine faults, `0` time losses, `0` infrastructure faults.
- **Score (`44.3%`):** `W-D-L = 737-1184-1079` (White `422-599-479`, `48.1%`;
  Black `315-585-600`, `40.5%`), `Ptnml = [113, 456, 636, 250, 45]`.
- **Elo:** **`-39.8 Elo [-48.0, -31.6]`** (95% pentanomial CI),
  `nElo = -61.1 +/- 12.4`.
- **Speed & depth (all moves):** `2.01M` vs `2.79M` nps (`137,013` vs `192,355`
  nodes/move, `68.1` vs `68.9` ms/move, mean non-mate depth `18.17` vs `18.24`,
  median `15` vs `17`; all moves depth `20.70` vs `24.77`).
- **PGN SHA-256:** `C74F38C843439C78E4EA2F8D8351678029A69DD82396220D2091BCD7184DC6DB`.
- **Assessment:** Matches the `-40` point prediction almost to the decimal
  (`-39.8`), closing `80.2 Elo` of BAS-M24's `-120.0 Elo` head-to-head gap to
  Stockfish 16 HCE.
