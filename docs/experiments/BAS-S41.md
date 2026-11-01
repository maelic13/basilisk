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

- (zero-game screens, then each run, appended in order)
