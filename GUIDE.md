# Basilisk development guide

A one-page overview for the maintainer. Every step is written once, in
`docs/PLAN.md`: its title, capability class, status and detail. The step list below
is generated from PLAN. How agents work is in `AGENTS.md`; how recurring work
runs, including the reusable research, implementation and review prompts, is
in `docs/PROCESS.md`; where the engine stands is PLAN §1.

## Now

| Item | Value |
|---|---|
| Released baseline | **1.11.0** on `master`, tagged `v1.11.0` (2026-11-18), `bench 13` 4,777,412 |
| Bench fingerprint | **3,359,329** (`bench 13`, since C.2.8's whole-surface refit `cbc3676`; 4,392,040 after the top-tier repair `cd4e453`; 4,777,412 before it; HISTORY's 1.11.0 record lists the earlier counts) |
| Active experiment | None |
| Long job | None |
| Next release | **1.12.0** after C (C.10), then 1.13.0 after E; **2.0.0** is the first NNUE release |

## Model by class

PLAN records only stable capability classes. The table names the model
tier only: versions change too often to track here, and the thinking mode
is the maintainer's choice at the time. These are maintainer judgments,
not measured rankings.

| Class | Capability | Model |
|---|---|---|
| `R3` | Frontier causal/architecture research | Claude Fable |
| `R2` | Bounded correctness-sensitive reasoning | Claude Opus |
| `I2` | Difficult implementation | Claude Opus |
| `I1` | Well-specified implementation | Claude Sonnet |
| `M` | Mechanical/docs/provenance | Claude Sonnet |
| `V` | Verification/measurement | Claude Sonnet |

## Steps

<!-- BEGIN GENERATED FROM docs/PLAN.md by `python tools/diag/check_roadmap.py --write-guide`; edit PLAN, not this block -->

**Next step:** **C.3.2** `[I2]` The port behind the switch, at parity — Claude Opus

### Phase A — Reset: documents, harness, instruments, baselines (EXTENDED)

A.1–A.7 closed between 2026-09-28 and 2026-10-06; their text is in
[`docs/archive/PLAN-closed-2026-10-07.md`](docs/archive/PLAN-closed-2026-10-07.md).

A.8–A.17 closed between 2026-10-07 and 2026-10-30; their text is in
[`docs/archive/PLAN-closed-2026-10-30.md`](docs/archive/PLAN-closed-2026-10-30.md).

### Phase B — Search programme (evaluation frozen)

B.0–B.2 and B.10 closed between 2026-10-06 and 2026-10-24; their text is
in [`docs/archive/PLAN-closed-2026-10-30.md`](docs/archive/PLAN-closed-2026-10-30.md).

B.3–B.16 closed by 2026-11-18; their text, and the phase's design notes
(why this shape, what "Stockfish-shaped" means, the scale conversion and
the six-stage gating shape), are in
[`docs/archive/PLAN-closed-2026-11-18.md`](docs/archive/PLAN-closed-2026-11-18.md).

### Phase C — Evaluation programme (search frozen)

- [x] **C.0** `[R3]` Evaluation programme investigation
- [x] **C.1** `[I1]` Evaluation restructure, behaviour-neutral
- [x] **C.2** Fit pipeline, corpus, labels and gate instruments
    - [x] **C.2.1** Fit-tooling contract and Texel handbook
        - [x] **C.2.1.1** `[I1]` Rows, labels and splits
        - [x] **C.2.1.2** `[I2]` Tuner correctness
        - [x] **C.2.1.3** `[R2]` Coordinate manifest and initial vectors
        - [x] **C.2.1.4** `[I1]` Fit driver and provenance
        - [x] **C.2.1.5** `[I1]` Texel tooling hygiene and CI
        - [x] **C.2.1.6** `[R2]` Texel handbook and the standing contract
    - [x] **C.2.2** `[R2]` Corpus design
    - [x] **C.2.3** `[V]` Generate and publish corpus A
    - [x] **C.2.4** `[R2]` Matched label arms
    - [x] **C.2.5** `[R2]` Initialization control
    - [x] **C.2.7** Endgame instruments and gate integrity
        - [x] **C.2.7.1** `[R2]` Audit against the Rarog snapshot
        - [x] **C.2.7.2** `[I2]` Truth contract v2 and guard tests
        - [x] **C.2.7.3** `[V]` Deployed nodes and the budget bracket
        - [x] **C.2.7.4** `[V]` The C head's endgame baselines
    - [x] **C.2.8** Matched fits and the label-contract gate
        - [x] **C.2.8.1** `[R2]` Register the fit and the gates
        - [x] **C.2.8.2** `[I1]` The fit driver to the registered schedule
        - [x] **C.2.8.3** `[V]` Fits of A and B and the fit report
        - [x] **C.2.8.4** `[V]` The registered gates
    - [x] **C.2.9** `[I1]` Family screen and matched-ablation tools
    - [x] **C.2.10** `[R3]` Family order from games and screens
- [ ] **C.3** Evaluation rebuilt whole in the donor's form
    - [x] **C.3.1** `[R3]` Rebuild research and handoff
    - [ ] **C.3.2** `[I2]` The port behind the switch, at parity
    - [ ] **C.3.3** `[R2]` Independent review
    - [ ] **C.3.4** `[V]` Seed-value read and fit
    - [ ] **C.3.5** `[V]` Gate, frontier pair and close
    - [ ] **C.3.6** `[R2]` Basilisk's own forms read back in
- [ ] **C.4** Endgame handling and winnability
    - [ ] **C.4.2** `[R3]` Occurrence, classification and ranking
    - [ ] **C.4.3** `[I2]` Generic winnability and scaling
    - [ ] **C.4.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
    - [ ] **C.4.5** `[I2]` Group B families by measured kind
    - [ ] **C.4.6** `[R2]` Lower-yield remainder
    - [ ] **C.4.7** `[V]` Endgame gate and closure
- [ ] **C.5** Residual and refit cycles
    - [ ] **C.5.1** `[R3]` Residual investigation
    - [ ] **C.5.2** `[V]` Refit cycles
- [ ] **C.6** `[V]` Nonlinear HCE SPSA or a written skip
- [ ] **C.7** Joint re-fit on the new evaluation
    - [ ] **C.7.1** `[R2]` Joint-fit investigation and registration
    - [ ] **C.7.2** `[V]` The joint fit, PGO and gate
- [ ] **C.8** `[V]` Freeze the classical evaluation
- [ ] **C.9** `[I2]` Evaluation throughput, bit-exact
- [ ] **C.10** Interim release after the evaluation programme
    - [ ] **C.10.1** `[I1]` Reported score and WDL in Stockfish's form
    - [ ] **C.10.2** `[M]` The 1.12.0 cut

### Phase D — Clock, threads, tablebases

- [ ] **D.1** `[R2]` Time management
- [ ] **D.2** `[R2]` Lazy SMP quality
- [ ] **D.3** `[R2]` Tablebase policy

### Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
- [ ] **E.2** `[V]` Classical target gate
- [ ] **E.3** `[M]` Classical release 1.13.0 after Phase E

### Phase F — NNUE (own data only)

- [ ] **F.0** `[R3]` NNUE investigation
- [ ] **F.1** `[I2]` Board events and accumulator scaffolding
- [ ] **F.2** Network contract, trainer and corpus
    - [ ] **F.2.3** `[I2]` Scalar integration, integer-exact conformance
    - [ ] **F.2.4** `[I2]` Trainer hardening and the learning curve
    - [ ] **F.2.5** `[V]` Data generation at scale
    - [ ] **F.2.6** `[V]` Classical refit on search-visited data
- [ ] **F.5** `[I2]` Incremental and SIMD inference
- [ ] **F.6** `[V]` First nets, then the search re-fit for the network
- [ ] **F.7** `[R3]` Architecture ladder, one axis at a time
- [ ] **F.8** `[V]` Data frontier
- [ ] **F.9** `[M]` NNUE release
- [ ] **F.10** `[V]` CCRL top-100 gate

### Phase G — Scaling, platforms and the top 50

- [ ] **G.1** `[R2]` High-thread and NUMA
- [ ] **G.2** `[I1]` Platform and product
- [ ] **G.3** `[R3]` Frontier

### Phase H — Refinements after the big gains

- [ ] **H.1** `[R2]` Refinement backlog triage
- [ ] **H.2** `[I2]` Search refinements, one at a time
- [ ] **H.3** `[I2]` Time-management refinements at their own control
- [ ] **H.4** `[R2]` Throughput at an identical tree on the network head
- [ ] **H.5** `[I2]` Harness rewritten in Python for a Linux host
- [ ] **H.6** `[R2]` Donor-form re-reads, both arms refitted

<!-- END GENERATED -->
