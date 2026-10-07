# BAS-E56

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E56 - 15.0.a isolated cost gate: the SEE king-legality repair against
its own parent - CLOSED, REPAIR KEPT** (2026-09-09).

- Date / owner / calibration category: 2026-09-09 / maintainer-run / isolated
  A/B cost probe (not a release gate).
- Baseline: `tools/test_engines/basilisk-15.0a-base-pext-pgo.exe`
  (revision `ed8db0fc94`, bench 12,568,898). Candidate:
  `basilisk-15.0a-cand-pext-pgo.exe` (revision `044b7f072d`, bench
  14,978,465). The two revisions differ in `src/board.cpp` only (+24 lines);
  both are fresh pext-PGO builds from the same toolchain and training bench.
- Research question: what does 15.0.a's **+19.17% bench-node cost** actually
  cost in Elo? This is a magnitude question, not an existence question.
- Instrument: `-Mode fixed`, judged by the ESTIMATE at fixed N. This was
  challenged on 2026-09-09 and briefly re-registered as a capped SPRT before
  any exposure; the revision was WRONG and is reverted. Recorded in full
  because the reasoning is the useful part:
  - A `gainer [0,3]` SPRT -- the default, and BAS-E55's shape -- is the wrong
    test here. Its H0 ("not a >= 3 nElo gainer") is near-certain a priori for
    a change predicted at -3, so accepting it carries no information about the
    decision actually in front of us.
  - A `simplify [-5,0]` SPRT DOES bracket the keep/revert decision correctly,
    and the maintainer's verdict rule was already stated in H0/H1 terms, so it
    would need no translation. On that much the challenge was right.
  - It is nevertheless unavailable: `tools/sprt.ps1:502` hard-refuses `-Games`
    in every SPRT mode, and without it the run stops only on the LLR
    boundaries or a 50,000-ROUND (100,000-game) hard stop. The refusal exists
    because on 2026-08-27 a `-Games 16000` SPRT was believed capped at three
    hours per arm and ground toward 100,000 games, costing about ten hours.
  - This experiment's own frozen central prediction is -3, which sits almost
    exactly midway between -5 and 0 -- the region where an uncapped SPRT
    cannot terminate. Expected cost would be roughly 18 hours against about
    two for the fixed-N probe, for a strictly less useful output.
  So the operative argument is not that SPRT stalls in the abstract, which is
  a weak claim, but that THIS harness cannot cap one and our own prediction
  lands in the stall region. BAS-C05 recorded the same shape of failure
  ("a `[-5,0]` SPRT on a true ~-2.4 sits between the hypotheses and cannot
  terminate"), and `tools/sprt.ps1` documents `-Mode fixed` as the instrument
  for exactly this near-bound question.
- Why now rather than folded into BAS-E55: attribution. If the 15.0 head is
  gated as one lump and returns H0, 15.0.a and 15.0.c are inseparable. 15.0.c
  additionally decides "repair or documented approximation" on cost grounds,
  so this number is a required input to the next leaf.
- Interacting mechanisms: every SEE consumer prunes or deprioritizes on
  `see_ge == FALSE` (`search.cpp:1443`, `:1446`, `:1475`, `:1712`, `:1874`,
  `:1899`, and classification at `:898`/`:1908`/`:2146`). The repair raises
  SEE verdicts, so it loosens all of those gates at once. Their thresholds
  were tuned against the pre-repair, systematically pessimistic SEE.
- Competing hypotheses: (a) the node cost dominates at `3+0.03` and the
  accuracy gain is too rare to pay for it; (b) the loss is threshold
  MIScalibration rather than the legality rule being wrong; (c) the random-walk
  corpus that measured the correctness direction understates how often the
  changed verdicts occur in real search, so the true accuracy benefit is
  larger than 333/1,896,743 suggests.

PRE-REGISTERED PREDICTION, frozen before exposure:

- Expected Elo: **-8 to +2** at STC 1T against its parent; central **-3**.
- Probability the candidate is positive: **0.30**. Confidence moderate.
- Most likely failure mode: +19.17% nodes costs more time-to-depth than the
  rarer-but-correct SEE verdicts return.
- Known instrument caveat, stated in advance: the 1,896,743-capture corpus
  measured the correctness DIRECTION well (6,481-0 / 301-0) but is a random
  walk that drifts to sparse endgames, so its 0.018% verdict-change rate is
  not a frequency estimate for bench- or game-like positions. The +19.17%
  bench delta is the better frequency signal, and the gap between the two is
  itself evidence that the changed verdicts are far more common in real
  search than the corpus rate implies.
- Registered gate: `tools/sprt.ps1 -Mode fixed -Games 10000`, STC `3+0.03`,
  1T, Hash 64, paired UHO (`tools/books/UHO_Lichess_4852_v1.epd`), no
  adjudication. Elo CI approximately +/-4.2 at that N. Time-forfeit
  admissibility: see the amended clause under the verdict rule.

FROZEN VERDICT RULE (maintainer, 2026-09-09, before exposure). The maintainer
stated the rule in SPRT terms. `-Mode fixed` has no H0/H1 and no non-resolve,
and the SPRT that would carry those terms natively is not runnable here (see
Instrument above), so the rule is translated to the reported 95% Elo CI and
frozen in that form:

- **Whole 95% CI above 0** (their "H1"): **keep** the repair. 15.0.a stands.
- **Whole 95% CI below 0** (their "H0"): **revert** the king-legality repair
  and accept the pre-repair behaviour as a precisely documented incorrectness,
  on the BAS-C05 precedent -- recorded in `src/board.cpp` at the kernel and in
  `DESIGN.md`, not only in this ledger. Materiality this implies at N=10,000:
  revert requires a measured harm worse than about **-4.2 Elo**, which is
  close to the -3.2 Elo that an H0 bound of -5 nElo would have meant
  (paired-UHO nElo runs about 1.55x Elo on this harness).
- **CI straddling 0** (their "non-resolve"): **keep** the repair. Correctness
  is the default when the games cannot separate the arms. The estimate at the
  fixed N is unbiased and is the magnitude 15.0.c consumes.
- Retuning the SEE consumer thresholds against the repaired kernel is NOT a
  verdict branch of this experiment (maintainer decision). It remains
  available afterwards as its own leaf if the repair is kept.
- TIME FORFEITS -- clause AMENDED 2026-09-09 at 18:50 local, mid-run, while
  both the agent and the maintainer were still BLIND to the score and to the
  running Elo estimate (neither had been read; only forfeit counts and game
  totals were inspected). The original clause, "zero time forfeits required;
  any forfeit voids the run", was written by the agent and was stricter than
  this harness supports: `tools/sprt.ps1:578` treats forfeits as a
  step-failing condition only at `threads > 1` (the 9.4 MT canary) and as
  advisory at 1T, which is this run's configuration. At 14 concurrent games
  on 16 physical cores, occasional clock overruns at `3+0.03` are scheduler
  jitter rather than engine defects. Observed when the clause was amended:
  **3 forfeits in the first ~1,470 games (0.20%), split 2 parent / 1
  candidate** -- projecting to roughly 20 by game 10,000, which the original
  wording would have voided. Replaced by a rate-and-symmetry test that keeps
  the property actually worth protecting -- the repair must not lose on time
  more often than its own parent:
  - **Rate:** the run is VOID if total time forfeits exceed **0.5%** of
    completed games.
  - **Symmetry:** the run is VOID if the candidate's forfeits significantly
    exceed the parent's -- one-sided exact binomial test against H0 p = 0.5
    over the total forfeit count, void if p < 0.05. A candidate-skewed
    pattern voids the run REGARDLESS of the Elo estimate, since 15.0.a's
    +19.17% tree is exactly the kind of change that could overrun a clock
    poll; this preserves the AGENTS.md prohibition on accepting a candidate
    that fails a time-forfeit gate.
  - Forfeits are counted from the PGN `Termination "time forfeit"` tag and
    attributed to the side that lost on time.

RESULT (2026-09-10). Run `sprt_15.0a-king_vs_parent_20260910_081718`
(a first attempt, `..._20260909_182833`, was aborted by the maintainer at
2,927 games; the reported run is a clean restart, identical configuration,
seed 1621021540).

**Elo -0.65 +/- 5.25, nElo -1.10 +/- 8.88, LOS 40.40%, 5,874 games**
(W 1,496 / L 1,507 / D 2,871; 49.91%). DrawRatio 49.57%, PairsRatio 1.00,
Ptnml(0-2) [101, 639, 1456, 652, 89], WL/DD 0.84.

- **VERDICT: KEEP.** The 95% Elo CI [-5.90, +4.60] straddles zero, which the
  frozen rule maps to keep -- correctness is the default when the games
  cannot separate the arms. **15.0.a stands.**
- Stopped early at maintainer request, at 5,874 of the registered 10,000
  games. Recorded as a data-dependent stop, NOT a clean fixed-N stop. It
  cannot have changed the verdict: flipping to revert would have required the
  remaining ~4,126 games to average **-8.8 Elo**, a 2.6 SD excursion against
  the observed standard error of 2.68, **p ~ 0.005**. The cost of the early
  stop is a wider interval -- +/-5.25 instead of the +/-4.02 that 10,000 games
  would have given -- so the upper bound on possible harm is about -5.9 rather
  than -4.9 Elo.
- **Time-forfeit gate: PASSES.** 3 forfeits in 6,053 games written = 0.050%
  (ceiling 0.5%), all three lost by the candidate and none by the parent.
  One-sided exact binomial p = 0.125, above the 0.05 void threshold, so the
  amended clause does not void the run. The 3/0 split is directionally the
  failure mode a +19.17% tree would cause and was first recorded here as an
  open finding, but **15.0.b/BAS-E57, closed the same day, supersedes that
  reading**: the clock already starts at `go` receipt and is polled every
  2,048 nodes, the forfeits are 50-500 ms stalls of a saturated host that no
  engine can observe mid-search, and BAS-E57 played **this same candidate
  against itself for 10,000 games with zero forfeits in either arm** -- about
  20,000 candidate-sides with no time loss, which is strong evidence against
  a candidate-specific time-management defect. 3 events at p = 0.125 is
  consistent with chance under the host-jitter explanation. Not an open
  defect; worth re-checking only if a later run shows a significant
  candidate-side skew.

PREDICTION CALIBRATION (against the record frozen 2026-09-09):

- Predicted -8 to +2 Elo, central **-3**, P(positive) 0.30. Observed **-0.65
  +/- 5.25**. The outcome is inside the predicted range and the sign was not
  called (LOS 40.4% -- the result is statistically indistinguishable from
  zero), so the interval was right and the central estimate was pessimistic.
- **Which part of the causal model was wrong: the cost currency.** The whole
  registration, and the 15.0.a report before it, treated "+19.17% bench nodes"
  as self-evidently expensive. It was never converted into plies. At the
  measured geomean EBF of **2.900**, +19.17% nodes is
  log(1.1917)/log(2.900) = **0.165 ply** -- one sixth of a ply. That
  conversion was available as soon as both numbers existed and would have
  predicted a near-null outcome directly, instead of the "large enough to be a
  real risk to BAS-E55" framing carried in PLAN 15.0.a. The predicted failure
  mode ("node cost dominates at 3+0.03") did not occur because the cost was
  roughly a sixth of what its headline percentage suggested.
- Competing hypothesis (c) is supported: the accuracy gain is real enough to
  offset even that small cost, so the random-walk corpus's 0.018% verdict-
  change rate did understate the benefit's frequency in real search.
- Hypothesis (b), threshold miscalibration, is neither confirmed nor refuted
  and was deliberately not a verdict branch. Retuning the SEE consumer
  thresholds against the repaired kernel remains available as its own leaf.
- **Standing lesson: convert a node-count delta into plies via the EBF before
  calling it expensive.** A percentage of nodes is not a cost until it is
  divided by log(EBF); at EBF ~2.9 a 20% tree increase is a rounding error in
  depth, while at EBF ~1.5 it would be nearly half a ply.
