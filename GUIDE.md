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
| Bench fingerprint | **3,783,135** (`bench 13`, since B.5.5's gates turned Cluster 4 on; 4,144,790 at B.4.7; 6,090,091 at B.4.2; 5,361,058 at B.3.6; 5,369,042 at B.3.5; 11,880,564 at B.14; 16,615,757 at B.12; 19,470,325 at A.13.2; 19,108,490 at A.13.1; 16,092,165 at A.11.11; 8,913,394 at A.11.10; 7,857,713 at A.11.9; 7,384,101 at A.11.7; 7,233,823 at A.11.6; 8,753,339 at A.11.2; 9,260,429 at A.11.1; 8,437,148 at B.10; 11,099,210 at B.2.4; 14,978,465 for 1.10.0 to 1.10.2) |
| Active experiment | BAS-S60 (B.6.6): razoring and the kings key against the head, SPRT `[0,3]`, prepared, maintainer-run |
| Long job | None |
| Next release | **1.11.0** after B (B.11.4), then 1.12.0 after C, 1.13.0 after E; **2.0.0** is the first NNUE release |

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

**Next step:** **B.6.6** `[R2]` Donor-form reconciliation — Claude Opus

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

- [x] **B.3** Cluster 2 — pruning and extensions
    - [x] **B.3.0** `[R2]` Research and handoff on the B.2 head
    - [x] **B.3.1** `[I2]` Implement behind the umbrella switch
    - [x] **B.3.2** `[V]` Diagnostics and the unfitted paired run
    - [x] **B.3.3** `[V]` Curvature sweep, then SPSA in blocks
    - [x] **B.3.4** `[V]` Basilisk's forms re-read on the fitted arm
    - [x] **B.3.5** `[V]` Cluster 2 gates
    - [x] **B.3.6** `[I2]` Colour-indexed histories (S2)
- [x] **B.16** Search-programme instruments
    - [x] **B.16.1** `[V]` Wall-clock overhead read (ANY TIME) — before B.4
    - [x] **B.16.2** `[I2]` Pruning and reduction pricing (ANY TIME) — before B.4
    - [x] **B.16.3** `[V]` Loss analysis by cell (ANY TIME) — before B.4
    - [x] **B.16.4** `[I1]` Identical-tree counter read (ANY TIME) — before B.4
    - [x] **B.16.5** `[I1]` Prompt helper stop (ANY TIME) — before B.4
- [x] **B.4** Cluster 3 — quiescence
    - [x] **B.4.1** `[I1]` Legal positions in the floors' generator
    - [x] **B.4.2** `[I2]` SEE's created pins and promotion recaptures
    - [x] **B.4.3** `[R2]` Research and handoff on the repaired head
    - [x] **B.4.4** `[I2]` Implement behind the umbrella switch
    - [x] **B.4.5** `[V]` Diagnostics and the unfitted paired run
    - [x] **B.4.6** `[V]` Curvature sweep, then SPSA in blocks
    - [x] **B.4.7** `[V]` Cluster 3 gates
- [x] **B.5** Cluster 4 — root and aspiration
    - [x] **B.5.1** `[R3]` Research and handoff on the entry head
    - [x] **B.5.2** `[I2]` Implement behind the umbrella switch
    - [x] **B.5.3** `[V]` Diagnostics, M1 and the unfitted paired run
    - [x] **B.5.4** `[V]` Curvature sweep, then SPSA in blocks
    - [x] **B.5.5** `[V]` Cluster 4 gates
    - [x] **B.5.6** `[R3]` SPSA method investigation and redesign
    - [x] **B.5.7** `[R2]` Two games per core on SMT siblings
    - [x] **B.5.8** `[I1]` SPSA read tool and checkpoint wrapper
- [ ] **B.6** Engine audit, reconciliation, then joint search SPSA
    - [x] **B.6.3** `[I1]` Search-path perft and kernel pins
    - [x] **B.6.4** `[R3]` Engine audit on the four-cluster head
    - [x] **B.6.5** `[I1]` Audit repairs before the fit
    - [ ] **B.6.6** `[R2]` Donor-form reconciliation
    - [ ] **B.6.7** `[V]` Joint search SPSA
- [ ] **B.7** Board review and rebuild
    - [ ] **B.7.1** `[R2]` Board contract audit and instruments
    - [ ] **B.7.2** `[R2]` Board review against Stockfish and Triumviratus
    - [ ] **B.7.3** `[I2]` Rebuild behind the board contract
    - [ ] **B.7.4** `[V]` Board speed read and gate
- [ ] **B.8** Cleanup and speed pass
    - [x] **B.8.1** `[I1]` Delete the legacy search kernel (ANY TIME) — before the A.11 gates
    - [ ] **B.8.2** `[I1]` Dead parameters, switches and diagnostics
    - [ ] **B.8.3** `[I1]` Allocation guard
    - [ ] **B.8.4** `[I1]` Large pages on every platform
    - [ ] **B.8.5** `[I1]` Speed pass
    - [x] **B.8.6** `[I1]` Delete the old search arms (ANY TIME) — before B.5
- [ ] **B.9** Robustness, verdicts and protocol before the release
    - [ ] **B.9.1** `[R2]` Lifecycle and protocol audit and tests
    - [ ] **B.9.3** `[R2]` Hard verdicts against Syzygy
    - [ ] **B.9.4** `[R2]` Deferred defects: repair now or justify
    - [ ] **B.9.5** `[I2]` Repairs and the repair gate
- [ ] **B.11** Checkpoint and interim release
    - [ ] **B.11.1** `[V]` Search programme checkpoint
    - [ ] **B.11.3** `[R2]` Originality and licence disclosure
    - [ ] **B.11.4** `[M]` Interim release 1.11.0
- [x] **B.12** `[I2]` False win in drawn KBP-K (ANY TIME) — before B.3.1
- [x] **B.13** `[I1]` Colosseum re-pinned to 0.5.0 (ANY TIME)
- [x] **B.14** `[I2]` Correction added to recognised draws (ANY TIME) — before B.3.2

### Phase C — Evaluation programme (search frozen)

- [ ] **C.0** `[R3]` Evaluation programme investigation
- [ ] **C.1** `[I1]` Evaluation restructure, behaviour-neutral
- [ ] **C.2** Fit pipeline, corpus, labels and gate instruments
    - [ ] **C.2.1** `[I1]` Fit-tooling contract and Texel handbook
    - [ ] **C.2.2** `[R2]` Corpus design
    - [ ] **C.2.3** `[V]` Generate and publish corpus A
    - [ ] **C.2.4** `[R2]` Matched label arms
    - [ ] **C.2.5** `[R2]` Initialization control
    - [ ] **C.2.7** `[R2]` Endgame instruments and gate integrity
    - [ ] **C.2.8** `[V]` Matched fits and the label-contract gate
- [ ] **C.3** `[I2]` King safety and phase cluster
- [ ] **C.4** `[I2]` Threats and mobility cluster
- [ ] **C.5** `[I2]` Pawns and passers cluster
- [ ] **C.6** `[I2]` Material, imbalance and pieces cluster
- [ ] **C.7** Endgame handling and winnability
    - [ ] **C.7.2** `[R3]` Occurrence, classification and ranking
    - [ ] **C.7.3** `[I2]` Generic winnability and scaling
    - [ ] **C.7.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
    - [ ] **C.7.5** `[I2]` Group B families by measured kind
    - [ ] **C.7.6** `[R2]` Lower-yield remainder
    - [ ] **C.7.7** `[V]` Endgame gate and closure
- [ ] **C.8** `[V]` Refit cycles
- [ ] **C.9** `[V]` Nonlinear HCE SPSA or a written skip
- [ ] **C.10** Joint re-fit on the new evaluation
    - [ ] **C.10.1** `[R2]` Joint-fit investigation and registration
    - [ ] **C.10.2** `[V]` The joint fit, PGO and gate
- [ ] **C.11** `[V]` Freeze the classical evaluation
- [ ] **C.12** `[I2]` Evaluation throughput, bit-exact
- [ ] **C.13** `[M]` Interim release after the evaluation programme

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
