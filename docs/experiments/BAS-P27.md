# BAS-P27

**BAS-P27 — Clang for Microsoft's ABI from winget against the MSYS2 build**

- **Registration.** Registered 2026-10-30, before any pool was built and
  before any reading; outside the roadmap, at the maintainer's request.
  Maintainer-run when the host is idle, by a launcher on the port's branch,
  which was not merged (the recipe is *The port* and *Builds* below).
- **Background.** Windows builds come from MSYS2's CLANG64 environment:
  Clang, lld, libc++ and the mingw-w64 headers over the UCRT. The maintainer
  asks whether Windows' own package manager can replace it: LLVM's official
  Windows release (`LLVM.LLVM`), CMake (`Kitware.CMake`) and Ninja
  (`Ninja-build.Ninja`) from winget, with Visual Studio 2026's C++ library
  and Windows SDK. That Clang targets Microsoft's ABI. Both toolchains are
  Clang 23.1.4, so the compiler version is held fixed. BAS-P22 read
  Microsoft's own compiler (`cl`) 14% behind Clang; Clang for Microsoft's ABI
  has never been read.
- **What differs between the arms**, everything else equal (the pext recipe,
  `-O3`, ThinLTO, Clang's PGO trained on the same bench):
  - the C++ standard library: Microsoft's STL against libc++;
  - the C runtime: the static Microsoft runtime (`/MT`, the UCRT linked in)
    against the UCRT DLLs with libc++ linked statically;
  - the linker: lld-link against lld's MinGW driver;
  - the C++ ABI and exception model of each target.
- **The port**, two changes on `master` at `43b4c35`, made before this
  registration and not merged:
  - `CMakeLists.txt`: under Microsoft's ABI (`CMAKE_CXX_SIMULATE_ID` is
    `MSVC`) `STATIC_RUNTIME` defaults to ON and sets every non-utility
    target's `MSVC_RUNTIME_LIBRARY` to `MultiThreaded` (`/MT`) instead of
    linking `-static`; every engine and test target gets
    `_CRT_SECURE_NO_WARNINGS`, since the runtime deprecates `getenv` and the
    tree builds with `-Werror`; the universal binary refuses the ABI.
  - `build_test.ps1 -MsvcAbi`: puts `C:\Program Files\LLVM\bin` first on
    PATH, drops every MSYS2 directory from it, fails unless `clang++`,
    `cmake` and `ninja` resolve outside MSYS2, and builds the ordinary
    recipe in `build/release-pext-msvcabi`, staged as
    `<suffix>-pext-msvcabi-pgo`.
  - Otherwise unchanged: `COMP=clang` finds LLVM's `clang++` on PATH, which
    finds Visual Studio's library and SDK itself, links with lld-link and
    trains PGO with LLVM's own `llvm-profdata`.
- **Trial build** (the port's tree, `-MsvcAbi`): no warnings, `bench 13`
  19,470,325, the executable imports `KERNEL32.dll` only. CTest: 23 of 24
  pass. `fatal_report_from_a_thread` fails: Microsoft's runtime keeps the
  `std::set_terminate` handler per thread, so an exception escaping a helper
  thread aborts (`0xC0000409`) without the `info string CRITICAL ERROR`
  report. It does not touch bench or speed; a switch would need it fixed.
- **Instrument.** `tools/nps_read.ps1`, decided by `tools/diag/nps_read.py`
  (PROCESS *Build, bench and NPS*), no-regression form.
- **Builds**, two pools of four `build_test.ps1` Release builds of one clean
  revision (the port), each pooled by `nps_pool.ps1 -ExpectBench 19470325`:
  - `p27-msys2`: the pext asset's recipe with MSYS2's Clang 23.1.4, the
    control;
  - `p27-msvcabi`: the same recipe with `-MsvcAbi`, LLVM 23.1.4 from winget.
- **Contrast:** `msvcabi`, `p27-msvcabi` against `p27-msys2`, no-regression
  (`-Contrasts 'msvcabi=p27-msys2:p27-msvcabi:no-regression'`).
- **Question.** Is the winget toolchain at least as fast as MSYS2's?

#### PRE-REGISTERED PREDICTION — freeze before exposure

- `msvcabi`: −0.5%, 80% interval [−2.5%, +1.5%]; PASS with probability
  about 0.5. The compiler and its optimisation are the same; the hot path
  uses fixed arrays and bit operations, not the standard library, so the
  library and runtime reach it only through the few calls it makes and code
  placement.
- **Decision rule.**
  - PASS: speed does not stand in the way of switching Windows to the
    winget toolchain. The switch stays the maintainer's decision, with its
    other costs named: the per-thread terminate handler, the universal
    binary's GNU-only relink, and CI, which installs MSYS2 today.
  - FAIL: Windows stays on MSYS2; the port's branch is closed unmerged.
- **Stop rule:** step 1, step 2 only when the contrast says EXTEND; a
  REPEAT repeats the read once into a fresh directory, a second stops for
  the maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, a
  `bench 8` warm-up per build, 60 s under 10% host load, no engine, harness
  or build process.

#### RESULT — append after exposure

- **Result (maintainer-run 2026-10-30, 08:54-09:00 UTC, both pools at the
  port's revision; host 0.4-0.8%, no busy process, both cycles
  undisturbed; CPU 30; `tools/results/p27-nps/p27/`).** Every build
  19,470,325. Means of build medians: MSYS2 3.5198M (3.515M to 3.524M),
  winget 3.3404M (3.336M to 3.345M); each pool's builds agree within 0.3%.

  | Contrast | Delta | 95% t-interval | Verdict (step 1) |
  |---|---:|---|---|
  | `msvcabi`, winget against MSYS2 | **−5.10%** | [−5.28%, −4.92%] | **FAIL** |

- **Decision (the registered rule).** FAIL: Windows stays on MSYS2. Clang
  for Microsoft's ABI is 5.1% slower than the same Clang 23.1.4 targeting
  MinGW, about 10 Elo at `3+0.03` by BAS-P01's rate. The port is not
  merged and its branch is deleted; this entry carries its recipe.
- **Not investigated.** Locating the 5% could only speed up a build that is
  not shipped, so it is not pursued. Thread-local storage, one candidate
  that could also have cost the MinGW build, is ruled out: the engine's only
  `thread_local` exists in Texel builds alone.
- **Clerical, after the result:** the registration's launcher and revision
  citations pointed at the deleted branch; they now point at the recipe.
  The prediction and the decision rule are unchanged.
- **Calibration.** −5.10% against −0.5% [−2.5%, +1.5%]: outside the
  interval, wrong in size. The wrong part of the model: that with the
  compiler held fixed, the library, runtime and ABI reach the hot path only
  through a few calls and code placement. Something in the target costs
  5%; this read does not say what (candidates: thread-local storage access,
  the static runtime's `memcpy`/`memset`, stack probes, code layout). A
  `profile.ps1` comparison of the two pools would locate it, if the
  question is ever reopened.
