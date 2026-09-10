# Research and implementation packets

Use `analysis/` for detail that would make PLAN hard to read: architecture
audits, interaction maps, mechanism derivations, discriminating experiments
and implementation handoffs. PLAN remains the authority for current state and
roadmap order; EXPERIMENTS remains the immutable evidence ledger. A packet
links to those authorities rather than copying their history.

Do not create a packet for trivial or mechanical work. Update an existing
packet when it already owns the question. Label claims so a later reader can
separate **evidence** (measured fact), **inference** (interpretation of that
evidence), **hypothesis** (prospective claim) and **decision** (current action).

Research packets use this compact shape, omitting inapplicable sections:

```markdown
# <PLAN leaf> — <research question>

- State / class:
- Owner / date:
- Decision needed:

## Known evidence

Concrete experiment IDs, measurements, provenance and known limits.

## Unknowns and hypotheses

- H1 (leading):
- H2/H3 (credible alternatives only):

## Interaction map

Producer -> stored state -> consumers -> invalidation/undo/reset, plus duplicate
signals, score/search populations, TT, rule-50/repetition/mate, promotion/
material-shed closure and deployment-budget interactions where relevant.

## Cheapest discriminating tests

Test order, isolation, known-bad controls and what each outcome would mean.

## Prospective prediction — freeze before exposure

Expected diagnostics, defensible Elo sign/range, probability of usefulness,
confidence, likely failure mode, falsifiers and stop rule. After exposure,
append calibration in EXPERIMENTS; do not rewrite this section.

## Decision

`READY_FOR_IMPLEMENTATION`, `MORE_RESEARCH`, or `NO_CHANGE`, with the evidence
that decides it and the objective retry trigger if closed.

## Implementation handoff — only when READY_FOR_IMPLEMENTATION

- Goal and exact intended semantics:
- Why it should work in this engine:
- Producer -> stored state -> consumer map:
- Relevant files/subsystems and interacting mechanisms:
- Invariants that must not change:
- Required instrumentation and deterministic tests:
- Cheap local qualification:
- Maintainer-owned expensive gate:
- Acceptance/rejection rule:
- Explicit non-goals and adjacent mechanisms not to alter:
```

If implementation exposes a material false premise, keep useful
instrumentation, record the contradiction and return the PLAN leaf to
`RESEARCH`. Do not rewrite the handoff into a different mechanism after seeing
results.
