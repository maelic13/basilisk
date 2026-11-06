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
