# Basilisk Development Workflow Guide

This is the short operational roadmap. Detailed rationale, contracts, gates
and lessons live in [`PLAN.md`](PLAN.md).

## Current checkpoint

| Item | State |
|---|---|
| Branch/release | `master`/`v1.9.3` at `d737123`; `development` at `16eff20`, with unchanged playing code |
| Baseline | Basilisk **1.9.3**, bench **11,941,440**; search-identical to 1.9.2. Reproduced clean at 5.0: CTest 12/12, manifest complete, artefact `tools/test_engines/basilisk-1.9.3-baseline-pext-pgo.exe` |
| Tournament record | Context only, gates nothing. Basilisk sits at 3023; the historical frontier is ~165–185 Elo ahead. Do not schedule or curate rating tournaments as plan work. |
| Evaluation | HCE constant refitting stays frozen. **Structural** feature work is unfrozen at 5.9 only. No Texel/SPSA refit. |
| Current phase | **Phase 5 — search and evaluation acceleration** (5.0 closed) |
| Reference | Stockfish `9587eeeb`, the last pure-HCE master commit before NNUE. An **idea source and oracle** — never a transcription source. Basilisk stays independent. |
| Portability branch | `origin/arm_fix` wrapper rejected at 5.11; retain ISA/ARM verification, never merge the branch wholesale |
| Next releases | **1.10.0 at 5.13** if the acceleration work transfers (higher minor if the gain is large); **1.9.4** is the maintenance-only fallback; baseline NNUE **2.0.0 at 7.7** |

**Phase 5 changed scope on 2026-08-12.** It was bounded hardening with no
booked Elo. Rarog's search-oracle experiment (BAS-X09/X10) measured Stockfish's
last pure-HCE search — driving Rarog's weaker evaluator, at 1.5M NPS against
our 2.5M — beating Basilisk 1.9.3 by about **+196 Elo**, and the matching
Stockfish HCE beating that hybrid by another **+329**. So the largest
measurable deficit is search coordination, with a second in HCE feature
coverage, and both can be attacked with ideas from a public reference instead of
being redesigned blindly.

Those are logistic estimates from someone else's stopped run against someone
else's evaluator. They size a target and order the work; they never accept a
Basilisk change and are never quoted as a release claim. **5.1 measures our own
magnitudes before any code moves.**

### Independence — binding, not aspirational

**Basilisk stays an independent engine.** Stockfish is an idea source, a
diagnostic oracle and evidence of what a mature search achieves — never
something to become. Both are GPLv3, so copying would be legal; that is not the
constraint. A transcribed engine inherits decisions we cannot explain and
discards work that already measured well.

| Do | Don't |
|---|---|
| Learn what problem a mechanism solves, then design our answer | Copy source, or paraphrase it into a translation |
| Reimplement in our idiom, types, structure and parameter table | Mirror upstream file layout, decomposition or naming |
| Treat reference constants as starting points to validate | Import constant tables verbatim — they were fitted to another search and scale |
| Record deliberate divergence as *intentionally different* | Delete a Basilisk-original mechanism because upstream lacks it |
| Attribute the idea in source and `README.md` | Accept a cluster because its trace looks more Stockfish-like |

**The test:** state why Basilisk does it this way *without* saying "because
Stockfish does". No such answer ⇒ not understood well enough to ship.

### Program order — Phase 5 is not an NNUE shortcut

```text
Phase 5  build the engine up: search → HCE → correctness/platform/SMP → release
Phase 6  NNUE runway: corpus, state contract, trainer preflight
Phase 7  train and integrate the baseline NNUE → 2.0.0
Phase 8  the engine adjustments NNUE makes necessary, then the single SPSA
```

Phase 5 ends when **its own** release gate passes, not when NNUE looks
reachable. No datagen or trainer work during Phase 5. A stronger engine is a
better NNUE teacher, so skipping ahead costs twice.

Equally, this is not a reason to abandon NNUE: search work here is
evaluator-agnostic and survives intact, and HCE work here improves the 7.1
teacher. The historical engine ladder still moves to 8.4 and still does not
block NNUE.

## Closed phases

### Phase 1 — Foundations and first strength line — ✅ 1.0.0–1.8.0

Built the board/UCI/search/TT/history/SEE/Syzygy/SMP stack, serious testing and
the accepted HCE. Repeated HCE self-play fitting stopped transferring.

### Phase 2 — Correctness and search architecture — ✅ 1.9.0

Banked state, repetition/rule-50, TT/mate/SEE correctness, staged ordering,
correction/history, root-instability TM and dense TT improvements.

### Phase 3 — Hardening, CI and PGO speed — ✅ 1.9.1

Centralized parameters, expanded invariants/CI/telemetry and shipped a
behaviour-identical **+4.34% NPS** PGO speed pass.

### Phase 4 — SMP durability and release tooling — ✅ 1.9.2/1.9.3

Repaired SPSA/MT harnesses, helper clock/node/thread safety and data tooling;
accepted +30.42 ±8.77 Elo at 4T with zero forfeits. 1.9.3 fixed PGO tool
matching without changing search.

## Forward phases

### Phase 5 — Search and evaluation acceleration (→ 1.10.0 or higher)

**Evidence and instrumentation**

- [x] **5.0 Baseline:** ✅ clean 1.9.3 reproduced — bench 11,941,440, CTest
      12/12, complete PGO manifest. Rating tournaments closed as context only.
- [ ] **5.1 Basilisk search oracle:** build Stockfish `9587eeeb` calling
      Basilisk's exact 1.9.3 HCE, plus its own HCE as control. C++ links
      directly — no DLL/FFI needed. Adjudication **off**. Measures our search
      gap and our HCE gap separately. **Stop rule: if the search contrast is
      under ~50 Elo, close the acceleration program and go to NNUE.**
- [ ] **5.2 Differential diagnostic harness** (the old "bounded diagnostics",
      kept whole): versioned fixed suite, fixed depth/nodes, 1T. Counters for
      TT producer/consumer kind, **prune recall and overlap** (not just node
      savings — lesson 5), **correction attribution**, history attribution,
      move source, cutoff index, LMR/re-searches, pruning, extensions,
      aspiration, root ownership, SMP share. Off ⇒ bench 11,941,440 exactly;
      on ⇒ same best move and nodes. Transient `OutcomeKind`/capability
      predicates may land when behaviour-neutral. **Shadow-record** stand-pat,
      ProbCut, NMP/IIR/singular, checking-LMR and root-confidence concerns —
      owned by the cluster that reaches them, else 8.3. Run it against the
      oracle: the counters that differ most select the work.
- [ ] **5.3 Idea inventory and order freeze:** find which *problems* the
      reference solves that we don't. Classify each equivalent / intentionally
      different / missing / coupled, ranked by 5.2 populations. Output is a
      list of problems worth solving, **not** upstream functions to reproduce
      — the design is each cluster's own work. An inventory with no
      "intentionally different" entries means transcription, not analysis. If
      evidence contradicts the cluster order, edit PLAN *before* implementing.

**Search acceleration clusters** — one at a time, each accepted or reverted
before the next starts

- [ ] **5.4 Cluster A — ordering, histories, LMR.** Owns the latent post-move
      `gives_check` LMR defect; repair it *inside* the cluster (durable lesson
      2: its standalone repair lost −21.55 ±9.83).
- [ ] **5.5 Cluster B — static eval, TT, qsearch.** Keep raw eval, pruning
      eval and searched bounds distinct. Preserve our draw/mate/rule-50
      semantics; they are assets, not targets.
- [ ] **5.6 Cluster C — main selectivity.** Razoring, RFP, NMP verification,
      ProbCut, move-count/history pruning, futility. Categoricals before
      constants; no broad SPSA.
- [ ] **5.7 Cluster D — extensions and depth authority.** Check, singular,
      double/negative, IIR vs TT provenance and LMR.
- [ ] **5.8 Cluster E — root and clock.** Aspiration, completed-root
      authority, PV/fallback, stability. Time allocation moves last and is
      gated separately.

**Evaluation gap closure**

- [ ] **5.9 HCE structural gap closure:** missing or materially weaker terms
      only, each with its local refit. **Not** another broad constant fit —
      cycle 6 washed at +1.37 ±5.21 and that stands. Two failed clusters ⇒
      close the track and carry the residual into NNUE data selection.

**Consolidation and release**

- [ ] **5.10 Correctness/safety only:** repair demonstrated legal-root,
      mate/rule-50, TT atomic/replacement or attribution failures. Do **not**
      force checking-move LMR (→5.4), qsearch staging (→5.5), subtree-null
      (→5.6), correction weighting (→5.5) or aspiration (→5.8) here as
      "cleanups"; each has a cluster owner and a history of losing alone.
- [ ] **5.11 Portability/ISA:** enforce x86 tier and ARM64 asset contracts,
      inspect emitted instructions and establish target anchors. Close the
      invalid `origin/arm_fix` wrapper; verify Basilisk's existing ARM prefetch.
- [ ] **5.12 SMP/TC checkpoint:** null-calibrated 1/2/4/8/16T NPS,
      time-to-depth, completed-depth and TT/root/work-share sweep; then a
      bounded current Basilisk-vs-Rarog `{1T,4T} × {3+0.03,10+0.1}` matrix.
      Test the thread × TC interaction with uncertainty. If an internal SMP
      deficit exists, classify it and test at most one targeted mitigation;
      otherwise close without code changes. Do not copy Rarog's rejected
      staggering.
- [ ] **5.13 Cumulative checkpoint and release:** same pinned PGO path both
      arms, direct 1.9.3 comparison, 1T STC/LTC + 4T LTC, zero forfeits,
      correctness matrix, 5.11 platform contract, final cross-engine cohort
      with adjudication off. **1.10.0** needs ≥ +40 Elo STC point estimate with
      95% lower bound above +25, plus positive LTC/4T lower bounds. Otherwise
      fall back to a maintenance 1.9.4.

### Phase 6 — NNUE runway and branch convergence

- [ ] **6.0** inventory old `origin/nnue`, then reimplement/cherry-pick only
      useful seams; do not rebase its obsolete `.mnn` contract wholesale.
- [ ] **6.1** per-ply state and complete dirty-piece make/unmake contracts.
- [ ] **6.2** frozen teacher/residual/search-disagreement corpora.
- [ ] **6.3** pinned `D:/code/net_trainer` data/manifests/resume preflight.
- [ ] **6.4** bench-identical runway gate and integration branch.

### Phase 7 — Baseline NNUE (→ 2.0.0)

- [ ] **7.0** harden trainer CLI, splits, manifests, determinism/conformance.
- [ ] **7.1** controlled 30–60M initial data and label/mining A/Bs.
- [ ] **7.2** H=512 pilot/H=1024 baseline with at least two seeds.
- [ ] **7.3** strict scalar loader/embedded net and exact references.
- [ ] **7.4** incremental accumulators and exact portable/x86/ARM64 kernels.
- [ ] **7.5** baseline data/architecture iteration one variable at a time.
- [ ] **7.6** gross NNUE-scale search safety calibration only.
- [ ] **7.7** HCE/STC/LTC/4T/external/parity gates and release 2.0.0.

### Phase 8 — NNUE frontier and final search fit

- [ ] **8.0** residual and search-disagreement analysis.
- [ ] **8.1** scale/deduplicate data, natural finishes and hard-position mining.
- [ ] **8.2** evidence-led king/threat/material/width architecture ladder.
- [ ] **8.3** resolve deferred search architecture with categorical A/Bs, then
      run the single planned post-NNUE search SPSA and ablations.
- [ ] **8.4** contemporary frontier plus contextual historical-engine ladder.

### Phase 9 — Scaling, platforms and product completeness

- [ ] **9.0** continue the 5.4 baseline into high-thread/NUMA/root/TT/
      accumulator scaling; no staggering retry without new Basilisk evidence.
- [ ] **9.1** advanced memory, full-budget TT and runtime ISA dispatch.
- [ ] **9.2** demanded product or additional-platform work; baseline ARM64 and
      NNUE/NEON parity are already release gates in 5.3 and 7.4/7.7.
- [ ] **9.3** scaling/platform release matrix.

### Phase 10 — Optional HCE fallback

Enter only after serious NNUE integration/data/architecture retries fail and
the user explicitly abandons that program.

- [ ] **10.0** document NNUE failure and approve HCE scope.
- [ ] **10.1** select a small residual-driven HCE program.
- [ ] **10.2** run one HCE fit and full external release matrix.

## What you run now

5.0 is closed and the 1.9.3 baseline binary exists. Next is **5.1 — the
Basilisk search oracle**: Stockfish `9587eeeb` calling our exact 1.9.3 HCE,
with its own evaluator as control, run with adjudication off.

Do this before any acceleration code. It is the step that tells us whether the
~+196/+329 split is real *for Basilisk*, and it carries the stop rule that can
end this program cheaply. Building the oracle is code work and can start now;
its games must wait for the machine.

A Rarog-seeded gauntlet currently occupies the machine at concurrency 14.
Defer all game and NPS/PGO timing work until it finishes. No pre-NNUE broad
SPSA is planned.

### Acceleration step lifecycle (5.4–5.9)

1. **Audit** — name the *problem* the reference solves, its Basilisk owner,
   every interacting consumer and the 5.2 diagnostic population. Update PLAN's
   order first if evidence contradicts it.
2. **Design** — decide Basilisk's answer and write down why, without appealing
   to upstream authority. Reference constants are starting points to validate,
   never values to trust.
3. **Register** — add an `EXPERIMENTS.md` ID with hypothesis, baseline SHA,
   scope, expected direction, gate, cap and stop rule, *before* games.
4. **Implement** — smallest dependency-complete change, in our own idiom and
   structure. Substeps may be diagnosed separately; an incomplete cluster never
   becomes a baseline.
5. **Prove correctness** — CTest 12/12, sanitizers where relevant, canaries.
   Diagnostics off ⇒ exact accepted fingerprint.
6. **Explain** — the fixed suite compares nodes, qnodes, move source, cutoff
   index, TT use, reductions, pruning, extensions, aspiration. Counters
   explain; they never accept.
7. **Gate** — revision-matched final-PGO both arms, registered paired UHO
   SPRT. Do not change candidate, bounds, cap, book or adjudication after
   seeing games.
8. **Close** — accept and commit, or revert the behaviour and restore the
   fingerprint. Keep the evidence row either way. Ablate a surprising
   integrated result before crediting a subcomponent.
9. **Advance** — only after the previous item is accepted, rejected or
   explicitly closed.

**Two fully implemented clusters with no accepted gain ⇒ stop and re-audit
5.2–5.3.** Do not continue down the list by sunk cost.

## Decision rules

| Situation | Action |
|---|---|
| Behaviour-neutral | Exact bench plus correctness/performance evidence |
| Strength candidate | Registered SPRT; H1 accepts, otherwise revert behaviour |
| Root/TM/SMP | 1T STC/LTC plus 4T LTC, zero forfeits |
| Mechanism de-tunes consumers | Fix it inside its 5.4–5.8 cluster and fit jointly; defer to 8.3 only if no cluster owns it |
| Reference contract differs | Adopt the *idea*, implement it our way. "Looks more like the reference" never accepts a cluster |
| Cross-evaluator cohort | Adjudication **off** — score-based adjudication moved a headline estimate ~75 Elo |
| HCE proposal | Structural gap vs the reference ⇒ 5.9. Another constant refit ⇒ refused; cycle 6 washed |
| SPSA | Phase 8.3 only. A cluster's own small local refit is part of the cluster, not a tune |
| NNUE baseline loses | Diagnose contract/data/training/architecture; do not jump to HCE |
| Historical target unavailable | Record the gap; it does not block Phase 5 or NNUE |

## Working rhythm

```text
You   -> Paste completed long-job artifacts or ask for the next step.
Model -> Implements, verifies, updates PLAN + GUIDE and commits without push.
You   -> Run only the requested SPSA/SPRT/gauntlet/datagen job.
```

## Common commands

```powershell
.\tools\build_test.ps1 -Suffix <name>
colosseum-cli --run-file tools/colosseum/profiles/sprt-gainer.toml `
  sprt <candidate> <baseline> --book <book.epd> --concurrency <games>
colosseum-cli nps <candidate> --self-pair --nodes 10000000
colosseum-cli nps <candidate> --against <baseline> --nodes 10000000 --repetitions 12
```

Use `tools/colosseum/README.md` for SPSA, calibration, tournament, datagen and
analysis commands. Colosseum owns generic harness behaviour; Basilisk owns
builds, engine correctness, profiling and engine-specific data/tuning policy.
