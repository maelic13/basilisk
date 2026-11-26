# BAS-E65

**BAS-E65 — C.3.5: baked `k-phase` rebuilt evaluation against the head, SPRT `[0,3]` and endgame bracket veto**

- Date / owner / calibration category: registered 2026-11-26, before any
  game; maintainer-run, agent-read; evaluation/HCE.
- Arms: clean final-PGO PEXT Release builds of revision
  `0374cfd8f36005e84fe0c1678e5352d14973c0c8` (`0374cfd`; engine/tests
  baked at `5128e6e`, Clang 23.1.5, dirty diff `clean`), launcher
  `tools/run_c35_gate.py` (`4a9c07d`):
  - Candidate (`Basilisk C.3 rebuild`):
    `tools/test_engines/basilisk-c35-cand-pext-pgo.exe`,
    `EVAL_REBUILD=ON`, SHA-256
    `DF1FAA70DFBBC30417CF238514F647822D749C979D3F98342231391BA787A473`,
    `bench 13` = `3,880,333`, played hash fa8739a3fc205e63.
  - Baseline (`Basilisk head`):
    `tools/test_engines/basilisk-c35-base-pext-pgo.exe`,
    `EVAL_REBUILD=OFF`, SHA-256
    `0A4D9CCB64E1435400DCE4CDD47AF77C6AD20513A0D56E7B52B93C7A7F1C4F20`,
    `bench 13` = `3,359,329`.
- Research question: does the rebuilt classical Stockfish evaluation
  fitted on Basilisk's Corpus A games (`k-phase` Run 2) pass the `[0,3]`
  SPRT gate and C.2.7 endgame bracket veto against the current head,
  justifying flipping `EVAL_REBUILD` to default `ON` in C.3.5?
- Hypothesis / competing hypotheses (`analysis/c31_rebuild_2026-11-25.md`
  §12): the seed arm beat the head by `+99.8 [+91.5, +108.3]` (BAS-M25)
  and `k-phase` trailed the seed by `−9.6 [−19.6, +0.4]` (BAS-E64), so
  the baked candidate sits around `+85` to `+90 Elo` above the head and
  clears `[0,3]` rapidly. Competing hazard: the C.2.7 zero-tolerance
  endgame truth bracket veto (`tools/results/c35/veto/`) flags clean-win
  differences in 4–6 men endgames as in BAS-E59 (requiring maintainer
  ruling).
- Pre-game deterministic checks (passed 2026-11-26):
  - Full CTest: `59/59` passed on `EVAL_REBUILD=OFF` (`bench 13` =
    `3,359,329`); `59/59` passed on `EVAL_REBUILD=ON` after updating
    `tests/test_eval_rebuild.cpp`, `tests/test_endgames.cpp` and
    `tests/data/endgame_verdicts_golden_rebuild.txt` (`5128e6e`,
    `bench 13` = `3,880,333`).
  - `test_endgames --conversion` (`60,000` nodes/move): `24/24` on all
    four families (`KQ-K` 11.1, `KR-K` 21.3, `KBB-K` 23.9, `KBN-K` 48.2).
  - C-98 lazy-margin audit (`eval_hash --lazy-audit`, `209,444` Corpus A
    validation rows): exact hash equality fa8739a3fc205e63, `37,725` lazy
    fires, `4` sign flips (`0.011%`), `764` crossings (`2.03%`), mean
    `|delta| = 110.8` cp; `LAZY_THRESHOLD_1 = 1400` and
    `LAZY_THRESHOLD_2 = 1300` held unchanged.
- Full conditions:
  - **SPRT (`c35-sprt/`)**: Colosseum CLI `sprt` (`sprt-default.toml`,
    `[0, 3]` nElo), `3+0.03`, margin `20 ms`, `1T`, `Hash 64`, UHO
    `UHO_Lichess_4852_v1.epd` random order, paired colour-reversed, cap
    `4,000` pairs, seed `350001`, no adjudication, no tablebases, fault
    cap `0`. Companion equal-node read if SPRT rejects (C-95).
  - **Endgame bracket veto (`c35/veto/`)**: `endgame_budget_bracket.py` at
    `21,000`, `64,000` and `190,000` nodes against both `BASE` and
    `tools/results/c274/head`.
- Commands: `python tools/run_c35_gate.py --run sprt` and `--run veto`
  (or `--run all`).

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- H1 accepted on SPRT `[0,3]`: probability `0.99`.
- First-crossing Elo estimate: `+85 Elo`, 80% band `+55` to `+115`.
- C.2.7 bracket veto passes at zero tolerance: probability `0.40` (given
  BAS-E59's observation that two good fits trigger 3–6 two-of-three
  differences across 480 cohort positions).
- Zero crashes, illegal moves or time losses: `0.99`.
- Falsification criteria: H0 on SPRT falsifies additivity of `BAS-M25`
  and `BAS-E64` or signals a broken candidate binary (`BAS-E65` triggers
  equal-node companion read by C-95).

#### RESULT — append after exposure
