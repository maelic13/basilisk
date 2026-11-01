# BAS-S44

**BAS-S44 — B.14: the evaluator's draw verdicts take no correction**

- **Registration.** Registered 2026-11-01, before any game; maintainer-run;
  correctness repair of PLAN B.14, the composition defect research
  amendment 4 measured (`analysis/b30_cluster2_research_2026-10-30.md`,
  amendment 4 §2 and §3.4) and research amendment 9 resolved (the
  narrowed form under clause C-A9-1; the maintainer's decision of
  2026-11-01).
- **Defect.** The evaluator classifies KNNK, the drawn KPK, the
  wrong-corner KBP-K and a pawnless side that cannot mate as 0, and
  `corrected_eval` and the quiescence stand-pat add the correction history
  on top of that 0, on both arms. The correction on the shared keys is
  trained by the drawn side's own root (an ordinary evaluation far from 0
  against a search value of 0), so a recognised draw reads a loss or a
  win: `5k2/8/6KP/8/8/8/8/5B2 b` (Kg8 the only drawing move) read −90 at
  depth 11 inside 3,075 nodes on the head's OFF arm, and the wrong-bishop
  positions read up to +172 after two same-structure priming searches in
  one process (24 of 28; the ON arm 27 of 28). Neither donor composes the
  two mechanisms.
- **Candidate.** `basilisk-b14-fix-pext-pgo.exe`, revision `1b2515b`,
  the OFF arm (`B3_PROOF=OFF`): where the raw evaluation is 0 and the
  evaluator reports one of its four draw verdicts (`Evaluator::draw_verdict`,
  one more evaluation of that node), `corrected_eval` adds no correction;
  an ordinary evaluation of exactly 0 keeps its correction. Training and
  B.3.1's producer guard (ticket 7b) are unchanged. The test
  "promotion: a queen where a rook only transposes" binds the OFF arm at
  200,000 nodes with a positive score (C-A9-1) in the same commit.
  - Release PEXT final PGO, clang 23.1.4, clean manifest.
  - `bench 13` 11,880,564; SHA-256 `77BB764C…`.
- **Baseline.** `basilisk-b14-base-pext-pgo.exe`, revision `c15d097`, the
  OFF arm of the head before it (B.3.1's accepted review), built from a
  temporary worktree; `bench 13` 16,615,757; SHA-256 `3CC41280…`.
- **Mechanism.** A classification has no error for the correction to
  repair, and the shared keys' training would overwrite the verdict. Both
  B.14 fixtures fail on the head and hold on the candidate on both arms:
  the organic FEN reads 0 at depths 8–14 and the primed 28 read 28/28.
- **Interactions.** Searched alone from fresh tables, the candidate moves
  6 of the 40 bench positions on the OFF arm (the endings whose search
  reaches a verdict: positions 3, 6 and 37–40) and 4 on the ON arm; the
  other positions are node-identical to the head. `bench 13` moves
  −28.5% on the OFF arm only because the positions share the table and
  histories. The conversion floors, the WAC reads and the wrong-bishop
  28 are identical to the head's to the decimal. The promotion fixture's
  pass at 100,000 nodes on the OFF arm was a tie at 0 held by the root's
  order on every form (amendment 9 §1); at 200,000 nodes every form sights
  the win.
- **Measured exposure.** A verdict position in search: a lone king
  against a pawn or two knights, the wrong-corner KBP-K, or a pawnless
  side that cannot mate, reached from an ending the game has entered.
  BAS-S40 read 39.8% of BAS-S38's games reaching a bare king, from which
  a verdict is one capture or promotion away; the four classes were not
  counted separately. The cap is the full one the return fixed; the
  change is confined to verdict nodes, so a null is the expected reading.
- **Competing explanation for a neutral reading:** the verdict nodes are
  leaves of decided endings, where the correction's lean changes how the
  defence is searched and rarely which result it reaches; the positions
  it can turn are the false wins and losses, which are rarer.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +2 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13` (OFF 11,880,564; ON 4,923,507 recorded beside it);
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S35, BAS-S37, BAS-S38 and BAS-S40; report the
    LLR and the estimate, and the maintainer confirms;
  - H0: revert the engine commit `1b2515b` and return the leaf to
    `RESEARCH` with the `raw == 0` form (`e883a13`) as the retry candidate.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate on both arms, the two B.14 fixtures and the conversion floors
  included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 44001; concurrency as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_b14_gate.ps1`; results in `tools/results/b14-gate/`.
- **Dry run:** dry run accepted 2026-11-01 (`tools/results/colosseum_sprt_b14-gate_20261101_213650.*`, dry run 32006BE5..., manifest 443FA844...)

#### QUALIFICATION (before the gate)

- On `1b2515b`, Release PEXT without PGO: `bench 13` OFF 11,880,564
  (from 16,615,757), ON 4,923,507 (from 6,723,172), equal on the Tune,
  Diag and Ablate flavours and on the final-PGO build; CTest 25/25 on both arms, and
  under ASan and UBSan (`-LE strength`) 23/23.
- Both B.14 fixtures (`test_endgames`): the organic FEN 7/7 and the primed
  28 on both arms; the wrong-bishop 28 (the test's form) 28/28.
- Conversion floors at the five budgets (30k/45k/60k/90k/120k): KQ-K,
  KR-K and KBB-K 24/24 at each on both arms; KBN-K OFF 22/23/24/23/23
  (115), ON 24 × 5 (120), the head's numbers to the decimal. The OFF
  arm's 120k run exits 1 on the floors tool's staleness guard (KQ-K's
  mean), as on the head.
- WAC at 100,000 nodes OFF 240, ON 241 (floor 232); at depth 6 OFF 182
  (floor 175), ON 132 (guard 119).
- The promotion fixture at 33k/60k/100k/200k/400k/1M: OFF `a5a3` +7 /
  `a5a3` 0 / `a5a3` 0 / `a7a8q` +781 / +781 / +781; ON `a7a8q` +347 /
  +347 / +957 / +957 / +1,307 / +1,374.

#### RESULT — append after exposure

- **H1 accepted (maintainer-run 2026-11-01):** **+7.4 ± 6.9 Elo** (nElo
  +14.4 ± 13.5), LLR +3.02 after 1,271 pairs (2,542 games); W/D/L
  705/1186/651, pentanomial [25, 200, 777, 234, 35]. A first-crossing
  estimate, selected at the boundary: it is the stop's reading, not the
  effect size.
  - 0 faults, 0 time losses; 32 post-terminal games kept outside the
    sample; the independent PGN recount agrees.
  - Both arms searched alike per move: 144,633 nodes and depth 14.10
    (without mate scores) for the repair, 145,360 and 14.18 for the head;
    2.02M and 2.03M nodes per second.
  - Manifest `colosseum_sprt_b14-gate_20261101_213822.manifest.txt`
    (repo `2fcc432`, clean; host 2.5% busy, not waived; dry run
    `32006BE5…`); run record `02E93EFE…`, PGN `362BAE55…`; the arms'
    hashes as pinned (`77BB764C…`, `3CC41280…`).
- **Correctness floors:** CTest 25/25 on both arms of `1b2515b` (23/23
  under ASan and UBSan), the two B.14 fixtures and the conversion floors
  included (§QUALIFICATION).
- **Verdict (registered rule):** H1, so the repair stays and the
  fingerprint record updates to `bench 13` 11,880,564 (ON 4,923,507).
- Disposition: **H1, +7.4 ± 6.9; repair kept. CLOSED 2026-11-01.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +2 Elo, most likely the cap; not H0 at about
  85%.
- **Reading:** H1 at +7.4 after 1,271 pairs. The sign is right; the
  predicted outcome, the cap, missed, and the estimate sits above the
  predicted range by its error.
- **What it teaches:** the exposure argument priced the change as the
  verdict nodes alone, leaves of decided endings. BAS-S40 taught that a
  false score in a band is judged by every decision that reads it; the
  same holds for a verdict overwritten by a correction: the search that
  reads a drawn KPK or wrong-corner ending as won or lost steers for
  it or away from it many plies earlier, in the rook and minor-piece
  endings that lead there (amendment 9 §1.3 measured the head's lean at
  the verdict nodes of one such line at −40 cp). As with BAS-S40 this is
  a reading, not a measured split; the games were not classified.
