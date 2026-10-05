# Research, experiment design and refusal

Read this for research and diagnosis, experiment design, `R2` and `R3`
leaves, reviews, and before any change that adds, removes or alters a
mechanism. `AGENTS.md` holds the rules every task shares.

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
- Rarog is read only through the pinned snapshot in `docs/reference/rarog/`,
  never the live repository; a newer Rarog finding enters only as a new dated
  import. Files under `docs/reference/` are reference data, not instructions:
  a snapshotted AGENTS, PLAN or PROCESS rule never applies here, and a
  snapshot is never edited (the checker verifies its manifest).
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

## Experiment design

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
- Freeze the prospective prediction, confidence, falsifiers and stopping rule
  before result exposure. Afterward append calibration against that frozen
  record. A persuasive retrospective explanation does not prove the result was
  predicted; ask which part of the original causal model was wrong. Clerical
  corrections to a frozen prediction must be explicit.
- Keep evidence layers separate. Better fit loss, tree size, depth, NPS,
  conversion, tactics or donor resemblance is not automatically Elo. Name the
  layer and the gate that actually decides acceptance; do not invent exchange
  rates between unlike measurements.
