# BAS-S31

**BAS-S31 — A.11.7: the pawnless scaler's winning side taken from the score**

- **Registration.** Registered 2026-10-26, before any game; maintainer-run;
  correctness repair from the Rarog audit (PLAN A.11.7; audit C-3, A-22,
  C-103), an evaluation change inside the freeze that the leaf admits.
- **Defect.** The near-draw scaler for a pawnless winning side
  (`apply_endgame`, `src/eval.cpp`) picked that side by material totals,
  pawns included. Equal totals always made it White, so a position and its
  colour mirror could be scaled differently (−9 against −150 on
  `3qk3/pp6/8/8/8/8/8/RNB1K3 w`); and where the totals and the score
  disagreed, the richer side's lack of pawns could scale down the other
  side's advantage.
- **Candidate.** `basilisk-a117-fix-pext-pgo.exe`, revision `d5442f8`: the
  winning side is the one the score favours, as in Stockfish
  (`eg > VALUE_DRAW` in `sf9587`'s `evaluate.cpp`). Nothing else changes.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 7,384,101; SHA-256
    `24C1629C5B5700A65102E2A842E69DAB3F45D662285C91D379B60A7C6AC030CF`.
- **Baseline.** `basilisk-a116-fix-pext-pgo.exe`, revision `07a357f`, the
  A.11.6 candidate that BAS-S30 accepted, whose engine is the head's before
  this repair; `bench 13` 7,233,823; SHA-256
  `EA3114242C434FA70E13E6113021C7F550DB6E0FC681137B98C765010D1DA09B`.
- **Mechanism.** A drawish pawnless ending is scaled for the side that is
  winning it, so the score of the side ahead is never cut by the other
  side's material shape.
- **Interactions.** The rook-ending and OCB scalers run only when this one
  did not (`scaled`), so a position this repair stops scaling can now reach
  them. The tuner does not mirror this scaler (C-1, owned by C.2.1). The
  legacy kernel shares the evaluation, so its bench moves too.
- **Competing explanation for a neutral reading:** the scaler fires only in
  pawnless endings with a small material gap, which games rarely reach.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +2 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix
    with no measured harm, as BAS-S20, BAS-S28 and BAS-S29; report the LLR
    and the estimate, and the maintainer confirms;
  - H0: revert the engine commit and return the leaf to `RESEARCH`.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the conversion floors and the WAC floor included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 31001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_a117_gate.ps1`; results in `tools/results/a117-gate/`.
- **Dry run:** not yet run. On 2026-10-26 the wrapper refused it as the host
  was not idle (another session's census was compiling); run it before the
  gate.

#### QUALIFICATION (before the gate)

- On `d5442f8`, Release PEXT without PGO: `bench 13` 7,384,101 (from
  7,233,823, +2.1%); the legacy OFF arm 17,606,287 (from 15,887,213), since
  the evaluation is shared; CTest ON 24/24, OFF 21/21.
- Tests: an equal-totals position equals its mirror (the old scaler gave
  −9 against −150); a symmetry sweep over 3,149 positions (the bench and
  WAC positions, ten odd- and promoted-material positions, each with a
  seeded eight-ply random walk) finds none asymmetric, where the old
  scaler gave nine.

#### RESULT — append after exposure
