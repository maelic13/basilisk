# Search rewrite, block 3: pruning

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). This block covers the rules that cut a
node or a move without searching it fully: reverse futility pruning, the null move, razoring, futility pruning of
quiet moves, ProbCut, and the pruning of losing captures.

## 3.1 Starting point

Reverse futility and futility pruning already have margins per expert of the network (`RfpExp`, `FutExp`), tuned in
8.0. The other rules use one margin for all four experts.

## 3.2 Directions

- **Margins per expert where they pay.** The confidence in the static evaluation differs between the experts, so the
  margin that is safe in a middlegame expert may be too wide or too narrow in an endgame one. Each rule gets per-expert
  margins only if an SPSA moves them apart and an SPRT confirms the gain.
- **Known ideas to measure.** Razoring with a margin linear in depth, disabled while the search is looking for a mate
  (Stockfish, 2026); an earlier linear form of ours, limited to all nodes and without the mate guard, lost −2.32 ±
  2.52 over 20,032 games. A null-move guard when the side to move has very few legal moves, which the legal generator
  makes cheap to check.
- **ProbCut with an adaptive depth**, tied to the reductions of block 2.

Status: planned, after block 2.

## 3.3 Status (20 October 2026)

Rewritten in a form of our own at the same tree: 60 fixed positions give the same node counts at depth 11, and the
bench is unchanged (209655). The rules before the moves sit in three functions called from one place: `eval_gates`
(razoring and reverse futility), `null_probe` (null move with its verification) and `capture_probe` (ProbCut), followed
by the reduction without a hash move. The rules on each move are `capture_gate` (futility and exchange tests of
captures), `quiet_gate` (history pruning, futility and exchange test of quiet moves) and `skip_move` (move-count
pruning). The numbers that were written in the code are parameters.

A data build (`prunelog.h`, compiled only with `TRIUMV_PRUNELOG`) records which rule decided at each node and move,
and searches a sample of the pruned nodes and skipped moves to see whether the decision lost something. From these
data came the new levers, all off at their neutral value:

- margins per expert for move-count pruning, futility and exchange tests of captures, razoring, the null move and
  ProbCut (`LmpExp`, `CaptFutExp`, `CaptSeeExp`, `RazorExp`, `NmpExp`, `ProbCutExp`), with `PrExpRootDepth` to apply
  them only from a given depth;
- `RfpDisagree`, a wider reverse futility margin where the two parts of the network disagree.

Measured and not adopted:

| Test | Result |
|---|---|
| Less pruning in endgames (expert 0) | −6.8 at 8+0.08 with HT, +0.2 at 10+0.1 without, −1.6 at 20+0.2 with HT |
| More pruning in experts 2 and 3 | +2.6 ± 4.2 over 6,438 games on the Xeon, negative on a Zen 4 |
| `RfpDisagree` 256 | negative, stopped |
| SPSA of 28 pruning parameters (10+0.1, 2,691 iterations), whole vector | −2.83 ± 4.35 over 6,130 games |

The SPSA regression found one parameter out of 28 with a significant gradient, which is what chance gives: the pruning
margins sit on a flat optimum, and the tuned vector only adds the spread of its random walk. The values stay as they
are. The levers stay in the code, off, for later attempts.

Remaining:

1. Idea, not started (`PrLearn`): pruning corrected by search outcomes, on the pattern of `RfLearn` (block 2). The data
   build probes a sample of the moves that the pruning rules skip, at the depth they would have had, and records
   whether they clear alpha; probing the skipped moves themselves avoids a fit that only sees the moves the rules
   kept. A logistic model of that outcome, with the features of the reduction plus the missed futility margin, the
   exchange threshold and the history, gives one lever: where a skipped move is likely to matter, the pruning is
   lifted; where it almost never does, a move the rules would keep is pruned. The centre is chosen at an equal tree
   size, measured on fixed positions. A logistic late-move pruning model labelled by replays exists in arche
   (PR #335, September 2026).
