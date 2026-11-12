# BAS-S54

**BAS-S54 — B.5.4: the Cluster 4 curvature sweep, then SPSA in blocks**

- **Registration.** Created 2026-11-10 at B.5.4's opening, before the joiner read,
  any sweep point or any game; agent-run joiner read and sweep, maintainer-run SPSA;
  parameter fit. Assembled from packet §11's BAS-S54 text
  (`analysis/b5_cluster4_research_2026-11-08.md`) with research amendment 1's §A1.5
  (the base's new unit, the sweep's points and statistic) and amendment 2's §A2.5
  (§5.8's median rule; `B5AspBase`'s step), and from B.5.3's verdicts (BAS-S53).
- **Subject.** The B.5.3-closed arm, source `d07380d`, the ON arm's Tune build
  `D8D9BC67…`: `B5AspBase` 1410, both categoricals at 0, the attribution switches at
  1, `B5FailHighStop` 100, `bench 13` 3,920,652. Theta is baked in one engine commit
  with the arm's new fingerprint; a joiner that moves keeps the OFF arm's value
  through `BASILISK_B5_CORE_DEFAULTS`.
- **Research question.** Which of the window's three coordinates carry curvature, which
  Cluster 1 to 3 coordinates the arm's root moves enough to refit, and what theta
  SPSA in blocks (PLAN rule 7c) reaches from the seeds.
- **Hypothesis.** The window's coordinates are converted from the donor and unfitted
  (packet §7), so the fit moves them; the arm's narrow windows and root reduction move
  a few earlier mechanisms' populations, which are fitted beside them (§5.8).
- **Competing explanation.** The surface is flat at STC and the fit is noise:
  BAS-S55's gate then reads H0. The width run already read 19000 level with the seed
  (−1.0 ± 9.1), which is evidence for this at the base.

#### SURFACE (fixed by the joiner read and the sweep, before the first game)

- **Cluster 4's X lines:** `B5AspBase`, `B5AspMsqDiv`, `B5AspGrowth`, seeded 1410,
  2874 and 47.
- **Admitted by §5.8's rule** as §A2.5 amends it, read before the sweep
  (`analysis/b54_sweep_2026-11-10.md`, its rows and their coordinates fixed there): the
  median over `suite_v2`'s positions of the per-position firing share, over the
  positions where the OFF arm's denominator is at least 1,000, at depth 14, at 1410.
  A row that moves in total and not by the median does not join.
- **Each joiner seeds** from the value the arm runs: Cluster 3's refit where it has
  one, then Cluster 2's.
- **Never on the surface:** the F lines, every S switch and every categorical, and
  the time constants (D.1's). A swept coordinate read flat is dropped and reported; a
  coordinate the fit leaves flat is reported, not dropped.
- **Steps** are the generator's, except `B5AspBase`'s: the smaller of the generator's
  and half the start value, 705 at the seed (§A2.5; `850050f`, with its test). Ranges
  are `src/search_params.h`'s. The generated configuration is cited by hash at the
  block registration.

#### SWEEP (frozen in `analysis/b54_sweep_2026-11-10.md` before its first point)

- `B5AspBase` at 1000, 1410, 2000, 2820, 4230, 5640, 9870, 19000 and 30000;
  `B5AspGrowth` at 32, 64 and 96 at the seed and at the cheapest base. Per point:
  `bench 13` (replayed), WAC at 100,000 nodes, and `suite_v2` at depths 14 and 17, by
  the median position beside the total.
- Rarog's frozen classification on the WAC column. A flat coordinate is dropped; the
  tune is not skipped on a flat or monotone surface (packet §11 BAS-S54, PLAN B.6's
  rule). Nodes do not classify.

#### SPSA (registered in detail before block 1's first game)

- Colosseum blocks on the arm's Tune build at `3+0.03`, 1T, Hash 64, UHO paired, no
  adjudication, tablebases off; 2,000 iterations of 30 games a block, the registered
  estimator, gain and block rule (PROCESS *SPSA go/no-go*). Seeds 54001, 54002, …,
  each later block seeded from the previous block's rounded centres.
- Stop rule (PLAN rule 7c): after each block, `tools/spsa_block_rule.py`. At least
  three coordinates moved a full step and the next block runs; fewer and the tune
  stops. Never more than three blocks without a new registration.
- Theta is the last completed block's rounded centres, never a selected checkpoint.
  The fitted-constant analysis is written before the bake: rails, full-step movers,
  reversals, unit and cohesion problems, each with an owner.
- If B.5.4 moves `B5AspBase` above 2820 or under 705, BAS-S55 (c) reads §9 item 2
  again (BAS-S53 *CONDITIONS*).

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **The joiner read's and the sweep's** (J1, J2, S1 to S4) are frozen in the sweep
  record: at most four rows join (60%); continuation pruning moves in total and not by
  the median (70%); the base curved on WAC (55%); the depth-14 total lowest at 19000 or
  30000 (75%); the growth monotone toward 96 (45%); the seed's depth-17 median within
  ±5% of the OFF arm's (70%).
- **The fit's.** A3 (BAS-S53, frozen 2026-11-09): the fit moves `B5AspBase` to 2820
  or above with 80%, to 4000 or above with 65%. P2 (BAS-S53): B.5 reads +8 Elo at STC
  after this fit and the gate.

#### RESULT — append after exposure

- **The joiner read and the sweep (2026-11-10, agent-run, zero-game;
  `analysis/b54_sweep_2026-11-10.md`):** five rows join by §5.8's median rule, admitting
  `CoreContHistPruneSlope` (+13.3% by the median, −22.8% in total), `CoreCorrUpdateScale`
  (−10.2%), `B3NmpPerPly` (+15.6%), `B3SingMargin` (+39.4%) and `B3IirMinDepth`
  (−21.4%). Five more move 10% in total and not by the median, carried by `suite_v2` 42
  and 50, and do not join. `B5AspBase` reads curved on WAC (265 at 2820 alone);
  `B5AspGrowth` monotone (268 at 32). None is flat, so **the surface is eight
  coordinates** and the tune runs. J1, J2 and S3 miss; S1, S2 and S4 hit.
- **The surface** (`2beb0bd`, generated with its test): `tools/spsa_configs/config_b54.json`
  and `colosseum/b54.tune.toml`.

  | Coordinate | Seed | Range | Step |
  |---|---:|---|---:|
  | `B5AspBase` | 1410 | 500–40000 | 705 |
  | `B5AspMsqDiv` | 2874 | 1000–12000 | 688 |
  | `B5AspGrowth` | 47 | 32–96 | 4 |
  | `CoreContHistPruneSlope` | 3832 | 1500–8000 | 406 |
  | `CoreCorrUpdateScale` | 17 | 4–30 | 2 |
  | `B3NmpPerPly` | 13 | 2–32 | 2 |
  | `B3SingMargin` | 232 | 13–378 | 23 |
  | `B3IirMinDepth` | 6 | 3–10 | 2 |

  Each seed is the value the arm's binary advertises (Cluster 3's refit for
  `B3NmpPerPly` and `B3IirMinDepth`, Cluster 2's for the other two joiners).

#### AUDIT READ (2026-11-10, zero-game, before block 1's first game)

An audit of the cluster the maintainer asked for, agent-run. It moves no default and
changes nothing registered above; what it bears on is the maintainer's to decide.

- **Checked and found sound:** the arm's driver, record and kernel against `sf_19`
  (`search.cpp` `:345-547`, `:1361-1506`, `:1887`) as §3, §A1.5 and §14 dispose them; no
  engine source changed since `d07380d`; the Tune build's hash, manifest and `bench 13`;
  all eight coordinates advertised at their seeds and ranges and live at one step either
  side of the seed (`bench_replay.py`, 16 points, every count different from 3,920,652);
  the generator's drift check and test; block 1's dry run again, the same `EF41871B…`.
- **The question:** M1 sent the stop to its 2× gate because it ends 23.42% of the arm's
  searches (BAS-R10), and that gate sits after this fit (BAS-S55 (c)). The stop fires on
  a fail high, and the number of fail highs a search is what `B5AspBase` and
  `B5AspGrowth` set. Does the fit's surface move the time manager?
- **Instrument:** M1's, `replay_games.py` on the arm's Tune build `D8D9BC67…`, the first
  60 games of the gate (a) PGN, candidate side, one option a column, seven columns at
  once on an idle host (`tools/results/b54-20261110/audit/`; `run.sh` 7ea8a8be…,
  `read.py` 4e12448f…, `read.txt` f1e4e1af…). Not registered in this entry before it ran;
  the prediction was stated to the maintainer before the read.
- **Prediction, stated first:** the firing rate falls as the window widens, by at least 3
  points between 705 and 2115, and is near the OFF arm's 10% at 19000.

  | Column | stop ended | even band | re-searches a window | at or under `TmEffortLo` | at or over `TmEffortHi` | depth a search |
  |---|---:|---:|---:|---:|---:|---:|
  | M1, the arm at 1410 (the same 60 games) | 26.08% | 35.32% | 1.49 | 35.3% | 21.3% | 17.68 |
  | M1, the OFF arm | 9.71% | 7.63% | 0.57 | 21.9% | 24.0% | 17.92 |
  | `B5AspBase` 705 | 27.73% | 38.07% | 1.81 | 37.7% | 19.9% | 17.53 |
  | 1410 | 27.01% | 36.53% | 1.53 | 35.9% | 20.8% | 17.62 |
  | 2115 | 25.76% | 34.46% | 1.30 | 34.0% | 21.8% | 17.69 |
  | 5640 | 21.04% | 28.32% | 0.81 | 27.9% | 24.7% | 17.92 |
  | 19000 | 10.16% | 13.37% | 0.24 | 18.0% | 31.6% | 18.32 |
  | `B5AspGrowth` 32 | 27.51% | 36.93% | 1.73 | 36.7% | 21.0% | 17.49 |
  | 64 | 25.55% | 34.83% | 1.39 | 35.4% | 21.3% | 17.71 |

- **Scored:** monotone, **hit**; 19000 at the OFF arm's rate, **hit**; 705 against 2115
  reads 2.0 points, not 3, **miss**. The anchor reads 27.01% against M1's 26.08% on the
  same games, so a point is the read's noise with seven columns running.
- **What it says.** Across its range `B5AspBase` is a time-management dial: from the seed
  to 19000 the stop's rate falls from 27% to 10%, the low-effort extension (`TmEffortLo`,
  ×1.20) from 36% of windows to 18%, and the high-effort cut (`TmEffortHi`, ×0.80) rises
  from 21% to 32%. The seed's time manager is not the OFF arm's, and 19000's nearly is.
  At the fit's first perturbation (705 against 2115) the stop moves 2 points and the low
  share 3.7. The width run read the base's whole range at −1.0 ± 9.1, so the direct
  signal on the base is small, and a stop worth what Rarog's cost (−15 at 24%, a prior)
  would put about 1 Elo across one perturbation: the same order.
- **What follows from it, by PROCESS's rule** (*Decision rules*: categorical architecture
  is gated before its constants are fitted): the stop's 2× gate belongs before this
  fit, on the seeds, and the fit then runs on its winner. A fit under 1× that the gate
  then moves to 2× was fitted to a time cut that is gone.
- **Whatever is decided,** the fitted-constant analysis reads any movement of
  `B5AspBase` and `B5AspGrowth` against this table before the bake. The effort share's
  dilution by the re-search count (§A2.4 e) stays D.1's.
- **Decided 2026-11-10 (maintainer):** the stop's gate first. BAS-S55 (c)'s stop bracket
  is played on the seeds before block 1, and the fit runs on its winner.

#### Block 1 (registered 2026-11-10, before its first game)

- **Binary:** `basilisk-b54-on-tune-pext-tune-pgo.exe`, ON Tune, final PGO, clean at
  `a4637b6`, SHA-256 D3E1B9E9…, `bench 13` 3,920,652, `B5FailHighStop` advertised at
  200; its advertised defaults equal all eight seeds. Registered first on `D8D9BC67…`
  at `d07380d`, the stop at 100; the last bullet says why it changed.
- **Runner and shape:** Colosseum `tools/spsa_colosseum.ps1` on
  `tools/spsa_configs/colosseum/search.run.toml` with `-Tune …/b54.tune.toml`. 15 slots ×
  30 games per iteration, 2,000 iterations (60,000 games, about 10 hours), `r_end`
  0.0031. `3+0.03`, 1T, Hash 64, UHO paired, no adjudication, tablebases off.
- **Seeds and blocks:** block 1 seed 54001; block *n* seed 5400*n* with `-SeedFrom` block
  *n*−1; results in `tools/results/b54-spsa-block<n>`.
- **Stop rule (PLAN rule 7c), fixed before the first game:** after each block,
  `python tools/spsa_block_rule.py <block dir> --config tools/spsa_configs/config_b54.json`.
  At least three coordinates moved a full step and the next block runs; fewer and the
  tune stops, with theta that block's rounded centres. A block cut short never counts;
  the same command resumes from its checkpoint.
- **Validation:** the runner's own dry run on block 1's exact arguments
  (`tools/results/b54-20261110/validate_block1_stop200.ps1`;
  `dryrun-block1-stop200.json` 877DD4D6…) accepts: 8 parameters, 2,000 × 30,
  concurrency 15, `r_end` 0.0031, seed 54001, no adjudication, the binary's revision,
  fingerprint and every option checked, and the stop advertised at 200. The first
  registration's dry run was `dryrun-block1.json` EF41871B….
- **Launch:** `pwsh -NoProfile -File tools/spsa_colosseum.ps1 -Engine tools/test_engines/basilisk-b54-on-tune-pext-tune-pgo.exe -ExpectBench 3920652 -ExpectRevision a4637b6b4c91fefa0e7909a9120d64afef895cf5 -Tune tools/spsa_configs/colosseum/b54.tune.toml -Seed 54001 -Dir tools/results/b54-spsa-block1`.
- **After BAS-S55 (c)'s stop gate (maintainer 2026-11-10).** On H0 or the cap, block 1
  runs as registered here. On H1 the arm's default stop moves to 200, the Tune build is
  rebuilt and re-pinned, and this block is registered again on it, its dry run repeated,
  before its first game; the surface, the seeds, the steps and the stop rule do not change.
- **Registered again 2026-11-10, before its first game:** the gate read H1 (+9.3 ± 4.4
  Elo over 8,260 games). The binary, the validation and the launch line above are the
  new ones; the surface file is the same (`b54.tune.toml` FB44912C…), as are the seeds,
  the steps, the shape and the stop rule. No game of block 1 was played on the first
  binary.
- Disposition: block 1 is prepared on the arm with the stop at 2×, maintainer-run, not
  started.
- **Block 1 (maintainer-run, 2026-11-10 20:25 to 2026-11-12 04:59 UTC):**
  - 2,000 iterations, 60,000 games, in three sessions: Windows Update restarted the host
    at iteration 650 (KB5132061), the maintainer stopped it at 844, and each session
    resumed from the block's own checkpoint at the stored seed and horizon. The three
    manifests (`colosseum_spsa_b54-spsa-block1_20261110_212550`, `…_20261111_072413`,
    `…_20261111_224716`) name the same binary, revision, surface file and runner.
  - The PGN recount agrees with the journal and the driver: 60,000 games, 2,000
    iterations of 30, the plus side 15,432 / 28,935 / 15,633. `result.json` 4bc6ef38…,
    `tuned-options.txt` 09b3a055….
  - **Two time forfeits:** games 571 (the minus side) and 601 (the plus side), iterations
    19 and 20, both while Windows installed a Visual Studio security update and the
    malicious-software removal tool. No other engine fault and no infrastructure fault.
    PROCESS *Harness* requires a tune block to have none, and the wrapper refused to
    certify the block (its last manifest has no completion lines).
  - **Their bound:** at iterations 19 and 20 one game point moves every centre 0.0036 of
    a step, so the two games together shift any centre by at most 0.0144 steps to first
    order. The nearest decision edge is `B3NmpPerPly`'s rounding edge, 0.073 steps away:
    the movement verdict and its three movers cannot change, and theta can change by at
    most one unit of a coordinate, at most 1/23 of a step.
  - **Accepted by the maintainer 2026-11-12** as an exception to the zero-fault rule for
    this block, on that bound and the host cause. The rule stands for later blocks, and
    the wrapper still enforces it.
  - **Movement rule: CONTINUE**, three of eight coordinates moved a full step:
    `B3SingMargin` 232 → 201 (−1.35 steps), `B5AspBase` 1410 → 2202 (+1.12) and
    `B3NmpPerPly` 13 → 11 (−1.00). The rest: `B5AspMsqDiv` 2874 → 2907, `B5AspGrowth`
    47 → 46, `CoreContHistPruneSlope` 3832 → 3783, `CoreCorrUpdateScale` 17 → 18,
    `B3IirMinDepth` 6 → 7.
  - **Interim reading of the constants** (the full analysis is written before the bake):
    - No rail. Every centre stayed at least 1.27 steps inside its range: `B5AspBase` at
      its lowest, early, and `B3IirMinDepth` 1.45 from both ends of 3–10.
    - Net travel against a random walk (net over the root sum of squared moves; PLAN
      B.6's B-101): `B3SingMargin` −4.06, `B5AspBase` +3.41, `B3NmpPerPly` −2.50, every
      other within ±1.6. `CoreCorrUpdateScale` reads +2.26 in the second half alone.
    - Two reversals, both on coordinates without a signal: `B5AspMsqDiv` fell to 2626
      and came back to 2907; `CoreContHistPruneSlope` rose to 3925 and came back to 3783.
    - `B5AspBase` flattened: its highest centre was 2282, and the last 300 iterations
      moved it −0.07 steps. It is under 2820, so BAS-S53 *CONDITIONS* sends nothing back to
      BAS-S55 (c). Beside the audit table's 2115 column the stop still ends about 26% of
      searches, as at the seed.
    - `B3SingMargin` is still moving (−0.26 steps over the last 300 iterations), and this
      is its second fit down (B.3's sweep seed 252, B.3's fit 232). A smaller margin puts
      the singular bound nearer the TT score, so more TT moves are extended. The analysis
      reads it against the arm's +39.4% singular tries by the median.
    - `B3NmpPerPly` lower makes the null move's entry stricter at depth: the allowance
      the static eval may fall below beta drops from 13·depth to 11·depth.
- **Block 2 (registered 2026-11-12, before its first game):** seed 54002, `-SeedFrom`
  block 1; the binary, surface file, steps, shape and stop rule are block 1's.
  - **Validation:** `tools/results/b54-20261110/validate_block2.ps1` (49bfdeee…, output
    `validate_block2.txt` 08d83016…; `dryrun-block2.json` 0C05CB5A…) accepts: 8
    parameters, 2,000 × 30, concurrency 15, `r_end` 0.0031, seed 54002, no adjudication,
    the binary's revision, fingerprint and options, the stop advertised at 200, and every
    start equal to block 1's rounded centre with block 1's `c_end` and bounds.
  - **Launch:** `pwsh -NoProfile -File tools/spsa_colosseum.ps1 -Engine tools/test_engines/basilisk-b54-on-tune-pext-tune-pgo.exe -ExpectBench 3920652 -ExpectRevision a4637b6b4c91fefa0e7909a9120d64afef895cf5 -Tune tools/spsa_configs/colosseum/b54.tune.toml -Seed 54002 -SeedFrom tools/results/b54-spsa-block1 -Dir tools/results/b54-spsa-block2`.
- Disposition: block 1 complete (CONTINUE; its two forfeits accepted by the maintainer);
  block 2 prepared, maintainer-run, not started.
- **Block 2 withdrawn unrun (maintainer, 2026-11-12),** before its first game and before
  any game of BAS-S56: a second block buys a few Elo at most, and gains that size wait
  for Phase H. Rule 7c's CONTINUE is overridden for this tune, not changed. **Theta is
  block 1's rounded centres**, the eight lines of `tuned-options.txt` (09b3a055…). Its
  registration and dry run stay above as the record.
- Disposition: one block; theta is block 1's centres. The fitted-constant analysis and
  the bake follow.

#### PREDICTION CALIBRATION — preserve the original prediction above

- *Owed at the leaf's close.*
