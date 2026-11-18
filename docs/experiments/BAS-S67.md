# BAS-S67

**BAS-S67 — B.11.4's release licence: the 1.11.0 candidate against 1.10.2, SPRT `[0,3]`**

- **Registration.** Created 2026-11-18, before any game; maintainer-run games.
  PLAN B.11.4 licenses 1.11.0 by an accepted SPRT over 1.10.2 at `3+0.03` 1T,
  with BAS-M19's direction checks and BAS-M20's 4T pool read beside it.
- **Candidate.** The frozen search head's release build, the binary B.11.1
  played: `tools/test_engines/basilisk-b111-head-pext-pgo.exe`.
  - Revision `4c0bc0e`; its engine source is master's `4d2742f`.
  - SHA-256 `31BC229A6ED4C05715CEF8F44EF97866C91FE4ACE7929D8B86D8316CF12B994D`.
  - `bench 13` 4,777,412.
  - The branch since has removed only the ablation flavour, which a release
    build compiles out. The release workflow re-checks the fingerprint on
    every asset.
- **Baseline.** 1.10.2, rebuilt from its tag `fac5064` with the candidate's
  Clang 23.1.5 by `tools/nps_pool.ps1 -Revision fac5064 -Builds 1`, because
  the harness refuses a compiler mismatch:
  - `tools/test_engines/basilisk-b114-v1102-pgo1-pext-pgo.exe`;
  - SHA-256 `21BE11923F0012F865662606647D067C2E3706481CA2B1B56DA812F73CAF7A94`;
  - `bench 13` 14,978,465 and `id name Basilisk 1.10.2`, as shipped.
- **Research question.** Is 1.11.0 stronger than 1.10.2?
- **Conditions.** SPRT `[0,3]` nElo (`sprt-default.toml`).
  - `3+0.03`, margin 20 ms, 1T, Hash 64, concurrency 15.
  - Paired reversed UHO in random order, seed 114001.
  - No adjudication, tablebases off, a fault cap of zero.
  - Launcher: `python tools/run_b114_gates.py --run sprt`; the dry run is
    accepted.
- **Cap: 2,000 pairs.**
  - At the expected value of about +400 Elo, the SPRT stops within tens of
    pairs. The cap only bounds a surprise.
  - A cap without a decision goes to the maintainer.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- H1, within 100 pairs, probability 0.97.
- The Elo estimate is about **+400**, 80% band +330 to +470.
  - Basis: B.11.1's attribution, the gates summing to +441.
  - Also: the head at 3416.3 against 1.10.0's 2994 on the same four targets
    (+422).
  - A first-crossing estimate this early is wide, so the band is wide.
- No candidate crash, illegal move or time loss: probability 0.95, after
  B.11.1's 8,400 clean games.
- Falsifier: H0, or an estimate below +250. Either means the candidate
  binary or the baseline is not what is registered. Check both builds
  before any reading.

#### RESULT — append after exposure
