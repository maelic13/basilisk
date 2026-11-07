# BAS-S51

**BAS-S51 — B.4.6: the Cluster 3 curvature sweep, then SPSA in blocks**

- **Registration.** Created 2026-11-06 at B.4.6's opening, before any sweep point
  or game; agent-run sweep, maintainer-run SPSA; parameter fit. Assembled from
  packet §11's BAS-S51 text (`analysis/b43_cluster3_research_2026-11-06.md`), which
  amendment 2 left unchanged, §5.8's surface rule and B.4.5's read of it (BAS-S50).
- **Subject.** The B.4.5-closed arm, source `516d422`, the ON arm's Tune build: every
  categorical at its default after BAS-S50 (`B4EvasionPrune` 1, `B3FailLowStore` 1,
  `B4GainValues` 0, `B4SeeValues` 0, `B4QuietChecks` 0, `B4RfpSeekMate` 0), `bench 13`
  4,003,560. Theta is baked in one engine commit with the arm's new fingerprint.
- **Research question.** Which of Cluster 3's coordinates carry curvature at STC, and
  what theta does SPSA in blocks (PLAN rule 7c) reach from the seeds, with the
  coordinates whose populations the arm moved fitted beside them?
- **Hypothesis.** The futility base and the blends are unit-correct conversions of
  the donor's values, unfitted, and the main search's margins were fitted on the old
  quiescence, so the fit moves both toward each other (§5.2, §5.8).
- **Competing explanation.** The surface is flat at STC for most coordinates and the
  fit is noise: BAS-S52 (b) then reads H0.

#### SURFACE (fixed before the sweep's first point)

- **Cluster 3's X lines:** `B4QsFutBase`, `B4QsStandPatBlend`, `B4QsFailHighBlend`.
- **Named unconditionally by §5.8 and PLAN:** `CoreRazorMargin`, `CoreRfpPerPly`,
  `CoreRfpImproving`, `CoreRfpWorsening`, `CoreRfpCorrMult`, `CoreRfpBetaWeight`,
  `CoreRfpDepthCap`, `B3NmpBase`; and, the store being at 1, `B3IirMinDepth` and
  `CoreLmrCutNodeNoTt`.
- **Admitted by §5.8's rule** on B.4.5's depth-14 read: `B3NmpPerPly` (+16.4%) and
  `CoreResearchDeeper` (−11.6%, a proxy).
- **15 coordinates.** Each seeds from the value the arm's binary advertises, so
  `CoreRfpPerPly` from 99 and `CoreRfpCorrMult` from 498.
- **Never on the surface:** the F lines (`B4QsMoveCount`, `B4QsSeeMargin` among
  them), every S switch and every categorical. A swept coordinate read flat is
  dropped and reported; a coordinate the fit leaves flat is reported, not dropped.
- **Ranges** are `src/search_params.h`'s. The generated configuration
  (`tools/generate_spsa_surface.py`) is cited by hash at the block registration.

#### SWEEP (frozen in `analysis/b46_sweep_2026-11-06.md` before its first point)

- `B4QsFutBase` at 43, 65, 86, 129, 172; `B4QsStandPatBlend` and `B4QsFailHighBlend`
  each at 256, 348, 441 (462), 732, 1024, the last being no blend; `CoreRfpPerPly` and
  `CoreRazorMargin` at 0.5×, 0.75×, 1×, 1.5× and 2×. Per point: `bench 13` (replayed),
  WAC at 100,000 nodes, and paired depth at 300,000 nodes, recorded.
- Rarog's frozen classification on the WAC column. Flat or monotone on all five skips
  the SPSA: the seeds ship, and BAS-S52 (b) is not run.

#### SPSA (registered in detail before block 1's first game)

- Colosseum blocks on the Tune build at `3+0.03`, 1T, Hash 64, UHO paired, no
  adjudication, tablebases off; the registered estimator and block rule (PROCESS
  *SPSA go/no-go*). Seeds 46001, 46002, … with `-SeedFrom` the previous block.
- Stop rule (PLAN rule 7c): after each block, `tools/spsa_block_rule.py`. At least
  three coordinates moved a full step and the next block runs; fewer and the tune
  stops. Never more than three blocks without a new registration.
- Theta is the last completed block's rounded centres, never a selected checkpoint.
  The fitted-constant analysis is written before the bake: rails, full-step movers,
  reversals, unit and cohesion problems, each with an owner.
- From B.4.4's review: if the bake trips `test_tt_switches_live`, its precondition is
  repaired in its own commit.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **The sweep's** (S1 to S4) are frozen in the sweep record: at least one coordinate
  curved (85%); `B4QsFutBase`'s WAC maximum at 1× or above (65%); a chaotic bench
  column (80%); no blend on the fail high shrinks the tree (65%).
- **The fit's.** P2 (BAS-S50) expects B.4 at +10 Elo at STC after this fit and the
  gate, frozen before the paired run read +29.3.

#### RESULT — append after exposure

- **The sweep (2026-11-06, agent-run, zero-game; `analysis/b46_sweep_2026-11-06.md`):**
  three coordinates are curved (`B4QsFutBase`, `B4QsStandPatBlend`, `CoreRazorMargin`).
  `B4QsFailHighBlend` reads monotone, its 256 edge tying the seed at 258, and
  `CoreRfpPerPly` monotone toward its 2× edge. None is flat, so none is dropped. **By
  the frozen rule the SPSA runs** on the 15-coordinate surface. S1 to S4 hit, S4 weakly.
- **Perturbation proof** (before the block; `tools/results/b46-20261106/perturb.txt`):
  each of the ten coordinates the sweep did not cover moves `bench 13` at both range
  edges, by 0.8% to 35.9%. The sweep proves the other five. None is inert.

#### Block registration (2026-11-06, before the first game)

- **Surface:** `tools/spsa_configs/colosseum/b46.tune.toml` (git blob `c3c4025a…`) and
  `tools/spsa_configs/config_b46.json` (git blob `0224fafa…`), generated by
  `tools/generate_spsa_surface.py` at `14a1810`: 15 coordinates, steps
  `max(2, round(range/16))`, `c_end` for 2,000 iterations.
- **Fixed, not tuned:** every categorical at its BAS-S50 setting, the F lines, the
  attribution and departure switches, the earlier clusters' coordinates §5.8 did not
  admit, and the root and time table.
- **Binary:** `basilisk-b45s-on-tune-pext-tune-pgo.exe`, ON Tune, final PGO, clean at
  `516d422`, SHA-256 FDB8D28C…, `bench 13` 4,003,560. No engine source has changed
  since. Its advertised defaults equal all 15 seeds.
- **Runner and shape:** Colosseum `tools/spsa_colosseum.ps1` on
  `tools/spsa_configs/colosseum/search.run.toml` with `-Tune …/b46.tune.toml`. 15 slots
  × 30 games per iteration, 2,000 iterations per block (60,000 games, about 10 hours),
  `r_end` 0.0031. `3+0.03`, 1T, Hash 64, UHO paired, no adjudication, tablebases off.
- **Seeds and blocks:** block 1 seed 46001; block *n* seed 4600*n* with `-SeedFrom`
  block *n*−1; results in `tools/results/b46-spsa-block<n>`.
- **Stop rule (PLAN rule 7c), fixed before the first game:** after each block,
  `python tools/spsa_block_rule.py <block dir> --config
  tools/spsa_configs/config_b46.json`. At least three coordinates moved a full step and
  the next block runs; fewer and the tune stops, with theta that block's rounded
  centres. A block cut short never counts; the same command resumes from its checkpoint.
- **Validation:** the runner's own dry run on block 1's exact arguments
  (`tools/results/b46-20261106/validate_block1.ps1`; `dryrun-block1.json` 6A970971…)
  accepts: 15 parameters, 2,000 × 30, concurrency 15, `r_end` 0.0031, seed 46001, no
  adjudication, the binary's revision, fingerprint and every option checked.
- **Launch:** `pwsh -NoProfile -File tools/spsa_colosseum.ps1 -Engine tools/test_engines/basilisk-b45s-on-tune-pext-tune-pgo.exe -ExpectBench 4003560 -ExpectRevision 516d42224038ee28ebd840f11cfbcc8188a2e480 -Tune tools/spsa_configs/colosseum/b46.tune.toml -Seed 46001 -Dir tools/results/b46-spsa-block1`.
- Disposition: the sweep sends B.4.6 to SPSA; block 1 is prepared, maintainer-run, not
  started.

#### Block 1 (maintainer-run 2026-11-06 to 2026-11-07)

- **Run:** `tools/results/b46-spsa-block1/`, seed 46001, one launch, 44,024 seconds at
  4,906 games an hour. 2,000 iterations, 60,000 games, 0 engine faults, 0 time losses,
  0 infrastructure faults. `result.json` 414a668f…, `tuned-options.txt` b9748818…,
  `games.jsonl` 0b5a891a…; binary FDB8D28C… as registered.
- **The rule:** one of 15 coordinates moved a full step (`B3NmpPerPly` 16 → 13, −1.50
  steps), so **STOP**: theta is block 1's rounded centres.
- **Theta:** `B4QsFutBase` 82, `B4QsStandPatBlend` 437, `B4QsFailHighBlend` 466,
  `CoreRazorMargin` 261, `CoreRfpPerPly` 107, `CoreRfpImproving` 94, `CoreRfpWorsening`
  13, `CoreRfpCorrMult` 457, `CoreRfpBetaWeight` 660, `CoreRfpDepthCap` 10, `B3NmpBase`
  172, `B3IirMinDepth` 6, `CoreLmrCutNodeNoTt` 856, `B3NmpPerPly` 13,
  `CoreResearchDeeper` 13.
- **Fitted-constant analysis** (`analysis/b46_constants_2026-11-07.md`): no rail and no
  clamped perturbation. `B3NmpPerPly` is the only clear gradient (z −4.08) and had not
  settled; the null move's entry tightens by 18 to 69 by depth, for B.6. The quiescence
  coordinates read flat, against the hypothesis. Theta reads `bench 13` 4,144,790 on the
  replay and WAC 254. Nothing blocks the bake.
- Disposition: tuned, not baked. Theta's bake is the leaf's last step.
