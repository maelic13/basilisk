# Cluster 4 research and handoff — PLAN B.5

Started 2026-11-08 on `b5-root-aspiration` at `ad17d2b`, the B.5 entry
head (`master` after B.4.7's flip and B.8.6's deletion of the old arms,
`bench 13` 4,144,790). Donor: Stockfish `sf_19` `edb0d9db`
(`src/search.cpp` line numbers below are that file's: `iterative_deepening`
is `:270-631`, the root bookkeeping `:1437-1500`, `reduction` `:1885-1888`).
Driver symbols are `src/search_worker.cpp`'s and kernel symbols
`src/search_kernel_core.cpp`'s at `ad17d2b`.

**Status: `READY_FOR_IMPLEMENTATION`, 2026-11-08.** §1 is the verdict, §2
the head re-measured, §3 every root and aspiration mechanism against the
donor, §4 every input PLAN B.5 lists, disposed, §5 the interactions with
Clusters 1 to 3's fitted cores and with the time manager, §6 the zero-game
reads, §7 the seeds, §8 the decision, §9 the categoricals and switches,
§10 the implementation contract, §11 the frozen predictions and the
registrations (M1, the diagnostics, the fit, the gates, the denser
conversion floor), §12 the four questions, §13 the artifacts, §14 the
departures table, §15 the instruments that inform this cluster, §16 the
sub-steps proposed to the maintainer.

**How the reads of §2 were taken.** They were run on 2026-11-08 before
this packet was written, with no prediction frozen for them. They size
the head and do not decide anything by themselves; every rule in §9 and
§11 was written after seeing them and is frozen from this commit. One
exception is scored: the fail-high stop packet
(`analysis/b5_failhigh_stop_2026-10-25.md`) registered, on 2026-10-25,
the falsifier for its reading of rec1 (§9.4 there), and §2.2 scores it.

## 1. Verdict

- Cluster 4's contents stand, narrowed: the donor's **root-move record**
  (score, previous score, average, mean square, effort, line, selective
  depth, tablebase rank per root move), its **aspiration loop** on that
  record (the window centred on the move's average, its width from the
  mean square, growth 47/128, beta pulled to alpha after a fail low, no
  give-up hatch) and its **root reduction** land as one arm behind
  `B5_ROOT`. MultiPV lines take their own windows from their records.
- **The largest departure from the donor is the window itself**, not
  its centre. Basilisk's window is ±19 at every score and grows by half;
  the donor's, converted to this engine's units, is ±1 at a level score,
  ±5 at a pawn, ±32 at three pawns and effectively open in a won ending
  (§7). At depth 14 on `suite_v2`, **53.7% of the nodes searched inside
  a window are spent in windows that failed** (Rarog read 61%, B-70),
  1.50 re-searches per window; the won endings of the suite re-search up
  to 73 times in eleven windows. Rarog's one lead that transfers is a
  direction, not a number: its Elo surface fell toward wider windows
  (B-71, B-72). Games decide, as those items say.
- **The root is the one node Basilisk never reduces**, and the donor's
  rule would reduce **44.1% of the root moves after the first, by 2.0
  plies on average** (S4). Every such move is a full-depth null-window
  search today. The arm reduces them as the donor does, behind a switch,
  with the WAC floor as the veto BAS-D17 taught.
- **S6 is not a one-thread defect.** At every first window of an
  iteration the root's TT move is the last iteration's best (0 of 1,034
  differ); after a fail low 1.1% differ (the fail-low store keeps the
  entry's move); after a fail high 26.4% differ, which is a new move
  failing high, searched first by the donor too. The record still takes
  the root's first move from its own line, because the record exists for
  the window and the helpers' side is D.2's.
- **The line growth is chained.** Of the extensions a negative reduction
  takes (7.9% of reduced searches, the PV ply aside), 29.8% fall on a line
  that already holds one and 21.8% on a line holding two or more; 48.5%
  of interior nodes lie under at least one. Triumviratus's per-line cap
  of one (TS-1, +3.88 ± 3.26, −6.6% nodes) has a population here. It is
  a categorical, default today's form, read by one paired run.
- **The fail-high stop's battery passes with no stop at `60000+600` on
  this head**, every search under 0.21× the maximum, with and without
  the stop (identical node counts: it never fired). BAS-R07's rec1 ends
  at depth 20 in 3.8 s because A.11's won-ending tiers lift its score
  into the won band in one jump, where ±19 windows stop failing. The
  2026-10-25 falsifier for "the cascade is gone" therefore fired, and the
  battery re-read it: the sink is gone at `60000+600`. At `180000+2000`
  rec1 still thinks **2.46× the optimum**: ten fail highs on the previous
  best before the optimum, then the give-up window from 13.6 s to 37.3 s
  proving `mate 18`. That is the hole of the stop packet's §2.3, exactly,
  and the stop cannot reach it. The stop's decision rule stands as
  registered; M1 runs on the arm (BAS-R10) with the ply-cap counter and
  the hole's rate on board.
- **Not built here:** optimism (the donor's form is a term of its NNUE
  blend; Rarog's HCE-side read was −5.6; it waits on the network, F),
  the draw jitter (a blindness read first, B-84), the fail-high depth
  reduction as a default (BAS-D17; it needs its clock half, D.1), a
  raised ply cap (it would move the HCE's won tiers into the tablebase
  band and overflow the table's depth byte, §5.5). The in-check draw at
  the cap is a defect repair and lands on both arms.
- Decision: `READY_FOR_IMPLEMENTATION` (§8). The contract is §10.

## 2. The head re-measured

All reads on the B.5 entry head `ad17d2b`, one thread, Hash 64, no
tablebases. Builds: the **census build**, `ad17d2b` plus the throwaway
`tools/results/b5-20261108/census.patch` (22 counters, two stack fields
that only count; `-DDIAGNOSTIC=ON`, no PGO), and the **stop-free census
build**, the same plus BAS-R07's stop removal
(`census_nostop.patch`). Both read `bench 13` **4,144,790**: the search is
unchanged. Hashes in §13.

### 2.1 `suite_v2` at depth 14 (`suite_d14.json`)

`run_suite.py --depth 14 --stride 1 --hash 64`, 106 positions.

| Quantity | This head | Reading |
|---|---:|---|
| interior / quiescence nodes | 7,116,466 / 4,893,846 | B.4.5's ON column read 6.49M / 4.49M before the fit |
| windows (iterations at depth > 3) | 1,034 | |
| fail lows / fail highs / re-searches per window | 0.639 / 0.846 / **1.503** | BAS-D16 read 1.37 on the pre-B.2 search; 13 of 106 positions never re-search; the median position 10.5 times, KBN-K `8/8/8/8/3k4/8/3KBN2/8 w` 73 times in 11 windows |
| give-up windows (delta ≥ 900) | 18 (1.74% of windows) | all in won positions |
| nodes inside windows / all nodes | 91.6% | |
| nodes in windows that failed / windowed nodes | **53.7%** | B-70; Rarog read 61% |
| nodes in give-up windows / windowed nodes | 6.3% | |
| root moves after the first at depth ≥ 2 | 64,233 | |
| of which the donor's rule would reduce; mean plies | **28,325 (44.1%); 2.01** | S4; the median position 44% |
| root TT move ≠ last completed best, at the first window | 0 / 1,034 | S6 at 1T |
| after a fail low | 7 / 658 (1.1%) | `B3FailLowStore` 1 keeps the entry's move; the seven are replaced entries |
| after a fail high | 227 / 860 (26.4%) | a new move failed high; the donor searches it first as well. 73.6% of root fail highs are on the previous best: the stop's same-move population |
| LMR-eligible moves; reduced-path searches | 4,711,017; 4,149,247 | |
| extensions from a negative reduction, the PV ply aside | **329,445 (7.9% of reduced-path searches)** | `lmr_extended`, which counts the PV ply, 395,737 (9.5%); BAS-R07 read 10.9% on the B.10 head |
| of which on a line already holding one / two or more | **98,224 (29.8%) / 71,701 (21.8%)** | TS-1's population: a cap of one removes 51.6% of them |
| interior nodes under a chain of one / two or more | 48.5% / 22.9% | |
| ply-cap returns, search and quiescence | 0 | the cap is a deep-line event; its rate is read in games (§11, BAS-R10) |

### 2.2 The won-ending battery (`probe_*.log`, `raw/`)

BAS-R07's `stall_probe.py` (a fresh process per search, Threads 1, Hash
64, no tables; wall time from `go` to `bestmove`). The scores print
`cp 19871`, the display clamp of the HCE's won tier
(`eval_display_max`), not a tablebase score.

| Position | `60000+600`, no stop | `60000+600`, stop | `180000+2000`, stop |
|---|---|---|---|
| rec1 (KRBP v KR) | **3,778 ms, 0.80× opt, 0.12× max**, d20, 1 bound line | 3,701 ms, same nodes | **37,309 ms, 2.46× opt, 0.36× max**, d30 `mate 18`, 26 bound lines |
| rec2 (KBNPP v K) | 2,272 ms, 0.49× opt, d100 `mate 5` | same nodes | 2,179 ms, 0.15× opt |
| rec3 (KRPP v KR) | 3,026 ms, 0.69× opt, d24 | same nodes | 8,516 ms, 0.60× opt, 5 bound lines |
| KRB v K | 3,469 ms, 0.79× opt, `mate 10` | same nodes | 11,019 ms, 0.78× opt |
| KR v K | 2,856 ms, 0.65× opt, `mate 14` | same nodes | 10,242 ms, 0.73× opt |
| KBN v K | 6,009 ms, 1.38× opt, 0.20× max, `mate 33` | same nodes | 11,012 ms, 0.78× opt |

- **The battery passes with no stop at `60000+600`** (the stop packet's
  rule: no search over 0.5× the maximum). In BAS-R07 the A.8 and B.2
  trees ran rec1 to the maximum and the B.10 head to 16.7 s through a
  mate proof. The identical node counts with and without the stop say
  the stop never fired on any of the six.
- **Why rec1 ends at depth 20.** Its score climbs 2152 → 3600 → 5008 →
  7430 → 8837 (depths 15 to 20, one fail high at 20), then jumps into
  the won tier in a single window, where the ±19 windows around a tier
  score stop failing; the stability scale ends the search at 0.80× the
  optimum. The cascade is not "proved away" by a mate: A.11.9's bare-king
  tiers and A.11.10's damping (after BAS-R07's head) give the root a
  score that stops moving. **The 2026-10-25 falsifier fired** (the head
  finishing rec1 stop-free without a mate proof), so the battery, not
  rec1 alone, re-read H3: it passes at this clock.
- **The hole is real at `180000+2000`.** Rec1's timeline: depth 21, ten
  fail highs between 3.9 and 4.9 s (all on e5e6, the previous best);
  depth 22, five fail highs, then f7d7; depths 23 to 29 no fail; depth
  30, **ten fail highs on f7d7 between 9.7 and 13.6 s, every one before
  the optimum of 15.2 s**, so the stop's clock condition never held;
  then the eleventh window is the give-up window (delta ≥ 900), which
  runs from 13.56 s to 37.3 s and returns `mate 18`. The stop acts only
  when a window returns; this one returned 22 s past the optimum. This is
  §2.3 of the stop packet, on the B.5 entry head, within the battery's
  bound (0.36× the maximum) and 2.46× the optimum. Its rate in games is
  M1's read; its repair is D.1's (A-47), as registered.

### 2.3 Games: where the head spends its clock (`pgn_census_*.txt`)

The B.4.7 gate (a) candidate side (`basilisk-b47-on-pext-pgo`, the tree
of this head) over 2,488 games at `3+0.03`, and the `10+0.1` direction
run's 2,000 games; clocks rebuilt from Colosseum's `t=` comments.

| | `3+0.03` | `10+0.1` |
|---|---:|---:|
| moves | 155,826 | 130,630 |
| moves spending ≥ 0.5× the clock left | 1,181 (0.76%) | 1,973 (1.51%) |
| ≥ 0.7× | 90 (0.06%) | 486 (0.37%) |
| of the ≥ 0.5× moves: lost (≤ −300) / even / won (≥ 300) / mate | 889 / 123 / 152 / 17 | 1,541 / 179 / 223 / 30 |
| draws; by repetition / insufficient material / fifty moves / stalemate | 1,142; 886 (77.6%) / 216 / 27 / 13 | 976; 759 (77.8%) / 185 / 26 / 6 |

The long thinks are fail-low searches in lost positions, three to one
over won ones, as BAS-R07 §2.5 found on A.9.2's games: A-45's
sub-second horizon (D.1), not the stop's. The repetition share of draws
is the input B-84 asked for; whether any of those draws was thrown (a
side that could have played on) needs a judge, registered in §11 as the
blindness read.

## 3. Mechanisms against the donor

Driver symbols are `Searcher::search`'s (`search_worker.cpp`), kernel
symbols `Searcher::negamax`'s. "Arm" is the `B5_ROOT` arm.

| Mechanism | Basilisk today | Donor (`sf_19`) | Disposition on the arm |
|---|---|---|---|
| Root-move state | `prev_pv`, `prev_root_score`, `root_excluded`, the pool's `RootMoveTable` (best per thread, D.2) | `RootMove` per legal root move: `score`, `previousScore`, `averageScore`, `meanSquaredScore`, `effort`, `pv`, `previousPV`, `selDepth`, `tbRank`, bounds; sorted after every window (`:403`, `:493`) | **ADOPT** the record (ticket 2); the pool table stays |
| Root ordering | the picker with the TT move first, plus the tablebase and pool-table ordering scores | the picker with `rootMoves[pvIdx].pv[0]` as the TT move (`:820`); the record's sort decides only the first move and the lines | **ADOPT** the record's previous best as the root's TT move once an iteration completed; before that today's TT move (§14) |
| Window centre | the last iteration's score | the move's effort-weighted average (`:377-379`) | **ADOPT** |
| Window width | `AspirationDelta` 19 | `5 + threadIdx % 8 + |meanSquaredScore| / 10193` (`:376`) | **ADOPT**, converted (§7): `B5AspBase` 1 + `(thread_id % 8) * 282 / 1000` + `|msq| / B5AspMsqDiv` 2874 |
| Growth | `delta += delta / 2` | `delta += 47 * delta / 128` (`:437`) | **ADOPT** as `B5AspGrowth` 47 |
| Fail low | alpha moves; beta stays | `beta = alpha; alpha = max(best − delta, −INF)` (`:420-421`) | **ADOPT**; BAS-D16's 5.8.3 read the midpoint form on the pre-B.2 search; games decide (B-71) |
| Fail high | `beta = min(score + delta, INF)` | `alpha = max(beta − delta, alpha); beta = min(best + delta, INF)` (`:430-431`) | **ADOPT** |
| Give-up hatch | full window at `delta ≥ 900` | none | **REMOVE** on the arm: with the mean-square width a won-tier window is open from its first search (§5.4) |
| Full window at depth ≤ 3 and at a mate score | yes | windows from depth 1 around a `−INF` average | KEEP Basilisk's (identity in substance, §14) |
| Fail-high depth reduction | none | `adjustedDepth = max(1, rootDepth − failedHighCnt − 3·(searchAgainCounter + 1)/4)` (`:392-393`) | `B5FailHighReduce`, default 0: the `failedHighCnt` half only; the clock half is D.1's (B-76). BAS-D17 is the prior |
| Fail-high clock stop | A.8.17's stop at 1× the optimum on the previous best | none | KEEP as shipped behind `B5FailHighStop` (0, or percent of the optimum; default 100, tune builds); M1 decides (BAS-R10) |
| Root reduction | never (`reducible = !is_root`) | root moves after the first reduced at depth ≥ 2 like any PV node (`:1366`) | **ADOPT** behind `B5RootLmr` 1; the first-move step-down and `depth −= 3` stay root-exempt as the donor's are |
| LMR's window term | `− delta·577/root_delta_` | `− delta·577/rootDelta` (`:1887`) | KEEP the form; the null window's unit corrected behind `B5LmrWindowUnits` 1 (§5.1) |
| Negative-reduction ceiling | `new_depth + 2`, `+ is_pv`; unbounded along a line | the same (`:1370`); Stockfish after 19 bounds by ply against root depth; Triumviratus caps the count per line (TS-1) | `B5NegExtMax` 0 / 1 / 2, default 0 (today's and the donor's form); the per-line count in the stack |
| Root bookkeeping | `root_depth_nodes`, `root_best_nodes`, `root_best_effort` per iteration | `rm.effort` across iterations; the EMA weights from the move's share of its own effort (`:1441-1468`) | **ADOPT** the record's effort and averages; the per-iteration effort share stays for the time manager (D.1, §14) |
| Best-move changes | a flip between iterations, halved each iteration | counted inside the iteration, summed over threads (`:1498`, `:564`) | KEEP Basilisk's (D.1's consumer, §14) |
| MultiPV | line 1 with the window, lines 2+ full-window; lines sorted for output | every line its own window from its record; `pvIdx`, `pvLast` by tablebase rank; the stopped-line rules (`:443-491`) | **ADOPT** the per-line window and sort; a stopped line keeps its previous depth as today |
| Forgotten mate | an iteration that loses a proven mate replaces the result | the previous line and score are kept (`:503-547`) | ADOPT if a test can bind it (ticket 8); otherwise recorded, not built |
| Aborted iteration | discarded whole | a completed new best of the partial iteration is kept | KEEP Basilisk's (§14) |
| Optimism | none | `114·avg/(|avg| + 85)` into the NNUE blend (`evaluate.cpp:54-60`) | **NOT BUILT**: an NNUE-blend term, the HCE frozen, Rarog −5.6 (B-83); waits on F (§14) |
| Draw value | 0 | `value_draw` = `−1 + (nodes & 2)` (`:134`) | **NOT BUILT**: the blindness read first (§11, B-84); Rarog −1.2 |
| Ply cap | `ply ≥ MAX_PLY` (128) returns the static evaluation, in check too, before the draw test | the draw test first; at the cap `inCheck ? value_draw : evaluate`; `MAX_PLY` 246 (`:785-789`) | **REPAIR** on both arms: the draw test before the cap, a draw in check at the cap; the cap stays 128 (§5.5) |
| Root TT store with MultiPV | none for lines 2+ | none for `pvIdx > 0` (`:1621`) | KEEP |

## 4. PLAN B.5's inputs, disposed

| Input | Disposition |
|---|---|
| Aspiration on the average with the mean-square width; the `pvIdx` structure with MultiPV | §3: adopted as one arm (tickets 2, 3, 7) |
| Optimism | not built; §1, §14; the producer (the average) is built, the consumer waits on F |
| Root-move effort | the record's effort and EMA weights built; the time manager keeps its per-iteration share until D.1 (§5.6) |
| Deep-iteration line growth (B-80, B-81, B-82, TS-1) | `B5NegExtMax` categorical with the chain census of §2.1 as its population; the correction term's cap is not moved (BAS-R07 §9.3: a tree-size lever, Rarog H0) |
| The fail-high stop: keep, 2×, remove (A-47, A-48, B-79) | the battery passes stop-free at `60000+600`; the hole stands at `180000+2000`; M1 as registered, on the arm, BAS-R10 (§11); removal is still refused until M1 and the battery on the arm say otherwise |
| Reads owed by the stop packet §9 | rec1 stop-free on the head: done (§2.2, the falsifier fired); rec3 at `180000+2000` with the stop: 8.5 s, 0.60× the optimum, no hole; the 2.63-ply cap's share and M1's 2× arm: BAS-R10 |
| B-75 score-scaled first window | the donor's mean-square width is that window; adopted inside the arm, not as a standalone candidate |
| B-76 fail-high depth reduction with its clock half | `B5FailHighReduce` default 0; the clock half is D.1's |
| B-70 cost in nodes | read: 53.7% of windowed nodes in failed windows; the counters stay (ticket 0) |
| B-71, B-72 windows by games only | the whole-arm paired run and the gates decide; the zero-game screens describe |
| B-73 the window–LMR coupling | §5.1: the null window's unit is corrected so the fitted LMR regime does not move with the window |
| B-83 optimism after A.11.1 | not built (§3) |
| B-84 draw blindness first | the repetition share is 77.6% / 77.8% of draws (§2.3); the judge read is registered (§11); no jitter |
| B-85 MultiPV lines 2+ | ticket 7 |
| S4 root reductions | 44.1% of root moves after the first, 2.0 plies: ticket 4, `B5RootLmr` |
| S6 the root's first move | not a 1T defect (§2.1); the record's line is adopted with the record; the 4T read is D.2's with the counters ticket 0 keeps |
| TS-1 chain cap | `B5NegExtMax`; chains of two or more are material (21.8% of extensions), so the paired run is owed (§9) |
| TS-15 priors | no decisive-fail-high rule, no depth recovery, no cap on root reductions |
| NODE-8 the ply cap | ticket 1 on both arms; the counter at game controls rides on M1; the raise waits (§5.5) |
| TR-4 clean child frame | ticket 6's node checks and test |
| A-25 deep-line stack test | ticket 9 |
| Denser conversion floor (maintainer 2026-11-06) | registered in §11, the grid tool promoted in ticket 0 |
| Departures table for B.6.1 | §14 |

## 5. Interactions with the fitted cores and the time manager

### 5.1 The LMR window term is not unit-free (B-73)

`lmr_reduction` subtracts `delta·577/root_delta_`. At a PV node `delta`
is the window handed down, about `root_delta_`, so the term is about 577
whatever the units. At a null-window node `delta` is 1: **one unit of the
engine's scale**. The donor's unit is 0.282 of Basilisk's (B.0's measured
ratio, which the kernel applies as `× 1000 / 282`), so the donor's null
window is 0.282 of ours against the same root window. Today's ±19 hides
this: at a root window of 38 the term reads 577/38 = 15, and the donor at
a one-pawn score (window ±17 of its units, 35 wide) reads 577/35 = 16.
The two engines sit in the same regime by coincidence. With the arm's
windows (±1 to ±9 in balanced middlegames) the uncorrected term would read
32 to 290 at every null-window node, 0.03 to 0.28 plies less reduction
than Clusters 1 to 3 were fitted with. The arm corrects the null window's
unit: `(delta > 1 ? delta·577 : 163) / root_delta_` (163 = 577 × 0.282),
behind `B5LmrWindowUnits` (1 corrected, 0 today's). This is the one place
B.5 touches a Cluster 1 formula, and it is to keep it where the fits left
it. The surface rule (§5.8) reads the LMR coordinates' shares either way.

### 5.2 Root reduction against the fitted LMR terms and the time manager

The root is a PV node at the iteration's depth. Its moves after the first
carry the same `r` as any PV node's (`tt_pv` terms, cut-node and
children's cutoff terms, history, the window term) and today none of it
is applied. With `B5RootLmr` 1 a reduced root move searches at
`new_depth − r/1024 + 1`, re-searches at full depth when it fails high
and then with the full window, as every PV node's moves do. Two
consumers move:
- **The time manager's effort scale.** `root_best_effort` is the best
  move's share of the iteration's nodes; reduced siblings shrink the
  denominator, so the share rises and `TmEffortHi` (80%, ×0.80 time)
  fires more often. The constants were tuned with unreduced root moves.
  B.5's screens read the effort distribution on both arms (§11, P11);
  the constants are D.1's.
- **The WAC floor.** BAS-D17 found a root move searched shallower misses
  the tactical shot that made it fail high. A reduced root move that
  fails high is re-searched at full depth, so the risk is a reduced
  fail-low on a shot the full depth would have found. The arm's WAC at
  100,000 nodes is a veto (≥ 246; the head reads 254), and the pricing
  instrument's `LmrFailLow` cell at the root depth prices what the root's
  reductions hide (§15).

### 5.3 The negative-reduction cap against the extension budget and the won endings

B.3 keeps negative reductions outside the per-line extension budget
(`B3ExtBudget` bounds positive singular plies by the root depth, packet
§5.10). A chain of negative reductions is the other way a line runs past
the root depth, and §2.1 says 22.9% of interior nodes sit under a chain
of two or more. The cap at 1 leaves every first extension and removes
51.6% of the extensions; Triumviratus read −6.6% nodes and −0.55 plies
of selective depth at fixed depth, +3.88 ± 3.26 in games. In won endings
BAS-R07 measured the extensions and the correction term inflating trees
5 to 30×; the cap bounds the former. The deep-iteration cost screen
reads it at depths 16 to 19 beside the depth-14 census (§11, P5). The
correction term's own cap (2.63 plies) is not moved: Rarog's one-ply cap
read H0 and BAS-R07 §9.3 calls it a tree-size lever.

### 5.4 The windows against the fail-high stop and the hole

With the mean-square width, a root score in the HCE's won tier (about
20,000 and above internally) gives `|msq|/2874` far beyond any score, so
every window there is open: no fail high, no cascade, no give-up window
and no stop firing in won endings. The cost moves into the iteration
itself. On rec1 at `180000+2000` (§2.2) the depth-30 window that proved
`mate 18` took 24 s as a give-up window; on the arm the depth-30
iteration is that same open-window search from its first node, and
nothing but the hard maximum ends an iteration in progress. So the arm
does not shorten that think; it removes the ten cheap fail highs before
it. The hole is a time-management question (D.1, A-47), and M1 reads its
rate on the arm. In middlegames the stop's population is the 73.6% of
root fail highs that land on the previous best (§2.1); with narrower
windows more of them will occur, each cheaper. P8 predicts the rate.

### 5.5 The ply cap against the score bands and the table

`MAX_PLY` 128 defines the mate band (`MATE_SCORE − MAX_PLY` = 31,872),
`tablebaseValue` (31,871) and `tablebaseWinInMaxPly` (31,743), and sizes
`pv_table`, `pv_len`, `move_buffers` and the stack. Raising it to the
donor's 246 would put `tablebaseWinInMaxPly` at 31,507, **below the
HCE's KBNK drive (31,660)**, so a static won-tier score would read as a
tablebase win; and the table's `depth` is an `int8_t`, so the tablebase
store at `min(MAX_PLY − 1, depth + 6)` would overflow at 245. The raise
waits on the evaluation's bands (Phase C) and the table's layout (D.2,
TP-1); §14 records it. The repair that does not wait: the draw test
before the cap, and a draw in check at the cap, in both searches. It
cannot change `bench 13` (0 cap returns at depth 14), which the counter
proves at every commit.

### 5.6 The record against the pool and the time manager

- The pool's `RootMoveTable` (the best move per thread by depth and
  score, the merge, the helpers' root filter) is D.2's and stays. The
  record is per thread and honours `root_filter_index`.
- The record's average is the producer D.1's `fallingEval` reads (the
  donor's `bestPreviousAverageScore` across moves); B.5 builds the
  producer and carries nothing across `go`s. The donor's
  `bestMoveChanges`, counted inside an iteration and summed over
  threads, and `highBestMoveEffort` from the accumulated effort are D.1's
  consumers; Basilisk's `best_move_changes` and per-iteration
  `root_best_effort` stay until D.1 fits the clock.
- `root_score_` (the `seek_mate` input of B.3 and B.4) reads the record's
  previous score of line 1; the value is today's.

### 5.7 MultiPV

Lines 2 and later take their windows from their own records (B-85), so
an analysis of a position with one crushing move no longer searches the
alternatives at full width. The identity tests at `MultiPV 1`
(`test_multipv`: line for line against the single-PV search, three
ordered lines, the searchmoves cap, Threads 4) bind the arm. The
stopped-line rules of the donor (`:443-491`) are not adopted: a line a
stop cuts short keeps its previous depth, as today.

### 5.8 The surface rule

The arm's own coordinates (`B5AspBase`, `B5AspMsqDiv`, `B5AspGrowth`)
form the surface. A Cluster 1, 2 or 3 coordinate joins when the arm
moves its mechanism's firing share by 10% of itself or more at depth 14
on `suite_v2` (B.3.0 §5.12's rule, applied in B.4.5), the LMR
coordinates first (`CoreLmrOffset`, `CoreLmrTtPv`, `CoreResearchDeeper`
through the window term and the root's reductions), then
`B3NmpPerPly` (B.4.6 left the null move's entry unsettled). Time
constants never join (D.1's).

## 6. Zero-game reads, done

§2's reads, with what each decides:

| Read | Result | Rule it sets |
|---|---|---|
| Nodes in failed windows (B-70) | 53.7% of windowed nodes | none: the window is adopted as a unit and games decide; P3 sizes the change |
| Root reduction (S4) | 44.1% of root moves after the first, 2.0 plies | `B5RootLmr` 1 in the arm, with the WAC veto and the pricing read |
| Root TT move (S6) | 0% / 1.1% / 26.4% by the previous window's outcome | the record's line is adopted with the record; no 1T defect to repair |
| Negative-reduction chains (TS-1) | 29.8% on a line holding one, 21.8% holding two or more | `B5NegExtMax`'s paired run is owed (chains of two or more are material) |
| Ply cap at depth 14 | 0 | the repair lands bench-neutral; the game-control rate rides on M1 |
| The battery stop-free at `60000+600` | passes, 0.21× the maximum at most | removal of the stop stays refused until M1 and the battery on the arm; the hole is confirmed at `180000+2000` |
| Long thinks in games | 0.76% / 1.51% of moves at ≥ 0.5× the clock, three to one in lost positions | D.1's; B.5's windows are not the lever |
| Draws by repetition | 77.6% / 77.8% of draws | the blindness read is registered before any jitter |

## 7. Seeds

One donor unit is 0.282 of a Basilisk unit (B.0's ratio; the kernel's
`× 1000 / 282`). Counts and weights are not converted.

| Coordinate | Donor | Converted | Seed | Range | Kind |
|---|---:|---:|---:|---|---|
| `B5AspBase`, the window's base | 5 | 1.41 | **1** | 1–40 | X |
| `B5AspMsqDiv`, the mean square's divisor | 10193 | 10193 × 0.282 = 2874 (the mean square is in squared units, so the divisor converts once) | **2874** | 1000–12000 | X |
| `B5AspGrowth`, per 128 of the width per failure | 47 | — | **47** | 32–96 | X (today's form is 64) |
| per-thread offset | `threadIdx % 8` | `(thread_id % 8) × 282 / 1000` | fixed | — | the donor's term, inert at one thread; D.2 may change it |
| EMA weights | `w = clamp(32·N·2 / (2N + 3·E_prev), 12, 24)`, `w_msq = min(w, 16)`, scale 32 | — | fixed constants | — | not advertised, as the 577 is not |
| `B5FailHighReduce` | `failedHighCnt` | — | **0** | 0–1 | S |
| `B5NegExtMax` | — (Triumviratus 1) | — | **0** | 0–2 | S |
| `B5RootLmr` | reduces | — | **1** | 0–1 | S |
| `B5Window` | the donor's loop | — | **1** | 0–1 | S (attribution: 0 is today's loop on the record) |
| `B5LmrWindowUnits` | — | 577 × 0.282 = 163 | **1** | 0–1 | S |
| `B5FailHighStop` | none | — | **100** (percent of the optimum; 0 off) | 0–400 | S (tune builds; the shared driver) |

The window at the seeds, in this engine's units: ±1 at a level score, ±2
at half a pawn, ±5 at a pawn, ±9 at a pawn and a half, ±32 at three
pawns, ±350 at ten, open in the won tier.

## 8. Decision: `READY_FOR_IMPLEMENTATION` (2026-11-08)

- The defect and the gaps are measured on this head (§2): half the
  windowed nodes in failed windows, the root never reduced, chained
  negative-reduction extensions under half the tree, the ply cap's
  in-check return, the hole at long clocks.
- The mechanism is the donor's root cluster as a unit (§3), with the one
  unit correction §5.1 derives and the three decisions §1 lists as not
  built, each with its reason and its waiting place (§14).
- The interactions are mapped with numbers (§5) and the fit's surface
  rule is set (§5.8).
- The falsifiers are registered (§11): the unfitted paired run, the WAC
  and conversion floors, the cost screen, M1's rates, the categoricals'
  brackets, the pooled NPS.
- Research amendments answer any return, as B.3 and B.4 did.

## 9. Categoricals, attribution switches and the stop's option

Tune-build UCI options, each categorical settled by one 2,000-game
paired run on one tune build at `3+0.03` (PROCESS *Cluster delivery
shape*: adopted at +5 Elo or more, rejected at −5 or less, each only
when the 95% interval also excludes 0; left at the default otherwise;
never an SPSA coordinate). The whole-arm paired run goes first, on the
defaults; the categoricals follow it. Arm A is the alternative, arm B
the default.

| # | Option | Values | Default | What decides | Bracket |
|---|---|---|---|---|---|
| 1 | `B5NegExtMax` | 0 unbounded (today's and the donor's); 1 or 2 extensions from a negative reduction per line | 0 | first the zero-game reads at 1: nodes at depth 14 and the cost screen at depths 16 to 19 (P5); then a paired run, 1 against 0. 2 is read only if 1 is adopted | ±5 |
| 2 | `B5FailHighReduce` | 1 the re-search after a fail high runs `max(1, depth − failed_high_cnt)` plies; 0 full depth | 0 | the gate first: on the arm at depth 14, nodes in fail-high re-searches above 20% of windowed nodes. A paired run, 1 against 0, with the WAC floor as veto, only past the gate; otherwise it closes at 0 with that reading, and D.1 owns the clock half | ±5 |

Attribution switches. Their defaults are the arm's form; a paired run
never moves them. They exist so that a negative whole-arm run is ablated
by switch before any mechanism is blamed, and so a floor can read one
rule alone. The ablation order on a failed P1 is `B5Window`,
`B5RootLmr`, `B5LmrWindowUnits`.

| Option | Values | Default |
|---|---|---|
| `B5Window` | 1 the donor's loop on the record (§3); 0 today's loop (±`AspirationDelta`, growth by half, alpha alone after a fail low, the give-up hatch) on the record | 1 |
| `B5RootLmr` | 1 root moves after the first reduced at depth ≥ 2; 0 never | 1 |
| `B5LmrWindowUnits` | 1 the null window's unit corrected (163); 0 today's 577 at every node | 1 |

The stop's option, `B5FailHighStop` (0 off; otherwise the percent of
the optimum past which a fail high on the previous best ends the
search), lives in the shared driver, defaults to 100 (today's stop) and
is advertised by tune builds, so M1's three arms are one binary. It is
settled by M1's registered rule (`analysis/b5_failhigh_stop_2026-10-25.md`
§5), not by this table. No deletion arm: the give-up hatch goes with
`B5Window` 1 and comes back with 0.

## 10. The B.5 implementation contract (`I2`)

- **Goal and exact semantics.** §3's "Disposition" column with §7's
  seeds and §9's defaults, behind one CMake `option(B5_ROOT ... OFF)`
  (compile definition `BASILISK_B5_ROOT`, `constexpr bool kB5Root`),
  flipped ON in one commit after the gates. Both arms build at every
  commit. The OFF arm reproduces `bench 13` **4,144,790** at every
  commit; the ON arm's fingerprint is recorded per ticket. Engineering
  is the implementer's. The preferred form is B.3's and B.4's: `if
  constexpr` branches in the existing driver and kernel, both arms
  compiling on every build. The OFF arm's code is edited only for seams
  both arms share: counters, the stop's option, the ply-cap repair,
  parameters the OFF arm leaves at their defaults.
- **Why here.** §1, §2, §5.
- **Producer → stored state → consumer.** The record (per thread, per
  `go`): the kernel's root loop writes a move's effort, average, mean
  square, score, bounds, line and selective depth after each root
  search; the driver reads the line's average and mean square for the
  window, its previous line for `follow_pv` and the root's TT move, its
  score for the result and the pool table, and the sorted lines for
  MultiPV output. New stack state: `negext_path` and `negext_taken`
  (ticket 6), a parent writes for every child as `ext_plies` is written.
  The driver's `root_delta_`, `root_depth_` and `root_score_` keep their
  meaning.
- **Files and subsystems.** `src/search_worker.cpp`, `.h` (the loop, the
  record, MultiPV, the stop's option), `src/search_kernel_core.cpp` (the
  root loop's bookkeeping and filter, root reduction, the window term,
  the negative-reduction cap, the ply cap), `src/search_root.h`, `.cpp`
  (the record's type, if it belongs beside the pool table),
  `src/search_types.h` (`kB5Root`, the stack fields), a `src/search_b5.h`
  for the pure helpers (the width, the adjusted depth, the EMA step, the
  ceiling), `src/search_params.h` and `src/parameters.cpp`
  (`BASILISK_B5_PARAMS(X, F, S)`), `src/search_diagnostics.h`, `.cpp`
  (counters, kv lines), `CMakeLists.txt`, `.github/workflows/ci.yml`,
  `tools/diag/run_suite.py`, `tools/generate_spsa_surface.py` (a B5
  table), `tools/diag/replay_games.py` and `tools/diag/floors_grid.py`
  (new, ticket 0), `tests/`.
- **Invariants that must not change.** DESIGN §3:
  - the mate band, the tablebase band and `MAX_PLY` are untouched; no
    static score enters either band;
  - a draw is 0, the ply cap in check included; the draw test precedes
    the cap at every interior node;
  - table scores are ply-adjusted on store and probe; the root stores
    nothing for a MultiPV line after the first;
  - `bestmove` and `ponder` are legal from the root (`sanitize_search_result`),
    and the last line before `bestmove` names the move played
    (`test_stopped_fail_high_line_names_the_move_played`);
  - a stop discards the iteration in progress; the result is the last
    completed iteration's, and the fail-high stop plays that move;
  - `MultiPV 1` reproduces the single-PV search line for line; lines are
    distinct moves, best first;
  - the per-ply stack unwinds and starts every child clean on every
    entry path (ticket 6's node checks);
  - the search is allocation-free below the root; the record is sized
    at the root once per `go`;
  - the conversion floors, the mate fixtures, the won-ending clock test
    (rec1 at `30000+300` under 8 s) and the WAC floor bind both arms at
    every ticket that changes a fingerprint.
- **Maintainer-owned expensive gate.** The diagnostics leaf's paired runs
  and M1 (BAS-S53, BAS-R10), then the gates (BAS-S55), §11.
- **Non-goals and adjacent mechanisms not to alter.** The time manager's
  budget, scales and constants (D.1); the stop's threshold (M1's rule);
  the hole's repair (D.1); the pool, the merge and the helpers' root
  filter (D.2); optimism and the draw value; quiescence, the move loop's
  pruning, the null move, ProbCut, singular extensions, the histories,
  the correction, the table and its replacement. No constant outside §7
  is moved by hand; no LMR term changes except §5.1's unit. No tuning
  before the fit leaf. A coordinate §5.8 pulls onto the surface is
  fitted there, never adjusted here.

### The arm's root, stated once

**Before the first iteration** the record holds every legal root move
the limits allow (searchmoves, the tablebase filter, the helper's
filter), in generation order, with `score`, `previous_score`, `average`
at the "none" sentinel and `mean_sq` likewise, `effort` 0, `pv` of one
move.

**Each iteration** saves every move's `score` and `pv` as its
`previous_score` and `previous_pv` (the donor's `:349-351`), then, for
each line `k` from 1 to the lines to search:

- the line's record is the first not yet searched this iteration, after
  the sort; its `previous_pv` is the line `follow_pv` follows and its
  first move is the root's TT move (today's TT move before any
  iteration completed);
- at depth ≤ 3, or when the line's previous score is a mate score, the
  window is open as today; otherwise `delta = B5AspBase + (thread_id %
  8) × 282 / 1000 + |mean_sq| / B5AspMsqDiv`, `alpha = max(average −
  delta, −INF)`, `beta = min(average + delta, INF)`, with `average` the
  line's; `failed_high_cnt = 0`;
- the loop: `root_search(adjusted depth, alpha, beta)` with the adjusted
  depth the iteration's, less `failed_high_cnt` only when
  `B5FailHighReduce` is 1, never below 1; `root_delta_` is `beta −
  alpha`; then a fail low sets `beta = alpha`, `alpha = max(score −
  delta, −INF)` and `failed_high_cnt = 0`; a fail high sets `alpha =
  max(beta − delta, alpha)`, `beta = min(score + delta, INF)` and
  `++failed_high_cnt`, after the bound line and the stop's test as
  today (the stop's threshold `B5FailHighStop`/100 of the optimum; 0
  never); then `delta += B5AspGrowth × delta / 128`; a window inside
  its bounds ends the loop. There is no give-up hatch. A stop ends the
  loop as today.
- inside `negamax<Root>` every root move the record admits is searched
  as a PV node's move is, the first at full depth and width, the later
  ones null-window, **reduced when `B5RootLmr` is 1 and `depth ≥ 2`**
  (the first-move step-down and `depth −= 3` stay root-exempt); after
  each root move its record adds the move's nodes to `effort` and
  blends the value into `average` and `mean_sq` with the donor's weights
  (`:1441-1468`); the first move or a move that raised alpha writes its
  `score`, bounds, `pv` and `sel_depth`, every other move's `score` is
  the "none" sentinel; the root's TT store is today's;
- after the window, the records from the line's index on are stably
  sorted by `score`, then by `previous_score`; line `k`'s record is the
  first.

**After the lines** the result is line 1's record (move, line, score,
depth, selective depth), the pool table takes its raw score as today,
the lines are reported best first, and the soft stop reads today's
signals. A stop inside an iteration discards it, as today.

### Tickets

Ordered; one mechanism each; every ticket keeps the OFF arm's exact
fingerprint and records the ON arm's. From ticket 2 on, each ticket ends
with the ON arm's `bench 13` counters at stride 1 (Diag build) against
the previous ticket's. A Cluster 1, 2 or 3 row whose share moves by more
than 20% of itself is reported with its cause in the implementation
record and not repaired. The floors grid, the mate fixtures, the
won-ending clock test and `test_wac --count` are read on the ON arm at
every ticket that changes its fingerprint.

0. **The switch and the instruments** (both arms 4,144,790: the arm is
   empty).
   - The option, the definition and `kB5Root`; a CI cell that configures
     `-DB5_ROOT=ON`, builds `all`, runs CTest and prints `bench 13`; the
     `BASILISK_B5_PARAMS` group with §7's lines and §9's switches,
     advertised by tune builds; the B5 table in
     `generate_spsa_surface.py`; `B5FailHighStop` on the shared driver.
   - Counters, appended and never renamed, each with its denominator in
     the table's comment and its unit in `run_suite.py`, in the census
     patch's shape (`tools/results/b5-20261108/census.patch`):
     `ply_cap_hits`, `ply_cap_in_check`, `qs_ply_cap_hits`,
     `qs_ply_cap_in_check`, `negext_total`, `negext_chain1`,
     `negext_chain2p`, `root_lmr_reduced`, `root_lmr_researched`,
     `root_tt_probe_first`, `root_tt_mismatch_first`, `root_tt_probe_fl`,
     `root_tt_mismatch_fl`, `root_tt_probe_fh`, `root_tt_mismatch_fh`,
     `asp_nodes_windowed`, `asp_nodes_failed`, `asp_nodes_giveup`,
     `asp_fail_high_research_nodes`, `fhstop_fired`, `root_effort_hi`
     (iterations whose best effort reached `TmEffortHi`), `root_effort_lo`;
     a `kv` line for them. The census patch's definitions are the
     reference (the PV ply aside for `negext_*`).
   - `tools/diag/replay_games.py`: one side of a Colosseum PGN replayed
     in one engine process per game (`ucinewgame`, `position ... moves`,
     `go` with both clocks rebuilt from the `t=` comments and the
     header's control), `TM_Debug` and `Diag` on, one row per search
     (game, ply, clock, increment, elapsed, optimum, maximum, score,
     depth, the aspiration counters, `fhstop_fired`, the ply-cap
     counters, the longest window's start and length). A test
     reconstructs the clocks of three games against the PGN's comments
     to the millisecond; a dry run of three games on the head.
   - `tools/diag/floors_grid.py`, promoted from
     `tools/results/b44-20261106/amend2/floors.py`: the four families on
     the 25-budget grid through `test_endgames --conversion --nodes N`,
     every playout named, `floors.json`, and the §11 bound applied as a
     pass or fail line.
1. **The ply cap** (both arms). At an interior node the draw test runs
   before the cap; at the cap, in check, the search returns 0, and
   quiescence likewise; `pv_len` is not indexed at the cap. A
   deterministic test through a lowered cap in the pattern of
   `SearchLimits::ext_budget_cap` (held in a kernel member set at
   `kernel_begin_search`, so the hot path reads one member): a position
   whose side to move is in check at the lowered cap reads 0 and a
   position not in check reads its static evaluation; a drawn position
   at the cap reads 0 on both. Both arms 4,144,790, and the counters
   read 0 at `bench 13`.
2. **The record.** `RootMove` and its vector, the saving of the previous
   scores and lines, the sort, the kernel's filter by the record and its
   bookkeeping after each root move (the donor's `:1437-1500`),
   `follow_pv` from the record's line, the root's TT move from it once
   an iteration completed, the result and the pool table from it. The
   driver's loop is still today's (`B5Window` is built in ticket 3; the
   arm runs today's loop on the record until then). Tests: identity with
   the OFF arm on `bench 13` positions at depth 10 where the counters
   show no root TT mismatch after a fail low, and a recorded fingerprint
   where they do; `test_search_respects_root_move_restrictions`; the
   tablebase root tests. Read: `root_tt_mismatch_*` on the arm.
3. **The window.** The loop stated above behind `B5Window`, `root_delta_`
   from it, `B5LmrWindowUnits` in `lmr_reduction`. Tests (`search_b5.h`
   helpers, pure): the width at the seeds and at the ranges' edges, the
   clamps at ±INF, the growth, the fail-low and fail-high bounds, the
   corrected term at `delta` 1 and at `delta` = `root_delta_`; the
   engine tests: a mate score keeps the open window; the bound lines
   print as today; the won-ending clock test. Reads: the depth-14 census
   (re-searches per window, failed-node share) and `bench 13` on the
   arm.
4. **Root reduction.** `B5RootLmr`: the reduced-search path admits the
   root for `searched > 0 && depth ≥ 2`; the counters. Tests: a reduced
   root move that fails high is searched again at full depth and then
   with the full window (the trace at the root, `searchmoves` of two
   moves); `test_wac --count` at 100,000 nodes on the arm, recorded.
   Reads: nodes to depth 14 against ticket 3, `root_lmr_*`,
   `root_effort_*`.
5. **The fail-high depth reduction.** `B5FailHighReduce` and
   `b5::adjusted_depth(depth, failed_high_cnt, on)`, tested as a pure
   function at 0 and 1 and at the floor of 1; the fingerprint at 1
   recorded. Read: `asp_fail_high_research_nodes` on the arm (§9 item
   2's gate).
6. **The negative-reduction cap.** `negext_path` and `negext_taken` in
   the stack, written as `ext_plies` and `ext_taken` are; the ceiling
   `new_depth + (B5NegExtMax == 0 || negext_path < B5NegExtMax ? 2 : 0)`;
   node checks `is_root || negext_path == (ss − 1)->negext_path +
   (ss − 1)->negext_taken` and `negext_taken == 0` at entry. Tests: the
   TR-4 clean-frame test, the Diag build at depth 8 over 20 positions of
   `suite_v2` with every entry path (the null move, ProbCut, the
   singular verification, quiescence's drop) exercised, and a planted
   stale `negext_taken` failing the check; at 1 the counters show no
   extension on a line already holding one. Reads: nodes at depth 14 and
   the cost screen at 0, 1 and 2.
7. **MultiPV on the record.** Each line its own window from its record;
   the stable sort of the lines by score, then previous score; the
   report best first. Tests: `test_multipv` as it stands (identity at 1
   line for line; three ordered lines of distinct moves; the searchmoves
   and legal-move caps; Threads 4's first line is `bestmove`; the stop
   under `go infinite` answers once).
8. **The forgotten mate.** The donor's guard (`:503-547`): an iteration
   whose line 1 loses a proven mate, or ends inexact, keeps the previous
   line and score. Lands only with a test that fails without it (a
   position where a shallower iteration proves a mate the next loses, or
   a stop-driven callback in the shape of
   `test_stopped_fail_high_line_names_the_move_played`); otherwise the
   implementation record says no fixture was found and §14 gains the
   row, and nothing is built.
9. **The final state.** The Ablate flavour's bits re-declared if any
   names a changed rule; the liveness proof of every switch and
   coordinate on the arm (`bench 13` moves for each); the sanitizer suite
   on both arms; the two-arm table in B.3's and B.4's shape (`bench 13`
   on Release, Tune, Diag and Ablate, `bench 10` sanitizer, CTest, WAC at
   depth 6 and at 100,000 nodes); the deep-line read (A-25): a search on
   the Diag build that reaches `ply_cap_hits > 0` within 10 s on a
   position the implementer finds (B.12's KBP-K reached seldepth 112),
   recorded with the thread's stack size; if none reaches it, the
   per-frame stack use of `negamax` and `quiescence` is read from the
   build and the margin at 128 plies recorded; CI green on both arms.

### Cheap qualification (end of the implementation leaf, before the review)

- The two-arm table, every fingerprint reproduced from a clean tree.
- The arm's WAC at 100,000 nodes **≥ 246** (the head reads 254) and the
  depth-6 floor.
- The floors grid on the arm: KQ-K, KR-K and KBB-K **≥ 598 of 600** each;
  KBN-K **≥ 575 of 600** and the five registered budgets' sum **≥ 112**;
  the wrong-bishop draws.
- The deep-iteration cost screen on B.3's five positions (bench 11 and
  15, `suite_v2` 50, 42 and 60), ON against OFF, by the clause of B.3.0's
  research amendment 7: a geometric mean of at most 3 and no depth the
  OFF arm finishes left unfinished.
- The won-ending clock test on the arm (rec1 at `30000+300` under 8 s,
  the win kept), and the battery of §2.2 at `60000+600` on the arm with
  `B5FailHighStop` 0: no search over 0.5× the maximum (recorded; the
  stop's decision stays M1's).
- The decision trace on B.2's four positions, both arms, archived.
- Tickets 2, 4, 5 and 6's reads with their figures.

### Review

A separate session reproduces every fingerprint and test on both arms,
reads each ticket's diff against this section and §7, checks the
invariants above, and records its acceptance or return in this packet
before the diagnostics leaf starts.

## 11. Frozen predictions and the registrations

Frozen 2026-11-08, before any ON-arm build exists. Calibration is
appended in the experiment entries, never here. IDs reserved: **BAS-R10**
(M1, the fail-high stop's firing read), **BAS-S53** (the diagnostics, M1
and the unfitted paired run), **BAS-S54** (the curvature sweep, then
SPSA), **BAS-S55** (the Cluster 4 gates); each entry file is created at
its leaf's opening from the text below, before any game.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, ON against OFF at one revision, §7's seeds, §9's defaults) reads **+4 Elo**, 80% interval [−12, +20]; probability positive 60% | low-moderate | below −20 is a defect or a wrong seed: ablate by switch in the order `B5Window`, `B5RootLmr`, `B5LmrWindowUnits`, then `B5AspBase` 19 with `B5AspMsqDiv` at its top |
| P2 | After the fit and the gate, B.5 reads **+8 Elo** at STC, 90% interval [−3, +20]; H1 at `[0,3]` probability 55% | low-moderate | H0 with P3 to P7 held says the donor's root cluster carries little on this HCE; the arm stays off and B.6 proceeds |
| P3 | Windows at depth 14 on `suite_v2`: re-searches per window rise from 1.50 to **2.0 to 3.5**; nodes in failed windows fall from 53.7% to **≤ 48%** of windowed nodes; give-up nodes 0 | low | a failed-node share above 58% says the narrow windows cost more than they save: `B5AspBase`'s sweep is read before the whole-arm run is blamed |
| P4 | Root reduction: nodes to depth 14, ON against OFF, geometric mean **0.80 to 0.97**; 5 to 15% of reduced root searches are searched again | moderate | a mean above 1.00 says the root's re-searches eat the saving: `B5RootLmr` 0 is the first ablation |
| P5 | `B5NegExtMax` 1 at fixed depth: nodes at depth 14 **−4 to −8%**, mean selective depth −0.3 to −0.8 plies; the cost screen ≤ 3; the paired run within ±6 (60%) | moderate | a node change inside ±2% says the chains are cheap here, and item 1 closes at 0 without a game |
| P6 | Tactics: the arm's WAC at 100,000 nodes in **[246, 262]** | moderate | under 246 is a floor failure and returns the leaf; `B5RootLmr` 0 is read first (BAS-D17) |
| P7 | Floors: the grid passes as registered on the arm | moderate | a miss attributes by switch, zero-game: `B5RootLmr`, `B5Window`, `B5NegExtMax`, `B5LmrWindowUnits` |
| P8 | M1 on the arm at `3+0.03`: the stop fires on **1 to 6%** of searches (the stop packet registered 5 to 20% on the old head; the mean-square width opens won-tier windows), removes **≤ 2%** of spend; the hole on **≤ 0.3%** of searches; 2× fires on ≤ 1% | low | outside the ranges the stop packet's §5 rule decides as registered; nothing here changes it |
| P9 | The ply cap in games: **≤ 0.05** cap returns per game at `3+0.03`, ≤ 1 per game at `60+0.6`, with the in-check share ≤ 30% | low | a rate above 1 per game at `3+0.03` makes the raise a Phase C item with the bands; none here |
| P10 | Speed: pooled NPS **0.99 to 1.01×** the OFF arm (per-iteration bookkeeping; two stack stores per node) | moderate-high | below 0.98 returns the record's bookkeeping for its cost before the gate |
| P11 | Time: the share of iterations whose best effort reaches `TmEffortHi` rises by **≤ 10 points** on the arm | low-moderate | above 10 points is recorded for D.1 and the direction run (d) is read beside it |
| P12 | Categoricals: item 1 within ±6 (60%); item 2's gate not met (70%), and within ±5 if met | low-moderate | item 1 at +5 or more with its interval excluding 0 adopts the cap and re-reads the floors |

Stop rules: P1 below −20 (then the ablation order above); a floor
failure at any ticket returns the leaf; PLAN rule 6 for the programme.

### The denser conversion floor (registered here, maintainer 2026-11-06)

For this cluster and the later ones, the floor is the four families over
the 25-budget grid (25,000 to 135,000 nodes; `floors_grid.py`), 600
playouts a family a tree:
- KQ-K, KR-K and KBB-K: **at least 598 of 600** each. B.4's read gave the
  healthy tree one miss in 1,800 major-piece playouts, so one more than
  that in one family false-alarms about one read in twenty-five.
- KBN-K: **at least 575 of 600**, and the five registered budgets'
  (30k, 45k, 60k, 90k, 120k) sum **at least 112**, the registered floor
  kept. B.4's trees read 587 and 577 of 600.
A miss returns the leaf, and the return's first read is the grid on the
failing tree and the previous ticket's, before any mechanism is blamed.
The five-budget read of 24 of 24 is no longer a verdict on its own.

### BAS-R10 — M1: the fail-high stop's firing read, on the arm

- Instrument: `replay_games.py` (ticket 0) on a Diag build of the
  accepted arm, `B5FailHighStop` 100, 200 and 0 as the three arms on one
  binary, and the OFF arm at 100 as the reference column.
- Corpus: 300 games of the B.4.7 gate (a) candidate side at `3+0.03`
  (`tools/results/b47-gate-a/games.pgn`) and 100 of the `10+0.1`
  direction run (`b47-direction-ltc/games.pgn`), the first games in
  file order, one thread.
- Outputs and the decision rule: the stop packet §4 and §5 as
  registered (the firing rate by score band, the time removed, the
  hole's rate; keep 1× under 10% of searches and 3% of spend; otherwise
  2× against 1× at `[0,3]` with the battery; removal only if the battery
  passes stop-free on the arm, then `[−5,5]`), plus the ply-cap counters
  per game (P9). Maintainer-run, about five hours.

### BAS-S53 — the diagnostics, M1 and the unfitted paired run

- Baseline / candidate: one revision, the accepted implementation head;
  the OFF arm against the ON arm, final-PGO pairs.
- Zero-game screens, both arms: the depth-14 census and the counters of
  ticket 0; `Pricing 4` at depth 16 with the `LmrFailLow` cell at the
  root depth; the cost screen; WAC at 100,000; the floors grid; the
  battery at `60000+600` with the stop off; the blindness read: the
  final positions of the repetition draws of the gate (a) PGN judged by
  Stockfish 19 at fixed nodes with `loss_by_cell.py`'s judge, the share
  where the repeating side stood ≥ 150 cp ahead by the judge, recorded;
  above 5% the draw value becomes an H.2 card with that figure.
- M1 (BAS-R10), maintainer-run.
- The pooled NPS read (P10), on an idle host the maintainer gives.
- The whole-arm paired run on the defaults, then §9's items in order;
  P1 to P7 and P10 to P12 scored. A moved default rebuilds and re-pins
  the arm before the fit.

### BAS-S54 — the curvature sweep, then SPSA in blocks

- Surface: `B5AspBase` (1 to 19 swept), `B5AspMsqDiv`, `B5AspGrowth`,
  plus the §5.8 joiners; categoricals held at their settled values.
- The sweep first, registered before its first point in its own
  `analysis/b5x_sweep_<date>.md`; a flat surface is recorded, not
  skipped (PLAN B.6's rule applies to this cluster as to B.4);
  SPSA in blocks; theta the last completed block's rounded centres; the
  fitted-constant analysis before the bake; the OFF arm's values kept
  through a `BASILISK_B5_CORE_DEFAULTS` table where a joiner moves.

### BAS-S55 — the Cluster 4 gates

- (a) the fitted arm against the OFF arm, SPRT `[0,3]` at `3+0.03`,
  final-PGO builds, cap 4,500 pairs;
- (b) the fit against the seeds, `[0,3]`, cap 12,000 pairs;
- (c) the brackets §9 and M1 leave owed: `B5NegExtMax` if adopted, the
  stop's 2× if M1 calls for it;
- (d) 2,000 games at `10+0.1`, direction only;
- then the flip in one commit, the fingerprint record moved, the OFF arm
  compilable; the tests bind the default arm alone (maintainer
  2026-11-07).
- Order: (a), (b), (c), (d); none changes the candidate after a game is
  seen.

## 12. The four questions

1. **Mechanism.** A root window centred on a smoothed estimate of the
   move's value and sized by its magnitude fails less often where the
   score is stable and widens where it moves, so fewer nodes are spent
   proving bounds that the next window discards (53.7% of windowed nodes
   today); reducing root moves after the first lets the root's late
   moves cost what late moves cost elsewhere in the tree (44.1% of them
   would lose 2.0 plies); bounding chained negative reductions keeps a
   line's depth near the root depth (22.9% of the tree sits under
   chains of two or more). Each names a search property, measured here.
2. **Interactions.** §5: the LMR window term's unit (`lmr_reduction`,
   `search_kernel_core.cpp`), the time manager's effort scale
   (`tm_effort_*`, `search_worker.cpp`) and the WAC floor against root
   reduction, the extension budget (`b3::` helpers, `B3ExtBudget`) and
   the correction term against the cap, the fail-high stop and the
   give-up window against the width, the score bands and the table's
   depth byte against the ply cap, the pool table and the time manager
   against the record.
3. **Invariants.** §10's list from DESIGN §3.
4. **Falsifiers.** §11: P1 to P12 with their rules, the floors, the cost
   screen, M1's registered rule.

## 13. Artifacts (`tools/results/b5-20261108/`, ignored storage)

| Artifact | What |
|---|---|
| `census.patch`, `census_patch.py` | the throwaway census: 22 counters, two stack fields, a `kv` line; applies to `ad17d2b` (SHA-256 `cebdfad4…`) |
| `census_nostop.patch` | the same plus BAS-R07's `nostop.py` (SHA-256 `2b0db01e…`) |
| `basilisk-census-diag-nopgo.exe` | `ad17d2b` + `census.patch`, `cmake --preset release -DDIAGNOSTIC=ON`, target `nopgo`, clang 23.1.5; `bench 13` 4,144,790 (SHA-256 `6705f151…`) |
| `basilisk-census-nostop-diag-nopgo.exe` | the stop-free twin; `bench 13` 4,144,790 (SHA-256 `a12d4119…`) |
| `suite_d14.json`, `suite_d14.log` | §2.1 (`run_suite.py --depth 14 --stride 1 --hash 64`) |
| `stall_probe.py`, `probe_*.log`, `nostop_60000+600.txt`, `stop_60000+600.txt`, `stop_180000+2000.txt`, `raw/` | §2.2, BAS-R07's probe unchanged |
| `pgn_census.py`, `pgn_census_stc.txt`, `pgn_census_ltc.txt` | §2.3 |
| `SHA256SUMS.txt` | every file above |
| The donor's sources | `git -C D:/code/stockfish show sf_19:src/search.cpp`, `sf_19:src/evaluate.cpp` (`edb0d9db`) |

## 14. Departures table (for B.6.1)

Every place the arm keeps Basilisk's form where the donor's differs, or
leaves a donor mechanism unbuilt, with what the re-read waits on.

| Place | Basilisk's form (kept) | The donor's form | Why kept | Waits on |
|---|---|---|---|---|
| Fail-high depth reduction (`B5FailHighReduce` 0) | re-searches at full depth | `rootDepth − failedHighCnt − 3·(searchAgainCounter + 1)/4` | BAS-D17 (WAC 137 → 119 on the pre-B.2 search); the clock half is time management's | D.1 (`increaseDepth`), then the §9 item 2 gate; H.2 if neither |
| Negative-reduction ceiling (`B5NegExtMax` 0) | unbounded along a line | the same in `sf_19`; a ply bound after 19; Triumviratus's count cap | today's form works; TS-1 is a prior | the diagnostics leaf's item 1 |
| Optimism | none | `114·avg/(|avg| + 85)` inside the NNUE blend | the form is the network's; Rarog's HCE read −5.6; the evaluation is frozen | F, with the donor's blend adopted whole |
| Draw value | 0 | `−1 + (nodes & 2)` | Rarog −1.2; blindness unmeasured | the diagnostics leaf's judge read; then H.2 |
| Fail-high clock stop (`B5FailHighStop` 100) | A.8.17's stop at the optimum on the previous best | none | BAS-S20; the battery passes stop-free at `60000+600`, the hole stands at `180000+2000` | M1 (BAS-R10); the hole is D.1's |
| The open window at depth ≤ 3 and at a mate score | open | a window around the `−INF` average from depth 1 | identity in substance; three cheap iterations | nothing |
| The root's TT move before an iteration completes | the table's move | the record's first legal move | immaterial (depth 1) | nothing |
| An iteration a stop interrupts | discarded whole | a completed new best of the partial iteration is kept | no reason; the fail-high stop plays the previous best already | B.9.1 (TR-3) and H.2 |
| Best-move changes and the effort share for time | per iteration | counted inside the iteration over threads; effort across iterations | the consumers are the clock's | D.1 |
| `MAX_PLY` 128 and the static evaluation at the cap when not in check | 128; evaluate | 246; evaluate | the bands and the table's depth byte (§5.5) | C (the evaluation's bands), D.2 (TP-1) |
| The LMR window term at the null window (`B5LmrWindowUnits` 1) | 163 | 577 in the donor's units, which is 163 in ours | a unit conversion, not a departure; recorded because the constant differs | B.6's joint surface may read the 577 |
| The per-thread window offset | `(thread_id % 8) × 282 / 1000` | `threadIdx % 8` | the conversion rounds the first four threads to 0 | D.2 |
| The stopped MultiPV line | keeps its previous depth | the donor's inexact-score rules (`:443-491`) | no analysis defect reported | B.9.1 (D-73, TR-3) |

## 15. Instruments, planned and built, that inform this cluster

Per the maintainer's standing request (B.4.3, 2026-11-06), the
instruments PLAN has built or planned, each with a verdict:

| Instrument | Verdict | Where |
|---|---|---|
| B.16.2 pricing (`pricing.py`, built) | **worth it**: the `LmrFailLow` cell at the root depth prices what root reduction hides, the read BAS-D17 lacked | BAS-S53's screens |
| B.16.3 loss analysis by cell (`loss_by_cell.py`, built) | **worth it, narrowed**: its judge on the final positions of repetition draws is the blindness read B-84 asks for, minutes not hours | BAS-S53's screens |
| The deep-iteration cost screen (built) | **worth it**: the one read of the window and the cap at depths a game reaches | ticket 6, the qualification |
| Triumviratus's `TRIUMV_TMLOG` (planned by D.1 in spirit) | **its analogue is built here**: `replay_games.py` logs one row per search with the clock and the window counters; D.1 extends it to the iteration verdict | ticket 0 |
| B.16.4 identical-tree counters (built) | lower value: the arm changes the tree, so the pooled NPS read is the speed instrument | BAS-S53 |
| B.16.1 wall-clock read (built) | not useful here: no protocol or latency change | — |
| TM-7, TM-8 tree-size centring | lower value: the sweep reads the window's coordinates on games' Elo, not tree size (B-71) | — |

## 16. Sub-steps proposed to the maintainer

B.5 is one `I2` leaf in PLAN. B.3 and B.4 ran as research, implementation,
diagnostics with the unfitted paired run, the sweep and SPSA, and the
gates. The same five are proposed, as a structure change for sign-off:

- **B.5.1** `[R3]` Research and handoff on the entry head — this packet,
  closed `READY_FOR_IMPLEMENTATION` 2026-11-08;
- **B.5.2** `[I2]` Implement behind the umbrella switch — §10;
- **B.5.3** `[V]` Diagnostics, M1 and the unfitted paired run — BAS-S53
  and BAS-R10;
- **B.5.4** `[V]` Curvature sweep, then SPSA in blocks — BAS-S54;
- **B.5.5** `[V]` Cluster 4 gates — BAS-S55.

Until the split is applied, B.5's register row reads
`READY_FOR_IMPLEMENTATION` and the implementation runs under §10 as
B.5's next unit.
