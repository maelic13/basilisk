# Basilisk development plan

This is the maintainer-facing source of truth for future work. `GUIDE.md` is
its short operational mirror; [`EXPERIMENTS.md`](EXPERIMENTS.md) is the
indexed, conditional evidence ledger. `README.md` and `CHANGELOG.md` remain
user-facing and must not contain experiment bookkeeping.

## 1. Current state

| Item | State |
|---|---|
| Branches | `master` and `v1.9.3` are at `d737123`; `development` is at `16eff20` with documentation/tooling/benchmark work. The only `src/` divergence is a comment-only edit in `search_params.h`, so playing code is unchanged. `origin/nnue` is an obsolete partial implementation whose useful seams must be reimplemented against the current trainer contract. `origin/arm_fix` is the one-commit `67a987b` TT-alignment experiment; Phase 5.11 closes the invalid wrapper hypothesis and retains only evidence-backed portability work. |
| Released baseline | **Basilisk 1.9.3**, bench-13 fingerprint **11,941,440**. It is search-identical to 1.9.2; 1.9.3 repaired Clang/`llvm-profdata` PGO tool matching. Reproduced clean at Phase 5.0 (see below). |
| Evaluation | The accepted HCE is the comparison/fallback evaluator. Constant refitting stays frozen; **structural** feature convergence is unfrozen at Phase 5.9 only. Phase 10 is the optional HCE fallback, entered only if NNUE is abandoned. |
| Active work | No Basilisk candidate or tuner is active. A Rarog-seeded Colosseum gauntlet occupies the machine at concurrency 14; do not start competing timed work while it runs. |
| Next release | **1.10.0 at Phase 5.13** if search/HCE convergence transfers, or a higher minor version if the cumulative gain is large. **1.9.4** is the maintenance-only fallback if convergence closes without transfer. |
| NNUE release | **2.0.0 at Phase 7.7**, using `D:/code/net_trainer`. |

### Live rating evidence — context only

The 36,400-game observation previously recorded here is superseded and is not
maintained. Basilisk's registered pool rating is now **3023**, with the
historical frontier (Houdini 1.5a, Critter 1.6a, Fritz 16) roughly **165–185
Elo** ahead and Rybka 4 about **+79**. These are mutually fitted pool ratings,
not independent confidence intervals.

This ladder is contextual evidence only. It gates nothing in Phase 5, is not a
precondition for starting NNUE, and moves to Phase 8.4. Do not schedule,
archive or curate rating tournaments as plan work.

Note the interaction with Phase 5's convergence program: the reference search
is stronger than this entire historical frontier, so a successful convergence
would close a large part of this gap as a side effect. That is a prediction to
be tested at 5.13, never a plan assumption.

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
2. A pre-NNUE **broad** tune is out of scope: its surface will be invalidated
   by NNUE. Any exception needs a demonstrated release blocker and explicit
   approval. Phase 5 convergence clusters may carry the small local refit a
   structural change requires — that is part of the cluster, not a tune — but
   must gate categorical architecture before fitting any constant.
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
   Frozen means no further constant refitting; structural gaps against a
   stronger reference are a separate question with a separate answer.
10. Git/CHANGELOG preserve experiment history; the forward GUIDE stays short.
11. A cross-compiled binary is not a validated asset. Compatibility requires
    target-native execution, exact search agreement, an executable ISA
    contract and same-target performance evidence.
12. Do not redesign a subsystem blind when a stronger public implementation can
    serve as a specification. Converge on its contracts, measure each
    dependency-complete cluster, and keep only what wins games. A reference
    tells you what to try and in what order; it never tells you what is
    accepted.
13. Isolate the variable before sizing the target. The search-oracle
    experiments are informative precisely because one side of the
    search/evaluation split was held exactly constant while the other changed.
14. Adjudication that reads engine scores is a confounder across engines with
    different evaluators; it moved a headline estimate by roughly 75 Elo.
    Cross-evaluator cohorts run with adjudication off.

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

## 5. Phase 5 — Search and evaluation convergence (→ 1.10.0 or higher)

### Objective and disposition

**Scope changed 2026-08-12 by maintainer decision.** Phase 5 was bounded
pre-NNUE hardening expected to produce no strength. Rarog's search-oracle
experiment (RAR-O01/O02, below) showed that Basilisk's largest measurable
deficit is not evaluation capacity or NNUE absence but **search coordination**,
with a second large deficit in **HCE feature coverage**. Both are addressable
against a public reference instead of being redesigned blindly. Phase 5 is
therefore now the main pre-NNUE strength program: converge Basilisk's search
and HCE toward the last pure-HCE Stockfish, one dependency-complete cluster at
a time, each gated by games.

The consequences are stated plainly:

- Phase 5 is no longer bounded maintenance. It is a multi-cluster program with
  a real chance of failure at any cluster.
- **1.9.4 is no longer the expected release.** The target is **1.10.0**, or a
  higher minor version if the cumulative gain is large. A maintenance-only
  1.9.4 remains the fallback if convergence closes without transfer.
- The HCE freeze is **partially lifted**, for structural feature convergence
  only. See "Two convergence tracks" below.
- Phase 5's correctness, portability and SMP work is retained in full and moves
  to the back of the phase, where it also serves as the release gate.

### Reference evidence (external, conditional)

From Rarog's `codex/search-convergence` `EXPERIMENTS.md`, IDs RAR-O01/O02. The
`hybrid` branch at `75d0d43` builds Stockfish `9587eeeb` — the last pure-HCE
master commit before NNUE merged — driving the unchanged Rarog 2.3.2 HCE
through a checked ABI, with the same binary's `Use Rarog HCE=false` giving an
exact-revision Stockfish-HCE control.

RAR-O02, the cleaner **no-adjudication** run (1,238 games, 982 ended in natural
checkmate, `3+0.03`, 1T, paired UHO, no forfeits):

| Contrast | What it isolates | Result |
|---|---|---|
| Hybrid − Basilisk 1.9.3 | Stockfish search vs Basilisk search | **≈ +196.5** Elo |
| Stockfish-HCE − Hybrid | Stockfish HCE vs Rarog HCE, same search | **≈ +328.6** Elo |
| Basilisk 1.9.3 − Rarog 2.3.2 | whole-engine, our own prior | ≈ +30.4 Elo |

The hybrid achieved this at **1.5M NPS against Basilisk's 2.5M**, so the search
result is not a throughput artifact and is if anything understated.

Read these with discipline:

1. They are ordinary logistic point estimates from a deliberately stopped run,
   **not** paired-pentanomial SPRT results. They size a target; they are never
   a release claim and are never added to anything.
2. They are *Rarog's* measurements against *Rarog's* HCE. The Basilisk-specific
   magnitudes are unknown until 5.1 measures them directly.
3. RAR-O01 with evaluator-dependent adjudication reported +270.9 where RAR-O02
   without it reported +196.5. **Cross-evaluator cohorts must run with
   adjudication off**; the confounder is worth ~75 Elo here.
4. The direction and order of magnitude are what transfer. No individual
   Stockfish mechanism has been credited with any Elo by this experiment.

### Two convergence tracks

**Search track (5.1–5.8).** Evaluator-agnostic. Every accepted search contract
survives NNUE intact, so this work is not spent against a soon-to-be-replaced
surface — it is the opposite of the constant-fitting that durable lesson 7
forbids. This track has priority.

**HCE track (5.9).** The freeze is lifted **only** for structural feature
convergence against the reference: terms Basilisk lacks entirely, or expresses
in a materially weaker form. It is **not** lifted for another broad constant
refit — HCE cycle 6 washed out at ±5.21 over 8.1k games and that verdict
stands. A stronger HCE also pays forward as a better NNUE teacher for Phase
7.1 datagen, which is the second reason it precedes NNUE rather than following
it.

### Reference-use rules

Basilisk and Stockfish are both GPLv3, so reuse is legally permissible. The
constraints here are engineering and product ones, not licensing ones:

- Stockfish `9587eeeb` is a **behavioral specification**. Converge on
  contracts, not on transcription; Basilisk keeps its own board, move
  generation, TT layout, UCI, build system and identity.
- Attribute derived work in source comments and `README.md`, naming the exact
  upstream revision.
- Never accept a cluster because its trace looks more Stockfish-like. Games
  decide, exactly as before.
- A reference contract Basilisk deliberately does otherwise is a valid
  outcome; record it as intentionally different with its reason.

### What still moves out of Phase 5

Move to Phase 8.3, after NNUE scale freezes: the single search SPSA, mechanism
ablations, and any evaluator-scale-sensitive consumer that convergence did not
already reach and settle. Convergence may supersede a deferred item when a
coherent cluster reaches that exact consumer; anything it does not reach stays
owned by 8.3.

Move full-budget TT and generic hotspot work to Phase 9.1, where NNUE has made
the final memory profile visible. Move the Rybka/Critter/Houdini/Fritz ladder
to Phase 8.4 as contextual frontier evidence, not a blocker for NNUE.

Rarog's Phase 4 remains cautionary evidence, not a language verdict: its
interacting search candidates did not compose and its broad SPSA was cancelled.
That is precisely why this phase gates dependency-complete clusters instead of
accumulating individually plausible mechanisms. C++23 remains the appropriate
implementation language.

### 5.0 — Freeze baseline — CLOSED 2026-08-11

The rating tournament is closed as contextual evidence and deliberately not
archived; no Basilisk plan artefact depends on it.

Clean 1.9.3 reproduced from `development` `16eff20` on a clean tree:

| Gate | Result |
|---|---|
| Bench-13 fingerprint | **11,941,440** — exact match |
| CTest | **12/12** passed |
| PGO manifest | complete (revision, `dirty_diff: clean`, Clang 22.1.8, binary SHA-256) |
| Branch divergence | docs/tooling only; the sole `src/` change is a comment in `search_params.h` |

Artefact: `tools/test_engines/basilisk-1.9.3-baseline-pext-pgo.exe`, SHA-256
`D80A4A7F…B3F87496`. This is the Phase-5 comparison baseline.

### 5.1 — Basilisk search-oracle experiment

Reproduce RAR-O01/O02 for Basilisk's own evaluator, because the Rarog figures
size a target for Rarog, not for us. Build Stockfish `9587eeeb` calling
Basilisk's exact 1.9.3 HCE, with the same binary's evaluator switch giving the
unmodified Stockfish-HCE control. Basilisk is C++, so this links directly and
needs no DLL or FFI adapter — expect a smaller throughput penalty than Rarog's
hybrid paid, which makes the measurement cleaner rather than merely cheaper.

Three contrasts, one binary, adjudication **off**:

| Contrast | Isolates | Decides |
|---|---|---|
| Hybrid − Basilisk 1.9.3 | search only, HCE held at Basilisk's | size of the search track |
| Stockfish-HCE − Hybrid | HCE only, search held at Stockfish's | size of the HCE track |
| Hybrid − Rarog hybrid | our HCE vs Rarog's under one search | whether our HCE is already ahead |

Freeze the oracle branch and its binary hashes on completion; never merge it,
never edit it retrospectively, and never ship any part of it. It exists to
size and later to explain, not to become product code.

**Stop rule.** If the search contrast is small — under roughly 50 Elo — the
premise of this phase is wrong for Basilisk. Close the convergence program,
restore bounded hardening and go to NNUE. Record that honestly rather than
proceeding on Rarog's numbers.

### 5.2 — Differential diagnostic harness

The former "bounded diagnostics and semantic census", **retained in full** and
widened to serve convergence. Define a versioned fixed suite spanning UHO
openings, quiet middlegames, tactics, checks, zugzwangs and endgames. At fixed
depth/nodes on one thread, emit deterministic counters for:

- **TT producer/consumer kind** — which mechanism wrote an entry and which read
  it, kept distinct per durable lesson 6;
- **prune recall and overlap** — not just node savings. Durable lesson 5 is
  explicit that a smaller tree can be worse; measure best-move recall and
  contradiction, and which prunes fire redundantly on the same node;
- **correction attribution** — which correction context claimed a node and
  whether the correction changed the decision;
- **history update attribution** — main, capture, continuation, low-ply, pawn;
- **completed-root ownership** and **SMP work distribution**;
- nodes and qnodes; move-picker source; fail-high move index; LMR population
  and re-searches; NMP, ProbCut, futility and razoring; extensions; aspiration
  retries.

Diagnostics **off** must preserve bench 11,941,440 exactly. Diagnostics **on**
must preserve best move and node counts with bounded overhead. Do not spend an
age bit, widen the dense TT or persist provenance until a measured consumer
needs it. Transient `OutcomeKind`/capability predicates may land here when
behaviour-neutral.

**Shadow-evidence discipline.** A concern this step surfaces but does not own —
stand-pat provenance, ProbCut, NMP/IIR/singular cooperation, checking-move LMR,
root confidence — is recorded as shadow evidence rather than acted on here. Its
first owner is the cluster that reaches it (5.4–5.8); anything no cluster
reaches falls through to Phase 8.3. Recording it is mandatory; acting on it in
this step is not permitted.

Run the same suite against the 5.1 oracle. A counter that differs sharply
between Basilisk and the oracle is the phase's primary work-selection signal —
this is what replaces guessing which mechanism to change.

### 5.3 — Reference contract map and order freeze

Map Stockfish `9587eeeb` search, move-picking and evaluation contracts to their
Basilisk owners. Classify each as **equivalent**, **intentionally different**
(with reason), **missing**, or **coupled to a later consumer**. Use the 5.2
populations to choose and document the first cluster.

If the evidence contradicts the provisional cluster order below, **edit this
plan before implementing** — never after seeing games.

### 5.4 — Cluster A: move ordering, histories, LMR

Implement in dependency order, as one coherent cluster:

- **5.4.1** move-picker contract: TT move, good captures, killers/counters,
  quiets, deferred bad captures; legality and duplicate guarantees.
- **5.4.2** evidence ownership: main, capture, continuation, low-ply and pawn
  history indexing, normalization, aging and cutoff attribution.
- **5.4.3** reduction/re-search contract: LMR population, improving/PV/cut-node
  adjustments, history feedback, zero-reduction floor, full-depth verification.
- **5.4.4** integrated gate, then ablate surprising contributors.

This cluster owns the latent post-move `gives_check` LMR defect. Durable lesson
2 applies directly: its standalone repair lost −21.55 ±9.83 because the old
surface was tuned around it. Repair it **inside** this cluster and fit jointly.

### 5.5 — Cluster B: static eval, TT and quiescence

Separate raw evaluation, pruning evaluation and searched bounds; align TT
capabilities, qsearch stand-pat, capture ordering and check handling, and
correction attribution. Preserve Basilisk's proven draw, mate-distance and
rule-50 semantics — these are correctness assets, not convergence targets.

### 5.6 — Cluster C: main selectivity

In observed dependency order, reconcile razoring, reverse futility, null-move
verification, ProbCut, move-count and history pruning, and quiet/capture
futility. Use prospective searched depth consistently. Gate categorical
architecture before any narrow constant fit; do not launch a broad SPSA.

### 5.7 — Cluster D: extensions and depth authority

Reconcile check, singular, double/negative extension and IIR semantics against
TT provenance and LMR. Preserve mate and abort correctness. Gate the integrated
contract, not the individual extensions.

### 5.8 — Cluster E: root search and clock handoff

Reconcile aspiration retries, completed-root authority, PV/fallback ownership
and stability inputs. Total time allocation must not move until the root
evidence is coherent; then gate any real-clock change separately under the
time/root/SMP evidence rule.

### 5.9 — HCE structural convergence

Entered only after the search track closes, so that evaluation is measured
against a settled search. Scope is **structural coverage**, set by the 5.1
Stockfish-HCE contrast and 5.2 residuals:

- terms the reference has that Basilisk lacks entirely;
- terms Basilisk expresses in a materially weaker form;
- each structural change carries its own local refit.

Explicitly **out of scope**: another broad Texel or SPSA constant fit over the
existing feature set. HCE cycle 6 washed at +1.37 ±5.21 over 8.1k games and
that verdict stands; do not retry it under a new name.

Gate each feature cluster on games like any other. If two consecutive HCE
clusters fail to transfer, close this track and carry the residual into NNUE
data selection instead.

### 5.10 — Correctness and safety repairs only

Repair only demonstrated safety or semantic failures with deterministic
regressions: legal completed-root fallback, symmetric decisive-score handling,
mate/rule-50 conversion, TT atomic/replacement invariants and attributable
history updates. The existing NMP unproven-mate clamp already matches the
useful Rarog fix and needs only coverage, not reimplementation.

A heuristic being mechanically cleaner is not a correctness proof. This step is
for residual safety defects only. In particular, do not force any of these here
as a "cleanup" — each has a named cluster owner and a history of losing when
moved alone:

| Deferred item | Owner | Why not here |
|---|---|---|
| Checking-move LMR repair | 5.4 | standalone repair lost −21.55 ±9.83 |
| New qsearch staging | 5.5 | stand-pat/capture provenance must move together |
| Subtree-null policy | 5.6 | couples to NMP verification and ProbCut |
| Correction weighting | 5.5 | attribution must be measured at 5.2 first |
| Aspiration model | 5.8 | root evidence must be coherent before the clock |

If a cluster has already closed, re-opening one of these is a new registered
experiment against the accepted head — not an amendment to a closed verdict.

If a repair changes reachable play, isolate it and apply the normal strength
gate. If it belongs to a cluster that already closed, it is a new registered
experiment against the accepted head, not an amendment to a closed verdict.
Anything with no owner here defers to Phase 8.3 rather than keeping Phase 5
open.

### 5.11 — Portability and ISA baseline (`origin/arm_fix`)

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

### 5.12 — SMP effectiveness checkpoint

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

### 5.13 — Cumulative checkpoint and release gate

Build the accepted head and 1.9.3 through the same pinned PGO path. Compare
directly at 1T STC/LTC and 4T LTC; require zero forfeits, the correctness
matrix and the 5.11 platform/ISA contract. Explain the cumulative result with
the frozen 5.2 diagnostics, and run a final cross-engine cohort with
adjudication **off**, including the 5.1 oracle as the convergence reference.

- Release **1.10.0** when convergence produced a material, independently
  confirmed gain: normally a cumulative STC point estimate of at least **+40
  Elo** over 1.9.3 with the 95% lower bound above **+25 Elo**, plus positive
  LTC and 4T lower bounds.
- A larger cumulative result may justify a higher minor version; that is a
  maintainer decision, not an automatic consequence of the number.
- Release **1.9.4** as the fallback if the convergence tracks close without
  material transfer but the correctness/platform/SMP work is non-regressing.
- If a candidate regresses, revert or defer it; do not extend Phase 5 into an
  open-ended rescue campaign.

Remove diagnostic scaffolding that has no future owner, resolve dormant
switches, version, rebuild revision-matched PGO/ISA assets, update user
documentation and archive the evidence; commit but do not push/tag.

### Cluster discipline and stop rules

These govern 5.4–5.9 and exist because Rarog's Phase 4 failed by accumulating
individually plausible search mechanisms that did not compose.

1. Each cluster starts from the last **accepted** integration head — never from
   another unresolved candidate.
2. Register the hypothesis, dependency map, baseline SHA, gate, cap and stop
   rule in `EXPERIMENTS.md` **before** any games. A coherent cluster with a
   plausible 10 nElo prior uses a preregistered `[3,10]` nElo SPRT; anything
   without that prior falls back to the §2 gate table. Do not use `[3,10]` to
   flatter a cluster that was never expected to pay that much.
3. Implement the smallest dependency-complete change. Substeps may be compiled
   and diagnosed separately, but an incomplete cluster never becomes the next
   strength baseline.
4. Counters explain a candidate; they cannot accept it. Only a registered
   final-PGO SPRT accepts.
5. Each cluster ends accepted or reverted before the next begins. Borderline
   results are not accumulated as hidden debt.
6. Ablate a surprising integrated result before crediting a subcomponent.
7. **After two fully implemented clusters fail to produce an accepted gain,
   stop and re-audit 5.2–5.3.** Do not continue down the list by sunk cost.
8. A touched dormant switch must be removed, kept inert with a named owner, or
   separately gated. It may not be activated opportunistically.

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
scalar inference must pass the Phase-5.11 matrix; every shipped x86 SIMD and
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

After architecture/scale freezes, resolve whatever Phase-5 convergence did not
already reach and settle, with isolated categorical A/Bs: TT/result evidence,
qsearch/ProbCut consumers, NMP/IIR/singular cooperation, prospective-depth/LMR,
history/correction attribution and root confidence. Consult the Phase-5 cluster
verdicts first — a contract accepted or rejected there under HCE is re-opened
here only because NNUE changed its inputs, not to relitigate it.

Then select ≤24 non-redundant search coordinates and run the only planned
search SPSA; require clean bake/PGO/SPRT/LTC/4T and post-fit ablations. This
remains the single planned search tune: Phase 5 converges architecture, Phase
8.3 fits it once to the NNUE scale.

### 8.4 — Frontier release gate

Beat 2.0.0 and test contemporary Stockfish, Reckless, PlentyChess and another
independent engine with calibrated odds if needed. Use installed Rybka,
Critter, Houdini and Fritz versions as a contextual historical ladder; missing
commercial engines do not block NNUE development. Archive all manifests and
release the next 2.x strength version when the matrix passes.

## 9. Phase 9 — Scaling, platforms and product completeness

### 9.0 — High-thread/NUMA scaling

Continue from the Phase-5.12 baseline: measure 8T+ topology, first-touch/NUMA,
TT/accumulator sharing, root stopping and false sharing while preserving
1T/4T. Do not retry Rarog-style iteration staggering without a new,
Basilisk-specific depth-diversity signal.

### 9.1 — Advanced memory and dispatch

Revisit full-budget TT if supported, runtime ISA dispatch, large pages,
topology-specific prefetch and net placement. Phase 5.11 already guarantees the
baseline asset matrix; this step takes only additional real-hardware gains.

### 9.2 — Protocol/platform completion

Add demanded work such as Chess960 or additional platform/tier support. ARM64
correctness is already required by Phase 5.11 and NNUE/NEON parity by 7.4/7.7.

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
