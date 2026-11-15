# BAS-P44

**BAS-P44 — the build's second read: inline threshold 500 and ext-tsp placement (B.8.9, item C)**

- **Registration.** 2026-11-15, B.8.9, before the reads; owed because full LTO won
  BAS-P43 at +3.23% (packet §6, C's second read). Agent-run on the idle host.
- **Change (a throwaway arm, cited by this recipe).** On the head with full LTO
  (`4612bbf`), every engine target adds `-mllvm -inline-threshold=500` at the compile
  and `-Wl,--mllvm=-inline-threshold=500 -Wl,--mllvm=-enable-ext-tsp-block-placement=1`
  at the link (`target_compile_options` and `target_link_options` with `SHELL:` and
  `LINKER:` forms, in `CMakeLists.txt`'s LTO block, when `FULL_LTO` and Clang).
- **Spelling confirmed before the read.** The link accepts both options without a
  warning; both appear in every compile and link command (129 and 29); and they act:
  two plain builds of the head have the same `.text` size (0x1430b6) although their
  bytes differ by path, while the arm's `.text` is 0x159b46 and holds 37 fewer
  functions (more inlining). `bench 13` 4,688,537 on the arm.
- **Arms.** `b89-c-lto` (BAS-P43's winner, the head's build) against `b89-c2`, four
  final-PGO builds of the arm's revision (`nps_pool.ps1 -Revision`), PEXT, clang
  23.1.5, large pages on.
- **Prediction.** None frozen in §8 for the second read; none added after the fact.
- **Reads.** The pooled `bench` read, gain; the cohort read at one engine, reported.
- **Verdict rule (packet §6, frozen, as BAS-P43).** Kept when the `bench` gain
  contrast ACCEPTs and the one-engine cohort estimate is −0.5% or above.
