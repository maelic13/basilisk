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

#### Block 1 (registered 2026-11-10, before its first game)

- **Binary:** `basilisk-b53-on-tune-pext-tune-pgo.exe`, ON Tune, final PGO, clean at
  `d07380d`, SHA-256 D8D9BC67…, `bench 13` 3,920,652. No engine source has changed
  since; its advertised defaults equal all eight seeds.
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
  (`tools/results/b54-20261110/validate_block1.ps1`; `dryrun-block1.json` EF41871B…)
  accepts: 8 parameters, 2,000 × 30, concurrency 15, `r_end` 0.0031, seed 54001, no
  adjudication, the binary's revision, fingerprint and every option checked.
- **Launch:** `pwsh -NoProfile -File tools/spsa_colosseum.ps1 -Engine tools/test_engines/basilisk-b53-on-tune-pext-tune-pgo.exe -ExpectBench 3920652 -ExpectRevision d07380daa5d707c237e3e6865022f370566b39eb -Tune tools/spsa_configs/colosseum/b54.tune.toml -Seed 54001 -Dir tools/results/b54-spsa-block1`.
- Disposition: block 1 is prepared, maintainer-run, not started.

#### PREDICTION CALIBRATION — preserve the original prediction above

- *Owed at the leaf's close.*
