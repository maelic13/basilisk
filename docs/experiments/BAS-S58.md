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

- **Speed read, first run (maintainer, 2026-11-12, zero games): VOID by its rule.**
  One window over the line: NNUE, L15, round 3, busy 16.66 CPUs on 15 against 16.6.
  No process fault. `tools/results/b57-speed` (`read.json`, `runs.jsonl`,
  `windows.jsonl`); `python tools/smt_read.py report tools/results/b57-speed`.
  - *Classical phase, every window clean* (busy 15.00–15.07 at L15, 27.72–27.92 at
    L28, 29.33–29.63 at L30). Read as description; its verdicts are void with the
    read.

    | | L15 | L28 | L30 |
    |---|---:|---:|---:|
    | `pf-on` rate per engine | 2.045M | 1.138M | 1.109M |
    | yield, `pf-on` | 1 | 0.557 [0.555, 0.558] | 0.542 [0.541, 0.543] |
    | yield, BAS-P25 arms | 1 | 0.542–0.543 | 0.529–0.530 |
    | prefetch cost (memory) | +2.33% | +2.85%, shift +0.52 pp [+0.19, +0.85] | +2.62%, shift +0.30 pp [−0.02, +0.63] |
    | p25 cost (compute) | +2.73% | +2.95%, shift +0.21 pp [+0.03, +0.39] | +2.63%, shift −0.11 pp [−0.29, +0.07] |
    | spread within an arm | 0.9–1.3% | 0.7–1.6% | 0.8–1.6% |

    The three rounds agree on each yield to ±0.004. Against the tolerance of
    ±0.75 pp, prefetch reads UNRESOLVED at L28 and PASS at L30, p25 PASS at both.
    Total search against today's 15 engines: ×1.04 at 28 and ×1.08 at 30 for
    `pf-on`; an effective control near `1.67+0.017` at 28 and `1.63+0.016` at 30.
  - *NNUE phase.* Stockfish's L15 windows ran 15.28–16.66 busy, against the
    classical 15.0, and its rate's spread was 4.3–7.8% a round at L15 against
    2.9–3.5% at L28, with a low tail (minimum 0.871M, median 1.192M). Yields as read:
    0.542 at L28, 0.503 at L30; a depressed L15 reference biases both upward.
  - *Diagnosis (agent, 2026-11-12, on four CPUs, the session running).* Stockfish
    keeps to its pin: its own CPU time was 3.98 CPUs on 4 pinned CPUs. The extra
    load is foreign work. Neither creating the process suspended and pinning it
    before it runs, nor keeping one process alive for the window, removed it
    (busy 4.77–4.86 against 0.57 with nothing running). L15 leaves fifteen idle
    siblings, where foreign work lands beside the measured engines; L28 and L30
    leave two to four. The first rule counted all busy work against the layout's
    CPUs, so it could not tell the engines' own work from foreign. Which process did
    the work is not identified: protected processes (Defender among them) are not
    readable without administrator rights.
  - *Not corrected from the data.* The slowdown per process is unknown. The
    plausible corrections, L15's upper quartile (1.259M) or its maximum (1.297M) as
    the undisturbed rate, move the NNUE gap at L30 from 0.04 to 0.07–0.08, across the
    PASS line (0.05), so the data cannot decide item 2.

#### AMENDMENT 1 — 2026-11-12, before the re-run (substantive: the instrument)

- **The NNUE phase is read again, alone** (`run_b57_reads.ps1 -Read nnue`,
  `tools/results/b57-speed-nnue`, about 13 minutes of an idle PC): the same binary,
  layouts, workload, three rounds of 80 s, with three changes to `smt_read.py`:
  1. every process is created suspended, pinned, then resumed;
  2. one Stockfish per CPU for each window, its benches typed into its UCI loop
     (`persistent`), so nothing is launched inside a window;
  3. disturbance is foreign work: the busy share less the engines' own CPU time
     (`GetProcessTimes`); a window is disturbed above 1.6 CPUs of it.
- **The classical phase stands from the first run.** Its windows were clean under
  either rule and its rounds agree to ±0.004. The read that the direction rule's
  item 1 judges is `smt_read.py report tools/results/b57-speed --phase-from
  nnue=tools/results/b57-speed-nnue`: the first run's classical phase with the
  re-run's NNUE phase, VALID if every window it uses is clean. Item 2 reads the
  re-run's NNUE yields.
- **Predictions.** S4 stands as frozen before either run. Added now, informed by the
  first run: **S4b**, the re-run's L15 Stockfish rate above the first's 1.188M and
  its spread under 4%, 70%; the gap to the classical yield 0.03 [0.00, 0.07] at
  L28 and 0.06 [0.02, 0.10] at L30.
- **The game reads' report** (`replay_compare.py`) adds, reported and not judged:
  each run's nElo; the nElo ratio of the replay over (a) on matched openings; the
  break-even ratio √(15/N), 0.732 at 28 and 0.707 at 30, above which an SPRT at the
  form decides sooner than today (games to a decision scale as 1/nElo², and games
  an hour as N/15); and the decision-speed ratio, nElo ratio² × N/15. (a)'s nElo is
  +25.4 ± 9.6, so a ratio of two such reads is known to about ±0.5; the resolving
  stage reads it, not these runs. The direction rule's item 4 is unchanged.
- **The maintainer's acceptability question (2026-11-12).** A sibling game is a
  game on a core at about 0.55 of the speed: a gate of more, slower cores. It is
  acceptable if its decisions match today's and come sooner. Items 1–4 judge the
  first; the decision-speed ratio describes the second.
- Order from here: `-Read nnue`, then `g28`, then `g30`, one at a time.

#### RESULT — the game reads (maintainer, 2026-11-12, before the NNUE re-run)

- **g28 and g30**, from the clean tree at `faee747`, each 5,006 games on (a)'s 2,503
  openings (every one matched), PGN recounts agreeing with the runner's pentanomials.
  `tools/results/b57-g28`, `b57-g30` (PGN SHA-256 b50fa01b…, b450c2a1…); the
  comparison is `python tools/diag/replay_compare.py tools/results/b55-gate-a-cap1
  tools/results/b57-<read>`.

  | | (a), 15 on 15 cores | g28 | g30 |
  |---|---:|---:|---:|
  | wall time | 49.8 min | 26.7 min | 25.0 min |
  | games an hour | 6,033 | 11,255 (×1.87) | 11,999 (×1.99) |
  | Elo | +15.6 ± 5.9 | +10.8 ± 6.1 | +10.5 ± 6.1 |
  | nElo | +25.4 ± 9.6 | +17.0 ± 9.6 | +16.6 ± 9.6 |
  | pentanomial | [75, 516, 1117, 700, 95] | [85, 572, 1064, 666, 116] | [89, 548, 1097, 661, 108] |
  | difference from (a), paired | — | −4.7 ± 8.4 | −5.1 ± 8.4 |
  | nodes per move | 137–140k | ×0.578 | ×0.557 |
  | non-mate depth | 16.21 / 16.40 | −1.6 ply | −1.7 ply |
  | time losses, faults | 0, 0 | 0, 0 | **2**, 0 |
  | harness overhead p99 (on / off) | 136 / 81 ms | 156 / 89 ms | 159 / 91 ms |
  | nElo ratio (break-even) | — | 0.669 (0.732) | 0.655 (0.707) |
  | an SPRT decides | ×1 | ×0.84 as fast | ×0.86 as fast |

- g30's two time forfeits are on the capped arm, replies charged 204 and 146 ms that
  the engine reported at 1 and 54 ms (harness overhead 203 and 132 ms), with one core
  left to the runner and Windows.
- The pair-score correlation between a replay and (a) is +0.03 (g28) and +0.01
  (g30): replaying the openings bought no precision.
- **Verdicts, by the frozen rule.** g30: item 3 fails (two time losses), so the 30
  form is **not adopted**. g28: items 3 and 4 pass (no loss or fault; −4.7 ± 8.4
  contains 0 and is under 8 Elo). Items 1 and 2 rest on the speed read, VOID with
  its NNUE re-run not played; the classical phase alone reads no FAIL at L28. On
  the rule, 28 would go to a resolving stage.
- **Disposition: NOT_WORTH_PURSUING; B.5.7 closes NO_CHANGE** (maintainer,
  2026-11-12), keeping 15 games on 15 cores at `3+0.03`. The resolving stage is not
  registered, a stated departure from the frozen path. Its cost, about five hours
  (the NNUE re-run, a 30,000-game null pair, a fixed-length 15-core replay, a second
  pair), would confirm a speed-up whose point estimate is a slow-down: an engine on
  a sibling keeps 0.56 of a core, searches 1.6 plies less, and Cluster 4's nElo fell
  to 0.67 of (a)'s against a break-even of 0.73. The ratio is uncertain (about
  0.2 to 1.1), but the depth loss is exact and its direction is the expected one:
  the same pair read +10.8 at 28 (near `1.65+0.017`), +15.6 at `3+0.03` and +20.3 at
  `10+0.1` (BAS-S55 (d)).
- The NNUE re-run (`-Read nnue`) is not played; Amendment 1's instrument stays in
  `smt_read.py`. The Colosseum prototype stays unreleased on its branch, and
  Basilisk's pin stays at 0.5.0.

#### PREDICTION CALIBRATION — preserve the original prediction above

- **S1** (classical yield at L28 0.57, [0.52, 0.63]): **hit**, 0.557; L30 within 0.02
  of L28: **hit**, 0.542. The smoke run's exposure is stated above.
- **S2** (p25 PASS or UNRESOLVED at L28, 80%): **hit**, PASS (descriptive; the read is
  void). **S3** (prefetch cost at L15 +1.5% [+0.5, +3.0]): **hit**, +2.33%; shift
  positive at L28: **hit**, +0.52 pp; FAIL 40%: did not, UNRESOLVED.
- **S4**, **S4b**: not scored; the NNUE phase was not read validly.
- **S5** (spread at L28 at least twice L15's): **miss**. Classical spreads are 0.7–1.6%
  on siblings against 0.9–1.3% on one core; sibling noise is small.
- **G1** (no loss or fault in g28, 85%): **hit**; overhead p99 under 250 ms: **hit**.
- **G2** (nodes per move ×0.57 [0.50, 0.66]): **hit**, ×0.578; depth 0.6 ply lower
  [0.3, 0.9]: **miss**, 1.6. The model converted nodes to depth with `bench 13`'s
  branching factor (2.55 a ply). In a timed game a ply that cannot finish is lost
  whole, and at depths of 14–16 the iteration that the lost nodes would have
  completed is the last one, so the time-limited depth falls about a ply per 30% of
  nodes here.
- **G3** (Elo +12 [+4, +20]): **hit**, +10.8; difference −3.5 [−10, +3]: **hit**,
  −4.7; pair-score correlation +0.20 [+0.05, +0.35]: **miss**, +0.03. UHO's openings
  carry almost none of a pair's result across runs at this sample size.
- **G4** (g30 within 3 Elo of g28 and within 0.02 of its nodes ratio, 70%): within
  3 Elo, **hit** (−0.3); nodes ratio 0.021 apart, **miss** by 0.001.
- **The causal model.** Throughput was framed as games an hour; what decides is
  games to a verdict, which grow as the effect shrinks. That was named only after
  the speed read (Amendment 1), and the frozen rule did not judge it. A
  depth-sensitive cluster lost a third of its nElo at 0.58 of the nodes, which ate
  the throughput. A change insensitive to depth would come out ahead; a gate cannot
  know which kind it is testing beforehand.
- **Conditional lesson.** A gate's speed is nElo² × games an hour. On this host a
  sibling buys a core-share of 0.54–0.56, about 4–8% more total search, so a
  sibling gate is a shorter time control in all but name, and a shorter control
  is a cheaper experiment on full cores if one is ever wanted.
- **Retry trigger.** A host or engine where two siblings do at least 1.3 times a
  core's work (yield 0.65 or more, read by `smt_read.py`), or an nElo ratio at or
  above break-even read on a resolving-size sample. Otherwise `CLOSED`.
- **Artifacts.** `tools/results/b57-speed`, `b57-g28`, `b57-g30` and their `-sampler`
  directories, `b57-20261112/` (guard and verdict tests, dry runs); tools
  `smt_read.py`, `diag/replay_compare.py`, `run_b57_reads.ps1`; commits `bdaaff4`,
  `a8b991d`, `24e1556`, `faee747`; Colosseum `e27a201`.
