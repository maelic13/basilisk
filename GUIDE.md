# Basilisk Development Workflow Guide

This is the short operational roadmap. Detailed rationale, contracts, gates
and lessons live in [`PLAN.md`](PLAN.md).

## Current checkpoint

| Item | State |
|---|---|
| Branch/release | `master`/`v1.9.3` at `d737123`; `development` at `16eff20`, with unchanged playing code |
| Baseline | Basilisk **1.9.3**, bench **11,941,440**; search-identical to 1.9.2. Reproduced clean at 5.0: CTest 12/12, manifest complete, artefact `tools/test_engines/basilisk-1.9.3-baseline-pext-pgo.exe` |
| Tournament record | Context only, gates nothing. Basilisk sits at 3023; the historical frontier is ~165–185 Elo ahead. Do not schedule or curate rating tournaments as plan work. |
| Evaluation | HCE constant refitting stays frozen; **structural** feature work unfrozen at 5.9 only. Audit (BAS-E07): structurally mature, functionally behind — the −232.8 Elo gap is values not features, so 5.9 is worth ~6 minor terms and **the gap is NNUE's to close**. |
| Current phase | **Phase 5 — search and evaluation acceleration** (5.0–5.3 closed; next 5.4) |
| Reference | Stockfish `9587eeeb`, the last pure-HCE master commit before NNUE. An **idea source and oracle** — never a transcription source. Basilisk stays independent. |
| Oracle | Branch `hybrid` `01df815`, binary `7E2433C3…`. Frozen. The 5.2 harness comparison target; never merge or ship it. |
| Diagnostics | `Diag`=true ⇒ `info string diag kv` counters. Fixed suite `tools/diag/suite_v1.epd` (107 pos, do not edit — mint v2), runner `tools/diag/run_suite.py`, baseline `tools/diag/baseline_v1.json`. |
| Portability branch | `origin/arm_fix` wrapper rejected at 5.11; retain ISA/ARM verification, never merge the branch wholesale |
| Next releases | **1.10.0 at 5.13** if the acceleration work transfers (higher minor if the gain is large); **1.9.4** is the maintenance-only fallback; baseline NNUE **2.0.0 at 7.7** |

**Phase 5 changed scope on 2026-08-12** from bounded hardening with no booked
Elo, on Rarog's search-oracle prior — and **5.1 has now confirmed the premise
on our own engine** (EXPERIMENTS BAS-O01–O03, 2,400 games, adjudication off):

| Contrast | Isolates | Result |
|---|---|---|
| Oracle − Basilisk 1.9.3 | search, our HCE constant | **+322.7 ±36** |
| Stockfish HCE − Oracle | evaluation, SF search constant | **+232.8 ±32** |
| Full Stockfish − Basilisk | the whole gap | +516.1 ±59 |

Both tracks are real and **search is the larger**, so the ordering stands. The
oracle won while searching *fewer* nodes per move (170k vs 226k) at lower NPS,
so the search figure is understated, and the two isolated legs compose to
within 39 Elo of the directly measured whole — the isolation held.

The mechanism is visible rather than inferred: at equal time Basilisk finishes
**15.6** plies where the oracle finishes **25.2**, giving an effective
branching factor of **2.20 against 1.61**. Our tree is not too small, it is too
**wide**. That points at ordering, reductions and selectivity — cluster 5.4 —
which is now first on evidence, not just on dependency order.

Rarog's original figures (+196 search, +329 evaluation) are superseded for
Basilisk: our search gap is larger and our evaluation gap smaller, the latter
indicating our HCE is materially better than theirs.

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
- [x] **5.1 Basilisk search oracle:** ✅ **CLOSED 2026-08-12.** 2,400 games,
      adjudication off, zero forfeits, all-natural terminations.
      **Search +322.7 ±36** (our HCE constant) · **HCE +232.8 ±32** (SF search
      constant) · whole gap +516.1 ±59 · Basilisk−Rarog +14.8 ±27.
      Stop rule cleared by a wide margin. The oracle won on *fewer* nodes
      (170k vs 226k) at lower NPS, so the search figure is understated.
      Mechanism: **EBF 2.20 vs 1.61** — 15.6 plies against 25.2 at equal time.
      Our tree is too **wide**, not too small. See EXPERIMENTS BAS-O01–O03.
- [x] **5.2 Differential diagnostic harness:** ✅ **CLOSED 2026-08-12.**
      15 counters added to the 8.6.6 substrate + machine-readable `diag kv`
      mirror + fixed 107-position `tools/diag/suite_v1.epd` +
      `tools/diag/run_suite.py`. Bench 11,941,440 unchanged, CTest 12/12,
      diag on/off identical nodes/PV/bestmove.
      **Ordering is fine** — 89.1% first-move cutoffs, mean index 0.214.
      **Reductions are not** — 36.1% of eligible reduced, 16.2% clamped to
      zero, **re-search rate 1.744%**. At 300k nodes we reach depth 20.80 vs
      the oracle's 32.87 (**+12.07 plies**, identical evaluation).
      ⇒ the width is **under-reduction**, so 5.4's centre is 5.4.3, not the
      move picker. See EXPERIMENTS BAS-D01/D02.
- [x] **5.3 Idea inventory and order freeze:** ✅ **CLOSED 2026-08-12** —
      `analysis/idea_inventory_v1.md`. Seven items classified.
      **#1 reduction modulation is nearly inert**: we have the right shape but
      ~10× too little magnitude — **+0.39 plies at cut nodes vs ~+2**, −0.02 on
      TT-PV vs ~−2. Those came from `hcefinal` (+35.94, so not arbitrary) fitted
      inside a search already built around timid reductions — lesson 2.
      **#5 ordering is EQUIVALENT** — move-picker rework leaves the cluster.
      **#4 history pruning is dead** (142 fires in 15.1M nodes) → 5.6.
      **Order amended before implementing:** check-move depth policy moves
      5.7 → **5.4.4**, because the unconditional check extension (15.8% of
      interior nodes) and the never-reduce-checks rule are one question and
      8.6.7 showed changing either alone loses ~10 Elo.

**Search acceleration clusters** — one at a time, each accepted or reverted
before the next starts

- [x] **5.4 Cluster A — ordering, histories, LMR:** ✅ **CLOSED 2026-08-13,
      no accepted change.** Ordering was already equivalent (BAS-D01);
      reduction magnitude refuted on the harness (BAS-S13/S14/S15); check-move
      depth rejected by games (BAS-S16, **−3.48 ±3.32** over 17,058, LLR −2.95).
      The engine is unchanged at bench 11,941,440. Re-audit done:
      `analysis/reaudit_v1.md`.
- [x] **5.5 Cluster B — static eval, TT, qsearch:** ✅ **CLOSED 2026-08-13,
      no candidate.** `analysis/cluster55_audit_v1.md`. Eval provenance is
      already correct (raw→TT, corrected→improving, TT-refined→pruning only,
      mate-clamped) and we hold **correction history, which `9587eeeb` lacks**.
      Qsearch mirrors it exactly and its structure is equivalent. **Qsearch
      share measured: ours 30.8% vs the reference's 36–37%** (BAS-D03) — ours
      is *smaller*, so qsearch is not a width source. TT layout is mature; the
      persisted TT-PV bit is missing but already adjudicated (costs an age bit,
      barred by 5.2; the 8.5.7 re-test measured +51% nodes, no operating point).
      Engine unchanged at bench 11,941,440.
- [x] **5.6 Cluster C — main selectivity:** ✅ **CLOSED 2026-08-13, no
      candidate.** `analysis/cluster56_audit_v1.md`. History pruning is
      genuinely defective — its `coeff × depth` threshold is compared against a
      sum of six bounded channels (max 81,920), so the depth-6 threshold of
      84,024 is **provably unsatisfiable**; it fires 142 times in 5,355,599
      tested quiets. But loosening it buys **no depth** (−0.019 / +0.037 /
      −0.019), and BAS-S16 priced that trade at −3.48 Elo. Recorded with a
      retry trigger (BAS-D04), not gated. ProbCut's 0.4% is correct rarity —
      67% hit rate. Engine unchanged.
- [ ] **5.7 Cluster D — extensions and depth authority.** Singular,
      double/negative, IIR vs TT provenance and LMR. **Check extensions are no
      longer owned here** — moved to 5.4.4 by the 5.3 inventory and closed
      there as rejected (BAS-S16).
- [ ] **5.8 Cluster E — root and clock.** Aspiration, completed-root
      authority, PV/fallback, stability. Time allocation moves last and is
      gated separately.

**Evaluation gap closure**

- [~] **5.9 HCE maturity program — IN PROGRESS** (unfrozen 2026-08-25). Sub-steps
      below. Only 5.9.6 produces an Elo verdict; everything before it is
      scaffolding and is **expected** to show no strength.

  - [x] **5.9.1 Coverage close-out — DONE 2026-08-25.** Seven terms added
        (`bad_outpost`, `bishop_xray_pawn`, `long_diagonal_bishop`,
        `knight_on_queen`, `slider_on_queen`, `trapped_rook`,
        `threat_safe_pawn`), all **seeded 0**. Evidence: bench 11,941,440
        unchanged, CTest 12/12, `--verify` exact on 10,000 positions, and every
        term shown to fire over 20,000 corpus positions (1,442–9,932 each).
        *No strength change, by design — inert terms cannot move Elo.*
  - [x] **5.9.2 Simpler-form audit — DONE 2026-08-25.** Most shapes already
        match the reference (threats graded by attacked piece, mobility as
        per-count tables, rook-on-file decomposed, hanging indexed). Two real
        defects repaired: `king_protector` was **one scalar shared by knight and
        bishop** — split per piece, seeded at the shared value; and **bishop
        outposts were never priced** despite a stale comment claiming they were.
        Evidence: bench 11,941,440, CTest 12/12, `--verify` exact,
        BishopOutpost fires 1,173 / KingProtectorN 13,399 / KingProtectorB
        14,234 of 20,000. *Still no Elo — by design.*
  - [ ] **5.9.3 Structure freeze.** No further evaluation structure changes; a
        fit against a moving structure has to be redone.
  - [ ] **5.9.4 Joint Texel refit** of the **enlarged** surface. Classify every
        coefficient free/fixed/excluded first (BAS-X14) — a coefficient fitted
        through a cap is a corrupted gradient, not a tuned value.
  - [ ] **5.9.5 SPSA** over what Texel cannot price: the capped, nonlinear
        king-danger funnel. Registered horizon/bounds/stop rule before launch.
  - [ ] **5.9.6 One promoting SPRT** + post-fit ablation. **This is the only
        step that can gain or lose Elo.**

      Why no individual SPRTs before 5.9.6: BAS-X11 records Manta losing ~−23
      Elo across two gates by hand-scaling exactly this class of term and gating
      each alone. Why this is a real retry and not cycle 6 repeated: cycle 6
      refit the **same** features and washed at +1.37 ±5.21; this refits an
      **enlarged** surface.

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

## Where we are

**Phase 5.9, sub-step 5.9.2.** The checklist above tracks all six sub-steps.

| | |
|---|---|
| Engine state | unchanged — bench **11,941,440**, CTest 12/12 |
| Last completed | **5.9.2** simpler-form audit (structural, no Elo by design) |
| Running now | **5.9.3** structure freeze, then **5.9.4** the joint fit |
| Next Elo verdict | **5.9.6**, after the 5.9.4 fit and 5.9.5 SPSA |
| Deferred, not skipped | **5.7** extensions/singular/IIR, **5.8** root/clock — after 5.9 |
| Nothing queued for your machine | 5.9.1–5.9.4 are code and fitting work |

### Was 5.9.1 successful?

Yes, for what it is — but it produced **no strength, and could not have.**

All seven terms are seeded to **0**, so the evaluator computes them and the
tuner can see them, while the engine plays exactly as before. That is deliberate:
the values come from the joint fit at 5.9.4, never from hand-reasoning, because
hand-set reference-family terms are what lost Manta ~−23 Elo across two gates.

So "success" at 5.9.1 means four things, all verified:

- the terms **exist and compute** — each fires on 1,442–9,932 of 20,000 corpus
  positions;
- they are **provably inert** — bench 11,941,440 unchanged, CTest 12/12;
- the tuner can **reconstruct the evaluator exactly** with them present
  (`--verify` on 10,000 positions);
- so they are **fittable** at 5.9.4.

**Nothing here is worth Elo yet, and the plan does not claim otherwise.** The
first and only Elo verdict in this program is 5.9.6. If you want a one-line
summary of 5.9.1: the scaffolding is in and provably harmless.



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

### Maturity preconditions — never adopt ahead of the host search

A mechanism strong in a mature search can be harmful in one lacking its inputs.
Adopting early doesn't get us partway — it measures a loss, and the wrong
conclusion is "doesn't transfer". Manta failed exactly this way.

| Cluster | Cannot start until |
|---|---|
| 5.4 A ordering/histories/LMR | — (the foundation; no upstream dependency) |
| 5.5 B eval/TT/qsearch | A accepted |
| 5.6 C selectivity | B accepted (margins need the pruning eval separated) |
| 5.7 D extensions | A + B accepted (singular needs a trustworthy TT move) |
| 5.8 E root/clock | C + D accepted (else refitted the moment the interior moves) |
| 5.9 HCE | search track closed |

Preconditions must be **present and healthy in the 5.2 diagnostics** — not
planned, not "roughly equivalent". If one is missing, that enabling work
*becomes* the cluster and the feature defers with a recorded trigger.

**When a cluster fails, triage in this order** before blaming the mechanism:
(1) was a precondition unhealthy where the mechanism fires? (2) was the cluster
dependency-complete? (3) were reference constants used unvalidated? (4) only
then — it genuinely doesn't transfer. Reasons 1–3 requeue with a trigger; only
reason 4 closes. Misfiling a premature adoption as (4) discards a real gain.

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
.\tools\setup_tools.ps1
.\tools\build_test.ps1 -Suffix <name>
.\tools\sprt.ps1 -EngineA <candidate> -EngineB <baseline> `
  -NameA Candidate -NameB Baseline -Elo1 3
.\tools\sprt.ps1 -EngineA <copy> -EngineB <same> `
  -NameA Self -NameB Self2 -Mode calibrate -Games 30000
.\tools\nps_ab.ps1 -EngineA <candidate> -EngineB <baseline> -Rounds 12
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <list> -TC "10+0.1"
```

The fastchess harness pins one physical core per **game**, so `Threads=1`
resolves to **14 concurrent games** on this 16-core host — the two engines in a
game alternate and share the core. `harness_common.ps1` owns topology
discovery, the `-use-affinity` core list and the fastchess >= 1.7.0 gate that
BAS-M01 required.

**The Colosseum CLI is not adopted.** It was trialled and reverted: it
allocates a disjoint physical core to *each engine*, so 14 slots would need 28
physical cores and the pinned ceiling is 7 — half throughput for no measurement
benefit. The Colosseum **GUI** remains the tournament tool.
`tools/colosseum/` keeps converted profiles and tune vectors for when the CLI
is ready; nothing in the current workflow reads them.
