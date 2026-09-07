# Basilisk agent working contract

These instructions apply to every agent working in this repository. The goal
is the strongest possible correct chess engine, developed through reproducible
evidence rather than intuition alone.

## Unit of work

- Treat PLAN.md as the detailed roadmap and GUIDE.md as its checklist mirror.
- First classify the requested work as research/diagnosis, experiment design,
  implementation, deterministic qualification, performance qualification,
  playing-strength gate, or documentation/provenance. Not every roadmap leaf
  asks for code.
- "Do the next step" means handle only the earliest unchecked leaf item in
  roadmap order. If a numbered step has lettered substeps, the next unit is the
  earliest unchecked substep, not the whole parent step. For example, when
  6.0.a is first, "next step" means 6.0.a only.
- Do not start later substeps, combine adjacent steps, or pull forward useful
  side work. Mark a parent complete only after all its substeps are complete.
- Finish the requested leaf, verify it proportionately, update PLAN.md and
  GUIDE.md together, commit it, report briefly, name the next unchecked leaf,
  and stop for the maintainer's next command.
- Run python tools/diag/check_roadmap.py whenever either roadmap file changes.

## Workflow states and ownership

The normal playing-change path is:

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

Not every task uses every state. Documentation may close after implementation;
correctness and behavior-neutral performance work use their relevant proof or
performance gate; research may close with `NO_CHANGE` or
`NOT_WORTH_PURSUING`. A playing-strength change normally may not bypass
`GAME_GATE`.

`READY_FOR_IMPLEMENTATION` is the hard boundary. It means the measured defect,
intended mechanism, local interactions, exact semantics, invariants,
instrumentation, falsifier, cheap qualification and deciding gate are concrete
enough that implementation does not need to invent the chess research. If a
material premise fails during implementation, preserve useful instrumentation,
record the contradiction and return the leaf to `RESEARCH`; do not silently
redesign or rescue it with adjacent heuristics.

The implementation owner may make ordinary local engineering choices: use
idiomatic structure, perform necessary local refactoring, compile and debug,
write focused tests/instrumentation and run cheap deterministic qualification.
That autonomy does not permit changing the hypothesis, broadening the
mechanism, tuning unrelated constants, importing adjacent donor behavior or
changing experimental meaning after exposure.

## Long-running work

- Agents may run short builds, benches, focused tests and targeted diagnostics
  when they are the appropriate verification for the current leaf.
- Do not start long SPRTs, SPSAs, tournaments, large datagen jobs, long fits or
  comparable machine-saturating work unless the maintainer explicitly asks the
  agent to run it.
- For a required long run, prepare and validate the runnable state, commit that
  state with a clear Prepare <step> subject, provide exactly one copy-pasteable
  single-line command, and stop. Keep the checklist item open.
- After the maintainer returns the artifacts, analyze them, apply the
  pre-registered verdict, finish the checklist/docs, commit with a clear
  Complete <step> subject, report the outcome and next leaf, then stop.
- Respect temporary resource reservations stated in the conversation. Do not
  compete with an active engine job merely because a command is normally short.

## Before implementing a mechanism

- Read `DESIGN.md` first. It holds the engine invariants, the score and mate
  semantics, the TT publication contract and the measurement doctrine.
- Answer these four questions **in the report, before writing the diff**:
  1. What mechanism should produce strength? Name the chess or search property.
     "The reference engine has it" is not a mechanism.
  2. What existing features interact with it? Name them from this codebase with
     file and line. "None" is a claim that has to be defended.
  3. What engine invariants must remain true? From `DESIGN.md` section 3, plus
     any the change touches.
  4. What experiment would falsify the idea? Register it, with its verdict rule,
     before running it. An experiment that cannot fail is not evidence.
- Chess-engine techniques are not independent parts. Two engines can both carry
  LMR, correction history, SEE pruning and singular extensions and still need
  different thresholds, because the whole selectivity stack differs. Porting a
  named function is not implementing a mechanism.
- Consult `EXPERIMENTS.md` before answering question 1. A closed mechanism may
  not be re-proposed without meeting its recorded retry trigger.
- For a substantial playing change, also state the measured defect/opportunity,
  evidence supporting it, credible competing explanations, the cheapest test
  capable of killing the leading hypothesis, and the exact condition for
  `READY_FOR_IMPLEMENTATION`. A plausible idea is not sufficient evidence.
- Prefer interaction-first analysis. Check for duplicate signals, calibration
  around another feature, evaluation/search population shifts, ordering-to-
  pruning feedback, TT amplification/masking, rule-50/repetition/mate effects,
  promotion/material-shed closure, and diagnostic/deployment budget mismatch.
  When an important interaction is cheaply separable, prefer a bounded
  baseline/A/B/A+B screen; this is not a demand to factorial-test every change.
- **Recommend before you document.** When research reaches a decision the
  maintainer is present to make, the first output is a short recommendation
  with its evidence and its main counter-argument -- not a packet. Write the
  full `analysis/` packet once the direction is chosen, or when the maintainer
  asks for it, or when the leaf will be handed off and returned to later. A
  packet written to answer a question that is about to be settled in one
  sentence spends the maintainer's clock on an artifact nobody needed yet.
- Price the experiment before substantial work: maintainer time, agent effort,
  CPU/game budget, implementation complexity and future maintenance burden.
  Prefer cheap discriminating evidence to elaborate implementation of an
  uncertain idea.

## Refutation and refusal

Refusing to build something, and refuting a claim the roadmap already believes,
are **deliverables of equal standing to a diff**. An agent that only ever
implements is failing at half the job.

- **Say "this does not fit; do not implement it yet."** When the four questions
  do not come out clean, the correct output is the reasoned refusal, not a
  best-effort implementation with caveats. A leaf may legitimately close as
  "not implemented, and here is why" -- record it in `EXPERIMENTS.md` with a
  retry trigger and mark the leaf accordingly.
- **Challenge the plan when the evidence does not support it.** PLAN and
  EXPERIMENTS are the maintainer's working beliefs, not settled fact. An
  unmeasured claim carried in the roadmap is a target, not an authority. When a
  step's stated premise is wrong, say so, measure it, and correct the file --
  BAS-E53 exists because a load-bearing claim in 6.5.a had never been measured
  and turned out to be false.
- **Report the cost even when the change works.** A candidate that does what it
  claims and costs +79% bench nodes is a rejection, not a trade-off to bury in
  a report's tail.
- **A refusal must be falsifiable too.** State what evidence would change it,
  and what the cheapest experiment producing that evidence would be. "It feels
  risky" is not a refusal; "this violates the mate-band invariant at
  `eval.cpp:131`, and the check that would settle it is X" is.
- **Do not soften a negative result to match what was hoped for**, and do not
  manufacture a disagreement to look rigorous. Both are failures of the same
  duty.

## Capability classes

Open PLAN and GUIDE leaves may carry an advisory capability tag:

| Class | Use |
|---|---|
| `R3` | frontier research; unresolved causal or architecture work |
| `R2` | bounded but correctness-sensitive architecture/reasoning |
| `I2` | difficult implementation requiring strong reasoning |
| `I1` | well-specified implementation |
| `M` | mechanical documentation, manifests or provenance |
| `V` | verification or measurement work |

Classes are routing hints, not permission, state or evidence. The editable
mapping from classes to currently available models belongs only in GUIDE.md.
Completed historical model tags may remain unchanged.

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

## Verification and acceptance

- Before closing an experimental leaf, audit whether the intervention isolates
  the claimed variable. Check especially for offset-versus-slope coupling,
  multiple parameters changing together, corpus/label policy changing
  together, search-policy changes that alter the sampled distribution, and
  candidate selection being evaluated again on the same data without a
  clearly reported held-out verdict.
- Report an experimental-design confound or unresolved alternative explanation
  immediately. Do not bury it in a later report, call the step complete, or
  advance to confirmation while it can materially change the conclusion. If
  discovered after closure, reopen the affected leaf and correct it first.
- Match verification cost to risk. Do not run the entire suite reflexively
  when a syntax check or focused test proves the current change; do not skip a
  bench, game gate or chess-specific test when that is what acceptance needs.
- A behavior-neutral engine change normally needs focused tests and exact bench
  identity. Memory, state or concurrency work also needs the relevant
  sanitizer/stress coverage.
- Exact bench identity is a necessary deterministic fingerprint, not proof of
  behavioral identity. Evaluation activation, terminal logic, time handling
  and other path-dependent changes can alter play while leaving bench equal;
  apply their domain-specific tests and registered game gates regardless.
- A playing change needs deterministic regression evidence, the relevant
  tactical/endgame tests, bench accounting and an appropriately registered
  strength gate. Reasoning, node counts and static fit loss do not prove Elo.
- Build the actual candidate configuration that will be tested. Keep compiler,
  PGO, binary, book, seed, time control, hash, threads, affinity, adjudication
  and data provenance comparable and recorded.
- Score-based game adjudication is off by default. Use it only for an explicitly
  registered compatibility experiment.
- Never accept a candidate that fails a hard correctness, mate, rule-50,
  tablebase or time-forfeit gate even if its strength estimate is positive.
- Consult EXPERIMENTS.md before retrying a mechanism and record completed
  experimental evidence there without rewriting historical identifiers.
- Freeze the prospective prediction, confidence, falsifiers and stopping rule
  before result exposure. Afterward append calibration against that frozen
  record. A persuasive retrospective explanation does not prove the result was
  predicted; ask which part of the original causal model was wrong. Clerical
  corrections to a frozen prediction must be explicit.
- Keep evidence layers separate. Better fit loss, tree size, depth, NPS,
  conversion, tactics or donor resemblance is not automatically Elo. Name the
  layer and the gate that actually decides acceptance; do not invent exchange
  rates between unlike measurements.

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

## Commits and reporting

- Commit after every completed step or substep. Use a concise imperative
  subject that names the result or numbered leaf where useful.
- Never add co-author trailers. Do not amend, squash, push or rewrite history
  unless the maintainer explicitly asks.
- Preserve unrelated maintainer changes and keep generated result artifacts out
  of source commits unless the roadmap explicitly requires them.
- **Every report opens with a one-line recommendation** -- what to do next and
  why, in one sentence, before any evidence. "Revert it; 1.22% NPS is too
  expensive for a defect that has never cost a game" is a recommendation.
  "Here is the measurement, the decision is yours" is not, and neither is a
  command handed over with a caveat explaining why it will not work. Evidence
  and options come after the line, and a genuine judgement call still ends with
  the agent's own position stated plainly.
- End-step reports are short: outcome, essential verification, commit, any
  separate findings/ideas, and the exact next unchecked leaf.
