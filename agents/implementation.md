# Implementing engine and tool changes

Read this for `I1` and `I2` leaves and any change to engine source or tools.
Workflow ownership (what implementation owns and must not change) is in
`AGENTS.md`. A change that adds, removes or alters a mechanism also needs
`agents/research.md`, *Before implementing a mechanism*; one that is
measured or gated also needs `agents/measurement.md`.

## Per-node code

Per-node code (negamax, qsearch, move picking, move generation, make/unmake,
evaluation, SEE, TT probes) does not allocate (no `new`, no growing
`std::vector` or `std::string`), take a lock, touch a reference count
(`std::shared_ptr`), or pass or return a large value by copy. Ownership is
solved with references, make/unmake and per-thread arrays. An exception states
its reason in one sentence and carries an NPS measurement by PROCESS's method.
A shared atomic states what it signals and which search it belongs to. PLAN
B.7.1's allocation guard checks the first clause.

## Qualifying a change

- Exact bench identity is a necessary deterministic fingerprint, not proof of
  behavioral identity. Evaluation activation, terminal logic, time handling
  and other path-dependent changes can alter play while leaving bench equal;
  apply their domain-specific tests and registered game gates regardless.

## Code and commits

- Never relax a correctness test in the commit whose change made it fail; fix
  its precondition in its own commit, with the justifying measurement.
- Comments explain the problem or the invariant, briefly: no roadmap step
  numbers or ledger IDs as the explanation, no narration of earlier versions.
