# BAS-P22

**BAS-P22 — the native release build, GCC and MSVC, against Clang, the universal binary and the pext asset**

- **Registration.** Registered 2026-10-29, before any reading; PLAN A.15.8.
  Agent-run at the maintainer's request: `pwsh tools/run_a158_nps_reads.ps1`.
- **Background.** A.15.8 reduces the presets to `release`, `universal` and
  `debug`; `release` builds for the building machine (`-march=native`) and
  turns PEXT on where the CPU runs it fast. The universal binary does not
  use `-march=native`, and nothing has measured what that costs. MSVC was
  last read in BAS-P09 (19.40, a Core Ultra 7 165H): −11.75% against Clang.
  It still builds (19.51, `bench 13` 19,470,325) but no CI job builds it.
- **Instrument.** `tools/nps_read.ps1`, one read of four pools with named
  contrasts, decided by `tools/diag/nps_read.py` (PROCESS *Build, bench and
  NPS*); the host-load measure of `7996e4d`.
- **Builds**, one clean revision, four `build_test.ps1` Release builds a
  pool, `bench 13` 19,470,325:
  - `a158-native`: `-Native`, the `release` preset as a user builds it on
    this 5950X (PEXT detected), Clang 23.1.4;
  - `a158-pext`: the pext asset's recipe (`release`, `USE_PEXT`,
    `PORTABLE_BUILD`), Clang 23.1.4;
  - `a158-universal`: `-Universal`, the four-tier universal binary;
  - `a158-msvc`: `-Msvc`, the `msvc-release-pext` preset, MSVC 19.51 with
    MSVC's PGO.
- **Contrasts:**
  1. `native`, native over universal, gain form: what the download gives up
     against building it yourself;
  2. `nativecg`, native over pext, gain form: native code generation alone;
  3. `hot`, universal against pext, no-regression form: the four-tier
     binary still matches the pext asset (BAS-P21 read three tiers);
  4. `msvc`, MSVC over Clang's pext, gain form: MSVC's distance from Clang.
- **Question.** How much faster is a native build than the download, and is
  MSVC within reach of Clang? (Amendment 1 adds GCC.)

#### PRE-REGISTERED PREDICTION — freeze before exposure

- `native`: +0.5%, 80% interval [−0.5%, +2.0%]. `-march=native` adds Zen 3
  scheduling and a few extensions the engine barely uses.
- `nativecg`: +0.5% [−0.5%, +2.0%], the same effect without the relink.
- `hot`: −0.2% [−0.7%, +0.3%], PASS with probability about 0.75.
- `msvc`: −10% [−15%, −5%].
- **Decision rule.**
  - `native` and `nativecg` decide nothing in the code: the result goes into
    the README's build section and `docs/release_tiers.md`.
  - `hot` FAIL reopens A.15.7: the fourth tier would have moved placement.
  - `msvc` is the maintainer's to decide on. The registered recommendation:
    MSVC 3% or more behind Clang goes (its preset, `-DCOMP=msvc` and the
    MSVC branches of the build); within 3% it stays, with a CI job that
    builds and tests it as GCC's does.
- **Stop rule:** step 1, step 2 only when a contrast says EXTEND; a REPEAT
  repeats the read once into a fresh directory, a second stops for the
  maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, a
  `bench 8` warm-up per build, 60 s under 10% host load, no engine, harness
  or build process.

#### Registration amendment 1 — 2026-10-29, before any reading

- **All three compilers, each at its best.** At the maintainer's request a
  fifth pool joins: `a158-gcc`, four `build_test.ps1 -Native -Gcc` builds
  (MSYS2 GCC 16.2, `-march=native`, PEXT detected, GCC's PGO; a trial build
  gave 19,470,325). MSVC has no native target, so its best is its pext
  build. The contrasts become six:
  1. `gcc`, GCC native over Clang native, gain form;
  2. `msvc`, MSVC over Clang native, gain form: best against best;
  3. `msvcisa`, MSVC over Clang's pext, gain form: the same instruction
     set, so the compiler alone;
  4. `native`, `nativecg` and `hot` as registered.
- **Predictions added:** `gcc` −2% [−6%, +2%]; `msvc` −10.5% [−16%, −5%];
  `msvcisa` −10% [−15%, −5%] (the registered `msvc` prediction).
- **Decision rule.** The MSVC recommendation reads `msvcisa`, where only
  the compiler differs: 3% or more behind Clang, MSVC goes; within 3% it
  stays with a CI job. GCC stays supported either way (CI builds it); `gcc`
  sets what the README says about compilers.
- **Builds.** `a158-native` and `a158-pext` were built at `72ea2b5`, before
  this amendment; the other three pools at the amendment's revision, whose
  difference is tooling and records only (`build_test.ps1`'s `-Gcc`, this
  launcher, documents), not the engine source or its build files.
- **Reading time:** five pools, 20 readings a cycle: about 15 minutes at step
  1, about 40 with step 2. The read waits for the maintainer to free the
  host.

#### RESULT — append after exposure
