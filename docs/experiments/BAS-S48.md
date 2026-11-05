# BAS-S48

**BAS-S48 — B.4.2: SEE scores promoting recaptures, `[-5,5]`**

- **Registration.** Registered 2026-11-05, before any game; maintainer-run;
  correctness repair A-3 of PLAN B.4.2 (BAS-C09's promotion half, repaired
  under the known-defects rule of 2026-10-26: none is kept). Its own gate,
  as the leaf requires; A-2 is BAS-S49.
- **Defect.** A pawn recapturing onto a back rank promotes: it wins the
  queen's surplus over the pawn and leaves a queen on the square. Both SEE
  kernels scored it as a plain pawn. The two errors cancel when the queen
  is recaptured; when it stands the exchange is 800 worse than SEE said,
  and `see_ge`'s "safe even if my piece is taken" exits (the first
  `swap <= 0` and the loop's `swap < res`) answered before looking at the
  recapture. B.2's quiet SEE sends quiet moves through the same loop, so a
  quiet move to a back-rank square an enemy pawn attacks was read as
  losing at most the mover.
- **Candidate.** `basilisk-b42-promo-pext-pgo.exe`, revision `9aaa528`:
  `see_ge` charges the surplus at both exits when the next recapturer is a
  pawn the pins allow; `see()` (tests only) adds it to the gain.
  - Release PEXT final PGO, clean manifest; `bench 13` 8,532,389;
    SHA-256 `416A153C…`.
- **Baseline.** `basilisk-b42-base-pext-pgo.exe`, revision `9f29f49`, the
  branch head before the repair (its engine source is `master`'s,
  `d7f3178`), built from a temporary worktree; `bench 13` 5,361,058;
  SHA-256 `EA12EE3A…`.
- **Mechanism.** Quiescence's and the main search's SEE pruning, the
  picker's good/bad capture split and the quiet SEE prune read `see_ge`
  verdicts. A verdict that misses a promoting recapture admits a capture
  or a quiet move onto a promotion square that loses a queen's worth, and
  rejects nothing it should keep, so the repair can only remove false
  "safe" verdicts there.
- **Invariant that fails on the old behaviour.** `tests/test_board.cpp`:
  "promotion recapture: the promoted queen stands" and its mirror read 0
  on the old kernels and −800, the make/unmake oracle's value, on the
  candidate; "the pawn is pinned" binds the pin filter on the surplus.
  The quiet corpus's promotion-recapture geometry falls from 22 to 0.
- **Measured exposure.**
  - On a 104,000-move random-walk corpus from the bench positions
    (`see_ge` boundary against the oracle on every legal move), 48
    verdict boundaries change, all on a back rank, all wrong before and
    equal to the oracle after; the 1,183 bench-root moves are unchanged.
  - In search, the branch head's `bench 13` tree (both repairs) with the
    old kernel run beside the new one on every call: 5,226 of 7,660,016
    `see_ge` calls change verdict (0.068%), 5,126 of them on a back rank.
  - Each bench position searched alone from fresh tables at depth 13: 10
    of 40 trees change (positions 2, 4, 5, 11, 19, 22, 23, 25, 31, 34),
    and position 34's best move. `bench 13` moves +59% only because the
    positions share the table and histories.
  - So the change reaches the games that reach a pawn one step from
    promotion with pieces left: a large share of games, as BAS-S44's
    verdict endings were. The cap is the full one.
- **Competing explanation for a neutral reading:** the false "safe"
  verdicts sit at nodes where the pawn's promotion already dominates the
  search (checks, the push itself), so correcting the exchange changes
  few decisions that the push or the evaluation had not already settled.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +3 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S35, BAS-S37, BAS-S38 and BAS-S40; report the
    LLR and the estimate, and the maintainer confirms;
  - H0: the wrong form is not restored (PLAN *Known defects*); B.4.2
    returns to `RESEARCH` with the loss, and the rework is the refit of
    the thresholds that read the verdict, quiescence's SEE pruning on
    B.4.6's surface first; the maintainer decides.
- **Correctness floors, read before the result is acted on:** release
  CTest and ASan/UBSan CTest on the candidate, the SEE fixtures and the
  conversion floors included (§QUALIFICATION).

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 48001; concurrency as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_b42_promo_gate.ps1`; results in
  `tools/results/b42-promo-gate/`.
- **Dry run:** DRYRUN_PROMO

#### QUALIFICATION (before the gate)

- On `9aaa528`, Release PEXT without PGO: `bench 13` 8,532,389 (from
  5,361,058), equal on the final-PGO build; `test_board` 399/399.
- On the branch head `d3add9c`, which carries both repairs: release CTest
  24/24 (`-LE strength`) and 2/2 (`-L strength`: the WAC floor and the
  conversion floors), ASan/UBSan CTest 24/24 (`-LE strength`).
- Conversion floors at the five budgets on `d3add9c`
  (`tools/results/b42-20261105/conversion/`): identical to B.4.1's table
  on the head before the repairs (KQ-K, KR-K, KBB-K 24 at every budget;
  KBN-K 21/22/24/24/24, 115).

#### RESULT — append after exposure

#### PREDICTION CALIBRATION — preserve the original prediction above
