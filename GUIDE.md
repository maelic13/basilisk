# Basilisk development guide

**Phase 15 is complete and 1.10.0 is released** — `master` carries the
`Version 1.10.0` commit tagged `v1.10.0`. **1.10.1 is release-ready on `dev`**:
a ponder-protocol bug fix with bench unchanged; the maintainer's PR,
squash-merge and publish remain. **The next phase is not planned yet**;
`PLAN.md` section 4 says how to plan it. The archived board with
Phases 1–14 is at
[docs/archive/GUIDE-2026-09-09.md](docs/archive/GUIDE-2026-09-09.md).

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
| Latest release | Basilisk **1.10.0**, tagged `v1.10.0` on `master` |
| Release ready | Basilisk **1.10.1** on `dev` — lost-`ponderhit` fix (BAS-C10), setup-on-the-clock fixes (BAS-C11), fatal rejected `position` (BAS-C12) |
| Development branch | `dev`, one release commit ahead of `master` |
| Bench fingerprint | **14,978,465** (unchanged by 1.10.1); CTest 12/12 release and sanitizer |
| Previous release | Basilisk 1.9.3; bench 11,941,440 |
| Strength | **+19.18 ± 6.76 Elo** over 1.9.3 at `3+0.03` 1T (BAS-E55) |
| Current step | **1.10.1 ponder-on game gate, run 2** (maintainer run), then publish 1.10.1; Phase 16 needs planning (`PLAN.md` section 4) |
| Long job | 1.10.1 ponder-on gate run 2 prepared, not started (~1.5 h) |

## Phase 15 — board correctness and release 1.10.0 (complete)

Repaired SEE king legality (BAS-C08); kept created pins and promotion
recaptures as documented approximations on measured reachability and cost
(BAS-C09); closed the time-forfeit residual as host stalls (BAS-E57); hardened
malformed UCI input and fixed a silent unknown-`setoption`; qualified
deterministically (CTest release + sanitizer, perft, invariants, ISA); and
passed the release gate at **+19.18 ± 6.76 Elo** over 1.9.3 (BAS-E55).
Released as 1.10.0. Full summary in `PLAN.md`, measured detail in
`EXPERIMENTS.md`.

## Patch 1.10.1 — lost `ponderhit` (release ready)

- [x] Diagnose the two ponder-on forfeits: `ponderhit` reset by the engine
      thread during search setup after an instant opponent reply (BAS-C10).
- [x] Repair: UCI thread owns the flag (set on `ponderhit`, reset on `go`);
      stale-`stop` reset re-checks the control epoch. Bench 14,978,465.
- [x] Qualify: new tests fail on 1.10.0 and pass; release CTest 12/12,
      ASan/UBSan CTest, TSan on ponder/threading/protocol tests.
- [x] Release prep: version 1.10.1, CHANGELOG, PLAN, GUIDE, HISTORY.
- [x] Ponder-on game gate, run 1: FAIL at ~130 games. The control fired
      (1.10.0 hung ~35 times), but 1.10.1-rc lost 4 games on time in 5-6-man
      positions. Diagnosed as a separate, older defect (BAS-C11).
- [x] Repair BAS-C11: no tablebase line extension; KPK built at start-up;
      hash resize and clear done at `setoption`/`ucinewgame`. Bench 14,978,465;
      release and ASan/UBSan CTest 12/12.
- [x] BAS-C12: a rejected `position` exits with status 1 (maintainer
      decision, replacing 8.6.3a's reject-and-retain); triple check rejected.
- [ ] Ponder-on game gate, run 2 (`V`, maintainer run, ~1.5 h): 1.10.1-rc3
      vs 1.10.0, 1,000 games `10+0.1` 1T. Passes only if 1.10.1-rc3 has zero
      failures and 1.10.0 at least one (PLAN "Patch 1.10.1").
- [ ] Publish (maintainer): PR, squash-merge `dev` into `master` as
      `Version 1.10.1` on clean CI, tag `v1.10.1`, publish the release.
