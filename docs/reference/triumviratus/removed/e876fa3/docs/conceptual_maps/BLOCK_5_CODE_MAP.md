# Block 5 code map: threads, quiescence, hash cutoff

How the core of the search of Triumviratus 8.1 works in the code: how a search starts and ends, what each thread
does, what the threads share, how the move is chosen at the end (sections 1-4), the quiescence search (section 5) and
the cutoff from the transposition table (section 6). The design and the tests are in the
[block 5 document](../development/8.1/SEARCH_5_CORE.md).

Files: [`source/search/15_threads.inc`](../../source/search/15_threads.inc) (start, stop, vote, bestmove),
[`14_deepen.inc`](../../source/search/14_deepen.inc) (iterative deepening of each thread),
[`12_quiesce.inc`](../../source/search/12_quiesce.inc) (quiescence), [`13_search.inc`](../../source/search/13_search.inc)
(hash cutoff), parameters in [`01_params.inc`](../../source/search/01_params.inc).

## 1. A search from `go` to `bestmove`

```mermaid
flowchart TD
  GO["UCI go<br/>launch_search"] --> TB{"root position<br/>in the tablebases?"}
  TB -->|yes| TBM["bestmove from the tablebase<br/>(no search)"]
  TB -->|no| MW["master worker runs<br/>search_position_mt"]
  MW --> PREP["copy the board to every thread<br/>collect the legal root moves<br/>reset the root lines"]
  PREP --> H["helpers 1 … n−1<br/>thread_main → deepen"]
  PREP --> M["thread 0<br/>deepen"]
  M --> STOP["time over, depth reached or stop<br/>(go infinite and ponder wait for stop or ponderhit)"]
  H -.->|"stopped by the same flag"| STOP
  STOP --> WAIT["wait_for_threads"]
  WAIT --> VOTE["pick_thread: the vote"]
  VOTE --> OUT["bestmove + ponder move<br/>expected reply for the time manager"]
```

The threads live for the whole session: each one is created once and waits on a condition variable for its next
search (`SearchWorker`), so a `go` costs no thread creation. The UCI loop stays free during the search for `stop`,
`ponderhit` and `isready`.

## 2. What each thread does

Every thread runs the same iterative deepening (`deepen`) on the same root. Lazy SMP needs no work splitting: the
threads help each other through the shared tables, and they explore different parts of the tree because they reach
the same nodes at different moments and with different bounds.

```text
deepen(td)                                                           (14_deepen.inc)
├─ SmpSkew > 0 and odd helper → start SmpSkew plies ahead           (off at 0)
└─ for each depth:
     delta = AspBase + (thread id mod SmpAspCycle) × SmpAspStep + spread of the scores / AspMeanSqDiv
     window = [average score − delta, average score + delta]
     search the root at depth − AspFhRed × (fail highs in a row) − …
       fail low  → beta = alpha + AspLoKeep/1024 of the window, alpha = result − delta, delta += AspGrowLo/128 × delta
       fail high → alpha up, beta = result + delta, delta += AspGrowHi/128 × delta
       inside    → done
     thread 0 only: time checks, output to the GUI, best-move changes for the time manager
```

The offset `(thread id mod SmpAspCycle) × SmpAspStep` gives each thread a slightly different aspiration window, so
the threads fail high and low at different moments and fill the shared table with different entries.

## 3. Shared and per-thread data

| Data | Shared | Per thread |
|---|:---:|:---:|
| transposition table | ✓ | |
| pawn-structure history, correction histories | ✓ | |
| quiet, first-ply, continuation and capture histories, hash-move history | | ✓ |
| board, stack of the search, root lines | | ✓ |
| stop flag, clock | ✓ | |

## 4. The vote (`pick_thread`)

When the search stops, every thread has a best line. The move comes from the thread with the strongest support.

```text
share(thread)   = (score − lowest score among the threads + SmpVoteBase) × SmpVoteScore / 1024
                  + SmpVoteDepth × completed depth
support(move)   = Σ share(thread) over the threads whose best move is that move

order of preference, thread by thread:
  1. a decided and complete result (mate or tablebase score, without a bound); the larger one wins
  2. otherwise, a result that is not lost, with more support
  3. equal support → the longer line
```

| Parameter | Start | What it changes |
|---|---:|---|
| `SmpVoteBase` | 14 | support of a thread with the lowest score |
| `SmpVoteScore` | 1024 | weight of the score in the support |
| `SmpVoteDepth` | 0 | support per completed ply, so a deeper thread counts more |
| `SmpAspCycle`, `SmpAspStep` | 8, 1 | offset of the aspiration window of each thread |
| `SmpSkew` | 0 | plies by which the odd helper threads start ahead |

At the starting values the behaviour is the one before the rewrite. With one thread nothing of this section acts:
the bench is identical (137419).

## 5. Quiescence (`12_quiesce.inc`, `quiesce`)

```mermaid
flowchart TD
  IN["quiesce(alpha, beta)"] --> EX{"1. early exits<br/>repetition ahead, max ply,<br/>fifty moves, insufficient material,<br/>hash entry that settles beta"}
  EX -->|exit| RET["return"]
  EX --> CHK{"in check?"}
  CHK -->|yes| WALK
  CHK -->|no| ST["2. the stand<br/>corrected evaluation (or a hash bound on the right side)"]
  ST -->|"stand >= beta"| PULL1["qs_pull(stand, beta, QsFhMix)<br/>return"]
  ST --> FB["alpha = max(alpha, stand)<br/>futility base = eval + QsFut[expert]"]
  FB --> WALK["3. the walk: tactical moves<br/>(all evasions in check)"]
  WALK --> W{"qs_worth"}
  W -->|skip| WALK
  W -->|search| REC["step_in, -quiesce(child), step_out<br/>update best, alpha; beta cut ends the walk"]
  REC --> WALK
  WALK -->|no more moves| CL["4. the close<br/>mate if in check and nothing played;<br/>stalemate check when possible;<br/>qs_pull(best, beta, QsStandMix); hash entry"]
```

```text
qs_worth(move)                                                       (12_quiesce.inc)
├─ being mated → search everything
├─ plain capture (no check, no promotion, not a recapture, futility base known):
│    ├─ more than QsGate moves seen        → skip
│    ├─ base + victim × QsVictimW/1024 <= alpha → skip, best = max(best, that value)
│    └─ SEE < (alpha − base) × QsSeeFutW/1024  → skip, best = max(best, min(alpha, base))
├─ quiet move (an evasion)                 → skip
└─ search only if SEE >= −QsSee[expert]
```

## 6. Cutoff from the transposition table (`13_search.inc`, `hash_cutoff`)

```text
hash_cutoff(node)
1. deep enough?   entry depth > node depth − (value <= beta ? HashLowSlack : 0)        else no cutoff
   (HashNearMiss, off: an entry one ply short below alpha by a margin)
2. node type      bound settles beta, and (node expects that side or depth > HashAnyNode)
                  otherwise: a deep bound against the window loses a ply if depth > HashWeaken → no cutoff
3. learning       quiet hash move failing high: reward min(HashCutQuietMul × depth, HashCutQuietMax)
                  previous move, if it came before move HashPrevLate: penalty HashCutPrevMalus
4. guards         fifty-move count >= HashFiftyGuard → no cutoff
                  depth >= HashVerify: hash_child_agrees (the child's entry must agree on the side of beta)
5. value          lower bound pulled toward beta: (HashDamp × v + 10 × beta) / (HashDamp + 10)
```

At the starting values both parts give the tree before the rewrite (bench 126352).
