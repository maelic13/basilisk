# BAS-P20

**BAS-P20 — the universal x86-64 binary against the pext asset, and the ISA tier ladder**

- **Registration.** Registered 2026-10-28, before any reading; PLAN A.15.4;
  agent-run at the maintainer's request ("do all the steps …
  stop only if you need my input and cannot decide or measure yourself").
  Two agent attempts that day, and the maintainer's at 23:01, took no
  reading: the read's idle check reported the host's load bursting to
  20-30%.
- **Instrument repair (2026-10-29, before any ladder reading).** That load
  was the instrument's: `Get-HarnessHostBusyPercent` read WMI's raw
  processor counters, whose idle count is not sampled with its timestamp.
  Over 2 s windows it reported 1.6-2.7 s of idle time, so −35% to +23%
  on a host that `GetSystemTimes` read at 0-1%; negative readings were
  clamped to 0, positive ones refused the host. `7996e4d` reads
  `GetSystemTimes` instead (idle 0.0-0.7%, eight bench threads of 32 at
  25.0-25.4%). The NPS readings and the decision rule are unchanged; only
  the quiet wait and the per-cycle disturbance flag used the broken
  measure.
  - **First `hot` attempt (06:50 UTC, before the repair),
    `tools/results/a15-nps/hot-attempt1/`:** REPEAT_STEP_1, both cycles
    flagged at 15.6% and 13.8% by the broken measure; −0.17%
    [−2.61%, +2.26%]. Its first readings were slow on both arms (3.19-3.31M
    against 3.43M later). Disclosed exposure: cycle 2 alone was read before
    the repeat, about −0.5%. By the stop rule the read repeats once, into
    a fresh directory, under the repaired instrument.
  - The `tier` read was stopped for the repair in its quiet wait, before
    any reading (`tier-stopped/` holds only its `read.json`); it restarts
    fresh.
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

- **Result (agent-run 2026-10-29, 07:00-08:20 UTC, after the instrument
  repair; every cycle undisturbed, host 0.2-1.2%;
  `tools/results/a15-nps/<read>/`).**

  | Read | Delta | 95% t-interval | Cycles | Verdict |
  |---|---:|---|---:|---|
  | `hot`, universal against the pext asset | **−0.58%** | [−0.82%, −0.33%] | 6 | **FAIL** |
  | `tier`, pext tier build against the pext asset | +0.04% | [−0.12%, +0.19%] | 2 | NO_CHANGE |
  | avx2 over portable | **+5.04%** | [+4.45%, +5.63%] | 2 | ACCEPT |
  | pext over avx2 | **+2.82%** | [+2.31%, +3.34%] | 2 | ACCEPT |
  | pext over portable | **+8.49%** | [+7.83%, +9.15%] | 2 | ACCEPT |

  `hot` read −0.62% [−0.87, −0.37] at step 1 (EXTEND) and failed at step 2.
- **Decision (the registered rule).** `hot` failed, so the release stays on
  per-tier assets; the universal build stays in the tree, unshipped. `tier`
  reads zero, so the loss is in the relink, not in the tier build.
- **Where the loss is (diagnosis after the read, not a registered test).**
  The pext tier's hot code is byte-identical in the universal binary and in
  its own tier build: `negamax` is the same 4,713 instructions modulo
  addresses, and 8 adjacent inversions in 560 functions' order. The
  call-graph profile survives the rename intact. What differs is placement:
  in all four builds the relink moves the hot functions by 16 bytes modulo
  64 (`CoreMovePicker::next`, `Board::is_legal`, `Board::unmake_move` from
  offset 0 to 48 of a cache line, `quiescence` from 16 to 0). The −0.58% is
  therefore a code-alignment effect of one layout, not a cost of dispatch.
- **Calibration.** `hot` missed its 80% interval [−0.4%, +0.4%]; `tier`
  hit. The ladder hit all three intervals: +5.04% against +5% [+3, +9],
  +2.82% against +2% [0, +4], +8.49% against +7% [+4, +12]. The wrong part
  of the model was that four builds would vary the layout and average it
  out: builds of one source differ only in their PGO profile, their tier
  objects are the same size within a few bytes, so all four shared one
  placement. A pool of builds does not randomise link layout, so a
  relinking change needs a layout control (a padding sweep), not only
  more builds.
