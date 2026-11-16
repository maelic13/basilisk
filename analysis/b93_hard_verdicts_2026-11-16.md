# B.9.3 — Hard verdicts against Syzygy

- State / class: closed; `READY_FOR_IMPLEMENTATION` for B.9.5 on V1 to V4,
  with V5 and V6 routed (§5); `R2`.
- Owner / date: 2026-11-16, on `master`'s engine at `ec9c86d` with one
  behaviour-neutral export (§1); `bench 13` 4,688,537.
- Decision needed: the routing of V5 and V6 (§5).

## 1. Scope and method

PLAN B.9.3's rule, frozen before the read: a site whose hard draws are
wins, or whose hard wins are draws, in more than 0.5% of its quiet positions
is a defect; its repair narrows the domain to what the tables call exact, or
discounts where the rule is approximate, and never refits a value.

**The instrument**, `tests/endgame_verdicts.cpp` (target `endgame_verdicts`),
built once here and meant for C.2.7 and C-142 to read again:

- A material class (`KRPvKR`, White the first side) is enumerated
  exhaustively under the board's symmetry, or sampled (`--random N`): the
  white king on the a1-d1-d4 triangle without pawns, on files a to d with
  them; each position with each side to move.
- A position is quiet when the side to move is not in check, has a move,
  and has no capture, en passant or promotion among its moves.
- Each quiet position is attributed to the evaluator site that fired, in
  the evaluator's own order: the search's dead-position rule
  (`Board::is_insufficient_material`), KNNK, KPK, a score at or above
  `KNOWN_WIN` (split into the queen tier, the rook tier and the minor mates),
  the wrong-bishop KBP-K and pawnless-0 draw verdicts
  (`Evaluator::draw_verdict`), the pawnless scales 4 and 14
  (`pawnless_scale`), KRP-KR's five draw rules and its other scales under
  the floor of 24, and the KRPP-KRP table under the floor.
- Where a site fired, the tables are probed (WDL, the fifty-move rule on).
  A cursed win or blessed loss is a draw and is counted beside. Wrong means:
  a draw claim where either side wins; a win claim where the favoured side
  does not; a scale where the side it scales down wins (BAS-E54's "won
  positions called draws").
- For each wrong site, a depth-8 search without tables (2M-node cap) on up to
  200 of its wrong positions, sampled across the enumeration, says how many
  the search itself resolves: it finds the win the site denied, or drops
  below 150 where the site claimed a win.
- The rook endings' rules are static in `eval.cpp`, so the instrument reads
  them through one export: `krpkr_scale_unfloored` (with the draw rule that
  fired) and `krppkrp_scale_unfloored`. Values unchanged; `bench 13`
  4,688,537 on the build that carries it.
- Tables: `D:/chess/tablebases/syzygy3456` and the 7-man `KRPPvKRP`. Every
  three- and four-man class exhaustively; KBPP-K and KRP-KR, the two
  five-man classes PLAN names, exhaustively (143.2M and 211.7M quiet
  positions); the other five-man classes and KRPP-KRP on 2,000,000 random
  quiet positions each (seed 20261116). Run time: about 35 minutes at 30
  threads.

**The population caveat**, stated before the verdicts because it shapes
them: the rule counts uniformly drawn legal positions. "Quiet" excludes an
available capture, not a piece the bare king forks or traps next move, so
the static claims are read on many positions a two-ply search resolves. The
search check is the measure of that; the verdicts below are the frozen
rule's all the same.

## 2. Results

Wrong rate, positions in brackets; "search" is the share of sampled wrong
positions a depth-8 search without tables resolves.

**Clean or under the line:**

| Site | Read | Wrong |
|---|---|---|
| Dead position (search): KN-K, KB-K, KB-KB same colour | exhaustive | 0 (60,723; 58,512; 1,050,743) |
| KPK draw and win | exhaustive | 0 (44,791; 98,458) |
| Wrong-bishop KBP-K, KBPP-K | exhaustive | 0 (58,110; 110,654) |
| KNNK draw | exhaustive | 0.0054% (88 of 1,627,889; search 88/88) |
| Queen and rook tiers, 3-5 men (23 classes) | exhaustive to 4 men, sampled at 5 | ≤ 0.0004% |
| Minor mates: KBB-K pair, KBBN, KBNN, KBBP, KBNP, KBBB | exhaustive / sampled | ≤ 0.027% |
| Pawnless 0 without pawns: KN-KN, KN-KB, KB-KN, KB-KB | exhaustive | ≤ 0.011% |
| Pawnless 0, KN-KNP and KB-KBP | sampled | 0.28%, 0.25% |
| Pawnless 4: KNN-KN, KNN-KB, KBB-KB | sampled | 0.059%, 0.016%, 0.43% |
| KRP-KR rule 2 (sixth, from behind); rule 3 (king on the square) | exhaustive | 0.26% (569,286); 0 (92,921) |

**Over the line (defects by the rule):**

| | Site | Classes (wrong, search resolves) |
|---|---|---|
| V1 | F6: bishops on one colour against a bare king | KBB-K same colour: all 765,152 score ≥ 300, all drawn |
| V2 | Pawnless 0 where the other side has pawns | KN-KP 4.24% (200/200), KB-KP 2.61% (200/200), KN-KPP 7.86% (182/200), KB-KPP 4.54% (170/200), KPP-K 68.6% of 3,490 (199/200) |
| V3 | KRP-KR draw rules 1, 4, 5 and the other scales under the floor | rule 1 Philidor 1.88% of 5,054,631 (200/200); rule 4 a7 6.93% of 202 (14/14); rule 5 blockade 2.68% of 395,368 (199/200); others under the floor 2.38% of 13,836,301 (196/200) |
| V4 | KRPP-KRP table under the floor | 5.53% of 6,346 sampled (160/200) |
| V5 | KBN-K's known win | 0.69% of 3,149,245 (158/200) |
| V6 | Pawnless 4 and 14, Stockfish's material scales | KR-KN 18.9%, KR-KB 6.0%, KBN-KN 6.8%, KBN-KB 1.3%, KR-KNP 7.8%, KR-KBP 3.8%; KR-KR 2.0%, KQ-KQ 4.8%, KRN-KR 4.0%, KRB-KR 5.2%, KQ-KRN 41.0%, KQ-KRB 29.6%, KR-KRP 0.97%, KRP-KR (14 when the defender is favoured) 0.80%; search 54% to 100% |

Examples, the side to move first (`eval` is the evaluator's, `wdl` the
tables', both the side to move's):

- V2: `8/8/8/8/7p/k7/6N1/K7 b` eval 0, wdl +2 (the pawn runs); KPP-K
  `8/8/8/8/2k5/2P5/2P5/1K6 w` eval 0, wdl +2: the evaluation favoured the
  bare king before scaling, and the scale made its 0 a verdict.
- V3: rule 1 `8/3kr3/8/8/8/8/2P5/KR6 w` scale 0 (floored 24), wdl +2; rule 4
  `R7/P6k/1K6/8/8/8/8/r7 w`; rule 5 `8/8/r5k1/6P1/8/8/7R/3K4 w`.
- V4: `2k3r1/3R4/8/2p5/8/1P6/K1P5/8 w`, wdl +2.
- V5: `8/8/8/8/8/8/8/K1BkN3 w` eval 19900, wdl 0: the king forks both minors.
- V6: KR-KN `8/8/8/8/8/8/8/KRn3k1 b`, wdl −2: the knight is lost.

## 3. Reading

- **V1** is a missing rule, not an approximation: the position is dead.
  Every same-coloured KBB-K position reads 770 to 11,860; the search's rule
  (`board.cpp`, `is_insufficient_material`) covers KB-KB and a lone minor
  only, and the evaluator's KXK gate rightly refuses the pair.
- **V2** is one cause in five classes. `pawnless_scale` picks the side the
  score favours and scales it to 0 when it cannot mate, and `apply_endgame`
  then calls the 0 a draw verdict, which takes no correction
  (`search_worker.h:468-473`). Where the other side has pawns, its winning
  chances are thrown away with the favoured side's. Without pawns on either
  side the verdict is exact to 0.011%.
- **V3 and V4** are Basilisk's own rules (BAS-E32, BAS-E54), floored at 24
  because unfloored they threw wins. They still call 1.9% to 6.9% of wins
  drawn. The search resolves almost all of those it is shown (≥ 98%,
  rule 4's 14 of 14, KRPP-KRP's 80%).
- **V5** is the bare king forking or trapping a minor, a tactic two plies
  deep. The search resolves 79% at depth 8.
- **V6** are Stockfish's material scale factors (`pawnless_scale`, its
  `material.cpp` form), approximate by design: one number per material
  class. The tables contradict them wherever the class has tactics (a loose
  minor, a king on the edge), from 0.8% to 41%. A repair that narrows them
  to what the tables call exact is a position-dependent rule per class:
  classical Stockfish's KRKN, KRKB and KQKR evaluators, Phase C's donor
  families.

## 4. The search beside the leaves

PLAN asked for Basilisk's four wrong-bishop draws to read as draws at depths
8 to 14, from cleared tables and after the two lost same-structure positions
have trained them. Both tests exist and pass on the head:
`test_wrong_bishop_draw_in_search` and
`test_recognised_draw_takes_no_correction` (`tests/test_endgames.cpp`;
CTest 57 of 57). Rarog's own failures there (its D.6.23) do not reproduce.

## 5. Decision and routing

- **V1, V2: `READY_FOR_IMPLEMENTATION`** (§6).
- **V3, V4: `READY_FOR_IMPLEMENTATION`** on an acceptance rule, not a
  form: each rule narrowed until the exhaustive KRP-KR read (KRPP-KRP: the
  seeded 2M sample) puts it at or under 0.5%, or dropped where it cannot
  be. The narrowing is chess design within the rule's own terms (king and
  rook squares, tempo), read with the instrument; no scale value changes.
- **V5: kept, recommended.** No static narrowing is exact: the miss is a
  fork. The search resolves most of it, and the known win is what drives
  KBNK's conversion (BAS-E29, BAS-E30). Retry trigger: a KBNK game lost to
  a fork or trap in any gate's conversion read.
- **V6: routed to Phase C, recommended.** The only repair the rule allows
  is a per-class evaluator, a redesign a later leaf owns, which is the same
  test B.9.4 applies to deferred defects. Until then a known limitation for
  1.11.0's notes; C.2.7 reads these classes again with this instrument.
  Retry trigger: any of these classes losing a half point in a conversion
  read with tables off.

Both recommendations depart from the letter of the frozen rule, which calls
V5 and V6 defects for B.9.5. Main counter-argument: KR-KN, the worst class
the search fails on (20% of its sample), is a real ending in games. The
measured rates stay in the record unchanged either way.

**Decided 2026-11-16 (maintainer).** None of the six stops play: no crash,
illegal move or protocol fault, only scores. The rule: a wrong evaluation
goes to Phase C with the other evaluation work, unless it is easy to fix
now.

- **B.9.5 repairs V1 and V2.** Both are a single condition, change no
  value, and neither is only an evaluation error. V1 is a game rule: a
  position no sequence of legal moves can mate is drawn (FIDE 5.2.2), and
  the search's dead-position rule misses it. V2 is a 0 the search treats
  as exact, which turns off the correction where the tables say the pawn
  side wins.
- **V3, V4 and V6 go to C.7.5.** Each is a rule or class scale to narrow
  by chess design, read with this instrument, and can move rook endings.
  That is evaluation work, not an easy fix.
- **V5 is kept**, with §5's retry trigger, which C.7.6 holds beside the
  KBN-K drive (C-144).

The V3 and V4 handoff in §6 moves with them unchanged. B.9.4 lists V3 to
V6 as known limitations for 1.11.0 and does not read them again.

## 6. Implementation handoff for B.9.5

- **V1.** The search's dead-position rule extended: every non-king piece
  on the board a bishop, and all of them on one colour, is dead, whatever
  the count and on either side. Then the evaluator's verdict is unused
  there. Test: `test_same_colour_bishops` drops `--expect-defect`; the
  instrument's KBB-K `--estimates` read shows no same-coloured position
  scored ≥ 300.
- **V2.** `draw_verdict` from the pawnless scale only when the other side
  has no pawns either. The 0 stays the score (no value refitted); it stops
  being a verdict, so the correction applies there. The site then reads
  exact (≤ 0.011%) on the pawn-free classes, and KN-KP, KB-KP, KN-KPP,
  KB-KPP and KPP-K leave it.
- **V3, V4.** §5's rule, now C.7.5's by the decision in §5; B.9.5
  repairs V1 and V2 only.
- **Invariants.** No value changes; the queen and rook tiers, KPK, KNNK,
  the wrong-bishop draw and the dead rule's existing classes read as today.
  V1 and V2 can move `bench 13` only if its searches reach those classes;
  V3 and V4 can, through rook endings. A moved fingerprint is recorded.
- **Qualification.** The golden test (`endgame_verdicts_fixtures`) and its
  file updated in each repair's commit, the full reads of §2 repeated for
  the classes the repair touches.
- **Games.** V1 and V2 change a score only where the tables say the old one
  was wrong; V3 and V4 change rook endings. They join B.9.5's repair SPRT
  if a static differential shows a score outside their site, as PLAN B.9.5
  says.

## 7. Tests

- `endgame_verdicts_fixtures`: the instrument on a seeded sample of 20,000
  quiet positions per class (one thread, deterministic) over nine fixture
  classes, compared line for line with `tests/data/endgame_verdicts_golden.txt`,
  exit status 1 while a site is over the line. The sample shows V5 (KBN-K),
  V2 (KPP-K), V6 (KR-KR, KR-KB) over the line and pins the clean sites at
  their counts. Seven four-man WDL tables join the fixtures (82 KB,
  `tests/fixtures/syzygy/README.md`).
- `test_same_colour_bishops --expect-defect` (label `known-defect`): five
  same-coloured positions, all read as wins on the head.
- Release CTest 57 of 57; the two new tests and `test_endgames` on the
  sanitizer build.

## 8. Instruments

The instrument is C.2.7's drawn-overclaim read (C-133) and the source of
the Syzygy-verified vetoes (C-142). With `--estimates` it also reads the
evaluation's unclaimed scores against the tables, which is how F6 was
measured. `tools/diag/tb_gate_audit.py conversion` is the game-level read of
V5 and V6.

## 9. Artifacts

`tools/results/b93-verdicts-20261116/` (ignored): `men3-4.txt` (every three-
and four-man class), `f6-kbbk.txt`, `men5-random.txt`, `men7-krppkrp.txt`,
`men5-exhaustive.txt` and the runner `run_men5_7.sh`. Each read repeats with
`endgame_verdicts --tables "D:/chess/tablebases/syzygy3456;D:/chess/tablebases/syzygy7" --threads 30 [--random 2000000] CLASS...`.
