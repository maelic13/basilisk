# BAS-P11

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P11 |
| Experiment and conditions | Main CI ran only after pushes to `master` or manual dispatch, while AVX2, PEXT, Linux ARM64 and Windows ARM64 were exercised only by the release workflow. Agent changes on the active development/PR path could therefore bypass the strongest available deterministic gates. |
| Result / disposition | **Delivery repair accepted; search unchanged.** CI now triggers for `development`, pull requests, `master` and manual runs. Its release matrix runs full CTest, TUNE compilation and a cross-OS/architecture/ISA bench fingerprint for portable, AVX2 and PEXT x86-64 plus Linux, Windows and macOS ARM64. A separate Linux GCC job generates, consumes and searches with a real PGO profile. Local Windows Clang route checks built all three x86 tiers with TUNE enabled and returned the same depth-10 fingerprint (3,022,019 nodes); target-native ARM and CI-service semantics remain verified by the workflow itself. |
| Conditional lesson and retry trigger | Release-only coverage detects defects after integration. Keep the development matrix aligned with every published platform/ISA tier, and exercise profile generation rather than only accepting PGO flags. |
| Source | `.github/workflows/ci.yml` |
