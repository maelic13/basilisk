# BAS-E45

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E45 - the 6.1 mechanism transfers across a 10x node budget, but the
6.1.e rejection does not** (2026-09-03). The same three arms and the same 198
frozen positions at 200,000 and 600,000 nodes per move, against the existing
60,000-node result. Held-out rows 61-198:

| budget | legacy | `1750,340` | `1900,460` | verdicts |
|---:|---:|---:|---:|---|
| 60,000 | 69/138 | 103/138, live 1 | 98/138 | 1750 **REJECTED**, 1900 confirmed |
| 200,000 | 84/138 | 116/138, live 0 | 119/138 | both confirmed |
| 600,000 | 109/138 | 133/138, live 0 | 132/138 | both confirmed |

**Two findings, and the second is a correction to a closed leaf.**

*The mechanism is robust.* Both candidates beat legacy at every budget with
paired z from +4.13 to +5.00, and the gain is not a shallow-search artifact: at
600,000 nodes legacy still converts only 109/138 while the candidates convert
133 and 132, with fifty-move draws at 22 against 4 and 5. Diagonal dominance is
real technique, not compensation for a weak search.

*The 6.1.e rejection was a 60,000-node artifact.* `15600,1750,0,340,0` was
rejected on a single live truth discard -- 2.Nc2 on KBNK0061 allowing 2...Kd1
to fork bishop and knight. At 200,000 and 600,000 nodes that arm has **no live
discard at all**: the fork is a two-ply tactic the search sees once it has a
game-representative budget. The verdict rule was applied correctly to the data
it had, but the data was taken at a budget the engine never plays at, so the
stated reason for preferring `1900,460` does not hold where it matters.

*The shipped vector still needs no change*, which is luck rather than
vindication. The two candidates are statistically indistinguishable at every
budget -- 116 against 119, then 133 against 132 -- so the plateau BAS-E39 found
persists, and `1900,460` remains defensible on the secondary criteria it won at
60,000 (8 discarded clean wins against 15). What changes is the strength of the
justification, not the choice.

*Consequence for the 6.1.f anchor.* `KBNK0061` at 60,000 nodes is now known to
be a low-budget canary. It still guards the historical failure and still fires
under the rejected vector, but it must not be read as evidence of game-level
safety, because at game budgets neither vector fails it.
