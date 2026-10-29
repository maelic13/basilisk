# BAS-P24

**BAS-P24 — the universal binary on Stockfish's assembly route, for Clang and GCC**

- **Registration.** Registered 2026-10-29, before any build of its pools and
  before any reading; PLAN A.15.9. Builds agent-run; the read needs the host
  idle and is run when the maintainer frees it:
  `pwsh tools/run_a159_nps_reads.ps1` (`-BuildOnly` builds the pools and
  stops).
- **Background.** A.15.9 moves the universal binary to one route for both
  compilers: each tier's LTO output is kept as assembly (Clang's linker
  writes it on a second link of the same objects; GCC's `-save-temps` keeps
  each partition's), its symbols are renamed in the text, and it is
  assembled for the final link. Clang's route until now kept the LTO
  objects and renamed them with `llvm-objcopy`
  (`BASILISK_UNIVERSAL_ROUTE=objects`, kept for this read). The code is the
  same LTO output either way; what can differ is the assembler's round trip
  and the placement the relink gives it (BAS-P20, BAS-P21). GCC's universal
  binary has never been read.
- **Instrument.** `tools/nps_read.ps1`, one read of five pools with named
  contrasts, decided by `tools/diag/nps_read.py` (PROCESS *Build, bench and
  NPS*), as BAS-P22.
- **Builds**, one revision, four `build_test.ps1` Release builds a pool,
  `bench 13` 19,470,325, Clang 23.1.4 and MSYS2 GCC 16.2.0:
  - `a159-casm`: `-Universal`, Clang's universal binary on the assembly route;
  - `a159-cobj`: `-Universal -UniversalRoute objects`, today's Clang route;
  - `a159-cpext`: the pext asset's recipe, Clang;
  - `a159-gasm`: `-Universal -Gcc`, GCC's universal binary;
  - `a159-gpext`: `-Gcc`, the pext recipe built by GCC.
- **Contrasts**, each in the no-regression form:
  1. `route`, `casm` against `cobj`: the assembly route against today's;
  2. `hot`, `casm` against `cpext`: the new route still matches the per-tier
     build, BAS-P21's question;
  3. `gcchot`, `gasm` against `gpext`: GCC's universal binary against GCC's
     own pext build.
- **Question.** Does the assembly route cost Clang's universal binary any
  speed, and does GCC's universal binary match GCC's single-tier build?

#### PRE-REGISTERED PREDICTION — freeze before exposure

- `route`: 0.0%, 80% interval [−0.4%, +0.4%], PASS with probability about
  0.85: the same LTO output, the same 64-byte placement of every function.
- `hot`: −0.1% [−0.6%, +0.4%], PASS with probability about 0.8 (BAS-P21
  +0.00%, BAS-P22 −0.14% and +0.10% for the objects route).
- `gcchot`: −0.2% [−1.0%, +0.6%], PASS with probability about 0.7: GCC
  leaves its never-run functions unaligned, and its relink is unread.

- **Decision rule.**
  - `route` PASS: Clang's universal binary takes the assembly route and the
    objects route is removed, one route for both compilers. `route` FAIL:
    Clang keeps the objects route and only GCC uses the assembly one
    (PLAN A.15.9's rule).
  - `hot` FAIL with `route` PASS: the loss is common to both routes, a
    placement question; nothing changes in the release and the leaf returns
    to research.
  - `gcchot` decides no shipped asset (the release ships Clang's): its
    result goes into `docs/release_tiers.md`. A FAIL is reported with the
    builds' placement, as BAS-P20's was.
- **Stop rule:** step 1, step 2 only when a contrast says EXTEND; a REPEAT
  repeats the read once into a fresh directory, a second stops for the
  maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, a
  `bench 8` warm-up per build, 60 s under 10% host load, no engine, harness
  or build process.
- **Reading time:** five pools, 20 readings a cycle: about 15 minutes at
  step 1, about 40 with step 2.

#### Registration amendment 1 — 2026-10-29, while building the pools, before any reading

- **Two revisions.** The `a159-casm` pool was built at `e092f0c`. The first
  `a159-cobj` build then failed: the `pgo` target did not forward
  `BASILISK_UNIVERSAL_ROUTE` to its child builds, so they took the default,
  the assembly route, and kept no LTO objects. `7a346fe` forwards it. The other
  four pools are built after this amendment, from `7a346fe`'s source (the
  commits after it are records); for the assembly route the forwarded value
  is the default the `casm` builds already had, so their recipe is
  unchanged. Nothing else changes.

#### RESULT — append after exposure

- **Result (agent-run 2026-10-29, 21:29-21:57 local, pools at `e092f0c`
  (`casm`) and `842259f` (the rest), `tools/results/a159-nps/p24/` and
  `p24-repeat/`).** The maintainer judged the host idle enough; WSL was
  shut down first. Every build 19,470,325.
  - **Read 1:** host 6.5% in cycle 1, 2.3% in cycle 2; four arms' two cycle
    levels disagreed by more than 1% (cycle 1 slower), so every contrast
    read REPEAT_STEP_1.
  - **Read 2 (the repeat),** host 1.5% and 2.9%; means of build medians:
    Clang assembly route 3.2996M, objects route 3.2886M, Clang pext 3.2984M,
    GCC universal 3.1527M, GCC pext 3.1786M.

  | Contrast | Delta | 95% t-interval | Verdict |
  |---|---:|---|---|
  | `route`, assembly against objects (Clang) | +0.33% | [−0.19%, +0.86%] | **PASS** |
  | `hot`, assembly route against Clang pext | +0.04% | [−0.21%, +0.29%] | **PASS** |
  | `gcchot`, GCC universal against GCC pext | −0.81% | [−1.88%, +0.26%] | REPEAT_STEP_1 (its universal arm disturbed: cycles 3.137M and 3.169M) |

- **Stop.** By the stop rule a second REPEAT stops for the maintainer.
  `route` and `hot` were undisturbed and decided in the repeat; `gcchot`
  is undecided twice, its GCC universal arm disturbed in both reads.
- **Calibration.** `route` +0.33% against 0.0% [−0.4%, +0.4%] and `hot`
  +0.04% against −0.1% [−0.6%, +0.4%], inside their intervals; `gcchot`
  −0.81% is outside its interval of −0.2% [−1.0%, +0.6%] on a disturbed
  arm, so not yet a calibration point.

#### Registration amendment 2 — 2026-10-29, after the second read, before the third

- **A third read of `gcchot` alone, at the maintainer's request.** The
  maintainer closed every application for a quieter host. Only the two GCC
  pools are read (`a159-gasm`, `a159-gpext`), the one contrast `gcchot` as
  registered, step 1 and step 2 on EXTEND, into
  `tools/results/a159-nps/p24-gcc/`. `route` and `hot` keep their verdicts
  from the second read.
- **Decision:** as registered; a REPEAT here stops for the maintainer with
  no further read.
