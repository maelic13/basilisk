# BAS-S53

**BAS-S53 — B.5.3: Cluster 4 diagnostics, M1 and the unfitted paired run**

- **Registration.** Created 2026-11-09 at B.5.3's opening, before any game;
  maintainer-run games and NPS read, agent-run zero-game screens; search cluster,
  Elo layer plus zero-game screens. Assembled from §11's BAS-S53 text
  (`analysis/b5_cluster4_research_2026-11-08.md`) and every clause that replaces
  it: research amendment 1 §A1.5, research amendment 2 §A2.5, and the review's
  corrections. The frozen predictions and rules are copied; where a clause
  replaced §11's words, the new words are given and the source named.
- **Baseline / candidate.** One revision, the accepted implementation head (engine
  source `c6f2440`); the OFF arm against the ON arm (`-DB5_ROOT=ON`), §7's seeds as
  §A1.5 replaces the width's row, §9's defaults; `bench 13` OFF **4,144,790**, ON
  **3,920,652**. Final-PGO pairs per arm; the width run and the categoricals on one
  tune build of the arm, the alternative as arm A by UCI option. Binary identities
  and hashes are recorded here at build time.
- **Research question.** Does the coherent Cluster 4 arm, the donor's root-move
  record, aspiration loop and root reduction, land without an Elo defect before
  fitting; at what width are its categoricals read; and what does it do to the
  clock in positions it already has in hand?
- **Hypothesis / mechanism.** Packet §12 question 1.
- **Interactions.** Packet §5 with its numbers; the time manager's inputs (§A2.4 e);
  the surface rule 5.8, as §A2.5 amends it, read at the width the width run leaves
  and carried to BAS-S54.
- **Competing explanations.** A defect or a wrong seed (P1 below −20, the width run
  read first, then ablated by switch); the seed's width costing more nodes than the
  root's reduction gives back (A2, A5); narrow windows failing more than they save
  (P3); the root's re-searches eating the saving (P4).

#### ORDER (§A2.5)

1. **The tools and this entry.**
2. **The node-budget screens**, both arms, recorded before the first game:
   - the depth-14 census with ticket 0's counters (`run_suite.py`, `suite_v2`, Hash
     64, stride 1);
   - `Pricing 4` at depth 16 on `suite_v2`, with the `LmrFailLow` cell at the root
     depth;
   - the cost screen in B.3.0's research amendment 7 §R's form (§A1.5);
   - WAC at 100,000 nodes;
   - the floors grid (`floors_grid.py`);
   - the blindness read: the final positions of the repetition draws of the gate
     (a) PGN judged by Stockfish 19 at fixed nodes with `loss_by_cell.py`'s judge,
     the share where the repeating side stood ≥ 150 cp ahead by the judge, recorded;
     above 5% the draw value becomes an H.2 card with that figure.
3. **The battery screen**, on a quiet host, before M1 (below).
4. **M1**, BAS-R10, maintainer-run.
5. **The pooled NPS read** (P10), on an idle host the maintainer gives.
6. **The whole-arm paired run** on the defaults. It does not wait on M1.
7. **The width run** (§A1.5, agreed by the maintainer 2026-11-09), after the
   whole-arm run and before §9's items.
8. **§9's items in order**, at the width the width run leaves.

P1 to P7 and P10 to P12 are scored here. A moved default rebuilds and re-pins the
arm before the fit.

#### THE BATTERY SCREEN (§A2.5; replaces §11's "the battery at `60000+600` with the stop off")

Zero-game, clocked, a quiet host. Instrument `tools/diag/won_battery.py`.

- **Positions.** From `tools/results/b47-direction-ltc/games.pgn` (SHA-256
  `6931997c…`), games in file order, one position a game and stratum:
  - **E, decided endings:** the first position whose mover reports a score of 1,500
    to 29,999 with at most eight men. The first 60 such positions are the review's
    and are left out; the next 120 are E.
  - **W, better and not decided:** the first position whose mover reports 300 to
    1,499 on this move and on its previous one, any material. The first 60 are W.
  - the six of §2.2.
- **Clocks.** `60000+600` and `10000+100`, a fresh process a search, Threads 1, Hash
  64; the six also at `180000+2000`, recorded.
- **Columns.** On the Tune builds BAS-S53's paired runs play: the OFF arm at the
  default; the arm at the default twice; the arm at `B5FailHighStop` 200 and 0; the
  arm at `B5NegExtMax` 1; the arm at `B5AspBase` 19000.
- **Per column and stratum:** searches over 0.5× the maximum and at it (0.95× or
  more), spend, searches past the optimum, root best-move changes a completed
  iteration, and the holes by where they began.
- **What it decides:** M1's gate; item 1's zero-game read; the trigger of BAS-S55
  (e). Nothing else. It is a screen.

#### THE GAMES CENSUS (§A2.5)

`tools/diag/pgn_time_census.py` on the PGN of every paired run here, both sides:
the mean time a move and the share of moves at half the clock left or more, by
score band; games decided at 300, 800 and 1,500 and those not won.

#### CONDITIONS

- **Every run:** 2,000 games at `3+0.03`, 1T, Hash 64, paired reversed UHO openings,
  no adjudication, tablebases off, Colosseum `match-fixed`, the recount from the
  PGN.
- **Categorical rule** (packet §9; PROCESS *Cluster delivery shape*): adopted at +5
  Elo or more, rejected at −5 or less, each only when the 95% interval also excludes
  0; left at the default otherwise; never an SPSA coordinate. Arm A is the
  alternative, arm B the default.

| # | Option | Values | Default | What decides | Bracket |
|---|---|---|---|---|---|
| 1 | `B5NegExtMax` | 0 unbounded (today's and the donor's); 1 or 2 extensions from a negative reduction per line | 0 | first the zero-game reads at 1: nodes at depth 14, the cost screen at depths 16 to 19 (P5) and its battery column (§A2.5); then a paired run, 1 against 0. The verdict is the paired run's. 2 is read only if 1 is adopted | ±5 |
| 2 | `B5FailHighReduce` | 1 the re-search after a fail high runs `max(1, depth − failed_high_cnt)` plies; 0 full depth | 0 | the gate first: on the arm at depth 14, nodes in fail-high re-searches above 20% of windowed nodes. Met on every tree read (29 to 53%; §A1.5), so a paired run, 1 against 0, with the WAC floor as veto, at the width the width run leaves. If B.5.4 moves `B5AspBase` to more than twice or under half that value, BAS-S55 (c) reads item 2 again | ±5 |

- **Attribution switches.** Their defaults are the arm's form; a paired run never
  moves them. The ablation order on a failed P1 is the width run first, then
  `B5Window`, `B5RootLmr`, `B5LmrWindowUnits`, at the width that run leaves
  (§A1.5).

| Option | Values | Default |
|---|---|---|
| `B5Window` | 1 the donor's loop on the record; 0 today's loop on the record | 1 |
| `B5RootLmr` | 1 root moves after the first reduced at depth ≥ 2; 0 never | 1 |
| `B5LmrWindowUnits` | 1 the null window's unit corrected (163); 0 today's 577 at every node | 1 |

- **The width run** (§A1.5). 2,000 games at `3+0.03` on the arm's tune build,
  `B5AspBase` 19000 (arm A) against 1410 (arm B), every other option at its default.
  Read by §9's rule: at +5 Elo or more for 19000 with the 95% interval excluding 0,
  §9's items are read with both arms at 19000 and BAS-S54's SPSA starts `B5AspBase`
  there; otherwise they are read at the seed and SPSA starts at the seed. It moves
  no default in the source: the base is a fitted coordinate, and the fit sets it. At
  −5 or less with the interval excluding 0, fixed-depth nodes mislead on this
  coordinate.
- **Runs and seeds**, registered before any game; numbered as BAS-S50's.

| Run | Arm A | Arm B | Control | Seed |
|---|---|---|---|---|
| the whole-arm run | ON release, the defaults | OFF release | `3+0.03` | 53000 |
| the width run | `B5AspBase=19000` | defaults | `3+0.03` | 53001 |
| item 1 | `B5NegExtMax=1` | defaults | `3+0.03` | 53002 |
| item 2 | `B5FailHighReduce=1` | defaults | `3+0.03` | 53003 |

#### PRE-REGISTERED PREDICTION — freeze before exposure

Packet §11's table, frozen 2026-11-08, before any ON-arm build; A1 to A5 frozen
2026-11-09 (§A1.6); A9 to A13 frozen 2026-11-09 on positions and games not yet seen
(§A2.6). P8, P9, P11 and A6 to A8 are scored in BAS-R10.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, ON against OFF at one revision, §7's seeds, §9's defaults) reads **+4 Elo**, 80% interval [−12, +20]; probability positive 60% | low-moderate | below −20 is a defect or a wrong seed: the width run is read first, then ablate by switch in the order `B5Window`, `B5RootLmr`, `B5LmrWindowUnits`, at the width that run leaves, then `B5AspBase` 19000 with `B5AspMsqDiv` at its top (§A1.5) |
| P2 | After the fit and the gate, B.5 reads **+8 Elo** at STC, 90% interval [−3, +20]; H1 at `[0,3]` probability 55% | low-moderate | H0 with P3 to P7 held says the donor's root cluster carries little on this HCE; the arm stays off and B.6 proceeds |
| P3 | Windows at depth 14 on `suite_v2`: re-searches per window rise from 1.50 to **2.0 to 3.5**; nodes in failed windows fall from 53.7% to **≤ 48%** of windowed nodes; give-up nodes 0 | low | a failed-node share above 58% says the narrow windows cost more than they save: `B5AspBase`'s sweep is read before the whole-arm run is blamed |
| P4 | Root reduction: nodes to depth 14, ON against OFF, geometric mean **0.80 to 0.97**; 5 to 15% of reduced root searches are searched again | moderate | a mean above 1.00 says the root's re-searches eat the saving: `B5RootLmr` 0 is the first ablation |
| P5 | `B5NegExtMax` 1 at fixed depth: nodes at depth 14 **−4 to −8%**, mean selective depth −0.3 to −0.8 plies; the cost screen ≤ 3; the paired run within ±6 (60%) | moderate | a node change inside ±2% says the chains are cheap here, and item 1 closes at 0 without a game |
| P6 | Tactics: the arm's WAC at 100,000 nodes in **[246, 262]** | moderate | under 246 is a floor failure and returns the leaf; `B5RootLmr` 0 is read first (BAS-D17) |
| P7 | Floors: the grid passes as registered on the arm | moderate | a miss attributes by switch, zero-game: `B5RootLmr`, `B5Window`, `B5NegExtMax`, `B5LmrWindowUnits` |
| P10 | Speed: pooled NPS **0.99 to 1.01×** the OFF arm (per-iteration bookkeeping; two stack stores per node) | moderate-high | below 0.98 returns the record's bookkeeping for its cost before the gate |
| P12 | Categoricals: item 1 within ±6 (60%); item 2's gate not met (70%), and within ±5 if met | low-moderate | item 1 at +5 or more with its interval excluding 0 adopts the cap and re-reads the floors |
| A1 | Ticket 3 as amended reproduces the probe: `bench 13` 4,022,487, WAC 249, 2.93 re-searches per window | high | a difference is an arithmetic difference between the probe and the implementation, to be found before ticket 4 |
| A2 | What research now expects of the unfitted paired run, beside P1 and not instead of it: **−8 Elo**, 80% interval [−22, +6], probability positive 20%. The seed's width costs a fifth more nodes in undecided positions; root reduction gives some back | low-moderate | below −20 is P1's stop; above +6 says fixed-depth nodes overstate the width's cost |
| A3 | B.5.4's fit moves `B5AspBase` up: at or above 2820 with 80%, at or above 4000 with 65% | low-moderate | a fitted base at or under the seed says the node read misleads on this coordinate |
| A4 | §9 item 2's paired run at the seed reads the reduction within ±6 (55%); positive with 55% | low | at +5 or more with its interval excluding 0 it is adopted, and re-read after the fit under §A1.5's clause |
| A5 | The width run, 19000 against 1410: **+10 Elo**, 80% interval [−2, +22]; at +5 or more with its interval excluding 0 with 55% | low-moderate | at 0 or below, the node read does not carry to games here |
| A9 | Battery, stratum E at `60000+600`: the arm spends **1.15 to 1.50** of the OFF arm; 5 to 16 of 120 over half the maximum against 0 to 5; best-move changes 1.4 to 2.0 times the OFF arm's | moderate | a ratio under 1.10 says the review's 60 were unlucky |
| A10 | Stratum W: the arm spends **0.95 to 1.25** of the OFF arm; at most 4 of 60 over half the maximum on either arm; stop 0 costs the arm 5 to 20% more spend than stop 100 | low | the arm over half the maximum on 8 or more says the tail reaches undecided positions, and (e) is triggered |
| A11 | `B5NegExtMax` 1, stratum E: **0.65 to 0.95** of the OFF arm's spend, at most 3 over half the maximum | moderate | over 1.10 says the review's 60 flattered it |
| A12 | The whole-arm run's census at `3+0.03`: the arm's won-band time a move within **0.95 to 1.12** of the OFF arm's; unconverted at 300 within 15 games of the OFF arm's | moderate | time above 1.20 at the gates' clock says the stop no longer carries the arm there |
| A13 | L1 holds on (d) (85%); (e) is triggered (35%); if played, no unconverted game at 1,500 on either side (85%) | low-moderate | L1 failing is the cost this amendment says is not there. Scored in BAS-S55 |

- **Registered gate / stop rule.** Stop rules: P1 below −20 (then the ablation order
  above); a floor failure at any ticket returns the leaf; PLAN rule 6 for the
  programme.
- **Calibration owed (§A1.6, §A2.6).** P3 missed on the failed share (70% against at
  most 48%) and held on re-searches. P4's share missed: 0.05% against 5 to 15%, since
  a reduced root move beats alpha 0.17% of the time (the review's §6), and P4's 5 to
  15% was an interior node's rate. P12's "gate not met (70%)" is contradicted. P8,
  P11 and the effort mechanism are in BAS-R10.

#### RECORDS CARRIED INTO THIS ENTRY (from the review of B.5.2)

- Reduced root moves are seldom searched again because they seldom fail high, not
  because a negative reduction leaves `d ≥ new_depth`.
- `cont_hist_pruned` halves because the rule fires inside the OFF arm's fail-high
  cascades on a few decided positions. §5.8's joiner rule is read by the median
  position, at the width the width run leaves.
- The long searches in won endings are systematic, not scatter, and no switch owns
  them. `blowup.py` cannot attribute them.
- CI has not run. The arm's cell prints `bench 13` and asserts only that a count
  exists; its 3,920,652 is read from the cell's output at the pull request.

#### RESULT — append after exposure

- **Binaries** (2026-11-09, `d07380d`, engine source that of `c6f2440`; every
  manifest clean, clang 23.1.5, PEXT final PGO;
  `tools/results/b5-20261109/b53/build_all.ps1`). Each reads its arm's `bench 13`,
  read directly on the staged copy: ON 3,920,652, OFF 4,144,790.

  | Build | ON | OFF |
  |---|---|---|
  | Release, pool build 1 (the whole-arm run) | `A1D8CE88…` | `64E4ECC7…` |
  | Tune | `D8D9BC67…` | `6DD0B156…` |
  | Diag | `DD0258A8…` | `63C9C26F…` |

  Pools `b53-on` and `b53-off`, four independent builds each, for the NPS read.
- **Node-budget screens** (2026-11-09, `tools/results/b5-20261109/b53/screens/`, read
  by `analyze.py` into `analyze.txt`). No clock in any read but the cost screen's
  limit, which ran alone.

  | Read, `suite_v2` depth 14, Diag | OFF | ON |
  |---|---:|---:|
  | Interior / quiescence nodes | 7,116,466 / 4,893,846 | 6,952,216 / 4,737,291 |
  | Re-searches a window | 1.50 | 2.77 |
  | Failed nodes of windowed nodes | 53.7% | 69.6% |
  | Fail-high re-search nodes of windowed nodes | 38.3% | 45.9% |
  | Give-up nodes | 696,441 | 0 |
  | Reduced root searches; searched again | 0 | 20,151; 10 (0.05%) |
  | Ply-cap returns | 0 | 0 |
  | Windows at or over `TmEffortHi`; at or under `TmEffortLo` | 11.7%; 41.1% | 11.8%; 54.3% |

  - **`Pricing 4`, depth 16** (Diag). `lmr_fail_low`: ON 1.94% [1.92, 1.96] of
    1,580,224 probes, OFF 2.10% [2.08, 2.13] of 1,254,842. **The cell at the root
    depth** (PV, remaining depth 15 and over): ON 3 beats of 3,563 probes (0.08%), OFF
    1 of 1,170 (0.09%). A root reduction that fails low hides very little. The
    instrument's top depth bucket also holds PV nodes just below the root, so the cell
    bounds the root and does not isolate it; the OFF arm never reduces at the root, and
    its rate is the same. All PV cells: ON 40 of 27,503, OFF 31 of 12,768.
  - **WAC at 100,000 nodes** (`fixed_budget_probe.py`, the release pair): ON **263**,
    OFF **254**. Mean depth 24.79 and 22.33; mean selective depth 19.49 and 19.50. The
    OFF arm's 254 is the packet's; B.5.2's record read 253 through the engine's own
    `wac nodes` command, so that difference is the instrument's and not the tree's.
  - **The floors grid** (`floors_grid.py`, the clean tree of `c6f2440`): the arm PASS,
    600 / 598 / 600 / 597, five budgets 118. The OFF arm PASS, 600 / 598 / 600 / 596,
    five budgets 119. KR-K sits on its floor on both.
  - **The cost screen** (§R's form, the release pair): the scorer's PASS on all five, no
    stop, no depth missing; steps 20 to 22 geometric means 1.102, 0.882, 0.793, 1.046,
    0.894; absolute at 22, recorded and not judged, 2.340, 0.473, 0.248, 0.225, 2.737.
    These are B.5.2's figures to the third place.
  - **The blindness read** (`b53/blindness.py`, `blindness.json`; Stockfish 19 at
    150,000 nodes, one thread, Hash 16, the final position's FEN alone): of the 885
    repetition draws of the gate (a) PGN (the harness's Threefold; game 1423 is also a
    threefold on the board and the harness ended it as FiftyMove), the repeating side
    stood at or above +150 cp on **0**, 0.00%. It stood at or below −150 on 2. The
    highest is +140; 670 are within 5 cp. At or under 5%: **no H.2 card**. The judge's
    scores are not all zero (342 are not), so the read could have fired.
  - **§9 item 1's reads at `B5NegExtMax` 1** (Tune, `b53/seldepth.py`, 105 positions; the
    106th has no search): nodes −6.06%; mean selective depth 19.51 to 18.59, −0.92 plies.
    The cost screen at 1 is ticket 6's, on the same source, and passes. Its battery
    column is owed.
  - **§9 item 2's gate:** 45.9% against 20%, met. Its paired run is owed at the width the
    width run leaves.

- **Predictions read at the screens, each scored as frozen:**

  | # | Read | Verdict |
  |---|---|---|
  | P3 | 2.77 re-searches; failed share 69.6%; give-up nodes 0 | **miss** on the failed share, above the falsifier's 58%: `B5AspBase`'s sweep is read before the whole-arm run is blamed. Re-searches and give-up nodes hit |
  | P4 | geometric mean 0.924 over 105 positions (median 1.142, total 0.973); 0.05% searched again | the mean **hit**, though the median says the typical position costs more; the share **miss** (calibration above) |
  | P5 | nodes −6.06%; selective depth −0.92 against −0.3 to −0.8; ticket 6's screen passes | **hit** on nodes and the screen, **miss** by 0.12 plies on selective depth; the paired run waits |
  | P6 | WAC 263 against [246, 262] | **miss** by one, high; the floor holds |
  | P7 | the grid passes | **hit** |
  | P12 | item 2's gate is met | that clause **missed**, as §A1.6 says |

- *The battery screen, M1, the NPS read and the games have not run.*

#### PREDICTION CALIBRATION — preserve the original prediction above

- *Owed at the leaf's close.*
