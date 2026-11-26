# BAS-M26

**BAS-M26 — C.3.5: baked `k-phase` rebuilt evaluation against the head at `10+0.1` (`1,000` games)**

- Date / owner / calibration category: registered 2026-11-26, before any
  game; maintainer-run, agent-read; measurement.
- Arms: same pinned clean final-PGO PEXT builds at `0374cfd` as
  [BAS-E65](BAS-E65.md) (`basilisk-c35-cand-pext-pgo.exe`, SHA-256
  `DF1FAA70…787A473`, `bench 13` = `3,880,333` against
  `basilisk-c35-base-pext-pgo.exe`, SHA-256 `0A4D9CCB…F1C4F20`,
  `bench 13` = `3,359,329`), launcher `tools/run_c35_gate.py` (`4a9c07d`).
- Research question (C-99): does the rebuilt evaluation's gain over the
  head hold at longer time control (`10+0.1`)?
- Full conditions: Colosseum CLI `match` (`match-fixed-ltc.toml`),
  `1,000` paired games (`c35-ltc/`), `10+0.1`, margin `20 ms`, `1T`,
  `Hash 64`, UHO `UHO_Lichess_4852_v1.epd` random order, seed `350002`,
  no adjudication, no tablebases, `--max-engine-faults 0`.
- Command: `python tools/run_c35_gate.py --run ltc` (or `--run all`).

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- `Basilisk C.3 rebuild` minus `Basilisk head` at `10+0.1`: point
  `+85 Elo`, 80% band `+55` to `+115`.
- Falsification criteria: a severe drop below `+40 Elo` at `10+0.1`
  indicates adverse TC scaling on the `-4.5%` slower rebuilt eval or
  horizon-sensitive endgame distortion.

#### RESULT — append after exposure
