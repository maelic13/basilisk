# BAS-E07

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 5. Evaluation and data experiments -->

**BAS-E07 — HCE maturity audit, 2026-08-13** (`analysis/hce_maturity_v1.md`).
Term coverage against Stockfish `9587eeeb` is near-complete: only `BadOutpost`,
`BishopXRayPawns`, `LongDiagonalBishop`, `KnightOnQueen`, `SliderOnQueen` and
`TrappedRook` are genuinely absent, and every term that once shipped seeded-inert
is now non-zero. Both evaluators over `suite_v1.epd` (103 scored positions):
correlation **r = 0.790** (r² 0.624), regression slope SF-on-Basilisk **1.749**,
std-dev 8.67 against 19.19 pawns, median absolute difference 0.94 pawns, and a
**17% sign-disagreement rate** — one position in six where the two evaluators
disagree about which side stands better.

*Conditional lesson.* The −232.8 Elo evaluation gap (BAS-O02) is **not** missing
features; six minor terms cannot carry it. It is the values assigned to features
we already have, and our fitting lever is exhausted — cycle 6 washed at
+1.37 ±5.21 over 8,100 games and holdout MSE never predicted Elo. The reference's
weights were fitted by game outcome at fishtest scale; ours by a static
objective, which cannot price a term whose value is realised through search.
That is a difference in method, not effort.

*Consequence.* Phase 5.9 is scoped as structural convergence with constant
refitting barred, so as written it cannot close a gap that is almost entirely
constants. Its realistic yield is the six terms above. Recorded so that a small
5.9 result is not later misread as failure, and so the barred constant refit is
not reopened on the strength of it. The evaluation gap is NNUE's to close.

*Retry trigger.* Only if a game-outcome fitting capability at scale becomes
available, which NNUE supersedes anyway.
