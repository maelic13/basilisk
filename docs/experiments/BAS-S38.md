# BAS-S38

**BAS-S38 — A.13.2: the pawnless scaler in Stockfish's form**

- **Registration.** Registered 2026-10-27, before any game; maintainer-run;
  correctness repair D1 of the code audit's second pass (PLAN A.13.2), with
  the scaler's near-draw factor (the Known-defects row C.3–C.7 held, found
  with A.11.7), both admitted by the maintainer on 2026-10-27 since the same
  predicate and test decide them. An evaluation change admitted into the
  evaluation freeze.
- **Defect.** The pawnless scaler asked whether the winning side was at
  most a minor piece ahead in the endgame table's units (Q 950, B 330,
  N 320), so a queen against two minors (950 − 650 ≤ 330) was scaled by
  4/64: served at depth 1, KQ-KBN read +21 and KQ-KNN +27 for the queen,
  against +697 for KQ-KB, though Syzygy wins 82.5%, 71.0% and 73.5% of those
  classes. Its near-draw factor was 4 against any defender material, where
  the donor gives 14 against more than a bishop's worth, so Basilisk's 14
  was unreachable.
- **Candidate.** `basilisk-a1132-fix-pext-pgo.exe`, revision `1375dff`:
  `pawnless_scale` (`eval.h`) counts material in the donor's middlegame
  values (N 781, B 825, R 1276, Q 2538, `sf9587-ks/src/material.cpp:201-207`)
  for this rule alone; the scale is 0 below a rook's worth, 4 against at
  most a bishop's worth, 14 otherwise, and the evaluator calls the
  predicate. Every other `EG_MAT` use is unchanged.
  - Release PEXT final PGO, clang 23.1.4, clean manifest, built from a
    temporary worktree at its revision.
  - `bench 13` 19,470,325; SHA-256
    `B7C3277E8EC0337A2E6AD46A00E0DB4A97DFACAD9D6438E3CF02C91DC0F43A8F`.
- **Baseline.** `basilisk-a1131-head-pext-pgo.exe`, revision `302cb6d`, the
  head after A.13.1; `bench 13` 19,108,490; SHA-256
  `8BC14CB4A9B3E78546E814E41E33AA804EE498092A59E71395DAC811FB260EF0`.
- **Mechanism.** A queen against two minors keeps its winning score, and a
  rook-or-better ending against more than a bishop's worth is a near draw
  at 14/64 rather than 4/64, as in the donor.
- **Interactions.** A.13.1's KPK return sits before this scaler; A.11.9's
  bare-king tiers and their material order are unaffected (no override row
  changes); the rook-ending and OCB scalers still run only when this one
  did not. The Texel tuner does not mirror this scaler (C-1, owned by
  C.2.1). The scaler also fires when the defender has pawns, as in the
  donor.
- **Measured exposure.** 606 of the 8,864 games of BAS-S35's second run,
  BAS-S36 and BAS-S37 (6.8%) reach a position where the two scales differ
  for the side ahead in material, transient positions in an exchange
  included, so this is an upper bound. Some of those are a queen won as a
  draw, so the cap is the full one.
- **Competing explanation for a neutral reading:** most such positions
  arise inside a search whose depth already resolves them.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +3 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S35 and BAS-S37; report the LLR and the
    estimate, and the maintainer confirms;
  - H0: revert the engine commit `1375dff` and return the leaf to
    `RESEARCH`.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the scaler's truth table, the conversion floors and the WAC
  floor included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 38001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_a1132_gate.ps1`; results in
  `tools/results/a1132-gate/`.
- **Dry run accepted 2026-10-27**
  (`tools/results/colosseum_sprt_a1132-gate_20261027_152911.*`, dry run
  `1fc465ec…`, manifest `05c72a55…`).

#### QUALIFICATION (before the gate)

- On `1375dff`, Release PEXT without PGO: `bench 13` 19,470,325 (from
  19,108,490, +1.9%); CTest 24/24.
- Tests (`test_endgames`): the truth table through `pawnless_scale` —
  KQ-KBN, KQ-KBB, KQ-KNN and KQ-KR unscaled; KR-KB 4; KB-KN 0; KRB-KR,
  KRN-KR and KQ-KRB 14 — and the three queen-against-two-minors FENs served
  above +150 for the queen at depth 1 (+354, +458, +272), which the old
  scaler fails at all three (about +20, a sixteenth).
- Conversion floors and WAC unchanged: KQ-K, KR-K, KBB-K 24/24, KBN-K
  23/24, the same means; WAC 182/300 at depth 6.
- Speed: the PGO arms, four interleaved `bench 13` runs each, 3,019,506 nps
  against 2,914,598. The arms search different trees, so this reads tree
  shape, not a cost.

#### RESULT — append after exposure

- **Cap without a decision (maintainer-run 2026-10-27):** **+0.3 ± 4.0
  Elo** (nElo +0.5 ± 7.6), LLR +0.33 in [−2.94, 2.94] at the 4,000-pair cap
  (8,000 games); W/D/L 2188/3630/2182, pentanomial [94, 728, 2343, 748, 87].
  - 0 faults, 0 time losses; the independent PGN recount agrees.
  - The run took a fresh dry run with the registered hash
    (`colosseum_sprt_a1132-gate_20261027_153018.dry-run.json`,
    `1fc465ec…`); manifest `10284949…`, run record `08e6cf4a…`, PGN
    `7e89d333…`.
- **Verdict (registered rule):** the cap without a decision, so the repair
  stays as a correctness fix with no measured harm; the maintainer
  confirmed on 2026-10-27. The fingerprint record updates to `bench 13`
  19,470,325.
- Disposition: **cap, +0.3 ± 4.0; repair kept. CLOSED 2026-10-27.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +3 Elo, most likely the cap; not H0 at about
  85%.
- **Reading:** the cap at +0.3, at the bottom of the predicted range. As
  predicted for the outcome; the estimate sits nearer 0 than the centre.
- **What it teaches:** the measured exposure (6.8% of games) was an upper
  bound that counted transient positions; the positions that decide a game
  are rarer, or a search resolves them, so this repair's evidence is the
  scaler's truth table and the three queen-against-two-minors positions,
  which now guard it.
