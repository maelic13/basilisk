# Basilisk agent working contract

These instructions apply to every agent working in this repository. The goal
is the strongest possible correct chess engine, developed through reproducible
evidence rather than intuition alone. `GUIDE.md` says what to work on; the
relevant section of `PLAN.md` says why; `PROCESS.md` holds the procedures
these rules assume; `DESIGN.md` holds what must stay true of the engine. Each
rule is stated once, here.

## Unit of work

- Treat PLAN.md as the roadmap and GUIDE.md as its status board.
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
- Finish the requested leaf, verify it proportionately, update PLAN.md and
  GUIDE.md together, commit it, report briefly, name the next unchecked leaf,
  and stop for the maintainer's next command.
- Run `python tools/diag/check_roadmap.py` whenever either roadmap file
  changes.

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
- The host is shared with Rarog. Never start a pinned harness while another
  pinned harness runs, from either repository: both compute the same core
  list, and two at once oversubscribe every core and forfeit games.

## Before implementing a mechanism

- Read `DESIGN.md` first. It holds the engine invariants, the score and mate
  semantics, the TT publication contract and the measurement doctrine.
- Answer these four questions **in the report, before writing the diff**:
  1. What mechanism should produce strength? Name the chess or search
     property. "The reference engine has it" is not a mechanism.
  2. What existing features interact with it? Name them from this codebase with
     file and symbol. "None" is a claim that has to be defended.
  3. What engine invariants must remain true? From `DESIGN.md` section 3, plus
     any the change touches.
  4. What experiment would falsify the idea? Register it, with its verdict rule,
     before running it. An experiment that cannot fail is not evidence.
- Chess-engine techniques are not independent parts. Two engines can both carry
  LMR, correction history, SEE pruning and singular extensions and still need
  different thresholds, because the whole selectivity stack differs. Porting a
  named function is not implementing a mechanism; this is why the search
  programme adopts donor architecture as whole clusters and fits them.
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
  programme or cluster investigation is always handed off, so its packet is
  its deliverable.
- Price the experiment before substantial work: maintainer time, agent effort,
  CPU/game budget, implementation complexity and future maintenance burden.
  Prefer cheap discriminating evidence to elaborate implementation of an
  uncertain idea.

## Donors

- Donor engines teach mechanisms, contracts, dependencies, failure modes and
  methods. The donors are modern Stockfish (search, TT, histories, time,
  threads, later NNUE runtime) and classical Stockfish `9587eeeb` (evaluation
  families, the oracle, seed values). Rarog is a sibling whose *method* this
  roadmap follows and whose results are imported priors; it is not a donor,
  and its verdicts do not transfer (BAS-X01: check-extension removal was +30.75
  there and −10.17 here).
- What may cross, and how the code is written, is `PROCESS.md`, *The
  independence boundary*. Neither similarity nor a copied value is acceptance
  evidence. Deciding that a donor mechanism does not apply here is a
  first-class result.

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
  BAS-E53 exists because a load-bearing claim had never been measured and
  turned out to be false.
- **Report the cost even when the change works.** A candidate that does what it
  claims and costs +79% bench nodes is a rejection, not a trade-off to bury in
  a report's tail.
- **A refusal must be falsifiable too.** State what evidence would change it,
  and what the cheapest experiment producing that evidence would be. "It feels
  risky" is not a refusal; "this violates the mate-band invariant in
  `src/eval.cpp`, and the check that would settle it is X" is.
- **Do not soften a negative result to match what was hoped for**, and do not
  manufacture a disagreement to look rigorous. Both are failures of the same
  duty.

## Capability classes

Open PLAN and GUIDE leaves carry a capability tag:

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
escalation, say why and update PLAN and GUIDE together. Completed historical
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

## Per-node code

Per-node code (negamax, qsearch, move picking, move generation, make/unmake,
evaluation, SEE, TT probes) does not allocate (no `new`, no growing
`std::vector` or `std::string`), take a lock, touch a reference count
(`std::shared_ptr`), or pass or return a large value by copy. Ownership is
solved with references, make/unmake and per-thread arrays. An exception states
its reason in one sentence and carries an NPS measurement by PROCESS's method.
A shared atomic states what it signals and which search it belongs to. PLAN
B.7.1's allocation guard checks the first clause.

## Measurement

- After PLAN A.3, Colosseum CLI is the main harness for gates, fixed matches,
  tunes, null pairs and gauntlets, driven by `tools/colosseum.ps1` from the
  committed run files; fastchess and weather-factory (`tools/sprt.ps1`,
  `tools/spsa.ps1`) stay installed and working as the backup and the second
  opinion. PROCESS's *Harness* section names the cross-check triggers. A
  registered experiment names its runner and never changes it mid-way.
- Never measure strength or NPS on a `TEXEL` or diagnostic build. If a number
  you are not changing changes, check the binary.
- Measure only on an idle host: check CPU use and running engine, harness or
  build processes first, and if the machine is busy stop and ask rather than
  measure. Keep builds and profiling off a match host while it plays.
- Rebuild before measuring, with the exact preset and options. For a
  multi-run study, build once, verify the fingerprint, hash and archive that
  executable, and measure the copy; rebuild only when source, options,
  toolchain or build settings change.
- `bench` and `Diag` counters are per position; sum them with the committed
  tools, never a hand-rolled parser. Before differencing two counters, confirm
  they are in the same unit; a passing invariant does not prove comparability.
  Counter ratios are valid only at sampling stride 1.
- A binary entered in a rated pool is a tagged release or carries its bench
  fingerprint or build flag in its version string, and its ledger row names
  the fingerprint. The harness runner is pinned the same way.
- The current fingerprint is declared once, in GUIDE's checkpoint;
  `check_roadmap.py` fails when this file's or DESIGN's restatement
  disagrees.

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
  Run each required check once on the final relevant state, and reuse the pass
  while its inputs are unchanged.
- A behavior-neutral engine change reproduces the immediate development fingerprint (currently **14,978,465** at `bench 13`) plus focused tests for behaviour the bench does not reach.
  Memory, state or concurrency work also needs the relevant sanitizer/stress
  coverage. The comparison fingerprint is revision-specific: a deliberately
  integrated behaviour change updates the fingerprint record; never preserve
  a known defect to keep an obsolete count. Investigate a cross-platform
  mismatch.
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
- Prove a guard or harness wire live before trusting a null from it: a known-bad
  input must make it fail, an absurd option value must move the numbers.
  Proving the engine responds is not proving the instrument reports it.
- Check exit status directly, never through a pipe. Every scripted edit
  asserts its anchor matched once and preserves the file's line endings.
- Freeze the prospective prediction, confidence, falsifiers and stopping rule
  before result exposure. Afterward append calibration against that frozen
  record. A persuasive retrospective explanation does not prove the result was
  predicted; ask which part of the original causal model was wrong. Clerical
  corrections to a frozen prediction must be explicit.
- Keep evidence layers separate. Better fit loss, tree size, depth, NPS,
  conversion, tactics or donor resemblance is not automatically Elo. Name the
  layer and the gate that actually decides acceptance; do not invent exchange
  rates between unlike measurements.
- Test constructs and behaviour, not words in a comment.

## The one failure mode

Almost every agent mistake in these projects has been a check that did not
check what it was thought to check: a stale binary measured, one record parsed
instead of forty, an exit code read from the wrong end of a pipe, a
fixed-depth test chosen because it provably could not be affected. **Verify
mechanically, never by eyeballing, and never by assuming a tool did what its
name says.**

## Gating

- The strength unit is one dependency-complete, locally fitted cluster;
  internal sub-steps get no gates of their own. Register it in
  `EXPERIMENTS.md` before any games, and never change bounds, cap, book or
  adjudication after seeing games.
- `[0,3]` nElo is the default bracket; `sprt.ps1`'s own default upper bound is
  5, so pass the registered bounds explicitly. Widen only for a genuinely large
  prior and say why. A removal or simplification uses `[-5,0]`; a repair of
  unknown sign uses a symmetric bracket. Size the cap from the drift model at
  the expected value first.
- High bounds reject small gains. That is overnight compute: budget the games,
  do not widen the bounds or poll.
- Bench and counter screens choose candidates and never accept strength. Do
  not invent an acceptance rule after seeing a result.
- An unresolved stop is not "probably fine": a high LOS on a point estimate is
  not evidence the mechanism works.
- One gate, one read: an SPRT's verdict and its estimate at the stop are the
  record; no fixed match of the same pair runs beside it.
- SPSA is conditional: first show activation, interaction and curvature with a
  zero-game sweep; a flat or monotone surface is evidence against the tune. A
  tune runs in registered blocks with a movement stop rule (PLAN rule 7c),
  never on a horizon chosen to fit the answer.

## Evidence

- Git holds source, build and CI files, reusable tools, required fixtures and
  concise records; raw runs, logs, executables, traces and bundles stay in
  ignored `tools/results/` with their paths, recipes and hashes recorded.
  Never force-add evidence.
- Information lives in the tracked documents, not in refs. A ledger row, PLAN
  record or analysis states what changed, why, the result and the decision,
  and carries the exact recipe plus a fingerprint proving a rebuild matched. A
  hash, branch or tag is at most a pointer beside that information.
- Only where exactness is needed and a recipe is impractical is a commit
  preserved, and then by an annotated tag named `<purpose>/<name>` (as
  `oracle/*`, `archive/*`), never by a kept branch. The citing document names
  the tag, why it exists and the condition that retires it. Pushing a tag and
  deleting a remote tag are the maintainer's commands.

## Documents

- `GUIDE.md` and `PLAN.md` change in the same commit when roadmap status or
  requirements change; an AGENTS-only edit needs no PLAN or GUIDE churn.
- GUIDE carries status. Tick a step only when finished and verified, in the
  commit that finishes it; tick the parent when its last sub-step is ticked.
- Sub-steps indent by 4 spaces; nothing goes deeper than three levels
  (`C.5.1`). Let `check_roadmap.py` check the structure rather than reading
  the file.
- Keep GUIDE short: its operator contract, model mapping, prompts, board and
  checkpoint. What a step involves goes in PLAN, a completed record in
  HISTORY, a procedure in PROCESS, evidence in EXPERIMENTS, a derivation in
  `analysis/`.
- `HISTORY.md` is history and resolves every retired numbering scheme; never
  take a next step from it or from `docs/archive/`. When documents disagree,
  source, defaults and reproducible artifacts outrank prose; fix the prose in
  the same change.

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

- Orient once per session: these rules, GUIDE's checkpoint and holds, the
  selected PLAN section. Follow up with targeted searches and bounded
  excerpts; re-read only changed regions or to answer a concrete question.
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
  unless the maintainer explicitly asks. `master` receives only squashed
  `Version X.Y.Z` commits, by the maintainer; development commits go on `dev`.
- Never relax a correctness test in the commit whose change made it fail; fix
  its precondition in its own commit, with the justifying measurement.
- Comments explain the problem or the invariant, briefly: no roadmap step
  numbers or ledger IDs as the explanation, no narration of earlier versions.
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
