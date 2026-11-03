# The next Triumviratus network: research and design (21/10/2026)

Answer to `docs/BRIEF_NEXT_NETWORK_2026-10-21.md`. Research and design only: no training, no engine change. The
two small measurements of section 3 ran on one core from existing game files and network samples. Scripts and raw
outputs are in `docs/measurements_2026-10-21/`.

Working name: **Consilium Plebis**.

**Author's decisions of 21/10 (they override the text below where it differs):**
- PassedPawns v1 stays in the new architecture.
- **No queen buckets**, neither on the input (R1, R6, K32Q2-like) nor in the layer stacks (S1 with queen states). Queen
  rules remain in the measurement tables as data only. Candidate rules: **R5** (opponent non-pawn material class,
  per perspective, 4 experts) and **R11** (total non-pawn material, 4 bands, symmetric), plus the learned table B4.

### What is ours and what is shared

| element | origin |
|---|---|
| HalfKAv2_hm, threat inputs (Full_Threats), PawnPair / PP_3Wide, layer stacks, finny cache | shared (Stockfish; PawnPair from Pawnocchio) |
| int8 FT trained with the clamp, row pairing, PSQT removal | Stockfish techniques (nnue-pytorch #542, e08dfa80, SFNNv17); ours only the clamp on merged base + per-expert delta |
| **PassedPawns** | **ours** (6.0, 16/07/2026) |
| **Consilium**: first-layer weights chosen by material | **ours** (8.0); SF #531 (20/10) is later related work |
| **Experts by non-pawn material chosen for change cost** (R5 per perspective, R11) | **new, ours**: NPM phase is classical, its use as the first-layer key to cut rebuilds is not |
| **Learned routing table on the first layer** (hard EM with a change penalty) | **new, ours**: SF's routers act on the layer stacks |
| **More data for the endgame expert** | **ours** (pc-y is the SF trainer's parameter) |
| Pawn-king subnetwork with a cache | proposed by A. Peng (2022), never measured: "revisited, first measured" if it works |
| kingstep and hybrid at expert changes | ours (block 6 module) |

## 0. Summary

1. **The memory of the first layer is the speed problem, and int8 is the way out.** Per node we read fewer weight rows
   than Stockfish 19, but each HalfKA row is 2,112 bytes in int16 and our 90,112 rows (190 MB) are the largest
   king-relative block of any engine read (SF19 22,528 rows, SF dev 45,056 in int8, Reckless 7,680). SF trained
   int8 feature-transformer weights (SFNNv18, 13/10/2026) by clamping the merged float weights to +-127/256. That is
   the scale our threat rows already use, so the engine reads int8 HalfKA rows with the code it has. Our only
   precision test (per-row requantisation, still int16) was neutral: +2.78 +- 6.49 over 3,124 games.
2. **Correction to the brief.** The "+7.8%" of the 8.1 speed work is **7.8% more cycles** (slower). It was the exact
   int8 path with sparse corrections for the 1.57% of weights outside int8, measured on today's network (b11:
   +12.2% instructions, +36.7% mispredicted branches). It is not the speed of a network trained inside int8. That
   ceiling has never been measured on the Xeon. It is gate G0a, and it costs one build.
3. **The bucket rule can be both smarter and cheaper.** Today's band counts every piece, pawns included, and a band
   change rebuilds both perspectives. Measured on game lines (section 3), a rule built on **non-pawn material** cuts
   the bucket changes caused by a capture by 59-68%. Pawn captures no longer cross a boundary, and in the
   per-perspective form a capture changes only one side. On the network's own clusters (P2 data of 09/10) these rules
   match the network at least as well as today's band. The comparison is biased toward today's band, because the
   network was trained with it.
4. **Learned buckets, cheap at inference.** Proposal: a router *table* over the non-pawn material signature (at most
   2,916 cells), learned by alternating assignment (hard EM) with a penalty for boundaries that captures cross often.
   At inference it is one lookup on a key that changes only with non-pawn captures and promotions. Stockfish's
   learned routers (nettest #517, #525, #530, 15-20/10/2026) route the layer stacks after the accumulator, not the
   input layer, and its first router (nnue-pytorch #446, June 2026) failed.
5. **Pawns.** We have never trained pawn blocks from scratch, and the grafts are no evidence against them. The
   proposal has three parts. First, give the endgame expert more data: the recipe samples positions with 8 pieces or
   fewer at 0.45x the midgame rate or less, and that expert learns only from them. Second, keep PassedPawns. Third,
   test one new idea: a **pawn-king subnetwork cached by a pawn-king key**, whose output is added after the first
   layer. It is proposed elsewhere (TalkChess, April 2022) but never published with results. It touches no
   feature-transformer row, so its cost is a cache lookup per evaluation.
6. **Plan.** Stage 0 runs on CPU at home, with no training, and settles speed, refresh frequency and occupancy before
   any GPU hour. Stage 1 is a few proxy trainings, one at a time, each judged by an SPRT. Stage 2 is one full run.
   Expected cost is about 200-250 $ in total (section 7). Each stage has a gate (section 8).

## 1. What we know about our networks

### 1.1 Lineage

| net | release | architecture | positions seen | result |
|---|---|---|---|---|
| rubicon-alea v3 | 6.0 | SFNNv13 + PawnPair + **PassedPawns** (graft) | graft | PassedPawns +6.96 +- 6.56 (2,596) |
| legio-septima | 7.0 | SFNNv16 + PassedPawns, L1 1024, 8 stacks | ~130 G | |
| **Consilium** (F4 avg5) | 8.0 | 4 HalfKA experts by piece count, L1 1024, 8 stacks, threats int8, HalfKA int16 | ~514 G (P 374 + F3 80 + F4 60), ~105 $ | 8.0 vs 7.0 +27.3 +- 8.3, about +15 from the network with its tuning |

Consilium recipe (`recipes/06_consilium`): BT4-relabelled corpus of 701 GB (master-binpacks, linrock 1
and 2, kaggle 1 and 2, xushawn), batch 524,288, lr 8e-4 one-cycle. P used SF's lambda cycle until epoch 292 (fix of
29/09), then descended linearly to 0.75 by epoch 332 and stayed there. F3 and F4 use 0.75 fixed, with pc-y -0.20/0.45/1.0/0.95/0.75. Throughput was 4.97 M pos/s on
4x5090 (about 3 $/h, about 0.17 $ per G positions). Lessons kept: loader thread ratio 0.1; λ fixed at 0.75 gave
the Elo that the cycle did not (the result loss had been flat since ep200 under the cycle); concurrency at most 38
(physical cores) for network tests; pentanomial errors.

### 1.2 Where the cycles go (release builds, Xeon socket 0, 20/10)

- Per node SF19 needs 7.7% fewer cycles in the middlegame and 4.9% fewer in endgames. Under load (40 engines) SF19 is
  11.4% faster. At 100k fixed nodes we are level (+3.6 +- 10.0) and at 40+0.4 we lose -22.9 +- 16.0 (440 games):
  speed and time management share the gap.
- Network: +6.6% cycles in the middlegame against SF19's network and +3.9% in endgames. Almost all of it is
  `apply_tile` (168k of 956k samples), `refresh_tile` (48k) and `kingstep_tile` (34k), which wait for memory on the
  HalfKA rows. The instructions of the evaluation are level with SF's.
- Weight traffic per node (bench, AC §1): 2.83 HalfKA rows and 6.54 threat rows, 195 cache lines, of which HalfKA
  is 46%. 60% of the L3 misses fall on weight rows.
- Expert changes: hybrids 0.071 per node against SF's 0.015 (X2), with about 15 HalfKA rows each, because in a node
  at 24, 16 or 10 pieces every capture changes the expert, and quiescence is mostly captures. ExpertFix (the root's
  expert for the whole search) saves 4.9% of cycles in the middlegame and 2.8% in endgames. That is the price of
  expert changes today. It lost -27.7 +- 18.2, because the experts really differ: sd 46 cp one band away, 78 cp two
  bands away, 89 cp three bands away.
- Widening the int8 blocks to int16 at load cost +60% L3 misses and -10% speed (17/10). The working set decides,
  in the direction we want to go as well.

### 1.3 int8 HalfKA: what has been tried

| attempt | date | result |
|---|---|---|
| AG4 weight study | 12/10 | 1.57% of HalfKA weights outside +-127 (1.65% of live rows), max 909, spread over 93% of rows. Best requantisations (Ropt, Copt, U3) add ~16 internal units of eval error (~4 cp); band 0 is the most sensitive |
| Ropt requantised, **still int16** (precision only), EvalFile, 5+0.05 HT | 12/10 | **+2.78 +- 6.49 / 3,124**: the precision loss is not measurable |
| int8 path with per-row integer scale (`vpmullw`), 15+0.15 HT | 12/10 | -20.4 +- 23.8 / 222, depth -0.16/-0.47 ply: the multiply eats the bandwidth gain |
| int8 + sparse exact corrections (b11) | 19-20/10 | **+7.8% cycles**, +12.2% instructions, +36.7% branch misses: rejected |
| clamp today's net to int8 (`_5clip`) | 20/10 | changes evals a lot (mean |d| 78, sign flips 7.8%): a speed ceiling only; built for Zen 4, **never measured on the Xeon** |

Conclusion: the remaining route is the one Stockfish took, **training with the clamp**. The engine then reads the
HalfKA rows exactly like the threat rows (`vpmovsxbw` + `vpaddw`, no scale), so no new arithmetic is needed.

### 1.4 What the network gets wrong (P1, 09/10, 3,888 positions, residual of d1 against 300k-node search)

Endgames (15 pieces or fewer): it underestimates passed pawns: +8 cp per rank (t 5.6), +18 cp when the enemy king is
outside the square (t 2.2; the single estimate was +45), connected passers +23 (single estimate, t 5), space +8 cp
per square. Middlegame: it underestimates danger to its own king (open files near the king -14, t -4.5), space (+6
cp per square) and connected passers (+18, t 3.3). Figures are the joint estimates of 09/10 18:45 unless marked. It handles drawing
factors well (opposite bishops, rook endings). The residual terms added to the search evaluation and the corrections
keyed on these concepts were all neutral or negative (removed 11/10). The grafts (PassedRel, PassedState, KingFiles,
Space, LockedPawns, Outposts) were all neutral or negative too, at an engine cost of 2-3% of cycles each. **None was
trained from scratch.**

A fact that has not been looked at: the recipe samples positions by piece count with the spline pc-y (x = 0, 8, 16,
24, 32 pieces; y = -0.20, 0.45, 1.0, 0.95, 0.75). Positions with 8 pieces or fewer are drawn at 0.45x the midgame
rate or less. In a shared feature transformer (SF) that is a deliberate balance. With experts, the endgame expert's
HalfKA rows learn only from those positions. The residual of P1 is largest exactly there, and AG4 found band 0 the
most fragile.

### 1.5 How the network divides positions (P2, 09/10; re-analysed 21/10)

k-means on first-layer outputs (92,442 evaluations): the best explanation is material band x queens (NMI 0.43 at
k = 6), then queens alone (0.39), then today's band (0.34). Within the opening band it splits by closed structure.
Section 3.2 adds the non-pawn material rules.

## 2. Recent NNUE developments (checked 21/10/2026)

### 2.1 Stockfish, merged

| change | date | what | measured |
|---|---|---|---|
| SFNNv17 (a17c0ce2) | 30/09 | PSQT accumulator removed | non-regression STC and LTC; "training speed significantly faster" |
| finetune at lr/10 (2d54ef33) | 10/10 | longer stage 2 at one tenth of the lr | STC and LTC gainers |
| **SFNNv18** (5c4cef0, PR #7215) | 13/10 | K32Q2 inputs (own king 32 buckets x opponent queen present, per perspective); **all FT weights int8**; 32 layer stacks = 8 piece buckets x 4 queen states; uncompressed file | STC 6198-5910-11700 (about +4.2), LTC 7533-7236-13911 (about +3.6); all three single changes failed against the branch; "output buckets only" was closest |
| nnue-pytorch #542 | 13/10 | int8 FT: bounds +-127/256 from the export dtype; **merged weight (real + virtual) clamped**, then the virtual part subtracted; clamp every N steps or per epoch; own weight decay for virtual weights | recipe (nettest #494): 560 x 1.07 G pretraining (3 repetitions) + 600 x 1.07 G finetune at half batch, λ 1.0 with cycle -0.3 and jitter, same pc-y as ours |
| row pairing (e08dfa80) | 17/10 | two int8 rows widened together (unpack + `maddubs` with ones) instead of one `vpmovsxbw` each: the update was bound by the shuffle port | +7.0% nps avx512icl, +5.2% avx2 (Ice Lake server) |

Closed without adoption: K32Q4 (both queens, #508: STC 5406-5633, about -3.7), 128 stacks by piece count x king
zone x queens (#509: STC 8333-8540, about -2.2) and by pawn bucket (#510).

### 2.2 Stockfish, in flight (nettest #511-531, 13-20/10; no results published yet)

- **K32Q2P4** (#531, xu-shawn, 20/10): K32Q2 with **4 piece-count buckets** (`(pieces - 1) // 8`, kings and pawns
  counted) on the input layer. That is material on the first layer, the category of Consilium, and even the name
  matches our feature `HalfKAv2_hm_P4` (28/09). It is independent related work, dated after our 8.0 release.
- Learned routing: #517 "smart bucketing again" (VQ codebook router, top-2 MoE of the **layer stacks**; context =
  own piece count x own queen x both king squares, exported as a 67 KB uint8 table), #525 dynamic bias routing,
  #527 gated skip biases, #530 top-2 MoE dual bias.
- Pawn inputs (xu-shawn): **Passed_48** (16/10, own passed pawns by square, 48 inputs: the concept of our
  PassedPawns, which is public since 16/07/2026, commit de4b786, 96 inputs, +6.96 measured; sources:
  [nnue-pytorch@5a382b4](https://github.com/xu-shawn/nnue-pytorch/commit/5a382b404d2584d4a8fca037a7f209304796a595),
  [nettest #516](https://github.com/vondele/nettest/pull/516)), Pawn_Push (pawns pushing against pieces), Promo_Atk
  (attackers of the promotion square), Ram_Ram (pairs of blocked pawns, #522), PassStop_Threats (threats on the stop
  square of passers, only with at most 4 or 8 non-pawn pieces, #523/#524).
- Others: material one-hot (including a 54 x 54 non-pawn signature) into the l1 outputs (#521, proxy loss -1.96%),
  per-stack widths (#528/#529), 8 king buckets for the pawn pairs (#526), L1 1152 (#519), higher λ throughout
  (#520), clamp every 1024 steps (#515).

Reading: Stockfish is moving into material conditioning and pawn knowledge, the two areas of this brief, and it
iterates quickly (a retrain "takes less than a day" on their cluster). Our edge has to come from what they are not
doing: material on the first layer chosen for change cost, a learned input table, cached pawn-king knowledge. We do
not copy their in-flight features. If one of them lands with a large gain, we judge it with our own test.

### 2.3 Other engines (sources in `_reference/`, June-September 2026)

| engine | king input buckets | L1 | first-layer types | output buckets | note |
|---|---|---|---|---|---|
| Reckless | 10 | 768 | pieces int16, threats int8 (66,864 rows) | 8 (table) | separate piece and threat accumulators |
| Stormphrax 8.0 | 16, mirrored | 640 | psq int16, threats + pawn-pawn int8 | 8 (material) | separate psq and threat accumulators |
| PlentyChess 8.0 | 12 | 640 | threats + pawn pairs + 768 x 12 | 8 | fused threat and psq accumulators since 07/2026 |
| Viridithas 20 | 16 (x2 mirror) | 1024 | psq int16, threats + pawn tuples int8 | 8 | |
| SF19 / SF dev | 32 / 32 x Q2 | 1024 | int16 HalfKA / **int8 all** | 8 / 32 | |
| **Triumviratus 8.0** | **32 x 4 experts** | 1024 | HalfKA int16, the rest int8 | 8 | 128 king-relative buckets: the largest first layer of all |

None of them chooses first-layer weights by material except Consilium and the SF experiment #531.

## 3. Two measurements made for this design (21/10, one core, existing data)

### 3.1 Bucket changes of candidate rules on game lines

`measurements_2026-10-21/bucket_rules.py`, 1,200 games sampled from a UHO self-play SPRT (8+0.08, 133,558 positions,
111,817 legal captures) and 1,200 from an endgame-book SPRT (112,169 positions). For each position and perspective:
bucket occupancy, changes per ply, and, for every legal capture, whether it would change the bucket of each
perspective. The last value is a stand-in for the quiescence hybrids, the expensive case. NPM = non-pawn material
(N, B 3, R 5, Q 9); class = <=5, 6-13, 14-22, >=23 per side.

| rule | buckets | occupancy max/min (UHO) | changes per ply | **changes per capture**, UHO | per capture, endgames |
|---|---:|---:|---:|---:|---:|
| R0 today: total pieces, 4 bands | 4 | 1.3 | 0.048 | **0.262** | **0.280** |
| R1 today x opponent queen (= Consilium x Q2) | 8 | 14.9 | 0.062 | 0.276 | 0.300 |
| R3 SF #531 (pieces-1)//8 x opponent queen | 8 | 33.0 | 0.060 | 0.229 | 0.299 |
| R9 opponent queen only (K32Q2) | 2 | 1.2 | 0.016 | 0.017 | 0.021 |
| **R5 opponent NPM class, per perspective** | 4 | 1.5 | 0.047 | **0.108** | **0.141** |
| **R6 opponent NPM class x opponent queen** | 6 | 4.9 | 0.047 | **0.108** | **0.141** |
| **R11 total NPM, 4 bands (<=12/13-28/29-46/47+), symmetric** | 4 | 1.7 | 0.049 | **0.085** | **0.105** |
| R12 opponent NPM, 6 classes | 6 | 2.5 | 0.063 | 0.148 | 0.138 |
| R7 pawn count, 4 bands | 4 | 3.9 | 0.048 | 0.482 | 0.222 |

Reading:
- Changes per **ply** are about the same for every material rule (about one change every 20 plies of a game). The
  difference is in the **tree**. Today 13% of captures cross a band and rebuild both perspectives. With non-pawn
  material, pawn captures never cross, and in the per-perspective form only the side that lost the piece changes.
- R6 adds the queen at no extra change cost: losing the queen always moves the opponent's NPM class.
- Pawn-count buckets change on every pawn capture: rejected as an input bucket.
- Limits: game lines, not search trees. The capture count weighs every legal capture equally. The endgame book's
  occupancy is that of a selected book. Gate G0b measures the real rate in the engine.
- Estimated saving: today's expert changes cost about 5% of cycles in the middlegame and 2.8% in endgames
  (ExpertFix). If that cost scales with the capture rate above, R6 saves about **3% in the middlegame and 1.4% in
  endgames**, and R11 about 3.4% / 1.8%. This holds before int8, which halves the bytes of each rebuild as well.

### 3.2 Which rule matches the network's own clusters (P2 data re-analysed)

`measurements_2026-10-21/neurons_clusters_npm.py` on `studio_neuroni/clu` (same 92,442 evaluations as 09/10), NMI between
k-means clusters of the first-layer outputs and each rule:

| rule | k = 4 | k = 6 | k = 8 | k = 12 |
|---|---:|---:|---:|---:|
| SF #531 piece buckets x queens 0/1/2+ | 0.345 | 0.429 | 0.375 | 0.349 |
| today's band x queens 0/1/2+ | 0.339 | 0.428 | 0.370 | 0.346 |
| total NPM 4 bands x queens 0/1/2+ | 0.363 | 0.385 | 0.370 | 0.336 |
| queens (side to move, other) | 0.328 | 0.408 | 0.369 | 0.346 |
| NPM classes (side to move, other) | **0.407** | 0.374 | 0.343 | 0.312 |
| opponent NPM class x opponent queen (R6) | 0.336 | 0.364 | 0.340 | 0.306 |
| opponent NPM class (R5) | 0.363 | 0.342 | 0.310 | 0.280 |
| total NPM 4 bands (R11) | 0.363 | 0.329 | 0.305 | 0.276 |
| **today's band** | 0.325 | 0.338 | 0.283 | 0.262 |

Every non-pawn-material rule explains the network at least as well as today's band, although the network was
trained with today's band (a bias in favour of R0). Queens remain the strongest single factor. The best cheap rules
for both criteria, fewer changes and closer to the network, are **R6** (per perspective, 6 buckets) and **R11 x
queens** (symmetric). NMI is a weak proxy for Elo. It ranks candidates; it does not choose among them.

## 4. The design space

### 4.1 First layer: storage

Rows per expert = 32 king buckets x 704 = 22,528. Row = 1,024 weights + 64 bytes (PSQT, padding): 2,112 bytes in
int16, 1,088 in int8, 1,024 in int8 without PSQT. The other blocks (threats, pawn pairs, passers) are about 70 MB.

| option | HalfKA rows | HalfKA MB | whole net MB | HalfKA lines per row read | finny cache per thread |
|---|---:|---:|---:|---:|---:|
| today (4 experts, int16) | 90,112 | 190 | 260 | 33 | 1.1 MB |
| 4 experts int8 | 90,112 | 98 | 168 | 17 | 1.1 MB |
| 6 experts int8 (R6) | 135,168 | 147 | 217 | 17 | 1.7 MB |
| 8 experts int8 (R1, or learned K = 8) | 180,224 | 196 | 266 | 17 | 2.3 MB |
| 8 experts int8 without PSQT | 180,224 | 185 | 252 | 16 | 2.3 MB |
| 16 king buckets x 8 experts int8 | 90,112 | 98 | 168 | 17 | 1.1 MB |

The bytes read per node halve with int8 whatever the number of experts. The miss rate depends on the working set,
which grows with the number of experts. Six int8 experts still use less memory than today's four.

### 4.2 Options compared

Speed is cycles per node against today, estimated; the number of experts includes its effect on the working set.
Strength is a prior from the evidence cited, not a measurement.

| id | option | strength prior | speed estimate | refresh / change cost | training cost | risk |
|---|---|---|---|---|---|---|
| **I1** | HalfKA int8 trained with the clamp (#542 method, on merged base + expert delta + virtual) | 0 +- 2 at fixed nodes (Ropt +2.78 +- 6.49; SF shipped it) | **-2% to -7%** (AG4: upper bound -7%, realistic -2/-4; G0a measures it) | rebuild bytes halve | none (a clamp per N steps) | low |
| I2 | PSQT removed (SFNNv17) | 0 (SF non-regression) | -0.5 to -1% (17 -> 16 lines per row) | none | faster training | medium: `nn_scale` uses psqt, rewrite needed |
| I3 | row pairing in the update (SF e08dfa80) | 0 (same tree) | 0 to -5%; our CN_W1 interleave was level/lost on the Xeon with int16 HalfKA, the bound may move once all rows are int8 | | none | low, engine only |
| B0 | today's rule (pieces, 4 bands) | reference | reference | reference | | |
| B1 | B0 x opponent queen (8) | +1 to +4 (K32Q2, P2 NMI) | -1% to -3% with I1 (working set as today, changes +5%) | like today | none | low |
| **B2** | R6: opponent NPM class x opponent queen, per perspective (6) | +1 to +4 (queens + NPM, P2) | **-4% to -9% with I1** (fewer rebuilds, working set 147 MB) | **-59% per capture**, one side per change | none | medium: asymmetric experts per perspective, new in this form |
| B3 | R11 x queens, symmetric (4 bands x 2-3 queen states = 8-12) | +1 to +4 | -2% to -6% with I1 | -68% from NPM, queen changes added | none | medium |
| **B4** | learned router table over the NPM signature (K = 6-8 experts, section 5) | +2 to +6 (data-driven, cost-aware) | like B2/B3 (penalty chooses) | tunable | +1 proxy run for the EM rounds, ~+10% per run | high: new method |
| B5 | 16 king buckets x 8 experts | unknown, could lose king detail | -3% to -7% | king refreshes less frequent | none | high |
| S1 | layer stacks 8 -> 32 (8 bands x 4 queen states, SF v18) | +1 to +2 (SF "closest gainer") | 0 to +0.5% (1 MB of stacks, L2 pressure) | none | ~+2% | low |
| **P1** | more endgame data for the endgame expert (pc-y0/y1 raised, or sampling per expert) | +1 to +3 in endgames | 0 | 0 | 0 (composition only) | low |
| P2 | PassedPawns v1 kept; relational passer rows (king square, connected) trained from scratch | +0 to +3 | -2% to -3% (graft measurements) | pawn and king events | 0 | medium |
| **P3** | pawn-king subnetwork cached by key, added to the l1 outputs (section 6) | +2 to +6 if it works (P1 residual effect sizes) | -0.5% to -2% (cache lookup; misses to measure) | none on the FT | +5-10% | high: never published with results |
| L1 | L1 1536 / 2048 | | -11% / -20% nps (28/09) | | | rejected |

Rejected and why: hysteresis in the bucket choice (the evaluation would depend on the path, breaking hash and
repetition consistency); blending experts at inference (Z2: +15.6% cycles, -24 +- 11); one expert per search
(ExpertFix -27.7); queen-square buckets (a refresh on every queen move); pawn-count input buckets (section 3.1);
mobility inputs (-16% nps, 29/09).

## 5. Learned input buckets (B4): method

Goal: a bucket that is learned from data, costs one table lookup, and changes rarely along a line.

- **Key.** The non-pawn signature from the perspective: own and opponent counts of N, B, R (each 0, 1, 2+) and Q
  (0, 1+), 54 x 54 = 2,916 cells. It is the same key SF's #521 feeds as one-hot input into l1. It changes only on
  non-pawn captures and promotions and is incremental in the make (a material key).
- **Table.** `cell -> expert` (uint8, 2.9 KB). HalfKA index = expert x 22,528 + king-relative feature, as today.
- **Training by alternation (hard EM).** (a) Start from R6 or from today's bands mapped onto cells. (b) Train the
  network for a block of steps with the table fixed (the shared base and per-expert deltas of `HalfKAv2_hm_P4`
  stay; a reassigned cell starts from the shared base, so moves are gentle). (c) For a sample of positions per cell
  (for example 2,000), evaluate the loss under every expert by forcing the bucket index (K forward passes, minutes
  on one GPU). (d) Reassign cells to minimise *sum of cell loss + mu x (frequency of captures that move between
  cells with different experts)* under a minimum occupancy per expert. Local search over cells is enough at 2,916
  cells. The transition frequencies come from data (section 3.1 method, extended to the trees of a counter build).
  (e) Repeat (b)-(d) a few times, then freeze the table for the rest of the run.
- **Why not a differentiable router.** A hard expert choice gives no gradient to the router. Soft mixtures need K
  feature transformers per sample in training, and a blend at inference (measured: too slow). SF's Gumbel router
  failed (#446), and their new VQ routers act on the layer stacks.
- **Variant "train fine, merge coarse".** Train 12-16 experts on a proxy, then merge experts with close weights
  (weighted average of deltas), then a short fine-tune at K = 6-8. Simpler, but needs a wider proxy.
- **Search coupling.** The per-expert search levers (RfpExp, FutExp, LmrExp, CorrPhase) are keyed by the
  piece-count band. They keep their own bands, so the search stays tuned. Only EvalScaleB (output scale per
  piece count) belongs to the network and goes to SPSA after the bake, as in 8.0.

## 6. Pawn-king subnetwork with a cache (P3): method

- **Inputs.** Own and enemy pawns (96 squares), both king squares, side to move; oriented per perspective like the
  threat block. Key = pawn key xor king-square keys.
- **Subnetwork.** Sparse pawn and king one-hot -> 256-512 hidden (int16 sums, CReLU) -> 32 outputs per perspective,
  one small dense layer. Its 32 values are added to the output of fc_0 of the chosen stack, before the activations,
  through one 32 x 32 matrix per stack. This is the place SF's #521 uses for material.
- **Cost.** One lookup per evaluation in a per-thread table (16k entries x 128 B = 2 MB, or a smaller table keyed by
  pawns only). On a miss (to measure: pawn-only keys hit about 95% in classical engines, and king moves lower the
  rate for the pawn-king key) the subnetwork costs a few thousand cycles. At 0.44 evaluations per node the expected
  cost is 0.5-2% of cycles. It reads no feature-transformer row.
- **Why it can work where the grafts failed.** It is non-linear in the pawn-king configuration ("passer outside the
  king's square", "open file in front of the castled king" are conjunctions over many squares, which the linear
  first layer represents badly). It is trained from scratch with the rest. Its cost does not grow with the number
  of features.
- **Prior art.** Alvin Peng proposed a cached pawn-king network concatenated after a hidden layer (TalkChess,
  22/04/2022), and Sopel wrote he would test a convolutional variant. We found no published result. Category if it
  works: revisited, first measured.
- **Training.** The loader emits the pawn and king squares (PP_3Wide already decodes the pawns). The module is a
  normal PyTorch layer; it is quantised to int8 like the stacks. `cross_check_eval` must cover it.

## 7. Compute plan

| item | where | throughput | cost |
|---|---|---|---|
| stage 0 measurements | Xeon, at home, when the machine is free | | 0 $ |
| data | 701 GB BT4 relabel, download at ~120 MB/s (~1.6 h) per VM; keep the disk between stage 1 runs | | ~5 $ per download |
| proxy run | 4x5090, L1 1024, ~40 G positions (~2.3 h at ~4.8 M pos/s) | | **~8-10 $ each**, 5-6 runs: ~50-60 $ |
| full run | 4x5090, ~520-600 G positions (Consilium scale; SF uses ~1.2 T) | ~4.5-5 M pos/s | **~100-130 $** (~32-36 h) |
| fine-tunes after the bake | MI50 at home (~0.3 M pos/s estimated, to measure first) | | 0 $ |

Total about 160-200 $ plus a 25% margin: **~200-250 $**. Proxies and full runs start only on the author's decision,
one at a time. The Xeon carries all SPRTs (CCRL gauntlets and SPSAs compete: plan around them).

## 8. Staged plan with gates

Stage 0 needs no training. Each later stage starts only if the previous gate passes and the author says go.

**Stage 0: measure before training (CPU, at home)**
- **G0a** int8 speed ceiling on the Xeon: PGO `-Det` AVX-512 build with HalfKA clamped to int8 (`-DCN_HKA8_CLIP`,
  the `_5clip` variant) against today's, `run_misura.ps1`, 3 rounds, middlegame and endgames. Different trees, so
  read cycles per node. **Pass: at least -2% cycles per node.** Below that, int8 is still needed to afford more
  experts, but the speed story changes.
- **G0b** change rate in the tree: a counter build (as `-DTRIUMV_NSTATS`) that computes the keys of R0, R5, R11,
  R12 and the B4 cells at every make, and counts per node the perspective rebuilds each rule would cause. bench
  and the 51 positions of the kit. **Pass for a rule: rebuilds per node <= 50% of R0's (R5, R11).**
- **G0c** occupancy on the training corpus: the rules of section 3 on a sample of the BT4 binpacks with the pc-y
  weighting; effective positions per expert. **Pass: every expert >= 5% of the sampled positions.**
- **G0d** shape speed: a random network of the chosen shape (`make_random_net.py`), correctness (bench, nnperft,
  nnueverify across bucket changes) on MinGW, then xperf on PGO `-Det` against today's network. Random networks are
  less sparse, so this is an upper bound on cost. **Pass: cycles per node <= today's.**
- **G0e** trainer correctness: the int8 clamp on merged weights in `HalfKAv2_hm_P4` (and the new key), the router
  table in the loader, `cross_check_eval` on a random network: R² >= 0.99999, mean error < 2 internal units.

**Stage 1: proxies (one at a time; same data, seed and steps; ~40 G positions each)**
- P-0 base: today's architecture with int16 (the proxy reference).
- P-1 = P-0 + I1 (int8 clamp). **Gate: non-regression at fixed nodes (100k), then at TC; SPRT [-1.75, 0.25].**
  Then I1 is in every following proxy.
- P-2 bucket rule: R5 or R11 (whichever passed G0b and G0c best; no queen buckets) against P-1. **Gate: SPRT [0, 3] at TC**, UHO, noHT, both
  sockets with per-socket sums.
- P-3 B4 learned table against the winner of P-2. Same gate.
- P-4 P3 pawn-king subnetwork on the winner. Gate: SPRT [0, 3] at TC, plus an endgame-book run without
  adjudication.
- P-1 to P-4 also give S1 (32 stacks) and P1 (endgame sampling) a place: each is a one-line change, and it enters
  the full run only with its own proxy SPRT (rule: a new technique gets its own SPRT).

Proxies are weaker networks, and SF's single changes did not pass alone at full scale. So proxies decide only
clear effects. Close calls go to the full run with the more conservative choice.

**Stage 2: the full run**
- Recipe: Consilium's P (λ cycle only before the 0.75 descent, as the fix of 29/09) or λ 0.75 fixed from the start
  if a proxy showed no difference; F at λ 0.75 fixed and lr/10 tail (SF 2d54ef33); clamp every N steps; average of
  the last epochs, FT permutation, bench plain = permuted.
- **Gate: SPRT at TC against Consilium**, same binary, only EvalFile different. 20+0.2 or longer, concurrency <= 38,
  pentanomial, >= 20,000 games or to the bound. Then 60+0.6 confirmation. Then xperf on PGO `-Det` release builds.
  Then the release build against SF19 (release builds only).
- Bake: SPSA of EvalScaleB and of the eval-unit margins (as MOE1), then SPRT of the vector.

**Stage 3: engine work on the new network** (identical trees): row pairing (I3) re-measured with all rows int8, the
finny cache keyed by the new bucket, prefetch of the new expert's finny entry, the B6 module's tile sizes for 17-line
rows.

## 9. Risks

- **Proxy results do not transfer.** Mitigation: only large effects decide; the full run keeps the conservative
  choice on close calls.
- **int8 costs strength in band 0.** AG4 found the endgame band the most sensitive. Mitigation: P1 (more endgame
  data), check per-band eval error on the proxy before stage 2.
- **Asymmetric experts (B2/B4) confuse the training.** Each perspective uses its own expert; K32Q2 shows it works for
  one queen bit. G0e covers correctness; P-2 is the test.
- **Learned table unstable or collapsed.** Minimum occupancy, the shared base, few EM rounds, R6 as a fallback.
- **Pawn-king cache misses higher than expected.** G0-style counter on the cache before training P-4; a pawn-only key
  as a fallback.
- **SF lands similar ideas first** (#531, #517, pawn inputs). Our documents keep dates and commits; NOVELTIES lists
  later related work as such.

## 10. Decisions for the author

1. Run stage 0 (no GPU): G0a and G0b first, they decide the rest.
2. Hand-structured rule for the first proxy comparison: R5 (per perspective, 4 experts) or R11 (symmetric, 4
   bands); no queen buckets (author, 21/10). G0b and G0c can decide it.
3. B4 learned table and P3 pawn-king subnetwork: in stage 1 or later.
4. PSQT: keep (no `nn_scale` rewrite now) or remove with the new network.
5. Budget for stage 1 (~60 $) and stage 2 (~130 $).
6. NOVELTIES: add SF #531 (K32Q2P4) and Passed_48 as later related work, with dates.

## Sources

- Our documents: `docs/audit_8.0/AG4_HALFKA_INT8_20261012.md`, `AC_VELOCITA_RETE_2.md`, `X2_VELOCITA_RETE.md`,
  `X4_VELOCITA_PROFONDA.md`; `docs/audit_8.1/R5_PROFILAZIONE_SF19_2026-10-20.md`, `B6_SPECIFICA_CONSILIUM.md`;
  `STATO_PROGETTO.md` §4.1 (P1, P2) and the HALFKA INT8 row; `recipes/06_consilium/README.md`,
  `LOG_RUN.md`; worktree `consilium-inference-rewrite-c0fcc4/patch_velocita_8.1/IDEAS.md`; `<work>\xp\b11_mg`.
- Stockfish: PR [#7215](https://github.com/official-stockfish/Stockfish/pull/7215) (SFNNv18), commits a17c0ce2
  (SFNNv17), 2d54ef33, e08dfa80, b877fbde; nnue-pytorch PR
  [#542](https://github.com/official-stockfish/nnue-pytorch/pull/542); nettest PRs
  [#494](https://github.com/vondele/nettest/pull/494) (v18 recipe) and #501-531 (in particular
  [#508](https://github.com/vondele/nettest/pull/508), [#516](https://github.com/vondele/nettest/pull/516),
  [#517](https://github.com/vondele/nettest/pull/517), [#521](https://github.com/vondele/nettest/pull/521),
  [#531](https://github.com/vondele/nettest/pull/531)); fishtest runs 6ad234bf, 6ad2a9f4 (K32Q4), 6ad23486 (128
  stacks).
- Pawn-king network proposal: [TalkChess, A. Peng, 22/04/2022](https://talkchess.com/viewtopic.php?p=924777).
- Engines in `_reference/`: Reckless (31d9cd6), Stormphrax 8.0, PlentyChess 8.0 (+ upstream #488), Viridithas 20.
