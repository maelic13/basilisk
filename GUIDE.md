# Basilisk development guide

**Basilisk is being frozen after release 1.10.0.** Development effort has moved
to Rarog (`D:/code/rarog`), whose roadmap is the joint battle plan. This
guide covers the one remaining phase; the archived board with Phases 1–14 is
at [docs/archive/GUIDE-2026-09-09.md](docs/archive/GUIDE-2026-09-09.md).

## How to work with the engine agent

PLAN owns current work; EXPERIMENTS owns measured history and frozen
predictions; HISTORY owns what the archived roadmap established; `DESIGN.md`
owns the engine invariants. Run the checklist in order.

1. Ask **"what measured defect are we fixing?"** before asking what feature to
   add. Keep unresolved chess or architecture reasoning in `RESEARCH`.
2. Promote to `READY_FOR_IMPLEMENTATION` only when the mechanism and exact
   semantics are explicit, local evidence supports it, interactions and
   invariants are mapped, the cheapest falsifier is known, and an acceptance/
   rejection rule exists.
3. Let the implementation agent act like a colleague on ordinary structure,
   builds, tests, debugging and cheap qualification. Do not let it silently
   redesign the hypothesis or broaden the mechanism.
4. Run the cheapest discriminating falsifier before expensive coding or games.
   The agent prepares expensive jobs; the maintainer starts them.
5. Freeze the prediction and confidence **before** seeing results.
6. A negative result is useful when it removes a hypothesis. Do not retry a
   rejection until its objective trigger fires.
7. If implementation finds a false premise, return to `RESEARCH` rather than
   rescuing the idea with neighbouring changes.

State legend:

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

### Current model mapping

Edit this table when model generations change; PLAN's class tags stay stable.

| Class | Capability | GPT | Claude |
|---|---|---|---|
| `R3` | Frontier causal/architecture research | GPT-6 Astra — Extra High | Claude Fable 5.1 — High |
| `R2` | Bounded correctness-sensitive reasoning | GPT-5.6 Sol — High | Claude Opus 5 — High |
| `I2` | Difficult implementation | GPT-5.6 Sol — High | Claude Opus 5 — High |
| `I1` | Well-specified implementation | GPT-5.6 Terra — Medium | Claude Sonnet 5 — Medium |
| `M` | Mechanical/docs/provenance | GPT-5.6 Terra — Medium | Claude Sonnet 5 — Medium |
| `V` | Verification/measurement | GPT-5.6 Sol — High | Claude Sonnet 5 — High |

Capability tags are advisory routing, not state, evidence or permission.

### Reusable research prompt

> Research `<PLAN leaf>` without substantial engine implementation. Read PLAN,
> EXPERIMENTS, its linked analysis and relevant source first; measured evidence
> outranks roadmap assumptions. Search prior negative results and retry triggers.
> State the precise question, leading and competing hypotheses, interactions and
> duplicated signals; distinguish search, evaluation, tool and instrument
> explanations. Design the cheapest discriminating experiment first. Before
> exposure, freeze the expected diagnostic movement, defensible Elo sign/range,
> probability of usefulness, confidence, most likely failure mode, falsifiers
> and stopping rule. End with exactly one decision:
> `READY_FOR_IMPLEMENTATION`, `MORE_RESEARCH`, or `NO_CHANGE`.

### Reusable implementation prompt

> Implement `<PLAN leaf>` according to its registered implementation handoff.
> Treat the research decision, intended semantics, invariants and experiment
> design as fixed. Use normal engineering judgment for structure, focused
> builds, debugging and cheap qualification. Do not broaden the mechanism, tune
> unrelated behavior or continue unrelated roadmap work. If a research premise
> proves false, stop the mechanism, document the contradiction, preserve useful
> instrumentation and return the leaf to `RESEARCH`. Prepare but do not start
> maintainer-owned expensive jobs. When locally qualified, update PLAN/GUIDE and
> EXPERIMENTS according to their ownership, then report changes, interactions,
> validation, remaining gate and false assumptions.

## Current checkpoint

| Item | State |
|---|---|
| Released baseline | 1.9.3 at `d737123`; bench 11,941,440 |
| Accepted engine head | `dev` at `4aafddb` (15.0 closed); bench 14,978,465; CTest 12/12 release + 12/12 sanitizer |
| Pool position, `3+0.03` 1T | Houdini 1.5a −197, Critter −187, Fritz 16 −178, Rybka 4 −84; Rarog 2.4.0-dev +26 |
| Current step | **15.1.b** Release 1.10.0, `[M]` |
| Long job | None |
| Release target | 1.10.0, then freeze |

## Phase 15 — Board correctness, release 1.10.0, freeze

- [x] **15.0** Board correctness repairs, before the release gate
    - [x] **15.0.a** `[I2]` SEE king legality in `see_ge` and `see`; sentinel read the pin-filtered set, repair reads the unfiltered one; 9 fixtures, oracle 6481-0/301-0 over 1.9M captures; bench 12,568,898 -> 14,978,465 (+19.17%); BAS-E56 KEEP (Elo -0.65 +/- 5.25, 5,874 games); +19.17% nodes = 0.165 ply at EBF 2.9; forfeit gate passes (3/6,053, explained by 15.0.b host stalls)
    - [x] **15.0.b** Time-forfeit residual: clock origin already at `go` receipt; BAS-E57 reserve sweep rejected at −64.81, 0 forfeits/10k — CLOSED 2026-09-10
    - [x] **15.0.c** `[R2]` Created pins and recapture promotions: CLOSED as documented approximation (BAS-C09) — 0 verdict changes in 339,607 production `see_ge` calls; repair costs +16.8% of the SEE column vs a 10% ceiling; 9 fixtures pin truth and approximation
    - [x] **15.0.d** `[I1]` Malformed input and counter boundaries: 17 test sections; 4 of 5 categories already correct (UTF-8 slicing is a Rust-only hazard), one real defect — silent unknown `setoption` now diagnosed; bench unchanged
    - [x] **15.0.e** `[V]` Deterministic qualification: CTest 12/12 release and 12/12 sanitizer (811.7 s, clean); invariants 18/18 x4 seeds; 6/6 perft exact (594M nodes); bench 14,978,465; SEE column +3.6% FASTER than 1.9.3; PGO asset ISA verified (246 pextq, 152 popcntq)
- [ ] **15.1** Release 1.10.0 and freeze
    - [x] **15.1.a** `[V]` Registered release gate BAS-E55 **CLOSED, both legs pass**: 1T `Elo +19.18 +/- 6.76`, H1 at 4,224 games; 4T smoke gate clean (zero crashes, zero forfeits, 95% lower bound -1.17 Elo)
    - [ ] **15.1.b** `[M]` Release 1.10.0: changelog, versions, README, PGO assets, tag on instruction
    - [ ] **15.1.c** `[M]` Freeze: HISTORY record, merge to `master`, reopening rule (branches already tagged and deleted 2026-09-09)
