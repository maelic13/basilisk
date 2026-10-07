# BAS-X08

Two different experiments were registered under this ID before the
ledger was split; both are kept, in ledger order.

<!-- part 1 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-X08 - `ca89031` include-what-you-use verified on Windows** (2026-09-04).
The commit was made to fix a Homebrew LLVM 23 libc++ build failure on an M4
Mac, where dropped transitive includes left `std::abs` and `std::llabs`
undeclared because `<cstdlib>` sat inside a `BASILISK_TUNE` guard. Its claim is
explicitly cross-platform -- which headers arrive transitively differs across
libc++, libstdc++ and the MSVC STL -- so it was re-verified on the Windows
MSYS2 clang toolchain. It is an ancestor of HEAD, so recent work already built
on it; this checks it deliberately rather than by inference.

| check | result |
|---|---|
| clean release-pext build from scratch | ok, no errors |
| full CTest | **12/12** |
| `bench 13` | 12,709,666, unchanged |
| every `src/*.h` compiles standalone | **23/23**, zero failures |
| TUNE configuration | ok |
| TEXEL configuration (fresh eval TU, `TEXEL_TRACE`) | ok, zero warnings on rebuild |
| Debug + `SANITIZE=ON` build | ok |
| sanitized test_eval / test_board / test_search / test_tt / test_move | 77, 307, 460, 64, 68 all pass |
| PGO target via the `release-pext` preset | completes; PGO binary bench 12,709,666 |

The standalone-header check is the one that actually tests the commit's thesis,
and it passes on a toolchain the author could not try. Nothing in the change is
Apple-specific and nothing regressed here.

*One false alarm worth recording so it is not repeated.* Invoking the PGO target
against an ad-hoc build directory fails with `No such configure preset:
v-pgo` -- `cmake/pgo-build.cmake` reconfigures by preset NAME, so the target
only works from a directory that corresponds to a real entry in
CMakePresets.json, which is what `tools/build_test.ps1` does. That is a
pre-existing property of the PGO plumbing, not a defect introduced by
`ca89031`, but it looks exactly like a build break if you meet it cold.

*Not verified:* the full CTest suite under sanitizers, which exceeded a
ten-minute budget. Five suites were run individually instead and all passed;
the sanitized build is functional, but no claim is made about the complete
sanitized run.

<!-- part 2 of 2: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X08 |
| Rarog evidence | Rarog's parity audit emphasizes shared `MoveEvidence`, prospective depth and correction attribution. |
| Possible Basilisk implication | These abstractions may reduce contradictory consumers only if Basilisk telemetry confirms the same failure modes. |
| Existing PLAN coverage | 9.0, 9.6 |
