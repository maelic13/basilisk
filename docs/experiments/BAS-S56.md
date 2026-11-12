# BAS-S56

**BAS-S56 — B.5.4 diagnostic: SPSA block 1's centres against the seeds**

- **Registration.** Registered 2026-11-12, before any game; maintainer-run; parameter
  fit, Elo layer. A diagnostic the maintainer asked for: what B.5.4's fit has bought
  after one block. Not a gate; the shape is BAS-S24's for B.2.3.
  - BAS-S54's blocks, movement rule and estimator are unchanged: block 2 runs as
    registered whatever this reads, and theta is still the last block's centres.
  - B.5.5's gates stay the cluster's decision.
- **Revisions.** One revision, one binary: SPSA block 1's ON Tune build,
  `basilisk-b54-on-tune-pext-tune-pgo.exe`, clean at `a4637b6`, `bench 13` 3,920,652,
  `B5FailHighStop` 200, SHA-256
  `D3E1B9E9BF8FC87FA291DCB1C0B5BE1BE0CFED8213490722BB827E9C72F659D7`. No build offset
  between the arms.
- **Arms.**
  - Arm A (`Block1`) sets block 1's eight rounded centres from
    `tools/results/b54-spsa-block1/tuned-options.txt` (SHA-256
    `09B3A0552AB1D9FAD7A7FF0773BDDE49367987F3C7E1F0CDE7F938AF318A8DCA`): `B5AspBase`
    2202, `B5AspMsqDiv` 2907, `B5AspGrowth` 46, `CoreContHistPruneSlope` 3783,
    `CoreCorrUpdateScale` 18, `B3NmpPerPly` 11, `B3SingMargin` 201, `B3IirMinDepth` 7.
  - Arm B (`Seeds`) plays the binary's defaults, BAS-S54's seeds.
- **Research question.** How much Elo do block 1's centres carry over the seeds, at the
  gates' conditions?
- **Hypothesis.** Three coordinates moved a full step with net travel well beyond a
  random walk (BAS-S54 *Block 1*: `B3SingMargin` z −4.06, `B5AspBase` +3.41,
  `B3NmpPerPly` −2.50), so the centres carry a small gain: a wider first window, more
  singular extensions, a stricter null-move entry.
- **Interactions.** All eight coordinates change together, as theta will; this reads the
  vector, not any one coordinate. Everything off the surface is the binary's on both
  arms.
- **Competing hypotheses.**
  - The surface is flat at STC (BAS-S54's competing explanation; the width run read the
    base's range level, −1.0 ± 9.1): the drift is SPSA's noise floor and this reads
    near 0.
  - The seeds were a poor operating point, as B.2.3's were (BAS-S24, +57.0): this reads
    well above the prediction.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected Elo:** **+3**, 80% interval [−4, +10].
- **Probability the point estimate is positive:** 70%.
- **Confidence:** low-moderate. Basis: three movers of about one step each, small
  against B.2.3's six; the width run's level read on the base; Cluster 3's fit adding
  little between its paired run (+29.3, BAS-S50) and its gate (+31.7, BAS-S52), a
  prior only.
- **Falsifiers:**
  - an interval wholly below 0 says block 1 moved toward worse play. Theta is not acted
    on from this: the fitted-constant analysis reads the three movers as suspect first.
  - a reading above +20 says the seeds were a poor operating point; the analysis then
    asks which mover carries it.
- **Conditions:**
  - Colosseum, `tools/run_b54_block1_match.ps1` on `match-fixed.toml`;
  - 6,000 games, 3,000 paired UHO openings, seed 56001; about ±5.5 Elo at 95%, sized
    so that the expected +3 and 0 are not the same reading by default, and 75 minutes
    at block 1's rate;
  - `3+0.03`, 1T, Hash 64, no adjudication, tablebases off;
  - concurrency 15 as the wrapper resolves it;
  - results in `tools/results/b54-block1-match/`.
  - Not on the host with block 2 or any of Rarog's pinned harnesses running.
- **Preparation:** dry run accepted 2026-11-12
  (`tools/results/colosseum_match_b54-block1-match_20261112_085846.*`, dry run
  194A0508…). Arm A carries the eight options above; arm B carries Hash and Threads
  only.

- **Note, 2026-11-12, before any game:** the maintainer withdrew BAS-S54's block 2, so
  block 1's centres are theta and this match reads the whole fit against its seeds. The
  arms, conditions and prediction above are unchanged.

- **Stopped and resumed (2026-11-12).** The maintainer stopped the run at 3,349 games
  (+12.3 ± 7.0 on the console) and resumed it to its registered 6,000. The read is the
  full 6,000; the partial count is not a result.
- **Gate (b) rule (maintainer 2026-11-12, after that partial read and before the
  remaining games were seen; the record says so):** BAS-S56 and BAS-S55's gate (b), the
  fit against the seeds, are one pair, and PROCESS allows one read of a pair.
  - If the 95% interval at 6,000 games excludes 0 on the positive side, gate (b) is
    answered by this read and retired unrun: a one-sided 2.5% error, no looser than the
    SPRT's. The bake's `bench 13` must then reproduce this Tune build's count with
    theta set, which shows the baked binary plays this tree.
  - Otherwise gate (b) runs as registered.
  - An SPRT read of these games is not used: the log-likelihood ratio on `[0,3]` was
    1.59 at 1,665 pairs, and this sample cannot reach 2.94.

#### RESULT — append after exposure

- *Not run.*
