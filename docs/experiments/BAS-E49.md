# BAS-E49

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E49 - the current head on the corrected 6.0.b cohort, and an unexpected
KBNK/KBP-K interaction** (2026-09-03). Same frozen 770-position cohort, same
registered conditions, fixed 60,000 nodes so build speed cannot matter. Only
`kbnk_score` differs between the two Basilisk arms: `git log 294a3e2..HEAD --
src/` contains nothing else behavioural, the remainder being TUNE-only options
and the 6.0.g diagnostic counters.

| arm | converted | vs reference |
|---|---:|---:|
| pre-6.1 head `294a3e2` | 361/480 (75.21%) | -105 |
| **current head** | **377/480 (78.54%)** | **-89** |
| Stockfish `dev-20260716-ebcea3ef` | 466/480 (97.08%) | - |

6.1 is worth +16 positions in this frame: KBN-K +8 (12 to 20), **KBP-K +8 (9 to
17)**, KBP-KN +1, KBP-KB -1.

**KBP-K was never targeted, and the effect is real.** Isolated on one binary
with only the option changed: legacy vector `15600,800,900,220,220` gives 9/24,
the accepted `15600,1900,0,460,0` gives 17/24, and the old PGO binary
independently reproduces 9/24. So the KBNK coefficients, not the build, move a
bishop-and-pawn family.

*The pathway is deduction, not conjecture.* KBP-K contains no knight, and
`kbnk_score` fires only on exactly one bishop and one knight against a bare
king. Therefore every KBN-K node inside a KBP-K search tree arises from a
**knight promotion**, and the drive's magnitude -- up to 31,660, against 26,760
for the legacy vector -- reprices those lines. What is not established is which
specific lines flip; root scores stay near 1,000, so KBNK nodes are not
dominating the backup, and the effect must be reaching move ordering or pruning
rather than the root evaluation.

*Consequences.* First, BAS-E43's finding that KBNK never occurs in trees from
real roots stands, but is narrower than it sounded: it excluded endgame roots by
construction, and KBNK nodes plainly do occur in four-man endgame trees. Second,
the KBP-K deficit routed into 6.8.c is 7 positions at the current head, not the
15 the stale pre-6.1 arm showed. Third, this is precisely 6.4.a's subject --
score resolution, saturation and interaction at Basilisk's scale -- and it now
has a concrete, reproducible instance to audit rather than a hypothetical one.
