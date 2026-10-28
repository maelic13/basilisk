# BAS-P20

**BAS-P20 — the universal x86-64 binary against the pext asset, and the ISA tier ladder**

- **Registration.** Registered 2026-10-28, before any reading; PLAN A.15.4;
  agent-run at the maintainer's request ("do all the steps …
  stop only if you need my input and cannot decide or measure yourself").
  The design is `analysis/universal_binary_2026-10.md` (§4.7's hot-path
  check, §9's ladder prediction, amendment 1 for the form that was built).
- **Instrument.** `tools/nps_read.ps1`, decided by `tools/diag/nps_read.py`
  (PROCESS *Build, bench and NPS*), validated by BAS-P18's null
  (−0.19% [−0.66, +0.27]). Interleaved cycles of a pinned `bench 13 3 1`,
  best of three per reading, per-build medians, Welch t-interval; step 2
  only on EXTEND.
- **Builds**, all clang 23.1.4 (MSYS2), `bench 13` 19,470,325, one revision:
  - **`a15-pext`:** four `build_test.ps1 -Flavor Release` builds, the pext
    asset as every measurement builds it today.
  - **`a15-u1` … `a15-u4`:** four `build_test.ps1 -Flavor Release -Universal`
    builds. Each stages the universal binary and the three tier builds it
    was linked from, giving four pools of four: `a15-universal`,
    `a15-tier-pext`, `a15-tier-avx2`, `a15-tier-portable`. A tier build is
    the ordinary PGO build of its preset with LTO's objects kept and a
    bare `main`, so the tier pools are the old per-tier assets.
- **Reads**, in this order, each into `tools/results/a15-nps/<read>/`:
  1. **`hot` (the gate):** base `a15-pext`, cand `a15-universal`,
     `-NoRegression`. On the 5950X (Zen 3) the dispatcher selects the pext
     tier, so this asks whether the universal binary costs anything on the
     hot path: its tier objects are relinked with renamed symbols, beside two
     other tiers, behind a dispatcher.
  2. **`tier` (diagnostic):** base `a15-pext`, cand `a15-tier-pext`. It
     separates the tier build (kept LTO objects, bare `main`, the banner) from
     the relink, if `hot` moves.
  3. **The ladder** (A.15.2, D-24), each hop read directly, never chained:
     `avx2-portable` (base `a15-tier-portable`, cand `a15-tier-avx2`),
     `pext-avx2` (base `a15-tier-avx2`, cand `a15-tier-pext`), and
     `pext-portable` (base `a15-tier-portable`, cand `a15-tier-pext`).
- **Question.** Is the universal binary as fast as the pext asset on a CPU
  that selects pext, and what does each tier buy a user who runs it?
- **Hypothesis.** The relink changes layout but not code: each tier is the
  same ThinLTO output, so `hot` and `tier` read zero within the null. The
  ladder is the per-tier asset ladder the packet predicted.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **`hot`:** 0.0%, 80% interval [−0.4%, +0.4%]; PASS at step 1 with
  probability about 0.8. **`tier`:** 0.0%, [−0.4%, +0.4%].
- **The ladder** (the packet's §9 prediction, frozen 2026-10-28):
  `avx2` over `portable` +5% [+3%, +9%]; `pext` over `avx2` +2% [0%, +4%];
  `pext` over `portable` +7% [+4%, +12%].
- **Confidence:** moderate-high for `hot` (Stockfish ships the same form at
  no measured cost; the probe benches were equal within single-run noise);
  moderate for the ladder (Rarog's RAR-P20/P22 are the priors; Basilisk's
  avx2 tier has no intrinsics).
- **Decision rule.** The universal binary is kept as the x86-64 release
  asset if `hot` passes the no-regression form. If `hot` fails, the release
  stays on per-tier assets, and `tier` says whether the tier build or the
  relink is the cause. The ladder decides nothing: it is recorded for the
  README's guidance and for D-24.
- **Falsifiers:** `hot` failing (−0.5% or worse at step 2, upper bound under
  0), or either of `hot` and `tier` outside [−0.9%, +0.9%]; a ladder hop
  outside its 80% interval is a calibration miss, recorded below.
- **Stop rule:** step 1, then step 2 only on EXTEND. A REPEAT verdict
  repeats that read once into a fresh directory; a second REPEAT means the
  host is not quiet enough, and the reads stop for the maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, one
  warm-up reading per build, 120 s of host load under 10% before each read,
  no Rarog or other engine, harness or build process running.

#### RESULT — append after exposure
