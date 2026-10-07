# BAS-C07

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C07 |
| Experiment or failure mode | `Board` exposed writable bitboards, occupancies, mailbox, king/check caches, incremental keys and undo history even though those fields are one redundant position representation. Any caller could mutate one view without repairing the others. |
| Disposition | **API containment accepted; search unchanged.** Canonical and derived storage is private, hot consumers use inline read-only scalar/array views, and compile-time guards reject renewed public exposure. Repository-wide callers compile, release and ASan/UBSan suites pass 12/12, and the exact 1T bench remains 12,568,898. |
| Conditional lesson / coverage | Redundant chess state needs one mutation authority. Read-only views preserve hot-path code generation while preventing consumers and tests from manufacturing illegal internal states. No SPRT is warranted for a behavior-neutral access-control change with exact deterministic identity. |
| Source | `src/board.h`; `tests/test_board.cpp` |
