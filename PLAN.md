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
| Evaluation | **HCE UNFROZEN 2026-08-25 by maintainer decision.** Phase 5.9 is now a maturity program: new mechanisms, a joint Texel refit of the enlarged surface, and SPSA over the coefficients Texel cannot price. Cycle 6's wash forbids refitting the *same* feature set, not an enlarged one. Phase 10 remains the optional HCE fallback if NNUE is abandoned. |
| Active work | No Basilisk candidate or tuner is active; the machine is free. Phase 5.1–5.4 closed. Cluster A produced **no accepted change** — reduction magnitude refuted on the harness (BAS-S13/S14/S15) and check-depth rejected by games (BAS-S16, −3.48 ±3.32 over 17,058). Re-audited 2026-08-13 (`analysis/reaudit_v1.md`): BAS-O04 attributes the depth gap **95.9% to search policy, 4.1% to evaluation**, refuting the eval-symptom hypothesis. **5.4, 5.5 and 5.6 closed with no engine change and the budget clause fired.** But the 2026-08-13 Manta import (BAS-D05) shows all three attacked a quantity that was never deficient: our per-ply branching is **1.692 against the reference's 1.894**, and the real deficit is a **4.4× constant-factor cost at shallow depth**. New step **5.14** owns that; the budget decision should be taken with it in view. |
| Next release | **1.10.0 at Phase 5.13** if search/HCE acceleration transfers, or a higher minor version if the cumulative gain is large. **1.9.4** is the maintenance-only fallback if the acceleration tracks close without transfer. |
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

Note the interaction with Phase 5's acceleration program: the reference search
is stronger than this entire historical frontier, so a successful acceleration pass
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
2. A pre-NNUE **broad search** tune is out of scope: that surface will be
   invalidated by NNUE. **Exception, granted 2026-08-25: evaluation.** Phase
   5.9.5 runs one registered SPSA over the evaluation coefficients a static
   Texel objective cannot price — the capped, nonlinear king-danger funnel above
   all (BAS-X14). It is bounded to that coordinate set, follows the same
   doctrine as any other run, and does not consume the single post-NNUE
   **search** SPSA reserved for Phase 8.3. Any other exception still needs a
   demonstrated release blocker and explicit approval. Phase 5 acceleration clusters may carry the small local refit a
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
   not just node savings. Confirmed directly at 5.4.4: a 28% smaller tree
   measured −3.48 ±3.32 Elo. Width is not waste to be trimmed — it is priced
   against the quality of the decision made at each pruning point. Note what
   that does *not* mean: BAS-O04 showed width is 96% search policy and only 4%
   evaluation strength, so "better eval would let us prune harder" is false
   here. The decision quality that matters is the mechanism's own, not the
   evaluator's.
6. Static eval, stand pat, qsearch moves, ProbCut, null cutoffs, reduced search
   and full search require different provenance.
7. Do not tune before architecture freezes; a tune can hide a defect and make
   its repair look negative.
8. Multi-thread strength is a separate deployment condition.
9. **Superseded 2026-08-25 — the HCE is no longer frozen.** What the freeze
   correctly captured survives as a narrower rule: refitting the *same* feature
   set is exhausted (cycle 6 washed at +1.37 ±5.21 over 8,100 games), and
   adding reference-family terms with hand-set coefficients loses (BAS-X11,
   ~−23 Elo across two Manta gates). A refit is justified only when the surface
   itself has changed. The HCE also remains a debug oracle, NNUE teacher and
   fallback.
10. Git/CHANGELOG preserve experiment history; the forward GUIDE stays short.
11. A cross-compiled binary is not a validated asset. Compatibility requires
    target-native execution, exact search agreement, an executable ISA
    contract and same-target performance evidence.
12. Do not redesign a subsystem blind when a stronger public implementation can
    show what problem it solves. Use it to learn the problem and the contracts,
    then write Basilisk's own answer and gate it on Basilisk's games. A
    reference accelerates *what to try and in what order*; it never supplies
    code, constants or acceptance. If the only justification for a design is
    that a stronger engine does it, it is not understood well enough to ship.
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

## 5. Phase 5 — Search and evaluation acceleration (→ 1.10.0 or higher)

### Objective and disposition

**Scope changed 2026-08-12 by maintainer decision.** Phase 5 was bounded
pre-NNUE hardening expected to produce no strength. Rarog's search-oracle
experiment (RAR-O01/O02, below) showed that Basilisk's largest measurable
deficit is not evaluation capacity or NNUE absence but **search coordination**,
with a second large deficit in **HCE feature coverage**.

Phase 5 is therefore now the main strength program: **build the engine up
properly, using Stockfish as an idea source to move faster than a blind
redesign could** — one dependency-complete cluster at a time, each designed as
Basilisk's own answer and each gated by our own games. The goal is a stronger
independent Basilisk, not a closer resemblance to anything. See the
Independence contract below; it is binding, not aspirational.

The consequences are stated plainly:

- Phase 5 is no longer bounded maintenance. It is a multi-cluster program with
  a real chance of failure at any cluster.
- **1.9.4 is no longer the expected release.** The target is **1.10.0**, or a
  higher minor version if the cumulative gain is large. A maintenance-only
  1.9.4 remains the fallback if the acceleration tracks close without transfer.
- The HCE freeze is **partially lifted**, for structural feature work
  only. See "Two acceleration tracks" below.
- Phase 5's correctness, portability and SMP work is retained in full and moves
  to the back of the phase, where it also serves as the release gate.

### Program order — this phase is not an NNUE shortcut

The intended order is deliberate and is **not** to be compressed:

```text
Phase 5   build the engine up: search, then HCE, then correctness/platform/SMP
Phase 6   NNUE runway — corpus, state contract, trainer preflight
Phase 7   train and integrate the baseline NNUE, release 2.0.0
Phase 8   the engine adjustments NNUE makes necessary, then the single SPSA
```

Phase 5 is finished when *its own* release gate passes, not when NNUE looks
reachable. Do not pull Phase 6/7 work forward, do not start datagen or trainer
work during Phase 5, and do not treat a strong Phase-5 result as a reason to
skip steps — a stronger engine makes a **better** NNUE teacher, so rushing past
this phase costs twice.

Equally, do not treat Phase 5 as a reason to abandon NNUE. Search work here is
evaluator-agnostic and survives the transition intact; HCE work here improves
the Phase 7.1 teacher. Both feed the same destination.

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
2. They are *Rarog's* measurements against *Rarog's* HCE. **Superseded for
   Basilisk by BAS-O01–O03**, which measured our own magnitudes directly: the
   search gap is larger than theirs (+323 vs +196) and the evaluation gap
   smaller (+233 vs +329). Rarog's figures remain useful only as the prior
   that justified building the instrument.
3. RAR-O01 with evaluator-dependent adjudication reported +270.9 where RAR-O02
   without it reported +196.5. **Cross-evaluator cohorts must run with
   adjudication off**; the confounder is worth ~75 Elo here.
4. The direction and order of magnitude are what transfer. No individual
   Stockfish mechanism has been credited with any Elo by this experiment.

### Two acceleration tracks

**Search track (5.1–5.8) — measured at ≈ +323 Elo (BAS-O01).**
Evaluator-agnostic. Every accepted search contract survives NNUE intact, so
this work is not spent against a soon-to-be-replaced surface — it is the
opposite of the constant-fitting that durable lesson 7 forbids. This track has
priority, now on evidence rather than expectation.

**HCE track (5.9) — measured at ≈ +233 Elo (BAS-O02).** The freeze is lifted
**only** for structural feature gap closure against the reference: terms
Basilisk lacks entirely, or expresses in a materially weaker form. It is
**not** lifted for another broad constant refit — HCE cycle 6 washed out at
±5.21 over 8.1k games and that verdict stands. A stronger HCE also pays
forward as a better NNUE teacher for Phase 7.1 datagen, which is the second
reason it precedes NNUE rather than following it.

Neither number is a budget. They bound what is available if convergence were
perfect, which it will not be; each cluster still has to earn its own SPRT.

### Independence contract

**Basilisk is and remains an independent engine.** Stockfish is used to
*accelerate* development — as an idea source, a diagnostic oracle and a
statement of what a mature search achieves — never as something to become.
`README.md` already states this position publicly; this section is its
engineering form.

Both projects are GPLv3, so reuse would be legally permissible. That is not the
constraint. The constraint is that a transcribed engine is not our engine: it
inherits decisions we cannot explain, discards work that already measured well,
and leaves us unable to reason about our own code. The point of this phase is to
get the *insight* cheaply, not the source.

**Required:**

- Read the reference to learn **what problem a mechanism solves** and **what
  contracts it needs to hold**. Then design Basilisk's answer.
- Reimplement in Basilisk's own idiom, naming, types and structure, against our
  board, move generator, TT layout and parameter table.
- Attribute the idea in source comments and `README.md`, naming the exact
  upstream revision.
- Gate every adopted idea through Basilisk's own SPRT. A reference tells you
  what to try and in what order; it never tells you what is accepted.

**Forbidden:**

- Copying source, or paraphrasing it closely enough that the result is a
  translation rather than an implementation.
- Importing constant tables verbatim. Reference constants were fitted to a
  different search and a different evaluation scale; they are starting points
  to validate, never values to trust.
- Mirroring upstream file layout, function decomposition or naming so that our
  code becomes navigable only by reference to theirs.
- Deleting a Basilisk-original mechanism merely because the reference lacks it.
  That is a measured decision like any other, and several of our mechanisms
  have no upstream counterpart because they were fitted to our engine.
- Accepting a cluster because its trace looks more Stockfish-like. Games decide,
  exactly as before.

**The test.** For each adopted idea, a maintainer should be able to state why
Basilisk does it this way without appealing to "because Stockfish does". If the
only available justification is upstream authority, the idea is not understood
well enough to ship. Record a deliberate difference as *intentionally
different* with its reason — divergence is a valid and expected outcome, not a
gap to be closed.

### What still moves out of Phase 5

Move to Phase 8.3, after NNUE scale freezes: the single search SPSA, mechanism
ablations, and any evaluator-scale-sensitive consumer that Phase 5 did not
already reach and settle. Phase 5 may supersede a deferred item when a
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

**Instrument built — branch `hybrid` at `01df815`, 2026-08-12.** See
`hybrid/README.md` on that branch. Nothing under `src/` is modified, so the
evaluator under test is the released one. Verified before any game:

| Check | Result |
|---|---|
| Adapter conformance | 471,519 random-walk positions (1,931 en passant, 21,361 in check) — bitboard reconstruction is evaluation-identical to Basilisk's own parsed board, **0 mismatches** |
| Scale mapping | Basilisk `+74` cp ⇒ oracle `eval` `0.74` |
| Evaluator switch | `+0.74` vs `+0.13` on one position — the arms genuinely differ |
| Throughput | oracle **2.55M** NPS, control **2.96M**, native Basilisk ~2.5M |

The ~14% adapter cost matters to the result's strength: Rarog's DLL hybrid paid
~37% and still won by ~196, so their figure carried an obvious objection. Ours
runs at essentially native Basilisk speed, making the search contrast close to
throughput-neutral.

**Remaining: run the three contrasts.** Adjudication off, `3+0.03`, 1T, paired
UHO, tablebases and ponder off, per durable lesson 14.

**Stop rule.** If the search contrast is small — under roughly 50 Elo — the
premise of this phase is wrong for Basilisk. Close the acceleration program,
restore bounded hardening and go to NNUE. Record that honestly rather than
proceeding on Rarog's numbers.

### 5.1 — CLOSED 2026-08-12: both tracks confirmed, search first

2,400 games, adjudication fully off, zero forfeits, all-natural terminations.
Full conditions and caveats in `EXPERIMENTS.md` BAS-O01–O03.

| Contrast | What it isolates | Result |
|---|---|---|
| Oracle − Basilisk 1.9.3 | **search**, our HCE held constant | **+322.7 ±36** |
| Stockfish HCE − Oracle | **evaluation**, SF search held constant | **+232.8 ±32** |
| Full Stockfish − Basilisk 1.9.3 | the whole gap | +516.1 ±59 |
| Basilisk 1.9.3 − Rarog 2.3.2 | our own prior | +14.8 ±27 |

**The stop rule is cleared by a wide margin** — +323 against a ~50 threshold.
Both tracks are real, and the search track is the larger, so the plan's
existing ordering stands unchanged.

Three things make this stronger than the Rarog result it replicates:

1. **The oracle was handicapped and still won.** It ran 2.4M NPS against
   Basilisk's 2.9M and searched *fewer* nodes per move — 170k against 226k.
   The search advantage is therefore understated, and the "it only won because
   of speed" objection is not available.
2. **The legs compose.** +232.8 + 322.7 = +555.5 against a directly measured
   +516.1; the 39-Elo shortfall is inside the combined interval and in the
   expected direction for non-additive Elo. Had isolation been leaking, these
   would not have agreed.
3. **The mechanism is visible, not inferred.** At equal time Basilisk finishes
   **15.6** plies where the oracle finishes **25.2** — 9.6 plies shallower on
   more nodes. Effective branching factor **2.20 against 1.61**.

That last number is the phase's most actionable finding. Our tree is not too
small, it is too **wide**: we spend nodes on breadth the reference spends on
depth. This is ordering, reduction and selectivity work — exactly cluster 5.4
— and it is why 5.4 goes first on evidence rather than merely on dependency
order.

Our HCE also measured meaningfully better than Rarog's on the same instrument
(+232.8 here against their +328.6, so ~96 Elo in our favour). That is a
cross-run comparison, not a controlled one — treat it as indicative. It does
support keeping the HCE track second, and it means Phase 5.9 starts from a
better base than Rarog's would have.

### 5.2 — Differential diagnostic harness

The former "bounded diagnostics and semantic census", **retained in full** and
widened to serve the acceleration work. Define a versioned fixed suite spanning UHO
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

### 5.2 — CLOSED 2026-08-12: the width is under-reduction, not mis-ordering

Substrate: 15 counters added to the existing 8.6.6 `DiagCounters`, a
machine-readable `info string diag kv` mirror, the fixed 107-position
`tools/diag/suite_v1.epd`, and `tools/diag/run_suite.py`. Gates: bench
**11,941,440** unchanged, CTest **12/12**, and diagnostics on reproduce the
same nodes, seldepth, PV and best move as diagnostics off.

Findings (BAS-D01/D02, `tools/diag/baseline_v1.json`):

| Signal | Value | Reading |
|---|---|---|
| First-move cutoffs | **89.10%** | ordering is already strong |
| Mean cutoff index | **0.214** | ordering is **not** the width source |
| LMR applied / eligible | **36.1%** | most eligible moves are never reduced |
| Reduction clamped to zero | **16.2%** of eligible | passed every gate, then reduced by nothing |
| Mean reduction when applied | 2.354 plies | |
| **LMR re-search rate** | **1.744%** | reductions almost never need undoing |
| Depth at 300k nodes | **20.80** vs oracle **32.87** | +12.07 plies on identical evaluation |

The re-search rate is the diagnostic. A well-tuned reduction policy pays for
its depth with visibly more re-searches; at 1.7% ours are so conservative they
are almost never wrong, which is exactly what under-reduction looks like. That,
plus a sixth of eligible moves reduced by zero, is where the width comes from.

Two consequences for cluster ordering:

1. **Move-picker rework leaves 5.4's likely content.** BAS-D01 refutes the
   ordering hypothesis we carried in. This is a saving: the work is expensive
   and would have been measured against a baseline that was already good.
2. **5.4.3, the reduction/re-search contract, is the cluster's centre.** 5.4.1
   and 5.4.2 remain as preconditions — histories feed the reduction formula, so
   they must still be coherent — but the payload is reductions.

**What this does not establish.** The counters localize the width; they do not
show that reducing more gains Elo, and durable lesson 5 cuts both ways — a
smaller tree can be worse. Prune recall is still uninstrumented, so "reduce
more" is a hypothesis with a mechanism, not a finding. Cluster 5.4 gates it on
games like anything else.

### 5.3 — Idea inventory and order freeze

Study Stockfish `9587eeeb`'s search, move-picking and evaluation to identify
**which problems it solves that Basilisk does not**, and map each to its
Basilisk owner. Classify every item as:

| Class | Meaning | Action |
|---|---|---|
| **Equivalent** | Basilisk already solves it, possibly differently | none; record and move on |
| **Intentionally different** | We solve it another way on purpose | record the reason; do not "fix" |
| **Missing** | A real capability gap | candidate for a cluster |
| **Coupled** | Only meaningful with a later consumer | defer to that consumer's cluster |

The output is a list of *problems worth solving*, ranked by the 5.2 diagnostic
populations — not a list of upstream functions to reproduce. For each candidate,
record what the mechanism must achieve and which Basilisk components it touches;
the design itself is the cluster's work, per the Independence contract.

Expect a healthy fraction of **intentionally different** and be suspicious of an
inventory that finds none: identical classification everywhere means the study
was transcription, not analysis.

If the evidence contradicts the provisional cluster order below, **edit this
plan before implementing** — never after seeing games.

### 5.3 — CLOSED 2026-08-12: `analysis/idea_inventory_v1.md`

Seven items classified against the reference, ranked by the 5.2 populations.
The full inventory, with what each item must achieve and why, is in
`analysis/idea_inventory_v1.md`; the headlines:

| # | Item | Class | Owner |
|---|---|---|---|
| 1 | **Reduction modulation is nearly inert** | Missing | 5.4.3 |
| 2 | Reduction eligibility excludes captures/checks | Intentionally different — suspect | 5.4.3 |
| 3 | Check extension is unconditional | Intentionally different — suspect | **5.4.4** |
| 4 | History pruning unreachable (142 fires in 15.1M nodes) | Missing consumer | 5.6 |
| 5 | Move ordering | **Equivalent — no action** | — |
| 6 | Reduction context we lack entirely | Missing (low rank) | 5.4.3, after item 1 |
| 7 | TT/qsearch/eval separation, aspiration, correction | Not yet inventoried | 5.5 / 5.8 |

Item 1 is where the width is. Basilisk has the right *shape* — every adjustment
exists and is wired — but the magnitudes are roughly an order of magnitude
smaller than the reference's, so context barely moves the reduction: **+0.39
plies at cut nodes against ~+2**, and −0.02 on a TT-PV line against ~−2. Those
values came from the `hcefinal` SPSA, which accepted +35.94 Elo, so they are not
arbitrary — they were fitted inside a search whose other constants were already
built around timid reductions. Durable lesson 2, exactly.

Item 5 is the useful negative: ordering is genuinely equivalent, so move-picker
rework leaves the cluster and is not measured against an already-good baseline.

**Cluster order amended, before implementation.** Check-move depth policy moves
from 5.7 to **5.4.4**. Item 3 and item 2 are two halves of one question, and
8.6.7 demonstrated that changing one alone loses ~10 Elo. Leaving the extension
in 5.7 would have 5.4 ship a reduction contract forbidden from touching a sixth
of all interior nodes, then have 5.7 test an extension change against a surface
refitted around the old exclusion — each half looking worse alone than the pair
is together. No new mechanism enters Phase 5; one moves so it can be adjudicated
jointly.

**Scope stated honestly.** Item 7 is deferred rather than done. Those surfaces
are what 5.4 is about to change, so inventorying them now would map a search that
is about to move; each cluster's audit step re-runs this exercise against a
current baseline.

### 5.4 — Cluster A: move ordering, histories, LMR

Implement in dependency order, as one coherent cluster:

- **5.4.1** move-picker contract: TT move, good captures, killers/counters,
  quiets, deferred bad captures; legality and duplicate guarantees.
- **5.4.2** evidence ownership: main, capture, continuation, low-ply and pawn
  history indexing, normalization, aging and cutoff attribution.
- **5.4.3** reduction/re-search contract: LMR population, improving/PV/cut-node
  adjustments, history feedback, zero-reduction floor, full-depth verification.
  **Three hypotheses tested and refuted before games (BAS-S13/S14/S15):**
  fractional history response, reference-scale context magnitudes, and the
  depth-ceiling explanation all moved depth-at-equal-nodes the wrong way or
  proved irrelevant. Reduction *magnitude* is not Basilisk's lever and this
  sub-step has no supported candidate. Do not retry a magnitude change without
  a new mechanism; the retry trigger for BAS-S13 is recorded in the ledger.
- **5.4.4** check-move depth policy — **REJECTED (BAS-S16).** SPRT accepted H0
  at the bound: −3.48 ±3.32 Elo over 17,058 games. Capping check extensions and
  permitting reduction of checking moves shrank the tree 28% and lost strength.
  Reverted; never committed active. Two inert switches: `CheckExtPathCap` bounds check
  extensions accumulated on one path, `LmrAllowCheck` lets checking moves be
  reduced. At `cap=2, allow=1`: **+0.458 paired ply** at equal nodes (43 better
  / 21 worse of 107) against **−6 WAC** at equal depth. The first monotonic
  lever this cluster has found.
  The unconditional check extension (15.84% of interior nodes) and the rule that
  checking moves are never reduced are two halves of one question: how much depth
  we spend on checks. 8.6.7 showed that changing one alone loses ~10 Elo, so they
  are adjudicated jointly, here, against the reduction contract that 5.4.3 sets.
- **5.4.5** integrated gate, then ablate surprising contributors.

### 5.4 — CLOSED 2026-08-13: no accepted gain, re-audit triggered

Both of the cluster's hypotheses failed and ordering needed no work, so the
cluster closes with **no change to the engine**. The accepted head is untouched
at bench 11,941,440.

| Sub-step | Outcome |
|---|---|
| 5.4.1 move picker | **Equivalent** — no work needed (BAS-D01: 89.1% first-move cutoffs) |
| 5.4.2 history channels | Precondition satisfied; all five feed the reduction |
| 5.4.3 reduction magnitude | **Refuted on the harness** before games (BAS-S13/S14/S15) |
| 5.4.4 check-move depth | **Rejected by games** — −3.48 ±3.32 over 17,058 (BAS-S16) |

Cluster discipline rule 7 has fired: stop and re-audit 5.2–5.3 rather than
continue down the list by sunk cost.

**What the cluster established.** The 12-ply gap at equal nodes is real, but it
is not reachable by pruning harder on the same decisions. Every attempt to
narrow the tree either failed to move the metric or measured worse in games.
The reference is narrow *and* strong, so its narrowness cannot be mere
aggression — it must come from better-informed decisions that let it prune
safely where we cannot.

Note the magnitude of the loss: a **28% smaller tree cost only −3.48 Elo**. The
depth-for-tactics trade is nearly balanced and merely sits on the wrong side of
zero. That is consistent with width being close to correctly priced *for the
quality of information we currently have*.

**Re-audited 2026-08-13 — see `analysis/reaudit_v1.md`. The hypothesis
recorded here was measured and REFUTED.**

BAS-O04 decomposed the depth gap on `suite_v1.epd` at a fixed 300,000 nodes by
holding one side constant at a time:

| Attribution | Ply | Share |
|---|---:|---:|
| **Search** (our evaluator held constant) | **+12.07** | **95.9%** |
| Evaluation (SF search held constant) | +0.51 | 4.1% |

An evaluator **+232.8 Elo stronger** buys half a ply and is not even
consistently deeper (36 positions better, 42 worse). Width is a search-policy
property by roughly 23 to 1, so "width is a symptom of evaluation quality" is
withdrawn.

The reconciliation with BAS-S16 is that 5.4 aimed at the right subsystem with
the wrong levers. The reference is narrow because its decisions are better
informed **at the point of pruning**, not because its margins are more
aggressive; pruning the same decisions harder is blindness, and games priced it
accordingly.

Consequences, from the re-audit:

- **5.3 item 7's deferral is lifted.** It was deferred because those surfaces
  were about to move under cluster 5.4; 5.4 changed nothing, so the reason has
  expired rather than renewed. The uninventoried selectivity, qsearch and TT
  contracts are now the main body of remaining work.
- **5.6 gains expected value rather than losing its slot** — the opposite of
  what I suggested when 5.4 closed. Selectivity is where the uninventoried
  width mechanisms live, and width is 96% search policy.
- **5.5 is retained but re-motivated**: not "fix the margins' information
  quality", which BAS-O04 kills, but provenance correctness (durable lesson 6),
  the TT contracts selectivity depends on, and qsearch — **35% of all nodes**,
  8.09M of 23.2M, never inventoried against the reference.
- **Order 5.5 → 5.6 stands.** Every margin is measured against a pruning
  evaluation, so separation has to exist before margins mean anything.
- **History pruning enters 5.6 as its first candidate**: 142 fires in 15.1M
  interior nodes, a width mechanism indistinguishable from absent.

**Budget honesty.** Phase 5 has now spent three sub-steps for no strength.
That is within tolerance — it is what the stop rules buy — but if 5.5 and 5.6
also close empty, re-open whether the remaining Phase-5 budget is better spent
going to NNUE. Do not invent a fourth cluster to avoid that question.

This cluster owns the latent post-move `gives_check` LMR defect. Durable lesson
2 applies directly: its standalone repair lost −21.55 ±9.83 because the old
surface was tuned around it. Repair it **inside** this cluster and fit jointly.

### 5.5 — Cluster B: static eval, TT and quiescence

Separate raw evaluation, pruning evaluation and searched bounds; align TT
capabilities, qsearch stand-pat, capture ordering and check handling, and
correction attribution. Preserve Basilisk's proven draw, mate-distance and
rule-50 semantics — these are correctness assets, not targets to change.

### 5.5 — CLOSED 2026-08-13: no candidate, contracts already sound

Full audit in `analysis/cluster55_audit_v1.md`. The lifted 5.3 item-7
inventory for this surface found every contract equivalent or intentionally
different for a recorded reason. **No engine change**; bench 11,941,440.

| Contract | Verdict |
|---|---|
| Raw / corrected / TT-refined eval separation | **Equivalent** — and we hold correction history, which `9587eeeb` lacks entirely |
| Mate-range clamp on the TT refinement | Present, with its reason recorded in place |
| Qsearch provenance | **Equivalent** — mirrors the main search exactly |
| Qsearch structure (MVV+caphist, delta, SEE, evasions) | **Equivalent**; uncapped evasions are a deliberate correctness choice |
| Qsearch quiet checks | **Intentionally different**, inert — matches current reference behaviour |
| **Qsearch share of nodes** | **30.8% against the reference's 36–37%** (BAS-D03) — ours is *smaller*; not a width source |
| TT layout | Mature: dense 10-byte, 3/cluster, lock-free |
| Persisted TT-PV bit | **Missing but adjudicated** — costs an age bit (barred by 5.2), and the 8.5.7 re-test measured +51% nodes with no operating point |

Instrumenting the reference was the decision the re-audit named. It was done on
a **derived branch `hybrid-diag`**, leaving the frozen oracle at `01df815` with
its tournament binary untouched — diagnostic work that would change the oracle
uses another branch.

This closing empty is a finding, not a failure: sound contracts here are what
make the remaining width attributable to 5.6's mechanisms by elimination.

**Budget note.** PLAN's honesty clause names 5.5 and 5.6. 5.5 is now one of the
two. 5.6 has a concrete quantified target — history pruning at 142 fires in
15.1M interior nodes — so it is not a hopeful step. If it also closes empty,
honour the clause and re-open going to NNUE rather than arguing around it.

### 5.6 — Cluster C: main selectivity

In observed dependency order, reconcile razoring, reverse futility, null-move
verification, ProbCut, move-count and history pruning, and quiet/capture
futility. Use prospective searched depth consistently. Gate categorical
architecture before any narrow constant fit; do not launch a broad SPSA.

### 5.6 — CLOSED 2026-08-13: no candidate; the budget clause now fires

Full audit in `analysis/cluster56_audit_v1.md`. Engine unchanged, bench
11,941,440, CTest 12/12.

**History pruning is genuinely defective and still not a candidate.** Its
threshold `coeff * depth` is compared against a sum of six bounded history
channels whose maximum is 81,920, so the depth-6 threshold of 84,024 is
**provably unsatisfiable** and depth 5 needs 85% of theoretical maximum negative
on every channel at once. It fires 142 times in 5,355,599 tested quiets. The
`hcefinal` SPSA stranded it above the distribution it was meant to cut.

Loosening would activate a real population (`coeff/4` → 234,235 fires, 4.4%),
but paired depth at 300k nodes is flat at every value tested (−0.019, +0.037,
−0.019). Pruning 4–9% more quiets for no depth is exactly the trade BAS-S16
priced at −3.48 ±3.32 Elo. Recorded with a retry trigger (BAS-D04) rather than
gated — a coefficient change alone does not reopen it.

**ProbCut's 0.4% is correct rarity, not a defect** — 56,311 cuts from 84,469
tries, a 67% hit rate. Every other selectivity mechanism has a healthy
population.

**A harness defect was found and fixed in the same work**: `print_diag` built
its kv line into `char buf[256]` and silently truncated the tail field, so the
corruption scaled with counter magnitude. Caught only because the threshold
series has a monotonicity invariant that made the result visibly impossible.
Buffer now 512, probe on its own line.

### Phase 5 budget clause — FIRED, decision belongs to the maintainer

The clause named 5.5 and 5.6. **Both have closed empty**, and it says to
re-open the question rather than argue around it.

The record so far:

| Step | Outcome |
|---|---|
| 5.1 oracle | search gap +322.7, evaluation gap +232.8 |
| 5.2 harness | built; ordering healthy, width localised |
| 5.3 inventory | seven items classified |
| 5.4 cluster A | **no change** — 3 hypotheses refuted, 1 SPRT lost |
| 5.5 cluster B | **no change** — contracts already sound |
| 5.6 cluster C | **no change** — one real defect, not worth gating |

Three clusters, one lost SPRT, no strength. The search gap is real and
measured, but has not yielded to the levers Phase 5 was scoped to use.

**Options, for the maintainer:**

1. **Continue to 5.7/5.8** (extensions, root/clock). Both are re-fitted anyway
   at Phase 8.3 once NNUE changes the score scale, so their value here is
   partly borrowed against that step.
2. **Close the strength track and finish Phase 5 as maintenance** — run 5.10
   correctness, 5.11 portability/ISA, 5.12 SMP, release **1.9.4** at 5.13, then
   go to Phase 6/7 NNUE. 5.9 is already re-scoped to roughly six minor terms
   and the evaluation gap is NNUE's to close.
3. Something else the evidence supports.

Option 2 is what the evidence favours, but this is a scope decision and belongs
to the maintainer, not to this document.

### 5.7 — Cluster D: extensions and depth authority

Reconcile singular, double/negative extension and IIR semantics against TT
provenance and LMR. Preserve mate and abort correctness. Gate the integrated
contract, not the individual extensions.

**5.7.1 Contract inventory — DONE 2026-08-30**, `analysis/cluster57_audit_v1.md`.
Precondition confirmed satisfied: 5.4 closed with ordering measured healthy
(BAS-D01, 89.1%) and 5.5 with TT provenance equivalent.

*The audit reframes the cluster.* Double extensions, negative extensions and
IIR-on-stale-TT all **post-date** `9587eeeb` — the vendored reference is the
last pure-HCE master and is older than parts of our search. We are not behind
it here; we are a **blend of eras**, and 5.7's job is **internal coherence**,
not catching up.

Four candidates, in expected-value order:

1. **`singularQuietLMR` — ABSENT.** The reference reduces LMR for a move it just
   extended; we do not. This is exactly the "extension and reduction decisions
   arbitrated against a settled LMR" the cluster is defined by. Cheapest and
   most directly on-mandate.
2. **Check-and-singular stacking.** Our check extension is unconditional and
   applied to `depth` *before* the move loop, so a checking node with a singular
   TT move can take **3 plies** where the reference allows **1**. `ss->check_exts`
   is already propagated and consumed by nothing. Care required: 5.4.4 lost
   BAS-S16 (−3.48 ±3.32) to a naive path cap, so this is not a re-run of that.
3. **`ttValue >= beta` semantics.** The reference runs a second verification
   search and can return a cutoff; we apply a negative extension. Different
   mechanisms, not variants — a design decision to take, not a port.
4. Singular gate depth 5 vs 6 — one constant, tested only after 1–3 settle.

Missing minor extensions (passed pawn, last captures, castling) are inventory
only. Adding unmeasured extensions to a search whose extension *semantics* are
unsettled is the ordering error the precondition rule exists to prevent.

**Diagnostics re-measured before ranking (BAS-D09).** Every metric the ranking
rests on was taken pre-5.9; ordering and LMR are unmoved, branching holds, and
**BAS-D03's qsearch finding is stale** — 30.8% → 35.1%, now inside the
reference's 36–37% band, so "ours is smaller" no longer holds.

**Sub-steps — every candidate numbered so none is silently dropped.**

- **5.7.2 `singularQuietLMR` — READING TAKEN, kept provisionally (BAS-D10, BAS-D12).**
  Gate stopped by decision at 24,956 games: **+1.49 ±2.77 Elo, LOS 83.52%**, LLR
  drift implying ~149,000 games to resolve. Not a regression, plausibly a small
  positive. **NOT accepted** — it carries into 5.7.7's integrated gate and comes
  back out if that fails.
  A singular TT move relaxes LMR for that node's *remaining* moves. Verified
  inert at 0 (bench exact), swept, shipped at **401** — the reference's
  full-ply value measured **−0.215 ply** and broke a mate test.
- **5.7.3 Check-and-singular stacking — REFUTED 2026-08-30 (BAS-D11).**
  Measured, implemented behind an inert knob, swept, and reverted without
  spending games. Exclusivity — the reference's rule — **fails the WAC floor**
  (137 → 124 against a floor of 130) while gaining +0.065 ply; the intermediate
  setting costs 5 solved positions for noise. Our check extension is per-node
  and unconditional where the reference's is per-move and gated on
  discovery-or-SEE, so removing our composition removes strictly more. **The
  3-ply stack is also rarer than the audit implied: 0.123% of interior nodes.**
  Composition stays; the knob was removed rather than parked.

  *Two findings carried out of it:* `singular_double_margin = 4` lets **53.52%**
  of singular extensions take the double, but tightening it measurably gains
  nothing (margin 25 → −0.009 ply, 40 → +0.000). And **`double_ext_max` is dead
  code** — capping at 16 versus its 200 default changes nothing on any of 107
  positions, so the Phase 6.4 path cap has never bound. That goes to 5.7.6.

- **5.7.3 (original text).** Our check extension is unconditional
  and applied to `depth` before the move loop, so a checking node with a
  singular TT move takes up to **3 plies** where the reference allows **1**.
  `ss->check_exts` is already propagated and consumed by nothing. Not a re-run
  of 5.4.4's path cap (BAS-S16, −3.48): that tested a budget, this tests
  whether the two extensions should compose at all.
- **5.7.4 `ttValue >= beta` semantics — REFUTED 2026-08-30 (BAS-D13).**
  Implemented behind an inert knob and measured: depth mean **+0.383** ply but
  **median +0.000**, better/worse **27/25**, the mean carried entirely by three
  trivial pawn/king endgames (+11, +11, +9). WAC 138 vs 137, noise. No broad
  gain, and the reference's form costs an extra search per firing. **Our
  negative extension stays** — a decision on evidence, not a gap left open. The
  branch's reach is 0.1775% of interior nodes.

- **5.7.4 (original text).** Reference runs a second verification
  search and can return a cutoff; we apply a negative extension. Two mechanisms,
  not two variants — decide which we want, do not port.
- **5.7.5 Singular gate depth 5 vs 6.** One constant, tested only after
  5.7.2–5.7.4 settle, since it interacts with all of them.
- **5.7.6 Dead instrumentation and inert switches.** `ss->check_exts` is
  propagated and unread; `check_ext_path_cap` and `lmr_allow_check` are 5.4.4
  leftovers sitting at 0; and **`double_ext_max` is measured dead** (BAS-D11) —
  its cap never binds at any value ≤ 200. Either 5.7.3 consumes them or they come out — an
  untested mechanism parked in the code is a trap for the next reader.
- **5.7.7 Integrated gate.** PLAN requires gating the integrated contract, not
  individual extensions. Once 5.7.2–5.7.6 have individual readings, the set that
  survives goes to **one** SPRT together.

Minor extensions (passed pawn, last captures, castling) stay out of the
numbering: they are additions, and this cluster is about coherence.

**Check extensions are no longer owned here** — the 5.3 inventory moved them to
5.4.4, because they are inseparable from the never-reduce-checking-moves rule.
What remains here genuinely depends on cluster 5.5's TT provenance: singular
verification needs a trustworthy TT move with sound depth and bound.

### 5.8 — Cluster E: root search and clock handoff

Reconcile aspiration retries, completed-root authority, PV/fallback ownership
and stability inputs. Total time allocation must not move until the root
evidence is coherent; then gate any real-clock change separately under the
time/root/SMP evidence rule.

### 5.9 — HCE maturity program — UNFROZEN 2026-08-25 by maintainer decision

**Scope decision.** The HCE is no longer frozen. It is to be brought to
maturity: new mechanisms added, then re-fitted by Texel, and SPSA is permitted
where it earns strength. This supersedes the constant-refit freeze, the
"structural convergence only" scope of 2026-08-13, and the pre-NNUE broad-tune
ban **for evaluation work specifically**. Search SPSA remains at Phase 8.3.

**The evidence this must respect.** Three recorded results argue against a
naive retry, and the step is designed around them rather than in spite of them:

| Evidence | What it forbids |
|---|---|
| HCE cycle 6 washed at **+1.37 ±5.21** over 8,100 games | Refitting the **same feature set** again. That surface is exhausted. |
| BAS-E07: coverage near-complete, r = 0.790, 17% sign disagreement | Expecting new *features* alone to close −232.8 Elo. The gap is in values. |
| BAS-X11: Manta's `MAN-E05`/`MAN-E07` lost ~−23 Elo between them | Adding reference-family terms with **hand-set coefficients**, each individually gated. That exact design lost twice. |
| BAS-X14: a linear static objective misprices capped, squared and truncated terms | Assuming Texel can fit the king-safety funnel. It cannot. |

The distinction that makes this a genuine retry rather than a repeat: **cycle 6
refit the same features; this refits an enlarged feature set.** A fit surface
with new terms is not the surface that washed.

**Ordered sub-steps.**

- **5.9.1 Coverage close-out.** Implement the six terms BAS-E07 found absent —
  `BadOutpost`, `BishopXRayPawns`, `LongDiagonalBishop`, `KnightOnQueen`,
  `SliderOnQueen`, `TrappedRook` — plus the "safe square" qualifier on our pawn
  threats. Land them **seeded inert or at provably neutral values**, gated on
  deterministic evidence only: tuner trace correctness, colour and phase
  symmetry, special-move cases, an independently ablatable switch, and a stated
  throughput budget. **No individual SPRT** — that is the design BAS-X11 shows
  losing.
- **5.9.2 Mechanism search beyond coverage — DONE 2026-08-25.** Audited our term
  *shapes* against the reference's, not just presence. Most already match: our
  threat-by-minor and threat-by-rook are graded arrays over the attacked piece
  type, our mobility is a per-count table per piece, our rook-on-file is
  decomposed into open and semi-open, and hanging is indexed. Two genuine
  simpler-form defects found and repaired:

  - `king_protector` was **one scalar shared by knights and bishops**, so no fit
    could ever learn that the two pieces value king proximity differently. Split
    per piece type, both seeded at the shared value so behaviour is unchanged.
  - **Bishop outposts were never priced.** The concept existed for knights only;
    a comment in the bishop loop claimed otherwise and was stale. Added beside
    the knight's in the cheap block, so lazy eval treats both minors alike —
    pricing one before the checkpoint and the other after would make the pair's
    relative value depend on whether the lazy margin fired.
- **5.9.3 Structure freeze — DONE 2026-08-25.** Maturity verdict in
  `analysis/hce_maturity_v2.md`.

  **Named term coverage is now at reference parity**: all 31 named terms have a
  counterpart, and the shapes match — threats graded by attacked piece type,
  mobility as per-count tables, rook-on-file decomposed, hanging indexed,
  king-protector per piece. Nothing is a scalar standing in for a table.

  **Endgame knowledge is partial and stays that way for now.** We carry six
  rules — KNNK, KPK bitbase, KBNK drive, wrong-rook-file KBP, no-pawn ≤ minor
  scaling, opposite-coloured bishops — against roughly 29 reference classes.
  Absent: rook-ending scaling, rook vs pawn, queen vs rook, rook vs minor, the
  bishop-pawn scale family, and a generic bare-king drive.

  Not added before the fit, deliberately: endgame rules are scale factors and
  exact evaluations, so BAS-X14 puts them in the *excluded* set where the fit
  cannot price them — the whole reason 5.9.1/5.9.2 landed inert does not extend
  here. `MAN-E05` lost −16.32 Elo on exactly this class. And an endgame rule
  cannot be seeded inert the way an additive term can: a scale factor of 1.0 is
  inert, but the recogniser that selects it is behaviour from the first line,
  which is how eight consecutive mechanisms tripped the KBNK/KQK canaries.

  Freezing an incomplete structure is normally the ADR-0050 error, but that
  warning is about *fittable* structure and the linear surface is complete.
  Endgame rules sit outside the fit, so adding them later does not invalidate
  these coefficients.

  **Retry trigger:** open the endgame program only after 5.9.6 returns a
  verdict, and design it recogniser-first — decide whether an ending is winnable
  before grading how well it converts. That is the correction `MAN-E05`'s own
  post-mortem demanded and never received.
- **5.9.4 Joint Texel refit — DONE 2026-08-25 (BAS-E08).** **No new datagen needed.** The existing
  corpus already satisfies this step: `beast_sf_all_train.csv` holds **116.5M**
  labelled rows with a 6.1M holdout, built by the pipeline this step would
  otherwise rebuild — Beast pool supplies starts, Basilisk self-play produces
  white-perspective WDL labels, extraction quiet-filters, dedups and balances
  across five phases. The new terms fire on it 1,173–14,234 per 20,000
  positions, so the surface is exercised.

  Manta's positions were considered and are **not** worth importing: their
  corpus is smaller, labelled by a weaker engine's self-play, and reusing their
  FENs would still require replaying them to obtain our own labels — which is
  the expensive half. There is no saving.

  The one real caveat is that these labels come from an older Basilisk's
  self-play, so they are slightly off-policy for 1.9.3. That is accepted for now
  because it costs nothing to try: if 5.9.6 rejects, fresh on-policy datagen is
  the first retry, not the first move.

  Group membership was verified rather than assumed: all fourteen new registry
  entries sit at indices 62–81, inside the `scalars` range (17–156) and clear of
  the king-safety skip (128–148), so `--tune scalars` reaches every one. A
  3-epoch smoke fit moved all of them off zero and already separated
  `KingProtectorN` from `KingProtectorB`, which is what 5.9.2's split existed to
  make possible.

  **Result, and the ablation matters more than the headline.** Holdout loss fell
  6.2% (0.0703086 → 0.0659483), 348 params, `--l2 1e-6`. But applying **only**
  the twenty new-term values, existing params untouched, gives **0.0703113**
  against a 0.0703086 baseline — marginally worse, inside noise. **The added
  structure carries no independent signal; the whole gain is the refit of the
  pre-existing surface.**

  That is recorded as a *weakened* prior for 5.9.6, not a strengthened one.
  Refitting the existing surface is close to what cycle 6 did, and cycle 6
  washed at +1.37 ±5.21 over 8,100 games. The claim that made this a genuine
  retry — an enlarged surface is not the surface that washed — does not survive
  the ablation intact. The loss number cannot rescue it: holdout-MSE-delta does
  not predict Elo (durable lesson), and BAS-X02 improved holdout 4.9% while
  losing −17.11 Elo.

  Endgame moved **0.4%** against opening's 12.4%, independently confirming
  5.9.3: the gap is endgame knowledge and the linear surface cannot reach it.

  **A cost is carried into the gate, but it is small — the first reading here
  was wrong.** Bench moves **11,941,440 → 15,655,764**, and that +31% was first
  written up as a ~2.5× per-iteration deficit. **BAS-E09 measured it and it does
  not hold.** Bench counts nodes to a *fixed depth 13*; two evaluators that
  disagree (251 against 281 on the last bench position) diverge in aspiration
  windows and TT behaviour, so a large bench delta between different evaluators
  is expected and is not itself an efficiency regression. NPS is unchanged, so
  it is not a costlier evaluator either. Paired depth at equal nodes over 107
  positions gives **+0.019 ply at 300k and −0.168 at 1M** (−0.250 excluding
  mate runaways). The real headwind is **about a quarter ply**, a small
  single-digit Elo cost — worth stating, not enough to predict rejection.

  The BAS-X14 classification held: the king-danger funnel, capped winnability and
  truncated tables stayed outside `scalars` (indices 128–148 skipped), so no
  coefficient was fitted through a cap. Those are 5.9.5's subject.
- **5.9.5 King-safety fit — DONE 2026-08-26 (BAS-E10), variant A.**
  The excluded set from 5.9.4 — principally king safety — is where a *linear*
  static objective fails. The plan previously jumped straight to SPSA. That was
  wrong about the available instruments: our own tuner already carries
  `--tune-kingsafety`, a **coordinate descent over 40 integer knobs** —
  `ks_unit`, `ks_safe_check`, shelter/storm, flank, and `safety_table[2..24]`
  under a monotonicity constraint — scored against the **real nonlinear
  evaluator**, with holdout-best restore. BAS-X14's objection is to *linearity*,
  not to static objectives, and this instrument is not linear. It is also the
  established path that produced the shipped king-safety values, and it costs
  CPU hours rather than an SPSA's thousands of games.

  **Result.** Converged in 83 passes, best holdout restored from pass 75,
  holdout 0.0658991 → 0.0652499 (−0.99%). It found `ks_unit[ROOK]` and
  `ks_unit[QUEEN]` both sitting at **0** — rook and queen attacks contributed
  nothing to the attack-unit count — and fitted both to 2.

  **Only variant A ships: the eleven scalar knobs, `safety_table` reverted.**
  Baking the full fit fails the mate-drive canary, collapsing the edge
  preference from 29cp to 4cp against a `>20` threshold; bisection isolated it
  to `safety_table` alone. The cause is corpus coverage — the corpus is
  quiet-filtered and carries no forced-mate positions, so the objective has no
  signal there. It also exposed that the canary was passing partly by accident:
  the mate-drive's own contribution in that position is ~15cp, and king safety
  was incidentally supplying the rest. **The table reshape is deferred to
  5.9.12**, and 5.9.11's corpus must carry mating material so the fit can price
  it. Bench 15,655,764 → 18,228,447; paired depth −0.196 ply, unchanged from
  5.9.4 alone.

  SPSA is still the escalation if it stalls — meaning it either finds no holdout improvement, or
  finds one that 5.9.6 then rejects. If SPSA is escalated to, it runs under
  PLAN's SPSA doctrine unchanged: registered horizon, bounds and stop rule
  before launch, ≥5,000 iterations, no post-hoc tail selection. Either way this
  is the answer to BAS-E07's finding that the reference's advantage is
  calibration, not features.
- **5.9.6 One promoting gate — REJECTED 2026-08-26 (BAS-E11), −77.92 ±15.32
  Elo, nElo −99.71, LLR −2.95 → H0 in 1,292 games.** Every hypothesis was tested
  without further games and none explains it: static holdout was **6.7% better**,
  WAC was unchanged (49 fails vs 48), the lazy audit showed **zero sign flips**
  and *fewer* margin crossings. Identified costs — 4.1% NPS, +52.6% bench nodes,
  −0.196 ply, 4.1% scale compression — sum to perhaps 15–20 Elo against 78.

  **The residual is the finding: the fitted values are worse in play while
  better on the corpus.** The corpus is quiet-filtered and off-policy, and quiet
  filtering removes exactly the positions where king safety governs, so the fit
  set those values wrongly at no measured cost. BAS-E10 found the same blindness
  for mating positions. One defect, two symptoms — **the corpus lacks the
  position classes these terms exist to price**, which is why 5.9.11 is now the
  load-bearing step rather than a tidy-up.

  **Disposition.** Values reverted to baseline; the structure is retained
  unbaked for 5.9.12's ablation, per the amended stop rule. The speed work
  (BAS-E12) is kept — it is behaviour-neutral and applies to any future
  candidate. The revert is **provably** the 1.9.3 engine (BAS-E13: 0 move and 0
  node mismatches over 107 positions), so no confirmation SPRT was spent.
  Endgame steps 5.9.7–5.9.10 **do not open**.

- **5.9.6 (original text)** A single registered SPRT of the cumulative
  evaluator against the accepted head, then targeted post-fit ablation to
  attribute a surprising result. Individual terms are not separately gated.

**Endgame knowledge — numbered, and deliberately after the fit.** 5.9.3 recorded
why these cannot ride on the linear fit: they are scale factors and exact
evaluations, so BAS-X14 puts them in the excluded set, and an endgame rule
cannot be seeded inert because the recogniser that selects a scale is behaviour
from its first line. `MAN-E05` lost −16.32 Elo on this class. They therefore get
their own steps, opened only once 5.9.6 has a verdict:

- **5.9.7 Recogniser inventory and risk order.** Enumerate the absent classes —
  rook-ending scaling (`KRPKR`, `KRPKB`), rook vs pawn, queen vs rook, rook vs
  minor, the bishop-pawn scale family, generic bare-king drive — and order them
  by *how often a fast-TC game actually reaches them*, not by how interesting
  they are. Frequency comes from the corpus, measured, not assumed.
- **5.9.8 Recognisers before grading.** Implement classification only: rules
  that answer "is this ending winnable / drawn / scaled", with no conversion
  grading on top. `MAN-E05`'s post-mortem is that it graded conversion while
  having no recogniser able to say whether an ending was winnable; this step
  exists so that cannot repeat. Deterministic evidence: exact-result tests
  against known theory, colour and phase symmetry, and **the mate canaries**,
  which this class has broken eight times.
- **5.9.9 Grading on top, if 5.9.8 holds.** Only after recognisers are accepted
  may conversion grading be added, and only for classes a recogniser already
  classifies.
- **5.9.10 Endgame gate.** One registered SPRT for the endgame block, with the
  TC ladder — endgame knowledge is exactly the class that shows at LTC and
  hides at STC (BAS-M07), so an STC-only verdict is not sufficient here.

If 5.9.6 rejects, these steps do not open: a failed calibration is not repaired
by adding a harder-to-fit class of knowledge on top of it.

**Execution order for the rest of 5.9** (the numbers are identifiers, not the
running order — later-numbered steps run first here, deliberately):

1. ~~**5.9.11**~~ — closed, no arm passed.
2. ~~**5.9.15 LTC probe**~~ — closed, no depth story.
3. ~~**5.9.14 king-safety reshape**~~ — **ACCEPTED, +2.64 Elo.**
4. ~~**5.9.12** / **5.9.13**~~ — **ACCEPTED, +9.52 Elo.**

**Phase 5.9 is done bar cleanup: +2.64 and +9.52, about +12 Elo total.** What
remains is not more fitting but two tidy-ups and a decision:

5. **Remove the refuted 5.9.1/5.9.2 terms** — inert in three separate
   measurements, 12 of 20 fitted to exactly zero. A `-Mode simplify` gate, never
   bundled with a gain. Also recovers the NPS the zero-guards cannot reclaim
   while eight values are non-zero.
6. **`winnable` finite-difference instrument** (optional) — the last 7 reachable
   parameters, independent of everything else, needing a small coordinate-descent
   mode like `--tune-kingsafety`.
7. Then **5.7 / 5.8**, the deferred search work, where BAS-O01 measured a
   **+322.7 Elo** gap with our own evaluation on both sides.

5.9.12 goes last because it is the largest and slowest, and because both steps
before it feed into it: the LTC probe says whether to weight endgame terms, and
the king-safety result determines what the non-Texel half of 5.9.12 starts from.

**On-policy refit of the whole surface — numbered, and unconditional.** These
two run **whatever 5.9.6 returns**. They are not a retry of a rejected gate;
they close two defects in how 5.9.4 was executed, both found after the fact.

*Defect one: two thirds of the surface was never fitted.* 5.9.4 used
`--tune scalars` and moved **348 of 1,190** fittable parameters — 29%. The
**768 piece-square-table** coefficients sat frozen through the entire program.
This is the leading candidate explanation for BAS-E08's ablation, where the new
terms measured inert: `bad_outpost`, `bishop_outpost`, `long_diagonal_bishop`
and `knight_on_queen` are all square-and-geometry dependent, and a PST is the
most expressive geometry container in the evaluator. A frozen PST that already
absorbs that signal leaves a new scalar term nothing to claim, and the ablation
would read exactly as it did. Coverage was assumed rather than checked.

*Defect two: the labels are off-policy.* They come from an older Basilisk's
self-play, so they describe a distribution the candidate no longer produces.

**The parameter taxonomy. Every fittable parameter has a home, and `--tune all`
is not the way to reach them.**

| class | count | instrument | why |
|---|---:|---|---|
| `scalars` | 348 | Texel gradient | well-behaved, uncapped |
| PSTs | 768 | Texel gradient | well-behaved; **never fitted since Phase 4.7** |
| **Texel total** | **1,116** | | |
| `kingsafety` | 57 | coordinate descent | capped nonlinear funnel (BAS-X14) |
| `winnable` | 7 | finite difference | capped; tuner's own help marks it finite-diff |
| material | 10 | **pinned, not fitted** | exactly collinear with PST |
| total | **1,190** | | |

Do **not** run `--tune all`. It is a trap in both directions: it reaches 1,183
parameters, sweeping the 57 king-safety coefficients into a gradient fit that
corrupts them, while silently **omitting the 7 `winnable` params**, which sit
after `EPG_Tempo` and fall outside its range.

Material is pinned and this costs **zero expressiveness**: `eval.cpp:399` builds
`MG_TABLE = mg_val[pt] + pst_mg[pt-1][sq]`, so adding a constant across a
piece's 64 PST squares *is* a material change. Fitting both is a perfect null
direction — rank-deficient by construction. Pinning removes the degeneracy
without removing any reachable evaluator.

- **5.9.11 Regenerate the corpus on-policy.** Full extract → label → pack cycle
  driven by the **current** binary, replacing the older Basilisk's self-play
  labels. Same pipeline as the existing corpus — Beast pool starts, self-play
  WDL, quiet filtering, dedup, five-phase balance — so the only variable that
  changes is the policy that produced the labels. Record row count, holdout
  split and the generating revision.

  **Run as a three-arm label-source experiment (BAS-E17), registered before
  launch.** The single variable is which engine's games produce the WDL labels:
  **A** Basilisk at 8,000 nodes, **B** Stockfish dev-20260716 at 8,000, **C**
  Basilisk at **25,000**. 125,000 rounds each, identical starts, adjudication
  none, identical extraction, and the same 348-parameter `scalars` fit 5.9.4
  used — so a pass directly confirms the BAS-E11 correction. Arm C separates
  *label quality* from *label source*: it buys less-noisy outcomes from our own
  engine, with none of the transfer risk. Decision rule and prediction are
  registered in BAS-E17 and must not be revised after results.

  Whichever source wins carries into 5.9.12's full-surface fit. The underlying
  design in all three arms is:

  **Beast positions as STARTS, self-play game results as LABELS, adjudication
  none, 8,000 nodes/move** — `beast_seed_2m.epd`, which already
  holds 2,000,000 sampled positions.

  **The label change is the important one.** The old `beast_sf_*` corpus is
  **Stockfish-distillation labelled** — `import_beast.py` reads `FEN<TAB>target`
  where the target is a Stockfish expected score, and 200,000 rows contain 427
  distinct target values on [0,1]. 5.9.4 fitted 348 coefficients to reproduce
  *Stockfish's static evaluation*, and BAS-X02 already recorded what that costs:
  holdout −4.9%, Elo **−17.11** in Rarog. Every row from here on carries the
  white-perspective **result of its own game** (BAS-E11 correction, BAS-E16).

  **Starts, not an opening book.** An opening book makes every game traverse an
  opening before it can reach an endgame, buying the scarce classes at ~60 plies
  each. Beast starts span all phases. Measured (BAS-E16): deep_endgame
  **2.612 → 3.018** per game and endgame **5.105 → 5.335**, and the binding
  constraint moves off `deep_endgame` onto `opening`, which we have in
  abundance — **116,528 games needed against 173,036**, a third fewer.

  This adopts Manta's `HCE_DATAGEN` design, which was reviewed and wrongly
  rejected earlier in Phase 5 on the grounds that importing their data saved no
  compute. That was the wrong question; the method costs the same and yields
  better data.

  Two earlier designs were tried and discarded. A UHO-only slice was specified
  but never run, superseded by the start-based design above. Before it, a
  three-slice design using `SuperGM_4mvs` for the bulk **failed on duplication**
  (BAS-E15): fixed-node self-play with the same binary
  on both sides is deterministic, that book holds only 2,668 openings, and
  80,000 rounds against it produced 2,668 distinct games repeated 15 times —
  93.3% duplicates. Rounds above book size buy nothing.

  Rounds above book size buy nothing. `beast_seed_2m.epd` carries 2,000,000
  distinct starts, so duplication is structurally impossible here.

  Adjudication stays `none` for the whole run, not for a special slice. BAS-E15
  measured why in positions per game, and it removes adjudication's label error
  as well, for ~19% more wall time.

  **Biases to watch.** Beast starts are positions a *different* engine reached,
  so they are off-policy as starts — accepted deliberately, because the labels
  are now ours and breadth of coverage is the point. Extraction still applies
  the quiet filter, which is correct for fitting a static evaluation.

  **It must additionally carry mating and near-mating material.** BAS-E10 showed
  the existing quiet-filtered corpus has no forced-mate positions at all, so the
  objective is blind there and a fit will happily destroy mating behaviour to
  buy loss elsewhere — which is exactly what happened to `safety_table`. Include
  a deliberate slice of bare-king and near-bare-king endings so the objective
  can price them, and verify the mate-drive canary against the fitted result
  rather than trusting the aggregate loss.

- **5.9.12 Full-surface Texel refit.** Fit the 1,116-parameter Texel set jointly
  on the 5.9.11 corpus, material pinned. Then re-run the 64 non-Texel
  coefficients on their own instruments and **iterate** — the two sets interact,
  so one pass of each is not a converged fit.

  Two checks are mandatory, not optional:

  1. **Score-scale audit.** Texel refits `K`, so a fit can rescale the entire
     centipawn scale at constant WDL loss and let `K` silently absorb it. Our
     futility, razoring and delta-pruning margins are centipawn constants
     calibrated to the current scale, and BAS-M05 records the resign threshold
     as engine-scale dependent — mechanism and consumer constants are one
     system. Compare the fitted `K` against the baseline (5.9.4 used
     **1.41868**) and report effective piece values as `mg_val + mean(PST)`. A
     moved `K` means the margins need re-examining, not that the fit failed.
  2. **Repeat the BAS-E08 ablation.** Apply only the new terms' values against
     the refit surface. If they are still inert once PSTs are free to move, the
     terms are genuinely redundant and should be considered for removal rather
     than carried as dead weight.

  SPSA remains available for whatever neither instrument prices, but it is not
  scheduled here: it is the escalation, on evidence, under PLAN's SPSA doctrine.

*Why this is not another cycle-6 repeat.* Cycle 6 and 5.9.4 both refit the
**same 348-parameter subset on the same off-policy labels**. 5.9.12 changes both
variables at once — 3.2× the parameters, and labels from the current policy.

- **5.9.14 King-safety `safety_table` reshape — ACCEPTED 2026-08-28, +2.64 ±2.05 Elo (BAS-E21).** The first and so far only gain of Phase 5.9. 5.9.5's coordinate descent found a sharply convex reshaping of
  `safety_table` (low end roughly halved, high end up ~45%) worth −0.99% holdout.
  **It never received an Elo verdict.** It was withdrawn because it collapsed the
  mate-drive canary from 29cp to 4cp — and BAS-E14 then showed that failure was
  caused by the corpus containing no mating positions at all, which 5.9.11 fixed.

  So: re-run `--tune-kingsafety` on the 5.9.11 corpus, bake the **whole** result
  including the table, verify the canary, and gate it. Do not split it into
  scalars-only again; BAS-E10 shows the scalars and the table are jointly
  fitted and splitting them ships half a solution.

  This is the highest information-per-hour item left: costs CPU hours rather
  than games, targets the capped non-linear funnel that BAS-E07 identified as
  where the reference's advantage actually lives, and tests a candidate that was
  blocked by a defect we have since repaired rather than by evidence.

- **5.9.15 LTC probe — a fixed-N estimate, NOT a gate.** Every 5.9 verdict so
  far is `3+0.03`. BAS-M07 records fast-TC results compressing at longer TC, and
  5.9.10 already requires the TC ladder on the grounds that endgame knowledge
  shows at LTC and hides at STC. The 5.9.11 arms improved the mate-drive canary
  (29 → 34) while measuring neutral at STC, which is exactly that profile.

  Run the best 5.9.11 arm at `10+0.1`, ~6,000 games (~3.5h at the measured
  rate). **Read it as an estimate with a ±6 Elo CI**: enough to detect a +10-15
  Elo depth effect, not enough to resolve +3, and not enough to promote
  anything. Its job is to tell us whether a depth story exists that STC is blind
  to — a question that changes what the rest of the phase is worth.

- **5.9.13 Post-refit gate — ACCEPTED 2026-08-29, +9.52 ±4.66 Elo (BAS-E23).** The largest gain of Phase 5.9; the 768 PSTs had been frozen since Phase 4.7.

- **5.9.13 (original text).** One registered SPRT of the 5.9.12 evaluator
  against whatever is the accepted head at that point. This is the second and
  final Elo verdict in 5.9. It exists because 5.9.6 gates a candidate fitted on
  **29% of the surface with off-policy labels**, and 5.9.12 changes both of
  those; a verdict on the first does not transfer to the second.

**Stop rule.** If 5.9.6 rejects, ablate to separate the added structure from the
refit from the king-safety fit. Do not hand-retune a failed gate — that is
forbidden in both the Basilisk and Manta records. If the ablation shows the
structure is sound but the calibration is not, the residual carries into NNUE
data selection rather than into another fit.

**A 5.9.6 rejection does not revert the added structure.** The earlier stop rule
implied it would, and that would destroy 5.9.12's central test — *do the new
terms come alive once the 768 PSTs are free to move?* — which cannot be asked if
the terms have been removed first. On rejection the candidate is unshipped, the
structure stays on `development` unbaked or reverted-in-values-only, and the
question passes to 5.9.12/5.9.13. The terms are removed for good only if the
BAS-E08 ablation still measures them inert **after** the full-surface refit.

**Honest expectation.** BAS-E07 measured the gap as calibration at fishtest
scale, which we cannot reproduce. A material fraction of −232.8 Elo is not the
expected outcome; a real but bounded gain is. The step is worth running because
a stronger HCE is also a better NNUE teacher for Phase 7.1, so the work is not
lost even if the direct gain is modest.

### Execution order after the 2026-08-25 decision

Numbering is historical; execution order is this:

1. ~~5.14 shallow-depth node cost~~ — **closed 2026-08-25, no target found.**
2. **5.9 HCE maturity program** — running now. Maintainer decision of
   2026-08-25: take the measured, unexploited evaluation gap first.
3. **5.7 and 5.8** — Cluster D (extensions, singular, IIR) and Cluster E (root
   and clock). **Deliberately deferred, not skipped**, and to be attempted after
   5.9. They remain open steps with their preconditions unchanged.
4. 5.10–5.13 correctness, portability, SMP and the release gate.

The order is inverted from the numbering because search has now failed to yield
across 5.4, 5.5, 5.6 and 5.14, while the −232.8 Elo evaluation gap (BAS-O02) is
measured and untouched. 5.7/5.8 are still owed a real attempt.

### 5.14 — Shallow-depth node cost (NEW 2026-08-13, from the Manta import)

**Why this exists.** Importing Manta's consecutive-depth branching method
(BAS-D05) overturned this phase's leading diagnosis. Measured properly, at
Hash 64 on every arm:

| | Basilisk | SF search + our eval |
|---|---:|---:|
| b(4–11) | **1.692** | 1.894 |
| nodes at depth 4 | 29,482 | 6,732 — **4.38×** |
| nodes at depth 11 | 1,170,224 | 588,190 — 1.99× |

**Our per-ply growth is better than the reference's.** Every EBF figure this
phase acted on came from `nodes^(1/depth)`, which folds the fixed cost of the
first plies into the estimate and pointed at the wrong quantity.

That explains the three failed clusters at once: 5.4, 5.5 and 5.6 all attacked
per-ply width, which was never deficient. Cutting harder could not help, and
BAS-S16 charged −3.48 Elo for trying.

**The target.** A depth-4 search costs us **4.4×** what it costs the reference,
and the multiple decays with depth exactly as a better ratio predicts. The
deficit is concentrated in shallow subtrees. Nothing in Phase 5 has examined
that, because the whole phase was framed around growth rate.

**Scope, deliberately narrow.**

- Diagnostic first. Decompose the depth-4 cost: interior against quiescence,
  nodes per root move, re-searches from aspiration, and how much is spent
  before the first fail-high. The 5.2 substrate already carries most of it.
- No candidate until the decomposition names a cause. This step is registered
  as measurement, not as a fourth pruning cluster — the budget clause forbids
  inventing one of those, and this is not that.
- If the cause is a mechanism, it becomes an ordinary registered candidate with
  its own gate. If it is a cost we cannot move, the step closes and the budget
  decision proceeds unchanged.

**Caveat.** Absolute node counts are not comparable across engines; the ratio
is. The 4.4× is stated as an observation about our own cost curve set beside
theirs, and the equal-time gap from BAS-O01/O03 — 15.6 plies against 25.2 — is
the engine-agnostic evidence that the deficit is real.

### 5.14 — CLOSED 2026-08-25 (revised same day after measuring to depth 19)

Results in `analysis/step514_shallow_cost.md`; evidence BAS-D06 and **BAS-D07**.
Engine unchanged, bench 11,941,440.

The first pass stopped at depth 11 and drew the wrong conclusion. Extended to
depth 19 — the range games actually reach — the picture changes:

| depth | 4 | 8 | 11 | 13 | 15 | 17 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|
| Basilisk / oracle nodes | 4.38× | 3.18× | 1.99× | 2.06× | 1.75× | 1.92× | 1.80× |

| segment | Basilisk | oracle |
|---|---:|---:|
| b(4–11) | 1.692 | 1.894 |
| **b(11–19)** | **1.570** | **1.590** |

**Two corrections to what this phase believed.**

1. **Deep branching is equal, not better for us.** BAS-D05's "our growth is
   better" came from the 4–11 segment alone; over 11–19 the two are
   indistinguishable (medians 1.548 against 1.558). The ratio therefore
   **plateaus at ~1.8–2.0× and never closes** — the projected crossover does not
   exist.
2. **The gap is 4 plies, not 12.** BAS-O04's 12.07 was a mean over a
   distribution containing forced mates reaching depth 100 and 245; 10 of 105
   positions hit depth ≥100. The **median gap is 4.00 plies**. Every "12 plies
   shallower" statement in this plan overstates by about three times.

**Correction 3 (BAS-D08) — there is no shallow target, and 5.14 yields none.**
Differencing the cumulative counts gives each iteration's own cost:

| depth | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| per-iteration ratio | 1.56× | 2.56× | 2.05× | 1.31× | 2.37× | 1.85× | 1.75× | 1.69× |

Our depth-19 **iteration itself** costs ~1.7× theirs. The cumulative ratio sits
near 1.9× because every iteration costs about that multiple — not because a
shallow overhead is carried. And **depths ≤6 are 0.205% of a depth-19 search**,
so removing that band entirely would change nothing.

Both earlier readings of this step were wrong: the depth-4 peak of 4.38× is an
artifact of cumulative accounting, and the "a saving propagates unchanged"
argument inferred a mechanism from a constant ratio whose simpler explanation is
a uniform per-iteration multiple.

**5.14 therefore produces no actionable target.** The deficit is a uniform ~1.9×
per-iteration cost at all depths, with no band where work pays
disproportionately — which is the same wall clusters 5.4–5.6 hit. The ~2.5-ply
discrepancy from BAS-D07 is the only concrete unexplained quantity left here.

**One discrepancy left open.** A flat 1.9× cost at b≈1.55 predicts ~1.5 plies,
while the measured median gap is 4.0. About 2.5 plies is unexplained. Candidate
causes, not yet separated: the 16-position summed branching sample is dominated
by its most expensive positions and may not represent the 107-position median,
and the engines may differ in when an iteration counts as complete. Recorded as
open rather than resolved by picking the convenient number. **A cluster against
the shallow band should not open until this is settled** — otherwise its metric
inherits the same ambiguity.

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
adjudication **off**, including the 5.1 oracle as the reference oracle.

- Release **1.10.0** when the acceleration work produced a material, independently
  confirmed gain: normally a cumulative STC point estimate of at least **+40
  Elo** over 1.9.3 with the 95% lower bound above **+25 Elo**, plus positive
  LTC and 4T lower bounds.
- A larger cumulative result may justify a higher minor version; that is a
  maintainer decision, not an automatic consequence of the number.
- Release **1.9.4** as the fallback if the acceleration tracks close without
  material transfer but the correctness/platform/SMP work is non-regressing.
- If a candidate regresses, revert or defer it; do not extend Phase 5 into an
  open-ended rescue campaign.

Remove diagnostic scaffolding that has no future owner, resolve dormant
switches, version, rebuild revision-matched PGO/ISA assets, update user
documentation and archive the evidence; commit but do not push/tag.

### Maturity preconditions — do not adopt a feature its host cannot support

A mechanism that is strong in a mature search can be worthless or harmful in a
search that lacks its inputs. Adopting it early does not "get us partway there";
it produces a measured loss, and the natural but wrong conclusion is that the
mechanism does not transfer. Sibling-project evidence: Manta implemented a
feature ahead of its search maturity and it failed for this reason. Rarog's
Phase 4 failed the same way at bundle scale — individually plausible mechanisms
that did not compose.

Each cluster therefore declares **preconditions** and may not begin until every
one is present in Basilisk *and* measurably healthy in the 5.2 diagnostics.
"Planned", "in the next cluster" or "roughly equivalent" does not satisfy a
precondition.

| Cluster | Preconditions | Why the feature is worthless without them |
|---|---|---|
| 5.4 A — ordering, histories, LMR | staged generation, a TT move at most interior nodes, existing history tables, post-move check knowledge | this cluster *is* the foundation; it has no upstream dependency and therefore goes first |
| 5.5 B — static eval, TT, qsearch | A accepted | qsearch capture ordering and TT bound use are only as good as the move ordering and history feeding them |
| 5.6 C — main selectivity | B accepted | every margin is measured against a pruning eval; if raw eval, pruning eval and searched bounds are not yet separated, the margins fit the wrong quantity. Move-count and history pruning additionally read A's normalized histories |
| 5.7 D — extensions | A and B accepted | singular verification needs a trustworthy TT move with sound depth/bound provenance; extension and reduction decisions must be arbitrated against a settled LMR |
| 5.8 E — root and clock | C and D accepted | aspiration and time policy fitted to an unsettled interior search are refitted the moment the interior changes |
| 5.9 HCE | the search track closed | evaluation terms are judged by the search that consumes them |

**Precondition failure is work, not a blocker.** If a precondition is absent or
the diagnostics show it unhealthy, that enabling work becomes the cluster and
the dependent feature defers to a later one. Record the deferral and its
trigger; do not carry the feature forward as an unlisted intention.

**Negative-result triage.** When a cluster fails its gate, answer *in this
order* before concluding the mechanism does not transfer:

1. Was a declared precondition actually unhealthy at the nodes where the
   mechanism fires? Check the diagnostics, not the design.
2. Was the cluster dependency-complete, or did it ship a consumer without its
   producer?
3. Were reference constants used unvalidated against our scale?
4. Only then: the mechanism genuinely does not transfer to Basilisk.

Record which of the four applied. A mechanism rejected for reason 1–3 is
**requeued with its trigger**, not closed; a mechanism closed under reason 4
needs new evidence to reopen. Misfiling a premature adoption as reason 4
permanently discards a real gain, which is the specific cost this section
exists to prevent.

### Cluster discipline and stop rules

These govern 5.4–5.9 and exist because Rarog's Phase 4 failed by accumulating
individually plausible search mechanisms that did not compose.

0. Each cluster designs **Basilisk's** answer to the problem the reference
   identified, and records why, per the Independence contract. A cluster whose
   only stated rationale is upstream authority does not proceed.
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

After architecture/scale freezes, resolve whatever Phase 5 did not
already reach and settle, with isolated categorical A/Bs: TT/result evidence,
qsearch/ProbCut consumers, NMP/IIR/singular cooperation, prospective-depth/LMR,
history/correction attribution and root confidence. Consult the Phase-5 cluster
verdicts first — a contract accepted or rejected there under HCE is re-opened
here only because NNUE changed its inputs, not to relitigate it.

Then select ≤24 non-redundant search coordinates and run the only planned
search SPSA; require clean bake/PGO/SPRT/LTC/4T and post-fit ablations. This
remains the single planned search tune: Phase 5 settles architecture, Phase
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

**Harness status (2026-08-12).** Colosseum's GUI remains the tournament tool.
Its **CLI is not adopted**: it was trialled and reverted because it cannot pin
14 concurrent games on this host. Colosseum allocates a disjoint physical core
to *each engine*, so 14 slots demand 28 physical cores against the 16 available
and the pinned ceiling is 7 — half throughput for no measurement benefit, since
with ponder off the two engines in a game alternate and share one core happily.
The fastchess harness above pins one core per **game** and reaches 14.

`tools/colosseum/` keeps the converted profiles and SPSA tune vectors for when
the CLI is ready; nothing in the current workflow reads them.

