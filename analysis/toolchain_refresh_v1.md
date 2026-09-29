# A.4.1 — toolchain refresh and freeze

- State / class: `CLOSED` / `I1`
- Owner / date: Codex / 2026-09-29
- Decision: retain the deployed compiler and standard-library lines, pin CI
  runner generations, and enforce each published x86 ISA contract from the
  exact release binary.

## Inventory and decision

| Surface | Frozen line | Newer line considered | Decision |
|---|---|---|---|
| Windows production | Clang/LLVM/`llvm-profdata` 22.1.8, libc++ 22.1.8, CMake 4.4.3, Ninja 1.13.2 | None: these were the newest stable upstream releases checked on 2026-09-29 | Keep |
| Linux CI | Ubuntu 24.04, Clang/LLVM 19, GCC 14/libstdc++, CMake and Ninja from the runner | Clang/LLVM 22; experimental GCC 16 | Keep; the alternatives passed isolated correctness checks but have no native pooled-PGO result |
| macOS CI | macOS 15 ARM64, Xcode 16.4 / AppleClang 17 / libc++, CMake 4.4.3, Ninja 1.13.2 | Xcode 26.3 is installed on the image | Keep; no native performance and sanitizer comparison selects the newer Xcode |

The runner aliases are frozen to `ubuntu-24.04`, `windows-2025` and
`macos-15`; the ARM64 Ubuntu label is already explicit. Compiler majors remain
explicit in the setup steps. Upstream and runner inventories used for this
decision:

- LLVM releases: <https://releases.llvm.org/index.html>
- GCC releases: <https://gcc.gnu.org/releases.html>
- CMake releases: <https://cmake.org/cmake/help/latest/release/index.html>
- Ninja releases: <https://github.com/ninja-build/ninja/releases>
- GitHub-hosted runner policy: <https://github.com/actions/runner-images/blob/main/README.md>
- Ubuntu 24.04 inventory: <https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2404-Readme.md>
- macOS 15 ARM64 inventory: <https://github.com/actions/runner-images/blob/main/images/macos/macos-15-arm64-Readme.md>

## ISA-contract repair found by the refresh

The published PEXT tier claimed AVX2 and hardware POPCNT, but its portable
release configuration passed only `-mbmi2`; `-mbmi2` does not imply either
feature. A local native PGO build had hidden this because `-march=native`
supplied the missing flags.

The repair makes the tier explicit (`SSE4.1`, `POPCNT`, `AVX2`, `BMI1`,
`BMI2`/`PEXT`, and `LZCNT`), keeps the startup launcher at baseline ISA, and
checks the complete requirement before entering optimized code. The exact
release binaries are now disassembled by `tools/diag/verify_isa.py` in CI and
release workflows. Its known-bad control rejects a portable binary presented
as PEXT. Commits `927238e`, `73069c4`, and `784c59f` carry the engine,
enforcement, and portability repairs.

## Qualification

| Build / host | Deterministic result | ISA result | Performance evidence |
|---|---|---|---|
| Windows Clang 22 portable, AVX2, PEXT | CTest 13/13 each; `bench 13` = **14,978,465** each | All tier contracts pass | Pooled final-PGO repaired PEXT versus pre-repair baseline: **+7.45%**, 95% CI **[+7.22%, +7.75%]**, 16/16 faster |
| Windows Clang 22 sanitizer | CTest 13/13 | N/A | N/A |
| Ubuntu 26.04 isolation, Clang 19 and 22 | release and PGO CTest 13/13; exact fingerprint | PEXT contract passes | No selection: WSL one-shot NPS is not pooled native evidence |
| Ubuntu 26.04 isolation, GCC 14 and experimental 16 | release and PGO CTest 13/13; exact fingerprint | PEXT contract passes | No selection: WSL one-shot NPS is not pooled native evidence |

The Windows self-pair was -0.02%, 95% CI [-0.21%, +0.22%], with the first
arm faster in 7/16 samples, validating the <=0.30% pooled-noise control. Raw
ignored evidence is in `tools/results/a41-pext-repair/`; the two old and two
candidate binaries were distinct builds and all reproduced the fingerprint.

The final PGO PEXT binary contained 168 `popcnt`, 246 `pext`, 411 `blsr`, 8
`lzcnt`, 512 backward-compatible `tzcnt`, and 215 AVX2 marker instructions.
The baseline launcher contained none of the optional instructions.

## Limits and retry triggers

No Linux or macOS compiler upgrade is selected from cross-host or one-shot
throughput. Reopen this comparison when a native idle host can run the same
pooled final-PGO protocol, or when a deployed compiler/library line becomes
unsupported or fails a required language, sanitizer, correctness, or release
contract. Compare one axis at a time and require the deterministic and ISA
checks before reading performance.
