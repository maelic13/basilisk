# BAS-P23

**BAS-P23 — BAS-P22's read on an Intel host: native, the universal binary and GCC**

- **Registration.** Registered 2026-10-29, before any reading; PLAN A.17
  (candidate 5) and A.15's universal binary. Run by the maintainer's Codex
  agent on their Intel Windows 11 machine from commit `7e34cdd`, which
  carries the launcher (the next commit removes it, so master does not):
  `pwsh tools/run_p23_intel_reads.ps1`
  (`-Cpu <n>` on a performance core of a hybrid CPU; `-NoGcc` without
  MSYS2's GCC). It builds the pools with `build_test.ps1` and reads them
  with `nps_read.ps1`, as `tools/run_a158_nps_reads.ps1` did for BAS-P22. The archive and `host.txt`
  come back to `tools/results/p23-intel/`.
- **Background.** BAS-P22 on the 5950X (Zen 3): Clang's `-march=native`
  1.0% slower than the fixed pext instruction set, the universal binary
  −0.14% (PASS) against it, GCC 2.7% behind Clang. All three are one CPU's
  answers; whether native's loss is Zen 3's tuning model is open.
- **Builds**, four `build_test.ps1` Release builds a pool, the Intel host's
  own Clang 23 and GCC: `p23-native` (`-Native`, NATIVE=ON), `p23-pext`
  (the pext asset's recipe), `p23-universal` (`-Universal`), `p23-gcc`
  (`-Native -Gcc`).
- **Contrasts:** `native` (native over universal, gain), `nativecg` (native
  over pext, gain), `hot` (universal against pext, no-regression), `gcc`
  (GCC native over Clang native, gain).
- **Question.** Do BAS-P22's answers hold on Intel?

#### PRE-REGISTERED PREDICTION — freeze before exposure

- `nativecg`: 0.0%, 80% interval [−1.5%, +1.5%]: the Zen 3 loss came from
  something CPU-specific or from layout, either of which can go either way
  on another CPU.
- `native`: 0.0% [−1.5%, +1.5%], about `nativecg` less the universal
  binary's small cost.
- `hot`: −0.2% [−0.7%, +0.3%], PASS.
- `gcc`: −3% [−7%, +1%].
- **Decision rule.**
  - `hot` FAIL on Intel would reopen the universal binary's placement
    question (A.15.6) for that microarchitecture.
  - `nativecg` ACCEPT (native faster on Intel) changes no default:
    `release` keeps the tier's fixed flags, the same on every CPU, and the
    README says native can win on some processors. Native slower again
    makes A.17 candidate 5 a question of native in general, not Zen 3.
  - `gcc` sets what the README says about GCC.
- **Stop rule:** step 1, step 2 only when a contrast says EXTEND; a REPEAT
  repeats the read once into a fresh directory, a second stops.
- **Conditions:** as BAS-P22, on the Intel host; it must be idle (the
  maintainer's call), with no other engine, harness or build process.

#### RESULT — append after exposure

- **Result (Codex-run 2026-10-29 on an Intel Core Ultra 7 165H, Windows 11
  Enterprise, revision `7e34cdd`; read pinned to logical CPU 0, Windows
  EfficiencyClass 1; `tools/results/p23-intel/`, archive returned to the
  maintainer).** Every reading 19,470,325 (64 readings of 16 builds).
- **Conditions: not the registered ones.** The host is a corporate laptop
  whose security software never lets it idle: the launcher's idle check
  refused two launches at about 25% load. The maintainer authorised a
  waiver, applied as a local launcher change (`launcher-override.patch` in
  the archive, restored afterwards): no load ceiling and no quiet wait;
  provenance, fingerprints, pinning, statistics and the stop rule
  unchanged. Host load during the repeat read's cycles: 3.2% and 91.0%.
- **Both reads returned REPEAT at step 1, so the stop rule stopped the
  experiment.** Means of build medians, first read and repeat: Clang pext
  2.8878M / 3.0031M, universal 2.8736M / 3.0075M, Clang native 2.8653M /
  2.9831M, GCC native 2.8029M / 2.9423M.

  | Contrast | First read | Repeat | Prediction (80%) |
  |---|---:|---:|---|
  | `native`, Clang native over universal | −0.29% [−2.79, +2.22] | −0.81% [−2.52, +0.90] | 0.0% [−1.5, +1.5] |
  | `nativecg`, Clang native over Clang pext | −0.78% [−3.39, +1.84] | −0.67% [−2.78, +1.45] | 0.0% [−1.5, +1.5] |
  | `hot`, universal against Clang pext | −0.49% [−3.16, +2.17] | +0.15% [−1.89, +2.18] | −0.2% [−0.7, +0.3] |
  | `gcc`, GCC native over Clang native | −2.18% [−4.41, +0.06] | −1.37% [−4.44, +1.70] | −3% [−7, +1] |

- **Why it could not decide.** The noise was about ten times the 5950X's:
  one binary's three runs spread 2-10%, the builds of one pool 3-4% (the
  BAS-P22 lesson repeats a read past 0.5%), every arm ran about 9% faster
  in the repeat's second cycle than its first, and the repeat as a whole
  about 4% faster than the first read. The intervals are about ±2%, so
  only an effect larger than that could have shown.
- **What it does show.**
  - The universal binary and every single-tier build run on Intel (Meteor
    Lake, the pext tier) and bench exact.
  - Every contrast has the same sign in both reads and the sign BAS-P22
    read on Zen 3: native no faster than the fixed pext instruction set or
    the universal binary, the universal binary level with the pext build,
    GCC behind Clang. No contrast excludes zero, so this is agreement, not
    confirmation; it bounds each effect on this Intel CPU to roughly ±2%.
  - The one result that could have changed a default, native clearly
    faster on Intel, is ruled out above about 2%.
- **Decision.** INCONCLUSIVE; no default changes and the experiment is not
  re-run: no decision depends on an effect under 2%, and this host cannot
  resolve one. A.17 candidate 5 stays a Zen 3 measurement; whether native's
  loss is general stays open. A future Intel read needs a host that can
  idle, mains power, and a performance core other than CPU 0 (Windows
  routes most interrupts there).
- **Calibration.** Every point estimate falls inside its predicted
  interval, but the read is too wide to test the predictions.
