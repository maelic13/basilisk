# Consilium Plebis expert population (21/10/2026)

How often each of the 8 experts of Consilium Plebis is used, in real games, in the training stream and inside the search,
for the exports ep9, ep13, ep16, ep19 and ep22, and how the routing table has changed over the EM rounds (8, 16, 24).
Nothing in the running training was changed.

## Summary

- **No expert is starved of data.** On real position sets the smallest expert is e3 (opponent with a lone queen or
  three minors): 2.8% of the training entries, 3.8% of game positions and 5.0% of search evaluations. The others are
  between 7% and 22%. The 8-position bench (e2-e4 at 0-2%) is not representative.
- **The mismatch is between training and use.** e3 is used in the search 1.8 times as often as it is trained, and e7
  (full opponent material) half as often (11-12% of evaluations against 21% of training). e0-e2 are trained about 20%
  below their search use.
- **The learned routing is still the default table.** 17 of 2,916 cells differ from it after the rounds of epochs 8 and
  16, 19 after round 24, so 99.4% of the cells (98.3% of the training mass) still route by the opponent's material only.
  The rounds gained nothing measurable: +1.6e-6, -2.5e-7 and -1.3e-7 of the loss, against a loss of 3.4-6.4e-3.
- **The budget does not keep e2-e4 small.** e2 and e4 are not small (12-16% everywhere). e3 is small because its class
  is a single value of opponent material (9). The budget of the run is 0.20 changes per capture (not 0.12). It bound
  only in round 8, because the default table started above it (0.209), and it does not bind any more (round 24:
  multiplier 0, crossing 0.196). What freezes the table are two other constraints of the M-step: `min_share` 0.04 is
  above e3's share (2.8%), so no cell can leave e3; and `min_cell_samples` 2000 leaves only 220 cells movable (90.5% of
  the mass), so the other 2,696 cells can never move.
- Verdict: **balanced in coverage, unbalanced in training per use (e3 low, e7 high)**. No action is needed in P now.
  Whether e3 is worse trained than its neighbours needs the offline E-step, which is postponed (section 7).

## 1. Method

The expert of a perspective depends only on its routing cell, sig(own) * 54 + sig(opponent), with
sig = min(N,2)*18 + min(B,2)*6 + min(R,2)*2 + min(Q,1). Each position set is therefore reduced to a 54 x 54 histogram
of (sig(White), sig(Black)), and every routing table is applied to it offline: expert(White) = t[sw * 54 + sb],
expert(Black) = t[sb * 54 + sw]. Shares are over perspective entries (each position counts once for White and once
for Black), as the network uses both accumulators at every evaluation.

| set | what | size |
|---|---|---|
| games | every position from the book exit to the end of the 927 games of the matches c2ep19 vs tm105r (20+0.2 and 100k nodes, `match_moe/sprt_c2ep19_vs_tm105r_20_02_n{0,1}.pgn`, `sprt_sprt_c2ep19_vs_tm105r_10_01_n100k_n{0,1}.pgn`) | 109,366 positions |
| training | the P run's stream read on the VM with the run's skip configuration (all 44 binpacks of `/data/bt4` drawn by size; random skipping 2, soft early skipping 20 with 0.25/0.40/0.60/0.90 at ply 0/6/10/18, pc-y 0.0/0.60/1.0/0.95/0.75, quiet filters on) | 3,276,800 positions |
| search | the routing cell at every network evaluation (`nn_pos_eval`) during 1M-node searches, one thread, Hash 64, from 256 roots drawn from the game positions; one run per export with that export's network | **partial**: about 140 of 256 roots per export, 70-75M evaluations per export, 366.5M in all |

The search was stopped at about 55% because the Xeon was needed. The shares of the five runs agree to within 1-1.5
points, so the partial sample is usable, but the full run is still to do (section 7).

## 2. The expert classes

The default table (the starting point of the learned routing, `DEFAULT_UPPER`) assigns the expert from the opponent's
non-pawn material (N = B = 3, R = 5, Q = 9). The training share of each opponent signature inside its class:

| expert | opponent NPM | opponent signatures (share of training entries, %) |
|---|---|---|
| e0 | 0-3 | B 3.65, none 2.98, N 2.00 |
| e1 | 4-5 | R 7.50 |
| e2 | 6-8 | RB 6.11, RN 4.17, BN 1.28, BB 0.56, NN 0.25 |
| e3 | 9 | Q 2.38, BBN 0.17, BNN 0.12 |
| e4 | 10-13 | RRB 2.84, RBN 2.56, RR 2.37, RRN 2.04, QB 1.33, RBB 0.89, QN 0.86, RNN 0.50 |
| e5 | 14-18 | RRBN 2.80, QRB 2.23, QR 1.82, QRN 1.55, RRBB 0.96, QBN 0.57, RBBN 0.52, RRNN 0.51, 6 more below 0.4 |
| e6 | 19-25 | QRRBN 5.70, QRRB 2.96, QRRN 2.22, QRRBB 1.97, QRBN 1.92, RRBBN 1.25, QRR 1.23, QRRNN 1.16, 7 more below 1 |
| e7 | 26-31 | QRRBBNN 10.03, QRRBBN 6.04, QRRBNN 4.48, QRBBNN 0.39 |

e3 is in practice the lone-queen expert (2.38 of its 2.8 points). Its class cannot be widened without moving another
material value into it, and e7 is dominated by the full set of pieces (NPM 31 alone is 10% of the entries), so classes
of equal frequency are not possible with these atoms.

## 3. Population

### Shares per expert, both perspectives, ep22 table (%)

| set | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| training | 8.8 | 7.4 | 12.2 | **2.8** | 13.3 | 12.0 | 22.2 | 21.2 |
| games | 8.9 | 9.7 | 14.7 | 3.8 | 12.4 | 11.1 | 18.7 | 20.8 |
| search ep22 | 10.8 | 8.8 | 15.7 | 5.0 | 15.2 | 12.7 | 20.3 | 11.4 |
| search, five exports pooled | 10.3 | 8.8 | 15.5 | 5.1 | 14.9 | 13.2 | 20.2 | 12.0 |
| **search / training** | 1.23 | 1.19 | 1.29 | **1.77** | 1.14 | 1.06 | 0.92 | **0.54** |

The tables of ep9 to ep22 differ in at most 4 cells, so on training and game positions their shares agree to within
0.1 point; with the round-24 table e0 loses 0.5 point of training share to e1 and e2. Search, each export with its own
table and network (the root sets overlap but are not identical, see section 1):

| export | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| ep9 | 9.2 | 8.8 | 15.6 | 4.7 | 15.0 | 13.4 | 21.5 | 11.8 |
| ep13 | 10.2 | 9.3 | 16.2 | 4.8 | 14.1 | 13.3 | 20.1 | 12.1 |
| ep16 | 10.5 | 8.4 | 15.8 | 5.4 | 15.1 | 13.1 | 19.8 | 12.0 |
| ep19 | 10.9 | 9.0 | 14.5 | 5.3 | 14.8 | 13.3 | 19.9 | 12.3 |
| ep22 | 10.8 | 8.8 | 15.7 | 5.0 | 15.2 | 12.7 | 20.3 | 11.4 |

The search spends about half as many evaluations as the games in e7: from a full-material root, most evaluated nodes
(quiescence above all) come after at least one exchange.

### Per perspective, ep22 table (%)

| set | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| training, White | 9.0 | 7.5 | 12.2 | 2.8 | 13.2 | 11.9 | 22.1 | 21.2 |
| training, Black | 8.6 | 7.4 | 12.2 | 2.9 | 13.4 | 12.1 | 22.3 | 21.2 |
| games, White | 9.9 | 10.2 | 14.4 | 3.4 | 11.9 | 10.8 | 18.5 | 20.8 |
| games, Black | 7.9 | 9.1 | 15.0 | 4.1 | 12.9 | 11.4 | 18.9 | 20.8 |
| search ep22, White | 11.8 | 9.4 | 16.1 | 4.7 | 14.3 | 12.1 | 20.1 | 11.4 |
| search ep22, Black | 9.9 | 8.3 | 15.2 | 5.3 | 16.1 | 13.3 | 20.6 | 11.4 |

The training stream is symmetric between the two perspectives; the games and the searches are slightly asymmetric
(a sample of 927 games from a book).

### Joint table (White expert in rows, Black expert in columns, % of positions), ep22 table

Training (diagonal 84.2%):

| W \ B | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| e0 | 6.3 | 1.0 | 0.3 | 0.6 | 0.3 | 0.4 | 0.1 | 0.0 |
| e1 | 0.9 | 5.2 | 1.1 | 0.1 | 0.1 | 0.1 | 0.0 | 0.0 |
| e2 | 0.3 | 0.9 | 9.5 | 0.3 | 1.1 | 0.1 | 0.0 | 0.0 |
| e3 | 0.5 | 0.1 | 0.3 | 1.3 | 0.4 | 0.2 | 0.0 | 0.0 |
| e4 | 0.2 | 0.1 | 0.9 | 0.4 | 10.6 | 0.8 | 0.2 | 0.0 |
| e5 | 0.3 | 0.1 | 0.1 | 0.1 | 0.7 | 9.7 | 0.9 | 0.0 |
| e6 | 0.1 | 0.0 | 0.0 | 0.0 | 0.1 | 0.8 | 20.8 | 0.2 |
| e7 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.3 | 20.9 |

Search ep22 (diagonal 67.5%):

| W \ B | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| e0 | 7.1 | 1.5 | 1.0 | 0.8 | 1.0 | 0.2 | 0.1 | 0.0 |
| e1 | 1.1 | 4.6 | 3.0 | 0.2 | 0.3 | 0.2 | 0.0 | 0.0 |
| e2 | 0.7 | 1.9 | 8.5 | 1.3 | 3.2 | 0.4 | 0.0 | 0.0 |
| e3 | 0.4 | 0.1 | 0.1 | 2.4 | 1.3 | 0.4 | 0.1 | 0.0 |
| e4 | 0.4 | 0.2 | 2.2 | 0.3 | 8.8 | 2.0 | 0.4 | 0.0 |
| e5 | 0.1 | 0.1 | 0.3 | 0.2 | 1.1 | 8.1 | 2.1 | 0.0 |
| e6 | 0.0 | 0.0 | 0.1 | 0.0 | 0.4 | 1.9 | 17.2 | 0.5 |
| e7 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.6 | 10.8 |

Games: diagonal 79.1%. Off the diagonal the search has about twice the training's share (32.5% against 15.8%):
positions with a material imbalance, where the two perspectives use different experts, are more frequent inside the
tree than in the data.

### Training received per epoch

With 1e9 positions per epoch and both perspectives counted (perspective entries, in millions):

| table | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| ep22 | 176 | 149 | 244 | **57** | 266 | 240 | 444 | 425 |
| round 24 | 166 | 155 | 248 | **57** | 265 | 237 | 447 | 425 |

e3 receives 7.8 times fewer entries than e6, about 31 billion over the 550 epochs of P. In absolute terms that is a
large amount of data for one HalfKA delta on top of the shared base weights; the open question is how well e3 is
fitted, not whether it gets data (section 7).

## 4. The routing table across the exports and the EM rounds

| table | cells != default | cells = default (%) | training mass in those cells (%) |
|---|---|---|---|
| ep9, ep13 (after round 8) | 17 | 99.42 | 1.34 |
| ep16, ep19, ep22 (after round 16) | 17 | 99.42 | 1.38 |
| round 24 (log; first export with it: ep25) | 19 | 99.35 | 1.74 |

Every cell that left the default is a minority in its opponent-signature column, so "depends only on the opponent"
and "equal to the default" give the same share (the 99.4% of the earlier reading).

Rounds, from `router_em.jsonl` (E-step sample: 4,194,304 perspective entries per round):

| round | loss gain | crossing before | after | multiplier | cells moved | share moved | E-step shares e0..e7 (%) |
|---|---|---|---|---|---|---|---|
| 8 | +1.60e-06 | 0.2092 | 0.1995 | 6.7e-04 | 17 | 1.33% | 8.8 7.5 12.2 2.8 13.3 12.0 22.2 21.3 |
| 16 | -2.51e-07 | 0.1995 | 0.1965 | 2.4e-04 | 4 | 0.31% | 8.8 7.4 12.1 2.9 13.3 12.0 22.2 21.3 |
| 24 | -1.29e-07 | 0.1965 | 0.1962 | 0 | 7 | 0.77% | 8.3 7.7 12.4 2.8 13.3 11.8 22.4 21.3 |

Moves (own signature v opponent signature: from -> to):

- round 8: - v QB 4->0, - v RB 2->0, - v RN 2->1, RR v - 0->2, RR v R 1->2, B v RB 2->0, RB v B 0->2, QRB v QBN 5->4,
  QN v - 0->3, RRN v RBN 4->3, QRRN v QRBN 6->5, BN v RN 2->4, QBN v QRB 5->6, RBN v RRN 4->5, QRBN v QRB 5->6,
  QRRBBN v QRRBB 6->7, QRRBBN v QRRBN 6->7.
- round 16: - v RN 1->0, B v RB 0->4, RB v B 2->0, QRRBNN v QRRBN 6->7.
- round 24: - v QR 5->6, - v QB 0->4, - v QN 4->1, - v RN 0->1, QR v - 0->1, B v RB 4->2, QB v - 0->2.

Reading:

- The moves are few and partly go back and forth: B v RB went 2 -> 0 -> 4 -> 2 and - v RN 2 -> 1 -> 0 -> 1. The loss
  differences of these cells are at the noise level of the E-step sample, and the gains of rounds 16 and 24 are about
  4e-5 of the loss. The remaining rounds (32, 48, 64) are likely to behave the same.
- Round 8 accepted a small loss increase (+1.6e-6): the default table had a crossing of 0.2092, above the budget of
  0.20, and the round had to bring it under. From round 16 the multiplier falls, and in round 24 it is 0: the
  unconstrained optimum is already within the budget.
- **The budget is 0.20** (`--router-em-budget 0.20` in `train_c2.sh`), measured in expert changes per capture summed
  over the two perspectives; the crossing is now 0.196. It does not keep e2-e4 small: e2 and e4 have 12-16% everywhere.
- Constraints that do bind (constructor defaults of `RouterEM`, not command-line flags):
  - `min_share` 0.04: a cell may leave expert e only if e keeps at least 4% of the sample. e3 has 2.8%, so **no cell can
    leave e3**; cells can only enter it (two did in round 8). The E-step cannot express "e3 should be merged".
  - `min_cell_samples` 2000: a cell is movable only with at least 2000 of the 4,194,304 entries (mass 4.8e-4).
    **220 cells are movable, holding 90.5% of the training mass**; the other 2,696 cells (9.5% of the mass, mostly
    rare imbalances) stay at the default for the whole run.
  - `max_move_share` 0.15: not binding (at most 1.33% moved).

## 5. Verdict

- Coverage is balanced: every expert gets at least 2.8% of the training entries and 3.8-5% of the real positions; no
  expert is wasted capacity in the sense of being unused.
- Training per use is not balanced: e3 is trained at 0.56 of its search use and e7 at 1.85. In the search e7 has 11-12%
  of the evaluations but takes 21% of the training. This comes from the data (full-material positions dominate the
  stream) and from the class boundaries, not from the EM.
- The learned routing has not learned anything yet: the table is the default plus noise-level moves, and the M-step
  constraints keep e3 and 92% of the cells fixed. The expert of a perspective is set by the opponent's material, as
  designed for the start.
- Whether e3's positions are fitted worse than a neighbour expert would fit them cannot be read from the population.
  That needs the loss change when e3's positions are evaluated by e2 or e4 (section 7).

## 6. Options (none applied; changing the running training needs the author's approval)

1. **No change in P now** (recommended until the offline E-step is measured). The experts are populated, the epoch
   loss is 13-14% below Consilium's at the same epoch, and the table is close to the default the run was designed to
   start from.
2. **Let the E-step decide about e3**: lower `min_share` from 0.04 to 0.02, so cells can leave e3 when another expert
   fits them better. A trainer change (new flag) and a restart from `last.ckpt` before round 32 (about 15:00 UTC 21/10),
   48 (about 19:45 UTC) or 64 (about 00:35 UTC 22/10). Cost: one resume (the epoch in progress is redone).
3. **More movable cells**: `--router-em-sample-batches 256` (the flag exists) quadruples the E-step sample, so cells
   with a quarter of today's mass become movable; the round takes about 4 times longer (minutes). Same restart cost.
4. **Merge and re-split**: merge e3 into e4 (or e2) and use the freed expert to split e6 (22% of training, 20% of the
   search), for example by the own material. The freed expert would start as a copy of its parent (delta and optimizer
   state), so the network does not lose what it learned. A larger trainer change (copy of weights at a round), worth
   it only if the offline E-step shows that e3 adds little over e4.
5. **Initialise starved experts from neighbours**: not needed. No expert is empty, and an expert's weights are the
   shared base plus its own delta, so a rarely trained expert already falls back to the base.
6. **Rebalance the data instead of the experts**: a sampling weight per routing class (in the loader, like pc-y) that
   lowers full-material positions and raises lone-queen ones, so training per expert follows search use. It changes
   the data distribution of the run: better tested in F or in the next run than in P.
7. **Fewer experts in F**: not suggested by these numbers. Merging would reduce capacity where the search spends 5% of
   its evaluations; it makes sense only if the E-step shows two experts are interchangeable.
8. **Budget**: raising it changes nothing (it no longer binds); lowering it would reduce accumulator refreshes after
   captures, not change the population.

## 7. Postponed measurements

Both were stopped on 21/10 because the Xeon was busy. The second should not run on the VM CPU either: with the
training running, one CPU forward of 8192 positions takes about 20 s there, so the E-step would take more than 2 hours.

- **Full search set** (about 35 minutes with 10 single-thread processes; the counts are written when the engine exits):

  ```
  cd <work>\c2eng\pop
  python c2pop.py search <work>\c2eng\pop\eng\t_c2pop.exe <work>\c2\nets\c2_P_ep22.nnue roots_a.fen search\search_ep22_a.txt 1000000
  ```

  One process per export and per half (`roots_a.fen`, `roots_b.fen`); `nn-consilium2.nnue` must sit next to the exe.
  Run from PowerShell (MinGW binaries crash under Git Bash pipes).
- **Offline E-step** (`c2_estep_offline.py`, the trainer's own E-step on an export, lambda 0.75): for each expert, the
  loss change when its positions are evaluated by each other expert, and the gain of unconstrained re-routing. A small
  change means the expert adds little over its neighbour (wasted capacity); a large one means it has specialised. The
  script imports the trainer, so copy it into `<work>\c2\nnue-pytorch` and run it there, on the local holdout
  `Training_NNUE\localcpu\data\t91_sample.binpack` (on Windows the stream creation can fail with an access violation;
  retry):

  ```
  python c2_estep_offline.py <work>\c2\nets\c2_P_ep25.nnue estep_ep25.npz <project>\Training_NNUE\localcpu\data\t91_sample.binpack --batches 24 --threads 24
  python <work>\c2eng\pop\c2pop.py estep estep_ep25.npz estep_ep25.md
  ```

  Best on the first export after round 24 (ep25), and again after round 32, to decide options 2-4 before round 48 or 64.

## 8. Files

- `<work>\c2eng\pop\c2pop.py`: histograms, search driver, report tables (`pop_full.md` = all tables, every export).
- `<work>\c2eng\pop\c2_cell_sample.py`: training-stream sampler (VM); `train_cells.npz` (cells and piece counts).
- `<work>\c2eng\pop\c2_estep_offline.py`: offline E-step (postponed).
- `<work>\c2eng\pop\games_hist.npz`, `roots.fen` (256 search roots), `search\search_epNN_{a,b}.txt` (partial counts),
  `em_v2.jsonl`, `em_v4.jsonl` (EM log of the run).
- `<work>\c2eng\pop\eng`: scratch copy of the 8.1 engine with `-DC2POP` (cell counter in `nn_pos_eval`,
  `cn::cell_of_board`), build `t_c2pop.exe`. Triumviratus_8.1 itself is unchanged.
