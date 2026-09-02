# Basilisk development plan

This is the maintainer-facing source of truth. [GUIDE.md](GUIDE.md) is its
short checklist mirror; [EXPERIMENTS.md](EXPERIMENTS.md) remains the detailed,
historical evidence ledger.

## 1. Current checkpoint

| Item | State |
|---|---|
| Branch | development |
| Released baseline | Basilisk 1.9.3; bench-13 fingerprint 11,941,440 |
| Current head | Bench 12,709,666; last recorded CTest 12/12; WAC 137/300 |
| Strength baseline | basilisk-5912-slim-pext-pgo; accepted HCE line about +12 Elo versus 1.9.3 |
| Current phase | Phase 6, step 6.0 |
| Evaluation | HCE is unfrozen for structural improvement and complete, controlled refits |
| Corpus rule | Game-result labels only; no engine-evaluation labels |
| Match/data rule | Natural termination by default; score-based adjudication requires explicit opt-in and registration |
| Long job | Rarog Texel datagen is active; do not run Basilisk builds, benches, games, extraction, tuning, or other CPU-heavy work |
| Release target | Classical release after Phase 8; NNUE 2.0.0 after Phase 10 |

The next engine change is not authorized by this planning pass. Step 6.1 records
the Rarog mate-drive lead for implementation only when that step is started.

## 2. Operating contract

- Work strictly in numbered order. A later step may be prepared, but may not
  change engine policy or consume experimental budget before its dependencies close.
- Commit each completed step with PLAN and GUIDE synchronized.
- Run python tools/diag/check_roadmap.py before committing either roadmap file.
- Consult and update EXPERIMENTS before retrying a mechanism.
- Preserve source, compiler, binary, book, corpus, split, seed, tablebase and
  command provenance. Hash immutable inputs and outputs.
- A behavior-neutral change needs the relevant static checks, CTest and exact
  bench. A playing change additionally needs its registered game gate.
- Strength tests use paired UHO openings and normalized Elo. Default SPRT is
  [0,3] nElo; broad/risky bundles use [-3,3]; simplification uses [-3,0].
- Score-based draw/resign adjudication is off by default in every tool. An
  opt-in compatibility run must record the exact adjudication policy and must
  never be mixed with natural-termination evidence.
- Do not run competing CPU-heavy work while a long tournament, tune, datagen
  or fit occupies the machine.
- Long jobs are run by the user after the model prepares and verifies the
  instrument. The model analyzes returned artifacts and applies the registered verdict.
- Reference engines teach mechanisms and experimental design. Reimplement in
  Basilisk's idiom and fit Basilisk's scale; never copy constants as acceptance evidence.
- Texel fits linear HCE coordinates. A bounded HCE SPSA prices only nonlinear,
  capped or contextual evaluation terms after the linear fit. Search SPSA runs
  only after the final HCE freezes; the later NNUE transition gets its own tune.

## 3. Required evidence

| Change | Minimum gate |
|---|---|
| Tool/docs/refactor | Syntax/static checks; focused tests; full tests/bench when execution semantics can change |
| Endgame evaluator | Deterministic truth corpus, paired WDL/DTZ evidence, conversion floors, tactical regression, SPRT |
| HCE refit | Frozen corpus/splits/surface, fixed K, untouched test set, source restore proof, clean PGO SPRT |
| Search change | Deterministic regression/telemetry, 1T STC SPRT, relevant LTC/4T confirmation |
| Time/root/SMP | 1T STC, 1T 10+0.1, 4T 10+0.1, zero forfeits, topology/hash recorded |
| Release | Reproducible PGO assets, correctness matrix, prior-release and external-cohort games |

Use tools/books/UHO_Lichess_4852_v1.epd for paired gates. SPRT decides strength;
node counts, traces, tactical suites, WDL/DTZ and fit loss explain the result.

## 4. Closed release phases

- [x] **1.0** Foundations and first strength line — 1.0.0 through 1.8.0.
  - [x] **1.0.a** Board, move generation, UCI, PVS/qsearch, TT, histories, SEE and Syzygy.
  - [x] **1.0.b** Time management, Lazy SMP, reproducible tests and accepted HCE.

- [x] **2.0** Correctness and search architecture — 1.9.0.
  - [x] **2.0.a** State, repetition/rule-50, TT/mate and SEE/pin correctness.
  - [x] **2.0.b** Staged ordering, correction/history, root-instability timing and dense TT.

- [x] **3.0** Hardening, CI and PGO speed — 1.9.1.
  - [x] **3.0.a** Centralized parameters, invariants, fuzzing, CI and telemetry.
  - [x] **3.0.b** Behavior-identical PGO speed pass accepted at +4.34% NPS.

- [x] **4.0** SMP durability and release tooling — 1.9.2/1.9.3.
  - [x] **4.0.a** SPSA/MT harness and helper clock/node/thread safety repaired.
  - [x] **4.0.b** Four-thread bundle accepted; PGO tool matching fixed without search change.

## 5. Completed development foundation

Everything complete remains here. Phase 6 is the first open engine phase, so no
completed item has a number after unfinished work.

- [x] **5.0** Reproduce the 1.9.3 baseline.
  - [x] **5.0.a** Freeze benchmark, test and compiler evidence.

- [x] **5.1** Measure search/evaluation authority.
  - [x] **5.1.a** Search oracle measured +322.7 +/-36 Elo.
  - [x] **5.1.b** HCE oracle measured +232.8 +/-32 Elo.

- [x] **5.2** Build differential diagnostics and inventory.
  - [x] **5.2.a** Add the 107-position diagnostic suite and diag-kv telemetry.
  - [x] **5.2.b** Split candidate work into dependency-complete clusters.

- [x] **5.3** Close search cluster A: ordering, histories and LMR.
  - [x] **5.3.a** Reject reduction magnitude on harness evidence.
  - [x] **5.3.b** Reject check-depth change by games.

- [x] **5.4** Close search cluster B: static-eval, TT and qsearch contracts.
  - [x] **5.4.a** Confirm existing contracts; accept no engine change.

- [x] **5.5** Close search cluster C: main selectivity.
  - [x] **5.5.a** Record the history-pruning defect and exhausted budget boundary.

- [x] **5.6** Close completed extension and root evidence.
  - [x] **5.6.a** Retain only mechanisms supported by completed gates.
  - [x] **5.6.b** Defer singular-extension depth and clock work to Phase 8.

- [x] **5.7** Audit shallow-depth node cost.
  - [x] **5.7.a** Measure rather than assume a width deficit.
  - [x] **5.7.b** Withdraw the target after the constant-factor diagnosis.

- [x] **5.8** Enlarge and freeze the HCE feature surface.
  - [x] **5.8.a** Add seven coverage terms.
  - [x] **5.8.b** Add bishop outpost and split king-protector structure.
  - [x] **5.8.c** Identify endgame technique as the remaining structural gap.

- [x] **5.9** Diagnose the first joint-fit failure.
  - [x] **5.9.a** Run the distilled-corpus refit.
  - [x] **5.9.b** Trace mate-drive loss to score-adjudicated corpus truncation.
  - [x] **5.9.c** Establish on-policy self-play WDL labels as the fit contract.

- [x] **5.10** Accept the repaired HCE line.
  - [x] **5.10.a** Accept king-safety refit.
  - [x] **5.10.b** Accept full-surface refit and freeze its baseline artifact.

- [x] **5.11** Remove redundant HCE terms.
  - [x] **5.11.a** Gate simplification without losing accepted strength.

- [x] **5.12** Inventory and improve endgames.
  - [x] **5.12.a** Inventory twenty reference endgame families.
  - [x] **5.12.b** Improve KBNK conversion from 13% to 54.5%.

- [x] **5.13** Add deterministic conversion floors.
  - [x] **5.13.a** Cover KQK, KRK, KBBK and KBNK with fixed-seed tests.
  - [x] **5.13.b** Record denominators and avoid treating tiny percentage changes as truth.

- [x] **5.14** Repair basic mate drive and diagnose KBNK residue.
  - [x] **5.14.a** Complete KXK/KBBK drive.
  - [x] **5.14.b** Classify KBNK failures: stalled drive, bishop-move ties and rule-50 loss.
  - [x] **5.14.c** Preserve the 198-position cohort for paired follow-up.

- [x] **5.15** Port the generalized endgame-truth instrument.
  - [x] **5.15.a** Support named <=6-man families with family-stable deterministic seeds.
  - [x] **5.15.b** Separate Syzygy WDL truth, WDL preservation, DTZ progress and conversion.
  - [x] **5.15.c** Emit per-position records and honest denominators for paired analysis.
  - [x] **5.15.d** Record engine identity/hash and prevent the tested engine from using Syzygy.

- [x] **5.16** Make no-adjudication the toolchain default.
  - [x] **5.16.a** Default SPRT, fixed gauntlet and datagen to natural termination.
  - [x] **5.16.b** Default weather-factory SPSA and its reinstall patch to natural termination.
  - [x] **5.16.c** Remove adjudication from all Colosseum strength, SPSA, tournament and datagen profiles.
  - [x] **5.16.d** Retain explicit opt-in only for registered legacy-compatibility runs.

- [x] **5.17** Synchronize the roadmap mechanically.
  - [x] **5.17.a** Reorder phases around endgame-first HCE development.
  - [x] **5.17.b** Add a PLAN/GUIDE checklist consistency checker.

## 6. Endgame maturity and truth

### 6.0 Evidence contract

- [ ] **6.0** Establish the truth baseline before another evaluator edit.
  - [x] **6.0.a** Freeze 770 Syzygy-verified positions across 21 endgame families.
  - [ ] **6.0.b** Measure the accepted Basilisk head and a strong reference at identical nodes.
  - [ ] **6.0.c** Set achievable family ceilings; do not assume 100% at a finite node budget.
  - [ ] **6.0.d** Define paired confidence rules: aggregate beyond 2 SE reports, family beyond 3 SE blocks.
  - [ ] **6.0.e** Add hard theory vetoes for clean-win discard, illegal play, crash and rule-50 regression.
  - [ ] **6.0.f** Census disagreements between self-play WDL labels and Syzygy on every <=6-man corpus row.

Step 6.0.a artifact: tools/diag/endgame_cohort_v1.epd and its manifest use
seed 0x4E9A2 and SHA-256
3CCF28EA3C8BC6C7E995BDA0BDD4833496B4C46758FF55C0B812E30E8AC6BF1B.
The 770 unique records contain 480 clean wins, 289 draws and one cursed win;
each carries family-stable seed, FEN, exact WDL and signed DTZ. The requested
24-clean-win/16-rule-draw family mix is left explicitly short where theory
does not supply that class. KRPP-KRP is seven men and remains outside the
available six-man tables.

### 6.1 Complete KBNK mate drive

- [ ] **6.1** Implement and tune the missing KBNK technique (historical step 5.9.22).
  - [ ] **6.1.a** Start from Rarog's useful finding: bishop-color corner diagonal potential can solve the drive without a bishop-position term.
  - [ ] **6.1.b** Port the mechanism shape, not constants: correct-corner diagonal resolution, magnitude and ratio must dominate king-distance terms.
  - [ ] **6.1.c** Scale coefficients to Basilisk and test interaction with its existing corner, edge, king-distance and knight-distance terms.
  - [ ] **6.1.d** Do not retry bishop proximity or escape-square count unless new evidence overturns their earlier failure.
  - [ ] **6.1.e** Compare on the identical 198 positions; report WDL preservation, rule-50 failures, conversion and mate efficiency.
  - [ ] **6.1.f** Require KQK/KRK/KBBK non-regression and tactical/bench stability.

### 6.2 Gate endgame Group A

- [ ] **6.2** Gate KBNK and accepted mate-drive changes.
  - [ ] **6.2.a** Run a fresh no-adjudication [0,3] nElo SPRT against the accepted head.
  - [ ] **6.2.b** Treat the old approximately 5,860-game adjudicated Group A run as preliminary only.
  - [ ] **6.2.c** Never resume that run if the KBNK candidate or match policy changes.

### 6.3 Passed-pawn king approach

- [ ] **6.3** Add general king-to-passed-pawn approach logic.
  - [ ] **6.3.a** Derive the feature from Basilisk truth failures, not reference constants.
  - [ ] **6.3.b** Verify KP-K, KPP-K, KBP-K and mixed rook/minor pawn families.
  - [ ] **6.3.c** Gate the isolated candidate with no adjudication.

### 6.4 Magnitude and coverage audit

- [ ] **6.4** Audit every endgame term before broadening the evaluator.
  - [ ] **6.4.a** Test score resolution, saturation and interaction at Basilisk's scale.
  - [ ] **6.4.b** Keep theory truth, move quality, conversion and game strength separate.
  - [ ] **6.4.c** Freeze the accepted Group A head and truth report.

### 6.5 Group B endgames

- [ ] **6.5** Implement high-value rook and bishop-pawn families.
  - [ ] **6.5.a** Cover KRPP-KRP and KRP-KR.
  - [ ] **6.5.b** Cover KR-KP, KQ-KRP and KR-KB.
  - [ ] **6.5.c** Cover bishop-pawn families, including wrong-bishop/rook-pawn draw logic.
  - [ ] **6.5.d** Add deterministic truth cases before coefficient fitting.

- [ ] **6.6** Gate Group B.
  - [ ] **6.6.a** Require paired truth improvement and no family veto.
  - [ ] **6.6.b** Run no-adjudication SPRT on the frozen Group A baseline.

### 6.7 Group C and closure

- [ ] **6.7** Evaluate remaining lower-yield families.
  - [ ] **6.7.a** Cover KPs-K, KQ-KP, KR-KN, KQ-KR, KP-KP and KNN-KP.
  - [ ] **6.7.b** Implement only mechanisms with a measurable truth gap and plausible game frequency.
  - [ ] **6.7.c** Stop the group when marginal value no longer pays for complexity.

- [ ] **6.8** Close endgame maturity.
  - [ ] **6.8.a** Freeze the accepted evaluator, truth corpus, reports and thresholds.
  - [ ] **6.8.b** Record every rejected mechanism and its retry trigger.
  - [ ] **6.8.c** Authorize post-endgame corpus generation only after closure.

## 7. Post-endgame corpus and complete HCE refit

This phase mirrors the useful experimental shape of Rarog step 4.10 while
correcting its unresolved ambiguity: post-hoc tablebase relabeling and
datagen-v3 game adjudication are distinct arms and must not be conflated.
The target is a mature final classical evaluator, not merely another fit.

### 7.0 HCE maturity and feature-completeness audit

- [ ] **7.0** Define and close the final handcrafted-evaluation surface.
  - [ ] **7.0.a** Compare Basilisk conceptually with strong maintained HCE engines in D:/code; learn coverage and interactions without copying code or constants.
  - [ ] **7.0.b** Audit material/imbalance, PST, mobility, pawn structure, passers, outposts, threats, space, king safety, initiative/winnability and draw scaling.
  - [ ] **7.0.c** Measure feature firing, phase/material coverage, correlation and ablation value on a phase-balanced corpus.
  - [ ] **7.0.d** Identify dead, duplicate, saturated and uncovered terms; simplify or add mechanisms only with position-level evidence.
  - [ ] **7.0.e** Add deterministic tests for every new categorical mechanism and freeze the architecture before production datagen.

### 7.1 Fit-tooling contract

- [ ] **7.1** Harden the fit pipeline before generating expensive data.
  - [ ] **7.1.a** Fit K once on training data and freeze it across all compared fits.
  - [ ] **7.1.b** Accept an explicit initial vector and record every surface coordinate.
  - [ ] **7.1.c** Freeze train/validation/test splits; open the test set once after selection.
  - [ ] **7.1.d** Enforce exact surface coverage, gauge anchors and source restore on failure.
  - [ ] **7.1.e** Hash corpora, splits, configs, binaries, tablebases, fitted vectors and reports.

### 7.2 Corpus design

- [ ] **7.2** Design a phase-efficient, natural-termination corpus.
  - [ ] **7.2.a** Pilot an opening/start book that yields opening, middlegame and endgame rows efficiently.
  - [ ] **7.2.b** Keep one extractor definition, sampling policy and split seed for all arms.
  - [ ] **7.2.c** Register corpus size, phase/material coverage and stop conditions before launch.

### 7.3 Generate and freeze the source corpus

- [ ] **7.3** Generate self-play with the accepted post-endgame head.
  - [ ] **7.3.a** Use no adjudication and game-result WDL labels.
  - [ ] **7.3.b** Verify termination mix, duplicate rate, phase coverage and <=6-man yield.
  - [ ] **7.3.c** Freeze corpus A, its row order and hashes before any relabeling.

### 7.4 Build matched label arms

- [ ] **7.4** Create the tablebase-relabel comparison.
  - [ ] **7.4.a** Corpus A keeps original self-play game-result labels.
  - [ ] **7.4.b** Corpus B is a byte-order-preserving copy except eligible <=6-man rows receive Syzygy truth labels.
  - [ ] **7.4.c** Treat cursed wins/losses as draws for rule-50-compatible WDL labels.
  - [ ] **7.4.d** Preserve identical rows, ordering and train/validation/test membership.
  - [ ] **7.4.e** At execution time analyze exactly which positions may be relabeled; never propagate an ending verdict backward into non-tablebase rows without a separately justified rule.
  - [ ] **7.4.f** Publish changed-row count, fraction, family distribution and before/after label matrix.

### 7.5 Analyze datagen-v3 separately

- [ ] **7.5** Decide whether datagen-v3 deserves a third arm.
  - [ ] **7.5.a** Inspect its semantics and provenance when this step is reached.
  - [ ] **7.5.b** Distinguish whole-game tablebase adjudication from row-local post-hoc relabeling.
  - [ ] **7.5.c** Pilot corpus C only if it can be matched closely enough for causal comparison.
  - [ ] **7.5.d** Never merge corpus C evidence into the registered A-versus-B verdict.

### 7.6 Initialization control

- [ ] **7.6** Measure optimizer dependence before the production fit.
  - [ ] **7.6.a** Fit identical targets from accepted-head and neutral initial vectors.
  - [ ] **7.6.b** Compare validation convergence, parameter distance and held-out loss.
  - [ ] **7.6.c** Register the production initialization rule before opening the test set.

### 7.7 Complete matched fits

- [ ] **7.7** Refit every relevant Texel-tunable HCE coordinate.
  - [ ] **7.7.a** Use the same complete surface, fixed K, optimizer budget and initial rule for A and B.
  - [ ] **7.7.b** Alternate nonlinear blocks where joint fitting is not valid.
  - [ ] **7.7.c** Produce independently applicable candidate vectors and exact manifests.
  - [ ] **7.7.d** Reject any fit with missing/frozen-by-accident coordinates or source drift.

### 7.8 Registered label-contract gate

- [ ] **7.8** Test whether tablebase relabeling transfers.
  - [ ] **7.8.a** Compare each candidate with the same accepted pre-fit baseline.
  - [ ] **7.8.b** Run the pre-registered A-versus-B no-adjudication gate.
  - [ ] **7.8.c** Use truth reports to explain endgame effects; use SPRT for strength.
  - [ ] **7.8.d** Accept the label policy and vector only by the registered rule.

### 7.9 Iterative refresh

- [ ] **7.9** Refresh data from the accepted fitted head.
  - [ ] **7.9.a** Generate a new no-adjudication corpus from the accepted engine.
  - [ ] **7.9.b** Reapply the accepted label contract and complete-surface fit.
  - [ ] **7.9.c** Repeat only while each cycle passes its independent gate.
  - [ ] **7.9.d** Stop at the first rejected cycle; never average rejected vectors into the head.

### 7.10 Nonlinear HCE tuning

- [ ] **7.10** Tune evaluation terms that Texel cannot price correctly.
  - [ ] **7.10.a** Inventory nonlinear, capped, thresholded and contextual terms after the accepted linear fit.
  - [ ] **7.10.b** Include only live, sufficiently frequent coordinates; likely candidates include the king-danger funnel and validated contextual scaling.
  - [ ] **7.10.c** Exclude sparse recognizer switches, exact endgame truth rules and every linear coordinate already handled by Texel.
  - [ ] **7.10.d** Wire only the selected coordinates as bounded tune options, generate configuration from current defaults and verify perturbation visibility.
  - [ ] **7.10.e** Run natural-termination SPSA and accept its clean PGO candidate only through an independent SPRT and truth/correctness gates.

### 7.11 HCE closure

- [ ] **7.11** Freeze the classical evaluator.
  - [ ] **7.11.a** Revalidate score scale, calibration, tactical suites and all endgame floors.
  - [ ] **7.11.b** Ablate new mechanisms and low-information fitted coordinates.
  - [ ] **7.11.c** Compare held-out loss, truth quality and game strength against the pre-Phase-7 head and selected HCE references.
  - [ ] **7.11.d** Archive the final surface, corpus policy, fit/tune artifacts and retry triggers.

## 8. Classical search consolidation and release

The final HCE invalidates assumptions embedded in centipawn margins and changes
the distribution consumed by pruning, histories and reductions. The release
toolchain is selected first because compiler throughput changes depth reached
at clock time controls; categorical search work and SPSA then run on the
toolchain that will actually ship.

### 8.0 Toolchain refresh and freeze

- [ ] **8.0** Update compilers and build tools to the newest validated stable versions.
  - [ ] **8.0.a** Inventory exact local, Linux CI, Windows MSYS2 and macOS AppleClang/compiler, standard-library, CMake, Ninja and profile-tool versions.
  - [ ] **8.0.b** Test current versus newest stable compiler families one change at a time; newest is a candidate, not an automatic winner.
  - [ ] **8.0.c** Require clean compile, CTest, sanitizers and identical cross-platform bench search before accepting a toolchain.
  - [ ] **8.0.d** Compare old/new release-mode and PGO throughput with pooled independent builds; retain the faster non-regressing production toolchain.
  - [ ] **8.0.e** Freeze validated major lines where exact package pins are impractical and record exact resolved versions/hashes in release manifests.
  - [ ] **8.0.f** Keep compiler-matched llvm-profdata and verify every supported architecture.

### 8.1 Finish categorical search work

- [ ] **8.1** Revisit singular-extension gate depth.
  - [ ] **8.1.a** Re-measure only on the frozen post-refit evaluator and selected release toolchain.
  - [ ] **8.1.b** Gate isolated search behavior before tuning constants.

### 8.2 Rebuild the search tuning surface

- [ ] **8.2** Audit and regenerate SPSA parameters from the final HCE head.
  - [ ] **8.2.a** Map every tunable consumer to eval scale, history scale, depth, node type and time control.
  - [ ] **8.2.b** Stage A contains live eval-coupled margins: reverse futility, razoring, futility, ProbCut, null-eval scaling, SEE pruning and aspiration as supported.
  - [ ] **8.2.c** Stage B contains coupled history/LMR coordinates and their consumers only where telemetry shows signal.
  - [ ] **8.2.d** Exclude categorical mechanism switches, mate/endgame constants, TT/hash/thread settings and clock policy from ordinary search SPSA.
  - [ ] **8.2.e** Replace stale config seeds with exact accepted defaults; verify every plus/minus perturbation at start, midpoint and end.
  - [ ] **8.2.f** Register dimensions, ranges, step sizes, schedule, game budget, seed/tail estimator and independent acceptance gates.

### 8.3 Run staged classical search SPSA

- [ ] **8.3** Tune the final-HCE search surface without adjudication.
  - [ ] **8.3.a** Calibrate the runner and use at least the current 5,000-iteration doctrine per production block unless a validated estimator changes it.
  - [ ] **8.3.b** Run and independently gate Stage A against the frozen HCE head.
  - [ ] **8.3.c** Start Stage B from the accepted Stage A head; run and independently gate it.
  - [ ] **8.3.d** Permit one narrow final polish only if residual sensitivity and budget were pre-registered.
  - [ ] **8.3.e** Bake a tail/averaged candidate chosen by the registered estimator, then require clean PGO SPRT, CTest, tactics and endgame truth.
  - [ ] **8.3.f** Preserve rejected tunes as evidence; never combine their apparent gains arithmetically.

### 8.4 Remeasure authority

- [ ] **8.4** Remeasure search/evaluation authority.
  - [ ] **8.4.a** Repeat the oracle split on the final tuned classical head.
  - [ ] **8.4.b** Use the result to prioritize post-release work, not rewrite completed evidence.

### 8.5 Complete clock and time management

- [ ] **8.5** Complete clock and time-management work.
  - [ ] **8.5.a** Diagnose remaining root-instability and time-allocation issues.
  - [ ] **8.5.b** If parameters need tuning, use a separate clock-based tune and gate; never mix them into fixed-node/search SPSA.
  - [ ] **8.5.c** Pass 1T and 4T clock gates with zero forfeits.

### 8.6 Correctness hardening

- [ ] **8.6** Complete correctness hardening.
  - [ ] **8.6.a** Run state, repetition/rule-50, TT/mate, SEE/pin and sanitizer matrices.
  - [ ] **8.6.b** Add regressions for every defect found.

### 8.7 Portability and ISA

- [ ] **8.7** Complete portability and ISA validation.
  - [ ] **8.7.a** Validate target-native execution and exact search agreement.
  - [ ] **8.7.b** Publish executable ISA and same-target performance evidence.

### 8.8 SMP validation

- [ ] **8.8** Complete SMP validation.
  - [ ] **8.8.a** Revalidate node/thread/helper-clock safety.
  - [ ] **8.8.b** Pass registered 1T/4T strength and scaling gates.

### 8.9 Classical release

- [ ] **8.9** Release the final classical line.
  - [ ] **8.9.a** Reproduce clean PGO binaries and manifests with the frozen toolchains.
  - [ ] **8.9.b** Pass cumulative 1.9.3 and external-cohort matches.
  - [ ] **8.9.c** Publish the warranted version from measured cumulative strength.

## 9. NNUE runway

- [ ] **9.0** Freeze the NNUE state and feature contract.
  - [ ] **9.0.a** Specify inputs, perspective, accumulators, serialization and refresh rules.
  - [ ] **9.0.b** Add scalar oracle and incremental-state differential tests.

- [ ] **9.1** Prepare the trainer and corpus.
  - [ ] **9.1.a** Audit D:/code/net_trainer against the frozen contract.
  - [ ] **9.1.b** Generate, validate, hash and split the teacher corpus.
  - [ ] **9.1.c** Complete trainer preflight and reproducibility manifest.

## 10. Baseline NNUE and 2.0.0

- [ ] **10.0** Train and integrate the baseline network.
  - [ ] **10.0.a** Train registered baselines and select on frozen validation data.
  - [ ] **10.0.b** Integrate inference, accumulator updates and network packaging.
  - [ ] **10.0.c** Pass scalar/incremental equality, bench and performance gates.

- [ ] **10.1** Adapt search to NNUE.
  - [ ] **10.1.a** Reprice evaluation-dependent pruning and correction mechanisms.
  - [ ] **10.1.b** Run the single reserved post-NNUE search SPSA.
  - [ ] **10.1.c** Gate 1T, LTC and 4T deployment conditions.

- [ ] **10.2** Release 2.0.0.
  - [ ] **10.2.a** Pass correctness, network provenance and fallback checks.
  - [ ] **10.2.b** Pass prior-release and external-cohort gates.

## 11. Post-NNUE frontier

- [ ] **11.0** Improve architecture and data only from measured bottlenecks.
  - [ ] **11.0.a** Evaluate larger/sparser architectures and better feature transforms.
  - [ ] **11.0.b** Refresh data only under a registered label and sampling contract.

- [ ] **11.1** Extend search selectively.
  - [ ] **11.1.a** Revisit rejected classical mechanisms only when NNUE changes their retry trigger.
  - [ ] **11.1.b** Require isolated gates and preserve Basilisk-specific design.

## 12. Scaling and platform

- [ ] **12.0** Improve parallel scaling.
  - [ ] **12.0.a** Profile split points, contention and TT traffic at 2/4/8 threads.
  - [ ] **12.0.b** Gate strength and throughput independently.

- [ ] **12.1** Expand supported platforms.
  - [ ] **12.1.a** Validate compilers, ISAs and packaging on target-native hardware.
  - [ ] **12.1.b** Keep portable fallbacks behaviorally checked.

## 13. Optional classical fallback

- [ ] **13.0** Reopen HCE only if NNUE is abandoned or a release blocker demands it.
  - [ ] **13.0.a** Require a new feature surface or new data contract; never refit the unchanged surface again.
  - [ ] **13.0.b** Register budget and acceptance before work begins.

## 14. Historical number map

Old references remain valid in EXPERIMENTS and git. Use this map rather than
rewriting historical evidence.

| Historical work | Current location |
|---|---|
| 5.0–5.3 | 5.0–5.2 |
| 5.4–5.6 | 5.3–5.5 |
| completed 5.7/5.8 evidence | 5.6 |
| 5.14 shallow cost | 5.7 |
| 5.9.1–5.9.6 | 5.8–5.9 |
| 5.9.11/5.9.15 | 5.9 |
| 5.9.12–5.9.14 | 5.10 |
| 5.9.16 | 5.11 |
| 5.9.7/5.9.17–5.9.21 | 5.12–5.14 |
| open 5.9.22–5.9.39 | 6.1–6.8 |
| open 5.7.5 | 8.1 |
| open 5.8.7 | 8.5 |
| old 5.10–5.13 | 8.6–8.9 |
| old Phases 6–10 | new Phases 9–13 |
