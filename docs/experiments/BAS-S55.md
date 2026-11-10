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
- **Validation (2026-11-10):** the launcher's dry run on the real pins is accepted
  (`tools/results/colosseum_sprt_b55-gate-c-stop_20261110_153433.manifest.txt`, dry run
  `0C2080C1…`: `--max-pairs 6000`, the option on arm A alone, seed 55003, no
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

- Disposition: prepared, maintainer-run, not started.

#### PREDICTION CALIBRATION — preserve the original prediction above

- *Owed at the gate's result.*
