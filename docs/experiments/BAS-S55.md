# BAS-S55

**BAS-S55 — B.5.5: the Cluster 4 gates**

- **Registration.** Created 2026-11-10, before any of its games, for **(c)'s stop
  bracket alone**. The maintainer moved that bracket ahead of B.5.4's fit on BAS-S54's
  audit read (2026-11-10). The other gates, (a), (b), (d), what else (c) holds and the
  flip's long-clock condition, are assembled here from packet §11 and §A2.5
  (`analysis/b5_cluster4_research_2026-11-08.md`) at B.5.5's opening, on the fitted
  binaries; nothing below registers them. Maintainer-run games; the agent pins and
  dry-runs.
- **Research question.** On the arm at its seeds, does the fail-high stop at twice the
  optimum play stronger than the stop at the optimum?
- **Why it is owed.** M1's registered rule (BAS-R10): the stop at 1× ends 23.42% of the
  arm's searches, at least 10%, so 2× against 1× is owed at `[0,3]`. The battery's gate
  for 2× is passed (BAS-S53).
- **Why before the fit.** The stop fires on a root fail high, and the fail highs a
  search are what `B5AspBase` and `B5AspGrowth` set: the stop ends 27% of searches at
  1410 and 10% at 19000 (BAS-S54 *AUDIT READ*). PROCESS *Decision rules*: categorical
  architecture is gated before its constants are fitted. A fit under 1× that this gate
  then moved to 2× would have been fitted to a time cut that is gone.

#### GATE (c), the stop's bracket

- **Arms.** One binary, the arm's Tune build `basilisk-b53-on-tune-pext-tune-pgo.exe`
  (`D8D9BC67…`, clean at `d07380d`, final PGO, `bench 13` 3,920,652), every coordinate
  at its seed and both categoricals at 0. Arm A sets `B5FailHighStop=200`; arm B plays
  the default, 100. No build offset between the arms.
- **Shape.** SPRT `[0,3]` nElo (`sprt-default.toml`, α = β = 0.05) at `3+0.03`, 1T, Hash
  64, paired reversed UHO, no adjudication, tablebases off. Seed 55003. Results in
  `tools/results/b55-gate-c-stop`.
- **Cap 6,000 pairs.** σ ≈ 245 nElo a pair (B.5.3's paired runs, ±15.2 nElo at 1,000
  pairs). The cap is fitted to the question, whether the stop costs enough that the fit
  must not be conditioned on it: at +14 nElo (about +9 Elo) the expected stop is
  2.94 / (3 · 12.5 / 245²) ≈ 4,700 pairs, and the cap is 1.3× that. A smaller effect
  ends at the cap. About two hours.
- **H1:** the stop moves to 2× on the arm. `B5FailHighStop` 200 becomes the arm's
  default through a `BASILISK_B5_CORE_DEFAULTS` line, the OFF arm keeping 100; the
  option reads the clock alone, so the arm's `bench 13` is shown unchanged. The Tune
  and release builds are rebuilt and re-pinned, and BAS-S54's block 1 is registered
  again on the new Tune build, its dry run repeated, before its first game.
- **H0:** the stop stays at 1×. Block 1 runs as BAS-S54 registers it.
- **The cap without a decision:** the stop stays at 1× for the fit, since an unresolved
  stop is not an adoption. The estimate is recorded with its interval and goes to the
  maintainer. Block 1 runs as registered.
- **In every case** the stop's bracket is not played again in B.5.5 (one gate, one
  read). (c) keeps §9 item 2's second read if the fit moves `B5AspBase` above 2820 or
  under 705 (BAS-S53 *CONDITIONS*). The hole stays D.1's research card.
- **Hard gates.** The wrapper's: no engine fault, the time-loss ceiling, the recounted
  PGN.
- **Also read, judging nothing:** `tools/diag/pgn_time_census.py` on the PGN, both
  arms (§A2.5).
- **Launch:** `pwsh -NoProfile -File tools/run_b55_gates.ps1 -Gate stop`. The same
  command resumes an interrupted run from its checkpoint; a finished gate is refused.
- **Validation (2026-11-10):** the launcher's dry run on the real pins, from the clean
  committed tree, is accepted
  (`tools/results/colosseum_sprt_b55-gate-c-stop_20261110_153703.manifest.txt`, dry run
  `0C2080C1…`: `--max-pairs 6000`, the option on arm A alone, seed 55003, 15 slots, no
  adjudication). Its guards read both ways: a wrong binary under the pinned name, a
  missing binary and a finished gate in the directory each end it with exit 1, and the
  real pins pass.
- No arm, bound, cap, book or adjudication changes after a game is observed.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Frozen 2026-11-10. Seen before freezing: M1 (BAS-R10), BAS-S53's paired runs and census,
BAS-S54's audit read, Rarog's leads (PLAN B.5).

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| G1 | 2× against 1× reads **+1 Elo**, 80% interval [−7, +9]; positive with 55% | low | outside the interval, the reasoning below has the stop's worth wrong by more than its noise |
| G2 | The gate ends at the cap (65%); H1 (20%); H0 (15%) | low | H1 says the stop was a cost on the arm and Rarog's lead carries here; H0 says the stop is a gain on the arm, and D.1 reads why before it touches it |

- **The reasoning.** The stop fires only when the fail high is on the previous best,
  where finishing the iteration seldom changes the move played. In M1's replays 2×
  spends 10% more a search for 0.38 plies of completed depth on the same move, and in a
  game the time 1× saves is spent later. So the stop on the arm is a time cut whose
  sign is open. Rarog's −15 at a 24% firing rate is a prior from another engine's rule.
- **What it does not predict:** the hole. 2× leaves 8.95% of searches in it against
  6.28% (BAS-R10), which the census reads in the won band.

#### RESULT — append after exposure

- **(c)'s stop bracket: H1 accepted (2026-11-10, maintainer-run).** 8,260 games (4,130
  pairs), LLR 2.955 in [−2.944, 2.944]: **+9.3 ± 4.4 Elo, +15.9 ± 7.5 nElo** for the stop
  at 2×. W/D/L 2220/4040/2000, pentanomial [98, 875, 1984, 1055, 118]. No fault and no
  time loss; the independent PGN recount agrees; 34 games finished after the boundary
  and are outside the sample. Manifest
  `tools/results/colosseum_sprt_b55-gate-c-stop_20261110_153812.manifest.txt`: clean at
  `e34666e`, the validated dry run's `0C2080C1…`, PGN `87AB4B8E…`. An SPRT's estimate at
  its stop reads high; the verdict is the boundary's.
- **Applied as registered.** `B5FailHighStop` is 200 on the arm through a
  `BASILISK_B5_CORE_DEFAULTS` line and 100 on the build without it (`a4637b6`). `bench
  13` is unchanged on both, 3,920,652 and 4,144,790, CTest passes on both, and the
  arm-defaults test fails on a wrong value. The arm's Tune build is rebuilt and
  re-pinned, `basilisk-b54-on-tune-pext-tune-pgo.exe` (`D3E1B9E9…`, clean at `a4637b6`,
  final PGO, `bench 13` 3,920,652, the stop advertised at 200), and BAS-S54's block 1 is
  registered again on it.
- **One departure from the registration:** no release build was rebuilt. Nothing reads
  one before the bake, and B.5.5's other gates take their release builds from the
  fitted source.
- **The census, judging nothing** (`tools/results/b55-gate-c-stop/census/`; 8,294 games,
  the 34 past the boundary included):

  | Band, mean ms a move; moves at half the clock or more | 2× | 1× |
  |---|---:|---:|
  | Even | 81.7; 0.21% | 78.3; 0.09% |
  | Won | 55.1; 2.32% | 59.6; 1.85% |
  | Lost | 52.5; 3.46% | 56.8; 3.05% |
  | Decided at 300 and not converted | 7.97% | 7.72% |

  The stop at 1× spent less on even positions and more on decided ones. The won band's
  share at half the clock or more is the hole (BAS-R10), larger at 2× as predicted; it
  stays D.1's research card.
- **Not explained:** the runner's summary reads 138,229 nodes a move at 2× against
  125,600 at 1×, in 70.3 ms against 69.2, from one binary. It judges nothing here and
  goes to D.1 with the census.
- Disposition: (c)'s stop bracket is closed, H1, and is not played again. The entry's
  other gates are assembled at B.5.5's opening.

#### PREDICTION CALIBRATION — preserve the original prediction above

- **G1: miss.** +9.3 ± 4.4 against +1 with an 80% interval of [−7, +9]. The sign was
  given 55%.
- **G2: the 20% outcome.** The cap was called at 65%.
- **What the reasoning had wrong.** It held that the time 1× saves is spent later at the
  same worth. The census reads it spent in decided positions, where it buys nothing,
  and taken from even ones. Rarog's lead (−15 at a 24% firing rate) carried to this
  engine, and was weighted as another engine's rule.
- **For the next prior:** a clock rule that fires in a quarter of searches is not a
  coin flip on sign when a sibling engine has measured the same rule at the same rate.
