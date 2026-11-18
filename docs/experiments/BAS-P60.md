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

- Read 2026-11-17, 12:50–12:53 UTC, by the maintainer on an idle host
  (`tools/results/b111-nps/`). Two cycles; host load 4.0% and 4.6%, with no
  foreign process in either cycle. Every reading ran the pinned hash and
  the arm's fingerprint.
- Arm means:
  - 1.10.1: **4.024M** (builds 4.002M to 4.045M);
  - the head: **2.521M** (2.508M to 2.526M).
- Gain form: **−37.36%**, 95% interval [−38.02, −36.71].
- The tool's verdict is `REPEAT_STEP_1`, because the head's two cycle
  levels (2.504M, 2.537M) differ by 1.29%, over its 1% agreement rule.
  - The registration fixed no repeat on the same pools, and the
    disagreement is a thirtieth of the effect. The read stands, with the
    flag recorded; a repeat would answer no new question.
- **Prediction missed: −4%, band −10% to +3%.** The falsifier (beyond
  ±15%: check the build first) fired, and the build was checked:
  - The pool reads the level of B.8's delivered head: BAS-P59's pext asset
    2.529M, BAS-P53's B.8.9 head 2.541M. The fingerprints and the compiler
    are the registered ones. The build is sound.
  - The prediction was the error. Its basis ignored the absolute levels on
    record: B.8.4's head read 2.396M (BAS-P53) against 1.10.1's 4.13M on
    clang 22 (BAS-P12). The search rebuild had already cost about 42% of
    nodes per second before B.8's speed pass won 6% back.
  - Lesson: a prediction about a level reads the last recorded level
    first, not only the deltas.
- In context: the head searches 4.78M nodes at `bench 13` against 1.10.1's
  14.98M. At the clock it plays at 2.10M and the oracle at 2.58M
  (BAS-O06), and it wins by 70.7 at equal time. Per-node cost is the price
  of the Stockfish-shaped machinery, and the tree it buys more than pays
  for it.
- The 1.10.1 arm reads 2.5% below BAS-P12's 4.127M (clang 22, another
  session). Session and compiler are confounded here, so that difference
  is not read as LLVM's.
- Disposition: COMPLETE. This is the checkpoint's NPS meter (2.52M at
  `bench 13`). PLAN §1's speed line inside B and C is a cost, not a
  recovery; the attribution says so.
