# Release binary tiers and ISA contract

> **Audience: developer / maintainer** (PLAN §1, "Document audience"). This is
> the build/ISA contract behind the published assets. The *user-facing* version
> of "which download do I pick" is the download table in `README.md`.

Basilisk ships five assets: one x86-64 binary each for Windows and Linux, and
one ARM64 binary each for Linux, Windows and macOS. Every asset is built
**PGO** (profile-guided: instrument → train on the bench suite → merge →
optimised link) from the exact tagged revision with production flags (`TUNE`
off, `PORTABLE_BUILD=ON`), and the release workflow **smoke-tests the exact
file it publishes** before any release exists: the binary must answer
`uci`/`uciok`, name the release version, and reproduce the `bench 13` node
count GUIDE declares, identical across the assets and across every x86 tier
(`.github/workflows/release.yml`; the procedure is `docs/PROCESS.md`
*Release*). Only the binaries are published; the per-build manifest is a local
artifact, and no per-asset `*.sha256` is published.

**Basilisk is 64-bit-only.** No 32-bit binary is built or supported, and the
engine will not compile for a 32-bit target (`static_assert` in `src/types.h`).

## The universal x86-64 binary

The x86-64 asset, `basilisk-<tag>-<os>-x86_64`, is a universal binary
(`analysis/universal_binary_2026-10.md`, the design and its amendments). It
keeps the name the portable asset had, so a link or script that fetched the
plain `x86_64` download now gets the fast build; Stockfish adds `-universal`
to its names, which would break that. It holds four complete builds of the engine,
each its own PGO build, and a dispatcher compiled for the baseline that picks
one at start-up:

| Tier | Codegen | Selected when the CPU has | Notes |
|---|---|---|---|
| `pext` | `-mavx2 -msse4.1 -mpopcnt -mbmi -mbmi2 -mlzcnt`, `USE_PEXT` | SSE4.1, POPCNT, AVX2 (with OS YMM state), BMI1, BMI2, LZCNT, and is not AMD family 15h or 17h | `PEXT` sliding attacks. |
| `avx2` | `-mavx2 -msse4.1 -mpopcnt` | SSE4.1, POPCNT, AVX2 | Also chosen on AMD Excavator and Zen 1/2, whose `PEXT` is microcoded. |
| `sse41-popcnt` | `-msse4.1 -mpopcnt`, `USE_SSE41_POPCNT` | SSE4.1, POPCNT | Hardware `popcount`; Nehalem to Ivy Bridge, AMD Bulldozer to Steamroller. Added 2026-10-29, as Stockfish has it. |
| `base` | none (x86-64 baseline) | anything else | Software `popcount`. |

- The start-up line names the tier: `Basilisk <version> by <author> (pext)`.
  `id name` does not change.
- `BASILISK_ISA=base|sse41-popcnt|avx2|pext` forces a tier. A tier the CPU cannot run is
  refused on stdout (`info string`) and stderr with exit status 1; a forced
  tier slower than the CPU's best is run with an `info string` saying so.
- AMD K10 (Phenom, Athlon II) has POPCNT but not SSE4.1, so it runs `base`.
- The base tier was named `portable` until 2026-10-29; the single-tier
  check keeps that name for the baseline contract (`--tier portable`).

How it is built (the `universal` preset, target `universal`;
`cmake/universal-tiers.cmake`):

1. Each tier is the ordinary PGO build of `release`, `release-avx2` or
   `release-pext` with `BASILISK_UNIVERSAL_TIER=ON`, which keeps the engine
   link's post-LTO objects (lld `-lldsavetemps` on Windows, the gold plugin's
   `save-temps` on Linux) and gives the tier a bare `main`.
2. Every symbol a tier's objects define is renamed `__basilisk_<tier>_<name>`,
   and `run_engine` becomes `basilisk_<tier>::run_engine`. Only type
   identities (`_ZTI`, `_ZTS`) and `.refptr` data stay shared. The script fails
   if anything else stays global, or if a tier object carries an initialiser,
   which would run before the dispatcher has chosen.
3. The dispatcher (`src/main.cpp` under `BASILISK_UNIVERSAL`) links the three
   tiers, one baseline Fathom and the static runtime.
4. Every tier's functions are aligned to 64 bytes (`BASILISK_ALIGN_FUNCTIONS=64`,
   which the preset sets and a universal configure requires). The relink
   places a tier's code differently from its own build, and without the
   alignment it moved the hot functions 16 bytes off their cache-line
   placement and read 0.58% slower (BAS-P20); with it every function starts
   on a cache line in any link.

Local builds and every measurement stay single-tier (`build_test.ps1` builds
the pext tier with the release's portable codegen). The `release`,
`release-avx2` and `release-pext` presets compile `-march=native` unless
`PORTABLE_BUILD=ON` is given; `-march=native` and the universal binary
contradict each other, so the universal builds require `PORTABLE_BUILD=ON`.
The TUNE, DIAGNOSTIC, ABLATION and TEXEL flavours stay single-tier.

## The ISA contract

`tools/diag/verify_isa.py --tier universal` checks the published image. Every
function names its tier, so it is attributed by its label:

- the base tier and everything shared (dispatcher, Fathom, the runtime)
  contain nothing above the floor: no SSSE3, SSE4.1, SSE4.2, AVX, BMI1, BMI2,
  LZCNT or POPCNT;
- the sse41-popcnt tier contains `popcnt`, and no SSE4.2, AVX, BMI1, BMI2 or
  LZCNT;
- the avx2 tier contains `popcnt` and AVX2, and no BMI1, BMI2 or LZCNT;
- the pext tier contains `popcnt`, `pext`, `blsr`, `lzcnt` and AVX2;
- the dispatcher references all three entry points and executes `cpuid`;
- at least 95% of each tier's functions, and every function of the hot path
  (`negamax`, `quiescence`, the move picker, `make_move`, `unmake_move`,
  `is_legal`, `see_ge`, `evaluate`), start on a 64-byte line;
- Fathom's functions are defined once, as shared code, and in no tier.

The floor is x86-64 with SSE3: MSYS2 builds its static libc++ for
`-march=nocona`, and every x86-64 CPU since 2005 has SSE3. `tzcnt` is allowed:
it is LLVM's `REP BSF`, which decodes as `BSF` on a CPU without BMI1 and gives
the same result for the non-zero inputs it is emitted for. On Linux the static
C runtime's archives are exempt, as a dynamic build would load them.

The release also benches every forced tier on its runner, and an Intel SDE job
(`tools/diag/check_universal.py`) checks that Pentium 4 (Linux) starts `base`,
Nehalem (Linux), Sandy Bridge and Ivy Bridge start `sse41-popcnt`, and
Haswell, Skylake, Ice Lake and Alder Lake start `pext`, each reproducing the fingerprint, and that forcing
`pext` on Sandy Bridge is refused. SDE cannot emulate AMD, so the slow-`PEXT`
rule is the unit truth table's (`tests/test_cpu_features.cpp`).

The build checks the tiers before it links them (`cmake/universal-tiers.cmake`):

- the source tree and the compiler are the same for all three: the source is
  hashed before the first tier and after each, and each tier's build
  directory must name the dispatcher's compiler and version;
- after the rename nothing a tier defines stays global except its entry
  point, type identities and `.refptr` data, and no tier object carries a
  static initialiser (`cmake/universal-check-tier.cmake`, which also runs on
  its own against any objects);
- each tier's PGO build treats a source file without profile data as an
  error, except the four bench never runs (`move.cpp`, `syzygy.cpp`,
  `search_diagnostics.cpp`, `wac.cpp`), so a tier that silently lost its
  profile does not build. A profile that does not match its code already
  fails under `-Werror`.

Each of these checks has been seen to fail on a planted fault
(`analysis/universal_binary_2026-10.md` §14).

The single-tier builds keep their own check (`--tier portable|avx2|pext`): a
specialized build's baseline launcher refuses a CPU without the tier's
instructions, and advises a faster tier when the CPU has one.

## Measured numbers

On the development machine (Ryzen 9 5950X, clang 23.1.4, pools of four PGO
builds):

- **The universal binary against the single-tier pext build** (BAS-P21,
  2026-10-29): +0.00% [−0.70%, +0.70%] at step 1, a PASS; −0.29%
  [−0.73%, +0.16%] over six cycles. The relink alone read −0.13% and the
  alignment alone −0.16%, both within noise. Without the alignment the
  universal binary read −0.58% [−0.82%, −0.33%] (BAS-P20).
- **The tier ladder** (BAS-P20, 2026-10-28, each hop read directly):

  | Hop | NPS | 95% interval |
  |---|---:|---|
  | `avx2` over `base` | +5.04% | [+4.45%, +5.63%] |
  | `pext` over `avx2` | +2.82% | [+2.31%, +3.34%] |
  | `pext` over `base` | +8.49% | [+7.83%, +9.15%] |

  At about 2 Elo per 1% NPS (BAS-P01), running `base` on a `pext` CPU
  would give up about 17 Elo; the dispatcher's choice avoids it.

- **The sse41-popcnt tier** (A.15.7, 2026-10-29, indicative: one universal
  build, each tier forced on the 5950X, four interleaved rounds): +5.6%
  over `base`, against +6.0% for `avx2` and +9.2% for `pext`. The hardware
  `popcnt` is nearly all of what `avx2` gains.

Per-tier NPS is measured locally and is not published as a per-asset file.

Sizes (2026-10-29, four tiers, aligned): 4,199,936 bytes on Windows; 5,812,912
on Linux, statically linked with glibc and libstdc++.
