# Basilisk development guide

A one-page overview for the maintainer. Every step is written once, in
`docs/PLAN.md`: its title, capability class, status and detail. The step list below
is generated from PLAN. How agents work is in `AGENTS.md`; how recurring work
runs, including the reusable research, implementation and review prompts, is
in `docs/PROCESS.md`; where the engine stands is PLAN §1.

## Now

| Item | Value |
|---|---|
| Released baseline | **1.10.1** on `master`, tagged `v1.10.1` (2026-09-27) |
| Bench fingerprint | **14,978,465** (`bench 13`, 1.10.0 and 1.10.1) |
| Active experiment | None |
| Long job | None |
| Next release | **1.10.2**, a correctness patch (A.8.21, once Phase A is finished); then **2.0.0** if the E.2 target gate is met, otherwise **1.11.0**, cut at E.3 |

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

<!-- BEGIN GENERATED FROM docs/PLAN.md by `python tools/diag/check_roadmap.py --write-guide`; edit PLAN, not this block -->

**Next step:** **A.8.20** `[M]` B-programme anchors on the A.8 head — Claude Sonnet 5 — Medium

**Held `(ANY TIME)` steps**, done between steps when asked:

- **A.8.21** `[M]` Patch release 1.10.2 (ANY TIME) — after A.8.20 — Claude Sonnet 5 — Medium
- **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2 — Claude Sonnet 5 — Medium
- **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1 — Claude Opus 5 — High

### Phase A — Reset: documents, harness, instruments, baselines

- [ ] **A.8** Rarog-parity repairs, repository and C++23
    - [x] **A.8.1** `[M]` Final Rarog import
    - [x] **A.8.2** `[I1]` CI on pull requests to master only
    - [x] **A.8.3** `[M]` Merge commits from dev to master
    - [x] **A.8.4** `[M]` Documents into docs/, closed Phase A archived
    - [x] **A.8.5** `[I1]` Experiment ledger split into entries
    - [x] **A.8.6** `[I1]` Tag-driven release flow
    - [x] **A.8.7** `[I1]` SMP: the chosen thread's line before bestmove
    - [x] **A.8.8** `[I1]` UCI info conformance
    - [x] **A.8.9** `[I2]` MultiPV
    - [x] **A.8.10** `[I1]` Command-line commands, fatal errors on stdout
    - [x] **A.8.11** `[I1]` Tablebase PV extension start rule
    - [x] **A.8.12** `[I2]` C++23 idiom pass, behaviour-neutral
    - [x] **A.8.13** `[R2]` Score bands and in-search tablebase probes
    - [x] **A.8.14** `[I2]` Score bands and probes, implementation
    - [x] **A.8.15** `[V]` Tablebase-enabled gate
    - [x] **A.8.16** `[R2]` Won-endgame time sink
    - [x] **A.8.17** `[I1]` Won-endgame time sink, implementation
    - [x] **A.8.18** `[V]` Won-endgame time sink gate
    - [x] **A.8.19** `[R2]` Displayed-score normalisation
    - [ ] **A.8.20** `[M]` B-programme anchors on the A.8 head
    - [ ] **A.8.21** `[M]` Patch release 1.10.2 (ANY TIME) — after A.8.20

### Phase B — Search programme (evaluation frozen)

- [ ] **B.0** Search programme investigation
    - [ ] **B.0.1** `[V]` Razoring depth reach in Elo (maintainer)
    - [ ] **B.0.2** `[V]` Oracle move-loop pruning family in Elo (maintainer)
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
- [ ] **D.4** `[R2]` Tablebase policy

### Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
- [ ] **E.2** `[V]` Classical target gate
- [ ] **E.3** `[M]` Release 2.0.0 if E.2 is met, else 1.11.0

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
