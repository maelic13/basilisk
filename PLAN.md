# Basilisk development plan

This is the maintainer-facing source of truth for future work. `GUIDE.md` is
its short operational mirror; [`EXPERIMENTS.md`](EXPERIMENTS.md) is the
indexed, conditional evidence ledger. `README.md` and `CHANGELOG.md` remain
user-facing and must not contain experiment bookkeeping.

## 1. Current state

| Item | State |
|---|---|
| Branches | `master` and `v1.9.3` are at `d737123`; `development` is at `f045b37` with documentation/tooling/benchmark work but unchanged playing code. `origin/nnue` is an obsolete partial implementation whose useful seams must be reimplemented against the current trainer contract. `origin/arm_fix` is the one-commit `67a987b` TT-alignment experiment; Phase 5.3 closes the invalid wrapper hypothesis and retains only evidence-backed portability work. |
| Released baseline | **Basilisk 1.9.3**, bench-13 fingerprint **11,941,440**. It is search-identical to 1.9.2; 1.9.3 repaired Clang/`llvm-profdata` PGO tool matching. |
| Evaluation | The accepted HCE remains the comparison/fallback evaluator, but all new HCE strength work is frozen. Phase 10 is the optional HCE fallback and is entered only if NNUE is abandoned. |
| Active work | No Basilisk candidate or tuner is active. The last recorded 36,400-game Rating Tournament observation is provisional; Phase 5.0 determines whether it is complete/running and archives its exact state. |
| Next release | **1.9.4 by default at Phase 5.5.** Promote it to **1.10.0 only if** the bounded Phase-5 work produces a material, independently confirmed strength gain. |
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
Houdini 2.0c and Fritz 16 are absent. The historical ladder remains useful
context, but it is not a sensible precondition for starting NNUE and moves to
Phase 8.4. Closing 150–210 Elo with bounded pre-NNUE maintenance is not
expected; Phase 5 is successful if it leaves a correct, portable, reproducible
handoff without losing strength.

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
- Before proposing or retrying a mechanism, consult `EXPERIMENTS.md` by
  subsystem. Update its verdict, conditions, lesson and retry trigger in the
  same commit that closes an experiment; keep forward sequencing only here.
- Preserve unrelated user changes. Dirty test artifacts record their diff hash
  and cannot become release baselines.
- While any long tournament, SPSA, SPRT or datagen job occupies the machine:
  do not start competing bench/NPS/PGO/game work.

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

1. **One post-NNUE search SPSA:** Phase 8.3, after the retained NNUE
   architecture/score scale freezes.
2. A pre-NNUE tune is out of scope: its surface will be invalidated by NNUE.
   Any exception needs a demonstrated release blocker and explicit approval.
3. A further post-NNUE run needs explicit evidence that the prior run could not identify
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

## 5. Phase 5 — Bounded pre-NNUE hardening (→ 1.9.4 by default)

### Objective and disposition

Create a correct, portable and reproducible NNUE handoff without spending
months fitting the soon-to-be-replaced HCE search surface. No HCE work, broad
search redesign, pre-NNUE SPSA or mandatory historical-engine ladder belongs
here. Expected strength is **zero to a small positive gain**, not a booked Elo
improvement; neutral correctness/platform work still completes this phase.

Rarog's Phase 4 is cautionary evidence, not a language verdict. Its interacting
search candidates did not compose, persisted TT provenance was neutral/costly,
and its planned broad SPSA was cancelled. C++23 remains the appropriate engine
implementation language here; Rust ownership did not cause the experimental
design and transfer failures, and changing languages would not cure them.

Move evaluator-scale-sensitive work to Phase 8.3 after NNUE freezes:

- persisted TT provenance and changed qsearch/ProbCut consumers;
- NMP/IIR/singular alternatives and the joint prospective-depth/LMR repair;
- history/correction contexts and root-confidence aspiration/TM changes;
- the single search SPSA and mechanism ablations.

Move full-budget TT and generic hotspot work to Phase 9.1, where NNUE has made
the final memory profile visible. Move the Rybka/Critter/Houdini/Fritz ladder
to Phase 8.4 as contextual frontier evidence, not a blocker for NNUE.

### 5.0 — Freeze baseline and close live observation

Determine whether the 36,400-game tournament is complete or still running and
archive its exact PGN/config/binary/manifest state without extrapolating the
provisional ratings. Reproduce clean 1.9.3 with CTest 12/12, PGO manifest and
bench 11,941,440. Record the actual `development`/release branch divergence.

### 5.1 — Bounded diagnostics and semantic census

Add one sampled, deterministic diagnostic substrate only where it answers a
Phase-5 decision or establishes a Phase-8 baseline: TT producer/consumer kind,
prune-recall/overlap, correction attribution, completed-root ownership and SMP
work distribution. Diagnostics off must preserve fingerprint; diagnostics on
must preserve best move/nodes and have bounded overhead.

Transient `OutcomeKind`/capability predicates may land when behaviour-neutral.
Do not spend an age bit, widen the dense TT or persist provenance until a
measured consumer needs it. Record stand-pat, ProbCut, NMP/IIR/singular,
checking-LMR and root-confidence concerns as shadow evidence for Phase 8.3.

### 5.2 — Correctness and safety repairs only

Repair only demonstrated safety or semantic failures with deterministic
regressions: legal completed-root fallback, symmetric decisive-score handling,
mate/rule-50 conversion, TT atomic/replacement invariants and attributable
history updates. The existing NMP unproven-mate clamp already matches the
useful Rarog fix and needs only coverage, not reimplementation.

A heuristic being mechanically cleaner is not a correctness proof. In
particular, do not force the previously losing checking-move LMR repair, new
qsearch staging, subtree-null policy, correction weighting or aspiration model
before NNUE. If a repair changes reachable play, isolate it and apply the
normal strength gate; otherwise defer it to Phase 8.3 rather than keeping
Phase 5 open.

### 5.3 — Portability and ISA baseline (`origin/arm_fix`)

Make Linux/Windows x86-64 baseline, AVX2 and PEXT plus Linux/Windows/macOS
ARM64 executable release contracts. Artifact names, flags, startup guards and
documentation must agree; record compiler/stdlib/PGO tool, target, binary hash,
dependencies and bench. Run production feature smokes on every target, the
full suite on at least one ARM64 OS, assert required TT atomics are lock-free,
and inspect representative binaries for the promised ISA and AArch64 prefetch.

`origin/arm_fix`'s 128-byte TT wrapper is closed as rejected: a 32-byte-aligned
32-byte cluster cannot straddle a 128-byte boundary, the wrapper changes
address arithmetic, and Rarog found no material Apple 4T false-sharing case.
Do not port or target-benchmark that wrapper absent new evidence. Basilisk
already expresses ARM prefetch through `__builtin_prefetch`; verify emitted
`PRFM` and add a fallback only if a production compiler drops it.

Query real cache-line/page sizes and inspect actual hot shared state (node/TB
counters, stop/root publication and later accumulators). Any measured layout
or prefetch candidate needs target-native interleaved PGO NPS A/B and no x86
regression. Create stable per-target performance anchors, but never compare raw
NPS between unrelated machines or GitHub runners.

### 5.4 — SMP effectiveness checkpoint

Rarog has better measured throughput scaling (about 12.3× NPS at 16T), but its
fixed-time search gained no depth; this is not proof of better chess scaling.
Basilisk's current full ladder is older and incomplete despite its accepted 4T
strength bundle and indicative +12.8% 16T node-counter batching result.

The historical cross-engine result also needs a current check. Rarog 2.3.0
minus Basilisk 1.9.1 was measured as pool Elo with one anchor:

| Threads | `3+0.03` | `10+0.1` |
|---|---:|---:|
| 1T | −55 ± 21 | −38 ± 27 |
| 4T | −32 ± 50 | **+34 ± 24** |

This does not show that Basilisk was generally weaker at longer TC: it led at
both 1T conditions, and the sign flipped only at 4T LTC. It does establish a
plausible thread × time-control crossover. Because Basilisk 1.9.2/1.9.3 then
changed SMP, the old versions cannot answer whether the crossover remains.

Run one null-calibrated, alternating same-process 1/2/4/8/16T sweep using a
clean PGO binary. Record NPS, nodes/time-to-fixed-depth, completed depth at
fixed time, main/helper node share, useful TT cutoffs, same-key writes and root
stability.

Then run one bounded paired UHO matrix for the current release pair: Basilisk
1.9.3 versus Rarog 2.3.2, or a newer pinned Rarog release if registered before
the run, at `{1T, 4T} × {3+0.03, 10+0.1}` with the same book, hash, affinity,
topology and adjudication. Pre-register per-cell minimum/cap and report Elo/CI,
forfeits and completed-depth telemetry. Report the time contrast at each thread
count, the thread contrast at each TC and their difference-in-differences with
propagated or bootstrap uncertainty; four point estimates alone cannot prove a
crossover.

If Basilisk converts threads efficiently, preserves 4T strength and the
historical crossover is not confirmed, close this step with no code change.
If the matchup crossover remains while internal depth conversion is healthy,
classify it as search/selectivity/time-management evidence for Phase 8.3, not
as an SMP implementation defect.

If an internal conversion deficit is reproduced, classify it first as
contention, redundant width, root ownership/stop publication or missing depth
diversity, then test **at most one** targeted mitigation through the full SMP
gate. Do not copy Rarog's
iteration-staggering table: it was tested and rejected. Defer high-thread NUMA
and accumulator-specific work to Phase 9.0.

### 5.5 — Handoff and release gate

Compare the cumulative Phase-5 candidate with 1.9.3 at 1T STC/LTC and 4T LTC;
require zero forfeits, the correctness matrix and the Phase-5.3 platform/ISA
contract. This is a prior-release non-regression gate, not an attempt to close
the entire historical rating gap.

- Release **1.9.4** when the maintenance/platform/SMP result is non-regressing
  but does not show a material strength gain.
- Release **1.10.0** only if a preregistered material gate (normally `[3,10]`
  nElo) accepts and positive transfer is independently confirmed at LTC/4T.
- If a candidate regresses, revert or defer it; do not extend Phase 5 into an
  open-ended rescue campaign.

Version, rebuild revision-matched PGO/ISA assets, update user documentation and
archive the evidence; commit but do not push/tag.

## 6. Phase 6 — NNUE runway and branch convergence

### 6.0 — Branch handoff and inventory

Record the Phase-5 handoff. Inventory the nine old `origin/nnue` commits against
current development and `D:/code/net_trainer`, but do **not** rebase that branch
wholesale: its `.mnn`, single-output and full-recompute contracts predate the
current Bullet quantised format, output buckets and integer math. Reimplement
or cherry-pick only useful current-contract seams in small commits. Do not
resurrect removed tests, tooling or layout.

### 6.1 — State/dirty-piece contract

Create per-ply accumulator-ready state. Record exact dirty pieces for quiet,
capture, EP, promotion and castling; define null. Randomized make/unmake tests
compare state, keys, attacks and dirty data after unwind.

### 6.2 — Frozen teacher and data contract

Freeze diverse Phase-5-release quiet/tactical/endgame/rule-50, phase-balanced and
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

Default embedded NNUE beats the Phase-5 release (1.9.4 unless promoted) at
STC/LTC, transfers at 4T, has zero
incremental/full mismatches and passes external/net-metadata gates. Portable
scalar inference must pass the Phase-5.3 matrix; every shipped x86 SIMD and
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

### 8.3 — Post-NNUE search architecture and single fit

After architecture/scale freezes, resolve Phase-5 shadow findings with isolated
categorical A/Bs: TT/result evidence, qsearch/ProbCut consumers, NMP/IIR/
singular cooperation, prospective-depth/LMR, history/correction attribution
and root confidence. Keep only mechanisms with diagnostic and game evidence.
Then select ≤24 non-redundant search coordinates and run the only planned
search SPSA; require clean bake/PGO/SPRT/LTC/4T and post-fit ablations.

### 8.4 — Frontier release gate

Beat 2.0.0 and test contemporary Stockfish, Reckless, PlentyChess and another
independent engine with calibrated odds if needed. Use installed Rybka,
Critter, Houdini and Fritz versions as a contextual historical ladder; missing
commercial engines do not block NNUE development. Archive all manifests and
release the next 2.x strength version when the matrix passes.

## 9. Phase 9 — Scaling, platforms and product completeness

### 9.0 — High-thread/NUMA scaling

Continue from the Phase-5.4 baseline: measure 8T+ topology, first-touch/NUMA,
TT/accumulator sharing, root stopping and false sharing while preserving
1T/4T. Do not retry Rarog-style iteration staggering without a new,
Basilisk-specific depth-diversity signal.

### 9.1 — Advanced memory and dispatch

Revisit full-budget TT if supported, runtime ISA dispatch, large pages,
topology-specific prefetch and net placement. Phase 5.3 already guarantees the
baseline asset matrix; this step takes only additional real-hardware gains.

### 9.2 — Protocol/platform completion

Add demanded work such as Chess960 or additional platform/tier support. ARM64
correctness is already required by Phase 5.3 and NNUE/NEON parity by 7.4/7.7.

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
.\tools\build_test.ps1 -Suffix <name>
colosseum-cli --run-file tools/colosseum/profiles/sprt-gainer.toml `
  sprt <candidate> <baseline> --book <book.epd> --concurrency <games>
colosseum-cli --run-file tools/colosseum/profiles/calibrate-4t.toml `
  calibrate <engine> <identical-copy> --book <book.epd> --concurrency <games>
colosseum-cli nps <candidate> --self-pair --nodes 10000000
colosseum-cli nps <candidate> --against <baseline> --nodes 10000000 --repetitions 12
colosseum-cli --run-file tools/colosseum/profiles/spsa.toml `
  spsa <tune-engine> --tune tools/colosseum/tunes/<group>.toml `
  --book <book.epd> --concurrency <games> --dir <run-directory>
```

Generic engine testing is owned by the independent Colosseum CLI. See
`tools/colosseum/README.md` for the complete workflow and responsibility
boundary.
