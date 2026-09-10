# HCE maturity audit — Basilisk 1.9.3 against Stockfish `9587eeeb`

**Date:** 2026-08-13. **Question:** is our HCE weak because it is immature?
**Short answer:** it is **structurally mature and functionally behind**. The
gap is not missing features — it is the values assigned to features we already
have, and our own fitting lever is exhausted.

## 1. Where HCE work lives in the plan

Worth stating because it prompted this audit: **5.5 contains no HCE content
work.** Its "static eval" scope is how the *search consumes* evaluation values
— separating raw eval from pruning eval from searched bounds, TT provenance,
qsearch. All HCE feature/weight work is **5.9**.

## 2. Term coverage — near-complete

Comparing Stockfish `9587eeeb`'s evaluation terms against
`src/eval_params.h`, Basilisk has: outposts (incl. reachable), minor behind
pawn, bad bishop, bishop/rook on king ring, rook open/semi/closed/7th/queen-file,
connected rooks, rook behind passer, king protector, queen infiltration, weak
queen protection, restricted pieces, hanging, threats by minor/rook/king,
pawn threats to pieces, threat by pawn push, passed-pawn rank/file/path-safety/
blocker/king-proximity, pawn islands, doubled/isolated/backward/connected/
blocked/weak-unopposed pawns, pawn majority, bishop pair (incl. pawn-count
coupling), material imbalance (quadratic), mobility (per-count one-hot tables),
space, trapped bishop, cornered bishop, tempo, winnable/complexity coupling,
and a full king-safety package — attack units, safety funnel table, safe
checks, weak ring, ring pressure, flank attack/defence, pawnless flank, king
blockers, central king, shelter/storm, no-queen scaling.

**Genuinely absent**, all minor: `BadOutpost`, `BishopXRayPawns`,
`LongDiagonalBishop`, `KnightOnQueen`, `SliderOnQueen`, `TrappedRook`. Our
pawn-threat terms also lack the reference's "safe square" qualifier.

Six small terms cannot plausibly account for **−232.8 Elo** (BAS-O02). Term
coverage is not the problem.

Also checked: every term that shipped "seeded INERT" in Phases 3.x is now
non-zero. Nothing is structurally present but functionally switched off.

## 3. Agreement with the reference — this is where the gap shows

Both evaluators run over `suite_v1.epd` (103 of 107 positions returned a static
score; in-check positions do not):

| Measure | Value |
|---|---:|
| Correlation `r` | **0.790** (r² = 0.624) |
| Regression slope, SF on Basilisk | **1.749** |
| Std-dev, Basilisk / SF | 8.67 / 19.19 pawns |
| Median absolute difference | 0.94 pawns |
| Mean absolute difference | 4.99 pawns |
| **Disagree on the sign** | **17 of 103 = 17%** |

Two readings, both material:

1. **We share only ~62% of variance with the reference.** On roughly one
   position in six we disagree about *which side is better* — not by how much.
2. **Our evaluation is compressed by about 1.75×.** The reference discriminates
   far more sharply between positions. Some of that spread is the tactical and
   endgame positions in the suite where the reference sees decisive material,
   so treat the slope as indicative rather than exact — but the direction is
   consistent and the median difference of 0.94 pawns is robust to those tails.

## 4. Is it immature? Two different answers

**Structurally: no.** The feature set is SF11-class, every term is live, and
the terms are organised through the same funnels (mobility curves, king-danger
accumulation, quadratic imbalance, winnable coupling).

**Functionally: yes, and the reason matters.** The weights came from Texel
tuning on static labels plus on-policy self-play cycles. That process is
**exhausted by our own measurement**: five cycles paid roughly +15 to +21 Elo
each, then cycle 6 washed at **+1.37 ±5.21 over 8,100 games** (BAS-E-series),
and holdout MSE never predicted Elo.

The reference's weights were fitted by **game outcome at fishtest scale** —
millions of games of SPSA against a strength objective. Our fitting optimises a
static objective, which cannot price a term whose value is realised through
search interaction. That is a difference in *method*, not effort, and it is the
most likely explanation for a 17% sign-disagreement rate in a feature-complete
evaluator.

## 5. Consequence for 5.9 — a contradiction to resolve

PLAN scopes 5.9 as **structural feature convergence only**, explicitly barring
another constant refit (HCE cycle 6's verdict stands).

But this audit says the gap is **almost entirely constants**. So 5.9 as written
cannot close the gap it exists to close. Its realistic yield is the six minor
terms in §2 — plausibly a handful of Elo, not the 232.8 the oracle measured.

Three options, stated honestly:

| Option | Assessment |
|---|---|
| Keep 5.9 narrow | Intellectually consistent, small yield. Six terms, each individually gate-able. |
| Widen 5.9 to a game-outcome refit | Contradicts both the HCE freeze and the pre-NNUE SPSA ban, and would fit a surface NNUE then discards. Would need fishtest-scale games we do not have. |
| Shrink 5.9, treat the gap as NNUE's job | NNUE fits by game-derived labels at scale — precisely the capability our Texel process lacks. This gap is the reason NNUE exists in the plan. |

**Recommendation: keep the maintainer's program order, but re-scope 5.9
honestly.** It is worth roughly six small terms, not 233 Elo. Record that the
evaluation gap is NNUE's to close, so no one later reads 5.9's small result as
a failure or is tempted to reopen the constant refit.

Note the knock-on: PLAN justifies 5.9 partly as producing "a better NNUE
teacher". That argument weakens with the same evidence — if our HCE cannot
improve much, neither can the teacher. Phase 7.1's teacher quality may be
better served by search depth and by the Stockfish-distillation route already
recorded as BAS-X02 (+6.75 in Basilisk) than by HCE feature work.

## 6. What this does *not* say

It does not say our evaluation is bad in absolute terms. It beats Rarog's under
an identical search by roughly 96 Elo (BAS-O02 against the Rarog figure), and
BAS-O04 shows evaluation quality accounts for only 4% of our tree-width gap.
Evaluation strength and tree shape are different axes; this audit is about the
first.
