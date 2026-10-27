# BAS-S37

**BAS-S37 — A.13.1: a won KPK scores as a known win**

- **Registration.** Registered 2026-10-27, before any game; maintainer-run;
  correctness repair F1 of the code audit (PLAN A.13.1), an evaluation
  change admitted into the evaluation freeze (maintainer decision
  2026-10-27).
- **Defect.** `apply_endgame` kept the tapered score of a KPK the bitbase
  calls won. Where that score favoured the bare king, the pawnless scaler
  took the bare king for the strong side and scaled the win to 0. Against
  the KPvK Syzygy table over all 662,704 legal KPK positions: 2,022 wins
  served 0 (mostly a second-rank pawn with the lone king beside it,
  `8/8/8/8/8/8/Pk6/3K4 w`), and 443,106 more decided positions scored
  below a known win, at their tapered values of about 100 to 900.
- **Candidate.** `basilisk-a1131-fix-pext-pgo.exe`, revision `6e39087`: a
  won KPK returns `KNOWN_WIN` + 206 (the donor's `PawnValueEg`) + the
  pawn's relative rank, 10,207 to 10,212, from the pawn side's view, as
  Stockfish's `Endgame<KPK>` (`sf9587-ks/src/endgame.cpp:153-171`), before
  every tier and scaler; a drawn KPK is still 0.
  - Release PEXT final PGO, clang 23.1.4, clean manifest, built from a
    temporary worktree at its revision.
  - `bench 13` 19,108,490; SHA-256
    `BD45E2DD69ABE25D52B17CE8CD23DCCC904D47CB2E18E3DE3F58DD9054837A16`.
- **Baseline.** `basilisk-a1111-fs-pext-pgo.exe`, revision `69ecf42`,
  BAS-S36's candidate, whose engine master reproduces (`bench 13`
  16,092,165); SHA-256
  `1F8A2058322ABC25514B1B14FE12573BF0FE35A99876B916F15554AEE8C6A43D`.
- **Mechanism.** A won KPK ranks as a known win, so the search no longer
  values a trade into it at an ordinary pawn's worth, and no won KPK can be
  served as a draw.
- **Interactions.**
  - The value lies above `KNOWN_WIN` and below the rook tier (26,100) and
    `is_decisive` (31,743), so pruning treats it as an ordinary evaluation.
  - The pawnless scaler is unchanged; it never sees a KPK now. KPPK, KNPK
    and KBPK reach a won KPK only when the defender captures, which minimax
    resolves, as in the donor.
  - Rule 50: the donor returns its KPK value before its damping; Basilisk
    damps every score, a deliberate departure.
  - Search trees: KPK positions in deep lines move 38 of the 40 bench
    positions.
- **Measured exposure.** 189 of the 4,864 games of BAS-S35's second run and
  BAS-S36 (3.9%) reach a KPK position; the positions the old score
  misjudged are 0.31% of KPK. The cap is a little above the small-exposure
  one, since the score also moves the trades into KPK.
- **Competing explanation for a neutral reading:** games at 3+0.03 mostly
  converted KPK through search depth already.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +2 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S35; report the LLR and the estimate, and the
    maintainer confirms;
  - H0: revert the engine commit `6e39087` and return the leaf to
    `RESEARCH`.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the KPK truth test, the conversion floors and the WAC floor
  included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 37001; concurrency 15 as the wrapper resolves it.
- **Cap: 2,000 pairs.**
- Launcher `tools/run_a1131_gate.ps1`; results in
  `tools/results/a1131-gate/`.
- **Dry run accepted 2026-10-27**
  (`tools/results/colosseum_sprt_a1131-gate_20261027_142616.*`, dry run
  `008bb0c5…`, manifest `89dbfb27…`).

#### QUALIFICATION (before the gate)

- On `6e39087`, Release PEXT without PGO: `bench 13` 19,108,490 (from
  16,092,165); CTest 24/24.
- The KPK truth test (`test_endgames`, a bundled `KPvK.rtbw`): 662,704
  legal positions, 0 unprobed, 0 of the wrong sign and 0 decided below a
  known win, in 1.1 s; on the old code 2,022 of the wrong sign and 443,106
  below a known win.
- Conversion floors and WAC unchanged: KQ-K, KR-K, KBB-K 24/24, KBN-K
  23/24, the same means; WAC 182/300 at depth 6.
- Speed: the PGO arms, four interleaved `bench 13` runs each, 2,884,428 nps
  against 2,940,692. The arms search different trees, so this reads tree
  shape, not a cost.

- Disposition: registered, maintainer-run.
