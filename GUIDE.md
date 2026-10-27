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
| Bench fingerprint | **7,857,713** (`bench 13`, since A.11.9's bare-king tiers; 7,384,101 at A.11.7; 7,233,823 at A.11.6; 8,753,339 at A.11.2; 9,260,429 at A.11.1; 8,437,148 at B.10; 11,099,210 at B.2.4; 14,978,465 for 1.10.0 to 1.10.2) |
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

**Next step:** **A.11.10** `[R2]` Fail-soft quiescence — Claude Opus 5 — High

**Held `(ANY TIME)` steps**, done between steps when asked:

- **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2 — Claude Sonnet 5 — Medium
- **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1 — Claude Opus 5 — High

### Phase A — Reset: documents, harness, instruments, baselines (EXTENDED)

- [x] **A.8** Rarog-parity repairs, repository and C++23
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
    - [x] **A.8.20** `[M]` B-programme anchors on the A.8 head
    - [x] **A.8.21** `[M]` Patch release 1.10.2 (ANY TIME) — after A.8.20
- [x] **A.9** Harness and measurement instruments
    - [x] **A.9.1** `[I1]` Colosseum re-pinned to 0.4.0
    - [x] **A.9.2** `[V]` Direction reads of the head against 1.10.2
    - [x] **A.9.3** `[M]` Measurement rules from the audit
    - [x] **A.9.4** `[I1]` NPS read rebuilt on pooled builds
    - [x] **A.9.5** `[I1]` Profiler with inline attribution
- [x] **A.10** Delivery and repository repairs
    - [x] **A.10.1** `[M]` Land dev and B.10 on master by squash PRs (ANY TIME) — before B.3
    - [x] **A.10.2** `[M]` CHANGELOG Unreleased caught up and kept
    - [x] **A.10.3** `[M]` PR refspec and required checks on master
    - [x] **A.10.4** `[I1]` Release and CI checks tightened
    - [x] **A.10.5** `[I1]` Ledger index dispositions kept in step
    - [x] **A.10.6** `[M]` Tools index and retired numbers in comments
    - [x] **A.10.7** `[I1]` Dev builds name their commit
- [ ] **A.11** Correctness repairs from the Rarog audit
    - [x] **A.11.1** `[I1]` Rule-50 damping out of the TT static eval
    - [x] **A.11.2** `[I1]` Singular search: lone excluded move returns alpha
    - [x] **A.11.3** `[I1]` Own clock of zero without increment is bounded
    - [x] **A.11.4** `[I1]` Upper-case UCI move text accepted
    - [x] **A.11.5** `[I1]` Fatal errors from every thread reach stdout
    - [x] **A.11.6** `[I1]` Frontier nodes counted once
    - [x] **A.11.7** `[I1]` Evaluation colour symmetry: tie-break and sweep
    - [x] **A.11.8** `[I1]` Info lines: bestmove after a stop, mate scores as cp
    - [x] **A.11.9** `[R2]` Override boundaries never reward shedding material
    - [ ] **A.11.10** `[R2]` Fail-soft quiescence
- [ ] **A.12** `[M]` Roadmap rules from the audit
- [ ] **A.13** Correctness repairs from the code audit
    - [ ] **A.13.1** `[I1]` Won KPK scores as a win
    - [ ] **A.13.2** `[I1]` PV copy reads past the per-ply length array
    - [ ] **A.13.3** `[I1]` A won queen against two minors is not scaled to a draw
    - [ ] **A.13.4** `[I1]` The tablebase store carries a stale PV bit
- [ ] **A.14** Simplifications from the code audit
    - [ ] **A.14.1** `[I1]` One definition of the score bands
- [ ] **A.15** Universal binaries and the release runners
    - [ ] **A.15.1** `[R2]` Universal x86-64 binary: design packet
    - [ ] **A.15.2** `[V]` The ISA tier ladder measured
    - [ ] **A.15.3** `[I1]` Tier advisory, ISA contract and toolchain pins

### Phase B — Search programme (evaluation frozen)

- [x] **B.0** Search programme investigation
    - [x] **B.0.1** `[V]` Razoring depth reach in Elo (maintainer)
    - [x] **B.0.2** `[V]` Oracle move-loop pruning family in Elo (maintainer)
- [x] **B.1** `[I1]` Search restructure, behaviour-neutral
- [x] **B.2** Cluster 1 — the selectivity core
    - [x] **B.2.0** `[R2]` Architecture review and neutral upgrades
    - [x] **B.2.1** `[I2]` Implement behind the umbrella switch
    - [x] **B.2.2** `[V]` Diagnostics and the unfitted paired run
    - [x] **B.2.3** `[V]` Curvature sweep, then SPSA in blocks
    - [x] **B.2.4** `[V]` Cluster 1 gates
- [x] **B.10** `[I1]` PST orientation repair (ANY TIME) — before B.3
- [ ] **B.3** `[I2]` Cluster 2 — pruning and extensions
- [ ] **B.4** `[I2]` Cluster 3 — quiescence
- [ ] **B.5** `[I2]` Cluster 4 — root and aspiration
- [ ] **B.6** `[V]` Joint search SPSA, if justified
- [ ] **B.7** Search speed pass
    - [ ] **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2
    - [ ] **B.7.2** `[I1]` Speed pass
- [ ] **B.8** `[I1]` Cleanup of legacy search and dead parameters
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
    - [ ] **D.3.3** `[R2]` Chess960
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
