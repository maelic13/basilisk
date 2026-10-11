# BAS-S21

**BAS-S21 — B.2.2: Cluster 1 diagnostics and the unfitted paired run**

- Date / owner / calibration category: registered 2026-10-11 (Amendment 3,
  packet §22.7), before any game; maintainer-run; search cluster, Elo layer
  plus zero-game screens.
- Baseline source revision / candidate source revision / dirty-diff identity:
  the accepted B.2.1 head (the delta review's acceptance names it; the B.1
  head `6c5ee63`-lineage off arm at bench 14,978,465 is the baseline). One
  revision, two arms: the umbrella-off arm against the umbrella-on arm
  (`-DB2_CORE=ON`), seeds unfitted (packet §7.3, Amendments 1–3).
- Binary / compiler / flags / PGO identity and hashes: recorded at run time
  in this entry; PGO pairs per arm as PROCESS *Cluster delivery shape*.
- Research question: does the coherent Cluster 1 core (TT, SEE, picker,
  histories, correction, node and move-loop pruning, LMR, update policy) land
  in the donor's population without an Elo defect, before fitting?
- Hypothesis / proposed mechanism: packet §2.6 and §13; the move-loop family
  is the ×4 node multiplier, and the cluster adopts the donor's architecture
  as a unit so its terms are fitted to each other.
- Interacting mechanisms / consumers: the kept legacy NMP, ProbCut, singular,
  check extension, IIR and quiescence (B.3, B.4); packet §4.
- Credible competing hypotheses: the oracle column seeds are wrong for this
  evaluation (P1 below −40); the cluster lands but the residual lies in
  B.3/B.4 (P2 fails with P3 held).

#### PRE-REGISTERED PREDICTION — freeze before exposure

Packet §11's B.2 table, verbatim, with P5 as Amendment 1 (§17.4) re-registered
it:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, §7 seeds) reads **−15 ± 30 Elo**; probability of the −40 defect stop 20% | moderate | below −40 is a defect or a wrong seed column |
| P2 | After B.2.3 and the `[0,10]` gates B.2 reads **+40 Elo at STC**, 90% interval [+5, +90]; H1 probability 65% | moderate | H0 or a stop under +10 refutes the coherent-core share; the residual then lies in B.3/B.4 |
| P3 | Nodes at depth 12 relative to the oracle fall from 3.97 to **≤ 2.5** (median ≤ 1.8) with branching inside [1.65, 1.90] | moderate-high | a ratio above 3.2 says the move-loop family did not land in the donor's population |
| P4 | WAC at 100k rises to **≥ 225** with razoring re-seeded and the LMR-off delta falls to **≤ +5** | moderate | WAC under 210 says the picker/LMR terms removed what razoring gave back |
| P5 | Pooled NPS lands at **0.92–0.98×** of the B.1 head (threat producer, larger histories **and quiet SEE calls**) | moderate | below 0.90 sends the threat producer to B.7 before the gate; reading rule (Amendment 1): B.2.2 reports `see_ge_calls` per node on both arms beside the pooled NPS; if P5 reads below 0.90 and the ON arm's `see_ge_calls/node` is more than twice the off arm's, quiet SEE is the suspect, the §17.1 fast path (if not taken) is the first repair, and only then does the cost go to B.7 with the threat producer |
| P6 | `lmr_researched / lmr_applied` rises from 1.7% to **3–5%**; `rfp_cuts` fall below 12% of interior nodes while the tree shrinks | high | neither moving means the floor or the estimated-score wiring did not land |

- Expected diagnostic movement: packet §10's screens at their registered
  floors and targets (branching window [1.65, 1.90]; nodes at depth 12 ≤ 2.5
  of the oracle; WAC at 100k ≥ 200 floor / ≥ 225 target, at 400k ≥ 240 /
  ≥ 258; LMR-off delta ≤ +12 / ≤ +5; agreement ≥ 60 / ≥ 75; time to depth
  ≤ 1.10× / ≤ 0.85×; 77 canaries). Also read, per Amendment 1 §17.4: BAS-C09's
  retry trigger once on the ON arm.
- Expected Elo sign/range: P1, −15 ± 30 unfitted.
- Probability candidate is positive/useful: P2's 65% for the fitted cluster.
- Confidence and basis: moderate; packet §2.6, BAS-S18 (G(32) = +102.3 ± 15.9).
- Most likely failure mode: a wrong seed column (P1 below −40) or a defect
  the fixed-node screens show (P3 above 3.2).
- Falsification criteria: the falsifier column above.
- Cheapest prior falsifier: the per-ticket fixed-node WAC readings (packet
  §20): 249 at 100k on the implemented head, above P4's 225; the ablation
  liveness proof (8 of 8 bits).
- Registered gate / stop rule: the 2,000-game paired run governs; > −40 Elo
  proceeds (below is a defect); ≥ +10 proceeds to B.2.3; −40..+10 the review
  decides after the family screen (packet §10). Programme stop rule: PLAN
  rule 6 and packet §11.
- Full conditions / provenance: Colosseum, `3+0.03`, 1T, Hash 64, UHO paired
  openings, no adjudication, tablebases off, 2,000 games (1,000 pairs), the
  A.3.1 run-file path; categorical switches (`CoreTtCutoffNodeTyped`,
  `CorePawnHistBucketsLog2`, `CoreCorrAdmission`, `CoreRazorDepthCap`,
  `CoreLmpExemptChecks`, `CoreLmrNegativeAllowed`) settled by 2,000-game
  paired runs on one tune build, never SPSA coordinates.

#### RESULT — append after exposure

- Diagnostics (not the verdict):
- Deciding result:
- Disposition: registered, not run.

#### PREDICTION CALIBRATION — preserve the original prediction above

- (appended at B.2.4, as packet §11 says)
