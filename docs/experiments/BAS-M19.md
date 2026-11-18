# BAS-M19

**BAS-M19 — B.11.4's direction checks: the 1.11.0 candidate against 1.10.2 at `10+0.1` 1T and at 4T**

- **Registration.** Created 2026-11-18, before any game; maintainer-run
  games. PLAN B.11.4: `10+0.1` and 4T direction checks whose 95% intervals
  exclude a loss, beside BAS-S67's SPRT.
  - A head-to-head read cannot see a thread-scaling loss that both versions
    share, so BAS-M20's 4T pool read sits beside this one.
- **Arms.** BAS-S67's candidate (`basilisk-b111-head-pext-pgo.exe`,
  `31BC229A…`, bench 4,777,412) and baseline
  (`basilisk-b114-v1102-pgo1-pext-pgo.exe`, `21BE1192…`, bench 14,978,465).
  Both are Clang 23.1.5 final-PGO PEXT release builds with clean manifests.
- **Research question.** Does the gain hold at a longer clock and at four
  threads?
- **(a) `ltc`.**
  - A 1,000-game fixed match in 500 colour-reversed pairs, run file
    `match-fixed-ltc.toml`.
  - `10+0.1`, margin 20 ms, 1T, Hash 64, concurrency 15, seed 114002.
  - Launcher: `python tools/run_b114_gates.py --run ltc`.
  - About 30 minutes.
- **(b) `4t`.**
  - A 1,000-game fixed match at `3+0.03`, 4 threads a side, Hash 512,
    seed 114003.
  - Sibling-free: three games at a time on explicit cores, one logical CPU
    per physical core, 4 cores a game, one core free (PROCESS *Harness*,
    BAS-X41).
  - Launcher: `python tools/run_b114_gates.py --run 4t`.
  - About 45 minutes.
- Both runs: paired reversed UHO in random order, no adjudication,
  tablebases off, a fault cap of zero. Both dry runs are accepted.
- **Pass rule.** Each read's 95% interval excludes a loss: its lower bound is
  above 0 Elo.
- **Comparison.** BAS-S27 read the head at that time against 1.10.2, after
  Cluster 1 alone and before Clusters 2 to 4 and the repairs: `ltc`
  +241.4 and `4t` +237.6. Those were unpinned at 4T, before BAS-X41.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- (a) `ltc`: about **+380**, 80% band +300 to +460.
  - Basis: the STC gain of about +400, and BAS-S27's `ltc` reading close to
    its STC.
- (b) `4t`: about **+420**, 80% band +330 to +500.
  - Basis: BAS-S27's `4t` read about 1.25 times its STC.
  - Both versions now play sibling-free, which removes the placement
    confound.
- Both pass, probability 0.98.
- No candidate fault, probability 0.95.
- Falsifier: a read whose 95% interval reaches 0 is a release blocker, and
  goes to the maintainer before any reading of B.11.4.

#### RESULT — append after exposure

- Run 2026-11-18 by the maintainer, queued one after the other
  (`tools/results/b114-ltc/`, `b114-4t/`). Both finished with 1,000 games
  in 500 complete pairs.
- **(a) `ltc`: +371.3 Elo, 95% interval [+348.8, +396.4].**
  - W-D-L 806-177-17; nElo +661.1 ± 21.5.
  - Pentanomial [0, 0, 25, 161, 314].
  - 33 min of play. The run was interrupted once at game 439 and resumed
    from its checkpoint, as designed (anomaly `run-resumed`). No game was
    replayed or lost.
- **(b) `4t`: +386.6 Elo, 95% interval [+363.7, +412.1].**
  - W-D-L 818-169-13; nElo +710.5 ± 21.5.
  - Pentanomial [0, 1, 15, 162, 322].
  - 48 min of play. Placement was explicit and sibling-free: 12 logical
    CPUs (2, 4, 6, 8 and 16 to 30, even numbers only), one a physical core,
    3 games of 4 cores.
- The intervals are recomputed from the pentanomials and agree with the
  runner's. The PGN recount agrees with both W-D-L counts, and every
  termination is `normal`.
- No faults and no time losses on either side in either run.
- The registration said "a fault cap of zero". The runner applied its
  run-file default of 10 engine faults and 10 time losses, a wording error
  in the registration. It had no effect, because no fault occurred.
- Per-move statistics:

  | Run | Arm | Mean depth | Nodes a second |
  |---|---|---|---|
  | `ltc` | candidate | 24.75 | 1.97M |
  | `ltc` | 1.10.2 | 18.81 | 2.71M |
  | `4t` | candidate | 22.94 | 9.12M |
  | `4t` | 1.10.2 | 17.53 | 12.92M |

  The candidate searches about 6 plies deeper at about 28% lower NPS, the
  same trade-off as at STC.
- **Pass rule: both pass.** Each lower bound is far above 0.
- Calibration:
  - (a) is inside its band (+300 to +460), 9 below the point.
  - (b) is inside its band (+330 to +500), 33 below the point.
  - Both passed (probability 0.98), and no candidate faulted (0.95).
  - (b)'s point sits below STC's +406.9 rather than at BAS-S27's 1.25×.
    BAS-S27 read unpinned and this read is sibling-free, so the ratios are
    not comparable. A head-to-head shows no sign of a 4T-specific gain or
    loss; thread scaling itself is D.2's read.
- BAS-M20's 4T pool read was withdrawn unplayed. These two checks and
  BAS-S67's SPRT are the whole of 1.11.0's licence.
- Disposition: COMPLETE. 1.11.0 is licensed for release.
