# Basilisk development plan

This is the maintainer-facing source of truth. `GUIDE.md` is its short
operational mirror; `README.md` and `CHANGELOG.md` remain user-facing and must
not contain experiment bookkeeping.

## 1. Current state

| Item | State |
|---|---|
| Branches | `master`, `development` and `v1.9.3` are at `d737123`. `origin/nnue` contains an old partial implementation. `origin/arm_fix` is the one-commit `67a987b` TT-alignment experiment based on 1.9.3; Phase 5.8 inventories it, but it must not be merged as a finished fix. |
| Released baseline | **Basilisk 1.9.3**, bench-13 fingerprint **11,941,440**. It is search-identical to 1.9.2; 1.9.3 repaired Clang/`llvm-profdata` PGO tool matching. |
| Evaluation | The accepted HCE remains the comparison/fallback evaluator, but all new HCE strength work is frozen. Phase 10 is the optional HCE fallback and is entered only if NNUE is abandoned. |
| Active work | The shared 36,400-game Rating Tournament is running. No Basilisk candidate or tuner is active. |
| Next strength release | **1.10.0 at Phase 5.11**, after the complete pre-NNUE search, portability and direct target ladder. |
| NNUE release | **2.0.0 at Phase 7.7**, using `D:/code/net_trainer`. |

### Live rating evidence — provisional 2026-08-05

At 8,626/36,400 games (~1,232 per engine), the pool reads:

| Engine | Rating | Gap from Basilisk 1.9.3 |
|---|---:|---:|
| Houdini 1.5a | 3217 | +211 |
| Critter 1.6a | 3190 | +184 |
| Rybka 6 | 3178 | +172 |
| Rybka 5 | 3156 | +150 |
| Rybka 4 | 3102 | +96 |
| Rybka 4.1 | 3088 | +82 |
| **Basilisk 1.9.3** | **3006** | — |
| Rarog 2.4.0-dev | 2967 | −39 |
| Rarog 2.3.1 | 2956 | −50 |
| Rybka 3 | 2928 | −78 |

These are mutually fitted live ratings, not independent confidence intervals.
Houdini 2.0c and Fritz 16 are absent and must be added to the Phase-5 cohort.
Closing 150–210 Elo with search alone cannot be promised. Phase 5 attacks
defects whose effects compound; if the direct target gate fails, 1.10.0 does
not ship.

## 2. Development process

### Responsibilities and commits

```text
Model  -> inspect, implement, locally verify, update PLAN + GUIDE, commit.
User   -> run long SPSA/SPRT/gauntlet/datagen jobs and return final artifacts.
Model  -> accept/revert from the registered verdict, update docs, commit.
```

- Commit after every completed plan step. Use an imperative subject and useful
  experiment body, no co-author trailers, and never push unless asked.
- Keep `PLAN.md` and `GUIDE.md` synchronized in the same commit.
- Preserve unrelated user changes. Dirty test artifacts record their diff hash
  and cannot become release baselines.
- While the shared Rating Tournament/Rarog SPSA occupies the machine: no
  bench/NPS/PGO/SPRT/datagen or competing game work.

### Required gates

| Change | Required evidence |
|---|---|
| Behaviour-neutral refactor/test/tooling | CTest **12/12**, relevant Python tests, sanitizers when memory/concurrency is touched, exact bench fingerprint |
| Correctness repair changing play | Deterministic regression, tactical/mate/endgame suites, then strength gate unless unreachable in legal play |
| Strength mechanism | Registered `[0,3]` nElo SPRT at `3+0.03`; broad/risky bundles use `[-3,3]` and need a positive cumulative result |
| Non-inferiority/simplification | `[-3,0]`; H1 supports non-regression |
| Speed-only | Exact bench identity plus interleaved pooled-PGO NPS A/B after an identical-binary self-pair |
| Time/root/SMP | 1T STC, 1T `10+0.1`, 4T `10+0.1`, zero forfeits, recorded topology/hash |
| Harness change | 30k identical-binary calibration at 1T; 10k at 4T where applicable; full 95% nElo CI inside ±5 |
| Phase boundary | Clean reproducible PGO artifacts, correctness matrix, cumulative prior-release match and external cohort |

Use `tools/books/UHO_Lichess_4852_v1.epd` as paired openings. Record
engine/source SHA, compiler/PGO manifest, binary and book hashes, TC, threads,
hash, concurrency, affinity and adjudication. SPRT decides strength; node
counts, static loss, WAC and telemetry explain it.

### SPSA budget

1. **One pre-NNUE search SPSA:** Phase 5.10, after architecture freezes.
   Diagnostics select at most ~24 non-redundant coordinates.
2. **One post-NNUE search SPSA:** Phase 8.3, after the retained NNUE
   architecture/score scale freezes.
3. A further run needs explicit evidence that the prior run could not identify
   the needed parameter class. No HCE coordinate enters either run.
4. Discrete mechanisms use A/B switches or small registered grids. Potentially
   de-tuned mechanisms may land inert and be adjudicated in the joint fit with
   post-fit ablations.
5. SPSA proposes; clean PGO SPRT accepts. Estimator, horizon, bounds and stop
   rule are registered before launch—no post-hoc tail choice.

## 3. Durable lessons

1. External transfer beats self-play arithmetic; never add accepted-arm Elo
   and call it a rating.
2. Mechanism and consumer constants are a system. The correct standalone
   pre-move-check LMR repair lost −21.55 ±9.83 because the old surface was
   tuned around the defect; repair and fit it jointly.
3. Canaries are mandatory but not strength oracles. KBNK/KQK, mate-distance,
   WAC and rule-50 tests catch semantics; games decide Elo.
4. Bench identity is behaviour identity, not speed identity. Pool/interleave
   independent PGO builds.
5. A smaller tree can be worse. Measure best-move recall and contradiction,
   not just node savings.
6. Static eval, stand pat, qsearch moves, ProbCut, null cutoffs, reduced search
   and full search require different provenance.
7. Do not tune before architecture freezes; a tune can hide a defect and make
   its repair look negative.
8. Multi-thread strength is a separate deployment condition.
9. HCE is frozen, not deleted: it remains a debug oracle, teacher and fallback.
10. Git/CHANGELOG preserve experiment history; the forward GUIDE stays short.
11. A cross-compiled binary is not a validated asset. Compatibility requires
    target-native execution, exact search agreement, an executable ISA
    contract and same-target performance evidence.

## 4. Released phases

These sequential phase numbers replace the old roadmap. Old numbers remain
historical references in git and `CHANGELOG.md` only.

### Phase 1 — Foundations and first strength line — CLOSED (1.0.0–1.8.0)

Built legal board state, move generation, UCI, PVS/qsearch, TT, histories, SEE,
Syzygy, time management, Lazy SMP, reproducible testing and the accepted HCE.
Later HCE self-play cycles stopped transferring, so the evaluator is frozen.

### Phase 2 — Correctness and search architecture — CLOSED (1.9.0)

Unified board/search state; repaired repetition/rule-50, TT, SEE/pin and mate
semantics; added staged ordering, correction/history infrastructure,
root-instability TM and dense 10-byte TT entries.

### Phase 3 — Hardening, CI and PGO speed — CLOSED (1.9.1)

Centralized parameters, strengthened invariants/fuzzing and cross-platform CI,
validated shipped assets, added telemetry and accepted **+4.34% NPS**
(approximately +8.69 ±6.63 Elo) with identical search.

### Phase 4 — SMP durability and release tooling — CLOSED (1.9.2/1.9.3)

Repaired SPSA/MT harnesses, thread/node/helper-clock safety and data tooling;
removed unproven helper-history blending. The accepted 4T bundle measured
+30.42 ±8.77 Elo with zero forfeits. 1.9.3 fixed PGO tool matching without
changing bench 11,941,440.

## 5. Phase 5 — Evidence-coherent pre-NNUE search (→ 1.10.0)

### Objective and source references

Build the strongest evaluator-agnostic search possible, then directly beat
every installed Rybka, Critter 1.6a, Houdini 2.0c and Fritz 16. No HCE feature,
weight or Texel work is permitted. Stockfish/Reckless are design references,
never code or constant donors:

- [Stockfish search.cpp](https://github.com/official-stockfish/Stockfish/blob/762dd1da9a5db458180b2c5db6c53dc40ec61e1a/src/search.cpp)
- [Reckless search.rs](https://github.com/codedeliveryservice/Reckless/blob/d6603046e76d66edd43622ded23458da1af50c68/src/search.rs)
- Stockfish [stand-pat TT repair](https://github.com/official-stockfish/Stockfish/commit/bb4eb04a), [PV-IIR repair](https://github.com/official-stockfish/Stockfish/commit/e20ef7ed), [TT mismatch penalty](https://github.com/official-stockfish/Stockfish/commit/319d61ef)
- Historical [null-move/TT provenance](https://talkchess.com/viewtopic.php?t=33679) and [`lmrDepth`](https://talkchess.com/viewtopic.php?t=63521) discussions

Forum material supplies hypotheses only; Basilisk measurements decide.

### Current audit and missing symbiosis

| Area | Basilisk today | Repair |
|---|---|---|
| TT evidence | 10-byte entry has score/raw eval/move/depth/bound/six-bit age, no result provenance. | Prototype compact provenance without losing three entries per 32-byte cluster; publish consumer capabilities. |
| Qsearch → main | No-TT stand-pat beta cutoff is stored depth-0 lower; any normal TT bound can refine static eval for RFP through depth 9. | Separate raw/corrected/stand-pat/searched evidence; stop laundering stand pat. |
| ProbCut → singular | Stores `pc_beta`, not searched `val`, at `depth-3`; singular accepts lower/exact at `depth-3`. | Store actual speculative score; forbid it from singular authority. |
| NMP | Verification disables null only at its root; descendants re-enable it. Missing subtree suppression, cut-node/potential-singularity and raw-eval/non-decisive contracts. | Add subtree suppression and evidence/node-role guards before tuning. |
| IIR | Safer than Rarog because it is non-PV, but begins at depth 4 and treats stale/missing TT guidance uniformly. | Audit all/cut roles and expose IIR debt downstream. |
| LMR/selectivity | Base `lmr_depth` feeds some pruning, but not the history/context refinements of real LMR. Checking-move exemption reads invalid post-move lazy state. | One pre-move `MoveEvidence`, history-aware prospective depth, joint repair/refit. |
| Qsearch evasions | In-check qsearch uses raw legal order and stores no searched evasion evidence. | TT/evasion staging with complete legality and mate safety. |
| Correction | Pawn/minor/non-pawn/one-step continuation are averaged `/5`; captures can train positional correction. | Attribution guards, per-source confidence, true compact 2/4-ply continuation correction. |
| Root | Per-move mean/variance/PV/nodes are collected but unused; aspiration/TM use separate coarser signals. | One completed-iteration confidence model for aspiration/TM/fallback/SMP. |
| SMP result | Shared table/final merge prefer deepest helper evidence; losses are not symmetric with wins and incomplete work is untyped. | Completed-root ownership, symmetric decisive scores, legal fallback; pool instability informs time only. |
| TT budget | Power-of-two indexing can waste almost half of requested non-power-of-two Hash. | Measure multiply-high/full-budget indexing after semantic TT work. |

### Cross-feature invariants

1. Every result carries an evidence kind: full, verified reduced, qsearch move,
   stand pat, ProbCut, null or incomplete.
2. Every consumer declares accepted evidence, bound, depth and node role.
3. LMP/futility/SEE/LMR share one prospective depth and monotone thresholds.
4. History/correction learn only from completed attributable searches.
5. Aspiration/TM/fallback/SMP share one completed root snapshot.
6. Every joint-fit mechanism remains independently ablatable.

### 5.0 — Freeze baseline and close live observation

Finish/archive the 36,400-game tournament unchanged. Reproduce clean 1.9.3:
CTest 12/12, PGO manifest and bench 11,941,440; capture fixed-position 1T/4T
diagnostics only once the machine is idle. Pool ratings establish gaps, not
mechanism Elo.

### 5.1 — Diagnostic substrate and interaction map

Add deterministic sampled traces for TT producer/consumer and contradiction;
stand-pat/qmove; NMP nested verification and raw/corrected/TT eval; ProbCut
storage/reuse/singularity; IIR/extension debt; move stage/prospective depth/
prune reasons/best-move recall; correction capture/collision/saturation; and
root variance/gap/effort/fails/fallback/worker instability. Add shadow
predicates for 5.2–5.7. Diagnostics off preserves fingerprint; on preserves
nodes/best moves with bounded overhead.

### 5.2 — Result evidence and TT contract

Introduce transient `OutcomeKind`, `NodeEvidence` and `MoveEvidence`. Compare
compact persisted provenance using one age bit with stack-only provenance;
widen only if both fail. Audit age wrap, replacement, torn reads, mate/rule-50
conversion and every reader. Define which evidence may cut off, refine eval,
suppress IIR, seed NMP or authorize singularity. Shadow-test a small confidence/
depth penalty for inexact bounds contradicting the current window, including
shared-TT races. Correctness/bench first, then `[0,3]` if behaviour changes.

### 5.3 — Qsearch and ProbCut evidence hygiene

Do not manufacture a searched lower bound from no-TT stand pat. Keep depth-0
stand-pat estimates out of deep main-search pruning. Store actual ProbCut
results with speculative provenance and measured depth; never authorize
singularity or exact learning. Stage in-check qsearch with TT/good/bad evasion
ordering and complete legality. Test qsearch capture/SEE history plus coherent
delta/SEE/futility only after storage semantics are correct. Gate useful arms
`[0,3]`, combined `[-3,3]`.

### 5.4 — NMP, IIR and singular cooperation

Add subtree null suppression, raw-eval/non-decisive/material and cut-node
guards, potential-singularity protection and zugzwang coverage. Compare raw
versus TT-adjusted null windows in shadow mode; nested verification nulls are
forbidden unless separately proven. Restrict IIR by measured node/TT quality
and propagate debt. Singular requires compatible full-search evidence,
separate single/double rules and extension-debt caps. Test isolated `[0,3]`
arms then a `[-3,3]` joint composition.

### 5.5 — Unified prospective-depth selectivity

Create pre-move `MoveEvidence` containing correct check state, move class,
node/TT evidence, SEE, histories, correction confidence and extension/IIR debt.
Derive LMP, history pruning, futility, quiet/capture SEE and LMR from one
history-aware prospective depth. Repair the known checking-move LMR bug by
construction; do not repeat its de-tuned standalone gate. Test check/evasion
classes, result-dependent verification and post-LMR learning. Track overlap and
best-move-was-pruned recall. Keep switches ablatable for 5.10.

### 5.6 — History and correction attribution

Prevent capture/speculative/null/aborted outcomes from training quiet
correction. Fit per-source confidence instead of unconditional `/5`; test true
2/4-ply continuation-correction pairs with cache-conscious layout. Add threat,
check/evasion or halfmove context only if held-out residual/ordering evidence
shows unique value. Centralize saturation/aging and prevent correction
double-counting across eval/pruning/reduction. No dedicated SPSA.

### 5.7 — One root-confidence model

Derive completed-iteration confidence from per-move mean/variance, score gap,
PV continuity, best-move age, effort, fail direction/count and depth. Use it
for bounded/asymmetric aspiration and TM without double-counting. On abort,
publish only last completed legal evidence; incomplete mate/win/loss scores are
never authoritative. Pool worker instability for timing while a designated
completed root owns the result. Gate aspiration `[0,3]`; root/TM/SMP `[-3,3]`
at 1T STC/LTC and 4T LTC with zero forfeits.

### 5.8 — Cross-platform and ISA baseline (`origin/arm_fix`)

Make the shipped x86-64/ARM64 assets first-class release conditions before
measuring final throughput. The current development machine proves only a
Windows x86-64 native PEXT build on a Ryzen 9 5950X. Cross-compilation, a UCI
handshake and equal node count are necessary but do not prove that the asset
uses its intended instructions or retains reasonable speed.

Pinned platform references: Apple requires querying
[`hw.cachelinesize`](https://developer.apple.com/documentation/apple-silicon/addressing-architectural-differences-in-your-macos-code)
rather than assuming it from the architecture; Clang documents
[`__builtin_prefetch`](https://clang.llvm.org/docs/LanguageExtensions.html#builtin-prefetch)
as a cache hint whose value depends on measured access; and GitHub currently
provides native
[Linux, Windows and macOS ARM64 runners](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

#### Branch and sibling-engine disposition

| Evidence | Finding | Disposition |
|---|---|---|
| Basilisk `origin/arm_fix` / `67a987b` | Wraps four 32-byte TT clusters in a 128-byte-aligned block on Apple ARM64. It has no ARM timing evidence. A 32-byte-aligned 32-byte cluster already cannot straddle a 128-byte boundary, while the wrapper changes address arithmetic. | Preserve as a target-measured hypothesis; do not merge or describe as a fix. |
| Rarog `0ddc8e5` | Its child-TT prefetch was x86-only; the branch emits AArch64 `PRFM PLDL1KEEP`. | Basilisk already uses Clang/GCC `__builtin_prefetch` on AArch64. Inspect emitted code on every ARM production compiler and add a target-specific fallback only if the builtin disappears. |
| Rarog `3ee4660` | Applies the same unmeasured 128-byte Apple TT grouping to 32-byte local and 64-byte shared clusters. | Use it to widen the audit to all hot shared atomics/false-sharing boundaries, not as evidence for the TT wrapper. |
| Rarog release infrastructure | Separates ISA tier from host-native tuning and runs bench fingerprints on Linux/Windows/macOS ARM64 before release. | Adopt the explicit contract and five-platform pre-release fingerprint shape. |

Execute in this order:

1. **Make the asset contract executable.** Supported production assets are
   Linux/Windows x86-64 baseline, AVX2 and PEXT, plus Linux/Windows/macOS
   ARM64. Reconcile the current contradiction in which `release_tiers.md`
   calls PEXT an AVX2/POPCNT tier while portable CMake adds only `-mbmi2` and
   startup checks only BMI2. Either make PEXT explicitly x86-64-v3 + BMI2 or
   document/test a BMI2-only tier; artifact names, compile flags, runtime
   checks and user guidance must describe exactly the same ISA. Prove that the
   baseline contains no forbidden instruction and that every faster asset
   refuses unsupported hardware before search.
2. **Move ARM correctness before release.** Extend manually dispatched CI so
   Linux ARM64, Windows ARM64 and macOS ARM64 build and execute the production
   feature set; run the full suite on at least one ARM OS and bench/UCI/perft/
   Syzygy smoke plus cross-platform node agreement on all five OS/architecture
   cells. Assert the TT atomics required by the lock-free design are lock-free
   on every supported target. Release-only jobs are too late to discover a
   development regression.
3. **Inspect generated artifacts.** Record target triple, compiler/stdlib,
   flags, PGO profile/tool, CPU requirements, dynamic dependencies, binary
   hash and bench fingerprint. Disassemble representative hot paths to prove
   PEXT/magic selection, POPCNT/AVX2 contract and AArch64 prefetch emission.
   Re-run this inspection on compiler/toolchain changes.
4. **Measure the branch hypotheses on their target.** Query/log the real cache
   line and page sizes. Audit the TT plus `shared_nodes`, `shared_tbhits`, stop/
   root publication and future accumulator storage for destructive sharing;
   the two `alignas(64)` counters may still share one 128-byte Apple line.
   Compare flat storage, over-aligned allocation and block wrappers with
   identical capacity/indexing/replacement. Accept a layout or explicit
   AArch64 prefetch only after target-native paired PGO A/B, emitted-code
   inspection and no x86 regression.
5. **Create per-target performance anchors.** On stable native hardware, use
   identical-binary calibration followed by interleaved baseline/candidate
   PGO runs for each tier. Never compare raw NPS between unrelated GitHub
   runners or CPU families. CI timing is diagnostic; a stable target-local A/B
   decides. On the 5950X, also exercise baseline and AVX2 assets for semantics,
   illegal-instruction guards and same-tier regression, not only native PEXT.

Behaviour-neutral platform work uses exact bench/search agreement and
same-target NPS, never SPSA. Any change to move choice follows the normal
strength gate. Exit when the development revision passes the complete matrix,
the ISA manifest matches the executable, ARM prefetch/layout claims have real
target evidence, and `origin/arm_fix` is recorded as accepted pieces or a
closed hypothesis rather than merged wholesale.

### 5.9 — Throughput, TT capacity and scaling

Profile accepted semantics at 1/2/4/8T: NPS, time-to-depth, TT hit/replacement/
same-key, root stability and strength. Audit cluster contention after
provenance and test multiply-high full-budget indexing at non-power-of-two
Hash. Take only proven invariant hoists/batching through pooled-PGO interleaved
NPS. Preserve the 5.8 platform/ISA matrix and regenerate target-native PGO
after final code shape. Do not reopen generic helper diversification without
a specific measured failure.

### 5.10 — Single consolidated pre-NNUE search fit

Freeze architecture. Generate configuration from `search_params.h`; use
sensitivity/collinearity diagnostics to select ≤24 coordinates across
prospective-depth/pruning, NMP family, correction/history, qsearch and root/TM.
Run one registered 5,000-iteration SPSA, one estimator and one bake. Exclude
HCE, dead/off and redundant knobs. Clean PGO + CTest/telemetry + `[0,3]`
against the pre-fit architecture. Post-fit switch ablations must show the tune
is not compensating for a harmful subsystem.

### 5.11 — Cumulative target ladder and release 1.10.0

Beat 1.9.3 cumulatively at 1T STC/LTC and transfer at 4T with zero forfeits;
pass correctness/tactical/mate/zugzwang/TB/provenance/history/extension/root
telemetry and the Phase-5.8 production platform/ISA matrix. Then run paired
Basilisk, every installed Rybka (minimum
3/4.1/4/5/6), Critter 1.6a, Houdini 1.5a/2.0c and Fritz 16. Every required
target needs a logistic-Elo lower bound above zero under the primary 1T
condition, with Holm-adjusted 95% family-wise confidence; confirm 1T/4T LTC.
Rating-list inference cannot replace a missing engine. If all pass: version,
PGO/ISA/default-UCI/docs/archive and commit 1.10.0; do not push/tag. Otherwise
Phase 5 stays open.

## 6. Phase 6 — NNUE runway and branch convergence

### 6.0 — Branch handoff and inventory

Record Phase-5 handoff. Inventory the nine old `origin/nnue` commits against
current development and `D:/code/net_trainer`; port useful pieces in small
commits or rebase once. Do not resurrect removed tests/tooling/layout.

### 6.1 — State/dirty-piece contract

Create per-ply accumulator-ready state. Record exact dirty pieces for quiet,
capture, EP, promotion and castling; define null. Randomized make/unmake tests
compare state, keys, attacks and dirty data after unwind.

### 6.2 — Frozen teacher and data contract

Freeze diverse 1.10.0 quiet/tactical/endgame/rule-50, phase-balanced and
search-disagreement cohorts. Record teacher SHA/settings/labels/hashes and
untouched split IDs. Define the integer engine/trainer contract in
`net_trainer/docs/nnue_format.md`.

### 6.3 — Trainer preflight

Pin `net_trainer`, Bullet, Rust/CUDA/driver/GPU. Verify conversion, shuffle,
seeded splits, manifests, reference vectors and exact resume—or forbid resume—
on a pilot. Malformed CLI and excessive conversion loss fail loudly.

### 6.4 — Runway gate

Search stays identical to Phase 5; state work is bench-identical and passes
CTest/sanitizers/random unwind. Corpus/pilot reproduce from hashes. Then create
the integration branch.

## 7. Phase 7 — Baseline NNUE and release 2.0.0

Trainer/data live in `D:/code/net_trainer`; engine state/accumulator/loader/
SIMD/UCI/search integration live here.

### 7.0 — Harden trainer and conformance

Add train/validation/untouched-test splits, checkpoint selection, manifests,
strict CLI, deterministic seeds and exact resume/no-resume contract. Compare
training float and exported integer loss; validate C++/Rust/NumPy references.

### 7.1 — Controlled data at scale

Generate initial 30–60M unique teacher positions with search score and WDL.
A/B WDL blend, node budget and natural-end holdout. Test behavioural
disagreement mining separately from large static-eval error.

### 7.2 — Baseline networks

Train `chess768 → H×2 perspectives → 1×8 material buckets` SCReLU at H=512
pilot/H=1024 baseline with ≥2 seeds. Select by validation, touch test cohorts
once and identify every net by full manifest/SHA.

### 7.3 — Scalar integration

Strictly validate raw size/H/padding; embed release net and optionally accept a
validated `EvalFile`. Full-recompute scalar output matches reference vectors
and a large FEN corpus exactly for both sides/all buckets. Keep
`Evaluator::evaluate(const Board&)` as the boundary.

### 7.4 — Incremental accumulator and SIMD

Use Phase-6 dirty deltas per thread/ply. Debug/sanitizer builds compare full
recompute after every randomized move/unmove. Prove bounds before narrowing;
add exact portable and shipped SIMD kernels, benchmark components/full search,
then regenerate PGO.

### 7.5 — Baseline strength/architecture loop

Compare data, WDL blend, H=512/1024, LR and duration one variable at a time;
require two-seed evidence for architecture claims. Diagnose contract/data/
training/architecture when a bring-up net loses.

### 7.6 — Provisional search-scale calibration

Inspect score/correction/pruning telemetry. Change only gross NNUE-scale safety
margins through isolated tests; comprehensive search SPSA waits for Phase 8.

### 7.7 — Release 2.0.0

Default embedded NNUE beats 1.10.0 at STC/LTC, transfers at 4T, has zero
incremental/full mismatches and passes external/net-metadata gates. Portable
scalar inference must pass the Phase-5.8 matrix; every shipped x86 SIMD and
ARM64/NEON kernel is bit-exact to it and target-native PGO-smoked. Complete user
docs/version and commit; do not push/tag.

## 8. Phase 8 — NNUE frontier and final search fit

### 8.0 — Residual and disagreement analysis

Measure phase/material/king/tactical/endgame residuals, calibration,
teacher-search disagreement and accumulator refresh cost; choose work from
evidence.

### 8.1 — Data and label frontier

Scale/deduplicate data, natural finishes and hard-position mining. A/B depth,
WDL blend, teacher subsets and disagreement replay with untouched cohorts.

### 8.2 — Architecture ladder

Test king/perspective buckets, threat inputs, material/output buckets, width/
activation and refresh-friendly variants one axis at a time. Require two seeds,
integer conformance, NPS and SPRT; static loss alone cannot promote.

### 8.3 — Single post-NNUE search SPSA

After architecture/scale freezes, reselect ≤24 search/history/correction/
qsearch/root coordinates whose optima may have moved. Run the second and
normally last search SPSA; clean bake/PGO/SPRT/LTC/4T and ablations.

### 8.4 — Frontier release gate

Beat 2.0.0 and test contemporary Stockfish, Reckless, PlentyChess and another
independent engine with calibrated odds if needed. Archive all manifests and
release the next 2.x strength version when the matrix passes.

## 9. Phase 9 — Scaling, platforms and product completeness

### 9.0 — High-thread/NUMA scaling

Measure 8T+ topology, first-touch/NUMA, TT/accumulator sharing, root stopping
and false sharing while preserving 1T/4T.

### 9.1 — Advanced memory and dispatch

Revisit full-budget TT if supported, runtime ISA dispatch, large pages,
topology-specific prefetch and net placement. Phase 5.8 already guarantees the
baseline asset matrix; this step takes only additional real-hardware gains.

### 9.2 — Protocol/platform completion

Add demanded work such as Chess960 or additional platform/tier support. ARM64
correctness is already required by Phase 5.8 and NNUE/NEON parity by 7.4/7.7.

### 9.3 — Scaling release

Publish only after thread/platform matrix, clock safety, net parity and user
documentation pass.

## 10. Phase 10 — Optional HCE fallback (only if NNUE is abandoned)

This last phase may never run. Enter only after serious Phase-7/8 contract,
data and architecture retries fail and the user explicitly abandons NNUE.

### 10.0 — Failure review and scope decision

Document what failed and distinguish trainer/data/integration/compute from
evaluation capacity. Re-enable no HCE work without approval.

### 10.1 — HCE residual program

Select a few independent king/threat/endgame/complexity features from frozen
residuals, not a feature-name menu. Each structure change includes its refit.

### 10.2 — HCE fit and release

Run one evidence-selected HCE fit and the complete external matrix; preserve
NNUE branches/artifacts for later return.

## 11. Release checklist

1. Phase gate and direct prior-release match passed.
2. Clean tree; version in `src/constants.h` and `CMakeLists.txt`.
3. CTest/Python/sanitizer/tactical/mate/TB and bench recorded.
4. Fresh revision-matched PGO/ISA assets smoke-tested; manifests archived; no
   tune-only UCI options.
5. NNUE releases record net/architecture/trainer/Bullet/data hashes and exact
   incremental/reference parity.
6. Update user-facing changelog/readme only for visible changes; no internal
   phase history in release notes.
7. Commit locally. Do not tag/push. User publication triggers `release.yml`;
   a bare tag does not upload assets.

## 12. Common commands

```powershell
.\tools\setup_tools.ps1
.\tools\build_test.ps1 -Suffix <name>
.\tools\sprt.ps1 -EngineA <candidate> -EngineB <baseline> `
  -NameA Candidate -NameB Baseline -Elo1 3
.\tools\sprt.ps1 -EngineA <copy> -EngineB <same> `
  -NameA Self -NameB Self2 -Mode calibrate -Threads 4 -Games 10000
.\tools\nps_ab.ps1 -EngineA <candidate> -EngineB <candidate>
.\tools\nps_ab.ps1 -EngineA <candidate> -EngineB <baseline> -Rounds 12
.\tools\spsa.ps1 -ConfigGroup search_final -EngineSuffix <base> -Iterations 5000
.\tools\spsa.ps1 -ConfigGroup search_final -Resume
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <list> -TC "10+0.1"
```
