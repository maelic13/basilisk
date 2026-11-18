# BAS-D22

**BAS-D22 — B.11.1 fixed-node meters on the head: depth at 300k nodes, branching profile, endgame truth cohort**

- Date / owner / calibration category: run 2026-11-17 by the agent while the
  host was in use (deterministic: node budgets, one thread, no clock);
  diagnostic meters, not a gate.
- **Deviation:** run before this entry existed, so no prediction was frozen.
  They are recorded as unpredicted meter reads and carry no calibration.
- Binaries: the head at `4c0bc0e` (engine source master's `4d2742f`):
  Release `basilisk-b111-head-pext-pgo.exe` (`31BC229A…`) and Diag
  `basilisk-b111-head-pext-diag-pgo.exe` (`3305BA59…`), both clean, bench
  4,777,412; the oracle `oracle-b111-4d2742f.exe` (`8ADEE9AE…`, BAS-O06).
  Baselines are the B.0 reads of 1.10.1 (`tools/results/b0-20261006/`) and
  BAS-E47, not re-run.

#### RESULT

- **Depth at 300k nodes and best-move agreement** (`tools/diag/run_suite.py
  --depth 14 --nodes 300000 --hash 64`, stride 1, `suite_v2.epd`, 106
  positions, Diag head): mean completed depth **30.41** against the oracle's
  **33.75**; best moves agree on **80/105** (76.2%); LMR identity holds. On
  1.10.1 (B.0): 20.62 against 32.84, 66/105 (62.9%). The gap at equal nodes
  fell from **12.2 plies to 3.3**. Depth conventions differ between the two
  searches; this is tree shape, not Elo.
- **Branching profile** (`tools/diag/branching.py`, depths 4–14, Hash 64,
  103 positions completing every depth on every arm): the head's endpoint
  factor **1.588** (per-position median 1.540), the oracle's 1.779 (1.778).
  On 1.10.1 (B.0, same corpus and depths): 1.767 (1.697), the oracle 1.757,
  Stockfish 19 1.549. The head's tree now grows more slowly than the
  oracle's over these depths.
- **Endgame truth cohort** (`tools/diag/endgame_truth.py`, the frozen
  `endgame_cohort_v1` manifest `77D2F84E…` and EPD `3CCF28EA…`, 60,000 nodes
  a move, 100 plies, seed 6200600, Hash 16, Release head, four workers):
  **458/480** clean tablebase wins converted (95.4%), against 361/480 for the
  6.0.b head and 466/480 for the Stockfish reference (BAS-E47). Twelve of
  twenty families convert 24/24; the residue is KNN-KP **10/24**
  (win-preserving 91.1% over 797 moves), KQ-KRP 22/24, and one miss each in
  KR-KP, KR-KN, KRP-KR, KBP-KN, KP-KP and KBPP-KB. The working copies of the
  cohort files are CRLF on this host, so the tool read LF exports of the
  committed blobs (hashes equal to the manifest's) from
  `tools/results/b111-checkpoint/cohort/`.
- Artifacts: `tools/results/b111-checkpoint/` (`suite_d14_diff300k.json`,
  `branching.json`, `truth.json` with per-position records, logs).
- Disposition: observation. These are B.11.1's fixed-node depth, branching
  and endgame-truth meters; C.0 starts from them.
