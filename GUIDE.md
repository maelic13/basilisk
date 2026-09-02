# Basilisk development guide

Run this checklist in order; PLAN.md owns rationale and gates.

## Phase 1 — Foundations

- [x] **1.0** Foundations and first strength line — 1.0.0 through 1.8.0
  - [x] **1.0.a** Board, move generation, UCI, PVS/qsearch, TT, histories, SEE and Syzygy
  - [x] **1.0.b** Time management, Lazy SMP, reproducible tests and accepted HCE

## Phase 2 — Correctness and search

- [x] **2.0** Correctness and search architecture — 1.9.0
  - [x] **2.0.a** State, repetition/rule-50, TT/mate and SEE/pin correctness
  - [x] **2.0.b** Staged ordering, correction/history, root-instability timing and dense TT

## Phase 3 — Hardening and speed

- [x] **3.0** Hardening, CI and PGO speed — 1.9.1
  - [x] **3.0.a** Centralized parameters, invariants, fuzzing, CI and telemetry
  - [x] **3.0.b** Behavior-identical PGO speed pass accepted at +4.34% NPS

## Phase 4 — SMP and release tooling

- [x] **4.0** SMP durability and release tooling — 1.9.2/1.9.3
  - [x] **4.0.a** SPSA/MT harness and helper clock/node/thread safety repaired
  - [x] **4.0.b** Four-thread bundle accepted; PGO tool matching fixed without search change

## Phase 5 — Completed foundation

- [x] **5.0** Reproduce the 1.9.3 baseline
  - [x] **5.0.a** Freeze benchmark, test and compiler evidence
- [x] **5.1** Measure search/evaluation authority
  - [x] **5.1.a** Search oracle measured +322.7 +/-36 Elo
  - [x] **5.1.b** HCE oracle measured +232.8 +/-32 Elo
- [x] **5.2** Build differential diagnostics and inventory
  - [x] **5.2.a** Add the 107-position diagnostic suite and diag-kv telemetry
  - [x] **5.2.b** Split candidate work into dependency-complete clusters
- [x] **5.3** Close search cluster A: ordering, histories and LMR
  - [x] **5.3.a** Reject reduction magnitude on harness evidence
  - [x] **5.3.b** Reject check-depth change by games
- [x] **5.4** Close search cluster B: static-eval, TT and qsearch contracts
  - [x] **5.4.a** Confirm existing contracts; accept no engine change
- [x] **5.5** Close search cluster C: main selectivity
  - [x] **5.5.a** Record the history-pruning defect and exhausted budget boundary
- [x] **5.6** Close completed extension and root evidence
  - [x] **5.6.a** Retain only mechanisms supported by completed gates
  - [x] **5.6.b** Defer singular-extension depth and clock work to Phase 8
- [x] **5.7** Audit shallow-depth node cost
  - [x] **5.7.a** Measure rather than assume a width deficit
  - [x] **5.7.b** Withdraw the target after the constant-factor diagnosis
- [x] **5.8** Enlarge and freeze the HCE feature surface
  - [x] **5.8.a** Add seven coverage terms
  - [x] **5.8.b** Add bishop outpost and split king-protector structure
  - [x] **5.8.c** Identify endgame technique as the remaining structural gap
- [x] **5.9** Diagnose the first joint-fit failure
  - [x] **5.9.a** Run the distilled-corpus refit
  - [x] **5.9.b** Trace mate-drive loss to score-adjudicated corpus truncation
  - [x] **5.9.c** Establish on-policy self-play WDL labels as the fit contract
- [x] **5.10** Accept the repaired HCE line
  - [x] **5.10.a** Accept king-safety refit
  - [x] **5.10.b** Accept full-surface refit and freeze its baseline artifact
- [x] **5.11** Remove redundant HCE terms
  - [x] **5.11.a** Gate simplification without losing accepted strength
- [x] **5.12** Inventory and improve endgames
  - [x] **5.12.a** Inventory twenty reference endgame families
  - [x] **5.12.b** Improve KBNK conversion from 13% to 54.5%
- [x] **5.13** Add deterministic conversion floors
  - [x] **5.13.a** Cover KQK, KRK, KBBK and KBNK with fixed-seed tests
  - [x] **5.13.b** Record denominators and avoid treating tiny percentage changes as truth
- [x] **5.14** Repair basic mate drive and diagnose KBNK residue
  - [x] **5.14.a** Complete KXK/KBBK drive
  - [x] **5.14.b** Classify KBNK failures: stalled drive, bishop-move ties and rule-50 loss
  - [x] **5.14.c** Preserve the 198-position cohort for paired follow-up
- [x] **5.15** Port the generalized endgame-truth instrument
  - [x] **5.15.a** Support named <=6-man families with family-stable deterministic seeds
  - [x] **5.15.b** Separate Syzygy WDL truth, WDL preservation, DTZ progress and conversion
  - [x] **5.15.c** Emit per-position records and honest denominators for paired analysis
  - [x] **5.15.d** Record engine identity/hash and prevent the tested engine from using Syzygy
- [x] **5.16** Make no-adjudication the toolchain default
  - [x] **5.16.a** Default SPRT, fixed gauntlet and datagen to natural termination
  - [x] **5.16.b** Default weather-factory SPSA and its reinstall patch to natural termination
  - [x] **5.16.c** Remove adjudication from all Colosseum strength, SPSA, tournament and datagen profiles
  - [x] **5.16.d** Retain explicit opt-in only for registered legacy-compatibility runs
- [x] **5.17** Synchronize the roadmap mechanically
  - [x] **5.17.a** Reorder phases around endgame-first HCE development
  - [x] **5.17.b** Add a PLAN/GUIDE checklist consistency checker

## Phase 6 — Endgame maturity

- [ ] **6.0** Establish the truth baseline before another evaluator edit
  - [ ] **6.0.a** Generate a frozen, Syzygy-verified multi-family cohort with per-position output
  - [ ] **6.0.b** Measure the accepted Basilisk head and a strong reference at identical nodes
  - [ ] **6.0.c** Set achievable family ceilings; do not assume 100% at a finite node budget
  - [ ] **6.0.d** Define paired confidence rules: aggregate beyond 2 SE reports, family beyond 3 SE blocks
  - [ ] **6.0.e** Add hard theory vetoes for clean-win discard, illegal play, crash and rule-50 regression
  - [ ] **6.0.f** Census disagreements between self-play WDL labels and Syzygy on every <=6-man corpus row
- [ ] **6.1** Implement and tune the missing KBNK technique (historical step 5.9.22)
  - [ ] **6.1.a** Start from Rarog's useful finding: bishop-color corner diagonal potential can solve the drive without a bishop-position term
  - [ ] **6.1.b** Port the mechanism shape, not constants: correct-corner diagonal resolution, magnitude and ratio must dominate king-distance terms
  - [ ] **6.1.c** Scale coefficients to Basilisk and test interaction with its existing corner, edge, king-distance and knight-distance terms
  - [ ] **6.1.d** Do not retry bishop proximity or escape-square count unless new evidence overturns their earlier failure
  - [ ] **6.1.e** Compare on the identical 198 positions; report WDL preservation, rule-50 failures, conversion and mate efficiency
  - [ ] **6.1.f** Require KQK/KRK/KBBK non-regression and tactical/bench stability
- [ ] **6.2** Gate KBNK and accepted mate-drive changes
  - [ ] **6.2.a** Run a fresh no-adjudication [0,3] nElo SPRT against the accepted head
  - [ ] **6.2.b** Treat the old approximately 5,860-game adjudicated Group A run as preliminary only
  - [ ] **6.2.c** Never resume that run if the KBNK candidate or match policy changes
- [ ] **6.3** Add general king-to-passed-pawn approach logic
  - [ ] **6.3.a** Derive the feature from Basilisk truth failures, not reference constants
  - [ ] **6.3.b** Verify KP-K, KPP-K, KBP-K and mixed rook/minor pawn families
  - [ ] **6.3.c** Gate the isolated candidate with no adjudication
- [ ] **6.4** Audit every endgame term before broadening the evaluator
  - [ ] **6.4.a** Test score resolution, saturation and interaction at Basilisk's scale
  - [ ] **6.4.b** Keep theory truth, move quality, conversion and game strength separate
  - [ ] **6.4.c** Freeze the accepted Group A head and truth report
- [ ] **6.5** Implement high-value rook and bishop-pawn families
  - [ ] **6.5.a** Cover KRPP-KRP and KRP-KR
  - [ ] **6.5.b** Cover KR-KP, KQ-KRP and KR-KB
  - [ ] **6.5.c** Cover bishop-pawn families, including wrong-bishop/rook-pawn draw logic
  - [ ] **6.5.d** Add deterministic truth cases before coefficient fitting
- [ ] **6.6** Gate Group B
  - [ ] **6.6.a** Require paired truth improvement and no family veto
  - [ ] **6.6.b** Run no-adjudication SPRT on the frozen Group A baseline
- [ ] **6.7** Evaluate remaining lower-yield families
  - [ ] **6.7.a** Cover KPs-K, KQ-KP, KR-KN, KQ-KR, KP-KP and KNN-KP
  - [ ] **6.7.b** Implement only mechanisms with a measurable truth gap and plausible game frequency
  - [ ] **6.7.c** Stop the group when marginal value no longer pays for complexity
- [ ] **6.8** Close endgame maturity
  - [ ] **6.8.a** Freeze the accepted evaluator, truth corpus, reports and thresholds
  - [ ] **6.8.b** Record every rejected mechanism and its retry trigger
  - [ ] **6.8.c** Authorize post-endgame corpus generation only after closure

## Phase 7 — Post-endgame HCE refit

- [ ] **7.0** Harden the fit pipeline before generating expensive data
  - [ ] **7.0.a** Fit K once on training data and freeze it across all compared fits
  - [ ] **7.0.b** Accept an explicit initial vector and record every surface coordinate
  - [ ] **7.0.c** Freeze train/validation/test splits; open the test set once after selection
  - [ ] **7.0.d** Enforce exact surface coverage, gauge anchors and source restore on failure
  - [ ] **7.0.e** Hash corpora, splits, configs, binaries, tablebases, fitted vectors and reports
- [ ] **7.1** Design a phase-efficient, natural-termination corpus
  - [ ] **7.1.a** Pilot an opening/start book that yields opening, middlegame and endgame rows efficiently
  - [ ] **7.1.b** Keep one extractor definition, sampling policy and split seed for all arms
  - [ ] **7.1.c** Register corpus size, phase/material coverage and stop conditions before launch
- [ ] **7.2** Generate self-play with the accepted post-endgame head
  - [ ] **7.2.a** Use no adjudication and game-result WDL labels
  - [ ] **7.2.b** Verify termination mix, duplicate rate, phase coverage and <=6-man yield
  - [ ] **7.2.c** Freeze corpus A, its row order and hashes before any relabeling
- [ ] **7.3** Create the tablebase-relabel comparison
  - [ ] **7.3.a** Corpus A keeps original self-play game-result labels
  - [ ] **7.3.b** Corpus B is a byte-order-preserving copy except eligible <=6-man rows receive Syzygy truth labels
  - [ ] **7.3.c** Treat cursed wins/losses as draws for rule-50-compatible WDL labels
  - [ ] **7.3.d** Preserve identical rows, ordering and train/validation/test membership
  - [ ] **7.3.e** At execution time analyze exactly which positions may be relabeled; do not propagate an ending verdict backward into non-tablebase opening/middlegame rows without a separately justified rule
  - [ ] **7.3.f** Publish the changed-row count, fraction, family distribution and before/after label matrix
- [ ] **7.4** Decide whether datagen-v3 deserves a third arm
  - [ ] **7.4.a** Inspect its semantics and provenance when this step is reached
  - [ ] **7.4.b** Distinguish whole-game tablebase adjudication from row-local post-hoc relabeling
  - [ ] **7.4.c** Pilot corpus C only if it can be matched closely enough for a causal comparison
  - [ ] **7.4.d** Never merge corpus C evidence into the registered A-versus-B verdict
- [ ] **7.5** Measure optimizer dependence before the production fit
  - [ ] **7.5.a** Fit identical targets from accepted-head and neutral initial vectors
  - [ ] **7.5.b** Compare validation convergence, parameter distance and held-out loss
  - [ ] **7.5.c** Register the production initialization rule before opening the test set
- [ ] **7.6** Refit every relevant Texel-tunable HCE coordinate
  - [ ] **7.6.a** Use the same complete surface, fixed K, optimizer budget and initial rule for A and B
  - [ ] **7.6.b** Alternate nonlinear blocks where joint fitting is not valid
  - [ ] **7.6.c** Produce independently applicable candidate vectors and exact manifests
  - [ ] **7.6.d** Reject any fit with missing/frozen-by-accident coordinates or source drift
- [ ] **7.7** Test whether tablebase relabeling transfers
  - [ ] **7.7.a** Compare each candidate with the same accepted pre-fit baseline
  - [ ] **7.7.b** Run the pre-registered A-versus-B no-adjudication gate
  - [ ] **7.7.c** Use truth reports to explain endgame effects; use SPRT for strength
  - [ ] **7.7.d** Accept the label policy and vector only by the registered rule
- [ ] **7.8** Refresh data from the accepted fitted head
  - [ ] **7.8.a** Generate a new no-adjudication corpus from the accepted engine
  - [ ] **7.8.b** Reapply the accepted label contract and complete-surface fit
  - [ ] **7.8.c** Repeat only while each cycle passes its independent gate
  - [ ] **7.8.d** Stop at the first rejected cycle; do not average rejected vectors into the head
- [ ] **7.9** Freeze the classical evaluator
  - [ ] **7.9.a** Revalidate score scale, pruning margins, tactical suites and all endgame floors
  - [ ] **7.9.b** Ablate new endgame mechanisms and low-information fitted coordinates
  - [ ] **7.9.c** Archive the final surface, corpus policy, fit artifacts and retry triggers

## Phase 8 — Classical consolidation

- [ ] **8.0** Revisit singular-extension gate depth
  - [ ] **8.0.a** Re-measure only on the frozen post-refit evaluator
  - [ ] **8.0.b** Gate isolated search behavior before tuning constants
- [ ] **8.1** Remeasure search/evaluation authority
  - [ ] **8.1.a** Repeat the oracle split on the final classical head
  - [ ] **8.1.b** Use the result to prioritize post-release work, not to rewrite completed evidence
- [ ] **8.2** Complete clock and time-management work
  - [ ] **8.2.a** Diagnose remaining root-instability/time-allocation issues
  - [ ] **8.2.b** Pass 1T and 4T clock gates with zero forfeits
- [ ] **8.3** Complete correctness hardening
  - [ ] **8.3.a** Run state, repetition/rule-50, TT/mate, SEE/pin and sanitizer matrices
  - [ ] **8.3.b** Add regressions for every defect found
- [ ] **8.4** Complete portability and ISA validation
  - [ ] **8.4.a** Validate target-native execution and exact search agreement
  - [ ] **8.4.b** Publish executable ISA and same-target performance evidence
- [ ] **8.5** Complete SMP validation
  - [ ] **8.5.a** Revalidate node/thread/helper-clock safety
  - [ ] **8.5.b** Pass registered 1T/4T strength and scaling gates
- [ ] **8.6** Release the final classical line
  - [ ] **8.6.a** Reproduce clean PGO binaries and manifests
  - [ ] **8.6.b** Pass cumulative 1.9.3 and external-cohort matches
  - [ ] **8.6.c** Release as 1.9.4 if maintenance-scale, otherwise the warranted higher minor

## Phase 9 — NNUE runway

- [ ] **9.0** Freeze the NNUE state and feature contract
  - [ ] **9.0.a** Specify inputs, perspective, accumulators, serialization and refresh rules
  - [ ] **9.0.b** Add scalar oracle and incremental-state differential tests
- [ ] **9.1** Prepare the trainer and corpus
  - [ ] **9.1.a** Audit D:/code/net_trainer against the frozen contract
  - [ ] **9.1.b** Generate, validate, hash and split the teacher corpus
  - [ ] **9.1.c** Complete trainer preflight and reproducibility manifest

## Phase 10 — Baseline NNUE

- [ ] **10.0** Train and integrate the baseline network
  - [ ] **10.0.a** Train registered baselines and select on frozen validation data
  - [ ] **10.0.b** Integrate inference, accumulator updates and network packaging
  - [ ] **10.0.c** Pass scalar/incremental equality, bench and performance gates
- [ ] **10.1** Adapt search to NNUE
  - [ ] **10.1.a** Reprice evaluation-dependent pruning and correction mechanisms
  - [ ] **10.1.b** Run the single reserved post-NNUE search SPSA
  - [ ] **10.1.c** Gate 1T, LTC and 4T deployment conditions
- [ ] **10.2** Release 2.0.0
  - [ ] **10.2.a** Pass correctness, network provenance and fallback checks
  - [ ] **10.2.b** Pass prior-release and external-cohort gates

## Phase 11 — Post-NNUE frontier

- [ ] **11.0** Improve architecture and data only from measured bottlenecks
  - [ ] **11.0.a** Evaluate larger/sparser architectures and better feature transforms
  - [ ] **11.0.b** Refresh data only under a registered label and sampling contract
- [ ] **11.1** Extend search selectively
  - [ ] **11.1.a** Revisit rejected classical mechanisms only when NNUE changes their retry trigger
  - [ ] **11.1.b** Require isolated gates and preserve Basilisk-specific design

## Phase 12 — Scaling and platform

- [ ] **12.0** Improve parallel scaling
  - [ ] **12.0.a** Profile split points, contention and TT traffic at 2/4/8 threads
  - [ ] **12.0.b** Gate strength and throughput independently
- [ ] **12.1** Expand supported platforms
  - [ ] **12.1.a** Validate compilers, ISAs and packaging on target-native hardware
  - [ ] **12.1.b** Keep portable fallbacks behaviorally checked

## Phase 13 — Optional HCE fallback

- [ ] **13.0** Reopen HCE only if NNUE is abandoned or a release blocker demands it
  - [ ] **13.0.a** Require a new feature surface or new data contract; never refit the unchanged surface again
  - [ ] **13.0.b** Register budget and acceptance before work begins
