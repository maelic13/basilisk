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
| Bench fingerprint | **16,615,757** (`bench 13`, since B.12's quiescence repair; 19,470,325 at A.13.2; 19,108,490 at A.13.1; 16,092,165 at A.11.11; 8,913,394 at A.11.10; 7,857,713 at A.11.9; 7,384,101 at A.11.7; 7,233,823 at A.11.6; 8,753,339 at A.11.2; 9,260,429 at A.11.1; 8,437,148 at B.10; 11,099,210 at B.2.4; 14,978,465 for 1.10.0 to 1.10.2) |
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

**Next step:** **B.3.0** `[R2]` Research and handoff on the B.2 head — Claude Opus 5 — High

**Held `(ANY TIME)` steps**, done between steps when asked:

- **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2 — Claude Sonnet 5 — Medium
- **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1 — Claude Opus 5 — High

### Phase A — Reset: documents, harness, instruments, baselines (EXTENDED)

A.1–A.7 closed between 2026-09-28 and 2026-10-06; their text is in
[`docs/archive/PLAN-closed-2026-10-07.md`](docs/archive/PLAN-closed-2026-10-07.md).
They reset the documents (A.1); cleaned the repository, branches and tags and
inventoried features, options and parameters (A.2); made the Colosseum CLI
the main harness with its SPSA path (A.3); refreshed and froze the toolchain
with build flavours for arms (A.4); built the search programme's
instruments: fixed-budget probe, reference-anchored branching profile,
counter summation and decision trace, matched ablation mask, PGN conversion
instrument and canaries (A.5); mapped the codebase consolidation (A.6); and
measured the 1.10.1 baselines (A.7): the 1T pool census (BAS-M08), 4T
performance **3040** [3026, 3054] (BAS-M11), G(0) **+312.6 ± 17.8**
(BAS-O05) and pooled-PGO NPS **4.127M** (BAS-P12).

A.8–A.17 closed between 2026-10-07 and 2026-10-30; their text is in
[`docs/archive/PLAN-closed-2026-10-30.md`](docs/archive/PLAN-closed-2026-10-30.md).
They brought Basilisk to Rarog's parity: the last Rarog import, CI on pull
requests, the tag-driven release, UCI and MultiPV conformance, tablebase
score bands and probes, the won-endgame time sink and the 1.10.2 patch
release (A.8); rebuilt the harness and the instruments, among them the
pooled-build NPS read and the inline-attributing profiler (A.9); repaired
delivery and the repository (A.10); fixed the correctness defects of the
Rarog audit (A.11) and of the code audit (A.13), with the roadmap rules
(A.12) and one simplification (A.14) the audits asked for; shipped one
universal x86-64 binary per OS, four PGO tiers behind a CPU dispatcher,
buildable with Clang or GCC by Stockfish's assembly route, every trap
guarded (A.15); deleted the evaluation's zero-weight terms and their
parameters, **+2.35%** NPS (BAS-P25, A.16); and found no instruction-specific code that
makes the engine faster, the native build's 1% loss being Zen 3's tuning
model (BAS-P26, A.17).

### Phase B — Search programme (evaluation frozen)

B.0–B.2 and B.10 closed between 2026-10-06 and 2026-10-24; their text is
in [`docs/archive/PLAN-closed-2026-10-30.md`](docs/archive/PLAN-closed-2026-10-30.md).
B.0 pinned the donor (`sf_19`, `edb0d9db`), measured the deficit and fixed
the cluster order: razoring's depth reach read +2.4 Elo [−7.5, +12.3]
(B.0.1), and the oracle's move-loop pruning family is about two thirds of
its lead, G(0) − G(32) = 210.3 ± 23.9 of 312.6 (B.0.2). B.1 restructured the
search behaviour-neutrally, +1.21% NPS (BAS-P16). B.2, Cluster 1, rebuilt
the selectivity core in the donor's shape behind `B2_CORE` and fitted it:
+136.2 ± 33.6 Elo unfitted and +58.7 ± 20.9 fitted, about +195 over the
B.1 head (BAS-S23); the legacy kernel and the switch were later deleted
(B.8.1). B.10 repaired seven rank-mirrored piece-square
tables, +18.3 ± 12.9 Elo (BAS-S26).

- [ ] **B.3** Cluster 2 — pruning and extensions
    - [ ] **B.3.0** `[R2]` Research and handoff on the B.2 head
    - [ ] **B.3.1** `[I2]` Implement behind the umbrella switch
    - [ ] **B.3.2** `[V]` Diagnostics and the unfitted paired run
    - [ ] **B.3.3** `[V]` Curvature sweep, then SPSA in blocks
    - [ ] **B.3.4** `[V]` Cluster 2 gates
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
- [x] **B.12** `[I2]` False win in drawn KBP-K (ANY TIME) — before B.3.1
- [x] **B.13** `[I1]` Colosseum re-pinned to 0.5.0 (ANY TIME)

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
