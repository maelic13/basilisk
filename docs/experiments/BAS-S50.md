# BAS-S50

**BAS-S50 — B.4.5: Cluster 3 diagnostics, the unfitted paired run and the categoricals**

- **Registration.** Created 2026-11-06 at B.4.5's opening, before any game;
  maintainer-run games, agent-run zero-game screens; search cluster, Elo
  layer plus zero-game screens. Assembled, as packet §11 requires, from §11's
  BAS-S50 text and every clause that replaces it
  (`analysis/b43_cluster3_research_2026-11-06.md`: research amendment 1 §5,
  research amendment 2 §6 to §9, the review's §7 and §9). The frozen
  predictions and rules are copied; where a clause replaced §11's words, the
  new words are given and the source named.
- **Baseline / candidate.** One revision: the accepted B.4.4 head with the
  opening's two commits, source `33654f8`. The OFF arm against the ON arm
  (`-DB4_QSEARCH=ON`), §7's seeds as amendment 2 §9 replaces the cap's row,
  §9's defaults; `bench 13` OFF **6,090,091**, ON **4,003,560**. PGO pairs per
  arm; the categoricals on one tune build of the arm, the alternative as arm A
  by UCI option. Binary identities and hashes are recorded here at build time.
- **Research question.** Does the coherent Cluster 3 arm, a quiescence that
  spends its nodes on the captures that can change the result, land without
  an Elo defect before fitting, and which of its categorical forms does the
  engine want?
- **Hypothesis / mechanism.** Packet §12 question 1.
- **Interactions.** Packet §5 with its numbers; the surface rule 5.8, read
  here for BAS-S51.
- **Competing explanations.** A defect or a wrong seed (P1 below −30, ablated
  by switch in P1's order before a mechanism is blamed); the recapture
  exemption outweighing the count rule (P3); history order not rescuing the
  evasion rule (P4); the base of 86 costing more than the exemption saves (P5).

#### ORDER

1. **The zero-game screens**, all recorded before the first game (§11): §2's
   table on both arms at depth 14, with 5.8's candidates' shares and verdicts;
   `Pricing 4` at depth 16 on `suite_v2` on both arms (P4, P5), and with
   `B4QuietChecks` 6 for item 5's gate; WAC at 100,000 and 400,000 nodes; the
   floors at five budgets; the canary calibration of B.3.2, re-used if the OFF
   arm is unchanged; the deep-iteration cost screen if B.4.4 ran it on an
   earlier revision; pooled NPS on an idle host. Added, recorded and carrying
   no rule:
   - amendment 1 §5 clause 2: the pricing screen reads P5 and queen captures
     by class on both arms, the first reading of H.2's candidate;
   - amendment 2 §8 read 1: `qs_evasion_tt_move` on both settings of item 2,
     beside `tt_move_absent` and `tt_move_d0`;
   - amendment 2 §8 read 2: P4's `QsEvasion` share by node type as well as
     pooled.
2. **Items 1 and 2:** their binding reads, then 2,000 games each.
3. **The whole-arm paired run** on the defaults they leave.
4. **Items 3 to 5** in order, 2,000 games each; item 5 only past its gate.

`B4RfpSeekMate` has no paired run here (amendment 2 §8): the cap is off by the
maintainer's decision, and its re-read is H.2's, on the network.

#### CONDITIONS

- **Every run:** 2,000 games at `3+0.03`, 1T, Hash 64, paired reversed UHO
  openings, no adjudication, tablebases off, Colosseum `match-fixed`, the
  recount from the PGN.
- **Categorical rule** (packet §9; PROCESS *Cluster delivery shape*): adopted
  at +5 Elo or more, rejected at −5 or less, each only when the 95% interval
  also excludes 0; left at the default otherwise; never an SPSA coordinate.
  Arm A is the alternative, arm B the default.

| # | Option | Values | Default | What decides | Bracket |
|---|---|---|---|---|---|
| 1 | `B4EvasionPrune` | 1 once an evasion holds, quiets and SEE-losing captures are skipped; 0 every evasion is searched | 1 | first the floors at five budgets and the mate fixtures on both settings: if 1 fails a binding read that 0 passes, the default is 0 without games. Then a paired run, 0 against 1 | ±5 |
| 2 | `B3FailLowStore` on the arm | 1 no move after a fail low in either search; 0 the best-scored move in both | 1 (the arm's default; the OFF arm keeps 0) | first ticket 6's binding reads, by the same rule; `tt_move_absent` and `tt_move_d0` on both settings at depth 14; then a paired run, 0 against 1 | ±5 |
| 3 | `B4GainValues` | 0 the search's values; 1 the HCE's | 0 | a paired run, 1 against 0 | ±5 |
| 4 | `B4SeeValues` | 0 the search's values; 1 the HCE's, for every `see_ge` and `see` call | 0 | a paired run, 1 against 0. Tilted against 1 (§5.7): it can adopt, never clear | ±5 |
| 5 | `B4QuietChecks` | 0 none; 6 up to six quiet checks with SEE ≥ 0 at the first quiescence ply | 0 | first the gate: `Pricing 4` at depth 16 on `suite_v2` with 0 and with 6. A paired run, 6 against 0, is owed only if razoring's beat share at expected cut nodes (25.0% on the B.3 head) falls by a quarter of itself or more with 6. Otherwise the card closes at 0 with that reading | ±5 |

- **Attribution switches.** Their defaults are the donor's form; a paired run
  never moves them. They exist so that a negative whole-arm run is ablated by
  switch before any mechanism is blamed, and so the floors can read one rule
  alone.

| Option | Values | Default |
|---|---|---|
| `B4QsRecapture` | 1 a recapture on the previous move's square is exempt from the count rule, futility and the alpha-relative SEE; 0 not exempt | 1 |
| `B4Stalemate` | 1 the stalemate test; 0 none | 1 |
| `B4QsDelta` | 0 no delta rule; 1 the OFF arm's delta rule | 0 |
| `B4QsMoveCount` (F) | the count limit; 64 turns the rule off | 2 |

- **The departure switch** (amendment 2 §9, in place of §9's attribution row):

| Option | Values | Default | Read |
|---|---|---|---|
| `B4RfpSeekMate` | 0 no cap, Basilisk's form; 1 a reverse-futility cut is refused at a node deeper than `B4RfpSeekMateCap` when the iteration's root depth is at least `B4RfpSeekMateDepth` and the previous iteration's root score is at least `B3SeekMateScore` either way | **0** | no paired run; §14's row; H.2 on the network |

- **Runs and seeds**, registered before any game. The packet fixes the arms;
  the seeds follow BAS-S41's numbering.

| Run | Arm A | Arm B | Control | Seed |
|---|---|---|---|---|
| item 1 | `B4EvasionPrune=0` | defaults | `3+0.03` | 50001 |
| item 2 | `B3FailLowStore=0` | defaults | `3+0.03` | 50002 |
| the paired run | ON release (the defaults items 1 and 2 leave) | OFF release | `3+0.03` | 50000 |
| item 3 | `B4GainValues=1` | defaults | `3+0.03` | 50003 |
| item 4 | `B4SeeValues=1` | defaults | `3+0.03` | 50004 |
| item 5, past its gate | `B4QuietChecks=6` | defaults | `3+0.03` | 50005 |

- **Item 1's build offset is gated** (the review's §7 follow-up 2, the
  maintainer's decision §9 item 2). The evasion rule's accounting ran in tune
  builds with `B4EvasionPrune` 1 alone, at up to 1.86% of nodes. Since
  `33654f8` it runs only under Diag or Pricing, so neither arm of item 1 pays
  for it and no offset is carried beside the verdict.
- **The counter suite with quiet checks on** is readable since `cdee38f`:
  `run_suite.py`'s two bounds subtract `qs_quiet_checks`. Item 5's gate reads
  `pricing.py` either way.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Packet §11's table, frozen 2026-11-06, with amendment 1 §5's re-registered P5:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, ON against OFF at one revision, §7's seeds, the defaults items 1 and 2 leave) reads **+5 Elo**, 80% interval [−20, +30]; probability positive 60% | low-moderate | below −30 is a defect or a wrong seed: ablate by switch in the order `B4EvasionPrune`, `B4QsMoveCount` 64, `B4QsRecapture`, the blends at 1024, `B4QsFutBase` 150 |
| P2 | After B.4.6 and the gate, B.4 reads **+10 Elo** at STC, 90% interval [−3, +28]; H1 at `[0,3]` probability 50% | low-moderate | H0 with P3 to P5 held says a donor-shaped quiescence carries no Elo on this HCE; the arm stays off and B.5 proceeds |
| P3 | Size: quiescence's share of nodes at depth 14 falls from 42.2% to **30 to 40%**; the arm's `bench 13` is 0.6 to 1.3× the OFF arm's | moderate | above 45% says the recapture exemption outweighs the count rule; the sweep reads `B4QsFutBase` first |
| P4 | Evasions: searched per node falls from 1.85 to **≤ 1.45**; first-move fail highs rise from 63.0% to **≥ 78%**; a legal table move at **≥ 15%** of evasion nodes; `QsEvasion`'s priced beat share **≤ 8%** | moderate | a beat share above 12% says history order does not rescue the rule: item 1's paired run is then read with that figure beside it |
| P5 | Gain rules: `QsCount`'s priced beat share **≤ 6%**; `QsFutility`'s beat share on the arm is **≤ 1.5%**; recaptures exempted are 15 to 30% of the moves the gain rules test | moderate | above 2.5% says the base of 86 costs more than the exemption saves; B.4.6's sweep reads `B4QsFutBase` first, and the whole-arm run is not blamed before it; `B4GainValues` and the futility base's sweep are read before the whole-arm run is blamed |
| P6 | Tactics: WAC at 100,000 nodes in **[239, 255]** | moderate | under 239 is a floor failure and returns the leaf |
| P7 | Floors: KQ-K, KR-K, KBB-K 24 at every budget; KBN-K's sum **≥ 112**; the wrong-bishop draws hold | moderate | a miss attributes by switch, zero-game: `B4EvasionPrune`, `B3FailLowStore`, `B4RfpSeekMate` |
| P8 | The estimate: the depth-0 share of the main search's refinement distance falls from 16.7% to **≤ 13%**; reverse futility's cuts on a table bound stay within 7 to 10% of its cuts | low-moderate | none: sizes 5.2 for the fit |
| P9 | Speed: pooled NPS **0.96 to 1.01×** the OFF arm (`gives_check` is computed for more captures; evasions pay the picker) | moderate | below 0.95 returns the evasion delivery of ticket 1 to implementation for its cost before the gate |
| P10 | Categoricals: item 1 within ±6 (60%); item 2 within ±5 (60%); item 3 within ±5 (65%); item 4 at or below +5 (75%); item 5's gate not met (60%) | low-moderate | item 1 at −5 or below with its interval excluding 0 flips the default and re-reads the floors |
| P11 | The delta rule's deletion: B.4.7 (c) accepts H1, removal is not worse (75%) | moderate | H0 restores the rule on the arm as `B4QsDelta` 1 and its margin becomes a coordinate |

Notes on the frozen table (the table stands):

- **P5** is re-registered by amendment 1 §5 clause 1, a substantive amendment
  made before any ON-arm build. It replaced the `QsFutility` clause and the
  falsifier's first half; the falsifier's last clause is §11's and stands.
- **P5's share** is `qs_recapture_exempt / (qs_gain_tested +
  qs_recapture_exempt)` (amendment 2 §6, clerical, before its registered read).
- **P3, P4's table-move clause and P6** stand as frozen and will score as
  misses on their registered reads (amendment 2 §7). P4's clause is a modelling
  error: a stored fail high ends the node's next visit before the counter, and
  the arm's store writes no move after a fail low. Nothing decides on it.
- **P3's bench clause** reads 0.66× on the arm's tree, 4,003,560 against
  6,090,091; `bench 13` is path-dependent, so it is read with amendment 2 §11's
  finding in mind.
- **P7** fired once, at ticket 7 as built, and found no owner (amendment 2 §7).
  It holds on the final tree (the review's §1).
- **Exposed on unregistered reads, standing as frozen:** on `T6` at depth 12
  over 40 positions, `QsEvasion` 7.99%, `QsFutility` 2.04% and `QsCount` 2.02%
  (amendment 2 §7); the exempt-recapture share 22.1% on the bench positions at
  depth 13 (*Implementation record — tickets 7b to 10*).

- **Registered gate / stop rule.** Stop rules: P1 below −30 (then the ablation order above); a floor failure at any ticket returns the leaf; PLAN rule 6 for the programme.
- **Expected:** P1, P3 to P10. **Falsification:** P1's stop; the floors.

#### RECORDS CARRIED INTO THIS ENTRY (from the review of B.4.4)

- **The cost screen's first row is `suite_v2` 12, not bench 11.** The record's
  file was the set B.3's amendment 8 chose, and its table mislabelled that row.
  Bench 11, the position §10 names, passes by the review's read: steps 20 to 22
  read 0.679, 0.441 and 0.887, geometric mean **0.643**, with no watchdog stop.
- **A2-P5 and A2-P11 are misses as frozen.** Amendment 2 scored each "half";
  each is one statement with one probability and a clause that missed (52.8% of
  games against "at least 60%"; KBN-K 115 against 118). Nothing decides on
  either.
- **Item 1's offset is gated:** `33654f8`, above.
- **`run_suite.py` is repaired:** `cdee38f`, above.

#### RESULT — append after exposure

- **Binaries** (2026-11-06, `abc3b0f`, every manifest clean, clang 23.1.5, PEXT
  final PGO; `tools/results/b45-20261106/build_all.ps1`; pinned in
  `tools/run_b45_games.ps1`): Release ON pool `b45-on`, build 1 5DC68ED8…, OFF pool
  `b45-off`, build 1 D318F3FE…; Tune ON AD267824…; Diag on both arms. Each reads its
  arm's `bench 13`.
- **Zero-game screens** (2026-11-06, `tools/results/b45-20261106/screens/`, read
  by `analyze.py` into `analyze.txt`; packet *B.4.5 — the zero-game screens* has
  the tables):
  - **Counters at depth 14** (`suite_v2`, Hash 64, stride 1): total nodes ON
    10,979,568 against OFF 16,325,679; quiescence 40.9% of nodes (OFF 42.2%).
  - **5.8's candidates:** `B3NmpPerPly` (+16.4%) and `CoreResearchDeeper`
    (−11.6%, a proxy) **join** B.4.6's surface; `B3ProbCutBase`,
    `B3ProbCutImproving`, `CoreCaptFutBase`, `CoreCaptSeeCoeff`,
    `CoreQuietFutBase` and `CoreLmrTtCapture` (a proxy) **stay off**.
  - **Pricing 4, depth 16:** `QsEvasion` 6.37%, `QsCount` 2.15%, `QsFutility`
    1.86%, `QsSee` 1.46% on the arm.
  - **Item 5's gate is met:** razoring's beat share at expected cut nodes reads
    17.31% at 0 and 6.44% at 6, −62.8%, against the rule's quarter. Item 5's
    paired run is owed.
  - **WAC:** 100,000 nodes ON 258, OFF 247; 400,000 nodes 276 and 271.
  - **The canary calibration:** the OFF arm changed since B.3.2, so it was read
    again: at Hash 32 and 128 it flips **0** of the 77 required canaries. ON
    passes 65 and OFF 66, with 13 flips: lost WAC.013, 026, 044, 122, 152, 201
    and 235; gained WAC.030, 031, 059, 083, 125 and 273.
  - **The floors:** KQ-K, KR-K and KBB-K 24 at every budget on both arms; KBN-K
    ON 118, OFF 115; `test_endgames` 191/191 on both. Both arms' 120,000-node
    runs exit 1 on KQ-K's staleness guard, as B.4.4's record says, not on a floor.
  - **The cost screen** (amendment 7 §R on §10's set and on bench 11):
    **PASS** on all six positions, no watchdog stop. Node for node B.4.4's read
    and the review's bench 11.
  - **Pooled NPS** (`nps_read.ps1`, pools `b45-off` and `b45-on`, four builds
    each, two cycles, host at 2%, no cycle disturbed; `tools/results/b45-nps`):
    the arm reads **−5.57%** on `bench 13`, 95% interval [−5.97%, −5.17%],
    2.620M against 2.774M, 0.944×. The tool flags it "not pure speed": the arms'
    trees differ.
- **Items 1 and 2's binding reads**, on variant builds of the head
  (`tools/results/b45-20261106/binding/`):

  | Setting | `bench 13` | Major floors | KBN-K | `test_endgames` | `test_search` | WAC 6 / 100k |
  |---|---:|---|---:|---|---|---|
  | The defaults | 4,003,560 | 24 | 118 | 191/191 | pass | 149 / 258 |
  | `B4EvasionPrune` 0 | 4,540,697 | 24 | 117 | 191/191 | 719/719 | 139 / 251 |
  | `B3FailLowStore` 0 | 3,548,571 | 24 | 117 | 191/191 | 718/719 | 148 / 249 |

  The one `test_search` failure is the arm-default assertion reading the default
  the variant changed. The defaults fail no binding read, so both stay 1 and both
  go to their paired runs. Item 2's table-move reads at depth 14: `tt_move_absent`
  72.58% at 1 and 62.00% at 0; `tt_move_d0` 3.76% and 6.74%.
- **Amendment 2 §8's recorded reads:** `qs_evasion_tt_move` is 3.16% of evasion
  nodes at store 1 and 7.76% at 0. `QsEvasion` by node type: 20.26% [18.41, 22.25]
  of 1,683 probes at PV quiescence nodes, 6.21% [6.09, 6.33] elsewhere.
- **Queen captures by class, the first reading of H.2's candidate:** a queen
  capture that is no recapture beats futility at 21.24% on the arm (45.88% OFF)
  and the SEE rules at 23.04% (27.27% OFF). Exempted recaptures no longer appear
  in the arm's futility cells.
- **Predictions read at the screens, each scored as frozen:**

  | # | Read | Verdict |
  |---|---|---|
  | P3 | quiescence 40.9% against 30 to 40%; bench 0.657× inside 0.6 to 1.3× | **miss** on the share; the falsifier, above 45%, does not fire |
  | P4 | searched 0.881; first-move 98.3%; table move 3.2% against ≥ 15%; `QsEvasion` 6.37% | **miss** on the table move, as amendment 2 §7 foretold; the falsifier, above 12%, does not fire |
  | P5 | `QsCount` 2.15%; `QsFutility` 1.86% against ≤ 1.5%; exempt share 23.6% | **miss** on `QsFutility`; the falsifier, above 2.5%, does not fire |
  | P6 | WAC 258 against [239, 255] | **miss**, high; the floor holds |
  | P7 | floors and draws | **hit** |
  | P8 | depth-0 distance 12.1% against ≤ 13%; table-bound cuts 6.0% against 7 to 10% | **miss** on the second clause; no falsifier |
  | P9 | 0.944× against 0.96 to 1.01× | **miss; the falsifier fires:** the whole interval is below 0.95 |
  | P10 | item 5's gate met, against "not met" (60%) | that clause **missed**; the rest wait on their games |

- **P9's falsifier** reads "below 0.95 returns the evasion delivery of ticket 1 to
  implementation for its cost before the gate". The read is of two different trees,
  so it is not ticket 1's cost alone, and nothing here attributes it. It binds before
  B.4.7's gate, not before this entry's runs. Its handling is the maintainer's.

#### PREDICTION CALIBRATION — preserve the original prediction above
