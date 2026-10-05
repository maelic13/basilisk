# Basilisk development guide

A one-page overview for the maintainer. Every step is written once, in
`PLAN.md`: its title, capability class, status and detail. The step list below
is generated from PLAN. How agents work is in `AGENTS.md`; how recurring work
runs, including the reusable research, implementation and review prompts, is
in `PROCESS.md`; where the engine stands is PLAN §1.

## Now

| Item | Value |
|---|---|
| Released baseline | **1.10.1** on `master`, tagged `v1.10.1` (2026-09-27) |
| Bench fingerprint | **14,978,465** (`bench 13`, 1.10.0 and 1.10.1) |
| Active experiment | None |
| Long job | None |
| Next release | **2.0.0** if the E.2 target gate is met, otherwise **1.11.0**, cut at E.3.2 |

## Model by class

PLAN records only stable capability classes; edit this table when model
generations change. These are maintainer judgments, not measured rankings.

| Class | Capability | Model — thinking mode |
|---|---|---|
| `R3` | Frontier causal/architecture research | Claude Fable 5.1 — High |
| `R2` | Bounded correctness-sensitive reasoning | Claude Opus 5 — High |
| `I2` | Difficult implementation | Claude Opus 5 — High |
| `I1` | Well-specified implementation | Claude Sonnet 5 — Medium |
| `M` | Mechanical/docs/provenance | Claude Sonnet 5 — Medium |
| `V` | Verification/measurement | Claude Sonnet 5 — High |

## Steps

<!-- BEGIN GENERATED FROM PLAN.md by `python tools/diag/check_roadmap.py --write-guide`; edit PLAN, not this block -->

**Next step:** **A.7.4** `[V]` Pooled-PGO NPS baseline — Claude Sonnet 5 — High

**Held `(ANY TIME)` steps**, done between steps when asked:

- **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2 — Claude Sonnet 5 — Medium
- **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1 — Claude Opus 5 — High
- **E.3.1** `[I1]` Tag-driven release flow (ANY TIME) — before E.3.2 — Claude Sonnet 5 — Medium

### Phase A — Reset: documents, harness, instruments, baselines

- [x] **A.1** Document reset
- [x] **A.2** Repository and inventory
    - [x] **A.2.1** `[M]` Tracked-file cleanup
    - [x] **A.2.2** `[M]` Branch and tag disposition
    - [x] **A.2.3** `[R2]` Feature, option and parameter inventory
- [x] **A.3** Colosseum CLI as the main harness
    - [x] **A.3.1** `[I1]` Run files, `colosseum.ps1` and the shared guards
    - [x] **A.3.2** `[I1]` SPSA tune path from the X-macro
    - [x] **A.3.3** `[M]` PROCESS *Harness* section
- [x] **A.4** Build and toolchain
    - [x] **A.4.1** `[I1]` Toolchain refresh and freeze
    - [x] **A.4.2** `[I1]` Build flavors and manifests for arms
- [x] **A.5** Instruments for the search programme
    - [x] **A.5.1** `[I1]` Fixed-budget probe
    - [x] **A.5.2** `[I1]` Reference-anchored branching profile
    - [x] **A.5.3** `[I1]` Counter summation and decision trace
    - [x] **A.5.4** `[I1]` Matched ablation mask
    - [x] **A.5.5** `[I1]` PGN conversion instrument
    - [x] **A.5.6** `[R2]` Reference-anchored canaries
- [x] **A.6** `[R2]` Codebase consolidation analysis
- [ ] **A.7** Baselines on the 1.10.1 binary
    - [x] **A.7.1** `[V]` 1T pool baseline from the Super Rating Tournament
    - [x] **A.7.2** `[V]` 4T gauntlet against the targets and Rarog 2.4.0
    - [x] **A.7.3** `[V]` Oracle deficit meter G(0)
    - [ ] **A.7.4** `[V]` Pooled-PGO NPS baseline

### Phase B — Search programme (evaluation frozen)

- [ ] **B.0** `[R3]` Search programme investigation
- [ ] **B.1** `[I1]` Search restructure, behaviour-neutral
- [ ] **B.2** Cluster 1 — the selectivity core
    - [ ] **B.2.0** `[R2]` Architecture review and neutral upgrades
    - [ ] **B.2.1** `[I2]` Implement behind the umbrella switch
    - [ ] **B.2.2** `[V]` Diagnostics and the unfitted paired run
    - [ ] **B.2.3** `[V]` Curvature sweep, then SPSA in blocks
    - [ ] **B.2.4** `[V]` Cluster 1 gates
- [ ] **B.3** `[I2]` Cluster 2 — pruning and extensions
- [ ] **B.4** `[I2]` Cluster 3 — quiescence
- [ ] **B.5** `[I2]` Cluster 4 — root and aspiration
- [ ] **B.6** `[V]` Joint search SPSA, if justified
- [ ] **B.7** Search speed pass
    - [ ] **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2
    - [ ] **B.7.2** `[I1]` Speed pass
- [ ] **B.8** `[I1]` Cleanup of legacy search and dead parameters
- [ ] **B.9** `[V]` Search programme checkpoint

### Phase C — Evaluation programme (search frozen)

- [ ] **C.0** `[R3]` Evaluation programme investigation
- [ ] **C.1** `[I1]` Evaluation restructure, behaviour-neutral
- [ ] **C.2** Fit pipeline, corpus and label contract
    - [ ] **C.2.1** `[I1]` Fit-tooling contract and Texel handbook
    - [ ] **C.2.2** `[R2]` Corpus design
    - [ ] **C.2.3** `[V]` Generate and publish corpus A
    - [ ] **C.2.4** `[R2]` Matched label arms
    - [ ] **C.2.5** `[R2]` Initialization control
    - [ ] **C.2.6** `[V]` Matched fits and the label-contract gate
- [ ] **C.3** `[I2]` King safety cluster
- [ ] **C.4** `[I2]` Threats and mobility cluster
- [ ] **C.5** Endgame handling and winnability
    - [ ] **C.5.1** `[R2]` Instruments and gate integrity
    - [ ] **C.5.2** `[R3]` Occurrence, classification and ranking
    - [ ] **C.5.3** `[I2]` Generic winnability and scaling
    - [ ] **C.5.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
    - [ ] **C.5.5** `[I2]` Group B families by measured kind
    - [ ] **C.5.6** `[R2]` Lower-yield remainder
    - [ ] **C.5.7** `[V]` Endgame gate and closure
- [ ] **C.6** `[I2]` Pawns and passers cluster
- [ ] **C.7** `[I2]` Material, imbalance, phase and pieces cluster
- [ ] **C.8** `[V]` Refit cycles
- [ ] **C.9** `[V]` Nonlinear HCE SPSA or a written skip
- [ ] **C.10** `[V]` Search re-fit after the new evaluation
- [ ] **C.11** `[V]` Freeze the classical evaluation
- [ ] **C.12** `[I2]` Evaluation throughput, bit-exact

### Phase D — Clock, threads, robustness

- [ ] **D.1** `[R2]` Time management
- [ ] **D.2** `[R2]` Lazy SMP quality
- [ ] **D.3** Engine lifecycle, protocol and board contracts
    - [ ] **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1
    - [ ] **D.3.2** `[R2]` Lifecycle and protocol robustness
    - [ ] **D.3.3** `[R2]` Displayed-score normalisation
- [ ] **D.4** `[R2]` Tablebase policy

### Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
- [ ] **E.2** `[V]` Classical target gate
- [ ] **E.3** Classical release
    - [ ] **E.3.1** `[I1]` Tag-driven release flow (ANY TIME) — before E.3.2
    - [ ] **E.3.2** `[M]` Release 2.0.0 if E.2 is met, else 1.11.0

### Phase F — NNUE (own data only)

- [ ] **F.0** `[R3]` NNUE investigation
- [ ] **F.1** `[I2]` Board events and accumulator scaffolding
- [ ] **F.2** `[V]` Data generation at scale
- [ ] **F.3** `[I2]` Trainer hardening and baseline nets
- [ ] **F.4** `[I2]` Scalar integration, integer-exact conformance
- [ ] **F.5** `[I2]` Incremental and SIMD inference
- [ ] **F.6** `[V]` Search re-fit for the network
- [ ] **F.7** `[R3]` Architecture ladder, one axis at a time
- [ ] **F.8** `[V]` Data frontier
- [ ] **F.9** `[M]` NNUE release
- [ ] **F.10** `[V]` CCRL top-100 gate

### Phase G — Scaling, platforms and the top 50

- [ ] **G.1** `[R2]` High-thread and NUMA
- [ ] **G.2** `[I1]` Platform and product
- [ ] **G.3** `[R3]` Frontier

<!-- END GENERATED -->
