# Basilisk development plan

This is the forward roadmap. It says what will be done, in what order, why,
and what decides each step. It does not record history: completed work lives
in [HISTORY.md](HISTORY.md), measured evidence in
[EXPERIMENTS.md](EXPERIMENTS.md), procedures in [PROCESS.md](PROCESS.md),
engine invariants in [DESIGN.md](DESIGN.md), and a one-page overview,
whose step list is generated from this file, in [GUIDE.md](../GUIDE.md). The Phase-15 roadmap this replaces is
archived verbatim at
[docs/archive/PLAN-2026-09-28.md](archive/PLAN-2026-09-28.md), and the
Phases 5–14 roadmap before it at
[docs/archive/PLAN-2026-09-09.md](archive/PLAN-2026-09-09.md);
HISTORY's number map resolves every retired identifier.

Rewritten 2026-09-28 on the model of Rarog's roadmap. Rarog rebuilt its
search as a donor-shaped architecture, cluster by cluster, and moved from
3001 to 3233 in the maintainer's pool in the fifteen days from its B.0
investigation (2026-09-13) to RAR-M63 (2026-09-28). This plan adopts that
method and its order of steps, with Stockfish as the donor. Phases are
lettered (A–G) so that no new identifier collides with a retired number, and
a step that is the same step as Rarog's carries the same identifier: Basilisk
B.3 is the analogue of Rarog B.3.

**The plan does not depend on the Rarog repository.** Everything it uses from
Rarog is in a pinned, verbatim snapshot at `docs/reference/rarog/` (Rarog
`015bccae`; [docs/reference/README.md](reference/README.md) says how to
use it): the documents, the evidence cited as `RAR-*`, the tools to port and
the worked examples. The evidence the plan relies on is also imported into
Basilisk's own ledger (BAS-X30–BAS-X34). "The snapshot" below means that
directory.

## 1. Objective and gates

**Objective.** Make Basilisk the strongest engine we can build, in two
stages (maintainer decision 2026-09-28: Rarog's targets):

1. **Classical stage.** With the hand-crafted evaluation, beat the strongest
   HCE-era engines in the maintainer's own pool: **Critter 1.6a, Houdini 3,
   Rybka 4.1 and Fritz 16**.
2. **NNUE stage.** Train networks on Basilisk's own data only, then reach the
   **CCRL top 100**, and later the top 50.

**The classical target gate (E.2).** Colosseum rating tournament, the
maintainer's fixed pool with Houdini 3 added, `3+0.03`, UHO book, no
adjudication, at least 400 games per pair, measured **both at 1 thread and at
4 threads**. The gate is met when Basilisk's head-to-head score against each
of the four named engines is at or above 50% at 1T and at 4T, with the 95%
interval of the pooled four-engine score excluding a loss. Ratings inside
that pool are relative; CCRL numbers do not transfer to this control.

**The NNUE target gate (F.10).** A CCRL 40/15 or Blitz list rating inside the
top 100, established by CCRL's own testing after a public release.

### Where we start (measured)

| Fact | Value | Source |
|---|---|---|
| Release | **1.10.1** (2026-09-27), bench-identical to 1.10.0: `bench 13` **14,978,465**, CTest 12/12 release and sanitizer. 1.10.0 was accepted at **+19.18 ± 6.76 Elo** over 1.9.3 at `3+0.03` 1T | BAS-E55; HISTORY |
| Pool, `3+0.03` 1T | On the Super Rating Tournament's scale (42 engines, 200 games per pair, no adjudication, 2026-09-15), 1.10.0 rates **2994**, which 1.10.1 carries, against Houdini 3 3287, Critter 1.6a 3192, Fritz 16 3173 and Rybka 4.1 3111: rating gaps of **−293, −198, −179 and −117**. Direct 1.10.0 records are **15-26-159, 25-35-140, 25-45-130 and 44-38-118** respectively, 200 games per pair. Rarog's twelve-engine pool of 2026-09-11 (600 games per pair) read the same order: 1.10.0 at 3012 against Houdini 3 3277, Critter 1.6a 3197, Fritz 16 3165 and Rybka 4 3102 | BAS-X34; BAS-M08 |
| Pool, 4T | 1.10.1 performs at **3040** [3026, 3054] at 4T against the same four targets and Rarog 2.4.0, priced at their 1T ratings (400 games each, no adjudication): Houdini 3 **−203**, Critter 1.6a **−129**, Fritz 16 **−177**, Rybka 4.1 **−56**, Rarog 2.4.0 **+14**. Rarog 2.5.0 scores **+312** against it. Played unpinned: a four-thread read now is sibling-free, one logical CPU per physical core (BAS-X41) | BAS-M11; BAS-M12 |
| Sibling | Rarog 2.5.0-dev, after its search clusters 1 and 2, rates **3233** and scores **+200** against Basilisk 1.10.1 | BAS-X34 |
| Search deficit | On 1.10.1, the same search driving Basilisk's 1.10.1 HCE beats native 1.10.1 by **+312.6 ± 17.8** (paired, 3,000 games, equal time, no adjudication; BAS-O05). Earlier, classical Stockfish `9587eeeb`'s search driving Basilisk's own 1.9.3 HCE beat native 1.9.3 by **+322.7 ± 36**, while searching fewer nodes per move at lower NPS (round robin, 400 games per pair, logistic estimate, no adjudication). At equal time the oracle completes 25.2 plies at EBF 1.61 against Basilisk's 15.6 at 2.20; at 300k nodes 32.88 plies against 21.47, **98.4%** of that width attributable to search | BAS-O05, BAS-O01, BAS-O03, BAS-O04 |
| Evaluation deficit | Classical Stockfish's HCE beats Basilisk's by **+232.8 ± 32** under the identical search; full classical Stockfish beat 1.9.3 by +516 | BAS-O02 |
| How the search was attacked before | One mechanism at a time against a tuned optimum: reduction magnitude (BAS-S13–S15, all refuted before games), check-move depth (BAS-S16, −3.48 ± 3.32), check-extension removal (BAS-S08, −10.17 ± 6.52) | EXPERIMENTS §3 |
| Speed | Basilisk's own pooled-PGO baseline for 1.10.1 is **4.127M NPS** at `bench 13` on one pinned 5950X core (four builds; self pair +0.11%). Board microbenchmarks 22–46% ahead of Rarog's (BAS-X16); **3.71 MNPS** against Rarog's 3.19 in Rarog's own pooled-PGO measurement | BAS-P12; BAS-X16; BAS-X34 |
| Conversion | After a persistent piece-up advantage against six HCE-era anchors, 1.10.0 records **16.67 draws and 6.67 losses per 1,000 games** (20 and 8 in 1,200); the imported 1.9.3 result was 17.5 / 5.0 and Rarog 2.4.0 24.2 / 3.3 | BAS-M08; BAS-X34 |
| Endgame truth | **361/480** clean tablebase wins converted at 60k nodes, against the Stockfish reference's 466/480 | BAS-E47 |

The evaluation deficit is older than 1.10.x and comes from a coarser estimator
than a paired run; the search deficit was re-measured on 1.10.1 at A.7.3.

### Where we stand (interim read, 2026-11-09)

The table above stays the starting point. An unregistered maintainer round
robin (BAS-M15: twelve engines, 800 games per pair, `3+0.03` 1T, UHO, no
adjudication) played the head after Clusters 1 to 3, with B.5 to B.8 still to
come. In the pool it stands **+314.2** [+305.1, +323.2] over 1.10.2, **+74.6**
[+67.2, +83.3] over Rarog 2.5.0, Rarog's release at the end of its Phase B, and
**−80.6** [−89.1, −72.7] under Rarog 2.6.0, the release after its evaluation
programme. Critter 1.6a, the one E.2 target in the field, scored 29.94%. That
gain already exceeds B's planned band below; B.11.1 corrects the table, and E.2
is unread until its own field plays, at 1T and 4T.

Both halves of the engine have room of the same order. **The search is
attacked first**, for four reasons:

- it is the larger measured deficit;
- a stronger search produces better self-play labels for every later refit;
- it is where Basilisk has already shown that increments to the co-adapted
  optimum measure zero or less (BAS-S13–S16);
- Rarog escaped the same trap by adopting a donor's search as a coherent
  architecture, seeded, fitted and gated in clusters. Its first cluster read
  +65 Elo unfitted and +138 fitted, its second +50.5 (RAR-S73, RAR-S84,
  imported as BAS-X30).

The evaluation is frozen until the search checkpoint (B.11.1). The evaluation
programme then does the same for the evaluation families with the search
frozen, and a joint re-fit closes the classical stage.

This order contradicts the hypothesis Phase 5 closed on: that the tree is wide
*because* the evaluator is weak, so width is a symptom (EXPERIMENTS, cluster
5.4 summary). B.0 tests that premise rather than assuming it away. The
counter-evidence on record is Rarog's: its HCE measured about 96 Elo weaker
than Basilisk's on the same oracle instrument (BAS-O02, cross-run), yet its
donor-shaped fitted search gained about 230 pool Elo.

### Elo budget, stated so it can be wrong

| Programme | Measured deficit | Planned recovery | Basis |
|---|---:|---:|---|
| B search | 322.7 (1.9.3; re-measured at A.7.3) | 150–250 | Rarog's clusters 1 and 2 recovered about 230 pool Elo against a 248 equal-time deficit; Basilisk's deficit is larger and its estimator coarser |
| C evaluation | 232.8 | 60–120 | BAS-E07 says the gap is values rather than missing terms; Rarog's refits paid +22.0, +6.7 and +11.8 (BAS-X23); Basilisk's HCE is already the stronger of the two |
| D clock, SMP, robustness | unmeasured | 15–40 | Stockfish-shaped time management; 4T quality |
| Speed inside B and C | — | 10–30 | Per-node cost of the new search and evaluation modules |

If those bands are right, the classical head closes the rating gaps to
Rybka 4.1 (117), Fritz 16 (179) and Critter 1.6a (198) at 1T after B, and
Houdini 3 (293) falls, if at all, only with C. Each
programme's checkpoint re-measures its deficit meter, so a miss is seen as a
miss and this table is corrected rather than defended.

## 2. Operating rules

`AGENTS.md` and its `agents/` task files are authoritative for how agents work and `DESIGN.md` for what must
stay true of the engine. The rules below decide order and acceptance in this
roadmap.

1. **Donor architecture, own implementation.** The donors, by maintainer
   decision 2026-09-28:
   - **modern Stockfish** is the architecture donor for search, TT,
     histories, move ordering, time management, threads and later the NNUE
     runtime (the role Reckless plays for Rarog);
   - **classical Stockfish `9587eeeb`** (2020-07-30, the last classical
     master) is the donor for the evaluation families, the deficit oracle
     (tag `oracle/hybrid`), and a seed column for eval-coupled margins;
   - **Triumviratus** (maintainer decision 2026-11-03) is the third donor,
     for mechanisms only: a GPL-3 engine first on CCRL 40/15 (2026-10-30)
     whose search was rebuilt on Stockfish 19's base and whose every change
     is measured and recorded. It gives measured priors for that search,
     board and infrastructure mechanisms, time-management rules, NNUE
     architecture and training lessons, and the failure modes it found in
     its own measurement. Its verdicts are priors, never acceptance; it is
     read through the pinned snapshots `docs/reference/triumviratus/` and
     `docs/reference/triumviratus-networks/`, and
     `analysis/triumviratus_review_2026-11-03.md` maps each finding to its
     leaf.

   PROCESS's *Independence boundary* says what may cross and how the code is
   written. Similarity to a donor is never an acceptance criterion; games
   are.
2. **Constants are seeds.** A ported constant sits on the donor's score scale
   and node population. It is converted through the measured scale ratio
   (B.0), seeded by B.0's three-column rule, fitted by SPSA over the cluster's
   live coordinates, and only then gated. The three columns are the modern
   donor's value converted, the classical oracle's value converted, and
   Basilisk's own fitted value where one exists. Search and evaluation
   coordinates never share a tune; Texel fits linear evaluation terms, and
   SPSA prices only what Texel cannot.
3. **Clusters, not features.** The unit of implementation and of strength
   acceptance is one dependency-complete, co-adapted cluster: every mechanism
   that consumes or produces a shared signal moves together. Internal
   sub-steps are compiled and diagnosed separately but are not expected to
   win standalone and do not get their own gates.
4. **Compatibility over completeness.** Before adding a mechanism, audit the
   ones it will feed or be fed by. Re-implement an existing feature when the
   donor's form composes better with its neighbours; keep ours when the
   evidence says ours is better. Candidates on record: the TT-bound pruning
   evaluation (BAS-S01), the exact/PV history reward (BAS-S03),
   surprise-scaled history (BAS-S04) and root-instability time (BAS-R03).
   Ordering-to-pruning feedback, evaluation-to-margin coupling, TT masking
   and history gravity are the interactions to name in every handoff.
5. **Each cluster: audit, register, implement, prove, explain, fit, gate,
   record.** Registration in `EXPERIMENTS.md` comes before any game:
   hypothesis, baseline revision, bracket, cap, stop rule and the frozen
   prediction. Brackets:
   - `[0,3]` nElo is the default;
   - a large prior uses `[0,10]` and says why;
   - a removal or simplification uses `[-5,0]` (`sprt.ps1 -Mode simplify`);
   - an unknown-sign repair uses a symmetric bracket.

   Never change a gate after seeing games. Accepting H1 is a decision, not
   an effect size: report the estimate and interval beside the verdict.
6. **Two rejected clusters in one programme stop it** and force a new
   evidence audit before a third is built.
7. **Every strength A/B runs with adjudication off**, at `3+0.03`, 1T,
   Hash 64, paired UHO (`tools/books/UHO_Lichess_4852_v1.epd`), normalized
   Elo, pinned physical cores, 14 concurrent games (15 slots for a tune),
   tablebases off, unless the registration states otherwise and why.
   Multi-thread Colosseum gates allocate one shared physical core per engine
   thread in each non-ponder game, one logical CPU per physical core
   (sibling-free: an SMT sibling packs the threads, BAS-X41), and calibrate
   a null pair first.

   7b. **A cluster may be fitted before its gate, with an argument.** Rarog's
   first cluster read +65.09 ± 23.26 unfitted and +138.60 ± 30.66 fitted
   (RAR-S73), so donor constants can understate a mechanism by more than its
   whole margin. That is a reason to fit *some* clusters first, never a
   routine step: a fit costs tens of hours. A pre-gate tune is registered
   case by case with the argument, the horizon and what the gate would
   otherwise measure.

   7c. **A tune is one run, read at checkpoints, and stopped when its
   gradients are gone.**
   - *Shape.* 30 games an iteration on 15 slots, with a registered ceiling:
     6,000 iterations (180,000 games) unless the registration argues
     another. The perturbation is twice the generator's step schedule and
     `r_end` is 0.00155, half the earlier 0.0031: the move per result is
     unchanged and each gradient's signal doubles. A coordinate without
     two steps of room at its seed takes the largest perturbation from one
     to two steps that stays inside its range; one with under a step of
     room is named at registration (BAS-S57: 10 of the head's 74
     coordinates, 3 of them under a step).
   - *Checkpoints.* Iteration 2,000, then every 1,000. At each,
     `tools/spsa_read.py` reads the last 1,000 iterations.
   - *The test.* For each coordinate, `z = Σ δ·D / √(Σ D²)`: δ is its
     perturbation's sign at an iteration and D the plus side's wins less
     its losses. For a coordinate with no gradient z has mean 0 and
     variance 1 exactly, because the tuner drew the signs. The window is
     **live** if any |z| passes the familywise 5% line for the surface's
     coordinate count, or if `Σ z² − n` is at least `2·√(2n)`; otherwise
     it is **quiet**.
   - *The rule.* Quiet: the tune stops, and theta is that checkpoint's
     rounded centres. Live: the next 1,000 iterations may run. The
     maintainer may stop at any checkpoint, and theta is then that
     checkpoint's centres. A run stopped between checkpoints is not
     theta; it resumes.
   - *Rails.* A centre within one perturbation of a bound, its z pointing
     outward past the familywise line, stops the tune for a new
     registration of the range. A weaker outward push (z of 2 or more) is
     reported as a candidate and stops nothing (maintainer, 2026-11-12:
     the clause exists for a coordinate that cannot move keeping every
     window live, which is an outward z past that line).
   - *The record.* Each read appends every coordinate's z, the tune's own
     read of its gain, the stop's floor in Elo and a frozen forecast for
     the next 2,000 iterations. The forecast decides nothing until three
     scored forecasts have each fallen within twice the combined SD of the
     forecast and of the gain read it is scored against (the gain read is
     the noisier of the two, so the forecast's own SD alone would fail a
     perfect forecast more often than not).

   Quiet means no gradient above the stated floor, not convergence. A
   tune is judged by its gate, not by its read, and movement is never read
   by net travel or by a count of movers. The derivation and the evidence
   are `analysis/b56_spsa_method_2026-11-12.md` (B.5.6, maintainer's
   sign-off 2026-11-12). The rule replaces the blocks of Rarog rule 7c
   (RAR-S75, RAR-S78, RAR-S82–S83; BAS-X31) and keeps Rarog's later
   shape, twice the perturbation at half the ratio (RAR-S104, RAR-S105;
   BAS-X36). No tune is registered until B.5.8 has built the read and the
   wrapper.
8. **State the measurement layer.** Theory truth, move quality, conversion,
   fixed-node tree shape, NPS and game strength are different units with no
   exchange rate. Counters, node counts, EBF, tactical suites and fit loss
   explain or screen; only a registered final-PGO SPRT or the target gate
   accepts. Every endgame number carries its layer, position set and node
   budget. **Cluster screens** (Rarog's ladder, adopted before any cluster is
   built):
   - **The paired run governs.** The unfitted 2,000-game paired run against
     the accepted head is the only screen that clears a cluster for its fit.
     Zero-game floors are diagnostics. A floor failure triggers the ablation
     sweep and a written cause; it never holds a candidate the paired run
     cleared, and a floor passed never accepts one it failed.
   - **Ablation:** one sweep of the ablation-mask bits in mechanism order
     (A.5.4 fixes the bit order on Basilisk and on the oracle), each bit
     alone on the fixed-node screens; no other order and no second sweep.
   - **Speed:** time-to-depth on `bench 13` replaces a pooled-NPS floor,
     because a tree-shape change makes nodes incomparable. Each arm's time is
     its bench nodes over its pooled-PGO median NPS from interleaved runs;
     pooled NPS is reported beside it as a diagnostic.
   - **Fixed-node quality:** WAC solved at 100k and 400k nodes, plus a
     positional screen at 100k nodes. Until a Strategic Test Suite fixture
     is placed, that screen is oracle best-move agreement on
     `tools/diag/suite_v1.epd`.
   - **Canaries:** a regression rule. No oracle-anchored canary the baseline
     solves may be lost, where solving means stable at no more than the
     anchor plus two plies and solved at 100k nodes. New passes are recorded,
     not required. The deterministic CTest floors (KBNK, KQK, KRK and KBBK
     conversion, the mate band) are correctness, not canaries: they bind
     every arm, and a failure is a false premise returned to research, never
     a test to relax.
   - **Curvature sweep before any SPSA.** Five registered coordinates, each
     at 0.5×, 0.75×, 1×, 1.5× and 2×, with `bench 13` and WAC at 100k per
     point. The classification is frozen before the first point. Flat or
     monotone on all five skips the cluster's SPSA.
9. **Freeze the prediction before exposure, append the calibration after.** A
   miss is recorded as sign, magnitude, mechanism, interaction, confidence or
   instrument. `NO_CHANGE`, refuted and too-sparse are successful outcomes.
10. **Deficit meters are re-measured at every programme checkpoint:**
    - the equal-time G(0) against the classical-Stockfish search oracle (B);
    - the same-search gap against classical Stockfish's HCE (C);
    - the conversion instrument;
    - the endgame truth cohort;
    - pooled-PGO NPS;
    - the pool score against the four target engines.
11. **Engine, tooling and documentation changes are separate commits.** A
    PLAN change is committed with GUIDE regenerated by
    `python tools/diag/check_roadmap.py --write-guide`, and the checker
    passes. Completed IDs never change; open IDs may be renumbered with a map
    in HISTORY.
12. **Expensive jobs are the maintainer's:** SPRTs, SPSA, datagen,
    tournaments, PGO campaigns, long profiles. The agent prepares and
    verifies them and hands over one runnable command. **The host is shared
    with Rarog:** one pinned harness at a time, never two (both compute the
    same core list). Basilisk's jobs queue with Rarog's in one budget of
    about three SPRT-sized runs per day, with SPSA overnight.

### Workflow states and capability classes

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

`READY_FOR_IMPLEMENTATION` is the boundary at which the mechanism, semantics,
evidence, interactions, invariants, falsifier and accept/reject rule are
frozen. Implementation owns ordinary engineering inside that contract, and
returns a false premise to `RESEARCH` instead of rescuing it.

| Class | Required capability | Typical use here |
|---|---|---|
| `R3` | Frontier causal/architecture research | programme investigations (B.0, C.0, F.0), cluster research |
| `R2` | Bounded correctness-sensitive reasoning | audits with a known question, reviews, registrations, contract definition |
| `I2` | Difficult implementation | cluster implementation in search, evaluation, NNUE runtime |
| `I1` | Well-specified implementation | tooling, instruments, behaviour-neutral restructures from a written handoff |
| `M` | Mechanical documentation/provenance | ledger rows, archives, inventories, changelogs |
| `V` | Verification/measurement | qualification runs, gate preparation, sweeps, re-measurements |

GUIDE maps classes to models. Investigation leaves spawn the implementation
and measurement sub-steps under their own step. The sub-steps listed below
under an investigation are the expected shape, and the investigation's
handoff confirms, splits or replaces them.

### The research–implementation loop

This is the working method the roadmap is built around. It is what carried
Rarog's search programme, and the capability classes exist to route it: the
frontier model researches, the mid-tier model implements, a separate session
reviews.

1. **Investigate (`R3`).** A programme or cluster opens with an investigation
   that measures before it designs. It covers:
   - the local defect and its population;
   - the donor's mechanism, read for population and interaction (read it,
     close the file, design from Basilisk's own code);
   - competing explanations and the interaction map;
   - the seeds and their scale;
   - the screens, with registered numbers, and frozen predictions.

   The deliverable is an `analysis/` packet that ends
   `READY_FOR_IMPLEMENTATION` handoffs or `NO_CHANGE`. No engine source
   changes during an investigation.
2. **Prepare (`R2`).** The EXPERIMENTS registration, the tooling the handoff
   needs, and an architecture review of the code the cluster will land in.
3. **Implement (`I2`, `I1`).** The cluster lands as ordered tickets behind one
   umbrella build switch; with the switch off, every ticket keeps the
   accepted fingerprint exactly. Ordinary engineering belongs to the
   implementer; the mechanism, semantics and experiment do not.
4. **Return on a false premise.** When a premise of the handoff fails in
   implementation, the implementer:
   - stops the mechanism and keeps the instrumentation;
   - records the contradiction, its evidence and the options in the packet;
   - returns the leaf to `RESEARCH`.

   The researcher answers with a numbered research amendment in the same
   packet, and implementation resumes on the amended contract. Rarog's
   cluster 2 returned to research five times and reached its gate through
   research amendment 7, then passed at +50.5 ± 10.9. The returns are the
   method working, not failing.
5. **Review (`R2`, a separate session).** A reviewer who did not implement
   reproduces the fingerprints and tests and checks the code against the
   handoff. Acceptance is recorded before diagnostics start.
6. **Measure (`V`) and gate (maintainer).** Screens, the paired run, sweeps
   and SPSA runs are prepared and verified by the agent and run by the
   maintainer; the registered gate decides.
7. **Calibrate.** The result is appended to the frozen prediction, and the
   next cluster's research starts from what the last one got wrong.

### Standing contracts

Live invariants that every change must keep. This table is the index; the
derivations are in the linked records.

| Contract | Where it is written down |
|---|---|
| Score perspective, mate band (`MATE_SCORE − MAX_PLY` = 31,872, duplicated in `search.h` and `tt.h`), endgame scale floor, lazy-eval path | `DESIGN.md` §3; `tests/test_endgames.cpp` static assert |
| TT publication incoherence is a known defect, tolerated until D.2 repairs it without an NPS cost; every consumer validates the stored move | `DESIGN.md` §3; BAS-C05 |
| SEE king legality, created pins and promotion recaptures repaired, the last two in B.4.2 before quiescence's fit; a check on the exchange stays unmodelled | BAS-C08, BAS-C09; `tests/test_board.cpp` |
| Draw, rule-50, null-move clock and legal-EP hashing semantics | BAS-C02; `tests/test_invariants.cpp` |
| One mutation authority for redundant board state | BAS-C07; `src/board.h` |
| Protocol: `ponderhit` owned by the UCI thread; setup work done before `readyok`, never on the clock; a rejected `position` exits with status 1; tablebase PV lines time-boxed the Stockfish way | BAS-C10–BAS-C13; `tests/test_engine_ponder.cpp`, `tests/test_uci_protocol.cpp` |
| Search parameters have one source (the X-macro table) | `src/search_params.h` |
| Endgame measurement layers (truth, move quality, conversion, strength) | `analysis/endgame_measurement_layers_v1.md`; `DESIGN.md` §4 |
| Behaviour-neutral change = exact fingerprint plus targeted checks plus pooled NPS | `AGENTS.md` |
| Diagnostic counters: one table, each counter with its unit and denominator; names and units are the cross-engine contract, extended at the end and never renamed | `src/search_diagnostics.h`; `tools/diag/run_suite.py` |
| Cross-engine board timings are compared only on the `cross-engine-board-v1` profile and its five-position corpus | `tests/board_performance.cpp` |

### Known defects

A defect is a mechanism that computes something other than what it claims:
a wrong value, a wrong rule, a check that cannot fail. Each is fixed in its
owner leaf and none is kept: a correct repair that loses Elo is reworked,
with the neighbours that were tuned around the defect refitted, rather
than reverted to the wrong form (maintainer decision 2026-10-26). A gap (a
missing mechanism, a coupling Basilisk shares with Stockfish, an unfitted
coordinate) is not a defect; its owner decides. IDs are the Rarog audit's
(`analysis/rarog_audit_2026-10-24.md`) unless named; F-numbers are §3 of
`analysis/rarog_import_2026-11-11.md`. A defect whose owner runs after B.11 is
read once in B.9.4, before the release: repaired there, or kept with its
reason.

| Owner | Defects |
|---|---|
| B.4 | B-17 the dead SEE clamp |
| B.5 (with D.1) | the fail-high stop's hole: a window that never returns is never checked (A-47; `analysis/b5_failhigh_stop_2026-10-25.md` §2.3). The ply cap's static evaluation in check (NODE-8) was repaired in B.5.2 |
| B.6 | F4 the multi-cut correction bonus's product could pass 32 bits and then trained the wrong way (confirmed by B.6.4, `analysis/b64_engine_audit_2026-11-12.md` §3.1; repaired in B.6.5). The audit confirmed no other defect in the search |
| B.9 | `go depth N` answered by a deeper helper (DRIVER-12); Fathom's `exit` on a table that fails to map (PROTO-11); A-50 a stop inside the first iteration; A-57 cold table reads on the clock; A-58 the tablebase root ranking ignores the game's repetitions; F1 `stop` and `quit` unseen during `bench` and `wac`; the clock's game ply at `ponderhit` read from the node the search is at (B.6.4, `analysis/b64_engine_audit_2026-11-12.md` §3.2); F6 bishops on one colour against a bare king scored as a win; each hard verdict B.9.3's read finds wrong |
| C.2.1–C.2.2 | C-1 the tuner's missing scalers; C-2 `--verify` cannot fail; C-8 the extractor's de-dup ignores the clock; C-26 to C-30 the data pipeline (book, sampler, split, labels, row identity); C-49 K refitted per run |
| C.3, C.5, C.6 | C-10 outpost ranks; C-11 a rear doubled passer scored as a passer; C-12 "opposed" read over the whole file; C-13 own-colour king blockers only |
| D.1 | A-46 reserve arithmetic |
| D.3 | A-59 the tablebase root's DTZ ranking |
| D.2 | BAS-C05 the TT key and payload published separately |

## 3. Required evidence

| Change | Minimum gate |
|---|---|
| Tool, docs or refactor | Syntax/static checks; focused tests; full CTest and bench when execution semantics can change |
| Behaviour-neutral hot path | Exact immediate fingerprint, targeted parity, pooled/interleaved PGO NPS on an idle host |
| Correctness repair | An independent invariant that fails on the old behaviour; a strength gate when deployed play changes materially |
| Search cluster | Umbrella-off exact fingerprint at every commit; release and sanitizer CTest on both arms; the rule-8 screen ladder; the unfitted 2,000-game paired run; a registered SPRT on the fitted final-PGO build |
| Evaluation cluster | `EvalTrace` coverage and reconstruction; a whole-surface refit on a frozen corpus with an untouched test split; clean-PGO SPRT; endgame truth vetoes |
| Endgame evaluator | Deterministic truth cohort, paired WDL/DTZ, conversion floors, promotion/material-shed closure, an occurrence-tiered game gate |
| Time, root, SMP | 1T STC, 1T `10+0.1`, 4T `10+0.1`, zero forfeits, sibling-free placement, topology and hash recorded |
| SPSA | Curvature sweep; registered surface, run shape, checkpoints and estimator; perturbation proof on every coordinate; fresh PGO bake; independent SPRT |
| Release | Reproducible PGO assets, correctness matrix, prior-release STC, LTC and 4T games |

## Phase A — Reset: documents, harness, instruments, baselines (EXTENDED)

**Goal:** put the documents, harness and instruments of the method in place
on the released 1.10.1 head, and measure the starting point with them. No
engine behaviour changes in A.1–A.7; A.8, added 2026-10-06, carries repairs
that do. A.9–A.12, added 2026-10-24 from the Rarog audit
(`analysis/rarog_audit_2026-10-24.md`), carry the instruments, delivery
fixes, correctness repairs and rules no other phase owns. They were added
after B.2 and B.10 closed, so the phase is marked `(EXTENDED)`. They run in
order, and all of Phase A closes before B.3 opens (maintainer decision
2026-10-24, which also renumbered them; HISTORY's *Number map* resolves the
first numbering). A.13–A.15, added 2026-10-27 from the code audit of the
engine's crucial paths (`analysis/code_audit_2026-10-27.md`), follow them
in the same order and under the same rule, as do A.16 (2026-10-28) and A.17
(2026-10-29, at the maintainer's request after A.15). Each step is reviewed with the maintainer when it is
reached; its resolution may be a fix, an improvement or a skip. Rarog's
Phase A also cut a release; 1.10.1 was cut the day before this plan, so
Phase A has no release step.

**Closed 2026-10-30** with A.17, its last leaf; B.3 is next.

A.1–A.7 closed between 2026-09-28 and 2026-10-06; their text is in
[`docs/archive/PLAN-closed-2026-10-07.md`](archive/PLAN-closed-2026-10-07.md).
They reset the documents (A.1); cleaned the repository, branches and tags and
inventoried features, options and parameters (A.2); made the Colosseum CLI
the main harness with its SPSA path (A.3); refreshed and froze the toolchain
with build flavours for arms (A.4); built the search programme's
instruments: fixed-budget probe, reference-anchored branching profile,
counter summation and decision trace, matched ablation mask, PGN conversion
instrument and canaries (A.5); mapped the codebase consolidation (A.6); and
measured the 1.10.1 baselines (A.7): the 1T pool census (BAS-M08), 4T
performance **3040** [3026, 3054] (BAS-M11), G(0) **+312.6 ± 17.8**
(BAS-O05) and pooled-PGO NPS **4.127M** (BAS-P12).

A.8–A.17 closed between 2026-10-07 and 2026-10-30; their text is in
[`docs/archive/PLAN-closed-2026-10-30.md`](archive/PLAN-closed-2026-10-30.md).
They brought Basilisk to Rarog's parity: the last Rarog import, CI on pull
requests, the tag-driven release, UCI and MultiPV conformance, tablebase
score bands and probes, the won-endgame time sink and the 1.10.2 patch
release (A.8); rebuilt the harness and the instruments, among them the
pooled-build NPS read and the inline-attributing profiler (A.9); repaired
delivery and the repository (A.10); fixed the correctness defects of the
Rarog audit (A.11) and of the code audit (A.13), with the roadmap rules
(A.12) and one simplification (A.14) the audits asked for; shipped one
universal x86-64 binary per OS, four PGO tiers behind a CPU dispatcher,
buildable with Clang or GCC by Stockfish's assembly route, every trap
guarded (A.15); deleted the evaluation's zero-weight terms and their
parameters, **+2.35%** NPS (BAS-P25, A.16); and found no instruction-specific code that
makes the engine faster, the native build's 1% loss being Zen 3's tuning
model (BAS-P26, A.17).

## Phase B — Search programme (evaluation frozen)

**Goal:** recover the measured search deficit, with the evaluation frozen at
the 1.10.1 HCE, by rebuilding Basilisk's search as a Stockfish-shaped
architecture implemented in Basilisk's own code, seeded, fitted and gated
cluster by cluster.

**Why this shape.** Basilisk's width is a search-policy property: 98.4% of
the 11.4-ply gap at 300k nodes follows the search, not the evaluator
(BAS-O04). Every one-mechanism attempt to narrow it measured worse or not at
all (BAS-S13–S16). Basilisk already carries most of the names:
- PVS, and a TT with Stockfish's own 10-byte entry and 3-entry cluster;
- correction histories keyed by pawn, minor and non-pawn material;
- continuation, capture, pawn and low-ply histories, and killers;
- singular extension with a double extension, ProbCut, razoring, NMP, IIR,
  LMP and futility.

What differs is the population each mechanism admits and how the signals that
gate them are produced and combined. Rarog's donor-shaped clusters succeeded
exactly where its own increments had failed.

**What "Stockfish-shaped" means, concretely.** The source of record was
pinned by B.0 (2026-10-06) at the `sf_19` release tag `edb0d9db`
(2026-09-05); the `0a215d6c` default is superseded because the release tag
is reproducible and the reference binary B.0 measured against is built from
it. The files are `src/search.cpp`, `movepick.cpp`, `history.h`, `tt.cpp`,
`timeman.cpp` and `thread.cpp`. Read them; do not copy them.

- **Node and stack.** Node types are template parameters (Root, PV, NonPV).
  The per-ply stack holds the static eval, move count, in-check, tt-pv,
  tt-hit, cutoff count, the applied reduction and `statScore`. It also points
  to the continuation-history and continuation-correction sub-tables selected
  by the move just made.
- **Transposition table.** On a probe miss, the raw static eval is stored.
  The TT value replaces the eval when its bound allows. At depth ≥ 7 a TT
  cutoff is checked one ply down against graph-history inconsistency, and
  there are no TT cutoffs above rule-50 count 96.
- **Correction.** Pawn, minor-piece and per-colour non-pawn keyed histories,
  plus continuation correction at plies 2, 4 and 6, combine into one
  correction value. It adjusts the raw eval, and its magnitude feeds margins,
  extensions and reductions. Training happens at node exit when the error
  sign agrees with the bound, and at a multi-cut.
- **Histories.**
  - The tables: butterfly main history by colour; low-ply history for the
    first five plies; capture history by piece, to-square and captured type;
    pawn-structure history by pawn key; continuation history at plies 1–6
    keyed by in-check and capture, with per-ply weights and a
    positive-consistency multiplier; a TT-move history.
  - Update rules: bonus and malus linear in depth with caps, the malus
    decaying across the searched moves.
  - Training signals: a quiet TT-move cutoff trains the histories; the
    parent's quiet move that caused a fail low is rewarded; the static-eval
    swing trains the parent's history.
  - No killers and no countermove table.
- **Move picker.**
  - Captures are scored by captured value plus capture history and split
    into good and bad by a SEE threshold derived from the score.
  - Quiets are scored from main, pawn, continuation and low-ply history plus
    checking-square and threat-escape terms, and split into good and bad at
    a fixed threshold.
  - The stages are TT move, good captures, good quiets, bad captures, bad
    quiets; evasion, ProbCut and qsearch have their own stage sets.
- **Node-level selectivity.**
  - A hindsight depth adjustment from the parent's reduction and the eval
    swing.
  - Razoring at all-nodes, dropping into qsearch.
  - Reverse futility with improving, opponent-worsening and correction
    terms, returning an interpolated value.
  - NMP at cut nodes with a prior-NMP-fail-high term, a dynamic reduction
    and a verification region (`nmpMinPly`) from depth 16.
  - IIR for PV and cut nodes without a TT move.
  - ProbCut with qsearch pre-verification and a margin-scaled depth, plus a
    small TT-based ProbCut.
- **Move loop.**
  - Pruning: move-count pruning; capture futility with capture history; SEE
    pruning with depth-scaled margins; continuation-history pruning; quiet
    futility on a history-adjusted `lmrDepth`.
  - Singular extensions with double and triple margins, multi-cut returning
    the soft bound, a negative extension and a shuffling guard.
  - LMR in 1024ths, with terms for tt-pv, cut and all nodes, TT capture,
    cutoff count, TT move, `statScore`, correction magnitude, the alpha−eval
    gap and move count. The reduced depth is floored at one ply and may
    extend up to two plies near the root. A fail high gets a deeper or
    shallower re-search and a post-LMR continuation update.
  - When LMR is skipped: a full-depth search, stepped down when the
    reduction is high.
  - PVS on PV nodes, a depth reduction after alpha rises, and fail-high
    score interpolation.
- **Quiescence.** TT probe and cutoff; corrected stand-pat with an
  interpolated fail-high; a futility base with move-count and SEE pruning;
  evasions only in check; a TT write on exit.
- **Root and time.**
  - Aspiration windows centred on the root move's average score, with the
    width taken from its mean squared score; optimism derived from the
    average.
  - Root-move node effort, and draw scores randomised by node count.
  - Optimum and maximum times scaled by falling eval, best-move stability,
    best-move instability across threads and best-move node effort.
- **Threads.** Lazy SMP, with correction, pawn and continuation histories
  shared between threads on a NUMA node, and per-thread main, low-ply and
  capture histories.

**Scale conversion (B.0).** Modern Stockfish's margins sit on its network's
scale and were sized for an evaluator whose error is far smaller than an
HCE's. Rarog found that converting Reckless's margins by the mean-|eval| ratio
alone would fire razoring and reverse futility at nearly every node, because
its HCE's search-minus-static residual was a third of its mean absolute eval.
It seeded from three columns instead (BAS-X33). Basilisk measures three
things: its own ratio to modern Stockfish's search-facing eval, its ratio to
the classical oracle's, and its own residual. The classical oracle's margins
were co-adapted with an HCE and proven to play +322.7 Elo better with
Basilisk's own evaluation, which is why the oracle is a seed column and not
only a meter.

**Gating shape for B.** Each cluster goes through six stages:
1. The umbrella-off arm stays at the exact fingerprint at every commit. The
   on arm's fingerprint moves, so no neutrality claim is made for it.
2. Fixed-node diagnostics against the oracle at stride 1, registered as
   explanation only.
3. A 2,000-game paired run at seeds converted but unfitted, to detect a
   broken port early. Worse than −40 Elo means a defect, not a tuning need.
4. The curvature sweep, then SPSA (rule 7c) over the cluster's live
   coordinates.
5. The SPRT, `[0,10]` for the selectivity core and `[0,3]` for the smaller
   clusters, sized from the drift model at the expected value.
6. The ledger row with its calibration.

A rejection returns the cluster to `RESEARCH` with its diagnostics; two
rejections stop B.

B.0–B.2 and B.10 closed between 2026-10-06 and 2026-10-24; their text is
in [`docs/archive/PLAN-closed-2026-10-30.md`](archive/PLAN-closed-2026-10-30.md).
B.0 pinned the donor (`sf_19`, `edb0d9db`), measured the deficit and fixed
the cluster order: razoring's depth reach read +2.4 Elo [−7.5, +12.3]
(B.0.1), and the oracle's move-loop pruning family is about two thirds of
its lead, G(0) − G(32) = 210.3 ± 23.9 of 312.6 (B.0.2). B.1 restructured the
search behaviour-neutrally, +1.21% NPS (BAS-P16). B.2, Cluster 1, rebuilt
the selectivity core in the donor's shape behind `B2_CORE` and fitted it:
+136.2 ± 33.6 Elo unfitted and +58.7 ± 20.9 fitted, about +195 over the
B.1 head (BAS-S23); the legacy kernel and the switch were later deleted
(B.8.1). B.10 repaired seven rank-mirrored piece-square
tables, +18.3 ± 12.9 Elo (BAS-S26).

- [x] **B.3** Cluster 2 — pruning and extensions
  Cluster 2 — proof searches and extensions: NMP (cut-node population,
  improving and prior-fail-high margin, dynamic reduction, verification
  region), ProbCut (qsearch pre-verification, margin-scaled depth, the small
  TT ProbCut), singular extensions (double and triple margins, multi-cut,
  negative extension, shuffling guard), IIR and Basilisk's check extension;
  SPRT `[0,3]`.
  Delivery checklist (prepared 2026-11-04 at the maintainer's request).
  B.12, B.14, B.3.0 and Cluster 2 reach `master` together as one squash PR
  from `b3-pruning-extensions` (maintainer decision 2026-11-01), once B.3's
  last leaf closes. The branch carries the Hash bound by physical memory
  as well (`1468936`, maintainer 2026-11-04). In order:
  1. On BAS-S43 (a) H1, one engine commit flips `B3_PROOF`'s CMake
     default to ON.
     - The fingerprint record moves to the ON arm's 5,369,042: GUIDE's
       *Now*, AGENTS' restatement and DESIGN, which `check_roadmap.py`
       cross-checks.
     - CHANGELOG gets an entry for the new pruning with (a)'s result.
     - The OFF arm stays compilable until the cleanup leaf.
     - On H0, BAS-S43's reject clause applies instead.
  2. Qualify the Hash bound, which is still syntax-checked only. Since
     2026-11-04 (maintainer) every `setoption` that sends a refused Hash,
     above the machine's limit or refused by the platform, is announced,
     and no other command repeats the notice:
     - release CTest on both arms, including `uci_hash_above_machine_limit`
       and `test_engine_ponder`'s refused-Hash section; the process test's
       script was checked against stub engines (a correct one passes; a
       silent one, a once-only one and one that repeats on other commands
       fail);
     - `bench 13` unchanged on both arms;
     - the sanitizer build's `test_tt` and `test_uci_protocol`.
     macOS and Linux are first compiled by the PR's CI.
  3. Final qualification on the delivery head, on both arms:
     - release and sanitizer CTest, and `bench 13` (OFF 11,880,564; ON the
       new record);
     - `python tools/diag/check_roadmap.py`;
     - `python tools/diag/check_citations.py`, which needs the PR refs;
     - `python tools/generate_spsa_surface.py --check`;
     - `python -m pytest tools/diag/test_profile_regions.py
       tools/diag/test_spsa_surface.py`.
  4. Merge `master` into the branch if it moved; it had not on 2026-11-04.
  5. Open the PR (the maintainer's command). The squash message follows
     PROCESS *Delivery* step 3:
     - a short imperative title (draft: "Rebuild pruning and extensions");
     - a few lines on what and why: Cluster 2's proof searches and
       extensions in Stockfish's form, fitted; the false win in drawn
       KBP-K; draw verdicts that take no correction; histories indexed
       by coloured piece; Hash bounded by physical memory;
     - the results, one per line: B.12 `[-5,5]` +3.8 ± 5.2 Elo (2,567
       pairs); B.14 `[-5,5]` +7.4 ± 6.9 Elo (1,271 pairs); B.3.5 (a) and
       (b) at their stops; (c) at `10+0.1`; B.3.6 `[0,3]` +4.7 ± 3.0 Elo
       (9,618 pairs);
     - after an empty line, `Bench: <the ON record>`.
  6. After the squash merge, delete the branch locally and on `origin`.
     The branch's commits stay citable under the PR's ref.
  - Progress 2026-11-04, after B.3.5's gates:
    - Step 1 is done: the B3_PROOF default is ON, the fingerprint record
      is 5,369,042, and the CHANGELOG has the entry. CI's extra arm job
      now builds and tests the OFF arm.
    - Step 2 is done:
      - release CTest passes 26/26 on the default, ON and OFF builds,
        `uci_hash_above_machine_limit` included;
      - the Tune builds' Hash and search tests pass;
      - the sanitizer builds' `test_tt`, `test_uci_protocol`,
        `test_engine_ponder` and the process test pass;
      - the allocator-refusal notice now reads "Unable to allocate Hash
        value N; keeping M MiB.";
      - `bench` is unchanged by the Hash work on every build.
    - Step 3's checks pass on this head: the SPSA-surface check, the tool
      tests, `check_roadmap` with its self-test, and `check_citations`.
      It is run again on the delivery head after B.3.6.
    - Steps 4-6 wait for B.3.6, B.3's last leaf.
  - Progress 2026-11-04, after B.3.6's gate (BAS-S46 H1, +4.7 ± 3.0
    Elo): `COLOUR_HISTORY` defaults ON, and the fingerprint record is
    5,361,058. CI's pre-Cluster-2 job builds `B3_PROOF=OFF` with
    `COLOUR_HISTORY=OFF`, at 11,880,564.
    - Step 3 passes on this head. `bench 13` is 5,361,058 on the default
      Release, Tune, Diag and Ablate builds, and 11,880,564 on the
      pre-Cluster-2 arm. CTest passes 26/26 on both arms, Release and
      sanitizer. `check_roadmap`, `check_citations`, the SPSA-surface
      check and the tool tests pass.
    - Steps 4-6 are done. `master` had not moved. PR #21's CI first
      failed the universal release build, because the physical-memory
      query has no bench profile. It was exempted beside the other
      unprofiled files (`6f76a99`). A macOS cell that never got a runner
      was re-run. Squash-merged 2026-11-04 as `d26f92b`, whose tree is
      the branch head's. The branch is deleted locally and on `origin`;
      its commits stay under `refs/pull/21/head`.
  Opens in the standard cluster shape: B.3.0 research on the accepted B.2
  head (re-basing B.0's handoff), B.3.1 implement, B.3.2 diagnostics and
  paired run, B.3.3 sweep and SPSA, B.3.4 Basilisk's forms re-read on the
  fitted arm, B.3.5 gate, B.3.6 colour-indexed histories (S2; the B.3.4,
  B.3.5 and B.3.6 leaves were added or renumbered 2026-10-31 by the
  maintainer: the packet's B.3.4 and BAS-S43 are B.3.5). Known risk:
  Rarog's donor multi-cut rule failed its KBNK drive test and Rarog kept
  its own rule. Basilisk's mate-drive floors are at least as fragile: literature seeds
  have tripped them eight times.
  Audit of the remaining path, 2026-11-01 (packet *Roadmap audit*, the
  maintainer's decisions): B-12 is a gap read by §9 item 16; §9 items
  1-3 precede the whole-arm paired run, which runs on the defaults they
  leave; items 6, 7, 8 and 11 run only on a measured population move;
  a categorical verdict needs the 95% interval to exclude 0 as well as
  the ±5 estimate (PROCESS *Cluster delivery shape*); B.3.4 re-reads
  only forms whose constants the fit moved; B.3.5 (b) reads the bake
  revision before B.3.4's adoptions; B.14 lands before B.3.2's first
  game; P5 (bench 0.40x, suite 0.26x against 0.6-1.4x) is scored at
  B.3.2's opening; the ledger entries BAS-S41 to BAS-S43 are assembled
  from the packet's §11 plus every amendment's *clauses replaced* list.
  B.2.3 residuals (maintainer decision 2026-10-24; evidence
  `analysis/b23_constants_2026-10-14.md`, findings 4 and 5):
  - `CoreHistoryFills` 0/1 categorical: the donor's negative fills for
    fresh history tables against Basilisk's clear to 0. Five history
    coordinates moved together in the fit, toward history counting more.
  - The correction update converted to the donor's units, which replaces
    the `CoreCorrLimit` 1024/289 categorical. The fit wanted the correction
    stronger, so a tighter limit would cap a signal it wants larger.
  - Both are settled before B.6 puts history or correction coordinates on
    a surface.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Correctness first: A.11.2 lands before any IIR arm. Then the arms only with
    a per-line extension budget (B-7: Rarog bench 8.10M to 7.48M, one position
    229M to 1.33M nodes), a deep-iteration cost screen (B-97: geometric mean
    of cumulative node ratios over depths 16-19 at most 2-3x, every depth the
    head finishes) and a silence watchdog (B-98); in Rarog IIR off or in
    Stockfish's form exploded the search (B-5), and a ply bound did not bound
    depth (B-6).
  - KBNK as a multi-budget majority before the multi-cut decision leans on it
    (B-3).
  - Categoricals: correction training on decisive results (B-14: refused at
    `search_kernel_core.cpp:1327`; Rarog's refusal read -15.1 ± 9.7) and at
    singular-excluded nodes (B-15); singular TT-depth margin 2 against 3
    (B-46, +12.9 in Rarog); cut-node-only NMP read at `10+0.1` as well (B-33:
    +3.1 at STC, -10.4 at LTC in Rarog).
  - Ranges and stores: `SingularBetaMult`'s range 1..6 per ply excludes the
    donor's converted margin of about 0.26 (B-45; Rarog's in-range read
    +24.2); the best-scored move is stored at fail-low nodes, unlike
    Stockfish, which changes the no-TT-move populations of IIR, razoring, RFP
    and LMR (B-12, `:1219-1223`; reclassified 2026-11-01 as a gap, not a
    defect: research amendment 6 read the store's own product as 7 of the
    83 IIR firings on the fixture line, a population calibration, so §9
    item 16 reads it as a categorical); IIR's range widened off its rail (B-55,
    B-103).
  - Mechanisms: the NMP verification region (B-31; same-ply verification is
    inert) with a deep wire check and the verify depth off the tune surface
    (B-32); the verification overwrites the node's stack record (B-37);
    ProbCut move cap and ordering (B-39), a TT-value guard with a depth
    condition (B-42), TT-served ProbCut small (B-43); every margin term
    audited against its entry guard (B-34); triple and check-extension
    composition (B-49); the negative extension (B-50, B-8); a new LMR term
    puts its bases on the surface (B-51); the in-check extension (B-53).
  - Instruments: the RFP/NMP crossover band per depth (B-35), the in-check
    share against the oracle (B-54), canary calibration under neutral
    perturbations (B-94), counter units (B-112); ablation bit 6 also removes
    the check extension (B-109); CI covers the cluster's switch arm (B-114).
  - Method: zero-game reads repeatedly mispredicted the game sign in Rarog
    (B-89, B-90), so BAS-D11 and BAS-D13, decided on WAC alone, are re-read by
    games here.
  Code audit inputs (2026-10-27 second pass,
  `analysis/code_audit_2026-10-27.md` §6.2: the kernel read against the
  pinned donor `edb0d9db`; IDs are its items):
  - S1: the three proof searches type their child as a cut node — NMP
    (`search_kernel_core.cpp:675`), ProbCut (`:715`) and the singular
    verification (`:983`) all pass `cut_node = true`, where the donor
    passes `false`, `!cutNode` and `cutNode`. The child takes the
    cut-node LMR term (about four plies at the donor's value) and the
    cut-node TT-cutoff rule, so refutations of the null move and of the
    ProbCut capture are searched shallower and singular alternatives are
    reduced harder — the opposite of what NMP's R and the singular
    margins were fitted around. B.2's packet left them "untouched
    (B.3)". Three categorical bits in B.3.0: fixed-node
    `null_cuts/null_tries`, `probcut_cuts/tries`, singular firings and
    bench, then the cluster SPRT.
  - S2: capture, continuation and pawn histories and the continuation
    correction are indexed by piece type with no colour
    (`history_core.h:51`, `:65`; `search_worker.h:256-257`): White's NxB
    on f6 and Black's share one entry, and a reply after White's Nf3
    shares a row with one after Black's. B.2.0's rationale (review §5.5,
    "colour-redundant") is false — the side to move at the consumer does
    not separate the storage and no board symmetry maps f3 to f3; the
    donor indexes by `Piece`. The tables feed ordering, statScore,
    history pruning, LMR depth and the capture margins. A B.3.0 residual
    beside B.2.3's, settled before B.6 as leaf B.3.6: a shadow counter of
    cross-colour hits per table, a colour-indexed build read on first-move cutoffs,
    mean cutoff index and bench (memory about doubles for the
    continuation and pawn tables), then SPRT `[0,3]`.
  - S3: in-check nodes are never reduced or move-loop pruned (`:854`,
    `:1121`, `:1137`, `:1268`), where the donor runs LMR, the step-down,
    `depth -= 3`, LMP, capture SEE, history pruning and quiet SEE in
    check and gates only quiet futility; this stacks with the
    unconditional check extension and hindsight on 15.84% of interior
    nodes (BAS-S16), and BAS-X01 and BAS-D11 measured the extension only
    in this state. On the check-extension card (B-49, B-53): one bit;
    fixed-node depth, in-check subtree share, the WAC floor; SPRT;
    BAS-X01 re-read with it on.
  - S5: the donor's `seekMate` coupling is absent — at root depth 16 or
    more with a root score of at least 2000 internal it caps RFP at
    depth 6 and disables singular extensions (`search.cpp:352`,
    `:395-396` at `edb0d9db`); Basilisk has no path from the root score
    to node selectivity, and its live problems are won-ending trees and
    mate drives (BAS-R07, the KBNK trips). The singular half here, RFP's
    in B.4; measured on the KBNK and KQK fixtures at several budgets and
    BAS-R07's rec1 timeline with the switch.
  - S7: NMP gates and sizes R on the TT-refined `eval` (`:655-659`); the
    donor uses the static eval for both, so a TT lower bound RFP already
    consumed also inflates NMP's population and reduction. Not covered
    by B-33 or B-34 as written; one switch, fixed-node `null_tries` and
    conversion.
  - S8: no quiet pruning at any PV node (`:916`, `else if (!is_pv)`);
    the donor prunes quiets at PV nodes off the previous PV (`!followPV
    || !PvNode`) and gates IIR on the same bit. A `followPV` stack bit;
    PV-node counts at fixed nodes.
  - S10: the TT bound-mismatch penalty (`penalize(1)` above depth 5) is
    in `edb0d9db`; B.0's table deferred it as post-SF-19. Low; a B.3.0
    note.
  - S12: `(ss+1)->double_exts` is written only in the move loop
    (`:1077`), so NMP and ProbCut children inherit a previous node's
    count; off by one or two against a cap of 16. Bookkeeping.
  Simplification read (2026-10-28, the kernel against the donor after
  A.14.1), deletion arms beside the mechanisms above:
  - The double-extension counter and its cap (`search_kernel_core.cpp:999-1001`,
    `:1080`; `DoubleExtMax` 16) have no donor counterpart. Count how often
    the cap binds per singular firing first; if it does not bind at the
    budgets the cluster reads, delete the counter and the cap rather than
    repair S12, under `[-5,0]`.
  - The same-ply NMP verification at depth 10 and more (`:684-690`), which
    B-31 reads as inert, is deleted at `[-5,0]` as the comparison arm for
    the donor's verification region, so the region is accepted against
    no verification as well as against today's.
  - Upcoming repetition (cuckoo) is absent from `src`; BAS-S12 closed it
    on the old harness with a retry trigger that wanted one named
    missing consumer. It is now named: at the TT probe, an upcoming
    cycle against a stored decisive bound (A-13, B.7.1), so the trigger
    fires here. The donor's form, after `is_draw` (`:347`) and before
    the tablebase and TT probes, and in quiescence after `:122`: when
    `alpha < draw` and a repetition is reachable, alpha rises to the
    draw score and the node returns on `alpha >= beta`. It couples to
    the TT cutoff's graph-history guard, mate-distance pruning, the
    fixed draw value (B-84) and the board's key history (B.7.1). Prior:
    a few Elo in Stockfish; one pre-registered terminal test as BAS-S12
    required.
    - [x] **B.3.0** `[R2]` Research and handoff on the B.2 head
      Opened 2026-10-30. Re-base B.0's Cluster 2 handoff on the accepted B.2
      head (bench 19,470,325 since A.13.2) and settle every input above
      into one handoff packet in `analysis/`: the contents, the
      interactions with Cluster 1's fitted core, the categoricals and
      deletion arms, the zero-game reads it needs (fixed-node counters,
      bench, the KBNK and KQK floors), the prospective predictions, the
      falsifiers and the B.3.1 contract with its seeds. Behaviour-neutral
      seams the cluster needs land at the exact fingerprint, as B.2.0's
      did. Closes `READY_FOR_IMPLEMENTATION` or returns the cluster with
      its evidence.
      - Stopped 2026-10-30 on the KBP-K false win (B.12, repaired the
        same day) and resumed on the repaired head. Seam: eight
        diagnostic counters (singular tries and multi-cuts, null-move
        tries and cuts by depth bucket) at the exact fingerprint
        (`1414039`, `bench 13` 16,615,757, Diag flavour verified).
      - Closed 2026-10-30 `READY_FOR_IMPLEMENTATION`
        (`analysis/b30_cluster2_research_2026-10-30.md` §8). Reads: the
        depth-14 stride-1 counters on the repaired head (§2.1: in-check
        nodes 26.4% of interior, NMP conversion 41.5%, singular tries
        2.41% with 28.5% extended, IIR's stale clause 56% of its
        firings and 0.3% overlap with hindsight, the double cap bound 5
        times in 51.5M nodes), the conversion floors at five budgets
        (KBN-K 115/120, the majority rule's base), WAC at 100,000 nodes
        240/300. Decisions: the donor's check policy is the ON arm with
        the current policy as the first two categoricals; the per-line
        extension budget (B-7) replaces the double-extension cap; B-12,
        B-14, B-45 and B-109 are repaired inside the ON arm behind
        attribution switches; thirteen categoricals and four attribution
        switches registered with brackets (§9); the cross-cluster surface
        rule (§5.12) puts a Cluster 1 coordinate on B.3.3's surface when
        the ON arm moves its population by 10% or more. The B.3.1
        contract is §10 (eleven tickets), the seeds §7, the frozen
        predictions P1–P10 and the B.3.2–B.3.4 registrations §11
        (BAS-S41 to BAS-S43 reserved). Corrections to the inputs:
        `ttMoveHistory` is not an LMR term in `edb0d9db` (B-51 moot);
        the oracle build exposes no counters, so B-54 reads ON against
        OFF.
    - [x] **B.3.1** `[I2]` Implement behind the umbrella switch
      The packet's contract behind one umbrella option, both arms building;
      accepted by an independent review that reproduces the fingerprints
      before diagnostics start.
      Contract: packet §10 (`B3_PROOF`, tickets 0–10 in order, each
      keeping the OFF arm at 16,615,757 and recording the ON arm's
      fingerprint and its per-ticket counter table for both clusters'
      rows), seeds §7, switches and defaults §9, invariants and tests
      §10, cheap qualification §10 (the two-arm table, WAC at 100,000
      nodes ≥ 232, the floors at five budgets with KBN-K ≥ 112, the
      depth-20 wire check, the deep-iteration cost screen, the liveness
      proof). A false premise returns the leaf with a *Return* section
      in the packet, as B.2.1 did.
      - Returned 2026-10-30 at ticket 1 (packet *Return 1*): the budget
        binds at bench depth (12,800 of 89,121 singular firings; ON
        13,315,120) and the cap it replaces binds there too; tickets 0-1
        committed (`823d087`), OFF arm 16,615,757.
      - Research amendment 1, 2026-10-30 (packet): the budget lands
        with ticket 6 on the graded extension and the cap stays until
        then; with the check extension off the budget binds 1.2% of
        firings and the cap 0.007% (no deletion games owed); P8 scored
        a miss and re-registered as P8a; `B3ExtBudget` read by games
        unconditionally; the quiet-pruning consumer of `follow_pv`
        assigned to ticket 2. Resumes at ticket 1b.
      - Returned 2026-10-30 at ticket 6 (packet *Return 2*): P8a fails,
        the budget truncating 76.7% of singular firings at depth 13 and
        83-97% at depth 20, because every quiet non-PV firing extends
        three plies; tickets 0-6 committed (`13cdda0`), OFF arm
        16,615,757; WAC floors and the bench-11 cost screen also fail.
      - Research amendment 2, 2026-10-30 (packet): each failed floor
        traced to a seed taken without its partner; the quiet triple
        term is the 24x (dropping it reads 1.15x the OFF tree), the
        small ProbCut's margin 121 the WAC loss (240 holds the floor),
        the null move's margin a hand-off band against Cluster 1's
        RFP (three RFP coordinates join the surface; the oracle's
        eval gate proposed as default); P8a scored a miss, P8b
        registered; the depth-6 floor re-based on the ON arm, the
        100k floor 232 binding. Maintainer decisions the same day:
        the quiet triple term seeded at 0 (the donor's pair a sweep
        point), the null-move gate a categorical with the donor's form
        as default. Resumes at tickets 3, 5 and 6 as amended.
      - Returned 2026-10-30 at ticket 6 on amendment 2 (packet *Return 3*):
        P8b 53.5% and WAC 234 at 100k pass, but the depth-13 tree is
        3.22x OFF's (bench 11 alone 82x; the other 39 at 0.41x) and the
        ON arm throws a won promotion (`test_search`, a7a8q); tickets 0-6
        committed (`1e77ec9`), OFF arm 16,615,757.
      - Research amendment 3, 2026-10-30 (packet): the thrown promotion
        is a tablebase win (a8=Q wins, h4 draws) lost to the arm's
        selectivity on a line the evaluation reads as level: quiet
        futility prunes the winning king move on a history-driven
        negative reduced depth (PV nodes off the previous PV included),
        the -3 negative extension strips the forced reply, the horizon
        takes the rest; no single switch names it (IIR off alone repairs
        at fixed depth within 1.3x). The singular margin at 252 repairs
        it with every floor held; bench 11's 82x is the donor's singular
        block in a won middlegame, chaotic in its constants, and the
        donor's seekMate guard fired from depth 12 collapses it (every
        gate as written passes: suite 0.26x, bench 13 6,263,268, floors
        118, WAC 237); P8b's lower edge scored, P11-P14 registered.
        Resumes at ticket 6b on the maintainer's two decisions (packet
        amendment 3 §5). Maintainer decisions the same day: both
        taken (the margin at 252, the guard from depth 12). Resumes
        at ticket 6b.
      - Returned 2026-10-30 at ticket 7 (packet *Return 4*): ticket 6b
        passed every gate (suite 0.26x, fixture +956, WAC 237, bench 13
        6,263,268); ticket 7's `B3CorrExcluded` default (the donor's)
        turns the KBP-K wrong-bishop draws into +150 to +400 from depth
        11 (18 of 121 `test_endgames` sections; 0 at every depth with
        it off); tickets 0-7 committed (`6007ab5`), OFF arm 16,615,757.
      - Research amendment 4, 2026-10-31 (packet): the route is
        confirmed by a trace: at the defender's corner the exclusion
        search excludes the one drawing king move, reads the loss
        without it (-3 to -9 pawns, never decisive, so neither the
        KNOWN_WIN guard nor B3CorrDecisive is involved) and charges it
        to the recogniser's 0; the -256 updates land on every structural
        table and the continuation correction, and the PV leaf's
        stand-pat (raw 0 plus correction) carries the score to the root.
        The consumer is shared: the OFF arm reads +90 on a drawn KBP-K
        one move before the recogniser fires and 4 of the 28 test reads
        fail after priming its tables; sf_19 has no evaluation-level
        recognised draw, so the donor's precedent says nothing here.
        Four guards priced: every one holds the 28, the floors and the
        fixture; B3CorrExcluded=0 holds the 28 but leaves the fixture at
        0 until 200,000 nodes. Recommended: ticket 7b takes the
        raw-evaluation guard on the exclusion producer (no exclusion
        training where the raw static eval is 0; default 1 kept; OFF
        16,615,757, ON 3,498,408, WAC 250, fixture +661, KBN-K 118), and
        the consumer guard goes to a correctness leaf (ANY TIME, before
        C.0). P15-P16 scored, P17-P18 registered. Resumes at ticket 7b
        on the maintainer's decision 1 (packet amendment 4 §5).
        Maintainer decision the same day: taken (ticket 7b takes the
        raw-evaluation guard, default 1 kept); the consumer-guard leaf
        opened as B.14. Resumes at ticket 7b.
      - Returned 2026-10-31 at ticket 8 (packet *Return 5*): ticket 7b
        passed every binding read (`920cc5d`: bench 13 3,498,408, WAC
        250, fixture +661, KBN-K 118, the 28 held); ticket 8's cuckoo
        test is built and checked (`7e92ecb`: tables complete, no false
        positive in 10,000 positions, floors hold, WAC 243), but in the
        donor's form it is a cutoff that never puts the draw on a
        full-window PV, so the registered terminal tests (a root score
        of 0 at depths 2 and 1) cannot be written as registered; OFF arm
        16,615,757.
      - Research amendment 5, 2026-10-31 (packet): Return 5's option 1
        taken; the terminal test reads the cutoff at the root's child
        under a test-only root window at the draw, (0, 1): ON 0 at depths
        1-3, `B3Cuckoo` 0 and OFF the win, and (-1, 0) unchanged by the
        switch; the TT pre-emption test (b) withdrawn (a PV first child
        never takes a TT cut); the delta-bound finding recorded for B.4.
        Resumes at ticket 8's terminal tests.
      - Returned 2026-10-31 at ticket 9 (packet *Return 6*): ticket 8's
        terminal test landed (`5b6bae1`); ticket 9 built as contracted
        (`58ea1ed`), but on its defaults the fail-low store throws the
        promotion fixture through IIR (h3h4 0 from 50k to 130k nodes;
        a7a8q +382 with IIR off) and, with the penalty, KR-K reads 23 at
        30,000; OFF arm 16,615,757.
      - Research amendment 6, 2026-10-31 (packet): the whole-root trace
        shows the fixture flips at iteration 9 when a7a8q fails low and
        h3h4 takes the root; the store's own product (fail-low entries
        without a move) is 7 of the 83 IIR firings on the line, the
        rest fresh and evaluation-only entries both settings make; IIR
        off, IIR's minimum depth 7 and CoreLmrCutNodeNoTt 0 each restore
        the win, and under `searchmoves a7a8q` the store-on tree wins it
        while the store-off tree reads 0 (a knife-edge both ways). The
        KR-K miss (`4K3/8/8/8/8/5k2/8/1R6 w`) converts under every one of
        six single-setting changes: the same class of event, still a
        floor miss. The donor composes the same forms; the difference is
        the calibration of the no-TT-move consumers. Recommended: Return
        6's option 1 (`B3FailLowStore` default 0, the penalty 1, the
        store read as §9 item 16 by a paired run); alternative
        `B3IirMinDepth` seeded 7, both priced in full. P24-P31 scored,
        P32-P33 registered. Resumes at ticket 9b on the maintainer's
        decision 1 (packet amendment 6 §5).
        Maintainer decisions 2026-10-31: both taken as recommended
        (`B3FailLowStore` default 0, the store §9 item 16; `B3TtPenalty`
        1). Resumes at ticket 9b.
      - Tickets 9b and 10 landed 2026-10-31 (packet *Implementation
        record — tickets 6b to 10*): OFF 16,615,757 and ON 6,723,172 on
        Release, Tune, Diag and Ablate, sanitizer `bench 10` equal, CTest
        25/25 and 23/23 under sanitizers on both arms; WAC 241 at 100k,
        the depth-6 floor re-based to 119 (count 132; a regression guard
        on the arm's own reading, not a correctness floor); fixture +918;
        floors 24 x 15, KBN-K 120; the 28 held; all eight ablation bits
        live; `B3MultiCut` stays 0. The re-scoped cost screen waits for
        an idle host; then the independent review.
      - Returned 2026-10-31 at the cheap qualification (packet *Return
        7*): the re-scoped cost screen passes four positions and fails
        `suite_v2` position 50, growth geomean 2.53 against 1.5. The OFF
        arm's tree settles at depth 14 (30.07M, then growth about 1.000);
        the ON arm is smaller at every depth (0.38x at 19) but still grows
        2.27, 1.93 and 3.71 per iteration. The screen records nodes only,
        so explosion and an unreached settling point are not yet told
        apart; three depth reads are listed. P11 missed. Resumes on
        research amendment 7.
      - Research amendment 7, 2026-11-01 (packet; written as `R2` in the
        implementing session at the maintainer's instruction): position
        50's growth is the ON arm reaching at depth 19 the mate the OFF
        arm found at 14, at 0.38 of its nodes. No mechanism carries it,
        and growth is 1.02 once the mate is found. The screen's positions
        were the OFF arm's largest depth-14 trees, each a resolution at
        13-14. The clause now judges steps 20-22 of a read to depth 22,
        continued to 25 if those fail; node counts decide. P11's
        consequence is not taken; P34-P35 were registered first
        (`50a97b8`).
      - Returned 2026-11-01 at the amended screen (packet *Return 8*):
        four positions pass. Bench position 11 fails, at depth 22 under
        (a) (ON's iteration 22 is 500M nodes) and on growth under (b)
        (4.57). ProbCut carries it: switched off, the depth-21 tree is
        0.16x; its tries are 4x the OFF arm's share at the matched event.
        Singular is silent under seekMate. The OFF arm has a blowup of the
        same kind three plies later (184x over depths 23-25). P34 missed.
        Resumes on research amendment 8.
      - Research amendment 8, 2026-11-01 (packet, `R3`): the blowup is the
        position's conversion band (a +24 game under the material-scale
        evaluation: the root score climbs 100-400 cp an iteration from
        about 2,650, four to eight full-depth fail-high re-searches each),
        which every arm has when its score enters it; ProbCut off still
        grows 3.27x and 3.62x at 21-22 (P36 hit) and no ProbCut constant
        inside its bracket removes it (0.24x to 4.1x at depth 21, chaotic
        and a phase reading; P37 missed); the quiescence load is
        razoring's and the leaves', not ProbCut's (P39 missed); the port
        is the donor's line for line. Recommended: no default moves;
        bench position 11 leaves the screen's set for `suite_v2` 12 by a
        written rule (|score| < 564 at depth 14, no resolution jump), its
        band reads recorded, not judged; ticket 10b runs the screen.
        Alternative priced in prospect: ProbCut standing down under
        `seekMate`. P41 registered. Resumes at ticket 10b on the
        maintainer's decision (packet amendment 8 §6).
      - Maintainer decision 2026-11-01: decision 1 taken (`suite_v2` 12
        replaces bench 11; the band reads recorded, not judged), decision
        2 not taken (ProbCut's form unchanged). `READY_FOR_IMPLEMENTATION`
        at ticket 10b.
      - Ticket 10b, 2026-11-01 (packet *Implementation record — tickets
        6b to 10*): amendment 7's screen passes all five positions of
        amendment 8's set. On `suite_v2` 12, steps 20-22 have geomean
        0.99, both arms finish every depth to 25, and no watchdog fired.
        Positions 2-5 reproduce amendment 7's read node for node. Bench
        11's conversion band is recorded and not judged. P41 hit. The
        cheap qualification is complete (`LOCAL_QUALIFIED`); the
        independent review is next (packet §10 *Review*, which includes
        amendment 7 §R and amendment 8 §1-§4).
      - Review accepted with named follow-ups, 2026-11-01 (packet
        *Review*, a separate session): every fingerprint reproduced from
        clean trees on all four flavours and both sanitizer trees, CTest
        25/25 and 23/23 on both arms, WAC 241 at 100k and 132 at depth 6,
        floors 24 x 15 with KBN-K 120, the fixture +918, the 28 held, all
        eight bits live, the wire check 237, amendment 8's selection
        re-scored and amendment 7's screen reproduced node for node (all
        five pass); the code matches the handoff as amended with four
        unrecorded deviations, none a change of mechanism; follow-ups:
        amendment 2's surface additions for B.3.3's registration, and
        three packet-record items. Closed.
    - [x] **B.3.2** `[V]` Diagnostics and the unfitted paired run
      Packet §11, BAS-S41, in the order the roadmap audit of 2026-11-01
      fixed (the entry file is assembled from §11 plus every amendment's
      *clauses replaced* list, never from §11 alone): the zero-game
      screens on both arms (counters, the oracle differential, WAC, the
      crossover band, the canary calibration, pooled NPS, the cost
      screen), with P5 scored at the opening (already outside its band:
      bench 0.40x, suite 0.26x; the mechanism named from the ablation
      table); §9 items 1-3 (five runs, two at `10+0.1`), which decide
      what arm the whole-arm run measures; the 2,000-game paired run
      that governs (P1, the −40 stop) on the defaults they leave; then
      items 4-16 in §9's order, items 6, 7, 8 and 11 only if the
      depth-14 counters show the alternative moving its mechanism's
      firing share by 10% or more of itself (§5.12's rule). Adoption or
      rejection needs the 95% interval to exclude 0 as well as the ±5
      estimate (PROCESS). The floors tool's staleness guard exits 1 on
      the OFF arm at 120,000 nodes (KQ-K's mean 10.8 plies); the run's
      scripts tell that exit from a floor failure before the first
      read. The §5.12 surface read decides which Cluster 1 coordinates
      B.3.3 fits. B.14 lands before the first game.
      - Opened 2026-11-01 on the head after B.14 (BAS-S44 H1; `bench 13`
        OFF 11,880,564, ON 4,923,507). BAS-S41 is registered
        (`docs/experiments/BAS-S41.md`): the order, the run table and
        the seeds, before any game. Item 2's read B is item 1's `3+0.03`
        run. The review's follow-ups 2-4 are recorded (packet *B.3.2 —
        opening*).
      - Zero-game screens 2026-11-01 (BAS-S41 *RESULT*; packet *B.3.2 —
        the zero-game screens*), on clean `c6f7151` binaries:
        - The cost screen passes all five positions.
        - The floors hold. The OFF arm's 120k exit is the staleness
          guard (KQ-K 24/24), not a floor.
        - WAC at 100k reads 240 on both arms; at 400k, 264 on both.
        - The canary calibration flips 0 under Hash 32 and 128. The ON
          arm then passes 65 of 77 canaries, the OFF arm 70.
        - Item 5's same-ply form refuses 0.036% of null cuts, so the
          1-against-2 run is not owed.
        - Seven §5.12 candidates join B.3.3's surface; the LMR pair and
          `CoreLmrCutNodeNoTt` stay off.
        - P5 misses: 0.41× on `bench 13`, 0.17× on the suite. The
          extension family carries it (0.78× with it removed on both
          arms): the check policy drops the check extension.
        - P6 and P7 held. P3, P4 and P8b's lower edge missed; P4's
          falsifiers fired. No stop rule fired.
        - Pooled NPS (P9) waits for an idle host.
        - `tools/run_b32_games.ps1` is prepared: items 1-3 (five runs)
          and the paired run, all dry runs accepted. Each game waits for
          the maintainer's go-ahead.
      - §9 item 1 at `3+0.03` (also item 2's read B), maintainer-run
        2026-11-01: `B3CheckExt=1` reads **−22.4 ± 9.7 Elo** (2,000
        games, 0 faults), 1.8 plies shallower per move. The interval
        excludes 0, so by item 1's rule the extension cannot be adopted
        and the default 0 stands; P10's first leg (within ±8) missed.
        The `10+0.1` run stays registered as the recorded LTC read.
      - §9 item 2 read A, maintainer-run 2026-11-01: the legacy check
        policy reads **−30.8 ± 9.5 Elo**, so both check defaults stand.
      - Preconditions for items 4-16, all zero-game:
        - By §5.12's 10% rule items 6 and 7 run, and items 8 and 11
          keep their defaults without games.
        - P33 held (+7.9 points).
        - Item 12's cost screen with `B3Iir` 0 passes.
        - NPS pools `b32-on` and `b32-off` are built: four clean
          builds each at `c6f7151`. They replaced the first `pgo1`
          and `pgo2` builds before any game used them, so the paired
          run is re-pinned to each pool's build 1.
        - `tools/run_b32_games.ps1` now carries items 4-16. An order
          guard refuses the paired run until items 1-3 leave the
          defaults, and items 4-16 until the paired run reads above −40.
        - Every queued launch was dry-run and accepted.
      - Overnight 2026-11-01/02 (maintainer-run; BAS-S41 *RESULT*):
        - Item 1 at `10+0.1` read −5.4 ± 8.6 (recorded).
        - Item 3 read −1.4 ± 9.2 at `3+0.03` and −5.0 ± 9.0 at
          `10+0.1`. No default moves.
        - **The paired run, ON against OFF: +67.5 ± 9.7 Elo** (2,000
          games, 0 faults), 2.6 plies deeper per move. P1 (+10)
          missed high, and the −40 stop is far away.
        - Pooled NPS on `bench 13` reads −11.1%. The arms search
          different trees; in games they read −2.4%. P9's falsifier
          fires as written ("the cuckoo loop to B.7 before the gate").
          That is the maintainer's decision before B.3.5.
        - Items 4-16 did not play: the order guard could not parse a
          positive estimate and refused every launch. Fixed and proved
          on all six finished runs; items 4-16 are next.
      - Items 4-16, maintainer-run 2026-11-02 (BAS-S41 *RESULT*, all
        clean):
        - `B3NmpEvalGate` 1 reads **+11.3 ± 9.3**, so the rule adopts it.
        - IIR off reads **−11.3 ± 9.2** and is rejected.
        - Every other run reads inside ±5, or at ±5 with its interval
          holding 0, and keeps its default.
      - Blocked 2026-11-02: the gate's adoption (default 0 → 1,
        `adopt/adopt_gate.patch`; ON `bench 13` 5,920,540) fails two
        ON release tests.
        - The promotion fixture plays `a7a8q` at 0, not positive, at
          100,000 nodes; it sights the win from 200,000.
        - `test_b3_proof`'s `B3TtPenalty` liveness section stops moving
          the node count.
        - Every other binding read holds.
        - The change is not committed: the default stays 0 and the ON
          arm stays 4,923,507. The maintainer decides.
      - Maintainer decision 2026-11-01 (review follow-up 5): B.12, B.14,
        B.3.0 and Cluster 2 reach `master` together, in one delivery at
        the end of Cluster 2, once everything is accepted.
      - Maintainer decision 2026-11-02 on the gate:
        - The two binding tests veto its adoption now. The donor's bare
          null move (`B3NmpEvalGate` 0) stays through B.3.3's fit.
        - B.3.4 re-reads the gate on the fitted arm (its clause below).
      - Closed 2026-11-02 (BAS-S41):
        - The paired run reads **+67.5 ± 9.7 Elo**, ON against OFF,
          unfitted.
        - Every §9 run is read; no default moved; IIR is confirmed.
        - §5.12 admits seven Cluster 1 candidates to B.3.3's surface.
        - The ON arm stays at `bench 13` 4,923,507; the OFF arm, the
          fingerprint record, at 11,880,564.
        - P9's consequence (the cuckoo loop to B.7) is the maintainer's
          decision before B.3.5's gate.
    - [x] **B.3.3** `[V]` Curvature sweep, then SPSA in blocks
      Packet §11, BAS-S42: five registered sweep coordinates (the
      singular margin with the donor's value as an extra point), then
      SPSA in blocks on Cluster 2's X lines plus §5.12's admissions;
      theta is the last completed block; the fitted-constant analysis
      precedes the bake.
      - Opened 2026-11-02. BAS-S42 is registered, with the sweep frozen
        before its first point (`analysis/b33_sweep_2026-11-02.md`).
        - The surface: Cluster 2's 19 X lines plus §5.12's ten Cluster 1
          admissions.
        - `B3ProbCutTtMargin` is dropped: its switch stays 0, so the
          coordinate is inert (the review's follow-up 1).
        - The donor's singular pair (17 with `B3SingTripleQuiet` −53)
          is a read-only sweep point.
      - Sweep read 2026-11-02 (`analysis/b33_sweep_2026-11-02.md`):
        - Three of the five coordinates are curved, so the SPSA runs:
          the null move's entry constant, its reduction divisor and
          ProbCut's margin.
        - The singular margin is monotone toward its range's top (378);
          the triple bar's base toward the 0.5× edge.
        - The surface is generated (`464ef33`, 29 coordinates).
        - Block 1 is prepared (seed 42001) and its dry run accepted.
          Maintainer-run overnight.
      - Schedule check, 2026-11-03 (Triumviratus review TM-2: its tuner
        decayed from the value meant as the final one and halved every
        tune): block 1's resolved schedule sets `c0 = c_end · 2000^0.102`
        (1.9999 for `c_end` 0.9211) and its log shows c falling from there
        each iteration, so `c_end` is reached at the last iteration, as in
        fishtest. Nothing to change.
      - Block 1 read 2026-11-03 (BAS-S42 *RESULT*; maintainer-run, resumed
        four times from its checkpoint with the same command):
        - 2,000 iterations, 60,000 games, 0 faults. The block's final
          progress record confirms the schedule check: c reached `c_end`
          at iteration 2,000.
        - **The movement rule reads STOP:** only `B3NmpRedBase` (7 → 4)
          moved a full step. Theta is block 1's rounded centres: 21 of 29
          coordinates differ from the seed.
        - Every coordinate the sweep did not cover moves `bench 13` at
          its range edges, so none is inert.
        - The fitted-constant analysis is written
          (`analysis/b33_constants_2026-11-03.md`). The fit makes the bare
          null move less aggressive on all four of its moving terms
          (B.3.4's gate re-read prices that; no refit is asked). The
          singular margin goes 252 → 232, against the sweep's WAC
          direction. The rest of the surface is near-flat.
        - Waiting on the maintainer: theta changes seven Cluster 1
          defaults that both arms share. Baking them moves the OFF arm's
          fingerprint as well, unless the defaults become per-arm
          (finding 8, recommended (A), per-arm, OFF unchanged).
      - Maintainer decision 2026-11-03: per-arm defaults.
      - Bake blocked 2026-11-03 (not committed;
        `tools/results/b33-bake/bake.patch` `527adc63…81d5`, with the test
        logs):
        - What the patch does:
          - The 14 Cluster 2 values go into their X lines.
          - The seven Cluster 1 values go into a B3_PROOF-only table,
            `BASILISK_B3_CORE_DEFAULTS`, which a constructor applies.
          - Tune builds now advertise the compiled default.
          - The generator seeds the ON arm's surfaces from the arm's own
            defaults, and `test_search` checks each arm's defaults.
        - What it reads:
          - ON `bench 13` **5,369,042**; OFF stays 11,880,564.
          - The old tune binary with theta's 29 options replays
            5,369,042.
          - OFF CTest passes, and the arm-defaults sections pass on all
            four builds.
        - Two ON binding tests fail:
          - The promotion fixture (`test_search` 1267) plays `a7a8q` at
            0 at 100,000 nodes. It sights the win at 150,000 (+1098),
            200,000 and 300,000. `B3ProbCutBase`, `CoreCaptFutBase` or
            `CoreRfpCorrMult` alone each drop the 100,000 reading to 0,
            and nine of the other single changes lower it.
          - The cuckoo fixture (`test_b3_proof` 584) needs a search
            without cuckoo to read above 0 at depths 1-3. At depth 3 it
            now finds White's repeating `Kf7` and reads the draw, 0.
            `CoreLmpBase` 3 → 4 alone does it: LMP searches one more
            quiet move at White's depth-2 node.
        - Neither is a wrong move or a wrong value. Both are a fixture's
          budget or premise. These are the binding tests that vetoed the
          eval gate on 2026-11-02, so the maintainer decides.
      - Maintainer decision 2026-11-03: fix the two fixtures'
        preconditions in their own commit, then bake.
        - `61ac728`: the promotion fixture reads within 200,000 nodes on
          both arms, and the cuckoo fixture's premise is asserted at
          depths 1-2. The cuckoo assertions stay at 1-3. Both pass on the
          pre-bake engine, ON and OFF.
        - `f02a9e4`, the bake, as the blocked patch:
          - ON `bench 13` **5,369,042** (was 4,923,507); OFF 11,880,564
            unchanged, on Release and Tune builds.
          - The old tune binary with theta's 29 options replays 5,369,042.
          - Release CTest passes 25/25 on both arms. `test_search` and
            `test_b3_proof` pass on both Tune builds, the arm-defaults
            sections included.
        - `a271a24`: the generator seeds the B3_PROOF surfaces from the
          arm's own defaults. `config_b33.json` now equals theta, and
          `--check` and its tests pass.
        - For B.3.4: the eval gate was vetoed partly by the promotion
          fixture's 100,000-node budget, which `61ac728` replaced, so its
          re-read meets the 200,000 budget.
      - Closed 2026-11-03 (BAS-S42):
        - Block 1 is the only block (STOP), and theta is its rounded
          centres.
        - The fitted-constant analysis is written
          (`analysis/b33_constants_2026-11-03.md`).
        - Baked per arm: the ON arm's fingerprint is 5,369,042, and the
          fingerprint record (OFF) stays 11,880,564.
    - [x] **B.3.4** `[V]` Basilisk's forms re-read on the fitted arm
      Added 2026-10-31 (maintainer). B.3.2's categorical runs read the
      donor's forms with Cluster 1's constants fitted to the old tree, so
      a donor form can lose only for want of its fit. On the fitted tune
      build, every place the arm still keeps Basilisk's form where the
      donor's differs is read once more, the donor's form against
      Basilisk's: a §9 categorical at Basilisk's value by default or by
      B.3.2's run (today `B3FailLowStore` 0, `CoreHistoryFills` 0 and
      `B3ProbCutTt` 0), and
      every categorical whose B.3.2 run read inside ±5 and whose
      mechanism's constants B.3.3 moved (a coordinate it gates, or is
      gated by, on the fitted surface); one whose constants did not move
      keeps B.3.2's verdict (audit 2026-11-01). Each is one
      2,000-game paired run at `3+0.03`, after the conversion floors at
      the five budgets and the WAC floor as vetoes; the donor's form is
      adopted at ≥ +5 with the 95% interval excluding 0, Basilisk's kept
      otherwise. An adoption does not
      re-run the fit unless the fitted-constant analysis asks. Seeds that
      differ from the donor's (the singular margin 252 against 17,
      `B3SeekMateDepth` 12 against 16) are B.3.3's, whose sweep carries
      the donor's points. Every departure that stays is written to the
      packet's departures table for B.6.6: Basilisk's form, the donor's,
      the reading that kept it, and the feature or fit it waits on.
      - Added 2026-11-02 (maintainer): the null move's eval gate
        (`B3NmpEvalGate` 1, a departure from the donor), re-read on the
        fitted arm against the donor's bare form.
        - B.3.2 read it +11.3 ± 9.3, but its adoption failed two ON
          binding tests, so the donor's form went into the fit.
        - One 2,000-game paired run at `3+0.03`, adopted at ≥ +5 with
          the 95% interval excluding 0.
        - Release CTest is a veto as well as the floors and WAC: the
          promotion fixture's ON clause and `test_b3_proof`'s liveness
          sections included.
        - Kept or not, the reading goes to the departures table.
      - Opened 2026-11-03 (BAS-S45, registered before any game;
        maintainer-run):
        - Nine runs on one fitted ON Tune build
          (`tools/run_b34_games.ps1`).
          - Clause a: `B3FailLowStore`, `CoreHistoryFills` and
            `B3ProbCutTt` at the donor's form.
          - Clause b, a B.3.2 read inside ±5 whose mechanism the fit
            moved: `B3NmpCutOnly` 0, `B3MultiCut` 1, `B3SingNegative` 1,
            `B3SingTtDepth` 1 and `B3IirStale` 1.
          - The eval gate.
        - Not re-read: `B3NmpVerify` (−5.0 is at ±5, not inside), the
          runs that read outside ±5, `B3Cuckoo` (no fitted coordinate on
          either side of it) and `B3ExtBudget` (settled without games;
          it goes to the departures table unread).
        - The fitted arm's binding set before any game
          (`tools/results/b34-20261103/fitted_price.*`):
          - release CTest passes;
          - the wrong-bishop 28, the organic 7/7 and the primed 28/28;
          - WAC 136 at depth 6 and 241 at 100,000 nodes;
          - KBN-K 119 summed over the five budgets, and `test_endgames`
            passes.
        - The build is pinned (`basilisk-b34-on-tune-pext-tune-pgo.exe`,
          `D47013B9…`, revision `220f36f`). All nine launches were dry-run
          and accepted.
        - An agent-started queue (not its to start) was stopped by the
          maintainer during run 1. `B3FailLowStore` checkpointed at 1,494
          of 2,000 games and resumes with its own command. Runs 2-9 have
          not started. Maintainer-run from here.
      - Maintainer decision 2026-11-04, before runs 2-9: **B.3.4 is the
        eval gate's run alone** (BAS-S45 amendment 1).
        - Why the other runs go:
          - The fit was made with Basilisk's forms in place, so a donor
            form read now plays unfitted against a form fitted for it.
            The read is tilted toward the kept form: it could confirm a
            donor form, but never clear one.
          - Clause b's five runs already default to the donor's form,
            and B.3.2 read them near zero.
        - Why the gate stays: its tilt runs the other way. The fit was
          made with the donor's bare null move, so a gate that wins
          against it is trusted.
        - The partial `B3FailLowStore` run is dropped, unread.
        - The three kept forms and `B3ExtBudget` stay Basilisk's while
          they work; there is no switch to the donor's form without a
          reason. A fair comparison needs both arms fitted, which is
          expensive fine tuning for a few Elo (B.3.2 read them at +3.3,
          −0.5 and +0.9), so it waits in Phase H (H.2). Their departures
          rows say so.
        - The same day, after a challenge the maintainer asked for:
          `B3FailLowStore` goes to B.4 instead.
          - B.4's TT write on exit must choose what quiescence stores
            after a fail low. The donor stores no move there, which is
            the same question in the main search.
          - So B.4 settles both together, and its own fit covers the
            chosen form. This is a form B.4 must choose anyway (Phase H's
            exception).
          - `CoreHistoryFills`, `B3ProbCutTt` and `B3ExtBudget` stay in
            H.2: nothing in B.4 or B.5 forces them.
      - Closed 2026-11-04 (BAS-S45; maintainer-run):
        - The eval gate reads **−6.1 ± 9.7 Elo** on the fitted arm (2,000
          games, 0 faults, PGN recounted), so it is not adopted. B.3.2
          read +11.3 unfitted. The fit, made with the bare form, made the
          null move less aggressive on all four of its moving terms.
        - No default moved. B.3.5's candidate is the fitted arm at
          `f02a9e4`, 5,369,042.
        - The departures table is in the packet
          (`analysis/b30_cluster2_research_2026-10-30.md`): four kept
          forms with their waits (B.4 for the fail-low store, H.2 for
          the rest), two seed values for the fit, and the gate's reading.
    - [x] **B.3.5** `[V]` Cluster 2 gates
      SPRT `[0,3]` for the cluster, and each categorical and deletion arm
      at the bounds the packet registers.
      Packet §11, BAS-S43 (the packet's B.3.4): (a) the fitted ON arm
      against the OFF arm, `[0,3]` at `3+0.03`; (b) the fitted arm at
      B.3.3's bake revision, before B.3.4's adoptions, against the
      unfitted arm, `[0,3]`, only if B.3.3 ran its SPSA; (c) a 2,000-game
      `10+0.1` direction read, recorded, not a gate. The candidate is
      B.3.4's: the fitted arm with its adoptions.
      - Added 2026-11-02 (maintainer), before the gate: P9's
        consequence is read on the fitted arm, never on `bench 13`
        pools of different trees (B.3.2 read −11.1% there and −2.4% in
        games).
        - First, the Release ON build's profile (`tools/profile.ps1`):
          the share of search time in the upcoming-repetition check.
        - Only if that share is material, pooled NPS of two Release
          pools that differ only in `B3Cuckoo`'s default.
        - Item 9 read the check at 0.0 ± 9.3 Elo. The maintainer then
          chooses B.7 or keeping the donor's form at its cost.
      - Registered 2026-11-03, before any of its games (BAS-S43, while
        BAS-S45 is prepared):
        - (a) has a cap of 2,000 pairs; the drift model at B.3.2's +67.5
          puts the stop near 550.
        - (b) has a cap of 12,000 pairs; at an expected +5 the stop is
          near 9,000.
        - (c) is 2,000 games at `10+0.1` and scores P14.
        - The order: B.3.4's games, the P9 profile, (b), the maintainer's
          P9 choice, then (a) and (c).
        - The packet gives (b) no consequence. A reversion of theta is
          the maintainer's decision.
      - Prepared 2026-11-04 (BAS-S43 *RESULT*):
        - B.3.4 adopted nothing, so the candidate is the fitted arm.
          ON Release (`3DFF7B64…`, 5,369,042), OFF Release (`4127849D…`,
          11,880,564) and ON Profile (`389FD285…`) are built clean at
          `5f1cdf3` and pinned in `tools/run_b35_gates.ps1`.
        - The dry runs of (a), (b) and (c) are accepted.
        - P9's profile: the upcoming-repetition check is in 0.30% of
          samples, Wilson [0.21%, 0.42%], on the bench 13 cohort. That is
          not material, so no NPS pools are owed. The profiler's region
          map gained two missing files on the way (`ec9d6de`).
        - Waiting on the maintainer: the P9 choice (B.7, or keep the
          donor's form at 0.3%), which (a) and (c) wait for. (b) is ready
          for the maintainer to run.
        - The maintainer's P9 choice (2026-11-04): keep the check at its
          0.3%. All three gates are released to the maintainer.
      - Closed 2026-11-04 (BAS-S43 *RESULT*; maintainer-run, every count
        recounted):
        - (a) **H1, +92.8 ± 14.1 Elo** (996 games) against the OFF arm.
        - (b) **H1, +17.6 ± 6.4 Elo** (4,574 games) for the fit itself.
          The two time forfeits were the unfitted arm's, within the
          allowance.
        - (c) **+84.7 ± 9.1 Elo** at `10+0.1` (2,000 games).
        - So B3_PROOF defaults ON, and the fingerprint record moves to
          5,369,042. The OFF arm stays compilable at 11,880,564.
        - The default build was re-read on a fresh configure: `bench 13`
          5,369,042. Release CTest passes 26/26 on the default, ON and OFF
          builds.
        - P14 holds at the instrument's ceiling (264/264 and 101/101
          conversions).
        - P9 missed: game NPS is 0.96× the OFF arm, and the cause is not
          the cuckoo loop (0.30%). B.8.5 takes it.
    - [x] **B.3.6** `[I2]` Colour-indexed histories (S2)
      Added 2026-10-31 (maintainer): S2 above, given a leaf so it runs. On
      the accepted B.3 head, research first: a shadow counter of
      cross-colour hits per table (capture, continuation and pawn
      histories, the continuation correction); a negligible rate closes
      the leaf `NO_CHANGE` without games. Otherwise the donor's indexing
      by `Piece`, read at fixed nodes on first-move cutoffs, mean cutoff
      index and bench with its memory priced, then SPRT `[0,3]` at
      `3+0.03`. Settled before B.6, which may put these tables on a
      surface.
      - Opened 2026-11-04 (BAS-S46, registered before the read):
        - Step 1 is the shadow counter, in diagnostic builds only
          (`src/history_shadow.h`): per table, how much of the training
          behind each read the other colour gave.
        - Every flavour's `bench 13` is unchanged (ON 5,369,042, OFF
          11,880,564), and `test_history_core` checks the counter on a
          planted case.
        - The read: `run_suite.py` on `suite_v2` at depth 14 with the ON
          Diag build.
        - The rule: NO_CHANGE without games if every table reads under 2%
          on both measures; otherwise step 2, registered separately.
      - Step 1 read 2026-11-04 (BAS-S46 *RESULT*):
        - The other colour's share of the training behind a read is 16.3%
          in the continuation history, 11.3% in the pawn history, 7.8% in
          the capture history and 3.1% in the continuation correction.
        - In the continuation history, 8.8% of reads were trained by the
          other colour alone.
        - Every table is above 2%, so step 2 follows: the donor's index by
          `Piece`.
        - The header's "colour-redundant" rationale (`history_core.h`) is
          measured false.
      - Step 2 prepared 2026-11-04 (BAS-S46). The maintainer decided:
        now, not Phase H.
        - The investigation: a continuation row is read one ply on by the
          other side and two on by its own, so it needs colour on both
          row and leaf, the donor's form. The pawn and capture histories
          need it on the piece. The continuation correction needs it on
          its row alone.
        - `70e978b`: behind the `COLOUR_HISTORY` switch, default OFF, with
          every flavour exact while it is off. With it on, `bench 13`
          reads 5,361,058.
        - The planted check reads 0% on the capture, continuation and
          pawn histories. The correction's 0.59% is all its shared
          sentinel row, as a probe showed.
        - From clear tables the cutoff reads are slightly worse: 0.8743 →
          0.8731 first-move, and 5.9% more nodes at depth 14.
        - The gate, SPRT `[0,3]` with a 16,000-pair cap, is pinned and
          dry-run accepted (`tools/run_b36_gate.ps1`). Maintainer-run.
        - The run was interrupted at 3,702 pairs, before a decision. A
          Rarog workload spiked the host for about four minutes and
          slowed both arms equally. It resumes from its checkpoint,
          unchanged (BAS-S46 *GATE INTERRUPTION*).
      - Gate 2026-11-04 (BAS-S46 *GATE RESULT*): **H1** at 9,618 pairs,
        +4.7 ± 3.0 Elo (a first-crossing estimate). It had eight time
        losses (5-3) and no other faults. In games the coloured arm
        searched 2.0% fewer nodes per second.
        - `COLOUR_HISTORY` defaults ON. The fingerprint record moves to
          5,361,058. `COLOUR_HISTORY=OFF` reproduces 5,369,042.
        - CLOSED.
- [x] **B.16** Search-programme instruments
  Added 2026-11-03 (maintainer: "the instruments which would help us now,
  somewhere close"): four instruments the Triumviratus review found that
  Basilisk lacks, built before B.4 because B.4 and B.5 are their first
  users. None changes the engine's tree or a gate; each is validated on a
  planted case before it is trusted. B.16.2 needs no idle host, so it can
  be worked while B.3.3's blocks hold the machine. Its leaves are marked
  `(ANY TIME) — before B.4` (maintainer 2026-11-04: B.16 now, after B.3),
  because the roadmap orders leaves by number. Sources:
  `analysis/triumviratus_review_2026-11-03.md` (TT-1, TT-2, TM-7, TM-9).
    - [x] **B.16.1** `[V]` Wall-clock overhead read (ANY TIME) — before B.4
      What the gate's clock costs outside the search, at the controls the
      gates use (`3+0.03` leaves about 50–100 ms a move, so a few ms is
      several per cent of the think) (TT-2). Timed from outside the
      engine: `go nodes 1000` wall time against the engine's own report
      and against Stockfish 19 on the same host; `go movetime` 50, 100 and
      300 overshoot; ponderhit-to-bestmove latency; at Threads 1 and 4.
      Triumviratus found 6.15 ms against Stockfish's 1.24 ms this way, a
      +29.8 Elo brake at 5+0.05 with an identical tree. Basilisk passed the
      code reading (steady clock, unsynced streams, persistent threads);
      this read verifies it. A timing read needs an idle host: the
      maintainer chooses when. A gap to Stockfish beyond 1 ms goes to the
      correctness-repair path with its own gate.
      - Prepared 2026-11-04 (BAS-R08, registered with its prediction):
        `tools/diag/wallclock.py` times each trial from outside the
        engine, Basilisk and Stockfish 19 alternating every round, each
        pinned to N game cores. It recovered delays planted in a fake
        engine (`tools/diag/test_wallclock.py`): +5.03, +2.99 and +4.01
        ms for 5, 3 and 4, and 0 with none. The deciding quantities are
        wall times, because both engines report `time` in whole ms.
        Binary `basilisk-b161-master-pext-pgo` (5,361,058, D5E67D0A...)
        and launcher `tools/run_b161_wallclock.ps1`, dry run accepted.
        The read takes about seven minutes on an idle host; the
        maintainer starts it.
      - Read 2026-11-05 (BAS-R08 *RESULT*), host idle, 3,600 trials.
        Threads 1 passes: Basilisk's stop costs about 0.5 ms a move,
        every gap to Stockfish at most +0.77 ms, `go nodes 1000` 0.51
        against 0.81 ms. Threads 4: `movetime` overshoot +1.08 to +1.14
        ms, over the 1 ms line; four threads add about 0.4 ms to every
        stop. Part of the gap is the reference's: Stockfish's whole-ms
        clock stops up to 1 ms early. The 4T stop latency is a
        correctness-repair finding whose leaf the maintainer places;
        D.1 takes the numbers as its overhead prior. CLOSED.
    - [x] **B.16.2** `[I2]` Pruning and reduction pricing (ANY TIME) — before B.4
      What each pruning rule and each reduction hides (TM-9, Triumviratus's
      `prunelog` and `redlog`). In the Diag flavour, a deterministic sample
      of the moves each rule prunes (every `TraceEvent` pruning kind,
      quiescence's included) and of the reduced searches that fail low is
      searched again at the depth the rule denied, with the window the node
      had. Recorded per rule, depth, node type and move number: decisions,
      the share that would have raised alpha, and the share that would have
      changed the node's best move. The label that matters is a move that
      failed low reduced and beats alpha at full depth (1.7% of probed
      moves there); a label of "beats alpha at the reduced depth" spends
      nodes on moves already re-searched (−9.65 there). Validation: with
      sampling off the bench is identical; a planted over-pruning rule
      shows a higher hidden share than its honest form. The readings
      screen, never accept: they point B.4's quiescence pruning, B.5's
      line growth, B.3.4's form re-reads and later fits at the rules worth
      a game.
      - Built 2026-11-05 (`f2d8116`, reader `tools/diag/pricing.py`): the
        `Pricing N` option of the tune and diagnostic builds probes every
        N-th decision of 16 rules (the table-served ProbCut too, off by
        default) and of reduced fail-lows. A probe is isolated: it writes
        neither the table nor the histories and its state is restored, so
        a priced search is the same search, a stronger check than "the
        bench is identical with sampling off". IIR, the hindsight and
        post-alpha depth changes and the negative singular extension are
        not priced.
      - Validated (BAS-S47): bench 5,361,058 on all three flavours; isolation
        clean on 106 positions (2,490,852 probes), and live (a leaking
        build differed everywhere); planted loose reverse futility and
        quiet futility read higher than honest with disjoint intervals.
        The first quiet-futility plant failed: lowering its slope tightened
        the rule for poorly rated quiets (Amendment 1).
      - Reading (106 positions, depth 16, every 4th decision): most hidden
        changes per million search nodes in quiescence futility (4.3% of
        pruned captures beat alpha, 5.1% of the first three), reduced
        fail-lows (2.4%; 4.1% at cut nodes, 4.8% for moves 1–3) and
        reverse futility (2.7%; 10.9% at all nodes); razoring 12.4%, 25.0%
        at cut nodes. The exchange rules hide little (SEE 0.5–0.6%, the
        null move 0.4%). For B.4 (quiescence futility and delta), B.5
        (early moves at cut nodes) and the fits and form re-reads (razoring
        and reverse futility by node type). CLOSED.
    - [x] **B.16.3** `[V]` Loss analysis by cell (ANY TIME) — before B.4
      Where Basilisk loses, from games already played (TT-1): the pool
      tournaments' PGNs (BAS-M08, BAS-M11) and the latest gates', every
      move scored by a judge (Stockfish 19 at fixed nodes) as the
      evaluation before it minus the evaluation after, from the mover's
      side. Tabulated for both sides by position class (defending, level,
      ahead), clock left and material, with the opponent's moves in the
      same cells as the control; the worst errors searched again at 4× and
      16× the game's nodes to split clock errors from evaluation errors.
      On Triumviratus one cell, defending with under 10% of the clock,
      held a third of the loss to Stockfish 19. It feeds B.9's checkpoint
      and the order of C and D. No new games; the judge's CPU is the cost,
      maintainer-run.
      - Prepared 2026-11-05 (BAS-M14, registered with its prediction):
        `tools/diag/loss_by_cell.py` judges every move at 150,000 nodes,
        tabulates ours against the opponents by class, clock and
        material, and searches the 100 worst open errors again at 1×,
        4× and 16× the game's nodes. Sources: BAS-M11's 2,400 games
        (exported read-only from the Colosseum app; its PGN on disk had
        none), the first 2,000 games of B.3.6's gate (today's engine) and
        Basilisk 1.10.0's 8,200 games of BAS-M08's tournament, which carry
        no clocks or nodes. Validated on synthetic games, a planted queen
        sacrifice (868 cp, counted 300) and a determinism check (761
        positions identical); dry run accepted. Launcher
        `tools/run_b163_loss.ps1`, resumable, about five hours on 15
        cores; the maintainer starts it.
      - Closed 2026-11-05 as an instrument built (maintainer: a read runs
        in the step that needs it). The run was withdrawn unplayed: two of
        its sources were 1.10.x, before the search rebuild, and the third
        today's engine against itself, which B.4 to B.6 change before any
        reader. The read moves to D.1, its reader, on the games of the
        engine D.1 tunes (BAS-M14). CLOSED.
    - [x] **B.16.4** `[I1]` Identical-tree counter read (ANY TIME) — before B.4
      Speed of a change that keeps the tree, by hardware counters (TM-7):
      cycles, instructions and branch misses per node, the engine pinned to
      one core, only the search thread counted (start-up excluded), and a
      byte-identical copy of the base as the noise control. The tool on the
      5950X is chosen here (ETW PMC sources or AMD uProf). Validated
      against A.9.4's pooled read on one patch of known effect. Then the
      instrument of B.7.9, B.8.5 and any profile share a gate asks for
      (B.3.5's repetition-check share).
      - Tool chosen 2026-11-05: ETW PMC through the Windows Performance
        Toolkit's `xperf`, which lists InstructionRetired, TotalCycles and
        BranchMispredictions on the 5950X (AMD uProf is not installed).
        Counters recorded at each context switch attribute them to the
        thread switched out, so only the search thread is counted. The
        kernel logger needs an elevated shell, which the agent lacks; the
        parser waits for a real trace: `tools/diag/pmc_probe.ps1` (about a
        minute, elevated, no idle host) saves one of a pinned `bench 13`
        to `tools/results/b164-pmc-probe/`. Then the read, its null (a
        byte-identical copy) and its check against BAS-P25's +2.35%
        (identical tree) are built and registered.
      - Probe traced 2026-11-05 by the maintainer: every one of 13,241
        counter logs paired with its switch, no negative delta; the search
        thread at 4.83 GHz, 3,656 instructions, 1,788 cycles and 19.5
        branch mispredictions a node; start-up 2.2% of the cycles.
      - Implemented 2026-11-05, `LOCAL_QUALIFIED` (BAS-P28):
        `tools/diag/pmc_read.py` (6 tests) and the elevated launcher
        `tools/run_b164_pmc.ps1`: BAS-P25's two revisions rebuilt as
        four-build pools (19,470,325 each) and a byte-identical copy as the
        null, three interleaved cycles of a pinned `bench 13`. Accepted
        when the null holds 0 within ±0.5% and P25's interval lies below 0
        and meets its NPS read's −2.30% [−2.63%, −1.97%]. Dry run accepted;
        the read needs an elevated shell and an idle host, about ten
        minutes, maintainer-run.
      - Closed 2026-11-05 as an instrument built (maintainer): its
        validation read (BAS-P28, registered, not run) moves to B.7.9, the
        first leaf that reads it; B.8.5 reads it after. CLOSED.
    - [x] **B.16.5** `[I1]` Prompt helper stop (ANY TIME) — before B.4
      Added 2026-11-05 (maintainer: "investigate and repair now"),
      worked straight after B.16.1: the repair of its four-thread
      finding. A timestamp build placed the extra 0.4 ms in the join,
      where the main thread waited for each helper's next stop check,
      made every 2,048 nodes with the clock read. Helpers now read the
      stop flag at every node (`fa2b866`); the clock read is unchanged.
      - Implemented 2026-11-05, `LOCAL_QUALIFIED`: bench 5,361,058
        unchanged; CTest 24/24; screening read at 4T, base / repair,
        `movetime` overshoot 0.99 / 0.82 and 1.07 / 0.80 ms, ponderhit
        1.10 / 0.81 ms, 1T within noise; the gap to Stockfish falls
        under 1 ms (BAS-R09).
      - Gate prepared: BAS-R09, `[-5,5]` at `3+0.03` with Threads 4,
        1,500-pair cap, `tools/run_b165_gate.ps1`, dry run accepted.
        At one thread the change cannot alter a game without pondering,
        so the gate runs at four. The maintainer starts it; `GAME_GATE`.
      - Gate 2026-11-05, `CLOSED`, retained: the cap without a boundary
        (3,000 games, LLR −0.96), −2.2 ± 7.1 Elo at 4T, no faults or time
        losses; the cap keeps a repair (BAS-R09).
- [x] **B.4** Cluster 3 — quiescence
  TT cutoff, corrected stand-pat with interpolated fail-high, futility base,
  move-count and SEE pruning, evasions only in check, TT write on exit; SPRT
  `[0,3]`.
  Its packet keeps a departures table for B.6.6 (maintainer 2026-10-31):
  every place the cluster keeps Basilisk's form where the donor's differs,
  the reason, and the feature or fit it waits on.
  Split into leaves 2026-11-05 (maintainer: the sub-steps visible in
  GUIDE), in the order they run: the two between-cluster repairs (B.4.1,
  B.4.2), then the standard cluster shape (PROCESS *Cluster delivery
  shape*): B.4.3 research, B.4.4 implement, B.4.5 diagnostics and paired
  run, B.4.6 sweep and SPSA, B.4.7 gates. Basilisk's forms are re-read
  against the donor's in B.6.6, from the departures table. B.4.3 is the
  research earlier records call B.4.0.
  B.4.3 research carries two cards:
  1. **The SEE and move-ordering value scale** (archived 9.2). The dedicated
     SEE vector 100/300/300/500/900/20000 and `search.cpp`'s legacy values
     are compared against the HCE's material by counting, at real
     thresholds, the changed `see_ge` verdicts, MVV-LVA order changes and
     delta-margin decisions. Unequal values alone are not a defect. An
     injectable value surface is built only if decisions move, with
     exact-default identity and normalized benchmark values kept separate.
  2. **First-ply quiet checks.** Count-based skipping, low-depth null
     cutoffs and zero-depth probes all assume that a mate threat by a quiet
     move stays visible one ply later, and a captures-only qsearch makes that
     false (Rarog, from Manta's MAN-S36). B.2's canaries include quiet mate
     threats, and B.4 may not remove any check generation B.2 turns out to
     rely on. Basilisk's qsearch quiet-check loop, inert at cap 0, is the
     existing switch. B.0 measured it (BAS-D21): at cap 6, WAC at 100k
     nodes 204 → 214 (24 gained, 14 lost), 32 more positions solved by
     depth 3, +33% nodes to depth 12; and half of razoring's tactical cost
     is razoring at depth 1 into the checkless quiescence.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Quiescence returns `alpha` on a fail-low (`search_kernel_core.cpp:225`,
    `:321`), so a root fail-high echoes beta and drives aspiration cascades
    (B-19; 93% of Rarog's root fail-highs equalled beta). Fail-soft, with the
    KBNK and KQK mate-distance fixtures as obligations; the tablebase band
    reachable without tables (B-87).
    - Pulled forward to A.11.11 (maintainer decision 2026-10-26); B.4
      fits on the repaired quiescence.
    - A defect, not an option: fail-hard is to be replaced. The earlier
      fail-soft attempts broke those fixtures; the fixtures bind the fix,
      they are not a reason to keep the defect.
    - It also miscalibrates the equal-score tie rule (`inc` in the move
      loop), which assumes Stockfish's fail-soft search. A fail-hard return
      echoes alpha exactly two plies up, inside the rule's window
      (`ply + 2 >= root_depth_`), so "equal" there is usually a bound, and
      one time in eight a refuted move becomes the node's best move, its
      history bonus and its TT move. A.11.6 kept the root out of the rule
      (`0eadf03`); below the root the cause is this fail-hard return.
  - Fail-high interpolation moves the estimate RFP and razoring were fitted on
    (Rarog's depth-14 tree +41%), so RFP, razoring and NMP-base coordinates
    join B.4's surface from the start (B-21).
  - Quiet-evasion pruning in check (B-22: Rarog's zero-game rule set it off,
    its game read adopted it at +7.8 ± 9.4); quiescence in check never stores
    to the TT (`:152-180`).
  - Margins as coordinates (B-25: futility 150, delta `QUEEN + 200`, SEE 200,
    late prune 6/-50 are literals); the SEE clamp's -800 is dead because
    `alpha >= stand_pat` there (B-17); the late-capture rule is nearly dead
    (B-18); `MAX_QSEARCH_PLY` 10, where Stockfish has no cap; decisive TT
    cutoffs at PV qnodes (B-27); keep the stored PV bit, not Reckless's
    subtree marking (B-16), and SEE without check exemptions (B-23).
  - Simplification read (2026-10-28): delta pruning
    (`search_kernel_core.cpp:221-230`) is removed, as the donor has
    none, at `[-5,0]` before its margin joins B-25's coordinates. The
    per-move futility at +150 already bounds every capture that does not
    check, and the node-level cut also drops checking captures and
    promotions, which futility exempts.
  - Instruments: depth-0 refinements counted by total mass (B-20, B-24); the
    qsearch's size re-measured on the current head (B-28); the SEE/evaluation
    unit census (B-26).
  - Cards: quiet checks, whose WAC gains did not predict games in Rarog
    (B-29); the late-mate exposure of `CoreLmpExemptChecks` 0 named, Rarog's
    fixture as a known-failing case (B-2); the dead shallower rule (B-63:
    Rarog's donor value read -12.9, so deletion in B.8 is the prior).
  Code audit inputs (2026-10-27 second pass, §6.2):
  - S9: quiescence evasions are searched in generator order (`:166`) —
    the probed TT move is ignored and there is no MVV or history order,
    where negamax's in-check nodes use the full picker and the donor
    orders evasions; B-22 covers pruning only. An evasion-node cutoff
    index counter, then TT-move-first at fixed nodes.
  - Futility card details: the base is the TT-tightened stand-pat where
    the donor uses the static eval plus its margin; no recapture
    exemption (`to == prevSq`) and no move-count pruning; the qsearch
    MVV-LVA formula differs from the picker's capture formula over the
    same capture-history table (card 1).
  - S5's RFP half: the donor's `seekMate` caps RFP's depth at 6 in won
    roots (B.3 has the singular half).
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Card 2's prior: on Triumviratus's Stockfish-19 search, removing quiet
    checks from quiescence read +9.71 ± 5.99 (3,294 games, 20+0.2) once a
    one-move-per-node cap was lifted, and the opposite sign under the cap.
    A check-generation read holds only for the quiescence it was taken in
    (TS-13, TM-5).
  - Negative priors on that base: the passed-pawn push to the seventh, the
    quiet hash move and a material stand-pat searched in quiescence (TS-14).
  - Already in the donor's form here, so nothing to do: quiescence TT
    entries at depth 0 (Triumviratus's −1 cost it +3.38 ± 2.58), pinned
    attackers filtered in SEE, promotions in SEE (the review's §3).
  B.3.4 input (maintainer decision 2026-11-04):
  - The fail-low store, decided once for both searches. The TT write on
    exit chooses what quiescence stores after a fail low; the donor stores
    no move there. The main search keeps Basilisk's form behind
    `B3FailLowStore` 0, which stores the best-scored move.
    - B.3.2 read the donor's form at +3.3 ± 9.1, unfitted, on an arm
      whose fit came later and was made with Basilisk's form.
    - B.4 chooses one form for both, in the main search and quiescence,
      before its fit, so that the fit covers the chosen form. The IIR
      coordinates the store feeds (`B3IirMinDepth`, the stale trigger) go
      on its surface if the form changes.
  Rarog sweep input (2026-11-04, `analysis/rarog_sweep_2026-11-04.md`):
  - Reverse futility's correction term when the evaluation is a TT bound
    (RAR-S40): the margin adds the correction magnitude
    (`CoreRfpCorrMult`) even when `eval` was replaced by a TT bound, in
    which the correction no longer is. Rarog's arm without it there read
    −4.50% nodes on 9.0% of nodes, never gated. B.16.2's pricing split by
    eval source (static against TT-refined) says whether it is material;
    if so, the form is a categorical on B.4's surface (B-21 puts RFP
    there). Stockfish has the same form, so it questions the donor's
    form; it is not a porting defect.
    - [x] **B.4.1** `[I1]` Legal positions in the floors' generator
      Between clusters, before B.4.3's first read (2026-11-05; B.9's
      until then). The floors' generator admits positions with the side
      not to move in check (research amendment 6 §3:
      `3k3R/8/1K6/8/8/8/8/8 w` is a KR-K sample). Fixing it re-draws the
      24 samples per family and re-bases every floor, so it is done
      between clusters, never inside one (audit 2026-11-01).
      - Prepared 2026-10-31 on `master` and kept as a patch
        (`tools/results/b9-floor-legality/legal-floors.patch`, SHA-256
        `d1b0fcce…293a`); the throwaway branch was deleted 2026-11-04.
        - The recipe: `random_family` in `tests/test_endgames.cpp` reads
          each FEN with `try_set_fen(fen, /*validate_legal_position=*/true)`,
          so a bare king in check with White to move is dropped. 24 of the
          96 samples were such positions: KQ-K 8, KR-K 4, KBB-K 7, KBN-K 5.
          The legal samples keep their order, and the next legal draws fill
          each 24.
        - Read on `master` (`43b4c35`) at 60,000 nodes: 24/24 in every
          family, means 12.2, 23.0, 24.4 and 54.4 plies. Every bound holds
          there.
        - The patch applies cleanly to the B.3 head (checked 2026-11-04).
      - The leaf applies it, reads all four families at the five budgets
        on the head, and re-bases a floor only if the read asks.
      - Closed 2026-11-05 (`933929d`): the patch applied as prepared, its
        comment carrying the head's read. `test_endgames --conversion
        --nodes N` on a release build of `7a926de` (`bench 13`
        5,361,058), a cold search per move
        (`tools/results/b41-20261105/conversion/`); conversions of 24,
        mean plies in brackets:

        | Family | 30k | 45k | 60k | 90k | 120k | Sum |
        |---|---:|---:|---:|---:|---:|---:|
        | KQ-K | 24 (15.9) | 24 (11.5) | 24 (12.1) | 24 (11.7) | 24 (10.8) | 120 |
        | KR-K | 24 (24.1) | 24 (23.2) | 24 (21.2) | 24 (20.9) | 24 (20.2) | 120 |
        | KBB-K | 24 (24.8) | 24 (24.4) | 24 (24.5) | 24 (23.7) | 24 (23.6) | 120 |
        | KBN-K | 21 (54.6) | 22 (51.9) | 24 (56.2) | 24 (51.5) | 24 (53.4) | **115** |

        - Every bound holds at 60k, where the bounds judge, staleness
          included, so no floor moves. Off 60k the read reports only:
          KBN-K's 21 at 30k and KQ-K's staleness trip at 120k (a mean
          below two thirds of its ceiling, as on the illegal sample).
        - The cluster's KBN-K rule (the B.3.0 packet §2.2: a change
          counts only as a majority over the five budgets, the sum moving
          by more than 3) reads against this table from here on; its base
          stays 115. The default endgame suite passes (191/191).
    - [x] **B.4.2** `[I2]` SEE's created pins and promotion recaptures
      Between clusters, before the fit (2026-11-05; B.15.1's until then):
      quiescence's SEE pruning and thresholds are fitted on SEE's
      verdicts. A-2 and A-3 (BAS-C09; fixtures in `tests/test_board.cpp`)
      are repaired here. Card 1's census counts the verdicts each repair
      changes; each goes through the correctness-repair path with its own
      gate.
      - Implemented 2026-11-05, one engine commit each; closed 2026-11-06.
        - The quiet-SEE test's bound is corrected first, in its own
          commit (`9f29f49`): a quiet move to a back rank an enemy pawn
          attacks can lose the mover and the queen's surplus; the oracle
          reads below −mover in exactly the corpus's 22 such cases.
        - A-3 (`9aaa528`, `bench 13` 8,532,389): a pawn recapturing onto
          a back rank wins the queen's surplus and leaves a queen.
          `see_ge` charges the surplus at its two "safe even if my piece
          is taken" exits when the next recapturer is a pawn the pins
          allow; `see()` adds it to the gain. The fixture whose promoted
          queen stands reads −800 (was 0).
        - A-2 (`d3add9c`, `bench 13` 6,090,091): a departure that shares
          a line with a king and an enemy slider of that line's kind
          recomputes the pins. The mid-exchange fixture reads −300 (was
          −400).
        - Card 1's census of the verdicts each repair changes was read
          here, ahead of B.4.3; these are its counts.
        - Every changed `see_ge` boundary on a 104,000-move random-walk
          corpus was wrong before and equals the make/unmake oracle
          after (A-3 48, A-2 16); the 138 divergences left all have a
          check on the exchange. In the head's `bench 13` tree 5,226 of
          7,660,016 calls change verdict (0.068%), 5,126 on a back rank.
          Searched alone from fresh tables, A-3 changes 10 of the 40
          bench trees and A-2 none; the rest of each `bench 13` move is
          the shared table and histories.
        - Qualified on `d3add9c`: release CTest 24/24 and the strength
          tests 2/2, ASan/UBSan CTest 24/24 (sanitizer `bench 10`
          2,135,353), the conversion floors at the five budgets equal to
          B.4.1's table (`tools/results/b42-20261105/conversion/`).
        - Gates, maintainer-run 2026-11-05, both `[-5,5]` at `3+0.03`, both
          the cap without a decision: BAS-S48 (A-3 against `9f29f49`)
          +1.9 ± 4.4 Elo, LLR +2.18, 4,000 pairs; BAS-S49 (A-2 against
          A-3) −3.8 ± 7.6 Elo, LLR −1.55, 1,500 pairs. Both repairs stay
          as correctness fixes with no measured harm, as the maintainer
          confirmed 2026-11-06. The fingerprint record moves to 6,090,091.
        - Not repaired, outside A-2 and A-3: SEE does not model check,
          so a capture that discovers check, or an exchange that opens
          one, still diverges from the oracle (BAS-C09's note of
          2026-10-10).
    - [x] **B.4.3** `[R2]` Research and handoff on the repaired head
      Settle the two cards and every input above into one handoff packet
      in `analysis/`: the contents, the interactions with Clusters 1 and
      2's fitted cores, the categoricals and deletion arms, the zero-game
      reads it needs, the departures table, the prospective predictions,
      the falsifiers and the B.4.4 contract with its seeds.
      Behaviour-neutral seams land at the exact fingerprint. Closes
      `READY_FOR_IMPLEMENTATION` or returns the cluster with its evidence.
      - Closed `READY_FOR_IMPLEMENTATION` 2026-11-06:
        `analysis/b43_cluster3_research_2026-11-06.md`. Run in one
        session on the `R3` model at the maintainer's request.
        - Seams: 33 diagnostic counters for quiescence and what its
          depth-0 entries feed (`1fd783b`), with their suite-runner
          units (`86769bf`). `bench 13` 6,090,091 on release and
          diagnostic builds; CTest 24/24.
        - Reads, registered at `8de2c95` before running (packet §R,
          scored in §6), `suite_v2` at depth 14:
          - quiescence is 42.2% of nodes (36.2% at B.3.0);
          - card 1: the HCE's material changes 1.03% of `see_ge`
            verdicts and 2.30% of quiescence's futility verdicts, over
            the registered 1% and 2%;
          - 24.5% of futility-pruned captures are recaptures, which the
            donor exempts;
          - 14.5% of the captures the donor's count rule would remove,
            and that are searched today, fail high;
          - evasions: 63.0% first-move fail highs against 92.6% for
            captures; 40.6% of the quiet evasions the donor would skip
            raise the node's value in generator order;
          - the ply cap binds at 0.008% of nodes and the late rule 289
            times; RAR-S40's population is 0.53% of interior nodes,
            under the registered 1%;
          - WAC at 100,000 nodes 247/300, the arm's floor base.
        - Decisions:
          - the donor's quiescence behind `B4_QSEARCH`, in eleven
            tickets (packet §10);
          - card 1: two-vector categoricals `B4SeeValues` and
            `B4GainValues`, exact default identity;
          - card 2: `B4QuietChecks`, default off, a paired run owed only
            if quiet checks lower razoring's priced hidden share by a
            quarter;
          - the fail-low store: the donor's form in both searches on
            the arm, read before the fit and bound by the floors;
          - quiet-evasion pruning on in the arm, read first;
          - the delta rule removed, its `[-5,0]` in B.4.7; the late
            rule and the ply cap removed without games;
          - no categorical for RAR-S40.
        - Surface for B.4.6: the futility base and the two blend
          weights, with PLAN's reverse futility, razoring and null-move
          base coordinates, `B3IirMinDepth` and `CoreLmrCutNodeNoTt`
          (packet §5.8).
        - IDs reserved: BAS-S50 (B.4.5), BAS-S51 (B.4.6), BAS-S52
          (B.4.7); the predictions are frozen in packet §11.
      - Read R2, after the close, at the maintainer's request
        (2026-11-06; packet *Read R2* and *Research amendment 1*):
        quiescence futility priced by class, with a throwaway split of
        B.16.2's instrument, registered at `cd2c294` before running.
        - Recaptures are 25.5% of the rule's decisions and 90.1% of the
          moves it hides: 14.77% of them beat alpha when searched,
          against 0.56% of other captures.
        - The packet's verdict stands by the registered rule. P5 is
          re-registered at 1.5%, and ticket 0's pricing keeps the class.
        - Separate finding, not added to the arm: a queen capture that
          is not a recapture beats futility 45.9% and the alpha-relative
          SEE rule 27.3% of the time (26,810 decisions in 31.2M nodes);
          B.4.5 reads it on the arm, and it is a candidate for H.2.
    - [x] **B.4.4** `[I2]` Implement behind the umbrella switch
      The packet's contract behind one umbrella option, both arms
      building; accepted by an independent review that reproduces the
      fingerprints before diagnostics start.
      - Tickets 0 to 7 built 2026-11-06 (packet *Implementation record*),
        each keeping the OFF arm at 6,090,091; the arm reads 3,166,820,
        WAC 149 at depth 6 and 257 at 100,000 nodes.
        - Ticket 2's and ticket 6's zero-game rules leave `B4EvasionPrune`
          and the arm's `B3FailLowStore` at 1: neither fails a binding read.
      - Returned 2026-11-06 at ticket 7 (packet *Return 1*): reverse
        futility's cap costs a KR-K floor, 23 of 24 at 45,000 nodes, a
        100-ply shuffle in `4K3/8/8/8/8/5k2/8/1R6 w`.
        - Every switch and neighbouring cap holds 24, so the miss is one
          cell of the position's sweep. The cap reads `seek_mate`'s bar,
          which is the donor's 16 lowered to 12 for the singular half.
        - Options in the packet; recommended: the cap's own bar at 16, read
          first. Tickets 8 to 10 wait on the amendment.
      - Resolved 2026-11-06 (packet *Research amendment 2*): the cap reads
        its own bar, `B4RfpSeekMateDepth` 16, and stays off
        (`B4RfpSeekMate` 0), so ticket 7b's tree is ticket 6's, `bench 13`
        4,003,560.
        - The miss was the floor's own rate: on a 25-budget grid the OFF
          arm also fails one KR-K playout of 600. The stop rule stands.
        - The cap is live in 53% of games at the donor's bar and finds no
          more mates at a node budget (505 of 1,092 against 512), so
          Basilisk's form stays and the departure waits on the network.
        - Tickets 7b and 8 to 10 resume on the amended contract.
      - Tickets 7b to 10 built and qualified 2026-11-06 (packet
        *Implementation record — tickets 7b to 10*): OFF 6,090,091, the arm
        4,003,560, WAC 149 at depth 6 and 258 at 100,000 nodes, the floors
        24 with KBN-K 118, the cost screen passed. The review is next.
      - Accepted with named follow-ups 2026-11-06 by an independent review
        in a separate session (packet *Review — B.4.4 accepted with named
        follow-ups*). Every fingerprint, test and binding read reproduces
        from clean sources on both arms, and the code matches the amended
        contract: six deviations the records do not list, none a change
        of mechanism.
        - Two statements of the record are corrected by the review's own
          reads. The screen's first row was `suite_v2` 12, not bench 11;
          bench 11 passes (steps 20 to 22, geometric mean 0.643).
          `run_suite.py` aborts on the arm with `B4QuietChecks` 6.
        - Amendment 2's tables reproduce from its data, and from searches
          for the two its decision leans on. The cap off overrides the
          amendment's own registered rule on the maintainer's standing
          decision, so it awaits the maintainer's confirmation.
        - The follow-ups sit in B.4.5, B.4.6 and B.4.7 below.
        - Decided 2026-11-06 (maintainer, taking the review's positions):
          - the cap stays off in the arm (`B4RfpSeekMate` 0), with its
            re-read on the network in H.2;
          - the evasion rule's accounting in tune builds is gated, not
            measured (B.4.5's opening);
          - a denser conversion floor is registered at B.5's research;
          - the full evasion sort stays an H.2 candidate, with no sixth
            categorical in B.4.5.
    - [x] **B.4.5** `[V]` Diagnostics and the unfitted paired run
      The packet's zero-game screens on both arms, then its registered
      categorical and deletion reads and the unfitted paired run.
      Its pricing screen also reads queen captures by class on both arms
      (packet research amendment 1 §4), the first reading of H.2's
      candidate.
      - Owed at the opening, from B.4.4's review (packet *Review — B.4.4
        accepted with named follow-ups*, §7):
        - `run_suite.py` is not read with `B4QuietChecks` above 0 until
          its bound "quiescence move rules within its captures" and
          `qs_gain_tested`'s comment are repaired in their own commit,
          with a test that fails first. Item 5's gate reads `pricing.py`
          and is not blocked.
        - BAS-S50 registers, before item 1's paired run, how the tune
          build's offset is handled: the evasion rule's accounting runs
          only with `B4EvasionPrune` 1, at up to 1.86% of nodes. Either
          it is gated on the flags that read it, at both fingerprints,
          or the offset is stated beside the verdict with its size.
          Decided 2026-11-06 (maintainer): gated, in a behaviour-neutral
          commit before the run; BAS-S50 cites the commit.
        - Records: the screen's first row is `suite_v2` 12, and bench 11
          passes by the review's read; A2-P5 and A2-P11 are misses as
          frozen; the departures table gains a row for the capture stage
          (every promotion, where the donor's is a capture or a queen
          promotion).
      - Done at the opening, 2026-11-06 (packet *B.4.5 — opening*):
        - `run_suite.py` reads quiet checks: `cdee38f`, with a test that
          failed first; `qs_gain_tested`'s comment is corrected in `33654f8`.
        - The tune build's offset is gated: `33654f8`, at both fingerprints.
          BAS-S50 is registered and cites it.
        - The records are in BAS-S50, and the capture stage's row is in the
          packet's opening section.
      - Zero-game screens 2026-11-06 (BAS-S50 *RESULT*; packet *B.4.5 — the
        zero-game screens*), on clean `abc3b0f` binaries:
        - The floors, the WAC floor and the cost screen pass. Both defaults
          of items 1 and 2 fail no binding read, so both go to their runs.
        - Item 5's gate is met (−62.8%), so its paired run is owed.
        - `B3NmpPerPly` and `CoreResearchDeeper` join B.4.6's surface.
        - P9's falsifier fires: pooled NPS 0.944× the OFF arm, on different
          trees. It binds before B.4.7's gate; its handling is the
          maintainer's.
        - The games are prepared in `tools/run_b45_games.ps1`, dry-run, and
          its order guards tested on 24 synthetic cases.
      - Speed work 2026-11-06 (packet *B.4.5 — the speed work*; BAS-P29),
        behaviour-neutral, before any game:
        - Both arms are about 4% faster at unchanged trees. P9's ratio stays
          0.945×, since the gain is shared.
        - P9's falsifier is answered: ticket 1's delivery measures about 0.8
          points of the gap, and the rest is mostly the arm's tree.
        - The games are re-pinned to `516d422`, dry-run again.
      - Games 2026-11-06 (BAS-S50 *RESULT*; packet *B.4.5 — the games*), six
        runs of 2,000 at `516d422`, no fault:
        - The unfitted arm reads +29.3 ± 9.5 Elo over the OFF arm. P1's stop
          does not fire. This is a diagnostic; the gates are B.4.7's.
        - No default moves. Items 1 to 5 read +6.8, −3.1, −1.9, −0.5 and
          −8.9, each ± about 9, and each interval holds 0.
        - In play the arm is level with the OFF arm (0.99 to 1.01× by game
          phase) against 0.945× on `bench 13`. P9 stays a miss and its
          falsifier still binds before B.4.7's gate.
        - B.4.6 fits this arm, `bench 13` 4,003,560. CLOSED.
    - [x] **B.4.6** `[V]` Curvature sweep, then SPSA in blocks
      The packet's registered sweep, then SPSA in blocks on Cluster 3's
      surface (with B-21's RFP, razoring and NMP-base coordinates); the
      fitted-constant analysis precedes the bake.
      - Opened 2026-11-06. BAS-S51 is registered, with the sweep frozen
        before its first point (`analysis/b46_sweep_2026-11-06.md`): 15
        coordinates on the surface, five swept.
      - Sweep read 2026-11-06 (`analysis/b46_sweep_2026-11-06.md`):
        - Three of the five coordinates are curved, so the SPSA runs: the
          futility base, the stand-pat blend and razoring's margin.
        - The fail-high blend reads monotone (its 256 edge ties the seed) and
          reverse futility's per-ply margin monotone toward its 2× edge.
        - None is flat, so all 15 coordinates stay. Each is proved live.
        - The surface is generated (`14a1810`). Block 1 is prepared (seed
          46001) and its dry run accepted. Maintainer-run overnight.
      - Block 1 read 2026-11-07 (`analysis/b46_constants_2026-11-07.md`):
        - 60,000 games, no faults. One of 15 coordinates moved a full step
          (`B3NmpPerPly` 16 → 13), so the rule stops the tune and theta is
          block 1's rounded centres.
        - No rail and no clamped perturbation. The null move's entry tightens
          and had not settled; the quiescence coordinates read flat. Both
          null-move readings go to B.6.
      - From B.4.4's review: `test_tt_switches_live` follows the tree
        (three unrelated quiescence mutants flip it). If the bake trips
        it, its precondition is repaired in its own commit. It did not.
      - Closed 2026-11-07 (BAS-S51):
        - `916618c`, the bake: three constants in their X lines, eight in
          `BASILISK_B4_CORE_DEFAULTS`, so the OFF arm keeps its values.
        - ON `bench 13` **4,144,790** (was 4,003,560), equal to the old
          tune binary's replay of theta; OFF 6,090,091 unchanged.
        - Release CTest 27/27 on both arms. The floors hold to 480,000
          nodes; KBN-K's five-budget sum is 119.
    - [x] **B.4.7** `[V]` Cluster 3 gates
      SPRT `[0,3]` for the cluster, and each categorical and deletion arm
      at the bounds the packet registers.
      - Registered 2026-11-07, before any of its games (BAS-S52):
        - (b) cap 12,000 pairs; (c) `[-5,0]`, cap 18,000 pairs; (a) cap
          4,500 pairs; (d) 2,000 games at `10+0.1`.
        - The order: (b), (c), the maintainer's P9 choice, then (a) and
          (d). (c) runs before (a) because its H0 changes the candidate.
      - The maintainer's P9 choice (2026-11-07): closed, ticket 1 not
        returned; the evasion delivery stays as built. P9 stays a miss.
      - (b) and (c) returned 2026-11-07 (BAS-S52 *RESULT*), no fault:
        - (b) H1 at 6,316 pairs, first-crossing +6.5 ± 3.6 Elo. The fit
          stands.
        - (c) H1 at 1,297 pairs, first-crossing +15.0 ± 8.0 Elo for the
          deletion. The delta rule stays deleted and the candidate is
          unchanged.
        - (a) and (d) are unlocked in `tools/run_b47_gates.ps1`.
      - Closed 2026-11-07 (BAS-S52 *RESULT*; maintainer-run, every count
        recounted, no fault):
        - (a) **H1, +31.7 ± 8.6 Elo** (2,460 games) against the OFF arm.
        - (b) **H1, +6.5 ± 3.6 Elo** (12,632 games) for the fit itself.
        - (c) **H1** (2,594 games): the delta rule stays deleted.
        - (d) **+38.4 ± 9.2 Elo** at `10+0.1` (2,000 games).
        - `a346d22` binds the ten clauses, each shown to fail on its mutant
          on Diag and Release trees.
        - So `B4_QSEARCH` defaults ON (`312dd5b`), and the fingerprint
          record moves to 4,144,790. The OFF arm stays compilable at
          6,090,091.
        - A fresh default build reads 4,144,790 and passes Release CTest
          27/27, as `B4_QSEARCH=OFF` does at 6,090,091.
        - The tests now bind the default arm alone (maintainer 2026-11-07),
          and CI builds the two OFF arms without testing them.
      - From B.4.4's review: (c)'s registration says that `B4QsDelta` 1
        restores the delta rule under the arm's band guard, not the OFF
        arm's.
      - From B.4.4's review: on H1, before the flip, the ten clauses of
        the arm's quiescence that no test binds (packet *Review — B.4.4
        accepted with named follow-ups*, §5) get tests in their own
        commit, each shown to fail on its mutant.
- [x] **B.5** Cluster 4 — root and aspiration
  Cluster 4 — root, aspiration and iterative deepening: aspiration centred on
  the root move's average score with width from its mean squared score,
  optimism, root-move effort, the `pvIdx` root structure with a `MultiPV`
  option, and deep-iteration line growth; SPRT `[0,3]`.
  Its packet keeps a departures table for B.6.6 (maintainer 2026-10-31),
  as B.4's does.
  Its research registers a denser conversion floor for this cluster and
  the later ones (maintainer 2026-11-06): the four families over the
  25-budget grid with a bound on unconverted playouts, the bound taken
  from B.4's read (`analysis/b43_cluster3_research_2026-11-06.md`,
  research amendment 2 §5: a healthy tree misses one KR-K playout of 600,
  so five budgets at 24 of 24 false-alarm about one read in five to ten).
  MultiPV exists since A.8.9, identity-gated at `MultiPV 1`, and BAS-C13's
  tablebase PV contract is kept. Time management
  proper is D.1's.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - The fail-high clock stop: keep, move to twice the target, or remove
    (maintainer items 2026-10-24 and 2026-10-25; B-79, A-47, A-48).
    Investigation `analysis/b5_failhigh_stop_2026-10-25.md`; probes
    BAS-R07. Basilisk's probes (rec1 at `60000+600`, stop removed):
    - The cascade is not gone. The A.8 (`fac5064`) and B.2 (`71a612b`) trees
      still sink to the maximum. The head finishes only because B.10's PSTs
      let its give-up window prove `mate 25`, after nine fail-highs.
    - Tree growth is present at Rarog's scale but does not cause the sink.
      10.9% of LMR-path searches go deeper than full depth. Removing the
      correction term or the `+2` ceiling cuts rec1's depth-26 tree 30× and
      23× and bench by 25% and 21%. Both arms still sink to the maximum
      without the stop.
    - The stop has a hole. It is checked only when a window fails high, so a
      give-up window started just before the optimum runs unchecked: 2.79×
      and 4.07× the optimum at `180000+2000` on those arms.
    - From the same timelines, 2× would have missed the B.2 tree.

    M1, registered in the packet with a frozen prediction, comes first: a
    sequential replay of 300 games at `3+0.03` and 100 at `10+0.1` on a
    counter build of the B.5 entry head, arms 1×, 2× and no stop. It reads
    the firing rate by score band, the time removed and the hole's rate.
    Decision rule, each option gated by a sink battery that must stay under
    0.5× the maximum:
    - keep 1× if it fires on under 10% of searches and removes under 3% of
      spend;
    - otherwise 2× against 1×, SPRT `[0,3]`;
    - removal only if the battery passes with no stop, then a `[-5,5]`
      bracket. On today's evidence removal is refused.

    The hole is a defect, owned with D.1. Rarog's leads are priors only: at the
    target its stop fired on 24% of searches and cost about −15 Elo.
    - Rarog's D.1.6 results (RAR-R25 to RAR-R27, relayed 2026-10-26) are
      leads to verify, not a solution; the packet's §9 maps each to
      Basilisk's own reads. At twice the scaled target, on any root
      fail-high, Rarog's stop fired on under 1% of searches and read no
      harm (+4.3 ± 8.3 at `3+0.03`, +4.9 ± 9.8 at `10+0.1`).
    - Reads still owed: M1's 2× arm against §2.4's B.2-tree miss; rec3 at
      `180000+2000` for the hole; how often the 2.63-ply LMR correction cap
      binds and its share of deeper searches (Rarog's one-ply cap read H0,
      −1.2 ± 3.0); and rec1 stop-free on the head after A.11.1 and A.11.2.
  - A score-scaled first window with B.4's fail-soft quiescence (B-75, B-19);
    Rarog's four-part cascade cluster read H0 (-7.4 ± 7.5) and was re-scoped
    to bare-king trees; a fail-high depth reduction only with its clock half
    (B-76).
  - Aspiration cost counted in nodes, not events (B-70); window changes
    decided by games only (B-71, B-72: one retry -18.3, delta 45 -25.4 in
    Rarog); the width is coupled to LMR's `delta / root_delta` term (B-73).
  - Line growth: negative LMR reductions (`new_depth + 2`, `+ is_pv`) and
    re-search extras escape any budget (B-80, B-81); correction history drove
    Rarog's won-ending trees (B-82). Both are verified in Basilisk (BAS-R07:
    won-ending trees 5–30× larger, bench −21% and −25% without them). Their
    Elo is unmeasured beyond BAS-S21's `CoreLmrNegativeAllowed 0` read of
    −5.6 ± 9.8.
  - Optimism needs A.11.1 and read -5.6 in Rarog (B-83); draw jitter read -1.2,
    so measure draw blindness first (B-84); MultiPV lines 2+ search a full
    window (B-85). The mate stop is D.1's (B-78).
  Code audit inputs (2026-10-27 second pass, §6.2):
  - S4: the root is never reduced (`:1137`, `:1121`, `:1268`); the donor
    reduces root moves after the first at depth 2 or more, so every
    Basilisk root move after the first is searched null-window at full
    depth. One bit; time-to-depth at fixed nodes and the root re-search
    rate; SPRT.
  - S6: the root's first move comes from the shared TT (`:444` is not
    special-cased at the root; root stores at `:1411`), not from the
    root's own previous PV as the donor forces (`rootMoves[pvIdx].pv[0]`):
    helpers store the same key at other depths and a fail-low root stores
    its best-scored move (B-12). A counter of root TT move ≠ the last
    iteration's best, split by fail-low and other, at 1T; the 4T read
    and the helper side are D.2's.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - A third form for the line growth above (B-80): at most one search
    beyond the nominal depth from a negative reduction per line, counted
    in the node frame (TS-1). On Triumviratus's Stockfish-19 search it cut
    6.6% of nodes and 0.55 plies of selective depth at fixed depth and
    read +3.88 ± 3.26 over 11,384 games at 12+0.12. Stockfish after 19
    bounds the same growth by ply against root depth instead. It sits
    between today's `+2` ceiling and BAS-S21's ceiling-off arm (−5.6 ±
    9.8): first the distribution of chain lengths at fixed depth beside
    BAS-R07's counters, then one paired run of the count cap against the
    ceiling, only if chains of two or more are material.
  Search-entry correctness, added 2026-11-05 so that it lands before the
  fits (`analysis/rarog_sweep_2026-11-04.md`):
  - The ply cap (Rarog NODE-8): `MAX_PLY` is 128, and search and
    quiescence return the static evaluation at the cap even in check and
    before the draw test (`search_kernel_core.cpp`, both `ply >= MAX_PLY`
    returns); Stockfish 19 returns a draw in check there and its cap is
    246. Lines reach seldepth 112 already (B.12's KBP-K). A defect, owned
    here with the line growth: first a counter of cap entries per game at
    `3+0.03`, `10+0.1` and `60+0.6` with the in-check share, then the
    donor's in-check draw. Raising the cap moves the mate band
    (`MATE_SCORE − MAX_PLY`, a standing contract) and the per-ply arrays
    with it.
  - From D.3.2: a test that every entry path (null move, ProbCut, the
    singular verification, quiescence) starts a clean child frame (TR-4:
    Triumviratus's ProbCut child initialised one of five per-ply stacks,
    so a sibling's captured piece, history bucket and extension count
    leaked in), and a deep-line stack test at the cap (A-25).
  - Priors from that base: a decisive aspiration fail-high (+0.7 ± 4.2),
    root depth recovery after a fail-high (−1.82 ± 4.54) and a cap on root
    reductions (−5.80 ± 5.93) did not pay (TS-15). Printing the final line
    after a stop, so that bestmove heads the last PV, is B.9.1's (TR-3).
  - Research closed `READY_FOR_IMPLEMENTATION` 2026-11-08 on the entry
    head `ad17d2b` (`analysis/b5_cluster4_research_2026-11-08.md`; run
    on the `R3` model at the maintainer's request). No engine source
    changed; the reads used a throwaway census build at `bench 13`
    4,144,790 (packet §13).
    - Zero-game reads (packet §2), `suite_v2` at depth 14: 53.7% of the
      nodes searched inside a window lie in windows that failed (B-70),
      1.50 re-searches per window; the donor's root reduction would
      reduce 44.1% of root moves after the first by 2.0 plies (S4); the
      root's TT move equals the last iteration's best at every first
      window and differs after 1.1% of fail lows, so S6 is not a
      one-thread defect; 51.6% of negative-reduction extensions fall on
      lines already holding one (TS-1's population); the ply cap is not
      reached at depth 14.
    - The won-ending battery passes with no stop at `60000+600`: every
      search under 0.21× the maximum, the same nodes with and without
      the stop. BAS-R07's rec1 ends at depth 20 in 3.8 s on the won tier,
      so the 2026-10-25 falsifier for "the cascade is gone" fired and the
      battery re-read it. At `180000+2000` rec1 thinks 2.46× the optimum
      through the give-up window, with ten fail highs on the previous
      best all before the optimum: the hole of
      `analysis/b5_failhigh_stop_2026-10-25.md` §2.3, the stop never
      firing. M1 stays as registered and runs on the arm (BAS-R10).
    - In the gate (a) and `10+0.1` games, 0.76% and 1.51% of the head's
      moves spend half the clock left or more, three to one in lost
      positions (D.1's); 77.6% and 77.8% of draws are by repetition, the
      input B-84's blindness read takes.
    - Decisions: the donor's root-move record, its aspiration loop (the
      average as centre, the mean square as width, growth 47/128, beta
      to alpha after a fail low, no give-up hatch) and its root reduction
      as one arm behind `B5_ROOT`, in ten tickets (packet §10); the LMR
      window term's null-window unit corrected so the fitted regime does
      not move with the window (`B5LmrWindowUnits`); `B5NegExtMax` 0/1/2
      (default 0) and `B5FailHighReduce` (default 0) as the categoricals;
      the in-check draw at the ply cap on both arms, the cap kept at 128
      (raising it moves the HCE's won tiers into the tablebase band and
      overflows the table's depth byte); optimism not built (an
      NNUE-blend term; waits on F); the draw jitter waits on a judge read
      of the repetition draws.
    - The denser conversion floor is registered (packet §11): the four
      families over the 25-budget grid, KQ-K, KR-K and KBB-K at least
      598 of 600 each, KBN-K at least 575 of 600 with the five-budget sum
      at least 112; `floors_grid.py` is promoted in ticket 0.
    - Seeds (packet §7): `B5AspBase` 1, `B5AspMsqDiv` 2874, `B5AspGrowth`
      47, by B.0's 0.282 ratio. Surface for the fit: those three plus
      the Cluster 1 to 3 coordinates whose firing share moves by 10% of
      itself (packet §5.8).
    - IDs reserved: BAS-R10 (M1), BAS-S53, BAS-S54, BAS-S55; the
      predictions are frozen in packet §11.
  Split into leaves 2026-11-08 (maintainer, on packet §16) in the
  standard cluster shape (PROCESS *Cluster delivery shape*): B.5.1
  research, B.5.2 implement, B.5.3 diagnostics, M1 and the paired
  run, B.5.4 sweep and SPSA, B.5.5 gates.
    - [x] **B.5.1** `[R3]` Research and handoff on the entry head
      The packet above (`analysis/b5_cluster4_research_2026-11-08.md`),
      closed `READY_FOR_IMPLEMENTATION` 2026-11-08 on `ad17d2b`: the
      zero-game reads, the battery and the games census, the
      mechanism decisions, the seeds, the denser floor, the
      predictions and the registrations.
    - [x] **B.5.2** `[I2]` Implement behind the umbrella switch
      The packet's §10 contract behind one umbrella option `B5_ROOT`,
      both arms building, in its ten ordered tickets; the OFF arm
      keeps `bench 13` 4,144,790 at every commit; its cheap
      qualification in full; accepted by an independent review that
      reproduces the fingerprints before diagnostics start.
      - Returned 2026-11-09 at ticket 3 (packet *Return 1*): at the
        seed `B5AspBase` 1 and growth 47 the width cannot grow
        (`47 × delta / 128` is 0 at 1 and 2), so the loop does not
        end on 3 of 300 WAC positions; where it ends, 14.03
        re-searches per window, 80.1% failed share and WAC 236 fail
        P3 and P6. Every repair read ends the loop and restores WAC,
        and all stay above P3's 58% failed share. Tickets 0 to 3 are
        committed; tickets 4 to 9 wait on a research amendment.
      - Research amendment 1, 2026-11-09 (packet *Research amendment
        1*; reads registered before they ran, on a throwaway probe):
        the width is carried in thousandths of a unit and rounded up
        at use, `B5AspBase` 1410 (the donor's 5 × 282) on 500–40000,
        growth and divisor unchanged; LMR's window term keys its unit
        on the node type. Nothing is left unfinished at any of 15
        points, the ranges' four corners, WAC at depth 10 or the cost
        screen to depth 25; the probe reads `bench 13` 4,022,487, WAC
        249 and 2.93 re-searches per window, and ticket 3 is to
        reproduce them. The seed stands. Two won positions of 106
        give back 17% of the OFF head's depth-14 nodes at every
        width; on the other 104 the seed costs 20% more nodes than
        today's loop and base 19000 costs 15% less, so the width is
        read by games in B.5.3. Tickets 4 to 9 stand; ticket 3
        resumes on packet §A1.5.
      - Implemented 2026-11-09 at `c6f2440` (packet *Implementation
        record*), the OFF arm 4,144,790 at all 24 commits and the arm
        `bench 13` 3,920,652. Ticket 3 reproduces the probe on every
        figure. From a clean tree: CTest and the sanitizers pass on both
        arms; WAC at 100,000 nodes 263 (the floor 246); the floors grid
        passes; nothing unfinished at the seed or the four corners; the
        cost screen passes. Ticket 8 landed on a fixture. Open for the
        review, first: §10's battery line misses. The arm reaches the
        maximum on two won endings at `60000+600`, the stop on or off,
        where the OFF arm reads at most 0.17. The searches finish,
        no switch owns it, and at `3+0.03` the arm's long searches are
        no worse than the OFF arm's. Recorded under §10 and not returned.
        Awaits the independent review; CI has not run.
      - Accepted with named follow-ups 2026-11-09 by an independent
        review in a separate session (packet *Review — B.5.2 accepted
        with named follow-ups*). Every fingerprint and test reproduces
        from a clean tree on both arms and every flavour: `bench 13`
        4,144,790 and 3,920,652, `bench 10` sanitizer 1,310,057 and
        1,212,940, CTest 28/28 and 26/26, WAC 149 and 199 at depth 6 and
        253 and 263 at 100,000 nodes, the floors grid, the census and
        the cost screen; six commits rebuilt on both arms, ticket 3's
        two forms among them. The code is §10 as §A1.5 amends it, the
        OFF arm edited only at shared seams.
        - The battery's miss reproduces and does not gate this leaf:
          §10 marks it recorded, and both long searches are one
          open-window iteration that no stop setting can reach, as
          packet §5.4 describes. It is systematic, though. On 60 won
          endings from the `10+0.1` games the arm reads 5 searches over
          half the maximum at `60000+600` against the OFF arm's 1,
          spends a third more clock at both clocks and changes its root
          move 1.7 to 1.8 times as often. No switch owns it, and
          `B5NegExtMax` 1 reads none over half. At `3+0.03` twenty
          replayed games spend the same on both arms, the arm's stop
          ending 25.1% of searches against 10.8%.
        - Three statements of the record are corrected by reads made
          in the review: reduced root moves are seldom searched again
          because they seldom fail high (0.17%); `cont_hist_pruned`
          halves because it fires inside the OFF arm's fail-high
          cascades on a few decided positions; and the long searches
          are not scatter.
        - The follow-ups sit in B.5.3, B.5.4 and B.5.5 below. CI has
          not run; the arm's cell prints its fingerprint and asserts no
          value.
        - Decided 2026-11-09 (maintainer, taking the review's
          positions): no engine change now; research amendment 2
          before M1; B.5.3 then measures and its games choose any
          repair; D.1's clock test inside an iteration is not pulled
          forward. No new leaf: each follow-up sits in the leaf that
          owns it (B.5.3, B.5.4, B.5.5, D.1 and D.2).
    - [x] **B.5.3** `[V]` Diagnostics, M1 and the unfitted paired run
      The packet's zero-game screens on both arms (BAS-S53), M1's
      replay on the arm (BAS-R10) deciding the fail-high stop's
      threshold by its registered rule, the blindness judge read of
      the repetition draws, the unfitted paired run of the whole arm,
      then the two categoricals by paired runs.
      - B.5.2's research amendment 1 (packet §A1.5) puts one more
        paired run between the whole-arm run and the categoricals:
        `B5AspBase` 19000 against 1410 on the tune build, the width
        read by games. It sets the width the categoricals are read
        at and where B.5.4's SPSA starts; it moves no default.
        Agreed by the maintainer 2026-11-09.
      - Research amendment 2, 2026-11-09 (packet *Research amendment
        2*; its two calibration reads registered before they ran),
        answers B.5.2's review. No engine source changes. Its §A2.5
        binds this leaf:
        - M1's battery gate is differential. A stop option passes when
          it reads no more than 2 searches over half the maximum and
          10% of spend above the stop at 1× on the same tree; a search
          over the bound at 1× is the hole's and gates no option. The
          hole is any root search ending after twice the optimum
          without the stop firing, split by whether it began before
          the optimum, read on both arms by score band.
        - Before M1, a battery screen (`tools/diag/won_battery.py`,
          promoted from the review's script, with a self-test): 120
          decided endings and 60 positions better by 300 to 1,500, all
          unseen, and the packet's six, at `60000+600` and `10000+100`,
          on both arms, at the stop's three settings, at `B5NegExtMax`
          1 and at `B5AspBase` 19000. It gives M1 its gate and the
          packet's §9 item 1 a zero-game read; it moves no default.
        - A games census (`tools/diag/pgn_time_census.py`) on every
          paired PGN: time a move by score band, and the games a side
          had in hand and did not win. M1's replays also report root
          best-move changes and the effort shares on both arms.
        - Why no repair now: on the head and its base a side whose own
          score reached 1,500 won all 2,260 such games at both clocks
          (4 of 2,353 not won at 800), so the arm's extra clock in
          decided endings is waste and not lost points, on the
          evidence. No stop setting moves the arm's long searches (the
          same four of 60 at 0, 100 and 200).
        - Order: the tools and the two entries; the node-budget
          screens; the battery screen on a quiet host; M1; the NPS
          read; the whole-arm paired run, which does not wait on M1;
          the width run; the categoricals.
        - Calibration owed in the entries: P4's re-search share, P8's
          firing rate and P11's low-effort share. A6 to A13 are frozen
          in the packet's §A2.6.
      - Opened 2026-11-09 on research amendment 2 (BAS-S53 and BAS-R10
        *RESULT*), at the head `d07380d`:
        - Tools, each with a test that failed first:
          `tools/diag/won_battery.py` (`9c96740`, `2662a5e`, `efb87ff`),
          `tools/diag/pgn_time_census.py` (`c671a25`, `0236a58`),
          `replay_games.py`'s new rows and `tools/diag/replay_read.py`
          (`ada160f`), and `find_spin.py`'s node count (`23a080a`). The
          launchers are `tools/run_b53_m1.ps1` and
          `tools/run_b53_games.ps1` (`8ce1e50`).
        - Entries: BAS-S53 and BAS-R10 are registered (`d07380d`).
        - Builds at `d07380d`, each reading its `bench 13` directly (ON
          3,920,652, OFF 4,144,790): release pool build 1 `A1D8CE88…` and
          `64E4ECC7…`, Tune `D8D9BC67…` and `6DD0B156…`, Diag `DD0258A8…`
          and `63C9C26F…`; pools `b53-on` and `b53-off`, four builds each.
      - Node-budget screens 2026-11-09, on both arms:
        - The floors and the cost screen pass. WAC at 100,000 nodes reads
          263 against 254.
        - P3 misses on the failed share (69.6%, above its falsifier's 58%),
          so `B5AspBase`'s sweep is read before the whole-arm run is
          blamed. P4's mean hits (0.924) and its share misses. P5 hits on
          nodes (−6.06%) and misses by 0.12 plies on selective depth. P6
          misses by one, high. P7 hits.
        - The blindness read finds 0 of 885 repetition draws with the
          repeating side at +150 or more, so there is no H.2 card.
        - §9 item 2's gate is met (45.9% against 20%), so its paired run
          is owed.
      - Battery screen 2026-11-09, on a quiet host (BAS-S53 *RESULT*):
        - The anchor exposed a fault in the new tool. `won_battery.py`
          had turned `Diag` on, which slows a search about 8% and moved
          the OFF arm's 60 positions 5%. With `Diag` off the anchor holds
          (`efb87ff`), and whether the stop fired is read in separate
          columns.
        - A9 misses and its falsifier fires: on 120 unseen decided
          endings the arm spends 1.07 of the OFF arm, with 8 searches
          over half the maximum on each, mostly different positions. So
          the review's 60 overstated the excess at this clock. Nothing in
          §A2.5's contract rests on the excess, so the leaf is not
          returned; the research owner reads it.
        - A10 misses low on W's spend (0.94) and holds on its other two
          clauses. A11 hits (0.74, 3 over half).
        - M1's gate: stop 200 passes. Removal fails on W's spend, mostly
          one position, so A7's "both pass" misses and the Remove branch
          is closed.
      - Handed over and returned 2026-11-09, maintainer-run (BAS-R10 and
        BAS-S53 *RESULT*):
        - M1, with the OFF arm at stop 0: the rule sends the arm to 2×
          against 1× (the stop ends 23.42% of searches at 1×), so BAS-S55
          (c)'s SPRT is owed in B.5.5. The hole is 6.28% of the arm's
          searches, so D.1 owes a research card; the clock test is not
          pulled forward, since the arm left fewer games unconverted.
          A6 hits, A7's rule hits, A8 misses high on the won band, P8
          misses, P9 misses by one game, P11 hits.
        - The NPS read: the arm is 0.989×, which misses P10 by 0.1 point;
          its falsifier does not fire.
        - The whole-arm run: +3.3 ± 9.3 Elo over 2,000 games, so P1 hits
          and nothing is ablated. A12 hits. In won positions the arm
          spends half its clock or more on 3.6 times the OFF arm's share
          of moves, which is (e)'s trigger if it carries to (d)'s games.
      - The width run (`B5AspBase` 19000 against 1410), prepared and
        handed over 2026-11-10 and maintainer-run: −1.0 ± 9.1 Elo over
        2,000 games, no fault. By §9's rule 19000 is not adopted, so
        §9's items are read at 1410 and B.5.4's SPSA starts the base at
        the seed. A5 misses and its falsifier fires: the fixed-depth
        node read does not carry to games on this coordinate.
      - §9 item 1 (`B5NegExtMax` 1 against 0 at 1410), prepared and
        handed over 2026-11-10 and maintainer-run: −3.6 ± 9.1 Elo over
        2,000 games, no fault. By §9's rule the cap stays at 0 and 2 is
        not read. P5's paired clause and P12's item-1 clause hit. The
        cap halves the won band's half-clock share (0.97% against
        1.94%), which nothing registered judges.
      - §9 item 2's WAC veto, read 2026-11-10: 269 at 1 against 263 at
        0, above the floor 246, so it cannot veto.
      - §9 item 2 (`B5FailHighReduce` 1 against 0 at 1410), prepared and
        handed over 2026-11-10 and maintainer-run: −10.9 ± 9.4 Elo over
        2,000 games, no fault, the interval excluding 0. By §9's rule the
        reduction is rejected and the re-search stays at full depth. A4
        misses on both clauses; P12's item-2 clause misses. If B.5.4
        moves `B5AspBase` above 2820 or under 705, BAS-S55 (c) reads it
        again.
      - Closed 2026-11-10 (BAS-S53 and BAS-R10 *RESULT* and *PREDICTION
        CALIBRATION*). B.5.3 leaves the width at 1410, where B.5.4's SPSA
        starts, and both categoricals at 0. Item 1 was not rejected, so
        `B5NegExtMax` 1 stays B.5.5's first remedy for the long-clock
        condition. M1's rule owes B.5.5 the SPRT of 2× against 1×, and
        D.1 owes the hole a research card. No default moved and no
        engine source changed. CLOSED.
    - [x] **B.5.4** `[V]` Curvature sweep, then SPSA in blocks
      The packet's registered sweep on the three window seeds and the
      Cluster 1 to 3 coordinates §5.8 pulls onto the surface, then
      SPSA in blocks (BAS-S54); the fitted-constant analysis precedes
      the bake into `BASILISK_B5_CORE_DEFAULTS`.
      - From research amendment 2 (packet §A2.5): `B5AspBase`'s SPSA
        step is the smaller of the generator's and half the start value
        (705 at the seed; the generated 2,469 exceeds the seed's
        distance to its lower bound and clamps), and the generator
        takes the rule with a test. The packet's §5.8 joiner rule is
        read by the median position with the total beside it:
        `cont_hist_pruned`'s −51.6% is a few decided positions.
      - Opened 2026-11-10 on the B.5.3-closed arm at `d07380d`
        (`B5AspBase` 1410, both categoricals 0; Tune `D8D9BC67…`,
        `bench 13` 3,920,652):
        - The generator takes §A2.5's step rule with a test
          (`850050f`): `B5AspBase` steps 705 at the seed.
        - BAS-S54 is registered, with the joiner read and the sweep
          frozen before either runs (`analysis/b54_sweep_2026-11-10.md`):
          18 rows for §5.8's median rule; the base at nine points and
          the growth at three, at the seed and the cheapest base;
          `bench 13`, WAC at 100,000 nodes and `suite_v2` at depths 14
          and 17 a point; Rarog's classification on WAC.
      - Joiner read and sweep 2026-11-10, agent-run, zero-game (BAS-S54
        *RESULT*):
        - Five rows join by the median: `CoreContHistPruneSlope`,
          `CoreCorrUpdateScale`, `B3NmpPerPly`, `B3SingMargin` and
          `B3IirMinDepth`. Five more move in total only, carried by two
          won positions, and do not join.
        - `B5AspBase` reads curved on WAC, though its column spans only
          261 to 265; `B5AspGrowth` monotone toward 32. None is flat.
        - The surface is eight coordinates (`2beb0bd`). Block 1 is
          prepared (seed 54001) and its dry run accepted. Maintainer-run
          overnight.
      - Audit 2026-11-10, at the maintainer's request (BAS-S54 *AUDIT
        READ*): the arm's code read against `sf_19` again and block 1's
        preparation verified again (all eight coordinates live at one
        step either side of the seed; the dry run reproduces). Nothing
        to repair. One finding, on the order:
        - The fail-high stop's rate is set by the window the fit moves.
          On 60 replayed games the stop ends 27% of searches at
          `B5AspBase` 1410 and 10% at 19000, and `TmEffortLo`'s share
          falls from 36% to 18%. The stop's 2× gate sat after the fit.
        - Decided 2026-11-10 (maintainer): the stop's gate is played
          first, on the seeds, and the fit runs on its winner. It is
          BAS-S55 (c)'s stop bracket: SPRT `[0,3]`, cap 6,000 pairs,
          seed 55003, one Tune build, `tools/run_b55_gates.ps1 -Gate
          stop`. H1 moves the arm's default to 200 and rebuilds and
          re-pins before block 1; H0 or the cap leaves block 1 as
          prepared.
        - The gate is prepared: its dry run accepted, its guards read
          both ways. Maintainer-run.
      - The stop's gate 2026-11-10, maintainer-run (BAS-S55 *RESULT*):
        H1 at 8,260 games, +9.3 ± 4.4 Elo for the stop at 2×, no fault
        or time loss, the PGN recount agreeing. Applied as registered:
        - `B5FailHighStop` is 200 on the arm through
          `BASILISK_B5_CORE_DEFAULTS` and 100 off it (`a4637b6`);
          `bench 13` is unchanged on both arms and CTest passes on both.
        - The Tune build is rebuilt and re-pinned (`D3E1B9E9…`, clean at
          `a4637b6`) and block 1 is registered again on it, its dry run
          accepted (`877DD4D6…`). Surface, seeds, steps and stop rule
          are unchanged. Maintainer-run.
      - Block 1 2026-11-10 to 2026-11-12, maintainer-run (BAS-S54
        *Block 1*): 60,000 games, the PGN recount agreeing. Rule 7c:
        CONTINUE, three movers (`B3SingMargin` 232 → 201, `B5AspBase`
        1410 → 2202, `B3NmpPerPly` 13 → 11); no rail.
        - Two time forfeits at iterations 19 and 20, during a Windows
          Update install, break PROCESS's zero-fault rule for tune
          blocks. They shift any centre by at most 0.0144 steps, which
          changes neither the verdict nor a mover; the maintainer
          accepted the block as an exception (2026-11-12).
        - Block 2 is prepared (seed 54002, `-SeedFrom` block 1) and its
          dry run accepted (`0C05CB5A…`). Maintainer-run.
        - At the maintainer's request (2026-11-12), a diagnostic of
          block 1's centres against the seeds, BAS-S56: 6,000 games on
          the block's binary, seed 56001, `tools/run_b54_block1_match.ps1`,
          dry run accepted. Not a gate.
      - Block 2 withdrawn unrun 2026-11-12 (maintainer), before BAS-S56's
        first game: a second block buys a few Elo at most, and gains that
        size wait for Phase H. Rule 7c's CONTINUE is overridden for this
        tune, not changed. Theta is block 1's rounded centres
        (`tuned-options.txt` 09b3a055…), so BAS-S56 reads the whole fit.
        Next: the fitted-constant analysis, then the bake into
        `BASILISK_B5_CORE_DEFAULTS` with the arm's new fingerprint.
      - BAS-S56 2026-11-12, maintainer-run: theta +10.8 ± 5.3 Elo over the
        seeds over 6,000 games, so gate (b) is retired by its rule.
      - Closed 2026-11-12 (BAS-S54 *The fitted-constant analysis and the
        bake* and *PREDICTION CALIBRATION*;
        `analysis/b54_constants_2026-11-12.md`). No rail, no clamp; the
        singular margin and the null move's entry keep moving fit after
        fit, for B.6.4 and B.6.7. `891edc0` bakes theta on the arm: ON
        `bench 13` 4,585,613, equal to the Tune build's replay of theta,
        so gate (b)'s condition holds; OFF unchanged at 4,144,790. CTest
        28/28 on both arms. CLOSED.
    - [x] **B.5.5** `[V]` Cluster 4 gates
      SPRT `[0,3]` for the cluster, and each categorical and
      attribution arm at the bounds the packet registers (BAS-S55);
      then the flip, the OFF arm deleted and the fingerprint record
      moved.
      - From B.5.2's review and research amendment 2 (packet §A2.5):
        the flip carries a long-clock condition read from games. On
        (d)'s 2,000 games at `10+0.1`, by the games census, it fails
        when the arm's games decided at 300 and not won exceed the OFF
        arm's by more than the larger of 6 and three times the square
        root of the OFF arm's count. A 400-game run at `60+0.6`, (e),
        is played only on a trigger (the battery's undecided stratum,
        or the arm's won-band long thinks at 1.5 times the OFF arm's)
        and with the maintainer's agreement at this leaf's opening. A
        failed condition holds the flip: first `B5NegExtMax` 1, if its
        paired run did not reject it, then the hole's repair pulled
        from D.1.
      - At the pull request the arm's printed fingerprint is read,
        since its CI cell asserts no value; the counter-reading tests
        run in no CI cell.
      - (c)'s stop bracket, 2× against 1×, is played before B.5.4's fit
        (maintainer 2026-11-10; BAS-S55) and not again here.
      - Gate (b), the fit against the seeds, is retired unrun if BAS-S56's
        6,000 games read a 95% interval excluding 0 on the positive side,
        the bake's `bench 13` reproducing the Tune build's count with
        theta set; otherwise it runs as registered (maintainer
        2026-11-12, BAS-S56). Both held (2026-11-12: +10.8 ± 5.3, ON
        `bench 13` 4,585,613), so gate (b) is retired.
      - Opened 2026-11-12 on the baked arm (BAS-S55 *GATES (a), (d) AND
        (e)*): final-PGO builds of both arms at `2c248bb` (ON 148104E9…,
        `bench 13` 4,585,613; OFF 77221608…, 4,144,790), pinned in
        `tools/run_b55_gates.ps1` (`45fa9be`).
        - (a) SPRT `[0,3]`, cap 4,500 pairs, seed 55001; (d) 2,000 games
          at `10+0.1`, seed 55004, with L1 read by the games census; (e)
          400 games at `60+0.6`, seed 55005, only on a trigger. Their
          dry runs are accepted from the clean tree and the launcher's
          guards read both ways. Predictions G3 to G5 frozen.
        - (e)'s battery trigger read zero-game on fitted Tune builds:
          stratum W at `60000+600`, the arm 1 search over half the
          maximum against the OFF arm's 1, so it does not fire; the
          census trigger is read after (d).
        - Maintainer-run: (a), then (d).
        - 2026-11-12: (a) H1 at 2,231 pairs, +16.8 ± 6.1 Elo; (d) +20.9
          ± 8.5 at `10+0.1`. **L1 fails**: 48 unconverted games at 300
          on the arm against the OFF arm's 28, past the bound of 15.9.
          (e)'s census trigger fires (won-band long thinks 6.82% against
          1.30%). The flip waits; the registered first read is (d) again
          with `B5NegExtMax` 1 on the arm (BAS-S55 *RESULT for (a), (d)
          and (e)*).
        - The remedy is prepared (BAS-S55 *L1's first remedy*): both arms
          built at `833659f`, the cap's default 1 on the arm (`bench 13`
          3,783,135; OFF 4,144,790 unchanged), pinned as `-Gate dcap` in
          `tools/run_b55_gates.ps1`; (d)'s run file and seed; dry run
          accepted, guards 13 of 13; G6 to G8 frozen. Maintainer-run.
        - 2026-11-12: the remedy reads +20.3 ± 8.9 at `10+0.1`; **L1 fails
          again** (32 against 18, bound 12.7), and (e)'s census trigger no
          longer fires (2.00% against 1.39%). The cap removes the long
          thinks; the conversion gap, about 1 Elo, stays. By §A2.5 the
          hole's repair is pulled from D.1 with the maintainer's
          agreement; the flip waits on the maintainer.
        - The maintainer waives L1 and adopts the cap (2026-11-12); the gap
          goes to D.1's hole card. (e) is not owed on the candidate: its
          census trigger reads 1.44 times and its battery 0 over half
          against 1. Gate (a) is played again on the capped pair: SPRT
          `[0,3]`, cap 4,500 pairs, seed 55006, `-Gate acap`; dry run
          accepted, guards 17 of 17; G9 frozen. Maintainer-run; on H1 the
          flip follows, and the packet's ceiling row moves with it.
      - Closed 2026-11-12 (BAS-S55; maintainer-run, every count recounted,
        no fault):
        - (a) on the capped arm **H1, +15.6 ± 5.9 Elo** (5,006 games)
          against the OFF arm; (d), the remedy's read, **+20.3 ± 8.9** at
          `10+0.1` (2,000 games); the fit alone +10.8 ± 5.3 (BAS-S56). L1
          waived by the maintainer; (e) not owed; (b) retired, (c) closed.
        - The arm's seven unconverted games at 800 in (a) are drawn
          endings the evaluation scores at +9 to +12; a finding for the
          evaluation's ending knowledge, judging nothing here.
        - WAC at 100,000 nodes reads 259 on the capped arm (floor 246);
          depth 6 solves 189 (floor 186).
        - So `B5_ROOT` defaults ON at `B5NegExtMax` 1 (`fe107e4`), and the
          fingerprint record moves to 3,783,135. The OFF arm is deleted
          (`c7c6326`); the tests bind the default alone with their check
          counts unchanged (release 217, Diag 294 in `test_b5_root`), and
          CI's ON-arm cell goes with it. A fresh default build reads
          3,783,135 and passes Release and Diag CTest 28/28.
        - `00024ee` sets the line-limit test's open leg to 0 itself, the
          precondition the cap's default had changed (Diag only).
        - The packet's ceiling row (§14) records the cap as a departure
          from `sf_19`, with the ply bound after 19 for Phase H.
    - [x] **B.5.6** `[R3]` SPSA method investigation and redesign
      Added 2026-11-12 (maintainer), after B.5.5 and before any later
      tune (B.6.7, C.10): a proper investigation of how Basilisk runs
      SPSA, ending in a registered redesign of rule 7c and PROCESS
      *SPSA go/no-go*. Expectations are not inputs; the evidence decides.
      - The questions:
        - Blocks or one long decaying schedule? Blocks exist because
          the gain decays, so a coordinate that starts moving late can
          never travel as far as one that moved early. That rests on an
          unverified premise: that moving some coordinates early unlocks
          others. Test it before keeping or dropping blocks.
        - The stop: movement counts against measured Elo. Candidates
          include a long schedule paused at registered checkpoints, each
          checkpoint's centres matched against the previous one, and the
          tune stopped when the gain falls under a registered line.
        - How movement is read: by windows, in step units, against the
          sign-randomisation null (below), not by net travel.
        - Steps, `r_end`, games per iteration and the estimator (final
          centre or an average), each against the evidence.
      - The noise line, read 2026-11-12 on BAS-S54's block 1 (zero
        games): every coordinate's move at an iteration has the same size
        in its own step units (spread under 1e-4), with a random sign.
        For a coordinate with no gradient, a window's travel is noise
        with SD √Σ move², one number per window for all coordinates. It
        depends on the gain schedule and the scores, not on the
        coordinate's step. The investigation sets the familywise line
        (|z| over 3 at 24 tests read here) and makes the read a tool.
      - Evidence in hand: B.2.3's two blocks (`b23-spsa-block1`, `-2`)
        and their matches (BAS-S24 +57.0 for block 1, about +2 more for
        block 2, BAS-S27); B.5.4's block 1 by thirds (BAS-S54) and
        BAS-S56; Rarog rule 7c's sources (RAR-S75, RAR-S78, RAR-S82–S83).
      - Cheap reads to consider: block 1's centres at iteration 1000
        against its final ones; whether coordinates idle in B.2.3's
        block 1 moved in its block 2, the unlock premise's direct test.
      - How others run it, read as practice and not as authority:
        Stockfish's fishtest SPSA, OpenBench's SPSA, Triumviratus
        through its pinned snapshots, and other strong open engines'
        tuning records.
      - Closed 2026-11-12 (`analysis/b56_spsa_method_2026-11-12.md`,
        BAS-S57; zero games; the maintainer's sign-off the same day):
        - The reads, on the five blocks on disk. A tune's own games give
          every coordinate a gradient z with an exact null, the
          equivalent of a match resolving one perturbation to about ±0.9
          Elo per 60,000 games. The unlock premise is not supported: two
          of 27 idle coordinates woke in B.2.3's block 2 where chance
          gives 1.2. A restart is a higher gain by another name. One
          block covers about a third of the distance at a curvature of
          1 Elo per step², which is why coordinates drift one way across
          fits.
        - Outside practice (fishtest, OpenBench, Triumviratus) is plain
          SPSA on a fixed budget with no stop rule; no other method has
          evidence at these surface sizes.
        - Rule 7c and PROCESS *SPSA go/no-go* items 5 and 6 are
          rewritten: one run to a ceiling, twice the perturbation at half
          the ratio, read at checkpoints by the gradient test and
          stopped when quiet; the gate is a fixed 6,000 games when the
          tune's own read is +10 Elo or more, the SPRT otherwise.
        - BAS-S57. Sizing steps by tree effect is not adopted: the
          pooled rank correlation is +0.24 against a line of +0.40, and
          `bench 13` answers a one-step move chaotically. Two-step
          perturbations run without a fault and stay within a factor of
          2 in nodes at 96 of 97 points. 10 of the head's 74 coordinates
          lack two steps of room, so the perturbation is graded by room.
        - The Elo forecast is recorded and scored and decides nothing
          yet. It has no usable test: B.2.3's block 2 has no match of its
          own, and the "+2" above is the difference of two reads, known
          to about ±23.
        - The tooling is B.5.8. Changes to Colosseum wait for one real
          tune under the new rule (the packet's §9).
    - [x] **B.5.7** `[R2]` Two games per core on SMT siblings
      Added 2026-11-12 (maintainer), after B.5.6: whether game gates
      can run two games per physical core, one on each SMT sibling by
      explicit affinity, and keep their meaning. Today fourteen
      single-thread games run on fourteen physical cores, each slot
      holding both siblings; with pondering off one engine per game
      thinks, so each core's second sibling sits mostly idle (PROCESS
      *Harness*). Fair conditions for both arms are necessary but not
      enough: the gate must also measure what today's gate measures.
      - Two forms, each against today's conditions:
        - 28 games at `3+0.03`: about twice the games per hour, each
          engine at about 0.63 of a core (Rarog RAR-P54), so an
          effective control near `1.9+0.019`.
        - 28 games at a control lengthened to today's nodes per move,
          about `4.8+0.048` if 0.63 holds: about 1.26 times today's
          games per hour at the same effective depth. The factor is
          read on this host (per-sibling NPS under the 28-game load
          against the 14-game slot), not taken from Rarog.
      - Comparison data: B.5.5's (a) at `3+0.03` and (d) at `10+0.1`,
        on the same frozen builds. A new form replays the same pair on
        the same book; the questions are whether it reads the same Elo
        within error, and how many hours each form needs to the same
        decision. An SPRT's estimate is biased by where it stopped, so
        the comparison is planned on fixed-length runs, or reads (a)'s
        games as a sample with that bias stated.
      - Whether B.5.5's pair is a good test case is investigated before
        it is relied on. A change's Elo can move with effective depth
        (pruning, reductions, extensions, time use) and with memory
        traffic, which a busy sibling partly hides, so a sibling-paired
        run may price a memory-heavy change cheaper than a full core
        would. Cluster 4 (root and aspiration) is one kind of change;
        (a) against (d) shows how much its Elo moves with depth. Name
        the bad cases (a depth-sensitive change, an NPS-costly change,
        a TT- or memory-heavy change), check whether frozen pairs in
        the ledger cover them, and build a deliberate one, such as an
        arm that only costs NPS, where none does.
      - Hazards to read: time losses inside the 20 ms margin with 28
        engines and Colosseum sharing the two free cores; sibling noise
        as the spread of per-game NPS; placement by explicit
        `--placement` lists, one logical CPU per game, checked with
        `host_sampler.ps1`; SPSA's fifteen two-arm slots.
      - The decision: keep today's conditions, or adopt a form for
        SPRTs (SPSA, NPS reads and multi-thread runs unchanged unless
        read separately). Adopting one changes PROCESS *Harness*'s
        registered conditions and the wrapper's placement guards, with
        a calibration that ties gates before the change to gates after
        it. Games are maintainer-run.
      - Prepared 2026-11-12 (BAS-S58), on the design the maintainer chose
        the same day:
        - The premise corrected: today's gates run 15 games, one per
          physical core 1–15, with CPU 0's core free (the run records);
          the fourteen above was the fastchess path's sizing. The forms
          are 28 (cores 1–14, cores 0 and 15 free) and 30 (cores 1–15),
          1.87 and 2.0 times today's games an hour, both at `3+0.03`
          with the effective control allowed to change. The lengthened
          control is not read.
        - Colosseum 0.5.0 cannot place two games on one core. A
          prototype runs them: `--slot-per-logical-cpu` on Colosseum's
          branch `smt-slot-prototype` (`e27a201`), pinned for these
          reads alone; a release follows only if a form is adopted.
          `colosseum.ps1 -Placement siblings` drives it, its guards
          tested 20 of 20.
        - Maintainer-run, in order, with `tools/run_b57_reads.ps1`:
          `-Read speed`, a zero-game read of about 40 minutes
          (`tools/smt_read.py`: each engine's share of a core, a
          memory-latency and a compute pair that search identical trees,
          and Stockfish standing in for a network engine); then `-Read
          g28` and `-Read g30`, B.5.5's capped pair over gate (a)'s 2,503
          openings (seed 55006), about 27 minutes each, read against (a)
          opening by opening (`tools/diag/replay_compare.py`). Dry runs
          accepted; predictions and the direction rule frozen.
        - A promising form goes to a separately registered resolving
          stage; nothing is adopted on direction reads alone.
        - 2026-11-12: the speed read is VOID by its rule, one Stockfish
          window over the disturbance line. Its classical phase is clean:
          an engine keeps 0.56 of a core at 28 and 0.54 at 30, and both
          speed pairs move under 0.6 pp. Stockfish keeps to its pin; the
          extra load was foreign work on the one-per-core layout's idle
          siblings, which the data cannot remove. Amendment 1 reads the
          NNUE phase again alone (`-Read nnue`, about 13 minutes),
          disturbance read as foreign work, and adds the games' nElo
          ratio against the break-even (√(15/N)), reported. Then g28 and
          g30.
      - Closed 2026-11-12, NO_CHANGE (BAS-S58, NOT_WORTH_PURSUING; the
        maintainer's decision the same day): gates keep 15 games on 15
        cores at `3+0.03`.
        - g28 and g30 replayed B.5.5's capped pair over (a)'s openings:
          +10.8 ± 6.1 and +10.5 ± 6.1 Elo against (a)'s +15.6 ± 5.9, in
          26.7 and 25.0 minutes against 49.8. 30 fails its rule on two
          time forfeits with one core left free. 28 passes its game
          items, but its engines search 0.58 of the nodes and 1.6 plies
          less, and the cluster's nElo fell to 0.67 of (a)'s against a
          break-even of 0.73: an SPRT would decide about ×0.84 as fast.
        - The resolving stage the rule would send 28 to is not
          registered, a stated departure; the NNUE re-run is not played.
          The Colosseum prototype stays unreleased on its branch and the
          pin stays at 0.5.0; the tools stay for a retry.
        - Retry: a host or engine where two siblings do 1.3 times a
          core's work, or an nElo ratio at break-even on a resolving
          sample.
    - [x] **B.5.8** `[I1]` SPSA read tool and checkpoint wrapper
      Added 2026-11-12 from B.5.6, on the maintainer's sign-off: the
      tooling rule 7c's redesign needs, before any later tune (B.6.7,
      C.10). The contract is `analysis/b56_spsa_method_2026-11-12.md`
      §8. No engine change and no games.
      - `tools/spsa_read.py`, replacing `tools/spsa_block_rule.py`: from
        a run directory, also while the run is live, every coordinate's
        gradient z per window, the familywise line, the live or quiet
        verdict with its floor, rails, the tune's own read of its gain
        and the frozen forecast. Its fixtures are the five blocks' reads
        in `tools/results/b56-20261112/` and synthetic journals of both
        verdicts.
      - `tools/generate_spsa_surface.py`: the perturbation for a
        registered ceiling, graded by each coordinate's room as rule 7c
        states, with the coordinates under two steps listed.
      - `tools/spsa_colosseum.ps1`: the ceiling, `r_end` and the
        checkpoints as arguments in place of the fixed block of 2,000;
        a loop that runs to each checkpoint with Colosseum's
        `--stop-after-iteration`, applies the read, and resumes or
        stops. Every guard is tested on synthetic results of both
        verdicts before any unattended run.
      - PROCESS *Harness* and its main-path commands follow the tools.
      - Colosseum is unchanged. The test in its driver, a held gain with
        a windowed average and a gain ratio per coordinate wait until
        the read has held on one real tune (the packet's §9).
      - Closed 2026-11-12 (no engine change, no games; smoke runs of 2
        games an iteration on 2 slots only):
        - `tools/spsa_read.py` reads a run at a checkpoint or live, from
          the pinned runner's `spsa history` and games.jsonl. Its signs
          are the runner's own, regenerated from the master seed
          (ChaCha12; Colosseum's golden vectors reproduced), and every
          recorded centre of the five blocks is checked as the runner's
          update of the one before. Verdicts: CONTINUE, STOP_QUIET,
          STOP_CEILING, STOP_RAIL, FAULT, and INTERIM off the grid;
          theta at a stop. 20 tests: the runner's vectors, the
          statistics and the forecast floor, synthetic runs of every
          verdict, and the recorded blocks (24 after the rail decision
          below).
        - The recorded reads: B.3.3, B.4.6 and B.5.4 reproduce B.5.6's
          to the digit. B.2.3's block 1 does not: its window 1,000 to
          2,000 is a rail (`CoreIirMinDepth` z +5.30 at its maximum,
          clamped on 67% of iterations), not quiet. B.5.6 recovered
          signs from the moves and lost the clamped iterations; the
          packet's erratum (§14) records it, and §4's unlock read is
          unchanged.
        - `tools/generate_spsa_surface.py --rule7c` writes a run's tune
          file at its ceiling, the perturbation graded by room, `c_end`
          rounded down so no first arm leaves its room; the head reads
          10 of 74 coordinates short of two steps, 3 of them of one, as
          BAS-S57 counted. The checked-in surfaces are unchanged.
        - `tools/spsa_colosseum.ps1` plays the run a checkpoint at a time
          with `--stop-after-iteration` (exit 6 is the pause), refuses
          faults, applies the read and resumes or stops; a `STOP` file is
          the maintainer's stop. 13 guard and route tests on a real run
          paused at its first checkpoint, every verdict forced through a
          shim around the real reader (`tools/results/b58-20261112/`);
          the production dry run is accepted. `spsa_block_rule.py` is
          removed; PROCESS *Harness*, its commands and the tool READMEs
          follow.
        - The rail first read z's sign alone, so a coordinate parked at
          a bound with no gradient stopped a tune half the time (B.2.3's
          block 2 read a rail at z +0.91). The maintainer's decision the
          same day: a rail passes the familywise line, the live test's own;
          an outward z of 2 or more under it is a candidate, listed and
          deciding nothing. No user-facing threshold remains. 24 reader
          tests on the final state; the guard and route suite reruns
          13 of 13. The five blocks at 2,000: B.2.3 block 1 STOP_RAIL
          (`CoreIirMinDepth` z +5.30), unchanged; block 2 STOP_RAIL to
          STOP_QUIET, the one change; B.3.3 STOP_QUIET, B.4.6
          STOP_CEILING and B.5.4 FAULT (its two recorded time forfeits)
          unchanged.
        - Reviewed 2026-11-12 in a separate session, against the packet's
          §8: the reader's 24 tests and the surface's 26 reproduce, the
          guard log reads 13 of 13, and the real smoke run reads through
          the pinned runner. Four corrections to the reader, none to the
          wrapper or the generator; 28 reader tests on the final state:
          - The tune's own read of its gain read high. Its gradient's
            denominator left out iterations whose D came out 0, which
            selects on the outcome. The form came from B.5.6's packet,
            which the reader had reproduced faithfully. Over 1,000
            simulated runs with a known gradient B.5.6's form reads
            1.106 times the true gain (+0.51 ± 0.16 on 4.78 Elo), the
            corrected one 0.995 (−0.02 ± 0.14;
            `tools/results/b58-review-20261112/`). The five blocks'
            reads fall by 11% to 16%: B.2.3 +29.8 and +2.4, B.3.3
            +11.6, B.4.6 +7.0, B.5.4 +9.95 (the packet's §14).
          - A forecast was scored against twice its own SD. The gain
            read it is scored against is noisier (null SD 3.3 to 8.3
            Elo over a block, against a forecast SD near 1 to 2), so a
            perfect forecast would have failed. It is now scored
            against the combined SD, and rule 7c says so.
          - `--record` on a read between checkpoints wrote a record. The
            wrapper takes the record's last line as the tune's state
            and would have refused to resume; the forecast would also
            have been scored. Such a read is now printed and not
            recorded.
          - At the ceiling the floor claimed "the forecast's 2 SD is
            0.0 Elo" for an empty segment. It now claims none.
          - A fault left no path to a theta after the maintainer
            accepted it, as B.5.4's two forfeits were. The maintainer's
            decision the same day: acceptance stays the maintainer's,
            made after the checkpoint that read the fault. `spsa_read.py
            --accept-faults "<reason>"` records the accepted counts and
            the reason; the tune's command then reads that checkpoint
            again for its real verdict, kept in the record after the
            FAULT one, and resumes or stops by it. A fault beyond the
            accepted counts stops the tune again. 29 reader tests; the
            wrapper's 13 guard and route tests rerun and 8 routes of
            the acceptance on a copy of the paused smoke run
            (`tools/results/b58-review-20261112/`). PROCESS *Harness*
            holds the rule.
          - Left as built for B.6.7's preparation: `--rule7c` writes
            whole tables only, where B.6.7's surface is a chosen
            subset.
- [x] **B.6** Engine audit, reconciliation, then joint search SPSA
  Joint search SPSA over the coordinates the four clusters left live;
  registered surface; PGO bake; SPRT `[0,3]`. It runs (maintainer
  2026-11-05: for the assurance that nothing larger was missed); the
  curvature evidence chooses its surface, and a flat surface is recorded,
  not a skip. Before it, the reconciliation (B.6.6, added
  2026-10-31 by the maintainer), so the surface is fitted on the forms
  the evidence chose.
  Before both, since 2026-11-11 (`analysis/rarog_import_2026-11-11.md` §4):
  the search-path perft and the kernel's pins (B.6.3), one audit of the
  whole engine as four clusters left it (B.6.4) and its repairs (B.6.5).
  A repair that moves the tree after the fit sends the fit back, so every
  one lands first. B.6.1 and B.6.2 are retired numbers: the
  reconciliation is B.6.6 and the fit B.6.7 (HISTORY's number map).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Before any surface: a both-ways relevance check on every `Core*`
    coordinate (B-105); the surface chosen by a firing-rate change of at least
    10% or a scale move above 5%, not by cp units alone (B-102); rails widened
    (B-103); a seed-against-perturbation clip check (B-104); coordinates with
    a switch boundary floored or read as switches (B-64).
  - Judging: an SPSA journal z-read of net travel against a null (B-101);
    movement is not Elo (B-100); Rarog's joint tune after fitted clusters
    bought +4.5 for 120k games (B-106).
    - [x] **B.6.3** `[I1]` Search-path perft and kernel pins
      Instruments the audit and the fit stand on; no engine change and
      the exact fingerprint. Each test is shown to fail on a planted
      fault, which is then reverted.
      - The search-path perft (from B.7.1 on 2026-11-11; TB-1, its
        `tdperft`; Rarog's D.4.3): perft through the search's own staged
        picker, make and unmake, in every stage set. The main and evasion
        sets deliver every legal move once and no other; the quiescence
        and ProbCut sets deliver exactly the moves their rule names. At
        every node the keys, the occupancy and the piece table
        are recomputed from scratch, and every parent move is checked
        again by the pseudo-legality test the table move uses. Rarog's
        caught five planted faults. It runs in CTest at the standard
        depths, among them position 3 at depth 6 (11,030,083) and
        position 4 at depth 5 (15,833,292).
      - Pins for what the kernel does today, so that a later change to
        any of them is a decision (F10 of
        `analysis/rarog_import_2026-11-11.md`; Rarog's NODE-14):
        - the node type each recursive call passes: the null child, its
          verification, ProbCut's two searches, the exclusion search,
          the reduced search and both re-searches;
        - one named predicate for each meaning of "capture" (the move
          loop's, which counts promotions, and correction training's,
          which does not), with their agreement over the legal moves of
          a corpus, castling quiet in both;
        - the table keeps its move when a store passes none;
        - IIR's condition, and the main search's refusal of a table
          cutoff from a rule-50 count of 96.
      - B.7.1 uses the perft as its safety net and builds no second one.
      - Closed 2026-11-12 (`bench 13` 3,783,135 unchanged; CTest 30 of
        30 in the release build):
        - `tests/test_search_path.cpp` walks `CoreMovePicker` (main and
          evasions) over randomised histories with make and unmake, and
          reproduces every count: startpos 5, Kiwipete 4, position 3 at 6,
          position 4 at 5, positions 5 and 6 at 4 (35 s; a build with
          assertions walks shallower). At every node `Board::assert_ok`
          recomputes keys, occupancy, piece table and checkers, unmake
          restores the position, and the parent's moves are rerun through
          the table move's test, as are the grandparent's (same side to
          move) at interior nodes: 1.45e9 rechecks. Every delivered move
          passes that test. At interior nodes quiescence's captures, its
          quiet checks (each move played), its in-check cut and ProbCut's
          SEE-cleared candidates each match their rule. Each node's table
          move comes from its key: none, legal or the grandparent's. All
          four picker sources deliver.
        - `tests/test_kernel_pins.cpp` links a test-only engine library
          built with `BASILISK_CHILD_LOG` (`src/search_child_log.h`; its
          hooks are empty in every other build). It pins the node type
          that each of the eleven call sites passes over 24 searches:
          every negamax call is named, every site is reached, and a
          site that reads the caller's type is seen from both. It also
          pins `is_capture_stage` against `captures_piece` over 1.18M
          legal moves, castling quiet in both, plus `b3::iir_applies` and
          `b3::tt_cutoff_rule50_allows`. The kernel's tests moved
          unchanged into these named predicates and `table_move_usable`
          (`move_picker_core.h`, `search_b3.h`).
        - `tests/test_tt.cpp`: a store without a move keeps the entry's
          move and takes none from another position's.
        - Thirteen planted faults, each caught, then reverted: a dropped
          bad capture, a repeated table move, a stale minor key, castling
          through an attacked square accepted by the table move's test, a
          missed discovered check, an underpromotion in ProbCut, the null
          child typed cut, an unnamed call site, promotions counted as
          captures by correction training, castling counted in the capture
          stage, IIR's stale bound off by one, a table cutoff at 96, and
          a store without a move dropping the entry's.
    - [x] **B.6.4** `[R3]` Engine audit on the four-cluster head
      One read of the engine as Clusters 1 to 4 left it, by a session
      that implemented none of it, before anything is fitted on it: the
      2026-10-27 audit's method (`analysis/code_audit_2026-10-27.md` §1)
      on the head B.5.5 leaves. Each cluster was reviewed against its
      own handoff; nothing has read them together. No engine source
      changes; the deliverable is a packet whose findings each carry a
      failing test or a counter.
      - Scope, read in full: the node kernel and quiescence, the picker,
        the histories and the correction, the table, the root driver and
        the inputs it hands the clock, the pool. The protocol is
        B.9.1's, the board B.7.1's, the evaluator's verdicts B.9.3's.
      - Two lenses, as in 2026-10-27: chess and search semantics
        (AGENTS, *Scope and discoveries*), and C++: undefined behaviour,
        overflow, lifetime, concurrency, dead or misleading code.
      - The interaction map across clusters (maintainer 2026-10-30):
        each producer, its stored state and its consumers, with the
        head's counters; a signal two clusters both read or both write
        is named.
      - Inputs to confirm or refute, from
        `analysis/rarog_import_2026-11-11.md` (F-numbers are its §3):
        - F4: the multi-cut correction bonus's product in `int`. The
          test trains at an extreme depth and residual. With it, the
          width of every history and correction update at the ply cap
          and a mate residual (TR-5; Rarog's D.5.23, where a wrapped
          product applied a full update of the wrong sign).
        - F5: quiet under-promotions as captures. A count of the
          quiescence and capture-stage nodes they add, and of the cases
          where one is searched before the queen's push.
        - Rarog's NODE-4: inside the exclusion search IIR, ProbCut and
          LMR's eval term read the table-refined estimate. A counter of
          exclusion nodes where that changes a decision.
        - The frame a re-entry rewrites (the exclusion search, the null
          verification): `move_count`, `stat_score` and the
          grandchildren's cutoff count, as the donor's does. A counter
          of children that read a rewritten value; Triumviratus read
          restoring the move count at about ±0.5.
        - The fail-low store beside IIR: Rarog's change to the donor's
          store lost −14.7 ± 13.8 nElo because its IIR reduced the nodes
          the store left without a move. Basilisk's IIR fires at PV and
          cut nodes only; the read is its rate at revisited fail-low
          nodes.
      - The idiom list (maintainer 2026-11-11: modern C++23, or a later
        standard where it pays): the constructs a current idiom would
        make smaller or safer, each with what it buys and what it risks
        in the pooled NPS read; and G.2's C++26 decision read again
        against the compilers then pinned. Behaviour-neutral items go
        to B.8.2.
      - Routing, stated per finding:
        - a defect that moves the tree is B.6.5's, before the fit;
        - an unrecorded departure from the donor is a row of B.6.6;
        - a mechanism worth about 5 Elo or less waits in H.2;
        - a protocol or lifecycle defect joins B.9.1's list;
        - a behaviour-neutral change is B.8.2's.
      - It ends with frozen handoffs for B.6.5, which confirm, split or
        replace that leaf, or with `NO_CHANGE`.
      - Closed 2026-11-12, `READY_FOR_IMPLEMENTATION` for B.6.5
        (`analysis/b64_engine_audit_2026-11-12.md`; no engine source
        changed, `bench 13` 3,783,135):
        - Read in full: the kernel, the picker, the histories and the
          correction, the table, the driver and the pool, against the
          donor's `search`, `qsearch`, updates, table and picker at
          `sf_19`. Counters come from a patched build that reproduces
          the fingerprint, read on `bench 13`, `bench 17` and 610
          self-play searches (`tools/results/b64-audit-20261112/`).
        - **No defect moves the tree.** F4 is confirmed: the product
          wraps from a singular depth of 54 and trains the correction by
          the full clamp in the wrong direction (−256 for +256); the
          cause is the unit conversion before the products. The reads
          came within a factor of nine of it. The widest other update,
          the fail-low parent's continuation bonus, tops out at 97.6% of
          `INT_MAX` on literals (87.0% seen).
        - A second defect, in the clock's inputs: `ponderhit` recomputes
          the budget with the game ply of the node the search is at
          (soft limit +4.9% to +11.3%). B.9.1's.
        - F5 is a departure, not a defect: under-promotions add at most
          0.19% of nodes in quiescence. In the main search the queen's
          push comes first unless the table move or the exclusion says
          otherwise; quiescence's selection is not stable, and 2 to 4%
          of its under-promotions are searched before the queen's push.
        - NODE-4 narrows: only LMR's alpha term reads the refined
          estimate in an exclusion search (0.04% of reductions change).
          The larger departure is the other way: the exclusion search
          skips razoring and reverse futility, which the donor runs
          (0.41 to 0.47% of exclusion searches).
        - The frame: the null verification restores `move_count` and
          `stat_score` (the donor does not); the exclusion search
          restores nothing, as the donor. Its own fail high, counted as
          a child's, moves 1.3 to 3.1% of all reduction decisions.
        - The import's "no path here" for the fail-low store beside IIR
          is refuted: 13 to 18% of IIR's reductions are at revisited
          fail-low nodes whose entry has no move. It is the donor's
          pair and B.4's gate ran with it.
        - Unrecorded departures: the TT-cutoff quiet bonus has no cap
          (the donor's is `min(112·depth, 695)`; 15 to 31% of bonuses
          exceed it), the tie rule's store, four of the table's rules.
        - G.2's C++26 decision stands: the pinned Clang 23 has neither
          reflection nor contracts.
        - Routing: the packet's §10; each receiving leaf carries its
          items below.
    - [x] **B.6.5** `[I1]` Audit repairs before the fit
      The expected shape; B.6.4's handoff confirms, splits or replaces
      it. Each confirmed defect is repaired in its own commit, with the
      test that failed before it: release and sanitizer CTest, the exact
      fingerprint where the tree does not move and a recorded move where
      it does.
      B.6.4's handoff (2026-11-12,
      `analysis/b64_engine_audit_2026-11-12.md` §12) confirms the leaf
      and narrows it. No repair moves the tree, so the screen, the
      floors and the repair SPRT below do not run and the leaf closes
      on its tests. The class is `I1` since then: the change is
      specified to the operation and is about thirty lines (it was `I2`
      against a repair that moved the tree).
      - The multi-cut correction bonus becomes a named function in
        `search_b3.h`, the same operations in the same order in 64
        bits. Tests: 256 at the three overflowing points and −256 at
        the mirrored one, and agreement with the 32-bit form on a grid
        where that is defined.
      - The continuation update's incoming bonus is bounded by 32,776:
        a compile-time proof for the fail-low parent's chain and a node
        check on every call, absent from release builds.
      - `bench 13` 3,783,135 exactly; each test shown to fail on its
        planted fault. No NPS read is owed and there is no game gate.
      - Not this leaf's: the TT-cutoff bonus's cap (B.6.6), the frame
        (H.2), `ponderhit` (B.9.1).
      - Before any game, each tree-changing repair passes the
        deep-iteration cost screen
        (`tools/diag/deep_iteration_screen.py`) and the conversion and
        WAC floors.
      - The tree-changing repairs are gated together in one repair SPRT
        `[-5,5]` at `3+0.03`, its cap sized from their measured exposure
        (`agents/measurement.md`). A correct repair that loses is
        reworked, with the neighbours fitted around the defect joining
        B.6.7's surface; it is never reverted to the wrong form (PLAN
        §2, *Known defects*).
      - With no tree-changing repair the leaf closes on its tests.
      - Closed 2026-11-12 on its tests (`bench 13` 3,783,135 unchanged;
        CTest 30 of 30 in the release build, 28 of 28 in the ASan/UBSan
        build; the packet's *Implementation record*):
        - `b3::multicut_correction_bonus` (`search_b3.h`) is the kernel's
          expression with the gap widened to 64 bits first; the
          multi-cut site calls it under its present condition.
        - The continuation update's weights and multipliers and the
          fail-low parent's scale terms are named in
          `search_history_core.cpp`, the quiet stat score's weights in
          `history_core.h` (`QuietStatScore`, which the kernel now
          calls). A `static_assert` derives the parent's largest bonus,
          31,978, from them and the history limits, against the bound
          `INT_MAX` / (largest weight · largest multiplier) = 32,776.
          `CORE_NODE_CHECK` moved to `search_worker.h` and checks the
          bound on every call; it is compiled out of release.
        - `tests/test_b3_proof.cpp`: 256 at the three overflowing points
          and −256 at the mirrored one; agreement with the 32-bit form
          at 8,085,737 grid points, 0 differ.
        - Planted faults, each caught and reverted: the arithmetic in
          `int` (four failures, −256 for +256), the order of operations
          changed (1,889 points differ), the weight 520 raised to 540
          (the `static_assert` stops the build).
    - [x] **B.6.6** `[R2]` Donor-form reconciliation
      It was B.6.1 until 2026-11-11.
      After B.4 and B.5 close, every place the search still keeps
      Basilisk's form where the donor's differs is re-read against the
      donor's on the current head, now that the features and fits those
      forms waited on have been tried. The input is the departures
      tables: B.3's (written by B.3.4), B.4's and B.5's from their
      packets, and B.2's collected from its packet; each row names
      Basilisk's form, the donor's, why it was kept and the feature or
      fit it waits on. A row whose prerequisite has landed and been
      accepted gets one 2,000-game paired run at `3+0.03`, the donor's
      form against Basilisk's, after the conversion and WAC floors as
      vetoes, and the donor's form is adopted at ≥ +5; a row whose
      prerequisite was rejected or never built stays and says so. A row
      that reads under +5 is not cleared by that run, which is tilted
      toward the form the arm was fitted with (maintainer 2026-11-04): it
      waits in H.2 for the fair re-read with both arms refitted. The
      adoptions land together behind one switch and pass SPRT `[0,3]`
      against the head before B.6.7.
      Rows added 2026-11-05, donor departures no packet had recorded
      (`analysis/rarog_sweep_2026-11-04.md`):
      - The TT key carries no rule-50 bucket. Stockfish probes and stores
        with the key adjusted by the rule-50 count once it reaches 14
        (`adjust_key50`), so bounds are not shared between positions with
        very different fifty-move horizons; the repetition key stays raw
        (A-14's test). Rarog parked it while games were adjudicated, a
        reason neither engine has now; it matters in the long played-out
        endings every gate now has. The child TT prefetch uses the
        adjusted key. Quiescence's table cutoff has no rule-50 guard
        at all, where the main search refuses from a count of 96 (F11
        of `analysis/rarog_import_2026-11-11.md`).
      - The non-pawn correction key leaves the kings out (`board.cpp`,
        `nonpawn_key`); Stockfish's includes them. A bare king's key is 0:
        one entry for every position where a side has a bare king, which
        carried part of the KBP-K correction drift (B.3.0's packet §1.4).
      Rows B.6.4's audit adds (2026-11-12,
      `analysis/b64_engine_audit_2026-11-12.md` §6, with each row's
      counter):
      - The TT-cutoff quiet bonus: `CoreTtCutoffBonusSlope · depth`
        with no cap, where the donor's is `min(112·depth, 695)`. It
        exceeds the cap in 15 to 31% of bonuses and reached 11,929 in
        self-play; from depth 48 one cutoff sets a main-history entry
        to its limit. **This row runs**, before B.6.7 registers its
        surface, because the slope is a coordinate and the form decides
        what it fits: a categorical on a tune build (the cap at 695,
        off by default), with a `10+0.1` leg beside `3+0.03`, since
        the form scales with depth.
      - The exclusion search: no razoring and no reverse futility
        (the donor runs both), and the estimate refined by the table
        (the donor's is the static eval). No run: 0.41 to 0.47% of
        exclusion searches, 0.04% of reductions.
      - The tie rule's store: an upper bound and no move where the
        donor stores the tied move, exact at a PV node. No run: 0.04 to
        0.09% of interior nodes, and a score equal to alpha is a bound.
      - The null verification restores `move_count` and `stat_score`;
        the donor does not. With H.2's frame item.
      - No clamp on a node's depth at entry (the donor's
        `MAX_PLY − 1`). Nothing.
      - The quiet under-promotions' class (F5) is read as a departure,
        not a defect; B.4's row stands and its re-read waits in H.2.
      The table's four departures (replacement ageing, the move of a
      declined store, the PV bonus and secondary aging, no store at a
      node without moves) are D.2's.
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
      a reconciliation that reaches past `sf_19`, or a later donor re-pin,
      reads first what did not pay on Triumviratus's Stockfish-19 base:
      Stockfish's post-19 linear all-node razoring (−2.32 ± 2.52 over
      20,032) and null threshold after sibling null fail-highs (+0.46 ±
      2.66 over 17,308) (TS-16), and four edge cases other engines handle,
      each flat over about 7,000 games (TS-17).
      - Prepared 2026-11-12, `GAME_GATE`
        (`analysis/b66_reconciliation_2026-11-12.md`, BAS-S59):
        - All 39 rows sorted (§3). The fail-low store is the donor's form
          since B.4. Four rows run, the maintainer's choice: the TT-cutoff
          cap (`3+0.03` and `10+0.1`), razoring in the donor's form (a
          B.2 departure no table had: any depth, `482·d²`, no guards), the
          rule-50 key with F11, and kings in the non-pawn key. The rest
          stay with their owners: H.2, D.1, D.2, F, B.9.1, B.6.7's surface,
          or nothing.
        - Four switches, off by default (`1b7256a`, `bench 13` 3,783,135):
          `CoreTtCutoffBonusCap`, `CoreRazorDonor`, `CoreTtKey50`,
          `CoreCorrNonPawnKings`. Tune build `9AE3C8C9…`; every switch
          moves the tree.
        - Vetoes before any game: every arm passes the 25-budget grid and
          the correctness suite, and reads WAC at 100,000 nodes within 5
          of the head's 259 (268, 259, 257, 261).
        - Five dry runs accepted. The maintainer runs the chain of §7.3;
          `tools/run_b66_games.py` applies the vetoes again before each.
      - Runs read 2026-11-13 (maintainer-run overnight; BAS-S59 *RESULT*,
        packet §8): all five clean, the PGN recount agrees.
        - Adopted: razoring in the donor's form, +10.6 ± 9.4; kings in
          the non-pawn key, +11.6 ± 9.3.
        - Basilisk's form stays, not cleared, H.2: the TT-cutoff cap
          (+8.0 ± 9.1 at `3+0.03`, −0.5 ± 8.6 at `10+0.1`) and the rule-50
          key (−4.5 ± 8.4).
        - Next: the two adoptions behind one switch, SPRT `[0,3]` against
          the head, registered and prepared before any game.
      - SPRT prepared 2026-11-13 (BAS-S60, packet §9):
        - `B6_DONOR_FORMS` (`65448d0`, off): ON `bench 13` 3,268,437,
          OFF 3,783,135. Release CTest: OFF 30 of 30; ON fails only
          three tests bound to the OFF tree, re-based before a flip.
          A test gap found on the way is closed in its own commit
          (`bd814cb`: the arm-default range check now covers the
          kernel switches).
        - Both forms delay a mate proof at fixed depth on one fixture;
          BAS-S59's games show no conversion cost (razoring 47
          unconverted at +300 against 38, kings 24 against 42), and
          the flip carries B.5.5's L1 census on the SPRT's games.
        - Final-PGO arms `A2BF3537…` (ON) and `07BB73F3…` (OFF); cap
          5,000 pairs; dry run accepted. The maintainer runs
          `python tools/run_b66_sprt.py`.
      - Closed 2026-11-13, `NO_CHANGE` (BAS-S60 *RESULT*, packet §10;
        `bench 13` 3,783,135 unchanged):
        - The SPRT reached its cap of 5,000 pairs undecided: +2.2 ± 4.2
          Elo, LLR +0.52, one time forfeit by the head, the recount
          agreeing. An unresolved stop is not an adoption.
        - No row adopts the donor's form. Razoring and the kings key
          (+10.6 and +11.6 alone, +2.2 together), the TT cap and the
          rule-50 key go to H.6 with their readings. B.6.7 fits the
          head's forms; `B6_DONOR_FORMS` and the four switches stay,
          off, for H.6.
        - The census reads no conversion cost from the fixed-depth mate
          delay: ON left 145 games unconverted at +300, the head 199.
    - [x] **B.6.7** `[V]` Joint search SPSA
      It was B.6.2 until 2026-11-11.
      The joint SPSA above, on the reconciled head.
      Horizon (maintainer 2026-11-12): one night, 2,000 iterations (about
      12.5 hours), which is rule 7c's first checkpoint. Phase C's
      evaluation moves the cp scale the margins act on, so one night
      takes most of what this fit can keep, and C.10 is the classical
      stage's long fit. A live read at that checkpoint does not start the
      next 1,000 iterations by itself: the read (every coordinate's z,
      the tune's own read of its gain, the forecast, rails) is discussed
      with the maintainer, who decides whether the run continues;
      otherwise theta is that checkpoint's centres. Reworded 2026-11-12
      for rule 7c's redesign (B.5.6); the decision is unchanged.
      `B3SingMargin` and `B3NmpPerPly` start known live: each still read a
      gradient when its last tune ended (B.5.6's packet, §12).
      The surface is every live search coordinate that passes B-102's
      relevance read, never only those still moving after their
      cluster's fit.
      A prior from the Rarog sweep (`analysis/rarog_sweep_2026-11-04.md`):
      a blind, uniform 15% shift of Rarog's whole selectivity surface
      toward less pruning read +4.06 ± 3.71 over 14,196 games (RAR-S54),
      evidence that block-wise self-play left it too aggressive, and led
      to its broad refit, +15.33 ± 7.34 nElo. The registration reads it
      beside the curvature sweep.
      Engine audit inputs (2026-11-12,
      `analysis/b64_engine_audit_2026-11-12.md` §6):
      - `CoreCorrUpdateScale`'s rail of 30 is below the donor's value in
        this engine's units (42.6, and 63.8 without a best move), and
        every fit has moved the coordinate up (16, 17, 18). B-103's
        widening reaches 64.
      - The correction's structural weights (15,341, 10,569, 12,906)
        and update shares (150, 186, 130, 70 over 128) are the donor's
        literals applied in this engine's units; they go on the literal
        screen's list.
      - The TT-cutoff bonus's form (B.6.6's row) is decided before the
        surface is registered.
      Added 2026-11-10 (maintainer): the kernel's own literals are
      screened before the surface is registered. A few dozen numbers the
      search reads are written into the code and are no option, so no
      tune reaches them and B-105's relevance check does not see them.
      They are the donor's values for its own network and tree: LMR's
      offsets and step thresholds (697, 2179, 940, 1127, 5234, 5487),
      its history weights (2252, 1126, 1093, 873), the correction
      history's slope, cap and update (7419, 2695, 1061), the history
      bonuses (−2210, 1334, 918 and −747) and the root record's
      averaging weights (12 to 24 of 32). The screen plays no game:
      - It lists every such literal in the node kernel, quiescence, the
        history updates and the root record, each with its donor line.
        The list is the screen's first output, and nothing is left off
        it for looking unimportant.
      - Each is exposed as an option on a tune build, `bench 13`
        unchanged, and read at 20% either side of its value by B-102's
        rule (a firing rate moved by 10% or a scale by 5%).
      - One that passes joins the surface as a coordinate. One that
        does not stays a literal, and the list says so with its read.
        The staircase lines stay off the surface as before.
      Added 2026-11-11: the screen's list of literals, each with its
      donor line, is kept as a tracked file; it is also the constants
      census B.11.3's disclosure reads.
      - Prepared 2026-11-13, `GAME_GATE` (BAS-S61; the rules are
        `analysis/b67_surface_2026-11-13.md`, the list
        `analysis/b67_kernel_literals.md`; `bench 13` 3,783,135 at every
        commit):
        - The literal screen (`bebf2ad`): 118 literals in
          `src/kernel_literals.h`, read as constants by release and tune
          builds (the release code instruction-identical to the head's)
          and advertised as `Lit<Name>` options by a `LITERAL_SCREEN` tune
          build. Release and sanitizer CTest 30/30. 115 move the bench
          replay at 20%; three are guards whose other side crashes.
        - The relevance read, zero-game: 42 of 74 coordinates and 60 of
          118 literals pass; eight rows were too chaotic to decide, among
          them both aspiration rows, so `B5AspBase` and `B5AspGrowth`
          could not be read and leave the surface by the rule.
        - Rails (`fb84591`): five ranges widened or floored, no default
          moved. The curvature sweep drops two flat coordinates.
        - The surface (`c0b3944`): 41 coordinates and 59 literals, the
          tune file generated for rule 7c's ceiling of 6,000 and held at
          its first checkpoint by `spsa_colosseum.ps1 -PauseAt 2000`
          (`c155612`, amendment 1; route tests 6/6, B.5.8's guards 13/13).
          Dry run accepted on the screen build `A15704C2…` at `6084ae0`.
        - Reviewed and amended the same evening by a second session, at
          the maintainer's request (amendment 2 in the packet, before
          any game):
          - The relevance read could not choose a surface: 81 of its 102
            passes rest on one row (continuation pruning) whose spread
            over all 381 points is 6.6%, so a pass or a fail said little
            about the subject. The surface is now every wired subject
            that is a magnitude, resolvable by a tune and not redundant:
            **170 coordinates, 69 X lines and 101 literals**. 88 of the
            100 stay; 71 the read had failed join, with `B3IirMinDepth`,
            `B5AspBase`, `B5AspGrowth` and the singular margins among
            them; 12 leave (two on or beside their floors, a guard, five
            staircases, four exact redundancies).
          - The screen gains the move picker's ten ordering literals,
            which PLAN's wording had left out (`425adcc`; `bench 13`
            3,783,135, release CTest 30/30). The screen build takes the
            continuation update's product in 64 bits, since its weights
            are run-time values the width proof does not cover.
          - Crash safety over the box: every surface coordinate at its
            tune minimum and maximum (340 points), no crash and no tree
            beyond three times the defaults'; with all 170 off their
            seeds at once, the packet's *Results of amendment 2*.
          - The tune's binary is the screen build at `425adcc`
            (`basilisk-b67-screen2-pext-tune-pgo.exe`, `2DFA7B95…`); the
            dry run was accepted on it. Conditions, schedule and seed
            are unchanged.
        - The maintainer runs the night; the read at 2,000 is discussed
          before the run continues or stops there.
      - Run 2026-11-13 to 14 and baked 2026-11-14, `GAME_GATE` (BAS-S61
        *RESULT*; `analysis/b67_constants_2026-11-14.md`):
        - The tune stopped quiet at its first checkpoint: 60,000 games,
          no fault, no rail, largest |z| 2.82 against the line's 3.61.
          The tune's own gain read is +22.9 Elo (null SD 11.6), taken
          by iteration 1,000. Theta is the checkpoint's centres.
        - The analysis: no rail or clamp; the singular margin falls a
          third fit running (201 to 160); the history updates lean on
          the malus; reductions deepen while extensions grow, the
          `bench 13` replay +13.1%; the correction's update scale and
          IIR's least depth drift up again; most literal movement is
          noise. Owners are named there.
        - The bake found a defect of the screen (`0f30bf1`): a scale
          literal was read as its own default in release builds, so its
          fitted value could not be baked. No game played it.
        - Baked (`25c570f`): 61 X lines in `BASILISK_B67_DEFAULTS`, 93
          literals in `src/kernel_literals.h`; `bench 13` **4,278,307**,
          the replay. Release and sanitizer CTest 30/30 after five tests
          bound to the old values were re-based; `test_endgames`
          191/191, the conversion grid PASS, WAC at 100,000 nodes 254.
          The forgotten-mate rule has lost its end-to-end witness on the
          fitted tree (finding 8, B.9's).
        - Owed: B.6's SPRT `[0,3]` of the bake against the B.6.6 head,
          registered on its own before its first game. The fingerprint
          record and `CHANGELOG.md` move when it accepts; a rejection
          reverts the bake.
        - Registered 2026-11-14 as BAS-S62: the baked release build at
          `6a9f81f` against the release build of `0f30bf1`, the commit
          before the bake (both final PGO, manifests clean), SPRT `[0,3]`
          at `3+0.03`, cap 6,400 pairs for an expected +8 Elo. Dry run
          accepted. The maintainer runs `python tools/run_b67_sprt.py`.
      - Closed 2026-11-14 (BAS-S62 *RESULT*): H1 accepted at game 2,620,
        LLR +2.95, first-crossing +28.2 ± 7.9 Elo (+47.9 nElo), no fault,
        the PGN recount agreeing. The bake stands: the fingerprint record
        is `bench 13` **4,278,307** and `CHANGELOG.md` has the entry. B.6
        closes with it. The analysis's findings keep their owners: the
        singular family and the drifting correction scale and IIR depth
        to C.10, the forgotten-mate witness to B.9, the literal census
        to B.11.3.
- [ ] **B.7** Board review and rebuild
  Added 2026-11-03 (maintainer): Basilisk's board is reviewed against
  Stockfish's (`Position` and `StateInfo`, the modern donor's form) and
  against Triumviratus's measured board ideas (the third donor), and
  rebuilt component by component where the other form measures better.
  Basilisk's form stays wherever it measures better: BAS-X16 read this
  board 22–46% ahead of a peer engine's on every microbenchmark. It runs
  after B.6 and before B.8's speed pass, so that the speed pass profiles,
  and B.11.1's NPS meter reads, the board that C's attack-heavy terms and
  F.1's board events will be built on. Numbered B.15 until 2026-11-05,
  when the open steps were renumbered in the order they run (HISTORY's
  number map). A step of Phase B rather than a phase of its
  own: phase letters are identifiers, and a phase between B and C would
  re-letter C–G. Inputs: `analysis/triumviratus_review_2026-11-03.md` (TB
  items) and the archived board leaves 7.1–7.12 (HISTORY's number map).
  Reworked 2026-11-14 (maintainer, after
  `analysis/b7_frontier_review_2026-11-14.md`): B.7 ends with the fastest
  board that can be measured here, not one that is fast enough. Its open
  leaves were renumbered in the order they run (B.7.3 is B.7.6, B.7.4 is
  B.7.9; HISTORY's number map): the instruments first, and a second round
  of candidates before the gate. Chess960 is not a goal (maintainer, the
  same day): the board is built for standard chess, C7 and TB-9 are
  withdrawn, and a later decision for Chess960 would rework the board
  then.
    - [x] **B.7.1** `[R2]` Board contract audit and instruments
      Was D.3.1, then B.15.1 (HISTORY's number map): the defect
      hunt, and the safety net B.7.6 rebuilds behind (archived 7.1, 7.7,
      7.9–7.12). Port the snapshot's board-v2 corpus and oracle
      (`tests/data/`, `tools/diag/board_v2_*.py`).
      The corpus freezes canonical FEN, legal and capture sets, perft and
      divides, keys, and restoration across normal, hinted, null, clone and
      unwind moves, plus checks and evasions, pinned en passant, every castle
      and underpromotion. Also: history capacity contracts; rule-50,
      repetition, null-boundary and mate-precedence policy audited
      separately; negative controls. A semantic defect goes through the
      correctness-repair path with its own gate.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Tests: a stored TT win never answers a repeated position (A-13); the
        clock stays out of the position identity (A-14); an external oracle
        corpus (A-5, A-24); the SEE approximations (A-2, A-3, BAS-C09) were
        repaired in B.4.2, before quiescence's fit; their fixtures join
        this corpus.
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
      - The search-board perft (TB-1, its `tdperft`) is B.6.3's since
        2026-11-11, built before the fit. It validated a board rewrite
        on Triumviratus and is this step's safety net as built.
      - A `gives_check` differential against playing the move, over a
        depth-3 walk, shown to fail without its discovered-check clause
        (Rarog's D.5.17: 305,311 moves, no mismatch).
      - A permanent Zobrist test (TB-2): the GF(2) rank of the key set is
        64 (Basilisk read 64 on 2026-11-03; Triumviratus's keys from a
        32-bit generator had rank 32 and lost a game to a false hash hit).
      - FEN robustness (TB-3): an en-passant square inconsistent with the
        side to move, castling rights that do not match the board, a side
        not to move in check; one position must never get two keys.
      Audit done 2026-11-14 (`analysis/b71_board_contract_2026-11-14.md`):
      instruments built, two defects repaired; the second moves the tree
      and waits for its gate. `bench 13` **4,278,307** through the first.
      - The corpus: 26 tagged positions checked by python-chess
        (`tools/diag/board_v2_oracle.py`), and 192 SEE cases with the
        legal exchange tree (`tools/diag/see_oracle.py`, every
        `test_board` fixture, Rarog's cases and mirrors). `test_board_v2`
        reads both. At the roots: FEN, checkers, legal, capture and
        checking sets, perft and divide equal the oracle.
      - Over 12,451 walked nodes: make, unmake and null restore every
        field; `gives_check` matches play on 9,498 checks; every FEN
        reads back to the same keys.
      - The rest: the generators' partition, clones and 160-ply unwinds;
        Zobrist rank 64 (TB-2); TB-3's FEN cases; the clock outside the
        key (A-14); `see_ge` exact on all 187 moves (A-5).
      - Negative controls show each check can fail: `gives_check`
        without its discovered clause disagrees 161 times; 32-bit keys
        read rank 32.
      - A-13 is a `test_search` fixture whose control without history
        shows the stored claim answering. TB-1 is B.6.3's search-path
        perft, reused.
      - Repaired: a board copy of a game longer than the reserve had no
        room for a search line, so the search grew the history at its
        first node. The copy now reserves `2 × MAX_PLY` past it, under a
        test that fails on the old code. Also repaired: `test_board`'s
        SEE fixture "pinner stays" was an illegal position; its bishop
        moves to f3.
      - F1, repaired at the maintainer's word (2026-11-14): `is_repetition`
        scored one return to the root as a draw, where the cuckoo test
        (donor form) and Stockfish do not. It now needs a second
        occurrence at or before the root, and a test holds the two
        predicates to one answer (it fails on the head). `bench 13`
        **4,247,965**. The proven-mate fixture of `test_b5_root` moves
        with the tree (proved in 9 from depth 19, in 8 from depth 24, where
        the head proved it in 8 from depth 17).
      - Gate: BAS-S63, `[-5,5]` at `3+0.03`, cap 4,000 pairs (12.1% of
        3,000 game positions choose another move at 50k nodes, by the new
        `tools/diag/decision_diff.py`), `python tools/run_b71_gate.py`.
        Maintainer-run 2026-11-14: the cap without a decision, 8,000
        games, +1.9 ± 4.1 Elo, LLR +2.29, no faults. The repair stays (the
        cap rule, confirmed by the maintainer) and the fingerprint record
        moves to **4,247,965**. CLOSED 2026-11-14.
      - For B.8.2 (F4): `Board::see` has no caller left in `src/`.
    - [x] **B.7.2** `[R2]` Board review against Stockfish and Triumviratus
      A packet that reads each component against the donor's form and
      Triumviratus's, measures Basilisk's (its share in A.9.5's profile at
      `bench 13`, a microbenchmark per component), and decides keep,
      replace or adapt, each with its reason and expected cost; a
      departures table records every kept Basilisk form. Its frozen handoff
      is B.7.6's contract.
      Added 2026-11-14 (maintainer: the board's speed is the point of B.7):
      - Its first deliverable is the board microbenchmark, so that every
        component is read on it before B.7.6 changes it and again after.
        It times each primitive alone, as Rarog's board-v2 bench did: legal
        generation, capture generation, staged generation, make and unmake
        alone, SEE alone. It adds `gives_check`, the check information, the
        legality test of a TT or killer move, and a perft through the
        search's generator.
      - Its inputs are built before timing, every sample is printed, and a
        checksum keeps the compiler from eliding the work. The timed binary
        is a pinned release build, read on an idle host (the maintainer
        says when).
      - `tests/board_performance.cpp` (cross-engine-board-v1) stays as it
        is, for its cross-engine comparability.
      - Donor techniques, with what each source measured, are inventoried in
        `analysis/b7_board_speed_inputs_2026-11-14.md`: Stockfish 19's
        board, Triumviratus's TB items and Rarog's measured board work.
        B.7.2 reads each against Basilisk's component. A technique no
        source measured is a candidate to measure, not to adopt, and a
        donor result measured as a loss counts as much as a gain.
      Components:
      - Check information computed once per node at make: checkers, pins
        and blockers, check squares, as `StateInfo` holds them (Basilisk
        computes `check_squares` and pins per call), shared by the
        picker's stages and quiet scoring; the child's in-check status from
        the parent's gives-check (TB-4).
      - Make and unmake: what is copied, what is recomputed, branches on
        captures, incremental keys (TB-5); the per-ply dirty-piece record
        F.1's accumulators need, fixed once here (TB-10).
      - Legality: the legal and pseudo-legal split, the TT and killer move
        validator, `gives_check`.
      - SEE: next attacker by index, both bounds before any lookup, the
        attack-map shortcut for quiet moves (TB-6), with BAS-C09's
        approximations as B.4 repaired them.
      - Attacks: bishop and rook together where both are needed,
        empty-board tables (TB-7); BAS-P26 read compressed slider tables
        0.98% slower, so flat tables stay unless a read says otherwise.
      - Generation: the side as a template parameter, staged delivery,
        captures not regenerated (TB-8); the archived 7.2–7.8 items
        (generation and list delivery, fused relocation, pin/check sharing,
        the SEE kernel, representation).
      - Chess960 representable without a second rewrite (TB-9); the
        feature itself is G.2's.
      Done 2026-11-14 (`analysis/b72_board_review_2026-11-14.md`, its §6 the
      frozen handoff):
      - Instruments: `board_micro_bench` (`tests/board_micro.cpp`), each
        primitive alone on 512 positions of BAS-S63's games, with a
        python-chess oracle line and a checksum per column. Rarog's and
        Reckless's boards are timed under one contract
        (`tools/board_compare/`, adapters as patches on pinned commits).
        `tools/diag/board_micro_run.py` alternates the binaries on one CPU.
      - Microbenchmark, five alternating rounds:
        - Basilisk leads legal generation by 7–29% and perft by 25–61%.
        - Rarog's make/unmake is 15% faster (its make is hinted).
        - Quiet-check generation costs four times the staged pair.
      - Added the same day, at the maintainer's request: both Stockfishes,
        19 and classical, in one five-engine session. Weighted by the
        search's counted call mix, Basilisk's board costs 65.8 ns per node.
        - Rarog is within 2%.
        - Classical Stockfish is 9% dearer and Stockfish 19 26% dearer.
          Stockfish's stored check information halves `gives_check` and SEE
          and doubles or triples make.
        - Rarog's and Stockfish's SEE pass promotions and en passant
          unexamined (36 moves), the donor's rule, not a defect.
        - Quiet-check generation is never called (quiet checks are off at
          the defaults), so C4 was withdrawn.
      - Profile at `bench 13`: board 24.0% of samples. By entry point: SEE
        6.13% (pins 1.71), `make_move` 3.48%, `gives_check` 3.33%,
        `unmake_move` 2.68%, generation 6.4%.
      - Disassembly: every slider lookup is inlined, and no hot function
        probes its stack (TB-11 closed).
      - Handoff, six behaviour-neutral components, one commit each:
        - C1: unmake without the key work it discards;
        - C2: a discovered-check pre-test in `gives_check`;
        - C3: a pin pre-test in SEE;
        - C5: the repetition scan from distance 4;
        - C6: the hinted make, BAS-P04's untested half; its trigger is met
          by reuse;
        - C7: castling squares from tables (TB-9).
      - Kept: lazy check information, legal generation, the exact SEE
        pins, flat PEXT tables, repetition on demand, the representation.
      - Deferred: to B.8.5, G.2 and F.1 (the dirty-piece record, its
        contract fixed here).
      - Maintainer's decision: whether C7, which buys representability,
        is held to cost-neutral rather than +0.5%.
      Amended 2026-11-14 by the frontier review
      (`analysis/b7_frontier_review_2026-11-14.md`, R3; its §5 and §6 are
      research amendments 1 and 2 to the packet):
      - Each component was read in real search order (a replay of every
        board operation of `bench 13`) and in the engine (16 alternating
        `bench 13` rounds, plain builds): C1 +1.86%; SEE pins read from
        aligned sliders +1.56%; C2 and C3's pre-test no gain;
        `gives_check` without lookups no gain; checkers derived in make
        −0.74%; C6 no gain on the replay.
      - The handoff is C1, C3′ (the new SEE pins) and C5. C2 and C6 are
        dropped on measurement. C7 is withdrawn (maintainer: no
        Chess960), which settles the decision above.
      - "Fastest of five" stands against Stockfish 19 and Reckless.
        Against Rarog and classical Stockfish it rests on the
        composite's stand-in for the quiet stage, and B.7.9 reads it
        without one.
      - The dirty-piece record is not written by make (F.1).
    - [ ] **B.7.5** `[I2]` Board instruments: replay and engine screen
      Added 2026-11-14 (`analysis/b7_frontier_review_2026-11-14.md` §2.5,
      §3, §8). The two instruments every later leaf of B.7 reads, made
      permanent from the review's prototypes. A microbenchmark column
      prices game positions on cold boards and underprices the board by
      22% to 57%; these two price the search's own work. No playing
      change: every build keeps `bench 13` at 4,247,965. New tools are
      Python. Class I2, not I1: every later keep or refusal in B.7 rests
      on these reads, and the adapters are patches on four pinned
      engines in two languages.
      Starting point: `tools/results/b7-frontier/replay/prototype_final.patch`
      (local, SHA-256 A7C836C81DAB6F0E…, on this branch's `2231b50`).
      Only the recorder and the replayer are taken from it, no `PROTO_`
      kernel.
      - The trace build, a CMake option that is off by default and leaves
        no code behind when off: it records every operation the search
        makes on its own board, in order and with its result. Make (the
        key after it), unmake, null moves, each generation stage (the
        count), `see_ge` (threshold and answer), `gives_check`,
        `check_squares`, `is_legal`, the draw tests, `is_in_check`. A
        call a board function makes itself is not recorded. Eight bytes
        a record.
      - `board_replay`, a test target on the production board:
        - `--verify` replays every record and compares every result;
          the first mismatch is exit 1. A CTest records a short trace
          (one bench root at a node cap), verifies it, and shows the
          check failing on a planted wrong answer.
        - A timed run filters the trace to the chosen classes before
          timing (make, unmake and null moves always run), takes one
          warm-up pass and prints a checksum.
        - `--stub` is the control: the same records through the same
          switch, each case an empty out-of-line call. Board cost is the
          timed run less the stub. The stub over-corrects, so the two
          figures bracket the truth; an A/B of two boards on one trace
          needs no correction.
      - `tools/diag/board_replay_run.py`: alternates binaries and class
        lists on one pinned CPU, refuses a busy host, records hashes and
        every sample, and writes its samples before it prints. A trace
        is regenerated (`bench 13`: 58.6 million records, 469 MB), never
        committed.
      - `tools/diag/engine_ab.py`, the engine screen: alternating
        `bench 13` runs of plain release builds on one pinned CPU, 16
        rounds, the order reversed on odd rounds, the per-round time
        ratio to the first binary. It refuses a busy host and any node
        count but the fingerprint. A null pair (one build, two copies)
        is read first and gives the resolution; the review read about
        ±0.3%.
      - Cross-engine contract v3 (`tools/board_compare/`): a `quiet_gen`
        column, each engine's quiet-stage generator as its search calls
        it, in Basilisk's harness and the four adapters, so that the
        per-node composite needs no stand-in. No timing here; B.7.9
        reads it.
      - Timed reads are short (under ten minutes each) and need the idle
        host: the agent builds, then asks for it.
      - Done when verify passes at the head and fails on the planted
        answer; the head's class table reproduces the review's within
        its spread (uncorrected, ns per node: make and unmake 26.1,
        generation 29.5 more, SEE 28.8 more, `gives_check` 16.9 more);
        and the null pair reads within ±0.3%.
    - [ ] **B.7.6** `[I2]` Rebuild behind the board contract
      It was B.7.3 until 2026-11-14. B.7.2's handoff as the frontier
      review amended it (its §6), one commit each, in this order.
      Rules for every component (B.7.2's packet §6):
      - `bench 13` stays 4,247,965, and every checksum of
        `board_micro_bench` stays as
        `tools/results/b72-micro/manifest.json` records it.
      - `test_board_v2`, `test_search_path` and `board_replay --verify`
        on a fresh `bench 13` trace pass.
      - The old form runs beside the new behind a build switch and
        aborts on the first difference; it is run once under
        `test_search_path` and `bench 13` (TM-10).
      - Nothing rides along: no search, evaluation or parameter change,
        and the generation order is unchanged.
      Components:
      - C1, unmake without key work. `put_piece`, `remove_piece` and
        `move_piece` gain a board-only form (bitboards, occupancies,
        mailbox, king square); `unmake_move` uses it and restores every
        key from `UndoInfo`. Review: replay −2.1 ns per node, screen
        +1.86%. PGO may already drop part of it; B.7.9 reads that.
      - C3′, SEE pins from aligned sliders. `see_ge` computes no pins up
        front. For each colour `c` it holds `snipers(c)`: the enemy
        rooks and queens on an empty-board rook line of `c`'s king, and
        the enemy bishops and queens on a bishop line (two 64-entry
        constant tables). Where it chooses an attacker of colour `c`,
        and in the promotion-threat test, a piece is excluded when it
        alone stands between its king and a sniper that is still in the
        exchange occupancy (`BB_BETWEEN[king][sniper] & occ` is that one
        piece) and `to` is not on that line. Read on the occupancy of
        that moment, this is the set `see_pins` returns, and it replaces
        both the pinner test and the recomputation after a departure:
        `see_pins` and `see_departure_opens_line` leave `see_ge`. The
        king-legality rule keeps its unfiltered opponent set.
        `Board::see` has no caller and is B.8.2's to delete. Review:
        exact on 6.05 million recorded SEE answers; replay −4.3 ns per
        node, screen +1.56%.
      - C5, the repetition scan from distance 4 (`is_repetition`,
        `is_draw`): cleanup, no measurable effect.
      - C8, `is_in_check` and `has_non_pawn_material` defined in
        `board.h`: each is called 1.2 times per node and is an
        out-of-line call that only LTO removes. Cost-neutral cleanup.
      Each of C1 and C3′ is read on B.7.5's two instruments against the
      commit before it. It stays when its replay class is faster by more
      than twice the spread of the untouched classes and the engine
      screen is faster in at least 13 of 16 rounds. A component that
      fails returns the leaf to research with its numbers.
      Not built (review §3): C2, C3's pre-test and C6 (no gain
      measured), C7 (withdrawn). `UndoInfo` gains no field for the
      network (F.1).
    - [ ] **B.7.7** `[R2]` Second round: candidates on the instruments
      Added 2026-11-14 (maintainer: the fastest board possible at the end
      of B.7). Bounded research on B.7.6's head. Each candidate below is
      prototyped behind a macro in a scratch worktree, as the review's
      nine were, proved exact, read on both instruments, and kept or
      refused by the rule frozen here. Nothing is merged in this leaf.
      Its packet (`analysis/`) carries every candidate with both reads,
      and the frozen handoff for B.7.8 or NO_CHANGE.
      The lesson that orders the list (review §3): on this host a slider
      lookup is not the cost, the code around it is. Every kernel that
      only removed or skipped lookups gained nothing or lost; the two
      that removed a whole path gained. A pre-test is a branch.
      First reading: the profile of B.7.6's head by board entry point
      (A.9.5, as B.7.2's packet §1.2). A candidate whose target is under
      0.5% of samples is not prototyped.
      Candidates, in order:
      - Undo as one block. Make saves the changing state as one image
        (the twelve piece bitboards and the occupancies, 136 bytes; the
        keys, counters, king squares and checkers, 72 bytes) and unmake
        copies it back: no branch on the move's kind, no
        read-modify-write. The mailbox is restored by two to four
        stores. The undo stack becomes a fixed array sized for the
        longest search line, and the key history a dense array of its
        own, so that no search node allocates (B.7.1's capacity
        contract) and a repetition scan reads eight keys a cache line.
        It must hold B.7.1's restore-every-field walk, the three
        repetition tests on the new history, clones and the 160-ply
        unwind. Prior: 0 to +1% beyond C1.
      - A byte mailbox: `Piece` stored in one byte (64 bytes instead of
        256), alone and inside the image above, where it frees unmake
        of its last branches. It must hold every `piece_on` reader.
      - Keys by table in make: one row per piece and square holding the
        main, pawn, minor and non-pawn key parts, in place of three type
        tests per helper (TB-5; part of a −1.05% cycles package there).
      - Next attacker by index in `see_ge` (TB-6; −0.38% and −0.14%
        cycles there).
      - The hinted make with the search wired (C6): `make_move` takes
        the search's `gives_check` answer where it has one, which is at
        43% of makes (38.5% no, 4.5% yes). The review's replay read no
        gain and its nearest form lost 0.74%; this is the one arm it
        could not build in the engine. BAS-P04's trigger is met by the
        changed reuse.
      - One enemy attack map per node, built on first use, for the
        quiet stage's king targets (4.0 tests per generation), castling,
        the quiet scorer's threat maps and `see_ge`'s exit when nobody
        attacks the square (28% of its calls; an attack through the
        mover's origin is not in the map, so that exit needs a line
        test). Priors conflict: Triumviratus −0.35% cycles, Rarog
        −0.58% for the SEE exit, BAS-P04 −0.16% for sharing between
        stages.
      - Whatever the profile shows above 0.5% inside a board entry point
        that no candidate above covers, its mechanism stated before the
        prototype.
      Not retried (measured in the review, §3; their trigger is the
      network's cache load, H.4): `gives_check` and make's checkers
      without lookups, SEE attackers from aligned sliders, the
      generator's pins from aligned sliders (RAR-M31), C2, C3's
      pre-test. Refused on record: eager check information and pin
      sharing (BAS-P04), compressed tables (BAS-P26), the side or stage
      as a template parameter (RAR-M44(c)), incremental attack tables,
      pseudo-legal generation, copy-make, Chess960 tables.
      The rule for each candidate, frozen:
      - Exact: `board_replay --verify` on a fresh trace, the
        fingerprint, the microbenchmark checksums, `test_board_v2` and
        `test_search_path`.
      - Kept: the engine screen at +0.3% or more with at least 13 of 16
        rounds faster, and no replay class slower by more than twice
        the null spread. Read alone against B.7.6's head, then once
        more against the kept set.
      - Between the null pair's resolution and +0.3%: reported, not
        kept, unless it also removes code.
      - The round ends after one pass over the list. A second pass is
        only for a candidate that a kept one changed.
      A premise found false goes back to the maintainer with its
      evidence (R3), not into a wider prototype.
    - [ ] **B.7.8** `[I2]` Second rebuild: the candidates that cleared
      B.7.7's handoff under B.7.6's rules, one commit each, each read
      again on both instruments against the commit before it. It closes
      as NO_CHANGE if B.7.7 kept nothing.
    - [ ] **B.7.9** `[V]` Board speed read and gate
      It was B.7.4 until 2026-11-14.
      First, B.16.4's counter read is validated: BAS-P28, registered
      2026-11-05, `tools/run_b164_pmc.ps1` (elevated, idle host, about ten
      minutes; its P25 pools rebuilt by `tools/nps_pool.ps1` if gone, the
      new pool hashes recorded). Then it reads beside the pooled read.
      Then, maintainer-run on the idle host:
      - Pooled-PGO NPS (A.9.4), three arms: the head before B.7.6, after
        B.7.6, after B.7.8. Each step's set is held to +0.5%. The
        review's frozen prediction for B.7.6's set is +2.0%, 80%
        interval [+0.8%, +3.2%]; under +0.5% falsifies the engine screen
        as a predictor, and the leaf returns to research (R3) instead of
        choosing members by the screen.
      - A set that misses its floor has its members read alone (D-16),
        and a member that does not pay returns to Basilisk's form.
      - The profile of the final head by board entry point (A.9.5),
        beside B.7.2's table.
      - The five-engine read on contract v3, and the composite
        recomputed from it: no stand-in for the quiet stage; a hinted
        make charged its `gives_check` for the 57% of makes the search
        does not ask about; Basilisk charged its draw tests and the
        picker's check squares. It states, per column and per node,
        where the board stands against Rarog and both Stockfishes, and
        names any column a donor still leads, with its mechanism.
      - A component that changes the generation order, and so the tree,
        takes a `[-5,0]` SPRT at `3+0.03`; none is planned.
      It closes B.7, and DESIGN §5 takes the final figures.
- [ ] **B.8** Cleanup and speed pass
  Cleanup: remove the legacy search path, dead parameters, unconsumed switches
  and diagnostics without an owner; exact fingerprint; no game gate. Then
  the allocation guard, large pages and the speed pass (B.7 until
  2026-11-05), so that the speed pass profiles the cleaned code on the
  rebuilt board.
    - [x] **B.8.1** `[I1]` Delete the legacy search kernel (ANY TIME) — before the A.11 gates
      Done 2026-10-27 at the maintainer's request, out of turn, so that
      A.11.10 and A.11.11 are gated on one arm. Deleted: the four legacy
      kernel files with `history.h` and `move_picker.h`; every
      `BASILISK_B2_CORE` arm (resolved as defined) in `search_worker.h`,
      `search_types.h`, `tt.h`, `search_params.h`, `search_diagnostics.h`,
      `uci_protocol.cpp` and the tests; the `B2_CORE` option and its CMake
      branches; CI's legacy build-and-test step; the `+legacy` id suffix;
      the legacy-only parameter groups (node margins, move loop, LMR,
      history), so the default SPSA surface is regenerated (41 to 19
      coordinates: the root table and the three groups the kernel
      inherited). DESIGN loses its legacy-arm sentences.
      - Kept, for B.8.2: the ten Diag counters only the legacy kernel wrote
        (`lmp_prunes`, `hist_prunes`, `see_prunes`, the four `hist_*` reach
        counters, three `lmr_blocked_*`). They are in the 57-counter
        contract the oracle prints and `run_suite.py`'s LMR accounting
        identity reads, so they now read 0 and are documented so.
      - Exact fingerprint: `bench 13` 7,857,713; CTest 24/24; the Tune,
        Diag, Ablate and TEXEL flavours build, their `test_uci_protocol`
        passes, and `--verify` reconstructs all seven positions; the
        `tools/diag` tests pass but `test_profile_regions`, which fails on
        master already (`version.cpp` and `version.h` are unmapped).
    - [ ] **B.8.2** `[I1]` Dead parameters, switches and diagnostics
      What B.8.1 and B.8.6 kept (their *Kept* lines). The thirteen zero
      counters leave the counter contract only with the oracle's side of it.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
      are its items):
      - Delete the dead shallower re-search (B-63) and the probably-dead PVS
        guard (B-13, counted first); read NPS after the deletions, which paid
        +1-6% in Rarog (D-18); Rarog's cleanup pitfalls (B-120); retired
        numbers in comments go with A.10.6.
      Code audit inputs (2026-10-27, `analysis/code_audit_2026-10-27.md` §3):
      - The unreferenced `Board::compute_hash`; the engine-layer includes of
        `uci_output.h` from `bench.cpp`, `engine.cpp`, `parameters.cpp` and
        `wac.cpp` (an output sink that belongs below the protocol, or an
        injected writer); `DecisionTrace::record`'s fourteen positional
        arguments (B.2.0's F20); named helpers for `negamax`'s move loop
        (1,084 lines at the audit) only under the exact fingerprint and the
        NPS floor.
      - The census of `00809cc` sized the legacy deletion at about 1,900
        lines and a CI cell; B.8.1 did it, except the ten counters above.
      Simplification read (2026-10-28, after A.14.1):
      - Telemetry counted in release builds, where nothing prints it (the
        `Diag` option is advertised only in Tune and Diag builds,
        `parameters.cpp:191-198`): `eval_calls` and `pawn_probes`/`pawn_hits`
        (`eval.cpp:915-917`, `:1072`), the board's `diag_see_ge_calls` and
        `diag_gives_check_calls` (`board.cpp:1416`, `:1784`), and the
        lazy-audit branch, a runtime test on every lazy cut
        (`eval.cpp:1236-1248`, `:2020-2030`). All go behind the flavour
        guard; exact fingerprint, read with the NPS of the B-63 deletions.
      Engine audit input (2026-11-11): the behaviour-neutral items of
      B.6.4's idiom list, each alone with the exact fingerprint and read
      with the same NPS read; an item that costs NPS is dropped, not
      traded.
      The list (2026-11-12, `analysis/b64_engine_audit_2026-11-12.md`
      §9):
      - Idioms: one named function for the `* 1000 / 282` unit
        conversion (three sites); `static_assert` width proofs at the
        widest products; `if constexpr` flavour constants for the
        counter, trace and pricing arms; a fixed-capacity list for the
        searched moves; a scoped `TTFlag`; libc++'s debug hardening in
        the sanitizer and debug cells.
      - Dead: `SearchStack::moved_piece` (written, never read); at the
        default arm `allow_null`, `best_scored` in both quiescence
        loops, `singular_quiet_lmr`, `did_check_ext`, the older
        aspiration loop, the delta rule, the quiet checks and the small
        ProbCut, which leave with their switches.
      - Misleading comments: `search_kernel_core.cpp` on reverse
        futility returning `eval` directly and the "LMR table" heading
        above the child log; `search_worker.h` "on either kernel";
        `constants.h` "in search.cpp"; `search_types.h` "reads zeros".
    - [ ] **B.8.3** `[I1]` Allocation guard
      Before the speed pass (B.7.1 until 2026-11-05): a test with a
      counting global `operator new` searches fixed positions at two depths at
      Threads 1 and 4.
      It asserts that allocations grow per iteration, never per node, while
      the node count grows by at least an order of magnitude. A planted
      per-node allocation must fail it, checked once and reverted. No engine
      change. The snapshot's `tests/allocation_guard.rs` is the model. The
      leaf adds the guard to *Standing contracts* (A.12).
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Beside the counting allocator, a disassembly search for `memcpy` and
        allocator calls in `negamax`, `quiescence` and `evaluate` (D-23).
    - [ ] **B.8.4** `[I1]` Large pages on every platform
      Large pages for every allocation big enough to pay, on every platform
      Basilisk ships, behind a `LargePages` UCI option so that one binary
      reads both arms (Rarog RAR-M77: on read +2.7% NPS and +10.3 ± 5.9 Elo
      at 4T). Split from the speed pass on 2026-11-05 and placed before it,
      because large pages change the profile it reads. Each platform falls
      back silently, and `bench` reports on, off, partial or not available.
      Engine audit input (2026-11-12,
      `analysis/b64_engine_audit_2026-11-12.md` §9): the cluster's
      constructor (six sequentially consistent stores) gives way to
      value-initialised atomics and a per-thread fill;
      `std::start_lifetime_as_array` is missing from the pinned libc++
      23, so mapped pages still need constructed clusters.
      - Windows x64 and ARM64: `VirtualAlloc(MEM_LARGE_PAGES)` under
        `SeLockMemoryPrivilege` (D-8, pulled forward from G.1: +2.48% bench
        and +2.76% in games in Rarog).
      - Linux x64 and aarch64: transparent huge pages through
        `madvise(MADV_HUGEPAGE)` on a 2 MiB-aligned block, as Stockfish
        does, with explicit `MAP_HUGETLB` as the alternative (maintainer
        2026-11-04).
      - macOS arm64, the only macOS build: Apple silicon has no userspace
        superpage request (the 2 MiB superpages macOS offers exist on Intel
        Macs alone) and its base page is already 16 KiB, so `bench` reports
        not available and nothing else changes (Rarog D.7, 2026-11-02;
        `analysis/rarog_sweep_2026-11-04.md`).
      - The search state too, not only the TT: histories, correction tables
        and per-thread data (−0.93% cycles on Triumviratus, TB-13).
      - The table's own fill (2026-11-11; F3 of
        `analysis/rarog_import_2026-11-11.md`; Rarog's D.5.2). Today a
        resize constructs every cluster with six sequentially consistent
        stores on one thread, and `ucinewgame` clears the table again on
        one thread. Pages the system hands over are already zero, so the
        new allocation needs no construction pass, and the clear runs on
        the pool's threads, each touching its own share first, as the
        donor's does. Timed at Hash 16384 before and after; the table's
        contents and `bench 13` do not change.
      - Read on the 5950X for Windows x64, and Linux x64 where a Linux host
        or WSL2 is available; the ARM64 platforms ship behind the fallback
        until hardware reads them. The 1T/4T bench scaling is re-read after
        (D-9), pinned one thread per physical core: Rarog's fall from 3.72
        to 2.60, unseen by its head-to-head, was later traced to SMT
        packing (BAS-X41), so the check guards placement as well as code.
    - [ ] **B.8.5** `[I1]` Speed pass
      Behaviour-neutral throughput work on the new modules, with a pooled-PGO
      floor of +0.5% per change and B.8.3's guard as the other floor, on
      B.8.4's large pages. The archived board-speed leaves (7.2–7.8) moved
      to the board review (B.7) on 2026-11-03; this leaf profiles the board
      B.7 leaves. It was B.7.2 until 2026-11-05.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Uses A.9.4's NPS read and A.9.5's profiler.
      - Candidates from Rarog: the child TT prefetch before the make from a
        predicted key (D-5, +1.69%) and at pick time (D-6, +0.5% and +0.3%
        under PGO; large pages are B.8.4's); the per-stage move-list copy
        (D-10; Rarog's in-place list +6.93%); a survivor count for the quiet
        early exit (D-11, +8.85%); attack lookups (D-12); thin against full
        LTO (D-32).
      - Measured by A.17's profile (2026-10-30, `bench 13` after A.16): the
        material imbalance recomputes 48 products of piece counts on every
        evaluation, 2.89% of a node, and depends on material alone (a
        material-keyed cache, Stockfish's material table); material and PST
        are summed from scratch over every piece, 1.37% (incremental in
        make and unmake). Slider lookups are 2.7%, and compressing their
        tables read 0.98% slower (BAS-P26): leave them flat.
      - Rules: re-read 1T/4T bench scaling after speed or large-page work
        (D-9, pinned one thread per physical core; Rarog's 3.72 to 2.60
        fall was SMT packing, BAS-X41); read a bundle's members alone
        (D-16); a maintainer rule for real but sub-bar gains (D-17); screen
        with PGO (D-14); priors D-7
        (most prefetches lose under 4 KB pages), D-15 and D-20.
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items),
      each measured there at an identical tree:
      - Prefetches at make: the child's TT entry from a predicted key
        (+2.21% NPS, Rarog's D-5 again), the child's correction entries
        (+2.04%) and the continuation-correction cells (−0.3% cycles, which
        also removed a memory-bandwidth asymmetry between its sockets)
        (TB-12).
      - The picker: branch-free selection (the best-quiet selection caused
        11% of branch misses there), quiet scoring per node with history
        rows read once, rank by the count of larger keys, captures not
        regenerated by the quiet stage, the victim from the mailbox (TB-8).
      - Stack probes (`__chkstk`) in hot functions whose frames pass 4 KB on
        Windows, read in the disassembly beside D-23 (−1.55% there) (TB-11).
      - Link flags: ext-tsp block layout, no cmov-to-branch conversion;
        deterministic PGO training at fixed nodes (TB-14); an eval cache the
        TT duplicates, if one exists (TB-15).
      - Priors on what did not pay there: prefetching and reordering loads,
        AVX-512 gathers, the next-move TT prefetch on AMD (TB-16).
      - B.16.4's identical-tree counter read beside A.9.4's pooled read
        (TM-7).
      - Shortcut builds (TM-10): each shortcut ships with a build switch that
        runs the original beside it at every node and aborts on the first
        disagreement, as Triumviratus checked every identical-tree patch.
      Added 2026-11-05:
      - B.3.5's P9: the ON arm's game NPS read 0.96× the OFF arm's, and the
        cause is not the repetition check (0.30%); profiled here first.
      - PGO trains at Threads 1 only (`cmake/pgo-build.cmake`, `bench 13`).
        Rarog's multi-thread kernel copies stayed cold until its profile
        trained a multi-thread run, +2.4% at 4T (RAR-P52,
        `analysis/rarog_sweep_2026-11-04.md`). In C++ one `negamax` serves
        every thread, so only branch-level coldness carries over (the pool
        branch in `record_node`, `flush_shared_nodes`, helper paths): one 4T
        pooled read of a profile trained on `bench 13` plus a Threads-2 run,
        kept only if 4T gains and 1T stays within the floor.
      Added 2026-11-11 (`analysis/rarog_import_2026-11-11.md`;
      Rarog's NODE-5 and D.8.19):
      - Quiescence generates and scores every capture before it tries
        the table move; the main search's picker generates after. First
        a count of the quiescence nodes the table move cuts, then staged
        delivery there only if that share is material. The tree must not
        move.
      - The frame of `negamax` and of quiescence in bytes, read in the
        disassembly beside TB-11's stack probes; B.9.1 sizes the thread
        stacks from it.
      Added 2026-11-14 (`analysis/b7_frontier_review_2026-11-14.md` §4;
      counted on the search-replay trace of `bench 13`):
      - SEE verdicts carried per move. 20.6% of `see_ge` calls ask about
        a move already asked at that node, and an earlier answer settles
        0.145 calls per node (10.2%), because `see_ge` is monotone in
        its threshold: a yes at `t` answers every threshold at or below
        `t`, a no every threshold at or above. The picker's verdict
        travels with the move to the search's pruning tests. Ceiling
        2.7 ns per node; 1.9.1's memo read +0.36%. Kept at +0.3% on
        B.7.5's engine screen.
      - Captures generated twice: 8.7% of capture generations repeat at
        the same node (the ProbCut picker, then the main picker), about
        1 ns per node. TB-8's item, now sized.
      - The evaluation's `check_squares`: 3.64 of its 4.19 calls per
        node are the safe-check loop, and the queen's set repeats the
        bishop's and the rook's lookups. Behaviour-neutral.
      - From B.7.2's deferrals: the stage as a template parameter stays
        refused (RAR-M44(c)); next attacker by index and the quiet-SEE
        exit are B.7.7's.
    - [x] **B.8.6** `[I1]` Delete the old search arms (ANY TIME) — before B.5
      Done 2026-11-08 at the maintainer's request, out of turn, so that
      Cluster 4 builds on one search path; every gate the arms served is
      read. Deleted: the `B3_PROOF`, `B4_QSEARCH` and `COLOUR_HISTORY`
      options and their defines; the kernel's OFF-only paths (null move,
      ProbCut, IIR and singular extension in their old forms; quiescence's
      old ordering, pruning, depth cap and in-check search); the colourless
      history index; CI's two OFF-arm builds; the lines only those paths
      read (`NullBase`, `NullEvalDiv`, `ProbCutMargin`, `SingularMinDepth`,
      `SingularBetaMult`, `SingularDoubleMargin`, `DoubleExtMax`,
      `CoreIirMinDepth`) and the `double_exts` stack field. The default SPSA
      surface regenerates from 19 coordinates to 12 and the core surface
      from 49 to 48; the B.3.3 and B.4.6 surfaces are unchanged.
      - Kept, for B.8.2: the clusters' refit tables
        (`BASILISK_B3_CORE_DEFAULTS`, `_B4_CORE_DEFAULTS`), which still
        apply over their X lines, because folding them in moves the core
        surface's start values; three more zero counters (`sing_cap_bound`,
        `qs_cap_hits`, `qs_late_pruned`) with the `QsLatePrune` trace event
        and the `QsLate` price rule. The `B4QsDelta` switch stays: it
        restores the old delta rule.
      - `test_search`'s lone-move section bound the deleted singular
        margin; it now binds the graded extension's boundary (one ply at
        margins of one, two below). Its count falls by one expectation per
        deleted line in every build and one more in Tune (Release 735 to
        728, Tune 804 to 789, each with the section's added check); the
        other suites' counts are unchanged.
      - Exact fingerprint: `bench 13` 4,144,790 on fresh Release, Tune and
        Diag builds, each passing CTest 27/27; the Ablate build's
        `test_uci_protocol` passes and the Texel build's `--verify`
        reconstructs all seven positions; `generate_spsa_surface.py
        --check` and the `tools/diag` tests (177) pass. Not run: the
        sanitizers, and the macOS and Linux builds, which CI runs.
      - The gate scripts that built an OFF arm (`tools/run_b32_games.ps1`,
        `run_b36_gate.ps1`, `run_b45_games.ps1`) reproduce only at the
        commits their entries record.
- [ ] **B.9** Robustness, verdicts and protocol before the release
  Added 2026-11-05 (maintainer: every correctness fix before the next
  release): the lifecycle and protocol work that was D.3.2, the
  release-visible defects of the Rarog sweep and the tablebase root
  defects that were D.4's. Nothing here is tuning. Each item gets a test
  that fails on the head before its repair. It runs before B.11.1, so
  that the checkpoint measures the repaired head.
  Since 2026-11-11 three research leaves feed one repair leaf and one
  repair gate (`analysis/rarog_import_2026-11-11.md` §4): the lifecycle
  and protocol audit (B.9.1), the evaluator's hard verdicts read against
  Syzygy (B.9.3) and the known defects that later leaves own (B.9.4);
  then the repairs (B.9.5, which was B.9.2).
    - [ ] **B.9.1** `[R2]` Lifecycle and protocol audit and tests
      Lifecycle and protocol (archived 9.4): worker start, stop and join,
      cancellation, result authority, new-game and option resets, command
      ordering, Syzygy thread and halfmove contracts; deterministic
      interleavings plus stress. Zero crashes, illegal moves and
      forfeits in a pool tournament is the step's closing criterion; it
      is read in B.11.1's gauntlet, where one reopens this leaf. It was
      D.3.2 until 2026-11-05.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Abort-sweep tests (A-65; the deep-line stack test A-25 is B.5's);
        command fuzzing (A-67); MultiPV stopped-search tests (D-73);
        seldepth excludes qsearch plies (B-122); 1.2.3's two illegal-move
        games replayed on the current binary (A-69); Fathom thread safety
        (A-61). The CPU-tier message and the slow-PEXT advisory (A-36, D-25)
        moved to A.15.3.
      Code audit inputs (2026-10-27, `analysis/code_audit_2026-10-27.md`
      F4, verified 2026-10-27), each decided against the protocol with a
      test: a bare `go` searches to `defaultSearchDepth` 7
      (`constants.h:12`, applied at `parameters.cpp:229-231` and
      `:282-283`, asserted by `test_search.cpp:240` and `:265`) where
      Stockfish searches until `stop`; `go mate N` caps the depth at
      `2N - 1` (`parameters.cpp:305-311`) where Stockfish searches until
      the mate is found; a `SyzygyPath` whose `tb_init` failed, or that
      initialised and found no files (`engine.cpp:102-114`), is cached,
      so the same text cannot be retried after the files appear. The
      audit's fourth item, a stale `stop` answering the next `go
      infinite` at once, was refuted: the Stop command clears the flag
      in queue order (`engine.cpp:461`) before the next `go` is handled.
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items),
      each a defect Triumviratus found in itself, to be tested for here:
      - `go`, `d` or `eval` before any `position` (it searched an empty
        board and crashed) (TR-1).
      - Input with repeated spaces and a trailing `\r`; long lines; a
        clock of exactly −1; `bestmove` and `readyok` written under one
        output lock; `go infinite` waiting for `stop` (TR-2).
      - After a stop mid-iteration, the final line printed so bestmove
        heads the last PV (its GUI warned about once in three games); with
        MultiPV every line printed again at the stop, beside D-73 (TR-3).
      - The clean child frame on every entry path (TR-4) is B.5's, beside
        the line growth.
      - The worker stack size on Windows and history arithmetic that can
        overflow in long tablebase endgames (TR-5).
      Defects from the Rarog sweep (2026-11-04, its D.5.3, D.5.5 and D.9;
      `analysis/rarog_sweep_2026-11-04.md`), each confirmed in Basilisk's
      code:
      - `go depth N` at Threads > 1: the helpers drop the depth limit
        (`search_thread_pool.cpp`, `infiniteDepth` for every thread but the
        main one) and `merge_results` takes the deepest result, so the
        engine can print `info depth N+1` and play a helper's move; the
        comment that the depth contract is kept is false. Stockfish answers
        from the main thread under a depth limit. A Threads-4 test
        (DRIVER-12).
      - Fathom calls `exit(EXIT_FAILURE)` when a table fails to map
        (`external/fathom/src/tbprobe.c`, two sites): lazily, mid-search,
        with no `bestmove` and nothing in a crash record; a zero-length or
        truncated table sets it off. The table is skipped instead, with an
        `info string` (PROTO-11).
      - A non-ASCII `SyzygyPath` cannot load on Windows: Fathom is built
        without `_UNICODE`, so its narrow `CreateFile` receives UTF-8 bytes.
        A wide-path conversion or a UTF-8 manifest, tested on a directory
        whose name has diacritics (PROTO-24).
      - `Hash` is floored to a power of two without a word (`tt.h`): an
        `info string` and a README line (TT-7).
      The tablebase root defects, from D.4 (2026-11-05):
      - The root ranking ignores repetitions already in the game (A-58); it
        drew BAS-S39's game 4965, a clean tablebase win, by threefold.
      - Cold table reads at the root count against the clock (A-57).
      From D.1's defect list, tested here because a failure is a repair,
      not a tuning (A-50, TT-4): a hard stop inside the first iteration
      prints a partial result and the fallback move is unguarded
      (`search_worker.cpp`); the game ply taken from a FEN's move number
      (Triumviratus's first move after a book got 30% less time); `go
      movetime` scaled by time factors.
      Engine audit inputs (2026-11-12,
      `analysis/b64_engine_audit_2026-11-12.md` §3.2, §4):
      - A defect: on `ponderhit`, `check_stop` recomputes the budget
        with the game ply of the board at the node the search is at,
        not the root's (`search_worker.cpp`). The soft limit read 3,465
        to 3,675 ms against 3,302 for a plain `go` on the same clock,
        different every time. The root's ply is kept from the search's
        start; the test compares TM_Debug's `soft_ms` both ways.
      - TR-5's arithmetic half is answered: the packet's width table,
        and B.6.5's repair and guard. Its stack-size half stays here.
      Thread contracts (TP-5): `go nodes` counts every thread; the main
      thread's PV is reset before the helpers start; a proven mate beats
      depth in the result choice.
      Added 2026-11-11 (`analysis/rarog_import_2026-11-11.md`;
      F-numbers are its §3, the rest Rarog's D.3, D.5 and D.9), each with
      a test that fails on the head, or a written reason why the
      behaviour stays:
      - `stop` and `quit` during `bench` and `wac` are not seen until
        the run ends, each run holding its own flag (F1): `quit` ends
        the process promptly, and a stopped run says it was stopped.
      - A clock that is zero, negative or does not parse falls through
        to the bare-`go` depth, since no limit is then set (F7): a `go`
        that names a clock is a clock search and moves at once on an
        empty one. Decided with the bare-`go` item above.
      - Thread stacks (F2, with TR-5's Windows item): no search thread
        states its stack size. A second thread gets 512 KB on macOS,
        the executable's reserve on Windows and 8 MB on Linux; the
        donor sets 8 MB everywhere for this reason. Measure the deepest
        frame use at the ply cap (B.8.5's frame read), set the size
        explicitly, and drive a line to the cap on a pool thread on
        every platform CI builds. Fathom's root-move buffer, about
        100 KB, stands twice on the engine thread's stack
        (`syzygy.cpp`, `probe_root_moves`) and moves to the heap.
      - Output: write failures are never checked, and a closed stdout
        is handled on POSIX only (F9). The engine ends cleanly when its
        output is gone, on every platform.
      - End-to-end process tests the suite lacks: `setoption` while a
        search runs (applied after it, `stop` still answered at once);
        the end of input during `go infinite` and during a ponder
        search; a rejected `position` while a search runs; a terminal
        root at Threads 4; and one test that lists everything
        `ucinewgame` clears, on every thread, so that state added later
        cannot survive a new game unseen (Rarog's D.6.23 found such
        state in itself).
      - Tablebase threads (Rarog's PROTO-15 and DRIVER-15, beside
        A-61): the root probes are documented as not thread safe, so
        none may run beside a helper's WDL probe, and a join must not
        wait without bound for a helper blocked in a table read. A
        four-man threaded test for each. The file-count notice splits
        the path as Fathom does on each platform (PROTO-12).
      - CI: a ThreadSanitizer cell over the threading and ponder tests,
        its suppressions naming only the table's tolerated race
        (BAS-C05), so that an unintended race in the pool, the counters
        or the stop path fails; and the ordinary PGO path and the
        `gcc-pgo` job fail on a kernel that went untrained, as the
        universal build already does (Rarog's D.5.20).
    - [ ] **B.9.3** `[R2]` Hard verdicts against Syzygy
      Added 2026-11-11 (`analysis/rarog_import_2026-11-11.md`;
      Rarog's D.5.19, which found a quarter of one of its own
      recogniser's quiet zeros to be wins). A hard verdict is a value the
      evaluator states rather than estimates: a recognised draw, a
      known-win floor, a scale at its floor. Each is read against the
      tables over every legal position that meets its precondition:
      exhaustively where the class has five men or fewer, on two million
      random quiet positions otherwise. Only KPK has been read this way
      (`test_kpk_matches_syzygy`, 662,704 positions). The evaluation
      stays frozen: this leaf reads, and B.9.5 repairs only a verdict the
      tables contradict.
      - The instrument is built once, here: a per-site enumeration
        against Syzygy WDL, with a fixture small enough for CI. It is
        also C.2.7's drawn-overclaim instrument (C-133) and the source
        of the Syzygy-verified vetoes (C-142), which read it again on
        the C head.
      - The sites: KNN-K; the wrong-bishop KBP-K draw, whose domain
        tests neither the pawn's rank, the side to move nor the strong
        king; `pawnless_scale`'s 0, 4 and 14 classes by position, where
        today's truth table reads them by class; `krpkr_scale`'s five
        draw sites under their floor of 24 (BAS-E54 found 18 of 3,000
        won positions called draws); the KRPP-KRP table; the bare-king
        and known-win floors; the search's own dead-position rule.
      - Known before the read (F6): any number of bishops on one colour
        against a bare king is a dead position that reads as a material
        win, and the dead-position rule covers only KB-KB and a lone
        minor.
      - Beside the leaves, the search: Basilisk's four wrong-bishop
        draws read as draws at depths 8 to 14, from cleared tables and
        after the two lost same-structure positions have trained them.
        On Rarog 7 and 9 of 28 such searches read beyond ±75, through
        ordinary leaves a move outside the recogniser (its D.6.23);
        B.14's guard covers the verdict leaves only.
      - Rule, frozen before the read: a site whose hard draws are wins,
        or whose hard wins are draws, in more than 0.5% of its quiet
        positions is a defect. Its repair narrows the domain to what the
        tables call exact, or discounts where the rule is approximate;
        it never refits a value, which is C.7's. A site under the line
        lands as a test at its measured count.
      - Priors from Rarog's read of its own rules: the wrong-corner
        draw exact over 937,176 positions; `krpkr_scale`'s zeros 1.7%
        wins; its KQ-KP fortress 26.7% of quiet zeros wins.
    - [ ] **B.9.4** `[R2]` Deferred defects: repair now or justify
      Added 2026-11-11 (maintainer: every known correctness fix before
      the next release). PLAN §2's *Known defects* gives some defects to
      leaves that run after 1.11.0. Each is read once here against one
      rule: it is repaired in B.9.5 when it is in the shipped engine,
      has a test that fails, and needs neither a refit nor a redesign a
      later leaf owns; otherwise it stays with its owner, and the table
      and the release notes say why.
      - To read: A-46 (the reserve arithmetic, and `bestmove` waiting
        for every helper to join, which B.16.5 shortened); A-45 (no
        sub-second horizon, so a `3+0.03` clock settles near 155 ms);
        A-59 (the root's DTZ ranking where DTZ is the distance to mate);
        the definitional evaluation repairs C-10 to C-13, which move
        fitted neighbours; BAS-C05, D.2's by decision, since both
        repairs cost NPS.
      - Out of scope: the tuner's and the data pipeline's defects ship
        in no release and stay C.2.1's and C.2.2's.
      - It closes with the table corrected and a list of known
        limitations for 1.11.0's notes.
    - [ ] **B.9.5** `[I2]` Repairs and the repair gate
      It was B.9.2 until 2026-11-11; it now repairs what B.9.1, B.9.3
      and B.9.4 each found.
      Each failing test of those leaves repaired in its own commit, through the
      correctness-repair path: CTest on release and sanitizer builds, the
      exact fingerprint where the search is untouched and a recorded move
      otherwise. The repairs that can change a move are gated together in
      one repair SPRT `[-5,5]` at `3+0.03` (at Threads 4 for a thread-path
      repair), its cap sized from their measured exposure
      (`agents/measurement.md`); protocol repairs that cannot change a move
      take no games. A verdict repair that a static differential confines
      to its own site takes none either; one that changes a score outside
      it joins the repair SPRT.
- [ ] **B.11** Checkpoint and interim release
  The search programme's checkpoint (B.9 until 2026-11-05), the
  originality and licence disclosure (B.11.3, added 2026-11-11), then
  the 1.11.0 release (B.11 until 2026-11-05, B.11.2 until 2026-11-11).
    - [ ] **B.11.1** `[V]` Search programme checkpoint
      Checkpoint: re-measure every deficit meter.
      G(0) is read twice (maintainer, 2026-11-14), so that the gain compares
      both with Basilisk's own baseline and with Rarog's reads:
      - BAS-O05's protocol unchanged. The oracle package is rebuilt with
        the head's HCE by `tools/oracle/build_oracle.ps1`; 3,000 paired
        games at `3+0.03`, 1T, Hash 64, UHO, no adjudication. Its
        difference from +312.6 is the programme's recovery.
      - RAR-O08's conditions (Rarog, 2026-11-13). The bridge's share of
        the oracle's time per node is measured first, by Rarog's
        bridge-price method, and returned to the oracle as time odds:
        Rarog's classical oracle paid 8.5%, ×1.093, a 3.279+0.033 clock.
        Rarog found that RAR-O03 and O04 read their oracle through a FEN
        round trip at 0.56 of its own speed, Basilisk's at 0.83, so only
        this read compares with Rarog's O08 and later reads. The method is
        read from Rarog at the time, with its commit cited.
      - Optional arms, chosen when the checkpoint is prepared: RAR-O08's
        Stockfish-dev and Reckless hybrids on Basilisk's HCE.
      - The sign: Basilisk states G(0) as the oracle over Basilisk, Rarog
        as its head over the oracle.
      The meters are G(0) against the oracle, fixed-node depth and
      branching, pooled NPS, the conversion instrument and the endgame truth
      cohort, plus a 1T pool gauntlet against the four targets and Rarog (400
      games each). Record the attributed Elo per accepted cluster and the
      checkpoint against the budget table, remove the ablation build, then
      freeze the search head for C.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The pool read includes the head on the match host: Rarog's pool
        still holds Basilisk at 1.10.0's 2993, and neither 1.10.2 nor the
        head has been played there (audit, tournaments). Ablation limits
        (B-110) and bit 6's asymmetry (B-109) in the readings. B.11.3
        and B.11.4 follow.
      Added 2026-11-05:
      - Its readings are C.0's starting point, and its pool reads license
        B.11.4 beside the release SPRT. The four-thread reads are
        sibling-free (PROCESS *Harness*, BAS-X41).
      - Basilisk is judged by the pool score, not the Rarog pairing: in
        Rarog's legacy 10.0 the sibling head-to-head differed from the pool
        difference by 35 to 45 Elo at 1T and 4T, a matchup effect of two
        similar evaluations (`analysis/rarog_sweep_2026-11-04.md`).
      - The floors' legality patch moved to B.4's opening, the first point
        between clusters (B.9 until 2026-11-05).
      Added 2026-11-09:
      - BAS-M15 played 1.10.2 and the head after Clusters 1 to 3 in the
        maintainer's pool. It is an interim observation and does not replace
        this leaf's reads: the final head and three of the four targets are
        still unplayed.
      Added 2026-11-11:
      - B.9's closing criterion is read in this gauntlet: no crash,
        illegal move or forfeit by Basilisk. One reopens B.9.1.
    - [ ] **B.11.3** `[R2]` Originality and licence disclosure
      Added 2026-11-11 (`analysis/rarog_import_2026-11-11.md`;
      Rarog's E.3.0). 1.11.0 is the first release whose search is built
      in a donor's shape, and B.6.7's screen names a few dozen literals
      in the kernel that are the donor's own values; 1.10.2's audit
      named only the PeSTO-derived tables and Stockfish 19's
      time-manager constants. B.11.4 does not cut until this closes. A
      small research step with its implementation:
      - Scans on the release candidate's source, built or ported here as
        tools with tests: shared token runs against `sf_19` and
        `9587eeeb`; the constants census, from B.6.7's list of literals
        and the search parameters whose fitted value is still the
        donor's seed; the evaluation's tables against both donors; the
        move-similarity screen against the donors and unrelated
        controls.
      - The licence: Basilisk, Stockfish and Triumviratus are GPL-3 and
        Fathom is MIT. The maintainer confirms that nothing the scans
        find needs more than naming; it is not legal advice.
      - The disclosure, each fact where it belongs: README's
        acknowledgements, short and for users; PROCESS's *Independence
        boundary*, saying what crossed unchanged; the scan record under
        `analysis/`, which also serves as the donor list for a
        tournament registration.
      - The GPL text and Fathom's notice ship with every release asset.
      - Stop rule: a group of donor material the scans find is added to
        the disclosure, not tuned away here. Refitting a copied constant
        is a strength experiment with its own registration.
      - C.13, E.3 and F.9 repeat the scans on their own candidates
        (PLAN §5).
    - [ ] **B.11.4** `[M]` Interim release 1.11.0
      It was B.11.2 until 2026-11-11, and cuts after B.11.3.
      Maintainer decision 2026-10-24, replacing 2026-09-28's no-interim
      rule: the search programme's head is released when B.11.1 closes, as
      Rarog cut 2.5.0 at the end of its search programme. Licensed by an
      accepted SPRT over 1.10.2 at `3+0.03` 1T with `10+0.1` and 4T direction
      checks whose 95% intervals exclude a loss, and a 4T pool read beside
      them: a head-to-head read cannot see a thread-scaling loss both
      versions share (Rarog RAR-M74, RAR-M76). Version 1.11.0 (maintainer
      decision 2026-10-24: releases follow B, C and E, and 2.0.0 is the
      first NNUE release). Cut through A.8.6's tag-driven flow after A.10.4.
      The 4T direction check and the 4T pool read run sibling-free (PROCESS
      *Harness*, BAS-X41); BAS-M11's 4T baseline was played unpinned.
- [x] **B.12** `[I2]` False win in drawn KBP-K (ANY TIME) — before B.3.1
  Correctness repair, opened 2026-10-30 at the maintainer's decision when
  B.3.0's counter suite stopped on it (packet §2.3,
  `analysis/b30_cluster2_research_2026-10-30.md`). `7k/8/6KP/8/8/8/8/5B2 w`,
  an h-pawn with the wrong bishop and the defending king in the corner, is a
  draw; 1.10.2 reads 0 at every depth (11,704 nodes at depth 14), the head
  +150.94 at depth 8 and +112.08 at depth 10 on 13,553,601 nodes with
  seldepth 112, and depth 11 does not finish in a minute. The static
  evaluation is right along the false line; the bisect names `75d7fce`
  (A.13: a won KPK scores `KNOWN_WIN` plus material), with an earlier drift
  to +3.94 before it.
  - Diagnose by ablation and counters on the position, then repair the
    cause in the donor's form, with a test that fails on the head: the
    position and its siblings read a draw at depths 8 to 14 inside a node
    bound near 1.10.2's.
  - Gate: exact `bench 13` or a recorded fingerprint move; CTest; the
    conversion floors at the five budgets of the packet's §2.2; a repair
    SPRT `[-5,5]` when play changes, unless the leaf argues none is owed.
  - Diagnosed and repaired 2026-10-30 (`d8d30e9`; BAS-S40 holds the
    record). The false line ends in an underpromotion beside the
    defending king: after `h8=B` the defender's stand-pat is a won KBB-K
    for the opponent, and quiescence's delta pruning, capture futility and
    alpha-relative SEE threshold read `Kxh8` as a gain of one minor
    against it, though it leaves a dead draw. The three now stand down
    when the futility base is below `-KNOWN_WIN`, the oracle's guard.
    Four wrong-bishop positions read 0 at every depth 8–14 inside a
    200,000-node cap (14,373 nodes at depth 14 on the suite's; the test
    fails on the head); CTest 24/24; the conversion floors hold at all
    five budgets (KBN-K 115/120 against 113). `bench 13` moves to
    16,615,757 through state the bench positions share; the fingerprint
    record updates when the gate keeps the repair.
  - Game gate: BAS-S40, `[-5,5]` at `3+0.03`, 4,000-pair cap,
    `tools/run_b12_gate.ps1`; maintainer-run 2026-10-30: H1, +3.8 ± 5.2
    Elo after 2,567 pairs (a first-crossing estimate), 0 faults. The
    repair stays and the fingerprint record moves to 16,615,757. CLOSED.
- [x] **B.13** `[I1]` Colosseum re-pinned to 0.5.0 (ANY TIME)
  Maintainer decision 2026-10-30, before B.3.0, with the null pair
  waived. 0.5.0 changes reporting only: counts in games, per-engine
  results in the run's `results.txt`, and search telemetry and terminations
  kept per side, so one binary on both arms is `name (A)` and `name (B)`.
  — CLOSED 2026-10-30.
  - The pin is `cli-v0.5.0` (`8fa63cc`): archive SHA-256 `DF03C7AB…`, as
    GitHub serves it; executable `81EA6CBA…`. `tools/setup_tools.ps1`
    refused the staged 0.4.0 and staged it. Rarog is still on 0.4.0, so
    this pin is not one Rarog has run.
  - The wrapper reads only the runner's JSON and the PGN, whose fields 0.5.0
    keeps; no tool needed a change. Guard cases 31/31 on 0.5.0, every
    policy dry run included, after their default arms moved to B.12's
    staged pair; the recount's unit tests pass.
  - Replays: `stats --json` and `status --json` of five finished runs
    (BAS-S40's and BAS-S39's SPRTs, a gauntlet, a match and an SPSA block)
    are identical leaf for leaf under 0.4.0 and 0.5.0, except BAS-S24's
    one-binary match, where 0.4.0 merged the two sides and 0.5.0 splits
    them; the split sums to the merged counts.
  - Live on 0.5.0 through the wrapper: a 100-game SPRT, a 40-game
    one-binary match and a 40-game calibrate run each finished with the
    independent PGN recount agreeing, and 0.4.0 reads their directories
    with the same scores. Evidence in `tools/results/col050-*`.
- [x] **B.14** `[I2]` Correction added to recognised draws (ANY TIME) — before B.3.2
  Correctness repair, opened 2026-10-31 at the maintainer's decision on
  research amendment 4 (`analysis/b30_cluster2_research_2026-10-30.md`,
  amendment 4 §2 and §3.4). The evaluation classifies the wrong-bishop
  KBP-K corner, KNNK, the drawn KPK and every `SCALE_DRAW` position as 0,
  and `corrected_eval` and the quiescence stand-pat add the correction
  history on top of that 0, on both arms. Measured on the head:
  `5k2/8/6KP/8/8/8/8/5B2 b` (Kg8 the only drawing move) reads +90 for
  White at depth 11 inside 3,075 nodes, trained by its own root (ordinary
  evaluation −797 for Black against a search value of 0); B.12's four
  positions read up to +172 (4 of 28) after two same-structure priming
  searches in one process, and 0 from cleared tables, which is why B.12's
  test passes. Neither donor composes the two mechanisms (sf_19's
  evaluation has no recognised draw; classical `9587eeeb` had no
  correction), so there is no donor form to port; C.5.2's E7 switch and
  C.10's ablation name the same idea, and this leaf owns the repair.
  - Candidate: the consumer guard priced in amendment 4 §3.2 — no
    correction added where the raw static evaluation is 0 (`corrected_eval`
    and the stand-pat). On the ON arm it read 28/28, floors unchanged to
    the decimal, WAC 244 at 100,000 nodes, the promotion fixture +479.
    The producer side (B.3.1's ticket 7b guard) stays; this is the consumer
    side.
  - Fixtures, both failing on the head: the organic FEN above at depths
    8–14 under 200,000 nodes (Hash 8, |score| ≤ 75) and the primed 28
    (amendment 4 §R, R2b's recipe: one process, one `ucinewgame`, two
    priming searches `go depth 12` on `8/8/5K1P/8/8/8/k7/5B2 b` and
    `8/8/6KP/8/8/8/8/k4B2 b`, then B.12's 28 searches).
  - Gate: a recorded fingerprint move on both arms (B.3.1's OFF arm then
    re-bases to it); CTest; the conversion floors at the five budgets; a
    repair SPRT `[-5,5]` at `3+0.03` in BAS-S40's shape, unless the leaf
    argues none is owed.
  - Lands after B.3.1's accepted review and before B.3.2's first game,
    never inside a B.3.1 ticket: it moves both arms' fingerprints, and
    the ON arm's primed 28 reads 27/28 until it lands, so every B.3.2 to
    B.3.5 read is on one head (audit 2026-11-01; the slot was "before
    C.0").
  - Implemented 2026-11-01, `LOCAL_QUALIFIED` (packet *B.14*,
    `e883a13`).
    - Both fixtures are in `test_endgames`, both arms, and failed on the
      head: the organic FEN on the OFF arm (−90 at depth 11), the primed
      28 on both (OFF 24/28, ON 27/28).
    - `bench 13`: OFF 16,615,757 → 15,100,437, ON 6,723,172 →
      6,178,678, equal on every flavour; B.3.1's OFF arm re-bases.
    - CTest 25/25 on both arms; the floors 24 x 15 on both, KBN-K OFF
      115, ON 120; WAC at 100k ON 242, OFF 241; the fixture a7a8q, ON
      +599 at 100k (the OFF arm 0, recorded).
    - The repair SPRT is owed: 0.2-0.3% of evaluations are a raw 0, in
      every bench position, and 78-86% of those are ordinary zeros, not
      classifications. Its shape is BAS-S40's: OFF arm, 4,000-pair cap,
      to be registered as BAS-S44.
    - The maintainer decides first whether the guard keys on the
      evaluator's classifications instead of `raw == 0`; then the gate
      is prepared and the maintainer starts it.
  - Narrowed form, 2026-11-01, on the maintainer's decision (packet
    *B.14*; `narrow.patch`, not committed).
    - Keyed on the evaluator's four draw verdicts, it holds both
      fixtures, the floors and WAC on both arms. Searched alone, it
      moves 4 and 6 of the 40 bench positions (the `raw == 0` form
      moves 36 and 38).
    - It fails `test_search` on the OFF arm: the promotion fixture
      plays `a5a3` (a draw at depth 20) instead of `a7a8q` (a tablebase
      win) at 33k-100k. The ON arm passes 25/25.
    - Blocked: the maintainer chooses between gating the `raw == 0`
      form and returning the narrowed form to research on the fixture.
  - Maintainer decision 2026-11-01: research on the narrowed form.
    Returned to `RESEARCH` (`R3`). The tree is at `e883a13` and the
    narrowed form is kept as `narrow.patch`. The research reads whether
    the OFF arm's pass at 100k is the defect's or a tie, what the
    fixture binds on the OFF arm, and which candidate BAS-S44 measures
    (packet *B.14*).
  - Research amendment 9, 2026-11-01 (packet, a separate session;
    registered at `65d892a` before any read): the head's `a7a8q` at
    100,000 nodes is a 4-cp preference inside a root reading +460 to
    +616 for every move, produced by its tree reaching the draw-reading
    iteration last, not by the verdict nodes' correction (which leans 40
    cp toward Black on that search; the complement form reads 0); the
    `raw == 0` form's pass is a tie at 0 held by the root's order; the
    narrowed form's `a5a3` is a 7-cp tie-break at depth 8 that throws a
    tablebase win. At 200,000 nodes both candidates sight the win
    (`raw == 0` +834 from 180,000 nodes, narrowed +781 from 150,000,
    held to 1,000,000). Proposed for the maintainer's decision: C-A9-1,
    the OFF arm's clause `a7a8q` with a positive score at 200,000
    nodes (the ON arm's 100,000-node clause unchanged, both arms
    asserting the score); under it the narrowed form holds every
    binding read on both arms (CTest 25/25 on both arms under the amended test, R8) and BAS-S44 measures
    it. Blocked on decisions 1 (C-A9-1) and 2 (the candidate); no
    fixture edit before decision 1.
  - Maintainer decision 2026-11-01: C-A9-1 taken and the narrowed form
    is BAS-S44's candidate. Resumes at the amended candidate (`I2`).
  - Implemented 2026-11-01, `LOCAL_QUALIFIED` (`1b2515b`: `narrow.patch`
    and C-A9-1's test edit in one engine commit; packet *B.14*, the
    record after amendment 9).
    - `bench 13`: OFF 16,615,757 -> 11,880,564, ON 6,723,172 ->
      4,923,507, equal on the Tune, Diag and Ablate flavours and on the
      final-PGO build; `bench 10` on the sanitizer trees equal to release.
      B.3.1's OFF arm re-bases; the fingerprint record stays at
      16,615,757 until BAS-S44's verdict.
    - CTest 25/25 on both arms, 23/23 under ASan and UBSan (`-LE
      strength`); both B.14 fixtures 7/7 and 28/28 on both arms; the
      floors 24 x 15 with KBN-K OFF 115 / ON 120; WAC at 100k OFF 240 /
      ON 241, depth 6 182 / 132; the promotion fixture OFF `a7a8q` +781
      at 200k (`a5a3` 0 at 100k, recorded), ON +957 at 100k.
    - Gate prepared: BAS-S44 registered (`docs/experiments/BAS-S44.md`),
      `tools/run_b14_gate.ps1`, the two final-PGO OFF-arm binaries
      pinned (`b14-fix` 77BB764C..., `b14-base` 3CC41280...),
      dry run accepted 2026-11-01 (`tools/results/colosseum_sprt_b14-gate_20261101_213650.*`, dry run 32006BE5..., manifest 443FA844...). The maintainer starts it; `GAME_GATE`.
  - Game gate: BAS-S44, `[-5,5]` at `3+0.03`, 4,000-pair cap,
    `tools/run_b14_gate.ps1`; maintainer-run 2026-11-01: H1, +7.4 ± 6.9
    Elo after 1,271 pairs (a first-crossing estimate), 0 faults. The
    repair stays and the fingerprint record moves to 11,880,564 (ON
    4,923,507). CLOSED.

### Active workflow register

One row per open leaf of the current phase. The checker requires every open
leaf of the current phase to have a row, and each row's class to match the
leaf's tag. Later phases carry only a class until they open.

| Leaf | Workflow state | Class | Current decision |
|---|---|---|---|
| B.7.5 | READY_FOR_IMPLEMENTATION | I2 | The frontier review's instruments made permanent, before the rebuild |
| B.7.6 | READY_FOR_IMPLEMENTATION | I2 | Was B.7.3; the amended handoff: C1, C3′, C5, C8 |
| B.7.7 | RESEARCH | R2 | Second round on B.7.6's head; the rule is frozen in the leaf |
| B.7.8 | RESEARCH | I2 | B.7.7's handoff, or NO_CHANGE |
| B.7.9 | RESEARCH | V | Was B.7.4 (B.15.4); after B.7.8, before B.8 |
| B.8.2 | RESEARCH | I1 | After the clusters and the board |
| B.8.3 | READY_FOR_IMPLEMENTATION | I1 | Was B.7.1; before the speed pass, no engine change |
| B.8.4 | RESEARCH | I1 | Split from the speed pass 2026-11-05; before it |
| B.8.5 | RESEARCH | I1 | Was B.7.2; after B.8.3 and B.8.4, on the new modules |
| B.9.1 | RESEARCH | R2 | Was D.3.2, with the release-visible defects (2026-11-05) |
| B.9.3 | RESEARCH | R2 | Added 2026-11-11; the evaluator's hard verdicts against Syzygy, the evaluation frozen |
| B.9.4 | RESEARCH | R2 | Added 2026-11-11; known defects owned after B.11, repaired now or justified |
| B.9.5 | RESEARCH | I2 | Was B.9.2; on the failing tests of B.9.1, B.9.3 and B.9.4, one repair gate |
| B.11.1 | RESEARCH | V | Was B.9; programme checkpoint |
| B.11.3 | RESEARCH | R2 | Added 2026-11-11; the disclosure on the release candidate, before the cut |
| B.11.4 | RESEARCH | M | Was B.11, then B.11.2; interim release after B.11.3 (maintainer decision 2026-10-24) |

## Phase C — Evaluation programme (search frozen)

**Goal:** recover the measured same-search evaluation deficit, with the search
frozen at the B.11.1 head. The families are re-implemented in classical
Stockfish's shape where its conditioning is stronger, and Basilisk's are kept
where the evidence says ours is better. The whole surface is refit after
every family cluster, and endgame handling gets its own bounded cluster that
absorbs the archived Phase 6 remainder.

**Why classical Stockfish `9587eeeb`.** It is the last classical Stockfish
master, the evaluator Basilisk's oracle measured (BAS-O02), and the reference
its maturity audit already compared against (BAS-E07). Modern Stockfish has no
hand-crafted evaluation. Its families and their conditioning are the donor;
its constants are seeds on a different scale and ride the next refit.

**The question C.0 must answer first.** BAS-E07 concluded that the 232.8-Elo
gap is the *values* of features Basilisk already has, that the six terms it
lacks cannot carry it, and that static fitting is exhausted: cycle 6 washed
at +1.37 (BAS-E04). That contradicts the family-cluster premise this phase is
built on. Evidence has arrived since: Rarog's complete refit (+22.04), matched
tablebase relabel (+6.73) and phase-yield corpus (+11.81) paid on an HCE
weaker than Basilisk's (BAS-X23). Also, cycle 6 refit an unchanged surface on
an unchanged search. C.0 measures residuals by cohort on the B.11.1 head before
choosing. If it finds values, not structure, the family clusters shrink to
refits and C.2's corpus and label work carries the programme.

**Cluster shape for C.** Each family cluster goes through seven stages:
1. The family's current terms are traced, and their activation and residual
   measured on the fitting corpus by cohort.
2. The donor's form is read for conditioning and populations.
3. A design states which terms are replaced, which stay, and which
   neighbouring families share inputs, so those inputs are computed once.
4. Implementation, with `EvalTrace` coverage for every new slot and the
   reconstruction test.
5. A whole-surface Texel refit, with the frozen test split reported once.
6. A PGO bake and SPRT, `[0,3]` (`[3,10]` when the family's residual is
   large).
7. The ledger row.

Fit loss is a screen and a falsifier, never acceptance: better holdout loss
has lost games in both engines (BAS-X02).

- [ ] **C.0** `[R3]` Evaluation programme investigation
  Investigation: the family map refreshed on the B.11.1 head, residuals and
  activation by cohort, the donor comparison, BAS-E07's
  values-versus-structure question, the shared-input plan, the cluster order
  by expected value, the corpus and refit protocol and the label policy;
  frozen handoffs for C.1 and the first family. No engine implementation.
  Inputs from A.2.3 (`analysis/parameter_inventory_v1.md`): the 23 all-zero
  groups by kind, and space, which no C cluster names yet.
  Input from B.10 (`analysis/pst_orientation_2026-10-23.md`): an L2 anchor
  at `1e-6` held seven mirrored tables in place for ten releases, so the
  refit protocol states how a fit can still correct a seed error. The
  directional screen that found the defect is the model: project an
  unanchored fit's movement onto each group and read any group that moves
  as a whole.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Instruments to build or port: the donor-direction residual screen on one
    `9587eeeb` build, with wire controls, comparators and the six-men split
    (C-79 to C-88); played and full scores both dumped (C-89); the static
    score against the engine's own re-search (C-91); calibration by phase
    (C-92); a term inventory by firing share, the untraced `rest` terms
    included (C-93); a fixed-depth tree read on game positions (C-96); a
    magnitude contract (C-97); the same-search meter re-read on the current
    head (C-102); a two-arm fixed-K PST screen (C-23).
  - Rules for every unit: an equal-node companion read when an equal-time gate
    fails (C-95; Rarog's lazy lesson read -104.5); a margin block and gate 2
    after each unit, since `LAZY_MARGIN` was never refit, not even after
    B.10's 31.7 cp shift (C-98); NPS and `10+0.1` reads per unit (C-99);
    repairs classed as correctness or definitional (C-107); comparison with
    the incumbent (C-108); new terms seeded at 0 behind the lazy gate with a
    post-refit falsifier (C-109); a structural stop rule (C-110); a refit
    grows the tree (C-72); static and game results disagree both ways (C-101);
    the frozen-search prior was refuted (C-100).
  - Priors: material, imbalance and piece rows carried no signal in Rarog, so
    screen before C.6 builds its list (C-118); space has no owner (C-119);
    zero-weight groups (C-120); transient material in quiet rows (C-77);
    remaining game length (C-78); Rarog's Phase C budget, about +105 from
    units and +42 from speed with the remainder in search-visited positions
    (C-124, F.2.6).
  Code audit inputs (2026-10-27 second pass,
  `analysis/code_audit_2026-10-27.md` §6.3: the evaluator read against
  `sf9587-ks`; IDs are its items):
  - E6: fitted signs opposite the donor's, read in the unanchored-fit
    directional screen before any cluster — `space_blocked_mg` −2,
    `rook_king_ring` −2/−4 (donor +16), `king_protector_n_mg` +1 (donor
    −8), `restricted_eg` −4 (donor +7), `threat_by_rook_mg[PAWN]` −4;
    the likely cause is collinear neighbours (knight PST and outposts
    against the protector, attack units against ring bonuses, isolated
    −13 against `pawn_islands_eg` +6 on the same pawns). Candidate
    removals: islands, the duplicated ring bonuses, the four bishop-pair
    carriers (`imbalance` slot 0, `bp`, `bishop_pair_pawns`; C-20).
  - E2: the phase gives middlegame terms far more weight in simplified
    endings than the donor's npm phase — the mg share is 33% in a
    double-rook ending, 25% in R+B against R+N and 33% in Q against Q,
    against the donor's 10%, 3% and 13% — so every mg-only term (king
    danger, shelter, storm, space, tempo, the king's mg PST) stays live
    in rook endings and partly stands in for the missing eg king danger
    (E1). Calibration by phase (C-92) reads it; phase and eg king danger
    change together (both in C.3 since 2026-11-05), never one at a time.
  - E8: space (C-119) is computed at every phase with no npm gate and
    lacks the donor's bonus for squares behind own pawns; its handoff
    carries E6's sign.
  - The term inventory, donor against Basilisk with status and lines, is
    §6.3's table; C.0's family map starts from it.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - A concept residual screen (TE-1), cheap and donor-free: on game
    positions, the score of a depth-1 search with quiescence against a deep
    search; about thirty chess concepts (passer rank, unstoppable and
    connected passers, space, open files by the king, king-zone attackers,
    locked pawns, drawing factors) regressed on that residual by least
    squares, holding the starting score, material and piece count fixed,
    |t| > 3 as a signal. On Triumviratus's network it named passers, space
    and king danger as underestimated; there every fix outside the network
    lost, but on an HCE a signed residual points at a term's value, the
    question BAS-E07 asks. It complements C-91 (the static score against
    the engine's own re-search) by naming concepts rather than terms.
  - A regime check for C.3's phase key (TE-2): Triumviratus's network
    divided positions by material × queens (normalised mutual information
    0.43) better than by material bands (0.34); a cheap read of whether
    Basilisk's fitted terms split the same way.
  - A measurement holds only in the regime it was taken (TM-5).
- [ ] **C.1** `[I1]` Evaluation restructure, behaviour-neutral
  `src/eval.cpp` split into family modules, one attack-map and mobility-area
  producer, `EvalTrace` unchanged in meaning, lazy-path semantics preserved;
  exact fingerprint; pooled NPS within noise.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Split the untraced `rest` bucket by source (C-90); a corpus-wide score
    hash and the three running-sum reads (`eval.cpp:1206`, `:1897`, `:1975`)
    as the behaviour-neutral proof (C-104); a cache-purity test (C-105); the
    order of terms around the lazy return documented (C-6).
- [ ] **C.2** Fit pipeline, corpus, labels and gate instruments
  Fit pipeline, corpus and label contract (archived 8.1–8.8), and since
  2026-11-11 the endgame instruments (C.2.7, which was C.7.1): PLAN §3
  gives every evaluation gate its endgame truth vetoes, so the
  instruments are audited before the first evaluation gate (C.2.8, which
  was C.2.6), not after four family gates.
    - [ ] **C.2.1** `[I1]` Fit-tooling contract and Texel handbook
      Fit-tooling contract (archived 8.1.a–h). The leaf adds the Texel data
      contract it writes to *Standing contracts* (A.12).
      - K is fitted once and frozen across stages and arms.
      - Initial vectors are complete and explicit, and B.10's PST
        orientation test passes on them before any fit reads them.
      - Train, validation and test splits are by game, and the test split is
        claimed atomically, once.
      - Every coordinate is classed free, fixed or excluded, with gauge
        anchors and reconstruction (Manta's catalogue, BAS-X14).
      - Every input and output is hashed, and source and binary are restored.
      - Labels must be exactly 0, ½ or 1, with rejection accounting.
      - Contracts are versioned, and the snapshot's fit tools
        (`tools/texel/`) are ported where useful; Basilisk's own
        `tools/texel/phase911.ps1` driver is adopted into the contract or
        retired.
      - A Basilisk Texel handbook is written. Rarog's, the snapshot's
        `analysis/texel_fitting_handbook.md`, is a template, not a source of
        constants.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The tuner's delta scale (`tools/texel/tuner.cpp:423-438`) omits the
        rook-ending and pawnless-minor scalers and the override rows, and
        applies OCB unconditionally (C-1): mirror every scaler or exclude
        override rows, with a per-row engine-against-tuner scale check.
        `--verify` is circular (C-2): compare against an independently
        accumulated raw score. Fit the played function, lazy path included
        (C-5).
      - Contract: K fixed once on validation (C-49); three splits with a
        frozen test opened once (C-50); residuals on unused train rows (C-51);
        a coordinate manifest with support-based freezing (C-52); explicit
        prior vectors (C-53); Rarog's stage schedule (C-54); never hand-revert
        a block (C-55).
      - Anchors: gauge freedoms (C-19: `phase911.ps1` runs `--tune all`, which
        frees material and every PST); per-coordinate king-safety steps
        (C-18); shelter and storm fitted by some stage (C-21); dead and
        collinear imbalance slots in the manifest (C-20).
      - Data and driver: the split by start, not start plus moves (C-28);
        games rejected by termination (C-29); game id and ply per row for
        clustered errors (C-30); scores re-dumped after a refit (C-69);
        fixtures that depend on fitted weights fixed in their own commit
        (C-70); fit-driver invariants (C-71); the dense matrix at full scale
        (C-73); B.10's orientation test limited to seeds (C-22).
      Code audit inputs (2026-10-27, `analysis/code_audit_2026-10-27.md`
      §6, a read of `tools/texel/` at `fe20f09`; items T1–T5 are new, the
      rest confirmed C-1, C-2, C-5, C-8, C-19, C-20, C-27 to C-29, C-49,
      C-50 and C-71 by reading). Parity of the linear part is sound:
      PST mirroring, phase buckets, tempo, rule-50 damping, imbalance
      and perspective match `evaluate` term for term, and runs are
      deterministic (sequential double accumulation).
      - T1: the Adam schedule, not the data, bounds how far a coordinate
        can travel — full-batch Adam moves a one-signed coordinate about
        `lr` per epoch (`tuner.cpp:67-68`, `:1154`; `phase911.ps1:41`
        runs 100 epochs at 0.3, so about 30 cp), and there is no
        convergence criterion; C-66's 11–57 cp PST movements would be
        clipped. Test: a one-feature synthetic CSV whose optimum is 60 cp
        from the seed must be reached; fail a run whose best epoch is the
        last or whose travel reaches 0.9 × lr × epochs.
      - T2: `bake.py` writes every slot that differs with no provenance or
        completeness check (`bake.py:173-174`; the dump carries no header
        hash): a stale dump reverted 42 members and both PST blocks in a
        dry run and skipped 92 absent members silently. Slot placement
        and CRLF are correct. Test: `test_bake` cases that fail on a
        missing member, an unknown name and a hash mismatch.
      - T3: hidden clamps apply to every coordinate whatever group is
        fitted (`tuner.cpp:458-545`, called at `:1157`): sign priors,
        `passed[0/7] = 0`, `ThreatQueen ≥ ThreatRook ≥ ThreatMinor`, a
        sequential max that biases mobility upward; none is in a
        manifest, and they would mask a sign bug. All defaults are inside
        them today. Test: the clamps leave the defaults unchanged, and
        inactive slots equal the base after a fit.
      - T4: `--tune all` moves `SafetyTable` linearly with no monotone
        projection (`tuner.cpp:1372`), and `--tune-kingsafety` then clamps
        each entry between its neighbours (`:1395-1398`): on a
        non-monotone table `lo > hi`, undefined behaviour for
        `std::clamp`. `T[0]` and `T[1]` are never descent knobs. Test: the
        table is asserted non-decreasing when the stage starts.
      - T5: resolved by A.16, which removed the winnable term, its seven
        slots and its tuner group; C.7.3's complexity term needs an
        instrument of its own.
      - An option to register, not a default (2026-11-11; Rarog's
        EVAL-13): a second-difference penalty on the mobility tables and
        the pawn table, so a fit cannot leave a sawtooth in a term that
        should be smooth.
      - Hygiene: no workflow runs `tools/texel/test_*.py` (two of three
        need python-chess, absent here) and `test_edge.csv` is
        referenced nowhere; `phase911.ps1` reads `$LASTEXITCODE` through
        `Tee-Object` and is Windows-only; `import_beast.py` is obsolete;
        the usage text omits four modes; comments cite 1,183 or 1,190
        parameters against `EVAL_PARAM_FLAT_SIZE` 1,178; the README and
        `sample_fens.py` hard-code `A:/` paths; the L2 penalty is left out
        of the reported loss, `--l2 0` is rejected, and `fit_K` stops
        silently at its [0.5, 2.5] bounds. C-1 addition: the KmK rule picks
        the strong side by the score's sign, so that scale depends on the
        sign too.
    - [ ] **C.2.2** `[R2]` Corpus design
      Corpus design (archived 8.2.a–e).
      - The source store is located and hashed.
      - Buckets use evaluator phase, not ply; only opening starts feed the
        opening bucket.
      - One extractor contract serves every arm, changing one causal axis at
        a time.
      - Size comes from effective independent rows and a learning curve.
      - The composition and budget are registered before launch.
      - Datagen uses the diverse book: the default book once collapsed 200k
        games into 31,880 unique positions.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The datagen default book is SuperGM (C-26); the sampler fills equal
        phase reservoirs and keeps its own phase definition (C-27); the de-dup
        key's false clock premise (C-8); book yield per start phase and an
        exact quota (C-75); transient material (C-77); extractor priors
        (C-74).
    - [ ] **C.2.3** `[V]` Generate and publish corpus A
      Generate and publish corpus A with the B.11.1 head at a fixed node budget,
      natural termination, maintainer-run; audit the returned composition and
      publish it immutably (archived 8.3).
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The datagen wrapper checks faults, refuses book wrap and unregistered
        binaries (C-32); the realised opening sequence pinned, a move-for-move
        pilot for a harness port (C-76); the label audit re-run on the B.11.1
        head (C-62); natural termination multiplied mates 57x in Rarog and
        inflated the king table, so register a tree read (C-56); Colosseum
        datagen only after A.9.1 (C-31).
    - [ ] **C.2.4** `[R2]` Matched label arms
      Matched label arms (archived 8.4–8.5): corpus B row-identical to A
      except eligible tablebase rows relabelled by Syzygy WDL, with cursed
      results as draws and missing tables as unknown.
      Whole-game tablebase adjudication is analysed separately as its own
      registered arm or a no-arm verdict. Tablebase adjudication never enters
      a strength gate. BAS-E46 sizes the opportunity: at 8,000 nodes, 19.77%
      of clean tablebase wins were not won.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The Syzygy relabel prior is +6.73 on an adjudicated corpus and may
        differ on natural termination (C-60); failures to mate in KR-K labels
        (C-61); relabelling cannot repair the larger rows of failed games
        (C-63); whole-game tablebase adjudication is a different change
        (C-64); blended targets lost (C-67).
    - [ ] **C.2.5** `[R2]` Initialization control
      Initialization control (archived 8.6): fit from the accepted and from
      neutral starts under identical K, data and schedule; register the
      production rule before the test split opens.
      The piece-square tables start from B.10's flipped values, whatever
      B.10's gate reads (maintainer decision 2026-10-23). A neutral PST start
      is not required, and replacing PeSTO-derived values is not a goal.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Rarog's control: a neutral start fitted 2.38% worse (-13.2 clustered
        SE; C-65), consistent with the 2026-10-23 decision.
    - [ ] **C.2.7** `[R2]` Endgame instruments and gate integrity
      It was C.7.1 until 2026-11-11. Instruments and gate integrity
      (archived 6.6.a–e and 6.6.h).
      - Audit the tools against the snapshot's endgame instruments
        (`tools/diag/endgame_*.py`), and classify
        every delta as portable, adapter-specific, covered or inapplicable
        before changing any instrument.
      - Add versioned report schemas, cohort digests and serial-identical
        sharding.
      - Add known-bad guard tests.
      - Measure deployed nodes per move and derive Basilisk's own budget
        bracket.
      - Add FEN-hash holdouts, McNemar and spent-cohort rules.
      - Re-run only the baselines this invalidates.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The floors use one cold search at one budget (C-25); bracket 60k, 200k
        and 600k (C-137); the drawn-overclaim instrument from Syzygy draws
        (C-133) and the Syzygy-verified vetoes in the unit suite (C-142)
        are B.9.3's enumeration, read again here on the C head, not built
        twice; measurement layers on reports (C-141); replay tools must not
        read bound scores as evaluations (C-145); KBNK as a multi-budget
        majority (B-3).
      - The colour-mirror test (`test_eval.cpp`'s symmetry sweep) extended
        to `tests/endgames.epd` (14 of 17 positions have White stronger),
        `endgame_draws.epd` and the recogniser positions (Rarog EVAL-2,
        D.5.17; `analysis/rarog_sweep_2026-11-04.md`): Basilisk has had
        one colour asymmetry already, the pawnless tie.
    - [ ] **C.2.8** `[V]` Matched fits and the label-contract gate
      It was C.2.6 until 2026-11-11.
      Matched whole-surface fits of A and B on the current surface, then the
      registered label-contract gate (archived 8.7–8.8); fit loss explains,
      the SPRT decides.
      The piece-square tables re-fit here, from B.10's flipped values: they
      have never been fitted in their correct orientation, and the old
      `1e-6` anchor moved them about 0.1 cp. The registration sets their
      anchor loose enough to let them move. The fit reports each table's
      movement (B.10's directional screen), and the gate tests the refitted
      tables (maintainer, 2026-10-24). A refit that moves the tables as
      Rarog's did (C-66) also ends their PeSTO derivation: when the
      refitted tables land, the originality disclosure of the
      PeSTO-derived tables (1.10.2's audit; E.3's D-69 item) is retired
      and the similarity screen is re-run (maintainer, 2026-10-27).
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - A free PST refit moved Rarog's tables 11-57 cp an entry, which sizes
        the anchor (C-66); B.10's PeSTO-distance test will fail on a correct
        refit, so the two-arm fixed-K direction test replaces it (C-22, C-23);
        one causal axis per corpus gate (C-57); the magnitude contract (C-97).
- [ ] **C.3** `[I2]` King safety and phase cluster
  King safety cluster in the donor's shape: attacker units and weights, safe
  and unsafe checks by piece, weak ring squares, flank attack and defence,
  shelter and storm with the castling alternative, the queenless reduction and
  the nonlinear danger map; refit; gate.
  Opens as sub-steps (research, implement, refit, gate) when C.0's handoff
  reaches it; the same holds for C.4, C.5 and C.6.
  The phase and tempo join it (from C.7, 2026-11-05): E2's phase and E5's
  tempo change together with E1's eg king danger, never one at a time, and
  before the other families, so that each later refit reads the final
  phase.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Definitions: king blockers of either colour (C-13, `eval.cpp:1278-1280`);
    the donor's safe and unsafe checks, check presence, the weak ring and the
    ring and zone shapes (C-14); storm without blocked pawns and the king-file
    sign (C-15).
  - Priors: the danger map carries the king residual and shelter and storm
    about 20%, while flank attack and defence, king-pawn distance and the
    pawnless flank were nil in Rarog (C-111); Rarog's open E.4: the remaining
    lead is the index's structure, since refitting its coefficients from donor
    seeds failed (RAR-E73); zero-fitted inputs are a resolution limit (C-17);
    the clamp is live (C-113); a donor fixture (C-112).
  - Guards: shelter in the pawn cache needs the king square and castling in
    the key (C-43); a won KRP-KR static guard before any endgame danger term
    (C-44).
  Code audit inputs (2026-10-27 second pass, §6.3):
  - E1: king danger is middlegame-only (`eval.cpp:1594`, `mg -=
    safety_table[units]`; the donor also subtracts `kingDanger/16` from
    eg) and lacks the donor's inputs: the mobility difference, the
    knight-defender relief, `kingAttacksCount`, pawns attacking the
    ring, the shelter coupling and the attacker-count × weight product.
    Bishop and queen attacker units are fitted 0 (C-111). Double
    counting: the open-file count enters the danger units
    (`:1576-1582`) on the files `shelter_missing_*` already penalises
    (`:1615-1627`), and the king-ring bonuses fire beside the attack
    units where the donor's `else if` keeps them exclusive. Measure
    first: the donor-direction residual screen on these inputs by phase
    and king danger's firing share at phase ≤ 12; the lead is index
    structure (RAR-E73).
  - E2's phase: pieces count N = B = 1, R = 2, Q = 4 of 24 against the
    donor's npm between its endgame and middlegame limits; changed with
    the eg king danger here, never alone, and with tempo (E5: tempo is
    mg-only, 22, so it tapers to 0 in pawn endings and is scaled with
    the score, where the donor adds a constant 28 after scaling; the
    lazy path omits it, C-6).
  Rarog sweep input (2026-11-04, `analysis/rarog_sweep_2026-11-04.md`;
  Rarog E.4's first step, EVAL-3):
  - In the donor's king-danger inputs bishops see through queens, rooks
    through queens and own rooks, and a pinned piece's attacks are cut to
    the king line (`sf9587-ks` `evaluate.cpp`); Basilisk's maps are plain
    occupancy (`eval.cpp`, `all_pieces()`). C-115's x-ray nil was
    mobility's alone. Rarog's C.3 fixture came from a donor patched to
    plain maps, so a donor fixture here is taken from the unpatched donor.
- [ ] **C.4** `[I2]` Threats and mobility cluster
  The mobility area, weak enemies, hanging pieces, restricted squares,
  pawn-push threats and queen threats (including BAS-E07's absent
  `KnightOnQueen` and `SliderOnQueen`); shared attack maps from C.1; refit;
  gate.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - SliderOnQueen is missing; screen KnightOnQueen before building it (nil in
    Rarog); ungated forms were worse than none (C-114); mobility priors
    (C-115); the dead `hang_pen` loop (C-37; A.16 removed it).
  - Code audit 2026-10-27 (§6.3): the safe-pawn threat is a flat 58/25
    whatever the target and counts pawns that are themselves unsafe
    (`eval.cpp:1403-1426`), overlapping hanging, restricted and
    weak-queen on the same target.
- [ ] **C.5** `[I2]` Pawns and passers cluster
  Passer king proximity, blocker ownership, path safety, unstoppable and
  rook-behind conditions; structure conditionality (doubled, isolated,
  backward, connected, levers); refit; gate. The dispositions of the archived
  6.3 passer study are its prior.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - A rear doubled passer scores as a full passer (C-11); the opposed and
    backward definitions (C-12); supported and phalanx share one table (C-16);
    passer priors (C-117); if an unstoppable-passer rule is added, avoid
    Rarog's off-by-one (C-45).
  Code audit inputs (2026-10-27 second pass, §6.3):
  - D2, a definitional repair: `passed_path_safe_eg` (9 × rank,
    `eval.cpp:1740-1745`) is not gated on an empty stop square as the
    free and safe terms are (`:1155`), so a blockaded passer with an
    undefended blockader is paid for a "safe path":
    `8/3n4/3P4/8/8/8/k7/4K3 w - - 0 1` reads +45 eg for a pawn that
    cannot move; the donor pays nothing (`:702`).
  - E4: rank is carried five times — the PeSTO pawn eg table (about
    +110 on the seventh), `passed_eg[r]`, and the free, safe and path
    terms, each linear in rank, with free and safe firing together on
    most endgame passers: collinear coordinates for C.2.1's manifest.
    The king-to-block step (19) is the same on rank 2 as on rank 7
    (C-117: the donor weights it by rank); free and safe sit before the
    lazy cut, proximity and path after it (C-6).
  - Absent: the donor's weak lever (eg 56) and passed file; a
    lever-based candidate passer (C-117).
  It was C.6 until 2026-11-05.
- [ ] **C.6** `[I2]` Material, imbalance and pieces cluster
  Imbalance in the donor's quadratic form, bishop pair and bishop-pawn
  colour, outposts (including BAS-E07's absent `BadOutpost`), long diagonal,
  rook files, trapped rook, queen weakness and king-protector distance;
  refit; gate.
  Outposts, `reachable_outpost`, mobility and `queen_infiltration` were
  fitted around seven rank-mirrored piece-square tables (B.10). Their trace
  reads them for compensation the flip left behind.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Outposts on ranks 5-8 against the donor's 4-6 (C-10); dead and collinear
    imbalance slots (C-20); queen infiltration and the trapped bishop (C-48);
    no trapped rook (C-46); no WeakQueen (C-116); screen first (C-118).
  Code audit inputs (2026-10-27 second pass, §6.3):
  - E2's phase and E5's tempo are C.3's (2026-11-05), changed with its
    eg king danger.
  - Bishop terms: the bishop outpost is absent although comments at
    `eval.cpp:1783` and `eval_params.h:222` say it exists; bishop x-ray
    on pawns is absent; bishop pawns is a linear count without the
    donor's factor for unprotected and blocked central pawns; the minor
    behind a pawn counts own pawns only. Screen first (C-118).
  - Doubled penalises every pawn on a file with two or more, supported
    or not (`:965`; the donor only the rear pawn when unsupported);
    Basilisk-only terms with no donor prior: pawn majority, pawn
    islands, closed file, seventh rank, connected rooks, the trapped
    bishop.
  It was C.7 until 2026-11-05, when the phase moved to C.3.
- [ ] **C.7** Endgame handling and winnability
  The goal is measured conversion and correct draw recognition where games
  actually go, not coverage of a function list.
  After the families it builds on (C.5 until 2026-11-05): the scalers and
  recognisers act on the pawn and material evaluation, so the endgame
  cluster follows C.5 and C.6. C.0 orders the clusters by expected value
  within this constraint, and the order is discussed again when reached.
  Its instruments leaf, C.7.1, moved to C.2.7 on 2026-11-11, so the
  cluster starts at C.7.2.
    - [ ] **C.7.2** `[R3]` Occurrence, classification and ranking
      Occurrence, classification and ranking (archived 6.7.a–d).
      - Board and tree occurrence with per-root concentration.
      - Each family classified as exact recognizer, scale, move-quality or
        conversion work, with its deciding instrument.
      - Every dispatcher's promotion and material-shed closure.
      - A re-rank by local defect × occurrence × deployment budget.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - A game-level ending audit from gate PGNs (C-134); per-root
        concentration (C-138); promotion-closure accounting (C-140); the
        recogniser rule and kinds (C-135, C-136, C-127); missing value
        functions (C-132); a material-signature table (C-106); A.11.9's
        boundary research feeds here.
      - Code audit 2026-10-27 (§6.3 E7), measured and switched in C.10
        (2026-11-05), where the search surface is refit; B.14 already took
        the draw verdicts' half. The original finding: correction history
        overrides exact recogniser scores — the drawn KPK, KNNK and
        wrong-bishop 0 and the scaled endings are corrected through pawn-,
        minor- and
        non-pawn-keyed tables (`search_kernel_core.cpp:195`, `:508`;
        `search_history_core.cpp:158-174`) whose keys won and drawn
        positions share, so a dead draw can stand at ±X.
    - [ ] **C.7.3** `[I2]` Generic winnability and scaling
      Generic winnability and scaling in the donor's form: pawn-count scaling,
      opposite-bishop scaling, rule-50 inside the scale and a complexity term;
      floored so a misfire discounts rather than throws a win.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - OCB: neutral from four pawns, no passer term, fires with other
        material (C-4; Rarog's `1 + 2 pawns + 10 passers` read +12.7 nElo);
        OCB below seven men (C-125); every scale floored (C-126); rule-50
        damping against the drive weights (C-128); initiative's unfitted
        `win_*` weights (C-47; A.16 removed the term and its slots, so C.7.3
        builds the donor's from nothing). The tuner has no winnable
        instrument, so the complexity term needs one before its first fit.
      - Code audit 2026-10-27 (§6.3 E3): the OCB (`eval.cpp:869`),
        pawnless (`:841`) and rook-ending (`:860`) scalers multiply the
        whole tapered score where the donor scales eg only, so with
        queens and opposite bishops at three pawns or fewer the
        middlegame half, king danger included, is cut to 32/48–44/48;
        the donor's order is winnable on mg and eg, scale on eg, taper,
        side to move, tempo, rule 50. Absent donor scalers: the
        one-flank rook ending (36/64) and the lone queen against minors
        (37 + 3 per minor). Firing share of the scalers at phase > 0 and
        a Syzygy residual on those cohorts first. A.13.2 repairs the
        pawnless classifier's units before this leaf and leaves its
        truth-table test for this leaf to keep.
    - [ ] **C.7.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
      Won-rook-ending only-move precision (archived 6.8.a; BAS-E53): all 68
      one-winning-move nodes studied with a held-out split; search, evaluation
      and knowledge explanations compared; a mechanism or a justified
      no-change. More depth is already refuted.
    - [ ] **C.7.5** `[I2]` Group B families by measured kind
      Group B families by measured kind (archived 6.8.b–e).
      The families: KR-KP, KQ-KRP and KR-KB; bishop-pawn fortresses and
      promotion races, with the KBP-K deficit treated as bishop-pawn
      technique. Deterministic theory, truth and closure cases come before
      fitting, each shown to fail on its defect. The covariant local cluster
      is fitted on training and validation data only.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KR-KN and KR-KB at a flat 4/64, and KRP-KB unscaled (C-129).
    - [ ] **C.7.6** `[R2]` Lower-yield remainder
      Lower-yield remainder (archived 6.10.a–d): KPs-K and KP-KP first; KQ-KP
      and KQ-KR reconciled across budgets; KR-KN, KNN-KP and KNN-K theory and
      rule 50; implement only what has a measured defect, occurrence and a
      valid closure.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KNN-KP: an additive drive plus a blockade term, never a replacement
        score (C-130); the KXK corner-to-king ratio (C-39); the KBN-K drive's
        speed and a distance-to-mate table option (C-144).
      - A prior for KQ-KP (2026-11-11, `analysis/rarog_import_2026-11-11.md`;
        Rarog's D.6.24): Rarog's fortress rule zeroed 172,256 positions with
        the pawn on the seventh, and 26.7% of the quiet ones are wins for
        the queen, most with her king three or four squares from the
        queening square. A rule here discounts, never zeroes, and is read
        by B.9.3's enumeration before it lands.
    - [ ] **C.7.7** `[V]` Endgame gate and closure
      Endgame gate and closure (archived 6.9, 6.10.e, 6.11).
      - Gates are tiered by occurrence: a normal STC gate for common
        families, endgame-start cohorts for medium ones, and truth plus a
        loss-permitting SPRT for tails.
      - No correctness veto may be traded for aggregate gain.
      - Cohorts, budgets and floors are frozen, and every reject, gap and
        retry trigger is recorded.
      - KRPPKRP's seven-man truth gap is recorded as an explicit exclusion
        unless independent truth appears.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KRPPKRP has truth: the 7-man tables are installed, so this leaf's
        wording is stale (C-24); gates without tablebases expose static
        small-endgame scores (C-147).
- [ ] **C.8** `[V]` Refit cycles
  Refit cycles (archived 8.9): regenerate data with the accepted head, refit
  the whole surface with a new untouched test split, and gate each cycle;
  repeat while a cycle accepts; stop at the first that does not.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Refit cycles diminish (+11.0, then +6.7 in Rarog), so size from earlier
    cycles' games (C-123); priors (C-68); scores re-dumped after each refit
    (C-69).
- [ ] **C.9** `[V]` Nonlinear HCE SPSA or a written skip
  Nonlinear HCE SPSA (archived 8.10): only live, frequent terms Texel cannot
  price, such as the king-danger map, from a zero-game activation and
  curvature screen; a written skip if the surface is flat.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - The nonlinear sweep (C-122); initiative and mop-up are untraced (C-47,
    C-36); the king-danger clamp (C-113).
- [ ] **C.10** Joint re-fit on the new evaluation
  One joint SPSA over the registered cp-valued search coordinates, including
  B.4's value surface if it exists, then PGO and SPRT `[0,3]`.
  Scope (maintainer 2026-11-12): this is the classical stage's big fit,
  properly investigated and as long as it needs: it finds the joint
  optimum of the engine's parameters on the new evaluation and finishes
  the best HCE engine before C.12's throughput work and C.13's release.
  Several nights are acceptable, with large early moves and a slowly
  decaying schedule. Rule 7c's ceiling does not bind it; its own
  registration sets the horizon and the stop.
    - [ ] **C.10.1** `[R2]` Joint-fit investigation and registration
      Registers the surface, the horizon, the schedule and the stop
      before any game. Decided from this leaf's own analysis, not from
      expectations set earlier (maintainer 2026-11-12). Its questions:
      - which coordinates join: every live search coordinate that passes
        B-102's relevance read on the new evaluation, and evaluation
        coordinates if the analysis says they belong, omitted if it says
        they do not.
      - B-107 (search and evaluation never share a tune) is Rarog's old
        lesson, not a settled rule here: this leaf verifies it, reading
        the Rarog evidence behind it and testing it on Basilisk's own
        engine before the surface is chosen.
      - the per-coordinate steps from curvature, and the starting steps;
      - one long decaying schedule against restarted blocks, and the
        horizon's games against the drift model. B.5.6's packet read a
        restart as a higher gain by another name, with no unlock (its
        §4 and §7): an input here, not a ruling;
      - the gate's bracket and cap for a fit of this size.
    - [ ] **C.10.2** `[V]` The joint fit, PGO and gate
      Runs C.10.1's registration, then the fitted-constant analysis, the
      bake, PGO and the SPRT.
  Cluster 1's margins (RFP, futility, razoring) were fitted on the eval
  before B.10's flip, which moves it by 31.7 cp mean; B.6.7's joint fit
  re-reads them on the B head, and this leaf refits them on the new
  evaluation.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - `LAZY_MARGIN` refit (C-98); correction-magnitude terms on the surface
    (B-69); search and evaluation never share a tune (B-107); `scale_ratio.py`
    after B.10 and C (B-108); the qsearch margins as coordinates (B-25); a
    runtime lazy margin clears the caches (C-42).
  - Code audit 2026-10-27 (§6.3 E7; C.5.2's until 2026-11-05): first the
    correction's size at the scaled endings' recogniser returns (B.14
    already took the four draw verdicts), then the switch, skip the
    correction when the evaluation came from a recogniser, read with the
    search surface.
  - The lazy-evaluation thresholds, a behaviour-changing question audited
    beside `LAZY_MARGIN`'s refit, moved here from C.12 (2026-11-05) so that
    C.11 freezes them: the lazy cut is one stage at 700 on the tapered
    cheap score with no npm term, where the donor uses two stages with a
    margin growing by npm/64 (about +260 at full material), and it skips
    king danger, threats, mobility, passer proximity and path, the mop-up
    drive and tempo (code audit §6.3 E8).
- [ ] **C.11** `[V]` Freeze the classical evaluation
  Checkpoint and freeze (archived 8.11).
  - Re-measure the same-search deficit against a fresh classical-Stockfish
    hybrid at the C head, plus conversion, the truth cohort, NPS and a 1T
    pool gauntlet.
  - Revalidate the score scale, mate bounds and every endgame floor.
  - Its 1T pool gauntlet, clock- and node-annotated, is D.1's loss-analysis
    source: C.12 is bit-exact, so those games are the play of D.1's entry
    head (2026-11-05).
  - Ablate low-information mechanisms.
  - Freeze the classical evaluation.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Conversion compared within one pool only (C-146).
- [ ] **C.12** `[I2]` Evaluation throughput, bit-exact
  Evaluation throughput, bit-exact (archived 8.12).
  - Profile terms, phases and cache paths first.
  - Optimize only measured hot paths, with exact bench plus direct
    score-corpus identity.
  - The lazy-eval thresholds are C.10's, audited before the freeze
    (2026-11-05); this leaf keeps every score identical.
  - Gate with interleaved pooled NPS and a natural-termination SPRT.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Played and full scores dumped (C-89); cache purity (C-105); a runtime lazy
    margin clears the caches (C-42); the profile comes from A.9.5.
  - Code audit 2026-10-27 (§6.3 E9): there is no material or imbalance
    cache, so the twelve-popcount census and the imbalance are rebuilt on
    every call (the pawn cache is pure); a cache that keeps every score
    identical belongs here. E8's lazy cut is C.10's.
- [ ] **C.13** `[M]` Interim release after the evaluation programme
  Maintainer decision 2026-10-24: the evaluation programme's head is released
  as 1.12.0 once C.12 closes, as Rarog cut 2.6.0 from its C head. Licensed as
  B.11.4 is: an accepted SPRT over the 1.11.0 release at `3+0.03` 1T, `10+0.1`
  and 4T direction checks whose 95% intervals exclude a loss, and a 4T pool
  read beside them, the 4T reads sibling-free. Cut through A.8.6's tag-driven
  flow, after B.11.3's scans are repeated on the candidate (PLAN §5).

## Phase D — Clock, threads, tablebases

- [ ] **D.1** `[R2]` Time management
  Basilisk's clock (optimum and maximum, `Move Overhead` 10, BAS-R03's
  instability extension, helper handling) audited against Stockfish's shape
  and against Manta's ADR-0065 checklist; SPRT `[0,3]` at STC, a `10+0.1`
  direction check and a 4T zero-forfeit run (archived 9.7).
  Stockfish's shape: optimum and maximum from clock and increment, with the
  total scaled by falling eval, best-move stability, best-move instability
  across threads and best-move node effort.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Sub-second horizon contraction: assuming fifty moves to go drifts a
    `3+0.03` game to about 155 ms of clock (A-45); hold and reserve (A-46);
    the long-clock stall probe on the PGO build at `180000+2000` (A-47; the
    2026-10-24 probe in `tools/results/stall-probe-20261024/` passed, but
    BAS-R07's tree variants run 2.8–4.1× the optimum there through the
    give-up window, which the fail-high stop cannot reach); a mate
    stop beyond mate in 1 (A-49, B-78); ponder credit (A-42); time changes
    read by the opening's time a move and the clock left (A-54); the
    fail-high stop's threshold if B.5's read calls for it (A-48).
  B.5 inputs (2026-11-09; `analysis/b5_cluster4_research_2026-11-08.md`,
  *Review — B.5.2 accepted with named follow-ups* §1 and §6, and
  research amendment 2):
  - The hole on Cluster 4's arm (A-47). Half of its long root searches
    begin after the optimum: the end-of-iteration test lets an
    iteration start there while the root's best move keeps changing,
    and that iteration does not return. The stop packet's definition
    assumed a window starting before the optimum. On 60 decided endings
    the arm changes its root move 1.7 to 1.8 times as often as the head
    and spends a third more clock, and no stop setting reaches those
    searches. B.5.3's M1 reads the rate by score band on both arms.
  - The effort share divides the last search's best-move nodes by every
    window's nodes of the iteration. With the arm's re-searches the
    share of iterations at or under `TmEffortLo` rises 12 points at
    fixed depth (44.5% to 56.6%); the share at or over `TmEffortHi`
    does not move.
  - The window's width sets these inputs (2026-11-10; BAS-S54 *AUDIT
    READ*, 60 replayed games). From `B5AspBase` 1410 to 19000 the
    fail-high stop ends 27% of searches against 10%, the share at or
    under `TmEffortLo` falls from 36% to 18% (the OFF arm 22%) and the
    share at or over `TmEffortHi` rises from 21% to 32%. B.5.4 fits
    the window under today's measure and the stop BAS-S55 (c) leaves,
    so a change here to the measure or the stop re-reads `B5AspBase`
    and `B5AspGrowth`.
  - The stop is at 2× on the arm (2026-11-10; BAS-S55 *RESULT*): H1,
    +9.3 ± 4.4 Elo over 8,260 games against 1×. The census on that
    PGN reads 1× spending less a move on even positions (78.3 ms
    against 81.7) and more on won and lost ones; 2× leaves more won
    moves at half the clock or more (2.32% against 1.85%), the hole.
    Unexplained and read here: the runner's summary gives 10% more
    nodes a move at 2× for 1.6% more time, from one binary.
  - Cluster 3's arm spends 7 to 8% more time a move in the won band
    than its base at both clocks (the B.4.7 runs, by amendment 2's
    census). A side whose own score reached 1,500 won all 2,260 such
    games there.
  - The clock test inside an iteration stays in this leaf (maintainer
    2026-11-09). It is pulled into Phase B only if B.5.5's long-clock
    condition fails with `B5NegExtMax` 1 unavailable.

  The ADR-0065 checklist:
  - distinct optimum and maximum budgets;
  - the maximum rooted at `go` receipt and never extended;
  - ponder credit used once;
  - the optimum adjusted only by completed iterations;
  - an easy root spends less;
  - a minimum completed depth;
  - helper contributions normalised by helper count.

  Each item is ticked present, absent or different before the donor shape
  is chosen. Read 2026-11-11 (F8 of `analysis/rarog_import_2026-11-11.md`):
  the easy-root item is absent. A root with one legal move, or a one-move
  `searchmoves`, is searched for its whole budget; Rarog's lesson is that
  a forced-move rule must not fire for a one-move `searchmoves` (its
  D.3.8). The forfeit margin is sized on a null pair: BAS-E57's reserve
  sweep lost −64.81. The won-endgame time sink is A.8.16's, not this leaf's.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Instruments, built in B.16 and read here first: the loss analysis by
    cell (B.16.3, TT-1; `tools/diag/loss_by_cell.py`, BAS-M14's method)
    says which clock situations lose games. It is run here, first, on the
    clock- and node-annotated games of C.11's pool gauntlet, the play of
    this leaf's entry head against stronger engines (C.12 is bit-exact);
    the wall-clock read (B.16.1, TT-2; `tools/diag/wallclock.py`) is
    re-read on this leaf's head, about seven minutes on an idle host, and
    sizes the overhead the budget must leave, B.16.1's numbers its prior.
  - Method: time levers are measured at the time control they act at and
    tuned against an external engine at release speed, since self-play
    rewards the side that spends more (TT-3); a fitted moves-left horizon
    and a game-time ramp between fitted and tuned values (TT-6).
  - The defects to test for (TT-4) and the first-iteration stop guard
    (A-50) are B.9.1's since 2026-11-05.
  - Negative priors: a lower cap on the longest thinks (−27.1 ± 17.0 at
    40+0.4) and an expected-reply depth floor (TT-5).
  - Its rules on top of a Stockfish-shaped clock (surprise, behind on the
    clock, defence window, early saving, the jump guard; each +5 to +7 at
    40+0.4 and about zero at short controls) wait in H.3.
  Restructure and Rarog sweep inputs (2026-11-05,
  `analysis/rarog_sweep_2026-11-04.md`):
  - The total scaled by best-move instability across threads is D.2's:
    it depends on the helper design D.2 settles, so it is fitted there.
    This leaf fits the clock at one thread and runs the 4T zero-forfeit
    check on the current helpers.
  - A negative prior for that factor: Rarog's pool-view instability lost
    −5.54 (RAR-R05); its retry condition is instability as one input to
    a completed-root confidence, and root instability from a completed
    root snapshot read +6.46 (RAR-X06).
- [ ] **D.2** `[R2]` Lazy SMP quality
  Self-relative scaling before any design: each engine against itself at
  4T against 1T, and against twice its time at 1T, so that four threads
  are priced in the engine's own doublings. Stockfish 18's and Reckless's
  legs are imported, sibling-free, from Rarog (BAS-X41: Stockfish 18 D
  1.54, Reckless 1.49, Rarog 1.36); Basilisk plays its own threads and
  time-odds legs (and 8T, without an imported reference) with the host
  sampler beside them (`colosseum.ps1 -ThreadsB` and `-BaseMsB`,
  `tools/host_sampler.ps1`, `tools/diag/smp_read.py`), plus a 200-game
  control of Stockfish 18's threads leg that must land inside the imported
  interval, or the host has changed and the references are re-played.
  Stockfish 18 is a reference; the donor stays Stockfish 19 (maintainer
  2026-11-05).
  Before those games, the zero-game reads (about fifty minutes in Rarog):
  time-to-depth at fixed depth, 4T against 1T (RAR-R29: four threads
  reached a fixed depth with about four times one thread's nodes, X
  0.87); the main thread's TT hit rate at 4T against 1T (RAR-R33); B.5's
  S6 counter (the root TT move against the last iteration's best) at 4T.
  The design questions: helper diversity, shared TT, shared correction,
  pawn and continuation histories (Stockfish shares them per NUMA node) and
  soft-stop voting. The gate is a 4T SPRT `[0,5]` with a null pair first,
  and only against a measured defect (archived 9.10.b; BAS-R04–R06).
  BAS-C05's TT publication incoherence is a known defect owned here: a
  coherent layout that costs no measured NPS replaces it (both earlier
  repairs cost 3.91% and 1.22%).
  Engine audit inputs (2026-11-12,
  `analysis/b64_engine_audit_2026-11-12.md` §6): the table's rules
  where they differ from the donor's and no record says why, read at
  the hash and control this leaf measures at:
  - Replacement ages an entry by 2 a search (plus 2 for an exact
    bound); the donor by 64. A current entry was evicted beside a
    stale one in 0.02% of live evictions at Hash 64 over 610 self-play
    searches and in 1.9% at Hash 16 in `bench 17`: nothing at the
    gate's conditions, something when the search outgrows the table.
  - A declined same-key store drops its move (the donor writes the
    move first): 0.28 to 0.36% of same-key stores.
  - No PV bonus in the overwrite test and no secondary aging of a
    decisive inexact entry; nothing stored at a node without moves
    (the donor stores at `depth + 6`).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Self-relative 1/2/4/8T scaling and hash adequacy (D-44, D-45); the first
    touch of per-thread tables before the first `go` (D-21).
  - Code audit 2026-10-27 (S6's helper side): helpers overwrite the root
    key the main thread reads its first move from; read with B.5's S6
    counter at 4T.
  - From B.5.2's review (2026-11-09; the B.5 packet, *Review* §3): on
    Cluster 4's arm the helper's root filter numbers the moves that
    pass `searchmoves` and the tablebase filter, in generation order;
    the old root numbers them in the picker's order, before the
    tablebase filter. `multipv_lines` is counted without the filter, so
    a filtered main thread with more lines than its record would index
    past it. The pool sets no filter today, so neither is reachable;
    the helper design this leaf settles takes one form.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - A candidate for BAS-C05's coherent layout (TP-1): 16-byte entries, four
    per 64-byte bucket, each verified by XOR (a 48-bit key tag and the
    static eval in one word, XORed with the data word, so a torn write
    fails the check), the bucket chosen by multiply-high so a Hash that is
    not a power of two is used whole. It read +6.30 ± 5.06 at Hash 16
    against its own 24-byte layout. Here it trades about a third of the
    entries per MB for coherence; BAS-C05's retry trigger names exactly
    such a redesign, priced against the −3.91% and −1.22% repairs.
  - The store race (TP-2): reusing an entry's old move and eval after
    another thread rewrote it; re-check the tag first.
  - Per-thread state aligned against false sharing at every make and
    unmake (TP-3).
  - Priors (TP-4): a fine-grained helper LMR bias +4.91 ± 7.97 at 8
    threads (from Reckless); thread vote −3.5 and optimism per thread
    +0.9; a helper one iteration ahead on the PV, no gain.
  - Contracts (TP-5): tested in B.9.1 since 2026-11-05.
  - Tests that land with the layout decision (2026-11-11; Rarog's
    D.5.17, TT-12): a deterministic torn pair the validation must
    reject; a false-hit bound from a seeded key set on a small filled
    table (at 5,000 entries in 16 MiB an 8-bit tag would pass unseen);
    a four-thread hammer on one cluster. B.9.1's ThreadSanitizer cell
    drops its one suppression if the layout becomes coherent.
  Restructure and Rarog sweep inputs (2026-11-05,
  `analysis/rarog_sweep_2026-11-04.md`):
  - The total time scaled by best-move instability across threads, from
    D.1, fitted after the helper design, with D.1's RAR-R05 prior.
  - The thread-result choice: Basilisk takes the deepest thread, then the
    higher score (`search_thread_pool.cpp`, `merge_results`); Rarog keeps
    a score- and depth-weighted vote it judged stronger, unmeasured;
    Triumviratus's thread vote read −3.5 (TP-4). Read, not assumed.
  - Rarog's legacy SMP negatives, not in this leaf until now (the
    2026-10-24 audit read them; the mapping was lost): a symmetric early
    stop vote at 2T lost −15.85, because taking the first of the votes'
    expiry estimates biases time downward, so the soft target is an
    estimate and the maximum is the budget (RAR-R04); helper-history
    blending read −0.52 and its ordering jitter was reverted, with the
    advice not to escalate to sharing continuation or pawn histories on
    that prior (RAR-R06); helper iteration staggering showed nothing once
    read with enough samples, NPS scaling 1.81/3.95/7.88/12.31 at
    2/4/8/16T beside a depth at fixed time 0.8 to 1.0 ply shallower and
    usable TT cutoffs per probe +76% from 1T to 16T (RAR-R07 to RAR-R10).
  - The method those reads taught: one engine process per cell, arms
    alternating, 40 to 80 pairs, and a null run under the identical
    protocol; an n = 6 cross-process time-to-depth read swung 21.6%.
- [ ] **D.3** `[R2]` Tablebase policy
  It was D.4 until 2026-11-05, when the lifecycle step D.3.2 moved to B.9.
  Probe depth and limits, WDL and DTZ in conversion, interaction with C.7's
  recognizers; the root and PV lines already follow Stockfish (BAS-C13), and
  in-search probing does from A.8.14.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - DTZ ranking (A-59). The cold root reads on the clock (A-57) and the
    root ranking that ignores the game's repetitions (A-58, which cost
    BAS-S39's game 4965 a clean tablebase win; `tools/results/a1134-tb-gate/`)
    are B.9.1's since 2026-11-05, repairs before the release.
  - Code audit 2026-10-27 (S11): A.13.4 moves the in-search WDL probe
    after the TT probe and cutoff, the donor's order; probe depth and
    limits stay here.
  - To read against Basilisk (2026-11-11, `analysis/rarog_import_2026-11-11.md`;
    Rarog's PROTO-9 and PROTO-14, found in itself): whether the PV
    extension and the root gate on the loaded tables' size or on
    `SyzygyProbeLimit`; whether `SyzygyProbeDepth` gates every probe
    where the donor gates only the largest loaded size.

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
  Attribution checkpoint (archived 9.9): the final head against 1.10.1 and the
  B.11.1 and C.11 heads at STC, `10+0.1` and 4T; attributed Elo per programme
  from the accepted SPRTs; every deficit meter; NPS; the maturity checklist.
  - The architecture counts of the 2026-10-27 audit (§3) are measured
    again on the release head with `tools/diag/source_census.py`
    (Rarog's E.1 re-runs its architecture review the same way).
  - With tables (Rarog H30, `analysis/rarog_sweep_2026-11-04.md`): a read of
    the head against the last release with Syzygy loaded at STC, because
    the search probes on its own (A.8.14, A.13.4, D.3's policy) and CCRL's
    testers use tables; conversion and threefold counts beside it (BAS-S39's
    game 4965).
- [ ] **E.2** `[V]` Classical target gate
  Target gate: the pool measurement of section 1 at 1T and 4T. Met, or not met
  with the measured shortfall per engine recorded. The 4T arm is
  sibling-free (PROCESS *Harness*, BAS-X41); BAS-M11's 4T baseline was
  played unpinned.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Pools on ARM hosts record native or emulated per engine and do not count
    for E.2 (D-35); opponents at 10 ms overhead (D-41); a field that brackets
    the engine, with Rarog's CCRL map (D-42); the hash rationale at 4T (D-45);
    contempt as the registered lever, as in Rarog's E.2 (C-121); 1.9.3's clock
    reached -22 ms in one M4 game under a 2,000 ms margin (audit,
    tournaments).
- [ ] **E.3** `[M]` Classical release 1.13.0 after Phase E
  Release (archived 9.10): **1.13.0**, whatever E.2 reads; E.2's result is
  recorded, not encoded in the version (maintainer decision 2026-10-24,
  replacing "2.0.0 if E.2 is met, else 1.11.0"; 2.0.0 is the first NNUE
  release, F.9):
  changelog, CTest and sanitizers, target-native ISA checks, reproducible PGO
  assets, prior-release STC, LTC and 4T gates, cut through A.8.6's flow on
  maintainer instruction. (E.3.1 moved to A.8.6 on 2026-10-06.)
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Originality and licence disclosure before the cut (D-69; Rarog's E.3.0):
    B.11.3's scans repeated on the candidate and its disclosure brought up
    to date (the PeSTO-derived tables until C.2.8's refit retires them,
    Stockfish 19's time-manager constants as D.1 leaves them); short
    user-facing text (D-71); a 4T pool read beside the head-to-head (D-43).

## Phase F — NNUE (own data only)

**Rules.** Networks train only on data generated by Basilisk's classical head
and later its NNUE heads. Stockfish is the runtime donor. The trainer
(`D:/code/net_trainer`, whose `.mnn` format is the cross-engine contract,
against Bullet) is chosen at F.0. The architecture ladder is ours. The
classical evaluation stays in the tree as the datagen baseline and the
fallback, which is why the archived "optional HCE fallback" phase is dropped.
The classical evaluator in releases (Rarog's Phase F rule of 2026-10-22,
adopted 2026-11-05; `analysis/rarog_sweep_2026-11-04.md`): the HCE ships as
a selectable evaluator in the one binary only while it is the strongest
classical version. Each release whose search changed reads its classical
arm against the strongest classical release (2,000 games at STC), and a
loss whose interval excludes zero drops the option. The scale-sensitive
search constants carry one overlay per evaluator, fixed at F.6, so that a
fallback never plays with network-fitted margins.

**Order** (2026-11-11, Rarog's of 2026-11-05;
`analysis/rarog_import_2026-11-11.md` §4): no step waits for a later one.
The engine reads a network exactly
before any is trained (F.1, F.2.3); the trainer is proved on a pilot
slice and sizes the corpus (F.2.4) before the corpus is generated
(F.2.5); the first nets are judged in games only once inference runs at
speed (F.5, F.6). F.2.1, F.2.2, F.3 and F.4 are retired numbers
(HISTORY's number map).

- [ ] **F.0** `[R3]` NNUE investigation
  Investigation (archived 10.0–10.3): board events and accumulator ownership,
  the trainer and its quantisation and export contracts, data format and
  splits, the first architecture (at least Gyatso's single-bucket 768×1024
  perspective net, CCRL 3258 at 40/15) and the cost ledger; frozen handoffs
  for F.1 and F.2's sub-steps.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Triumviratus trains on public Leela and Stockfish data, which this
    phase's rule excludes; its architecture, inference and training method
    transfer, its data recipe does not.
  - Cost ledger points measured there (TN-1, TN-3): L1 2048 −20% NPS, L1
    1536 −11.2%, four input-layer experts at L1 1024 −7.5%; a mobility
    input block −16.2% untrained; the network on large pages +6.7% at
    250 MB.
  - A refusal prior (TN-6): a policy network inside alpha-beta lost in four
    forms (−85 to −22.8); ordering already reached 79.2% first-move cutoffs
    without a hash move, and a static prior computed at the parent loses to
    histories that adapt to the subtree.
  - Triumviratus's seven networks, with what each cost and taught, are the
    review's §4.12 (TD-9 to TD-16); F.0 reads them before choosing the
    first architecture and recipe. Its next-network design is the model
    for every network step here: CPU-only gates first (speed of the shape
    with a random net, bucket-change rate in the tree, occupancy per
    bucket on the training data, engine against trainer), then small proxy
    trainings each judged by an SPRT, then one full run; close calls take
    the conservative choice (TD-16).
  - The state of Stockfish's networks as of October 2026, a free survey
    for F.7: PSQT removed (SFNNv17), int8 first-layer weights trained by
    clamping, king × opponent-queen buckets and 32 layer stacks (SFNNv18,
    about +4 at STC and LTC together, every single change failing alone),
    row pairing in the update (+5 to +7% NPS), learned routers for the
    layer stacks, pawn input blocks in test (TD-16).
- [ ] **F.1** `[I2]` Board events and accumulator scaffolding
  Board events and accumulator scaffolding, behaviour-neutral for the HCE:
  factual per-ply dirty-piece deltas, evaluator-owned per-thread stacks,
  refresh and unwind semantics, randomized differential tests, exact
  fingerprint, cost recorded (archived 10.1–10.2).
  Contract fixed 2026-11-14 (`analysis/b7_frontier_review_2026-11-14.md`
  §5, research amendment 1 to B.7.2's packet): make writes nothing for
  the network and `UndoInfo` gains no field. The delta of a ply is the
  triple the search holds at every make (the move, the piece that moves,
  the piece captured), stored at the make call site in the evaluator's
  stack. A null move stores an empty entry, and unmake does nothing. The
  triple is complete (at most four piece changes) and reversible, so a
  lazy update walks it back to the last computed ply, and a king-bucket
  refresh is taken at the current ply from the twelve piece bitboards.
  The board keeps those bitboards, the king squares and the occupancy
  directly readable and carries no evaluator state. F.1's cost target is
  zero on the pooled read.
- [ ] **F.2** Network contract, trainer and corpus
  Four sub-steps in the order of work (2026-11-11): the engine reads a
  network exactly, the trainer is proved and sizes the corpus, the corpus
  is generated, and the classical evaluation is refitted on it.
    - [ ] **F.2.3** `[I2]` Scalar integration, integer-exact conformance
      It was F.4 until 2026-11-11.
      Scalar integration: the network file contract and integer-exact
      trainer/engine conformance; clean HCE fallback (archived 11.2).
      It is proved on nets the trainer exports untrained, random and with
      perturbed parts, so it needs no trained net and comes before every
      training run.
      Prior: tag `archive/nnue-local` holds the 2026-07 scalar `.mnn` loader
      and its conformance test; F.2.3 decides whether any of it is reused.
      Embedding (code audit 2026-10-27): `#embed` puts the network file in
      the binary without a generated object or a resource step; it is
      C++26 but available as an extension in C++23 mode from GCC 15 and
      Clang 19, below the release matrix's floor (G.2's C++26 note).
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
      - Conformance gates before any training run (TN-9): an integer
        evaluator matching the trainer on exported nets; loader indices against
        python-chess; a random net whose per-bucket parts are perturbed, since
        zero deltas hide a wrong bucket index; engine against evaluator move by
        move across bucket changes; incremental against refresh at every node;
        a golden position set bit-identical after any rewrite.
    - [ ] **F.2.4** `[I2]` Trainer hardening and the learning curve
      It was F.3 until 2026-11-11; its baseline nets are F.6's.
      Deterministic pipeline, two seeds per configuration, validation selects,
      test opened once (archived 11.0.a, 11.1).
      It trains on a pilot slice generated to F.0's data contract
      (maintainer-run, hours), checks the recipe against a reference recipe
      there, and reads the learning curve (effective independent rows, label
      depth) that sizes F.2.5's corpus. Its runs are judged by holdout loss;
      nets are first judged in games in F.6, once F.2.3's conformance and
      F.5's speed stand.
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
      - Watch holdout loss at fixed λ=0 and λ=1, never the moving training
        loss: a λ cycle climbing back to 1.0 made Triumviratus's network imitate
        the labels, its game-result loss stopped where its Elo flattened, and a
        fixed λ restored the gain (TD-1).
      - Registered checkpoints with Elo targets; an early logarithmic fit
        mispredicted the plateau (TD-2). A restart cycle at a fraction of the
        peak rate moved a plateaued run; the final net averages the last epochs'
        weights (TD-3).
      - The loader, not the GPU, was the limit; files drawn uniformly over-read
        small files; sampling by piece count fed the endgame buckets; soft
        rather than hard early skipping (TD-4).
      - Its first network's failures, each visible in the command line (TD-11):
        no factoriser, a pure-evaluation target (λ 1.0) and a stop on a flat
        validation loss read −102 against the reference; with the factoriser,
        about 400 epochs and a λ squeeze, −39. A recipe is checked against a
        reference recipe before a GPU hour is paid.
      - Judging checkpoints (TD-12), applied in F.6: at 20+0.2 or longer
        (one net read −27 at 8+0.08 and −84 at 20+0.2) and against one
        fixed anchor
        (checkpoint-to-checkpoint gains summed to +71 where the direct gap
        closed by +28); the first epochs after a restart at a higher rate are
        recovery, not progress; a validation loss computed at a moving λ cannot
        rank checkpoints.
    - [ ] **F.2.5** `[V]` Data generation at scale
      It was F.2.1 until 2026-11-11.
      The corpus F.2.4's learning curve sizes: unique positions, by-game
      splits, manifests and hashes, maintainer-run (archived 11.0.b).
      Reference forms from Rarog's F.0 and its datagen step: node-limited
      self-play with soft and hard caps, opening
      books sampled by inverse use-count (`gyatso_read`, audit 2026-10-24).
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items), read
      for self-play data, since Basilisk's networks train only on its own
      games:
      - One label scale per run (TD-9): Triumviratus re-labelled every
        source with one evaluator before mixing, because two target
        distributions in one run train toward neither. Self-play from
        successive heads is re-scored by one search before it is mixed.
      - Label strength decides more than volume (TD-9): its network trained
        on Stockfish's 5,000-node self-play alone ended −84 against
        Stockfish's net, and a second stage on data with stronger labels
        closed half the gap. For self-play, stronger means deeper: the node
        budget per label is sized by the learning curve, not by throughput.
      - The stream's shape is measured, not assumed (TD-10): files drawn
        uniformly over-read small files; positions by piece count shaped
        toward endgames, which a material bucket otherwise starves; soft
        early skipping rather than hard; the positions the search actually
        evaluates compared with the training stream bucket by bucket (on
        Triumviratus the search held 2–3.5× more unbalanced material pairs
        than its data), with game positions at every ply as the source.
    - [ ] **F.2.6** `[V]` Classical refit on search-visited data
      It was F.2.2 until 2026-11-11.
      Added 2026-10-24 from Rarog's open plan (its F.2.1 then, 2026-10-22,
      `analysis/phase_c_review_2026-10-19.md` question 3). Rarog's same-search
      meter left 99 Elo that was not in the evaluation families, which read at
      static parity with the donor on played positions (RAR-E70), but in how
      the evaluation scores the positions the search visits. F.2.5's data is
      those positions with deep labels. The frozen classical evaluation's
      whole surface is refitted on it with C.2's protocol, screened statically
      on held-out rows and the frozen test, and gated as an evaluation change
      against the strongest classical release, with the same-search meter
      re-read at the close. A win is released as the classical line's next
      version; a loss closes the classical programme `NO_CHANGE`.
- [ ] **F.5** `[I2]` Incremental and SIMD inference
  Same-net parity on every move type, SIMD tiers with the scalar reference
  retained, cost attribution (archived 11.3).
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Measured there (TN-7): a refresh cache per bucket; a pawn-block
    refresh cache keyed by the pawn bitboards (+2.8%); the hybrid update on
    king moves (Stockfish `db98633b`, +3.4%); both perspectives in one
    dirty-list pass (+1.5%); first-layer neurons permuted at export so zero
    blocks are skipped (+2.5%, bit-identical); no feature opened and closed
    inside one capture; the System V convention for inner functions on
    Win64; the network on large pages.
  - Memory is the cost: 47% of last-level misses in the incremental update
    (TN-8). A split accumulator cost +12% cycles; 8-bit weights quantised
    after training were slower than the bandwidth they saved (TN-5).
- [ ] **F.6** `[V]` First nets, then the search re-fit for the network
  The baseline nets first (F.3's until 2026-11-11): two seeds per
  configuration on F.2.5's corpus, validation selects, the frozen test
  reports once, and each net is judged at `20+0.2` or longer against one
  fixed anchor in a sibling-free slot (TD-12, in F.2.4's list). Then the
  search: score scale, correction, margins, qsearch and SEE thresholds,
  using C.10's protocol (archived 11.4.a–b); BAS-R02, R03, S08 and S09's retry triggers are
  reviewed here. The classical evaluator keeps its own overlay of the
  scale-sensitive constants (Phase F rules).
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - The eval scale per piece-count bucket recalibrated for each net, but by
    games: matching the raw ratio to the previous net lost; the
    scale-dependent margins retuned on the final net; blend constants
    inherited from another net tuned on this one (TD-8).
  - Network gains shrink at longer controls and hide behind shared cores:
    one net read +0.8 at 75 concurrent games and +23.4 at 38 (TD-7).
  - The printed score's scale is fitted here, per network (G.2's until
    2026-11-11; TA-2; Rarog's F.6), so that the first NNUE release
    prints +1.00 as an even chance to win.
- [ ] **F.7** `[R3]` Architecture ladder, one axis at a time
  Architecture ladder: output buckets, king buckets with mirroring, then
  relation and threat inputs, one axis at a time with multiple seeds; each net
  gated against the previous (archived 12.0.b).
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Axes Triumviratus measured (TN-1 to TN-3): input-layer experts by total
    material, a shared base plus zero-started deltas (about +15 with its
    retune); per-side experts by the opponent's non-pawn material with int8
    king-relative weights and a cached pawn-king subnetwork (+10.3 ± 4.3);
    a passed-pawn block (+7); a pawn-pair block (+18 with own self-play
    data); threat inputs. Bucket keys can be chosen by measurement: its
    network's own regimes followed material × queens (TE-2).
  - The zero-initialised graft as a cheap screen for a new block, and its
    limit: frozen later layers may not use the new information (TN-4).
  - Negative priors (TN-5): averaging two buckets near an edge, a split
    accumulator, fixing the root's bucket for the whole search.
  - The order it climbed and what each rung bought (TD-13): threat inputs
    over plain king-relative inputs (+50 for the engine), a pawn-pair block
    grafted (+18), a passed-pawn block (+7), the whole net trained from
    scratch on one label scale (+23), then saturation: more epochs, an
    annealing tail and fine-tunes all read zero, and only capacity paid
    (material buckets, +15). Capacity is priced in speed first (L1 2048
    −20% NPS).
- [ ] **F.8** `[V]` Data frontier
  On-policy refresh with the strongest net, deduplication, hard-position
  mining; repeat while a cycle accepts (archived 12.0.a, 12.0.c).
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - More epochs on data of the same family moved a converged net by 1% in
    scale and 0 Elo; per-bucket fine-tunes +0.7 (TD-5). Hard examples
    (positions lost, or where a deep search contradicts the static score)
    need a dedicated generation at scale (TD-6).
  - A refresh adds new information, not more of the old: Triumviratus's
    one refine with its own self-play (6.5% of the mix, repeated ten times)
    rode with a new input block and new data, and its gain cannot be split
    between them (TD-14). Each refresh is gated against the previous net on
    the fixed anchor (TD-12).
- [ ] **F.9** `[M]` NNUE release
  Beat the classical release at STC, LTC and 4T; platform matrix; the
  classical arm's read against the strongest classical release (Phase F
  rules); version
  **2.0.0** (archived 11.4.c, 11.5; maintainer decision 2026-10-24);
  B.11.3's scans repeated on the candidate, with the trainer and the
  network's provenance named (PLAN §5). Open
  decision for this leaf: whether releases from 2.0.0 on use integer
  versions, as Stockfish does (Stockfish 17, 18), instead of X.Y.Z.
- [ ] **F.10** `[V]` CCRL top-100 gate
  Submit; the list decides; the shortfall is fed back into F.7 and F.8.

## Phase G — Scaling, platforms and the top 50

- [ ] **G.1** `[R2]` High-thread and NUMA
  8/16/32T scaling measured as time-to-depth and strength, TT and network
  placement, large pages, affinity (archived 13.0).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Large pages moved to B.8.4 (D-8).
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Threads spread over NUMA nodes, one network copy and one set of shared
    histories per node, each thread's handle on its node (TP-6).
- [ ] **G.2** `[I1]` Platform and product
  Compilers and ISAs on native hardware with behaviourally identical portable
  fallbacks; the optional universal CPU-dispatched binary, designed first and
  adopted only if it pays (archived 9.11, 13.1).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - The ISA tier ladder measured, not chained (D-24); the ARM contract goes
    with A.10.4; the universal-binary design notes and traps (D-72); a
    slow-PEXT advisory (D-25).
  The universal binary was built in Phase A at the maintainer's choice
  (A.15.2 to A.15.5, 2026-10-28; `analysis/universal_binary_2026-10.md`
  and its amendments) and shipped by A.15.6, its tiers aligned to 64
  bytes after BAS-P20 read the unaligned relink 0.58% slower (BAS-P21
  passed). G.2 extends its tier list when Phase F's SIMD tiers arrive; the
  form applies unchanged. The candidates are Stockfish's: `avxvnni`,
  `avx512`, `vnni512` and `avx512icl` beside the four Basilisk has
  (A.15.7), each tried with the network and kept on a registered read.
  The development host (a 5950X) has none of them, so the reads wait for
  hardware that does; SDE covers only selection and the fingerprint.
  C++26 (checked 2026-10-27, `analysis/code_audit_2026-10-27.md` §6):
  the standard was finished at the March 2026 meeting. The release
  compilers are Clang 19 with libstdc++ on `ubuntu-24.04` (Linux; CI
  runs Clang 22), the pinned MSYS2 Clang 23 (Windows) and the macOS
  runner's AppleClang with libc++; GCC 15 runs only the PGO smoke job
  and MSVC is not in the matrix, so the floor is Clang 19 and
  AppleClang. On that floor `-std=c++26` buys `#embed`, pack
  indexing and `= delete("reason")`, nothing the engine needs; the
  features that would remove code — reflection for the hand-kept
  option, parameter and counter tables, erroneous behaviour for
  uninitialised reads, contracts — are in GCC 16 alone
  (`-freflection`). Decision: stay on C++23. Re-decide here when the
  macOS and Windows clangs ship reflection, with those tables as the
  first use; until then nothing is gained by changing the standard.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Chess960 (TB-9): withdrawn 2026-11-14 (maintainer). The engine is
    built for standard chess. A later decision for Chess960 would rework
    the board then (X-FEN, castling written as the king taking its rook,
    the FRC perft suite).
  - An analysis build (TA-1): the second root move always searched with a
    full window, paused in quiet positions; the more direct of two clear
    wins played; the analysis search compiled as a second copy so games pay
    nothing.
  - The printed scale calibrated so +1.00 is a 50% chance to win is F.6's
    since 2026-11-11, fitted per network; tablebase wins printed as
    `cp 20000` minus the distance stay here (TA-2). Strength levels by
    search limits and line choice, calibrated in games (TA-3).
- [ ] **G.3** `[R3]` Frontier
  Larger nets, data scaling, search fit at LTC, rejected classical mechanisms
  reopened only where the NNUE fires their retry trigger (archived 12.1); the
  CCRL top-50 gate.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - The long-control tree shape (TS-19): Stockfish's reductions run deeper
    in endgames and its TT bounds fit more often; no single transplant
    shortened Triumviratus's tree, and its short-control tunes never saw
    depth beyond 15. Depth-dependent levers need a fit at a long control.

## Phase H — Refinements after the big gains

**Rule** (maintainer decision 2026-11-03): a mechanism whose measured gain
elsewhere is about 5 Elo or less waits here, after the NNUE switch and the
programmes that carry the large gains, so that small gains are not chased
while the engine is hundreds of Elo behind its targets. Items are
candidates with their source and evidence, not commitments. A programme
leaf may take one earlier only when the item is part of a defect it is
already repairing or a form it must choose anyway, and says so.

- [ ] **H.1** `[R2]` Refinement backlog triage
  Opens when F.10 and G.3 have closed, or earlier by maintainer decision
  once two consecutive programme steps have each read under +5 Elo.
  Re-ranks the backlog of H.2 and H.3 against the NNUE head: which items its
  evaluation or search already covers, which need a network with buckets,
  what each would cost to screen, and the screening order, registered
  before any game. Sources: the items the Triumviratus review marks small
  (`analysis/triumviratus_review_2026-11-03.md`) and any later donor read.
- [ ] **H.2** `[I2]` Search refinements, one at a time
  Each item alone against the head on the same binary, SPRT `[0,3]` run to
  a decision with no early stop on a lean (Triumviratus's stopped leans
  added up to nothing, TM-4); levers that tune together enter one SPSA
  rather than many SPRTs.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - Causal reduction: quiet siblings that touch none of a sibling's
    refutation squares reduced half a ply more (+4.27 ± 4.13) (TS-3).
  - TT cutoff value damped toward beta (+2.2 ± 3.2 over 11,774) (TS-4);
    a softmax-gradient history update (+0.9 ± 3.4) (TS-5); a
    discovered-check ordering bonus (TS-6); the late-move count scaled by
    the continuous two-ply eval change (TS-8); no null move with a minor
    piece or less (+1.3 ± 2.3 in endgames) (TS-9).
  - A low-depth hash-move extension with its per-line cap (+4.5 at 10 s,
    −9.4 at 40 s together with time rules) (TS-2); passed-pawn pushes in
    endgames reduced less and never pruned (+4.4 ± 6.8) (TS-10); a
    continuation correction six plies back with all correction weights
    rebalanced (TS-11); a draw value in defence (TS-18).
  - Chess-knowledge ordering terms (outposts, safe attacks, king shield,
    passed pushes, promotion, bishop-pair capture): worth nothing alone,
    baked through an SPSA, then removing them gained +4.21 ± 2.79 on the
    next network (TS-7). Read only as SPSA coordinates beside the
    histories they compete with.
  - Corrections and margins per network bucket (`CorrPhase` +3.2 ± 3.8,
    per-bucket pruning offsets +1.6 ± 4.3, piece values per bucket
    unbaked) (TS-12): only if F.7's network has buckets the search can key
    on.
  B.3.4 input (maintainer decision 2026-11-04):
  - Basilisk's kept forms against the donor's, each arm fitted: today
    `CoreHistoryFills` 0, `B3ProbCutTt` 0 and `B3ExtBudget` 1 (the
    fail-low store went to B.4). B.3.2 read the first two at −0.5 and
    +0.9 with the donor's form unfitted.
    - A paired run on one fitted arm is tilted toward whichever form the
      fit was made with, so the comparison refits each arm: the donor's
      forms switched on and refitted, against the current fitted arm,
      then one SPRT.
    - Until then Basilisk's forms stay, since they work.
  B.6.6 input (2026-11-05): a departures row that reads under +5 there is
  not cleared by a run on one fitted arm; it waits for the fair re-read
  with both arms refitted, which is H.6's since 2026-11-13, as are the
  B.3.4 forms above.
  B.4.3 input (2026-11-06, `analysis/b43_cluster3_research_2026-11-06.md`,
  research amendment 1 §4):
  - A queen capture exempt from quiescence's gain rules. On the B.4.2
    head, of the queen captures that are not recaptures, 45.9% of those
    futility prunes and 27.3% of those the alpha-relative SEE rule prunes
    beat alpha when searched. The population is 26,810 decisions in
    31.2M nodes, about a twentieth of what the recapture exemption
    recovers, which is why it waits here.
    - The cause is unmeasured. The leading hypothesis is the evaluation's
      king-safety swing when a queen leaves the board; an even queen
      trade has an SEE of 0 on any value vector.
    - First reading: B.4.5's pricing by class on Cluster 3's arm, which
      exempts the recaptures. The item is worth a game only if the
      remaining queen captures still beat either rule more than 10% of
      the time there.
    - Cheapest test of the cause, before any game: the same pricing
      split by whether the capturing side's king is under attack.
    - The figure belongs to this evaluation: re-read after C.3 rebuilds
      king safety, and again on the network.
  B.6.4 inputs (2026-11-12, `analysis/b64_engine_audit_2026-11-12.md`
  §5; each is the donor's behaviour, copied faithfully, with its
  counter on the four-cluster head):
  - The frame a re-entry rewrites. An exclusion search's own fail high
    counts in `cutoff_cnt` as a child's and moves 1.3 to 3.1% of all
    reduction decisions; 22 to 25% of exclusion searches leave the
    grandchildren's count above 1 for the table move's child; the
    leaked `move_count` misdates about 1% of fail-low bonuses
    (Triumviratus read the move count alone at about ±0.5). One switch
    that saves and restores the three fields around the exclusion
    search and the verification, each arm refitted on the three
    cutoff-count coordinates.
  - IIR at a revisited fail-low node: 13 to 18% of IIR's reductions
    fall on a searched fail-low entry that has no move because the
    store keeps none. A switch exempting those; Rarog's −14.7 ± 13.8
    is the prior. First reading: B.16.2's pricing with a cell for it,
    which says whether the item deserves a game.
  - The null verification that is a quiescence search: 19 to 38% of
    verifications, because the reduction's eval term has no cap. A
    floor on the verification's depth.
  - The capture stage's under-promotions (F5): at most 0.19% of nodes
    in quiescence for a handful of fail highs; the donor's class is
    the re-read.
- [ ] **H.3** `[I2]` Time-management refinements at their own control
  Each measured at the time control it acts at, never only at `3+0.03`,
  and tuned against an external engine at release speed (TT-3); every
  rule here was about zero at short controls on Triumviratus.
  Triumviratus review inputs (2026-11-03,
  `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
  - The surprise rule: ×1.2 time when the opponent does not play the
    predicted reply, silent while behind on the clock (+6.23 ± 6.75 at
    40+0.4, +0.7 ± 7.1 at 16+0.16) (TT-7).
  - Less time while behind on the clock (+5.23 ± 4.72 at 6+0.06, +0.36 ±
    5.13 at 30+0.3) (TT-8).
  - More time in a defence that is not lost, less with 24 or more pieces
    (+7.27 ± 7.08 at 40+0.4, the two never separated) (TT-9).
  - A guard against saving time on an unconfirmed score jump, a graded
    surprise, more time when the PV trades into another bucket, a time
    share by phase (TT-10).
- [ ] **H.4** `[R2]` Throughput at an identical tree on the network head
  Added 2026-11-11 (Rarog's H.4; `analysis/rarog_import_2026-11-11.md`).
  B.8.5 and C.12 profile the classical engine, and F.5 covers the
  network's inference. On the network head's profile, the leads that did
  not pay there, or were never read, are read again where the network
  changed what is hot: the board, SEE, picker, prefetch and layout items
  of B.8.5's list (TB-4 to TB-8, TB-12, TB-14; TB-16 holds the
  negatives). The profile is the first reading, and an item under 0.5%
  of samples is not built. Each change is behaviour-neutral: the exact
  fingerprint and the pooled read.
  Added 2026-11-14 (`analysis/b7_frontier_review_2026-11-14.md` §3, §5):
  the board forms that removed slider lookups and gained nothing on the
  classical head are read again here, where the network's weights and
  accumulators take the cache the slider tables sit in: `gives_check`
  and make's checkers without lookups, SEE attackers from aligned
  sliders, the generator's pins from aligned sliders, BAS-P26's
  compressed tables. Their kernels are in the review's patch. They are
  read first on B.7.5's replay and engine screen.
- [ ] **H.5** `[I2]` Harness rewritten in Python for a Linux host
  Added 2026-11-12 at the maintainer's request, so that development can
  move to Linux, or run on a second Linux host, when the maintainer
  decides; the move itself is not scheduled. PowerShell 7 runs on Linux,
  but the harness is bound to Windows in places (survey 2026-11-12): the
  idle-host refusal is skipped off Windows (`Get-HarnessHostBusyPercent`
  returns nothing there), `host_sampler.ps1` reads `typeperf`,
  `nps_read.ps1` reads Windows counters and priority classes, the
  Colosseum and fastchess pins name only Windows assets, and the live
  scripts default to backslash paths, `.exe` names and the `D:/chess`
  tablebase path. The engine already builds and passes CI on Linux.
  - Python, already most of `tools/`, with `psutil` for affinity, host
    load and priority on both systems. One script per change, not a
    wholesale rewrite: the shared guards (`harness_common.ps1`) first,
    then `colosseum.ps1`, `build_test.ps1`, the NPS tools,
    `setup_tools.ps1`, `datagen.ps1`, `gauntlet.ps1` and
    `spsa_colosseum.ps1`. The historical `run_*.ps1` scripts are not
    ported.
  - A port replaces its script only after parity: on fixed synthetic
    inputs covering the pass and fail case of every guard, old and new
    accept and refuse the same runs, and a dry-run preflight agrees on
    Windows and Linux. The `.ps1` is deleted in the commit that switches
    its callers.
  - Linux acceptance: a Linux build reproduces the bench fingerprint, the
    byte-hash fixtures pass in a Linux checkout, and Colosseum's Linux
    release is pinned beside the Windows one with its own SHA-256.
  - Ports are tested in WSL2 for function: parity, dry runs, process
    handling. WSL2 is not a measurement host, because its virtual CPUs
    float over the host's cores, so placement checks and speed reads
    need native Linux.
  - Windows-only opponents (Rybka and the other old gauntlet `.exe`
    engines) under Wine are Colosseum's to support, not Basilisk's.
    H.5 only reads those opponents' fault rate and time losses at
    `3+0.03` before a Linux gauntlet is trusted.
  - Speed reads and NPS baselines become specific to host and OS: both
    arms of a read or a gate run on one host under one OS, and the build
    manifest records the OS.
- [ ] **H.6** `[R2]` Donor-form re-reads, both arms refitted
  Added 2026-11-13 at the maintainer's request: every place the search
  still keeps a form that is not the donor's is checked against `sf_19`
  and tested, so that no departure stays a choice nobody measured
  fairly. It runs last in the phase, on the search H.2's refinements
  leave. The rows earlier leaves sent to "H.2 for the fair re-read" are
  this leaf's; H.2 keeps the new mechanisms.
  - **The check.** One census of the head against the donor, read afresh
    and not only from the departures tables: each place where the forms
    differ, Basilisk's form, the donor's, the prerequisite and whether it
    has landed, and the population counter on `bench 13` and the gate
    games. The tables and readings so far are the input:
    `analysis/b66_reconciliation_2026-11-12.md` §3 and §8 (B.6.6's 39
    rows), BAS-S59 and BAS-S60, and every later packet's departures table.
    Rows the network or time management owns go to F and H.3.
  - **The testing.** For each row whose population can carry a game: a
    switch for the donor's form, then both arms refitted on the
    coordinates the row moves, the same SPSA budget each, then one SPRT
    `[0,3]` of the refitted donor arm against the refitted head (the
    maintainer's 2026-11-04 rule: a run on one fitted arm can confirm the
    donor's form, never clear it). Rows that share coordinates are
    refitted and read together. H1 adopts the donor's form; anything else
    keeps Basilisk's, recorded with the reading as a measured choice.
  - Known rows today (B.6.6, BAS-S59): the TT-cutoff bonus whole, the
    donor's slope 112 with its cap 695 (BAS-S59 read the cap alone on the
    fitted slope 151: +8.0 ± 9.1 at `3+0.03`, −0.5 ± 8.6 at `10+0.1`); the
    rule-50 key (−4.5 ± 8.4); `CoreHistoryFills`, `B3ProbCutTt`,
    `B3ExtBudget`; the stand-pat store; the evasion order; the SEE vector;
    the capture stage's promotions (F5); razoring and reverse futility in
    the exclusion search and its estimate (R2a, R2b); the verification's
    frame (R7, with H.2's frame item); razoring in the donor's form and
    the kings in the non-pawn key (+10.6 ± 9.4 and +11.6 ± 9.3 alone,
    +2.2 ± 4.2 together over 10,000 games, BAS-S60). Their switches
    and `B6_DONOR_FORMS` are in the source, off.

## 4. Measurement protocols

| Meter | Protocol | Owner |
|---|---|---|
| Pool score | Colosseum, fixed pool with Houdini 3, `3+0.03`, UHO, no adjudication, 400 games per pair, 1T and 4T | A.7, B.11.1, C.11, E.2 |
| Search deficit G(0) | Paired, head against the frozen classical-Stockfish search oracle rebuilt with the head's HCE, 3,000 games, no adjudication; at equal time (BAS-O05) and, from B.11.1, also with the bridge's price returned as time odds (RAR-O08) | A.7.3, B.11.1 |
| Evaluation deficit | Hybrid at the head against the classical-Stockfish-HCE hybrid, same search | C.11 |
| Cluster acceptance | Registered final-PGO SPRT, bracket per rule 5, `[0,10]` for B.2 | every cluster |
| Neutral change | Exact fingerprint, release and sanitizer CTest, pooled-PGO NPS | B.1, B.8, C.1, C.12, F.1 |
| Conversion | A.5.5's audit on the latest pool tournament | every checkpoint |
| Fixed-node shape | Oracle differential at stride 1, depth at 300k nodes, branching profile, WAC at fixed nodes, positional screen | B clusters |
| Endgame layers | Truth cohort, drawn overclaim, conversion at the deployment bracket, floors | C.7 |

Size every SPRT at the expected value before registration. Until Basilisk's
own gates calibrate a drift model, Rarog's RAR-M10 fit is the prior: drift
per game is about 8.3e-6 × (Elo1 − Elo0) × (true nElo − midpoint) at this
host's `3+0.03` conditions (BAS-X32). Bracket, cap, book, clock and
adjudication never change after games are seen.

## 5. Release rules

- A release ships only from a head whose every accepted cluster has a ledger
  row and whose deficit meters were recorded at the checkpoint before it.
- Releases follow each programme (maintainer decision 2026-10-24,
  replacing 2026-09-28's single release at E.3): **1.11.0** after B (B.11.4),
  **1.12.0** after C (C.13), **1.13.0** after E (E.3), and **2.0.0**, the
  first NNUE release (F.9). Between them nothing is released unless a
  correctness repair forces a patch.
- A classical release requires an accepted SPRT over the previous release at
  `3+0.03` 1T, `10+0.1` and 4T direction checks whose 95% intervals exclude
  a loss, and a 4T pool read beside them: a head-to-head read cannot see a
  thread-scaling loss both versions share (RAR-M76).
- An NNUE release requires a win over the last classical release at STC, LTC
  and 4T and a clean platform matrix. Whether versions from 2.0.0 on are
  integers, as Stockfish's are, is decided at F.9.
- `master` is the trunk: every coherent piece of work lands from its own
  short-lived branch as one squash commit through a PR, and the branch's
  commits stay cited under the PR (maintainer decision 2026-10-09, adopting
  Rarog's process; it replaces the long-lived `dev` branch and its
  `archive/dev-X.Y.Z` tags of 2026-10-08). PROCESS *Delivery* and *Release*
  hold the procedure.
- Every release is preceded by the originality and licence disclosure on
  its own candidate (maintainer decision 2026-10-23 for Rarog, adopted
  here 2026-11-11): B.11.3 builds the scans and writes the first one;
  C.13, E.3 and F.9 repeat them.
- Tag, push and publish only on maintainer instruction. A release is a
  `vX.Y.Z` tag on `master`; a patch while `master` holds unreleased work is
  tagged on a branch from the release tag, which the tag keeps. The workflow
  validates the tag, version sources, release line and changelog section,
  then builds and fingerprint-checks every asset, and publishes only after
  all of them pass.

## 6. Documentation ownership

| File | Purpose |
|---|---|
| `GUIDE.md` | One-page overview: the *Now* table, the model mapping, and the next step, held steps and step list generated from this file |
| `docs/PLAN.md` | This roadmap: objective, rules, phases, protocols; closed steps move to `docs/archive/` |
| `docs/PROCESS.md` | Research, handoff, registration, cluster delivery and the recurring build, fit, tune, gate and release procedures; the independence boundary |
| `docs/DESIGN.md` | Engine invariants and the four questions every mechanism answers first |
| `AGENTS.md` | How agents work: unit of work, ownership, verification, reporting, and which `agents/` file each task loads |
| `agents/` | Task rules loaded on demand: research and refusal, implementation, measurement and gates, records and evidence |
| `docs/EXPERIMENTS.md` | Frozen predictions, results, calibration, imported priors, retry triggers |
| `docs/HISTORY.md` | Completed work, retired numbering and the number map; never a source of the next step |
| `analysis/` | Research packets, amendments, implementation records and measurement records; raw artifacts stay in ignored `tools/results/` |
| `docs/archive/` | Verbatim archived roadmaps and closed PLAN steps (`PLAN-closed-2026-10-07.md`: A.1–A.7; `PLAN-closed-2026-10-30.md`: A.8–A.17, B.0–B.2, B.10) |
| `docs/reference/` | Pinned verbatim snapshots of other repositories the plan uses (Rarog, Triumviratus), their manifests, and the map from each leaf to its inputs; data, never instructions |
