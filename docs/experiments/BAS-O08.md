# BAS-O08

**BAS-O08 — the same-search evaluation deficit E(0) on the 1.11.0 head (BAS-O02 re-read)**

- Date / owner / calibration category: registered 2026-11-18, before any
  game; maintainer-run, agent-read; measurement.
- Binary: `tools/test_engines/oracle-b111-4d2742f.exe`, SHA-256
  `8ADEE9AE…`, Stockfish `9587eeeb`'s search around the head's evaluation
  (`src/eval.cpp` differs from `4d2742f` in comments only), played against
  itself with `Use Basilisk HCE=false` and `=true`.
- Research question: how many Elo is `9587eeeb`'s evaluation worth over
  Basilisk's inside one search, on this head?
- Registration, confounds, prediction (+190, 80% band +135 to +245) and
  falsifiers: `analysis/c0_evaluation_programme_2026-11-18.md` §4.2,
  frozen in the commit that adds this file.
- Registered design and stop rule: no gate; one fixed run of 3,000 games,
  never extended or re-run on the same binary.
- Full conditions: `python tools/run_c0_reads.py --read meter`; Colosseum
  CLI, two-engine gauntlet, 1,500 cycles, UHO `UHO_Lichess_4852_v1.epd` in
  random order, seed `120001`, `3+0.03`, margin 20 ms, 1T, Hash 64,
  concurrency 14, no adjudication, no tablebases, zero permitted faults.

#### RESULT — append after exposure

- **Instrument note, appended 2026-11-18 before any game.** In B.11.1's
  two G(0) runs the oracle on Basilisk's evaluation drew 51 of 6,000 games
  with a bare-king win on the board (KR-K 22, KBN-K 15, KBB-K 7, KQ-K 3,
  KRN-K or KRB-K 4); Basilisk's own search converted every such row in
  three gates (BAS-E58). The `true` arm carries that here, about 3 Elo of
  E(0). The prediction is not changed.
- Wire check at launch: `bench 16 1 8 default depth` reads 322,185 nodes
  with the oracle's own evaluation and 266,667 with the head's. Dry run
  accepted 2026-11-18.
