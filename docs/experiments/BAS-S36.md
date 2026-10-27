# BAS-S36

**BAS-S36 — A.11.11: fail-soft quiescence in Stockfish's form**

- **Registration.** Registered 2026-10-27, before any game; maintainer-run;
  correctness repair B-19 (PLAN A.11.11), pulled forward from B.4 so that
  B.3 is not fitted around it. It runs only after BAS-S35 keeps the A.11.10
  repair, which is this gate's baseline; if BAS-S35 reverts it, this
  registration is withdrawn before any game.
- **Defect.** Quiescence fails hard: a fail-low returns `alpha`, so the
  window's edge echoes up the tree. Inside the equal-score tie rule's
  window an "equal" score is then usually that echo: 79,408 of the rule's
  83,061 promotions over `suite_v2` at depth 12 were of a score equal to
  alpha.
- **Candidate.** `basilisk-a1111-fs-pext-pgo.exe`, revision `69ecf42` (the
  fail-soft commit `3d31b50` with the A.11.10 repair merged): no table
  cutoff at a PV qnode; the stand-pat tightened by any bound that proves a
  better estimate, never by a decisive score; the delta prune returns and
  stores its upper bound; a futility-pruned capture raises the best to its
  optimistic value; the final store is the fail-soft best as an upper
  bound, never exact. The tie rule's `& 14` is unchanged.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.4, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 16,092,165; SHA-256
    `1F8A2058322ABC25514B1B14FE12573BF0FE35A99876B916F15554AEE8C6A43D`.
- **Baseline.** `basilisk-a1110-fix-pext-pgo.exe`, revision `3d7559a`, BAS-S35's
  candidate; `bench 13` 8,913,394; SHA-256
  `C8F6296C378A6F6FB2B388C01BDA63DC0C2EA3747FED3BA3FD31318A8BA5A511`. The
  candidate's other commits since are the Diag counters and records.
- **Mechanism.** Leaf values are the positions' own bounds rather than the
  window's edge, so a refuted move no longer ties the best at the leaves'
  echo, and the rules that compare a value with a margin (LMR's deeper
  re-search, the singular double and triple extensions, aspiration) see
  real distances.
- **Interactions.**
  - The tree at fixed depth grows: `bench 13` +80.5% on the repair, time
    to depth 13 +74% (5,476 ms against 3,149, four interleaved runs) at
    +3.9% nps. Over `suite_v2` at depth 12 the growth is spread over every
    family, most in LMR deeper re-searches (x3.3) and singular double
    (x3.0) and triple (x2.3) extensions, near dormant under fail-hard
    leaves.
  - RFP, razoring and aspiration were fitted on fail-hard values; B.3 to
    B.5 refit them on this search.
  - The tie rule now acts on genuine fail-low ties: promotions of an alpha
    echo 6,993, of a lower value 14,353.
  - The legacy kernel is untouched.
- **Measured exposure.** Quiescence runs in every game, so every game is
  exposed; the cap is the full one.
- **Competing explanations for a loss:** the margins fitted on fail-hard
  values, and a ply less at the same clock; neither is the fail-soft
  values' own error, and the leaf's verdict rule says how a loss is read.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **−2 Elo**, with a wide spread (−10 to +6): the
  donor rules the fail-hard leaves held dormant now run at their designed
  rate on margins fitted without them, against a deeper tree at the same
  clock. Confidence that the result is not H0: about 55%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix
    with no measured harm, as BAS-S28 and BAS-S32; report the LLR and the
    estimate, and the maintainer confirms;
  - H0: revert the engine commit `3d31b50` and return the leaf to
    `RESEARCH`, whose next question is whether the loss is the fail-hard
    fit of the neighbouring margins (B.3 to B.5) rather than fail-soft.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the conversion floors and the WAC floor included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 36001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_a1111_gate.ps1`; results in
  `tools/results/a1111-gate/`.
- **Dry run accepted 2026-10-27**
  (`tools/results/colosseum_sprt_a1111-gate_20261027_123019.*`, dry run
  `294ba988…`, manifest `676fa7f9…`).

#### QUALIFICATION (before the gate)

- On `69ecf42`, Release PEXT without PGO: `bench 13` 16,092,165 and the
  legacy arm 22,652,055 (the repair's, the kernel untouched); CTest ON
  24/24, OFF 21/21.
- Mate fixtures (fixed depth 18): KQK shortest mate 4, KBNK mate 5, the
  stalemate trap mate 1, as the fail-hard search.
- Conversion over 40 positions, 60,000 cold nodes, Hash 8: KR-K 40/40 at a
  mean 23.1 plies (the repair 40/40 at 22.1).
- WAC at depth 6, before the repair: 182/300 against the fail-hard 187.

#### RESULT — append after exposure

- **H1 accepted (maintainer-run 2026-10-27):** **+12.1 ± 9.9 Elo** (nElo
  +19.5 ± 15.9), LLR +2.95 after 917 pairs (1,834 games); W/D/L
  519/860/455, pentanomial [25, 212, 393, 248, 39]. A first-crossing
  estimate, selected at the boundary: it is the stop's reading, not the
  effect size.
  - 0 faults, 0 time losses; 15 post-terminal pairs kept outside the
    sample; the independent PGN recount agrees.
  - It ran after BAS-S35 kept its baseline, as registered. The run took a
    fresh dry run with the registered hash
    (`colosseum_sprt_a1111-gate_20261027_132605.dry-run.json`,
    `294ba988…`); manifest `e5ac1563…`, run record `04744d02…`, PGN
    `2025cd99…`.
- **Verdict (registered rule):** H1, so the repair stays and the
  fingerprint record updates to `bench 13` 16,092,165.
- Disposition: **H1, +12.1 ± 9.9; repair kept. CLOSED 2026-10-27.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about −2 Elo (−10 to +6); not H0 at about 55%.
- **Reading:** H1 at +12.1 after 917 pairs, above the predicted range.
  Missed high, in sign as well as size.
- **What it teaches:** the prediction priced the doubled tree as a cost and
  the fail-hard fit of the neighbouring margins as a risk. Both readings
  treated the extra nodes as waste. They are mostly the donor's own rules
  (LMR's deeper re-search, the singular double and triple extensions)
  running at the rate they were designed for, on values that now carry
  their distance from the window, and a fixed-depth node count does not
  price what they find. The margins fitted on fail-hard values were not
  the binding cost; B.3 to B.5 still refit them on this search.
