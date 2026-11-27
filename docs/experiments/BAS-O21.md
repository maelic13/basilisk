# BAS-O21

**BAS-O21 — C.5.1.1: the same-search meter on the head, at equal time and at equal nodes**

- Date / owner / category: registered 2026-11-27, before any game;
  maintainer-run, agent-read; measurement.
- Binary: `tools/test_engines/oracle-c511-b5b8817-rebuild.exe`, SHA-256
  CD44AAC920A8C7DF214157BD5A83749D36E1C67F885C4BF8F0BEEAFF3E25A67F.
  Stockfish `9587eeeb`'s search around `b5b8817`'s `src/`,
  byte-identical, built by `tools/oracle/build_oracle.ps1` with
  BAS-O16's four patches in its order. Conformance 471,519 positions, 0
  mismatches; oracle bench 967,578.
- Arms: `Use Basilisk HCE=false`, the donor's own evaluation, against
  `Use Basilisk HCE=true`, the head's (`bench 13` 3,525,148), fixed at
  1500. `EvalAblationMask=0` on both. The first arm's rating less 1500
  is E.
- Research question: how much of the same-search deficit is left on the
  head, and how much of that is the head's evaluation being slower and
  not worse?
- Registration, hypotheses, rules and limits:
  `analysis/c511_residual_2026-11-27.md` §3 to §6 and §9.
- Two reads, 3,000 games each, one seed (`511001`) and so the same
  openings in the same order:
  - `e0`, equal time: `3+0.03`. BAS-O08's, BAS-O14's and BAS-O16's
    conditions.
  - `e0n`, equal nodes: both arms carry `nodestime=1000` and the clock
    is `8+0.08` (`tools/colosseum/gauntlet-node-clock.toml`), so each
    arm has 8,000,000 nodes and 80,000 a move, whatever its speed.
- Registered design and stop rule: no gate; one fixed run per read,
  never extended or replaced.
- Full conditions: `python tools/run_c511_reads.py --read e0` and
  `--read e0n`; Colosseum CLI, two-engine gauntlet, 1,500 cycles, UHO
  `UHO_Lichess_4852_v1.epd` in random order, margin 20 ms, 1T, Hash 64,
  concurrency 14, no adjudication, no tablebases, zero permitted faults.
- Known differences between the arms, as in BAS-O16: the head's
  evaluation has no contempt and no grain, serves Basilisk's endgame
  layer and rule-50 curve, and reaches the search about a quarter larger
  than the donor's. The bridge is 2.07% of the oracle's time (BAS-O07).
- Wire checks, before any game of a read: the binary's hash and
  manifest; the evaluation switch moves `bench 16 1 8 default depth`
  (322,185 nodes and 292,784); for `e0n`, two clock searches of the
  start position count the same nodes and doubling `nodestime` raises
  the count (415,883 twice and 1,291,759; 340,478 twice and 1,324,843).
  Dry runs accepted 2026-11-27; an eight-game smoke of `e0n` had no
  fault and no time loss, 201,107 and 197,263 nodes a move.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-27

Elo, the donor's evaluation over the head's, with an 80% band:

- `e0`: +40, +25 to +55.
- `e0n`: +24, +8 to +40.
- `e0` less `e0n`: +16, +6 to +28.
- `e0n`'s arms agree in nodes a move within 3%: 0.9.
- `e0n`'s 95% lower bound, with drawn bare-king wins rescored, is above
  +10, which opens a matched ablation by cluster: 0.6.
- Basis: BAS-O16 read +42.4 at the seed values, with the donor's arm
  1.129 times as fast. That is 0.175 of a doubling: 10 Elo by the
  textbook rule and 25 by PROCESS's.
- Falsifier: `e0` less `e0n` below zero by more than its interval. A
  slower evaluation cannot gain from equal nodes, so the node clock or
  the pairing is then wrong and neither read is used.
- Flags: `e0n` with the arms' nodes a move more than 3% apart, or with
  time losses above the wrapper's ceiling, decides nothing alone.

#### RESULT — append after exposure

#### PREDICTION CALIBRATION — preserve the original prediction above
