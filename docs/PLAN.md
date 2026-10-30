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
| Pool, 4T | 1.10.1 performs at **3040** [3026, 3054] at 4T against the same four targets and Rarog 2.4.0, priced at their 1T ratings (400 games each, no adjudication): Houdini 3 **−203**, Critter 1.6a **−129**, Fritz 16 **−177**, Rybka 4.1 **−56**, Rarog 2.4.0 **+14**. Rarog 2.5.0 scores **+312** against it | BAS-M11; BAS-M12 |
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

The evaluation is frozen until the search checkpoint (B.9). The evaluation
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
     (tag `oracle/hybrid`), and a seed column for eval-coupled margins.

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
   thread in each non-ponder game and calibrate a null pair first.

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
| SEE king legality repaired; created pins and promotion recaptures are known defects with fixtures, repaired in D.3.1 | BAS-C08, BAS-C09; `tests/test_board.cpp` |
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
| B.3 | B-12 a move stored at fail-low nodes; B-14 correction training refuses decisive results; B-45 the singular margin's range excludes the donor's value; B-109 ablation bit 6 also removes the check extension |
| B.4 | B-17 the dead SEE clamp |
| B.5 (with D.1) | the fail-high stop's hole: a window that never returns is never checked (A-47; `analysis/b5_failhigh_stop_2026-10-25.md` §2.3) |
| C.2.1–C.2.2 | C-1 the tuner's missing scalers; C-2 `--verify` cannot fail; C-8 the extractor's de-dup ignores the clock; C-26 to C-30 the data pipeline (book, sampler, split, labels, row identity); C-49 K refitted per run |
| C.3–C.7 | C-10 outpost ranks; C-11 a rear doubled passer scored as a passer; C-12 "opposed" read over the whole file; C-13 own-colour king blockers only |
| D.1, D.4 | A-46 reserve arithmetic; A-57 cold table reads on the clock; A-58 and A-59 tablebase root ranking |
| D.2 | BAS-C05 the TT key and payload published separately |
| D.3.1 | A-2 and A-3 SEE's created pins and promotion recaptures (BAS-C09) |

## 3. Required evidence

| Change | Minimum gate |
|---|---|
| Tool, docs or refactor | Syntax/static checks; focused tests; full CTest and bench when execution semantics can change |
| Behaviour-neutral hot path | Exact immediate fingerprint, targeted parity, pooled/interleaved PGO NPS on an idle host |
| Correctness repair | An independent invariant that fails on the old behaviour; a strength gate when deployed play changes materially |
| Search cluster | Umbrella-off exact fingerprint at every commit; release and sanitizer CTest on both arms; the rule-8 screen ladder; the unfitted 2,000-game paired run; a registered SPRT on the fitted final-PGO build |
| Evaluation cluster | `EvalTrace` coverage and reconstruction; a whole-surface refit on a frozen corpus with an untouched test split; clean-PGO SPRT; endgame truth vetoes |
| Endgame evaluator | Deterministic truth cohort, paired WDL/DTZ, conversion floors, promotion/material-shed closure, an occurrence-tiered game gate |
| Time, root, SMP | 1T STC, 1T `10+0.1`, 4T `10+0.1`, zero forfeits, topology and hash recorded |
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

- [ ] **B.3** Cluster 2 — pruning and extensions
  Cluster 2 — proof searches and extensions: NMP (cut-node population,
  improving and prior-fail-high margin, dynamic reduction, verification
  region), ProbCut (qsearch pre-verification, margin-scaled depth, the small
  TT ProbCut), singular extensions (double and triple margins, multi-cut,
  negative extension, shuffling guard), IIR and Basilisk's check extension;
  SPRT `[0,3]`.
  Opens in the standard cluster shape: B.3.0 research on the accepted B.2
  head (re-basing B.0's handoff), B.3.1 implement, B.3.2 diagnostics and
  paired run, B.3.3 sweep and SPSA, B.3.4 gate. Known risk: Rarog's donor
  multi-cut rule failed its KBNK drive test and Rarog kept its own rule.
  Basilisk's mate-drive floors are at least as fragile: literature seeds
  have tripped them eight times.
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
    and LMR (B-12, `:1219-1223`); IIR's range widened off its rail (B-55,
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
    beside B.2.3's, settled before B.6: a shadow counter of cross-colour
    hits per table, a colour-indexed build read on first-move cutoffs,
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
    cycle against a stored decisive bound (A-13, D.3.1), so the trigger
    fires here. The donor's form, after `is_draw` (`:347`) and before
    the tablebase and TT probes, and in quiescence after `:122`: when
    `alpha < draw` and a repetition is reachable, alpha rises to the
    draw score and the node returns on `alpha >= beta`. It couples to
    the TT cutoff's graph-history guard, mate-distance pruning, the
    fixed draw value (B-84) and the board's key history (D.3.1). Prior:
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
    - [ ] **B.3.1** `[I2]` Implement behind the umbrella switch
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
    - [ ] **B.3.2** `[V]` Diagnostics and the unfitted paired run
      Packet §11, BAS-S41: the zero-game screens on both arms (counters,
      the oracle differential, WAC, the crossover band, the canary
      calibration, pooled NPS, the cost screen), the 2,000-game paired
      run that governs (P1, the −40 stop), then the categorical paired
      runs in §9's order, two of them at `10+0.1` as well; the §5.12
      surface read decides which Cluster 1 coordinates B.3.3 fits.
    - [ ] **B.3.3** `[V]` Curvature sweep, then SPSA in blocks
      Packet §11, BAS-S42: five registered sweep coordinates (the
      singular margin with the donor's value as an extra point), then
      SPSA in blocks on Cluster 2's X lines plus §5.12's admissions;
      theta is the last completed block; the fitted-constant analysis
      precedes the bake.
    - [ ] **B.3.4** `[V]` Cluster 2 gates
      SPRT `[0,3]` for the cluster, and each categorical and deletion arm
      at the bounds the packet registers.
      Packet §11, BAS-S43: (a) the fitted ON arm against the OFF arm,
      `[0,3]` at `3+0.03`; (b) the fit against the unfitted arm, `[0,3]`,
      only if B.3.3 ran its SPSA; (c) a 2,000-game `10+0.1` direction
      read, recorded, not a gate.
- [ ] **B.4** `[I2]` Cluster 3 — quiescence
  TT cutoff, corrected stand-pat with interpolated fail-high, futility base,
  move-count and SEE pruning, evasions only in check, TT write on exit; SPRT
  `[0,3]`.
  Opens with B.4.0 research, which carries two cards:
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
- [ ] **B.5** `[I2]` Cluster 4 — root and aspiration
  Cluster 4 — root, aspiration and iterative deepening: aspiration centred on
  the root move's average score with width from its mean squared score,
  optimism, root-move effort, the `pvIdx` root structure with a `MultiPV`
  option, and deep-iteration line growth; SPRT `[0,3]`.
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
    iteration's best, split by fail-low and other, at 1T and 4T; the
    helper side is D.2's.
- [ ] **B.6** `[V]` Joint search SPSA, if justified
  Joint search SPSA over the coordinates the four clusters left live, only if
  their curvature evidence and results justify it; registered surface; PGO
  bake; SPRT `[0,3]`.
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
- [ ] **B.7** Search speed pass
    - [ ] **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2
      Allocation guard (ANY TIME) — must land before B.7.2: a test with a
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
    - [ ] **B.7.2** `[I1]` Speed pass
      Behaviour-neutral throughput work on the new modules, with a pooled-PGO
      floor of +0.5% per change and B.7.1's guard as the other floor.
      The archived board-speed leaves (7.2–7.8: generation and list
      delivery, fused relocation, pin/check sharing, the SEE kernel,
      representation) open only if a profile of board work inside HCE
      search makes them hot, and default to no-change.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - Uses A.9.4's NPS read and A.9.5's profiler.
      - Candidates from Rarog: the child TT prefetch before the make from a
        predicted key (D-5, +1.69%) and at pick time (D-6, +0.5% and +0.3%
        under PGO); large pages, pulled forward from G.1 (D-8: +2.48% bench,
        +2.76% in games; `VirtualAlloc(MEM_LARGE_PAGES)` under
        `SeLockMemoryPrivilege` with a silent fallback, `bench` reporting on,
        off or partial); the per-stage move-list copy (D-10; Rarog's in-place
        list +6.93%); a survivor count for the quiet early exit (D-11,
        +8.85%); attack lookups (D-12); thin against full LTO (D-32).
      - Measured by A.17's profile (2026-10-30, `bench 13` after A.16): the
        material imbalance recomputes 48 products of piece counts on every
        evaluation, 2.89% of a node, and depends on material alone (a
        material-keyed cache, Stockfish's material table); material and PST
        are summed from scratch over every piece, 1.37% (incremental in
        make and unmake). Slider lookups are 2.7%, and compressing their
        tables read 0.98% slower (BAS-P26): leave them flat.
      - Rules: re-read 1T/4T bench scaling after speed or large-page work
        (D-9: Rarog's 4T/1T ratio fell 3.72 to 2.60, unseen by its
        head-to-head); read a bundle's members alone (D-16); a maintainer rule
        for real but sub-bar gains (D-17); screen with PGO (D-14); priors D-7
        (most prefetches lose under 4 KB pages), D-15 and D-20.
- [ ] **B.8** Cleanup of legacy search and dead parameters
  Cleanup: remove the legacy search path, dead parameters, unconsumed switches
  and diagnostics without an owner; exact fingerprint; no game gate.
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
  - Delete the dead shallower re-search (B-63) and the probably-dead PVS guard
    (B-13, counted first); read NPS after the deletions, which paid +1-6% in
    Rarog (D-18); Rarog's cleanup pitfalls (B-120); retired numbers in
    comments go with A.10.6.
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
- [ ] **B.9** `[V]` Search programme checkpoint
  Checkpoint: re-measure every deficit meter.
  The meters are G(0) against the oracle, fixed-node depth and branching,
  pooled NPS, the conversion instrument and the endgame truth cohort, plus a
  1T pool gauntlet against the four targets and Rarog (400 games each).
  Record the attributed Elo per accepted cluster and the checkpoint against
  the budget table, remove the ablation build, then freeze the search head
  for C.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - The pool read includes the head on the match host: Rarog's pool still
    holds Basilisk at 1.10.0's 2993, and neither 1.10.2 nor the head has been
    played there (audit, tournaments). Ablation limits (B-110) and bit 6's
    asymmetry (B-109) in the readings. B.11 follows.
- [ ] **B.11** `[M]` Interim release after the search programme
  Maintainer decision 2026-10-24, replacing 2026-09-28's no-interim rule: the
  search programme's head is released when B.9 closes, as Rarog cut 2.5.0 at
  the end of its search programme. Licensed by an accepted SPRT over 1.10.2 at
  `3+0.03` 1T with `10+0.1` and 4T direction checks whose 95% intervals
  exclude a loss, and a 4T pool read beside them: a head-to-head read cannot
  see a thread-scaling loss both versions share (Rarog RAR-M74, RAR-M76).
  Version 1.11.0 (maintainer decision 2026-10-24: releases follow B, C and
  E, and 2.0.0 is the first NNUE release). Cut through A.8.6's tag-driven
  flow after A.10.4.
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

### Active workflow register

One row per open leaf of the current phase. The checker requires every open
leaf of the current phase to have a row, and each row's class to match the
leaf's tag. Later phases carry only a class until they open.

| Leaf | Workflow state | Class | Current decision |
|---|---|---|---|
| B.3.1 | RESEARCH | I2 | Returned at ticket 6 (packet *Return 2*: every singular firing a triple, P8a outside its band); resumes on research amendment 2 |
| B.3.2 | RESEARCH | V | Waits for B.3.1's accepted review |
| B.3.3 | RESEARCH | V | Waits for B.3.2 |
| B.3.4 | RESEARCH | V | Waits for B.3.3 |
| B.4 | RESEARCH | I2 | Contents fixed by B.0 |
| B.5 | RESEARCH | I2 | Contents fixed by B.0 |
| B.6 | RESEARCH | V | Only if the clusters' curvature justifies it |
| B.7.1 | READY_FOR_IMPLEMENTATION | I1 | Any time between leaves; must land before B.7.2 |
| B.7.2 | RESEARCH | I1 | After B.7.1, on the new modules |
| B.8.2 | RESEARCH | I1 | After the clusters |
| B.9 | RESEARCH | V | Programme checkpoint |
| B.11 | RESEARCH | M | Interim release after B.9 (maintainer decision 2026-10-24) |

## Phase C — Evaluation programme (search frozen)

**Goal:** recover the measured same-search evaluation deficit, with the search
frozen at the B.9 head. The families are re-implemented in classical
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
an unchanged search. C.0 measures residuals by cohort on the B.9 head before
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
  Investigation: the family map refreshed on the B.9 head, residuals and
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
    screen before C.7 builds its list (C-118); space has no owner (C-119);
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
    change together (C.7 and C.3), never one at a time.
  - E8: space (C-119) is computed at every phase with no npm gate and
    lacks the donor's bonus for squares behind own pawns; its handoff
    carries E6's sign.
  - The term inventory, donor against Basilisk with status and lines, is
    §6.3's table; C.0's family map starts from it.
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
        slots and its tuner group; C.5.3's complexity term needs an
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
      Generate and publish corpus A with the B.9 head at a fixed node budget,
      natural termination, maintainer-run; audit the returned composition and
      publish it immutably (archived 8.3).
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - The datagen wrapper checks faults, refuses book wrap and unregistered
        binaries (C-32); the realised opening sequence pinned, a move-for-move
        pilot for a harness port (C-76); the label audit re-run on the B.9
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
- [ ] **C.3** `[I2]` King safety cluster
  King safety cluster in the donor's shape: attacker units and weights, safe
  and unsafe checks by piece, weak ring squares, flank attack and defence,
  shelter and storm with the castling alternative, the queenless reduction and
  the nonlinear danger map; refit; gate.
  Opens as sub-steps (research, implement, refit, gate) when C.0's handoff
  reaches it; the same holds for C.4, C.6 and C.7.
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
- [ ] **C.5** Endgame handling and winnability
  The goal is measured conversion and correct draw recognition where games
  actually go, not coverage of a function list.
    - [ ] **C.5.1** `[R2]` Instruments and gate integrity
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
    - [ ] **C.5.2** `[R3]` Occurrence, classification and ranking
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
      - Code audit 2026-10-27 (§6.3 E7): correction history overrides
        exact recogniser scores — the drawn KPK, KNNK and wrong-bishop 0
        and the scaled endings are corrected through pawn-, minor- and
        non-pawn-keyed tables (`search_kernel_core.cpp:195`, `:508`;
        `search_history_core.cpp:158-174`) whose keys won and drawn
        positions share, so a dead draw can stand at ±X. Measure the
        correction's size at recogniser returns; the ablation (skip the
        correction when the score came from a recogniser) is C.10's
        switch.
    - [ ] **C.5.3** `[I2]` Generic winnability and scaling
      Generic winnability and scaling in the donor's form: pawn-count scaling,
      opposite-bishop scaling, rule-50 inside the scale and a complexity term;
      floored so a misfire discounts rather than throws a win.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - OCB: neutral from four pawns, no passer term, fires with other
        material (C-4; Rarog's `1 + 2 pawns + 10 passers` read +12.7 nElo);
        OCB below seven men (C-125); every scale floored (C-126); rule-50
        damping against the drive weights (C-128); initiative's unfitted
        `win_*` weights (C-47; A.16 removed the term and its slots, so C.5.3
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
    - [ ] **C.5.4** `[R3]` Won-rook-ending only-move precision (BAS-E53)
      Won-rook-ending only-move precision (archived 6.8.a; BAS-E53): all 68
      one-winning-move nodes studied with a held-out split; search, evaluation
      and knowledge explanations compared; a mechanism or a justified
      no-change. More depth is already refuted.
    - [ ] **C.5.5** `[I2]` Group B families by measured kind
      Group B families by measured kind (archived 6.8.b–e).
      The families: KR-KP, KQ-KRP and KR-KB; bishop-pawn fortresses and
      promotion races, with the KBP-K deficit treated as bishop-pawn
      technique. Deterministic theory, truth and closure cases come before
      fitting, each shown to fail on its defect. The covariant local cluster
      is fitted on training and validation data only.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KR-KN and KR-KB at a flat 4/64, and KRP-KB unscaled (C-129).
    - [ ] **C.5.6** `[R2]` Lower-yield remainder
      Lower-yield remainder (archived 6.10.a–d): KPs-K and KP-KP first; KQ-KP
      and KQ-KR reconciled across budgets; KR-KN, KNN-KP and KNN-K theory and
      rule 50; implement only what has a measured defect, occurrence and a
      valid closure.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - KNN-KP: an additive drive plus a blockade term, never a replacement
        score (C-130); the KXK corner-to-king ratio (C-39); the KBN-K drive's
        speed and a distance-to-mate table option (C-144).
    - [ ] **C.5.7** `[V]` Endgame gate and closure
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
- [ ] **C.6** `[I2]` Pawns and passers cluster
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
- [ ] **C.7** `[I2]` Material, imbalance, phase and pieces cluster
  Imbalance in the donor's quadratic form, the phase interpolation, bishop
  pair and bishop-pawn colour, outposts (including BAS-E07's absent
  `BadOutpost`), long diagonal, rook files, trapped rook, queen weakness and
  king-protector distance; refit; gate.
  Outposts, `reachable_outpost`, mobility and `queen_infiltration` were
  fitted around seven rank-mirrored piece-square tables (B.10). Their trace
  reads them for compensation the flip left behind.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Outposts on ranks 5-8 against the donor's 4-6 (C-10); dead and collinear
    imbalance slots (C-20); queen infiltration and the trapped bishop (C-48);
    no trapped rook (C-46); no WeakQueen (C-116); screen first (C-118).
  Code audit inputs (2026-10-27 second pass, §6.3):
  - E2's phase: pieces count N = B = 1, R = 2, Q = 4 of 24 against the
    donor's npm between its endgame and middlegame limits; changed with
    C.3's eg king danger, never alone, and with tempo (E5: tempo is
    mg-only, 22, so it tapers to 0 in pawn endings and is scaled with
    the score, where the donor adds a constant 28 after scaling; the
    lazy path omits it, C-6).
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
  before B.10's flip, which moves it by 31.7 cp mean. B.10's gate does not
  refit them; this leaf does.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - `LAZY_MARGIN` refit (C-98); correction-magnitude terms on the surface
    (B-69); search and evaluation never share a tune (B-107); `scale_ratio.py`
    after B.10 and C (B-108); the qsearch margins as coordinates (B-25); a
    runtime lazy margin clears the caches (C-42).
  - Code audit 2026-10-27: C.5.2's E7 switch — skip the correction when
    the evaluation came from a recogniser — is read here with the search
    surface.
- [ ] **C.11** `[V]` Freeze the classical evaluation
  Checkpoint and freeze (archived 8.11).
  - Re-measure the same-search deficit against a fresh classical-Stockfish
    hybrid at the C head, plus conversion, the truth cohort, NPS and a 1T
    pool gauntlet.
  - Revalidate the score scale, mate bounds and every endgame floor.
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
  - Audit the lazy-eval thresholds as a separate, behaviour-changing
    question.
  - Gate with interleaved pooled NPS and a natural-termination SPRT.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Played and full scores dumped (C-89); cache purity (C-105); a runtime lazy
    margin clears the caches (C-42); the profile comes from A.9.5.
  - Code audit 2026-10-27 (§6.3 E8, E9): the lazy cut is one stage at
    700 on the tapered cheap score with no npm term, where the donor
    uses two stages with a margin growing by npm/64 (about +260 at full
    material), and it skips king danger, threats, mobility, passer
    proximity and path, the mop-up drive and tempo; there is no
    material or imbalance cache, so the twelve-popcount census and the
    imbalance are rebuilt on every call (the pawn cache is pure). Both
    go into the lazy audit.
- [ ] **C.13** `[M]` Interim release after the evaluation programme
  Maintainer decision 2026-10-24: the evaluation programme's head is released
  as 1.12.0 once C.12 closes, as Rarog cut 2.6.0 from its C head. Licensed as
  B.11 is: an accepted SPRT over the B.11 release at `3+0.03` 1T, `10+0.1` and
  4T direction checks whose 95% intervals exclude a loss, and a 4T pool read
  beside them. Cut through A.8.6's tag-driven flow.

## Phase D — Clock, threads, robustness

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
    stop beyond mate in 1 (A-49, B-78); a first-iteration stop guard and
    abort-sweep tests (A-50, A-65); ponder credit (A-42); time changes read by
    the opening's time a move and the clock left (A-54); the fail-high stop's
    threshold if B.5's read calls for it (A-48).

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
- [ ] **D.2** `[R2]` Lazy SMP quality
  Self-relative scaling (each engine against itself at 4T and 8T versus 1T:
  Basilisk, Stockfish, Rarog) before any design.
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
- [ ] **D.3** Engine lifecycle, protocol and board contracts
    - [ ] **D.3.1** `[R2]` Board contract audit (ANY TIME) — before E.1
      Board contract audit (ANY TIME) — must land before E.1 (archived 7.1,
      7.7, 7.9–7.12): port the snapshot's board-v2 corpus and oracle
      (`tests/data/`, `tools/diag/board_v2_*.py`); this is a defect hunt, not
      a speed item.
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
        corpus (A-5, A-24); the SEE approximations (A-2, A-3, BAS-C09) are
        known defects, repaired here.
    - [ ] **D.3.2** `[R2]` Lifecycle and protocol robustness
      Lifecycle and protocol (archived 9.4): worker start, stop and join,
      cancellation, result authority, new-game and option resets, command
      ordering, Syzygy thread and halfmove contracts; deterministic
      interleavings plus stress; zero crashes over the pool tournaments.
      Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`;
      IDs are its items):
      - A deep-line stack test (A-25); abort-sweep tests (A-65); command
        fuzzing (A-67); MultiPV stopped-search tests (D-73); seldepth excludes
        qsearch plies (B-122); 1.2.3's two illegal-move games replayed on the
        current binary (A-69); Fathom thread safety (A-61). The CPU-tier
        message and the slow-PEXT advisory (A-36, D-25) moved to A.15.3.
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
- [ ] **D.4** `[R2]` Tablebase policy
  Probe depth and limits, WDL and DTZ in conversion, interaction with C.5's
  recognizers; the root and PV lines already follow Stockfish (BAS-C13), and
  in-search probing does from A.8.14.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Cold tablebase reads at the root on the clock (A-57); the root ranking
    ignores repetitions already in the game (A-58); DTZ ranking (A-59).
  - A-58 has cost a game: BAS-S39's game 4965 (`tools/results/a1134-tb-gate/`)
    drew a clean tablebase win by threefold repetition, the root reporting
    the win at every move; 1 of 2,286 clean wins in that run.
  - Code audit 2026-10-27 (S11): A.13.4 moves the in-search WDL probe
    after the TT probe and cutoff, the donor's order; probe depth and
    limits stay here.

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
  Attribution checkpoint (archived 9.9): the final head against 1.10.1 and the
  B.9 and C.11 heads at STC, `10+0.1` and 4T; attributed Elo per programme
  from the accepted SPRTs; every deficit meter; NPS; the maturity checklist.
- [ ] **E.2** `[V]` Classical target gate
  Target gate: the pool measurement of section 1 at 1T and 4T. Met, or not met
  with the measured shortfall per engine recorded.
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

- [ ] **F.0** `[R3]` NNUE investigation
  Investigation (archived 10.0–10.3): board events and accumulator ownership,
  the trainer and its quantisation and export contracts, data format and
  splits, the first architecture (at least Gyatso's single-bucket 768×1024
  perspective net, CCRL 3258 at 40/15) and the cost ledger; frozen handoffs
  for F.1–F.4.
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
- [ ] **F.4** `[I2]` Scalar integration, integer-exact conformance
  Scalar integration: the network file contract and integer-exact
  trainer/engine conformance; clean HCE fallback (archived 11.2).
  Prior: tag `archive/nnue-local` holds the 2026-07 scalar `.mnn` loader
  and its conformance test; F.4 decides whether any of it is reused.
  Embedding (code audit 2026-10-27): `#embed` puts the network file in
  the binary without a generated object or a resource step; it is
  C++26 but available as an extension in C++23 mode from GCC 15 and
  Clang 19, below the release matrix's floor (G.2's C++26 note).
- [ ] **F.5** `[I2]` Incremental and SIMD inference
  Same-net parity on every move type, SIMD tiers with the scalar reference
  retained, cost attribution (archived 11.3).
- [ ] **F.6** `[V]` Search re-fit for the network
  Score scale, correction, margins, qsearch and SEE thresholds, using C.10's
  protocol (archived 11.4.a–b); BAS-R02, R03, S08 and S09's retry triggers are
  reviewed here.
- [ ] **F.7** `[R3]` Architecture ladder, one axis at a time
  Architecture ladder: output buckets, king buckets with mirroring, then
  relation and threat inputs, one axis at a time with multiple seeds; each net
  gated against the previous (archived 12.0.b).
- [ ] **F.8** `[V]` Data frontier
  On-policy refresh with the strongest net, deduplication, hard-position
  mining; repeat while a cycle accepts (archived 12.0.a, 12.0.c).
- [ ] **F.9** `[M]` NNUE release
  Beat the classical release at STC, LTC and 4T; platform matrix; version
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
  - Large pages moved to B.7.2 (D-8).
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
- [ ] **G.3** `[R3]` Frontier
  Larger nets, data scaling, search fit at LTC, rejected classical mechanisms
  reopened only where the NNUE fires their retry trigger (archived 12.1); the
  CCRL top-50 gate.

## 4. Measurement protocols

| Meter | Protocol | Owner |
|---|---|---|
| Pool score | Colosseum, fixed pool with Houdini 3, `3+0.03`, UHO, no adjudication, 400 games per pair, 1T and 4T | A.7, B.9, C.11, E.2 |
| Search deficit G(0) | Paired, head against the frozen classical-Stockfish search oracle rebuilt with the head's HCE, 3,000 games, equal time, no adjudication | A.7.3, B.9 |
| Evaluation deficit | Hybrid at the head against the classical-Stockfish-HCE hybrid, same search | C.11 |
| Cluster acceptance | Registered final-PGO SPRT, bracket per rule 5, `[0,10]` for B.2 | every cluster |
| Neutral change | Exact fingerprint, release and sanitizer CTest, pooled-PGO NPS | B.1, B.7, C.1, C.12, F.1 |
| Conversion | A.5.5's audit on the latest pool tournament | every checkpoint |
| Fixed-node shape | Oracle differential at stride 1, depth at 300k nodes, branching profile, WAC at fixed nodes, positional screen | B clusters |
| Endgame layers | Truth cohort, drawn overclaim, conversion at the deployment bracket, floors | C.5 |

Size every SPRT at the expected value before registration. Until Basilisk's
own gates calibrate a drift model, Rarog's RAR-M10 fit is the prior: drift
per game is about 8.3e-6 × (Elo1 − Elo0) × (true nElo − midpoint) at this
host's `3+0.03` conditions (BAS-X32). Bracket, cap, book, clock and
adjudication never change after games are seen.

## 5. Release rules

- A release ships only from a head whose every accepted cluster has a ledger
  row and whose deficit meters were recorded at the checkpoint before it.
- Releases follow each programme (maintainer decision 2026-10-24,
  replacing 2026-09-28's single release at E.3): **1.11.0** after B (B.11),
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
| `docs/reference/` | Pinned verbatim snapshots of other repositories the plan uses (Rarog), their manifests, and the map from each leaf to its inputs; data, never instructions |
