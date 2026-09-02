# Basilisk agent working contract

These instructions apply to every agent working in this repository. The goal
is the strongest possible correct chess engine, developed through reproducible
evidence rather than intuition alone.

## Unit of work

- Treat PLAN.md as the detailed roadmap and GUIDE.md as its checklist mirror.
- "Implement the next step" means implement only the earliest unchecked leaf
  item in roadmap order. If a numbered step has lettered substeps, the next
  unit is the earliest unchecked substep, not the whole parent step. For
  example, when 6.0.a is first, "next step" means 6.0.a only.
- Do not start later substeps, combine adjacent steps, or pull forward useful
  side work. Mark a parent complete only after all its substeps are complete.
- Finish the requested leaf, verify it proportionately, update PLAN.md and
  GUIDE.md together, commit it, report briefly, name the next unchecked leaf,
  and stop for the maintainer's next command.
- Run python tools/diag/check_roadmap.py whenever either roadmap file changes.

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

- Match verification cost to risk. Do not run the entire suite reflexively
  when a syntax check or focused test proves the current change; do not skip a
  bench, game gate or chess-specific test when that is what acceptance needs.
- A behavior-neutral engine change normally needs focused tests and exact bench
  identity. Memory, state or concurrency work also needs the relevant
  sanitizer/stress coverage.
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

## Commits and reporting

- Commit after every completed step or substep. Use a concise imperative
  subject that names the result or numbered leaf where useful.
- Never add co-author trailers. Do not amend, squash, push or rewrite history
  unless the maintainer explicitly asks.
- Preserve unrelated maintainer changes and keep generated result artifacts out
  of source commits unless the roadmap explicitly requires them.
- End-step reports are short: outcome, essential verification, commit, any
  separate findings/ideas, and the exact next unchecked leaf.
