# Release binary tiers and ISA contract

> **Audience: developer / maintainer** (PLAN §1, "Document audience"). This is
> the build/ISA contract behind the published assets. The *user-facing* version
> of "which download do I pick" is the download table in `README.md`.

Basilisk ships one binary per (OS, architecture, CPU-feature tier). Every
release asset is built **PGO** (profile-guided: instrument → train on the bench
suite → merge → optimised link) from the exact tagged revision with production
flags (`TUNE` off, `PORTABLE_BUILD=ON`), and CI **smoke-tests the exact file it
publishes** before any release exists: the binary must answer `uci`/`uciok`,
name the release version, and reproduce the `bench 13` node count GUIDE
declares, identical across all nine assets — proving the move generator,
evaluator and search survived PGO on that runner
(`.github/workflows/release.yml`; the procedure is `docs/PROCESS.md`
*Release*). Only the binaries are published as release
assets; the per-build manifest (revision, compiler, bench fingerprint, NPS) is a
**local** artifact by the 8.6.5 decision and is not uploaded, and no per-asset
`*.sha256` is published — downloads are served over GitHub's HTTPS release
infrastructure.

## Tiers and the CPU each requires

**Basilisk is 64-bit-only.** Every tier below targets x86-64 or aarch64; no
32-bit binary is built or supported, and the engine will not compile for a
32-bit target (`static_assert` in `src/types.h`). This has always been true in
practice — the bitboard primitives use unconditional 64-bit builtins — and is
now stated as a contract rather than left implicit.

| Asset suffix | Target contract | Requires | Notes |
|---|---|---|---|
| *(none)* | Portable x86-64 baseline (SSE2) | Any 64-bit x86 CPU | Safe default; runs everywhere. Uses a **software `popcount`** — baseline x86-64 has no `POPCNT`, so no `-mpopcnt` is passed (see below). |
| `-avx2` | x86-64 with AVX2 | Haswell (2013+) / Zen (2017+) | AVX2 codegen (`-mavx2 -msse4.1 -mpopcnt`). |
| `-pext` | x86-64 with SSE4.1, POPCNT, AVX2, BMI1, BMI2/`PEXT` and LZCNT | Haswell+ / **Zen 3+** (2020+) | Uses `PEXT` for magic bitboards. On Zen 1/2 `PEXT` is microcoded and slow — use `-avx2` there instead. |
| aarch64 *(none)* | ARMv8-A / NEON | Apple Silicon, ARM64 servers/Windows | NEON is baseline on ARMv8; no separate SIMD tier. |

Pick the **most specific tier your CPU satisfies**: `-pext` on Zen 3+/Haswell+,
`-avx2` on Zen 1–2 and older AVX2 x86, portable on anything else. The `-avx2`
and `-pext` binaries check their required CPU features in a baseline-ISA
launcher (`src/main.cpp`) before entering the optimized engine body. They exit
with a message naming the build to use instead of faulting on an illegal
instruction, on stdout (`info string`) as well as stderr, since a GUI records
stdout. Every x86 build also says, at start-up, when the CPU would run a
faster tier, and the `-pext` build says so on AMD family 15h and 17h, whose
`PEXT` is slow. When unsure, the portable binary always works.

There is intentionally **no `x86-64-v2`/POPCNT tier** yet: it would sit between
portable and `-avx2` and has not been shown to be worth a fourth x86 asset.
Adding it is a build-system change (a new preset + matrix row), tracked as a
possible future tier, not promised.

## `POPCNT` and the tier fast path

`popcount` is on the hot path (mobility, material and attack-map counting), so
which tier gets the hardware `POPCNT` instruction matters. Verified by
disassembly of the built binaries:

- **Portable** (no CPU-feature flags): **software `popcount`** — 0 `popcnt`
  instructions in the binary. This is deliberate and correct: baseline x86-64
  does not guarantee `POPCNT` (it arrived with SSE4.2 / `x86-64-v2`), and a
  hardware `POPCNT` would fault on a CPU that lacks it. Slower, universal, safe.
- **`-avx2`** (`-mavx2 -msse4.1 -mpopcnt`) and **`-pext`** (the AVX2 flags plus
  explicit `-mbmi -mbmi2 -mlzcnt`): **hardware `POPCNT`**. No flag is assumed
  to imply another tier feature.

Verified after the A.4.1 contract repair (2026-09-29, clang 22.1.8,
disassembly counts):

| Tier | `popcnt` | `pext` | `blsr` | `lzcnt` | `tzcnt` |
|---|---:|---:|---:|---:|---:|
| portable (`PORTABLE_BUILD=ON`) | 0 | 0 | 0 | 0 | 225 |
| `-avx2` | 125 | 0 | 0 | 0 | 233 |
| `-pext` (PGO) | 168 | 246 | 411 | 8 | 512 |

`tzcnt` in the portable binary is **not** a tier violation: it is encoded as
`REP BSF` and decodes as plain `BSF` on a CPU without BMI1, so it is
backward-compatible. `popcnt` and `lzcnt` are not — `lzcnt` decodes as `BSR`
and returns a different value — and both are correctly absent.

`tools/diag/verify_isa.py` enforces the positive and negative contracts from
the exact binaries in CI and the release workflow. The portable set forbids
everything above x86-64 with SSE3: SSSE3, SSE4.1, SSE4.2, AVX, BMI1 (other than
`tzcnt`), BMI2, LZCNT and POPCNT. SSE3 is allowed because MSYS2 builds its
static libc++ for `-march=nocona` and every x86-64 CPU since 2005 has it;
SSSE3 is not (AMD K10 lacks it). The `-avx2` set forbids BMI1, BMI2 and LZCNT. It also verifies that the
launcher remains baseline-only and that CPU detection and the transfer to the
optimized engine body are live.

> **Build the portable tier with `-DPORTABLE_BUILD=ON`.** Without it the
> `release` preset compiles `-march=native` (CMakeLists `-march=native` is
> gated only on `PORTABLE_BUILD`), and the resulting host-tuned binary is
> still published into `build/dist/` under the **portable** asset name, where
> nothing in the filename reveals it. A local native build measured 103
> `popcnt` and 130 `blsr` under that name on 2026-09-10. CI is unaffected:
> `release.yml` passes `-DPORTABLE_BUILD=ON` for every matrix row.

A future `x86-64-v2` tier would give `POPCNT` (and SSE4.2) to portable-class
CPUs that have it; it is not currently built (see the no-`v2`-tier note above).

## The universal binary (built, not shipped)

`cmake --preset universal` and `cmake --build --preset universal --target
universal` build one x86-64 executable holding the three tiers, each its own
PGO build, behind a baseline dispatcher that picks pext (avx2 on AMD family
15h and 17h), avx2 or portable at start-up; `BASILISK_ISA` forces one
(`analysis/universal_binary_2026-10.md` and its amendments). Every tier
reproduces the fingerprint, and `verify_isa.py --tier universal` checks it.

It is not a release asset. BAS-P20 read it **0.58% slower** than the pext
asset on the 5950X (95% interval [−0.82%, −0.33%]), with byte-identical hot
code: the relink places the pext tier's functions 16 bytes off their 64-byte
alignment in the pext build. The registered rule kept the per-tier assets.

## Measured numbers

The tier ladder (BAS-P20, 2026-10-28, four PGO builds a tier, each hop read
directly on the 5950X, clang 23.1.4):

| Hop | NPS | 95% interval |
|---|---:|---|
| `-avx2` over portable | +5.04% | [+4.45%, +5.63%] |
| `-pext` over `-avx2` | +2.82% | [+2.31%, +3.34%] |
| `-pext` over portable | +8.49% | [+7.83%, +9.15%] |

At about 2 Elo per 1% NPS (BAS-P01) a user on the portable build of a pext
CPU gives up about 17 Elo.

Per-tier NPS is measured **locally** (the `tools/nps_ab.ps1` pooled-PGO protocol
and the build manifests) and summarized in the release notes; it is **not**
published as a per-asset file (the 8.6.5 local-only-manifest decision). The
audit's "5–10% faster public binaries" figure was a hypothesis; any speed number
in the notes is a **measured** result, not a promised speedup.

Reference point (developer machine, single-thread `bench`, not a release runner):

| Tier | Binary size | bench NPS |
|---|---:|---:|
| `windows-x86_64-pext` (PGO) | ~2.27 MB | ~2.75 M nps |

Other tiers' numbers are filled in from CI manifests at release time; this table
is a sanity anchor, not a cross-CPU comparison (NPS is machine-specific).
