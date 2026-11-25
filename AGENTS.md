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

This file is always in force. The files under `agents/` are modules, as
binding as this file: read one in full before starting when any of its
triggers is true, and skip it only because none is, never to save time.
Triggers are facts -- the task's kind, the paths changed, the actions taken --
and most leaves fire several. When unsure, read the module.

| Module | Scope | Task | Paths changed | Actions |
|---|---|---|---|---|
| `agents/research.md` | Mechanisms, research, reviews, donors, refusals and experiment design. | Research, diagnosis, experiment design or a review; any `R2` or `R3` leaf. | `docs/DESIGN.md`, `docs/experiments/`, `analysis/`. | Before proposing, adding, removing or altering a mechanism, a code change meant to alter a move included; when a change moves the bench fingerprint; before reading another engine's source or documents; before registering an experiment or closing a leaf that ran one. |
| `agents/implementation.md` | Engine, test, build and tool code. | Any `I1` or `I2` leaf; a review of code. | `src/`, `tests/`, `tools/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/`. | Before writing or changing code anywhere else. |
| `agents/measurement.md` | Measurement, acceptance, gates and long runs. | Any `V` leaf. | `docs/experiments/`, `docs/EXPERIMENTS.md`, `tools/colosseum/`, `tools/run_*`, `.github/windows-clang-version`. | Before building a binary to measure; before running or reading bench, NPS, `Diag` counters or a profile; before running, registering or preparing games, an SPRT, SPSA, gauntlet, datagen or fit; before handing the maintainer a command or launcher; before any run that loads the machine for minutes; before reading returned artifacts. |
| `agents/records.md` | Roadmap and ledger documents, evidence, citations, tags, capability classes and the pull-request message. | Any `M` leaf; closing any leaf. | `docs/PLAN.md`, `GUIDE.md`, `CHANGELOG.md`, `docs/HISTORY.md`, `docs/EXPERIMENTS.md`, `docs/experiments/`, `docs/archive/`, `analysis/`, `tools/diag/citation_exceptions.tsv`. | Before citing a commit in a document; before adding a result, log or fixture to git; before creating, pushing or deleting a tag, or keeping a branch; before opening a pull request or writing its message; before changing a capability tag; when a change is user-visible (strength, UCI options or output, defaults, the bench fingerprint, platforms). |

**Before reporting work as done**, hold the triggers against what happened:
the files changed, the commands run, and the commits, registrations and
hand-overs made. `python tools/diag/check_rules.py --triggered` lists the
modules whose path triggers the branch's changes fire; check the task and
action triggers by hand. Read in full any triggered module not yet read,
recheck the work against it, and name the modules read in the report.

If a rule conflicts with the task as given, or with another rule, stop and
ask; do not choose between them silently.

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

- **A false premise returns the leaf.** Keep useful instrumentation, record
  the contradiction with its evidence and options in the leaf's packet, and
  return the leaf to `RESEARCH`. The researcher answers with a numbered
  research amendment in the same packet; implementation resumes only on the
  amended contract. This is PLAN §2's research–implementation loop, and a
  return is the method working, not failing.

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

- Orient once per session: these rules, the modules whose triggers are true,
  GUIDE's *Now* table and next step, and in PLAN the selected leaf's entry
  with the lead text of its parent step and its phase -- never the whole
  phase, and §2 only for a rule cited by number. Follow up with
  targeted searches and bounded excerpts; re-read only changed regions or to
  answer a concrete question.
- Batch independent reads and checks; send verbose output to logs and return
  exit status plus a short result. Back off unchanged polls: waiting on the
  CPU is not reasoning.
- Keep a compact working record: leaf, source and binary identity, evidence
  paths, completed checks, live process IDs, blocker, next action. After an
  interruption, inspect it and existing outputs before restarting anything.
- Before repeating any read, check or run, name what changed or what it
  answers; if nothing, skip it.

## Commits and reporting

- `master` is the trunk. Every coherent piece of work -- one feature or
  experiment, or one tooling or documentation change -- lives on its own
  short-lived branch from `master` and lands as ONE squash commit through a
  pull request: no merge commits, no rebase merges. On the branch, commit in
  small verified steps after every completed step or substep; GitHub keeps
  those commits under the PR (`refs/pull/<n>/head`) after the squash.
- Commits on a branch are working records, kept under the PR: a concise
  imperative subject that may name the plan leaf or phase ("Prepare B.0.1").
  Engine changes and tooling or documentation changes go in separate commits.
- No AI attribution anywhere: no `Co-Authored-By`, no "Generated with" line,
  in commits, PRs or files. Do not amend, push, open or merge a PR, or rewrite
  history unless the maintainer explicitly asks.
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
  recommendation from its class and GUIDE's mapping (`Claude: <model>`, the
  tier only, with a thinking mode only where the leaf needs a specific
  one) and a one-line reason. Prefer the least costly model judged
  sufficient; never substitute a model the mapping does not name.

## Changing these rules

- Budget: **18,000** bytes for this file with everything a tool loads beside
  it, and for each module, and **200** lines for this file: 75% of the
  smallest documented load limit, and Claude Code's target for one rule file
  (`analysis/agent_rules_2026-11-25.md`). `python tools/diag/check_rules.py`
  enforces both, with the index and the module headers, in CI. Raising either
  is the maintainer's decision, recorded here with its reason.
- A new rule goes into the module whose triggers reach the work it governs,
  and a new trigger into the index and the module's header together. Only a
  rule whose violation does damage on any task enters this file; if it does
  not fit, a rule here moves to a module or is compressed first. The incident
  behind a rule goes to the ledger or an `analysis/` packet, cited beside it.
- A tool stub such as `CLAUDE.md` may import this file and never a module: an
  import is loaded on every task. The rules are kept for Claude, Codex,
  Antigravity and Gemini CLI; no other tool's limit shapes them.
