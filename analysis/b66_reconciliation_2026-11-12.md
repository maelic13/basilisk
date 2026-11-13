# B.6.6 — Donor-form reconciliation

- State / class: `GAME_GATE` (runs prepared, maintainer-run); `R2`
- Owner / date: agent; 2026-11-12
- Source revision: `5ced6f1` read (B.6.4's head), B.6.5 on top (`52d6e43`,
  `b92adcb`), the switches at `1b7256a`; `bench 13` **3,783,135** at every
  one. The donor is `sf_19` (`edb0d9db`), read from `D:/code/stockfish`
  by `git show sf_19:<path>`, never the working tree (which is later).
- Decision taken: the maintainer chose all four running rows (2026-11-12)
  and runs them; the commands are §7's.

## 1. Decision

Every departures row is sorted (§3). **Four run**, each a 2,000-game paired
run of the donor's form against Basilisk's on one tune build, behind the
conversion and WAC vetoes (BAS-S59):

1. the TT-cutoff quiet bonus capped at the donor's 695, at `3+0.03` and at
   `10+0.1`;
2. razoring in the donor's form (any depth, a margin quadratic in depth, no
   guards);
3. the rule-50 bucket in the table key (`adjust_key50`), which also gives
   quiescence's cutoff the donor's rule-50 protection (F11);
4. the kings in the non-pawn correction keys.

Every other row stays Basilisk's, with its owner named: the fair re-read in
H.2, a later phase, B.6.7's surface, or nothing because the forms agree or
the difference cannot matter. One row is already closed: the fail-low store
is the donor's form since B.4.

## 2. The rule

PLAN B.6.6, as frozen:
- a row whose prerequisite has landed and been accepted gets one 2,000-game
  paired run at `3+0.03`, the donor's form against Basilisk's, after the
  conversion and WAC floors as vetoes;
- the donor's form is adopted at ≥ +5 (PROCESS *Categorical switches*: only
  when the 95% interval also excludes 0);
- a row whose prerequisite was rejected or never built stays and says so;
- a row under +5 is not cleared by the run, which is tilted toward the form
  the arm was fitted with (maintainer 2026-11-04): it waits in H.2;
- the adoptions land together behind one switch and pass SPRT `[0,3]`
  against the head before B.6.7.

The TT-cutoff row runs by PLAN's own clause (B.6.4's R1), with a `10+0.1`
leg. Small gains wait for Phase H (maintainer 2026-11-03) except where a
leaf must choose the form anyway: each running row here changes the tree
B.6.7 fits, so the form is chosen now or fitted by default.

## 3. Every row

Sources: B.3's table (`analysis/b30_cluster2_research_2026-10-30.md`,
*Departures table*), B.4's (`analysis/b43_cluster3_research_2026-11-06.md`
§14, and PLAN B.4.5's added capture-stage row), B.5's
(`analysis/b5_cluster4_research_2026-11-08.md` §14), B.2's collected from
`analysis/search_programme_2026-10-06.md` §3 and §26, PLAN's rows of
2026-11-05, and B.6.4's (`analysis/b64_engine_audit_2026-11-12.md` §6).

| # | Place | Source | Basilisk's form | The donor's | Prerequisite and its state | Disposition |
|---|---|---|---|---|---|---|
| 1 | Fail-low TT store (`B3FailLowStore`) | B.3 | — | no move after a fail low | B.4: settled | **closed**: the donor's form in both searches since B.4 (`BASILISK_B4_CORE_DEFAULTS`) |
| 2 | Fresh history tables (`CoreHistoryFills` 0) | B.3 | zero after a clear | the donor's fill values | none; B.3.2 −0.5 ± 9.3 unfitted | stays; H.2 |
| 3 | Small TT ProbCut (`B3ProbCutTt` 0) | B.3 | none | `beta + margin` from a deep lower bound | none; B.3.2 +0.9 ± 9.5 | stays; H.2 |
| 4 | Extension budget (`B3ExtBudget` 1) | B.3 | positive singular plies bounded by the root depth | none | none; kept as a guard | stays; H.2 |
| 5 | Singular margin (`B3SingMargin` 201) | B.3 | fitted | 17/63 per ply | the fit | B.6.7's surface (13 to 378) |
| 6 | `seekMate` depth (`B3SeekMateDepth` 12) | B.3 | 12 | 16 | the fit; an F line | B.6.7 (BAS-S43 (c)); no run |
| 7 | Stand-pat store | B.4 | a depth-0 lower bound | the evaluation alone, on a miss | none | stays; H.2 |
| 8 | Known-loss guard | B.4 | the HCE's known-win band | `!is_loss` | none | nothing; leaves with the HCE |
| 9 | Quiet table move outside check | B.4 | not delivered | delivered, counted, skipped | none | nothing (TS-14) |
| 10 | En passant's gain | B.4 | a pawn | 0 | none | nothing; exact |
| 11 | Stalemate test | B.4 | legal generation | a pawn-push pre-filter first | none | nothing; exact |
| 12 | Evasion order | B.4 | the picker's stages | captures by victim, then quiets | B.4.5's P4 | stays; H.2 (B.4.5) |
| 13 | SEE vector (`B4SeeValues` 0) | B.4 | 100/300/300/500/900 | one vector | BAS-S50 item 4: −0.5 ± 9.2, not adopted | stays; H.2 |
| 14 | Reverse futility's cap (`CoreRfpDepthCap` 10) | B.4 | 10, fitted | 18 | the fit | B.6.7's surface (6 to 19) |
| 15 | The capture stage (F5) | B.4 | every promotion | a capture or a queen promotion | none; B.6.4 §5.1: ≤ 0.19% of nodes | stays; H.2 |
| 16 | RFP seek-mate cap (`B4RfpSeekMate` 0) | B.4 | off | on | the network | stays; H.2 on the network |
| 17 | Fail-high depth reduction (`B5FailHighReduce` 0) | B.5 | full depth | reduced | D.1 (`increaseDepth`) | stays; D.1 |
| 18 | Negative-reduction ceiling (`B5NegExtMax` 1) | B.5 | a count cap | unbounded at `sf_19` | the ply bound is post-19 | stays; Phase H |
| 19 | Optimism | B.5 | none | inside the NNUE blend | the network | F |
| 20 | Draw value | B.5 | 0 | `−1 + (nodes & 2)` | B.5.3's blindness read: 0 of 885 repetition draws with the repeater at +150 | stays; no H.2 card |
| 21 | Fail-high clock stop (`B5FailHighStop`) | B.5 | A.8.17's stop | none | M1, D.1 | stays; D.1 |
| 22 | Window at depth ≤ 3 and at mate | B.5 | open | around the average | none | nothing |
| 23 | Root TT move before an iteration completes | B.5 | the table's | the record's first | none | nothing; immaterial |
| 24 | An interrupted iteration | B.5 | discarded | a completed best kept | TR-3 | B.9.1 and H.2 |
| 25 | Best-move changes and effort | B.5 | per iteration | inside it, over threads | D.1 | D.1 |
| 26 | `MAX_PLY` 128 | B.5 | 128 | 246 | the bands, the depth byte | C, D.2 |
| 27 | LMR window term (`B5LmrWindowUnits` 1) | B.5 | 163 | 577 in its units, 163 in ours | none | nothing; a unit conversion |
| 28 | Per-thread window offset | B.5 | `(thread % 8) · 282/1000` | `thread % 8` | D.2 | D.2 |
| 29 | The stopped MultiPV line | B.5 | keeps its depth | inexact-score rules | D-73, TR-3 | B.9.1 |
| 30 | **Razoring** | **B.2** | depth ≤ 1, `alpha − 261·d`, not with a quiet table move or a lower bound (the oracle's form with Reckless's guards) | non-PV, any depth, `alpha − 482·d²` (`:991`), no guards | **B.4's quiescence**, into which razoring returns: landed and accepted (BAS-S52) | **runs** (§4.2) |
| 31 | B.2's other categoricals | B.2 | `CoreTtCutoffNodeTyped` 1, `CorePawnHistBucketsLog2` 13, `CoreCorrAdmission` 1, `CoreLmrNegativeAllowed` 1, `CoreLmpExemptChecks` 0 | the same | — | none is a departure |
| 32 | **The rule-50 key** (with F11) | PLAN 2026-11-05 | the raw key; quiescence's cutoff unguarded | `adjust_key50` (`position.h:322`) | none: Rarog parked it while games were adjudicated, and no gate adjudicates now | **runs** (§4.3) |
| 33 | **Kings in the non-pawn key** | PLAN 2026-11-05 | kings left out (`board.cpp`) | included (`position.cpp:509`) | none | **runs** (§4.4) |
| 34 | **The TT-cutoff bonus's cap** (R1) | B.6.4 | `151·d`, no cap | `min(112·d, 695)` (`:884`) | none; PLAN says it runs | **runs** (§4.1) |
| 35 | Exclusion search pruning and estimate (R2a, R2b) | B.6.4 | none; the refined estimate | razoring and RFP; the static eval | — | no run (0.41 to 0.47%; 0.04%) |
| 36 | The tie rule's store (R3) | B.6.4 | an upper bound, no move | the tied move, exact at PV | — | no run (0.04 to 0.09% of nodes) |
| 37 | The verification's frame (R7) | B.6.4 | restores `move_count`, `stat_score` | does not | — | H.2's frame item |
| 38 | Depth at node entry (R6) | B.6.4 | unbounded | `MAX_PLY − 1` | — | nothing |
| 39 | The table's four (R4a–R4d) | B.6.4 | — | — | — | D.2 |

Triumviratus's TS-16 and TS-17 apply only past `sf_19`; nothing here is.

## 4. The four rows that run

Each answers `agents/research.md`'s four questions.

### 4.1 The TT-cutoff bonus's cap

1. **Mechanism.** A cutoff from the table trains the quiet histories by
   `151 · depth` with no bound: from depth 48 one cutoff sets a main-history
   entry to its limit, and self-play reached 11,929 (B.6.4 §6, R1). The cap
   keeps a deep cutoff from overwriting what many shallower updates
   learned. The slope is an SPSA coordinate fitted at `3+0.03` depths, the
   shape that reads well at the gate and badly at long controls.
2. **Interactions.** The histories feed the picker in both searches, LMR's
   history term and `stat_score` (and through it the fail-low parent's
   scale), continuation pruning, the double bar's TT-move history. The
   slope (`CoreTtCutoffBonusSlope`) is the one coordinate the cap reshapes;
   B.6.7 fits it under whichever form wins.
3. **Invariants.** The bonus stays within the history limits (gravity
   clamps it); the continuation update's width bound (B.6.5) only tightens.
4. **Falsifier.** Both legs at or below −5 with the interval excluding 0
   say the uncapped form's deep training is what the engine uses.

### 4.2 Razoring

1. **Mechanism.** A node whose estimate is far below alpha is left to
   quiescence. The donor razors at every depth with a margin quadratic in
   depth (in our units 136 at depth 1, 544 at 2, 1,224 at 3), so it razors
   more at depth 1 (136 against 261) and keeps a sparse deep reach. Basilisk
   kept the oracle's depth-1 form, seeded when quiescence was checkless; B.0
   measured that razoring into the checkless quiescence carried about half
   of razoring's tactical cost. B.4 rebuilt quiescence, so the reason the
   depth cap was seeded no longer holds. Razoring's decisions beat alpha
   13.42% of the time on the B.4 head, the highest share of any rule priced
   (B.4.3 read R2).
2. **Interactions.** Reverse futility and the null move cut the same
   nodes from above; their margins (`CoreRfpPerPly`, `B3NmpBase`,
   `B3NmpPerPly`) and `CoreRazorMargin` were all fitted with depth-1
   razoring, so the arm is tilted against the donor's form. The estimate it
   reads is refined by table bounds, among them quiescence's depth-0 stores
   (C3's blends). Its guards, which the donor's form drops, read the table
   move and bound.
3. **Invariants.** It returns a quiescence value at a non-PV node, so no
   mate is shortened; the mate band holds because `eval` is clamped.
4. **Falsifier.** A reading at or below −5 excluding 0 confirms the fitted
   depth-1 form; it does not clear the donor's (§2).

### 4.3 The rule-50 key

1. **Mechanism.** A bound stored at one fifty-move count is reused at
   another, where the draw horizon differs and the score may not hold. The
   donor keeps them apart by eight-ply buckets from a count of 14. The main
   search refuses a cutoff from 96; quiescence has no guard (F11), and the
   bucket gives it the donor's protection. Every gate now plays the long
   reversible endings to the end.
2. **Interactions.** Every table consumer: cutoffs, the refined estimate,
   the singular and ProbCut gates, IIR's "has a move", the evaluation-only
   store, the graph check one ply down, the ponder move and the prefetch.
   Repetition and cuckoo keep the raw key. Within a reversible stretch the
   table splits into buckets, which costs hits.
3. **Invariants.** Two positions differing only in the count share a key
   below 14 and within a bucket (`tests/test_tt.cpp`); the repetition key is
   raw, so draw detection is unchanged.
4. **Falsifier.** A reading at or below −5 excluding 0 says the lost
   sharing costs more than the stale bounds did.

### 4.4 Kings in the non-pawn correction key

1. **Mechanism.** The non-pawn correction is keyed by each side's
   non-pawn pieces. Without the king a bare king's key is 0, so every
   position where a side has a bare king trains one entry; that entry
   carried part of the KBP-K correction drift (B.3.0's packet §1.4). With
   the king, the entry also separates king placements in every position.
2. **Interactions.** The corrected eval everywhere: RFP's correction term,
   LMR's correction term, the singular bars, the stand-pat, every margin
   that reads the corrected eval. The correction weights (12,906 for the
   two non-pawn tables) were fitted on the kingless key.
3. **Invariants.** The correction stays inside the band (it is clamped);
   the key is the XOR of the king's piece key into the same incremental key,
   so it is the donor's content without a board change.
4. **Falsifier.** A reading at or below −5 excluding 0 says the kingless
   key's pooling is the better estimate on this evaluation.

## 5. The interaction map

| Row | Writes | Read by | Fitted coordinates it moves | How B.6.7 sees it |
|---|---|---|---|---|
| TT-cutoff cap | the quiet histories at a table cutoff | picker, LMR, pruning, `stat_score`, the double bar | `CoreTtCutoffBonusSlope` | the slope's response changes shape above depth 4.6 |
| Razoring | the node's return | the tree above; RFP and null move see fewer nodes | `CoreRazorMargin` (unread when on), RFP and null margins | a new F line, `CoreRazorQuadMargin`, joins the surface if adopted |
| Rule-50 key | every table entry past a count of 14 | every table consumer | none directly | a population shift in long endings |
| Kings | the non-pawn correction entries | the corrected eval | `CoreCorrUpdateScale`, the non-pawn weights | a different correction population |

The four are independent in code; razoring and the TT cap both act on the
nodes near the leaves, but no run combines them. If two adopt, the SPRT
reads them together, as PLAN requires.

## 6. Instruments that could measure what this packet infers

- **Razoring's priced share** (`pricing.py`, B.16.2): the 13.42% beat
  share is on the B.4 head. A read on the donor's form would say whether it
  prunes worse decisions; **lower value**, since the paired run decides and
  the read would not change the rule.
- **The TT-cutoff counters** (B.6.4's `audit_counters.patch`): bonus sizes
  above the cap by depth. **Not useful** now; they were read in B.6.4.
- **The conversion grid** reads the rule-50 key where it matters most (long
  reversible endings); it runs as a veto here (§7).
- **WAC at 100,000 nodes** reads razoring's tactical cost first; it is the
  razoring arm's likeliest veto (P8).

## 7. Preparation and the commands

- **Switches** (`1b7256a`): four S lines in `BASILISK_CORE_KERNEL_SWITCHES`
  and two F lines (`CoreTtCutoffBonusMax` 695, `CoreRazorQuadMargin` 136),
  every default Basilisk's form. Release `bench 13` 3,783,135, release
  CTest 30 of 30, `test_tt`'s rule-50 key section.
- **The binary**: `tools/build_test.ps1 -Suffix b66 -Flavor Tune`, SHA-256
  `9AE3C8C9…1F649`, `bench 13` 3,783,135, manifest clean.
- **Liveness**: BAS-S59's table; every switch moves the tree.
- **Vetoes**: `tools/results/b66-20261112/floors.py` builds the head and
  each arm (one default set to 1, the source restored byte-exact) and reads
  them; the result is below.
- **The runner**: `tools/run_b66_games.py`. Its veto guard was fed 17
  synthetic summaries (pass, each failure, the WAC boundary at exactly 5
  and 6 below, missing data, another arm's pass) and took the intended
  branch on every one (`tools/results/b66-20261112/veto_test.py`). A
  refusal exits 2 and plays nothing, so the next command in the chain
  runs.

### 7.1 The vetoes (read 2026-11-12, before any game)

`python tools/results/b66-20261112/floors.py`; release-preset PEXT trees
`build/b66-<tag>`, the source restored byte-exact after each build and
clean at the end; `floors/summary.json` (SHA-256 `0567F88F…EEB1CE`).

| Tree | `bench 13` | KQ-K | KR-K | KBB-K | KBN-K (five) | Grid | `test_endgames` | WAC d6 | WAC 100k | Veto |
|---|---:|---:|---:|---:|---:|---|---|---:|---:|---|
| head | 3,783,135 | 600 | 599 | 600 | 594 (119) | PASS | 191/191 | 189 | 259 | — |
| `ttcap` | 3,878,257 | 600 | 599 | 600 | 587 (118) | PASS | 191/191 | 190 | 268 | pass |
| `razor` | 3,422,264 | 600 | 600 | 600 | 595 (119) | PASS | 191/191 | 181 | 259 | pass |
| `key50` | 4,279,787 | 600 | 598 | 600 | 596 (120) | PASS | 191/191 | 189 | 257 | pass |
| `kings` | 3,019,185 | 600 | 600 | 600 | 597 (120) | PASS | 191/191 | 196 | 261 | pass |

Every arm plays. P8 hits. The rule-50 key moves `bench 13` too (+13.1%):
its positions reach a count of 14 inside a depth-13 search.

### 7.2 The dry runs

All five accepted on the final runner, the binary pinned, host idle
(`tools/results/colosseum_match_b66-<run>_20261112_2344*.manifest.txt`;
arm A's option resolved to 1 and arm B's absent in each dry-run JSON;
`10+0.1` on `match-fixed-ltc.toml`, the rest on `match-fixed.toml`;
concurrency 15). The working tree was dirty during them only by this
record, which the next commit holds.

### 7.3 The commands

From the repository root, one PowerShell line; each run refuses on its
own without stopping the chain, the `10+0.1` leg last since it is the
longest (about 30 minutes for each `3+0.03` run and 80 for `10+0.1`, from
B.4.5's and B.5.5's):

```
python tools/run_b66_games.py TtCap; python tools/run_b66_games.py Razor; python tools/run_b66_games.py Key50; python tools/run_b66_games.py Kings; python tools/run_b66_games.py TtCap10
```

Results land in `tools/results/b66-<run>/`. Then BAS-S59's decision rule
reads each, and any adoptions go behind one switch to the `[0,3]` SPRT.

## 8. The runs (maintainer-run, 2026-11-12 to 13)

BAS-S59 *RESULT* holds the records. All five ran 2,000 games clean, and
the PGN recount agrees.

| Row | Reading | Decision |
|---|---|---|
| TT-cutoff cap | +8.0 ± 9.1 at `3+0.03`; −0.5 ± 8.6 at `10+0.1` | Basilisk's form stays; H.2 |
| Razoring | **+10.6 ± 9.4** | the donor's form adopted |
| Rule-50 key | −4.5 ± 8.4 | Basilisk's form stays; H.2 |
| Kings in the non-pawn key | **+11.6 ± 9.3** | the donor's form adopted |

What follows from it:
- **The SPRT** (PLAN B.6.6): `CoreRazorDonor` 1 and `CoreCorrNonPawnKings`
  1 become the defaults together behind one umbrella switch, and the ON
  release build plays the OFF build of the same revision at `[0,3]`. Its
  cap comes from the drift model at the expected value. Registered and
  prepared before any game; the maintainer runs it.
- **B.6.7's surface, if the SPRT accepts.** `CoreRazorMargin` and
  `CoreRazorDepthCap` go dead under the donor's razoring and leave the
  surface. `CoreRazorQuadMargin` (136, an F line) becomes the margin the
  fit reads; whether it joins as an X line is B.6.7's curvature read. The
  non-pawn correction weights and `CoreCorrUpdateScale` were fitted on the
  kingless key and are refitted on the new one.
- **If the SPRT rejects**, both rows stay Basilisk's and wait in H.2 with
  their readings, and B.6.7 fits the current forms.

## 9. The gate, prepared (2026-11-13)

- **What BAS-S59 tested for the TT cap.** Arm A ran the donor's cap on
  the fitted slope, `min(151·d, 695)`, not the donor's whole bonus
  `min(112·d, 695)`: the slope is B.6.7's coordinate. The whole form is
  H.6's row, with both arms refitted.
- **The umbrella.** `B6_DONOR_FORMS` (`65448d0`, off) sets
  `CoreRazorDonor` and `CoreCorrNonPawnKings` to 1 through
  `BASILISK_B6_CORE_DEFAULTS`. OFF reproduces 3,783,135 and passes release
  CTest 30 of 30; ON reads 3,268,437.
- **ON-arm CTest: three tests bound to the OFF tree**, each re-based in
  its own commit with its measurement before a flip (BAS-S60's H1):
  - `test_b5_root`, *a proven mate stands*: the fixture needs a tree that
    proves mate in 8 at depth 15 and reads 2,607 at 16. OFF proves mate
    in 8 from depth 14. The pinned Tune build with razoring alone first
    proves mate in 9 at depth 17, with the kings key alone mate in 10 at
    depth 18, and ON proves mate in 9 at depth 16. The guard's own
    assertions pass; the search never reaches the state the fixture
    exercises.
  - The depth-6 WAC floor (186): ON reads 178 against OFF's 189. At
    100,000 nodes ON reads 256 against the head's 259; more pruning
    solves less at a fixed depth.
  - KQ-K's staleness guard: ON mates in 10.7 plies on average against
    OFF's 12.0, below the guard's ratio of its 16.3-ply ceiling. The
    ceiling is due to be tightened.
- **A test gap closed on the way** (`bd814cb`): the arm-default range check
  did not expand the kernel switches, so the umbrella's overrides could not
  match.
- **The mate delay, read in games.** On BAS-S59's PGNs
  (`pgn_time_census.py`; `tools/results/b66-20261112/census-*.json`),
  games decided at +300 for a side and not won: razoring 47 against 38
  (L1's margin is 18.5), the kings key 24 against 42; none at +800 on
  either donor arm. The delay costs no conversions there. BAS-S60's flip
  carries the same census on its own games.
- **The gate.** BAS-S60: SPRT `[0,3]`, cap 5,000 pairs, final-PGO arms ON
  `A2BF3537…` and OFF `07BB73F3…` from `65448d0`, seed 66010, dry run
  accepted (`tools/results/colosseum_sprt_b66-sprt_20261113_091911.*`).
  Command: `python tools/run_b66_sprt.py`.

## 10. The gate's result and the leaf's close (2026-11-13)

BAS-S60 reached its cap undecided: **+2.2 ± 4.2 Elo** over 10,000 games,
LLR +0.52, the recount agreeing. Neither donor form is adopted. With the
TT cap and the rule-50 key, all four rows that ran stay Basilisk's and
are H.6's, each with its readings. B.6.7 fits the head as it stands, and
the fingerprint stays 3,783,135. The single runs' +10.6 and +11.6 did not
survive together; one 2,000-game paired run past +5 is a weak basis for
adoption when several rows run on one rule, and the joint SPRT did its
job as the guard.
