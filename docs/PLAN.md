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

   7c. **A tune runs in blocks with a movement stop rule.** Each block is
   2,000 iterations × 30 games (60,000 games, about 12.5 hours at Rarog's
   measured rate). It starts from the previous block's rounded centres with a
   fresh gain schedule, the same surface and the same steps. After a block:
   - at least three coordinates moved a full step → the next block runs;
   - fewer → the tune stops, and theta is the last block's rounded centres;
   - never more than three blocks without a new registration.

   The count saves unattended compute only: a tune the maintainer is watching
   may run every registered block. A block cut short never counts, and a tune
   is judged by its gate, not by its movement. This is Rarog rule 7c
   (RAR-S75, RAR-S78, RAR-S82–S83, imported as BAS-X31).

   The blocks are the shape for seeds far from their optimum. Where a
   zero-game curvature sweep and an earlier block read the surface as near
   its optimum with small gradients, a tune may instead be registered as
   one longer run with a larger perturbation and a smaller gain ratio: a
   restarted schedule only re-inflates the random walk near an optimum,
   while a larger perturbation raises each gradient estimate's signal. The
   registration states the perturbation, the gain ratio and the resolution
   they buy; a full step stays the block's step in every movement reading,
   and a run cut short is not theta. Maintainer decision 2026-10-27, from
   Rarog's amendment of 2026-10-20 (RAR-S104, RAR-S105; BAS-X36).
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
   and SPSA blocks are prepared and verified by the agent and run by the
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
(`analysis/rarog_audit_2026-10-24.md`) unless named.

| Owner | Defects |
|---|---|
| B.4 | B-17 the dead SEE clamp |
| B.5 (with D.1) | the fail-high stop's hole: a window that never returns is never checked (A-47; `analysis/b5_failhigh_stop_2026-10-25.md` §2.3); the ply cap returns the static evaluation in check (NODE-8) |
| B.9 | `go depth N` answered by a deeper helper (DRIVER-12); Fathom's `exit` on a table that fails to map (PROTO-11); A-50 a stop inside the first iteration; A-57 cold table reads on the clock; A-58 the tablebase root ranking ignores the game's repetitions |
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
| SPSA | Curvature sweep; registered surface, blocks and estimator; perturbation proof on every coordinate; fresh PGO bake; independent SPRT |
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
4. The curvature sweep, then SPSA in blocks over the cluster's live
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
      packet's departures table for B.6.1: Basilisk's form, the donor's,
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
      instrument of B.7.4, B.8.5 and any profile share a gate asks for
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
        validation read (BAS-P28, registered, not run) moves to B.7.4, the
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
- [ ] **B.4** Cluster 3 — quiescence
  TT cutoff, corrected stand-pat with interpolated fail-high, futility base,
  move-count and SEE pruning, evasions only in check, TT write on exit; SPRT
  `[0,3]`.
  Its packet keeps a departures table for B.6.1 (maintainer 2026-10-31):
  every place the cluster keeps Basilisk's form where the donor's differs,
  the reason, and the feature or fit it waits on.
  Split into leaves 2026-11-05 (maintainer: the sub-steps visible in
  GUIDE), in the order they run: the two between-cluster repairs (B.4.1,
  B.4.2), then the standard cluster shape (PROCESS *Cluster delivery
  shape*): B.4.3 research, B.4.4 implement, B.4.5 diagnostics and paired
  run, B.4.6 sweep and SPSA, B.4.7 gates. Basilisk's forms are re-read
  against the donor's in B.6.1, from the departures table. B.4.3 is the
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
    - [ ] **B.4.4** `[I2]` Implement behind the umbrella switch
      The packet's contract behind one umbrella option, both arms
      building; accepted by an independent review that reproduces the
      fingerprints before diagnostics start.
    - [ ] **B.4.5** `[V]` Diagnostics and the unfitted paired run
      The packet's zero-game screens on both arms, then its registered
      categorical and deletion reads and the unfitted paired run.
    - [ ] **B.4.6** `[V]` Curvature sweep, then SPSA in blocks
      The packet's registered sweep, then SPSA in blocks on Cluster 3's
      surface (with B-21's RFP, razoring and NMP-base coordinates); the
      fitted-constant analysis precedes the bake.
    - [ ] **B.4.7** `[V]` Cluster 3 gates
      SPRT `[0,3]` for the cluster, and each categorical and deletion arm
      at the bounds the packet registers.
- [ ] **B.5** `[I2]` Cluster 4 — root and aspiration
  Cluster 4 — root, aspiration and iterative deepening: aspiration centred on
  the root move's average score with width from its mean squared score,
  optimism, root-move effort, the `pvIdx` root structure with a `MultiPV`
  option, and deep-iteration line growth; SPRT `[0,3]`.
  Its packet keeps a departures table for B.6.1 (maintainer 2026-10-31),
  as B.4's does.
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
- [ ] **B.6** Donor-form reconciliation, then joint search SPSA
  Joint search SPSA over the coordinates the four clusters left live;
  registered surface; PGO bake; SPRT `[0,3]`. It runs (maintainer
  2026-11-05: for the assurance that nothing larger was missed); the
  curvature evidence chooses its surface, and a flat surface is recorded,
  not a skip. Before it, the reconciliation (B.6.1, added
  2026-10-31 by the maintainer), so the surface is fitted on the forms
  the evidence chose.
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
    - [ ] **B.6.1** `[R2]` Donor-form reconciliation
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
      against the head before B.6.2.
      Rows added 2026-11-05, donor departures no packet had recorded
      (`analysis/rarog_sweep_2026-11-04.md`):
      - The TT key carries no rule-50 bucket. Stockfish probes and stores
        with the key adjusted by the rule-50 count once it reaches 14
        (`adjust_key50`), so bounds are not shared between positions with
        very different fifty-move horizons; the repetition key stays raw
        (A-14's test). Rarog parked it while games were adjudicated, a
        reason neither engine has now; it matters in the long played-out
        endings every gate now has. The child TT prefetch uses the
        adjusted key.
      - The non-pawn correction key leaves the kings out (`board.cpp`,
        `nonpawn_key`); Stockfish's includes them. A bare king's key is 0:
        one entry for every position where a side has a bare king, which
        carried part of the KBP-K correction drift (B.3.0's packet §1.4).
      Triumviratus review inputs (2026-11-03,
      `analysis/triumviratus_review_2026-11-03.md`; IDs are its items):
      a reconciliation that reaches past `sf_19`, or a later donor re-pin,
      reads first what did not pay on Triumviratus's Stockfish-19 base:
      Stockfish's post-19 linear all-node razoring (−2.32 ± 2.52 over
      20,032) and null threshold after sibling null fail-highs (+0.46 ±
      2.66 over 17,308) (TS-16), and four edge cases other engines handle,
      each flat over about 7,000 games (TS-17).
    - [ ] **B.6.2** `[V]` Joint search SPSA
      The joint SPSA above, on the reconciled head.
      A prior from the Rarog sweep (`analysis/rarog_sweep_2026-11-04.md`):
      a blind, uniform 15% shift of Rarog's whole selectivity surface
      toward less pruning read +4.06 ± 3.71 over 14,196 games (RAR-S54),
      evidence that block-wise self-play left it too aggressive, and led
      to its broad refit, +15.33 ± 7.34 nElo. The registration reads it
      beside the curvature sweep.
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
    - [ ] **B.7.1** `[R2]` Board contract audit and instruments
      Was D.3.1, then B.15.1 (HISTORY's number map): the defect
      hunt, and the safety net B.7.3 rebuilds behind (archived 7.1, 7.7,
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
      - A search-board perft (TB-1, its `tdperft`): perft through the
        search's own generator, make and unmake that recomputes every key,
        the occupancy and the piece table from scratch at every node, and
        re-checks every parent move against the pseudo-legality test the TT
        and killer moves use. It validated a board rewrite there.
      - A permanent Zobrist test (TB-2): the GF(2) rank of the key set is
        64 (Basilisk read 64 on 2026-11-03; Triumviratus's keys from a
        32-bit generator had rank 32 and lost a game to a false hash hit).
      - FEN robustness (TB-3): an en-passant square inconsistent with the
        side to move, castling rights that do not match the board, a side
        not to move in check; one position must never get two keys.
    - [ ] **B.7.2** `[R2]` Board review against Stockfish and Triumviratus
      A packet that reads each component against the donor's form and
      Triumviratus's, measures Basilisk's (its share in A.9.5's profile at
      `bench 13`, a microbenchmark per component), and decides keep,
      replace or adapt, each with its reason and expected cost; a
      departures table records every kept Basilisk form. Its frozen handoff
      is B.7.3's contract. Components:
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
    - [ ] **B.7.3** `[I2]` Rebuild behind the board contract
      B.7.2's accepted components, one commit each, each passing B.7.1's
      corpus and search-board perft and keeping the exact `bench 13`
      fingerprint while the generation order is unchanged. No search,
      evaluation or parameter change rides along. Each replaced component
      keeps, behind a build switch, a check that runs the old form beside
      the new at every node and aborts on the first disagreement (TM-10).
    - [ ] **B.7.4** `[V]` Board speed read and gate
      Pooled-PGO NPS of the rebuilt board against the head (A.9.4) and
      each component's share in the profiler (A.9.5); a component that
      does not pay its +0.5% returns to Basilisk's form. A component that
      changes the generation order, and so the tree, takes a `[-5,0]`
      SPRT at `3+0.03`.
      First, B.16.4's counter read is validated: BAS-P28, registered
      2026-11-05, `tools/run_b164_pmc.ps1` (elevated, idle host, about ten
      minutes; its P25 pools rebuilt by `tools/nps_pool.ps1` if gone, the
      new pool hashes recorded). Then it reads beside the pooled read.
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
      What B.8 keeps after the kernel's deletion. The ten zero counters
      leave the counter contract only with the oracle's side of it.
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
- [ ] **B.9** Robustness and protocol before the release
  Added 2026-11-05 (maintainer: every correctness fix before the next
  release): the lifecycle and protocol work that was D.3.2, the
  release-visible defects of the Rarog sweep and the tablebase root
  defects that were D.4's. Nothing here is tuning. Each item gets a test
  that fails on the head before its repair. It runs before B.11.1, so
  that the checkpoint measures the repaired head.
    - [ ] **B.9.1** `[R2]` Lifecycle and protocol audit and tests
      Lifecycle and protocol (archived 9.4): worker start, stop and join,
      cancellation, result authority, new-game and option resets, command
      ordering, Syzygy thread and halfmove contracts; deterministic
      interleavings plus stress; zero crashes over B.11.1's pool
      tournament. It was D.3.2 until 2026-11-05.
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
      Thread contracts (TP-5): `go nodes` counts every thread; the main
      thread's PV is reset before the helpers start; a proven mate beats
      depth in the result choice.
    - [ ] **B.9.2** `[I2]` Repairs and the repair gate
      Each failing test of B.9.1 repaired in its own commit, through the
      correctness-repair path: CTest on release and sanitizer builds, the
      exact fingerprint where the search is untouched and a recorded move
      otherwise. The repairs that can change a move are gated together in
      one repair SPRT `[-5,5]` at `3+0.03` (at Threads 4 for a thread-path
      repair), its cap sized from their measured exposure
      (`agents/measurement.md`); protocol repairs that cannot change a move
      take no games.
- [ ] **B.11** Checkpoint and interim release
  The search programme's checkpoint (B.9 until 2026-11-05), then the
  1.11.0 release (B.11 until 2026-11-05).
    - [ ] **B.11.1** `[V]` Search programme checkpoint
      Checkpoint: re-measure every deficit meter.
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
        (B-110) and bit 6's asymmetry (B-109) in the readings. B.11.2
        follows.
      Added 2026-11-05:
      - Its readings are C.0's starting point, and its pool reads license
        B.11.2 beside the release SPRT. The four-thread reads are
        sibling-free (PROCESS *Harness*, BAS-X41).
      - Basilisk is judged by the pool score, not the Rarog pairing: in
        Rarog's legacy 10.0 the sibling head-to-head differed from the pool
        difference by 35 to 45 Elo at 1T and 4T, a matchup effect of two
        similar evaluations (`analysis/rarog_sweep_2026-11-04.md`).
      - The floors' legality patch moved to B.4's opening, the first point
        between clusters (B.9 until 2026-11-05).
    - [ ] **B.11.2** `[M]` Interim release 1.11.0
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
| B.4.4 | READY_FOR_IMPLEMENTATION | I2 | The contract of `analysis/b43_cluster3_research_2026-11-06.md` §10 |
| B.4.5 | RESEARCH | V | After B.4.4's review |
| B.4.6 | RESEARCH | V | After B.4.5 |
| B.4.7 | RESEARCH | V | After B.4.6 |
| B.5 | RESEARCH | I2 | Contents fixed by B.0; the ply cap and the clean child frame added 2026-11-05 |
| B.6.1 | RESEARCH | R2 | Added 2026-10-31; after B.4 and B.5 close |
| B.6.2 | RESEARCH | V | Runs (maintainer 2026-11-05); the curvature evidence chooses its surface |
| B.7.1 | RESEARCH | R2 | Was D.3.1, then B.15.1; after B.6, the board's safety net |
| B.7.2 | RESEARCH | R2 | Was B.15.2; after B.7.1 |
| B.7.3 | RESEARCH | I2 | Was B.15.3; on B.7.2's frozen handoff |
| B.7.4 | RESEARCH | V | Was B.15.4; after B.7.3, before B.8 |
| B.8.2 | RESEARCH | I1 | After the clusters and the board |
| B.8.3 | READY_FOR_IMPLEMENTATION | I1 | Was B.7.1; before the speed pass, no engine change |
| B.8.4 | RESEARCH | I1 | Split from the speed pass 2026-11-05; before it |
| B.8.5 | RESEARCH | I1 | Was B.7.2; after B.8.3 and B.8.4, on the new modules |
| B.9.1 | RESEARCH | R2 | Was D.3.2, with the release-visible defects (2026-11-05) |
| B.9.2 | RESEARCH | I2 | On B.9.1's failing tests |
| B.11.1 | RESEARCH | V | Was B.9; programme checkpoint |
| B.11.2 | RESEARCH | M | Was B.11; interim release after B.11.1 (maintainer decision 2026-10-24) |

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
    (C-124, F.2.2).
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
- [ ] **C.2** Fit pipeline, corpus and label contract
  Fit pipeline, corpus and label contract (archived 8.1–8.8).
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
    - [ ] **C.2.6** `[V]` Matched fits and the label-contract gate
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
    - [ ] **C.7.1** `[R2]` Instruments and gate integrity
      Instruments and gate integrity (archived 6.6.a–e and 6.6.h).
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
        and 600k (C-137); a drawn-overclaim instrument from Syzygy draws
        (C-133); measurement layers on reports (C-141); Syzygy-verified vetoes
        in the unit suite (C-142); replay tools must not read bound scores as
        evaluations (C-145); KBNK as a multi-budget majority (B-3).
      - The colour-mirror test (`test_eval.cpp`'s symmetry sweep) extended
        to `tests/endgames.epd` (14 of 17 positions have White stronger),
        `endgame_draws.epd` and the recogniser positions (Rarog EVAL-2,
        D.5.17; `analysis/rarog_sweep_2026-11-04.md`): Basilisk has had
        one colour asymmetry already, the pawnless tie.
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
- [ ] **C.10** `[V]` Search re-fit after the new evaluation
  One joint SPSA over the registered cp-valued search coordinates, including
  B.4's value surface if it exists, then PGO and SPRT `[0,3]`.
  Cluster 1's margins (RFP, futility, razoring) were fitted on the eval
  before B.10's flip, which moves it by 31.7 cp mean; B.6.2's joint fit
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
  B.11.2 is: an accepted SPRT over the 1.11.0 release at `3+0.03` 1T, `10+0.1`
  and 4T direction checks whose 95% intervals exclude a loss, and a 4T pool
  read beside them, the 4T reads sibling-free. Cut through A.8.6's tag-driven
  flow.

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

  The ADR-0065 checklist:
  - distinct optimum and maximum budgets;
  - the maximum rooted at `go` receipt and never extended;
  - ponder credit used once;
  - the optimum adjusted only by completed iterations;
  - an easy root spends less;
  - a minimum completed depth;
  - helper contributions normalised by helper count.

  Each item is ticked present, absent or different before the donor shape
  is chosen. The forfeit margin is sized on a null pair: BAS-E57's reserve
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
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Self-relative 1/2/4/8T scaling and hash adequacy (D-44, D-45); the first
    touch of per-thread tables before the first `go` (D-21).
  - Code audit 2026-10-27 (S6's helper side): helpers overwrite the root
    key the main thread reads its first move from; read with B.5's S6
    counter at 4T.
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

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
  Attribution checkpoint (archived 9.9): the final head against 1.10.1 and the
  B.11.1 and C.11 heads at STC, `10+0.1` and 4T; attributed Elo per programme
  from the accepted SPRTs; every deficit meter; NPS; the maturity checklist.
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
    name the PeSTO-derived tables and Stockfish 19's time-manager constants,
    ship Fathom's MIT notice and the licence with the assets, and rerun the
    similarity screen on the candidate; short user-facing text (D-71); a 4T
    pool read beside the head-to-head (D-43).

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

- [ ] **F.0** `[R3]` NNUE investigation
  Investigation (archived 10.0–10.3): board events and accumulator ownership,
  the trainer and its quantisation and export contracts, data format and
  splits, the first architecture (at least Gyatso's single-bucket 768×1024
  perspective net, CCRL 3258 at 40/15) and the cost ledger; frozen handoffs
  for F.1–F.4.
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
- [ ] **F.2** Data generation at scale and the classical refit on it
    - [ ] **F.2.1** `[V]` Data generation at scale
      Unique positions sized by learning curve, by-game splits, manifests and
      hashes, maintainer-run (archived 11.0.b). Reference forms from Rarog's
      F.0 and F.2: node-limited self-play with soft and hard caps, opening
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
    - [ ] **F.2.2** `[V]` Classical refit on search-visited data
      Added 2026-10-24 from Rarog's open plan (its F.2.1, 2026-10-22,
      `analysis/phase_c_review_2026-10-19.md` question 3). Rarog's same-search
      meter left 99 Elo that was not in the evaluation families, which read at
      static parity with the donor on played positions (RAR-E70), but in how
      the evaluation scores the positions the search visits. F.2.1's data is
      those positions with deep labels. The frozen classical evaluation's
      whole surface is refitted on it with C.2's protocol, screened statically
      on held-out rows and the frozen test, and gated as an evaluation change
      against the strongest classical release, with the same-search meter
      re-read at the close. A win is released as the classical line's next
      version; a loss closes the classical programme `NO_CHANGE`.
- [ ] **F.3** `[I2]` Trainer hardening and baseline nets
  Deterministic pipeline, two seeds per configuration, validation selects,
  test opened once (archived 11.0.a, 11.1).
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
  - Judging checkpoints (TD-12): at 20+0.2 or longer (one net read −27 at
    8+0.08 and −84 at 20+0.2) and against one fixed anchor
    (checkpoint-to-checkpoint gains summed to +71 where the direct gap
    closed by +28); the first epochs after a restart at a higher rate are
    recovery, not progress; a validation loss computed at a moving λ cannot
    rank checkpoints.
- [ ] **F.4** `[I2]` Scalar integration, integer-exact conformance
  Scalar integration: the network file contract and integer-exact
  trainer/engine conformance; clean HCE fallback (archived 11.2).
  Prior: tag `archive/nnue-local` holds the 2026-07 scalar `.mnn` loader
  and its conformance test; F.4 decides whether any of it is reused.
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
- [ ] **F.6** `[V]` Search re-fit for the network
  Score scale, correction, margins, qsearch and SEE thresholds, using C.10's
  protocol (archived 11.4.a–b); BAS-R02, R03, S08 and S09's retry triggers are
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
  **2.0.0** (archived 11.4.c, 11.5; maintainer decision 2026-10-24). Open
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
  - Chess960 (TB-9): X-FEN and Shredder-FEN, castling written as the king
    taking its rook, the occupancy updated by XOR so king and rook may share
    squares, the FRC perft suite; B.7.2 keeps the board able to carry it.
  - An analysis build (TA-1): the second root move always searched with a
    full window, paused in quiet positions; the more direct of two clear
    wins played; the analysis search compiled as a second copy so games pay
    nothing.
  - The printed scale calibrated so +1.00 is a 50% chance to win, refitted
    per evaluation; tablebase wins printed as `cp 20000` minus the distance
    (TA-2). Strength levels by search limits and line choice, calibrated in
    games (TA-3).
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
  B.6.1 input (2026-11-05): a departures row that reads under +5 there is
  not cleared by a run on one fitted arm; it waits here for the fair
  re-read with both arms refitted.
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

## 4. Measurement protocols

| Meter | Protocol | Owner |
|---|---|---|
| Pool score | Colosseum, fixed pool with Houdini 3, `3+0.03`, UHO, no adjudication, 400 games per pair, 1T and 4T | A.7, B.11.1, C.11, E.2 |
| Search deficit G(0) | Paired, head against the frozen classical-Stockfish search oracle rebuilt with the head's HCE, 3,000 games, equal time, no adjudication | A.7.3, B.11.1 |
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
  replacing 2026-09-28's single release at E.3): **1.11.0** after B (B.11.2),
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
