# Implementing engine and tool changes

Scope: Engine, test, build and tool code.

Read in full when: Task -- Any `I1` or `I2` leaf; a review of code. Paths --
`src/`, `tests/`, `tools/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`,
`.github/workflows/`. Actions -- Before writing or changing code anywhere
else.

The workflow states and the return of a false premise are in `AGENTS.md`. A
change that adds, removes or alters a mechanism also needs
`agents/research.md`, *Before implementing a mechanism*; one that is
measured or gated also needs `agents/measurement.md`.

## Ownership

- **Implementation owns** ordinary engineering: idiomatic C++ structure,
  necessary local refactoring, focused instrumentation and tests,
  compilation, debugging and cheap deterministic qualification. The
  maintainer need not prescribe it.
- **Implementation does not** replace the hypothesis, broaden the mechanism,
  add adjacent heuristics, tune unrelated constants, port extra donor
  behaviour, relax a correctness test, change the experiment after exposure,
  or rescue a weak candidate by changing its neighbours.

## Per-node code

Per-node code (negamax, qsearch, move picking, move generation, make/unmake,
evaluation, SEE, TT probes) does not allocate (no `new`, no growing
`std::vector` or `std::string`), take a lock, touch a reference count
(`std::shared_ptr`), or pass or return a large value by copy. Ownership is
solved with references, make/unmake and per-thread arrays. An exception states
its reason in one sentence and carries an NPS measurement by PROCESS's method.
A shared atomic states what it signals and which search it belongs to.
`tests/test_allocation_guard.cpp` checks the first clause at run time and
`tools/diag/hot_path_calls.py` the allocation and lock clauses in the shipped
code (a `PROFILE_SYMBOLS` PGO build); a new guarded path joins its KNOWN list
with its reason.

## Qualifying a change

- A behavior-neutral engine change reproduces the immediate development fingerprint (currently **3,880,333** at `bench 13`) plus focused tests for behaviour the bench does not reach.
  Memory, state or concurrency work also needs the relevant sanitizer/stress
  coverage. The comparison fingerprint is revision-specific: a deliberately
  integrated behaviour change updates the fingerprint record; never preserve
  a known defect to keep an obsolete count. Investigate a cross-platform
  mismatch.
- Exact bench identity is a necessary deterministic fingerprint, not proof of
  behavioral identity. Evaluation activation, terminal logic, time handling
  and other path-dependent changes can alter play while leaving bench equal;
  apply their domain-specific tests and registered game gates regardless.

## Code and commits

- Never relax a correctness test in the commit whose change made it fail; fix
  its precondition in its own commit, with the justifying measurement.
- Comments explain the problem or the invariant, briefly: no roadmap step
  numbers or ledger IDs as the explanation, no narration of earlier versions.

## Tools

- New tools are written in Python and portable: Windows stays the primary
  platform, but every new tool is expected to run on Linux in time. Use
  `pathlib`, `subprocess` with argument lists and no shell syntax, and keep
  any Windows-only call (CPU affinity, process times, `.exe` names) behind
  one function that says so and picks the other platform's form or refuses
  plainly (maintainer, 2026-11-12).
- No new PowerShell tools. The existing ones are maintained where they are;
  a command may still be started from PowerShell, and a fix to an existing
  `.ps1` stays in PowerShell rather than forcing a rewrite.
