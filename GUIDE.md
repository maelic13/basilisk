# Basilisk development guide

A one-page overview for the maintainer. Every step is written once, in
`docs/PLAN.md`: its title, capability class, status and detail. The step list below
is generated from PLAN. How agents work is in `AGENTS.md`; how recurring work
runs, including the reusable research, implementation and review prompts, is
in `docs/PROCESS.md`; where the engine stands is PLAN §1.

## Now

| Item | Value |
|---|---|
| Released baseline | **1.10.2** on `master`, tagged `v1.10.2` (2026-10-08), `bench 13` 14,978,465 |
| Bench fingerprint | **19,470,325** (`bench 13`, since A.13.2's pawnless scaler; 19,108,490 at A.13.1; 16,092,165 at A.11.11; 8,913,394 at A.11.10; 7,857,713 at A.11.9; 7,384,101 at A.11.7; 7,233,823 at A.11.6; 8,753,339 at A.11.2; 9,260,429 at A.11.1; 8,437,148 at B.10; 11,099,210 at B.2.4; 14,978,465 for 1.10.0 to 1.10.2) |
| Active experiment | None |
| Long job | None |
| Next release | **1.11.0** after B (B.11), then 1.12.0 after C, 1.13.0 after E; **2.0.0** is the first NNUE release |

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

**Next step:** **B.3** `[I2]` Cluster 2 — pruning and extensions — Claude Opus 5 — High

**Held `(ANY TIME)` steps**, done between steps when asked:

- **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2 — Claude Sonnet 5 — Medium
- **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1 — Claude Opus 5 — High

### Phase A — Reset: documents, harness, instruments, baselines (EXTENDED)


### Phase B — Search programme (evaluation frozen)

- [ ] **B.3** `[I2]` Cluster 2 — pruning and extensions
- [ ] **B.4** `[I2]` Cluster 3 — quiescence
- [ ] **B.5** `[I2]` Cluster 4 — root and aspiration
- [ ] **B.6** `[V]` Joint search SPSA, if justified
- [ ] **B.7** Search speed pass
    - [ ] **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2
    - [ ] **B.7.2** `[I1]` Speed pass
- [ ] **B.8** Cleanup of legacy search and dead parameters
    - [x] **B.8.1** `[I1]` Delete the legacy search kernel (ANY TIME) — before the A.11 gates
    - [ ] **B.8.2** `[I1]` Dead parameters, switches and diagnostics
- [ ] **B.9** `[V]` Search programme checkpoint
- [ ] **B.11** `[M]` Interim release after the search programme

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
- [ ] **C.13** `[M]` Interim release after the evaluation programme

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
- [ ] **E.3** `[M]` Classical release 1.13.0 after Phase E

### Phase F — NNUE (own data only)

- [ ] **F.0** `[R3]` NNUE investigation
- [ ] **F.1** `[I2]` Board events and accumulator scaffolding
- [ ] **F.2** Data generation at scale and the classical refit on it
    - [ ] **F.2.1** `[V]` Data generation at scale
    - [ ] **F.2.2** `[V]` Classical refit on search-visited data
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
