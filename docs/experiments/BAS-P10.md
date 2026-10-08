# BAS-P10

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P10 |
| Experiment and conditions | GCC PGO advertised `-fprofile-generate/-use`, but the orchestrator always searched for Clang `.profraw` files and invoked `llvm-profdata`; a separate final build tree would also change GCC's object-path-based profile names. |
| Result / disposition | **Tooling repair accepted; search unchanged.** The orchestrator now branches on the configured compiler ID, verifies GCC emitted `.gcda`, and reconfigures/rebuilds the same object tree under `-fprofile-use`. MSYS2 GCC 16.2.0 produced and consumed 15 `.gcda` files without missing-profile diagnostics; the final native PGO binary preserved the exact 12,568,898-node bench. Clang's independent `.profraw`/`.profdata` route remains separate. |
| Conditional lesson and retry trigger | PGO formats and profile lookup identities are compiler-specific. A flag being accepted is not proof that training output was consumed; retain a real GCC PGO build in routine coverage. |
| Source | `cmake/pgo-build.cmake`; `CMakeLists.txt` |
