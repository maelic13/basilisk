# BAS-P23

**BAS-P23 — BAS-P22's read on an Intel host: native, the universal binary and GCC**

- **Registration.** Registered 2026-10-29, before any reading; PLAN A.17
  (candidate 5) and A.15's universal binary. Run by the maintainer's Codex
  agent on their Intel Windows 11 machine from this revision:
  `pwsh tools/run_p23_intel_reads.ps1` (`-Cpu <n>` on a performance core of
  a hybrid CPU; `-NoGcc` without MSYS2's GCC). The archive and `host.txt`
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
