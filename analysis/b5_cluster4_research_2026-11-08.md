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

**Amended 2026-11-09 by research amendment 1**, at the end of this file,
which answers *Return 1*. §3's width and growth arithmetic, §5.1's
formula, §7's `B5AspBase` row, ticket 3's tests and reads, two lines of
§10's cheap qualification, §9 item 2 and four of §11's registrations are
read as its §A1.5 amends them. The sections themselves are left as
written.

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

## Return 1 — B.5.2 ticket 3: at the seed the window's width cannot grow, and the loop does not end (2026-11-09)

**The premise.** §3 and §10 ticket 3: `delta = B5AspBase + (thread_id % 8) ×
282 / 1000 + |mean_sq| / B5AspMsqDiv`, then after each failure `delta +=
B5AspGrowth × delta / 128`; "a window inside its bounds ends the loop", with
no give-up hatch. §7 seeds `B5AspBase` at 1, the donor's 5 converted (1.41)
and truncated, and `B5AspGrowth` at 47. The clause assumes the width grows, so
that the loop ends.

**The failure.** In integer arithmetic `47 × delta / 128` is 0 at `delta` 1 and
2. At a level score the mean-square term is 0 too (under 2874 of mean square,
about 53 units), so the width stays at 1 for every failure and the loop has no
reason to end. The donor starts at 5, where the same expression grows (5, 6,
8, 10, 13, ...), so the donor's own loop never meets this.

- `tools/results/b5-20261109/find_spin.py` on ticket 3's Release binary, WAC at
  depth 6, a fresh process per position, 20 s each: **3 of 300 do not finish**
  (`2r1k3/6pr/p1nBP3/1p3p1p/2q5/2P5/P1R4P/K2Q2R1 w`,
  `r5k1/pppb3p/2np1n2/8/3PqNpP/3Q2P1/PPP5/R4RK1 w`,
  `6k1/5p2/p3p3/1p3qp1/2p1Qn2/2P1R3/PP1r1PPP/4R1K1 b`), each above 95M nodes
  at depth 5.
- A probe printing the loop's own values (`t3_probe/trace_*.txt`, 4,000 windows
  each): the width ends at 1, 2 and 1 and stays there. The second position is an
  exact cycle at `delta` 2: (197, 201) fails low at 197, (195, 197) fails high at
  197, (195, 199) fails high at 199, and the next window is (197, 201) again.
  Search instability through the table gives the loop a fixed point it cannot
  leave.
- `test_wac_floor` timed out at 1500 s; `test_wac --count` was killed after 72
  minutes of one core. `go depth N` on such a position does not return. Under a
  clock the stop ends the loop, with the iteration unfinished and its time gone.
- Bench 13 and every earlier test finish, so no fingerprint showed it.

**Where the loop ends it still costs.** P3 and P6 fail, both frozen, and the
width is the cause: ticket 3 has neither root reduction nor the record's later
tickets, and the variants below restore WAC with no other change.

| Read, the arm | Ticket 2 | Ticket 3 | Frozen |
|---|---:|---:|---:|
| re-searches per window (bench positions, depth 13) | 1.23 | **14.03** | P3 2.0–3.5 on `suite_v2` depth 14 |
| failed share of windowed nodes (same) | 48.7% | **80.1%** | P3 ≤ 48%; above 58% the falsifier |
| interior nodes (same) | 2,346,643 | 3,638,191 (1.55×) | |
| WAC at 100,000 nodes | not read | **236** of 300 | P6 [246, 262]; under 246 a floor failure |
| floors grid | not read | PASS 600 / 598 / 600 / 595, five budgets 119 | P7 |

23 Cluster 1 to 3 rows moved over 20% at ticket 3 (`t3/compare.md`), among them
`tt_cutoffs` +85.7%, `follow_pv_nodes` +161.4%, `sing_multicut` +128.3%,
`in_check_nodes` −41.7%. The cause is the repeated root re-search at nearly the
same window, which turns the tree over the table. They are reported, not
repaired, and were not re-read on the variants.

**This was in plain sight, and the implementer's test fixed it in place.**
`tests/test_b5_root.cpp` asserts `grow_delta(1, 47) == 1`, commented "47/128 of a
unit rounds away". The arithmetic was noticed and bound, and its consequence for
the loop was not followed. That test passes and must change on whatever the
amendment decides.

**Reads on throwaway builds, evidence only.** Each variant is one edit on a
copy of ticket 3's source (`probe_t3.py`, `build/b5-probe/`); nothing below is
in the tree.

- **V1:** the width grows by at least one each failure.
- **V2:** `B5AspBase` 3, the smallest base whose width grows at growth 47.
- **V3:** the width carried in thousandths from the donor's converted 1.41,
  growing by 47/128 untruncated, rounded down at use.

| Tree | bench 13 | WAC spins, depth 6 | WAC 100k | floors grid | bench positions: re-searches, failed share | `suite_v2` d14: re-searches, failed share | d14 interior / qs |
|---|---:|---:|---:|---|---:|---:|---:|
| OFF head (§2, census) | 4,144,790 | none (CTest) | 254 | — | — | 1.503, 53.7% | 7,116,466 / 4,893,846 |
| Ticket 3 as written | 5,039,771 | 3 | **236** | PASS | 14.03, 80.1% | not read, it can spin | — |
| V1 | 3,984,800 | 0 | 253 | PASS | 3.03, 67.4% | 3.56, 74.3% | 7,785,642 / 5,301,231 |
| V2 | 3,631,713 | 0 | 246 | PASS | 2.14, 63.6% | 2.69, 69.5% | 6,671,605 / 4,615,645 |
| V3 | 4,639,581 | 0 | 252 | PASS | 3.00, 66.7% | 3.46, 73.3% | 7,660,371 / 5,233,865 |

Reading it:

- **Every variant ends the loop**, at depth 6 by the read and at any depth by
  construction, since each width grows without bound. The spin read was not run
  deeper on ticket 3 or on the variants.
- **No repair restores P3's failed share.** All three stay above the 58% that
  P3's falsifier names, on the registered population, where the OFF head reads
  53.7%. So "the narrow windows cost more than they save" is open with the
  termination repaired, and P3 says to read `B5AspBase`'s sweep before the whole
  arm is blamed. The re-search rate of V2 and V3 is inside P3's range, and V1's
  is just above it.
- **The floors grid cannot choose among these.** All four trees read the same
  grid, 2,400 of 2,400 playouts equal to the ply (`floors_differ.py`), on four
  distinct binaries. That is real, not a tool fault: in a won ending the
  mean-square term opens the window from its first search (16535² / 2874 ≈
  95,000; no bound lines at all on the four positions of `floors_visible.py`), so
  base and growth never bind. The grid does see the arm, whose searches differ
  from the OFF arm's on those positions. KR-K's 598 sits on the floor, its two
  misses independent of the width.
- **The bench and counter columns are different runs**: bench 13 shares its
  process and hash, the counters run cold per position, so V3's bench total and
  interior count do not rank the same way as V1's. Neither is a strength read.
- **WAC at V2 is on the floor**, 246.

**The whole range is exposed, not just the seed.** The width grows only when
`growth × delta ≥ 128`: from `delta` 3 at growth 47, from 4 at the range's
growth 32, from 2 at 96. §7's ranges (base 1–40, growth 32–96) therefore reach a
stuck width anywhere low on the surface, so a seed-only repair leaves B.5.4's
sweep and SPSA able to hang a game's iteration: base 3 is stuck at growths 32 to
42.

**As built and kept.** Tickets 0 to 3 stand as committed; ticket 3 (`9a77439`) is
§10's loop as written, with the spin in it, so the failure can be reproduced from
the tree. On ticket 3's source the OFF arm reads 4,144,790 and CTest 28/28; the arm
reads 5,039,771 and CTest 27/28 (`test_wac_floor` times out). The counters, the
record, `replay_games.py` and `floors_grid.py` stand whichever way this goes.
Ticket 3's tests other than the growth row are unaffected.

**Options.**

1. *Carry the width in a finer unit (V3's mechanism).* Answers the cause: 1.41
   was truncated to 1, and the donor's width is a real quantity on its own scale
   that truncates only where it no longer matters. No constant moves in meaning,
   and every growth in the range terminates. Cost: `B5AspBase` must become a
   finer unit or a fixed fraction for the seed to say 1.41, a change to §7's row
   and to the SPSA surface; V3's `1410 + (delta − 1) × 1000` is a probe's form,
   not a proposal.
2. *Grow by at least one (V1).* One line, no coordinate changes, terminates over
   the whole range, keeps every seed. Cost: a form the donor does not have, whose
   first steps (1, 2, 3, 4, 5, 6, 8) differ from 47/128 at the small widths that
   matter most here.
3. *Raise the base (V2).* No new arithmetic. Cost: a seed chosen because it
   passes, WAC on the floor, and the range's bottom must rise with it: base at
   least 4 to be safe at growth 32.
4. *Restore a bound on the loop.* Open the window after N failures, or at a
   width. It guarantees termination whatever the arithmetic, and §3 removed the
   hatch deliberately. It also leaves the windows stuck until the bound, so it
   answers neither P3 nor P6 on its own.

Recommendation: option 1, on the same reasoning as this cluster's other unit
corrections (§5.1's null window), since the defect is a unit conversion that
truncated the donor's quantity. Whichever is chosen, §10's test becomes a
property over the whole registered range, that the width strictly grows from
every reachable `delta`, with a WAC-at-depth run in the cheap qualification
guarding the loop. Main counter-argument: V1 reads about the same as V3 on WAC
and the census (253 against 252; 3.56 and 74.3% against 3.46 and 73.3%) for one
line and no surface change, and nothing measured here separates them on strength;
a reviewer who weighs simplicity over faithfulness has good grounds.

**Questions for the amendment.**

1. Which termination repair, and does it change §7's coordinates or ranges?
2. P3's failed share is above its falsifier under every repair. Does the seed
   stand into B.5.3 with the unfitted run deciding, or does its `B5AspBase` read
   come first? P3 names the read and not its timing against the paired run.
3. The floors cannot discriminate width choices. Is WAC at 100,000 nodes with the
   spin read enough for ticket 3's cheap qualification, or is another read
   needed before ticket 4?
4. Is §10's ticket 3 test list amended with the range property, and is a
   time-bounded depth read added to §10's cheap qualification?
5. Do tickets 4 to 9 stand as written on the repaired width? They have not
   started, and their reads take ticket 3's new fingerprint.

**State at the return.** Tickets 0 to 3 are committed. Tickets 4 to 9 have not
started, because `AGENTS.md` resumes implementation only on the amended contract;
ticket 4's edit script is written and unapplied. No game ran, no default moved,
no NPS read started.

## Implementation record — tickets 0 to 3, at Return 1 (2026-11-09)

- Commits per ticket: 0 `e767c0a` (engine), `3a52800` (tools), `4adad6e` (tools,
  below); 1 `2757cfa` (engine), `910c9b1` (tools); 2 `03b4a1a`; 3 `9a77439`. The
  maintainer's `dd5d0b6` (docs) landed on the branch during ticket 0.
- Fingerprints (`bench 13`, PEXT, clang), read on Release, Tune and Diag trees of
  both arms at tickets 0 to 2, and for ticket 3 on all six before the read hung:

| Ticket | OFF | ON | CTest OFF / ON |
|---|---:|---:|---|
| 0 | 4,144,790 | 4,144,790 | 27/27, 27/27 |
| 1 | 4,144,790 | 4,144,790 | 28/28, 28/28 |
| 2 | 4,144,790 | 4,144,790 | 28/28, 27/28 (below) |
| 3 | 4,144,790 | 5,039,771 | 28/28, 27/28 (`test_wac_floor` times out) |

- Tests added: `tests/test_b5_root.cpp` (the ply cap through a lowered cap, the
  record's arithmetic, the root's first move from the record, the window's
  arithmetic, a mate score's open window, the bound lines, no give-up on the arm);
  `tools/diag/test_replay_games.py`; `test_run_suite.py` and `test_spsa_surface.py`
  extended.
- Deviations from the handoff, each with its reason:
  - The negative-extension stack fields landed at ticket 0, since the chain
    counters need them; written only where counters compile until ticket 6.
  - M1 must run on a Tune build: a Diag build advertises no `S` lines, and BAS-R10
    says Diag. The longest window is read from `TM_Debug` fields, not counters.
  - §2.1's "seven replaced entries" do not exist: they are a new move failing
    high then failing low, where table and record agree. Arm identity held on 40 of
    40 positions at depths 10 and 13, including those with `root_tt_mismatch_fl`
    above 0.
  - Ticket 2 left the arm's `test_search` failing on the promotion fixture
    (`3r4/P7/2K1p3/R7/5p2/6kP/8/8 w`): the rule that searches the queen before the
    underpromotion makes the queen the record's first after a fail low while the
    table holds the rook, and the win was sighted only at 207,370 nodes. The test
    was not relaxed and ticket 3 cures it (32,565 nodes).
  - `root_effort_*` count windowed iterations; the EMA truncates toward zero where
    the donor's arithmetic floors negatives, a sub-unit difference; the sort and the
    node check are skipped while `root_excluded` is in use, until ticket 7.
  - `floors_grid.py` was promoted at ticket 0 assuming a per-playout print that
    only B.4's patched harness had, and its dry run did not exercise the real
    binary, so its first real run failed at ticket 3. `test_endgames --list` now
    prints them (`4adad6e`). The grid gained a correctness run for the wrong-bishop
    draws.
- Not yet done, for the resumption: two of ticket 1's three mutants did not build
  and must be rerun; the per-commit check of both arms; §10's cheap qualification.
- What the reviewer must check: deferred to the implementation's final record.
- Review: with the record of tickets 3 to 9 (*Review — B.5.2 accepted with named follow-ups*, at the end of this
  file).

## Research amendment 1 — answers Return 1: the width in a finer unit (2026-11-09)

### A1.0 Registered before its runs: the width read (zero-game)

Written and committed before any run of the probe below. Only the
Return's own evidence (`tools/results/b5-20261109/`) was read first.

**What is probed.** The repair this amendment intends to specify, on a
throwaway tree of ticket 3's source (`a1_build.py`, SHA-256 `a6877f8a…`;
Release `bfd5c9c2…`, Diag `5a897b86…`; `build/b5-probe/a1-*`):

- the width is carried in thousandths of a unit: `width = base + (thread_id
  % 8) × 282 + |mean_sq| × 1000 / B5AspMsqDiv`, the base 1410 (the donor's 5
  × 282); after each failure `width += growth × width / 128`;
- the bounds use the width rounded up to a whole unit, `delta = ceil(width /
  1000)`: on integer scores that is the open interval `(average − width,
  average + width)` exactly;
- LMR's window term keys the unit on the node type, `(pv ? delta × 577 : 163)
  / root_delta_`, where ticket 3 keys it on `delta > 1` (§5.1 says "a
  null-window node"; a PV node whose window is one unit wide is not one).

The base, the growth, the rounding and the LMR form come from the
environment, so one pair of binaries serves every point.

**Points.** Base 500, 1000, 1410, 2000, 2820, 4230, 5640, 9870 and 19000 at
growth 47; growth 32, 64 and 96 at base 1410; rounding down and to nearest
at the seed; ticket 3's LMR form at the seed; and, for termination only, the
corners 500/32, 500/96, 40000/32 and 40000/96.

**Reads per point.** `bench 13` (Release); WAC at depth 6, a fresh process
per position, 20 s each, the unfinished counted (`find_spin.py`); WAC at
100,000 nodes; `suite_v2` at depth 14, stride 1 (windows, re-searches,
windowed, failed and fail-high re-search nodes, interior and quiescence
nodes). At the seed also WAC at depth 10 with 60 s each, the floors grid,
and the `bench 13` counters against ticket 2's for the rows over 20%.

**Predictions, frozen.**

| # | Prediction | Confidence |
|---|---|---|
| W1 | No position is left unfinished at any point, the corners included | high (by construction) |
| W2 | At the seed: 2.7 to 3.3 re-searches per window; failed share 68 to 74%; interior plus quiescence nodes 11.4M to 12.9M (the OFF head 12,010,312); WAC at 100,000 nodes in [246, 256], at least 246 with 80% | moderate |
| W3 | Along the base at growth 47, re-searches per window and the failed share fall as the base rises; nodes to depth 14 are least at a base between 2820 and 9870, there 3 to 12% under the OFF head | low-moderate |
| W4 | At the seed at most 10 Cluster 1 to 3 rows move over 20% against ticket 2 (ticket 3 as written: 23), `tt_cutoffs` and `follow_pv_nodes` among them | low-moderate |
| W5 | The node-type LMR form moves `bench 13` by under 1% of its nodes at the seed | moderate |

**What the read decides, and what it cannot.**

- Termination: any unfinished position at any point refuses the form as
  probed, and the amendment adds a bound on the loop instead.
- The seed is the donor's quantity, 1410 with 47 and 2874, whatever the
  cost columns read. Nodes to depth, re-searches and the failed share are
  priors for B.5.4's sweep points; they are not strength (B-71), and no
  seed is chosen from them.
- WAC at 100,000 nodes under 246 at the seed: the seed is not handed on as
  qualified. The neighbouring points' reads are reported as the floor's
  sensitivity to the width, and the choice between the floor and the seed
  goes to the maintainer with a recommendation; neither is moved here.
- W4 answers the Return's attribution of the 23 rows: if the rows stay over
  20% in number with the loop ending, the cause is the narrow window, not
  the spin.

**Addendum to the registration, before any result of the read was seen**
(the first runs were in progress and unread). Two deeper reads, since
depth 14 may not rank the widths as the depths a game reaches do:

- the deep-iteration cost screen (`deep_iteration_screen.py`, depths 16 to
  19, B.3's five positions, 60 s watchdog) of the probe at base 1410, 2820,
  5640 and 19000 and of the Return's V1 binary, each against the OFF arm's
  Release binary. Its registered clause stands (a geometric mean of at most
  3, no depth the OFF arm finishes left unfinished); an unfinished depth at
  any point is a termination failure as above;
- `suite_v2` at depth 17, stride 1, at base 1410, 2820 and 5640 and on the
  OFF arm's Diag binary: interior plus quiescence nodes and re-searches per
  window.

W6 (low confidence): the three bases rank at depth 17 as they do at depth
14, and every screen passes with each position's mean between 0.7 and 1.4.
Neither read chooses a seed.

**Correction to the addendum, registered before its run** (the main read
and the addendum's two reads were seen first; this screen's result was
not). The addendum ran the cost screen at depths 16 to 19 under the tool's
own absolute rule, a geometric mean of at most 3, because §10's cheap
qualification words the clause that way. That wording is this packet's
error: B.3.0's research amendment 7 §R replaced the absolute rule with the
growth rule, read to depth 25 and scored by `screen_score_a7.py` (SHA-256
`57c0ea35…`), and says the tool's own verdict is not the verdict. B.4's
review ran §R's form. The screen is therefore read again in §R's form
(`--depth-min 16 --depth-max 25 --watchdog 60 --hash 64`, §R's scorer) on
the same five trees against the OFF arm. The depth 16 to 19 results stay
in the evidence and are reported, not judged. No prediction is added: W6's
screen half was written for the wrong rule and is scored as it reads.

**One follow-up read, registered before its run** (every result above was
seen first). The depth-14 totals fall to 0.73 and 0.71 of the OFF head at
base 9870 and 19000, and the depth-17 read was registered only up to 5640.
`suite_v2` at depth 17 is read at 9870 and 19000 as well, for B.5.4's
prior. W7 (low-moderate): both read under 0.90 of the OFF head's
32,125,699 nodes at depth 17, with the median position under 1.00. It
chooses no seed.

### A1.1 Verdict

Return 1 is right, and the error is this packet's: §7 truncated the
donor's 1.41 to 1, and at 1 the growth rounds to nothing. The width is
carried in thousandths of a unit and rounded up at use (§A1.5). That ends
the loop everywhere on the registered ranges, restores the donor's first
window, and keeps the three coordinates live where the windows are. The
seed stays the donor's quantity, 1410. Tickets 4 to 9 stand. B.5.2 returns
to `READY_FOR_IMPLEMENTATION` and resumes at ticket 3.

Two things the Return did not ask weigh as much as what it did:

- **The narrow width is not paid for.** Two positions of 106, both won by
  a piece or more, give back 2.07M of the OFF head's 12.0M nodes at every
  width: the mean-square term stops the OFF head's fail-high cascade
  there. On the other 104 the seed costs 20% more nodes to depth 14 than
  today's loop and 24% more to depth 17; the same loop at base 19000 costs
  15% and 8% less (§A1.3). Fail-high re-searches are 29 to 53% of windowed
  nodes on every tree read (§9 item 2's gate is 20%). The donor runs them
  at reduced depth; the arm's default runs them at full depth. Games
  decide the width (B-71), so the seed stands, and the width is read by
  games in B.5.3 before the categoricals (§A1.5).
- **Zero-game totals cannot rank repairs.** Neighbouring bases differ by
  10 to 15% in depth-14 nodes without order (1410, 2000 and 2820 read 0.99,
  0.99 and 1.14 of the OFF head), so the Return's variant table separates
  nothing, and neither does this read between rounding rules. Only the
  trend across a tenfold range of the base is a signal.

### A1.2 The Return, checked

Reproduced:

- The arithmetic. `47 × delta / 128` is 0 at 1 and 2.
- Ticket 3's Release binary reads `bench 13` 5,039,771 and leaves the same
  three WAC positions unfinished at depth 6 (`control-ticket3-spin6.log`).
  So the detector detects, and a zero from it is a zero.
- The probe method. The Return's trace tree, ticket 3's source with one
  print, reads 5,039,771, so a patched copy built this way is the arm plus
  its patch and nothing else. Build options of the probe trees and the
  committed trees are the same.
- V1 to V3's census figures, from their own counter files.
- The floors' blindness to the width. It is by construction: the
  mean-square term opens the window in a won ending, and the probe's grid
  reads the same 600 / 598 / 600 / 595 and 119. Nothing about the width can
  reach conversion while that term stands, and a test binds it (the won
  tier's first window is open).

Wrong or missing in the Return:

- **The attribution of the 23 rows (question 6).** With the loop ending,
  V1 moves 17 Cluster 1 to 3 rows over 20% against ticket 2, V2 13, V3 11
  and the probe 10 (`*-compare-t2.md`). The spin is the cause of about
  half; the narrow, re-searched root is the cause of the rest. The ten on
  the probe: `cont_hist_pruned` −51.6%, `corr_cont2_updates` and
  `corr_cont4_updates` −21.7%, `in_check_nodes` −25.0%, `null_tries`
  +28.3%, `null_cuts` +29.4%, `sing_multicut` +24.9%,
  `null_region_refusals` −75.4%, `follow_pv_nodes` +59.8%,
  `qs_evasion_nodes` −23.2%. `cont_hist_pruned` moves −3.8% on ticket 3 as
  written and −45 to −52% on all four trees that end the loop; the cause
  was not found here. They are reported under §10's rule, not repaired,
  and §5.8 reads them for the surface.
- **The fail-high share**, above. The counter was there to read.
- **The node columns as evidence between repairs.** They are single draws
  from a scatter of 10 to 15%.

This packet's own errors, found while checking:

1. §7's truncation, and its size. On integer scores a window `±delta` admits
   `2 × delta − 1` scores. The donor's ±5 admits nine of its scores, 2.5 of
   ours; ±1 admits one, the average itself. Ticket 3 as written searched
   under half the donor's window before any growth.
2. §5.1 keyed the corrected unit on `delta > 1`. Its own sentence says "a
   null-window node"; a PV node whose window has narrowed to one unit is
   not one, and read 163 where it should read 577.
3. §10's cheap qualification words the cost screen as a geometric mean of
   at most 3. B.3.0's research amendment 7 §R replaced that rule, and B.4's
   review ran §R's form. §A1.0's addendum repeated the error before it was
   caught.
4. BAS-R10 asks for three arms on one Diag binary. Tune and Diag are
   exclusive flavours, and only a Tune build advertises `B5FailHighStop`.
   The implementation record's deviation is right.
5. §2.1's "seven replaced entries". The record's reading (a new move
   failing high, then low, table and record agreeing) is accepted.

### A1.3 The read (`tools/results/b5-20261109/amend1/`)

Registered in `f8d3311`, `af7d3e6`, `de6d008` and `b8be98e`, each before
its run. The OFF arm's binaries read 4,144,790 before the comparisons.
"Median" is the median over positions of nodes against the OFF head's
census on the same position. "Without two" leaves out `suite_v2` 42 and 50
(the cost screen's positions 4 and 3), where the OFF head spends 1.26M and
0.84M nodes and every point about 15,000 each. Both are won by a piece or
more. On 42 the OFF head proves mate in 9 at depth 11 through a fail-high
cascade; the arm reports +20 at depth 14 with the same move, its first
window some ±1000 wide by the mean-square term. That is the arm's form
working as §3 intends, at every width alike, and depth does not mean the
same thing on the two arms there. The saving is the window's and not the
record's: today's loop on the record (`B5Window` 0) spends 0.73M and
0.89M on the two (`two_positions.txt`).

`suite_v2` at depth 14, `bench 13` and WAC, growth 47 unless named:

| Point | `bench 13` | WAC 100k | re-searches per window | failed share | fail-high re-search share | nodes / OFF | median | without two |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| OFF head (§2.1) | 4,144,790 | 254 | 1.50 | 53.7% | — | 1.000 | 1.000 | 1.000 |
| base 500 | 3,597,259 | 248 | 3.39 | 72.5% | 51.2% | 1.104 | 1.135 | 1.334 |
| base 1000 | 3,600,470 | 250 | 3.06 | 69.3% | 44.3% | 1.078 | 1.184 | 1.302 |
| **base 1410, the seed** | **4,022,487** | **249** | **2.93** | **70.4%** | **49.2%** | **0.993** | **1.148** | **1.200** |
| base 2000 | 4,026,375 | 250 | 2.65 | 69.2% | 40.2% | 0.992 | 1.015 | 1.200 |
| base 2820 | 3,726,469 | 254 | 2.46 | 72.5% | 53.0% | 1.143 | 1.037 | 1.382 |
| base 4230 | 3,826,763 | 258 | 2.04 | 61.8% | 40.8% | 0.838 | 1.019 | 1.011 |
| base 5640 | 2,860,299 | 249 | 1.82 | 61.5% | 48.5% | 0.951 | 1.000 | 1.149 |
| base 9870 | 3,165,834 | 250 | 1.36 | 54.6% | 35.6% | 0.733 | 0.946 | 0.885 |
| base 19000 | 2,556,002 | 248 | 0.90 | 42.3% | 29.3% | 0.706 | 0.926 | 0.853 |
| seed, growth 32 | 3,870,439 | 250 | 3.14 | 71.4% | 47.4% | 1.002 | 1.142 | 1.211 |
| seed, growth 64 | 3,593,192 | 255 | 2.56 | 67.4% | 41.6% | 0.897 | 1.071 | 1.083 |
| seed, growth 96 | 3,292,694 | 257 | 2.35 | 63.6% | 44.9% | 0.906 | 1.000 | 1.095 |
| seed, rounded down | 3,925,785 | 254 | 3.14 | 69.9% | 42.9% | 0.958 | 1.110 | — |
| seed, rounded to nearest | 4,021,096 | 250 | 3.00 | 71.0% | 45.4% | 1.078 | 1.158 | — |
| seed, ticket 3's LMR form | 3,795,838 | 246 | 2.94 | 69.5% | 47.7% | 1.039 | 1.136 | — |
| Return's V1 / V2 / V3 | 3,984,800 / 3,631,713 / 4,639,581 | 253 / 246 / 252 | 3.56 / 2.69 / 3.46 | 74.3 / 69.5 / 73.3% | 47.7 / 43.3 / 47.1% | 1.090 / 0.940 / 1.074 | 1.212 / 1.008 / 1.168 | — |

`suite_v2` at depth 17 (the OFF head 32,125,699 nodes, 1.39 re-searches
per window, failed share 48.3%):

| Base | nodes / OFF | median | without two | re-searches per window | failed share | fail-high re-search share |
|---|---:|---:|---:|---:|---:|---:|
| 1410 | 1.100 | 1.000 | 1.241 | 2.83 | 60.7% | 50.1% |
| 2820 | 1.101 | 1.038 | 1.243 | 2.37 | 60.5% | 44.6% |
| 5640 | 0.944 | 1.000 | 1.057 | 1.73 | 54.8% | 44.5% |
| 9870 | 0.886 | 0.992 | 0.994 | 1.28 | 45.3% | 36.4% |
| 19000 | 0.828 | 0.955 | 0.922 | 0.81 | 36.7% | 24.7% |

The three statistics do not agree on size (at the seed, depth 17: total
+10%, median level, without two +24%), because a few positions carry any
total. They agree on direction across the range: wider is cheaper.

Termination: no position unfinished at depth 6 at any of the fifteen
points or the four corners (500/32, 500/96, 40000/32, 40000/96); none at
depth 10 at the seed; no watchdog stop and no missing depth to depth 25 on
the five screen positions at any of five trees. The seed's floors grid
passes (600 / 598 / 600 / 595, five budgets 119).

The cost screen in §R's form passes on all five trees (the seed, 2820,
5640, 19000 and V1 against the OFF arm; V1 under (c) on position 5). Its
absolute ratios are recorded and wild: 0.13 to 0.33 on positions 3 and 4
on every tree (the two won positions above), and on position 5 at depth
22 0.72 at the seed against 4.5 to 11.2 on the other four. One late root resolution on one position moves
a tree's total by more than any width does.

Scored:

| # | Result |
|---|---|
| W1 | **hit.** Nothing unfinished anywhere |
| W2 | **hit** on all four: 2.93, 70.4%, 11,925,525, WAC 249 |
| W3 | **half.** Re-searches fall with the base at every step. The failed share falls only in trend (1410 and 2820 rise). The least nodes are at 19000, the top of the sweep, 29% under the OFF head, not at 2820 to 9870 and 3 to 12% under |
| W4 | **hit, barely,** and half wrong in its detail: 10 rows; `follow_pv_nodes` is among them, `tt_cutoffs` (+7.6%) is not |
| W5 | **missed.** 5.6% of `bench 13`'s nodes. Every one-line change here moves `bench 13` by that much (base 1410 to 2820: 7.4%), so the miss measures the scatter, not the form |
| W6 | **half.** The three bases keep their order at depth 17, 1410 and 2820 a tie. The screen half was written for the wrong rule; under it 5640 fails on position 5 (3.32) and the positions' means run 0.07 to 3.3, not 0.7 to 1.4 |
| W7 | **hit.** 0.886 and 0.828, medians 0.992 and 0.955 |

What the misses say about the model. P3 and W3 share one error: this
packet expected failed windows at a narrow width to be cheap, as null
windows are. They are not. At 2.9 re-searches per window and a 70% failed
share, a failed search costs about four fifths of the one that succeeds.
The table carries the work between them, which is why the total stays
near the OFF head's instead of trebling.

### A1.4 The six questions

**1. Which repair; does it change §7?** The finer unit, on four grounds,
none of them the Return's recommendation:

- It is the donor's window. Rounded up, 1.41 admits the three scores the
  donor's ±5 admits on our scale; V1 keeps the one-score first window.
- The growth stays live. In whole units the growth does nothing below
  `128 / growth`, so under V1 growth 32 and growth 47 give the same first
  six widths, where at 2.9 re-searches per window nearly every re-search
  happens. B.5.4 would fit a coordinate that integer division had
  switched off.
- The base gets a usable step. In whole units the smallest move from 1 is
  +100%.
- The mean square's fraction and the helper's offset count. Both were
  truncated: up to a unit of a two-unit window, and three helpers of
  seven reading 0.

It changes §7's `B5AspBase` row (unit, seed, range) and nothing else on
the surface: `B5AspMsqDiv` and `B5AspGrowth` keep their seeds and ranges.
V2 is refused: a seed chosen because it passes, and stuck below it. A
bound on the loop is refused while the property test holds: it would hide
the next stuck width instead of forbidding it.

*Falsifiers.* An unfinished search at any point of the registered ranges
adds the bound after all. If B.5.4 finds `B5AspGrowth` and the sub-unit
part of `B5AspBase` flat, V1 would have fitted as well and the finer unit
bought the range guarantee alone. Nothing zero-game separates the two on
strength, and this amendment does not claim it does.

**2. Does the seed stand into B.5.3?** Yes, as the arm's default and as
P1's subject, and P3's base read comes next, by games, before the
categoricals. The zero-game half of that read is §A1.3; it was registered
to choose no seed and chooses none. A constant is a seed on the donor's
scale, converted, then fitted (PLAN rule 2), and window changes are
decided by games (B-71). Rarog's surface fell toward wider windows
(B-72); this engine's node read rises toward narrower ones. Those
disagree, and a node count is not the arbiter. What changes:

- **The width read by games** joins BAS-S53, straight after the whole-arm
  run: one paired run on the tune build, `B5AspBase` 19000 against 1410
  (§A1.5). At 19000 the arm's window is today's width on the donor's
  form, so the run reads the width alone. It runs before the
  categoricals because item 2's population halves between the two widths
  (49% of windowed nodes against 29%), and a verdict read at a width the
  fit then leaves is a run spent twice. Its consumers are item 2's run
  and B.5.4's start, both downstream in the same two leaves.
- On a P1 stop that run is the first read, before any switch is ablated.
- §9 item 2's gate is met at ticket 3 already, so its paired run is owed.
- B.5.4's sweep takes its points and its statistic from §A1.3.

*Falsifier.* If B.5.4's games rank the base against the node read
(narrower better), fixed-depth nodes are not a prior for this coordinate
and are not used as one in the later clusters.

**3. Is WAC at 100,000 nodes with the spin read enough before ticket 4?**
Yes, with the property test, which is the real bound: the spin read
samples what the test proves. The floors need not see the width. No
other read comes before ticket 4; the cost screen stays in the final
qualification, in its right form. *Falsifier:* an unfinished search, or
WAC under 246, at ticket 3 returns the leaf as registered.

**4. The tests.** Accepted, stated more strongly (§A1.5): strict growth to
the cap, a bound on the number of failures, and a bound on how often one
whole-unit width repeats. The time-bounded fixed-depth read is accepted
at the seed and the four corners, at ticket 3 and in the final
qualification.

**5. Tickets 4 to 9.** They stand as written. Three notes, none a change:
ticket 5's read will show the gate met; ticket 7's lines use the same
width function; ticket 9's liveness for `B5AspBase` steps a whole unit
(1410 to 2410), since a sub-unit step can leave `bench 13` unmoved.

**6. What is wrong in the Return.** §A1.2: the attribution, the unread
fail-high share, and the node columns taken as evidence.

### A1.5 The amended contract

**The width** (replaces the arithmetic of §3's "Window width" and "Growth"
rows, §7's `B5AspBase` row and the width clause of *The arm's root, stated
once*; the bounds, the centre, the fail-low and fail-high rules and the
absence of a hatch are unchanged).

- `width` counts thousandths of a unit, in 64 bits; divisions truncate;
  `cap = 1000 × kMaxWindowDelta`.
- First: `width = min(B5AspBase + (thread_id % 8) × 282 + |mean_sq| × 1000
  / B5AspMsqDiv, cap)`.
- Wherever the loop uses `delta`: `delta = (width + 999) / 1000`.
- After each failure: `width = min(width + B5AspGrowth × width / 128, cap)`.

| Coordinate | Donor | Seed | Range | Kind |
|---|---:|---:|---|---|
| `B5AspBase`, in thousandths | 5 | **1410** (5 × 282) | 500–40000 | X |
| `B5AspMsqDiv` | 10193 | 2874 | 1000–12000 | X, unchanged |
| `B5AspGrowth` | 47 | 47 | 32–96 | X, unchanged |

At the seeds the first window is ±2 at a level score, ±3 at 50 units, ±5
at 100, ±10 at 150, ±33 at 300, ±350 at 1000, open in the won tier; after
failures at a level score ±2, 2, 3, 4, 5, 7, 10, 13, 18, 24. The donor's
own integers give 2, 2, 3, 3, 4, 5, 7, 9, 12, 17 on our scale: its growth
truncates at small widths and this one does not. Helpers 1 to 7 start at
±2, 2, 3, 3, 3, 4, 4.

**LMR's window term** (replaces §5.1's formula): `(pv ? delta × 577 : 163)
/ root_delta_` under `B5LmrWindowUnits` 1, `pv` the node's type. A non-PV
node's window is always one unit.

**Ticket 3's tests**, replacing the growth rows and
`grow_delta(1, 47) == 1`:

- the widths above at the seeds, the thread offsets, the clamp at the cap,
  an empty record and the won tier open;
- **the range property:** for every base, growth, divisor and thread on
  the registered ranges' corners and a grid between them, at a level
  score, the width strictly grows at every failure until it equals the
  cap; it reaches the cap within 60 failures (the worst corner, 500 with
  32, takes 53); `delta` never falls; and no value of `delta` is used more
  than 4 times running (500 with 32 uses ±1 four times);
- the term: PV at `delta` 1 against a root window of 1 reads 577 and of 4
  reads 144; non-PV reads 163, 81 and 4 against 1, 2 and 38; PV at the
  root's own width reads 577.

**Ticket 3's reads before ticket 4.** Each figure is the probe's, whose
arithmetic is the one above, so the implementation should reproduce it;
a difference is found and explained in the record before ticket 4 and is
not by itself a failure.

- Both arms' CTest, `test_wac_floor` finishing; the OFF arm 4,144,790.
- The arm's `bench 13`: **4,022,487**.
- The depth-bound read: WAC at depth 6, a fresh process a position, 20 s
  each, nothing unfinished, on the Release arm at the seed and on the Tune
  build at 500/32, 500/96, 40000/32 and 40000/96. The Return's
  `find_spin.py` is promoted to `tools/diag/` with an option argument and
  a self-test that an engine which never answers is counted; the review
  reruns it.
- WAC at 100,000 nodes at least 246 (249).
- The depth-14 census: 2.93 re-searches per window, failed share 70.4%,
  fail-high re-search share 49.2%, 7,052,433 interior and 4,873,092
  quiescence nodes.
- The floors grid, the mate fixtures and the won-ending clock test (600 /
  598 / 600 / 595 and 119).
- The `bench 13` counters against ticket 2's, the rows over 20% listed
  (§A1.2's ten).

**§10's cheap qualification**, two lines changed:

- the cost screen is read in B.3.0's research amendment 7 §R's form
  (`--depth-min 16 --depth-max 25 --watchdog 60 --hash 64`, scorer
  `tools/results/b31-20261030/t10/a7/screen_score_a7.py`, SHA-256
  `57c0ea35…`), as B.4's review ran it. The tool's own verdict and the
  absolute ratios are recorded, not judged. Position 5 is knife-edged to
  the width (§A1.3), so a failure there is reported with its growth steps
  on the arm at `B5AspBase` 1000 and 2000 beside it before any mechanism
  is blamed;
- the depth-bound read above is added, on the final arm.

**§9 item 2**, one clause added. Its gate is met on every tree read (29 to
53% against 20%), so the paired run is owed. The share falls with the
width (49% at the seed, 29% at 19000), so the verdict holds at the width
it was read at: it is read at the width BAS-S53's width run leaves (below),
and if B.5.4 then moves `B5AspBase` to more than twice or under half that
value, BAS-S55 (c) reads item 2 again at the fitted width.

**§11's registrations**, each change stated:

- **BAS-S53 gains the width run** (agreed by the maintainer
  2026-11-09), after the whole-arm paired run and before §9's items:
  2,000 games at `3+0.03` on the arm's tune build,
  `B5AspBase` 19000 (arm A) against 1410 (arm B), every other option at
  its default. Read by §9's rule: at +5 Elo or more for 19000 with the
  95% interval excluding 0, §9's items are read with both arms at 19000
  and BAS-S54's SPSA starts `B5AspBase` there; otherwise they are read at
  the seed and SPSA starts at the seed. It moves no default in the
  source: the base is a fitted coordinate, and the fit sets it. At −5 or
  less with the interval excluding 0, fixed-depth nodes mislead on this
  coordinate (question 2's falsifier).
- P1's falsifier, order amended: on a read below −20, the width run is
  read first, then the switches in the order written, at the width that
  run leaves. P3 already said the base is read before the arm is blamed;
  P1's list put it last.
- P1's and BAS-S54's values are in the new unit: "`B5AspBase` 19" is
  19000 and "1 to 19 swept" is 1000 to 19000. BAS-S54's sweep reads
  `B5AspBase` at 1000, 1410, 2000, 2820, 4230, 5640, 9870, 19000 and
  30000, and `B5AspGrowth` 32, 64 and 96 at the seed and at the cheapest
  base, at depths 14 and 17, by the median position beside the total.
- BAS-R10's instrument: the three stop arms on a Tune build of the arm
  through `B5FailHighStop`, the longest window from its `TM_Debug`
  fields; the ply-cap and aspiration counters (P9) from one more replay
  of the same games on a Diag build at the default stop.
- Each entry file takes this text as amended when its leaf opens.

**§14 gains two rows** (and loses the per-thread offset's "rounds the
first four threads to 0"):

| Place | Basilisk's form (kept) | The donor's form | Why kept | Waits on |
|---|---|---|---|---|
| The window's width unit | thousandths of a unit, rounded up at use; growth untruncated | an integer of its own unit, 0.282 of ours; `47 × delta / 128` truncates (5, 6, 8, 10) | the donor's integer is exact on its scale and is not on ours; its truncation is an artefact of the scale, not a mechanism | B.5.4's fit of the three coordinates |
| LMR's window term, the unit by node type | 163 at a non-PV node, `delta × 577` at a PV node | `delta × 577` at every node | the donor's one-unit window is a null window; ours can be a PV node's | B.6's joint surface |

### A1.6 Predictions, frozen 2026-11-09, and calibration owed

P1 to P12 stay as frozen and are scored as written in their entries.
Owed there: P3 missed on the failed share (70% against at most 48%) and
held on re-searches (2.93 in 2.0 to 3.5); P12's "gate not met (70%)" is
already contradicted at ticket 3; P6 holds at ticket 3 on the probe.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| A1 | Ticket 3 as amended reproduces the probe: `bench 13` 4,022,487, WAC 249, 2.93 re-searches per window | high | a difference is an arithmetic difference between the probe and the implementation, to be found before ticket 4 |
| A2 | What research now expects of the unfitted paired run, beside P1 and not instead of it: **−8 Elo**, 80% interval [−22, +6], probability positive 20%. The seed's width costs a fifth more nodes in undecided positions; root reduction gives some back | low-moderate | below −20 is P1's stop; above +6 says fixed-depth nodes overstate the width's cost |
| A5 | The width run, 19000 against 1410: **+10 Elo**, 80% interval [−2, +22]; at +5 or more with its interval excluding 0 with 55% | low-moderate | at 0 or below, the node read does not carry to games here |
| A3 | B.5.4's fit moves `B5AspBase` up: at or above 2820 with 80%, at or above 4000 with 65% | low-moderate | a fitted base at or under the seed says the node read misleads on this coordinate (question 2's falsifier) |
| A4 | §9 item 2's paired run at the seed reads the reduction within ±6 (55%); positive with 55% | low | at +5 or more with its interval excluding 0 it is adopted, and re-read after the fit under §A1.5's clause |

### A1.7 Artifacts (`tools/results/b5-20261109/amend1/`, ignored storage)

`a1_build.py` (the probe's patch; `a6877f8a…`), `a1_read.py` and
`a1_read.json` (the fifteen points and the corners), `a1_deep.py` and
`a1_deep.json` (the screens to depth 19 and depth 17 at three bases),
`a1_screen25.py` and `a1_screen25.json` (the screens in §R's form),
`suite17-b9870.json` and `suite17-b19000.json`, `a1_table.py` (the
per-position reading), `a1_two_positions.py` and `two_positions.txt`
(`suite_v2` 42 and 50 on the OFF arm, on today's loop on the record and
on the probe), `*-compare-t2.md` (the rows against ticket 2),
`b1410-floors/`, `control-ticket3-spin6.log`, `SHA256SUMS.txt`. Probe
binaries: `build/b5-probe/a1-rel` (`bfd5c9c2…`), `a1-diag` (`5a897b86…`).

## Implementation record — tickets 3 to 9, on research amendment 1 (2026-11-09)

Continues the record of tickets 0 to 3 (above, at Return 1), on §A1.5. Branch
`b5-root-aspiration`. Builds: the release preset, PEXT portable, Clang, non-PGO
`all` trees; the sanitizer trees are the `debug` preset, ASan and UBSan. Per-ticket
reads used `build/b5-{on,off}`, `-diag`, `-tune`. The final table used a detached
worktree at `c6f2440` with every flavour built from nothing (`build/b5-clean`).
Evidence: `tools/results/b5-20261109/{t3c,t4,t5,t6,t7,t8,t9,qual}/` (ignored
storage); `qual.py` holds the final table's recipe, `qual.out` and `qual2.out` its
output, and `qual/hashes.json` every binary's SHA-256. No game, SPSA or NPS read
ran.

### Per ticket

| Ticket | Commits | OFF `bench 13` | ON `bench 13` | CTest OFF / ON | Settings read |
|---|---|---:|---:|---|---|
| 3, amended | `4e17bdf` engine, `8222878` tools | 4,144,790 | **4,022,487** | 28/28, 28/28 | |
| 4 | `9df2cc7` engine, `8926890` the arm's floor, `4c4f4dc` tools | 4,144,790 | **3,920,652** | 28/28, 28/28 after `8926890` | |
| 5 | `f9845b0` | 4,144,790 | 3,920,652 | 28/28, 28/28 | `B5FailHighReduce` 1: 2,889,790 |
| 6 | `62df532` | 4,144,790 | 3,920,652 | not run alone; ticket 7's | `B5NegExtMax` 1: 3,585,026; 2: 4,170,358 |
| 7 | `ee346b1` | 4,144,790 | 3,920,652 | 28/28, 28/28 | |
| 8 | `c6f2440` | 4,144,790 | 3,920,652 | 28/28, 28/28 | |
| 9 | none: no source changed | | | | below |

Both arms at all 24 commits since the entry head, each built and read in order
(`t9/every_commit.txt`): the OFF arm 4,144,790 at every one, no build failure.

### Ticket 3 against the probe (§A1.5's reads)

Prediction A1 hits on every figure, so no arithmetic difference is left to
explain. `bench 13` 4,022,487; WAC at 100,000 nodes 249; `suite_v2` at depth 14,
2.93 re-searches per window, failed share 70.4%, fail-high re-search share 49.2%,
7,052,433 interior and 4,873,092 quiescence nodes; the floors grid 600 / 598 / 600
/ 595, five budgets 119; nothing unfinished at depth 6 at the seed or at 500/32,
500/96, 40000/32 and 40000/96, with the options shown to reach the search on the
Tune build (`t3c_corners.py`: node counts move with them).

Ten Cluster 1 to 3 rows over 20% against ticket 2, the ten §A1.2 names:
`cont_hist_pruned` −51.6%, `corr_cont2_updates` and `corr_cont4_updates` −21.7%,
`in_check_nodes` −25.0%, `null_tries` +28.3%, `null_cuts` +29.4%, `sing_multicut`
+24.9%, `null_region_refusals` −75.4%, `follow_pv_nodes` +59.8%,
`qs_evasion_nodes` −23.2%. Cause: the narrow, re-searched root (§A1.2). Why
`cont_hist_pruned` halves is still not found. Reported, not repaired.

### Ticket 4's reads

Against ticket 3, on `suite_v2` at depth 14: nodes 0.980 (11,689,507, 0.973 of the
OFF head's 12,010,312); 2.77 re-searches per window; failed share 69.6%; fail-high
re-search share 45.9%. `root_lmr_reduced` 20,151, `root_lmr_researched` **10**.
`root_effort_hi` 124 and `root_effort_lo` 570 of 1,049 windows. The floors grid
passes, 600 / 598 / 600 / 597, five budgets 118. WAC at 100,000 nodes **263**, WAC at
depth 6 **199** (OFF 149). One row over 20%: `null_region_refusals` −64.1%, a share
of 0.0005%.

The depth-6 count rises because root reduction lets a later root move search past
the first move's depth, with the PV ply and a negative reduction, up to three plies.
So depth means more at the root on the arm. The same reason is why so few reduced
root moves are searched again: where `d ≥ new_depth` the only re-search is with the
full window. This is reasoned from the code and the counters, not separately read.

### Ticket 5's read

`asp_fail_high_research_nodes` is 45.9% of windowed nodes at depth 14: §9 item 2's
gate is met, as §A1.4 expected, and its default stays 0.

### Ticket 6's reads

| `B5NegExtMax` | nodes at depth 14 | of 0 | `negext_total` | chain 1 | chain 2+ | the screen, §R's form |
|---:|---:|---:|---:|---:|---:|---|
| 0 | 11,689,507 | 1.000 | 303,535 | 83,095 | 36,215 | PASS |
| 1 | 10,978,840 | 0.939 | 181,660 | 0 | 0 | PASS |
| 2 | 11,893,709 | 1.017 | 266,032 | 81,834 | 0 | PASS |

The clean-frame test walks the entry paths (null move, ProbCut, singular, quiescence)
at depth 8 over 20 `suite_v2` positions, with the counters showing each walked. Each
planted stale line aborts the node check: the null move, ProbCut, the move loop and
after the reduced search. The singular verification shares its node's frame and has no
site of its own.

### Mutants

Killed: the width's growth truncated to whole units; the width rounded down; LMR's term
keyed on the width again; root reduction off; the ceiling ignoring its limit; a later
line reaching the table; a later line with no window; the lines not sorted; the proven
mate not kept; ticket 1's two owed mutants, quiescence's and the search's cap evaluating
in check. Two survivors, both explained. First, two ticket 6 anchors hit the pricing
probe, which this build compiles out, and were replaced by the null move's own site.
Second, "a later line reaches the table" survived my first test, whose assertion admitted
a reordered line; the test was tightened to a position with an unmistakable best move
and the mutant then died.

### The two-arm table (clean tree at `c6f2440`)

| Read | OFF | ON |
|---|---:|---:|
| `bench 13`, Release | 4,144,790 | 3,920,652 |
| `bench 13`, Tune | 4,144,790 | 3,920,652 |
| `bench 13`, Diag | 4,144,790 | 3,920,652 |
| `bench 13`, Ablate | 4,144,790 | 3,920,652 |
| `bench 10`, sanitizer | 1,310,057 | 1,212,940 |
| CTest, Release | 28/28 | 28/28 |
| CTest, sanitizer, `-LE strength` | 26/26 | 26/26 |
| WAC at depth 6 (floor 136; arm 186) | 149 | 199 |
| WAC at 100,000 nodes | 253 | 263 |

The OFF arm's WAC at 100,000 nodes reads 253 where the packet says the head reads 254.
It is not this work: built at the entry commit `9c9c233`, the same read matches the final
OFF arm on all 300 records (`off_wac.py`). The 254 was read on some other binary, which I
did not identify.

The arm with `B5Window`, `B5RootLmr` and `B5LmrWindowUnits` all 0 reproduces the OFF arm's
node counts exactly on the two positions read (`blowup.py`: rec1 19,340,283, KBN-K
17,359,875). `B5RootLmr` 0 alone reads ticket 3's 4,022,487.

### Ticket 9 and the rest of the qualification

- **The mask.** No bit is re-declared: bit 7 is `lmr`, and root reduction falls under it.
  All eight bits are live on both arms (`t9/ablation.json`).
- **Liveness.** `bench 13` moves for each: `B5AspBase` 2410 3,493,675; `B5AspMsqDiv` 3874
  4,734,646; `B5AspGrowth` 64 3,298,290; `B5FailHighReduce` 1 2,889,790; `B5NegExtMax` 1
  3,585,026 and 2 4,170,358; `B5Window` 0 5,135,809; `B5RootLmr` 0 4,022,487;
  `B5LmrWindowUnits` 0 3,856,870. `B5FailHighStop` cannot move a bench, which has no clock;
  its replay of six gate games fires 122 stops at 100 and none at 0.
- **The floors grid** PASS: 600 / 598 / 600 / 597, five budgets 118; the wrong-bishop draws
  pass. KR-K sits on its floor, as on every tree read here.
- **The depth-bound read**, final arm: nothing unfinished of 300 at the seed or the four
  corners.
- **The cost screen**, §R's form: PASS on all five, no stop, no depth missing. Steps 20 to 22
  geometric means 1.102, 0.882, 0.793, 1.046, 0.894; absolute at 22 recorded, not judged:
  2.340, 0.473, 0.248, 0.225, 2.737. Position 5 did not fail, so its side read was not owed.
- **The won-ending clock test:** rec1 at `30000+300` 3,570 ms, under 8 s, the win kept.
- **The deep-line read.** No search reached the cap within 10 s, or at 30 s, on 22 endings
  tried: the deepest line reads seldepth 121. So the stack was read instead, in a throwaway
  tree recording the frame address at node entry over 15 s searches: 843 bytes per ply to ply
  126 (max ply 127) on KNN-KP, 1,012 per ply on a middlegame. At 128 plies that is about 126
  KiB of the 8 MiB the executable reserves. The read does not include callees below the node.
- **The decision trace** on WAC.001, 085, 203 and 253 (depth 8, the WAC move as root, Diag):
  ON 479 / 294 / 792 / 116 records, OFF 479 / 749 / 1,002 / 158, status ok on all;
  `qual/trace_{on,off}_WAC.*.json`.
- **Not run.** CI: nothing is pushed, so it first runs at the PR, the arm's cell included. NPS:
  P10 is the maintainer's.

### The battery misses §10's line (recorded, and the review should weigh it first)

§10's line is "no search over 0.5× the maximum (recorded; the stop's decision stays M1's)". At
`60000+600` with `B5FailHighStop` 0 the arm reads **1.001** of the maximum on rec1 and on KBN-K.
This is the arm's own, and the stop at its default does not help:

| Share of the maximum, `60000+600` | rec1 | rec2 | rec3 | KRB-K | KR-K | KBN-K | worst |
|---|---:|---:|---:|---:|---:|---:|---:|
| OFF arm, stop 100 | 0.100 | 0.060 | 0.167 | 0.098 | 0.102 | 0.167 | 0.167 |
| OFF arm, stop 0 | 0.099 | 0.061 | 0.169 | 0.097 | 0.096 | 0.167 | 0.169 |
| The arm, stop 100 | **1.001** | 0.042 | 0.104 | 0.120 | 0.085 | **1.001** | 1.001 |
| The arm, stop 0 | **1.001** | 0.041 | 0.105 | 0.120 | 0.085 | **1.001** | 1.001 |

At `30000+300` with the default stop KBN-K also reads 1.003, 14.7 s of a 30 s clock; the clock
test proper, rec1, passes there at 0.227.

What it is. Neither search spins: under a 120 s watchdog at fixed depth both finish
(`blowup.py`). On KBN-K the arm finishes depth 54 at 4.4M nodes, and depth 55 costs 115.7M and
ends in mate 34; the OFF arm has held mate 33 since depth 50 at 17M. On rec1 depth 26 is 16.9M
and depth 27 118.9M, still cp 19871. One root search runs from 5.9 s to the maximum. The won
tier's window is open, so there is no fail high for the stop to act on, and the driver reads the
clock only between iterations. This is the hole the stop packet names (A-47), reached on the arm
at clocks where the OFF arm does not reach it.

No rule owns it. One switch moved removes it nearly everywhere (depth 27 on rec1: `B5RootLmr` 0
14.9M, `B5LmrWindowUnits` 0 20.5M, `B5NegExtMax` 1 11.2M, `B5FailHighReduce` 1 22.2M, `B5AspBase`
19000 20.6M, `B5AspMsqDiv` 12000 20.0M), yet `B5Window` 0 costs 154.1M there and ends in mate 23,
and on KBN-K three of them leave it unchanged. That reads as one late root resolution falling
differently, the scatter §A1.3 describes, rather than a mechanism; two positions cannot say more.

At the gates' clock it does not show. Six gate games replayed at `3+0.03`, 472 searches:

| Replay | spent | over 0.5× the maximum | at the maximum | stops fired |
|---|---:|---:|---:|---:|
| OFF arm, stop 100 | 31,427 ms | 18 | 5 | 46 |
| OFF arm, stop 0 | 32,893 ms | 21 | 1 | 0 |
| The arm, stop 100 | 31,438 ms | 16 | 2 | 122 |
| The arm, stop 0 | 35,432 ms | 31 | 8 | 0 |

Why this is not a Return. §10 marks the line recorded; the time manager and the hole's repair are
non-goals here (D.1), the stop is M1's, no floor failed, and nothing moved it that this leaf owns.
My position: proceed to the review with this at its head. A reviewer who reads the line as a gate
should return the leaf, and that would be a fair reading. It bears on B.5.3 and on the long clocks of
the direction run.

Two things for BAS-R10 to expect. Six games is no measure, but the arm's stop fired on 122 of 472
searches (25.8%; the OFF arm 9.7%), against P8's 1 to 6%. The stop's removal costs the arm more than it
costs the OFF arm (31 long searches against 21).

### Predictions, as read here

| # | Read | Against the frozen |
|---|---|---|
| P3 | 2.77 re-searches per window; failed share 69.6%; give-up nodes 0 | re-searches inside; failed share misses, above the falsifier's 58% (§A1.6) |
| P4 | geometric mean **0.924** against the OFF head's census over the 105 positions with nodes (`p4.py`; ticket 3 0.956), median 1.142, total 0.973; **0.05%** of reduced root searches searched again | the mean inside 0.80 to 0.97, though the median says the typical position costs more (§A1.3's two won positions carry the mean); the share misses 5 to 15% badly |
| P5 | `B5NegExtMax` 1: −6.1% at depth 14; the screen passes | inside on nodes and the screen; selective depth not read |
| P6 | WAC at 100,000 nodes 263 | one above the range's top; the falsifier is under 246 |
| P7 | the grid passes | hit |
| P9 | no ply-cap return in any census or bench | the games' rate is BAS-R10's |
| P11 | `root_effort_hi` 14.8% to 17.2% of windows on the bench positions | inside at bench depth; the games' rate is owed |
| A1 | every figure | hit |

### Deviations from the handoff

Those of the record at Return 1 stand; §A1.2 accepts them. New:

- **Ticket 4's chain test reads the whole root**, not `searchmoves` of two moves. A second move is
  almost never reduced: over five pairs on each of the first positions a reduced second move appeared at
  most three times in a position and no chain at all. The whole root gives chains over the bench
  positions at depth 14.
- **The arm's own WAC floor at depth 6, 186** (`8926890`, its own commit): root reduction moved the count
  to 199, above the old stale bound, as Cluster 2's and 3's arms did. The OFF arm's bounds are unchanged.
  The floor is that count less 13 by the file's own rule; research did not set it.
- **Ticket 6's planted stale line is mutants, not a test in the tree.** The check aborts the process, so
  a planted failure cannot live in a passing test binary.
- **Ticket 7.** A move that was no line's last iteration holds the "none" previous score, which §3's mate
  test does not mean: it searches under its window, or open when it has no average yet. Only line 1 prints
  bound lines, counts the root's table probes, or can end the search on a fail high. Only finished lines
  are sorted, so a stopped line cannot displace one.
- **Ticket 8 landed**, on a fixture found by scanning 462 positions at 2M nodes (one position loses a proven
  mate): `r1bq1r2/pp4k1/4p2p/3pPp1Q/3N1R1P/2PB4/6P1/6K1 w`, mate 8 at depth 15, cp 2607 at depth 16. The test
  fails without the guard. Two differences from the donor. It reads line 1's search, before the later lines
  where the donor reads after them; a later line overtaking a lost mate in the same iteration is not covered.
  And there is no "inexact" clause, since this engine discards a stopped iteration and has no completed
  inexact one.
- **Ticket 9 changed no source** and has no commit of its own.
- **`B5FailHighStop`'s liveness is a replay, not a bench**, which has no clock.
- **`B5AspBase`'s SPSA step** is the generator's 2,469 (a sixteenth of the range), larger than the seed
  1,410. Left as generated. BAS-S54 may want to read that before it starts.

### What the reviewer must check

1. **The battery, first:** whether §10's line gates this leaf, and what the arm's long searches in won endings
   mean for B.5.3. Rerun `battery.py` and `blowup.py`.
2. Reproduce every fingerprint and CTest of the table from a clean tree, on both arms and every flavour.
3. Ticket 3's arithmetic in `search_b5.h` against §A1.5, and the range property's grid in `test_b5_root.cpp`;
   rerun `tools/diag/find_spin.py` at the seed and the four corners, as §A1.5 asks.
4. Each ticket's diff against §10 and §7, one mechanism per ticket; that the OFF arm's code changed only at
   §10's seams. The driver's `search_line_window` lambda and the MultiPV branch deserve the slowest read.
5. Ticket 8's two differences from the donor, and whether the fixture earns its test.
6. The arm's WAC floor of 186, which I set by the file's rule.
7. The invariants of §10: a draw before the cap, no store for a later line, the stop discarding its iteration,
   `MultiPV 1` line for line.
8. CI has not run. The arm's cell first runs at the PR.

- Review: **accepted with named follow-ups**, 2026-11-09, by an independent review in a separate session (*Review —
  B.5.2 accepted with named follow-ups*, below). Reproduced from that session's own clean tree at `c6f2440`: `bench 13`
  4,144,790 (OFF) and 3,920,652 (ON) on Release, Tune, Diag and Ablate; `bench 10` sanitizer 1,310,057 and 1,212,940;
  CTest 28/28 on both arms on Release, Tune and Diag, 26/26 under the sanitizers; WAC 149 and 199 at depth 6, 253 and
  263 at 100,000 nodes; six commits on both arms, ticket 3's two forms among them (5,039,771 and 4,022,487); the floors
  grid, the census, the cost screen, the depth-bound read, the clock test and the battery, each as this record reads
  them. The battery's miss does not gate this leaf; it is systematic, and research amendment 2 is owed before M1.

## Review — B.5.2 accepted with named follow-ups (2026-11-09)

Independent review of B.5.2 at `c6f2440` (`c7a034b` records it and changes
no source), in a separate session from the implementing one (§10
*Review*; the record's *What the reviewer must check*). Nothing was
fixed. No engine source, default, seed, floor, test or clause was moved,
and no game, SPSA or NPS read ran. Every tree was built by this session
from a detached worktree of `c6f2440` (`build/b5-review`; release preset,
PEXT portable, Clang 23.1.5, target `all`; the `debug` preset for the
sanitizer trees). The probe of §6 is a throwaway tree
(`build/b5-review-probe`, `rv_probe_patch.py`). The clock reads ran on a
quiet host, the node reads beside each other. Evidence:
`tools/results/b5-20261109/review/` (ignored storage).

**Verdict: accept, with the follow-ups of §9.** Every fingerprint and
test reproduces from clean sources on both arms and every flavour. The
code is §10 as §A1.5 amends it, one mechanism a ticket, the OFF arm
edited only at shared seams, no constant outside §7 moved. The
invariants hold. The battery's miss reproduces, and it is larger than
the record says: it is systematic, not two positions. It still does not
return this leaf. §10 marks the line recorded, the arm's long searches
are the mechanism §5.4 describes and not a defect of the code, and
nothing this leaf owns can change them. What it breaks is registered
for the next leaf: M1's rule has no admissible option on the arm, and
nothing between here and the flip reads a long clock. That needs a
research amendment before M1 runs, not a repair here.

What would change this: a defect in the arm's code behind the long
iteration (none was found: both are open-window iterations, as §5.4
says they would be), or a floor failing on a reread (none did).

### 1. The battery, first

**Reproduced** (`battery.py` on this session's Tune builds, `60000+600`,
share of the engine's own maximum):

| | rec1 | rec2 | rec3 | KRB-K | KR-K | KBN-K |
|---|---:|---:|---:|---:|---:|---:|
| OFF arm, stop 100 | 0.104 | 0.067 | 0.153 | 0.101 | 0.101 | 0.173 |
| OFF arm, stop 0 | 0.102 | 0.066 | 0.178 | 0.102 | 0.100 | 0.172 |
| The arm, stop 100 | **1.002** | 0.043 | 0.105 | 0.121 | 0.077 | **1.002** |
| The arm, stop 0 | **1.002** | 0.045 | 0.111 | 0.114 | 0.080 | **1.002** |

At `30000+300` (Release, default stop) the clock test passes on the arm
(rec1 3,546 ms, the win kept) and KBN-K reads 1.003; the OFF arm's worst
is 0.288. `blowup.py` on this session's trees reproduces every node
count and score of the record's.

**What the two searches are** (the logs' own lines):

- rec1. The OFF arm holds `e5e6` from depth 13 and stops after depth 20
  at 3.3 s, 0.70 of the optimum, on the stability discount. The arm's
  best move changes at 7 of the 10 iterations from depth 17 to 26, among
  `e3f4`, `e5e6` and `f7d7`, all in the won tier. So the discount is
  lost and the instability term extends the budget: depth 26 ends at
  6.13 s, 1.30 of the optimum, and the depth-27 search then runs 26.2 s
  to the maximum without returning.
- KBN-K. The arm reaches depth 54 in 1.43 s, `b1c3` since depth 43, and
  its depth-55 search runs 28.6 s to the maximum. At fixed depth that
  search costs 111.3M nodes and ends in mate 34. The OFF arm proves mate
  34 at depth 49 in 12.8M nodes and stops at 5.2 s. At this clock the
  arm plays `b1c3` on `cp 16318` where the OFF arm announces the mate.

Both are one open-window iteration: no fail high, nothing for the stop
to act on at any setting, and the clock read only between iterations.
That is §5.4's own description. What §5.4 and §10 did not expect is the
clock at which the arm meets it.

**Widened, because two positions cannot say whether it is scatter.**
Sixty won endings from the `10+0.1` direction run's games
(`rv_wide.py`: one a game in file order, the first position where the
mover reports 1,500 or more and no mate with at most eight men;
`wide_positions_60.json`), each searched from a fresh process, Threads
1, Hash 64. "Changes" is root best-move changes per completed iteration.

| `60000+600`, 60 searches | over 0.5× the maximum | at the maximum | spent | changes |
|---|---:|---:|---:|---:|
| OFF arm | 1 | 1 | 252.0 s | 0.125 |
| The arm | 5 | 3 | 341.5 s | 0.225 |
| The arm, `B5RootLmr` 0 | 2 | 2 | 268.0 s | 0.173 |
| The arm, `B5Window` 0 | 4 | 2 | 322.2 s | 0.166 |
| The arm, `B5NegExtMax` 1 | 0 | 0 | 193.1 s | 0.183 |
| The arm, root searches never past the full depth (probe) | 4 | 3 | 308.1 s | 0.168 |

At `10000+100`: the OFF arm none over half the maximum, 40.3 s, 0.170
changes; the arm 2 over half, none at the maximum, 52.5 s, 0.281.

Reading it:

- **It is systematic.** The arm spends a third more clock on won
  endings at both clocks, changes its root move 1.7 to 1.8 times as
  often, and meets the maximum three to five times as often. The counts are
  small (5 against 1); the spend and the change rate are not.
- **The hole is the OFF arm's too**: one of the sixty is at the maximum
  on both arms. The arm reaches it sooner and more often.
- **No switch owns it**, as the record says. Root reduction and the
  window each carry part of the change rate, and removing the root's
  searches past the full depth brings the rate to the level of no root
  reduction without moving the count at the maximum.
- **`B5NegExtMax` 1 removes the tail** on this sample and spends less
  than the OFF arm. That is zero-game evidence for §9 item 1 and moves
  no default.
- **`blowup.py` cannot attribute.** It reads each setting at one depth,
  the arm's own last. On KBN-K no setting that reads cheap there has
  found the mate by depth 55, which is what the arm's depth 55 pays
  for. The table above reads each switch under the clock instead.

**At the gates' clock it does not show**, on three times the record's
sample (`replay_games.py`, twenty gate (a) games at `3+0.03`, 1,226
searches):

| Replay | spent | over 0.5× the maximum | at the maximum | searches the stop ended |
|---|---:|---:|---:|---:|
| OFF arm, stop 100 | 89,018 ms | 42 | 13 | 10.8% |
| OFF arm, stop 0 | 95,289 ms | 63 | 17 | — |
| The arm, stop 100 | 90,282 ms | 37 | 10 | 25.1% |
| The arm, stop 0 | 100,385 ms | 65 | 17 | — |

**What it means.**

- *For this leaf:* not a gate. The code is the contract's, and no repair
  inside §10's non-goals exists.
- *For M1 (BAS-R10):* PLAN gates every option of the stop's rule on a
  battery under 0.5× the maximum, and the arm fails it at every setting
  by searches the stop cannot reach. As registered, M1 has no
  admissible option on the arm; removal is closed outright. The rule
  must be amended before M1 runs. P8 is already contradicted: the stop
  ends 25.1% of the arm's searches on twenty games, against 1 to 6%
  predicted. At that rate the registered rule sends the arm to 2×
  against 1×.
- *For B.5.3's paired runs at `3+0.03`:* valid as registered. Both arms
  spend the same clock there with the stop at 100. They are blind to
  the long clocks, and the stop is carrying the arm's clock at that
  control: a moved stop moves the arm's time use more than the OFF
  arm's.
- *For the direction run (d) at `10+0.1`:* the arm's extra spend on won
  endings is present at that clock (1.30× on this sample). The run is
  direction only and reads Elo, so it would show a large cost and miss
  a small one.
- *For the long clocks:* nothing registered between here and the flip
  reads `60+0.6` except this battery.

Position: research amendment 2 before M1 and before any long-clock
read (§9, item 1). The whole-arm paired run does not wait on it. D.1's
in-iteration clock test is not pulled forward on this evidence.

### 2. Rebuilt from clean sources, every row reproduced

| Read | OFF | ON | Record |
|---|---:|---:|---|
| `bench 13`, Release / Tune / Diag / Ablate | 4,144,790 × 4 | 3,920,652 × 4 | equal |
| `bench 10`, sanitizer | 1,310,057 | 1,212,940 | equal |
| CTest, Release | 28/28 | 28/28 | equal |
| CTest, sanitizer, `-LE strength` | 26/26 | 26/26 | equal |
| CTest, Diag and Tune (the counter-reading tests run only here) | 28/28, 28/28 | 28/28, 28/28 | not in the record's table |
| WAC at depth 6 | 149 | 199 | equal |
| WAC at 100,000 nodes, 300 records | 253 | 263 | equal |
| Depth-bound read, seed and four corners | — | 0 of 300 unfinished at each | equal |
| `find_spin`'s self-test; `test_replay_games`, `test_run_suite`, `test_spsa_surface` | pass | pass | |
| Floors grid on the arm | — | PASS: 600 / 598 / 600 / 597, five budgets 118; the wrong-bishop draws pass | equal |
| Depth-14 census on the arm | — | 2.77 re-searches per window; failed share 69.6%; fail-high re-search share 45.9%; 11,689,507 nodes; `root_lmr_reduced` 20,151, `root_lmr_researched` 10 | equal |
| Cost screen, §R's form | — | PASS on all five, no stop, no depth missing; steps 20 to 22 geometric means 1.102, 0.882, 0.793, 1.046, 0.894 | equal, number for number |

Six commits rebuilt on both arms in their own worktrees (`rv_more.py`):
`e767c0a`, `03b4a1a`, `9a77439`, `4e17bdf`, `9df2cc7` and `62df532`. The
OFF arm reads 4,144,790 at each; the arm reads 4,144,790, 4,144,790,
**5,039,771** (ticket 3 as the handoff wrote it), **4,022,487** (ticket 3
amended), 3,920,652 and 3,920,652, as the records say. The other
eighteen of `t9/every_commit.txt` stay the record's claim.

`find_spin.py` writes no node count, so a clean read does not show its
options reached the search. They do: `go depth 14` from the start
position reads 140,035 nodes at the seed, 203,738 at 500/32 and 82,043
at 40000/96 on the Tune build. `bench` ignores UCI options by design.
Every B5 option but the stop moves a depth-15 search on the Tune build.

Not rerun: the mutants, the deep-line stack read and the decision
traces. They stay the record's claims.

### 3. The code against §10, §7 and §A1.5

Read whole: `search_b5.h`, the driver's and the kernel's diffs from the
entry head, the parameter table, the CMake and CI edits, the tests.

- **One mechanism a ticket**, by each commit's files and diff. Ticket 4
  also adds the root's trace event its test reads.
- **The OFF arm** changes only at §10's seams: the counters, the stop's
  option (`b5_fail_high_stop / 100.0` of the optimum, identical at
  100), the ply cap's repair, `root_search`'s second depth (equal to the
  first), the hoisted bound-line and stop lambdas, the timing wrapper.
  Every `kB5Root` branch folds to today's form.
- **Constants**: §7's rows as §A1.5 amends them, each clamped to its
  range at the UCI boundary; the donor's 12, 24, 16 and 32 in the
  average, checked against `sf_19` `:1437-1468` line for line; 577 and
  163; nothing else moved.
- **`search_line_window`**: the donor's loop. The bounds use the width
  before it grows; a fail low sets `failed_high_cnt` to 0; only line 1
  prints bounds, counts table probes or can be ended by the stop. The
  reference it holds into the record is read only before the first
  search, so the sort inside `root_search` cannot stale it. A move with
  no average yet gets the open window through the "none" mean square.
  The loop ends by construction: at the cap the window is open within
  two more failures (§4).
- **The MultiPV branch**: line `k` searches the record from index
  `k − 1`, which excludes the finished lines; the finished lines are
  sorted among themselves after each line, so a later line can overtake
  the first and the result follows it; the root stores nothing for a
  later line. At `MultiPV 1` the loop does not run and the path is the
  single-PV one. Probed beyond `test_multipv`, on WAC's first forty
  positions at `MultiPV 4` and depth 9 on both arms: at every depth the
  lines are numbered once each, distinct and best first, and `bestmove`
  is line 1; `MultiPV 1` against no option is line for line on twenty
  positions; five lines asked of two `searchmoves` report two.
- **The record**: effort before the blend, the first move or a move
  above alpha writing score, bounds and line, everything else "none";
  the stop returns before the bookkeeping, so a stopped search writes no
  score. The node check in `root_search` binds the record's first line
  to the kernel's own inside the window.

Differences the records do not list, none a change of mechanism:

1. **The helper's filter counts differently on the arm.** The record
   numbers the moves that pass `searchmoves` and the tablebase filter,
   in generation order; the OFF arm numbers in the picker's order,
   before the tablebase filter. The pool sets no filter today, so
   nothing plays differently. D.2's.
2. **`multipv_lines` is counted without the filter**, so a filtered
   main thread with MultiPV above its record's size would index past
   it. Unreachable today, for the same reason. D.2's.
3. **The arm sorts with `std::stable_sort` at the root**, which may
   allocate. §10's invariant is "below the root", and it holds.

### 4. Ticket 3's arithmetic and the range property

`search_b5.h` is §A1.5: thousandths in 64 bits, truncating divisions,
the cap `1000 × kMaxWindowDelta`, `delta = (width + 999) / 1000`. The
tests bind the widths §A1.5 lists.

The grid test bounds the loop, and the property holds beyond the grid.
The width grows whenever `growth × width ≥ 128`; the smallest registered
width is 500 and the smallest growth 32, so the step is at least 125
thousandths everywhere on the surface, and the options clamp to their
ranges. From the cap the loop needs at most two more failures: a fail
low then sets alpha to −INF, and the fail high after it sets beta to
INF. So no search fails more than 62 times, 55 at the worst corner. Two
notes: the grid's divisor loop is vacuous at a level score, where the
mean square is 0 (it costs nothing and proves nothing); and the test
grows the width in isolation, with the loop itself sampled by the
depth-bound read.

### 5. The deviations, on their merits

- **Ticket 8's guard reads line 1's search.** Accepted. At `MultiPV 1`
  it is the donor's test, on both signs: a mated score is as much a
  proof here as there, since the move loop stops pruning while the best
  score is a proven loss (`best_score > -tablebaseWinInMaxPly`), as the
  donor's does. With
  more lines, a later line that overtakes with a faster mate is taken
  (the sort), but the guard's memory is not told, so that mate is not
  protected in the next iteration. Analysis mode only; a departures
  row (§9). The missing "inexact" clause is right: a stopped iteration
  is discarded here, so no completed line is inexact.
- **The fixture earns its test.** Stockfish (a development build, 8
  threads, 20 s) proves mate in 8 by `f4g4` at depth 90. The arm proves
  it at depth 15 and, by the record and its mutant, reads 2607 at 16
  without the guard. The OFF arm holds the mate from depth 14 to 18:
  the forgetting is the arm's own, which is what the guard is for.
- **The arm's WAC floor of 186.** Accepted as the file's rule, with its
  meaning stated: depth 6 is not the same search on the two arms (§6).
  The floor is a tripwire for the arm's own tree. The like-for-like
  tactical read is WAC at 100,000 nodes, 263 against 253.
- **Ticket 4's chain test on the whole root.** Accepted: the assertions
  are §10's (the re-search at the full depth give or take the margin's
  ply, then the node's window), and a root of two moves almost never
  reduces its second. Its `again.result > again.alpha` reads the reduced
  search's score, which is what triggers the re-search.
- **Ticket 6's planted line as mutants**, `B5FailHighStop`'s liveness
  by replay, ticket 9 without a commit: accepted as recorded.
- **The counter-reading tests run in no CI cell.** The chain test, the
  clean-frame and limit tests, the MultiPV window count and the no
  give-up test need the counters, so they run on Tune and Diag builds
  only; CI's Diag cell runs `test_uci_protocol` alone and the arm's cell
  is a Release build. This is B.3's and B.4's practice too. They pass
  here on all four such trees.

### 6. The two readings, measured; `cont_hist_pruned`

The probe is the arm with counters on the root's later moves and two
environment switches; with neither set it reads `bench 13` 3,920,652.

**Why so few reduced root moves are searched again.** The record's
reason is not the cause. On `bench 13` the root searches 26,955 moves
after the first: 6,547 reduced, 5,729 at the full depth, 13,533 past it
(by 1.40 plies on average) and 1,146 outside the reduced path. Of the
6,547 reduced, **11 fail high (0.17%), and all 11 are searched again**.
A reduced root move almost never beats alpha, because alpha at the root
is the best move's own exact score and the reduced moves are the late
ones. P4's 5 to 15% was an interior node's rate. What the record
describes is where the new best moves come from: 123 of the 190 the
root's later moves produce are moves searched past the full depth.

**Why WAC at depth 6 reads 199.** The record's reading holds. With the
root never reduced the arm solves 143; reduced but never past the full
depth, 145; as built, 199. At depth 6, 84% of the root's reduced-path
searches go past the full depth, by 1.63 plies on average. "Depth 6" on
the arm is a deeper search for most root moves.

**Why `cont_hist_pruned` halves.** It is a population effect, and not
the narrow window. On `suite_v2` at depth 12 (Tune builds) its share of
move-loop tests reads 1.700% on the OFF arm, 1.022% on the arm, 1.400%
with `B5Window` 0, and falls further as the window widens (0.689% at
base 19000, 0.624% at 40000). Eight positions carry 73,050 of the OFF
arm's 100,840 prunes. Positions 42 and 50, the two won positions of
§A1.3, carry 43,518, and on the arm 16: the rule fires inside the OFF
arm's fail-high cascades on decided positions, and the mean-square
width removes those trees. The median position's share moves from
0.549% to 0.486%. A width effect remains in ordinary positions (0.334%
at base 19000) and is not explained here.

One more read from the same runs: the share of iterations at or under
`TmEffortLo` is 44.5% on the OFF arm and 56.6% on the arm (34.9% at
base 19000); the high share is 8.8% against 8.9%. P11 bounded the high
share alone.

### 7. The invariants, in chess terms

- **A draw before the cap, a draw in check at it.** By the tests, which
  read behaviour on every flavour: a checked side at the cap reads 0
  where its evaluation is far from 0, a fifty-move draw at the cap
  reads 0 in check and out of it, and the same move off the boundary
  reads the evaluation. `pv_len` is not written at the cap.
- **No table store for a later line.** By the code and by the queen
  fixture, whose mutant died.
- **A stop discards its iteration.** The break precedes every use of
  the iteration; the fail-high stop sets the same flag, so the move
  played is the last completed iteration's, which the stop's own
  condition requires to be the move that failed high. The record is
  mutated by a discarded iteration and is not read again.
- **`MultiPV 1` line for line.** By construction, and `test_multipv` on
  both arms.
- **`bestmove` and `ponder` legal; the last line names the move
  played.** The shared sanitation and the shared repeat line; the
  record's line is the kernel's own, and a kept mate line was legal
  from the same root.
- **Mate and tablebase bands untouched.** No static score enters them;
  the guard compares mate scores only.
- **Helpers on the record.** Each thread owns its record; the pool
  table takes the raw score as today; CTest's threading tests and the
  sanitizer trees pass on the arm. No thread sanitizer ran.

### 8. CI has not run

Nothing is pushed. Open until the pull request:

- the workflow edit itself, and the arm's cell on the runner;
- the new code under the other compilers and warnings CI builds with;
- the fingerprints on CI's platform. The arm's cell **prints**
  `bench 13` and asserts only that a count exists, as §10 asked. Its
  3,920,652 has to be read from the cell's output at the pull request;
- the universal and PGO builds of the arm;
- the counter-reading tests, which CI does not run on either arm (§5).

### 9. Follow-ups

None blocks the acceptance. Owners in brackets.

1. **Research amendment 2, before M1 and before any long-clock read**
   (research; B.5.3's opening). It answers: what gates M1's options on
   the arm, now that the battery fails at every stop setting by a
   search the stop cannot reach; why P8 missed by a factor of four or
   more; what reads the long clocks before the flip. This review's
   position on the last: register the widened battery as a BAS-S53
   screen on both arms and at `B5NegExtMax` 1, with a frozen prediction
   and a rule; read time per move by score band in the direction run
   (d); give BAS-S55's flip a long-clock condition. A prompt for the
   amendment is `review/fable_amend2_prompt.md`.
2. **§9 item 1 reads the battery beside its paired run** (research, in
   amendment 2): `B5NegExtMax` 1 is the one setting with no search over
   half the maximum.
3. **The time manager's inputs move on the arm** (research; D.1): the
   root's best-move changes nearly double in won endings and the
   low-effort share rises 12 points. Recorded for D.1, or read in
   B.5.3, as the amendment decides.
4. **§5.8's joiner rule and `cont_hist_pruned`** (research; B.5.4's
   opening): the −51.6% is a few decided positions. Read the rule by
   the median position before `cont_hist_prune_slope` joins the surface.
5. **`B5AspBase`'s SPSA step** (research; B.5.4's opening): `c_end`
   1,137 on a seed 910 above its lower bound, so the low perturbation
   clamps from the first iterate. The record raised it; it needs a
   decision before the fit.
6. **Calibration owed in the entries**: P4's share misses, for the
   reason of §6 and not the record's; P8 is contradicted at twenty
   games; P11's low share is unpredicted.
7. **§14's table** (records; B.6.1): the forgotten mate read after line
   1; the root's later moves searched past the full depth, with its
   effect on every depth-denominated read.
8. **At the pull request** (maintainer): read the arm's printed
   fingerprint; decide whether the arm's cell also runs `test_b5_root`
   on a Diag tree until the flip.
9. **D.2** (recorded, nothing now): the helper filter's count on the
   arm and `multipv_lines` under a filter (§3).
10. **`find_spin.py`** (tooling, any time): print each search's node
    count, so a read shows its options were live.
11. **`CHANGELOG.md`** (records; with the pull request): no line for
    the ply cap's repair, which is live on the default arm since
    ticket 1.

### Corrections to the implementation record

- "A negative reduction leaves `d ≥ new_depth`" is not why few reduced
  root moves are searched again (§6).
- "One late root resolution falling differently, the scatter §A1.3
  describes, rather than a mechanism" understates it: no switch owns
  it, and it is systematic (§1).
- "One switch moved removes it nearly everywhere" is not shown by
  `blowup.py`, which reads one depth a setting (§1).

### Artifacts (`tools/results/b5-20261109/review/`, ignored storage)

`rv_build.py` and `hashes.json` (the ten trees; Release OFF `d0df8b0a…`,
ON `f4377a3c…`; Tune OFF `384fdd42…`, ON `c818fb5d…`), `rv_fp.py` and
`rv_fp.json` (the table), `rv_more.py` (the six commits, CTest on Diag
and Tune), `battery-*` (the battery's logs at both clocks), `rv_blowup.py`
and `rv_blowup.out`, `rv_wide.py` (`a51cb446…`), `wide_positions_60.json`
(`581c8822…`) and `wide_*.json`, `replay-*.jsonl`, `rv_probe_patch.py`
(`35b47e55…`; the probe binary `f8654944…`) and `probe-*`, `spin-*`,
`ch-*.json` (the `cont_hist_pruned` and effort reads), `floors/`,
`suite14.json`, `screen.json`, `fable_amend2_prompt.md`.

## Research amendment 2 — answers the review of B.5.2: the stop's rule on the arm, and the long clocks (2026-11-09)

The maintainer accepted the review's positions on 2026-11-09: no engine
change now; this amendment before M1; B.5.3 then measures, and its games
choose any repair; D.1's clock test inside an iteration is not pulled
forward. Written in the reviewing session, so it leans on the review's
reads (§1 and §6 there), which were taken with no prediction frozen.
They size the questions and decide nothing by themselves; every rule
below is frozen from this commit, and each read the amendment registers
for B.5.3 and B.5.5 runs on positions and games nobody has seen.

### A2.0 Registered before its runs: two calibration reads (zero-game)

Written and committed before either read below runs.

**R1, the games census on the B.4.7 runs.** `amend2/a2_census.py` over
`tools/results/b47-direction-ltc/games.pgn` (2,000 games at `10+0.1`) and
`tools/results/b47-gate-a/games.pgn` (`3+0.03`), both arms of each run,
from the PGN's own `s=` and `t=` comments. Per arm and threshold T of
300, 800 and 1,500: the games in which that side's own score was at
least T on two of its consecutive moves ("decided for it"), and of
those the games it did not win ("unconverted"); and, per score band,
the mean time a move and the share of moves spending at least half the
clock left. No engine runs. It is an instrument's base rate on a pair
of arms that differ by Cluster 3, not a read of the B.5 arm.

- *What it sets:* the threshold of §A2.5's flip census. It is the
  highest of 1,500, 800 and 300 at which each B.4.7 arm has at least 15
  unconverted games in the 2,000 at `10+0.1`. If none has, the census is
  read at 300 and the amendment says the instrument is weak.
- *What it cannot set:* the census's bound, which is fixed here without
  it. The condition fails when the arm's unconverted games exceed the
  OFF arm's, in the run being read, by more than the larger of 6 and
  three times the square root of the OFF arm's count (the difference of
  two equal Poisson counts has a standard deviation of 1.41 square
  roots, so this is about two of them).
- *Predictions.* C1 (low): at 1,500 each arm's unconverted share of its
  decided games at `10+0.1` is 0.5 to 4%. C2 (low-moderate): the two
  arms' unconverted counts at 1,500 differ by at most 8. C3 (low): at
  300 the unconverted share is 8 to 20%. C4 (moderate): the Cluster 3
  arm, which won that run by 38 Elo, spends within 5% of the other
  arm's time a move in the won band (300 or more).

**R2, the battery's noise and the stop's reach, on the review's 60
positions.** `review/rv_wide.py` at `60000+600` on this session's Tune
builds, a fresh process a search, a quiet host: the arm at
`B5FailHighStop` 0 and 200 and at the default once more, and the OFF
arm once more. The positions are the review's, already seen, so this is
a calibration and not a test of the arm.

- *What it sets:* the tolerance of §A2.5's battery gate for the stop's
  options: the larger of 2 searches and twice the largest difference
  between two runs at the same setting, the review's runs included; and
  the same for spend, the larger of 10% and twice the largest
  difference.
- *Predictions.* C5 (moderate): the arm reads 3 to 7 searches over half
  the maximum and 2 to 4 at it at every stop setting, and spends 315 to
  370 s; the OFF arm reads 0 to 2 over half and spends 235 to 270 s. C6
  (moderate): the stop's setting moves the arm's count over half the
  maximum by no more than 2 either way, because the long searches are
  ones no fail high interrupts.
