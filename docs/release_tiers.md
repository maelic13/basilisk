# Release binary tiers and ISA contract

Basilisk ships one binary per (OS, architecture, CPU-feature tier). Every
release asset is built **PGO** (profile-guided: instrument → train on the bench
suite → merge → optimised link) from the exact tagged revision with production
flags (`TUNE` off, `PORTABLE_BUILD=ON`), and each is published with a
`*.manifest.txt` (revision, compiler, bench fingerprint, measured NPS, size,
SHA-256) and a `*.sha256`. Verify a download against its `.sha256` before use.

## Tiers and the CPU each requires

| Asset suffix | Target contract | Requires | Notes |
|---|---|---|---|
| *(none)* | Portable x86-64 baseline (SSE2) | Any 64-bit x86 CPU | Safe default; runs everywhere. |
| `-avx2` | x86-64 with AVX2 | Haswell (2013+) / Zen (2017+) | AVX2 codegen (`-mavx2 -msse4.1 -mpopcnt`). |
| `-pext` | x86-64 with AVX2 **and** fast BMI2 `PEXT` | Haswell+ / **Zen 3+** (2020+) | Uses `PEXT` for magic bitboards. On Zen 1/2 `PEXT` is microcoded and slow — use `-avx2` there instead. |
| aarch64 *(none)* | ARMv8-A / NEON | Apple Silicon, ARM64 servers/Windows | NEON is baseline on ARMv8; no separate SIMD tier. |

Pick the **most specific tier your CPU satisfies**: `-pext` on Zen 3+/Haswell+,
`-avx2` on Zen 1–2 and older AVX2 x86, portable on anything else. A binary run
on a CPU that lacks its required features will fault with an illegal
instruction — when unsure, the portable binary always works.

There is intentionally **no `x86-64-v2`/POPCNT tier** yet: it would sit between
portable and `-avx2` and has not been shown to be worth a fourth x86 asset.
Adding it is a build-system change (a new preset + matrix row), tracked as a
possible future tier, not promised.

## Measured numbers

Per-release, exact numbers live in each asset's `manifest.txt` (recorded by CI
on the file that is actually uploaded — see `.github/workflows/release.yml`).
The audit's "5–10% faster public binaries" figure is a hypothesis; we publish
**measured** per-tier NPS rather than a promised speedup.

Reference point (developer machine, single-thread `bench`, not a release runner):

| Tier | Binary size | bench NPS |
|---|---:|---:|
| `windows-x86_64-pext` (PGO) | ~2.27 MB | ~2.75 M nps |

Other tiers' numbers are filled in from CI manifests at release time; this table
is a sanity anchor, not a cross-CPU comparison (NPS is machine-specific).
