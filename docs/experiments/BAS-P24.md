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
