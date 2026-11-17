# BAS-P60

**BAS-P60 — B.11.1 pooled-PGO NPS: the head against 1.10.1, both on clang 23.1.5**

- Date / owner / calibration category: registered 2026-11-17, before the
  read; the pools built by the agent, the read on an idle host;
  measurement.
- Arms (`tools/nps_pool.ps1`, four independent final-PGO builds of one clean
  revision each, from temporary worktrees, clang 23.1.5):
  - `b111-head`: `4c0bc0e`, bench 4,777,412; builds `36349345…`, `D2EE354A…`,
    `EC43C9F4…`, `2B259074…`.
  - `b111-v1101`: 1.10.1's `3e5294be`, bench 14,978,465; builds
    `D9C08B7E…`, `276FEEA7…`, `919E6B0E…`, `22C77FA0…`.
- Research question: the checkpoint's pooled NPS meter. BAS-P12's 4.127M for
  1.10.1 was read on clang 22, and an LLVM major step is a speed change at an
  identical fingerprint (RAR-P36 to P38: 1.5–1.7%), so the programme start is
  rebuilt on today's compiler and read in the same session.
- Reading: the trees differ, so nodes per second is not time to depth and
  accepts nothing. The contrast is reported for its estimate and interval;
  the gain form's verdict words are the tool's, not a decision.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Head against 1.10.1: **−4%**, 80% band −10% to +3%. Basis: the programme
  added Stockfish-shaped histories, corrections and pruning machinery per
  node, and the board rebuild and the speed pass (B.7, B.8; BAS-P53 +6.05%
  on its own baseline) won part of it back. Low confidence: no read has
  compared the two directly.
- Falsifiers: beyond ±15% points at a build (flavor, PGO, ISA) before the
  search.
- Registered design and stop rule: `tools/nps_read.ps1`, gain form, step 1
  of two cycles, extended to six only when the tool says EXTEND; no repeat
  on the same pools.
- Full conditions: `pwsh -NoProfile -File tools/nps_read.ps1 -Base
  tools/test_engines/b111-v1101.pool.json -Cand
  tools/test_engines/b111-head.pool.json -Out tools/results/b111-nps -Label
  "B.11.1 head against 1.10.1"`; `bench 13 3 1`, pinned, High priority,
  60 s quiet wait under 10% load, idle host.

#### RESULT — append after exposure

- Registered, not yet run.
