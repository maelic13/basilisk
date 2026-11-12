# BAS-S58

**BAS-S58 — B.5.7: 28 or 30 one-thread games on SMT siblings against 15 on 15 cores, at `3+0.03`**

- **Registration.** Registered 2026-11-12, before any read; maintainer-run (one speed
  read of about 40 minutes on an idle PC, two game reads of about 27 minutes each);
  method and harness, no engine change. The maintainer's question: does a gate of 28
  (or 30) games at `3+0.03`, two per physical core, one on each SMT sibling, reach
  the decisions that today's 15 games on 15 cores reach? The effective control is
  allowed to change. These are direction reads: if they look promising, a resolving
  stage is registered separately; if not, B.5.7 closes with today's conditions.
- **What today's gates are (evidence, the run records).** Colosseum's automatic
  placement with one core of headroom runs **15** games, one per physical core 1–15,
  each slot holding both siblings of its core; core 0 (CPUs 0–1) is left free. Gate
  (a) on the capped arm (BAS-S55) ran 15 slots on CPUs 2–31, 6,033 games an hour.
  PROCESS *Harness* said fourteen with two cores free, which is the fastchess
  path's sizing; PLAN B.5.7's "28 on 14 cores" came from it.
- **The forms.**
  - **28**: cores 1–14, both siblings each, 28 games; cores 0 and 15 (CPUs 0, 1, 30,
    31) free for the runner and Windows. 28/15 = 1.87 times today's games an hour.
  - **30**: cores 1–15, both siblings, 30 games; only core 0 free. 2.0 times.
  At the same clock a game lasts as long whatever its node rate, so the throughput
  ratio is the concurrency ratio. Each engine keeps a share *y* of a core while its
  sibling is busy, so it searches about *y* times today's nodes per move: an
  effective control near `3y+0.03y`.
- **The runner.** Colosseum 0.5.0 sizes slots in whole physical cores and refuses 30
  slots on 15 cores ("30 game slots need 30 physical cores (game-slots x
  cores-per-game), but placement provides 15"). Two disjoint 15-game runs on the
  even and odd siblings would be accepted, but an SPRT cannot be split across two
  runs. So the reads use a **prototype**: branch `smt-slot-prototype` of
  `D:/code/colosseum`, commit `e27a201` on 0.5.0 (`8fa63cc`), adding an opt-in
  `--slot-per-logical-cpu` to `match`, `sprt` and `calibrate`. Every selected logical
  CPU is a slot shared by both of its engines, recorded as allocation mode
  `shared-logical-cpu`; nothing else changes. Its workspace tests pass (636), with a
  unit test and a CLI test of the mode. The binary is staged as
  `tools/bin/colosseum-cli-smt-proto.exe` (87A95FA5…) and pinned by
  `tools/colosseum/colosseum-smt-proto.pin.json`. Basilisk's pin stays at 0.5.0; a
  release and adoption follow only if B.5.7 adopts a form.
- **The wrapper.** `colosseum.ps1 -Placement siblings -Concurrency N` passes the first
  N/2 game cores with both siblings (`Get-HarnessSiblingPairCpuList`), never CPU 0's
  core, with `--slot-per-logical-cpu`. It refuses: the default pin, more than one
  thread, a missing or odd concurrency, more pairs than game cores, a core without
  exactly two siblings, and a resolved plan whose allocation mode, CPU list, slot
  size or CPU reuse differ from the pairs. Guards: 20 of 20 refuse their bad input
  or accept the good one (`tools/results/b57-20261112/guard_test.ps1`).
- **Power: why these are direction reads.** Two runs of about 5,000 games each
  resolve their Elo difference to about ±8.3 Elo unpaired. The bias that would matter
  at `[0,3]` is one or two Elo, which no affordable replay sees. Two things help. The
  replays use (a)'s master seed 55006, which schedules (a)'s 2,503 openings in order
  (checked on a 4-game run: the same first two openings), so the difference is read
  opening by opening (`tools/diag/replay_compare.py`) and the openings' share of the
  variance cancels. And the mechanism that could misprice a change is read directly,
  without games, by the speed read.
- **The speed read (zero games).** `tools/smt_read.py` via `run_b57_reads.ps1 -Read
  speed`. Pinned bench processes keep every listed CPU busy for a window; a process
  counts only if it started after the window's warm-up and ended before it closed.
  Layouts: L15 (CPUs 2, 4, …, 30: one per game core, today's load), L28 (2–29), L30
  (2–31). Three rounds, each running every layout in turn. A window whose busy share
  exceeds its CPUs by more than 1.6 voids the read, as does any process fault.
  - *Classical phase*, 150 s a window, `bench 13 3 1` at Hash 64 (`bench 10` for the
    older BAS-P25 builds, a tree of the same size). Two contrasts whose arms search
    an identical tree, so their Elo is their speed alone:
    - **prefetch** (memory latency): the TT prefetch removed
      (`src/tt.h` `prefetch()` returns at once, a test arm cited by its patch below)
      against the same source with it, two final-PGO builds each at `6329d8f`,
      `bench 13` 3,783,135 for all four;
    - **p25** (compute): BAS-P25's pools, the zero-weight evaluation terms computed
      (base) or not (cand), four builds each, +2.35% [+2.01, +2.70] idle.
  - *NNUE phase*, 80 s a window: Stockfish `dev-20260707-db91fd16` with its embedded
    network (`stockfish-b57-nnue.exe`, F7C3D427…), `bench 64 1 15`, a stand-in for a
    network engine until Basilisk has one.
  - *Statistics.* Yield = an arm's mean node rate in L28 or L30 over L15. A contrast's
    cost = 1 − slow/fast; its shift = cost in L28 (L30) − cost in L15, with a 95%
    bootstrap interval over processes. Tolerance = max(0.75 pp, 25% of the L15
    cost): 0.75 pp is about 0.5–0.75 Elo of mispricing.
  - Verdict tests: 13 of 13 on synthetic reads of both signs
    (`tools/results/b57-20261112/report_test.py`).
- **The game reads.** B.5.5's capped pair (`basilisk-b55-cap1-on-pext-pgo.exe`
  122BE50F…, `-off-` 17978E36…, revision `833659f`, `bench 13` 3,783,135 and
  4,144,790), 5,006 games at `3+0.03`, Hash 64, one thread, UHO paired in random
  order, master seed 55006, no adjudication, 20 ms margin; `host_sampler.ps1`
  beside. g28 at 28, g30 at 30, g30 only after g28 has finished with no time loss
  and no fault. Reference: gate (a) on the capped arm, 5,006 games, +15.6 ± 5.9 Elo,
  pentanomial [75, 516, 1117, 700, 95], `tools/results/b55-gate-a-cap1`.
  - (a) stopped at H1, so its estimate is a first-crossing estimate, and so is the
    difference; the replays' own Elo is not selected (a fixed length on the same
    openings). Stated, not corrected.
- **Exposure stated before the predictions.** A smoke run of `smt_read.py` on 2 and 4
  CPUs at bench depth 9 and 7 (not the read; its workloads are cache-resident) read
  classical yields of 0.53–0.56, Stockfish 0.58–0.59, and the prefetch cost 1.6% at
  one per core and about 3% on siblings. The predictions below were written after it.
- **Changed from the plan put to the maintainer.** The short null pair is dropped:
  2,000 games resolve a null to about ±15 nElo against PROCESS's ±5 rule, so it
  could pass nothing, and its hazard reading (time losses, faults, overhead) is
  g28's own. The full null pair (30,000 games, about 2.7 hours at 28) belongs to
  the resolving stage, which a topology change owes in any case. B.5.5's pair left
  the speed read for the games' own telemetry.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **S1.** Classical yield at L28 (`pf-on`): **0.57**, 80% interval [0.52, 0.63]; L30
  within 0.02 of L28: 80%.
- **S2.** p25 (compute) at L28: PASS or UNRESOLVED, 80%; FAIL 20%.
- **S3.** prefetch (memory) cost at L15 +1.5%, 80% [+0.5, +3.0]; its shift at L28
  positive (the prefetch worth more on siblings), 65%; FAIL at L28, 40%.
- **S4.** NNUE: Stockfish's yield at L28 no more than 0.05 under the classical yield
  (PASS), 70%; FAIL (more than 0.10 under), 10%.
- **S5.** Sibling noise: the node rate's CV within an arm at L28 at least twice that
  at L15, 80%.
- **G1.** g28: no time loss and no fault, 85%. Harness overhead p99 rises from
  81–136 ms to under 250 ms, 70%.
- **G2.** g28's nodes per move against (a)'s, both arms: ×0.57, 80% [0.50, 0.66]; the
  non-mate depth 0.6 ply lower, 80% [0.3, 0.9].
- **G3.** g28's Elo **+12**, 80% interval [+4, +20]; the paired difference from (a)
  −3.5, 80% [−10, +3]; the pair-score correlation between the runs +0.20, 80%
  [+0.05, +0.35].
- **G4.** g30 within 3 Elo of g28 and within 0.02 of g28's nodes-per-move ratio, 70%.
- **Direction rule, fixed now.** A form (28, or 30) is **promising** if all hold:
  1. the speed read is VALID and neither judged contrast reads FAIL at its layout;
  2. the NNUE yield at its layout is not FAIL;
  3. its game read has no time loss, no engine fault and no infrastructure fault;
  4. its paired 95% interval for the difference from (a) contains 0, and the
     difference is under 8 Elo in size.

  It is **not adopted** if 1, 2 or 3 fails, or if its interval excludes 0. A form that
  passes 1–3 with an interval containing 0 but a difference of 8 Elo or more is
  **unclear**, and the maintainer decides whether to play more. A promising form
  goes to a separately registered resolving stage (the full null pair at that form;
  a fixed-length replay of (a)'s openings at 15, which removes the first-crossing
  bias; a second pair with a real speed cost, such as B.4.5's); nothing is adopted
  on direction reads alone.
- **Falsifiers.** S1 under 0.45 or a time loss in g28 says the form loses more depth,
  or more clock, than its throughput buys. A FAIL on p25, the compute contrast,
  would contradict the expectation that only latency costs move on siblings.
- **Conditions.** As above; runner prototype `e27a201` (87A95FA5…); speed config
  written by the launcher to `tools/results/b57-speed.read.json`; results in
  `tools/results/b57-speed`, `b57-g28`, `b57-g30` and their `-sampler` directories.
  Commands: `pwsh -NoProfile -File tools/run_b57_reads.ps1 -Read speed` (then `g28`,
  then `g30`); one pinned harness at a time, Rarog's included.
- **The prefetch test arm** (a throwaway local branch, deleted after the builds; cited
  by its patch, applied to `6329d8f`):

  ```diff
       void prefetch(Key key) const {
  +        (void)key;
  +        return;  // B.5.7 test arm: the TT prefetch removed
           if (!clusters_)
               return;
  ```

  Builds: `basilisk-b57-pfon{1,2}-pext-pgo.exe` (2DA8401D…, 76776566…) and
  `basilisk-b57-pfoff{1,2}-pext-pgo.exe` (A4E82F0B…, 0AB244D2…), `build_test.ps1
  -Flavor Release`, Clang 23.1.5, each manifest clean.

#### RESULT — append after exposure

#### PREDICTION CALIBRATION — preserve the original prediction above
