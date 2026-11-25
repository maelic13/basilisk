# BAS-O15

**BAS-O15 — C.3.1: the transplant read, the donor's passers and king safety on Basilisk's evaluation**

- Date / owner / category: registered 2026-11-25, before any game;
  maintainer-run, agent-read; measurement.
- Binary: `tools/test_engines/oracle-c31-3d2d8f0-transplant.exe`,
  SHA-256 ECF6C8BD71BB011899F0AFCBF5431C2100D500C8DE74A18BA11CD87FC37F1C4D,
  built by `tools/oracle/build_oracle.ps1` with
  `oracle-eval-ablate.patch` then `oracle-eval-transplant.patch` around
  `3d2d8f0`'s `src/`, byte-identical to `e56f1e1`'s (conformance 471,519
  positions, 0 mismatches; oracle bench 1,017,388). Stockfish
  `9587eeeb`'s search; Basilisk's evaluation at the accepted head,
  `bench 13` 3,359,329. The six zero-game checks pass on it
  (`tools/diag/c31_checks.py`, the packet's implementation record).
- Research question: do the donor's passers and king safety, set on
  Basilisk's own evaluation, come close to the donor's whole evaluation?
  The answer sizes the port: two families, or the whole rebuild accepted
  on 2026-11-25.
- The transplant: T(S) = Basilisk's evaluation with its families S
  removed, plus the donor's own `Score` for those families, tapered by
  the donor's phase and scale factor and scaled by 697/1024 on the
  middlegame half and 2022/1024 on the endgame half. Where the donor's
  whole endgame score is zero, the factor is the one its rule gives the
  side the transplanted terms favour (the packet's Amendment 1).
- Reads, in this order, 3,000 games each:
  - `t40-b`: T(40) against Basilisk's evaluation; g, the gain.
  - `d-t40`: the donor's evaluation against T(40); s, the shortfall.
  - `d-t127`: the donor's evaluation against T(127); c, the control.
- Registration, the scales' rule, the zero-game checks, confounds, the
  decision rule, predictions and the falsifier:
  `analysis/c31_transplant_2026-11-25.md`, frozen in the commit that adds
  this file.
- Registered design and stop rule: no gate; one fixed run per read,
  never extended or re-run on the same binary.
- Full conditions: Colosseum CLI, two-engine reads, 1,500 cycles, UHO
  `UHO_Lichess_4852_v1.epd` in random order, seed `210003` for every
  read, `3+0.03`, margin 20 ms, 1T, Hash 64, concurrency 14, no
  adjudication, no tablebases, zero permitted faults,
  `EvalMatchedCost=true` on both arms. The launcher is
  `tools/run_c31_reads.py`; the command is
  `python tools/run_c31_reads.py --read all`, and the table
  `python tools/diag/c31_table.py`.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-25

Elo, with an 80% band.
- g, T(40) over Basilisk's evaluation: +75, +30 to +120.
- s, the donor's evaluation over T(40): +55, +15 to +100.
- c, the donor's evaluation over T(127): +15, −20 to +50.
- The rule's outcome: two families suffice (s ≤ 30) 0.35; the whole
  (g ≤ 40 or under 40% of g + s) 0.25; staged 0.40.
- The control passes (c ≤ 40): 0.8.
- Falsifier of the instrument: g below zero with c above 60.

#### RESULT — append after exposure
