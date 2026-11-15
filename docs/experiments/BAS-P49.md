# BAS-P49

**BAS-P49 — quiet scoring's rows hoisted per piece type (B.8.9, item B5)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** `score_quiets` keeps, per piece type, the pawn-history row and the five continuation rows of the type's piece, found at its first quiet (locals of the scoring pass rather than picker members: the pass is the rows' one reader); the same entries enter the same sums in the same order; the verify build agrees on both fingerprints (`671c79e`).
- **Prediction (packet §8, frozen).** Bench +0.2% [−0.3, +0.7] (80%); P(kept) 0.25.
- **Arms.** `b89-b3` (the head before B5, with B1, B2 and B3, B4 reverted: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted, and the items kept since) against `b89-b5`, four
  final-PGO builds of the head with B5, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** Kept alone when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2). Otherwise, as an item that only removes, hoists or reorders work, it is kept under the bar when its step-1 estimate is −0.1% or above, and it then stands or falls with the sub-bar bundle's read (+0.5% or more with the lower bound above 0, eight builds an arm, six cycles). Below −0.1% at step 1 it is reverted.

#### RESULT

Idle host, 2026-11-15. Exactness before the read as BAS-P47's. Archive
`tools/results/b89-b5-bench`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| step 1 | −1.41% | [−1.60%, −1.22%] | NO_CHANGE |

- **Disposition: REVERTED**, below D-17's −0.1% floor at step 1.
- Prediction +0.2% [−0.3, +0.7]: missed low; Rarog read the same idea at −0.59%.
  The row pointers are 42 words a pass held across the loop, where the compiler
  had kept the five row bases in registers and indexed them per move.
