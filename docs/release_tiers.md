# Release binary tiers and ISA contract

> **Audience: developer / maintainer** (PLAN §1, "Document audience"). This is
> the build/ISA contract behind the published assets. The *user-facing* version
> of "which download do I pick" is the download table in `README.md`.

Basilisk ships one binary per (OS, architecture, CPU-feature tier). Every
release asset is built **PGO** (profile-guided: instrument → train on the bench
suite → merge → optimised link) from the exact tagged revision with production
flags (`TUNE` off, `PORTABLE_BUILD=ON`), and CI **smoke-tests the exact uploaded
file** before publishing it: the binary must answer `uci`/`uciok`, carry the
release version string, and return a real `bench` node count — proving the move
generator, evaluator and search survived PGO on that runner
(`.github/workflows/release.yml`). Only the binaries are published as release
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
| `-pext` | x86-64 with AVX2 **and** fast BMI2 `PEXT` | Haswell+ / **Zen 3+** (2020+) | Uses `PEXT` for magic bitboards. On Zen 1/2 `PEXT` is microcoded and slow — use `-avx2` there instead. |
| aarch64 *(none)* | ARMv8-A / NEON | Apple Silicon, ARM64 servers/Windows | NEON is baseline on ARMv8; no separate SIMD tier. |

Pick the **most specific tier your CPU satisfies**: `-pext` on Zen 3+/Haswell+,
`-avx2` on Zen 1–2 and older AVX2 x86, portable on anything else. The `-avx2`
and `-pext` binaries check their required CPU features at startup
(`src/main.cpp`, before the UCI loop) and exit with a message naming the tier to
use instead, rather than faulting on an illegal instruction — when unsure, the
portable binary always works.

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
- **`-avx2`** (`-mavx2 -msse4.1 -mpopcnt`) and **`-pext`** (`-mbmi2`, which on
  this clang pulls in `POPCNT`): **hardware `POPCNT`** — the built `-pext`
  binary contains 97 `popcnt` instructions. These are the fast path.

A future `x86-64-v2` tier would give `POPCNT` (and SSE4.2) to portable-class
CPUs that have it; it is not currently built (see the no-`v2`-tier note above).

## Measured numbers

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
