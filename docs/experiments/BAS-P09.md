# BAS-P09

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P09 |
| Experiment and conditions | Windows x64 PEXT+PGO compiler comparison at `5b92458`, dirty build-support patch `33b60cc9`: MSVC 19.40.33816 (`/O2 /GL /LTCG /favor:INTEL64 /arch:AVX2`, `/GENPROFILE` -> depth-13 bench -> `/USEPROFILE`) versus MSYS2 Clang 22.1.8 (`-O3 -march=native -mbmi2`, LTO+PGO). Two independent builds/arm, Hash 64, 1T, logical CPU 18 (Windows efficiency class 1), High priority, 8 alternating depth-13 rounds. MSVC SHA-256 prefixes `C7DA3551`/`D8174BC6`; Clang `C557183B`/`1F528F4D`. Self-pair first passed at +0.17%, CI [-1.46%, +2.25%]. |
| Result / disposition | **MSVC rejected as the Windows production default; build support retained.** Exact fingerprint 12,568,898 on both arms. Pooled medians: MSVC 2,605,492 NPS, Clang 2,952,260 NPS; MSVC -11.75%, bootstrap 95% CI [-15.54%, -9.23%], 0/8 paired round wins. WAC parser/SAN/floor test passed 5/5 after splitting the embedded corpus for MSVC's literal limit. No games were run; this is a throughput/toolchain verdict, not Elo evidence. |
| Conditional lesson and retry trigger | On this Core Ultra 7 165H P-core and engine revision, MSVC's PGO codegen is materially slower despite equivalent work. Keep Clang for Windows releases; retry MSVC only after a materially newer toolset/codegen change, and repeat on other CPU families before generalizing. |
| Source | `msvc-release-pext` preset; this commit |
