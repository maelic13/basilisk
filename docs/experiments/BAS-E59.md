# BAS-E59

**BAS-E59 — C.2.8.4: the whole-surface refit from Stockfish 16's tables on corpus A against the head, SPRT `[0,3]`**

- Registration: created 2026-11-23 (C.2.8.1), before any fit or game;
  maintainer-run games.
- Candidate: BAS-O13's A fit, baked into `src/eval_params.h`, as a clean
  final-PGO PEXT release build. Its revision, hash and `bench 13` are
  recorded when it is built.
- Baseline: the head that the fit's root header belongs to. Today that is
  `basilisk-c273-head-pext-pgo.exe`, `EDDBA36E…`, bench 4,777,412, rebuilt
  if the head moves. Both arms use the same compiler.
- Question: does the fitted surface (C.2.5's seed, corpus A's labels,
  `fit.py`'s four stages) make Basilisk stronger? One causal axis: the
  surface. `LAZY_MARGIN` and every search constant are unchanged.
- Conditions, BAS-S67's:
  - SPRT `[0,3]` nElo (`sprt-default.toml`), 3+0.03 with a 20 ms margin;
  - 1T, Hash 64, concurrency 15;
  - paired reversed UHO in random order, seed 280401;
  - no adjudication, tablebases off, a fault cap of zero.
- Cap: 4,000 pairs.
  - At the expected value, about +20 Elo, the SPRT stops within a few
    hundred pairs. The cap bounds a result near zero.
  - A cap without a decision goes to the maintainer.
- With the gate:
  - **C.2.7's bracket veto:** the candidate's truth at 21,000, 64,000 and
    190,000 nodes against `tools/results/c274/head`. It vetoes at two of
    three budgets, and on an anomaly at any one.
  - **Read beside, never deciding:** the deterministic floors, `bench 13`,
    and an equal-node companion read if the SPRT rejects (C-95).

#### PRE-REGISTERED PREDICTION — freeze before exposure

- H1: probability 0.75.
- The first-crossing estimate is about **+20 Elo**, 80% band 0 to +45
  (C.0's prior is +15 to +40).
- The bracket veto passes: 0.6.
- No crash, illegal move or time loss: 0.95.
- Falsifier: H0, which sends the retries of the registration's §1 in
  order: a per-phase-K fit, then a lazy-margin refit.

#### RESULT — append after exposure
