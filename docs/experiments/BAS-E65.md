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

#### RESULT — recorded 2026-11-26

- **SPRT `[0,3]` (`tools/results/c35-sprt/`):** `status: h1` at **`1,034` games**
  (`517` official pairs; `18` post-terminal pairs / `36` games kept outside the
  official sample), **`LLR = +2.953`** (`+2.953367` vs upper bound `+2.944439`).
  - `W-D-L = 434-415-185`, `Ptnml = [3, 60, 193, 207, 54]`.
  - **`+85.3 Elo [+71.9, +99.0]`** (95% pentanomial CI), `nElo = +138.9 +/- 21.2`.
  - Faults: `0` engine faults, `0` time losses, `0` infrastructure faults.
  - Speed & depth (all moves): `1.93M` vs `1.95M` nps (`137,449` vs `141,062`
    nodes/move, `71.2` vs `72.2` ms/move, mean non-mate depth `16.75` vs `16.48`,
    median `15` vs `15`).
  - PGN SHA-256: `B8A15DE32D46280C0D94C4A9FA1E07CA3353E212CAC997A514C9BD171D589FF7`.
- **C.2.7 endgame bracket veto (`tools/results/c35/veto/`):** at
  `21,000 / 64,000 / 190,000` nodes of `480` Cohort positions, candidate converts
  **`441 / 448 / 457`** vs baseline **`434 / 445 / 458`** (`+7 / +3 / -1`;
  `tools/results/c274/head`: `443 / 448 / 463`). Zero `new_rule50_failure`,
  `engine_anomaly`, or `malformed_record`. Two-of-three clean-win discards:
  `count = 7` vs `base` (`KBP-KB/EG0580`, `KBPP-KB/EG0745`, `KNN-KP/EG0454`,
  `KNN-KP/EG0462`, `KNN-KP/EG0465`, `KNN-KP/EG0466`, `KRP-KB/EG0545`) and
  `count = 5` vs `c274/head` (`KBPP-KB/EG0742`, `KBPP-KB/EG0745`, `KNN-KP/EG0454`,
  `KNN-KP/EG0468`, `KNN-KP/EG0471`).
- **The veto binds, so acceptance is the maintainer's ruling**, as at
  BAS-E59 (PLAN C.2.8.4): the SPRT is not weighed against the veto, and
  the review cannot accept over it.
  - **Ruling, 2026-11-26 (maintainer):** the candidate is accepted over the
    veto, and the vetoed positions go to C.4 with the endings already
    routed there.
  - The basis on record: the two-of-three rule fires 3 to 6 times between
    two fits of equal quality (C.2.8.4), so 7 against the base and 5
    against `c274/head` sit at that floor; conversion of the 480 rose at
    21,000 and 64,000 nodes (`+7`, `+3`) and fell by one at 190,000; and
    the vetoes fall in the families C.2.8.4 already found (KBPP-KB, KNN-KP,
    KRP-K*), five of the seven in KNN-KP.
- **Outcome:** SPRT H1 prediction (`+85`, `P(H1) = 0.99`) hit almost identically
  (`+85.3`, a first-crossing estimate). The veto prediction (passes, `0.40`)
  missed: it fired. Candidate accepted for C.3.5's flip (`e60aa45`) by the
  ruling above.

#### NPS read beside the gate — registered 2026-11-26, before the read

- Question (C-99): the shipped evaluation's speed against the head's, at
  one revision. BAS-M25 read the same code at its seed values at
  `−4.47% [−4.79%, −4.16%]`; the fit changes only values, which move the
  lazy-exit share (`14.67%` to `14.01%`) and the tree.
- Pools: `tools/nps_pool.ps1`, four final-PGO PEXT builds each:
  `c35-nps-on`, the shipped evaluation at `ab0c141` after the retired one's
  removal (`bench 13` 3,880,333), and `c35-nps-off`, the head at `99e6353`,
  the last commit that compiles it (`EVAL_REBUILD=OFF`, 3,359,329).
  Amended before any pool was built, when the maintainer moved the removal
  ahead of the read; the removal is behaviour-neutral (`bench 13` exact).
- Read: `pwsh -NoProfile -File tools/nps_read.ps1 -Base
  tools/test_engines/c35-nps-off.pool.json -Cand
  tools/test_engines/c35-nps-on.pool.json -Out tools/results/c35-nps
  -Label "C.3.5 shipped evaluation vs head"`, gain form, step 1 of two
  cycles, extended only when the tool says EXTEND, decided by
  `tools/diag/nps_read.py`; idle host.
- Prediction: `−4.5%`, 80% band `−6.0%` to `−3.0%`. The gate is already
  decided; this read decides nothing alone and is recorded beside it.

#### NPS read — RESULT, 2026-11-26

- `tools/results/c35-nps/` (`verdict-2.json`), two cycles of `bench 13`
  pinned on CPU 30, host 0.8% and 0.6% busy, no disturbed cycle, the tool's
  next step `DONE`.
- **The shipped evaluation against the head: −3.88% [−4.65%, −3.10%]**
  (Welch df 3.8): 2.351M against 2.446M nodes a second, the means of the
  four builds' medians (2.356M, 2.350M, 2.345M, 2.354M against 2.440M,
  2.432M, 2.460M, 2.452M). The tool's gain-form verdict reads `NO_CHANGE`:
  no gain; the interval shows the slowdown.
- Inside the prediction's band (`−4.5%`, `−6.0%` to `−3.0%`), and 0.6
  points less than BAS-M25's seed read (`−4.47%`). The fingerprints differ
  (3,880,333 against 3,359,329), so this is speed per node of two
  evaluations, not a pure speed change; the gate's games already price it.
