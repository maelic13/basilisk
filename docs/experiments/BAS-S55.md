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

#### GATES (a), (d) AND (e) (registered 2026-11-12, at B.5.5's opening, before any of their games)

Assembled from packet §11 and §A2.5 (`analysis/b5_cluster4_research_2026-11-08.md`) on
the fitted binaries. (b) is retired, answered by BAS-S56 (+10.8 ± 5.3 Elo for the fit
over the seeds) under the rule BAS-S56 records; (c) is closed.

- **Binaries.** Final-PGO PEXT Release builds of one clean revision, `2c248bb` (engine
  source `891edc0`'s), built by `tools/build_test.ps1`:
  - the fitted arm, `basilisk-b55-on-pext-pgo.exe`, SHA-256 148104E9…, `bench 13`
    4,585,613, `B5_ROOT` on;
  - the OFF arm, `basilisk-b55-off-pext-pgo.exe`, SHA-256 77221608…, `bench 13`
    4,144,790, the current development fingerprint.
- **(a) the fitted arm against the OFF arm.** SPRT `[0,3]` nElo (`sprt-default.toml`,
  α = β = 0.05) at `3+0.03`, 1T, Hash 64, paired reversed UHO, no adjudication,
  tablebases off, seed 55001, **cap 4,500 pairs**, as the packet registers. At σ ≈ 245
  nElo a pair, the expected stop is about 2,500 pairs at +25 nElo (about +15 Elo, the
  prediction below) and about 5,200 at +13 nElo (P2's +8 Elo), past the cap; the cap
  is kept as registered. Results in `tools/results/b55-gate-a`.
  - **H1:** the cluster is accepted, and the flip waits only on (d)'s condition.
  - **H0 or the cap:** the flip does not happen; the estimate and the evidence go to
    the maintainer.
- **(d) the direction read.** 2,000 games of (a)'s pair at `10+0.1`
  (`match-fixed-ltc.toml`), seed 55004, direction only: its interval is recorded and
  decides nothing by itself. Results in `tools/results/b55-direction-ltc`.
  - **L1, the flip's long-clock condition** (§A2.5), read on (d)'s PGN by
    `tools/diag/pgn_time_census.py`, both sides: it **fails** when the fitted arm's games
    decided at 300 and not won exceed the OFF arm's by more than the larger of 6 and
    3·√(the OFF arm's count). At 800 and 1,500, four or more on the arm are reported, each
    game read before the flip.
  - **A failed condition** holds the flip: first the failing run again with
    `B5NegExtMax` 1 (§9 item 1's paired run did not reject it), then the hole's repair
    from D.1 as a research card, with the maintainer's agreement.
- **(e) the long-clock run, only on a trigger.** 400 games of (a)'s pair at `60+0.6`
  (`match-fixed-long.toml`), seed 55005, read by the same census and bound, its Elo
  recorded as direction. Triggers (§A2.5):
  1. the battery on the fitted binaries reads the arm over 0.5× the maximum more often
     than the OFF arm by more than the tolerance, 2, in stratum W at `60000+600`. **Read
     2026-11-12, zero-game, on the fitted Tune builds** (`basilisk-b55-on-tune-pext-tune-pgo.exe`
     F2AF4E1E…, `basilisk-b55-off-tune-pext-tune-pgo.exe` 08CB307C…, both clean at
     `2c248bb`, `TM_Debug` being a tune option as in BAS-S53's screen; BAS-S53's
     positions; `tools/results/b55-20261112/battery.ps1` cbcbac0e…): **the arm 1 over
     half, 0 at the maximum, 207.3 s; the OFF arm 1, 0, 200.2 s. Not fired.**
  2. (d)'s census reads the fitted arm's won-band share of moves at half the clock or
     more at 1.5 times the OFF arm's or above. Read after (d).
  - If neither fires, (e) is not played and the condition rests on L1. If one fires,
    the maintainer agrees or refuses the run; the launcher refuses (e) until a commit
    records the trigger and the agreement.
- **Order:** (a), then (d) with L1, then (e) if triggered, then the flip. Nothing
  changes the candidate after a game is seen.
- **Hard gates.** The wrapper's: no engine fault, the time-loss ceiling, the recounted
  PGN. The games census runs on every PGN, both sides, judging nothing beyond L1 and
  trigger 2.
- **Launch:** `pwsh -NoProfile -File tools/run_b55_gates.ps1 -Gate a`, then `-Gate d`.
  The same command resumes an interrupted run; a finished one is refused.
- **Validation (2026-11-12):** the launcher's dry runs on the real pins, from the clean
  committed tree at `45fa9be`, are accepted:
  - (a) `colosseum_sprt_b55-gate-a_20261112_102922.manifest.txt`, dry run D66C7499…:
    `--max-pairs 4500`, seed 55001, `3000+30`, 15 slots, no adjudication;
  - (d) `colosseum_match_b55-direction-ltc_20261112_102927.manifest.txt`, dry run
    34352B20…: `--games 2000`, seed 55004, `10000+100`;
  - (e) `colosseum_match_b55-long-clock_20261112_102943.manifest.txt`, dry run
    7F8374F3…: 400 games, seed 55005, `60000+600`.
  - Its guards read both ways (`tools/results/b55-20261112/guard_test.ps1` 4ab1b448…,
    output e316f2d6…): a wrong binary under a pinned name, a missing binary, a finished
    gate or read in its directory, and (e) without its agreement each end it with exit
    1, each for its stated reason; the real pins pass.

#### PRE-REGISTERED PREDICTION for (a), (d) and (e) — freeze before exposure

Frozen 2026-11-12. Seen before freezing: BAS-S53's unfitted paired run (+3.3 ± 9.3, the
stop at 1×), (c)'s H1 (+9.3 ± 4.4, a first-crossing estimate), BAS-S56 (+10.8 ± 5.3 for
the fit), the battery above.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| G3 | (a) ends H1 (80%), at the cap (17%), H0 (3%); its estimate at the stop **+15 Elo**, 80% interval [+6, +24] | moderate | H0, or an estimate under +6: the three pieces do not add, and the fit or the stop was read high |
| G4 | (d) reads **+12 Elo** at `10+0.1`, 80% interval [−3, +27]; positive with 85% | low-moderate | an interval wholly below 0: the gain is a short-clock effect, and the flip waits for the maintainer |
| G5 | L1 passes (85%); (e)'s second trigger fires (15%) | moderate | L1 failing says the hole costs points at `10+0.1`, which R1 read as waste only |

- **The reasoning.** The pieces measured on the way add to about +20: the unfitted arm
  +3.3, the stop's 2× something under its first-crossing +9.3, the fit +10.8. They were
  read on different trees and do not simply add, and the unfitted read's interval is
  ±9.3, so the centre is set lower, +15. The longer clock usually compresses a search
  gain, hence +12 for (d). The census read the hole as waste in decided positions, not
  lost points, so L1 is expected to pass.

#### RESULT for (a), (d) and (e) — append after exposure

- **(a)** (2026-11-12, maintainer-run, `tools/results/b55-gate-a/`; manifest
  `colosseum_sprt_b55-gate-a_20261112_104025`, dry run D66C7499… as validated, both pins
  and `bench 13` counts confirmed, exit 0): **H1 at 2,231 pairs**, LLR +2.96. **+16.8 ±
  6.1 Elo** (nElo +28.4 ± 10.2), 1,244 wins, 2,190 draws, 1,028 losses, pentanomial
  [49, 460, 1027, 616, 79]; the PGN recount agrees. No fault, no time loss; 14 pairs past
  the crossing kept outside the sample. The estimate is a first crossing's and reads
  high. The arm searches 17.70 plies a move against 18.33, at 70.8 ms against 69.5.
- **(d)** (2026-11-12, maintainer-run, `tools/results/b55-direction-ltc/`; manifest
  `colosseum_match_b55-direction-ltc_20261112_112529`, dry run 34352B20… as validated,
  same pins, exit 0): **+20.9 ± 8.5 Elo** (nElo +37.5 ± 15.2) at `10+0.1`, 558 wins,
  1,004 draws, 438 losses, pentanomial [14, 186, 494, 278, 28]; the PGN recount agrees.
  No fault, no time loss. Direction only: positive, and not compressed from (a).
- **The games census** (`pgn_time_census.py` e96e9651…; `census/` in each directory,
  (a) c7ca4bc7…, (d) a7fb3712…):

  | | (d) the arm | (d) OFF | (a) the arm | (a) OFF |
  |---|---:|---:|---:|---:|
  | Decided at 300; unconverted | 605; **48** (7.93%) | 465; **28** (6.02%) | 1,358; 105 (7.73%) | 1,102; 72 (6.53%) |
  | Decided at 800; unconverted | 559; 3 | 435; 1 | 1,254; 2 | 1,026; 0 |
  | Decided at 1,500; unconverted | 536; 0 | 434; 0 | 1,185; 0 | 994; 0 |
  | Won band: mean ms; at half the clock or more | 184.3; **6.82%** | 184.2; **1.30%** | 54.3; 2.53% | 56.3; 0.62% |
  | Even band | 279.3; 0.35% | 268.8; 0.19% | 82.5; 0.28% | 78.6; 0.12% |
  | Lost band | 163.1; 5.45% | 192.1; 10.46% | 50.4; 4.14% | 57.2; 4.56% |

  Every unconverted game at 300 in (d) is a draw on both sides.
- **L1 fails as registered.** The arm's unconverted games at 300 exceed the OFF arm's by
  20 (48 − 28), past the bound max(6, 3·√28) = 15.9. At 800 the arm has 3, under the
  four that call for a read; at 1,500 none.
  - What the count carries, judging nothing: the arm reaches 300 in 140 more games, so
    the count grows with its strength. Per decided game the rates are 7.93% and 6.02%
    (two-proportion z ≈ 1.2), and (a) reads the same direction, 7.73% against 6.53%. The
    rule is the count, and it stands.
- **(e)'s second trigger fires.** The arm's won-band share of moves at half the clock or
  more is 6.82% against 1.30%, 5.2 times the OFF arm's, past 1.5. At `3+0.03`, (a) reads
  4.1 times (2.53% against 0.62%), so the long thinks in won positions grow with the
  clock. The trigger is the maintainer's to agree or refuse; `$eAgreed` stays false.
- **Disposition by the registration:** the flip waits. The first read is the failing run
  again, (d) with `B5NegExtMax` 1 on the arm, since §9 item 1's paired run did not
  reject it (−3.6 ± 9.1, BAS-S53). That run's census read the cap at 5.00% unconverted
  at 300 and 0.97% won-band long thinks, against 7.63% and 1.94% uncapped. If it fails
  too, the hole's repair is pulled from D.1 as a research card, with the maintainer's
  agreement.
- **Not settled by the registration**, and the maintainer's: whether a passing rerun
  makes the arm with `B5NegExtMax` 1 the flip's candidate without (a) again on it, and
  whether (e) is played on (a)'s pair or the rerun's.

#### PREDICTION CALIBRATION for (a), (d) and (e) — preserve the prediction above

- **G3: hit.** H1 (called 80%); +16.8 against +15, inside [+6, +24].
- **G4: hit.** +20.9 against +12, inside [−3, +27], positive (85%). The compression from
  the longer clock did not come.
- **G5: miss on both parts.** L1 fails (passing was called 85%); the trigger fires
  (called 15%). The reasoning held that the hole wastes time in decided positions
  without costing points. At `10+0.1` the long thinks quintuple and the arm has 20 more
  draws from +300, though the rate excess is not significant on its own.
- **For the next prior:** the won-band long-think share scaled with the clock (4.1× at
  `3+0.03`, 5.2× at `10+0.1`), so a hole seen at the short clock is a lower bound at the
  long one.

#### L1'S FIRST REMEDY: (d) WITH `B5NegExtMax` 1 (registered 2026-11-12, before any of its games)

The read §A2.5 names for a failed condition: the failing run again with the arm at
`B5NegExtMax` 1, available because §9 item 1's paired run did not reject it. Maintainer
agreement 2026-11-12.

- **Binaries.** Final-PGO PEXT Release builds of one clean revision, `833659f` (the cap's
  default set to 1; it is read on the `B5_ROOT` arm alone), by `tools/build_test.ps1`:
  - the arm, `basilisk-b55-cap1-on-pext-pgo.exe`, SHA-256 122BE50F…, `bench 13`
    3,783,135. The Tune build `F2AF4E1E…` at `B5NegExtMax=1` replays the same count, so
    the built-in default is the option exactly;
  - the OFF arm, `basilisk-b55-cap1-off-pext-pgo.exe`, SHA-256 17978E36…, `bench 13`
    4,144,790, unchanged. CTest passes 28 of 28 on both arms' dev trees.
- **The run.** (d)'s exactly: 2,000 games at `10+0.1` (`match-fixed-ltc.toml`, the same
  run file, 9B45B36D…), seed 55004 and so (d)'s openings, 1T, Hash 64, no adjudication.
  Results in `tools/results/b55-direction-ltc-cap1`.
- **What it reads.** L1 on its own PGN by the same census and bound: it fails when the
  arm's unconverted games at 300 exceed the OFF arm's by more than max(6, 3·√OFF). At
  800 and 1,500, four or more on the arm are read game by game. (e)'s second trigger is
  re-read. Its Elo is direction only.
- **If L1 fails again:** the hole's repair is pulled from D.1 as a research card, with
  the maintainer's agreement (§A2.5).
- **Not decided by this run:** whether a pass makes the capped arm the flip's candidate
  without (a) on it, and on which pair (e) is played. Both are the maintainer's.
- **Launch:** `pwsh -NoProfile -File tools/run_b55_gates.ps1 -Gate dcap`. Its dry run
  from the clean tree at `4b59706` is accepted
  (`colosseum_match_b55-direction-ltc-cap1_20261112_125434`); the guard test
  (`b55-20261112/guard_test.ps1` 6543cc32…, output `guard_test_dcap.txt` 8a9d8d75…)
  passes 13 of 13, among them a wrong binary, a missing binary and a finished read for
  this run.

#### PRE-REGISTERED PREDICTION for the remedy — freeze before exposure

Frozen 2026-11-12. Seen before freezing: (a), (d) and their census; §9 item 1's paired
run and census at `3+0.03` (the cap: 5.00% unconverted at 300, 0.97% won-band long
thinks, against 7.63% and 1.94% uncapped; −3.6 ± 9.1 Elo); the bench count.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| G6 | L1 passes (75%); the arm's unconverted rate at 300 about **6%**, [4.5%, 7.5%] | moderate | a fail, or a rate at or above (d)'s 7.9%: the chains are not where the draws come from |
| G7 | the arm reads **+17 Elo** over OFF, 80% interval [+4, +30] | low-moderate | an interval wholly below (d)'s +20.9 centre and under +4: the cap costs strength at `10+0.1` |
| G8 | the won-band long-think share falls to about **3.5%**, [2%, 5%]; (e)'s trigger still fires (70%) | moderate | a share at (d)'s 6.8% or above: the chains do not cause the long thinks |

- **The reasoning.** At `3+0.03` the cap cut the arm's unconverted rate by a third and
  its won-band long thinks by half, at a cost inside the noise. Applied to (d), that gives
  a rate near 6%, the OFF arm's. On (d)'s openings the OFF count stays near 28, so a
  count near 35 sits under a bound near 16. The long-think share halves to about 3.5%,
  still past 1.5 times the OFF arm's 1.3%. The cap trims 17.5% of the bench tree, and
  item 1 read it near neutral, so the arm keeps most of (d)'s gain.

#### RESULT for the remedy — append after exposure

- **The run** (2026-11-12, maintainer-run, `tools/results/b55-direction-ltc-cap1/`;
  manifest `colosseum_match_b55-direction-ltc-cap1_20261112_125604`, both pins and
  `bench 13` counts confirmed, (d)'s run file, exit 0): **+20.3 ± 8.9 Elo** (nElo +34.8
  ± 15.2) at `10+0.1`, 589 wins, 939 draws, 472 losses, pentanomial [13, 216, 446, 291,
  34]; the PGN recount agrees. No fault, no time loss.
- **The games census** (`census/census.json` 516f80aa…), beside (d)'s:

  | | the capped arm | OFF | (d) the arm | (d) OFF |
  |---|---:|---:|---:|---:|
  | Decided at 300; unconverted | 620; **32** (5.16%) | 489; **18** (3.68%) | 605; 48 (7.93%) | 465; 28 (6.02%) |
  | Decided at 800; unconverted | 588; 0 | 470; 0 | 559; 3 | 435; 1 |
  | Won band: mean ms; at half the clock or more | 179.6; **2.00%** | 179.0; **1.39%** | 184.3; 6.82% | 184.2; 1.30% |
  | Even band | 278.7; 0.35% | 268.6; 0.20% | 279.3; 0.35% | 268.8; 0.19% |

- **L1 fails again.** 32 − 18 = 14, past max(6, 3·√18) = 12.7. Nothing at 800 or 1,500 to
  read.
- **(e)'s second trigger no longer fires:** 2.00% against 1.39%, 1.44 times, under 1.5.
- **What the reads say, judging nothing:**
  - The cap removes the long thinks: the arm's won-band share falls from 6.82% to 2.00%
    while the OFF arm's, which the cap does not reach, holds at 1.30% and 1.39%.
  - It does not close the conversion gap. Both arms' unconverted counts fell by about a
    third (48 to 32, and the untouched OFF arm 28 to 18), so the fall is the run's and
    not the cap's; the ratio holds at about 1.4 per decided game. Per decided game the
    excess is z ≈ 1.2 in each of (a), (d) and this run, and z ≈ 2.0 pooled over the three:
    a small, consistent gap in turning +300 into wins.
  - Its size: about 1.5 points per hundred decided games, of which each draw costs half a
    point, about 4 points in 2,000 games, or roughly 1 Elo. The bound penalises the arm's
    strength as well: at the OFF arm's rate the arm's 620 decided games would hold 23.
  - The cap costs nothing visible here: +20.3 against (d)'s +20.9 on the same openings.
- **Disposition by the registration:** the condition fails with the first remedy, so the
  hole's repair is pulled from D.1 as a research card, with the maintainer's agreement
  (§A2.5). The flip waits on the maintainer.

#### PREDICTION CALIBRATION for the remedy — preserve the prediction above

- **G6: miss.** L1 fails (passing was called 75%), though the rate, 5.16%, is inside
  [4.5%, 7.5%]: the OFF arm's rate fell too, which the prediction held fixed.
- **G7: hit.** +20.3 against +17, inside [+4, +30].
- **G8: half.** The share fell to 2.00%, at the edge of [2%, 5%]; the trigger did not fire
  (firing was called 70%).
- **For the next prior:** a count across two runs of the same opponent moved 28 to 18 on
  the same openings. A bound of 3·√OFF on one run's count difference sits near 1.8
  standard deviations of that difference, before the stronger arm's extra decided games.

#### THE MAINTAINER'S DISPOSITION OF L1, AND GATE (a) ON THE CAPPED ARM (registered 2026-11-12, before any of its games)

- **L1 is waived** (maintainer, 2026-11-12), an exception recorded with the analysis
  above: the conversion gap is about 1 Elo against the arm's +20, consistent across three
  runs (z ≈ 2.0 pooled) and not the cap's, and the bound tightens with the arm's
  strength. The gap goes to D.1's hole card with these four censuses. The rule is not
  rewritten: a later flip condition is registered afresh.
- **The cap is adopted:** the candidate is the arm at `B5NegExtMax` 1 (`833659f`). It
  removed the won-band long thinks at `10+0.1` and read +20.3 ± 8.9 against (d)'s +20.9
  on the same openings. The packet's donor-form row for the ceiling (§14) changes at the
  flip, with Triumviratus's count cap (TS-1) as the source.
- **(e) is not owed on the candidate.** Trigger 2, the remedy's census: 2.00% against
  1.39%, 1.44 times, under 1.5. Trigger 1, read 2026-11-12, zero-game, on a quiet host:
  the fitted Tune build `F2AF4E1E…` at `B5NegExtMax=1` (its `bench 13` replay equals the
  capped arm's 3,783,135), stratum W at `60000+600`, BAS-S53's positions
  (`b55-20261112/battery_cap1.ps1` afe94e71…, `battery/60000+600/arm-cap1.json`
  4d7365d8…): **0 over half the maximum, 0 at it, 202.2 s, against the OFF column's 1,
  0, 200.2 s. Not fired.**
- **(a) again, on the candidate**, since the cap changes what (a) passed. (a)'s design
  exactly, on the remedy's pinned pair (`833659f`; the arm 122BE50F…, 3,783,135; OFF
  17978E36…, 4,144,790): SPRT `[0,3]` nElo at `3+0.03`, 1T, Hash 64, paired reversed
  UHO, no adjudication, tablebases off, **cap 4,500 pairs**, a fresh seed, 55006. Results
  in `tools/results/b55-gate-a-cap1`.
  - **H1:** the candidate is accepted and the flip follows.
  - **H0 or the cap:** the flip does not happen; the evidence goes to the maintainer.
- **Launch:** `pwsh -NoProfile -File tools/run_b55_gates.ps1 -Gate acap`. Its dry run from
  the clean tree at `6f7fafd` is accepted (`colosseum_sprt_b55-gate-a-cap1_20261112_140954`,
  9DC37DAC…: `--max-pairs 4500`, seed 55006, `3000+30`). The guard test (bdd74bdb…, output
  `guard_test_acap.txt` c4a815e9…) passes 17 of 17, among them a wrong binary, a missing
  binary and a finished gate for this run.

#### PRE-REGISTERED PREDICTION for (a) on the capped arm — freeze before exposure

Frozen 2026-11-12. Seen before freezing: (a), (d), the remedy and their censuses, §9 item
1's paired run (the cap −3.6 ± 9.1 at `3+0.03`, on the seeds' tree), the battery above.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| G9 | H1 (80%), the cap (17%), H0 (3%); the estimate at the stop **+15 Elo**, 80% interval [+6, +24] | moderate | H0, or an estimate under +6: the cap costs at the short clock what it did not cost at `10+0.1` |

- **The reasoning.** (a) read +16.8 at a first crossing, which reads high. At `10+0.1`
  the cap cost 0.6 Elo; item 1 read it at −3.6 ± 9.1 at `3+0.03` on another tree. Netting
  a small short-clock cost against the crossing's bias leaves about +15, with G3's odds.

#### RESULT for (a) on the capped arm — append after exposure

- **(a) on the candidate** (2026-11-12, maintainer-run, `tools/results/b55-gate-a-cap1/`;
  manifest `colosseum_sprt_b55-gate-a-cap1_20261112_141218`, dry run 9DC37DAC… as
  validated, both pins and `bench 13` counts confirmed, exit 0): **H1 at 2,503 pairs**,
  LLR +2.94. **+15.6 ± 5.9 Elo** (nElo +25.4 ± 9.6), 1,437 wins, 2,390 draws, 1,215
  losses, pentanomial [75, 516, 1117, 700, 95]; the PGN recount agrees. No fault, no time
  loss; 18 pairs past the crossing kept outside the sample. The arm searches 18.33 plies
  a move against 18.37, at 70.9 ms against 70.0.
- **The games census** (`census/census.json` 4d83b54e…), judging nothing: unconverted at
  300, the arm 117 of 1,552 (7.54%) and OFF 80 of 1,294 (6.18%); at 800, 7 of 1,432 and 2
  of 1,207; won-band long thinks 1.62% against 0.69%.
- **The arm's seven unconverted games at 800, read** (the clause is (d)'s; read here
  because seven is past its four): games 1877, 2474, 3149, 3232, 3359, 3762 and 4580, all
  drawn, from endings the evaluation scores at +9 to +12 that are not won: rook against
  bishop with a rook pawn, rook against rook and two pawns, minor-piece endings with a
  wrong-coloured bishop and a rook pawn (3232 ends in stalemate), bishop against bishop.
  The scores, not the search, are what is wrong; both arms share the evaluation. It goes
  to the evaluation's ending knowledge (Phase C) as a finding and judges nothing here.
- **Disposition:** the candidate is accepted. The flip follows: `B5_ROOT` defaults ON at
  `B5NegExtMax` 1, and the fingerprint record moves to 3,783,135.

#### PREDICTION CALIBRATION for (a) on the capped arm — preserve the prediction above

- **G9: hit.** H1 (called 80%); +15.6 against +15, inside [+6, +24]. Against the
  uncapped (a)'s +16.8 ± 6.1, the cap costs nothing visible at `3+0.03` either.
