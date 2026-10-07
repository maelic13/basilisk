# BAS-E24

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E24 — removing the refuted terms; the expected NPS recovery did not
appear** (2026-08-29, candidate `eb717a7033`). Eight terms and their 16
parameters removed, along with the `EVAL_TERM_ACTIVE` guard macro, the
`EVAL_BISHOP_EMPTY` table and the geometry constants that served only them.

*Registry integrity verified rather than assumed:* slots **1,194 → 1,178**
(exactly −16), texel group **1,116 → 1,100**, groups 161 → 145, and
`--verify` **PASS** on 10,000 positions — the trace and the evaluator still
agree exactly, which is the check that would catch a half-removed term.
CTest 12/12, canary unchanged at on=35, bench **20,005,943 → 13,981,020**.

**I predicted removal would recover NPS. Interleaved, both PGO, it measures
−6.70% — slower, not faster.**

| | median NPS |
|---|---:|
| `5912-full` (terms present) | 3,233,900 |
| `5912-slim` (terms removed) | **3,017,099** |

*This is not a clean speed claim in either direction, and should not be quoted
as one.* NPS is nodes per second, and the two engines do not search the same
nodes: the slim tree is 30% smaller to the same depth, so its node
**composition** differs — proportionally more full evaluations, fewer cheap
interior nodes. BAS-M06 already recorded that a single PGO build is
insufficient for a small speed claim, and this is that situation exactly, with
the added confound of a changed tree. What it does establish is that the
"removal buys back throughput" argument is unsupported, so it should not appear
in the case for this change.

*The case for removal is therefore narrower than I first put it:* the terms
were measured inert three times, the fit set 12 of 20 values to zero, and less
evaluation code is worth having for its own sake. Whether it costs strength is
a question only the gate answers.
