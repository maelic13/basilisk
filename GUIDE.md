# Basilisk Development Workflow Guide

This is the short operational roadmap. Detailed rationale, contracts, gates
and lessons live in [`PLAN.md`](PLAN.md).

## Current checkpoint

| Item | State |
|---|---|
| Branch/release | `master`/`v1.9.3` at `d737123`; `development` at `f045b37`, with unchanged playing code |
| Baseline | Basilisk **1.9.3**, bench **11,941,440**; search-identical to 1.9.2 |
| Tournament record | Last observation at 8,626/36,400: Basilisk 3006; Rybka 4.1/4/5/6 +82/+96/+150/+172; Critter +184; Houdini 1.5a +211. Provisional pool evidence only; 5.0 determines and archives current state. |
| Evaluation | HCE frozen. No HCE feature/weight/Texel work before NNUE. |
| Current phase | **Phase 5 — bounded pre-NNUE hardening** |
| Portability branch | `origin/arm_fix` wrapper rejected at 5.3; retain ISA/ARM verification, never merge the branch wholesale |
| Next releases | **1.9.4 by default at 5.5**; **1.10.0 only for material verified gain**; baseline NNUE **2.0.0 at 7.7** |

Phase 5 has no booked Elo gain. Its expected result is neutral to slightly
positive; correctness, portability and a reproducible NNUE handoff are enough
for 1.9.4. The historical engine ladder moves to 8.4 and does not block NNUE.

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

### Phase 5 — Bounded pre-NNUE hardening (→ 1.9.4 by default)

- [ ] **5.0 Baseline:** determine/archive tournament state and reproduce clean
      1.9.3, PGO manifest and bench 11,941,440.
- [ ] **5.1 Bounded diagnostics:** one behaviour-neutral sampled substrate for
      result consumers, pruning recall, attribution, root ownership and SMP.
      Keep speculative search concerns as shadows for 8.3; do not widen TT.
- [ ] **5.2 Correctness/safety only:** repair demonstrated legal-root,
      mate/rule-50, TT atomic/replacement or attribution failures. Do not force
      cleaner but de-tuned search heuristics before NNUE.
- [ ] **5.3 Portability/ISA:** enforce x86 tier and ARM64 asset contracts,
      inspect emitted instructions and establish target anchors. Close the
      invalid `origin/arm_fix` wrapper; verify Basilisk's existing ARM prefetch.
- [ ] **5.4 SMP/TC checkpoint:** null-calibrated 1/2/4/8/16T NPS,
      time-to-depth, completed-depth and TT/root/work-share sweep; then a
      bounded current Basilisk-vs-Rarog `{1T,4T} × {3+0.03,10+0.1}` matrix.
      Test the thread × TC interaction with uncertainty. If an internal SMP
      deficit exists, classify it and test at most one targeted mitigation;
      otherwise close without code changes. Do not copy Rarog's rejected
      staggering.
- [ ] **5.5 Release:** prior-release non-regression plus platform/SMP gates.
      Release 1.9.4 by default; use 1.10.0 only after a registered material
      `[3,10]` nElo gate and positive LTC/4T transfer.

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

First determine whether the 36,400-game tournament is still running. If so,
let it finish unchanged and do not start competing bench/NPS/PGO/game work. If
not, archive the available PGN/config/binaries/manifests and begin 5.0 by
reproducing the 1.9.3 baseline. No pre-NNUE SPSA is planned.

## Decision rules

| Situation | Action |
|---|---|
| Behaviour-neutral | Exact bench plus correctness/performance evidence |
| Strength candidate | Registered SPRT; H1 accepts, otherwise revert behaviour |
| Root/TM/SMP | 1T STC/LTC plus 4T LTC, zero forfeits |
| Mechanism de-tunes consumers | Keep shadowed/deferred until post-NNUE 8.3; post-fit ablation required |
| SPSA | Phase 8.3 only; any pre-NNUE exception needs a demonstrated blocker and approval |
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
