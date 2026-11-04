# Search rewrite, block 2: reductions

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). This block covers late-move reductions:
how much shallower than the nominal depth each move after the first is searched, and when a reduced move is searched
again at full depth.

## 2.1 The current form

The reduction starts from a base that grows with the logarithm of the depth and of the move number, and about
nineteen terms are added to it one at a time: node type (principal variation, expected cut or all node, a
principal-variation hash entry), improvement of the static evaluation, the size of the evaluation correction, the
statistic score of the move, captures and checks, cutoffs found by the children. Three terms are already our own: a
level per expert of the network, the causal reduction (a quiet move that does not touch the squares of the refutation
of a sibling that failed low is reduced more), and a smaller reduction for passed-pawn pushes in endgames.

Each term was tuned with the others in place, so none can be changed alone without moving the balance of the rest.

## 2.2 Design: three groups

The rewrite gathers the terms into three groups, each with few weights:

1. **How promising the node is.** Principal variation, hash entry, expected cut or all node, improvement and the
   evaluation correction, in one term.
2. **How promising the move is.** In place of the move number, the rank of the move's ordering score among its
   siblings: move ten with a score close to the first is not move ten at the end of a tail of poor moves. With it,
   the threat balance of the move from block 1 and its statistic score. When the legal move generator provides the
   exact number of legal moves, the move number can also be read as a fraction of them (a known idea, proposed by A Tin
   Man together with the legal generator).
3. **The signals of the network.** The level per expert and the causal reduction, already ours. Whether the slope of
   the reduction should also change with the expert is measured here; an earlier try of a per-expert slope alone was
   neutral, so it enters only if the new form gives it a role.

The rank of the ordering score is the part with the most risk and the most potential: it ties the reduction to the
same information that orders the moves, so a better order also means better reductions.

## 2.3 First piece: the rank of the ordering score (`LmrRank`)

**Code.** For a quiet move taken from the sorted list, the move queue now passes its ordering score and the score of
the best quiet move of the node. The base reduction reads an effective move number: the number of the first quiet
move of the node, plus one move for every `LmrRankScale` points of distance from the best score. It is mixed with the
ordinary move number by one weight per expert (`LmrRankMix0` to `LmrRankMix3`, 1024 = effective number only). With
`LmrRank` off the tree is identical.

**Untuned**, starting values set by hand (scale 4000, mix one half for every expert), on top of the factored history
of block 1, without hyperthreading at 8+0.08: **−0.70 ± 5.73 over 3,478 games**, level.

**Tuning, coarse.** An SPSA run on the five parameters (8+0.08, 8 games per iteration). After 300 iterations the mix
of the endgame expert (nine pieces or fewer) fell steadily from 512 toward 430: with few quiet moves and close
ordering scores, the distance says less than the move number. With the coarse vector the package lost
**−4.34 ± 5.67 over 3,602 games** (8+0.08, no hyperthreading), so the vector was set aside.

Status: off. `LmrRank` adds a signal without replacing any inherited formula, so it waits for the rewrite of this
block, where it is tuned again with the other terms at a longer time control and 16 games per iteration: effects of
one or two Elo on the reductions are below what an 8-game iteration at 8 seconds can resolve.

## 2.4 A reduction formula of our own (`RedForm`)

**Rule.** The rewrite of this block does not keep the inherited formula behind new names, even at an identical tree.
A form that gives the same reduction to every move computes the same function, with the same coefficients. So the
block starts from data and a form of its own, and accepts a changed tree from the first step: games prove the non-regression.

**Data.** A build compiled with `TRIUMV_REDLOG` writes, for one move in 256 that reaches the late-move code, the
features of the node and of the move and the reduction the current formula gives it (`redlog.h`). 6,000 positions
taken from our own test games, stratified so that each expert of the network gets 1,500, were searched to one million
nodes each: 20.7 million records (`tools/redlog_collect.py`).

**Form.** In units of 1/1024 ply, with d the depth, m the move number and e the expert of the node:

- *base*: a power law with an amplitude per expert, A[e] · max(d + d0, 0.5)^α · max(m + m0, 0.5)^β, read from two
  tables built from the parameters;
- *trust*: the features that say how far the node can be believed (hash entry on the principal variation, its value
  and depth, expected cut node, hash capture, no hash move, improvement, the opponent worsening, the size of the
  evaluation correction) scale the base by (1024 + trust) / 1024;
- *facts*: the same features also add a fixed amount, and the move adds its own terms: statistic score, check,
  capture, cutoffs of the children, the gap between alpha and the evaluation for quiet moves, passed-pawn push, the
  causal term, move number, window, depth;
- *all nodes*: the whole value grows by a share that fades as 1/√d.

The pruning rules read a lighter value of the same kind: a power law scaled by improvement, plus the hash entry on
the principal variation, the move number, the window and a constant.

**Fit.** Alternating least squares, with a grid over the exponents and the offsets, then rounding to integers and an
emulation of the integer arithmetic of the engine (`tools/redfit_engine.py`). On the reduced moves the new formula
searches 92% of them at the same depth as before and all of them within one ply; the value read by the pruning rules
gives the same depth on 88%. Two lessons from the fit: a factor of trust on every node feature at once reached only
70% (the node facts are mostly additive), and an encoding of the correction size cut at a low ceiling lost a term
worth up to two and a half plies, since the correction reaches millions of internal units.

### Method, step by step

The same method can replace any inherited formula of the search. It has six steps.

1. **Instrument.** A data build (`TRIUMV_REDLOG`) records, for a sample of the decisions, every input the formula
   reads and the value it returns. The record is written at the place where the value is used, so it sees exactly
   what the search sees. The tree of the data build is the tree of the engine (same bench).
2. **Collect on our own positions.** Positions are taken from our own test games, one per piece band per game, in
   equal numbers for the four experts of the network, and searched to a fixed node count with one thread. The
   sample covers every depth the games reach and every phase.
3. **Choose a form of our own.** The form is chosen before the fit and is not the inherited one. Here: a power law
   per expert in place of the product of two logarithms, a trust factor on the base, additive facts, a scale for all
   nodes that fades as 1/√d. A test with the inherited base inside our form gave the same fidelity (80% against 80%),
   so the choice of base costs nothing.
4. **Fit and check the residual.** Least squares on the recorded values, a grid on the shape parameters, and a check
   against an oracle: the inherited structure fitted with the same tools. When our form falls far below the oracle,
   the gap points to a missing input or a wrong encoding. This is how two faults were found: the
   node features were mostly additive, so a pure trust factor reached only 70%; and the correction size had been
   capped at a value that 90% of the nodes exceeded.
5. **Emulate the integers.** The parameters are rounded and the integer arithmetic of the engine (shifts, truncated
   divisions, table lookups) is reproduced in the fit tool, so the reported fidelity is the fidelity of the engine.
   The fidelity is measured on the depth the search actually uses, after its truncation and its clamps.
6. **Test with games, then tune.** The new formula enters behind a switch on the same binary (`RedForm`), and an SPRT
   compares the two forms. Tuning comes after, and a lever enters a package only if its move is larger than noise
   (see below).

### Tests

| Test | Conditions | Games | Elo | Outcome |
|---|---|---|---|---|
| First form (correction size wrongly capped) | 8+0.08, HT | 1,500 | −3.98 ± 9.29 | stopped: fault found in the fit |
| Fitted form | 8+0.08, HT | 4,044 | +1.80 ± 5.54 | level |
| Fitted form + 15 levers moved by a coarse SPSA (459 iterations) | 8+0.08, HT | ~3,000 | −4.07 ± 6.50 | stopped: the moves were noise |
| Fitted form, both tests pooled | 8+0.08 and 12+0.12, HT | 8,276 | −0.67 ± 3.84 | baked on 18 October |
| Fitted form at a long time control | 20+0.2, no HT | 7,422 | −2.43 ± 3.84 | refit |
| Refit on the engine with block 1 reverted (26.5 million moves) | 10+0.1 and 6+0.06, no HT | 4,278 | +0.32 ± 5.20 | level |

### What the coarse tuning taught

The SPSA run on 37 levers (8+0.08 without hyperthreading, 16 games per iteration) moved no lever beyond noise: the
largest net move was 1.4 times what a random walk with the same steps would give. A package built from levers that
moved "steadily" lost about four Elo, as an earlier package of the evaluation scale had. The reason is statistical: the
mean of segments of a pure random walk looks monotonic (rank correlation of 0.85 or more) in 30% of cases, so with
37 levers about 11 pass that test by chance, and baking random moves away from a point that is already good costs
strength. From here a lever enters a package only when its net move is at least 2.5 times the random-walk
expectation, and a run with no such lever leaves the values where they are.

## 2.5 Taken back out, to come back one piece at a time (19 October 2026)

The reduction of our own, the re-search rule of our own (`ResOwn`: the depth of the re-search after a reduced move
fails high, from its margin over alpha against the uncertainty of the node; fit on 9.2 million moves, +0.22 ± 4.42
over 6,238 games against the old margins) and `RfVolatile` were each level or slightly positive against the form they
replaced. Built together into 8.1 and measured against the official 8.0, they lost at every long time control, and the
new build searched 0.35 to 0.6 ply less deep at the same time per move: a wider tree that ate the speed gained
elsewhere. Level tests of single pieces leave room for losses of two or three Elo each, and three of them add up.

So block 2 returns to the tree of 8.0. The three pieces stay in the code as options, off, with their fitted values
(`RedForm`, `ResOwn`, `RfVolatile`), and each comes back alone against the new base, with a test at a long time
control and a check of the depth reached, before the next one is tried. The base itself, the rewritten code of the
other blocks at the tree of 8.0, is +10.97 ± 5.67 Elo against 8.0 (section 3 of `DEVELOPMENT_8.1.md`).

Two lessons of the re-search fit. A grid that contains the inherited form as a special case falls back onto it (with
the margin on value minus best allowed, the best fit was the old rule with 48 in place of 51), so a fit of a rule of
our own leaves that form out. And probes that share the hash table must run in random order: in a fixed order the
deeper probe found the table filled by the shallower ones and looked almost free.

## 2.6 The reduction learned from search outcomes (`RfLearn`, 19 October 2026)

The data build also records what happened to each reduced move, and probes a quarter of the moves that fail low at
the reduced depth with a full-depth search. A logistic model gives, from features the search already has, the
log-odds that a move matters; the reduction moves by `RfLearn`/1024 ply per unit of log-odds away from a centre, at
most two plies.

Three faults were found on the way, each by measurement:

1. **Extensions by accident.** A correction that brought the reduction below zero became an extension of up to two
   plies. Less reduction now stops at zero.
2. **The centre.** With the centre on the median log-odds, the correction reduced less overall: the tree grew by 14%
   at `RfLearn` 256 and 26% at 768, and the build searched shallower in games. The centre is now chosen so that the
   tree keeps its size: geometric mean of the nodes to depth 12 over 400 positions of our games, for several centres.
   The bench is no use for this: on 50 positions a few of them dominate the sum and the count jumps with no order.
3. **The label.** The first model called a move good when its final value went above alpha. Most of those moves
   already fail high at the reduced depth and are searched again anyway, so the model spent nodes on them (−9.65 ±
   10.36 over 1,152 games). The model now predicts the moves the reduction hides: failed low reduced, above alpha at
   full depth (1.7% of the probed moves). With this label the weight of a hash entry on a PV node turns from +1.64 to
   −0.68, and the moves with a 2% to 10% chance of being hidden turn out to be the most reduced of all.

A lesson for every lever that changes the size of the tree: a fit on search data plus a tree-size sweep on fixed
positions finds in minutes a value that an SPSA would look for over hours, if it found it at all.

## 2.7 Status and remaining steps (19 October 2026)

Done:

- [x] Data, method and the reduction formula of our own (`RedForm`), refitted on the engine with block 1 reverted.
- [x] Re-search depth of our own (`ResOwn`), `RfVolatile`: each level or slightly positive alone; taken out together
  with `RedForm` (section 2.5). Block 2 is back at the tree of 8.0.
- [x] `RfLearn` with the hidden-move model (section 2.6).
- [x] Follow-up reward after a re-search and the extra ply of the PV hash move in our form (19 October): the reward
  depends on what the re-search found (`FollowFhBonus` plus `FollowFhRed` per ply of reduction overcome when it
  confirms, `FollowFhMiss` when it refutes); `PvHashDepth`, `PvMinDepth`. Same tree at the starting values.

To do:

1. [x] `RedForm` + `RfLearn` against the formula of 8.0, pooled: −0.51 ± 3.55 Elo over 10,272 games (8+0.08 with HT,
   +0.46 ± 4.24 over 6,872; 2+0.02 on a second machine, −2.45 ± 6.42 over 3,400): parity. Tree size at depth 16:
   0.98 of the formula alone. Baked on 19 October; the formula of 8.0 and its parameters are removed again (bench
   213149). Still to do: a confirmation at a long time control against 8.0.
2. `ResOwn` on the new base.
3. The reward when the re-search refutes the move: `FollowFhMiss` 0 and `FollowFhRed` above 0, in one test.
4. A form of our own for the full-depth search of the first move (thresholds 5234 and 5487, `LmrNoTTFull`); the fit only
   found the current thresholds again.
5. The weights of the statistic score (2252, 1126, 1093, 873): long tuning.
6. `RfVolatile` back in the queue.

## 2.8 On hold (19 October 2026, midday)

The baked reduction of our own with `RfLearn` lost on a Zen 4 at long time controls against 8.0, with both engines
universal builds: −4.43 ± 7.35 Elo over 1,962 games at 12+0.12 (a development build had shown −5.35 ± 4.37 over 5,520
games at 15+0.15). The rest of 8.1 has the tree of 8.0, so the loss belongs to block 2. Block 2 goes back to the
formula of 8.0 (bench 126352) and stays on hold until a form of our own holds at long time controls on both machines.
`RedForm`, `RfLearn` and `ResOwn` remain in the code as options, off.

A note on the measurements: the Xeon of the tests has AVX-512 without VNNI, so the development builds are compiled
without VNNI. On processors with VNNI (Zen 4 among them) the universal executable of 8.0 picks its VNNI variant, and
every comparison on the Zen 4 before 19 October gave 8.0 that advantage. Comparisons on such processors use the
universal build of 8.1.

## 2.9 The formula of 8.0 in our form (20 October 2026)

With block 2 on hold, the formula in use is now written in a form of our own at the same tree (bench 209655, same node
counts on 60 fixed positions at depth 11): `red_entry_v80` builds the reduction of a move from the terms of the node
and of the move, and `red_final_v80` applies the corrections that depend on what the move does. The constants of the
formula are parameters. `RedForm`, `RfLearn` and `ResOwn` stay as options, off.
