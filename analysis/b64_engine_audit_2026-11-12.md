# B.6.4 — Engine audit on the four-cluster head

- State / class: CLOSED (analysis delivered); §12's handoff for B.6.5 is
  `READY_FOR_IMPLEMENTATION`; `R3`
- Owner / date: agent, in a session that implemented none of Clusters 1 to
  4; 2026-11-12
- Source revision: `662e5ad` (branch `b6-perft-pins`, B.6.3 recorded),
  `bench 13` **3,783,135**; the donor is `sf_19`, `edb0d9db`, read from
  `D:/code/stockfish`
- Decision needed: the maintainer's sign-off on §10's routing, of which
  one item asks for games (§6 R1, the TT-cutoff bonus's cap, in B.6.6)

**Recommendation.** Fit on the head as it stands: the audit found no defect
that moves the tree, so B.6.5 shrinks to one behaviour-neutral repair (F4's
overflow, with width guards) and closes on its tests, and the only finding
that should cost games before B.6.7 is the TT-cutoff history bonus, which
has no cap where the donor caps it and whose slope is on the fit's surface.

The main counter-argument: three donor behaviours that the head copies
faithfully (§5.3: the frame a re-entry rewrites) touch 1 to 3% of all
reduction decisions, more than any departure found. They are the donor's,
the clusters were fitted around them, and each is a refinement of a few Elo
at most, so they wait in H.2 with their counters; they are not defects.

Every claim is **evidence** (a line read, a count, a program run) unless
marked **inference** or **decision**. Line numbers are at `662e5ad`;
`kernel` is `src/search_kernel_core.cpp`. Donor lines are `src/search.cpp`
at `sf_19` unless a file is named.

## 1. Scope and method

**Read in full**, with the two lenses (chess and search semantics; C++:
undefined behaviour, overflow, lifetime, concurrency, dead or misleading
code): `search_kernel_core.cpp` (2,223 lines: quiescence, negamax, the
kernel's entry points), `search_history_core.cpp`, `history_core.h/.cpp`,
`move_picker_core.h/.cpp`, `tt.h`, `search_b3.h`, `search_b4.h`,
`search_b5.h`, `search_types.h`, `search_params.h`, `search_worker.h/.cpp`
(the driver, the clock's inputs), `search_root.h/.cpp`,
`search_thread_pool.h/.cpp`, `threats.cpp`, `constants.h`.

**Read in the donor**, line against line: `search()` and `qsearch()`
(`:721-1883`), the update functions (`:1956-2060`), `tt.cpp` (`save`,
`probe`), `movepick.cpp` (scoring and stages), `position.h` (`capture`,
`capture_stage`), `movegen.cpp` (`make_promotions`).

**Not read line by line**, and why: `board.cpp` beyond the generator's
promotion sites (B.6.3's search-path perft holds it; the board is
B.7.1's), the evaluator (B.9.3's), the protocol and `engine.cpp`
(B.9.1's), `search_diagnostics.*` and `search_pricing.*` (compiled out of
release), `syzygy.cpp`. The driver's MultiPV path was read for lifetime
and bounds only; its semantics against the donor are B.9.1's (D-73).

**Verified mechanically**:
- an instrumented build (§13): the head plus a counters patch, which
  reproduces `bench 13` 3,783,135, read three ways: `bench 13`, `bench 17`
  (18,816,178 nodes) and a self-play sequence of 610 searches at `go nodes
  250000`, Hash 64, four games of 160 plies in one process, so the table
  ages as in a game;
- F4's arithmetic as written against the same operations in 64 bits, with
  UBSan, and their equality on 8,085,737 grid points where 32 bits do not
  overflow;
- the soft limit after `ponderhit` against a plain `go` on the same clock;
- 21 feature probes on the pinned Windows compiler (Clang 23.1.5, libc++).

**Built on, not repeated**: the departures tables (B.3.4's in
`b30_cluster2_research_2026-10-30.md`, §14 of
`b43_cluster3_research_2026-11-06.md` and of
`b5_cluster4_research_2026-11-08.md`), `rarog_import_2026-11-11.md` §3,
`code_audit_2026-10-27.md` §6.2. A candidate already recorded there is not
a finding here. Recorded and confirmed by this read: the stand-pat store,
the draw value 0, the discarded partial iteration, the per-iteration
effort share, the evasion order, the capture stage's under-promotions,
the small ProbCut, the fresh tables, the extension budget.

## 2. Verdicts

| # | Finding | Kind | Evidence | Goes to |
|---|---|---|---|---|
| D1 | F4: the multi-cut correction bonus overflows `int`, and the wrapped product is a full update of the wrong sign | defect, tree unchanged | §3.1: −256 where +256 is right; UBSan; unreachable in the three reads by a factor of nine | B.6.5 |
| D2 | `ponderhit` recomputes the clock with the game ply of the node the search is at, not the root's | defect, clock input | §3.2: soft limit +4.9% to +11.3% over a plain `go` | B.9.1 |
| W1 | The widest update, the fail-low parent's continuation bonus, tops out at 97.6% of `INT_MAX` on literals; observed 87.0% | no defect; unguarded | §4 | B.6.5 (a guard) |
| R1 | The TT-cutoff quiet bonus is `151·depth` with no cap; the donor's is `min(112·depth, 695)` | unrecorded departure | §6: 15 to 31% of bonuses over the cap, 1.4 to 1.8 times the donor's total, up to 11,929 | B.6.6, one run |
| R2 | The exclusion search skips razoring and reverse futility and reads the table-refined estimate; the donor runs both and reads the static eval | unrecorded departure | §5.2: 0.41 to 0.47% of exclusion searches; 0.04% of reductions | B.6.6 row, no run |
| R3 | The tie rule's store and correction: no move and an upper bound where the donor stores the tied move (exact at PV nodes) | unrecorded departure | §6: 0.04 to 0.09% of interior nodes | B.6.6 row, no run |
| R4 | The table: replacement ages an entry 2 a search (donor 64); a declined store drops its move; no PV bonus; no secondary aging; nothing stored at a node without moves | unrecorded departures | §6: immaterial at Hash 64 (0.02%), 1.9% of evictions at Hash 16 in `bench 17` | D.2 |
| R5 | F5: under-promotions in the capture stage | recorded departure, not a defect | §5.1: 0.04 to 0.19% of nodes in quiescence | B.6.6 (its row), H.2 |
| H1 | The frame a re-entry rewrites (donor-conform) | interaction | §5.3: 1.3 to 3.1% of reduction decisions | H.2 |
| H2 | IIR at revisited fail-low nodes (donor-conform); the import's "no path here" is refuted | interaction | §5.4: 13 to 18% of IIR's reductions | H.2 |
| H3 | A null verification that is a quiescence search (donor-conform) | interaction | §5.5: 19 to 38% of verifications | H.2 |
| S1 | `CoreCorrUpdateScale`'s rail (30) is below the donor's value in this engine's units (42.6, 63.8) | surface input | §6 | B.6.7 |
| N | Stale comments, dead fields and arms, the idiom list | neutral | §8, §9 | B.8.2, B.8.4 |

No finding moves the tree when repaired as specified. **Decision:** B.6.5
is confirmed, narrowed and reclassified (§12).

## 3. Defects

### 3.1 D1 (F4) — the multi-cut correction bonus

`kernel:1689-1691`:

```cpp
const int bonus = std::clamp(
    (s_val - static_eval) * 1000 / 282 * s_depth * kp.b3_sing_mc_corr / 1024,
    -256, 256);
```

- **Cause.** The donor's line (`:1283-1285`) is `int(value -
  ss->staticEval) * singularDepth * 177 / 1024`, in its own units, where
  the product cannot pass 1.38e9. Cluster 2 converts the gap to the
  donor's units first (`* 1000 / 282`), which widens every later product
  3.55 times.
- **Bound.** `s_val` is not decisive and the corrected eval is clamped
  inside the band, so the gap is at most 63,484; in the donor's units
  225,120. The product passes 2³¹ from `s_depth` 54 at the default 177 and
  from 24 at `B3SingMcCorr`'s rail of 400. A gap of 34,600 overflows at
  `s_depth` 99.
- **Effect.** `tools/results/b64-audit-20261112/f4_demo.cpp` runs the
  expression as written beside the same operations in `int64_t`:

  | `s_val` | static eval | `s_depth` | mult | as written | in 64 bits |
  |---|---|---|---|---|---|
  | 31,742 | −31,742 | 53 | 177 | 256 | 256 |
  | 31,742 | −31,742 | 54 | 177 | **−256** | 256 |
  | 31,742 | −31,742 | 24 | 400 | **−256** | 256 |
  | 17,300 | −17,300 | 99 | 177 | **−256** | 256 |

  The wrapped product trains all five correction tables by the full
  clamp in the wrong direction: Rarog's D.5.23 exactly. UBSan on the same
  file: `signed integer overflow: 12156480 * 177 cannot be represented in
  type 'int'`.
- **Reach.** Not reached in the three reads: the largest gap at a
  multi-cut was 19,569 and the deepest `s_depth` 19, which together make
  10.9% of `INT_MAX`. It needs a won-band static eval of the wrong sign at
  a node whose depth is 109 or more; the search does reach root depth 100
  in simple endings (the self-play read did, inside 250,000 nodes).
- **Repair.** The same operations in the same order in 64 bits, as a named
  function. `f4_equiv.cpp` checked 8,085,737 points (gap −63,484 to 63,484
  in steps of 7, `s_depth` 0 to 99, five multipliers) where the 32-bit
  product is defined: 0 differ. The tree cannot move.

### 3.2 D2 — the clock's game ply at `ponderhit`

`search_worker.cpp:209-219`. `check_stop()` runs at a node inside the
tree, and on `ponderhit` it recomputes the time limits with

```cpp
const int game_ply = 2 * (state_.board->fullmove() - 1) + (state_.board->turn() == BLACK ? 1 : 0);
```

`state_.board` is the board the search makes and unmakes moves on, so the
ply is the root's plus the depth of whichever node saw the flag. The
budget's `ply` term then grows with it. `ponderhit_read.py` (a middlegame
FEN at move 30, `wtime 60000 btime 60000 winc 600 binc 600`, TM_Debug):

| | soft limit, ms |
|---|---|
| plain `go`, twice | 3,302, 3,302 |
| `go ponder`, `ponderhit` after 0.4 s, eight times | 3,465, 3,527, 3,675, 3,527, 3,465, 3,512, 3,617, 3,573 |

Between +4.9% and +11.3%, different every time. The side is right
(`state_.root_side`); only the ply is wrong. The repair keeps the root's
game ply from the search's start. No gate plays with pondering on, so no
measurement is affected. A protocol and clock defect: B.9.1's list.

## 4. Widths (TR-5): every history and correction update at its extreme

Bounds are at the limits of the tables (main 7,183, capture 10,692,
continuation 30,000, pawn 8,192, correction 1,024), a gap of 63,742 (a
mate score against a static eval at the band's other end), a depth of 255
and each coordinate at its rail. A node's depth has no clamp (the donor
clamps to `MAX_PLY − 1` at entry, `:734`); the largest the audit can
construct is about 200, and the reduction table already reads
`min(depth, 255)`.

| Site | Product | Largest value | Share of `INT_MAX` |
|---|---|---|---|
| Multi-cut correction (`kernel:1690`) | gap·1000/282 · `s_depth` · mult | overflows (§3.1) | over, from `s_depth` 54 |
| Fail-low parent → continuation (`search_history_core.cpp:158-161`, `:38`) | 1,337 · 1,490 · 263 / 16,384 = 31,978, then · 520 · 126 | 2,095,198,560 | **97.6%** |
| TT cutoff → continuation (`kernel:967`, `search_history_core.cpp:56`, `:38`) | slope · depth · 750/1,024 · 520 · 126 | at slope 151: 1.85e9 at depth 255; at the rail 300: over from depth 150 | 86% at the default |
| Node-exit correction (`kernel:2123`) | gap · depth · scale·3/2 | 63,742 · 255 · 45 = 7.3e8 | 34% |
| Fail-low parent, its own products (`:158-165`) | 1,992,130 · 324 | 6.5e8 | 30% |
| Best move's bonus (`search_history_core.cpp:102-110`) | at most 6,857 into the quiet histories, 5,022 of it into the continuations | 3.3e8 | 15% |
| History gravity (`history_core.h:49`) | entry · \|bonus\|, both clamped | 9e8 | 42% |
| Correction value (`search_history_core.cpp:211`) | weights · 1,024 | 8.6e7 | 4% |
| LMR's history term (`kernel:1776`) | 80,806 · 1,200 | 9.7e7 | 5% |

- The continuation update is safe exactly while its incoming bonus is at
  most 32,776 (520 · 126 = 65,520). The fail-low parent's 31,978 clears
  that by 2.4%, on literals alone; the donor's own comment at that line
  (`:1590`) says its form overflows past a multiplier of 900. The reads
  saw 28,522 (87.0%).
- The TT-cutoff bonus is the one update that grows with depth without a
  cap (R1). At the default it stays inside for any reachable depth; at the
  slope's rail it would not from depth 150.
- **Decision:** the multi-cut bonus is repaired; the continuation update
  gets a guard that fails a build or a sanitizer run, not a silent wrap
  (§12).

## 5. The inputs PLAN asked to confirm or refute

Counters are given as `bench 13` / `bench 17` / self-play.

| Denominator | `bench 13` | `bench 17` | self-play |
|---|---|---|---|
| nodes | 3,783,135 | 18,816,178 | 610 searches of 250,000 |
| interior nodes | 2,222,503 | 11,246,746 | 99,797,905 |
| quiescence entries | 1,612,275 | 7,835,563 | 50,962,409 |
| reduction decisions (the cutoff count read) | 2,735,639 | 13,725,021 | 115,313,247 |
| exclusion searches | 39,620 | 168,223 | 2,299,351 |

### 5.1 F5 — quiet under-promotions as captures: a departure, not a defect

The generator emits Q, R, B, N in that order (`board.cpp:1204-1207` and
its five sister sites), and `capture_order_score` gives all four the same
score (one capture-history cell, no victim).
- The main picker's sort is stable, so there the queen's push comes first
  unless the table move is an under-promotion or the queen's push is the
  excluded move.
- Quiescence's selection (`pick_next`, `move_picker_core.h:175-187`) is
  not stable: it swaps the chosen move with the first slot, so a
  higher-scored capture generated after the promotions moves the queen's
  push behind its under-promotions. That, with the table move, is the
  "before the queen's push" row below: 2 to 4% of the under-promotions
  quiescence searches. A fail high there stores the under-promotion as
  the table move, which is where the main search's table-move row comes
  from.

| | `bench 13` | `bench 17` | self-play |
|---|---|---|---|
| Quiescence: under-promotions searched | 5,234 | 28,406 | 50,417 |
| … nodes below them | 6,997 (0.18%) | 36,208 (0.19%) | 57,431 (about 0.04%) |
| … that failed high | 15 | 135 | 316 |
| … searched before the queen's push | 112 | 1,233 | 1,321 |
| Main search: under-promotions searched | 8,458 | 55,109 | 154,573 |
| … pruned in the move loop | 6,664 | 41,778 | 24,487 |
| … nodes below them | 26,196 (0.69%) | 294,043 (1.56%) | 468,686 (about 0.31%) |
| … that failed high | 120 | 379 | 871 |
| … searched before the queen's push | 346 | 1,660 | 6,585 |
| … of those, the table move | 26 | 97 | 127 |

- The donor searches under-promotions in the main search too, as late
  quiets, so the main-search rows are a difference of order and
  reduction, not of presence. In quiescence the donor never searches one
  outside check (`qsearch`, `if (!capture) continue`), so those rows are
  Basilisk's addition: at most 0.19% of nodes, for 15 to 316 fail highs.
- "Searched before the queen's push" in the main search is almost all the
  exclusion search of a queen promotion, where the rook's promotion is the
  next best move by construction; it is what keeps a promotion from being
  singular, in the donor too.
- **Decision:** `b43`'s row stands as a departure. Nothing is repaired:
  the order inside quiescence costs nodes, never a bound, and the root
  has its own rule for the move played. A re-read (the donor's class,
  which also ends the ordering question) is worth a few Elo at most and
  waits in H.2.

### 5.2 NODE-4 — what the exclusion search reads

The import's claim was that IIR, ProbCut and LMR's eval term read the
table-refined estimate inside an exclusion search. Read against the code:

- IIR reads no estimate (`b3::iir_applies`). ProbCut reads the static eval
  for its SEE threshold and the table's score for its gate, as the donor
  does. Only LMR's alpha term reads the refined estimate
  (`kernel:1317`, `:1779`).
- The refinement itself is the departure: `kernel:910-923` runs in an
  exclusion search, where the donor sets `eval = ss->staticEval` and
  refines nothing (`:831-832`). The entry it reads is the excluded move's
  own lower bound, which says nothing about the other moves.
- The larger departure runs the other way. `kernel:1065` guards razoring,
  reverse futility and the null move by `ss->excluded == MOVE_NONE`. The
  donor guards only the null move: its Step 8 and Step 9 (`:991`, `:996`)
  run in exclusion searches. Neither departure is in a departures table.

| | `bench 13` | `bench 17` | self-play |
|---|---|---|---|
| Exclusion nodes | 39,609 | 168,181 | 2,286,867 |
| … whose estimate is refined | 14,119 (35.6%) | 60,525 (36.0%) | 531,857 (23.3%) |
| … where reverse futility's own rule would cut | 180 (0.45%) | 687 (0.41%) | 10,664 (0.47%) |
| … of those: now singular / multi-cut / neither | 2 / 129 / 49 | 19 / 444 / 224 | 148 / 7,503 / 3,055 |
| … where razoring would cut | 0 | 0 | 0 |
| Reductions inside them whose alpha term differs | 21,983 | 112,783 | 1,013,929 |
| … whose reduced depth differs | 1,033 | 5,635 | 52,664 |

- Reverse futility would end fewer than one exclusion search in 200: it
  needs a capture as the table move, a node off the PV line and a static
  eval above the singular bound. Razoring cannot fire (its cap is depth 1
  and an exclusion search is at least 2 deep).
- The refined estimate changes a reduced depth in 0.04% of all reduction
  decisions.
- Of the 148 exclusion searches reverse futility would cut that now read
  singular, 144 take a double or triple extension: the static eval is
  high, yet every other move falls far below the bound. Those are real
  only-moves (a forced recapture), which the donor's form would not
  extend.
- **Decision:** two rows for B.6.6, no run. The donor's form would take
  the extension from 0.006% of singular tries (148 of 2.3M) and could
  reclassify at most another 0.13% between multi-cut and neither.
  **Inference:** where it differs most, on the forced recaptures,
  Basilisk's is the defensible form.

### 5.3 The frame a re-entry rewrites

PLAN's text says the exclusion search and the null verification rewrite
`move_count`, `stat_score` and the grandchildren's cutoff count "as the
donor's does". Read:

- **The null verification restores** `move_count` and `stat_score`
  (`kernel:1180-1195`); the donor leaves the verification's values for
  ProbCut's children. Cluster 2 added the restore; it is a departure
  toward consistency and unrecorded. Neither engine restores the
  grandchildren's count.
- **The exclusion search restores nothing**, as the donor. `stat_score` is
  rewritten before the move is made (`kernel:1739`), so only `move_count`
  and the two cutoff counts leak.
- Three children's reads see the leaked `move_count`: the TT cutoff's
  malus (`kernel:971`, `< 5`), the best move's malus
  (`search_history_core.cpp:121`, `== 1 + tt_hit`) and the fail-low
  bonus's late-move term (`:154`, `> 9`).

| | `bench 13` | `bench 17` | self-play |
|---|---|---|---|
| TT-cutoff malus: reads / rewritten / decision changed | 78,602 / 10 / 4 | 413,156 / 157 / 110 | 10,757,977 / 4,127 / 2,006 |
| Best move's malus: reads / rewritten / changed | 551,473 / 1,934 / 282 | 2,820,263 / 9,482 / 1,395 | 23,737,568 / 96,791 / 21,010 |
| Fail-low bonus: reads / rewritten / changed | 225,871 / 5,370 / 2,391 (1.06%) | 1,178,416 / 25,953 / 11,134 (0.94%) | 11,864,905 / 298,731 / 99,523 (0.84%) |
| Exclusion searches that leave the grandchildren's count above 0 / 1 / 2 | 15,147 / 9,615 / 5,614 | 65,354 / 41,496 / 24,802 | 877,765 / 503,356 / 272,218 |
| Verifications that leave it above 0 / 1 | 11 / 7 | 1,297 / 735 | 143,055 / 71,012 |
| Reduction decisions whose cutoff count holds an exclusion search's own fail high | 167,432 (6.1%) | 705,521 (5.1%) | 9,162,638 (7.9%) |
| … that cross a threshold because of it | 39,672 (**1.45%**) | 182,744 (**1.33%**) | 3,551,630 (**3.08%**) |

- The move count is the smallest leak. Triumviratus read restoring it at
  about ±0.5; here it misdates the table move as a late move in about 1%
  of fail-low bonuses and does little else.
- The cutoff counts are the larger one, and no engine has read them. An
  exclusion search that fails high counts in `ss->cutoff_cnt`
  (`kernel:2006`) as though a child of the parent had failed high, and
  the parent then adds 241, 996 and 910 thousandths of a ply to later
  moves' reductions on that count (`kernel:1771-1773`): 1.3 to 3.1% of
  all reduction decisions. And 22 to 25% of exclusion searches leave the
  grandchildren's count above 1 for the child searched next, the table
  move's, before it has searched a move.
- **Inference:** all of it is the donor's behaviour and its constants
  were fitted around it, there and here; restoring the frame is a
  mechanism of unknown sign. **Decision:** H.2, one switch that saves and
  restores the three fields around both re-entries, each arm refitted
  (`CoreLmrCutoffCnt`, `…Cnt2`, `…CntAll`).

### 5.4 The fail-low store beside IIR

The import said Rarog's failure "has no path here", because IIR fires at
PV and cut nodes only. **Refuted:** the path exists and is not small.

| IIR's reductions, by what the table held | `bench 13` | `bench 17` | self-play |
|---|---|---|---|
| no entry | 9,410 | 63,271 | 488,509 |
| an evaluation-only entry | 5,031 | 28,675 | 222,645 |
| a lower bound without a move (a stand-pat store) | 2,017 | 13,265 | 127,389 |
| an upper bound from quiescence, no move | 128 | 1,118 | 17,584 |
| **a searched fail low, no move** | **2,487 (13.0%)** | **17,752 (14.3%)** | **184,018 (17.7%)** |
| all | 19,073 | 124,081 | 1,040,145 |
| Not reduced: a fail-low entry that kept an older move | 3,493 | 19,189 | 278,372 |

- One IIR reduction in six to eight is at a node the search already
  searched and failed low, revisited as a PV or cut node, whose entry has
  no move only because `B3FailLowStore` 1 stores none. Of the fail-low
  entries that reach IIR's gate, 42 to 50% are reduced.
- That is the donor's pair, and B.4's fit and gate ran with it. Rarog's
  −14.7 ± 13.8 is a prior that the pair costs something where IIR is
  wider than the donor's; here it is the donor's width.
- **Decision:** nothing changes before the fit. H.2 gets a switch that
  exempts a searched fail-low entry from IIR, read with both arms
  refitted.

### 5.5 Null move, beside the inputs

`R = 4 + depth/3 + max((static eval − beta)/80, 0)` is the donor's form
(`:1017`, with its 7 and 256). Its eval term has no cap, so:

| | `bench 13` | `bench 17` | self-play |
|---|---|---|---|
| Null searches | 163,305 | 907,362 | 9,275,225 |
| … that are a quiescence search (`depth − R ≤ 0`) | 129,850 (79.5%) | 704,577 (77.7%) | 6,139,241 (66.2%) |
| Verifications (depth ≥ 16) | 177 | 7,246 | 1,059,109 |
| … that are a quiescence search | 68 (38.4%) | 1,377 (19.0%) | 272,145 (25.7%) |

A verification exists to catch zugzwang, and a fifth to a third of them
verify with the stand-pat that already cleared beta. It is the donor's
behaviour (its base of 7 makes it more frequent there). H.2: a floor on
the verification's depth.

## 6. Unrecorded departures from the donor

Each is Basilisk's form where the donor's differs and no departures table
has it. Rows for B.6.6; the last column is this audit's reading of what
the row deserves.

| Place | Basilisk's form | The donor's form | Counter | Reading |
|---|---|---|---|---|
| **R1** TT-cutoff quiet bonus (`kernel:967`) | `CoreTtCutoffBonusSlope · depth`, 151 a ply, no cap | `min(112·depth, 695)` (`:884`) | over 695 in 15.3% / 17.7% / 31.0% of 48,550 / 247,695 / 7,116,740 bonuses; the total is 1.44 / 1.49 / 1.83 times the donor's; largest 2,567 / 3,473 / 11,929 | **run it**: below |
| **R2a** Node pruning in an exclusion search (`kernel:1065`) | none | razoring and reverse futility run (`:991`, `:996`) | §5.2: 0.41 to 0.47% | no run; stays |
| **R2b** The estimate in an exclusion search (`kernel:910`) | refined by the table's bound | the static eval (`:832`) | §5.2: 0.04% of reductions | no run; the donor's is the sounder form, a one-line change for B.8.2's list only if R2a is ever adopted |
| **R3** The tie rule's store (`kernel:1964`, `:2134-2143`) | a tie at alpha stores an upper bound and no move, and trains the correction as a fail low | stores the tied move, exact at a PV node (`:1623-1626`), and trains as a raise | 1,955 / 7,945 / 40,769 stores (0.09% / 0.07% / 0.04% of interior nodes), 206 / 852 / 4,603 of them at PV nodes | no run; a score equal to alpha is a bound, and Basilisk stores it as one |
| **R4a** Replacement (`tt.h:384-390`) | depth − 2 a search + 2 for an exact bound | depth − 64 a search (`tt.cpp`, `probe`: `depth8 − 8·relative_age`) | a current entry evicted beside a stale one: 0.08% / 1.91% / 0.02% of live evictions | D.2 |
| **R4b** A declined same-key store (`tt.h:228-230`) | drops the new move with the rest | writes the move first (`tt.cpp`, `save`) | 0.28% / 0.36% / 0.28% of same-key stores | D.2 |
| **R4c** The overwrite test and aging | no PV bonus; no secondary aging | `+2·pv`; a declined store ages a decisive inexact entry by a ply | not counted | D.2 |
| **R4d** A node without moves (`kernel:2077`) | returns, stores nothing | stores at `depth + 6` (`:1626`) | 5,734 / 30,855 / 182,499 nodes, nearly all table misses | D.2 |
| **R6** Depth at node entry | unbounded | `min(depth, MAX_PLY − 1)` (`:734`) | largest seen below the root: `s_depth` 19 | nothing |
| **R7** The verification's frame (`kernel:1180-1195`) | `move_count` and `stat_score` restored | left as the verification wrote them | §5.3 | with H1 |

**R1, the one to run.** The slope is an SPSA coordinate (131 at B.2's
seed, 151 after its fit), so B.6.7 will fit it again under whichever
form the head has, at `3+0.03` depths. Without the cap the bonus is
unbounded in depth: at depth 48 one cutoff sets the main-history entry
to its limit in a single update (7,183), and the self-play read reached
11,929. A coordinate fitted at short depths on a form that scales with
depth is the shape that reads well at the gate and badly at long
controls. **Inference:** worth a few Elo either way at `3+0.03`, more at
long controls, sign unknown. **Decision:** a B.6.6 row with its paired
run before the fit: a categorical on a tune build (cap at 695 in history
units, off by default so the fingerprint holds), with a `10+0.1` leg
beside `3+0.03` because the form scales with depth. Whichever form wins
is the one B.6.7 fits.

**S1, for B.6.7's rails.** The donor's node-exit bonus is `gap · depth ·
12/128` (18 without a best move, `:1635`) on a gap in its own units. On
this engine's gap, 0.282 of that, the same learning rate is 42.6 (and
63.8). `CoreCorrUpdateScale` is 18 on a rail of 4 to 30, and every fit
has moved it up (16, 17, 18). B-103's widening should reach 64. The
correction's structural weights (15,341, 10,569, 12,906) and update
shares (150, 186, 130, 70 over 128) are the donor's literals applied in
this engine's units, where the same entry corrects 3.5 times as much
(the comment at `kernel:1762-1766` knows it for LMR's cap); they belong
on the literal screen's list.

## 7. The interaction map across clusters

C1 is the selectivity core (B.2), C2 the proof searches and extensions
(B.3), C3 quiescence (B.4), C4 the root and its window (B.5). A signal
that two clusters both write, or that one writes and another reads, is
in bold.

| Signal (stored state) | Produced by | Consumed by | Note |
|---|---|---|---|
| Corrected static eval (`ss->eval`) | node entry: raw eval + corrections (C1); quiescence does not write the record | improving and worsening → RFP, LMP (C1), the null margin and ProbCut's beta and depth (C2); hindsight, eval-difference training, both futilities (C1) | An interior node's ancestors are interior, so no stale quiescence record is read |
| **The refined estimate (`eval`)** | node entry from any table bound, among them **C3's depth-0 stores** (blended stand-pat and fail-high values) | razoring, RFP, LMR's alpha term (C1) | C3's blend weights move the estimate C1's margins were fitted on; the exclusion search reads it too (R2b) |
| **Correction histories** | node exit (C1's admission; C2's decisive and exclusion switches), the multi-cut (C2, D1) | the static eval everywhere; RFP's margin term and LMR (C1); the singular bars (C2); the stand-pat (C3) | Two clusters write, three read |
| **Table entry** | node exit and the evaluation-only store (C1); ProbCut's store and the penalty (C2); quiescence's three stores (C3); never a later MultiPV line (C4) | the cutoff and its node-typed rule (C1); the refined estimate; the singular gate and ProbCut's gate (C2); IIR's "has a move" (C2); quiescence's cutoff and stand-pat (C3) | The fail-low store (C2's switch, C3's choice) feeds IIR: §5.4. The penalty lowers the depth the singular gate reads |
| **`cutoff_cnt`** | every fail high at a ply, **the exclusion search's own included (C2)**; reset by the node two plies up, and again by each re-entry | LMR's three terms (C1) | §5.3: 1.3 to 3.1% of decisions |
| **`move_count`** | the move loop (C1); rewritten by the exclusion search, restored by the verification (C2) | the child's two maluses and its fail-low bonus (C1) | §5.3 |
| `stat_score` | LMR (C1) | the child's bonus and fail-low scale (C1) | rewritten before use after a re-entry |
| **`reduction`** | LMR, **the root's included (C4)**, and negative reductions under C4's cap | the child's hindsight (C1) | Since C4, ply-1 nodes take hindsight |
| `tt_pv` | the node type, the table's bit, inheritance on a fail low, **an exclusion search that fails low (C2)** | RFP's veto and LMR (C1); the singular depth and bars (C2); every store | donor-conform |
| **Previous PV (`follow_pv`)** | the root record (C4) | IIR and PV quiet pruning (C2) | |
| **`root_delta_`** | the window's width (C4) | LMR's window term at every node (C1) | The window's three coordinates move every reduction in the tree |
| **`root_depth_`**, `root_score_` | the iteration (C4) | the extension budget, the "deep" bars and `seekMate` (C2); the tie rule (C1) | The iteration's depth, not a fail high's reduced one |
| `ext_plies`, `negext_path` | the singular extension (C2); the negative-reduction cap (C4) | the budget (C2); the ceiling (C4) | node checks hold both |
| **Fail-high values** | quiescence's blend (C3), then the parent's depth blend (C1) | every cutoff above | Two blends stack on one return path |
| Histories | node exit, the TT cutoff (R1), eval difference, post-LMR (C1); the TT-move history also at the multi-cut (C2) | the picker in both searches; pruning and LMR (C1); the double bar (C2) | Quiescence orders evasions on them and never trains them |
| **Node count** | every node, re-entries included (the donor counts in `do_move`, so its re-entries are not nodes) | the tie rule's one-in-eight (C1); the root's effort and the clock (C4) | |

## 8. Read and found sound

- Mate scores: distance pruning, ply adjustment on store and probe, the
  rule-50 downgrade, `no_move_score` under exclusion, the mate at a
  quiescence node in check only after every evasion was searched.
- The null move: child and verification typing, the region, the pawn-only
  guard, no null after a null.
- Bounds: the stack (142 records; ply 127 writes `ss + 2` at index 136),
  the PV table at ply 127, the move buffers (the exclusion search reuses
  the node's buffer only because the table move is delivered before
  anything is generated), the searched lists, the root's promotion
  arrays.
- The table: an empty slot against a zero key, the depth offset (ProbCut
  may store depth −1 with a bound; the bound tells it from an
  evaluation-only entry), the evaluation stored undamped, the store's
  evaluation at an exclusion node.
- The picker against the donor's: quiet scoring term for term, the
  cumulative threat maps, the good-capture threshold in this engine's
  units, the two quiet stages.
- The root: the window's arithmetic keeps alpha below beta and its width
  grows to the open window, so the loop ends; a stopped search stores and
  trains nothing; the record's sort is stable.
- The pool: wake by epoch, results written under the lock while the main
  thread waits, node counts published in batches with the limit tested
  every node.
- Shared with the donor and left alone: `tt_score > alpha` true on a
  missing score in LMR's PV term; hindsight in check; eval-difference
  training repeated by a re-entry; the capture SEE margin without a floor;
  the under-promotion's capture-history cell.

## 9. C++: the lens and the idiom list

### 9.1 Findings

- **Undefined behaviour:** D1 only. The reads ran the UBSan-relevant
  arithmetic to 87% of the limit at one site (W1).
- **Concurrency:** nothing new. `penalize` reads and writes in two steps
  (BAS-C05's class, accepted). The static `reservable_clusters` is
  written only by tests.
- **Lifetime:** nothing. The search's board is a parameter of `search()`
  and the pointer is cleared before it returns.
- **Dead at the default arm**, for B.8's deletion of the old arms:
  `allow_null` (read only by `B3NmpVerify` 2), `best_scored` in both
  quiescence loops (`B3FailLowStore` 0), `singular_quiet_lmr`,
  `did_check_ext`, the older aspiration loop (`B5Window` 0), the delta
  rule, the quiet checks, the small ProbCut.
- **Dead outright:** `SearchStack::moved_piece` is written by `do_move`
  and `do_null_move` and read by nothing.
- **Misleading:** `kernel:907-909` says reverse futility "returns `eval`
  directly", which it has not since the blend; `kernel:24` heads the
  child log with "LMR table"; `search_worker.h:86-88` says "on either
  kernel"; `constants.h:41` says the thread cap is defined in
  `search.cpp`; `search_types.h:37-39` says a record without a move
  "reads zeros", true only while `CoreHistoryFills` is 0. `docs/DESIGN.md`
  named `src/search.cpp`, which no longer exists, twice in its table
  section; corrected in this leaf's commit, as that file's own rule asks.

### 9.2 The idiom list

Each item with what it buys and what it risks in the pooled NPS read. All
are behaviour-neutral and go to B.8.2 unless a leaf is named; none is
urgent.

| # | Construct today | Idiom | Buys | NPS risk |
|---|---|---|---|---|
| 1 | The unit conversion `* 1000 / 282` written at three sites (`kernel:1060`, `:1690`, `:1779`) | one named `constexpr` function | one place to widen; F4 came from this conversion | none |
| 2 | Widths argued in comments | `static_assert` over named `constexpr` bounds at the three widest products | a tune's rail or a literal's change fails the build, not a game | none |
| 3 | `DIAG_COUNT`, `TRACE_DECISION`, `PRICE` preprocessor arms and 18 `[[maybe_unused]]` in the kernel and the driver | `if constexpr` on flavour constants | every flavour's code type-checks in every build; CI builds one arm | none if folded; prove with the exact fingerprint and one pooled read |
| 4 | `TRACE_DECISION` with fourteen positional arguments | an aggregate with designated initialisers | a swapped argument cannot compile | none (already B.8.2's, B.2.0's F20) |
| 5 | `(Move[32], int)` pairs passed to the update | one fixed-capacity list type | two parameters fewer, one bounds rule | low; read |
| 6 | The 620-line `search_one` lambda capturing by reference | named helpers | reviewable units | real: inlining and layout; only under the exact fingerprint and the NPS floor (already B.8.2's) |
| 7 | Unscoped `enum TTFlag` | scoped, with `std::to_underlying` | no implicit integer compare | none |
| 8 | `TTCluster()` with six sequentially consistent stores | C++20 value-initialised atomics, then `memset` or first-touch per thread | F3's slow fill | none in search; B.8.4 |
| 9 | Hand-rolled shutdown flag, epoch and join | `std::jthread` and `std::stop_token` | a shorter lifecycle | none; only after B.9.1's interleaving tests exist |
| 10 | Sanitizer cells build with libc++'s default hardening | `_LIBCPP_HARDENING_MODE_DEBUG` in the sanitizer and debug cells | library bounds checks for free; makes items 5 and 11 pay | none in release |
| 11 | Nested C arrays in the history tables | `std::mdspan` or nested `std::array` | checked indexing under item 10 | low; UBSan's `bounds` already covers fixed C arrays, so low value |

Not recommended: `[[assume]]` hints in the kernel (a wrong one is
undefined behaviour for a speed nobody has measured); modules (PGO and
three toolchains).

### 9.3 G.2's C++26 decision, read again

The Windows compiler is pinned at LLVM 23 (`.github/windows-clang-version`)
and is Clang 23.1.5 with libc++ here. Probed with `-std=c++26`
(`tools/results/b64-audit-20261112/probes/`):

- absent: reflection (no `<meta>`, `-freflection` changes nothing),
  contracts (`pre` does not parse, no `-fcontracts`),
  `std::inplace_vector`, `std::mul_sat`, `constexpr std::log`,
  `std::start_lifetime_as_array` (a C++23 library item);
- present: pack indexing, `= delete("reason")`, `#embed`,
  `[[indeterminate]]`, and in C++23 mode `std::mdspan`, deducing `this`,
  `[[assume]]`, `std::flat_map`, `std::print`, `std::expected`, static
  `operator()`, `std::jthread`.

**Decision:** G.2's stands, stay on C++23. The features that would remove
code (reflection for the option, parameter and counter tables) are still
missing on the compiler measurements use. B.8.4 cannot use
`start_lifetime_as_array` for mapped pages on this library.

## 10. Routing

| To | Items |
|---|---|
| **B.6.5** | D1 with its test; W1's guard. Nothing moves the tree, so the leaf closes on its tests (§12) |
| **B.6.6** | rows R1 (with its paired run), R2a, R2b, R3, R6, R7 (no run each); R5's row stands from `b43` |
| **B.6.7** | S1: `CoreCorrUpdateScale`'s rail to 64; the correction's structural weights and update shares on the literal screen's list; R1's form decided before the surface is registered |
| **B.8.2** | §9.1's dead and misleading items; idiom items 1 to 7, 10, 11 |
| **B.8.4** | idiom item 8; `start_lifetime_as_array` is not available |
| **B.9.1** | D2 with a test; TR-5's arithmetic half is answered by §4 (its stack-size half stays); idiom item 9 after its tests |
| **D.2** | R4a to R4d, read at the hash and control D.2 measures at |
| **H.2** | H1 (the frame, one switch, the cutoff-count coordinates refitted); H2 (IIR exempting a searched fail-low entry); H3 (a floor on the verification's depth); R5's re-read |

## 11. Instruments that could measure what this packet infers

| Inference | Instrument | Verdict |
|---|---|---|
| R1's sign and size | B.6.6's paired run on a tune build | **worth it**, and already the row's method; a `10+0.1` leg is the read that matters for a depth-scaling form |
| H1's sign | B.16.2's pricing has no cell for a frame field; a paired run after a refit is the only read | wait in H.2 |
| H2's cost | B.16.2's pricing, with a cell for "IIR at a searched fail-low entry" judged by the unreduced search | **worth it when H.2 opens**: minutes, and it says whether the item deserves a game |
| R4a at the gate's conditions | this audit's counters on `replay_games.py`'s searches | lower value: 0.02% at Hash 64 |
| D2 | TM_Debug's `soft_ms`, as §3.2 | the test B.9.1 writes |
| The counters themselves | `audit_counters.patch` (§13) | kept as a patch, not merged: 89 counters for one read; a row that B.6.6 or H.2 takes up re-reads its own |

## 12. Decision and handoff for B.6.5

**Decision.** `READY_FOR_IMPLEMENTATION`. B.6.5 is **confirmed and
narrowed**: no tree-changing repair exists, so its cost screen, its
floors and its repair SPRT do not run, and by its own last clause it
closes on its tests. It is a well-specified change of about thirty lines
and its class becomes `I1` (it was `I2` against the chance of a repair
that moved the tree).

### Implementation handoff

- **Goal and exact semantics.**
  1. *The multi-cut correction bonus.* A function in `search_b3.h`,
     `multicut_correction_bonus(value, static_eval, s_depth, mult)`,
     returning `clamp(int64(value − static_eval) · 1000 / 282 · s_depth ·
     mult / 1024, −256, 256)` with the multiplications and divisions in
     exactly that order, each truncating toward zero, in 64 bits. The
     kernel's multi-cut site (`kernel:1688-1693`) calls it with `s_val`,
     `static_eval`, `s_depth` and `kp.b3_sing_mc_corr` under its present
     condition (`!in_check && s_val > static_eval`).
  2. *The continuation update's width.* `update_continuation_histories`
     keeps its arithmetic. Its incoming bonus is bounded by 32,776 in
     magnitude (`INT_MAX / (520 · 126)`), and the bound is enforced two
     ways: a compile-time proof that the fail-low parent's bonus cannot
     exceed it at the history limits (the chain of §4: at most 31,978),
     and a node check (the kernel's `CORE_NODE_CHECK` class, live in the
     sanitizer and Diag builds, absent from release) on every call. How
     the constants are named is the implementer's.
- **Why it should work here.** §3.1: the wrapped product is reproduced
  and the 64-bit form equals the 32-bit one wherever that is defined. §4:
  the bound holds today on literals.
- **Producer → state → consumer.** The multi-cut → the five correction
  tables → every static eval (§7). The fail-low parent, the best move's
  bonus, the TT cutoff and the post-LMR bonus → the continuation rows →
  the picker, pruning and LMR.
- **Files.** `src/search_b3.h`, `src/search_kernel_core.cpp` (the one
  call), `src/search_history_core.cpp` (the guard),
  `tests/test_b3_proof.cpp` or `tests/test_history_core.cpp`.
- **Invariants.** `bench 13` 3,783,135 exactly, at both commits. No
  release-build instruction is added on the history path. The correction
  bonus stays within ±256.
- **Tests, each shown to fail on a planted fault that is then reverted.**
  - `multicut_correction_bonus(31742, −31742, 54, 177) == 256`; the same
    at `(…, 24, 400)` and `(17300, −17300, 99, 177)`; `(−31742, 31742,
    54, 177) == −256`. Planted fault: the arithmetic in `int`, which
    returns −256 for the first three (and trips UBSan).
  - Agreement with the 32-bit expression on a grid where that is defined
    (the audit's `f4_equiv.cpp`: gaps in steps of 7, `s_depth` 0 to 99,
    multipliers 0, 1, 100, 177, 400). Planted fault: a changed order of
    operations.
  - The width proof. Planted fault: the weight 520 raised to 540, which
    must fail the build.
- **Cheap qualification.** Release and sanitizer CTest; the exact
  fingerprint; nothing else. The multi-cut runs 9,790 times in `bench
  13`'s 3.78M nodes, so no NPS read is owed.
- **Expensive gate.** None.
- **Acceptance.** The tests pass, the planted faults fail, the
  fingerprint is exact.
- **Non-goals.** No cap on the TT-cutoff bonus (R1 is B.6.6's). No frame
  restore, no pruning in the exclusion search, no change to the table, no
  `ponderhit` repair (B.9.1's), no 64-bit arithmetic on the continuation
  update, no clamp on depth at node entry.

## 13. Artifacts

`tools/results/b64-audit-20261112/` (ignored):

| File | SHA-256 |
|---|---|
| `audit_counters.patch` (576 lines, on `662e5ad`; adds `src/audit.h` and 89 counters, printed to stderr at exit) | `5FD584D96B1267014D50367F8CD7255B60965202089974FA4D45EE63AD7BA373` |
| `bench13.audit` | `C0887D5C31E139E3EC2790C8376D0FED2F188E30AE277BA0D0A99F3B78E77269` |
| `bench17.audit` | `4BDB01662C78BB7865793C0B8A375AB1D79DF567FDC5AF7CAEAA4DD2D1A43921` |
| `selfplay_h64.audit` | `A368D9C6FC66B5A2142875E2C59D58F0A3516355BAEBDFEF50E77BEF37383F08` |
| `selfplay_read.py` | `50CCB873C856973E0D5A9B314DE6EF822AD52E55AD226F2B6C9E8CE90A7D76AB` |
| `ponderhit_read.py`, `ponderhit.out` | `C62D1460CD66C39D100CC49092CDD4503F6DEB4CA989990A245A84DE4B3C5EB7` (the script) |
| `f4_demo.cpp` | `4706DF744BF0A46F96A63CF3C73699C30294CBDDD0D463459A833DA8E830EB8A` |
| `f4_equiv.cpp` | `772310FB57B182FE36525E9B5C8597AE87602A4BD98BC8AA5B81ABB7A95CC979` |
| `probes/` | 21 one-file feature probes and their logs |

Recipe for the reads:

```bash
git worktree add --detach <dir> 662e5ad
git -C <dir> apply tools/results/b64-audit-20261112/audit_counters.patch
cmake -S <dir> --preset release
cmake --build <dir>/build/release --target nopgo
<dir>/build/release/basilisk bench 13 2> bench13.audit      # 3,783,135 or the read is void
<dir>/build/release/basilisk bench 17 2> bench17.audit
python tools/results/b64-audit-20261112/selfplay_read.py <dir>/build/release/basilisk 64 250000 160 selfplay_h64.audit
```

The build was Clang 23.1.5, the `release` preset's pext tier, no PGO
(kept beside the reads as `basilisk-audit.exe`, SHA-256
`7215F0EC19134BE8D8081CCE7B9E8CF7232E2BF7F643A7FD60D29D9FEE467356`).
The validity rule: the patched build reads `bench 13` 3,783,135, which it
did; the counters are one thread's and the self-play read is one process
through four games with `ucinewgame` between them.
