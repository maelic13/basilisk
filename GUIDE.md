# Basilisk development guide

## How to work with the engine agent

PLAN owns current/future work; EXPERIMENTS owns measured history and frozen
predictions; linked `analysis/` packets hold research and implementation
handoffs. Run the checklist in order.

1. Ask **"what measured defect are we fixing?"** before asking what feature to
   add. Keep unresolved chess or architecture reasoning in `RESEARCH`.
2. Promote to `READY_FOR_IMPLEMENTATION` only when the mechanism and exact
   semantics are explicit, local evidence supports it, interactions and
   invariants are mapped, the cheapest falsifier is known, and an acceptance/
   rejection rule exists.
3. Let the implementation agent act like a colleague on ordinary structure,
   builds, tests, debugging and cheap qualification. Do not let it silently
   redesign the hypothesis, broaden the mechanism or tune adjacent behavior.
4. Run the cheapest discriminating falsifier before expensive coding or games.
   The agent prepares expensive jobs; the maintainer starts them.
5. Freeze the prediction and confidence **before** seeing results. Compare the
   postmortem with that frozen record; a convincing retrospective explanation
   is not evidence that the result was predicted.
6. A negative result is useful when it removes a hypothesis. Do not chase a
   donor feature, manufacture implementation work, or retry a rejection until
   its objective trigger fires.
7. If progress stalls, return to evidence -> competing hypotheses -> cheapest
   discriminating experiment. If implementation finds a false premise, return
   to `RESEARCH` rather than rescuing the idea with neighboring changes.

State legend:

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

Research owns the causal decision and prospective prediction. Implementation
owns normal local engineering inside the registered handoff. Local qualification
owns cheap correctness/determinism/performance evidence. The maintainer owns
machine-saturating gates. Playing changes normally close only after their
registered game verdict; documentation and behavior-neutral work may use a
shorter path.

### Current model mapping

Edit this table when model generations change; canonical PLAN tags stay stable.

| Class | Current routing |
|---|---|
| `R3` | strongest available frontier research model |
| `R2` | strong frontier reasoning model |
| `I2` | strongest implementation model |
| `I1` | efficient implementation model |
| `M` | inexpensive implementation/mechanical model |
| `V` | model appropriate to the qualification task |

Capability tags are advisory routing, not state, evidence or permission.
Completed historical vendor tags are intentionally left as history.

### Reusable research prompt

> Research `<PLAN leaf>` without substantial engine implementation. Read PLAN,
> EXPERIMENTS, its linked analysis and relevant source first; measured evidence
> outranks roadmap assumptions. Search prior negative results and retry triggers.
> State the precise question, leading and competing hypotheses, interactions and
> duplicated signals; distinguish search, evaluation, tool and instrument
> explanations. Design the cheapest discriminating experiment first. Before
> exposure, freeze the expected diagnostic movement, defensible Elo sign/range,
> probability of usefulness, confidence, most likely failure mode, falsifiers
> and stopping rule. End with exactly one decision:
> `READY_FOR_IMPLEMENTATION`, `MORE_RESEARCH`, or `NO_CHANGE`.

### Reusable implementation prompt

> Implement `<PLAN leaf>` according to its registered implementation handoff.
> Treat the research decision, intended semantics, invariants and experiment
> design as fixed. Use normal engineering judgment for structure, focused
> builds, debugging and cheap qualification. Do not broaden the mechanism, tune
> unrelated behavior or continue unrelated roadmap work. If a research premise
> proves false, stop the mechanism, document the contradiction, preserve useful
> instrumentation and return the leaf to `RESEARCH`. Prepare but do not start
> maintainer-owned expensive jobs. When locally qualified, update PLAN/GUIDE and
> EXPERIMENTS according to their ownership, then report changes, interactions,
> validation, remaining gate and false assumptions.

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

- [x] **6.0** Establish the truth baseline before another evaluator edit
  - [x] **6.0.a** Freeze 770 Syzygy-verified positions across 21 endgame families
  - [x] **6.0.b** Measure accepted Basilisk and Stockfish on all 770 positions at identical nodes
  - [x] **6.0.c** Freeze attained 60k reference results—not theoretical ceilings—and paired-union stretch evidence
  - [x] **6.0.d** Pair by position; report at 2 SE and block family regressions at 3 SE
  - [x] **6.0.e** Veto new truth/rule-50 failures and every engine anomaly
  - [x] **6.0.f** Census all Arm C <=6-man labels against Syzygy; freeze the 14.12% disagreement baseline
  - [x] **6.0.g** Measure exact reference-family occurrence inside search trees
- [x] **6.1** Implement and tune the missing KBNK technique (historical step 5.9.22)
  - [x] **6.1.a** Map Rarog's bishop-colour diagonal potential onto Basilisk without a bishop-position term
  - [x] **6.1.b** Make the already-equivalent diagonal geometry explicit; defer scale and competing pulls
  - [x] **6.1.c** Bound the strongest truth-safe KBNK diagonal/king vector
  - [x] **6.1.d** Do not retry bishop proximity or escape-square count unless new evidence overturns their earlier failure
  - [x] **6.1.e** Confirm on held-out positions 61-198, then report all 198 positions
  - [x] **6.1.f** Require endgame/tactical gates; bench identity is necessary, not behavioral proof
- [x] **6.2** Gate KBNK and accepted mate-drive changes
  - [x] **6.2.a** Run a fresh no-adjudication [0,3] nElo SPRT against the accepted head
  - [x] **6.2.b** Treat the old approximately 5,860-game adjudicated Group A run as preliminary only
  - [x] **6.2.c** Never resume that run if the KBNK candidate or match policy changes
- [x] **6.3** Add general king-to-passed-pawn approach logic
  - [x] **6.3.a** Derive the feature from Basilisk truth failures, not reference constants
  - [x] **6.3.b** Verify KP-K, KPP-K, KBP-K and mixed rook/minor pawn families
  - [x] **6.3.c** Gate the isolated candidate with no adjudication
- [x] **6.4** Audit every endgame term before broadening the evaluator
  - [x] **6.4.a** Test score resolution, saturation and interaction at Basilisk's scale
  - [x] **6.4.b** Keep theory truth, move quality, conversion and game strength separate
  - [x] **6.4.c** Freeze the accepted Group A head and truth report (also: static_assert the shipped KBNK default)
- [x] **6.5** Finish the prepared rook-ending scale candidate
  - [x] **6.5.a** `[Sol/H]` Accept KRPKR/KRPPKRP scaling: +3.29 +/- 4.61 Elo, 6,332 games, H1
- [ ] **6.6** Upgrade instrument and gate integrity
  - [ ] **6.6.a** `[R2]` Audit Basilisk tools against Rarog 4.10
  - [ ] **6.6.b** `[I2]` Add versioned schemas, cohort digests and serial-identical sharding
  - [ ] **6.6.c** `[V]` Prove every guard fails on known-bad inputs; stamp layer/budget/cohort
  - [ ] **6.6.d** `[R2]` Measure deployed nodes/move and derive Basilisk's budget bracket
  - [ ] **6.6.e** `[I2]` Generalize FEN-hash holdout, runner-up, McNemar and spent-cohort rules
  - [ ] **6.6.f** `[I2]` Refuse dirty/wrong-revision/config-mismatched gates and ignored options
  - [ ] **6.6.g** `[I2]` Add CMake feature/ISA combination coverage
  - [ ] **6.6.h** `[V]` Re-run invalidated baselines and refreeze Group A
- [ ] **6.7** Rank and classify remaining endgames from Basilisk evidence
  - [ ] **6.7.a** `[R3]` Recompute board/tree occurrence and root concentration
  - [ ] **6.7.b** `[R3]` Classify recognizer, scale, move-quality and conversion work
  - [ ] **6.7.c** `[R3]` Record every dispatcher's promotion/material-shed closure
  - [ ] **6.7.d** `[R2]` Re-rank by local defect, occurrence and deployment budget
- [ ] **6.8** Resolve, then implement demonstrated Group B work
  - [ ] **6.8.a** `[R3]` Resolve the BAS-E53 won-rook only-move defect
  - [ ] **6.8.b** `[R2]` Cover KR-KP, KQ-KRP and KR-KB by their measured kind
  - [ ] **6.8.c** `[R3]` Cover bishop-pawn fortress and promotion families
  - [ ] **6.8.d** `[I2]` Add deterministic theory/truth/closure cases before fitting
  - [ ] **6.8.e** `[R3]` Fit the dependency-complete local family cluster
- [ ] **6.9** Qualify Group B
  - [ ] **6.9.a** `[R2]` Register occurrence-tiered whole-match/cohort/no-regression gates
  - [ ] **6.9.b** `[V]` Require truth improvement, no veto and the registered PGO verdict
- [ ] **6.10** Evaluate the lower-yield remainder
  - [ ] **6.10.a** `[R2]` Measure KPs-K and KP-KP first
  - [ ] **6.10.b** `[R3]` Reconcile KQ-KP and KQ-KR across deployment budgets
  - [ ] **6.10.c** `[R3]` Audit KR-KN and KNN-KP/KNN-K theory and rule 50
  - [ ] **6.10.d** `[I2]` Implement only measured, occurrence-supported mechanisms
  - [ ] **6.10.e** `[V]` Gate the dependency-complete remainder and apply the stop rule
- [ ] **6.11** Close classical endgame maturity
  - [ ] **6.11.a** `[M]` Freeze accepted cohorts, reports, budgets and floors
  - [ ] **6.11.b** `[R2]` Record rejects, gaps and retry triggers
  - [ ] **6.11.c** `[V]` Reconcile every family layer and hard veto
  - [ ] **6.11.d** `[M]` Synchronize the accepted head before later phases

## Phase 7 — Board CORRECTNESS (defect hunt, not a speed phase)

Basilisk already leads on board throughput (BAS-X16). 7.3-7.8 are conditional on
7.2 and "profiled, not hot, no change" closes them successfully.

- [ ] **7.0** `[I1]` Refresh and freeze the validated classical toolchain
- [ ] **7.1** `[R2]` Port board-v2 tests and audit board/parser/SEE contracts
- [ ] **7.2** `[R3]` Profile board work inside actual HCE search
- [ ] **7.3** `[R3]` Optimize legal generation/list delivery only if hot
- [ ] **7.4** `[I1]` Measure fused piece relocation only if hot
- [ ] **7.5** `[R3]` Share pin/check information only with a valid lifetime contract
- [ ] **7.6** `[R2]` Optimize SEE only after independent contract parity
- [ ] **7.7** `[I1]` Make history capacity and mutation contracts explicit
- [ ] **7.8** `[R3]` Decide whether a larger board representation change pays
- [ ] **7.9** `[R3]` Audit draw/repetition/null policy separately from speed
- [ ] **7.10** `[I1]` Qualify the integrated board candidate
- [ ] **7.11** `[R2]` Gate deliberate playing changes
- [ ] **7.12** `[R2]` Refresh affected endgame evidence and close

## Phase 8 — Mature HCE refit

- [ ] **8.0** Define and close the final HCE surface
  - [ ] **8.0.a** `[R3]` Compare pinned HCE references by contracts and interactions
  - [ ] **8.0.b** `[R3]` Audit the complete HCE feature and score surface
  - [ ] **8.0.c** `[R3]` Trace activation, caches and search consumers
  - [ ] **8.0.d** `[R2]` Classify repairs, candidates, fit issues and no-change findings
  - [ ] **8.0.e** `[I1]` Add categorical tests and freeze architecture
- [ ] **8.1** Harden and document the complete fit pipeline
  - [ ] **8.1.a** `[I1]` Fit and freeze K across stages/arms
  - [ ] **8.1.b** `[I1]` Require complete explicit initial vectors
  - [ ] **8.1.c** `[I1]` Freeze by-game splits and atomically claim test once
  - [ ] **8.1.d** `[R2]` Partition every coordinate free/fixed/excluded
  - [ ] **8.1.e** `[M]` Hash all inputs/outputs and restore source/binary
  - [ ] **8.1.f** `[I1]` Enforce label domain and rejection accounting
  - [ ] **8.1.g** `[I1]` Version contracts and port useful Rarog fit tools
  - [ ] **8.1.h** `[R2]` Write the Basilisk Texel handbook
- [ ] **8.2** Design a phase-efficient natural-termination corpus
  - [ ] **8.2.a** `[I1]` Locate/profile/hash the source store
  - [ ] **8.2.b** `[R3]` Measure row yield by material-phase start bucket
  - [ ] **8.2.c** `[I1]` Freeze extraction/dedup/split semantics
  - [ ] **8.2.d** `[R2]` Size by effective rows and learning curve
  - [ ] **8.2.e** `[M]` Register book/corpus quality and budget
- [ ] **8.3** Generate and publish self-play corpus A
  - [ ] **8.3.a** `[I1]` Prepare the exact long-run generator and command
  - [ ] **8.3.b** `[R2]` Audit returned composition, lineage and manifest
  - [ ] **8.3.c** `[M]` Publish immutable corpus A
- [ ] **8.4** Build matched self-play versus Syzygy-row label arms
  - [ ] **8.4.a** `[M]` Preserve corpus A labels
  - [ ] **8.4.b** `[I1]` Build row-identical Syzygy corpus B
  - [ ] **8.4.c** `[R2]` Treat cursed outcomes and halfmove clocks correctly
  - [ ] **8.4.d** `[I1]` Prove row/order/split identity
  - [ ] **8.4.e** `[R2]` Bound row-local relabel eligibility
  - [ ] **8.4.f** `[M]` Publish changed-label matrix and provenance
- [ ] **8.5** Analyze whole-game TB adjudication separately
  - [ ] **8.5.a** `[R3]` Audit game-to-row causal semantics
  - [ ] **8.5.b** `[I1]` Pilot matched corpus C only if isolatable
  - [ ] **8.5.c** `[R2]` Register C separately; never use TB in strength gates
  - [ ] **8.5.d** `[R2]` Close no-arm if sampling cannot be isolated
- [ ] **8.6** Measure optimizer initialization dependence
  - [ ] **8.6.a** `[I1]` Fit accepted and neutral starts identically
  - [ ] **8.6.b** `[R2]` Compare convergence, covariance and held-out loss
  - [ ] **8.6.c** `[M]` Register the production initialization
- [ ] **8.7** Complete matched whole-surface fits
  - [ ] **8.7.a** `[I1]` Hold surface/K/budget/initialization equal
  - [ ] **8.7.b** `[R2]` Alternate nonlinear and complete-linear stages
  - [ ] **8.7.c** `[I1]` Verify reconstruction, activation and convergence
  - [ ] **8.7.d** `[M]` Produce independent vectors/manifests
  - [ ] **8.7.e** `[I1]` Reject source/test/surface drift
- [ ] **8.8** Gate the label contract
  - [ ] **8.8.a** `[R2]` Register matched candidates and gates
  - [ ] **8.8.b** `[I1]` Run clean-PGO natural-termination gates
  - [ ] **8.8.c** `[R2]` Explain with loss/truth; decide with SPRT
  - [ ] **8.8.d** `[R2]` Accept by the prospective rule only
- [ ] **8.9** Run iterative whole-surface Texel cycles
  - [ ] **8.9.a** `[R2]` Register mandatory cycle 1 and loop cap/stop
  - [ ] **8.9.b** `[I1]` Generate each accepted-head corpus
  - [ ] **8.9.c** `[I1]` Refit complete surface with fresh test
  - [ ] **8.9.d** `[R2]` Gate each cycle; stop at first non-acceptance
  - [ ] **8.9.e** `[R2]` Publish cycle/residual closure
- [ ] **8.10** Conditionally tune nonlinear HCE residue
  - [ ] **8.10.a** `[R3]` Measure activation, interaction and curvature
  - [ ] **8.10.b** `[R2]` Select only live non-Texel coordinates
  - [ ] **8.10.c** `[I1]` Wire/verify or close SPSA no-change
  - [ ] **8.10.d** `[R2]` Register pilot and immutable full tune if justified
  - [ ] **8.10.e** `[I1]` Run, bake and independently gate
- [ ] **8.11** Freeze the classical evaluator
  - [ ] **8.11.a** `[I1]` Revalidate scale, tactics, mate and endgame floors
  - [ ] **8.11.b** `[R2]` Ablate new/low-information mechanisms
  - [ ] **8.11.c** `[R3]` Reconcile loss, truth and strength
  - [ ] **8.11.d** `[M]` Archive surface, data, cycles and retry triggers
- [ ] **8.12** Optimize frozen evaluation cost bit-exactly
  - [ ] **8.12.a** `[R3]` Profile terms/phases/cache paths
  - [ ] **8.12.b** `[I2]` Optimize only measured hot paths bit-exactly
  - [ ] **8.12.c** `[R2]` Audit lazy thresholds separately
  - [ ] **8.12.d** `[I1]` Gate pooled NPS and natural-termination strength

## Phase 9 — Classical search and release

- [ ] **9.0** Audit complete search composition and authority
  - [ ] **9.0.a** `[R3]` Inventory search, picker, histories, qsearch, pruning/reduction/extension and score contracts
  - [ ] **9.0.b** `[R3]` Test interaction/cancellation hypotheses with bounded factorial screens
  - [ ] **9.0.c** `[R2]` Rerun oracle/counter/depth profiles and derive candidates or no-change
- [ ] **9.1** Revisit categorical search work
  - [ ] **9.1.a** `[R3]` Measure extension authority at fixed depth/nodes/equal cost
  - [ ] **9.1.b** `[R2]` Implement and gate only an isolated dependency-complete candidate
- [ ] **9.2** Audit and conditionally fit SEE/move-order value scale
  - [ ] **9.2.a** `[R2]` Count real SEE/order/delta decisions changed by final HCE scale
  - [ ] **9.2.b** `[I2]` Add exact-default value injection only if justified
  - [ ] **9.2.c** `[R2]` Fit/gate separately and preserve normalized benchmarking
- [ ] **9.3** `[R3]` Audit TT, caches, hashes and hot memory
- [ ] **9.4** `[R3]` Audit threading, UCI lifecycle and tablebases
- [ ] **9.5** `[R2]` Audit diagnostics, harnesses and build delivery
- [ ] **9.6** Run optional post-HCE search SPSA only on a displaced optimum
  - [ ] **9.6.a** `[R3]` Select live interacting non-clock search coordinates
  - [ ] **9.6.b** `[I1]` Rebuild seeds/wires and register immutable tune metadata
  - [ ] **9.6.c** `[R2]` Pilot only if useful; skip flat/monotone/low-value surfaces
  - [ ] **9.6.d** `[I1]` Complete, bake and independently gate justified stages
  - [ ] **9.6.e** `[R2]` Preserve rejects; forbid post-selected vectors
- [ ] **9.7** Complete time management separately
  - [ ] **9.7.a** `[R3]` Audit clock/root/worker interactions
  - [ ] **9.7.b** `[R2]` Size overhead/forfeit experiments prospectively
  - [ ] **9.7.c** `[R3]` Resolve completed-root confidence consumers
  - [ ] **9.7.d** `[I1]` Tune/gate clock policy separately and require zero forfeits
- [ ] **9.8** Correctness, cleanup and checkpoint
  - [ ] **9.8.a** `[R2]` Close audit ownership and dead/dormant mechanisms
  - [ ] **9.8.b** `[I1]` Run full correctness/sanitizer/config matrices
  - [ ] **9.8.c** `[I1]` Freeze benchmark/NPS/node/game checkpoint
- [ ] **9.9** `[R2]` Run final classical attribution, authority and cumulative gates
- [ ] **9.10** Portability, SMP and classical release
  - [ ] **9.10.a** `[I1]` Validate target-native builds, ISA and reproducible PGO
  - [ ] **9.10.b** `[R3]` Revalidate SMP contention, time-to-depth and strength
  - [ ] **9.10.c** `[I1]` Pass prior-release STC/LTC/4T and external gates
  - [ ] **9.10.d** `[M]` Publish the warranted release and manifests
- [ ] **9.11** Investigate universal CPU-dispatched binaries
  - [ ] **9.11.a** `[R3]` Specify safe dispatch and build ownership
  - [ ] **9.11.b** `[I2]` Build one isolated prototype
  - [ ] **9.11.c** `[I1]` Verify tiers, identity and target-native cost
  - [ ] **9.11.d** `[R2]` Adopt/defer/reject and repeat affected release gates

## Phase 10 — NNUE runway

- [ ] **10.0** Hand off the final classical measurement/data contract
  - [ ] **10.0.a** `[M]` Freeze data, score, board and reference manifests
- [ ] **10.1** Add factual per-ply dirty-piece deltas
  - [ ] **10.1.a** `[R3]` Define every move/null/refresh transition
  - [ ] **10.1.b** `[I2]` Implement, differentially verify and price the seam
- [ ] **10.2** Add evaluator-owned accumulator scaffolding
  - [ ] **10.2.a** `[R2]` Define storage, validity, refresh, clone and unwind
  - [ ] **10.2.b** `[I2]` Verify full-refresh parity and HCE overhead
- [ ] **10.3** Prepare trainer/corpus path
  - [ ] **10.3.a** `[R3]` Audit trainer, toolchain, formats, splits and resume
  - [ ] **10.3.b** `[I1]` Run pilot train/reload and freeze manifests
- [ ] **10.4** Close the runway gate
  - [ ] **10.4.a** `[I1]` Pass transition, oracle, build and performance gates

## Phase 11 — Baseline NNUE and 2.0.0

- [ ] **11.0** Harden trainer and controlled data
  - [ ] **11.0.a** `[I1]` Enforce deterministic CLI/splits/hashes/checkpoints
  - [ ] **11.0.b** `[R2]` Size and publish data by learning curve
- [ ] **11.1** Train registered baseline networks
  - [ ] **11.1.a** `[R3]` Compare one axis and multiple seeds on frozen data
- [ ] **11.2** Integrate scalar inference and packaging
  - [ ] **11.2.a** `[I2]` Require integer-exact trainer/engine conformance
- [ ] **11.3** Integrate incremental and SIMD inference
  - [ ] **11.3.a** `[R2]` Prove actual-network incremental/full parity
  - [ ] **11.3.b** `[I1]` Qualify SIMD/scalar and bounds per target
  - [ ] **11.3.c** `[R2]` Attribute update/refresh/inference costs
- [ ] **11.4** Adapt search to NNUE
  - [ ] **11.4.a** `[R3]` Re-audit score authority and eval-coupled margins
  - [ ] **11.4.b** `[R2]` Run one justified post-NNUE search SPSA
  - [ ] **11.4.c** `[I1]` Pass deployment strength/correctness/time gates
- [ ] **11.5** Release 2.0.0
  - [ ] **11.5.a** `[I1]` Reproduce network, binaries and fallbacks
  - [ ] **11.5.b** `[R2]` Beat classical/prior baselines at STC/LTC/4T

## Phase 12 — Post-NNUE frontier

- [ ] **12.0** Improve data/architecture only from residuals
  - [ ] **12.0.a** `[R3]` Analyze residuals by phase/material/king/family
  - [ ] **12.0.b** `[R3]` Test scale, hard mining and architectures one axis at a time
  - [ ] **12.0.c** `[I1]` Refresh only under a registered data hypothesis
- [ ] **12.1** Extend search selectively
  - [ ] **12.1.a** `[R3]` Reopen only mechanisms whose retry trigger fired
  - [ ] **12.1.b** `[R2]` Require isolated clean-PGO gates

## Phase 13 — Scaling and platform

- [ ] **13.0** Improve high-thread/NUMA scaling
  - [ ] **13.0.a** `[R3]` Profile diversity, useful TT traffic, contention and bandwidth
  - [ ] **13.0.b** `[R2]` Optimize measured causes; gate depth and strength
- [ ] **13.1** Expand platforms and delivery
  - [ ] **13.1.a** `[I1]` Validate compilers/ISAs/dispatch on native hardware
  - [ ] **13.1.b** `[I2]` Preserve tested portable fallbacks

## Phase 14 — Optional HCE fallback

- [ ] **14.0** Reopen HCE only if NNUE is abandoned or blocked
  - [ ] **14.0.a** `[R3]` Require a new structural residual or data contract
  - [ ] **14.0.b** `[R2]` Register mechanism, fit, budget and acceptance
