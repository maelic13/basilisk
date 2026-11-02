# BAS-S41

**BAS-S41 — B.3.2: Cluster 2 diagnostics, the unfitted paired run and the categoricals**

- **Registration.** Created 2026-11-01 at B.3.2's opening, before any
  game; maintainer-run games, agent-run zero-game screens; search
  cluster, Elo layer plus zero-game screens. Assembled, as packet §11
  requires, from §11's BAS-S41 text and every *clauses replaced* list
  that touches it (`analysis/b30_cluster2_research_2026-10-30.md`:
  amendments 1–4, 6 and 8, the *Roadmap audit* of 2026-11-01, and B.14's
  close). Where a clause replaced §11's words, the new words are given
  and the source named; the frozen predictions are copied verbatim.
- **Baseline / candidate.** One revision, the B.3.1 head after B.14
  (BAS-S44 H1): the umbrella-off arm against the umbrella-on arm
  (`-DB3_PROOF=ON`), seeds unfitted (packet §7 with amendments 2 and 3),
  defaults §9 as amended; `bench 13` OFF **11,880,564**, ON
  **4,923,507**. PGO pairs per arm (PROCESS *Cluster delivery shape*);
  the categoricals on one ON Tune build of the same revision, the
  alternative as arm A by UCI option against the defaults as arm B.
  Binary identities and hashes are recorded below at build time.
- **Research question.** Does the coherent Cluster 2 arm (proof searches
  on better-informed estimates: the null move, ProbCut, the singular
  block with its graded extension and budget, IIR, the check policy,
  upcoming repetition, the TT penalty) land without an Elo defect before
  fitting, and which of its categorical forms does the engine want?
- **Hypothesis / mechanism.** Packet §12 question 1.
- **Interactions.** Packet §5 with its numbers; §5.12's cross-cluster
  surface rule, read here for B.3.3.
- **Competing explanations.** A wrong seed column or a defect (P1 below
  −40, ablated by switch in P1's order before a mechanism is blamed); a
  check-policy decision that costs Elo (items 1–2); the null move's
  cut-node restriction losing at longer time controls (item 3, B-33).

#### ORDER (the *Roadmap audit*'s clause, replacing §11's)

1. **The zero-game screens**, both arms, all recorded before the first
   game:
   - §2.1's counter table at depth 14 on `suite_v2` (the §5.12
     candidates' two shares and verdicts included), with the in-check
     share (B-54) and the crossover band (§5.3);
   - the fixed-node differential against the oracle at 300,000 nodes
     (`run_suite.py --oracle`, the B.0 recipe);
   - WAC at 100,000 and 400,000 nodes and `depthpv 10`;
   - the canary calibration (B-94: the OFF arm at Hash 32 and 128
     against 64, flips counted) before the ON arm's canaries are read;
   - the cost screen (§10, amendment 7 §R's clause on amendment 8's set),
     since B.3.1 ran it on an earlier revision;
   - pooled NPS (`tools/nps_pool.ps1`, ≥ 2 PGO builds per arm, an idle
     host);
   - the `B3NmpVerify` counter read (§9 item 5);
   - the won-position row (amendment 8 decision 4): the ON arm's
     `probcut_tries`, `probcut_cuts`, `qs_nodes`, `qs_evasion_nodes` and
     `razor_cuts` per interior node on the bench positions whose OFF root
     score at depth 14 is at or beyond ±564, beside the whole-suite
     shares;
   - P5 scored at the opening, the mechanism named from the ablation
     table;
   - the floors tool's staleness exit (KQ-K's mean at 120,000 nodes on
     the OFF arm) told from a floor failure before the first read.
2. **§9 items 1–3**, which decide the arm the whole-arm run measures:
   five runs, because item 2's read B is item 1's `3+0.03` run.
3. **The whole-arm paired run** on the defaults items 1–3 leave. It
   governs: P1's interval, the −40 stop.
4. **Items 4–16** in §9's order as amended: 4, 5, the small TT ProbCut
   (`B3ProbCutTt`, directly after item 5, amendment 2), `B3NmpEvalGate`
   (after item 5, amendment 2), 14 (after item 5, amendment 3), 15, 6, 7,
   8, 9, 10, 11, 12, then 16 last. Items 6, 7, 8 and 11 run only if the
   depth-14 counters on the arm the paired run measured show the
   alternative moving its mechanism's firing share by 10% or more of
   itself (§5.12's rule); otherwise the default stays and the read is
   recorded. Item 13 (`B3ExtBudget`) is settled without games (amendment
   2: the budget stays). Item 12 runs only after the cost screen passes
   with `B3Iir` 0 (B-5). Item 16 is preceded by `tt_move_absent` on both
   settings at depth 14 (amendment 6).

#### CONDITIONS

- **Every run:** Colosseum `match-fixed`, 2,000 games (1,000 paired
  reversed UHO openings), 1T, Hash 64, no adjudication, tablebases off,
  the recount from the PGN.
- **Time control:** `3+0.03`; items 1 and 3 also at `10+0.1`.
- **Categorical rule** (PROCESS *Cluster delivery shape*, as the audit
  amended it): adopted at ≥ +5 Elo and rejected at ≤ −5, each only when
  the 95% interval also excludes 0; the default kept otherwise. 2,000
  games read about ±8, so a true-zero categorical crosses ±5 about one
  time in four (the audit). Item 1 adopts 1 only if neither read is ≤ −5
  and one is ≥ +5. Item 2's read A at ≤ −5 flips both check defaults to
  the legacy policy and reopens §5.1. Item 3 keeps 1 unless the
  `10+0.1` read is ≤ −5.
- **Runs and seeds**, registered before any game:

| Run | Arm A (by UCI option) | Arm B | Control | Seed |
|---|---|---|---|---|
| item 1 STC (= item 2 read B) | `B3CheckExt=1` | defaults | `3+0.03` | 41001 |
| item 1 LTC | `B3CheckExt=1` | defaults | `10+0.1` | 41011 |
| item 2 read A | `B3CheckExt=1`, `B3CheckPrune=0` | defaults | `3+0.03` | 41002 |
| item 3 STC | `B3NmpCutOnly=0` | defaults | `3+0.03` | 41003 |
| item 3 LTC | `B3NmpCutOnly=0` | defaults | `10+0.1` | 41013 |
| the paired run | ON release (defaults after items 1–3) | OFF release | `3+0.03` | 41000 |
| item 4 | `B3MultiCut=1` | defaults | `3+0.03` | 41004 |
| item 5 | `B3NmpVerify=0` | defaults | `3+0.03` | 41005 |
| item 5, 1 against 2 (only if the same-ply form changes > 1% of null cuts) | `B3NmpVerify=2` | defaults | `3+0.03` | 41025 |
| `B3ProbCutTt` | `B3ProbCutTt=1` | defaults | `3+0.03` | 41051 |
| `B3NmpEvalGate` | `B3NmpEvalGate=1` | defaults | `3+0.03` | 41052 |
| item 14 | `B3SingNegative=1` | defaults | `3+0.03` | 41014 |
| item 15 | `B3PvQuietPrune=0` | defaults | `3+0.03` | 41015 |
| item 6 | `B3SingTtDepth=1` | defaults | `3+0.03` | 41006 |
| item 7 | `B3IirStale=1` | defaults | `3+0.03` | 41007 |
| item 8 | `B3ProbCutPv=0` | defaults | `3+0.03` | 41008 |
| item 9 | `B3Cuckoo=0` | defaults | `3+0.03` | 41009 |
| item 10 | `CoreHistoryFills=1` | defaults | `3+0.03` | 41010 |
| item 11 | `B3CorrExcluded=0` | defaults | `3+0.03` | 41012 |
| item 12 | `B3Iir=0` | defaults | `3+0.03` | 41020 |
| item 16 | `B3FailLowStore=1` | defaults | `3+0.03` | 41016 |

- A default flipped by items 1–3 is set in the ON arm's source before
  the paired run (a fingerprint move, recorded here) and the Tune build
  is rebuilt; the later categoricals read on that arm.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Packet §11's table, verbatim (frozen 2026-10-30):

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, ON against OFF at one revision, §7 seeds, §9 defaults) reads **+10 Elo**, 80% interval [−15, +35]; probability positive 65%; probability of the −40 defect stop 15% | moderate | below −40 is a defect or a wrong seed column; the switches of §9 ablate it before any mechanism is blamed, in the order check policy, `B3NmpCutOnly`, the singular margin at 378 (today's 4/ply) |
| P2 | After B.3.3 and the gate, B.3 reads **+20 Elo at STC**, 90% interval [0, +45]; H1 probability 55% | moderate | H0 at `[0,3]` or an estimate under +5 refutes the coherent-cluster share for proof searches; the residual then lies in B.4/B.5 |
| P3 | NMP: `null_tries/interior` falls to **≤ 50%** of the OFF arm's 7.48%; conversion rises from 41.5% to **≥ 65%**; the region fires at depth ≥ 16 and refuses under 5% of its null cuts there | moderate-high | conversion under 55% says the margin or the population did not land |
| P4 | Singular: tries stay within 1.5–3.5% of interior; extensions rise from 0.69% to **≥ 1.2%** of interior; multi-cuts 10–30% of tries; triples ≤ 15% of extensions; negatives 5–25% of tries | moderate | extensions under 0.9% say the margin did not reach the population; triples above 25% say the bars are mis-scaled |
| P5 | Nodes: the ON arm's `bench 13` within **0.6–1.4×** of the OFF arm's; `suite_v2` at depth 14 within 0.5–1.5× | moderate | outside the band on both reads says a mechanism exploded or died; the cost screen and the liveness proof say which |
| P6 | Tactics: WAC at 100,000 nodes in **[232, 262]**; the depth-6 count ≥ 175 | moderate | under 232 is a floor failure and returns the leaf |
| P7 | Floors: KQ-K, KR-K, KBB-K 24 at every budget; KBN-K's five-budget sum **≥ 112** | moderate-high | a miss attributes to the multi-cut rule, `B3CorrDecisive` or the check policy by the switches, zero-game |
| P8 | The budget: `ext_budget_bound` = 0 on `suite_v2` at depth 14 and on `bench 13`; under 0.1% of extensions at depth 20 | high | binding at depth ≤ 14 says the graded extension is mis-scaled, not that the budget is wrong |
| P9 | Speed: pooled NPS **0.98–1.01×** the OFF arm (the cuckoo loop and `follow_pv` are the only per-node additions) | moderate-high | below 0.97 sends the cuckoo loop to B.7 before the gate |
| P10 | Categoricals: `B3CheckExt` 1 against 0 within ±8 (the trigger's premise: with informed pruning the extension's value is gone); `B3NmpCutOnly` 0 within ±5 at STC and ≤ 0 at `10+0.1`; `B3MultiCut` 1 within ±5 if the donor's rule survives KBN-K | low-moderate | `B3CheckExt` ≥ +5 at both controls says the policy decision was wrong and flips the default |

Notes the amendments attach to the frozen table (the table stands):

- **P1's ablation order** (amendment 2 §6, clerical): "the singular
  margin at 378 (today's 4/ply)" reads "at 252 (today's 4 cp per ply;
  378 is 6 per ply, the range top)".
- **P6's depth-6 leg** is retired (amendment 2 §6): the depth-6 floor
  was re-based on the ON arm at 119, a regression guard, not a
  correctness floor (the audit); the 100,000-node leg stands.
- **P5** is not amended (amendment 1); its early input is amendment 1
  §2's 0.28× at ticket 1; it is scored at the opening (the audit).

Predictions frozen by the amendments for B.3.2's runs (verbatim):

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P13 | The paired run of `B3PvQuietPrune` 0 against 1 reads within ±5 | moderate | ≤ −5 says the donor's PV pruning costs the engine the endings its evaluation cannot see, and the default flips to the OFF arm's exemption |
| P17 | B.3.2's paired run of §9 item 11 (`B3CorrExcluded` 1 against 0 on the ON arm, G-C in place) reads within ±5 Elo — probability 60% | — | a read ≤ −5 says the biased estimator costs more than the donor's tactics gain on this evaluation and flips the default to 0 by §9's rule, no return |
| P32 | §9 item 16's paired run (store 1 against the default 0, 2,000 games at `3+0.03`) reads inside ±5 and leaves the default (60%) | — | a read ≥ +5 adopts 1 and B.3.3's sweep of `B3IirMinDepth` and `CoreLmrCutNodeNoTt` then runs on that population |
| P33 | `tt_move_absent` at depth 14 on the ON arm is at least 5 points of interior nodes higher with the store at 1 than at 0 (80%; the ticket-8 → ticket-9 read was +7.3) | — | — |

(P13 is amendment 3 §4's; P17 amendment 4 §4's; P32 and P33 amendment
6 §4.6's. P12 and P14 belong to BAS-S42 and BAS-S43.)

- **Registered gate / stop rule.** The paired run governs: P1's
  interval; below −40 is the defect stop, then the switch ablation in
  P1's order. A floor failure returns the leaf. PLAN rule 6 for the
  programme.
- **Expected:** P1, P3–P10. **Falsification:** P1's stop; the floors.

#### PREDICTION CALIBRATION — notes the amendments recorded for this entry

- **P8** (amendment 1 §4, verbatim): *"P8's `bench 13` leg was falsified
  on 2026-10-30 before any ON-arm mechanism existed, on the legacy
  extension at ticket 1 (12,800 truncations of 89,121 firings). Cause:
  the mechanism assumption that a bound of `root_depth_` plies is looser
  than a cap of 16 doubles, which is false; the check extension's
  lengthening of forcing lines was the missed interaction. Sign and
  magnitude wrong; confidence over-calibrated (it was marked high). The
  prediction is scored a miss on the mechanism, and the budget's
  expectation is re-registered as P8a below, before any ON-arm build
  carrying the budget exists."*
- **P8a** (amendment 2 §6, verbatim): *"P8a was falsified at ticket 6 on
  2026-10-30 (76.7% at depth 13, 82.4% and 96.6% at depth 20). Its band
  [0.5%, 10%] came from one read on the legacy extension (1.22%) and
  ignored the prior the same amendment cited: Rarog's budget truncated
  44,914 extensions at bench 13, about half of its extensions. Second
  miss of the same kind as P8's; confidence (moderate) was
  over-calibrated and should have been low."*
- **P8b** (amendment 3 §4, verbatim): *"P8b's lower edge assumed the
  singular block runs in won positions, where the triples chain into the
  budget (position 11 alone carried 25,946 of the ticket-6 arm's 30,255
  truncations); with the donor's guard fired from root depth 12 those
  positions leave the block above depth 11 and the budget binds on the
  rest at 15%. The band should have been conditioned on the guard."*
- **P5's early input** (amendment 1 §2): 0.28× at ticket 1; 25.7× at
  `1e77ec9` and 0.38× on V6 (amendment 3 §4); 0.40× (bench) and 0.26×
  (suite) at the audit.

#### RESULT — append after exposure

- **Binaries** (2026-11-01, `c6f7151`, a documentation commit on the
  engine of `1b2515b`; every manifest clean, clang 23.1.4, PEXT final
  PGO; `tools/results/b32-20261101/build_all.ps1`):
  - Release ON `pgo1` `4CE2E748…`, `pgo2` `040BF082…` (`bench 13`
    4,923,507); OFF `pgo1` `A919936F…`, `pgo2` `2F981F2B…` (11,880,564).
  - ON Tune `B757C936…` (4,923,507), and Diag and Ablate on both arms,
    each at its arm's fingerprint.
- **Zero-game screens** (2026-11-01, deterministic node and depth reads;
  `tools/results/b32-20261101/screens/`, read by `analyze.py` into
  `analyze.txt`; packet *B.3.2 — the zero-game screens* has the tables):
  - **Counters at depth 14** (`suite_v2`, Hash 64, stride 1, the LMR
    identity holds on both arms): total nodes ON 13,587,191 against OFF
    80,635,359. In-check interior nodes 16.8% (OFF 26.5%); check
    extensions 0 (OFF 26.2%). The null move tries 8.30% of interior nodes
    (OFF 7.49%) and converts 56.9% (OFF 41.4%). ProbCut tries 6.66% and
    converts 76.9% (OFF 1.40%, 53.9%). Singular: tries 1.67%, extensions
    0.52% of interior, multi-cuts 23.7% and negative extensions 42.0% of
    tries, three-ply extensions 83.8% of extensions, `ext_budget_bound`
    37.0% of extensions.
  - **The crossover band (§5.3):** RFP cuts 22.75 / 1.75 / 0.28% at
    remaining depths 1–3 / 4–7 / 8+ (OFF 14.08 / 0.81 / 0.05%); NMP
    tries 1.82 / 4.01 / 2.47% converting 58.5 / 61.1 / 48.9% (OFF 2.30 /
    4.19 / 1.00%, 37.4 / 42.4 / 46.6%). The crossover stays at depth 4.
  - **§5.12's candidates** (the mechanism's firing share, ON against OFF,
    and the in-check share of the ON arm's firings): **join** B.3.3's
    surface `CoreLmpBase` (+36.1%), `CoreQuietFutBase` (+52.6%),
    `CoreCaptFutBase` (−28.0%), `CoreCaptSeeCoeff` (−16.2%),
    `CoreContHistPruneSlope` (−81.7%; in check 21.6%),
    `CoreRfpCorrMult` with amendment 2's `CoreRfpPerPly`,
    `CoreRfpImproving` and `CoreRfpDepthCap` (RFP +65.8%), and
    `CoreCorrUpdateScale` (+82.1%). **Stay off:** `CoreLmrCutNode`,
    `CoreLmrTableScale` and `CoreLmrOffset` (LMR applied per move tested
    −6.4%, in check 4.1%) and `CoreLmrCutNodeNoTt` (`tt_move_absent`
    +8.4%). The LMR and no-TT-move shares are proxies: no counter
    separates cut nodes.
  - **The oracle differential at 300,000 nodes:** best-move agreement ON
    54/105, OFF 56/105; median depth ON 17, OFF 15.
  - **WAC:** 100,000 nodes ON 240, OFF 240; 400,000 nodes 264 and 264;
    `depthpv 10` 223 and 248 (`fixed_budget_probe.py`; B.14's
    `test_wac` read 241 and 240 at 100,000 on the same engine).
  - **The canary calibration (B-94):** the OFF arm at Hash 32 and 128
    flips **0** of the 77 required canaries against Hash 64 (70 pass at
    every size). Read after it, the ON arm passes **65**: it loses
    WAC.031, 072, 125, 126, 150 and 164 and gains WAC.255. With the
    neutral flip count at 0, all seven flips are the arm's.
  - **The cost screen** (amendment 7 §R on amendment 8's set,
    `cost_screen25.json`, scored by the registered `screen_score_a7.py`):
    **PASS** on all five positions, depth 25 finished on both arms with no
    watchdog stop. It is node for node ticket 10b's read: the five
    positions never reach a B.14 verdict.
  - **Item 5's counter:** with the same-ply form (`B3NmpVerify=2`) the ON
    arm refuses 147 of 406,493 null cuts (0.036%), under 1%. So the
    1-against-2 run is not owed, and PLAN's `[-5,0]` arm is retired as
    behaviour-identical by §9's rule. The OFF arm's legacy verification
    has no counter, so §9's "on the OFF arm" read cannot be taken with
    the committed instruments; the ON read answers the rule's question.
    The region (the default) verifies 1,577 times and refuses 2.
  - **The won-position row** (amendment 8 decision 4; the ten bench
    positions whose OFF root at depth 14 is at or beyond ±564: 5, 10–12,
    15, 16, 29, 31, 38, 39). Per interior node on the ON arm, for the won
    subset, all 40 positions and `suite_v2`: ProbCut tries 6.12 / 7.03 /
    6.66%; ProbCut cuts 4.91 / 5.82 / 5.12%; quiescence 87.2 / 88.1 /
    75.9%; evasions 11.3 / 6.7 / 7.0%; razoring 2.44 / 2.94 / 3.27%.
    ProbCut's won-position shares are not above the whole-suite shares.
  - **The floors**, read on this engine at B.14 (BAS-S44
    §QUALIFICATION, not repeated: same source): KQ-K, KR-K and KBB-K 24
    at every budget on both arms; KBN-K OFF 115, ON 120. The OFF arm's
    120,000-node run exits 1 on the tool's staleness guard alone: KQ-K
    converts 24/24 with a mean of 10.8 plies against its ceiling of 16.3
    (`conv_commit_off_120000.log`). That is not a floor failure.
  - **Pooled NPS** (P9): not read. It needs an idle host, and the
    maintainer is using it.
- **Predictions read at the opening:**
  - **P3 missed on two legs.** Null tries per interior node are 1.11×
    the OFF arm's share, not ≤ 50%, and conversion is 56.9%, not ≥ 65%.
    The falsifier (under 55%) does not fire. The region leg holds: 2
    refusals in 1,577 verifications. The shares are of a tree 6.7×
    smaller, so the per-node rates compare different populations.
  - **P4 missed on three legs; two falsifiers fire.** Tries hold (1.67%)
    and multi-cuts hold (23.7%). Extensions read 0.52% of interior nodes
    (falsifier: under 0.9%, "the margin did not reach the population"),
    three-ply extensions 83.8% (falsifier: above 25%, "the bars are
    mis-scaled") and negatives 42.0% (band 5–25%). The three-ply share
    has been on record since Return 2 (98.3% at depth 13) and amendments
    2 and 3 set the bars. The falsifiers are diagnoses, not §11 stop
    rules.
  - **P5 missed on both reads, the mechanism named.** `bench 13` is
    0.414× and `suite_v2` at depth 14 0.169×. The 40 bench positions
    searched alone at depth 13 read 0.316× (`ablate_alone13.json`, both
    arms' Ablate builds; `bench` itself ignores `AblationMask`). With
    the extension family (bit 6) removed on both arms the ratio is
    **0.780**, inside the band. With any other single bit removed it is
    0.25–0.53. The OFF arm's extensions multiply its tree 4.51× (its
    unconditional check extension extends 26.2% of interior nodes); the
    ON arm's graded singular extension multiplies its tree 1.83×. So the
    mechanism is the check policy (§5.1) removing the check extension,
    by design: it "died". Item 1 reads it in games.
  - **P6 held:** WAC 240 at 100,000 nodes, in [232, 262].
  - **P7 held:** the floors as above, KBN-K 120 ≥ 112.
  - **P8 missed again (it was scored a miss in amendment 1):**
    `ext_budget_bound` is 14,889 of 40,266 extensions on `suite_v2` at
    depth 14 and 2,053 on the bench positions at depth 14. P8b at depth
    13 on the 40 bench positions reads **16.2%** (1,767 of 10,904):
    under the band's lower edge, as on V6 (14.9%). Its upper edge, the
    return trigger (70%), does not fire.
  - **P9:** pending the idle-host NPS read. **P1, P10, P13, P17, P32 and
    P33** are read by the games and their counter reads.
- **§9 item 1 at `3+0.03`, which is also item 2's read B** (maintainer-run
  2026-11-01; `B3CheckExt=1` as arm A against the defaults; seed 41001;
  2,000 games; 0 faults, 0 time losses; the PGN recount agrees):
  - **−22.4 ± 9.7 Elo** (nElo −35.4 ± 15.2); W/D/L 457/957/586;
    pentanomial [57, 272, 444, 197, 30]. The 95% interval [−32.1, −12.7]
    excludes 0.
  - Per move, the check extension searches 14.59 plies on average without
    mate scores (median 14) against 16.38 (median 15) for the default. It
    spends 125,636 nodes per move at 1.77M nodes per second, against
    133,797 at 1.92M.
  - Provenance: manifest `colosseum_match_b32-cat-CheckExt_20261101_223700.manifest.txt`,
    repo `92575a3` clean, host 3.8% busy and not waived, dry run
    `13A24567…`; run record `0409D8E9…`, PGN `A3EA295E…`.
  - **By the registered rule:** item 1 adopts 1 only if neither read is
    ≤ −5 and one is ≥ +5. This read is ≤ −5 with its interval excluding
    0, so the unconditional check extension cannot be adopted whatever
    the `10+0.1` read says, and the default 0 stands. Item 2's read B,
    the default against (1, 1), reads +22.4 for the default.
  - **P10's first leg missed:** it predicted within ±8. The sign is the
    premise's: with informed in-check pruning, the extension costs
    depth.
  - The `10+0.1` run stays registered and is read as recorded: the rule
    is already decided, but it is BAS-X01's re-read, and a registered
    run is not shrunk after a result.
- **§9 item 2, read A** (maintainer-run 2026-11-01; the legacy check
  policy `B3CheckExt=1`, `B3CheckPrune=0` as arm A against the defaults;
  seed 41002; 2,000 games; 0 faults, 0 time losses; the PGN recount
  agrees):
  - **−30.8 ± 9.5 Elo** for the legacy policy (nElo −49.6 ± 15.2); W/D/L
    425/973/602; pentanomial [55, 298, 441, 181, 25]. Read as the rule
    states it (the default against the legacy policy) that is **+30.8**,
    interval [+21.3, +40.3].
  - Per move, the legacy policy searches 14.09 plies on average without
    mate scores against 16.46, at 1.77M against 1.90M nodes per second.
  - Provenance: manifest `colosseum_match_b32-cat-CheckLegacy_20261101_230307.manifest.txt`,
    repo `bd0a71b` clean, host 2.9% busy and not waived, dry run
    `8693F554…`; run record `50918E8C…`, PGN `50575899…`.
  - **By the registered rule** (read A at ≤ −5 flips both defaults): it
    is not ≤ −5, so both check defaults stand (`B3CheckExt` 0,
    `B3CheckPrune` 1) and §5.1 stays closed. Together with item 1, the
    donor's check policy is worth about 31 Elo over the legacy policy
    on this arm, and about 22 of them come from not extending checks.
- **Preconditions for items 4–16** (2026-11-01, zero-game, on the ON
  Tune build at the defaults items 1–3 leave unless item 3 moves one;
  `screens/suite_d14_on_*.json`, depth 14, `suite_v2`):
  - **The 10% rule**, the alternative's firing share against the
    default's: item 6 `B3SingTtDepth=1` singular tries −17.1% (runs);
    item 7 `B3IirStale=1` IIR +137.9% (runs); item 8 `B3ProbCutPv=0`
    ProbCut tries +4.2% (no run: the default 1 stands); item 11
    `B3CorrExcluded=0` correction updates +3.7% (no run: the default 1
    stands, and P17 is not read; the update count is a proxy, since no
    counter isolates training at exclusion nodes).
  - **P33 held:** `tt_move_absent` reads 73.0% of interior nodes with
    the store at 1 against 65.2% at 0, +7.9 points (≥ 5).
  - **Item 12's cost screen with `B3Iir` 0** (`screen_opts.py`, the
    option wire proved live first: the arm moves on all five positions
    at depth 12 and the base does not): **PASS** on all five, no
    watchdog stop (steps 20–22 geomeans 1.137, 0.507, 1.035, 1.045,
    1.045). Item 12 runs.
  - Item 4 runs (the KBN-K majority rule passes at the default, ticket
    10's read, the floors unchanged by B.14). Item 9's terminal tests
    pass in release CTest. Item 13 has no run (amendment 2).
- **The overnight queue, 2026-11-01/02** (maintainer-run from one
  chained command; every run on repo `d567850`, clean, host at most 0.26%
  busy and never waived, 0 faults, 0 time losses, the PGN recount
  agreeing; logs `tools/results/b32-night/`):
  - **Item 1 at `10+0.1`** (seed 41011): `B3CheckExt=1` reads
    **−5.4 ± 8.6 Elo** (nElo −9.6 ± 15.2; W/D/L 489/991/520;
    pentanomial [20, 255, 478, 230, 17]; 17.83 against 20.23 plies per
    move without mates). Recorded; item 1's verdict was already fixed
    by its `3+0.03` read: **the default 0 stands.** Run record
    `5A699FDA…`, PGN `15380C7B…`.
  - **Item 3 at `3+0.03`** (seed 41003): `B3NmpCutOnly=0` reads
    **−1.4 ± 9.2** (W/D/L 495/1002/503; pentanomial [31, 239, 471, 225,
    34]). Run record `FF0601DA…`, PGN `A995B25C…`.
  - **Item 3 at `10+0.1`** (seed 41013): `B3NmpCutOnly=0` reads
    **−5.0 ± 9.0** (W/D/L 486/999/515; pentanomial [28, 251, 471, 222,
    28]), so cut-only reads +5.0 at the long control with its interval
    including 0. By the rule (keep 1 unless the `10+0.1` read is ≤ −5
    for it), **the default 1 stands.** Run record `903018D1…`, PGN
    `8FBF9F1A…`.
  - **Items 1–3 move no default**, so the paired run measured the
    registered arm.
  - **The paired run** (seed 41000; ON `0B5D9DC6…` against OFF
    `8E5B9874…`): **+67.5 ± 9.7 Elo** (nElo +108.7 ± 15.2); W/D/L
    767/850/383; pentanomial [12, 138, 388, 378, 84]. Per move the ON arm
    searches 16.57 plies without mate scores against 13.96, at 145,496
    nodes and 2.07M nodes per second against 151,386 and 2.12M. Run
    record `6FEE0406…`, PGN `D47B3F00…`, dry run `B4821293…`. Far above
    the −40 stop, so items 4–16 follow. PROCESS re-runs no Colosseum
    result on the backup path, a surprising one included.
  - **Pooled NPS** (`nps_read.ps1`, pools `b32-on` and `b32-off`, four
    builds each, two cycles, CPU 30, host idle; archive
    `tools/results/b32-nps-try1`): the ON arm reads **−11.09%** on
    `bench 13`, 95% interval [−11.39%, −10.78%] (2.813M against
    3.164M), next DONE. The tool flags it "not pure speed": the arms'
    `bench 13` trees differ (4,923,507 against 11,880,564 nodes).
  - **Items 4–16 did not run.** The order guard read the paired run's
    "+67.5" as unparseable, because its pattern allowed only a minus
    sign, so it refused each launch and played nothing. The pattern now
    accepts a sign. On all six finished runs it reads the estimates and
    the adoption rule as above, and item 4's dry run is accepted.
- **Predictions read:**
  - **P1 missed high.** It predicted +10 with an 80% interval of
    [−15, +35]; the read is +67.5 ± 9.7, positive as its 65% said, and
    the −40 stop is far away. Same direction as BAS-S21's P1 (−15 ± 30
    against +130.9). The model underpriced what the donor's proof
    searches add once Cluster 1's core feeds them; about 2.6 plies per
    move is the measured channel, with about 22 Elo of it from the check
    policy alone (items 1–2).
  - **P9 missed, and its falsifier fires as written.** 0.889× is below
    0.97, which "sends the cuckoo loop to B.7 before the gate". The
    premise it tested, that the cuckoo loop and `follow_pv` are the only
    per-node additions, is not what this instrument separates. The two
    arms' `bench 13` trees differ in composition (quiescence 43% of the
    ON arm's nodes against 36%, ProbCut tried at 6.7% against 1.4% of
    interior nodes), and in games the same arms read 2.07M against 2.12M
    nodes per second (−2.4%). The consequence is recorded for the
    maintainer's decision before B.3.5's gate; the per-node cost of the
    cuckoo loop is unmeasured.
  - **P10:** the `B3CheckExt` leg missed (STC −22.4, outside ±8; LTC
    −5.4 inside); its falsifier (≥ +5 at both) did not fire. The
    `B3NmpCutOnly` leg held (STC −1.4 within ±5, LTC −5.0 ≤ 0). The
    `B3MultiCut` leg waits for item 4.
- **Items 4–16** (maintainer-run 2026-11-02; ON Tune `B757C936…`; repo
  `2537f15` clean for every run; host at most 2.1% busy and never
  waived; 0 faults, 0 time losses; every PGN recount agreeing; arm A the
  alternative, so a positive result favours it):

| Run | Arm A | Elo (nElo) | W/D/L | Pentanomial | Verdict | Run record, PGN |
|---|---|---|---|---|---|---|
| 4 (41004) | `B3MultiCut=1` | +3.3 ± 9.6 (+5.2) | 522/975/503 | [32, 254, 419, 253, 42] | default 0 kept | `CA4F7214…`, `FB07425D…` |
| 5 (41005) | `B3NmpVerify=0` | −5.0 ± 8.8 (−8.7) | 514/943/543 | [21, 259, 474, 220, 26] | default 1 kept (interval holds 0) | `C680CF00…`, `E59268A2…` |
| `B3ProbCutTt` (41051) | `=1` | +0.9 ± 9.5 (+1.4) | 529/947/524 | [41, 228, 453, 241, 37] | default 0 kept | `B681675C…`, `DA5475B2…` |
| `B3NmpEvalGate` (41052) | `=1` | **+11.3 ± 9.3** (+18.6) | 538/989/473 | [37, 195, 466, 270, 32] | **adopted by the rule; blocked by two binding tests (below)** | `66AC249B…`, `2F4191D6…` |
| 14 (41014) | `B3SingNegative=1` | 0.0 ± 9.2 (0.0) | 541/918/541 | [36, 229, 464, 241, 30] | default 3 kept | `2A47A629…`, `1D387629…` |
| 15 (41015) | `B3PvQuietPrune=0` | +5.9 ± 9.9 (+9.1) | 531/972/497 | [43, 231, 419, 263, 44] | default 1 kept (interval holds 0) | `C7CB880A…`, `C18DED3B…` |
| 6 (41006) | `B3SingTtDepth=1` | −1.0 ± 9.2 (−1.7) | 512/970/518 | [31, 242, 463, 230, 34] | default 0 kept | `22895C56…`, `61DE0474…` |
| 7 (41007) | `B3IirStale=1` | +2.8 ± 9.2 (+4.6) | 503/1010/487 | [30, 238, 448, 254, 30] | default 0 kept | `42134BE9…`, `FD195991…` |
| 9 (41009) | `B3Cuckoo=0` | 0.0 ± 9.3 (0.0) | 510/980/510 | [31, 252, 428, 264, 25] | default 1 kept | `A671E7CE…`, `3816BA2A…` |
| 10 (41010) | `CoreHistoryFills=1` | −0.5 ± 9.3 (−0.9) | 507/983/510 | [36, 236, 452, 247, 29] | default 0 kept | `91C51040…`, `D27EEC79…` |
| 12 (41020) | `B3Iir=0` | **−11.3 ± 9.2** (−18.6) | 472/991/537 | [41, 255, 456, 224, 24] | **IIR off rejected**: default 1 kept, interval [−20.5, −2.1] | `385F3079…`, `6C02568C…` |
| 16 (41016) | `B3FailLowStore=1` | +3.3 ± 9.1 (+5.5) | 509/1001/490 | [29, 224, 480, 233, 34] | default 0 kept | `DB296C1E…`, `ABCB25E7…` |

  Items 8, 11 and 13 had no run (the 10% rule; amendment 2).
- **The adoption of `B3NmpEvalGate` 1 is blocked.** Applied as one line
  (`adopt/adopt_gate.patch`, SHA-256 96856677…, the default 0 → 1), it
  moves the ON arm's `bench 13` to 5,920,540 on Release, Tune, Diag and
  Ablate (`bench 10` 1,240,782 release and sanitizer); the OFF arm is
  unchanged at 11,880,564. On the ON arm the binding set otherwise holds:
  WAC 243 at 100,000 nodes and 132 at depth 6; floors 24 for KQ-K, KR-K
  and KBB-K and KBN-K 120; the wrong-bishop 28, the organic 7/7 and the
  primed 28/28; `test_endgames` exit 0 (`adopt/adopt_price.{txt,json}`).
  Two release tests fail (`adopt/ctest_on.log`):
  - `test_search`, "promotion: a queen where a rook only transposes":
    the ON arm plays `a7a8q` but scores it **0** at 33,000, 60,000 and
    100,000 nodes, where its clause asks for a positive score at
    100,000. It sights the win from 200,000 nodes (+1,232; +1,297 at
    400,000; +1,567 at 1,000,000): the tie at 0 that research
    amendment 9 found on the OFF arm.
  - `test_b3_proof`, "B3FailLowStore and B3TtPenalty move the ON arm
    only": with the gate on, `B3TtPenalty` 0 no longer changes the node
    count on that section's position (line 550).

  Neither test is relaxed, and a strength result does not outrank a
  binding test, so the change is not committed. The source keeps the
  default 0 and the ON arm's fingerprint stays 4,923,507 (rebuilt and
  read on every flavour). The adoption waits for the maintainer's
  decision.
- **Predictions read:** P10's `B3MultiCut` leg held (+3.3, within ±5).
  P13 missed narrowly: `B3PvQuietPrune` 0 read +5.9, outside ±5; its
  falsifier (≤ −5) did not fire. P32 held: the store read +3.3, inside
  ±5, and leaves the default.
- **The paired run's binaries are re-pinned before any game used
  them.** Building the NPS pools (`nps_pool.ps1 -Revision c6f7151`, four
  clean builds per arm, pools `b32-on` and `b32-off`) staged its builds
  under the same names as the first `pgo1` and `pgo2`, replacing them.
  The paired run now plays each pool's build 1: ON `0B5D9DC6…`, OFF
  `8E5B9874…`, both clean at `c6f7151` with their arms' fingerprints.
  Items 1 and 2 played the Tune build, which was untouched
  (`B757C936…`).
