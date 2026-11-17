# C.7.5 input: what B.9.3's routed verdicts get wrong, and why no quick narrowing fixes them

2026-11-17, on B.9.5's branch after `a669288`. This is a
read for C.7.5, not a leaf: the maintainer asked for an attempt at the
verdicts routed to Phase C on 2026-11-16 (V3, V4, V6), to be placed in PLAN
afterwards. **Recommendation: implement none of them now.**
- V3 and V4 are discounts by design, already floored, and a short search
  resolves them.
- V6 needs a fitted per-class evaluator, which is C.7.5's work.

Evidence below; raw files in `tools/results/c75-rook-20261117/`.

## 1. Method

- The hard-verdict instrument gained `--dump FILE`
  (`tests/endgame_verdicts.cpp`). It writes every judged position's site,
  FEN and the side to move's WDL, so each site's errors can be split by its
  own conditions. The read is unchanged, so the golden test still matches.
- KRP-KR and KRPP-KRP: the seeded 2,000,000-position random sample,
  10 to 14 s each.
- KR-KN and KR-KB: exhaustive under symmetry.
- The splits are python-chess scripts kept beside the dumps
  (`rook_lab*.py`, `krpp_lab.py`, `krkn_lab.py`). Squares are taken from the
  pawn side's view, as `krpkr_scale` reads them.

## 2. V3, KRP-KR's draw rules

- **Exact with the defender to move.** Rule 1 (Philidor), rule 5 and the
  scales under the floor have 0, 0 and 12 wrong in 21,690, 783 and 57,739
  positions with the defending side to move. Every error but those 12 has
  the attacker to move.
- **The errors with the attacker to move are tactics, mostly skewers.** A
  rook check with the defending king between the checking rook and its own
  rook wins the rook. Excluding that one case gives:

  | Site (sample) | Today | Without skewers | Correct draws given up |
  |---|---|---|---|
  | Rule 1, Philidor | 1.90% (907 of 47,833) | 0.55% | 852 |
  | Rule 5, blockade | 2.66% (107 of 4,020) | 0.70% | 105 |
  | Under the floor | 2.28% (3,008 of 132,242) | 0.88% | 4,376 |

- **What remains** is the attacking king within three squares of the pawn
  with the move: a double attack, or a check that drives the king off the
  queening square. Each further cut gives up ten to forty correct draws
  per error it removes:
  - rule 1 with any near king and the move dropped: 0.01%, giving up
    17,955 of its 46,926 correct draws;
  - the low-pawn branch only, at two squares: 0.32%, giving up 9,191.
- **Why not ship the skewer cut.** Every one of these rules is floored at
  24/64 (`SCALE_FLOOR`, BAS-E54). As the code's own note says, "no rule
  here can ever assert a draw -- it can only discount". So an "error" is a
  won position evaluated at 37.5%, and a depth-8 search resolves 98% to
  100% of them (B.9.3 §2).
  - The cut moves rule 1 to 0.55% and does not bring the floor region
    under the line.
  - It costs a move-generating test in the evaluator.
  - The expected gain is far under the 5 Elo that waits for Phase H.
- **The question for C.7.5.** B.9.3's frozen rule reads a floored discount
  as a verdict. A fairer test for a floored site is the search-resolved
  residual, or the Elo of a fitted grading, not the share of wins the
  floored scale touches.

## 3. V4, the KRPP-KRP table

5.03% wrong in the sample (323 of 6,418). Unlike V3, the defender to move is
wrong too (1.66%). With the attacker to move it is 5.63% without a skewer and
44.7% with one. Excluding skewers leaves 3.8%. The table is indexed by the
leading pawn's rank alone, and the errors are positional, so no tactic test
brings it under 0.5%. A better table is a fit over these positions: C.7.5.

## 4. V6, the pawnless scales: KR-KN and KR-KB

Exhaustive: KR-KN's scale 4 is wrong in 18.9% of 2,401,487 positions, and
KR-KB's in 6.0% of 2,232,559. The tables' wins follow three features (KR-KN;
KR-KB the same, weaker):

| Feature | Won share |
|---|---|
| Knight's distance from its king: 1, 2, 3, 5, 7 | 3.1%, 1.4%, 9.3%, 27%, 59% |
| Rook side to move against not | 29.8% against 10.0% |
| Defending king on the edge, against the centre | 27.5% against 5.5% |
| Defender to move, knight within two, king off the edge | 0.14% to 0.26% |

These are the features classical Stockfish's KRKN evaluator grades: the
defending king pushed to the edge, the knight driven away from it. One number
per class cannot be exact. Narrowing scale 4 to its exact domain (the last
row) would keep about a tenth of the class and leave the rest unscaled, as if
the rook were a clean win, which mis-scores the 81% of draws instead. **The
repair is a position-graded evaluator per class (KR-KN, KR-KB, the KQ-KR
pairs), fitted: C.7.5's own work.**

## 5. For PLAN (placed in C.7.5 by the maintainer, 2026-11-17)

- **C.7.5**, an input: this packet.
  - V3 and V4 are floored discounts, exact for V3 with the defender to move.
    Their tactical errors are mostly skewers.
  - V6 grades by the minor's distance from its king, the edge and the move.
  - The acceptance for floored sites should be the search-resolved residual
    or a gate, not B.9.3's 0.5% share.
- No B.9 change.
