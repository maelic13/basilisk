# BAS-S22

**BAS-S22 — B.2.3: Cluster 1 curvature sweep, then SPSA in blocks**

- Date / owner / calibration category: registered 2026-10-11 (Amendment 3,
  packet §22.7), before any game; maintainer-run; parameter fit.
- Baseline source revision / candidate source revision / dirty-diff identity:
  the B.2.2-accepted head (BAS-S21 proceeds); the fit runs on the ON arm's
  Tune build; theta is baked in one engine commit.
- Binary / compiler / flags / PGO identity and hashes: recorded at run time.
- Research question: which of the cluster's 50 coordinates (review §5.10,
  Amendments 1 and 3) carry curvature at STC, and what theta does SPSA in
  blocks (PLAN rule 7c) reach from the §7.3 seeds?
- Hypothesis / proposed mechanism: the seeds are unit-correct (§7.2 rules 1–4)
  but column-uncertain; the ranges span the Stockfish-converted, the
  oracle-converted and the Basilisk-fitted columns, so the sweep locates the
  column and SPSA refines inside it.
- Interacting mechanisms / consumers: every coordinate of the cluster; the
  categorical switches are fixed before the fit (BAS-S21's paired runs).
- Credible competing hypotheses: the surface is flat at STC for most
  coordinates (BAS-E18's saturation lesson), in which case the fit is noise
  and B.2.4(b) reads H0.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected diagnostic movement: curvature on the node-margin and move-loop
  groups (the ×4 family, packet §2.6); little on the LMR 1024ths terms.
- Expected Elo sign/range: the fitted arm against the unfitted one is
  B.2.4(b)'s gate; P2 (BAS-S21) expects +40 at STC for the fitted cluster in
  all.
- Probability candidate is positive/useful: 65% (P2).
- Confidence and basis: moderate; Rarog's method (docs/reference) and the
  B.2.2 screens.
- Most likely failure mode: a flat surface at the harness noise floor;
  over-fitting to the UHO book.
- Falsification criteria: B.2.4(b) H0 at `[0,10]`.
- Cheapest prior falsifier: the curvature sweep itself (a coordinate with no
  curvature is dropped from the SPSA block).
- Registered gate / stop rule: B.2.4(b).
- Full conditions / provenance: as BAS-S21; SPSA overnight per the compute
  schedule; blocks and iteration counts recorded here when registered in
  detail (PLAN rule 7c).

#### Block registration (2026-10-13, before the first game)

- **Sweep:** `analysis/b23_sweep_2026-10-13.md`, classification frozen
  before its first point (`67ff4b8`). Four of the five coordinates are
  curved, so the tune runs.
- **Surface:** the core kernel table's 50 coordinates (review §5.10 with
  Amendments 3–4), generated from `src/search_params.h` by
  `tools/generate_spsa_surface.py` (`e24914f`):
  - `tools/spsa_configs/colosseum/core.tune.toml` (git blob `175eec7c…`);
  - `tools/spsa_configs/config_core.json` (git blob `15fac8c8…`);
  - steps `max(2, round(range/16))`; `c_end` set for 2,000 iterations.
- **Fixed, not tuned:**
  - the six categorical switches at their settled values
    (`CoreLmpExemptChecks` 0, the other five at their defaults);
  - the eight legacy coordinates the core table splices in (null move,
    ProbCut, singular, `LmrSingularQuiet`: B.3's mechanisms);
  - the root and time table.
- **Binary:** `basilisk-b23-on-tune-pext-tune-pgo.exe`, ON Tune, final
  PGO, source `526dd49`. SHA-256
  `CBCB9EBE67343B6055D891723D5E697C76B382C06E6B77988506D99ED50D1AC6`;
  `bench 13` 6,199,439.
- **Runner and shape:** Colosseum `tools/spsa_colosseum.ps1` on
  `tools/spsa_configs/colosseum/search.run.toml` with
  `-Tune …/core.tune.toml`:
  - 15 slots × 30 games per iteration, 2,000 iterations per block (60,000
    games), `r_end` 0.0031;
  - `3+0.03`, 1T, Hash 64, UHO paired, no adjudication, tablebases off.
- **Seeds and blocks:**
  - block 1 seed 23001;
  - block 2 seed 23002 with `-SeedFrom` block 1;
  - block 3 seed 23003 with `-SeedFrom` block 2;
  - results in `tools/results/b23-spsa-block{1,2,3}`.
- **Stop rule (PLAN rule 7c), fixed before the first game:** after each
  block, `python tools/spsa_block_rule.py <block dir> --config
  tools/spsa_configs/config_core.json`.
  - At least three coordinates moved a full step: the next block runs.
  - Fewer: the tune stops, and theta is that block's rounded centres.
  - Never more than three blocks. A block cut short never counts.
- **Estimator:** the last completed block's rounded centres, without
  checkpoint selection (PROCESS, go/no-go step 6).
- **Afterwards:**
  - Theta is baked in one engine commit into the core table's defaults.
  - The correctness floors are verified under generous limits (step 7).
  - B.2.4(b) gates the fitted arm against the unfitted one at `[0,10]`.
- **Budget:** at most 180,000 games, about 10.5 hours per block at the
  5,700 games/hour B.2.2's runs read. Maintainer-run overnight, one
  pinned harness at a time.
- **Validation:** the wrapper's preflight and the runner's own `--dry-run`
  on the exact block-1 arguments (`tools/results/b23-20261013/
  validate_tune.ps1`, `dryrun-block1.json` `adfc6f80…5018`). They read:
  - 50 parameters, all advertised by the binary;
  - 2,000 iterations × 30 games, concurrency 15, automatic placement;
  - no adjudication, `r_end` 0.0031, seed 23001, manifest bench 6,199,439.

#### RESULT — append after exposure

- Curvature sweep (2026-10-13; `analysis/b23_sweep_2026-10-13.md`):
  - **curved:** `CoreRfpPerPly`, `CoreLmpSquareScale`, `CoreLmrOffset`,
    `CoreCorrUpdateScale`;
  - **monotone:** `CoreQuietFutBase`.
  - Every curve peaks at the seed (WAC 249 at 100k against 237–247), so
    the curvature is shallow.
  - Against the prediction: the node-margin and move-loop groups held,
    except `CoreQuietFutBase`; the LMR offset curved against "little".
- Disposition: sweep read, the tune runs; block 1 prepared, maintainer-run, not started.
- 2026-10-13, before any game (Amendment 5, packet §27): the block-1 registration above is superseded. An `I1` commit corrects three history-unit sums (the donor's `PieceValue`), the good-capture split (`−score/47`) and the `CoreRfpCorrMult` seed (676), and fixes `CoreQuietSeeCoeff` off the surface; the surface (49), binary hash, bench, dry run and the sweep are re-recorded here before block 1 launches. Shape, gain, seeds, blocks, stop rule and estimator are unchanged.
- **Block 1 re-registered 2026-10-13, before any game (Amendment 5; packet
  §28).** Shape, gain, seeds, blocks, stop rule and estimator are
  unchanged.
  - **Engine:** `3eea3c5`.
  - **Surface:** 49 coordinates, `CoreRfpCorrMult` initial 676,
    `CoreQuietSeeCoeff` off (an `F` line), regenerated at `b5983c6`.
    Git blobs: `core.tune.toml` `f461bb38…`, `config_core.json`
    `12b234af…`.
  - **Binary:** `basilisk-b23b-on-tune-pext-tune-pgo.exe` (ON Tune, final
    PGO, clean manifest at `b5983c6`). SHA-256
    `A900EFD2AC77CD3F4178A1161A4FA5091A313DA5F4CCCE4EA799043C99CA2AEB`;
    `bench 13` 6,096,792.
  - **Sweep re-run** on that binary under the frozen rule
    (`analysis/b23_sweep_2026-10-13.md`, *Re-run on `3eea3c5`*):
    - curved: `CoreLmpSquareScale`, `CoreLmrOffset`, `CoreCorrUpdateScale`;
    - monotone: `CoreRfpPerPly`, `CoreQuietFutBase`;
    - so the tune runs. The seed is now the lowest or tied-lowest WAC
      point on four of five lines.
  - **Validation:** the wrapper's preflight and the runner's own
    `--dry-run` on the exact block-1 arguments
    (`tools/results/b23-20261013b/validate_tune.ps1` `9706b9b9…ad21`,
    `dryrun-block1.json` `55684045…2c2b`). They read:
    - 49 parameters, all advertised;
    - 2,000 iterations × 30 games, concurrency 15, automatic placement;
    - no adjudication, `r_end` 0.0031, seed 23001;
    - `CoreRfpCorrMult` initial 676, manifest bench 6,096,792.
  - **Lost evidence:** the superseded registration's dry run
    (`adfc6f80…5018`) was overwritten in
    `tools/results/b23-20261013/` by a mis-pathed first validation run of
    this registration (kept there as
    `dryrun-block1.overwritten-by-b23b.json`). The superseded file is not
    recoverable; its hash above is the record.
  - **Launch:** `pwsh -NoProfile -File tools/spsa_colosseum.ps1 -Engine
    tools/test_engines/basilisk-b23b-on-tune-pext-tune-pgo.exe
    -ExpectBench 6096792 -ExpectRevision
    b5983c6dfa08c136231d636d9bcbd5eb7cecfc5f -Tune
    tools/spsa_configs/colosseum/core.tune.toml -Seed 23001 -Dir
    tools/results/b23-spsa-block1`.
- Disposition: block 1 re-registered and prepared, maintainer-run, not started.
- **Block 1 (maintainer-run, 2026-10-13 21:16 to 2026-10-14 09:37 UTC):**
  - 2,000 iterations, 60,000 games, 0 engine, time or infrastructure
    faults (`tools/results/b23-spsa-block1/`, `result.json`
    `40c63cdb…8c3c`, manifest
    `colosseum_spsa_b23-spsa-block1_20261013_231631.manifest.txt`).
  - **Movement rule: CONTINUE**, six of 49 coordinates moved at least one
    full step:
    - `CoreIirMinDepth` 4 → 8 (at the top of its range);
    - `CoreRfpPerPly` 133 → 113;
    - `CoreRfpImproving` 89 → 99;
    - `CoreLmrCutoffCntAll` 1138 → 982;
    - `CoreResearchShallower` 2 → 0 (at the bottom of its range);
    - `CoreCorrUpdateScale` 12 → 14.
  - The interim analysis of the constants:
    `analysis/b23_constants_2026-10-14.md`.
- **Block 2 prepared:** seed 23002, `-SeedFrom` block 1, the same surface
  and steps.
  - The runner's dry run (`tools/results/b23-20261013b/
    dryrun-block2.json` `85c2a64e…a1cb`) accepts: 49 parameters, 2,000
    × 30, concurrency 15, `r_end` 0.0031, no adjudication.
  - It starts from block 1's rounded centres (`CoreIirMinDepth` 8,
    `CoreResearchShallower` 0, `CoreRfpPerPly` 113, `CoreRfpCorrMult`
    581).
  - Launch: `pwsh -NoProfile -File tools/spsa_colosseum.ps1 -Engine
    tools/test_engines/basilisk-b23b-on-tune-pext-tune-pgo.exe
    -ExpectBench 6096792 -ExpectRevision
    b5983c6dfa08c136231d636d9bcbd5eb7cecfc5f -Tune
    tools/spsa_configs/colosseum/core.tune.toml -Seed 23002 -SeedFrom
    tools/results/b23-spsa-block1 -Dir tools/results/b23-spsa-block2`.
- Disposition: block 1 complete (CONTINUE); block 2 prepared, maintainer-run, not started.
