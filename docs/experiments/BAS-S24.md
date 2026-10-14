# BAS-S24

**BAS-S24 — B.2.3 diagnostic: SPSA block 1's centres against the seeds**

- **Registration.** Registered 2026-10-14, before any game; maintainer-run;
  parameter fit, Elo layer. A diagnostic the maintainer asked for: where
  the fit stands after one block. Not a gate.
  - BAS-S22's blocks, movement rule and estimator are unchanged; block 2
    runs as registered whatever this reads.
  - B.2.4(b) stays the fitted arm's gate.
- **Revisions.** One revision, one binary: the ON Tune build at `b5983c6`
  (engine `3eea3c5`, `bench 13` 6,096,792), SHA-256
  `A900EFD2AC77CD3F4178A1161A4FA5091A313DA5F4CCCE4EA799043C99CA2AEB`. The
  same binary as SPSA block 1, so there is no build offset between the
  arms.
- **Arms.**
  - Arm A (`Block1`) sets block 1's 49 rounded centres from
    `tools/results/b23-spsa-block1/tuned-options.txt` (SHA-256
    `5DB77308154B088BE3E7D641F38776DEAA7390DDFB30D5E666A58C1E231282A1`).
  - Arm B (`Seeds`) plays the binary's defaults: the unfitted cluster with
    Amendment 5.
- **Research question.** How much Elo do block 1's centres carry over the
  seeds, at the conditions of the gates?
- **Hypothesis.** Six coordinates moved a full step:
  - IIR almost off;
  - RFP more aggressive on every term;
  - the shallower re-search off;
  - the all-node `cutoffCnt` term down;
  - the correction learning faster.

  The 43 others moved less (`analysis/b23_constants_2026-10-14.md`).
- **Interactions.** The whole Cluster 1 surface; the categorical switches
  and B.3's legacy coordinates stay fixed on both arms.
- **Competing hypotheses.**
  - SPSA's centres after 2,000 iterations are mostly noise around the
    seeds, which reads near 0.
  - The IIR and RFP moves carry real Elo, which reads positive.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected Elo:** **+8 ± 12** (80% interval [−4, +20]).
- **Probability the point estimate is positive:** 75%.
- **Confidence:** moderate. Basis: two curved sweep lines peaking away from
  the seed (packet §28), six full-step movers, and Rarog's first blocks.
- **Falsifiers:**
  - an interval wholly below 0 says block 1 moved toward worse play. Its
    theta is still not acted on: BAS-S22's estimator is the last block's,
    and B.2.4(b) decides. The constant analysis then reads the movers as
    suspect.
  - a reading above +30 would be far outside the band, and PROCESS
    *Harness* owes the fastchess cross-check.
- **Conditions:**
  - Colosseum, `tools/run_b23_block1_match.ps1` on `match-fixed.toml`;
  - 2,000 games, 1,000 paired UHO openings, seed 24001;
  - `3+0.03`, 1T, Hash 64, no adjudication, tablebases off;
  - concurrency 15 as the wrapper resolves it;
  - results in `tools/results/b23-block1-match/`.
- **Preparation:** dry run accepted 2026-10-14
  (`tools/results/colosseum_match_b23-block1-match_20261014_114416.*`).
  Arm A carries all 49 options (`CoreIirMinDepth`=8,
  `CoreResearchShallower`=0, …); arm B carries Hash and Threads only.

#### RESULT — append after exposure

- **Result (maintainer-run 2026-10-14):** **block 1's centres +57.0 ± 9.8
  Elo over the seeds** (nElo +90.0 ± 15.2).
  - W/D/L 706/913/381 for `Block1`, pentanomial [21, 146, 395, 363, 75],
    2,000 games over 1,000 pairs;
  - 0 faults, natural termination, the independent PGN recount agrees;
  - `colosseum_match_b23-block1-match_20261014_114850.*`, run record
    `0105B11B…9768`, PGN `0824360F…252D`.
- **Against the prediction:** +57.0 lies far above the 80% interval
  [−4, +20] and above the registered +30 line. It is a miss on magnitude,
  and the falsifier owes the backup-path cross-check:
  `tools/run_b23_block1_crosscheck.ps1` (fastchess, the same arms, seed
  24002), prepared, maintainer-run.
- **What it does not change:** block 2 runs as BAS-S22 registers, and
  B.2.4(b) remains the fitted arm's gate.
- **Reading, not a verdict.** The seed point was a poor operating point
  for this binary. The sweep re-run already showed it as the lowest WAC
  point on four of five lines after Amendment 5 (packet §28).
  - Part of the +57 may be the fit recovering what the unit commit cost.
    Amendment 5's own Elo is unmeasured until B.2.4(a).
- Disposition: run, +57.0 ± 9.8; cross-check owed.
