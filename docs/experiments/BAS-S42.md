# BAS-S42

**BAS-S42 — B.3.3: the Cluster 2 curvature sweep, then SPSA in blocks**

- **Registration.** Created 2026-11-02 at B.3.3's opening, before any
  sweep point or game; agent-run sweep, maintainer-run SPSA; parameter
  fit. Assembled from packet §11's BAS-S42 text
  (`analysis/b30_cluster2_research_2026-10-30.md`) and every *clauses
  replaced* list that touches it (amendments 2 and 3, the *Roadmap audit*
  of 2026-11-01), the review's follow-up 1, and B.3.2's §5.12 read
  (BAS-S41).
- **Subject.** The B.3.2-closed head (engine of `1b2515b`), the ON arm's
  Tune build: every §9 categorical at its default after BAS-S41
  (`B3NmpEvalGate` 0, `B3ProbCutTt` 0, `B3FailLowStore` 0,
  `CoreHistoryFills` 0, the rest the donor's forms), `bench 13`
  4,923,507. Theta is baked in one engine commit with the ON arm's new
  fingerprint.
- **Research question.** Which of Cluster 2's coordinates carry curvature
  at STC, and what theta does SPSA in blocks (PLAN rule 7c) reach from the
  unfitted seeds, with the Cluster 1 coordinates whose populations the ON
  arm moved fitted beside them?
- **Hypothesis.** The seeds are unit-correct conversions of the donor's
  values (§7) but unfitted, and Cluster 1's coordinates were fitted on the
  old Cluster 2 (B.2.3), so the fit moves both groups toward each other
  (§5.12's cohesion rule).
- **Competing explanation.** The surface is flat at STC for most
  coordinates, and the fit is noise: BAS-S43 (b) then reads H0.

#### SURFACE (packet §11 as amended; fixed before the sweep's first point)

- **Cluster 2's X lines**: the null move's entry and reduction (`B3NmpPerPly`,
  `B3NmpImproving`, `B3NmpBase`, `B3NmpRedBase`, `B3NmpRedEvalDiv`), ProbCut
  (`B3ProbCutBase`, `B3ProbCutImproving`), IIR (`B3IirMinDepth`), the
  singular margins (`B3SingMargin`, `B3SingMarginTtPv`) and the double and
  triple bars (9, the audit's count: `B3SingDouble{Base,Pv,Quiet,Deep}`,
  `B3SingTriple{Base,Pv,Quiet,TtPv,Deep}`): 19 coordinates.
- **Dropped:** `B3ProbCutTtMargin`, the small TT ProbCut's margin. Its
  switch stays 0 after BAS-S41 (+0.9 ± 9.5), so the coordinate is inert
  on this arm and would only add noise to the fit (the review's follow-up
  1 asked for this decision).
- **Cluster 1 coordinates admitted by §5.12's rule** on B.3.2's
  depth-14 read: `CoreLmpBase`, `CoreQuietFutBase`, `CoreCaptFutBase`,
  `CoreCaptSeeCoeff`, `CoreContHistPruneSlope`, `CoreRfpCorrMult` and
  `CoreCorrUpdateScale`, with amendment 2's `CoreRfpPerPly`,
  `CoreRfpImproving` and `CoreRfpDepthCap` (RFP's share +65.8%): 10
  coordinates. `CoreLmrCutNode`, `CoreLmrTableScale`, `CoreLmrOffset`
  and `CoreLmrCutNodeNoTt` stay off.
- **Never on the surface:** the F lines (verification depth,
  `seek_mate`, the correction and `ttMoveHistory` terms, the depth
  shapes), every S switch and every categorical. A swept coordinate read
  flat is dropped and reported; a coordinate the fit leaves flat is
  reported, not dropped.
- **Ranges** are `src/search_params.h`'s, spanning §7's three columns. The
  generated configuration (`tools/generate_spsa_surface.py`) is cited by
  hash at the block registration.

#### SWEEP (frozen in `analysis/b33_sweep_2026-11-02.md` before its first point)

- The five coordinates `B3NmpBase` (151), `B3NmpRedEvalDiv` (72),
  `B3ProbCutBase` (68), `B3SingMargin` (252) and `B3SingTripleBase` (20),
  each at 0.5×, 0.75×, 1×, 1.5× and 2×. Per point: `bench 13` (replayed)
  and WAC at 100,000 nodes, with paired depth at 300,000 nodes recorded.
- Read-only points: `B3SingMargin` 17 (the donor's), the donor's pair
  (17 with `B3SingTripleQuiet` −53), `B3SeekMateDepth` 8 and 16 and
  `B3SingNegative` 0 and 1 (amendment 3's F-line sweep coordinates).
- Rarog's frozen classification on the WAC column. Flat or monotone on
  all five skips the SPSA: the seeds ship as the fitted values, and
  BAS-S43 (b) is not run.

#### SPSA (registered in detail before block 1's first game)

- Colosseum blocks on the Tune build at `3+0.03`, 1T, Hash 64, UHO
  paired, no adjudication, tablebases off; the registered estimator and
  block rule (PROCESS *SPSA go/no-go*). Seeds 42001, 42002, … with
  `-SeedFrom` the previous block.
- Stop rule (PLAN rule 7c): after each block, `tools/spsa_block_rule.py`.
  At least three coordinates moved a full step and the next block runs;
  fewer and the tune stops.
- Theta is the last completed block's rounded centres, never a selected
  checkpoint. The fitted-constant analysis is written before the bake:
  rails, full-step movers, reversals, unit and cohesion problems, each
  with an owner.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Packet §11's predictions that BAS-S42 carries, verbatim:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P12 | B.3.3's sweep reads `B3SeekMateDepth` 8 and 16 within ±5 Elo of 12 at `3+0.03`, and `B3SingNegative` 1 within ±5 of 3; neither moves by games what position 11 moves by nodes | moderate | either outside ±5 says the won-position tree is Elo, and the fit moves it |

- **P12's reading.** P12 states its legs in Elo, but the registered sweep
  is zero-game, so the sweep cannot score them. BAS-S41's item 14 read
  `B3SingNegative` 1 at 0.0 ± 9.2 against 3, which scores that leg (held).
  The `B3SeekMateDepth` leg stays unscored unless games are registered for
  it.
- **The sweep's own predictions** (S1–S3) are frozen in the sweep record:
  at least one coordinate curved (75%); `B3SingMargin`'s WAC maximum at
  1× or above (70%), the donor's 17 under 232 (80%); a chaotic bench
  column (80%).
- **The fit's.** P2 (BAS-S41) expects B.3 at +20 Elo at STC after this
  fit and the gate.

#### RESULT — append after exposure

- (the sweep, then each block)
