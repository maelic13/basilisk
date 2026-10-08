# Basilisk agent working contract

These instructions apply to every agent working in this repository. The goal
is the strongest possible correct chess engine, developed through reproducible
evidence rather than intuition alone. `GUIDE.md` says what to work on; the
relevant section of `docs/PLAN.md` says why; `docs/PROCESS.md` holds the
procedures these rules assume; `docs/DESIGN.md` holds what must stay true of
the engine. Each
rule is stated once: here if every task needs it, otherwise in the one
`agents/` file that owns it.

## Rules by task

This file is always in force. Before starting, read the `agents/` files the
task needs; most leaves need more than one, and closing any leaf needs
`agents/records.md`.

| Task | Read |
|---|---|
| Research, diagnosis, experiment design, reviews; `R2`/`R3` leaves; proposing or changing a mechanism | `agents/research.md` |
| Engine or tool code; `I1`/`I2` leaves | `agents/implementation.md` (plus `agents/research.md` *Before implementing a mechanism* for any playing change) |
| Builds to measure, bench/NPS/counters, games, SPRT, SPSA, gauntlets, datagen, fits, long runs, returned artifacts; `V` leaves | `agents/measurement.md` |
| PLAN, GUIDE, HISTORY, EXPERIMENTS or `analysis/` edits, evidence placement, tags; `M` leaves; closing any leaf | `agents/records.md` |

## Unit of work

- Treat `docs/PLAN.md` as the roadmap: every step's title, class, status and detail
  live there. GUIDE.md is the maintainer's overview; its step list is
  generated from PLAN.
- First classify the requested work as research/diagnosis, experiment design,
  implementation, deterministic qualification, performance qualification,
  playing-strength gate, or documentation/provenance. A roadmap leaf is not
  automatically an instruction to write code. PLAN owns each leaf's workflow
  state and capability class; GUIDE maps classes to models.
- "Do the next step" means handle only the earliest unchecked leaf in
  roadmap order. If a step has sub-steps, the next unit is the earliest
  unchecked sub-step, not the whole parent step: when A.2 is first, "next
  step" means A.2.1 only. A leaf marked `(ANY TIME)` is done between leaves
  when the maintainer asks for it, and always before its stated deadline.
- Do not start later steps, combine adjacent steps, or pull forward useful
  side work. Mark a parent complete only after all its sub-steps are complete.
- Finish the requested leaf, verify it proportionately, record it in `docs/PLAN.md`,
  regenerate GUIDE.md, commit both, report briefly, name the next unchecked
  leaf, and stop for the maintainer's next command.
- After any PLAN change run `python tools/diag/check_roadmap.py --write-guide`;
  run `python tools/diag/check_roadmap.py` whenever PLAN or GUIDE changes. It
  fails when GUIDE's generated block is stale.

## Workflow states and ownership

The normal playing-change path is:

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

Not every task uses every state. Documentation may close after
implementation; correctness and behavior-neutral performance work use their
relevant proof or performance gate; research may close with `NO_CHANGE` or
`NOT_WORTH_PURSUING`. A playing-strength change normally may not bypass
`GAME_GATE`.

`READY_FOR_IMPLEMENTATION` is the hard boundary. It means the measured defect,
intended mechanism, local interactions, exact semantics, invariants,
instrumentation, falsifier, cheap qualification and deciding gate are concrete
enough that implementation does not need to invent the chess research.

- **Research owns** the causal question, competing hypotheses, interaction
  map, prospective prediction, falsifiers, the meaning of each experiment and
  the readiness decision. Prefer cheap discriminating evidence to a
  sophisticated implementation of an uncertain idea.
- **Implementation owns** ordinary engineering: idiomatic C++ structure,
  necessary local refactoring, focused instrumentation and tests,
  compilation, debugging and cheap deterministic qualification. The
  maintainer need not prescribe it.
- **Implementation does not** replace the hypothesis, broaden the mechanism,
  add adjacent heuristics, tune unrelated constants, port extra donor
  behaviour, relax a correctness test, change the experiment after exposure,
  or rescue a weak candidate by changing its neighbours.
- **A false premise returns the leaf.** Keep useful instrumentation, record
  the contradiction with its evidence and options in the leaf's packet, and
  return the leaf to `RESEARCH`. The researcher answers with a numbered
  research amendment in the same packet; implementation resumes only on the
  amended contract. This is PLAN §2's research–implementation loop, and a
  return is the method working, not failing.
- **Review is independent.** A cluster implementation is accepted by a
  reviewer in a separate session who reproduces the fingerprints and tests
  and checks the code against the handoff; the acceptance is recorded before
  diagnostics start.

## Capability classes

Open PLAN leaves carry a capability tag, which GUIDE shows:

| Class | Use |
|---|---|
| `R3` | frontier research; unresolved causal or architecture work |
| `R2` | bounded but correctness-sensitive architecture/reasoning, reviews |
| `I2` | difficult implementation requiring strong reasoning |
| `I1` | well-specified implementation |
| `M` | mechanical documentation, manifests or provenance |
| `V` | verification or measurement work |

Classes are routing hints, not permission, state or evidence. The editable
mapping from classes to currently available models belongs only in GUIDE.md.
Do not silently downgrade a class; if scope or uncertainty calls for
escalation, say why, change the PLAN tag and regenerate GUIDE. Completed historical
model tags may remain unchanged.

## Scope and discoveries

- Do the work specified by the current leaf. Avoid opportunistic refactors,
  cleanup, feature additions or unrelated documentation changes.
- If a newly found bug blocks the leaf, stop at the safe boundary and report
  the blocker with evidence.
- If a bug, chess error or promising improvement does not block the leaf,
  finish the leaf first. Report the finding separately at the end without
  implementing it. The maintainer decides whether to do it immediately,
  discard it or schedule it.
- Analyze code in chess terms as well as software terms: legality, terminal
  rules, mate-score semantics, evaluation sign/perspective, phase behavior,
  zugzwang, rule 50, repetition, tablebase WDL/DTZ meaning, search-node type,
  pruning safety, time control and SMP effects all matter.
- Call out suspicious or incorrect chess behavior even when it is outside the
  current leaf, but do not silently expand scope to repair it.

## Verification

Almost every agent mistake in these projects has been a check that did not
check what it was thought to check: a stale binary measured, one record parsed
instead of forty, an exit code read from the wrong end of a pipe, a
fixed-depth test chosen because it provably could not be affected. **Verify
mechanically, never by eyeballing, and never by assuming a tool did what its
name says.**

- Match verification cost to risk. Do not run the entire suite reflexively
  when a syntax check or focused test proves the current change; do not skip a
  bench, game gate or chess-specific test when that is what acceptance needs.
  Run each required check once on the final relevant state, and reuse the pass
  while its inputs are unchanged.
- A behavior-neutral engine change reproduces the immediate development fingerprint (currently **14,978,465** at `bench 13`) plus focused tests for behaviour the bench does not reach.
  Memory, state or concurrency work also needs the relevant sanitizer/stress
  coverage. The comparison fingerprint is revision-specific: a deliberately
  integrated behaviour change updates the fingerprint record; never preserve
  a known defect to keep an obsolete count. Investigate a cross-platform
  mismatch.
- Check exit status directly, never through a pipe. Every scripted edit
  asserts its anchor matched once and preserves the file's line endings.
- Test constructs and behaviour, not words in a comment.

## Interruptions, delegation and communication

- If the maintainer interrupts work with a correction, finish and cheaply
  qualify that correction, report it and return control. Do not resume the
  interrupted objective unless explicitly asked.
- Do not spawn broad parallel agents merely because they are available.
  Delegate only a concrete bounded subproblem whose value exceeds its context
  and compute cost; ordinary bounded work should normally remain coherent in
  one agent.
- Before nontrivial edits, briefly state the approach and affected contracts.
  During longer local work, report meaningful milestones and changed
  assumptions rather than narrating commands.
- A nontrivial final report normally names files changed, semantic effect,
  interactions/invariants, qualification and result, remaining expensive gate,
  false assumptions and unresolved concerns.

## Token-efficient execution

- Orient once per session: these rules, the `agents/` files the task needs,
  GUIDE's *Now* table and next step, the selected PLAN section. Follow up with
  targeted searches and bounded excerpts; re-read only changed regions or to
  answer a concrete question.
- Batch independent reads and checks; send verbose output to logs and return
  exit status plus a short result. Back off unchanged polls: waiting on the
  CPU is not reasoning.
- Keep a compact working record: leaf, source and binary identity, evidence
  paths, completed checks, live process IDs, blocker, next action. After an
  interruption, inspect it and existing outputs before restarting anything.
- Before repeating any read, check or run, name what changed or what it
  answers; if nothing, skip it. Never shrink a registered run after seeing
  results.

## Commits and reporting

- Commit after every completed step or substep. Use a concise imperative
  subject that names the result or numbered leaf where useful. Engine changes
  and tooling or documentation changes go in separate commits.
- Never add co-author trailers. Do not amend, squash, push or rewrite history
  unless the maintainer explicitly asks. `dev` reaches `master` only through a
  pull request the maintainer squash-merges as one `Version X.Y.Z` commit;
  development commits go on `dev`, and an `archive/dev-X.Y.Z` tag keeps them
  reachable once `dev` is deleted (PROCESS *Release*).
- Preserve unrelated maintainer changes and keep generated result artifacts out
  of source commits unless the roadmap explicitly requires them.
- **Every report opens with a one-line recommendation** -- what to do next and
  why, in one sentence, before any evidence. "Revert it; 1.22% NPS is too
  expensive for a defect that has never cost a game" is a recommendation.
  "Here is the measurement, the decision is yours" is not, and neither is a
  command handed over with a caveat explaining why it will not work. Evidence
  and options come after the line, and a genuine judgement call still ends with
  the agent's own position stated plainly.
- When maintainer action is needed, give runnable commands in their own fenced
  block and restate them rather than referring back.
- End-step reports are short: outcome, essential verification, commit, any
  separate findings/ideas, and the exact next unchecked leaf with a model
  recommendation from its class and GUIDE's mapping (`Claude: <model> —
  <mode>`) and a one-line reason. Prefer the least costly model judged
  sufficient; never substitute a model the mapping does not name.
