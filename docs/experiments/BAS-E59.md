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

- **Run 2026-11-23, by the agent** (the maintainer's authority for the
  night), 11 min 39 s:
  - `tools/results/c284-sprt/`; manifest
    `colosseum_sprt_c284-sprt_20261123_225305.manifest.txt`;
    launcher `tools/run_c284_gates.py` (`b139bc7`).
  - Candidate: the refit, `cbc3676`, `basilisk-c284-refit-pext-pgo.exe`
    `B2495083…`, bench 3,359,329.
  - Baseline: the head with Amendment 2's top-tier repair, `b912051`,
    `basilisk-c284-base-pext-pgo.exe` `10F6D74E…`, bench 4,392,040. The
    head's later `6be9af7` changed comments only. Both are clean final-PGO
    PEXT builds with Clang 23.1.5.
- **H1 accepted at game 1,110,** LLR +2.95.
  - W-D-L 493-391-226; Elo **+85.2 ± 14.9**; nElo +122.0 ± 20.4.
  - Pentanomial [10, 78, 185, 199, 83].
  - Zero faults; the PGN recount agrees.
  - 36 post-terminal games are kept as evidence outside the sample.
- **The bracket veto BLOCKS** (`tools/results/c284/veto/`):
  - against the repaired head, re-measured: 8 vetoes, new clean-win
    discards at two of three budgets, in KBPP-KB (3), KNN-KP (2), KP-KP
    (1) and KRP-KR (2);
  - against `c274/head`: 7.
  - Conversion of 480 at 21k/64k/190k: the refit 434/445/458 against
    the repaired head's 443/448/460.
- **The registration's §8 read:** B2 baked over the repaired head
  (`5af6d65` on a scratch branch, `57A93DCD…`, bench 3,582,344).
  - It draws 7 vetoes against the repaired head and repeats 3 of A2's 8.
  - A2 against B2: 3 vetoes. B2 against A2: 6.
  - B2's conversion is 436/443/457.
  - By §8's frozen reading this is "between": neither systematic (5 or
    more repeats) nor churn.
- **Calibration:**
  - H1 as predicted (0.75). The first-crossing estimate is far above the
    point (+20) and outside the 80% band (0 to +45).
  - The bracket veto passes (0.6): miss.
  - No crash, illegal move or time loss (0.95): hit.
- **Disposition: H1 run, not accepted.** The veto binds (the
  registration's §8). Acceptance is the maintainer's ruling: whether a
  +85 Elo refit can be kept against the veto's 8 endgame discards, or
  what repair comes first.
