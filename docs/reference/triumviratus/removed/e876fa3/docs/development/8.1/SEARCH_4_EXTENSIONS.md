# Search rewrite, block 4: extensions

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). This block covers the rules that search
a move deeper than the nominal depth: the singular extension, the low-depth hash-move extension, and the internal
reduction of nodes without a usable hash move.

## 4.1 Direction

The three rules decide the same question from different sides (is this node worth more depth or less?), and today
each has its own conditions and margins. The rewrite gives them one design, so that a node is not extended by one rule
and reduced by another for the same reason.

**Known ideas to measure**, made cheap by the exact legal move count of the legal generator: an extension when the
side to move has a single legal move, with no singular verification search in that case (a single move is singular
by definition), and no check extension when the evasions are many.

Status: planned, after block 3.

## 4.2 Rewritten in our form (19 October 2026)

The singular extension is in our code at the tree of 8.0 (bench unchanged). The hash move is tested for being the only
good move: the other moves are searched at about half depth against a bar below the hash value. Three outcomes: the
hash move is extended, by more plies the further the others stay under the bar; the exclusion search clears beta and
the node returns (several good moves); or the hash move is searched shallower when the node expects a cutoff.

- The thresholds of the previous code are parameters at their old values: `SingMinDepth` 6 (+`SingMinDepthTtPv` on
  hash entries of the principal variation), `SingHashSlack` 3, `SingMarginDiv` 63, `SingDepthMul` (half depth).
- The terms shared by the double and the triple extension are computed once.
- The negative extension is split by its two reasons: `SingNegHigh` (the hash value clears beta) and `SingNegCut`
  (expected cut node), both 3.
- Terms of our own, at 0: `SingExp0`–`3` (margin of the bar per expert of the network, the idea of the per-expert
  margins that gained in 8.0), `SingSurplus` (margin per ply of hash depth beyond what is needed), `SingMoving`
  (margin per 64 units between the hash value and the static evaluation: a node in motion trusts its hash less),
  `SingLvlExp0`–`3` (thresholds of the extra plies per expert).
- Already ours: `Ldse` (extension of the hash move near the horizon when the evaluation is below alpha) and the cap on
  extensions from negative reductions along a line.

## 4.3 Tuning and bake (19 October 2026)

An SPSA of 30 levers (the inherited values and the terms of our own) ran at 10+0.1 without hyperthreading for about
1,450 iterations. The vector was taken whole, with the levers that kept oscillating around their start left out, and
the six of those that come from Stockfish 19 take the small move the SPSA gave them. Against the previous values,
pooled at 10+0.1 without HT and 8+0.08 with HT: **+0.22 ± 2.89 Elo over 14,266 games**, level; baked (bench 209655).

Of the terms of our own, five are on: the margin of the bar per expert (`SingExp1` −1, `SingExp2` 3), the distance
between hash value and static evaluation (`SingMoving` 2) and the thresholds of the extra plies per expert
(`SingLvlExp0` 1, `SingLvlExp1` 1). A regression of the SPSA results on its perturbations found no lever with an effect
beyond noise: the block sits near a flat optimum, and the tuning moved its values without changing its strength.

## 4.4 The extra plies as a ladder (19 October 2026)

The extra plies of the singular extension were decided by two separate thresholds, one for the second ply and one for
the third, each a weighted sum of the same node facts. They are now a ladder on the gap under the bar: a first rung
(the threshold of the second ply, with an offset per expert) and a step with its own weights of the same facts
(`SingStepBase`, `SingStepPv`, `SingStepQuiet`, `SingStepTtPv`, `SingStepDeep`, and the hash-move history with the
opposite sign). A move earns one extra ply per rung it clears, up to `SingRungs`. The divisor of the hash-move history
became a power of two (2^17, multiplier 1371). The start values give the tree of §4.3: bench unchanged, 209655.

Two degrees of freedom of the ladder were measured at strong values (8+0.08 with hyperthreading, UHO book):

| change | result | decision |
|---|---|---|
| step shorter with depth, `SingStepDepth` −32 | −3.05 ± 5.53 (4,106 games) | discarded |
| step longer with depth, `SingStepDepth` +32 | −5.49 ± 8.18 (1,836) | discarded |
| a third rung, `SingRungs` 3 | −13.61 ± 9.47 (1,456) | discarded |
| negative extension on an expected cut node 2 instead of 3 | −1.98 ± 4.51 (6,322) | 3 stays |

A step constant with depth and two rungs stay. The remaining inherited numbers are the generic gates of the rule: the
negative extension on an expected cut node (3), the depth from which the exclusion search runs (6, one more on hash
entries of the principal variation) and the slack on the hash depth (3). Every other value of the block comes from our
tuning or from terms of our own.
