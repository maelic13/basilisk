# BAS-S33

**BAS-S33 — A.11.9 follow-up: root underpromotions judged against the queen**

- **Registration.** Registered 2026-10-26, before any game; maintainer-run;
  a correctness repair found while closing A.11.9, on the same branch
  (maintainer decision 2026-10-26).
- **Defect.** A queen makes every move a rook or bishop can, so a rook or
  bishop promotion is better only where it avoids a stalemate. The search
  nevertheless chose one where it merely scored a few points above the
  queen: a rook line that transposes into the queen line is a shallower
  subtree that has not yet met the reply working only against the rook.
- **Measured exposure.** Among 228 rook promotions played in the A.11.x
  gate games where a queen would not have stalemated, the A.11.9 engine at
  100,000 nodes promotes to a rook or bishop in 59; in seven of them
  Stockfish 18 rates the rook worse than the queen. Stockfish 18 itself
  picks the rook in 50 of the 228, so most of these picks cost nothing.
- **Candidate.** `basilisk-a119-up-pext-pgo.exe`, revision `1641e64`: at
  the root a rook or bishop promotion counts as better than the queen
  promotion on the same squares only when its exact score exceeds the
  queen's by more than 200; otherwise it takes the queen's score. The
  queen promotion is searched first at the root, so its score is known; a
  fail high is a bound and is never judged. Nothing changes below the
  root.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 7,857,713; SHA-256
    `0D80B72FB5B2196283F70CA2C8E25F97F7CA4B603460AA5C579860D587DBBCEF`.
- **Baseline.** `basilisk-a119-fix-pext-pgo.exe`, revision `fdf4277`, the
  A.11.9 candidate BAS-S32 kept; `bench 13` 7,857,713; SHA-256
  `FE85A3EF8AA2BD784D87861601AAD0DDF122A219E321FA3B4807B6F3EBD10CE0`. The
  candidate's other change since, the repeated final line through
  `send_info` (`28b7018`), is output only.
- **Mechanism.** The root plays the queen unless the weaker piece wins
  clearly more, so a transposing rook line no longer displaces it, while a
  stalemate escape (a draw against a win) still clears the margin.
- **Interactions.**
  - Root move order: the queen sibling is pulled ahead of a rook or bishop
    promotion the picker delivers first (the best move of an earlier
    iteration comes first). Without it the rule held only 5 of the 7.
  - PVS and aspiration: a null-window or aspiration fail high is a lower
    bound; capping it stopped the re-search from finding Saavedra's
    `c8=R` (the screen below), so only scores below beta are judged.
  - `searchmoves`, MultiPV exclusions and the tablebase root filter apply
    to the pulled queen as to any root move.
  - The legacy kernel is untouched.
- **Rejected on the way.** Pruning rook and bishop promotions below the
  root, searching them at the root only where the queen stalemates at once
  (`de8f3e1`), fixed the seven but cannot see a stalemate that comes later,
  as in Saavedra's study, so it was reverted in the candidate.
- **Competing explanations for a neutral reading:** a root underpromotion
  arises in well under 1% of games, so most games are unchanged.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 Elo**; most likely the cap without a decision.
  Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays; the fingerprint is unchanged;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S32; report the LLR and the estimate, and the
    maintainer confirms;
  - H0: revert the engine commits `de8f3e1` and `1641e64`, so the engine
    is A.11.9's again, and return the follow-up to research.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the promotion sections of `test_search` included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 33001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_a119_up_gate.ps1`; results in
  `tools/results/a119-up-gate/`.
- **Dry run accepted 2026-10-26**
  (`tools/results/colosseum_sprt_a119-up-gate_20261026_172409.*`, dry run
  `3980489f…8da8`, manifest `4d60d12c…1fc5`).

#### QUALIFICATION (before the gate)

- On `1641e64`, Release PEXT without PGO: `bench 13` 7,857,713 and the
  legacy arm 23,696,970, both exact; CTest ON 24/24, OFF 21/21, and
  `test_search` again with the new sections on both arms.
- Speed: the PGO arms, four interleaved `bench 13` runs each on an idle
  host, 2,815,142 nps against 2,813,611 (+0.05%, noise).
- **Screens at 100,000 nodes, Hash 64.**
  - The 228 positions: rook or bishop picks 59 → 1; the one left,
    `a7b8r` taking a queen, wins either way and is `a7b8q` at 1M nodes.
  - The seven Stockfish errors: 7 rook picks → 0.
  - The 25 positions where a queen stalemates at once: 22 underpromotions
    on both; one position plays `c4c3` (mate in 6) instead of `d2d1r`
    (mate in 4) at 100,000 nodes and `d2d1r` at 1M.
  - Saavedra's study after 5...Rd4: `c7c8r`, mate in 7, at 100,000, 200,000
    and 1,000,000 nodes, as the A.11.9 engine.
- Tests (`test_search`, promotion sections): Saavedra's `c7c8r` with a mate
  score, on both arms; `g7f8q` where the rook was searched first, ON arm.
  Removing the exact-score guard fails the first (`c2b3`); removing the
  queen-first order fails the second (`g7f8r`).

#### RESULT — append after exposure

- **H0 (maintainer-run 2026-10-26):** **−2.7 ± 4.1 Elo** (nElo −5.5 ±
  8.4), LLR −3.00 in [−2.94, 2.94] at 3,285 pairs (6,570 games); W/D/L
  1773/2973/1824, pentanomial [72, 538, 2103, 513, 59].
  - 0 faults, 0 time losses; the independent PGN recount agrees.
  - The run took a fresh dry run with the registered hash
    (`colosseum_sprt_a119-up-gate_20261026_195900.dry-run.json`,
    `3980489f…`); manifest `7ac82b27…`, run record `275b3f56…`, PGN
    `b08af6ca…`.
- **Verdict (registered rule):** H0, so the engine commits are reverted
  (`c1791b3`): the engine is A.11.9's again. The stalemate and Saavedra
  tests stay; the two tests only the rule passed go with it.
- **Replay of the games (after exposure, not a verdict).**
  - Games in which no promotion was ever legal, where the two engines play
    identically: −0.6 ± 4.5 (2,823 games). Games in which one was: −4.3 ±
    10.4 (3,747 games).
  - Every promotion played was searched again by the other engine at the
    mover's node count: in 176 games (2.7%) the two choose differently
    between a queen and an underpromotion, 177 decisions. Stockfish 18 at
    1M nodes rates the candidate's choice better in 21, equal in 148, the
    baseline's in 8, none of which changed a result.
  - Over 95,637 positions from the A.11.x gate games where an
    underpromotion is legal, the engines' choices differ at 100,000 nodes
    in 3,159: 1,404 queen against rook or bishop, as intended, and 1,755
    other moves, because searching the queen first changes the root
    search wherever a promotion is legal. The registration did not
    foresee the second kind.
  - The H0 is most likely noise around a true effect near 0, but that is
    a reading after exposure; BAS-S34 is the targeted re-test.
- Disposition: **H0, −2.7 ± 4.1; reverted. CLOSED 2026-10-26.** Retry
  trigger: BAS-S34, which reached its cap at −0.3 ± 1.1 on the positions
  where the rule acts; the rule is re-applied (`d7d2c34`).

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 Elo, most likely the cap; not H0 at about 85%.
- **Reading:** H0 at 82% of the cap, centred at −2.7. A miss on the
  outcome; the centre is within the interval of 0.
- **What it teaches:** a `[-5,5]` SPRT cannot tell 0 from a small loss on
  a change that touches 3–6% of games, and ends at H0 about as often as
  at H1 when the truth is 0. Such a change is measured where it acts (now
  the rule in `agents/measurement.md` *Gating*). The registration also
  understated the footprint: reordering the root changes more than the
  promotion choice.
