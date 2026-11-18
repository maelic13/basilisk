# BAS-M19

**BAS-M19 — B.11.4's direction checks: the 1.11.0 candidate against 1.10.2 at `10+0.1` 1T and at 4T**

- **Registration.** Created 2026-11-18, before any game; maintainer-run
  games. PLAN B.11.4: `10+0.1` and 4T direction checks whose 95% intervals
  exclude a loss, beside BAS-S67's SPRT.
  - A head-to-head read cannot see a thread-scaling loss that both versions
    share, so BAS-M20's 4T pool read sits beside this one.
- **Arms.** BAS-S67's candidate (`basilisk-b111-head-pext-pgo.exe`,
  `31BC229A…`, bench 4,777,412) and baseline
  (`basilisk-b114-v1102-pgo1-pext-pgo.exe`, `21BE1192…`, bench 14,978,465).
  Both are Clang 23.1.5 final-PGO PEXT release builds with clean manifests.
- **Research question.** Does the gain hold at a longer clock and at four
  threads?
- **(a) `ltc`.**
  - A 1,000-game fixed match in 500 colour-reversed pairs, run file
    `match-fixed-ltc.toml`.
  - `10+0.1`, margin 20 ms, 1T, Hash 64, concurrency 15, seed 114002.
  - Launcher: `python tools/run_b114_gates.py --run ltc`.
  - About 30 minutes.
- **(b) `4t`.**
  - A 1,000-game fixed match at `3+0.03`, 4 threads a side, Hash 512,
    seed 114003.
  - Sibling-free: three games at a time on explicit cores, one logical CPU
    per physical core, 4 cores a game, one core free (PROCESS *Harness*,
    BAS-X41).
  - Launcher: `python tools/run_b114_gates.py --run 4t`.
  - About 45 minutes.
- Both runs: paired reversed UHO in random order, no adjudication,
  tablebases off, a fault cap of zero. Both dry runs are accepted.
- **Pass rule.** Each read's 95% interval excludes a loss: its lower bound is
  above 0 Elo.
- **Comparison.** BAS-S27 read the head at that time against 1.10.2, after
  Cluster 1 alone and before Clusters 2 to 4 and the repairs: `ltc`
  +241.4 and `4t` +237.6. Those were unpinned at 4T, before BAS-X41.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- (a) `ltc`: about **+380**, 80% band +300 to +460.
  - Basis: the STC gain of about +400, and BAS-S27's `ltc` reading close to
    its STC.
- (b) `4t`: about **+420**, 80% band +330 to +500.
  - Basis: BAS-S27's `4t` read about 1.25 times its STC.
  - Both versions now play sibling-free, which removes the placement
    confound.
- Both pass, probability 0.98.
- No candidate fault, probability 0.95.
- Falsifier: a read whose 95% interval reaches 0 is a release blocker, and
  goes to the maintainer before any reading of B.11.4.

#### RESULT — append after exposure
