# BAS-S32

**BAS-S32 — A.11.9: bare-king wins in tiers that rise with material**

- **Registration.** Registered 2026-10-26, before any game; maintainer-run;
  correctness repair from the Rarog audit (PLAN A.11.9; audit C-9, C-131),
  an evaluation change admitted into the freeze by the maintainer
  (2026-10-26).
- **Defect.** Against a bare king, queen and rook mates scored ordinarily
  (about 2,400 for KQB-K) while the KBNK override scored 15,600 or more and
  KXK's minor-piece mates about 10,650 or more. More material therefore
  scored lower: from KBP-K a promotion to a knight or a second bishop
  outranked a queen, and losing a pawn could raise the score.
- **Measured exposure.** Over 26,574 gate and read games (BAS-S27 to
  BAS-S31), about 1.3% of `3+0.03` games and 0.3% of `10+0.1` games entered
  an override from a position with a pawn, almost always by underpromotion;
  the strong side then drew 17 of 352 (4.8%) positions a queen wins.
- **Candidate.** `basilisk-a119-fix-pext-pgo.exe`, revision `fdf4277`:
  bare-king wins sit in tiers by the winner's strongest piece, queen
  (28,700 to 31,742) above rook (26,100 to 28,699) above the minor-piece
  mates (below 26,100). The queen and rook tiers carry the ordinary score
  above their base; KBNK's drive keeps its weights with its base moved
  from 15,600 to `KNOWN_WIN`; pawns add 3 each within a minor tier.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 7,857,713; SHA-256
    `FE85A3EF8AA2BD784D87861601AAD0DDF122A219E321FA3B4807B6F3EBD10CE0`.
- **Baseline.** `basilisk-a117-fix-pext-pgo.exe`, revision `d5442f8`, the
  A.11.7 candidate BAS-S31 accepted, whose engine is `master`'s except for
  A.11.8's output changes; `bench 13` 7,384,101; SHA-256
  `24C1629C5B5700A65102E2A842E69DAB3F45D662285C91D379B60A7C6AC030CF`.
- **Mechanism.** A winning side converts with the most material it can
  have: it promotes to a queen and keeps its pawns, so its mates are the
  short ones, inside the search horizon and the fifty-move clock.
- **Interactions.**
  - The KBNK drive's magnitude reaches KBP-K trees through knight
    promotions (BAS-E49: KBP-K 9/24 to 17/24); the base moves by 5,600 but
    the drive's steps are unchanged.
  - BAS-E34's queen and rook override cost +20.5% bench nodes with large
    drives; carrying the ordinary score instead costs +6.4% here.
  - The pruning margins act on these scores like any other; the conversion
    floors measure that.
  - The legacy kernel shares the evaluation: its bench moves from
    17,606,287 to 23,696,970.
- **Competing explanations for a neutral reading:** the boundary is met in
  about 1% of games, so most games are unchanged.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +2 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 80%.
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
  tablebases off; seed 32001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_a119_gate.ps1`; results in `tools/results/a119-gate/`.
- **Dry run accepted 2026-10-26**
  (`tools/results/colosseum_sprt_a119-gate_20261026_152216.*`, dry run
  `9f81f044…645e`, manifest `6676624b…b1cc`).

#### QUALIFICATION (before the gate)

- On `fdf4277`, Release PEXT without PGO: `bench 13` 7,857,713 (from
  7,384,101, +6.4%); the legacy OFF arm 23,696,970 (from 17,606,287);
  CTest ON 24/24, OFF 21/21.
- Conversion floors at 60,000 nodes per move: KQ-K 4/4, KR-K 4/4, KBB-K 6/6,
  KBN-K 6/6; the WAC floor holds.
- Screens on the way: tiers with only a small drive held KQ-K but lost KR-K
  (2/4) and left a wandering queen in a tablebase analysis line; carrying
  the ordinary score in the queen and rook tiers fixed both.
- **The underpromotion screen.** The 342 positions where those games
  underpromoted into an override, searched at 100,000 nodes: `master`'s
  engine underpromotes in 337 (160 to a knight, 177 to a bishop); the
  candidate in none (289 queen promotions, 53 other moves).
- Test `test_eval`: from KBP-K a queen promotion outranks a rook and a rook
  a knight or bishop; KBN-K and KR-K with an extra pawn score at least as
  much as without it. Both fail on the old evaluation.

#### RESULT — append after exposure

- **Cap without a decision (maintainer-run 2026-10-26):** **−0.3 ± 4.0 Elo**
  (nElo −0.5 ± 7.6), LLR −0.33 in [−2.94, 2.94] at the 4,000-pair cap
  (8,000 games); W/D/L 2171/3652/2177, pentanomial [97, 754, 2286, 784,
  79].
  - 0 faults, 0 time losses; the independent PGN recount agrees, and the
    score recomputed from the pentanomial matches.
  - The run took a fresh dry run before starting
    (`colosseum_sprt_a119-gate_20261026_152646.dry-run.json`,
    `9f81f044…`); engines, seed and run file are the registered ones.
  - `colosseum_sprt_a119-gate_20261026_152646.*`, manifest `9c8d22a0…`,
    run record `3fc5bab3…`, PGN `70279cef…`.
- **Verdict (registered rule):** the cap without a decision, so the repair
  stays as a correctness fix with no measured harm, on the maintainer's
  confirmation; the fingerprint record then updates to `bench 13`
  7,857,713 (the legacy arm 23,696,970).
- Disposition: **cap, −0.3 ± 4.0; the repair is kept (maintainer
  confirmed 2026-10-26). CLOSED 2026-10-26.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +2 Elo, most likely the cap; not H0 at about
  80%.
- **Reading:** the cap came, centred at −0.3, at the low edge of the
  predicted range, with the LLR leaning neither way. Hit on the outcome,
  slightly low on the centre.
- **What it teaches:** the boundary is met in about 1% of games, so even a
  repair that removes nearly every underpromotion (337 of 342 positions)
  cannot move the score measurably at `3+0.03`; its value is correctness
  and the shorter mates it chooses.
