# Search rewrite, block 1: histories and move ordering

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). This block covers the statistics the
search keeps about moves (quiet history, continuation history, pawn history, capture history, the correction
histories of the evaluation and the hash-move history) and the order in which the move loop tries the moves. It comes
first because the later blocks read these statistics: the reductions use the history of a move to decide how much to
reduce it, and pruning uses it to decide which moves to skip. How the code works, with the call tree of every update,
is in the [block 1 code map](../../conceptual_maps/BLOCK_1_CODE_MAP.md).

## 1.1 Method

Every part of the block goes through four steps.

1. **Rewrite in our form, identical tree.** New code with a structure built for the network, which has four experts
   chosen by material: where the old code had one constant, the new one has a weight for each expert. The starting
   values reproduce the old behaviour exactly, checked by an identical bench, and the replaced code is removed. The
   origin of every piece is declared.
2. **Formulas for the new structure, our ideas on top.** The formulas are generalized (a gravity in the saturating
   update, a quadratic term in the amounts by depth, separate weights where one weight served two cases) and ideas
   of our own are added. Every new term starts at the value that leaves the tree unchanged, so it enters the engine
   only through the tuning.
3. **Tune one expert at a time** (section 1.6).
4. **Non-regression SPRT of the whole block.** If it holds, the formulas and the values are ours and the block is
   closed.

A technique that is new, beyond a generalization of the old code, also gets an SPRT of its own (on against off)
before it enters the register of novelties, because the SPRT of the block measures the package as a whole.

## 1.2 A quiet history indexed by threat context

The same quiet move means different things when the piece is escaping a threat, when it walks into one, or when
neither square is threatened. Indexing the history of quiet moves by threats is a known idea, used by other engines
(Ethereal among them).

**Our definition.** Each quiet move gets a two-bit context: the first bit says whether its from-square is
threatened, the second whether its to-square is. A square is threatened for a piece when an enemy piece of lower
value attacks it, the attack that costs material even if the square is defended: for a pawn, an enemy pawn; for a
knight or a bishop, a pawn; for a rook, a pawn or a minor piece; for a queen, also a rook. The king has no context.
The threat maps are the ones the move orderer already builds, so the context costs one lookup per move.

The first version gave each context its own history table; a second one used the common definition (a square
attacked by any enemy piece), for comparison.

| Version | Time control | Games | Elo | Decision |
|---|---|---:|---:|---|
| separate tables, our definition | 8+0.08, hyperthreading | 33,510 | −3.32 ± 1.93 | H0 |
| separate tables, our definition | 15+0.15 | 2,892 | +0.48 ± 6.37 | stopped, level |
| separate tables, any attack | 10+0.1, hyperthreading | 1,804 | −1.16 ± 8.27 | stopped, undecided |

**Why it lost.** The orderer already had a fixed threat term built on the same maps, so the context repeated
information it had; the updates of one move were divided among four tables, and the three threatened contexts, being
rare, learned from few examples while the shared table lost exactly those updates; and the history with context also
entered the reductions and the history pruning, whose parameters were tuned on a single table. Both versions are
removed from the engine.

## 1.3 The factored history

**Design**, our own, built on the common pattern of summed history tables. The single table receives every update.
Three small correction tables, one per threatened context, learn only how much better or worse the move does in that
context than its overall history says. The orderer reads the sum of the two; the reductions and the history pruning
read the single table only, so their parameters keep their meaning. Two parameters weigh the correction:
`ThreatCtxW`, its weight in the ordering score, and `ThreatCtxUpd`, the scale of its update (1024 = 1.0).

| Test | Time control | Games | Elo |
|---|---|---:|---:|
| hand-set weights | 8+0.08 | 6,212 | −3.69 ± 4.41 |
| after a short coarse SPSA (`ThreatCtxW` 1156, `ThreatCtxUpd` 1083, fixed threat term 20 → 25) | 8 to 9 s | 6,626 | −0.89 ± 4.25 |

Level after tuning: the factored history is on in every tuning run of the block, and its final decision comes with
the SPRT of the whole block. Its two weights get a longer run of their own (section 1.7).

## 1.4 The threat balance (off)

One term in place of three, the threat balance of a quiet move in units of material: value saved (the piece leaves a
threatened square) minus value exposed (it enters one) plus a share of the value of the best target it attacks from a
safe square, with one weight per expert. With the share set by hand the package lost **−17.28 ± 12.73 over 684
games**, so the balance is off. It comes back only through a tuning run that starts from the old terms.

## 1.5 Sub-blocks and parameters

The block is rewritten in full. At the starting values the search tree is identical to the engine before the
rewrite (bench 137419, and 270429 with the factored history on), checked after every step; every replaced formula
and option is removed from the engine.

The parameters below exist once per expert (suffix 0 to 3: 0 = 9 pieces or fewer, 1 = 10 to 15, 2 = 16 to 23,
3 = 24 or more). The last column says what each one changes:

- **formula**: the formula itself is new or more general; at the starting value it gives the old result;
- **split**: one value of the old code became one value per expert, the formula is the same;
- **new**: a technique of our own, off at the start; it gets its own SPRT (section 1.1).

**A. Reading the histories.** The score of a quiet move in the ordering is a weighted sum of its histories.

| Parameter | What it weighs | Start | Change |
|---|---|---:|---|
| `HmMain` | main history (with its threat-context correction) | 2048 | split |
| `HmPawn` | pawn-structure history | 2048 | split |
| `HmNear` | continuations 1 and 2 plies back | 1024 | split |
| `HmFar` | continuations 3, 4 and 6 plies back | 1024 | split |
| `HmLow` | history of the first plies from the root | 1024 | split |
| `HmAgree` | when the two nearest continuations agree in sign, a share of the smaller one is added | 0 | new |
| `LpDecay` | how fast the weight of the first-ply history falls with the ply (it was fixed at 1/(1+ply)) | 1024 | formula |

**B. Fixed terms of the ordering.** Bonuses that do not come from statistics.

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `ChkBonus` | bonus for a quiet move that gives a direct check | 13323 | split |
| `ChkSee` | the exchange loss a checking move may accept | −75 | split |
| `ChkDisc` | bonus for a quiet move that discovers a check | 0 | new |
| `ThrEsc` | a piece leaves a square attacked by a lower-value piece (one weight served both cases) | 20 | formula |
| `ThrEnt` | a piece enters such a square | 20 | formula |
| `ThrSafe` | in addition, the threatened piece goes to a square no enemy piece attacks | 0 | new |

**C. Captures and check evasions.**

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `HtCapt`, `HtVict` | capture history and value of the victim in the score of a capture | 1024, 7168 | split |
| `HeCont` | first continuation against main history in the order of quiet evasions (only the ratio matters, so `HeMain` stays at 1024 as the unit) | 1024 | split |

**D. Groups of the move queue.** The staged queue is a known idea: hash move, captures that hold the exchange, quiet
moves with good statistics, losing captures, the remaining quiet moves.

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `StGood` | how much a good capture score loosens its exchange test (it was a fixed divisor) | 1024 | formula |
| `StQuiet` | boundary between good and bad quiet moves | −15563 | split |
| `StSort` | how many quiet moves are sorted, by depth | 3223 | split |

**E. How much the histories learn** after a cutoff.

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `HlQBonus`, `HlQMalus` | reward of the best quiet move, penalty of the quiet moves tried before it | 860, 1043 | split |
| `HlQDecay` | how the penalty falls along the list of tried moves | 921 | split |
| `HlCBonus`, `HlCMalus` | the same for captures | 1368, 1270 | split |
| `HlDepthQ` | quadratic term in depth of every reward and penalty (they were linear) | 0 | formula |

**F. The saturating update.** A known idea: an entry moves toward a limit and slows near it. One function for every
statistic of the engine, with a gravity: lower, the entry gets closer to the limit; higher, it forgets sooner.

| Parameter | Statistics | Start | Change |
|---|---|---:|---|
| `HlGrav` | quiet histories (main, first plies, pawn, continuations) | 1024 | formula |
| `HlCGrav` | capture history | 1024 | formula |
| `CorrGrav` | correction histories of the evaluation | 1024 | formula |
| `TtmGrav` | hash-move history | 1024 | formula |

**G. The auxiliary updates.**

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `HlAux` | scale of the updates of the first-ply, continuation and pawn histories | 1024 | split |
| `HlFNear`, `HlFFar` | update of the near and far continuations | 1024 | split |
| `ContNear` | fixed reward of the continuation one ply back | 73 | split |
| `ContChk` | how many continuations are updated when the node is in check | 2 | split |
| `HlEval` | history of the opponent's last move from the change of the evaluation | 1024 | split |
| `EvPen` | separate weight for the penalties of that update (one weight served rewards and penalties) | 1024 | formula |

**H. The previous move**, rewarded when no move of the node raised alpha.

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `PrGain` | gain on the scale of the reward | 1024 | split |
| `PrLate` | from which move number the previous move counts as late | 9 | split |
| `PrCapt` | reward of the opponent's capture that led to the node | 892 | split |

**I. The correction histories of the evaluation.**

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `CorrGain` | how much the correction weighs in the evaluation (the tables were already separate by expert, the weight was one) | 1024 | split |
| `CorrDepthQ` | quadratic term in depth of the reward of the corrections (it was linear) | 0 | formula |

**Shared parameters**, one value for all experts: the threat-context correction (`ThreatCtxW`, `ThreatCtxUpd`,
section 1.3), and values already tuned by earlier runs of 8.0 (weights of the continuations by distance and by
agreement, base scales of the auxiliary histories, the terms of the previous-move scale, the weights of the
correction tables, the history limits). They enter the tuning queue only where a trend asks for it.

**In all**, 42 parameters per expert, 41 tuned (`HeMain` is the unit): 11 formulas generalized, 27 values split by
expert, 3 techniques of our own.

## 1.6 Tuning one expert at a time

One SPSA run per expert, on an opening book of that phase of the game: short openings for the expert with all the
pieces (6,996 lines); for the other three, quiet positions taken from games already played as the game enters the
phase, with both engines evaluating between 0.30 and 1.50 pawns, without duplicates (208,390 positions at 21 to 23
pieces, 124,889 at 13 to 15, 39,520 at 7 to 9). Every run: 12+0.12, 16 games per iteration, learning rate 0.045,
hyperthreading on one socket. A coarse pass gives each expert about 600 iterations; a fine pass follows on the levers
whose trends were still open and on the levers added after the coarse pass.

A first check, with only the expert of the opening tuned (coarse pass, 22 levers): **+0.58 ± 5.90 over 3,570 games**
against the engine before the rewrite (10+0.1, hyperthreading, one socket). The rewritten block holds its strength
before most of its tuning.

After the coarse pass of all four experts (expert 3 from rows 452 to 542 of its run, experts 2, 1 and 0 from the mean
of their last 80 iterations, after 751, 860 and 844 iterations): **+0.92 ± 4.96 over 4,926 games** against the engine
before the rewrite (10+0.1, both sockets). At this pace the non-regression test would need about 32,000 games, so it
was stopped and kept as a measurement: the block, in its new form and with its first values, plays at the level of
the old one, and a loss larger than about 4 Elo is excluded. The test that closes the block comes after the fine
pass.

## 1.7 The tuning queue

| # | Run | Levers | Status |
|---|---|---|---|
| 1 | coarse pass, expert 3 (24 pieces or more) | 22 | done, 568 iterations |
| 2 | coarse pass, experts 2, 1 and 0, three runs side by side on one socket | 23 each | done, 751, 860 and 844 iterations |
| 3 | SPRT of the block after the coarse pass, against the engine before the rewrite | | stopped as a measurement, +0.92 ± 4.96 over 4,926 games |
| 4 | threat context: `ThreatCtxW` and `ThreatCtxUpd`, with the leaving and entering weights of the four experts, 8+0.08 | 10 | done, 481 iterations, no clear trend; the mean of the last 150 kept |
| 5 | fine pass, expert 2: its 23 coarse levers from the coarse values with half the step, plus the 14 levers added after the coarse pass | 37 | done, 684 iterations; mean of the last 150 |
| 6 | SPRT of the tuning so far (rows 4 and 5 plus the coarse vectors of experts 3, 1, 0) against the engine before the tuning | | stopped as a measurement, +4.79 ± 6.61 over 2,754 games |
| 7 | short pass, experts 1 and 0, then expert 3 (for expert 3 also its two gravities), each starting from the latest vectors of the others, on both sockets; their purpose is values of our own for every lever of the block | 37 each | expert 1 done (463 iterations, mean of the last 50 adopted; four levers were still moving, so it is checked again in row 8 and in the long retuning); expert 0 running; expert 3 next |
| 7b | the tuned values against the old ones without hyperthreading: level at 8+0.08 (16.9 plies, +0.55 ± 6.52 over 2,546 games); with hyperthreading the shallower socket had gained a few points. The runs of block 1 used full hyperthreading (about 15.5 plies), so a retuning at a longer search comes later, in the joint run before the release | | measured; same test at 4+0.04 without hyperthreading to separate depth from hyperthreading |
| 8 | closing non-regression SPRT of block 1 with all the fine vectors; if it holds, the block is ours and closed | | after 7 |
| 9 | SPRTs of the techniques of our own on their own (agreement term, discovered-check bonus, safe-escape bonus), before they enter the register of novelties | | at the end of the 8.1 queue, at 8 s |
| 9b | the block 1 runs repeated with the corrected protocol and long runs (up to about 8,000 iterations per expert; 8 to 15 levers per run, safe ranges, schedule length equal to the iterations played, no hyperthreading, a gradient test halfway): the first runs had a schedule built for 3,000 iterations and stopped early, so a few levers (`CorrDepthQ`, `CorrGain`) moved away from a broken test point rather than toward a better value | 8 to 15 each | at the end of the 8.1 queue, after the rewrite |
| 10 | before the 8.1 release, when every block is closed: one joint SPSA on a selected set of levers of all the blocks (the ones whose runs kept moving, and the ones that act on the same nodes across blocks), at the release time control and without hyperthreading | about 40 to 60 | at the end, step 4 of the plan |

**New levers of block 1**: the agreement term (`HmAgree`), the discovered-check bonus (`ChkDisc`) and the
safe-escape bonus (`ThrSafe`). The last two start off and stay out of the tuning of the block; the agreement term
was already in the coarse passes. Each gets its own SPRT at the end of the 8.1 queue, at 8 seconds, and only the ones
that pass are tuned and switched on.

**After block 1.** Blocks 2 to 5 follow the same path: rewrite at an identical tree, a coarse pass for each of the
four experts, an SPRT, a fine pass, a final SPRT. Block 6 (the inference of the network) needs no tuning, since its
evaluations are bit-identical by construction. Time management has its own runs at the target time control, then
come the six final runs on the finished structure (step 4 of the plan). In all, about fifty SPSA runs.

## 1.8 Status and remaining steps (19 October 2026)

Done:

- [x] Code of our own at the tree of 8.0 (16 and 17 October): per-expert weights, the saturating update, one function
  for the reward amounts, the staged queue with per-expert thresholds. Bench identical at the starting values.
- [x] Threat-context history, factored (section 1.3): written, off since the revert (`ThreatHist` 0).
- [x] Coarse tuning (17 October): baked, then withdrawn on 18 October because 8.1 lost at long time controls. Measured
  alone on the new base on 19 October: −5.83 ± 8.13 Elo over 2,086 games (4+0.06, HT). Discarded, with every other
  coarse run.

To do:

1. Groups of values that only describe a curve become functions of our own with two or three parameters, fitted to
   give the same numbers: the reward of the previous move (21 values), the continuation distances and multipliers
   (13), the correction updates (11). Same tree; fewer levers for the tuning.
2. Long tuning: SPSA at 15+0.15 or longer without hyperthreading, about 20 levers per run, many iterations; a lever
   moves only when its net move is at least 2.5 times the random-walk expectation.
3. The techniques of our own that are off get their own test: `ThreatHist`, the discovered-check bonus (`ChkDisc`),
   the safe-escape bonus (`ThrSafe`, from 0).
4. Bake with a non-regression test at a long time control.

Until step 2 moves the values, the logic of this block keeps the algorithmic base of Stockfish 19 in our code.
