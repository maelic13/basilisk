# Basilisk Development Workflow Guide

This is the short operational roadmap. Detailed rationale, contracts, gates
and lessons live in [`PLAN.md`](PLAN.md).

## Current checkpoint

| Item | State |
|---|---|
| Branch/release | `master`, `development` and `v1.9.3` at `d737123` |
| Baseline | Basilisk **1.9.3**, bench **11,941,440**; search-identical to 1.9.2 |
| Live tournament | At 8,626/36,400: Basilisk 3006; Rybka 4.1/4/5/6 +82/+96/+150/+172; Critter +184; Houdini 1.5a +211. Provisional pool evidence only. |
| Evaluation | HCE frozen. No HCE feature/weight/Texel work before NNUE. |
| Current phase | **Phase 5 — evidence-coherent pre-NNUE search** |
| Portability branch | `origin/arm_fix` = unproven Apple-TT alignment hypothesis; inventory at 5.8, never merge wholesale |
| Next releases | **1.10.0 at 5.11**; baseline NNUE **2.0.0 at 7.7** |

The 1.10.0 target is direct paired superiority over every installed Rybka,
Critter 1.6a, Houdini 2.0c and Fritz 16. Catching one rung does not close it.

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

### Phase 5 — Evidence-coherent pre-NNUE search (→ 1.10.0)

- [ ] **5.0 Baseline:** archive the live tournament and reproduce clean 1.9.3.
- [ ] **5.1 Diagnostics:** trace provenance, pruning recall/overlap,
      NMP/ProbCut/singularity, correction attribution and root confidence.
- [ ] **5.2 Evidence/TT:** add result kinds and explicit consumer contracts
      while preserving dense TT unless measurements justify a layout change.
- [ ] **5.3 Qsearch/ProbCut:** stop stand-pat laundering, separate speculative
      cutoffs and improve safe evasion/capture ordering.
- [ ] **5.4 NMP/IIR/singular:** add subtree null suppression, node/eval guards,
      IIR debt and trustworthy extension evidence.
- [ ] **5.5 Selectivity:** unify LMP/futility/SEE/LMR around history-aware
      prospective depth and repair the checking-move LMR defect inside it.
- [ ] **5.6 History/correction:** prevent tactical contamination, fit source
      confidence and test compact contextual signals.
- [ ] **5.7 Root confidence:** connect root variance to aspiration, TM,
      completed legal fallback and SMP ownership.
- [ ] **5.8 Portability/ISA:** inventory `origin/arm_fix`; make x86 tier
      contracts executable, run Linux/Windows/macOS ARM64 before release,
      inspect emitted instructions and target-measure prefetch/alignment/
      false sharing. Adopt Rarog's five-platform fingerprint pattern.
- [ ] **5.9 Throughput/scaling:** profile accepted semantics, TT capacity and
      1/2/4/8T without regressing the platform/ISA matrix.
- [ ] **5.10 One search SPSA:** freeze architecture, select ≤24 coordinates and
      run the only consolidated pre-NNUE fit plus post-fit ablations.
- [ ] **5.11 Release gate:** cumulative 1T/LTC/4T plus production platform/ISA
      matrix and Holm-adjusted
      paired wins over every Rybka, Critter 1.6a, Houdini 2.0c and Fritz 16;
      then release 1.10.0.

### Phase 6 — NNUE runway and branch convergence

- [ ] **6.0** inventory/rebase old `origin/nnue` once onto the Phase-5 handoff.
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
- [ ] **8.3** the single post-NNUE search SPSA after architecture freezes.
- [ ] **8.4** contemporary-frontier and cumulative release gate.

### Phase 9 — Scaling, platforms and product completeness

- [ ] **9.0** high-thread/NUMA/root/TT/accumulator scaling.
- [ ] **9.1** advanced memory, full-budget TT and runtime ISA dispatch.
- [ ] **9.2** demanded product or additional-platform work; baseline ARM64 and
      NNUE/NEON parity are already release gates in 5.8 and 7.4/7.7.
- [ ] **9.3** scaling/platform release matrix.

### Phase 10 — Optional HCE fallback

Enter only after serious NNUE integration/data/architecture retries fail and
the user explicitly abandons that program.

- [ ] **10.0** document NNUE failure and approve HCE scope.
- [ ] **10.1** select a small residual-driven HCE program.
- [ ] **10.2** run one HCE fit and full external release matrix.

## What you run now

Nothing new for Basilisk while the shared tournament and Rarog aspiration SPSA
occupy the machine. Let all 36,400 tournament games finish unchanged and
archive PGN/config/binaries/manifests. Do not run Basilisk bench, NPS, PGO,
SPRT, SPSA, datagen or another tournament concurrently.

Afterward, provide the final artifacts. The model will reproduce 1.9.3 and
implement 5.1 before requesting a long job.

## Decision rules

| Situation | Action |
|---|---|
| Behaviour-neutral | Exact bench plus correctness/performance evidence |
| Strength candidate | Registered SPRT; H1 accepts, otherwise revert behaviour |
| Root/TM/SMP | 1T STC/LTC plus 4T LTC, zero forfeits |
| Mechanism de-tunes consumers | Keep inert/ablatable until 5.10; post-fit ablation required |
| SPSA | Phase 5.10 and 8.3 only unless new evidence explicitly authorizes another |
| NNUE baseline loses | Diagnose contract/data/training/architecture; do not jump to HCE |
| Target unavailable | Phase 5 stays open; rating-list inference is insufficient |

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
.\tools\nps_ab.ps1 -EngineA <candidate> -EngineB <candidate>
.\tools\nps_ab.ps1 -EngineA <candidate> -EngineB <baseline> -Rounds 12
```
