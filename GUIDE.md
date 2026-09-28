# Basilisk development guide

## How to work with the engine agent

1. Ask **"what measured defect are we fixing?"** before asking which feature
   to add. Keep unresolved chess or architecture reasoning in `RESEARCH`.
2. Use the cheapest useful falsifier before expensive coding or games. Search
   prior negative results and do not retry one unless its recorded trigger
   fired (`EXPERIMENTS.md` §9).
3. Promote to `READY_FOR_IMPLEMENTATION` only when the mechanism, semantics,
   local evidence, interactions, invariants, falsifier and accept/reject rule
   are frozen.
4. Let the implementation agent act like a colleague on ordinary code
   structure, builds, tests and cheap qualification. It does not redesign or
   broaden the experiment; a false premise returns the leaf to `RESEARCH`,
   and the researcher answers with a numbered amendment.
5. The agent prepares and verifies long tournaments, SPRTs, SPSA, datagen,
   PGO and profiling jobs; the maintainer starts them. One pinned harness at
   a time on the shared host (Rarog's jobs included), about three SPRT-sized
   runs per day, SPSA overnight.
6. Freeze the prediction before exposure; judge the postmortem against it.
7. A clean negative result is progress. Clusters, not features; compatibility
   over completeness; donor architecture, own implementation.

| State | Boundary / control |
|---|---|
| `RESEARCH` | Evidence, alternatives, interactions, prediction, falsifier and stop rule are being established. |
| `READY_FOR_IMPLEMENTATION` | Research decision frozen; implementation may make ordinary local engineering choices. |
| `IMPLEMENTED` | Intended semantics exist; no qualification claim. |
| `LOCAL_QUALIFIED` | Cheap correctness/performance checks passed; expensive gate prepared. |
| `GAME_GATE` | Registered playing gate running or resolved under maintainer control. |
| `CLOSED` | Accepted, rejected, no-change or deferred disposition and calibration recorded. |

### Current model mapping

PLAN records only stable capability classes. Edit this table when model
generations change; do not rewrite the roadmap. These are maintainer
judgments, not measured rankings. Claude models only, adopted with the
2026-09-28 rewrite as Rarog decided on 2026-09-25.

| Class | Capability | Model — thinking mode |
|---|---|---|
| `R3` | Frontier causal/architecture research | Claude Fable 5.1 — High |
| `R2` | Bounded correctness-sensitive reasoning | Claude Opus 5 — High |
| `I2` | Difficult implementation | Claude Opus 5 — High |
| `I1` | Well-specified implementation | Claude Sonnet 5 — Medium |
| `M` | Mechanical/docs/provenance | Claude Sonnet 5 — Medium |
| `V` | Verification/measurement | Claude Sonnet 5 — High |

### The loop in practice

PLAN §2's *research–implementation loop* is the method; this is who runs each
turn of it.

| Turn | Class | Prompt | Output |
|---|---|---|---|
| Programme or cluster investigation; research amendment after a returned premise | `R3` | research | `analysis/` packet ending in handoffs or `NO_CHANGE`; the next sub-steps |
| Registration, architecture review, implementation review, audits | `R2` | review, or research for an audit | EXPERIMENTS row; review record with fingerprints reproduced |
| Cluster implementation | `I2` | implementation | Ordered tickets behind the umbrella switch; implementation record in the packet |
| Tooling, instruments, behaviour-neutral restructure | `I1` | implementation | Code with its tests and a negative check |
| Documents, inventories, provenance | `M` | implementation | Documents in their owners |
| Gate preparation, sweeps, reading returned artifacts | `V` | implementation | One runnable command; the registered verdict applied |

The reviewer is never the implementer's session. A returned premise never
goes back to the implementer for a fix: it goes to research.

### Reusable research prompt

> Investigate `<PLAN leaf>` as research, not implementation. Read PLAN,
> EXPERIMENTS (including the retry map), HISTORY's number map, DESIGN, the
> linked analysis and the relevant source; measured evidence outranks
> roadmap assumptions. Read the donor (modern Stockfish at the pinned
> revision for search, TT, histories, time and threads; classical Stockfish
> `9587eeeb` for the evaluation and as the oracle) for mechanism, population
> and interaction, never for transcription; read Rarog as a worked example of
> the same method, not as a donor. State the precise question, the leading
> and competing hypotheses, the shared signals and interactions, and whether
> search, evaluation, tooling or instrument effects could explain it. Design
> the cheapest discriminating test first; freeze its prediction, confidence,
> falsifiers and stop rule before exposure. Spawn the implementation and
> measurement sub-steps the handoff needs under the investigation's step,
> with a class each. Finish `READY_FOR_IMPLEMENTATION`, `MORE_RESEARCH` or
> `NO_CHANGE`, with the evidence for that verdict. When answering a returned
> premise, write a numbered research amendment in the same packet.

### Reusable implementation prompt

> Implement `<PLAN leaf>` from its registered handoff. Treat the research
> decision, semantics, invariants and experiment design as fixed. Write the
> donor's mechanism in Basilisk's own structure; do not transcribe. Keep the
> umbrella-off arm at the exact accepted fingerprint at every commit. Use
> normal engineering judgment for code, focused builds, debugging, tests and
> cheap qualification. Do not broaden the mechanism, tune unrelated
> behaviour, relax a correctness test or continue other roadmap work. If a
> research premise is false, stop the mechanism, preserve useful
> instrumentation, document the contradiction with its evidence and options
> in the packet and return the leaf to `RESEARCH`. Prepare but do not start
> maintainer-owned expensive jobs. Report changes, interactions, validation,
> the remaining gate and false assumptions; update PLAN, GUIDE and
> EXPERIMENTS under their ownership rules.

### Reusable review prompt

> Review `<PLAN leaf>` as an independent reviewer; you did not implement it.
> Rebuild both arms from clean sources and reproduce every fingerprint the
> implementation record claims, on each ISA the record names. Run the test
> suites on both arms. Check the code against the handoff clause by clause:
> list every deviation, whether it is an ordinary engineering choice or a
> change of mechanism, and whether the packet records it. Check the
> invariants in `DESIGN.md` §3 and the interactions the handoff names. Do not
> fix anything: record accept, accept with named follow-ups, or return to
> research with the evidence.

## Status board

Every phase, step and sub-step is a checkbox here, with the same identifier,
state, capability tag and ordering marker as in PLAN. Rationale and design
live in `PLAN.md`; durable evidence in `EXPERIMENTS.md`; procedures in
`PROCESS.md`; finished work in `HISTORY.md`. `GUIDE.md` and `PLAN.md` change
together, and `python tools/diag/check_roadmap.py` must pass.

## Current checkpoint

| Item | Value |
|---|---|
| Released baseline | **1.10.1** on `master`, tagged `v1.10.1` (2026-09-27); a correctness patch on 1.10.0 (BAS-C10–C13), which was accepted at +19.18 ± 6.76 Elo over 1.9.3 (BAS-E55) |
| Development head | `dev`, recreated from `master` at `38c42e6` on 2026-09-28; engine source identical to 1.10.1 |
| Bench fingerprint | **14,978,465** (`bench 13`, 1.10.0 and 1.10.1); CTest 12/12 release and ASan/UBSan |
| Pool position, `3+0.03` 1T | 1.10.0 rates 3012 in Rarog's reference pool against Houdini 3 3277, Critter 1.6a 3197, Fritz 16 3165, Rybka 4 3102 (RAR-M45), and 2994 in the Super Rating Tournament (RAR-M54); head-to-heads against the four targets are read at A.7.1. Rarog 2.5.0-dev rates 3233 and scores +200 against 1.10.1 (RAR-M63) |
| Pool position, 4T | Not measured for 1.10.x (A.7.2) |
| Search deficit | +322.7 ± 36 Elo equal time against the classical-Stockfish search oracle on 1.9.3 (BAS-O01, round robin); EBF 2.20 against 1.61. Re-measured as a paired G(0) at A.7.3 |
| Evaluation deficit | +232.8 ± 32 Elo against classical Stockfish's HCE under the same search (BAS-O02) |
| Speed | 3.71 MNPS in Rarog's pooled-PGO measurement (RAR-M48); Basilisk's own pooled baseline at A.7.4 |
| Conversion | 17.5 draws and 5.0 losses per 1,000 after a persistent piece-up, against six anchors (1.9.3; RAR-M54); re-read for 1.10.0 at A.7.1 |
| Active experiment | None |
| Current step | **A.2.1** tracked-file cleanup (`M`) |
| Long job | None |
| Next release | **2.0.0** if the E.2 target gate is met, otherwise **1.11.0**, cut at E.3.2; nothing is released before it unless a correctness repair forces a 1.10.x patch |

## Next and held work

Follow the earliest unblocked leaf in roadmap order. A leaf marked
`(ANY TIME)` is done between leaves when the maintainer asks, and always
before its deadline; held items stay unticked in place.

| Open hold / obligation | Resume or resolve when | Must be resolved before |
|---|---|---|
| B.7.1 allocation guard | Any time between leaves; a test only, no engine change | B.7.2 |
| D.3.1 board contract audit | Any time between leaves; a defect found takes the correctness-repair path with its own gate | E.1 |
| E.3.1 tag-driven release flow | Any time between leaves, never inside a registered experiment's window | E.3.2 |
| Retry triggers the search programme may fire (BAS-S07–S12) | B.0 records which fire and why | B.2.1 |
| KRPPKRP seven-man truth gap | Independent truth becomes available, or C.5.7 records an explicit exclusion | C.5.7 |
| Unsourced 2026-09-04 pool figures in HISTORY | A.7.1 replaces them with a sourced census | A.7.1 |
| Shared host with Rarog | One pinned harness at a time; Basilisk jobs queue with Rarog's | Always |

## Phase A — Reset: documents, harness, instruments, baselines

- [x] **A.1** Document reset: PLAN, GUIDE, PROCESS, AGENTS, DESIGN, HISTORY; archive; checker — CLOSED 2026-09-28
- [ ] **A.2** Repository and inventory
    - [ ] **A.2.1** `[M]` Tracked-file cleanup
    - [ ] **A.2.2** `[M]` Branch and tag disposition: citing document, reason, retirement condition
    - [ ] **A.2.3** `[R2]` Feature, option and parameter inventory, inert knobs with owners
- [ ] **A.3** Harness: Colosseum CLI as the main path
    - [ ] **A.3.1** `[I1]` Run files, `colosseum.ps1` and the shared guards; parity, guard suite, live smoke
    - [ ] **A.3.2** `[I1]` Tune path: surface generated from the X-macro, 15 × 30 shape, block chaining
    - [ ] **A.3.3** `[M]` PROCESS *Harness* section finalised; backup path and cross-check triggers
- [ ] **A.4** Build and toolchain
    - [ ] **A.4.1** `[I1]` Toolchain refresh and freeze, one axis at a time
    - [ ] **A.4.2** `[I1]` Build flavors and manifests for arms; CI option/ISA combination matrix
- [ ] **A.5** Instruments for the search programme
    - [ ] **A.5.1** `[I1]` Fixed-budget probe: WAC at fixed nodes, oracle agreement
    - [ ] **A.5.2** `[I1]` Reference-anchored branching profile with per-position medians
    - [ ] **A.5.3** `[I1]` Counter summation at stride 1 and the decision trace
    - [ ] **A.5.4** `[I1]` Matched ablation mask on Basilisk and the oracle
    - [ ] **A.5.5** `[I1]` PGN conversion instrument, seed-reproduced
    - [ ] **A.5.6** `[R2]` Reference-anchored canaries
- [ ] **A.6** `[R2]` Codebase consolidation analysis: B.1 and C.1 move tables; refactors nothing
- [ ] **A.7** Baselines on the 1.10.1 binary
    - [ ] **A.7.1** `[V]` 1T pool baseline and conversion from the Super Rating Tournament PGN, zero games
    - [ ] **A.7.2** `[V]` 4T gauntlet against the four targets and Rarog 2.4.0 (maintainer-run)
    - [ ] **A.7.3** `[V]` Oracle rebuilt with the 1.10.1 evaluation; paired G(0), 3,000 games (maintainer-run)
    - [ ] **A.7.4** `[V]` Pooled-PGO NPS baseline

## Phase B — Search programme (evaluation frozen)

- [ ] **B.0** `[R3]` Investigation: mechanism map against Stockfish, clusters, scale and seeds, screens, predictions, handoffs B.1–B.3
- [ ] **B.1** `[I1]` Search restructure, behaviour-neutral; exact fingerprint
- [ ] **B.2** Cluster 1 — the selectivity core; `[0,10]`
    - [ ] **B.2.0** `[R2]` Architecture review of the B.1 head and neutral upgrades
    - [ ] **B.2.1** `[I2]` Implement behind the umbrella switch; reviewer acceptance
    - [ ] **B.2.2** `[V]` Diagnostics and the unfitted 2,000-game paired run
    - [ ] **B.2.3** `[V]` Curvature sweep, then SPSA in blocks; bake
    - [ ] **B.2.4** `[V]` Gates: unfitted vs off, fitted vs unfitted; default flip
- [ ] **B.3** `[I2]` Cluster 2 — NMP, ProbCut, singular/multi-cut/negative extensions, IIR, check extension; `[0,3]`
- [ ] **B.4** `[I2]` Cluster 3 — quiescence; SEE value-scale and first-ply check research cards; `[0,3]`
- [ ] **B.5** `[I2]` Cluster 4 — root, aspiration, iterative deepening, MultiPV; `[0,3]`
- [ ] **B.6** `[V]` Joint search SPSA, only if curvature justifies it
- [ ] **B.7** Search speed pass
    - [ ] **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2
    - [ ] **B.7.2** `[I1]` Speed pass; board-speed leaves only if hot
- [ ] **B.8** `[I1]` Cleanup: legacy search, dead parameters, ownerless diagnostics
- [ ] **B.9** `[V]` Checkpoint: deficit meters, attribution, pool gauntlet; freeze the search head

## Phase C — Evaluation programme (search frozen)

- [ ] **C.0** `[R3]` Investigation: family map, residuals, values-versus-structure, cluster order, corpus and label protocol
- [ ] **C.1** `[I1]` Evaluation restructure, behaviour-neutral; exact fingerprint
- [ ] **C.2** Fit pipeline, corpus and label contract
    - [ ] **C.2.1** `[I1]` Fit-tooling contract and the Basilisk Texel handbook
    - [ ] **C.2.2** `[R2]` Corpus design
    - [ ] **C.2.3** `[V]` Generate and publish corpus A (maintainer-run)
    - [ ] **C.2.4** `[R2]` Matched label arms; whole-game tablebase adjudication analysed separately
    - [ ] **C.2.5** `[R2]` Initialization control
    - [ ] **C.2.6** `[V]` Matched whole-surface fits and the label-contract gate
- [ ] **C.3** `[I2]` King safety cluster; refit; gate
- [ ] **C.4** `[I2]` Threats and mobility cluster; refit; gate
- [ ] **C.5** Endgame handling and winnability
    - [ ] **C.5.1** `[R2]` Instruments and gate integrity
    - [ ] **C.5.2** `[R3]` Occurrence, classification and ranking
    - [ ] **C.5.3** `[I2]` Generic winnability and scaling
    - [ ] **C.5.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
    - [ ] **C.5.5** `[I2]` Group B families by measured kind
    - [ ] **C.5.6** `[R2]` Lower-yield remainder
    - [ ] **C.5.7** `[V]` Endgame gate and closure
- [ ] **C.6** `[I2]` Pawns and passers cluster; refit; gate
- [ ] **C.7** `[I2]` Material, imbalance, phase and pieces cluster; refit; gate
- [ ] **C.8** `[V]` Refit cycles; stop at the first non-accepting cycle
- [ ] **C.9** `[V]` Nonlinear HCE SPSA, or a written skip
- [ ] **C.10** `[V]` Search re-fit after the new evaluation; `[0,3]`
- [ ] **C.11** `[V]` Checkpoint and freeze the classical evaluation
- [ ] **C.12** `[I2]` Evaluation throughput, bit-exact

## Phase D — Clock, threads, robustness

- [ ] **D.1** `[R2]` Time management against Stockfish's shape and the ADR-0065 checklist; `[0,3]`
- [ ] **D.2** `[R2]` Lazy SMP quality: self-relative scaling first; 4T `[0,5]`
- [ ] **D.3** Engine lifecycle, protocol and board contracts
    - [ ] **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1
    - [ ] **D.3.2** `[R2]` Lifecycle and protocol robustness
    - [ ] **D.3.3** `[R2]` Displayed-score normalisation research card
- [ ] **D.4** `[R2]` Tablebase policy

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
- [ ] **E.2** `[V]` Target gate: ≥50% against Critter 1.6a, Houdini 3, Rybka 4.1 and Fritz 16 at 1T and 4T
- [ ] **E.3** Release
    - [ ] **E.3.1** `[I1]` Tag-driven release flow (ANY TIME) — before E.3.2
    - [ ] **E.3.2** `[M]` Release 2.0.0 if E.2 is met, else 1.11.0

## Phase F — NNUE (own data only)

- [ ] **F.0** `[R3]` Investigation: board events, accumulators, trainer, data format, first architecture
- [ ] **F.1** `[I2]` Board events and accumulator scaffolding, behaviour-neutral
- [ ] **F.2** `[V]` Data generation at scale
- [ ] **F.3** `[I2]` Trainer hardening and baseline nets
- [ ] **F.4** `[I2]` Scalar integration, integer-exact conformance
- [ ] **F.5** `[I2]` Incremental and SIMD inference
- [ ] **F.6** `[V]` Search re-fit for the network
- [ ] **F.7** `[R3]` Architecture ladder, one axis at a time
- [ ] **F.8** `[V]` Data frontier
- [ ] **F.9** `[M]` NNUE release
- [ ] **F.10** `[V]` CCRL top-100 gate

## Phase G — Scaling, platforms and the top 50

- [ ] **G.1** `[R2]` High-thread and NUMA
- [ ] **G.2** `[I1]` Platform and product; optional universal binary
- [ ] **G.3** `[R3]` Frontier; CCRL top-50 gate
