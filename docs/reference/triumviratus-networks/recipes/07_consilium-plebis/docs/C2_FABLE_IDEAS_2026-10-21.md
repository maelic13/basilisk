# Consilium Plebis and the search: eleven ideas of our own (21/10/2026)

Research note for the 8.1 search around Consilium Plebis (C2). Written by an analysis agent from the code of
`Triumviratus_8.1/search/*.inc`, `consilium/cn.h`, `consilium_bridge.h`, the C2 documents of 21/10
(`ARCHITECTURE`, `C2_EXPERT_POPULATION`, `C2_PROPOSALS_AFTER_EP19`, `consilium2_elo_points.csv`), the two
reports of today's implementation agents (`<work>\c2r_base\REPORT_A.md`, `REPORT_B.md`) and the closed ideas of
`STATO_PROGETTO.md` §3. No code was changed and no engine was run.

What is excluded by construction: the 14 ideas implemented today (per-expert margins, switching captures, conversion
push, pawn history per expert, pawn-king volatility, null move per expert, asymmetric experts; corrections keyed by
expert, time on a PV switch, conversion term, time near an edge, correction keyed by the pawn-king output, time and
scale per expert), and the closed ideas unless a reason specific to C2 is stated.

Facts used throughout:

- The router follows the opponent's non-pawn material in 99.4% of the cells: `e_us` (the side to move's perspective)
  says what the opponent has, `e_them` says what we have. The pair is one byte in the make's state
  (`dp.phase`, `c2_experts_td`), free at every node.
- The two perspectives use different experts in 32.5% of search evaluations against 15.8% of training entries; e3 is
  used 1.8 times more in the search than it is trained, e7 half as much.
- A switch happens only on non-pawn captures and promotions: 0.108 changes per capture on game lines against 0.262 for
  the piece bands of Consilium (`DESIGN_NEXT_NETWORK` §3.1).
- The pawn-king subnetwork adds 74 units on average to a positional output of about 415; its cache hits 59% of the
  time; its linear skip is small (|skip| < 8 in 83% of nodes).
- Z1 (11/10, Consilium): the jump between neighbouring experts on the same position has mean 0 and standard deviation
  20-42 cp; the neighbour's slope is 0.94-0.99. C2 was not measured yet; its experts share a base per king bucket,
  so a smaller jump is likely, not certain.
- Speed is not the issue (+0.7% / +2.8% cycles per node); the search margins and the time verdict were tuned on
  Consilium's scale (`C2_PROPOSALS_AFTER_EP19`).

## Ranked list

Priority = expected gain x confidence / effort. "Train" marks ideas on the training side (F phase or the EM rounds).

| # | idea | side | expected gain | confidence | effort | priority |
|---|---|---|---|---|---|---|
| 1 | Regime piece values: SEE, ordering and futility values per expert | search | 3-8 | medium | small | high |
| 2 | Search-weighted routing EM | train | 2-6 on the net | medium | small | high |
| 3 | Data weighting by expert pair and lambda per expert (F phase) | train | 3-8 on the net | medium | medium | high |
| 4 | Engine-side routing overrides (class merges, the e3 question) | both | 0-5 | low until the E-step | small | medium |
| 5 | Regime-gated chess-knowledge ordering | search | 1-4 | medium | small | medium |
| 6 | Structure history keyed by the pawn-king signature | search | 1-4 | low-medium | small | medium |
| 7 | Same-regime comparisons (improving, opp_worse, prior-reduction steps) | search | 1-3 | low-medium | small | medium-low |
| 8 | Imbalance-directed selectivity (the side behind by class, the side ahead) | search | 1-5 | low | medium | medium-low |
| 9 | Expert diversity across threads (ensemble at no per-node cost) | search (SMP) | 1-4 at 4-8 threads | low-medium | medium | medium-low |
| 10 | Three internal uncertainty signals, measured on prunelog data before any game | search | 0-3 | low | small to measure | low (gated) |
| 11 | Regime-settled horizon: one ply after a switching capture at depth 0 | search | 0-2 | low | small | low |

Suggested order of the tests (one at a time, user's rule of 11/10): 1 (SPSA then SPRT), 5 (SPSA then SPRT), 6, 7, 4
(after the offline E-step), 8 (after the redlog check), 9, 11; 10 is a data build, not a game test; 2 and 3 go to the
trainer.

---

## 1. Regime piece values (`C2Val*`)

**Mechanism.** The search uses one set of piece values everywhere: `see_piece_values[12]` (`see.h`, 208 / 781 /
825 / 1276 / 2538 for P N B R Q), `unit_value[7]` (`09_history.inc`) in the capture futility (`capture_gate`), the
quiescence futility (`qs_worth`, `QsVictimW`), the capture ordering (`score_tactic`, `HtVict*`) and the stat score of
a capture (`move_loop`, `CaptStatMul`). The network learned a value per piece per regime: in the routing classes a rook
against two minors, the exchange, or a queen against rook and minor have values that depend on the opponent's
remaining material, which is exactly `e_us` for the opponent's pieces and `e_them` for ours.

- Parameters: `C2ValN[e], C2ValB[e], C2ValR[e], C2ValQ[e]` for e = 0..7, offsets per 1024 of the base value
  (default 0 = identical tree). The pawn stays fixed (the unit of the scale).
- The value of a piece is read by the class of its owner: `v[pc] = base[pc] * (1024 + C2Val[type][class(owner)]) /
  1024`, where class(owner of white pieces) = `e_black` (the Black perspective's expert reads White's material) and
  the reverse. The twelve values are written once per make into `td.see_val[12]` (`07_makemove.inc`, next to the
  phase byte) and `see.cpp` reads `td.see_val` instead of the static table; in release builds with every offset at 0
  the compiler keeps the static table (constexpr check as `g_c2_node`).
- `unit_value[vk]` in `capture_gate`, `qs_worth` and `score_tactic` reads the same per-thread table through
  `victim_kind` and the victim's colour (the side not to move).
- Seeds for the SPSA: the mean PSQT value per (piece type, expert) read from the exported net (`wHka` rows carry
  their 8 int32 PSQT values after the weights, `cn.h`), averaged over the squares weighted by the search's square
  frequency, divided by the same figure for the knight. A script over the `.nnue` file (the `c2pop` tools of
  `<work>\c2eng\pop` already read the router section) gives the 32 ratios in an hour; the search values start from
  those ratios scaled to the 781 of the knight, as a starting point for the SPSA, not as a bake.

**Why it should gain.** Every exchange decision in the tree (SEE gates of captures and quiet moves, the futility
of captures, the order of captures) uses values fitted once for all regimes. The network's own values per class are
available and change where the material classes change. It is specific to C2 because the routing makes "the material
class of each side" a first-class quantity of the evaluation and of the make.

**Cost.** Twelve multiplications per make (or lazily per node at the first SEE); the SEE loop reads a thread array
instead of a static one (same code path). Expected under 0.3% of cycles; measure with the xperf kit.

**Risk, closest closed idea.** The SEE thresholds (`StGood`, `QuietSeeMul`, `CaptSeeDepth`) were tuned on the
fixed values; the SPSA must include them or the offsets will absorb their error. Closest closed: END1 (per-band
evaluation scale, no gradient): that scaled the evaluation, this changes the exchange arithmetic; not the same
quantity.

**Test.** SPSA, 32 offsets plus the four SEE thresholds, 8+0.08 without HT, UHO (exchanges are frequent there),
starting from the seeds; then SPRT of the vector to >= 20k games at 10+0.1. An endgame-book SPRT without
adjudication as the second check (e0-e2 are endgame classes).

**Priority.** High: the gain is plausible in every phase, the cost is nil, the effort one day.

## 2. Search-weighted routing EM (trainer, `router_em.py`)

**Mechanism.** The M-step minimises the summed loss difference per (cell, expert) over a sample of training
positions, with a budget on expert changes per capture measured on game transitions (`c2_transitions.npz`). Two
changes:

- Weight every cell's loss difference by `w(cell) = clamp((search share of the cell / training share of the cell)^g,
  0.25, 4)`, g = 1 at first, from the 54 x 54 histograms of `c2pop.py` (the search run must be completed first:
  the `C2_EXPERT_POPULATION` §7 command, about 35 minutes on 10 single-thread processes).
- Measure the budget's capture transitions on search trees (the same `c2pop` run can count, per capture made in the
  tree, the cell before and after), since the quiescence, where most captures live, is what the budget should protect.
- Lower `min_share` from 0.04 to 0.02 so that e3's cells can leave if the E-step says so (option 2 of the population
  document); keep `min_cell_samples` but let the weight raise rare asymmetric cells into the movable set by using the
  weighted count.

**Why it should gain.** The router decides which expert sees which position; the search sees the off-diagonal cells
twice as often as the training stream. A table chosen for the training distribution puts its small gains where the
search does not go. The weight costs nothing at inference: the table is a lookup either way.

**Cost.** One EM round, minutes; the search histogram once.

**Risk.** The rounds so far gained 1e-6 of the loss; the weighted rounds may gain as little. Changing the running
training needs the author's approval and a restart from `last.ckpt` before round 48 or 64. An alternative with no
restart: compute the weighted table offline (`c2_estep_offline.py` on an export, then the weighted M-step) and
export two `.nnue` files from the same checkpoint that differ only in the router section; the experts were trained
under the old table, so this undervalues the new one.

**Test.** The two-file comparison at fixed 100k nodes (same binary, `EvalFile`), UHO and the endgame book, 10k games
each; if the weighted table is at least level there, use it from the next round.

**Priority.** High on the training side: small effort, acts on the quantity (the table) that the whole design rests
on, and the population report shows the current table has not learned anything yet.

## 3. Data weighting by expert pair and lambda per expert (F phase)

**Mechanism.** In the loader, which already computes the cell and expert of each perspective:

- A sampling weight per expert pair (8 x 8 = 64 values, symmetric in the two colours): `w(pair) = clamp(search share
  / training share, 0.5, 2)` normalised to mean 1, from the pooled search histogram. The diagonal pairs stay near 1,
  the off-diagonal pairs rise toward 2 (they are 32.5% of the search against 15.8% of the stream), e3 pairs rise
  (1.77), e7/e7 falls (0.54). Option 6 of the population document proposes a weight per class; the pair form is the
  one that reaches the asymmetric positions, which no per-class weight can.
- A lambda per expert of the side to move: `loss = lambda(e) L_eval + (1 - lambda(e)) L_wdl`, with lambda(e) lower
  for e0-e2 (the endgame classes: conversion and drawishness are outcome questions) and the reference 0.75 for e5-e7.
  Starting proposal: 0.60 / 0.65 / 0.70 / 0.75 / 0.75 / 0.75 / 0.75 / 0.75. The author lifted the fixed-lambda rule
  for the new design on 21/10; this gives the rule a structure instead of one global value.

**Why it should gain.** The trained network is judged by the search, which spends its evaluations in a distribution
the stream does not reproduce; the pair weight closes that gap at the data level, where it is cheapest. The
lambda per expert uses the routing index as the phase indicator the loss never had.

**Cost.** Loader and loss changes in the trainer; an F fine-tune of a few epochs per variant from the P checkpoint.

**Risk.** Weighting changes the data distribution of a running run: F only, never P (population document, option 6).
The lambda schedule interacts with the eval scale per expert (`C2EvalExp*`): calibrate after. Closest closed: none;
the FT1/FT2 fine-tunes of October were about grafts, not weights.

**Test.** Two F fine-tunes from the same checkpoint (with and without), same epochs; SPRT between the two exports at
20+0.2 and at fixed 100k nodes, UHO; the endgame book without adjudication for the lambda part.

**Priority.** High on the training side; it needs the completed search histogram (same as idea 2).

## 4. Engine-side routing overrides (`C2RouteE0..7`, class merges)

**Mechanism.** At load (`consilium_bridge.cpp`, after the `RTB1` section is read into `g_router`), replace every
cell whose expert is e by `C2RouteE[e]` (default e = itself). The make (`dp.phase`), the refresh (`Persp`) and
`c2_switches` all read `g_router`, so the tree stays consistent; the bench changes only when an override is set. A
second form, `C2RouteFile` (a text file of cell overrides), serves later experiments on single cells.

**Why it should gain.** e3 (opponent with a lone queen) has 2.8% of the training entries and 5% of the search
evaluations; the EM cannot move its cells (`min_share`). Whether e3's positions are evaluated better by its own
delta or by e4's (or e2's) is the open question of the population document, to be read from the offline E-step. The
engine test answers the same question with games, which is the judge this project uses for everything else
("SPRT decides"). The same switch tests merging the rare asymmetric cells (the 2,696 cells that never move) into the
expert of their diagonal neighbour.

**Cost.** None at run time.

**Risk.** e4's delta never saw e3's positions; the loss can rise. The E-step number comes first; the game test is
the confirmation. Closest closed: ExpertFix (-27.7 +- 18.2 over 352): that kept the root's expert across material
changes, so positions far from their class were evaluated by the wrong expert along the line; here whole classes
move permanently and consistently, which the network's shared base makes possible.

**Test.** Fixed 100k nodes, same binary, `C2RouteE3=4` then `=2` against the default; UHO plus the endgame book
without adjudication (queen endgames live in e3); 10k games each. Also a build with the override baked for xperf
(fewer rows in cache; not the point, but worth a number).

**Priority.** Medium: trivial to add, informative either way, confidence low until the E-step runs.

## 5. Regime-gated chess-knowledge ordering (`C2Ks*`, `C2Atk*`, `C2Pass*`)

**Mechanism.** The baked ordering terms of PR4 (`KingShield` 5371, `AttackOrder` 5807, `AttackOrderQ` 6371,
`OutpostOrder*`, `PassedPush` 3975, `PromoOrder`, `BishopPairCapt`) and the reduction credit `PassedPushRed` (gated by
`PassedPushNpm` on the total non-pawn material) were tuned as one value across all regimes. Each matters in a regime
that the pair names from the right side:

- the shelter of our king (`KingShield`) matters when the opponent has attacking material: scale by `C2KsExp[e_us]`;
- our attack on their king (`AttackOrder*`, `RdKingAtt`) matters when we have the material: `C2AtkExp[e_them]`;
- passed-pawn pushes (`PassedPush`, `PassedPushRed`) matter when the opponent has little to stop them:
  `C2PassExp[e_us]`, replacing the `PassedPushNpm` test by a class test;
- outposts and the bishop pair: `C2MinorExp[e_them]` (whether we still have the minors that use them).

All scales per 1024, default 1024 (identical tree); read once per node from the pair byte in `queue_start` (as
`q.ex` is today) and in `move_loop` for the reduction credit.

**Why it should gain.** The terms were accepted by SPSA with one value each; the SPSA could not express "only when
the opponent has a queen". The expert of the right perspective is that condition, free. Specific to C2 because the
per-side classes exist only there (the old band mixes both sides and the pawns).

**Cost.** One byte read per node and a multiplication per ordered move: nil.

**Risk.** 32 parameters for the SPSA; the gains are ordering gains (small). Closest closed: `PasserGuard` (-9),
`PasserLmr` (+0.3) were different mechanisms (guards on passer moves), not scales of the ordering terms.

**Test.** SPSA at 8+0.08 without HT on UHO (ordering shows at short TC), 4 groups of 8; then SPRT of the vector
>= 20k at 10+0.1; the passed-pawn group also on the endgame book without adjudication.

**Priority.** Medium: cheap, bounded, likely a few Elo at most.

## 6. Structure history keyed by the pawn-king signature (`PkHistW`)

**Mechanism.** A new move table `g_stats.pk_rows[1 << PkHistBits][12][64]` (int16; 12 bits = 6 MB, 10 bits = 1.5
MB), keyed by `nn_pk_signature(td.nnpos, bb, stm, PkHistBits, compute = 0)` (the signs of the most variable
pawn-king outputs against their medians; -1 on a cache miss = no term, no update). In `score_quiets`: `+ PkHistW *
pk_rows[key][pc][to] / 1024`; in `learn_quiet`: `hist_learn(pk_rows[key][pc][to], bonus * PkHistUpd / 1024, LIM_PAWN,
grav)`. Ordering only; the reductions and the history pruning keep reading the flat tables (the lesson of ThreatHist:
the factored form, main tables untouched).

**Why it should gain.** The pawn history (`pawn_row`, exact pawn key) transfers nothing between structures: every
exact structure learns its own plan from scratch. The signature groups the structures the network reads alike (229
distinct values over the bench), so "the plan for this kind of structure" (a minority attack, a king walk, a
break) is learned once and read in the next game of the same kind. It differs from today's idea 4 (the same exact
key salted by expert: finer, not coarser) and from CorrPkW (corrections, not moves).

**Cost.** One cache lookup per node at `score_quiets` and per quiet learned (59% hits; a miss costs nothing and
teaches nothing). The table is one more memory stream in the ordering; measure with xperf.

**Risk.** Key noise (signs of 12 outputs flip on a king step); dilution of the ordering score. Closest closed:
`CorrKingPawnW` and `CorrShelterW` (corrections with hand-made keys, too fine, -4.7 / -5.6): this key is coarse and
learned, and the table orders instead of correcting.

**Test.** SPRT UHO 8+0.08 without HT, `PkHistW 1024, PkHistUpd 512, PkHistBits 12`, >= 20k; then 10 and 16 bits if
the first is level or better.

**Priority.** Medium.

## 7. Same-regime comparisons (`C2SameRegime`, bits)

**Mechanism.** Several rules compare static evaluations of different plies: `improving = static_eval >
f[-2].static_eval`, `opp_worse = static_eval > -f[-1].static_eval` (`node_evaluate`), the two prior-reduction depth
steps (`PriorRedEvalSum`), and the eval-change learning (`learn_from_eval_change`, already skipped after a capture).
When a switching capture or a promotion sits between the two plies, the two values come from different experts. Each
frame stores its pair byte (`f->c2pair`, one byte from `c2_experts_td` at `node_open`). Bits of `C2SameRegime`:

- 1: `improving` falls back to `f[-4]` when `pair(f) != pair(f-2)` and `pair(f) == pair(f-4)`, else `false`;
  variant 1b: else `true` (the SPRT decides which default);
- 2: `opp_worse` is `false` when `pair(f) != pair(f-1)`;
- 4: the two `PriorRedEvalSum` steps are skipped when the pair changed at -1.

**Why it should gain.** Z1 measured 20-42 cp of noise between neighbouring experts on Consilium; a flag that flips on
that noise feeds `LmrNotImproving`, the halved move-count threshold and the RFP lean. With C2 the regime change is
exact and per perspective, so the guard fires only where the regime did change (23% of capture-reached nodes in the
bench showed a switch).

**Cost.** One byte compare per node.

**Risk and closest closed idea.** This is PhaseEdgeImproving (M study idea 2, 05/10) re-opened: it was stopped at
-14.1 +- 19.4 over 296 games (not a measurement) with the piece bands, where 13% of captures crossed a band and the
trigger also fired on pawn captures. The new reason is the exact per-perspective trigger. Before any game: the
prunelog data build records `improving`, the eval and the probe value; adding the pair bytes to `PruneNodeRec`
shows in one run whether `improving` predicts the probe worse when the pair changed. If it does not, drop the idea.

**Test.** Prunelog check first; then SPRT UHO 8+0.08 without HT, bits 1 then 1b, then 2 and 4, >= 20k each.

**Priority.** Medium-low (cheap, previously unlucky, gated by a data check).

## 8. Imbalance-directed selectivity (`C2Behind*`, `C2Ahead*`)

**Mechanism.** Direction `d = class(e_them) - class(e_us)` = our material class minus theirs, from the pair byte;
"ahead" = d >= 1 and corrected static eval > 0, "behind" = d <= -1 and eval < 0 (an exchange sacrifice with eval >
0 and d < 0 is a third state, "compensation", left alone at first). Levers, all 0:

- `C2BehindCheck` (1/1024 ply): when behind, moves that give check and captures that fail the SEE (sacrifices) are
  reduced less (the counterplay the defender must find; against SF19 we lose the defences, X5);
- `C2BehindLmp` (/1024): when behind, the move-count threshold scaled (the defender must look wider among quiet
  moves too);
- `C2AheadTrade` (SEE margin in `capture_gate`): when ahead, the exchange test of captures that remove the
  opponent's last piece of a type is relaxed (simplification: the conversion push of today's idea 3 orders even
  trades; this keeps them from being pruned);
- `C2AheadNmp`: when ahead, the null-move floor lowered (passing is rarely wrong for the side with more material).

**Why it should gain.** A third of the tree is asymmetric; the side behind and the side ahead need different
selectivity, and the direction is free from the pair. It is specific to C2 because the pair gives the direction by
material class, not by the sign of the evaluation alone (which conflates compensation with material).

**Cost.** One subtraction per node and a few tests per move: nil.

**Risk.** Speculative; the Tal style (-40) biased the root toward sacrifices, which this does not (no value change,
only depth). Before any game: the redlog data of block 2 labels reduced moves that were "hidden" (above alpha at
full depth); split those rates by d and by check / failing-SEE capture. If hidden rates do not differ by d, drop.

**Test.** Redlog check first; SPRT 10+0.1 without HT on UHO (imbalanced openings) and 8moves without adjudication,
one lever at a time, >= 20k.

**Priority.** Medium-low.

## 9. Expert diversity across threads (`C2ThreadDiv`)

**Mechanism.** Each handle gets its own router copy (`Handle::router`, `nn_router_table(handle)`; the make reads it
through `td.nnpos`). Thread t with `t % C2ThreadDiv == C2ThreadDiv - 1` (thread 0 never) uses a shifted table in
which every cell's expert is the expert of the cell reached by removing one minor piece from the opponent's
signature (the regime one trade ahead); a second variant shifts toward more material. The threads then evaluate the
same position with a different but related expert and share the hash table, as they already share it with a
different optimism per thread.

**Why it should gain.** The blend of neighbouring experts gained +5.3 +- 7.9 at fixed nodes and lost at time because
each node paid 15-18% of cycles; an ensemble across threads pays nothing per node, and lazy SMP gains from diversity
(SmpSkew +1.7 +- 4.6). C2 makes it possible because every cell has a neighbour class and the switch is a table
lookup, not a refresh of a different structure.

**Cost.** None per node; one router copy per thread (2.9 KB).

**Risk.** The root vote (`15_threads.inc`, `SmpVote*`) compares scores of threads with different evaluations; the
helper's score may win the vote with a biased value. Exclude the diverse threads from the vote at first. Only SMP
benefits; single-thread play is unchanged (bench identical).

**Test.** 4 threads at 2+0.05 and 8 threads at 4+0.04 (the SmpSkew protocol), UHO, >= 10k games; `C2ThreadDiv 2`
and `4`.

**Priority.** Medium-low (SMP only).

## 10. Three internal uncertainty signals, measured before any game

**Mechanism.** Three quantities the evaluation already has:

- (a) the sum of the two perspectives' PSQT, `us.psqt[bucket] + them.psqt[bucket]` (`cn.cpp` evaluate, the line
  that forms `r.psqt` from the difference): on Consilium it is the net's material bias; on C2 in an asymmetric node
  it is the disagreement of the two experts about the material balance. Export as `thread_local nn_last_psqt_sum`
  like `nn_last_disagree`.
- (b) the number of active first-layer neurons, the length of the NNZ list the sparse product builds (`nnz4` stage):
  a measure of how many learned features fire.
- (c) the L1 norm of the 32 pawn-king outputs, from the cache on a hit.

Step 1: add the three to `PruneNodeRec` and `RedresRec`, run the prunelog data build, and regress |probe - eval| on
them together with `n.disagree` and |signal| (`tools/redfit_engine.py` style). Step 2, only for a quantity that
explains error beyond the known ones: a margin term in `eval_gates` or a reduction lever (`RdX`), or, more promising
given the history, a root-only use in the time verdict (root uncertainty scales the target as TmSurprise does).

**Why.** The three closed uses of an uncertainty measure in the margins (`Disagree*`, `RfpDisagree` -6.6,
`RdDisagree` -4.9) all used |psqt - positional|. These are different quantities, two of them specific to C2 (the
asymmetric PSQT sum, the pawn-king norm), and the data build refutes or supports them in hours without games.

**Cost.** Nil to measure; nil to use.

**Risk.** The family's record is poor; `TMEvalDisagree` was neutral over 40k at the root. Hence gated.

**Test.** Data build first; a game test only for a signal with a measured effect on the error.

**Priority.** Low, gated by the measurement.

## 11. Regime-settled horizon (`C2SwitchExt`)

**Mechanism.** In `move_loop`, after `step_in`: if `c2_last_switched(td)`, `new_depth == 0` (the child would be a
quiescence node), the node is not an all node and `f->switch_path == 0` (once per line, like `negext_path`), set
`new_depth = 1`. Lever 0/1.

**Why.** The child's stand-pat comes from an expert that has never been searched on this line; one ply of real
moves lets at least one reply test the new regime's view before the quiescence closes it. Today's idea 2 reduces such
captures less where they are reduced; this covers the horizon, where there is nothing to reduce.

**Cost.** Tree growth; the once-per-line guard bounds it.

**Risk and closest closed idea.** `TransitionExt` (the capture that leaves only pawns, -18.2 +- 16.4 over 402) and
the lean package of 05/10 (neutral). New reason: C2 switches are per-perspective non-pawn captures (0.108 per
capture against 0.262), so the extension fires on fewer and more meaningful transitions, and the guard did not exist
then.

**Test.** SPRT 10+0.1 without HT on UHO, then 15+0.15 (a depth idea); >= 20k.

**Priority.** Low.

---

## Extensions of today's levers (not counted as ideas; for the same SPSA bundles)

- Idea 1 of agent A reads one side (`C2ExpSide`). A two-sided additive form, `a[e_us] + b[e_them]` (16 values per
  margin instead of 8), reaches the asymmetric nodes without 64 parameters.
- The quiescence margins per C2 expert (`QsFut`, `QsSee`) are missing from agent A's tables; add them to the same
  SPSA.
- `ProbCutMargin` plus a term when the probed capture switches an expert; `SingMoving`-like term `SingSwitch` when
  the hash move is a switching capture (the exclusion search runs in the old regime, the hash move's line in the
  new one).
- `RfLearn` refit with the C2 pair, the switch flag and the pawn-king norm as features: the data build exists
  (`tools/redout_fit.py`).

## Considered and not proposed

- Blend of neighbouring experts, in any per-node form: closed on cost (11/10); idea 9 is its cost-free form.
- The pawn-king subnetwork as a standalone "pawn endgame oracle": its lookup is not cheaper than the incremental
  evaluation (which already includes it), and a pawn endgame still needs the HalfKA view of pawns and kings. Its
  value for the search is as a signal (today's ideas 3, 5 and B3, B5; ideas 6 and 10 here).
- A correction keyed by the expert transition on the path: Z2 (idea 5) found such a correction explains R2 0.02 of
  the jump; today's CorrC2Key mode 2 (pair key) already covers the bias per regime.
- "Exchange planning toward a favourable expert" as a hand-built root rule: the only learned quantity available is
  the pair-keyed correction (CorrC2Key 2), which already pulls the root value toward what the search found in that
  regime; a fixed preference per pair would be a prior with no measured support.
- Root cross-expert disagreement in the time verdict: `TMEvalDisagree` was neutral over 40k with a related signal;
  left inside idea 10 as a gated use.

## Measurements to run before the game tests (no engine process needed on the Xeon while a tournament runs)

1. Complete the search histogram of `C2_EXPERT_POPULATION` §7 (ideas 2, 3), including per-capture cell transitions.
2. The offline E-step on ep25 (idea 4).
3. The PSQT-per-expert script over an export (idea 1 seeds).
4. The prunelog build with the pair bytes and the three signals (ideas 7, 10) and the redlog split by direction
   (idea 8): data builds run on the laptop as well.
