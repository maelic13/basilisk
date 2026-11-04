# BAS-R08

**BAS-R08 — the wall-clock overhead read: what a clocked move costs
outside the search, against Stockfish 19 on the same host**

- Date / owner / calibration category: registered 2026-11-04, agent;
  measurement (PLAN B.16.1, Triumviratus TT-2).
- Baseline source revision / candidate source revision / dirty-diff identity:
  the current `master` source (B.3's squash `d26f92b`; built from the
  branch `b16-instruments`, whose `src/` equals it), clean.
- Binary / compiler / flags / PGO identity and hashes:
  `tools/test_engines/basilisk-b161-master-pext-pgo.exe`, Release PEXT
  final-PGO, Clang 23.1.4 (MSYS2), `bench 13` 5,361,058, SHA-256
  `D5E67D0A59EC7DD50630EE81754155A36CDC17647B31C47DC61BC87E6CBDA07A`.
  Reference: Stockfish 19
  `D:\chess\engines\stockfish\stockfish-19-windows-x86-64-universal.exe`,
  SHA-256 `45BC8E4969147DB9C2EB533810637994619BFF0EACC81CCFD9854394901BCBD0`.
- Research question: does Basilisk lose clock time outside its search that
  Stockfish does not? At `3+0.03` a move gets about 50–100 ms, so a few ms
  is several per cent of the think. Triumviratus found 6.15 ms against
  Stockfish's 1.24 ms at `go nodes 1000`, a +29.8 Elo brake at `5+0.05`
  with an identical tree.
- Hypothesis / proposed mechanism: none to accept; Basilisk passed the code
  reading (steady clock, unsynced streams, a persistent pool on condition
  variables; `analysis/triumviratus_review_2026-11-03.md` §3). This read
  verifies it from outside.
- Interacting mechanisms / consumers: D.1's time budget (Move Overhead and
  the clock model) reads the result; the gates' `3+0.03` think time.
- Credible competing hypotheses: a gap could come from output flushing,
  the UCI thread's hand-off to the pool, the helpers' stop and join at 4
  threads, the stop check's node interval, or the 1 ms ponder poll in
  `Engine::wait_until_bestmove_allowed`. The read places a gap; it does not
  attribute it.

#### Instrument

`tools/diag/wallclock.py`: one process per engine and Threads value, pinned
to the first N game cores (physical cores except CPU 0) at high priority; a
pump thread stamps each line on arrival with the host's QPC clock. Trials
alternate engines every round, the order flipping each round; positions,
`isready` and start-up are outside every timing; one unrecorded warm-up
trial per engine.

- nodes: `go nodes 1000`; wall = `go` written to `bestmove` read; the
  engine's report is the `time` field of its last info line.
- movetime: `go movetime` 50, 100, 300; overshoot = wall − movetime.
- ponderhit: `go ponder wtime 1000 btime 1000 winc 10 binc 10`, `ponderhit`
  500 ms later (past either engine's maximum on that clock); latency =
  `ponderhit` written to `bestmove` read; a `bestmove` before it is counted.

Validation (2026-11-04, `tools/diag/test_wallclock.py`): a fake engine
(`tools/diag/wallclock_fake_engine.py`) with delays planted by spin-wait
read +5.03 ms for a planted 5 ms (nodes), +2.99 for 3 (movetime
overshoot), +4.01 for 4 (ponderhit), and 0 ± 1 ms with nothing planted.

Both engines report `time` in whole milliseconds, so wall − report carries
±1 ms of rounding (Stockfish's read slightly negative in the dry runs). It
is recorded as a secondary number; the deciding quantities are wall times.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Exposure before freezing: two protocol dry runs (2026-11-04, 2 to 6 trials
a cell, `tools/results/b161-wallclock-dryrun/`) were seen. In them every
Basilisk − Stockfish median gap was under 1 ms except 4-thread `movetime`
overshoot (+0.80 to +1.46 ms over three budgets) and 4-thread nodes wall −
report (+1.07, a rounding-affected secondary quantity).

- Expected diagnostic movement: at Threads 1, every deciding gap within
  ±1 ms of Stockfish; `go nodes 1000` wall about 0.7 ms against about
  1.0 ms. At Threads 4, movetime overshoot about 0.5–1.2 ms above
  Stockfish's; nodes wall and ponderhit latency within 1 ms.
- Expected Elo sign/range: not defensible from timing; a gap below 1 ms
  is under 2% of a `3+0.03` think.
- Probability candidate is positive/useful: 0.85 that Threads 1 passes on
  every deciding quantity; 0.5 that some 4-thread movetime gap exceeds
  1 ms.
- Confidence and basis: moderate; the code reading and the dry runs.
- Most likely failure mode: the helpers' stop and join at 4 threads.
- Falsification criteria: see the stop rule.
- Cheapest prior falsifier: the dry runs above; the read is itself cheap
  (about seven minutes).
- Registered gate / stop rule: one read by
  `tools/run_b161_wallclock.ps1` on an idle host (it refuses a busy one
  and records the load before and after). Per Threads value (1, 4) the
  deciding quantities are the medians of: `go nodes 1000` wall (500
  rounds); `go movetime` overshoot at 50, 100 and 300 ms (100 rounds
  each); ponderhit latency (100 rounds). Any Basilisk − Stockfish gap above
  **1.0 ms**, any early `bestmove` or any missing `bestmove` is a finding
  for the correctness-repair path, with its own leaf and gate, which the
  maintainer schedules. Otherwise the read passes and D.1 takes the numbers
  as its overhead prior. A read whose host check fails before or after is
  repeated whole; no cell is re-read alone.
- Full conditions / provenance: 50 positions, every 52,640th line of
  `tools/books/UHO_Lichess_4852_v1.epd` from the first, Hash 64, Threads
  1 and 4, default options otherwise (Move Overhead 10 in both; neither
  engine subtracts it from `movetime`), no tablebases, CPUs from
  `Get-HarnessGameCpus`, archive `tools/results/b161-wallclock/`.

#### RESULT — append after exposure

- Diagnostics (not the verdict):
- Deciding result:
- Disposition:

#### PREDICTION CALIBRATION — preserve the original prediction above

- Original prediction summary:
- Observed result:
