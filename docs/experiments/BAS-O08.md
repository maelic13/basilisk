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
- Run 2026-11-18 by the maintainer (`tools/results/c0-meter`, PGN
  `b766e934…`): 3,000 games, no faults, no time losses; the recount
  agrees.
- `9587eeeb`'s evaluation over Basilisk's: 1940-785-275, 77.75%,
  **E(0) = +217.3 [+207.1, +228.0]**, pentanomial [5, 35, 299, 612, 549];
  as a rating +216.8 ± 14.9.
- Speed: 2.50M nodes a second with Basilisk's evaluation against 2.85M,
  12.3% slower (14.0% counted from the slower arm); 177,143 nodes a move
  against 186,902. Under the registered 15%, so no equal-node companion.
- Sixteen of the Basilisk arm's draws are elementary mates lost to the
  fifty-move rule (KR-K 15, KBB-K 1). The bridge clamps a known-win score
  one unit under Stockfish's `VALUE_KNOWN_WIN`, and the search then cuts
  the ending off on its static score. Counted as wins, E(0) is +214.6.
- Post hoc, not registered (the packet's §5.7): 62 to 64% of the excess
  score is realised at Basilisk phase 20–24 and 68 to 69% at phase 18–24,
  under four calibrations; the control between equals reads within ±0.024
  a band.
- Against the donor BAS-O09 kept, `sf_16`'s evaluation, the meter is
  E(0) + `a95` = +215.7 [+202.7, +228.7].
- Disposition: COMPLETE, a measurement. E(0) replaces 232.8 in PLAN §1's
  budget row.

#### PREDICTION CALIBRATION — preserve the original prediction above

- +190 was predicted, 80% band +135 to +245: held, 27 above the point.
  P(below 232.8) = 0.75: held. No falsifier was reached.
- What the model had wrong: it took +30 to +50 of evaluation gains since
  BAS-O02 off 232.8. BAS-O02's interval is ±32, so the comparison cannot
  say how much of those gains this search sees.
- Conditional lesson: a 400-game meter cannot anchor a 40-Elo prediction.
  This read, ±10.5, is the baseline.
- Objective retry trigger: C's checkpoint, on this binary, reporting the
  count of clamped draws beside the figure.
