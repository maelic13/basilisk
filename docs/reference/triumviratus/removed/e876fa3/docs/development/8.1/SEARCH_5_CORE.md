# Search rewrite, block 5: the core of the search

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). This block covers the parts of the search
that the first four blocks do not touch: quiescence search, the use of the transposition table (probe, cutoffs,
store), iterative deepening with aspiration windows, and the threads of the parallel search.

## 5.1 Scope

- **Quiescence**: stand-pat, the choice of captures and checks, the pruning of losing captures, and the value
  returned on a cutoff.
- **Transposition table use**: when a stored entry ends the search of a node, how its value and depth are trusted,
  and what is stored on exit. The table itself (layout, replacement, prefetch) is already our own and stays.
- **Iterative deepening and aspiration windows**: the window around the previous score, how it widens after a fail,
  and the root move order between iterations.
- **Threads**: the shared table, the helper threads and the vote that picks the move. Scaling with 4 and 8 threads
  matters for the 4CPU lists and is measured on its own.

## 5.2 Method

The four steps of block 1 ([section 1.1](SEARCH_1_ORDERING.md#11-method)). This block holds mostly logic and few
numbers, and the numbers it has were already moved by the tuning of 8.0, so step 1 (our form, identical tree) is
enough for most of it; new terms, such as a stand-pat margin per expert, start off and get their own SPRT. With one
thread the check is an identical bench. With more threads the search is not deterministic, so a change of behaviour
is checked by an SPRT at 4 threads and by the scaling of the speed.

## 5.3 Threads (rewritten, 17 October 2026)

The parallel search is a lazy SMP, the common scheme of most engines: every thread searches the same root with its
own move ordering statistics, and the threads share the transposition table, the pawn-structure history and the
correction histories. The thread infrastructure was already our own (threads that live for the whole session and
wait on a condition variable, 12 October). Rewritten now, at unchanged behaviour:

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `SmpVoteBase`, `SmpVoteScore` | the support a thread gives its move: (score − lowest score among the threads + base) × weight | 14, 1024 | formula: the support of a move is computed once, the weight is a parameter |
| `SmpVoteDepth` | support added per completed ply, so a deeper thread counts more | 0 | formula: a new term, additive |
| `SmpAspCycle`, `SmpAspStep` | the aspiration window of each thread is offset by (thread id mod cycle) × step, so the threads search slightly different windows | 8, 1 | split |
| `SmpSkew` | the odd helper threads start this many plies ahead, so the threads finish their iterations at different times | 0 | a known variant of lazy SMP, off |

A decided and complete result (a mate or a tablebase score without a bound) still wins the vote, and equal support
goes to the longer line. With one thread the bench is identical (137419). Both new terms are known ideas in a form of
our own (staggered helper threads and a depth-weighted vote appear in other engines), so they do not enter the
register of novelties. How the code works is in the [block 5 code map](../../conceptual_maps/BLOCK_5_CODE_MAP.md).

**Test.** The two terms on (`SmpSkew=1`, `SmpVoteDepth=10`) against the defaults, which reproduce the old behaviour;
4 threads per engine, 2+0.05, ten games at a time on each socket:

| Games | Elo |
|---|---:|
| with hyperthreading, ten games at a time per socket: 4,210 | +1.32 ± 5.33 |
| without hyperthreading, five games at a time per socket: 1,438 | +2.90 ± 9.18 |
| **all** 5,648 | **+1.72 ± 4.61** |

No game lost on time. The terms do no harm, and a gain, if there is one, is small at this time control; the test
was stopped before a decision. The rewrite of the threads stays, with the old behaviour at the defaults, and the two
terms stay off: they are tested again at the end of the 8.1 queue, at 8 seconds and at 8 threads (section 5.7).

Status: threads rewritten; new terms off, queued.

## 5.4 Aspiration windows (rewritten, 17 October 2026)

Every iteration searches the root in a narrow window around the average score of the move. A result outside the
window is searched again with a wider one: after a fail low the upper bound comes down and the lower bound goes below
the result; after a fail high the lower bound goes up, the upper bound above the result, and each fail high in a row
takes plies off the next search. Rewritten at an identical tree:

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `AspGrowLo`, `AspGrowHi` | growth of the window after a fail low and after a fail high (one factor served both) | 51, 51 (/128) | formula |
| `AspFhRed` | plies taken off for each fail high in a row | 1 | split |
| `AspLoKeep` | after a fail low, the upper bound keeps this share of the window instead of falling to the lower bound | 0 (/1024) | new, off |

## 5.5 Quiescence (rewritten, 17 October 2026)

At the end of the main search, the quiescence search plays only tactical moves until the position is quiet. A node
now has four parts in our own order: early exits (draws and the hash), the stand (out of check, the corrected
evaluation is a value the side to move can claim without moving), the walk over the tactical moves, and the close
(mate, a stalemate check, the value pulled toward beta, the hash entry). The decision on each move is one function
of its own: a plain capture (no check, no promotion, not a recapture on the square of the previous move) is dropped
when the gate of plain captures is closed, when even winning the victim for free leaves the node under alpha, or when
the exchange cannot close the gap to alpha; a dropped capture can still raise the node's value to what it was sure to
reach. Any capture must then pass an exchange test with a margin. At the starting values the tree is identical.

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `QsFut0`-`QsFut3` | margin over the stand for a capture to be worth searching | 312 each | split by expert |
| `QsSee0`-`QsSee3` | exchange loss a capture may accept | 74 each | split by expert |
| `QsGate` | plain captures searched before the gate closes | 2 | was a fixed number |
| `QsVictimW` | weight of the victim's value in the futility estimate | 1024 (/1024) | formula |
| `QsSeeFutW` | weight of the gap to alpha in the exchange test of a plain capture | 1024 (/1024) | formula |
| `QsFhMix`, `QsStandMix` | how far a value above beta is pulled toward beta, when the stand fails high and when the node closes | 441, 462 (/1024) | already tuned in 8.0 |

## 5.6 Cutoff from the transposition table (rewritten, 17 October 2026)

An entry ends a node when it is deep enough and its bound settles the comparison with beta. The cutoff is now six
steps in our own order: the depth test, the node-type test, the learning on a quiet hash move, the guards (the
fifty-move rule and, at depth, the agreement of the child of the hash move, now a function of its own), the value
returned, and the weakening of a deep entry whose bound contradicts the window. Every fixed number became a lever:

| Parameter | What it does | Start | Change |
|---|---|---:|---|
| `HashLowSlack` | plies of depth an upper bound may lack and still cut | 1 | was a fixed number |
| `HashAnyNode` | depth above which an entry cuts whatever the node type expects | 4 | was a fixed number |
| `HashVerify` | depth from which the child of the hash move must agree with the cutoff | 7 | was a fixed number |
| `HashWeaken` | depth above which an entry against the window loses a ply | 5 | was a fixed number |
| `HashFiftyGuard` | fifty-move count from which no cutoff is taken | 96 | was a fixed number |
| `HashPrevLate` | the previous move is penalized only if it came before this move number | 5 | was a fixed number |
| `HashCutQuietMul`, `HashCutQuietMax`, `HashCutPrevMalus` | reward to a quiet hash move, penalty to the previous move | 112, 695, 2210 | already tuned in 8.0 |
| `HashDamp` | a lower bound that cuts is pulled toward beta (ours, after an idea seen in Coda) | 41 | already tuned in 8.0 |

The table itself (layout, replacement, prefetch, the fix of a race between threads from Bastiii's review) was
already our own.

## 5.7 Tests and tuning of block 5

The rewrite keeps the old behaviour at the starting values, so it needs no test of its own. Every new lever does:

1. **An SPRT before it is switched on**, against the defaults: at one thread for the levers of the single search
   (`AspLoKeep`), at 4 threads for those of the parallel search (`SmpSkew`, `SmpVoteDepth`), then at 8 threads.
2. **An SPSA** for the levers that pass, in the tuning pass of the block; where the right value may change with the
   material on the board (the growth of the aspiration window, the plies taken off after a fail high), the lever is
   split per expert before the run.

| Lever | Block part | SPRT | SPSA |
|---|---|---|---|
| `SmpSkew`, `SmpVoteDepth` | threads | first test level (+1.72 ± 4.61, 5,648 games, 4 threads, 2+0.05); again at the end of the queue, 8 s and 8 threads | if it passes, with `SmpVoteBase`, `SmpVoteScore`, `SmpAspStep` |
| `AspLoKeep` | aspiration | at the end of the queue | if it passes, with `AspGrowLo`, `AspGrowHi`, `AspFhRed`, possibly per expert |

**SPSA of quiescence and hash cutoff.** One run with the 23 levers of sections 5.5 and 5.6, starting from their
current values, after the fine pass of block 1. Quiescence and the cutoff act in every phase of the game, so only
the two margins that most depend on the material are split by expert; the run uses the general opening book. Then a
non-regression SPRT of the block against the engine before the rewrite. The levers of this run that are still moving
at its end join the selected set of the joint SPSA before the 8.1 release (step 4 of the plan).

## 5.8 Status and remaining steps (19 October 2026)

Done: threads, aspiration windows, quiescence and the transposition-table cutoff in our code at the tree of 8.0
(17 October). Still equal to Stockfish 19: 13 values (3 more uncertain) and 5 neutral weights on the old shape.

To do:

1. Long tuning of the block (the SPSA of the block never ran; the coarse runs are discarded).
2. Bake with a long non-regression test.
