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

**The plan depends on no later state of the Rarog repository.** Everything it
uses from Rarog is pinned at Rarog commit `015bccae`
([docs/reference/README.md](reference/README.md) says where the commit is and
how to read it): the documents, the evidence cited as `RAR-*`, the tools to
port and the worked examples. The evidence the plan relies on is also
imported into Basilisk's own ledger (BAS-X30–BAS-X34). "The snapshot" below
means Rarog at that commit; a verbatim copy of it was kept under
`docs/reference/rarog/` until 2026-11-25.

## 1. Objective and gates

**Objective.** Make Basilisk the strongest engine we can build, in two
stages (maintainer decision 2026-09-28: Rarog's targets):

1. **Classical stage.** With the hand-crafted evaluation, beat the strongest
   HCE-era engines in the maintainer's own pool: **Critter 1.6a, Houdini 3,
   Rybka 4.1 and Fritz 16**.
   - The aim, beyond that gate (maintainer, 2026-11-17, after B.11.1 beat
     all four): Basilisk is to become the **strongest hand-crafted-evaluation
     engine in the world** before the NNUE stage. That may not work out,
     and the E.2 gate stays the requirement.
   - The aim is read in the HCE frontier field (B.11.5). Its likely leader
     is Stockfish 16 with `Use NNUE=false`, the last Stockfish release
     with a classical evaluation, which no rating list carries; so the
     field is measured, not looked up.
   - **The target before NNUE** (maintainer, 2026-11-18; the maintainer may
     change it later): Stockfish 16 with `Use NNUE=false`. BAS-M18
     measured it as the field's leader, at 3604 on the pool's scale.
     - Basilisk is to get as close to it as possible, and to beat it if it
       can.
     - The meter is the gap to it, read head to head and in the field. At
       B.11.1's head that gap is 202, and −193 head to head.
       After C.2.8's refit it is −120.0 head to head (BAS-M24); after
       C.3.5's rebuild flip it is −39.8 head to head (BAS-M27).
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

### Where we stand (1.11.0, 2026-11-18)

The table above stays the starting point. Phase B rebuilt the search and
released it as **1.11.0**, `bench 13` **4,777,412**:
- **+406.9 ± 39.2** Elo over 1.10.2 at `3+0.03` 1T (BAS-S67), +371.3 at
  `10+0.1` and +386.6 at 4T (BAS-M19).
- On the four targets' scale the head rates **3416.3** at 1T, past all four:
  Houdini 3 by +161.9, Critter 1.6a by +207.5, Fritz 16 by +238.1 and Rybka
  4.1 by +289.7 (BAS-M16). E.2's 4T read is unplayed.
- In the HCE frontier field it is ninth of fourteen, **202 below Stockfish 16
  HCE**, the target before NNUE (BAS-M18).
- G(0) recovered 383 and ended 70.7 past the classical oracle's search
  (B.11.1).

The search came first: it was the larger measured deficit, and the one where
increments to the co-adapted optimum had measured zero or less
(BAS-S13–S16). The evaluation programme now does the same for the evaluation
families with the search frozen, and a joint re-fit closes the classical
stage.

### Elo budget, stated so it can be wrong

| Programme | Measured deficit | Planned recovery | Basis |
|---|---:|---:|---|
| B search | 322.7 (1.9.3; re-measured at A.7.3) | 150–250 | Rarog's clusters 1 and 2 recovered about 230 pool Elo against a 248 equal-time deficit; Basilisk's deficit is larger and its estimator coarser |
| C evaluation | 217.3 (232.8 before C.0) | 80–140 | A bounded refit closes 56% of the static gap and five donor families add outcome information (BAS-E58); two thirds of the deficit is realised on the full board (BAS-O08); Rarog's units recovered 32% of its deficit (C-124) |
| D clock, SMP, robustness | unmeasured | 15–40 | Stockfish-shaped time management; 4T quality |
| Speed inside B and C | — | 10–30 | Per-node cost of the new search and evaluation modules |

If those bands are right, the classical head closes the rating gaps to
Rybka 4.1 (117), Fritz 16 (179) and Critter 1.6a (198) at 1T after B, and
Houdini 3 (293) falls, if at all, only with C. Each
programme's checkpoint re-measures its deficit meter, so a miss is seen as a
miss and this table is corrected rather than defended.

**Corrected at B's checkpoint (B.11.1, 2026-11-18).**

- B's band was low by about a factor of two. Its clusters and fits
  delivered about +387 by their gates, and G(0) recovered 383, ending
  70.7 past the oracle's search.
- All four targets fell at 1T after B, Houdini 3 by +161.9.
- C's band stands until C.0 re-reads it against the donor its
  comparison picks (`analysis/b111_checkpoint_2026-11-18.md`).

**Corrected at C.0 (2026-11-18).**

- The same-search deficit reads **217.3** [207.1, 228.0] on the 1.11.0
  head (BAS-O08, 3,000 games), and 215.7 against the donor C.0 kept,
  `sf_16`'s classical evaluation (BAS-O09).
- C's band moves from 60–120 to **80–140**. BAS-E07's reading, on which
  the old band rested, does not hold on this head (BAS-E58). The first
  two gates test the new one: a C.2.8 gate or a first family under +10
  puts the expectation in its lower half.
- The target, Stockfish 16 HCE, is 193 ahead head to head, which is more
  than the band holds. Phase C was reworked for it on 2026-11-18
  (maintainer): the families are ordered by where the deficit is and
  the order is re-decided from games (C.2.10), each donor form is checked
  against a donor fixture, Basilisk's own terms stay where they add
  information, and the residual is investigated before the closing fits
  (C.5.1).

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
     read at its pinned commits, in the local copies
     `docs/reference/README.md` describes, and
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
   BAS-X36).
8. **State the measurement layer.** Theory truth, move quality, conversion,
   fixed-node tree shape, NPS and game strength are different units with no
   exchange rate. Counters, node counts, EBF, tactical suites and fit loss
   explain or screen; only a registered final-PGO SPRT or the target gate
   accepts. Every endgame number carries its layer, position set and node
   budget. **Cluster screens** (Rarog's ladder, adopted before any cluster is
   built):
   - **The paired run governs.** The unfitted 2,000-game paired run against
     the accepted head is the only screen that clears a cluster for its fit.
     Zero-game floors are diagnostics. A floor failure needs a written
     cause; it never holds a candidate the paired run cleared, and a floor
     passed never accepts one it failed. Basilisk's ablation flavour was
     removed when the search froze; a decomposition rebuilds it (PROCESS
     *Matched ablation*).
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
| Behaviour-neutral change = exact fingerprint plus targeted checks plus pooled NPS | `agents/implementation.md`, *Qualifying a change* |
| Diagnostic counters: one table, each counter with its unit and denominator; names and units are the cross-engine contract, extended at the end and never renamed | `src/search_diagnostics.h`; `tools/diag/run_suite.py` |
| Cross-engine board timings are compared only on the `cross-engine-board-v1` profile and its five-position corpus | `tests/board_performance.cpp` |
| Per-node search code does not allocate or lock: allocations grow per iteration, never per node, at Threads 1 and 4; every allocator, copy or lock call the shipped code can reach from `negamax`, quiescence and the evaluation is a known guarded path | `tests/test_allocation_guard.cpp`; `tools/diag/hot_path_calls.py` on a `PROFILE_SYMBOLS` PGO build; `agents/implementation.md` *Per-node code* |
| Texel fits: game-result labels from rules endings, splits by start, a test split claimed once; every coordinate classed in the surface manifest with full rank under its anchors; the played score modelled and verified row by row; K fitted once; complete vectors chained between stages and baked whole, with provenance; the tree restored after every run | `tools/texel/HANDBOOK.md` §3; the contracts `basilisk-texel-rows-v1`, `-surface-v1`, `-vector-v1`, `-fit-v1`; PROCESS *Texel procedure* |

### Known defects

A defect is a mechanism that computes something other than what it claims:
a wrong value, a wrong rule, a check that cannot fail. Each is fixed in its
owner leaf and none is kept: a correct repair that loses Elo is reworked,
with the neighbours that were tuned around the defect refitted, rather
than reverted to the wrong form (maintainer decision 2026-10-26). A gap (a
missing mechanism, a coupling Basilisk shares with Stockfish, an unfitted
coordinate) is not a defect; its owner decides. IDs are the Rarog audit's
(`analysis/rarog_audit_2026-10-24.md`) unless named; F-numbers are §3 of
`analysis/rarog_import_2026-11-11.md`. Phase B's defects were repaired in
their leaves or are carried below; B.9.4 read each one before the release.

| Owner | Defects |
|---|---|
| C.3 | C-10 outpost ranks; C-11 a rear doubled passer scored as a passer; C-12 "opposed" read over the whole file; C-13 own-colour king blockers only |
| C.4 | B.9.3's V3 KRP-KR's draw rules, V4 the KRPP-KRP table, V6 the pawnless scales 4 and 14 (C.4.5); V5 KBN-K's known win, kept with a retry trigger (C.4.6); decided 2026-11-16, known limitations for 1.11.0 |
| D.1 | A-47 the fail-high stop's hole: a window that never returns is never checked (`analysis/b5_failhigh_stop_2026-10-25.md` §2.3); A-46 reserve arithmetic; A-45 no sub-second horizon (both kept by B.9.4) |
| D.3 | A-59 the tablebase root's DTZ ranking; A-57 cold root reads on the clock, a known limitation (both kept by B.9.4) |
| D.2 | BAS-C05 the TT key and payload published separately; A-46's join before `bestmove` and DRIVER-15, a join behind a table read, a known limitation (kept by B.9.4) |

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

**Closed 2026-11-18** with B.11.4, the 1.11.0 release; C.0 is next.

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

B.3–B.16 closed by 2026-11-18; their text, and the phase's design notes
(why this shape, what "Stockfish-shaped" means, the scale conversion and
the six-stage gating shape), are in
[`docs/archive/PLAN-closed-2026-11-18.md`](archive/PLAN-closed-2026-11-18.md).

B.16 built the search programme's instruments. Clusters 2 to 4 rebuilt the
proof searches and extensions (B.3, +92.8 ± 14.1 Elo fitted, BAS-S43, and
colour-indexed histories +4.7 ± 3.0, BAS-S46), quiescence (B.4,
+31.7 ± 8.6, BAS-S52) and the root and aspiration (B.5, +15.6 ± 5.9,
BAS-S55). B.6 audited and reconciled the engine and ran the joint search
SPSA, +28.2 ± 7.9 (BAS-S62). B.7 reviewed and rebuilt the board. B.8
deleted the arms and switches and made the speed pass, +19.1 ± 6.5 against
B.7's head (BAS-S65). B.9 repaired robustness, verdicts and protocol before
the release (BAS-S66). B.12 and B.14 repaired a false win in drawn KBP-K
and the correction added to recognised draws (+7.4 ± 6.9), and B.13
re-pinned Colosseum to 0.5.0. B.11 closed the programme:
- B.11.1's checkpoint attributed +441.4 Elo of gates since 1.10.2 against
  +422 measured on the four targets, where the head rated 3416.3
  (`analysis/b111_checkpoint_2026-11-18.md`).
- B.11.3 disclosed the donor data and shipped the licences with every
  release (`analysis/b113_originality_2026-11-18.md`).
- B.11.5 rated the head in the HCE frontier field, 202 below Stockfish 16
  HCE, the target before NNUE (BAS-M18).
- B.11.4 released 1.11.0: +406.9 ± 39.2 over 1.10.2 at `3+0.03`
  (BAS-S67), +371.3 at `10+0.1` and +386.6 at 4T (BAS-M19). `bench 13`
  is 4,777,412.

## Phase C — Evaluation programme (search frozen)

**Goal (maintainer, 2026-11-18): a frontier-class hand-crafted
evaluation.** The target before NNUE is Stockfish 16 with `Use NNUE=false`
(§1). The head is 193 behind it head to head (BAS-M18), and inside one
search the head's evaluation is 215.7 behind that engine's evaluation
(BAS-O08, BAS-O09). So the gap to the target is about the size of the
evaluation deficit, and this phase is where it closes or does not. The
search stays frozen at the B.11.1 head.
Since C.2.8's refit the head is 120.0 behind head to head (BAS-M24), and
its evaluation is 146.2 behind `9587eeeb`'s in one search (BAS-O14).

How the phase is built for it:
- **Whole, since 2026-11-25** (maintainer, after C.2.10). The evaluation
  is rebuilt as one unit in the donor's form (C.3). That replaces the
  family order below, and the union becomes its second step: a Basilisk
  term returns beside the donor's only on its own read. C.3.1's
  transplant read first asked whether two families are enough. It
  could not answer: its control failed (BAS-O15).
- **By where the deficit is.** Two thirds of it is realised on the full
  board (C.0), so the full-board families go first, and the order is
  decided again from games before the first family opens (C.2.10).
  The games moved it (2026-11-25): the passers and the king hold the
  deficit, so pawns and passers go first and mobility and space last.
- **The union, not a replacement.** A donor form is built where it carries
  information Basilisk's whole score lacks, and a Basilisk term stays
  where it carries information the donor's whole score lacks. Statically
  the two together beat the donor alone by more than the donor beats
  Basilisk (BAS-E58): the case that the donor can be passed, not only
  reached.
- **Faithful forms, own values.** Each donor form is checked against a
  donor fixture before any fit, and every value is then fitted on
  Basilisk's own games.
- **The whole surface is refit after every family cluster**, and endgame
  handling gets its own bounded cluster that absorbs the archived Phase 6
  remainder.
- **The phase ends on its meters, not on its list.** The frontier pair is
  read after every accepted cluster, and the residual is investigated
  before the closing fits (C.5.1).

The expectation is lower than the goal and is stated in §1's budget: 80
to 140 from the listed clusters, against about 195 needed, and about 120
since C.2.8's refit (BAS-M24). C.2.10 and the first family's gate say
early whether the phase is on course.

**Why classical Stockfish `9587eeeb`.** It is the last classical Stockfish
master, the evaluator Basilisk's oracle measured (BAS-O02), and the reference
its maturity audit already compared against (BAS-E07). Modern Stockfish has no
hand-crafted evaluation. Its families and their conditioning are the donor;
its constants are seeds on a different scale and ride the next refit.

**Correction, 2026-11-17: `9587eeeb` is not Stockfish's last classical
evaluation.** It is the last master before NNUE (tag `SF_classical`), so its
search and evaluation are the last pair tuned together as a classical engine.
That is why it stays the oracle's search. The classical evaluation itself
was kept and retuned for five more releases:
- `af110e02` removed it on 2023-07-11, after Stockfish 16. Its message
  calls that evaluation "probably the best of its class".
- Between `SF_classical` and `sf_16`, the pawn, material and PSQT values
  moved, and terms were added (`DoubledEarly`, `KingOnFile`, a two-phase
  imbalance).
- From `sf_16` to the removal's parent `6a8767a0`, the evaluation files
  differ only in the network's name, so no development build is needed.
- Stockfish 16 with `Use NNUE=false` evaluates with that evaluation alone.
- SF11's evaluation is older than `9587eeeb`'s, so SF11 is no donor
  candidate.

Caveat: from SF12 on, the classical evaluation was only one part of
Stockfish. Some of its later tuning was done with NNUE on, for the
lopsided positions it was still used in, and may not help a classical
engine. So the donor is chosen by measurement in C.0 (the donor choice
below): Stockfish 16's classical evaluation unless that comparison says
otherwise. The other evaluation is the second seed column.

**Decided at C.0 (2026-11-18): the donor is `sf_16`'s classical
evaluation.** Each evaluation was played in both searches, 3,000 games a
host: `sf_16`'s read −3.7 [−10.9, +3.5] in its own search and −1.6
[−9.4, +6.1] in `9587eeeb`'s, so the comparison does not say otherwise
(BAS-O09). The oracle's own evaluation, `9587eeeb`'s, stays a valid
stand-in for the donor's in the meter.

**Reference for the rebuild, 2026-11-25 (maintainer): `9587eeeb`'s
evaluation.** For a whole port the choice reopened. `sf_16`'s classical
evaluation reads the search's root score in its lazy cut and has no
tempo; `9587eeeb`'s is a function of the position and the one the
meters measured; the two play level (BAS-O09). `sf_16`'s added terms
are candidates for the residual work (`analysis/c31_rebuild_2026-11-25.md` §4).

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

**Answered at C.0 (2026-11-18): both, by family.** The values are off
their optimum on this head: a bounded refit closes 56% of the static gap
to the donor. The donor's space, mobility, king-safety, winnable and
pawn-structure terms also carry outcome information Basilisk's whole
score lacks; its passer, threat, rook and queen terms carry none
(BAS-E58). The family clusters stand, each screened before it is built.

**Cluster shape for C.** Each family cluster goes through eight stages
(reworked 2026-11-18, after C.0):
1. The family's current terms are traced, and their activation and residual
   measured on the fitting corpus by cohort. The family screen is read both
   ways on that day's head: what the donor's terms add to Basilisk's score,
   and what Basilisk's terms add to the donor's.
2. The donor's form is read for conditioning and populations, in both donor
   evaluations where they differ.
3. A design states which terms are built, which are replaced, which stay
   (the union rule above), and which neighbouring families share inputs, so
   those inputs are computed once. A term whose gain alone includes zero is
   not built; a unit expected under 5 Elo waits for Phase H.
4. Implementation, with `EvalTrace` coverage for every new slot and the
   reconstruction test. **The donor fixture comes before any fit:** with
   the donor's values, the family's middlegame and endgame rows reproduce
   the unpatched donor's on a fixture of game positions, to the trace's
   resolution. Every deliberate departure is listed with its reason and
   has its own test.
5. A whole-surface Texel refit, with the frozen test split reported once.
6. A PGO bake and SPRT, `[0,3]` (`[3,10]` when the family's residual is
   large).
7. The frontier pair after an accepted gate: the new head against
   Stockfish 16 HCE, 3,000 games at `3+0.03`, the phase's progress meter.
8. The ledger row.

Fit loss is a screen and a falsifier, never acceptance: better holdout loss
has lost games in both engines (BAS-X02).

- [x] **C.0** `[R3]` Evaluation programme investigation
  Investigation: the family map refreshed on the B.11.1 head, residuals and
  activation by cohort, the donor comparison, BAS-E07's
  values-versus-structure question, the shared-input plan, the cluster order
  by expected value, the corpus and refit protocol and the label policy;
  frozen handoffs for C.1 and the first family. No engine implementation.
  Inputs from A.2.3 (`analysis/parameter_inventory_v1.md`): the 23 all-zero
  groups by kind, and space, which no C cluster names yet.
  Added 2026-11-17 (maintainer), the donor choice: whether `9587eeeb`'s
  classical evaluation or `sf_16`'s is the donor (*Correction* above).
  - B.11.5's ranking cannot decide it: there, search and evaluation move
    together, and a stronger search can carry a weaker evaluation.
  - Swapping the evaluation inside one search is not neutral either. A
    search's margins are tuned to its own evaluation, so a host favours
    the evaluation it was tuned with.
  - So each evaluation is played in both hosts:
    - SF16's search with `9587eeeb`'s evaluation ported in, against stock
      SF16 at `Use NNUE=false`;
    - `9587eeeb`'s search with SF16's evaluation ported back, against
      stock `9587eeeb`.
  - The two hosts bracket the answer. The same sign in both decides it.
    Opposite signs say the values are bound to their host, and then the
    family's refit on Basilisk's own search decides: in C the donor gives
    terms and seeds, and every value is refit.
  - The two evaluations' scale ratio is read first (B.0's method), so a
    margin mismatch is measured, not inferred.
  - Cost: the two ports, mostly mechanical (the namespace, the score-typed
    imbalance, the new terms dropped or added), then about 3,000 paired
    games per host at `3+0.03`, roughly 35 minutes each at 14 concurrent
    games.
  - If the donor moves, BAS-O02's evaluation-deficit meter (232.8, read
    against `9587eeeb`) is re-read against the new donor, and C's budget
    row moves with it.
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
    screen before the material cluster builds its list (C-118); space has
    no owner (C-119);
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
    change together (both in C.3 since 2026-11-05, C.4 since
    2026-11-18), never one at a time.
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
  - A regime check for C.4's phase key (TE-2): Triumviratus's network
    divided positions by material × queens (normalised mutual information
    0.43) better than by material bands (0.34); a cheap read of whether
    Basilisk's fitted terms split the same way.
  - A measurement holds only in the regime it was taken (TM-5).
  **Closed 2026-11-18** (`analysis/c0_evaluation_programme_2026-11-18.md`;
  BAS-E58, BAS-O08, BAS-O09).
  - **Donor: `sf_16`'s classical evaluation.** In play it and
    `9587eeeb`'s are equal within about 8 Elo in either search (−3.7 in
    `sf_16`'s, −1.6 in `9587eeeb`'s, 3,000 games each); the frozen rule
    keeps the default. `9587eeeb`'s is the second seed column.
  - **Size: E(0) = +217.3 [+207.1, +228.0]** in `9587eeeb`'s search, and
    +215.7 against the donor.
  - **Values or structure: both, by family.** A bounded refit of the
    existing terms closes 56% of the static gap, so BAS-E07's "fitting is
    exhausted" does not hold on this head. The donor's space, mobility,
    king-safety, winnable and pawn-structure terms add outcome
    information; its passer, threat, rook and queen terms add none.
  - **Where: the full board.** 68 to 69% of the meter's excess score is
    realised at phase 18–24, where the donor's space and mobility lead
    the family screen.
  - Handoffs: C.1 is `READY_FOR_IMPLEMENTATION` (§8.1); the first
    family's contract is §8.2; the corpus, refit and label rules are
    §7.5.
  - Applied 2026-11-18 on the maintainer's sign-off (the packet's
    Amendment 1): the mobility and space cluster is C.3 and king safety
    C.4, C.2.9 holds the family instruments, C.2.10 decides the order
    from games, and C.8.1 investigates the residual.
- [x] **C.1** `[I1]` Evaluation restructure, behaviour-neutral
  `src/eval.cpp` split into family modules, one attack-map and mobility-area
  producer, `EvalTrace` unchanged in meaning, lazy-path semantics preserved;
  exact fingerprint; pooled NPS within noise, amended on 2026-11-20 to a
  measured and accepted cost (below).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Split the untraced `rest` bucket by source (C-90); a corpus-wide score
    hash and the three running-sum reads (`eval.cpp:1206`, `:1897`, `:1975`)
    as the behaviour-neutral proof (C-104); a cache-purity test (C-105); the
    order of terms around the lazy return documented (C-6).
  C.0 handoff (2026-11-18), `READY_FOR_IMPLEMENTATION`:
  `analysis/c0_evaluation_programme_2026-11-18.md` §8.1 freezes the
  modules, the producer's contents, the invariants, the proof and the
  files that list the evaluator's sources. Its §9.4 amends it
  (2026-11-18): `src/eval.cpp` stays the one translation unit and the
  modules are headers, because the oracle's build names that file; and
  the oracle rebuilt from the restructured commit must read the same
  nodes.
  Implemented 2026-11-19, `LOCAL_QUALIFIED` (the packet's §10): bench
  4,777,412, and the served, verdict, trace and lazy-audit digests over
  254,486 positions unchanged; the suites, the sanitizer build and the
  rebuilt oracle (266,667 nodes) pass; the cache-purity and
  lazy-checkpoint tests fail on deliberate breaks. The untraced remainder
  is reported by source and nothing is unattributed. The pooled NPS
  read fails (2026-11-19, the packet's §10.8): −0.71%, 95% interval
  [−0.94%, −0.48%] over six undisturbed cycles. Two bit-exact repairs read
  worse, −2.73% and −1.61%, and are reverted (the packet's §10.9).
  Closed 2026-11-20 (maintainer) with the cost accepted, the packet's
  §10.10:
  - C.1 is the ground for what follows: C.2.9's matched ablation switches
    its modules, each cluster edits one module and adds its inputs to the
    producer, and its digest and tests prove each later change.
  - Under 1% NPS is under about 1 Elo. The clusters rewrite this code, and
    its code generation swung by up to 2.7% under the two repairs.
  - The later clusters gate against the post-C.1 head, so the cost is paid
    once and hides no later read.
  - C.12 recovers it against the pre-C.1 base.
- [x] **C.2** Fit pipeline, corpus, labels and gate instruments
  Fit pipeline, corpus and label contract (archived 8.1–8.8), and since
  2026-11-11 the endgame instruments (C.2.7, which was C.7.1): PLAN §3
  gives every evaluation gate its endgame truth vetoes, so the
  instruments are audited before the first evaluation gate (C.2.8, which
  was C.2.6), not after four family gates.
    - [x] **C.2.1** Fit-tooling contract and Texel handbook
      Fit-tooling contract (archived 8.1.a–h). The leaf adds the Texel data
      contract it writes to *Standing contracts* (A.12). Split on 2026-11-22
      (maintainer) into six sub-steps in the order they run. The archived
      contract merged eight leaves, two of them `R2`, and the audits added
      about twenty defects, each with its test. Every input below sits under
      the sub-step that owns it. The IDs are the Rarog audit's
      (`analysis/rarog_audit_2026-10-24.md`, C-*), the code audit's
      (`analysis/code_audit_2026-10-27.md` §6, a read of `tools/texel/` at
      `fe20f09`, T1–T5) and C.0's (its packet §7.5).
      - Parity of the linear part is sound (the code audit): PST mirroring,
        phase buckets, tempo, rule-50 damping, imbalance and perspective
        match `evaluate` term for term, and runs are deterministic
        (sequential double accumulation).
      - T5 is resolved by A.16, which removed the winnable term, its seven
        slots and its tuner group; C.7.3's complexity term needs an
        instrument of its own.
        - [x] **C.2.1.1** `[I1]` Rows, labels and splits
          The data contract, versioned (archived 8.1.c, 8.1.f, 8.1.g).
          - Every row carries its game id, its start's id and its ply,
            for clustered errors (C-30).
          - Labels are exactly 0, ½ or 1, with the accepted count and every
            rejection reason published. The run fails on a suspiciously
            large rejection fraction.
          - Games are rejected by termination (C-29).
          - Train, validation and test splits are by start, not start plus
            moves (C-28), three of them (C-50). The test split is claimed
            atomically, once, after model selection.
          - Materially different corpus semantics get a named, versioned
            contract. Rarog's snapshot tools (`tools/texel/`: the book
            builder, the yield and relabelling tools, the manifest
            writers) are ported where useful. No gate is widened until
            unknown data fails it.
          Done 2026-11-22 (`d8017a7`). The contract is
          `basilisk-texel-rows-v1`, defined in `tools/texel/extract.py`:
          - **Rows.** Each file opens with a schema line, then
            `fen,result,game_id,start_id,ply,phase`. The result is White's
            `1`, `0.5` or `0`, and the ids are 16 hex digits of SHA-256 over
            the game and over its start.
          - **Labels.** A game is labelled only if it ended by the rules:
            - a game result, with any `Termination` header `normal`;
            - a final position that is a mate the result agrees with, or a
              stalemate, insufficient material, the fifty-move rule or a
              position seen three times, with a draw recorded.

            Every rejection is counted by reason in the manifest, and the
            run fails past `--max-reject-pct`, 1% by default.
          - **Splits.** Train, validation and test by the start's hash, so
            every game from one start shares a split, published together
            with `manifest.json` (inputs and outputs hashed) and never over
            an existing file.
          - **The test split.** `tools/texel/claim_test.py` claims it once,
            by exclusive create, after checking it against the manifest.
          - **Both extractors** decide each game with one function, and
            their outputs are byte-identical.
          - **The tuner** reads v1 through one row reader, a malformed v1
            row being fatal; legacy `FEN;target` files still load.
            `phase911.ps1 -Stage extract` now refuses, pending C.2.1.4.

          Checks:
          - `tools/texel/test_extract.py`, 20 tests, catches three
            deliberate breaks: a split keyed on the moves, the ply off by
            one, and a mate labelled against its result.
          - The tuner's `--verify` reconstructs a v1 corpus exactly (60 of
            60). It exits 2 on a five-column v1 row and on a 0.7 target.

          Not ported:
          - Rarog's book builder and yield read, which are C.2.2's (C-75).
          - Its tablebase relabeller, which waits for the label policy.
          - The de-duplication key, which still ignores the clocks; that is
            C-8, C.2.2's.
        - [x] **C.2.1.2** `[I2]` Tuner correctness
          The tuner computes what the engine serves, and its checks can
          fail.
          - The delta scale (`tools/texel/tuner.cpp:423-438`) omits the
            rook-ending and pawnless-minor scalers and the override rows,
            and applies OCB unconditionally (C-1). Mirror every scaler or
            exclude the rows, with a per-row engine-against-tuner scale
            check. The KmK rule picks the strong side by the score's sign,
            so that scale depends on the sign too.
          - Rows carrying a known-win override or a draw verdict take no
            part in K or in any loss (C.0: left in, they moved K from
            0.00606 to 0.00428).
          - `--verify` is circular (C-2): compare against an independently
            accumulated raw score.
          - Fit the played function, lazy path included (C-5).
          - T3: hidden clamps apply to every coordinate whatever group is
            fitted (`tuner.cpp:458-545`, called at `:1157`): sign priors,
            `passed[0/7] = 0`, `ThreatQueen ≥ ThreatRook ≥ ThreatMinor`,
            and a sequential max that biases mobility upward. None is in a
            manifest, and they would mask a sign bug. All defaults are
            inside them today. Test: the clamps leave the defaults
            unchanged, and inactive slots equal the base after a fit.
          - T4: `--tune all` moves `SafetyTable` linearly with no monotone
            projection (`tuner.cpp:1372`), and `--tune-kingsafety` then
            clamps each entry between its neighbours (`:1395-1398`). On a
            non-monotone table `lo > hi`, which is undefined behaviour for
            `std::clamp`. `T[0]` and `T[1]` are never descent knobs. Test:
            the table is asserted non-decreasing when the stage starts.
          - The L2 penalty is left out of the reported loss, `--l2 0` is
            rejected, and `fit_K` stops silently at its [0.5, 2.5] bounds.
          Done 2026-11-22. The tuner now fits the score the engine serves,
          read from the evaluator's own record of it, not re-derived from
          its rules:
          - **The record** (`1248530`, traced builds only; release bench
            4,777,412 and every C.1 digest unchanged).
            - `apply_endgame` records at each exit whether it left the
              score, scaled it by num / den, or overrode it, and its draw
              verdict.
            - At the lazy checkpoint a traced build records whether a
              release build would stop there, apply_endgame on that value,
              and the trace written up to that point.
          - **The model** (`99ea86f`) plays each row as the engine does: the
            cheap block's value where a release build stops lazily, the
            engine's own endgame step, and the damping.
            - Every stage is recomputed in the engine's integer arithmetic
              and checked against the evaluator: the running sum, the
              taper, the endgame step, the damping, and the lazy taper and
              step. A mismatch is fatal at load (C-1, C-5).
            - Overrides and draw verdicts take no part in K or any loss, in
              both fits.
            - The gradient is the trace times the seed's scale and the
              damping's slope.
          - **`--verify`** checks that model on every row against the sum
            the evaluator accumulated itself, so it can fail (C-2).
          - **`--played-hash`** hashes the played scores as
            `tests/eval_hash.cpp` hashes a release build's, so equal hashes
            prove the lazy path and the endgame steps row for row.
          - **The other items:**
            - the clamps must leave the seed unchanged, or the fit stops;
            - slots outside the fitted group are held at the seed and
              asserted after the fit (T3);
            - the king-safety stage asserts a non-decreasing table (T4);
            - `--l2 0` is accepted, and the objective is reported beside
              the data loss;
            - K at a bound of its search is fatal.

          Read on 111,112 rows extracted from `armA_basilisk8k.pgn`:
          - In train, 92,758 rows are used, 8,468 of them at the lazy
            checkpoint; 7,168 overrides and 74 verdicts are left out. The
            three splits pass `--verify`.
          - The played hash equals the release `served_hash` on all three
            splits. The traced full value, which the tuner used to fit,
            hashes differently on each.

          Deliberate breaks, each caught:
          - dropping the frozen additions fails `--verify` on 2,792 rows;
          - ignoring the endgame scale fails it on 143;
          - playing the full value on lazy rows changes the played hash;
          - a clamp narrowed below the seed stops `--tune`;
          - a dented safety table stops the king-safety stage;
          - inverted labels drive K to its bound, which is fatal.

          Known limits, recorded rather than repaired:
          - A row's lazy membership and its pawnless scaler's strong side
            are the seed's, as in any linearisation; a row the fit moves
            across the margin keeps its seed path.
          - `--dump-eval` still prints the full traced evaluation, which
            `tools/diag/scale_ratio.py` reads as such.
        - [x] **C.2.1.3** `[R2]` Coordinate manifest and initial vectors
          Archived 8.1.b and 8.1.d, with Manta's catalogue (BAS-X14).
          - Every coordinate is classed free, fixed or excluded, with gauge
            anchors and reconstruction. Every production scalar has one
            instrument, or a written invariant or unidentifiable
            disposition.
          - Gauge freedoms (C-19): `phase911.ps1` runs `--tune all`, which
            frees material and every PST.
          - Per-coordinate king-safety steps (C-18); shelter and storm
            fitted by some stage (C-21); dead and collinear imbalance
            slots in the manifest (C-20); support-based freezing (C-52).
          - Initial vectors are complete and explicit (C-53), every surface
            coordinate recorded, and a partial or missing-by-accident
            vector rejected. B.10's PST orientation test passes on them
            before any fit reads them, limited to seeds (C-22).
          Done 2026-11-22 (`a2df86d`). `tools/texel/surface_v1.tsv`
          (basilisk-texel-surface-v1) classes all 1,159 slots, one row each
          with its reason. It is generated by `tools/texel/surface.py` from
          the tuner's own registry, and `check` fails when it drifts from
          the rules.
          - **The counts:** 1,025 free linear, 38 free king-safety knobs,
            96 fixed, none excluded.
          - **Ten mean anchors** hold a set's mean at the seed where the
            data cannot see its level: both king PSTs, and the eight
            mobility tables, where every piece reads one entry.
          - **Held as gauge anchors:** material, while the PSTs are free
            (C-19); `KsNoqueenDen`; and `SafetyTable[0]`. Each king reads the
            table once with opposite signs, so `SafetyTable[1]` is now a knob
            (C-18).
          - **King-safety knobs** carry their own bounds and first step, a
            tenth of their range, and each step halves on its own (C-18).
          - **The shelter and storm terms are free linear** (C-21).
          - **Dead slots are held:** pawns on ranks 1 and 8, relative ranks
            0 and 7, the never-read piece indices, and the `ImbTheir`
            diagonal.
          - **Duplicates are held** (C-20), including three the rank
            analysis found and the audits had not:
            - `ImbLinear` is material and the bishop pair again;
              `ImbOur[0]` is the pair, and `ImbOur[1]` is
              `BishopPairPawns*`.
            - `Rook7thMg/Eg` is the rook PST's seventh-rank squares, since
              it has no other condition.
            - `PassedMg/Eg[6]` is the pawn PST's seventh rank, since such a
              pawn is always passed.
            - The taper's phase is a weighted piece count, so a piece's mg
              PST level (and `BpMg`) is exactly a phase-weighted sum of the
              imbalance's cross terms. The queen's cross term is held, one
              per identity: `ImbTheir[15..19]` and `ImbOur[20]`.
            - `ImbOur[6]` and `ImbOur[9]` restate the pair while no side has
              three bishops.
          - **`rank`** reads `basilisk-texel --dump-coeffs` (float64). On
            30,000 rows of `armA_basilisk8k.pgn` the free columns under the
            anchors have full rank: 1,013 of 1,013, the smallest relative
            singular value 0.0024. With support freezing at 200 rows (C-52),
            269 slots freeze and 746 of 746 remain. `--out` writes the
            corpus's manifest.
          - **The tuner:**
            - `--init FILE` takes a complete vector and refuses a partial,
              duplicated or unknown entry (C-53).
            - `--surface FILE` with `--tune surface` fits the free linear
              slots under the mean anchors, and with `--tune-kingsafety` its
              knobs.
            - On real data, a 30-epoch fit moved 738 slots, no other slot
              moved, and each anchored mean stayed within rounding (0.2).
          - **`check-vector --seed`** runs the PST orientation test on a seed
            vector (C-22), and catches a rank-mirrored eg knight table.

          Checks:
          - `tools/texel/test_surface.py`, 11 tests.
          - Removing the king-PST anchor fails the rules test and the
            registry check, and `rank` names the two king-PST levels.

          Left as they are:
          - The named tuner groups (`texel` and the rest) are unchanged:
            they still skip shelter and storm and free the collinear slots.
            The contract's path is the surface, and C.2.1.4's driver uses
            only that.
          - The linear clamps stay in the tuner's code, guarded at the seed
            (T3).
          - `test_eval`'s orientation test still reads the engine's
            defaults, which are seeds until the first bake; C.2.8's bake
            moves it onto the seed vector.
        - [x] **C.2.1.4** `[I1]` Fit driver and provenance
          Archived 8.1.a and 8.1.e.
          - K is fitted once on validation and frozen across stages and
            arms (C-49), and K and held-out loss are reported by phase band
            (C.0).
          - T1: the Adam schedule, not the data, bounds how far a
            coordinate can travel. Full-batch Adam moves a one-signed
            coordinate about `lr` per epoch (`tuner.cpp:67-68`, `:1154`;
            `phase911.ps1:41` runs 100 epochs at 0.3, so about 30 cp), and
            there is no convergence criterion; C-66's 11–57 cp PST
            movements would be clipped, and C.0's screening fit hit the
            bound at epoch 16. Test: a one-feature synthetic CSV whose
            optimum is 60 cp from the seed must be reached; fail a run
            whose best epoch is the last or whose travel reaches
            0.9 × lr × epochs.
          - Rarog's stage schedule (C-54); residuals on unused train rows
            (C-51); never hand-revert a block (C-55).
          - Every input and output is hashed (corpora, row order, splits,
            configs, binaries, source revisions, tablebases, vectors,
            reports), and source and the production binary are restored
            byte for byte on success or failure.
          - T2: `bake.py` writes every slot that differs with no provenance
            or completeness check (`bake.py:173-174`; the dump carries no
            header hash). A stale dump reverted 42 members and both PST
            blocks in a dry run and skipped 92 absent members silently.
            Slot placement and CRLF are correct. Test: `test_bake` cases
            that fail on a missing member, an unknown name and a hash
            mismatch.
          - Scores re-dumped after a refit (C-69); fixtures that depend on
            fitted weights fixed in their own commit (C-70); the fit
            driver's invariants (C-71); the dense matrix at full scale
            (C-73).
          - Basilisk's `tools/texel/phase911.ps1` driver is adopted into the
            contract or retired. A new driver is Python (`agents/implementation.md`
            *Tools*); `phase911.ps1` reads `$LASTEXITCODE` through
            `Tee-Object` and is Windows-only.
          - An option to register, not a default (2026-11-11; Rarog's
            EVAL-13): a second-difference penalty on the mobility tables
            and the pawn table, so a fit cannot leave a sawtooth in a term
            that should be smooth.
          Done 2026-11-22.
          - **The tuner** (`d21de4f`, `3169663`):
            - **Sparse rows.** Only nonzero coefficients are stored (C-73);
              a fit's vector is byte-identical to the dense form's.
            - **K.** `--K` freezes it (C-49), and `--loss` reports the loss
              by phase band at any vector, optionally after `--skip` rows,
              the residuals on unused train rows (C-51).
            - **Convergence (T1).** `--patience` stops a converged run. A
              run whose holdout still improves at its last epoch, or whose
              largest move reaches 0.9 × lr × epochs, exits 3 without
              writing a vector.
            - **`--smooth`**, registered and off by default: a
              second-difference penalty on the mobility tables and the
              passer and connected-pawn rank tables.
            - **Provenance.** Every written vector carries the hash of its
              seed and of its chain's root.
            - **The clamps never move a held slot** and run after the mean
              anchors: an ordering chain bounds a free entry by its held
              neighbours.
          - **`bake.py` (T2)** refuses a vector that has no schema line,
            misses a member, names an unknown one, or has a root that is not
            this header's defaults.
          - **`tools/texel/fit.py`** (`40e7a1e`, `10c08da`) replaces
            `phase911.ps1`, which is retired (`987732e`).
            - It needs a clean tree, rehashes the corpus files against their
              manifest, gates parse errors at 0, and checks the surface.
            - It builds its own traced tuner and release engine and checks
              the seed's bench.
            - It verifies the model, fits K once on validation, and runs the
              stages king safety, linear, king safety and polish (C-54),
              each from the vector the last one wrote.
            - It bakes the final vector and rebuilds. The rebuilt tuner must
              hold that vector exactly and pass `--verify`; the new bench,
              the played hash of validation (C-69) and `test_eval` are
              recorded. A failing test is reported, not edited (C-70).
            - It hashes every input and output in `run.json` and restores
              `src/eval_params.h` byte for byte on success or failure
              (C-71), leaving the baked header and its diff in the run
              directory.
            - It bakes the whole final vector, never a block of it (C-55;
              the handbook says why).

          Checks:
          - **`tools/texel/test_fit.py`.** One unclamped slot,
            `PawnIslandsMg`, is labelled so its optimum is 60 cp above the
            seed. A run with room lands within 4 cp; lr 0.3 × 100 epochs
            exits 3. With the checks disabled, the second test fails.
          - **`test_bake.py`**, 7 tests, covers each refusal.
          - **The driver end to end** on the 100,000-row `armA` corpus:
            - the seed's bench was 4,777,412;
            - K was 1.714, by band from 1.44 in the opening to 1.94 in the
              endgame;
            - the four stages chained, and the validation loss went from
              0.08973 to 0.08885;
            - the baked build verified, with bench 3,219,101 and `test_eval`
              126 of 126;
            - the header was restored. That is a toolchain check, not a fit
              to keep.
          - **The driver's refusals.** A forced failure after the bake
            restored the header byte for byte. A dirty tree and a used run
            directory are refused.

          Found on the way, and fixed: the first end-to-end run stopped at
          the polish stage's seed check. The mean anchors had shifted free
          mobility entries past frozen neighbours, and `ThreatMinorMg` had
          risen past a held `ThreatRookMg`, because the ordering clamps
          could only raise the later entry. The clamps now bound a free
          entry by its held neighbours instead.
        - [x] **C.2.1.5** `[I1]` Texel tooling hygiene and CI
          - No workflow runs `tools/texel/test_*.py`; two of three need
            python-chess, absent here.
          - `test_edge.csv` is referenced nowhere, and `import_beast.py` is
            obsolete.
          - The usage text omits four modes, and comments cite 1,183 or
            1,190 parameters against `EVAL_PARAM_FLAT_SIZE` 1,178.
          - The README and `sample_fens.py` hard-code `A:/` paths.
          Done 2026-11-22 (`58baa0e`):
          - **CI.** A new `texel` job builds the traced tuner and runs
            `test_extract`, `test_surface`, `test_bake`, `test_sample_fens`
            and `test_fit` with `BASILISK_TEXEL` set. It puts python-chess
            1.11.2 and numpy in a venv, and `CI complete` needs it. The
            YAML parses and every step's Python passes here; the Linux job
            itself first runs on push.
          - **Removed:** `test_edge.csv` and `import_beast.py`. The one
            record that linked the importer now names its last commit,
            `23f5557`.
          - **The usage text** names all eleven modes, as checked against
            the dispatch. The stale counts (1,116, 1,190; the registry is
            now 1,159) are gone from the usage text and comments;
            `--audit-coverage` prints the live count.
          - **No `A:/` path** remains: the README and `sample_fens.py`
            point to the handbook for where the Beast pool lives.
        - [x] **C.2.1.6** `[R2]` Texel handbook and the standing contract
          Archived 8.1.h. A Basilisk Texel handbook: canonical paths,
          immutable resources, supported commands, the surface partition,
          the pipeline, traps, reference timings and the recovery
          procedure. Rarog's, the snapshot's
          `analysis/texel_fitting_handbook.md`, is a template, not a source
          of constants. The data contract C.2.1.1 to C.2.1.4 settle is
          written into *Standing contracts*.
          Done 2026-11-22.
          - **`tools/texel/HANDBOOK.md`:** what a fit is; the resources and
            where they live; the four named contracts (rows, surface,
            vector, fit); every tool and tuner mode; the played-score model;
            the surface partition and its identities; a fit end to end; ten
            traps, among them C-55's "never bake or revert one block of a
            joint fit"; measured reference numbers; and the recovery
            procedure. Rarog's handbook gave the shape only.
          - **PROCESS *Texel procedure*** points to the handbook, and its
            policy now matches the contracts. The old "sequential joint
            bake" rule is gone: stages chain through `--init` and the whole
            vector is baked once.
          - ***Standing contracts*** gains the Texel row.
          - **The Texel toolchain's leaves, C.2.1.1 to C.2.1.6, are done**,
            so C.2.1 closes.
    - [x] **C.2.2** `[R2]` Corpus design
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
      C.0 inputs (2026-11-18, its packet §5.4 and §7.5): the first read
      is H-distribution, the evaluators on positions one move off the
      game line beside the game positions; the gate games (25,970 games,
      254,486 rows) can serve as validation; 48% of quiet game rows are
      at phase 9 or below, where 16% of the same-search deficit is.
      Done 2026-11-22 (`analysis/c22_corpus_design_2026-11-22.md`):
      - **Repaired** (`040dbea`): C-8 (the de-duplication key holds the
        rule-50 clock), C-26 (datagen needs an explicit book) and C-27 (the
        sampler shares the extractor's phase rule and states its mix).
      - **Measured:** the Beast pool hashed, read-only; the yield per start
        phase (C-75); and the rows' dependence within a game, ICC 0.69. So
        corpus A is sized in games: about 1.4 effective independent rows a
        game is the ceiling.
      - **Registered:** the design, and BAS-O10, the H-distribution read
        (`7e3bfe3`).
      - **Decided (maintainer):** equal output phase weights and about
        1,000,000 games.
      - **BAS-O10: valid, NOT SUPPORTED.** The game-line deficit is there
        (+0.015 and +0.019 skill), and one move off the line it is no
        wider (deltas −0.013 and +0.003). So corpus A's starts are pool
        positions only.
      - **The design:** a 1,000,000-start book of distinct pool positions
        at 70/10/10/5/5 by start phase, disjoint from every earlier
        corpus's starts; 8,000 nodes, natural terminations; two rows per
        phase per game, equal output weights, 5% validation and 5% test by
        start; nested learning-curve fits in C.2.8.
    - [x] **C.2.3** `[V]` Generate and publish corpus A
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
      Done 2026-11-23, by the agent on the maintainer's overnight leave.
      Evidence: `tools/results/c22/`. Corpus A is `tools/texel/data/corpusA/`
      (ignored), with rows in `rows/` (read-only).
      - **C-32** (`0575687`): `tools/datagen.ps1` now refuses three things
        before any game. These are book wrap and a Basilisk binary whose
        manifest is missing, hashes another binary, or records a dirty tree
        or no bench. After fastchess it audits the game count and every
        `[Termination]`, so any ending but `normal` fails the run. Each guard
        was tested both ways, among them an illegal-move engine (4 of 4
        faults caught). The same edit fixed a latent bug: PowerShell names
        are case-insensitive, so `$enginePath` overwrote `$EnginePath` and
        the manifest check had never run.
      - **The engine:** `basilisk-c2-corpusA-40ae560`, PGO pext from clean
        `40ae560`, bench 4,777,412, `8D4925EC…`. It plays as the C.1 head
        does (BAS-M23).
      - **The book:** `corpusA_book_1m.epd`, `fcb02130…`. It holds 1,000,000
        distinct pool starts at exactly 700k/100k/100k/50k/50k by start
        phase, with 734,834 earlier corpus starts excluded, seed `20261122`.
        It was sampled in one more read-only pass over the pool (§2 of the
        packet), under the design the maintainer accepted. BAS-O10 read NOT
        SUPPORTED, so there are no off-line starts.
      - **The games:** 1,000,000, in ten slices of 100,000 with disjoint
        book slices, seeds `20261122+k`, 8,000 nodes, Hash 16, 31
        concurrent, no adjudication. They took 5 h 34 min, about 2,990 games
        a minute. Every slice is `normal=100000`, 0 faults, and its
        manifest holds the slice's `opening_sequence_sha256` (C-76).
        Results: 37.81% 1-0, 30.72% 0-1, 31.47% drawn. Every decisive game
        ends in checkmate (68.53% of games). The draws are 21.72%
        threefold, 8.43% insufficient material, 0.94% fifty-move and 0.38%
        stalemate.
      - **C-76:** a 4,000-game pilot from the book's first 4,000 starts
        replays move for move in slice 0, whose order and seed differ: all
        4,000 games are identical (`replay_check.py`, which catches a
        one-move change).
      - **C-62:** on 20,000 slice-0 games, 2.54% of the 8,137 clean
        tablebase wins reached were not won. BAS-E46 measured 19.77% at the
        same 8,000 nodes on 1.9.3, and 13.65% at 25,000. The worst families
        are KRP-KRP at 10.7% and KRPP-KR at 8.7%. So C.2.4's relabel prize
        is about an eighth of what BAS-E46 sized.
      - **C-56:** the tree read is registered in C.2.8 (`2193afb`).
      - **Rows** (`basilisk-texel-rows-v1`, extractor at `2193afb`): cap 2
        a phase a game, quiet filter, `--skip-end 6`, equal weights, 5%
        validation and 5% test by start. Train is 3,770,000 rows,
        754,000 a phase (`32CD3D63…`), from 896,341 games. Validation is
        209,444 rows (`851B6FF8…`) from 49,737 games, and test 209,444
        (`F2A07F4D…`) from 49,899. No game was rejected, and 4,023 gave no
        candidate.
        The deep endgame binds: validation's 41,943 eligible deep rows set
        the size. The tuner's model reproduces the engine on validation
        (`--verify` PASS). The test split is unclaimed.
    - [x] **C.2.4** `[R2]` Matched label arms
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
      Done 2026-11-23 (`analysis/c24_label_arms_2026-11-23.md`, BAS-O11):
      - **The label contract** `basilisk-texel-labels-syzygy-v1` and
        `tools/texel/relabel.py` (`fbeceaf`), with `test_relabel.py` in
        CI. A row of six men or fewer with no castling right takes
        Syzygy's verdict under its own rule-50 clock, in Stockfish's
        root_probe form: a win when `dtz + clock <= 100`, a loss when
        `-dtz + clock <= 100`, otherwise a draw. So cursed results are
        draws. Missing tables are unknown, and nothing propagates into
        larger rows.
      - **Corpus B** (`tools/texel/data/corpusB/rows`, train
        `B456BCE4…`) is corpus A row for row except 10,584 train labels
        (0.28%), and `check` proves it. The changes are symmetric: 5,525
        draws become decisive and 5,008 decisive labels become draws. The
        clock draws 380 rows Syzygy alone would call decided. Basilisk's
        own probe agrees on all 1,204 changed validation and test rows.
        Labels stay WDL; no blended targets (C-67).
      - **Whole-game adjudication: no arm now** (BAS-O11). It would change
        2.58% of rows, 62,259 of them above six men (C-63), with errors in
        both directions. A post-hoc relabel would isolate it, so the
        verdict is about power, not confound. Retry triggers are recorded.
      - **For C.2.8, the maintainer's decision:** the packet recommends
        fitting A and B, and running the label-contract SPRT only if the
        fits differ beyond noise.
    - [x] **C.2.5** `[R2]` Initialization control
      Initialization control (archived 8.6): fit from the accepted and from
      neutral starts under identical K, data and schedule; register the
      production rule before the test split opens.
      **The piece-square tables start from classical Stockfish 16's**
      (maintainer decision 2026-11-18, replacing 2026-10-23's start from
      B.10's PeSTO-derived values).
      - The seed is `sf_16`'s `psqt.cpp` (GPL-3, like Basilisk), converted
        to Basilisk's evaluation scale by B.0's measured ratio. Its
        mirrored half-board piece tables are expanded to 64 squares; its
        pawn tables are already full width.
      - Why: the refit then carries no PeSTO derivation, so PeSTO's CC
        BY-SA attribution and its share-alike question both end when the
        tables land. The values follow the evaluation donor Phase C ports.
      - If C.0's donor comparison picks another evaluation, the seed
        follows that donor.
      - A neutral start stays not required. The fit's other terms keep
        their own starts.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Rarog's control: a neutral start fitted 2.38% worse (-13.2 clustered
        SE; C-65), consistent with the 2026-10-23 decision.
      C.0 inputs (2026-11-18): the donor stays `sf_16` (BAS-O09), so the
      seed stands. By outcome a donor value converts at 0.613 Basilisk
      centipawns a Stockfish unit, against B.0's 0.557 by magnitude; the
      fixed-K screen (C-23) decides between them.
      Done 2026-11-23 (`analysis/c25_initialization_2026-11-23.md`,
      BAS-O12 with its Correction 1). The maintainer left the decision to
      the agent, asking for the reasoning to be checked.
      - **Built:** the seed builder `tools/texel/seed_sf16.py` (`c423754`),
        which transplants `sf_16`'s table shapes and keeps their levels,
        and the tuner's `--min-delta` (`1f16f4c`).
      - **Measured:** four fits on corpus A at frozen K: the header start,
        Stockfish 16's at 0.557 and 0.613, and 0.613 flipped by rank.
        - Every table converges from every start except the mg king,
          which the data does not identify; the eg queen and eg pawn
          converge slowly.
        - No single slope converts a Stockfish 16 table: the converged
          tables' slopes run from −1.21 to +3.0.
        - The header start's 2.1e-5 lead is on the tuner's model. On the
          fitted vectors' realized losses it is 1e-7, below what one
          linear stage resolves (about 2e-5).
        - Every arm re-levels the tables by up to 179 cp, which is for
          C.2.8's fit report.
      - **Production rule** (registered before the test split opens):
        - C.2.8 starts from `seed_sf16.py --scale 0.35`, BAS-O12's frozen
          rule, from one start.
        - Arms are compared by realized loss.
        - A ported threshold is converted by the leaf that ports it,
          at 0.557 or 0.613.
        - Retry trigger: a header-start arm, if C.2.8's king-table read
          flags the mg king or its gate fails while the loss improves.
      - **Withdrawn:** the first recommendation, 0.557 with both starts
        fitted. It misread the pooled slope, which is mostly the king
        tables', and the flipped start, which is not a negated shape.
    - [x] **C.2.7** Endgame instruments and gate integrity
      It was C.7.1 until 2026-11-11. Instruments and gate integrity
      (archived 6.6.a–e and 6.6.h). Split into four sub-steps on 2026-11-23
      (maintainer, on the audit's §5); the held-out tooling (FEN-hash
      halves, the write-once registration, the spent-cohort rule) moved to
      C.7.3, its first consumer.
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
        - [x] **C.2.7.1** `[R2]` Audit against the Rarog snapshot
          Classify every delta between `tools/diag/endgame_*.py` and the
          snapshot's endgame instruments as portable, adapter-specific,
          covered or inapplicable before changing any instrument.
          Done 2026-11-23
          (`analysis/c27_endgame_instruments_audit_2026-11-23.md`):
          - No instrument changed. Of 24 deltas, 16 are portable or
            covered, 6 need Basilisk adapters, and 2 are prioritisers for
            C.7.2.
          - **Found:**
            - the truth report changed its termination rule without a new
              schema, and the veto and compare tools still default to the
              pre-fix baseline;
            - a thin family's zero variance reads −∞ sigma and blocks;
            - the cohort files are not pinned byte-exact.
          - No gate since 6.0.e used these tools, so no recorded result
            changes.
        - [x] **C.2.7.2** `[I2]` Truth contract v2 and guard tests
          The audit's §4 handoff:
          - a versioned truth schema with its layers and per-family
            digests;
          - no default baseline;
          - McNemar with a thin-sample refusal;
          - byte-exact cohort files;
          - a known-bad test for every guard;
          - the colour-mirror sweep over the endgame EPDs.
          Done 2026-11-23 (tools and tests only; bench unchanged):
          - **The contract:** `tools/diag/endgame_contract.py`.
            - `endgame_truth.py` writes `basilisk-endgame-truth-v2`, with
              layers, per-family digests and each game's moves.
            - Every consumer loads through the contract. It accepts v2,
              and v1 only when post-fix: the 6.0.b report is refused,
              while the refixed arms and BAS-D22 are accepted.
            - Digests are recomputed from the records, so a subset of
              families compares by digest.
          - **The guards:**
            - `endgame_compare.py` tests conversion by McNemar, and a
              thin or zero-variance family is INDETERMINATE instead of
              ±∞;
            - neither compare nor vetoes has a default baseline;
            - `material_lost` is no longer an outcome;
            - the cohort files are `-text`.
          - **Tests:**
            - `test_endgame_contract.py`: 22 tests, each guard on a
              known-bad and a good input. A fake UCI engine that keeps
              state across `ucinewgame` is caught sharded against
              serial; one that resets reproduces. It runs in CI's texel
              job beside `test_endgame_vetoes.py`.
            - In `test_endgames`, the colour-mirror check covers 90
              positions: the suite, the 60 drawn games, and the pawnless
              and wrong-bishop recogniser positions. It passes, and with
              the sign flip removed 78 of the 90 fail. `test_eval`
              shares its helper (`tests/colour_mirror.h`).
          - **Checked on the real engine** (`tools/results/c272/`,
            BAS-D22's binary `31BC229A…`, KQ-K and KRP-KR at 60,000
            nodes):
            - serial and four workers are identical move for move;
            - the v2 runner reproduces BAS-D22's 64 records exactly.
          - **For C.2.7.3:** against the 6.0.b-era head, BAS-D22's head
            (458/480 against 361/480) trips 9 single-budget clean-win
            vetoes, 5 of them in KNN-KP. That is the case the frozen
            two-of-three-budgets rule exists for.
        - [x] **C.2.7.3** `[V]` Deployed nodes and the budget bracket
          Port `nodes_per_move.py` and `endgame_budget_bracket.py`. Read
          nodes per search at the gate's time control on an idle host, and
          set three budgets straddling the median. The veto rule is frozen
          in the audit's §4: a position veto needs two of three budgets.
          Done 2026-11-23 (BAS-D23):
          - **Built** (`901e519`, 16 tests in `test_nodes_bracket.py`):
            - `nodes_per_move.py` plays the gate's conditions: UHO starts,
              two engines a game, 15 games at once, the real clock;
            - `endgame_budget_bracket.py` has `run`, which refuses
              differing cohorts or a misnamed budget, and `veto`, the
              two-of-three rule.
            - Both were dry-run (`tools/results/c273/dry/`).
          - **The rule, frozen in BAS-D23:** M is the median over moves
            with at most 6 men (the overall median if there are under 300
            such moves), and the budgets are M/3, M and 3M.
          - **Run 1 discarded:** the host was not fully idle, which cost
            it 12% of its nodes, so BAS-D23's stop rule withdrew it.
          - **Run 2, kept** (the maintainer, on an idle host): the head's
            binary `basilisk-c273-head-pext-pgo.exe` (bench 4,777,412).
            - 150 games at 3+0.03, 15 at once, no forfeits;
            - the median is 121,819 nodes a search, and 63,876 over
              2,129 moves with 3 to 6 men.
          - **The bracket: 21,000, 64,000 and 190,000 nodes.** Every
            earlier truth read ran at 60,000, so C.2.7.4 runs all three on
            the head.
        - [x] **C.2.7.4** `[V]` The C head's endgame baselines
          - The head over the whole cohort at 21,000, 64,000 and 190,000
            nodes under v2 (`endgame_budget_bracket.py run`). BAS-D22's
            60k report is at no bracket budget, so it is not reproduced.
          - Re-read B.9.3's enumeration on the C head (C-133, C-142).
          - Re-run nothing else.
          Prepared 2026-11-23 (`tools/results/c274/`):
          - **Binaries:**
            - truth: the head's `basilisk-c273-head-pext-pgo.exe`
              (`EDDBA36E…`, bench 4,777,412);
            - enumeration: `endgame_verdicts.exe` built at `f69a54a`
              (`1E1F5266…`). It passes its golden fixture test and
              reproduces B.9.3's KPvK and KBNvK counts.
          - **Dry runs:** a sliver of the bracket at the three budgets, and
            the bracket veto of the head against itself (PASS).
          - **The runner:** `run_c274.py` holds B.9.3's class lists,
            checked equal to its records. It runs the bracket at 30 workers,
            then the enumeration (about 40 minutes, the whole CPU), and
            writes each step's exit status to `status.json`.
          - **Run** by the maintainer (`run_c274.py`, about 43 minutes).
          Done 2026-11-23 (`analysis/c27_endgame_instruments_audit_2026-11-23.md`
          §6):
          - **Baselines** (`tools/results/c274/head/`, v2): the head
            converts 443, 448 and 463 of 480 at 21k, 64k and 190k. The
            bracket veto of the head against itself passes.
          - **Single budgets are noisy.** At 60k the head reproduces
            BAS-D22's 770 records exactly, so the evaluator split changed
            nothing here. Yet 60k to 64k flips 22 positions (KNN-KP 10 to
            4).
          - **B.9.3 re-read** (C-133, C-142):
            - the exhaustive classes change only where B.9.5 repaired:
              same-coloured bishops are dead (F6's 765,152 overclaims now
              0), and the pawnless scale of 0 no longer fires with pawns;
            - nothing new is wrong;
            - KBB-KB's scale 4 crosses 0.5% only because its denominator
              shrank (V6, C.7.5);
            - V3 to V6 remain at their recorded rates.
    - [x] **C.2.8** Matched fits and the label-contract gate
      It was C.2.6 until 2026-11-11. Split into four sub-steps on
      2026-11-23 (maintainer), so the fit and the gate are registered by
      research before anything runs.
      Matched whole-surface fits of A and B on the current surface, then the
      registered label-contract gate (archived 8.7–8.8); fit loss explains,
      the SPRT decides.
      The piece-square tables re-fit here, from C.2.5's Stockfish 16 seed
      (below). B.10's flipped values have never been fitted in their correct
      orientation, and the old
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
      C.0 inputs (2026-11-18, its packet §5.2 and §7.5): this refit is a
      strength candidate, prior +15 to +40. A bounded screening fit
      improved held-out Brier by 2.58% and moved every endgame passer
      term down together, the compensation the mirrored pawn table left;
      the directional screen reads that group and E6's signs before the
      bake.
      Registered by C.2.3 (2026-11-22, C-56): Rarog's natural-termination
      corpus multiplied its checkmates 57 times, inflated the king table,
      grew the tree 12.3% and broke the KBNK floor. Corpus A also ends its
      games naturally, so before the bake the fit report reads three things
      against the seed:
      - the king-safety and king-PST terms' movement;
      - `bench 13` nodes, where growth over 5% is a finding the gate must
        explain;
      - the deterministic endgame floors (KBNK, KQK, KRK, KBBK), which must
        all still pass.
      Corpus A's checkmate share is in its C.2.3 record.
      Registered by C.2.7 (2026-11-23, its audit's §6): the endgame veto
      is `endgame_budget_bracket.py veto` against
      `tools/results/c274/head` at 21,000, 64,000 and 190,000 nodes. A
      position vetoes at two of the three budgets, and an engine anomaly
      at any one. The candidate is measured with `endgame_budget_bracket.py
      run` at the same budgets.
      Registered by C.2.5 (2026-11-23, its packet §4):
      - The tables start from `tools/texel/seed_sf16.py --scale 0.35`,
        from one start. BAS-O12 bounds the start's effect below 2e-5.
      - Arms are compared by realized loss (`--loss --init`), never by one
        stage's model loss.
      - The fit report explains the tables' re-levelling, up to 179 cp.
      In progress 2026-11-23 (BAS-O12, Correction 2): the re-levelling is
      explained, and it raises a registration question.
      - **Explained:**
        - It is not a mobility trade, and the mg pawn is not negative:
          removing one from a middlegame costs 111 to 120 cp fitted, 145
          at the header.
        - In endgames every piece is worth 12% to 35% more, the
          expected response of one frozen K to endgames whose own K is
          2.33 against 2.01.
      - **The question:** the fitted vectors raise the lazy-checkpoint
        share about 27% (C-97; `LAZY_MARGIN` never refit, C-98).
      - **Also open:** `fit.py` cannot pass `--min-delta`. Its default
        schedule (400 epochs at a rate of 1.0) is unvalidated, and
        BAS-O12 needed about 3,000 epochs at 0.3.
        - [x] **C.2.8.1** `[R2]` Register the fit and the gates
          The registration must settle, before any fit runs:
          - **The magnitude contract** (C-97, C-98): how the fit holds or
            prices the endgame re-valuation and the lazy share's rise.
            The starting position:
            - one frozen K, as the protocol has it;
            - `LAZY_MARGIN` unchanged in the candidate, so one causal
              axis;
            - the lazy share and the per-phase piece values read as the
              contract's reads;
            - retries registered: a per-phase-K fit and a lazy-margin
              refit.
          - **The schedule:** `fit.py`'s stage budgets, validated for
            Basilisk with `--min-delta` (8.7.b).
          - **The anchors** for the tables, loose enough to move them.
          - **The gates:** one causal axis each (C-57).
            - The refit against the head.
            - Corpus A against B, conditional on the fits differing
              beyond noise (C.2.4's packet §6).
          - **Before any bake:** the reads registered above (C-56's king
            tree, `bench 13`, the floors, the endgame bracket veto).
          - **Each gate:** its prediction, bracket, cap and stop rule.
          Done 2026-11-23 (`analysis/c281_fit_registration_2026-11-23.md`;
          BAS-O13, BAS-E59, BAS-E60):
          - **Magnitude:** one frozen K, `LAZY_MARGIN` unchanged. The
            contract is read (lazy share, score change by band, piece
            values by phase) and not enforced. Retries, in order: a
            per-phase-K fit, then a lazy-margin refit.
          - **Schedule:**
            - the linear stages run at BAS-O12's validated rate 0.3,
              patience 50, `--min-delta 1e-6`, 20,000 epochs at most;
            - the polish runs at a quarter of the rate;
            - king safety keeps its defaults.
            - Anchors: L2 0, the surface's own only.
          - **K:** fitted once on A's validation at the seed, and frozen
            for both arms.
          - **Gates:**
            - BAS-E59, the A fit against the head: `[0,3]`, 4,000
              pairs, with the bracket veto;
            - BAS-E60, B against A, runs only if B's fit beats A's on B's
              labels by at least 2e-5, with a game-bootstrap interval
              excluding zero.
        - [x] **C.2.8.2** `[I1]` The fit driver to the registered schedule
          The registration's §5, in `tools/texel/fit.py`:
          - `--init FILE`, a seed vector rooted at the header;
          - `--K` to freeze a K;
          - `--min-delta` for the linear and polish stages;
          - per-stage settings;
          - the lazy-checkpoint count in its loss records.
          Also `tools/texel/piece_values.py`, the marginal-value read, with
          tests. The tuner's travel guard stays.
          Done 2026-11-23 (`9640b24`, `3d66c84`; tools only, bench
          unchanged):
          - `fit.py` gains `--init`, `--K` and `--min-delta`. It records
            each stage's settings and the lazy share in every loss.
          - `piece_values.py` reproduces BAS-O12's piece-value read
            exactly. Its 16 tests (`test_piece_values.py`) also cover
            the driver's seed check, stage commands and loss parsing; it
            runs in CI's texel job.
          - **Dry runs** (`tools/results/c282/`):
            - The first stopped at the tuner's travel guard on a
              five-epoch schedule, as designed.
            - The second found a bug: the seed was written without its
              `# root` line, so the bake refused the fitted vector as
              stale. Fixed, with a test.
            - The third ran seed, K, king safety, bake, rebuild and verify
              end to end, and restored the header.
          - **For C.2.8.3's bake:** on fitted weights `test_eval` fails
            the four PST orientation tests, as HANDBOOK step 6 foresees,
            and the pawnless mate-drive test. Their fixtures move in
            their own commits (C-70).
        - [x] **C.2.8.3** `[V]` Fits of A and B and the fit report
          The maintainer's runs. The report reads every registered item
          before any bake.
          - The fits run one after the other, never together: both build
            in `build/fit` and bake into `src/eval_params.h`.
          - B takes A's K from A's `run.json`.
          - **Run 1 returned, 2026-11-23**
            (`analysis/c283_fit_report_2026-11-23.md`, BAS-O13): not
            baked.
            - A's loss is 0.09648, 2.67% below the header's. The floors
              pass. BAS-E60's condition fails.
            - But `fit.py`'s surface froze 242 slots on support counted
              in a 30,000-row sample. Only 4 are rare at full scale. 204
              are PST entries, stranded at the seed while their tables
              re-levelled by up to 215 cp.
            - The registration's Amendment 1 (`R2`) chooses the support
              rule before a re-run. The report recommends counting it on
              the whole train split, with a guard against frozen entries
              in a free-level table.
          - **Amendment 1 recorded, 2026-11-23** (the registration's §6;
            BAS-O13, BAS-E60; reads in `tools/results/c283/amend1/`).
            The maintainer accepted it in full the same day:
            - Support is counted on every row the linear stages read,
              at 200 rows. By count, two slots stay frozen
              (`MobQMg[0]` and `MobQEg[0]`, 10 rows each), and no PST
              entry has under 1,457 rows.
            - A fit stops when a support-frozen slot sits in an
              unanchored table that still has a free member.
            - The rank sample grows to 300,000 rows. At 30,000 it fails
              under the new freeze.
            - Both fits are re-run. BAS-E60's condition is re-read on
              run 2, with a label-specific part added: on run 1 the
              labels' own share was 1.3e-6 of a 2.0e-5 difference.
            - BAS-O13's predictions stand. `bench 13` and the condition
              are restated after run 1.
            - Findings for the mobility cluster and the material
              cluster are in their entries (the registration's §6.7).
            - Next: the tool change to C.2.8.2's driver (§6.5) with its
              dry run on corpus A, then run 2.
          - **The tool change, 2026-11-23** (`9570c36`; §6.5, tools only,
            bench unchanged):
            - The tuner's `--count-support` streams each slot's support
              on `--dump-coeffs`'s rows; `--dump-coeffs` is byte-identical.
            - `surface.py rank` freezes on that count over the linear
              stages' rows. It reads the rank on a 300,000-row sample. It
              fails on a free slot absent from the sample and on a frozen
              slot in a free-level table.
            - `fit.py` records the freeze in `run.json`.
            - Tests fail on each known-bad input, including the old tuner.
            - **Dry run** (`tools/results/c283/amend1/dry/`):
              - Corpus A: all 1,159 counts equal `support_full.json`'s on
                3,547,892 rows. Only `MobQMg[0]` and `MobQEg[0]` freeze,
                1,023 slots stay free, and the rank is 1,013 of 1,013.
              - Cut to 30,000 support rows, it reproduces run 1's 242
                slots and counts. The guard then fails on 155 of them
                (153 non-king PST entries and both `ConnectedRank`) and
                writes no manifest.
              - Corpus B's manifest is A's, byte for byte (`E37D5983…`).
            - Next: run 2, the maintainer's two fits.
          - **Run 2 returned, 2026-11-23**
            (`analysis/c283_run2_report_2026-11-23.md`): every control
            and every Amendment 1 prediction holds, and A's loss is
            0.09612. But A2's baked build converts KR-K 23 of 24 at
            60,000 nodes, so nothing is baked.
          - **Amendment 2 recorded, 2026-11-23** (the registration's §7):
            - A bare king against a queen or rook scores its tier plus
              the winner's ordinary evaluation. So tables fitted on
              other rows are that ending's only mating gradient.
            - The two top tiers take the donor's bare-king form instead:
              frozen material, an edge push and a king pull, with no
              fitted slot.
            - Qualified on the conversion grid (KQ-K and KR-K, 40,000
              to 120,000 nodes, on the repaired head and with A2 baked),
              with no game gate of its own. A2 is not re-fitted.
            - Then the pre-bake reads again, the bake, and C.2.8.4 with
              the repair in both arms.
          - **Amendment 2 done, 2026-11-23:**
            - The repair is `cd4e453`, with its span nit fixed in
              `6be9af7`. The fingerprint is now 4,392,040.
            - BAS-D24: the grid passes, 24/24 for KQ-K and KR-K at every
              budget on both builds, and the ply lists are identical.
            - Two fixtures moved, as the review decided (C-70):
              - the blockade test is scale-free (`2d3622b`);
              - the PeSTO orientation test is retired (`b912051`), with
                orientation now checked at the import in `seed_sf16.py`
                (`7796153`).
            - Over the repair, A2's pre-bake reads all pass: the four
              floors at 60,000 nodes, `test_eval`, `test_endgames`,
              `test_search` and `--verify`. Bench 3,359,329.
            - The review found the fit sound. Three fitted-constant
              findings go to their owners:
              - `Trapped*` on its 0 clamp: the material and pieces
                cluster (C.5 since 2026-11-25), C-48;
              - the three threat terms collapsed to 57: the mobility
                and threats cluster (C.6 since 2026-11-25);
              - `QueenInfiltrationEg` at 186: the material and pieces
                cluster, and first in line if the refit is retried.
            - Baked as `cbc3676`, then reverted as `7f92ec9` while
              C.2.8.4's veto is ruled on (below), and restored on its
              acceptance (`92a4c1a`).
        - [x] **C.2.8.4** `[V]` The registered gates
          Clean-PGO arms under identical conditions, and the maintainer's
          runs. Fit loss explains; only the SPRT decides.
          - **Run 2026-11-23** (launcher `tools/run_c284_gates.py`;
            BAS-E59):
            - The arms: the refit `cbc3676` against the repaired head
              `b912051`.
            - **BAS-E59 accepts H1** at 1,110 games: **+85.2 ± 14.9 Elo**,
              zero faults.
            - **The bracket veto blocks:** 8 new clean-win discards
              against the repaired head, in KBPP-KB, KNN-KP, KP-KP and
              KRP-KR. Conversion is 434/445/458 of 480 against
              443/448/460.
          - **The review ruled** (the registration's §8) that the veto
            binds. The SPRT is not weighed against it, and the refit is
            not accepted on this evidence.
            - §8's B2 read: B2 has 7 vetoes and repeats 3 of A2's 8. A2
              vetoes B2 3 times, and B2 vetoes A2 6 times: "between",
              with no verdict.
            - So acceptance is the maintainer's ruling: keep a +85 Elo
              refit against the veto, or repair the endgames first.
          - **BAS-E61**, the refit against the head before the top-tier
            repair (`901e519`): H1 at 1,164 games, **+74.6 ± 13.3 Elo**.
            The gain holds against either head.
          - **Accepted 2026-11-24 (maintainer):** the refit is kept over
            the bracket veto. The endgame deficit is repaired in its own
            steps as planned (C.7 holds the three families). Done the
            same day:
            - the bake restored (`92a4c1a`) and the tracked surface
              manifest rebuilt for it (`7c1801c`);
            - the fingerprint moved to 3,359,329 (`38487c5`);
            - the PeSTO disclosure retired after the similarity screen
              (`68dc18a`, `analysis/c28_pst_provenance_2026-11-24.md`):
              no table match against any donor, and 4 of 736 entries
              equal to PeSTO's;
            - two fixtures on fitted weights moved (C-70, `5381de1`): the
              WAC floor to 168, and the verdict golden, where KR-KR's
              scale verdict is now wrong 350 times against 397.
          - **WAC reads lower on the refit** at every fixed depth, in about
            the same time: 181/191/212/217 at depths 6 to 9 against the
            head's 186/206/216/226. WAC decides nothing; reported to the
            maintainer, not routed.
          - **Open, not scheduled:** the review recommends giving the
            bracket veto a calibrated tolerance before C.3's gate, since
            two equal fits veto each other 3 and 6 times. The maintainer
            decides whether and where.
    - [x] **C.2.9** `[I1]` Family screen and matched-ablation tools
      Added 2026-11-18 (maintainer, after C.0). Two instruments the
      family order and every cluster read; neither changes a served
      score.
      - **The family screen**, committed: C.0's prototype
        (`tools/results/c0-screen/c0_reanalysis.py` and its dataset
        builder) as a tool with tests. It reads both ways: a donor
        family's rows added to Basilisk's score, and a Basilisk family's
        added to the donor's, by phase band, for both donor evaluations.
        Override and specialised-endgame rows are separated, K is fitted
        per evaluator, and intervals are a bootstrap over games. A
        known-bad input fails it.
      - **The matched evaluation ablation**: one mask that switches the
        same family off in Basilisk's evaluation and in the oracle's own,
        compiled away in a shipped build (PROCESS, *Matched ablation*).
        The meter with a family off on both sides, taken from the meter
        with nothing off, is the Elo of the deficit that family explains,
        at the layer that decides. Every bit is proved live at fixed
        depth before any game, and the release `bench 13` stays
        4,777,412.
      - Families for the mask: space, mobility, threats, king danger with
        shelter and storm, pawn structure, passers, the piece terms.
        C.1's modules are its switch points on Basilisk's side.
      - **The screen, done 2026-11-23** (`2a284cc`):
        - `tools/diag/family_screen.py` (`collect`, `screen`) with
          `test_family_screen.py`, run in CI with the tuner.
        - The tuner gains `--dump-row-lines` for the join.
        - It refuses a misaligned join, a donor table that does not sum,
          and labels that are not results.
        - A 3,000-row smoke on corpus A reads both ways for both donors.
      - **The matched ablation mask, done 2026-11-24:**
        - Basilisk's side is the Ablate flavour, rebuilt from `4d2742f`'s
          build plumbing (`de68d5a`): `EvalAblationMask`, bits in
          `src/eval.h`'s order: 0 space, 1 mobility, 2 threats, 3 king
          danger with shelter and storm, 4 pawn structure, 5 passers,
          6 the piece terms.
          - A bit removes its family wherever it enters the score, the
            lazy checkpoint included. The pawn cache's key carries the
            mask, so no entry answers for another mask; `test_eval`'s
            ablation section fails without it (checked).
          - Never masked: material, the tables, the imbalance, the
            bishop pair (Stockfish's is in its imbalance), the tempo and
            the endgame knowledge.
          - CI builds the flavour and runs `test_uci_protocol` and
            `test_eval` on it.
        - The oracle's side is `tools/oracle/oracle-eval-ablate.patch`
          (`780198e`), applied by `build_oracle.ps1 -ExtraPatch`.
          - The same option sets both evaluations' masks. Stockfish's bit
            drops its family's score and keeps every computation, so the
            attack maps and hash tables stay stock.
          - Basilisk's evaluator is compiled as its Ablate flavour, and
            the option aborts if Basilisk's side refuses the mask.
        - **Proved:**
          - Mask 0 is stock: bench 3,359,329 on the Ablate flavour. The
            oracle benches 1,017,388 with Basilisk's evaluation and
            1,030,221 with Stockfish's, the same as the unpatched oracle
            on the same `src/`.
          - Every bit is live on all three evaluations
            (`ablation_liveness.py --mask eval`, depth 9, 8 positions):
            it changes nodes on all 8 positions. Basilisk standalone:
            ×0.84 to ×1.56. The oracle with Basilisk's evaluation: ×1.02
            to ×1.33. With Stockfish's: ×0.77 to ×1.25.
          - Evidence: `tools/results/c29-mask/`.
        - **The meter's binary:**
          `tools/test_engines/oracle-c29-e56f1e1-evalablate.exe`
          (`CAA8B70B…`, `src/` of `e56f1e1`). Its arms are `Use Basilisk
          HCE` true and false at one `EvalAblationMask`.
          - A family's meter is E(0) − E(mask): the arms at mask 0, less
            the arms with that family's bit set on both.
        - Not exercised: `build_test.ps1 -Flavor Ablate`, whose PGO build
          C.2.10 does not need; the meter runs in the oracle.
    - [x] **C.2.10** `[R3]` Family order from games and screens
      Added 2026-11-18 (maintainer, after C.0): the decision point before
      the first family. On the refitted head it registers, prepares and
      reads:
      - the matched evaluation ablation by family, about 3,000 games a
        family at `3+0.03`, maintainer-run;
      - the family screen both ways;
      - the frontier pair's baseline: the head against Stockfish 16 HCE,
        3,000 games.
      Then it orders the families by the Elo each explains, with the
      screen as the tiebreak for what games cannot measure (under about
      10 Elo), and renumbers C.3 to C.6 if the order changes. A family
      that explains nothing in games and adds nothing in the screen
      closes `NO_CHANGE` without a port (C-110).
      C.0's prior, scored here: mobility and space first, king safety
      second (its packet §7.3). The constraints stay: the phase, the
      endgame king danger and tempo change together, and the endgame
      cluster follows pawns and pieces.
      A second prior, from outside (2026-11-21, Rarog's RAR-O10,
      `analysis/rarog_import_2026-11-21.md` §2.4; a reference loss, not
      Elo):
      - Scored alone against Stockfish 19's expected score on 10,000
        game positions, Basilisk's evaluation on 2026-11-14 read skill
        0.1646, against Stockfish 11's classical evaluation 0.2054 and
        Rarog's head 0.1769.
      - Its moves at 100 ms lose clearly more than Rarog's only with 45
        or more points of non-pawn material.
      - That agrees with C.0's phase 18 to 24.
      **Registered 2026-11-24**
      (`analysis/c210_family_order_2026-11-24.md`; BAS-O14, BAS-M24,
      BAS-E62). No game is played and no screen is read.
      - The meter: E(mask) on the C.2.9 oracle, M(mask) = E(0) − E(mask).
        Eleven reads of 3,000 games on one seed: masks 0, 7, 8, 48, 64,
        127, 2, 1, 4, 16 and 32.
      - Three reads are added to the seven families: the cluster masks 7
        (C.3) and 48 (C.5), and 127 for the families' joint share.
        Clusters are compared by their own reads, never by a sum.
      - The rule, frozen: the present order stands unless a later
        cluster's E is lower than an earlier one's by more than the
        difference's 95% interval, about 15 Elo. The screen breaks a
        tie only when its intervals separate.
      - If the seven families together explain under 40 Elo, the order
        returns to the maintainer before C.3 opens.
      - Known confounds: mobility's meter carries piece-value level, and
        Basilisk's arm gains speed under a mask. Both raise a meter.
      - Predicted: E(0) +160, the frontier pair −140, C.3's meter +55
        and the largest (0.7), mobility's above space's (0.75).
      - Owed: the launcher `tools/run_c210_reads.py` and the head's
        build (implementation), then the games (maintainer), about nine
        hours, and the screen.
      **The screen, read 2026-11-24** (BAS-E62; the packet's §8.1):
      - On the refitted head the donor's space adds 3.4 and its
        mobility 2.1 at phase 18–24, both including zero. BAS-E58
        read 47.0 and 29.6 on the 1.11.0 head.
      - King safety is the largest donor gain: 16.2 at phase 18–24,
        29.4 at 10–17, 12.1 over all rows, clear of space and mobility.
      - So the tiebreak points at C.4, and C-110's static half holds
        for space, mobility and pawn structure. The games decide.
      - Three of four screen predictions missed. A control with the
        header before the refit is owed.
      - Prepared 2026-11-24: `tools/run_c210_reads.py`,
        `tools/diag/c210_table.py` and the head's build. The games
        are the maintainer's.
      **The screen's control, 2026-11-24** (BAS-E62's CONTROL; the
      packet's §8.2 and §9):
      - The read above is of corpus A only. On C.0's gate-game rows the
        donor's space adds 40.4 [32.7, 49.1] to the head's score at
        phase 18–24 (50.8 before the refit), and mobility 13.4 (29.6).
      - On corpus A's rows space read 5.7 before the refit too. The
        refit did not remove it; the sample never held it.
      - Amendment 1, before any game: the screen breaks a tie only when
        both samples agree, and C-110's static half needs both. On C.3
        against C.4 they disagree, so a tie in games keeps C.3 first.
      **Closed 2026-11-25** (BAS-O14, BAS-M24; the packet's §10 to §12).
      Twelve reads of 3,000 games, no fault and no time loss.
      - In one search the donor's evaluation leads Basilisk's by +146.2
        [+137.5, +155.1]. The head trails Stockfish 16 HCE by 120.0
        [111.0, 129.2]; 1.11.0 trailed by 193 (BAS-M18).
      - Cluster meters: pawns and passers +94.9, king +43.1, piece
        terms −37.7, mobility, space and threats −52.0, each ± 13.
      - Single meters: passers +125.4, king +43.1, space −2.1, threats
        −9.9, pawn structure −33.2, piece terms −37.7, mobility −46.0.
      - All seven together: +162.8 ± 13.9. The unmasked remainder holds
        no deficit (E(127) −16.6, flagged for speed). The singles sum
        to +40, so a meter ranks and does not size a port.
      - The rule, applied: pawns and passers move ahead of king safety
        by 51.8 ± 14.2 and both ahead of the rest. The piece terms move
        ahead of mobility, space and threats by 14.2 against an
        interval of 14.1.
      - **The order: pawns and passers, king safety, material and
        pieces, mobility and space.** C.3, C.5 and C.6 are renumbered
        (HISTORY); C.4 keeps its number.
      - C-110 closes the donor's queen terms. No whole family meets it.
      - C.0's prior is refuted: mobility and space are last, and the
        screen's leading family reads −2.1 ± 12.5 in games.
      - Scored: 5 of 12 reads inside their 80% bands; all four ranked
        predictions missed.
- [x] **C.3** Evaluation rebuilt whole in the donor's form
  Decided 2026-11-25 (maintainer, after C.2.10): the evaluation is
  rebuilt as one unit in classical Stockfish's form, as Phase B rebuilt
  the search, and not family by family. The king-safety, material and
  mobility clusters (C.4, C.5 and C.6 until then) are built here as
  parts of it. They were retired on 2026-11-25; their entries, with
  their inputs, are in HISTORY (*Retired and renumbered in Phase C*).
  Amended the same day (maintainer): a transplant read comes first
  (C.3.1, BAS-O15). If the donor's passers and king safety on
  Basilisk's own base come within 30 Elo of the donor's whole
  evaluation, only those two are ported and the other families stay
  Basilisk's.
  Read 2026-11-25 (BAS-O15): the instrument's control failed, so the
  reads show nothing about a two-family port, and the rebuild stays
  whole.
  Why:
  - The donor's lead belongs to the whole. All seven families together
    explain +162.8, the single meters sum to +39.6, and five of seven
    read zero or below alone (BAS-O14).
  - The screen did not rank the families as games did: space led it
    on gate games and reads −2.1 in play (BAS-E62, BAS-O14).
    Corrected 2026-11-25: an earlier line here said results cannot
    see the donor's lead. That holds only with one K over all rows.
    With K linear in the phase `9587eeeb`'s score predicts better
    than Basilisk's by 44.3e-4 nats on gate games and 43.7e-4 on
    corpus A (`analysis/c31_transplant_2026-11-25.md` §10).
  - The families are coupled through the tables. Basilisk's endgame
    pawn table pays up to 270 on the seventh rank, so a passer port
    beside it has its credit split again by the fit.
  - A whole port has the strongest check there is: its value equal to
    the reference's on a fixture of game positions, before any value
    moves.
  - A prior, from Rarog (its RAR-O13 and its E.4.0 packet, read at
    `a8c116e` on the maintainer's request): `sf_16`'s evaluation in
    Rarog's own search read +80.3 ± 10.8 at equal nodes and +68.8 ± 10.4
    on the clock, `sf_11`'s +74.4 ± 10.6, and Rarog rebuilds whole.
  Not played: the donor's evaluation in Basilisk's own search. The
  port's first read, at converted seed values, gives that number
  (C.3.4), so no separate instrument is built for it.
  Settled 2026-11-25 (maintainer, on `analysis/c31_rebuild_2026-11-25.md`):
  - the reference is `9587eeeb`'s evaluation (the phase's preamble);
  - the fit is static, on Basilisk's own games, in two arms: one K,
    and K by phase. Each is read against the seed arm (C.3.4);
  - Basilisk keeps its endgame layer and rule-50 curve from the
    start;
  - the README's acknowledgements name the form at the flip, and the
    constants no static fit prices stay disclosed until the SPSA leaf
    fits them.
  The maintainer's directions (2026-11-25):
  - Ideas and forms may come from the donor. Every value and constant
    that ships is fitted as Basilisk's own. The donor's values appear
    only in a diagnostic arm.
  - Basilisk's own forms return where they give strength, each on its
    own read (C.3.6). Mobility, pawn structure and the piece terms
    read above the donor's in BAS-O14.
  **Pawns and passers, the first family by C.2.10's order:**
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
  It was C.5 until 2026-11-25, and C.6 until 2026-11-05.
  C.2.10 inputs (2026-11-25, BAS-O14; the packet's §10 to §12):
  - With the passer terms removed from both evaluations the donor's
    lead falls from +146.2 to +20.9: a meter of +125.4 ± 12.9. Pawn
    structure reads −33.2 ± 13.1, and the two together +94.9 ± 13.4.
  - So the deficit is the passers'. It is not in the structure terms,
    which are worth more to Basilisk than the donor's are to the donor.
  - A meter is the difference of what each family is worth to its own
    evaluation; it does not size a port. Bit 5 leaves the tables in,
    and Basilisk's endgame pawn table pays 152 to 270 on the seventh
    rank and 63 to 186 on the sixth (E4 above), so part of its passer
    value survives the mask.
  - Static, on corpus A's validation rows, after the fact
    (`tools/results/c210-close/own_ablation.py`): removing its own
    passer terms costs the donor 379.9 and Basilisk 103.1 (1e-4 nats),
    while the donor's term adds 5.4 to Basilisk's whole score, and 14.0
    at phase 0–9 (BAS-E62).
  - Three readings stay open: the donor's form is worth more;
    Basilisk's passer value sits in a table that cannot see a blocker
    or a king; or Basilisk's own terms cost it in play. The rebuild
    takes the donor's form and its tables together, so it does not wait
    on them.
    - [x] **C.3.1** `[R3]` Rebuild research and handoff
      It opens with the transplant read (BAS-O15,
      `analysis/c31_transplant_2026-11-25.md`, registered 2026-11-25):
      the donor's passers and king safety on Basilisk's own base, in
      the oracle, against Basilisk's evaluation and against the
      donor's. The instrument and launcher are implementation's; the
      three reads are the maintainer's.
      Then the packet, for the whole rebuild or for the two families:
      - the reference evaluation and its scope: terms, tables,
        imbalance, winnable, scale factors, the lazy cut and tempo;
      - the scale conversion, and the known-win and mate tiers;
      - the parity fixture;
      - what Basilisk keeps;
      - the fit's method and the gate's registration.
      It proposes the PLAN edits that retire C.4 to C.6 and resize C.7,
      for the maintainer's sign-off. It ends in a frozen handoff.
      Handoff frozen 2026-11-25: `analysis/c31_rebuild_2026-11-25.md`.
      The packet's §13 was signed and applied on 2026-11-25: the
      reference, the retirements and the renumbering, the endgame
      scaling leaf and the SPSA leaf's scope.
      **Closed 2026-11-25** (BAS-O15; the transplant packet's result,
      the rebuild packet's Amendments 1 and 2).
      - The transplant, 3,000 games a read, no fault: its gain over
        Basilisk's evaluation −1.6 [−10.7, +7.5]; the donor's over it
        +122.1 [+113.1, +131.3]; the donor's over the control, all
        seven families transplanted, +172.9 [+162.8, +183.4].
      - The control failed and the instrument's falsifier fired, so
        the reads show nothing about a two-family port. All three
        predictions missed.
      - The rebuild stays whole on the maintainer's decision and
        BAS-O14. C.3.2's handoff is frozen.
    - [x] **C.3.2** `[I2]` The port behind the switch, at parity
      Stage 4: the whole evaluation behind one switch, in Basilisk's own
      structure, equal to the reference on the fixture before any value
      changes. The off arm stays at the accepted fingerprint at every
      commit.
      Handoff (2026-11-25): `analysis/c31_rebuild_2026-11-25.md` §5
      to §10. The reference is `9587eeeb`'s evaluation; Basilisk
      keeps its endgame layer and rule-50 curve; parity is exact
      integers against a new oracle reference command; the work
      lands in eight tickets, the tuner's model of the new arm last.
      **Closed 2026-11-25** (the rebuild packet's *Implementation
      record*; Amendments 1 and 3 to 6 rule its three returns):
      - `EVAL_REBUILD` compiles `src/eval_rb*.h`. The off arm stays at
        `bench 13` = 3,359,329 at every commit; the new arm, at the
        seed and S = 620, reads 3,717,775. Both arms pass all 59
        suites, and 58 of 58 under the sanitizers.
      - At the seed, 35 fields equal the reference on the 13,217-row
        fixture, wherever they are compared; 91.7% of its rows are
        layer-silent.
      - The tuner's model is exact on corpus A's validation rows, and
        the played hash equals the served hash. S is set by §7: K
        2.0027 against the accepted 2.0043. The surface has 1,262
        free linear slots and 59 knobs, and `rank` is full.
      - An oracle built from the new arm reads its value exactly.
    - [x] **C.3.3** `[R2]` Independent review
      A separate session rebuilds both arms, reproduces the fingerprints
      and the donor fixture, and checks the code against the handoff
      clause by clause. Its acceptance is recorded before the refit.
      **Closed 2026-11-25** (the rebuild packet's *Review*): accepted,
      with named follow-ups. No defect in either arm's engine code.
      - From a clean clone both arms reproduce: `bench 13` 3,359,329
        and 3,717,775, 59 of 59 suites each, the fixture's hash from
        the pinned producer, parity and its failure on one changed
        seed, `--verify` and the hashes on corpus A's validation
        rows, and the surface check.
      - The new arm is the packet's §6 as amended, clause by clause,
        and its families are the reference's. The off arm's changes
        are behaviour-neutral.
      - The record's played hash was the one at S = 628. At S = 620
        it is e87063a94a5a10bc, equal to the served hash.
      - Follow-ups, none before the seed read:
        - the structure constants and S are outside the surface
          manifest, so C.3.5's audit and C.6 take them from the
          record;
        - the fixture reaches the clamp on no row;
        - the Diag counters of the pawn cache, the lazy exits and the
          clamp report nothing on the new arm, and the knobs' bounds
          are implementation's: both are C.3.4's to settle.
    - [x] **C.3.4** `[I2]` Seed-value read and fit
      The port at converted seed values against the head, with its speed
      and tree reads: the donor's evaluation in Basilisk's own search.
      Then the fit by C.3.1's method, the fitted-constant analysis, the
      bake and the PGO builds; the lazy margin block (C-98); the gate
      prepared and dry-run.
      Registered 2026-11-25 (the packet's §11): the seed read is
      BAS-M25, +85 predicted, and below +30 the leaf returns to
      research before any fit. The fit has two arms, one K and K by
      phase, each read against the seed arm; they are registered
      here before any fitted game.
      Added 2026-11-25 (maintainer, after C.3.3's review; the rebuild
      packet's Amendment 7). The class rose from `V` to `I2`: the
      leaf now holds an engine ticket and a tuner mode.
      - Preparation, before any read: the new arm's diagnostics
        (the pawn cache's counters, the audit of both lazy exits,
        the endgame census, the board trace's scope, the clamp
        count printed and reset) and a constructed-position test of
        the clamp. Behaviour-neutral: both arms' `bench 13` exact.
      - Beside the seed read, the oracle check (BAS-O16): the
        donor's evaluation against the rebuilt one in `9587eeeb`'s
        search, 3,000 games; +15 predicted, and above +50 the port
        is suspect in play.
      - The pooled NPS read needs an idle machine: the builds are
        prepared and the maintainer is asked first.
      - At the fit's registration research reviews the knobs'
        bounds. A knob that ends on a bound is widened and the fit
        repeated.
      - At the bake each table's mean moves into its material
        slot. The function does not change, and the material values
        that ship are the fit's own.
      Done 2026-11-26 (`5128e6e`, `0374cfd`, `4a9c07d`; `analysis/c31_rebuild_2026-11-25.md`):
      - **Seed reads:** `BAS-M25` **+99.8 [+91.5, +108.3]** (3,000 games,
        pooled NPS −4.47%, tree −2.07% nodes, 0 faults); `BAS-O16`
        +42.4 [+35.1, +49.7] in `9587eeeb`'s search (< +50 return line).
      - **Static fits (`BAS-O17` Run 2, widened knob bounds):** zero
        artificial bound hits on both arms (`one-k` −3.58%, `k-phase`
        −2.03%, `k-phase` lower in every phase band); `24/24`
        conversion floors on both.
      - **Paired fit reads (2,000 games, seed `210006`):** `BAS-E63`
        (`one-k`) **−25.8 [−35.7, −15.9]** rejected; `BAS-E64`
        (`k-phase`) **−9.6 [−19.6, +0.4]** accepted (clears `>= −10`).
      - **Bake & verification:** `k-phase` Run 2 baked into
        `src/eval_rb_params.h` (`5128e6e`; 59/59 CTest on both
        `EVAL_REBUILD=OFF` `3,359,329` and `ON` `3,880,333`; `24/24`
        conversion floors); C-98 lazy audit on `validation.csv` exact
        (37,725 fires, 4 sign flips, 764 crossings, margins held); PGO
        gate binaries (`0374cfd`) and `tools/run_c35_gate.py` (`4a9c07d`;
        `BAS-E65`, `BAS-M26`, `BAS-M27`) dry-run cleanly.
    - [x] **C.3.5** `[V]` Gate, frontier pair and close
      Stages 6 to 8: the maintainer-run SPRT with its `10+0.1` and NPS
      reads (C-99), an equal-node companion if an equal-time gate fails
      (C-95), then the frontier pair, the flip and the ledger rows. The
      bracket and the falsifier are C.3.1's registration.
      At the flip the README's acknowledgements name the evaluation's
      form, and the close lists every slot shipped at its seed value
      (the packet's §8 and §13).
      Added 2026-11-25 (after C.3.3's review): that list covers the
      structure constants too, which are bare numbers in the source.
      They are taken from a committed inventory that a check holds
      to the source.
      Done 2026-11-26 (`e60aa45`, `7eb0047`; `analysis/c31_rebuild_2026-11-25.md`):
      - **Gate (`BAS-E65`, `0374cfd`):** SPRT `[0,3]` (`3+0.03`, seed
        `350001`) accepted H1 at `1,034` games (`517` pairs; `18`
        post-terminal pairs kept outside sample), `LLR = +2.953`,
        **+85.3 [+71.9, +99.0]** (`nElo +138.9 +/- 21.2`, `434-415-185`,
        `0` faults). C.2.7 endgame bracket veto (`21k/64k/190k` of 480):
        candidate **`441 / 448 / 457`** vs base **`434 / 445 / 458`**
        (`7` discards vs base, `5` vs `c274/head`, 0 rule-50/anomaly/malformed);
        the veto binds, and the maintainer ruled on 2026-11-26 to accept
        the candidate over it, as at BAS-E59; the vetoed endings go to
        C.4 (BAS-E65).
      - **LTC & frontier (`BAS-M26`, `BAS-M27`):** `10+0.1` (`1,000` games)
        **+95.1 [+82.3, +108.1]** (`425-417-158`, `0` faults); frontier vs
        Stockfish 16 HCE (`3+0.03`, `3,000` games) **−39.8 [−48.0, −31.6]**
        (`737-1184-1079`, `44.3%`, `0` faults), closing `80.2 Elo` of
        BAS-M24's gap.
      - **Flip, CI & audit:** `EVAL_REBUILD` flipped to default `ON` in
        `CMakeLists.txt` (`e60aa45`; `59/59` CTest on both `ON` `3,880,333`
        and `OFF` `3,359,329`). Five CI Python tests updated so all 12 pass
        on both arms; `tools/diag/rb_structure_constants.tsv` (`44` constants)
        and `tools/diag/check_rb_constants.py` wired into CI (`7eb0047`);
        `README.md`, `AGENTS.md`, `GUIDE.md`, `docs/DESIGN.md`, and the
        complete own-values audit (`156/1,450` equal to seed: `119` fixed,
        `16` support-frozen, `18` free linear, `3` free knobs) recorded.
      - **The retired evaluation removed** (maintainer, 2026-11-26;
        `e58d51f`, `ab0c141`): the `EVAL_REBUILD` switch, the old
        evaluation's seven family headers, its suite and its verdict golden
        file. Behaviour-neutral: `bench 13` 3,880,333 on the release, tune,
        diagnostic and ablation builds, 59/59 CTest, the played hash
        fa8739a3fc205e63 and the tuner's `--verify` unchanged. The fit tools
        now default to `src/eval_rb_params.h` and `surface_rebuild_v1.tsv`.
        C.3.6 takes the displaced forms from `99e6353`, the last commit that
        compiles them.
      - **NPS (BAS-E65, C-99):** pooled, four PGO builds an arm, the
        shipped evaluation at `ab0c141` against the head at `99e6353`:
        **−3.88% [−4.65%, −3.10%]**, inside the predicted band (BAS-M25's
        seed read was −4.47%).
    - [x] **C.3.6** Basilisk's own forms read back in
      Added 2026-11-25 (maintainer). After the gate, each Basilisk
      form the port displaced is read against the donor's on the
      accepted port, behind its switch: one registered read each,
      both arms fitted, since a read on one fit favours the form the
      fit was made with. A form that gains returns. One that does not
      stays out with a retry trigger. C.3.1's handoff lists the forms.
      The list is the packet's §14. The retired clusters' audit inputs
      (HISTORY) are re-read only for a form that returns.
      Split 2026-11-26 (maintainer, on C.3.6.1's recommendation): a
      static screen of every form first, then a `[0,3]` SPRT for each
      form that passes. The packet is `analysis/c36_own_forms_2026-11-26.md`.
        - [x] **C.3.6.1** `[R2]` Inventory, screen design and registration
          **Closed 2026-11-26.** The forms compared at `99e6353`
          against the shipped evaluation (the packet's §2 and §4):
          - §14's order came from BAS-O14's meters, which measure what a
            family is worth to its own evaluation, not what Basilisk's
            form would add to the donor's. No item has a prior from them.
          - Mobility has almost nothing left: the same tables, own values
            since C.3.4, and the same area apart from the pinned set.
          - Twenty forms are read, one bit each: thirteen replace a donor
            form, seven are added beside it. The rest are the same form or
            contained in the donor's, each listed with its reason.
          - The screen, BAS-O18: each form fitted by C.3.4's method
            against a null refit of the shipped vector, at C.3.4's frozen
            K, two positive controls first. A form passes at a validation
            gain of 2.0e-5 or more with its bootstrap over games above 0.
          - Passing forms take SPRTs one at a time, largest gain first,
            each against the evaluation then shipped and re-screened
            after any acceptance.
        - [x] **C.3.6.2** `[I2]` The forms behind one switch, and the screen's tools
          The packet's §8: `EVAL_OWN_FORMS`, a bit mask in §4's order,
          default 0. Mask 0 keeps `bench 13` 3,880,333 and the played
          hash at every commit. Each bit is modelled exactly by the
          tuner and is colour symmetric, and an added bit at seed 0 is
          the shipped evaluation. Then the screen tool and the frozen
          launcher for BAS-O18's 24 arms, in Python, dry-run.
          **Closed 2026-11-26** (`17d6d66`, `ea2834c`, `05568ce`,
          `a270c05`, `b2009cc`, `7566d9e`):
          - Nineteen forms behind `EVAL_OWN_FORMS`, each with its own
            slots, appended after `Tempo`. Bit 15 is reserved
            (R5: `ROOK_SEVENTH` is the 64-square rook table's seventh
            rank), and a mask that sets it fails to compile.
          - Mask 0: `bench 13` 3,880,333, played hash fa8739a3fc205e63,
            59/59 CTest, `--verify` exact on 197,277 rows, CI's Python
            tests green.
          - Every bit alone (`run_c36_screen.py check`): `--verify`
            exact; the six added forms serve mask 0's bench and hash at
            seed 0 and move it at a nonzero value; `test_eval` passes
            at every mask but the opposite-bishop arm's ruled failure
            (the classical arm's parity coverage).
          - The tuner models the piece-count phase and the middlegame
            tempo; the surface classes slots by mask; `fit.py` gained
            `--own-forms`, `--no-bake` and `--fix-groups`;
            `form_screen.py` reproduces the tuner's loss to 2e-10.
        - [x] **C.3.6.3** `[R2]` Independent review
          A separate session reproduces mask 0, every bit's seed
          identity and `--verify`, and checks each form against the
          packet's §4 and `99e6353`'s code. Its acceptance is recorded
          before the screen runs.
          **Accepted 2026-11-26** by research in a separate session, at
          `f011260`. Reproduced from its own clone:
          - mask 0 at `bench 13` 3,880,333 and played hash
            fa8739a3fc205e63; every bit's bench, `--verify` on 197,277
            rows and played hash, equal to the implementer's on all 20
            masks;
          - the six added forms identical to mask 0 at seed 0, and live
            at a nonzero value;
          - `test_eval` at every mask, with the opposite-bishop arm's
            single ruled failure;
          - the release build's served hash equal to the tuner's played
            hash at every mask;
          - colour symmetry at nonzero values: 0 of 197,277 rows
            asymmetric at every mask.
          Each form was read against `99e6353` and R1 to R5. Rulings:
          R5 (`ROOK_SEVENTH` withdrawn); the opposite-bishop arm's
          parity-coverage failure expected; the lazy census's
          precondition fixed in `b2009cc`; R6 (`PAWN_THREAT` and
          `TRAPPED_BISHOP` barely active on quiet rows).
        - [x] **C.3.6.4** `[V]` The screens and the first refit
          Amended 2026-11-26 (the packet's Amendment 1; maintainer):
          C.5.2's first cycle is pulled forward. Corpus C, corpus A's
          recipe with the shipped engine; BAS-O18 on corpus A; the refit
          R on corpus C (BAS-O19); BAS-O18's arms again on C from R
          (BAS-O20). All fits end before any game.
          Reviewed 2026-11-27 (research; the packet's Amendment 2): the
          three readings stand. For the close:
          - R8 froze 59 knobs, 36 of them the material imbalance;
          - the null N2 passed the registered fallback at 9.68e-6;
          - the index lines of BAS-O18, BAS-O19 and BAS-O20 still read
            "registered".
          **Closed 2026-11-27** (`704b957`, `879863a`, `1f07c8e`; BAS-O18,
          BAS-O19, BAS-O20):
          - **Corpus C** (BAS-O19): corpus A's recipe with the shipped
            PGO PEXT build, ten slices of 100,000 games, 0 faults. Its
            audit has A's kind, its validation starts equal A's, and
            `--verify` is exact on all 197,304 validation rows.
          - **The screen on A** (BAS-O18, label `A3`: R8's
            `linear,polish` from the shipped vector). Gate 1 3.59e-6
            against 1.0e-5; both controls above δ. One marginal pass,
            b00 `OWN_PHASE` +5.78e-5 [+1.4e-7, +1.19e-4]. Before R8 the
            null N2 had passed the registered fallback at 9.68e-6.
          - **The refit R** (BAS-O19): 1,176 linear slots and the nine
            material values moved; loss on C's validation −1.19e-4
            (−0.134%); piece values within ±3% in every band; no fitted
            knob on a bound; played hash fb1ed67763bb0dac through the
            bake; `bench 13` 3,525,148 (`1f07c8e`). R8 froze 59 knobs:
            22 of king danger, 36 of the material imbalance and
            `SpaceWeight`.
          - **The screen on C** (BAS-O20, from R). Gate 1 0; both
            controls above δ_C 1.915e-5. One marginal pass, b12
            `PAWN_CLASS` +4.10e-5 [+3.4e-6, +8.1e-5], which reads
            −3.07e-5 on A. Neither pass repeats on the other corpus
            (pooled z 0.1 for b12, 1.1 for b00).
          - **Calibration:** one pass on each corpus against three of
            nineteen predicted, inside the 80% band; the largest Δ came
            from a replacing form, a miss of the 0.6.
        - [x] **C.3.6.5** `[V]` The refit's read, the cluster read and close
          Amended 2026-11-26 (maintainer: "One cluster SPRT is ok"): the
          refit's SPRT (BAS-E66, cap 10,000 pairs), then one SPRT for the
          forms (BAS-E67, cap 20,000 pairs) on the base it fixes: arm P,
          or the best single form. For this run H1 alone keeps a
          candidate, bar a hard failure, which returns to research; the bracket is
          recorded and its positions go to C.4. Every form ends returned, unresolved or out with its
          retry trigger.
          Reviewed 2026-11-27 (research; the packet's Amendment 2):
          - BAS-E66 was killed from outside at 2,772 games and resumed
            from its checkpoint. Its entry's run note explains the one
            `EngineCrash` record.
          - The branch head carries R's values (`bench 13` 3,525,148)
            while GUIDE declares 3,880,333. H1 moves the declaration;
            anything else reverts `1f07c8e`.
          - R loses at phases 0 and 1 and gains everywhere else, so
            the bracket is expected to fire.
          - BAS-E67 was withdrawn by the maintainer on 2026-11-27, before
            any game (its entry). No form returns.
          - BAS-E66 read H1 at 2,581 pairs. Its bracket, its record and
            the declaration of the fingerprint are what this leaf still
            owes.
          - With every form out, the `EVAL_OWN_FORMS` switch, its forms
            and their 44 slots leave at the close, behaviour-neutral
            (the maintainer's rule of 2026-11-26 for an arm no step
            removes). A later read takes a form from the last commit
            that compiles it, named in the close.
          - Two added forms repeat on both corpora below δ,
            `ROOK_CLOSED_FILE` and `ISLANDS`. The close records them
            with the forms' routing.
          **Closed 2026-11-27** (BAS-E66, BAS-E67; the packet's
          Amendments 1 and 2):
          - **The refit is kept.** BAS-E66: H1 at 5,162 games (2,581
            pairs), LLR +2.97, **+14.3 Elo [+8.8, +19.8]**, a
            first-crossing estimate, 0 faults. The bracket at 21k, 64k
            and 190k nodes: candidate 443 / 448 / 457 of 480, base
            441 / 448 / 457. No hard failure. The two-of-three rule fires
            once, a clean-win discard at KR-KN/EG0350, which goes to C.4
            with the fifteen single-budget discards the entry lists.
            Under the run's ruling the veto does not hold R. The bench
            fingerprint is now 3,525,148 (`1f07c8e`), and corpus C is the
            base.
          - **No form returns.** BAS-E67 was withdrawn before any game,
            so no form was read in games. Every form is out:
            - *Worse than the shipped form on both corpora:* `OCB`,
              `TEMPO`, `OUTPOST`, `BAD_BISHOP` and `MINOR_BEHIND_PAWN`.
              The donor's form carries more.
            - *Failed both screens without a clear sign:* `SPACE`,
              `MOBILITY`, `KING_ZONE` (with the king-safety knobs frozen
              by R8, the form it touches most), `ROOK_QUEEN_FILE`,
              `QUEEN_INFILTRATION`, `MAJORITY`, `CONNECTED_ROOKS` and
              `ROOK_BEHIND_PASSER` (+1.90e-5 on A, partly the stop rule's
              doing, and +2.05e-6 on C). Retry: a fit by games (C.6) that
              takes in the slot, the packet's §9.
            - *A marginal pass that did not repeat:* `PHASE` passed on A
              and read −1.03e-5 on C, the base's screen; `PAWN_CLASS`
              passed on C and read −3.07e-5 on A (pooled z 0.1), and its
              SPRT was withdrawn, so it was not read in games. Two such
              passes in 38 reads come by chance about one time in four.
            - *Small and positive on both corpora, below δ each time:*
              `ROOK_CLOSED_FILE` (−30/−3 on A, −29/−8 on C, mg/eg units)
              and `ISLANDS` (+2/+21 and −3/+19); their gain is the
              form's own, not the stop rule's. If a forms read is wanted
              later, these two together are the better-supported
              candidate, under their own registration.
            - *Not readable by this screen* (R6): `PAWN_THREAT` (active
              on 3.6% of rows) and `TRAPPED_BISHOP` (0.06%). Their
              retry is a read at the search's leaves, which C.5.1.1
              plans for threats.
            - `ROOK_SEVENTH` was withdrawn before any read (R5): it is
              contained in the 64-square rook table. Its conditioned
              form is C.5.1.1's Candidate C.
            - Every form that waits goes to Phase H, whose triage reads
              against the network head, so in practice this closes them.
          - **The switch removed** (`8ee292d` engine, `bf7b636` tools;
            the maintainer's rule of 2026-11-26). `EVAL_OWN_FORMS`, the 19
            forms and their 44 slots are gone, and the registry is 1,450
            slots again; `run_c36_screen.py` went with them, and
            `form_screen.py`, `--no-bake` and `--fix-groups` stay.
            Behaviour-neutral: `bench 13` 3,525,148 on the release, tune,
            diagnostic and ablation builds; 59/59 CTest; `--verify` exact
            and the played hash fb1ed67763bb0dac on corpus C's
            validation, as at the head before; CI's Python tests,
            `check_rb_constants.py` and the roadmap, citation and rules
            checks pass. No NPS read was taken. **`1bb866f` is the last
            commit that compiles the forms**; a later read takes a form
            from it.
          - The tracked surface manifest's seeds were still C.3.4's after
            R's bake, which failed `test_surface`; `f0ccd19` rebuilt them
            (the seed column alone, 1,185 slots).
- [ ] **C.4** Endgame handling and winnability
  It was C.7 until 2026-11-25.
  The goal is measured conversion and correct draw recognition where games
  actually go, not coverage of a function list.
  After the families it builds on (C.5 until 2026-11-05): the scalers and
  recognisers act on the pawn and material evaluation, so the endgame
  cluster follows the rebuild, which builds both (C.3 since
  2026-11-25). C.0 orders the clusters by expected value within this
  constraint, and the order is discussed again when reached.
  Its instruments leaf, C.4.1, moved to C.2.7 on 2026-11-11, so the
  cluster starts at C.4.2.
  From C.2.8.4 (2026-11-23, the registration's §8):
  - Both whole-surface refits, A2 and B2, convert 2 to 9 fewer of the
    truth cohort's 480 than the head at 21,000 and 64,000 nodes, closing
    by 190,000. The losses fall in the same three families: KBPP-KB,
    KNN-KP and KRP-KR.
  - The bracket veto's two-of-three rule fires 3 to 6 times between two
    fits of equal quality, so its floor between different evaluations is
    several positions, not the 0 to 1 two heads give.
  Since 2026-11-18 (maintainer, after C.0): the endings hold 16% of the
  same-search deficit and the full board 68 to 69%. When C.3 closes, its
  frontier pair and screen are read against that. If the full board still
  leads the residual, C.5.1's investigation is taken before this cluster,
  on the maintainer's decision. The truth repairs routed here (V3, V4,
  V6) are defects and keep their place in the phase either way.
    - [ ] **C.4.2** `[R3]` Occurrence, classification and ranking
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
      - Code audit 2026-10-27 (§6.3 E7), measured and switched in C.7
        (2026-11-05), where the search surface is refit; B.14 already took
        the draw verdicts' half. The original finding: correction history
        overrides exact recogniser scores — the drawn KPK, KNNK and
        wrong-bishop 0 and the scaled endings are corrected through pawn-,
        minor- and
        non-pawn-keyed tables (`search_kernel_core.cpp:195`, `:508`;
        `search_history_core.cpp:158-174`) whose keys won and drawn
        positions share, so a dead draw can stand at ±X.
      A Rarog prior, 2026-11-26 (maintainer-supplied, Rarog's RAR-E75;
      `analysis/rarog_import_2026-11-26.md` §2), an idea researched here
      and kept or dropped on Basilisk's own read:
      - Finding 1: on Rarog's corpus `sf_16`'s evaluation is under-scaled
        in light endings (the slope in pawn endings is 1.75 times the
        overall slope, with pure opposite bishops 1.68) and over-scaled
        with heavy material (0.63), mostly one slope by phase. The read is
        the same ratio per ending class on the accepted head and corpus
        A's held-out half: minutes, no games. It ranks the classes for
        C.4.3's candidate A. A phase slope the fit has not absorbed is a
        fit question before it is a term.
      An early read on corpus C, 2026-11-27 (research's review of C.3.6;
      `analysis/c36_own_forms_2026-11-26.md`, Amendment 2). It is an
      input, not this leaf's read:
      - The scale left after K by phase differs by ending class at the
        same phase. As the ratio of a class's own K to the fit's:
        pawns only 1.10, one minor 0.95, opposite bishops 0.88, other
        minor against minor 0.82, a rook or two minors against pawns
        0.72, rook against rook 0.93. Finding 1's pawn endings agree in
        sign, and its opposite bishops do not.
      - The refit R gained in the drawish classes and lost in the
        decisive ones (pawns only, one minor, the uneven phase-2
        class), which share the endgame values. BAS-E66's bracket is
        read against that.
      - The scale-factor and complexity numbers are still the donor's
        (`GENERAL_SF_*`, `WINNABLE_*`). This leaf decides whether C.4.3
        fits them by class on quiet rows, where a class's own K is a
        direct statistic, or leaves them to C.6's games.
    - [ ] **C.4.3** `[I2]` Generic winnability and scaling
      Narrowed 2026-11-25 (`analysis/c31_rebuild_2026-11-25.md` §6.8 and §13): the
      donor's winnable step and general scale heuristics arrive with
      the rebuild (C.3). What stays here is the endgame layer's scales
      against those heuristics, and the donor's scaling functions the
      rebuild did not port.
      Held-out tooling first (moved from C.2.7, 2026-11-23; audit D24):
      - halves assigned by FEN hash;
      - a write-once registration naming the deciding half and a runner-up;
      - McNemar on discordant positions;
      - plateaus reported as plateaus;
      - a cohort that chose a winner is spent for selection and stays
        valid as a veto.
      Generic winnability and scaling in the donor's form: pawn-count scaling,
      opposite-bishop scaling, rule-50 inside the scale and a complexity term;
      floored so a misfire discounts rather than throws a win.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - OCB: neutral from four pawns, no passer term, fires with other
        material (C-4; Rarog's `1 + 2 pawns + 10 passers` read +12.7 nElo);
        OCB below seven men (C-125); every scale floored (C-126); rule-50
        damping against the drive weights (C-128); initiative's unfitted
        `win_*` weights (C-47; A.16 removed the term and its slots, so C.4.3
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
      A Rarog prior, 2026-11-26 (`analysis/rarog_import_2026-11-26.md`
      §3), candidate A, researched here on C.4.2's read and implemented
      and tested or dropped:
      - One multiplier on the final score per class, chosen by non-pawn
        material alone: no pieces; one bishop each on opposite colours
        and nothing else; rooks only, if the fit still wants it. Applied
        after the endgame layer and the scale factors, never to mate,
        tablebase or recognised-draw scores.
      - Beside a phase slope, `sf_16`'s residuals were ×1.12, ×1.14 and
        ×0.95 (−0.020% held-out cross-entropy); Basilisk's are read on
        its own evaluation first.
      - The opposite-bishop class waits for C.3.6's read of Basilisk's
        own opposite-bishop scale, which measures the same thing. The
        inputs that pick the class also key the correction-history
        tables (C.4.2's E7).
      - Kill rule: on tablebase positions from Basilisk's own games, the
        scaled evaluation raises drawn-position overclaims by more than
        won positions gain. Then gate.
    - [ ] **C.4.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
      Won-rook-ending only-move precision (archived 6.8.a; BAS-E53): all 68
      one-winning-move nodes studied with a held-out split; search, evaluation
      and knowledge explanations compared; a mechanism or a justified
      no-change. More depth is already refuted.
    - [ ] **C.4.5** `[I2]` Group B families by measured kind
      Group B families by measured kind (archived 6.8.b–e).
      The families: KR-KP, KQ-KRP and KR-KB; bishop-pawn fortresses and
      promotion races, with the KBP-K deficit treated as bishop-pawn
      technique. Deterministic theory, truth and closure cases come before
      fitting, each shown to fail on its defect. The covariant local cluster
      is fitted on training and validation data only.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KR-KN and KR-KB at a flat 4/64, and KRP-KB unscaled (C-129).
      B.9.3's verdicts over its 0.5% line, routed here 2026-11-16
      (`analysis/b93_hard_verdicts_2026-11-16.md` §2, §5, §6):
      - V3: KRP-KR's draw rules 1, 4 and 5 and its other scales under the
        floor call 1.9% to 6.9% of wins drawn. V4: the KRPP-KRP table,
        5.5%. Each rule is narrowed within its own terms until the
        instrument's exhaustive KRP-KR read (KRPP-KRP: the seeded 2M
        sample) puts it at or under 0.5%, or it is dropped.
      - V6: the pawnless scales 4 and 14, one number per class, which
        the tables contradict in 0.8% (KRP-KR) to 41% (KQ-KRN) of
        positions. KBB-KB's scale 4 joined them after V1 (B.9.5): its
        same-coloured draws left the site for the dead rule, and the
        exhaustive read gives 0.507%. KR-KN is worst for the search: it resolves only 80%
        of the sampled misses. A position-dependent rule per class (KR-KN,
        KR-KB, KQ-KR with minors) is read with the same instrument.
      The 2026-11-17 read of V3, V4 and V6
      (`analysis/c75_routed_verdicts_lab_2026-11-17.md`, the instrument's
      `--dump`; nothing shipped):
      - V3 and V4 are discounts floored at 24/64, not verdicts, and a
        depth-8 search resolves 98% to 100% of their errors. V3 is exact
        with the defender to move. With the attacker to move its errors are
        mostly rook skewers: excluding the skewer gives rule 1 0.55%, rule
        5 0.70%, the floor region 0.88%, and every further cut tried gives
        up ten to forty correct draws per error removed.
      - V4 stays at 3.8% without skewers: the table is indexed by the
        leading pawn's rank alone and its errors are positional, so it
        wants a fitted table.
      - V6, exhaustive: KR-KN's scale 4 is wrong in 18.9%, KR-KB's in 6.0%.
        The wins follow the minor's distance from its king, the defending
        king on the edge and the side to move (classical Stockfish's KRKN
        features). The repair is a fitted, position-graded evaluator per
        class; narrowing the one number cannot be exact.
      - Acceptance for a floored site is its search-resolved residual or a
        gate, not B.9.3's 0.5% share of positions the scale touches.
      A Rarog prior, 2026-11-26 (`analysis/rarog_import_2026-11-26.md`
      §4), candidate B, researched with the promotion races and
      implemented and tested or dropped there:
      - A pawn-race term in pawn endings only: the difference, in ranks,
        between each side's nearest unstoppable passer's plies to queen
        (a clear path, and the enemy king outside the square counting
        the move, or its own king guarding the rest of the path), times
        one weight. Neither donor has it.
      - On `sf_16`: −0.087% held-out cross-entropy alone, −0.034% beside
        the pawn-ending scale (about +83 cp a rank), active on 1.1% of
        rows. Fitted beside C.4.3's A, never before. It reads both kings
        and the side to move, so it cannot live in the pawn entry as
        keyed now.
      - Kill rule: on pawn endings of seven men or fewer from Basilisk's
        games, its sign does not agree with Syzygy clearly more often
        than the evaluation without it.
    - [ ] **C.4.6** `[R2]` Lower-yield remainder
      Lower-yield remainder (archived 6.10.a–d): KPs-K and KP-KP first; KQ-KP
      and KQ-KR reconciled across budgets; KR-KN, KNN-KP and KNN-K theory and
      rule 50; implement only what has a measured defect, occurrence and a
      valid closure.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KNN-KP: an additive drive plus a blockade term, never a replacement
        score (C-130); the KXK corner-to-king ratio (C-39); the KBN-K drive's
        speed and a distance-to-mate table option (C-144).
      - B.9.3's V5, kept 2026-11-16: KBN-K's known win is wrong in 0.69%
        of positions, where the bare king forks a minor. Depth 8 resolves
        79% of them. Retry trigger: a KBN-K game lost to a fork or trap in
        any gate's conversion read.
      - A prior for KQ-KP (2026-11-11, `analysis/rarog_import_2026-11-11.md`;
        Rarog's D.6.24): Rarog's fortress rule zeroed 172,256 positions with
        the pawn on the seventh, and 26.7% of the quiet ones are wins for
        the queen, most with her king three or four squares from the
        queening square. A rule here discounts, never zeroes, and is read
        by B.9.3's enumeration before it lands.
    - [ ] **C.4.7** `[V]` Endgame gate and closure
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
- [ ] **C.5** Residual, refit cycles and the speed pass
  It was C.8 until 2026-11-25.
  Since 2026-11-26 (maintainer) the speed pass closes this step (C.5.3
  to C.5.5), after the forms are final and before the fits that read
  the evaluation's cost.
  Reworked 2026-11-18 (maintainer, after C.0): the phase ends on its
  meters, so the residual is investigated before the closing fits.
    - [ ] **C.5.1** Residual investigation and its clusters
      Opened 2026-11-26 (maintainer) so that the clusters the
      investigation opens have numbers that run before the refit
      cycles (C.5.2). The investigation is C.5.1.1. Each cluster it
      opens takes the next numbers from C.5.1.2 on, one leaf for each
      stage of the cluster shape that is its own leaf (research,
      implementation, review, fit, gate), as C.3 did. Opening them is
      a plan change, signed by the maintainer with the handoff. Nothing
      after C.5.1 is renumbered.
        - [ ] **C.5.1.1** `[R3]` Residual investigation
          After the family and endgame clusters. It was C.5.1 until
          2026-11-26. It re-reads the same-search meter on a fresh oracle
          build, the matched ablation by family, the family screen both
          ways and the frontier pair, and reads C.2.2's H-distribution
          result beside them. Its questions:
          - how much of 217.3 is left, and in which family and phase;
          - whether what is left is evaluation at all: speed (C.5.3),
            the search's margins on the new evaluation (C.7), or
            positions a search visits and a game does not (the corpus);
          - which donor forms were screened out and are worth a
            game-level read now, threats first: they have no static case
            and were never measured in play;
          - what information of Basilisk's own a concept residual screen
            (TE-1) can still name.
          It ends by opening further clusters as leaves of C.5.1, from
          C.5.1.2 on and ahead of the refit cycles, each with its
          handoff, or by closing the family work. Stop rule: two
          consecutive units under +5 at their gates, or a frontier pair
          whose interval includes zero. A unit expected under 5 Elo goes
          to Phase H.
          Rarog priors, 2026-11-26 (`analysis/rarog_import_2026-11-26.md`
          §5 to §7), ideas this investigation researches and then opens
          as clusters, routes or drops:
          - Candidate D: a bonus to the attacker for each of a side's
            pieces attacked by a lesser piece beyond the first, since one
            move saves one piece. Unmeasured; 0.3% of quiet rows cannot
            judge it. Its read is the search's own leaves: how often two
            or more occur where the static evaluation decides a node, and
            that evaluation's error against a deeper search there, with
            and without the term. Dropped on negligible activation or an
            error that does not fall.
          - Candidate C: an endgame-only bonus for a rook on its seventh
            while an enemy pawn is on that rank or the enemy king on its
            back rank. On `sf_16` −0.015% held-out cross-entropy, the
            condition unresolved against a plain rook on the seventh.
            Small: the note reads it inside a sweep.
          - Screened without gain on Rarog's instrument, and not
            re-proposed without a new reason: rook mobility counted
            through pieces, an offset per uneven piece configuration, a
            multiplier for each of 52 piece configurations.
    - [ ] **C.5.2** `[V]` Refit cycles
      It was C.8 until 2026-11-18.
      Refit cycles (archived 8.9): regenerate data with the accepted
      head, refit the whole surface with a new untouched test split, and
      gate each cycle; repeat while a cycle accepts; stop at the first
      that does not.
      The first cycle was pulled forward into C.3.6 on 2026-11-26
      (maintainer): corpus C, BAS-O19 and BAS-E66. This leaf starts
      from its result: it begins with the second cycle if BAS-E66 kept
      the refit, and it stops at once if BAS-E66 read H0.
      BAS-E66 kept the refit on 2026-11-27: H1, +14.3 Elo [+8.8, +19.8]
      (a first-crossing estimate). So this leaf begins with the second
      cycle, from R (`bench 13` 3,525,148) on corpus C.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Refit cycles diminish (+11.0, then +6.7 in Rarog), so size from
        earlier cycles' games (C-123); priors (C-68); scores re-dumped
        after each refit (C-69).
      From the first cycle, 2026-11-27 (research's review;
      `analysis/c36_own_forms_2026-11-26.md`, Amendment 2):
      - **What is left for a plain cycle.** A second pass on corpus C
        from R moved 0 of 1,244 slots (BAS-O20's null). Corpus A to C
        was a change of evaluation form, and K moved +5% and +15%; the
        next corpus differs by a few units a slot. So the second cycle
        follows C.4 and C.5.1's form changes, as this order already
        has it, and is not run on unchanged forms.
      - **The knob stage, before the second cycle's method is
        registered.** The first cycle froze its 59 knobs (the packet's
        R8).
        - The 22 king-danger knobs and `SpaceWeight` are C.6's.
        - The 36 imbalance knobs are pinned at C.3.4's values. Their
          retry trigger is the stage on the whole training set
          (`--ks-positions 0`), from small first steps, with a bootstrap
          interval on its validation gain and a null pass that does not
          move. The maintainer rules on the pin here.
      - **A pre-game check.** The refit's loss against the base by
        phase and by ending class, paired, with a bootstrap over games,
        before any game. R read −2.60e-4 [−4.13e-4, −1.08e-4] at phases
        0 and 1. A class that loses with its interval below 0 is
        reported to research before the SPRT. The review's scripts
        (`tools/results/c36/review/`) are the recipe; this leaf makes
        them a tool.
      - A loss delta does not size a cycle's Elo: R moved the loss by
        0.13% and `bench 13` by 9%.
    - [ ] **C.5.3** `[R3]` Speed research: the gap to the donors, split
      It was C.9 until 2026-11-26 and C.12 until 2026-11-25 (evaluation
      throughput, bit-exact; archived 8.12).
      Widened and moved 2026-11-26 (maintainer): a speed pass over the
      whole engine, not the evaluation's hot paths alone, because
      Basilisk is much slower than its donors. It runs as research,
      implementation and a gate (C.5.3 to C.5.5), as B.8's pass did
      (B.8.8, B.8.9).
      - **Why here.** The forms are final after C.5.2, and the fits
        after it read the evaluation's cost: C.6 fits the two lazy
        thresholds, which trade accuracy for the time a skipped
        evaluation saves, and C.7 refits the search on the result.
        C.8's reads at a clock (the frontier field, D.1's gauntlet,
        C.10.1's scores) are then taken at the release's speed.
      - **The measured gap.** In the frontier pair Stockfish 16 HCE
        searches 2.79M nodes a second and 192,355 nodes a move against
        the head's 2.01M and 137,013 (BAS-M27): 39% more. Before the
        rebuild it was 2.76M against 2.00M (BAS-M24), so the gap is
        older than the donor's forms and is not theirs alone. The
        rebuild cost −3.88% [−4.65%, −3.10%] pooled on `bench 13`
        (BAS-E65), beside C.1's −0.71%.
      - **Identity.** Every change in the pass keeps every score and
        the `bench 13` fingerprint of the head it starts from.
      - A lever that changes a score is not built in the pass. This
        leaf reports it: one on the lazy exits is C.6's input, and any
        other goes to H.1's triage.
      This leaf is a packet in `analysis/` and a ranked handoff, with no
      engine change. Its questions:
      - whether the two node counts are the same count (B.8.7 counts as
        modern Stockfish does; `sf_16` is read for its own), and the
        gap in time per node if they are not;
      - how the gap splits: the evaluation's cost per call on one
        fixture of game positions, Basilisk's against the reference's
        (`9587eeeb`) built with the same compiler, flags and PGO; and
        the rest of a node (make, generation, picker, table, search
        bookkeeping) from profiles of both engines on one workload,
        with the oracle's hybrids where they separate the two;
      - what the donors do that Basilisk does not, read in the local
        latest Stockfish first and in the reference for the classical
        evaluation: tables and caches, what is kept incrementally,
        attack maps built once, data layout, build flags;
      - duplicate calculations across a seam: search and evaluator,
        picker and search, make and search;
      - ideas no donor uses, where the profile supports them.
      Inputs carried from the earlier leaf and from B.8:
      - Profile terms, phases and cache paths first.
      - B.8.8's packet (`analysis/b88_speed_packet_2026-11-15.md`): its
        negatives (TB-16) are not re-read without a new reason, and its
        Return 1 stands: a memory item is judged on the game cohort
        (`tools/cohort_read.py`, BAS-P37), not on `bench` alone.
      - C.1's accepted cost is recovered here (2026-11-20). The C.1
        restructure read −0.71% [−0.94%, −0.48%] against the pre-C.1
        base `496248d`, so the pools
        `tools/test_engines/c1-base.pool.json` and `c1-head` stay as
        its reference
        (`analysis/c0_evaluation_programme_2026-11-18.md` §10.10). The
        cost's mechanism is not known. It tracked the PGO build's
        `Evaluator::evaluate_full` instruction count: +95 over the base
        at −0.71%, +272 at −1.61%, +533 at −2.73%. Watch that count.
      Rarog audit inputs (2026-10-24,
      `analysis/rarog_audit_2026-10-24.md`; IDs are its items):
      - Played and full scores dumped (C-89); cache purity (C-105); a
        runtime lazy margin clears the caches (C-42); the profile comes
        from A.9.5.
      - Code audit 2026-10-27 (§6.3 E9) found no material or imbalance
        cache in the retired evaluation. Corrected 2026-11-26 for the
        rebuilt one (C.3):
        - The imbalance is cached. `imbalance()`
          (`src/eval_rb_material.h`) keys a 1,024-entry table
          (`IMBALANCE_TABLE_BITS`, `src/eval.h`) on the ten piece
          counts. It still counts them on every call to form the key.
        - Material and the piece-square tables are summed over every
          piece on every call (`material()`, same file), and the phase
          and the non-pawn material count the pieces again. The board
          keeps none of them incrementally; the donor keeps material and
          the tables in its position.
        - The pawn cache is pure.
        So the open item is an incremental material, table and count
        state, kept by make and unmake. It is behaviour-neutral when
        every score stays identical. The audit's E8, the lazy cut, is
        C.6's (C.7's note).
      It ends with a ranked list. Each item carries its mechanism, its
      share of the profile, its predicted gain, its proof of identity
      and the read that keeps or reverts it. An item under 0.5% of
      samples is not built (H.4's rule). The guard for each kind of
      item is registered here, before C.5.4 builds one.
    - [ ] **C.5.4** `[I2]` Speed pass: the implementation
      Added 2026-11-26 (maintainer). C.5.3's items in its order, one
      commit each.
      - Optimize only measured hot paths, with exact bench plus direct
        score-corpus identity: the played and the full scores dumped
        and compared (C-89).
      - Each item is kept or reverted on C.5.3's registered read, and a
        reverted item is recorded with its figure.
      - A false premise returns the leaf to C.5.3's packet.
    - [ ] **C.5.5** `[V]` Speed pass: pooled read, gate and close
      Added 2026-11-26 (maintainer).
      - Gate with interleaved pooled NPS and a natural-termination SPRT,
        the pass against the head it started from.
      - The game-cohort read beside it, loaded and at one engine.
      - Then the frontier pair, as after every accepted gate (the
        cluster shape's stage 7): its speed and depth figures are read
        against BAS-M27's.
      - The close states what share of C.5.3's gap is recovered and
        what is left, with its owner (H.4 on the network head).
- [ ] **C.6** `[V]` Nonlinear HCE SPSA or a written skip
  Nonlinear HCE SPSA (archived 8.10): only live, frequent terms Texel cannot
  price, such as the king-danger map, from a zero-game activation and
  curvature screen; a written skip if the surface is flat.
  It was C.9 until 2026-11-25.
  Scope since 2026-11-25 (maintainer; the rebuild packet's §8 and §13):
  the rebuilt evaluation's structure constants, which no static fit
  prices: the phase limits, the lazy thresholds, king danger's
  threshold and divisors, the complexity and scale-factor numbers.
  They ship at the donor's values until this leaf fits them, so a
  written skip is not open while one of them is unfitted.
  C.3.5's inventory of them is this leaf's surface list, and the
  constants become tune-build options here.
  From C.3.6, 2026-11-27 (the packet's R8 and Amendment 2):
  - The first refit cycle froze the 22 king-danger knobs
    (`KingAttackWeight`, `SafeCheck`, `Kd*`) and `SpaceWeight`, and
    routed them here. They join this leaf's candidate surface, read by
    its activation and curvature screen like any other coordinate.
  - The null refits' stage vectors (`tools/results/c36-screen-A/N/fit`,
    `c36-screen-A2/N/fit`) show how far a static stage walks them.
  - The 36 imbalance knobs are not on this surface: games cannot price
    them, and C.5.2 holds their pin.
  - If C.4.2 sends the scale-factor numbers to a static fit in C.4.3,
    they leave this surface.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - The nonlinear sweep (C-122); initiative and mop-up are untraced (C-47,
    C-36); the king-danger clamp (C-113).
  - The lazy thresholds' refit (C-98, moved from C.7 on 2026-11-26): the
    two exits' thresholds and their npm divisor are on this surface. A
    runtime threshold clears the caches (C-42) once they are tune-build
    options.
- [ ] **C.7** Joint re-fit on the new evaluation
  It was C.10 until 2026-11-25.
  One joint SPSA over the registered cp-valued search coordinates, including
  B.4's value surface if it exists, then PGO and SPRT `[0,3]`.
  Scope (maintainer 2026-11-12): this is the classical stage's big fit,
  properly investigated and as long as it needs: it finds the joint
  optimum of the engine's parameters on the new evaluation and finishes
  the best HCE engine before C.8's freeze and C.10's release.
  Several nights are acceptable, with large early moves and a slowly
  decaying schedule. Rule 7c's ceiling does not bind it; its own
  registration sets the horizon and the stop.
    - [ ] **C.7.1** `[R2]` Joint-fit investigation and registration
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
    - [ ] **C.7.2** `[V]` The joint fit, PGO and gate
      Runs C.7.1's registration, then the fitted-constant analysis, the
      bake, PGO and the SPRT.
  Cluster 1's margins (RFP, futility, razoring) were fitted on the eval
  before B.10's flip, which moves it by 31.7 cp mean; B.6.7's joint fit
  re-reads them on the B head, and this leaf refits them on the new
  evaluation.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Correction-magnitude terms on the surface (B-69); search and
    evaluation never share a tune (B-107); `scale_ratio.py` after B.10 and
    C (B-108); the qsearch margins as coordinates (B-25).
  - The lazy items (C-98, C-42) were written for the retired evaluation's
    `LAZY_MARGIN`, which no longer exists. On the rebuilt evaluation they
    are C.6's (corrected 2026-11-26), and below. C-42 applies here only if
    C.7.1 puts the thresholds on the joint surface: a runtime threshold
    clears the caches.
  - Code audit 2026-10-27 (§6.3 E7; C.5.2's until 2026-11-05): first the
    correction's size at the scaled endings' recogniser returns (B.14
    already took the four draw verdicts), then the switch, skip the
    correction when the evaluation came from a recogniser, read with the
    search surface.
  - The lazy-evaluation thresholds, a behaviour-changing question, moved
    here from the throughput leaf on 2026-11-05 so that C.8 freezes them.
    Corrected 2026-11-26: the code audit's E8 (§6.3) described the
    retired evaluation, a single cut at 700 with no npm term. The
    rebuilt evaluation (C.3) has the donor's form:
    - two exits (`lazy_skip`, `src/eval_rb.h`): the first after
      material, imbalance and pawns at `LAZY_THRESHOLD_1` = 1400, the
      second after pieces, mobility, king safety and passers at
      `LAZY_THRESHOLD_2` = 1300;
    - each compares half the sum of the two halves with its threshold
      plus a 64th of the non-pawn material;
    - an exit skips the families after it. Winnable, the scale factors,
      the endgame layer and the mate drive still run.
    They ship at the donor's values and are on C.6's surface (C.3.5's
    inventory, `tools/diag/rb_structure_constants.tsv`), so C.6 fits
    them and C.8 freezes them. This leaf reads them again only if
    C.7.1 puts evaluation coordinates on the joint surface.
- [ ] **C.8** `[V]` Freeze the classical evaluation
  It was C.11 until 2026-11-25.
  Checkpoint and freeze (archived 8.11).
  - Re-measure the same-search deficit against a fresh classical-Stockfish
    hybrid at the C head, plus conversion, the truth cohort, NPS and a 1T
    pool gauntlet.
  - Revalidate the score scale, mate bounds and every endgame floor.
  - Re-read the HCE frontier field (BAS-M18) with the C head: the aim's
    meter, in the field and head to head (2026-11-18).
  - Its 1T pool gauntlet, clock- and node-annotated, is D.1's loss-analysis
    source: nothing after the freeze changes a move or the speed (the
    speed pass is C.5's since 2026-11-26), so those games are the play
    of D.1's entry head (2026-11-05).
  - Ablate low-information mechanisms.
  - Freeze the classical evaluation.
  - Its games are C.10.1's fitting data: the frozen head's own score at
    each move, the material and the result (2026-11-21).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Conversion compared within one pool only (C-146).
  Rarog import input (2026-11-21, `analysis/rarog_import_2026-11-21.md`
  §2.4):
  - A zero-game reading of the C head's evaluation beside the deficit,
    by RAR-O10's recipe: the evaluation alone scored against Stockfish
    19's expected score at 1,000,000 nodes on 10,000 positions from
    Basilisk's own games, with Stockfish 11's classical evaluation and
    the C.0 head as anchors, by material class. It locates where the
    programme moved the evaluation and accepts nothing. Its prior is
    Rarog's read of the 2026-11-14 head (C.2.10).
- [ ] **C.10** Interim release after the evaluation programme
  It was C.13 until 2026-11-25.
  Maintainer decision 2026-10-24: the evaluation programme's head is released
  as 1.12.0 once C.8 closes, as Rarog cut 2.6.0 from its C head. Since
  2026-11-21 (maintainer) the release first prints its score on a fitted
  scale (C.10.1), so 1.12.0 is the first release whose +1.00 means an even
  chance. Since 2026-11-26 (maintainer) the remote is cleaned before the
  cut (C.10.3, C.10.4).
    - [ ] **C.10.1** `[I1]` Reported score and WDL in Stockfish's form
      Added 2026-11-21 (maintainer; `analysis/rarog_import_2026-11-21.md`
      §2.1, after Rarog's D.7.12). Output only: no search or clock
      decision reads the printed score, and `bench 13` is unchanged. Its
      constants are fitted on the frozen head (C.8), and nothing after
      the freeze changes a move, so they are the release's.
      - **The form is Stockfish's, whole** (`src/uci.cpp`,
        `win_rate_params`, `win_rate_model`, `to_cp` and `wdl` at
        `sf_19`).
        - The material is `m` = pawns + 3 × knights + 3 × bishops +
          5 × rooks + 9 × queens, clamped to [17, 78] and divided by 58.
        - `a(m)` and `b(m)` are cubics in that value.
        - An ordinary score prints as `round(100 · v / a(m))` at the
          root's material, held below the tablebase band.
        - The win rate is `1 / (1 + exp((a − v) / b))` in permille, the
          loss the same at `−v`, the draw the rest.
        - `UCI_ShowWDL`, a check option off by default, adds
          `wdl W D L` after the score and its bound on every `info` line.
        - Mates and tablebase results print as they do today
          (`format_info_line`, `src/search_root.cpp`).
      - **The constants are Basilisk's own,** fitted on its own games as
        Stockfish fits its own on fishtest's games.
        - The data is the frozen head's games from C.8's reads: its own
          score at each move it searched, the material, and the result
          from its side. The leaf records the games, the control and the
          positions kept.
        - Mate scores and scores in the clamped bands are left out, and
          each material count from 17 to 78 needs a stated minimum of
          positions.
        - `a` and `b` are fitted per material count by maximum likelihood,
          then each by a weighted cubic.
      - **The tool** is new Basilisk Python, `tools/wdl_fit.py`, with
        tests. Its tests recover known constants from synthetic games and
        check the reader's side and filters. It reports the calibration in
        bands of the printed score, observed against predicted W / D / L,
        and its check mode fails unless the source holds the fitted
        constants. Rarog's tool of the same name is the method reference
        (`78d4681`); no Rarog value crosses.
      - **Tests:**
        - +100 prints at an even chance, and the map is monotone at every
          material.
        - The rates are defined at every material a board can hold, sum to
          1,000 and are mirrored.
        - The field appears only when the option is on, and the option
          reaches the engine.
        - A search's nodes, move, internal score and line count are equal
          with the rates shown or not.
      - **Known limit** (Rarog's read): the model has no term for a proven
        draw, so draws next to zero run above its prediction. That is
        recorded in the calibration, not repaired.
      - The changelog says that scores now mean an even chance at +1.00
        and that `UCI_ShowWDL` shows the rates.
      - The constants are checked again at E.3, and refitted per network
        at F.6.
    - [ ] **C.10.3** `[R2]` Remote inventory and the ruling
      Added 2026-11-26 (maintainer): too much is pushed to the remote,
      some of it things that do not belong there at all, and it leaves
      before the next release. The rule is the maintainer's of
      2026-11-25: the remote holds the project and its source, as lean
      as possible. This leaf removes nothing. It ends in a list the
      maintainer signs, class by class.
      - **The tree.** Every tracked path is classified: the engine and
        what builds, tests and releases it; what the working method
        needs (the rules, the roadmap, the ledger, the procedures); and
        what is neither. Measured 2026-11-26, the tree with BAS-M28
        noted: 940 files, 21.2 MiB. By size: `logo/` 8 files and
        7.8 MiB, six of them images of 0.9 to 2.0 MiB and three of
        those alternatives; `analysis/` 89 files and
        2.6 MiB; `tests/fixtures/` 2.5 MiB, of which
        `eval_reference.tsv` is 2.3 MiB; `tools/diag/` 140 files and
        2.4 MiB; `docs/experiments/` 327 files and 1.2 MiB;
        `docs/archive/` 0.7 MiB; `src/` 87 files and 0.9 MiB.
      - **Not ruled on at 2026-11-25:** `external/fathom/` (compiled
        into the engine), `tools/oracle/*.patch` and
        `tools/weather-factory-overlay/`.
      - **Off the tree.** Branches (2), tags (31), releases (29) and
        their assets, pull-request refs (35), and the Actions artifacts
        and caches. Each is kept with its reason or listed to go.
      - **For each class that goes:** what reads it (a check, a test,
        CI, a citation, a procedure), where it lives afterwards
        (ignored local storage with a manifest, or nowhere), and what
        changes so that nothing breaks.
      - **The history question, with its price.** Removing a file from
        the tip leaves it in every clone's history. Removing it from
        history rewrites every commit of `master`: the documents'
        commit citations break (762 at this date), the release tags
        move, and GitHub keeps the old commits under its pull-request
        refs, which a push cannot rewrite. The packet prices both and
        the maintainer rules: the tip only, or the history too.
      - It reads `READY_FOR_IMPLEMENTATION` when the list is signed.
        If the ruling includes the history, C.10.4's class is raised
        here.
    - [ ] **C.10.4** `[M]` The remote clean-up
      Added 2026-11-26 (maintainer). C.10.3's signed list, and nothing
      beside it.
      - A file kept locally is verified against its manifest before it
        is untracked.
      - The checks, the tests and CI are changed in the same commit as
        the removal they follow.
      - Done when a fresh clone builds, passes CTest and CI, and the
        roadmap, citation and rules checks pass on it.
      - Deleting a tag, a release or a remote branch, and any forced
        push, are the maintainer's commands: prepared, handed over one
        at a time and verified after.
      - C.10.5's scans then run on the lean tree.
    - [ ] **C.10.5** `[M]` The 1.12.0 cut
      It was C.10.2 until 2026-11-26.
      Licensed as B.11.4 is: an accepted SPRT over the 1.11.0 release at
      `3+0.03` 1T, `10+0.1` and 4T direction checks whose 95% intervals
      exclude a loss, the 4T check sibling-free. Cut through A.8.6's
      tag-driven flow, after B.11.3's scans are repeated on the candidate
      (PLAN §5).

### Active workflow register

One row per open leaf of the current phase. The checker requires every open
leaf of the current phase to have a row, and each row's class to match the
leaf's tag. Later phases carry only a class until they open.

| Leaf | Workflow state | Class | Current decision |
|---|---|---|---|
| C.4.2 | RESEARCH | R3 | Not started |
| C.4.3 | RESEARCH | I2 | Not started |
| C.4.4 | RESEARCH | R3 | Not started |
| C.4.5 | RESEARCH | I2 | Not started |
| C.4.6 | RESEARCH | R2 | Not started |
| C.4.7 | RESEARCH | V | Not started |
| C.5.1.1 | RESEARCH | R3 | Not started |
| C.5.2 | RESEARCH | V | Not started |
| C.5.3 | RESEARCH | R3 | Not started |
| C.5.4 | RESEARCH | I2 | Not started |
| C.5.5 | RESEARCH | V | Not started |
| C.6 | RESEARCH | V | Not started |
| C.7.1 | RESEARCH | R2 | Not started |
| C.7.2 | RESEARCH | V | Not started |
| C.8 | RESEARCH | V | Not started |
| C.10.1 | RESEARCH | I1 | Not started |
| C.10.3 | RESEARCH | R2 | Not started |
| C.10.4 | RESEARCH | M | Not started |
| C.10.5 | RESEARCH | M | Not started |

## Phase D — Clock, threads, tablebases

- [ ] **D.1** `[R2]` Time management
  Basilisk's clock (optimum and maximum, `Move Overhead` 10, BAS-R03's
  instability extension, helper handling) audited against Stockfish's shape
  and against Manta's ADR-0065 checklist; SPRT `[0,3]` at STC, a `10+0.1`
  direction check and a 4T zero-forfeit run (archived 9.7).
  The fail-high depth reduction (`B5FailHighReduce`, B.6.6 row 17) is
  this leaf's. Its switch leaves in B.8.2 and is rebuilt here from
  `55e3010` (`search_worker.cpp:608`; `analysis/b82_switch_inventory_2026-11-14.md`
  §3).
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
    clock- and node-annotated games of C.8's pool gauntlet, the play of
    this leaf's entry head against stronger engines (nothing after the
    freeze changes a move or the speed);
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
  Node-count and effort inputs (2026-11-15, after B.8.7; maintainer: add
  what the count change showed):
  - The effort term is not the donor's form. Basilisk's is the best
    move's nodes in the iteration's last search over every window's nodes
    of that iteration (`search_worker.cpp:705-707` at `dfa922c`), read as
    a step: at or over `TmEffortHi` 80 the total is ×0.80, at or under
    `TmEffortLo` 25 ×1.20 (`:881-883`). The donor's
    (`search.cpp:571` and `:588-589` at `sf_19`) is the best root move's
    effort over the whole search against every node searched,
    interpolated from 75.8% to 104.5% onto ×0.969 to ×0.714 and clamped
    to [0.693, 0.838]: it only ever shortens a think.
  - So Basilisk's low step is in force most of the time: 55.1% of the
    iterations with a window on `suite_v2` at depth 12 before B.8.7,
    56.7% after it (the high step 10.5% and 11.1%); B.5's input above
    read 56.6%. The ×1.20 is close to a constant, not an easy-or-hard
    signal, and it extends where the donor's term never does.
  - B.8.7 moved the term's input to the donor's unit (once per move
    made) and its firing by about a point. The donor's form is now
    portable exactly: its effort is the root move's own count, summed
    over iterations.
  - The audit ticks each of the donor's factors present, absent or
    different beside Basilisk's: falling eval (Basilisk's score-drop
    extension, `TmScoredropThr`, `TmScoredropDiv`), the best move's
    stability over depths (`TmStability`), the previous move's time
    reduction (absent), instability across threads (D.2's), and node
    effort (above). The donor's shape is built whole as one arm, then
    both arms are fitted, as H.6's rule says for a form a fitted arm
    cannot clear.
  - Then one SPSA of the clock's coordinates on the chosen form (the
    default surface's `Tm*` lines and whatever the donor's shape adds),
    at the control it acts at and against an external engine (TT-3),
    not in self-play. A change to the effort measure re-reads
    `B5AspBase` and `B5AspGrowth` (above).
  Rarog import inputs (2026-11-21, `analysis/rarog_import_2026-11-21.md`
  §2.2; Rarog's D.7.9.1, RAR-R34):
  - **The whole form is the prior.** Rarog's hybrid clock was replaced by
    one donor's time manager whole (Reckless's budget, horizon and five
    factors; its guards kept with their tests). It read **+30.0 ± 8.8 Elo**
    at `3+0.03` and **+18.6 ± 8.2** at `10+0.1`, predicted +4 and +3.
    The hybrid's early under-spend (0.60 of the base in the first twenty
    moves against 0.75) cost far more than a null horizon read suggested.
  - **The pieces a partial port misses** are the ones Rarog's hybrid
    lacked: Stockfish's per-control scale, its cut when behind on the
    clock, and `increaseDepth`. The audit above ticks them beside the
    factors.
  - **A zero-game census before the arms:** the share of time spent past
    the optimum and the share of moves at three times the optimum or more,
    on the entry head's games. Rarog's hybrid read 30% and 6.1%, past D.1's
    3% trigger for `increaseDepth`.
  - **Guards against shape:** every piece of the clock is classed as a
    guard, kept with its tests, or shape, replaced by the donor's, before
    the port. The guards are the reserve under `Move Overhead`, the
    proved-mate stop, the first-iteration guard, the tablebase box, the
    single-move rule and the `go` semantics.
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
  Speed pass input (2026-11-16, B.8.10, `analysis/b88_speed_packet_2026-11-15.md`
  §13): the table prefetch across the make (X0, kept) predicts the child's
  key at every make and costs about 1% of a node where it buys nothing
  (−1.02% at Hash 1, +0.56% at Hash 64, +1.13% at Hash 256, one engine;
  +1.32% with fourteen engines). A form that keeps the lead and pays less
  (the key or the prefetch only where the probe is likely to miss the
  cache) is a question for the table's redesign, read by PROCESS *NPS*'s
  rule for a memory item; a new layout moves the cost and the payoff both.
  Rarog import inputs (2026-11-21, `analysis/rarog_import_2026-11-21.md`
  §2.3; priors only):
  - **Shared correction** (Rarog's D.13 card, Reckless's form): the pawn
    and non-pawn correction tables shared per NUMA node.
    - A read is a relaxed 16-bit load and an update a relaxed load and
      store, so a racing update may be lost, never torn.
    - One thread plays the same, bench exact, for a pointer load a node.
    - The four-thread acceptance is costed at 13 to 18 hours for an SPRT
      `[0,5]`, or 1 hour 45 minutes for a no-harm read.
    - Stockfish shares correction, pawn and continuation histories per
      NUMA node (`SharedHistories`).
  - **Soft-stop voting, at zero games first** (Rarog's D.14): a `diag`
    counter at 4T, on 300 positions at `3000+30`, counts the searches in
    which a thread that voted to stop would have withdrawn its vote before
    the pool stopped. Under 2% of searches, withdrawal is `NO_CHANGE`.
  - **The prefetch's place** (Rarog's D.7.8.12, pooled PGO NPS):
    - prefetching the child's cluster before the make, from the key the
      move will produce, read +1.09%;
    - moving it up to where the move is picked, before the pruning
      rules, read a further +1.12%;
    - prefetching the correction and pawn entries as well read nothing
      once the table's line was hidden.

    A candidate for the cheaper form asked above, read by the same rule.
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
  Probe depth and limits, WDL and DTZ in conversion, interaction with C.4's
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
  - Added 2026-11-18 (maintainer): **Stockfish's probing code as an
    option, not a plan.**
    - Fathom stays the prober while it works. Fathom is the MIT-licensed
      standalone form of Ronald de Man's code, from which Stockfish's
      `tbprobe.cpp` also grew.
    - It is replaced only if this leaf measures a reason:
      - a probe cost that shows in NPS or time to depth with tables
        loaded;
      - or Fathom causing thread or integration defects again, after
        B.9's repairs (its `exit()` on a failed map, non-ASCII paths, its
        root buffer).
    - The replacement would port `sf_16`'s or `sf_19`'s `tbprobe.cpp`
      (GPL-3) onto Basilisk's board, under Stockfish's existing
      attribution, with B.9.3's hard-verdict tests and the truth cohort as
      its correctness bar.
    - It removes only Fathom's MIT notice: the GPL text ships regardless.

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
  Attribution checkpoint (archived 9.9): the final head against 1.10.1 and the
  B.11.1 and C.8 heads at STC, `10+0.1` and 4T; attributed Elo per programme
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
    user-facing text (D-71); E.2's 4T pool read beside the head-to-head
    (D-43).
  The reported score (2026-11-21): `tools/wdl_fit.py`'s check is run on
  the candidate against C.10.1's constants. Phase D moves neither the
  evaluation nor its scale, so a failed check is investigated before any
  refit.

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
  using C.7's protocol (archived 11.4.a–b); BAS-R02, R03, S08 and S09's retry triggers are
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
  - The printed score's scale is refitted here, per network, with
    C.10.1's tool and Stockfish's form (G.2's until 2026-11-11; TA-2;
    Rarog's F.6). Since 2026-11-21 the first fit is C.10.1's, on the
    classical head, so 1.12.0 already prints +1.00 as an even chance to
    win; each network gets its own constants.
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
  - The printed scale calibrated so +1.00 is a 50% chance to win is
    C.10.1's since 2026-11-21, refitted per network in F.6; tablebase wins printed as
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
  Added 2026-11-17 (B.11.1 declined its hybrid arms): whether Stockfish's
  search has moved far enough past `sf_19`, a later release or master, to
  re-read the donor's shape on the head's evaluation; RAR-O08's method
  applies, Stockfish only.
- [ ] **H.2** `[I2]` Search refinements, one at a time
  Each item alone against the head on the same binary, SPRT `[0,3]` run to
  a decision with no early stop on a lean (Triumviratus's stopped leans
  added up to nothing, TM-4); levers that tune together enter one SPSA
  rather than many SPRTs.
  Reverse futility's seek-mate cap (`B4RfpSeekMate`, B.6.6 row 16) is
  this leaf's, on the network. Its switch leaves in B.8.2 and is rebuilt
  here from `55e3010` (`search_kernel_core.cpp:1108`;
  `analysis/b82_switch_inventory_2026-11-14.md` §3).
  Rarog import input (2026-11-21, `analysis/rarog_import_2026-11-21.md`
  §2.5):
  - **Quiet checks after the skip.** Once late-move pruning or futility
    fires, the picker skips every quiet move left, direct checks among
    them (Stockfish's `skip_quiet_moves`, Reckless's alike).
    - On WAC positions and their continuations, Rarog's kernel with the
      skip then played a move allowing a mate in one in 1 of 123
      positions where a move avoids one, at every depth to 7. Without the
      skip it did not.
    - First a zero-game count of the same on Basilisk; then, if it fires,
      the skip that keeps direct checks, as one item.
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
  B.8.5 and C.5.3 profile the classical engine, and F.5 covers the
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
    and `B6_DONOR_FORMS` leave in B.8.2 (maintainer, 2026-11-14). Each
    form is rebuilt here from `55e3010` at the place
    `analysis/b82_switch_inventory_2026-11-14.md` §3 names, with
    `B5NegExtMax`'s other two values.
  - The draw jitter (B.6.6 row 20; added 2026-11-15 at the maintainer's
    request): the donor scores a draw `VALUE_DRAW - 1 + (nodes & 2)`
    (`value_draw`, `search.cpp:134` at `sf_19`), Basilisk 0. B.6.6 kept 0
    with no card: B.5.3's blindness read found 0 of 885 repetition draws
    with the repeating side at +150 or better, and Rarog's jitter read
    −1.2. Since B.8.7 its input, the node count, is the donor's, so the
    form ports exactly. Its population counter is the draw returns
    (repetition, rule 50, the ply cap in check) on `bench 13` and the
    gate games; a small expected value, so it is read here and not
    earlier.

## 4. Measurement protocols

| Meter | Protocol | Owner |
|---|---|---|
| Pool score | Colosseum, fixed pool with Houdini 3, `3+0.03`, UHO, no adjudication, 400 games per pair, 1T and 4T | A.7, B.11.1, C.8, E.2 |
| Search deficit G(0) | Paired, head against the frozen classical-Stockfish search oracle rebuilt with the head's HCE, 3,000 games, no adjudication; at equal time (BAS-O05) and, from B.11.1, also with the bridge's price returned as time odds (RAR-O08) | A.7.3, B.11.1 |
| Evaluation deficit | Hybrid at the head against the classical-Stockfish-HCE hybrid, same search | C.8 |
| Cluster acceptance | Registered final-PGO SPRT, bracket per rule 5, `[0,10]` for B.2 | every cluster |
| Neutral change | Exact fingerprint, release and sanitizer CTest, pooled-PGO NPS | B.1, B.8, C.1, C.5.4, F.1 |
| Conversion | A.5.5's audit on the latest pool tournament | every checkpoint |
| Fixed-node shape | Oracle differential at stride 1, depth at 300k nodes, branching profile, WAC at fixed nodes, positional screen | B clusters |
| Endgame layers | Truth cohort, drawn overclaim, conversion at the deployment bracket, floors | C.4 |

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
  **1.12.0** after C (C.10), **1.13.0** after E (E.3), and **2.0.0**, the
  first NNUE release (F.9). Between them nothing is released unless a
  correctness repair forces a patch.
- A classical release requires an accepted SPRT over the previous release at
  `3+0.03` 1T, and `10+0.1` and 4T direction checks whose 95% intervals
  exclude a loss. There is no 4T pool read at a release: a head-to-head cannot see a thread-scaling loss both versions share,
  and D.2's self-relative scaling read measures exactly that against
  Stockfish's and Reckless's references; E.2's target gate plays the field
  at 4T (maintainer decision 2026-11-18, replacing the 4T pool read beside
  every classical release; RAR-M76).
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
  C.10, E.3 and F.9 repeat them.
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
| `docs/reference/` | The pinned commits of the other repositories the plan uses (Rarog, Triumviratus), the manifests of their local copies, and the map from each leaf to its inputs; no other project's file is tracked |
