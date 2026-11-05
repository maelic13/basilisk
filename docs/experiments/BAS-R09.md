# BAS-R09

**BAS-R09 — helpers read the stop flag at every node: the four-thread
stop latency BAS-R08 found**

- Date / owner / calibration category: registered 2026-11-05, agent;
  correctness repair (PLAN B.16.5).
- Baseline source revision / candidate source revision / dirty-diff identity:
  base `2f91a78` (the `master` source after B.3), candidate `fa2b866`
  (one engine commit, `src/search_kernel_core.cpp`), both clean.
- Binary / compiler / flags / PGO identity and hashes: Release PEXT
  final-PGO, Clang 23.1.4 (MSYS2), `bench 13` 5,361,058 on both.
  Candidate `basilisk-b165-stop-pext-pgo` SHA-256
  `4AF28C7E03C70B9D59AC4985CCCC5AAF04083240AACEA10EEDE82D14710E8997`;
  base `basilisk-b161-master-pext-pgo`
  `D5E67D0A59EC7DD50630EE81754155A36CDC17647B31C47DC61BC87E6CBDA07A`.
- Research question: does the repair cost Elo at four threads?
- Hypothesis / proposed mechanism: every thread reads the clock and the
  stop flag together, every 2,048 nodes. The main thread then waits, before
  `bestmove`, for each helper's next read. Reading the stop flag at every
  node (a relaxed load of a line written once per search) lets a helper
  leave on its next node, as Stockfish's helpers do. The clock is still read
  every 2,048 nodes.
- Evidence before the change (2026-11-05):
  - A throwaway timestamp build (not PGO, CPUs 2, 4, 6, 8; 60 trials a
    cell) put the four-thread wait in the join: main thread's return to
    the join 0.377 ms at `movetime 100`, 0.365 of it waiting for the last
    helper's return; the condition-variable wake 0.011 ms; stop to return
    and join to write 0.003–0.024 ms. At one thread the whole cost is the
    clock read's interval (stop seen 0.54 ms after the movetime).
  - A screening read with `tools/diag/wallclock.py` (host idle, 40 trials
    a `movetime` and ponder cell), base / candidate, ms: 4T `movetime 50`
    0.99 / 0.82, `movetime 300` 1.07 / 0.80, ponderhit 1.10 / 0.81; 1T
    within noise (0.71 / 0.76, 0.68 / 0.65, 0.62 / 0.77); `go nodes 1000`
    unchanged. The candidate's gaps to Stockfish 19 at 4T, +0.90 and
    +0.92, fall under BAS-R08's 1.0 ms line.
- Interacting mechanisms / consumers: the pool's join
  (`SearchThreadPool::search`) and its result merge, which may take a
  helper's deeper completed iteration: a helper that leaves sooner can
  lose an iteration it would have finished in the old 2,048-node tail.
  The helpers' table writes after the main thread stopped also end sooner.
  At one thread the flag is set only by a `stop` command (a ponder miss,
  an analysis stop), so a game without pondering cannot change.
- Credible competing hypotheses: none for the latency (the timestamps
  place it); for strength, the lost tail iterations could matter more than
  0.25 ms of clock, in either direction.
- Qualification: bench 5,361,058 unchanged (bench never sets the flag);
  CTest 24/24 on the release tree (`-LE strength`), the ponder and
  threading tests included.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected diagnostic movement: four-thread stop latency down about
  0.25 ms (seen in the screening read above).
- Expected Elo sign/range: 0 to +1 at 4T `3+0.03`; not resolvable at this
  cap.
- Probability candidate is positive/useful: 0.9 that the gate keeps it.
- Confidence and basis: high; the change only shortens work done after the
  main thread has stopped.
- Most likely failure mode: a harmful loss of helper tail iterations in
  the vote, which the gate would show as H0.
- Falsification criteria: H0.
- Cheapest prior falsifier: the timestamp build and screening read above;
  both confirmed the latency mechanism.
- Registered gate / stop rule: SPRT `[-5,5]` (`sprt-repair.toml`) at
  `3+0.03`, Threads 4 for both engines, capped at 1,500 pairs (the repair
  rule's cap: the clock effect is about 0.4% of a think and cannot be
  measured at any affordable cap; the games answer only whether the
  change harms). H1 or the cap keeps it; H0 reverts it.
- Full conditions / provenance: `tools/run_b165_gate.ps1`, Colosseum CLI
  0.5.0, UHO book in random order (`common.toml`), normalized Elo, alpha
  and beta 0.05, seed 47001, Hash 64, concurrency 3 with 4 cores a game,
  a 20 ms clock margin, no adjudication, tablebases off, default Move
  Overhead; dry run accepted 2026-11-05
  (`tools/results/colosseum_sprt_b165-gate_20261105_085158.*`).

#### RESULT — append after exposure

Gate run 2026-11-05 09:25–11:54 UTC by the maintainer,
`tools/run_b165_gate.ps1` at repository `661bb3b` (clean), archive
`tools/results/b165-gate/`, manifest
`tools/results/colosseum_sprt_b165-gate_20261105_102520.manifest.txt`.
Both engine hashes and benches matched; host 2.6% busy at the start, the
idle guard not waived; the manifest holds the PGN's and run record's
SHA-256; the PGN recount agrees with the runner.

- Diagnostics (not the verdict): no faults and no time losses in 3,000
  games. Per move, repair / head: 68.1 / 68.2 ms, mean depth 19.89 /
  19.85 (median 16 both), 648,314 / 640,570 nodes. The helpers' work
  after the stop no longer runs, and the engines' node and time reports
  end at different moments, so the node figures are not a speed read.
- Deciding result: **cap reached without a boundary**: 3,000 games, LLR
  −0.96 in [−2.94, 2.94]; −2.2 ± 7.1 Elo (nElo −3.9 ± 12.4); W/D/L
  739 / 1,503 / 758; pentanomial [29, 381, 699, 362, 29].
- Disposition: **retained** under the registered rule (the cap keeps a
  repair): the stop latency it removes is measured (BAS-R08, the screening
  read above), and the games show no harm beyond about −9 Elo at 4T.

#### PREDICTION CALIBRATION — preserve the original prediction above

- Original prediction summary: 0 to +1 Elo at 4T `3+0.03`, not resolvable
  at the cap; 0.9 that the gate keeps it; likeliest failure a loss of
  helper tail iterations, shown as H0.
- Observed result: kept at the cap, as predicted; −2.2 ± 7.1 Elo, below
  the predicted range but well inside its interval, and LLR −0.96 is far
  from H0. Search depth and time per move were unchanged, so no sign of
  the lost-tail failure. The gate answered only "no harm", as registered.

#### AUDIT NOTE 2026-11-05 — placement

The gate ran in Colosseum's automatic slots (each slot logical CPUs 2–9,
16–23 and 24–31: four physical cores with both SMT siblings), where Windows
packs a four-thread engine onto two or three cores' siblings (Rarog
RAR-P54, RAR-M82). Both arms played in the same state, so the head-to-head
verdict, no harm, stands; the conditions were not the sibling-free ones a
four-thread read now uses (PROCESS *Harness*), and the depth and node
figures above are of packed threads. No re-run is owed for a retained
repair.
