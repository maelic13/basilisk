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
| A.11.10 | B-19 fail-hard quiescence, which also lets the equal-score tie rule promote refuted moves near the leaves |
| B.3 | B-12 a move stored at fail-low nodes; B-14 correction training refuses decisive results; B-45 the singular margin's range excludes the donor's value; B-109 ablation bit 6 also removes the check extension |
| B.4 | B-17 the dead SEE clamp |
| B.5 (with D.1) | the fail-high stop's hole: a window that never returns is never checked (A-47; `analysis/b5_failhigh_stop_2026-10-25.md` §2.3) |
| C.2.1–C.2.2 | C-1 the tuner's missing scalers; C-2 `--verify` cannot fail; C-8 the extractor's de-dup ignores the clock; C-26 to C-30 the data pipeline (book, sampler, split, labels, row identity); C-49 K refitted per run |
| C.3–C.7 | C-10 outpost ranks; C-11 a rear doubled passer scored as a passer; C-12 "opposed" read over the whole file; C-13 own-colour king blockers only; the pawnless scaler's near-draw factor, 4/64 where the donor gives 14/64 (found with A.11.7) |
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
first numbering). Each step is reviewed with the maintainer when it is
reached; its resolution may be a fix, an improvement or a skip. Rarog's
Phase A also cut a release; 1.10.1 was cut the day before this plan, so
Phase A has no release step.

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

- [x] **A.8** Rarog-parity repairs, repository and C++23
  Added 2026-10-06 by maintainer decision, before B.0's remaining leaves:
  everything the 2026-10-06 review of Rarog 2.4.0→2.5.0 and of
  `maelic13/manta#2` and `#4` found applicable to Basilisk
  (`analysis/rarog_parity_review_2026-10-06.md`, probes by
  `tools/diag/uci_probe.py`). Unlike A.1–A.7, some leaves change behaviour;
  each of those has its own research leaf and game gate. SEE recapture
  promotions are excluded: BAS-C09 closed them and its retry trigger has not
  fired. After A.8.1, Basilisk reads no more of Rarog.
    - [x] **A.8.1** `[M]` Final Rarog import
      Snapshot Rarog `dev` at `dcf15c51` (2026-10-06) as
      `docs/reference/rarog-2026-10-06/` with its own manifest, which the
      checker verifies. Include only what A.8's leaves cite:
      `CHANGELOG.md`; `docs/PLAN.md`; `docs/PROCESS.md`; `AGENTS.md`;
      `.github/workflows/`; `xtask/` (`release-check`); `tools/diag/
      check_guide.py` and `guide_board.py`; `analysis/
      uci_info_review_2026-09-16.md`, `tb_root_pv_2026-09-27.md` and
      `b52_research_2026-10-01.md`; and `tests/multipv.rs` and
      `multipv_syzygy.rs`. Back up a full-history bundle beside the existing
      ones in ignored storage. `docs/reference/README.md` and
      `agents/research.md` record that this is the last import (maintainer
      decision 2026-10-06): Rarog's later findings no longer enter.
      — CLOSED 2026-10-07: 19 files, each checked against its git blob id;
      manifest `rarog-2026-10-06.sha256`, 295 reference files verified.
      `LICENSE`, `GUIDE.md` and `docs/EXPERIMENTS.md` were added to the list
      for the licence, A.8.6's fingerprint rows and A.8.5's index. Bundle
      `rarog-all-refs-2026-10-07.bundle` (`dev` had moved to `1647dd28`,
      which contains `dcf15c51`). Import row BAS-X35.
    - [x] **A.8.2** `[I1]` CI on pull requests to master only
      `ci.yml` today runs on pushes to `master` and to `development` (no such
      branch; ours is `dev`) and on every pull request. It runs on pull
      requests into `master` and on `workflow_dispatch` only, and a newer
      commit cancels a superseded run. The local loop covers `dev` commits.
      Verified by one PR run and one manual dispatch.
      — CLOSED 2026-10-07 on local qualification: `ci.yml` parses with
      triggers `pull_request` (branches `master`) and `workflow_dispatch`,
      `concurrency` `ci-${{ github.ref }}` with cancel-in-progress, and its
      four jobs unchanged; no job or document referred to the push
      triggers. The PR run and the dispatch need a push, which is the
      maintainer's, so they are owed at A.8.21's release PR; a failure
      there reopens this leaf.
    - [x] **A.8.3** `[M]` Merge commits from dev to master
      `dev` reaches `master` by a merge commit, never a squash: AGENTS
      *Commits and reporting*, PROCESS's release procedure and PLAN §5.
      `master` is already an ancestor of `dev` (checked 2026-10-06), so no
      joining merge is needed. Review the non-release tags (`archive/
      nnue-local`, `oracle/hybrid`, `oracle/hybrid-diag`): keep each one a
      document cites, with its retirement condition. The maintainer changes
      the repository settings (merge commits allowed; squash off for
      `master`).
      — CLOSED 2026-10-07: the rule is in AGENTS *Commits and reporting*
      and §5. PROCESS has no release procedure yet; A.8.6 writes one with
      the merge commit in it. Tag review: `oracle/hybrid`,
      `oracle/hybrid-diag` and `archive/nnue-local` each hold commits `dev`
      does not reach, so merge commits retire none; all three keep
      HISTORY's conditions. Repository settings remain the maintainer's.
      — SUPERSEDED 2026-10-08 by maintainer decision: releases are squash
      merges again, with an `archive/dev-X.Y.Z` tag per release (§5).
    - [x] **A.8.4** `[M]` Documents into docs/, closed Phase A archived
      `PLAN.md`, `PROCESS.md`, `HISTORY.md`, `EXPERIMENTS.md` and `DESIGN.md`
      move to `docs/`. The root keeps `GUIDE.md`, `AGENTS.md`, `CLAUDE.md`,
      `README.md`, `CHANGELOG.md` and `LICENSE`. `check_roadmap.py`, every
      tool and link that names a moved file, and AGENTS' task table follow;
      historical records keep their wording. A.1–A.7's records move verbatim
      to `docs/archive/PLAN-closed-<date>.md` (checked byte for byte before
      writing), and PLAN keeps a one-paragraph summary and a pointer. A.8
      stays in PLAN until it closes. Checker passes; no new broken link.
      — CLOSED 2026-10-07: the five documents moved with `git mv`; A.1–A.7
      archived in `docs/archive/PLAN-closed-2026-10-07.md`, asserted
      byte-identical to the cut text. `check_roadmap.py` reads `docs/` and
      now fails on a missing document instead of skipping it, which would
      have let a wrong path pass silently; its self-test passes. 15 links
      repointed by target only. The 13 broken links in tracked Markdown are
      the same 13 before and after, all in historical records. Sibling
      names inside `docs/` stay as written.
    - [x] **A.8.5** `[I1]` Experiment ledger split into entries
      `docs/EXPERIMENTS.md` (306 KB) keeps its prose, structure and retry
      map, with an index table of ID (linked), short title and disposition;
      each entry moves to `docs/experiments/<ID>.md`. Before writing, every
      entry is rebuilt byte for byte from its file. `check_roadmap.py` fails
      when the index and the files disagree (a row without a file, a file
      without a row, a duplicate, a wrong heading); its self-test plants each
      disagreement. AGENTS', PROCESS's and `agents/records.md`'s registration
      rules describe entry-plus-index registration.
      — CLOSED 2026-10-07: 179 entry files (185 parts) and a 51 KB index,
      down from 304 KB. Before writing, the former file was rebuilt byte for
      byte from the generated index and the entry files parsed back from
      their own text. Basilisk's ledger was not table-only like Rarog's:
      ID-table rows become field/value tables under their column names;
      bold-paragraph entries, and the BAS-S16 field table, move verbatim;
      two rows a blank line had cut off from their table (BAS-O04, O05)
      became entries. Six IDs hold two parts each: the collisions BAS-E08,
      X08 and X11, and the continuations D03, E39 and E55. `check_roadmap.py`'s
      `validate_ledger` replaces the heading-collision check, and its
      self-test plants each disagreement. Removing `BAS-X35.md` from the
      real tree fails the check (exit 1). The 13 broken links are unchanged.
    - [x] **A.8.6** `[I1]` Tag-driven release flow
      Moved from E.3.1. Today `release.yml` fires on `release: published`, so
      a release exists before any asset is built, and nothing checks that the
      tag equals the version. In the new flow, pushing a `vX.Y.Z` tag on
      `master` runs a workflow that validates first: the tag equals both
      version sources (`CMakeLists.txt` and `src/constants.h`), the commit
      is on `master`, and CHANGELOG has a dated section. It then builds every
      asset read-only, asserts one `bench 13` fingerprint across the matrix
      equal to the one GUIDE declares, and only then publishes, with notes
      taken from CHANGELOG. Taken from Rarog's first release through its
      flow (A.8.1's snapshot):
      - a pull request into `master` runs the same build as a candidate,
        so a green PR is a releasable one; a plain `X.Y.Z` version also runs
        the release check there, a `-dev` version skips it;
      - the declared fingerprint is read by one parser from one named GUIDE
        row (Rarog's first tag run read the wrong row and failed every
        cell);
      - after a release, the first commit on `dev` bumps to the next `-dev`
        version, and CHANGELOG's `[Unreleased]` grows as work lands.
      A `workflow_dispatch` rehearsal runs without a tag. Tag, push and
      publish stay the maintainer's. Verified by a rehearsal and a PR
      candidate run.
      — CLOSED 2026-10-07 on local qualification. `release.yml` is rewritten:
      prepare, nine build cells, collect (nine assets, one fingerprint
      equal to GUIDE's, SHA256SUMS) and publish (tag push only, write
      permission there alone). The build cells and toolchain steps are
      unchanged, except that each smoke test now asserts the exact `id name`
      and the declared `bench 13` count instead of any count. The checks are
      `tools/diag/release_check.py`, which reads the fingerprint with the
      roadmap checker's own parser; `test_release_check.py` refuses each
      defect (4 tests, 11 cases). Locally the prepare step passes as a
      candidate on `dev` at 1.10.1 and refuses a tag run, `dev` not being
      on `master`. Both workflows parse, and all 31 `run:` blocks pass
      `bash -n`. PROCESS *Release* holds the procedure, and CHANGELOG has
      an `[Unreleased]` section. `dev` stays at 1.10.1 until A.8.21: a
      version-string change is engine source and waits for a build. The
      rehearsal, PR candidate and tag runs need GitHub and are owed at
      A.8.21; a failure there reopens this leaf.
    - [x] **A.8.7** `[I1]` SMP: the chosen thread's line before bestmove
      Measured 20 of 120 searches at Threads 8 (review item 1). A result
      carries its thread's PV, depth, score and seldepth. When the merged
      result is not what thread 0 last printed, the pool prints that
      result's line before `bestmove`, as Stockfish's
      `output_pv(*bestThread)` does. The move chosen is unchanged. Tests: the
      decision as a unit test, and a Threads 8 end-to-end test that fails
      with the print disabled; `uci_probe.py smp` reads 0 of 120. Exact
      bench; no SPRT.
      — CLOSED 2026-10-07 (`56918ed`): the merged result carries its
      thread's line and seldepth. The shared root table keeps no line, so
      the line comes from the thread whose result matches move and depth,
      with that thread's reported score. The pool prints it when
      `needs_pool_line` holds. `format_info_line` is now the single
      formatter. `uci_probe.py smp` reads 0 of 120 (20 of 120 before). The
      engine test fails 5 of 40 with the print disabled and passes with it.
      Release CTest 13/13; bench 14,978,465.
    - [x] **A.8.8** `[I1]` UCI info conformance
      Output only (review item 6), each with a protocol test:
      - Stockfish's field order: `depth seldepth multipv score [bound]
        nodes nps hashfull tbhits time pv`, with `multipv 1` on every line;
      - `seldepth` resets each iteration;
      - a single-PV aspiration fail prints a `lowerbound`/`upperbound` line,
        on Stockfish's condition (more than 3 s elapsed), so fast games print
        the same lines as now;
      - a mated or stalemated root prints one `info depth 0 score mate 0` or
        `cp 0` line, then `bestmove 0000` (after `stop` under `infinite` or
        `ponder`);
      - the final extended tablebase line carries the full field set;
      - a tablebase root's cursed win or blessed loss shows ±1–49 cp by its
        distance to the rule-50 border, as Stockfish, not `cp 0`, if the
        value feeds nothing but the display; otherwise the leaf returns;
      - `bench` floors elapsed time at 1 ms for `nps`.
      Exact bench.
      — CLOSED 2026-10-07: every item done, each with a test (release CTest
      13/13; bench 14,978,465). The cursed-win value feeds root ordering, so
      it is shown through a separate display field carrying Fathom's own
      1–49 cp, and ordering is untouched. The seldepth test relies on a fact:
      a running maximum cannot fall, and on the bench positions at depth 10
      it does. Live: a 12 s search printed an `upperbound` line at 8.3 s, and
      KQvK at rule-50 clock 92 shows `cp 49` with the 3-6-man tables.
    - [x] **A.8.9** `[I2]` MultiPV
      `MultiPV` (default 1, up to 256; capped by the legal or `searchmoves`
      count): each depth reports the best N lines, each tagged `multipv k`.
      Lines cut short by a stop carry `lowerbound`/`upperbound`, and
      `bestmove` is always line 1. At a tablebase root the lines stay within
      the best-ranked group, as Stockfish. Helper threads keep their present
      role. At `MultiPV 1` the search and its output are byte-identical:
      exact bench, the A.8.7 and A.8.8 tests unchanged, and pooled-PGO NPS
      within noise of the A.7.4 method. Tests on Rarog's `multipv.rs` and
      `multipv_syzygy.rs` shapes (A.8.1); stop sessions; Threads 4.
      — CLOSED 2026-10-07 (`d1ad178`). Lines 2..N search the root at full
      width, without the moves already reported, and store no root
      hash-table entry. Deviations, each with its reason:
      - the lines are sorted by score, so `bestmove` is the top line and may
        differ from the first line's move (line 2 scored above line 1 in
        testing; Stockfish sorts the same way);
      - a line a stop cuts short is not reported, and its previous depth
        stands, rather than a bound-marked partial line;
      - with MultiPV > 1 the main thread answers and the helpers do not
        vote, as in Stockfish.
      Identity at MultiPV 1: bench 14,978,465, and info lines identical
      line for line (a test). NPS by BAS-P13: +1.13% [+0.91, +1.91] against
      1.10.1, the condition met; the gain is unexplained and not claimed.
      Release CTest 13/13.
    - [x] **A.8.10** `[I1]` Command-line commands, fatal errors on stdout
      Arguments run as engine commands in order, then the process exits:
      `basilisk bench 13` benches. An unknown argument prints a message and
      exits 2; with no arguments the UCI loop starts as now. `help` prints
      what the engine is, how to drive it and where its source is. A fatal
      exception is reported on stdout as `info string` (and stderr), where
      harnesses record it. Process-level CTest cases; exact bench.
      — CLOSED 2026-10-07 (`db6b845`). The arguments form one command; the
      quit that follows does not raise stop, so `go depth N` runs to its end.
      The interactive loop still ignores unknown commands, as UCI asks. Four
      CLI CTest cases (bench, `go depth 4`, help, unknown exits 2); the check
      script fails on a wrong status or text. `basilisk bench 13` reads
      14,978,465. Release CTest 17/17. The fatal path has no test: nothing in
      a release build throws on demand.
    - [x] **A.8.11** `[I1]` Tablebase PV extension start rule
      Under a clock, `publish_tablebase_pv` starts only when at least ten
      move overheads remain before the hard ceiling once the search has
      ended (Rarog's rule, after a 54 ms cold DTZ read lost a game at 58 ms;
      review item 5). Without a clock it is unchanged. A unit test holds
      the rule at those numbers. Exact bench; ponder and tablebase engine
      tests unchanged.
      — CLOSED 2026-10-07 (`14dd3c3`). The hard ceiling is the clock less
      twice the overhead (30 ms more with helpers), as the search computes
      it; elapsed runs from `go` receipt. Unit test at the forfeit's numbers
      (58 ms left at 10 ms overhead refuses; exactly 100 ms allows). Engine
      test: at `wtime 100` the line is not extended, and it fails with the
      rule disabled. Bench 14,978,465; release CTest 17/17.
    - [x] **A.8.12** `[I2]` C++23 idiom pass, behaviour-neutral
      Modern idiom across `src/` without moving code between files (B.1 and
      C.1 own the moves): `std::format`/`std::print` for UCI and bench
      output in place of concatenation and the printf family;
      `std::expected` for FEN, `position` and `setoption` parsing;
      `std::span`, ranges, `constexpr` tables, `std::to_underlying`,
      `std::unreachable` and `[[nodiscard]]` where they make code clearer.
      Per-node code keeps `agents/implementation.md`'s rules. Each commit
      reproduces the fingerprint; release and sanitizer CTest; pooled-PGO
      NPS within noise, by A.7.4's method, against the head before the pass.
      — CLOSED 2026-10-07 (`0e80d1a`). `std::format` replaces the 25
      `snprintf` Diag printers (whose fixed buffers once truncated fields), the
      info formatter, and the bench and WAC reports. The remaining board
      queries and new helpers are `[[nodiscard]]`; the exhaustive WDL switch
      ends in `std::unreachable()`. Not used, with reasons:
      - `std::print`: the Ubuntu runners' libstdc++ 13 lacks it;
      - `std::expected` for FEN: already there;
      - ranges and constexpr rewrites of hot code: no clarity gain worth an
        NPS risk.
      Byte-identical output with times masked: the TUNE build's Diag at
      depth 11 on three positions (75 lines), the bench report, and WAC in
      both modes. Bench 14,978,465; release and ASan/UBSan CTest 17/17 (ASan
      shown live on a planted overflow). NPS by BAS-P14: −0.11% [−0.20,
      +0.15].
    - [x] **A.8.13** `[R2]` Score bands and in-search tablebase probes
      Two defects (review items 3 and 4). (a) Known-win evaluations up to
      27,420 (KBNK prints `cp 22048`) share the band `is_tablebase_decisive`
      reads as tablebase results. (b) The in-search probe returns a flat
      ±20,000 as an exact score, at every node from the probe depth and at
      ply 1, with no zeroing-move condition, no distance and no bound
      handling. Research the Stockfish shape: a tablebase band of its own
      between evaluations and mates, holding `win - ply`; evaluations clamped
      below it; probes only after a zeroing move within the probe limits; a
      win as a lower bound and a loss as an upper bound, returning on a
      cutoff and storing at depth + 6; display `cp ±(20000 - plies)`.
      Required content:
      - which consumers read the band (pruning and mate guards, the hash
        table's ply adjustment, the root display, the PV extension);
      - Rarog's refuted part, moving band values by ply in the table, which
        changed its bench without tablebases;
      - the KBNK drive's range under the clamp (BAS-E28/E29);
      - a prediction and falsifier;
      - the gate for A.8.15. No current run file sets `SyzygyPath`, so the
        gate is designed here: an endgame-start cohort with tables on both
        sides, conversion plus an SPRT, and a faults rule.
      — CLOSED 2026-10-07: `READY_FOR_IMPLEMENTATION`, packet
      `analysis/a813_tb_bands_2026-10-07.md`. Clerical correction: the probe
      has the zeroing-move condition under `Syzygy50MoveRule`; (b) above
      overstated it. New finding: with tables, the flat 20,000 passes every
      "not a mate" guard, so correction history trains on it and the
      reverse-futility refinement uses it. The handoff moves the band
      directly below mates, clamps evaluations under it, and widens six
      guards to `is_decisive`. Bench exactness is the deciding check for
      ply-adjusting the band in the hash table, with Rarog's unadjusted form
      as the fallback. The gate is changed from the endgame cohort: its
      roots are all in the tables, where in-search probes are off. It is a
      `[-5,5]` repair bracket at `10+0.1` on the standard book with tables
      configured, plus conversion counts and an activation read.
    - [x] **A.8.14** `[I2]` Score bands and probes, implementation
      Implements A.8.13's handoff. Qualification by that handoff: a
      fingerprint change if the clamp reaches the bench, the tablebase tests
      on the committed fixture, and protocol tests for the display.
      — CLOSED 2026-10-07: the handoff implemented in full, with the hash
      table ply-adjusting the band. H2 is refuted: bench 14,978,465 exact
      without tables, so Rarog's unadjusted fallback was not needed.
      `test_tt`'s boundary moves with the contract. New tests:
      - the band constants;
      - the display mapping;
      - KBNK without tables below the band;
      - a fixture search capturing into KQvK, scoring `tablebaseValue − 1`
        (`cp 19999`), where the old flat value gave 20,000.
      Release and ASan/UBSan CTest 17/17. With the 3–6-man tables, a 7-man
      root probes in search (365k hits) and reaches depth 29 in 2.3 s, where
      the head before reached depth 21 in 3.8 s.
    - [x] **A.8.15** `[V]` Tablebase-enabled gate
      A.8.13's registered gate, maintainer-run.
      Prepared 2026-10-07 as BAS-S19: candidate PGO build of `b0a078a`
      against the pre-A.8.14 PGO build. `sprt-repair-ltc.toml` (`[-5,5]`,
      `10+0.1`), concurrency 7, tables on both sides, cap 4,000 pairs, seed
      815; dry run accepted. Deviation from the packet: the existing wrapper
      flags carry `SyzygyPath`, so only the clock needed a new run file.
      — CLOSED 2026-10-07 (BAS-S19, maintainer-run): H1 at 280 pairs, **+31.7
      ± 13.4 Elo**, zero faults or time losses. All 181 clean tablebase wins
      reached were converted (candidate 115, baseline 66). Both binaries
      probe in search in 200/200 sampled 7–9-man positions. The prediction
      (+3) missed: the old flat value acted at every probe in half the games.
      The three parts are not separated; no ablation was run.
    - [x] **A.8.16** `[R2]` Won-endgame time sink
      A won ending without tablebases spent the whole hard maximum on one move
      (review item 2: 32,305 ms of a 60 s clock, last `info` at 1,020 ms;
      Stockfish about 6 s). Establish why the iteration never completes
      (Rarog's diagnosis: an aspiration cascade of fail-highs on a rising
      score). Also check whether the maximum itself, more than half the clock
      at move 86, is a defect. Design a bounded repair, its interaction with
      the aspiration loop (B.5's cluster) and the stability terms, a frozen
      prediction and a falsifier. D.1 keeps the general clock audit.
      — CLOSED 2026-10-07: `READY_FOR_IMPLEMENTATION`, packet
      `analysis/a816_time_sink_2026-10-07.md`. Confirmed: depth 28 fails high
      six times on the move depth 27 chose (`f7d7`), with no iteration
      completing before the maximum. The maximum is Stockfish's formula
      exactly, and stays. The donor's shallower re-search is refused
      (BAS-D17). Repair: past the optimum, a root fail-high on the last
      iteration's move ends the search; no move can change. The gate is
      amended to a `[-5,5]` repair bracket at `3+0.03`.
    - [x] **A.8.17** `[I1]` Won-endgame time sink, implementation
      Implements A.8.16's handoff. `uci_probe.py clock` on the review's
      positions shows the repair. Exact bench unless the handoff says
      otherwise.
      — CLOSED 2026-10-07: the handoff as written. At `60000+600`:
      - rec1: 32,313 → 4,750 ms (predicted about 4.7 s);
      - rec2: 3,817 → 3,875 ms;
      - rec3: 6,943 → 5,156 ms;
      - KQvK: 1,056 → 891 ms;
      - every move unchanged.
      KRvK reads about 1.7 s, the same on the pre-fix A.8.14 build, so that
      change predates this leaf. Engine test: 2.36 s at `30000+300`, 15.72 s
      with the stop disabled. Bench 14,978,465; release and ASan/UBSan CTest
      17/17.
    - [x] **A.8.18** `[V]` Won-endgame time sink gate
      SPRT `[0,3]` at `3+0.03` 1T and a `10+0.1` direction check, as A.8.16
      registers them; zero time forfeits. Maintainer-run.
      Amended by A.8.16 to a `[-5,5]` repair bracket at `3+0.03`. Prepared
      2026-10-07 as BAS-S20: candidate PGO build of `9c93d15` against the
      BAS-S19 candidate, cap 4,000 pairs, seed 818; dry run accepted.
      Played 2026-10-07: cap reached, **−0.7 ± 4.5 Elo** (nElo [−8.71, +6.51]),
      zero time losses. The registered acceptance (interval above −5) is not
      met, and neither is the return condition (H0). — CLOSED 2026-10-08:
      A.8.17 kept by maintainer decision, with the registered acceptance
      not met (BAS-S20).
    - [x] **A.8.19** `[R2]` Displayed-score normalisation
      Moved from D.3.3. Displayed-score research card: fit a win-rate model
      on Basilisk's own games, decide the `cp` mapping as Stockfish
      normalises, then implement. It works on A.8.14's tablebase band, and
      decides how known-win evaluations display (KBNK `cp 22048` beside KRvK
      `cp 715`). Display only, gated by identity: exact bench, no SPRT.
      — CLOSED 2026-10-08, NO_CHANGE by maintainer decision (BAS-C14). The fit
      puts the 50% point at 135 internal units in the middlegame and 334 in
      the endgame. `cp` stays internal: 29 tools, B.0's scale-ratio
      instrument among them, read it as internal units. `UCI_ShowWDL` was
      deferred by the maintainer. Known-win evaluations display as they are;
      since A.8.14 they sit below the tablebase band.
    - [x] **A.8.20** `[M]` B-programme anchors on the A.8 head
      Record A.8's fingerprint changes. Re-anchor the B.0 packet's §12
      handoffs and A.6's move table, which cite line ranges at `2e7914e`, on
      the A.8 head. State which A.7 baselines stand: G(0) and the gauntlets
      are 1.10.1's release baselines. The pooled-PGO NPS baseline is re-read
      if any A.8 leaf moved NPS (maintainer-run). Confirm B.0.1 and B.0.2's
      registered binaries are pinned and unaffected.
      Records done 2026-10-08:
      - fingerprint unchanged through A.8 (14,978,465);
      - A.6's map re-anchored in its packet (`search.cpp` 3,125 → 3,298
        lines; `eval.cpp` unchanged), with what A.8 added to each section;
      - B.0 packet §12.4: §10's fixed-node baselines unaffected, the
        instruments' parsers checked on A.8.8's line shape, `cp` unchanged
        (BAS-C14), and B.1's NPS reference re-based on BAS-P15;
      - B.0.1 and B.0.2's binaries re-hashed and unchanged;
      - G(0) and the gauntlets stand as 1.10.1's release baselines.
      — CLOSED 2026-10-08: BAS-P15 (maintainer-run) puts the A.8 head at
      **4.189M NPS** pooled, +1.5% over BAS-P12, with every prediction met.
      It is B.1's NPS reference.
    - [x] **A.8.21** `[M]` Patch release 1.10.2 (ANY TIME) — after A.8.20
      Maintainer decisions 2026-10-07: cut **1.10.2** through A.8.6's flow
      once the rest of Phase A is finished (amended the same day from "after
      A.8.11"), so it ships the gated A.8.14 and A.8.17 changes. Version sources, CHANGELOG's
      `[Unreleased]` dated as `[1.10.2]`, release and sanitizer CTest,
      the bench A.8.20 records, and a maintainer-run smoke run with ponder
      on and at Threads 4 against 1.10.1 with zero faults. A.8.15 and A.8.18
      are maintainer-run gates (decision 2026-10-07). The PR into
      `master` merges with a merge commit (A.8.3). The GitHub checks the
      earlier leaves could not run locally are owed here: A.8.2's PR run
      and manual dispatch, and A.8.6's candidate run on this PR and its
      tag run. A failure reopens the leaf it proves. Tag, push and
      publish stay the maintainer's.
      — CLOSED 2026-10-08 with the release commit:
      - version 1.10.2 in both sources;
      - CHANGELOG's user-facing `[1.10.2]` section;
      - README, GUIDE, DESIGN and HISTORY marked released.
      Maintainer decisions 2026-10-08:
      - the verification match against 1.10.1 (BAS-M13) is withdrawn
        unplayed, and the CHANGELOG quotes BAS-S19;
      - the ponder-on Threads 4 smoke run (BAS-C15) was stopped at 121 of 200
        games and accepted on judgement, with 1.10.2 at zero failures, as a
        check that 1.10.1's ponder fixes still hold.
      The PR's CI and Release runs and the tag run complete A.8.2's and
      A.8.6's owed checks.

- [x] **A.9** Harness and measurement instruments
  Added 2026-10-24. Evidence: `analysis/rarog_audit_2026-10-24.md`, appendix
  D.
    - [x] **A.9.1** `[I1]` Colosseum re-pinned to 0.4.0
      (D-36, D-38, C-31) Basilisk pins `colosseum-cli` 0.2.0. Rarog moved to
      0.3.0 (engine processes kept per slot, the setup position counted for
      threefold, faster datagen; cross-checked against fastchess, +6.9 ± 9.4
      against +6.78 ± 9.07) and 0.4.0 (per-engine depth, NPS and hold
      percentiles, pair Elo with the pentanomial, terminations and faults by
      engine). Re-pin with a registered cross-check, as Rarog's RAR-M70 was.
      The wrapper keeps zero tolerance for Basilisk's own faults and reads
      opponent faults under a rate rule instead of voiding a run (BAS-M10).
      Before B.3's gate, so B.3 is read on the new runner.
      — CLOSED 2026-10-24. Maintainer decisions: no cross-check and no null
      pair are owed (2026-10-14, and 2026-10-24: 0.4.0 is proven in Rarog).
      - The pin is `cli-v0.4.0` (`9c72539`), Rarog's executable and archive
        digests. Gauntlets no longer unset `engine-processes`, and the
        wrapper refuses any resolved value but `per-slot`.
      - The guard test had been failing on 0.2.0 too: its default arms
        were no longer staged. It now uses BAS-S27's pair.
      - Gauntlet faults are read per engine from `colosseum-cli stats`
        (D-38). A Basilisk build's fault or time loss voids the run, and so
        does an infrastructure fault. A foreign opponent's faults are read
        up to 1% of its games, and the runner's cap is 1% of the foreign
        games. BAS-M10's two Houdini 3 crashes would have been read at the
        registered 400 games a row.
      - Guard cases 31/31. A strengthened control fails on a return bug
        the smoke run found.
      - An 8-game smoke gauntlet streamed live progress to the console,
        and a dry run on 0.4.0 accepted the SPSA command and its run file.
      - The three BAS-S27 dry runs resolve as on 0.2.0, except
        `engine_processes`.
    - [x] **A.9.2** `[V]` Direction reads of the head against 1.10.2
      Added 2026-10-24 by maintainer decision. Every gate since 1.10.2 read
      `3+0.03` on one thread; the head has never been read at `10+0.1`, at
      four threads, or with tablebases, and B.2's kernel has its own tablebase
      probes. Three 1,000-game gauntlets against 1.10.2 rebuilt with the same
      compiler, registered as BAS-S27 with frozen predictions and a reading
      rule: each read passes when its 95% interval excludes a loss; a read
      below its band, or any Basilisk fault, opens a diagnosis before B.3.
      Maintainer-run, `tools/run_head_reads.ps1 -Read ltc|4t|tb`; B.11's
      release reads repeat them on the B.9 head.
      — CLOSED 2026-10-25 (BAS-S27, maintainer-run 2026-10-24). All three
      reads pass, with zero faults or time losses:
      - `ltc` +241.4 [+221.9, +262.3];
      - `4t` +237.6 [+217.4, +259.2];
      - `tb` +221.9 [+202.4, +242.7], with tablebases live.
      None is below its band, so no diagnosis opens before B.3. All three
      came in above their predictions: there was no LTC compression, and
      thread scaling is intact. B.11's release reads predict accordingly.
    - [x] **A.9.3** `[M]` Measurement rules from the audit
      (D-39, D-40, D-41, D-28, D-47) Into `agents/measurement.md` and PROCESS:
      a stopped SPRT's estimate is a first-crossing estimate, never the effect
      size, and is never extended or re-scored; about 2 Elo per 1% of speed
      held to +22% at `3+0.03` (RAR-S98, RAR-S99), replacing PROCESS's "small
      deltas only"; opponents play their smallest safe overhead (10 ms); a
      compiler bump owes a pooled NPS read and never falls between an SPRT's
      arms (rustc 1.98 to 1.99 cost Rarog 1.6% at the same fingerprint); a
      SQLite database is copied with its `-wal` file.
      — CLOSED 2026-10-25.
      - `agents/measurement.md` gained three rules: the first-crossing
        label and its no-extension rule (D-39), the compiler bump as a
        pooled speed read never between an SPRT's arms (D-28), and the
        SQLite copy (D-47).
      - D-47 was unverified in the audit. `export_tournament_pgn.py` opens
        the database read-only in place, so it already sees the WAL.
      - PROCESS gained the speed-to-Elo prior to +22% (D-40) and the 10 ms
        opponent overhead, which is dormant until E.2's field (D-41).
      - Under D-28, MSYS2's clang 22.1.8 to 23.1.3 step of 2026-10-23 owes
        a pooled NPS read, which no record yet carries. A.9.4 registers it
        as BAS-P19.
    - [x] **A.9.4** `[I1]` NPS read rebuilt on pooled builds
      (D-1 to D-4) Rarog's method study (RAR-P32): builds, not cycles, set the
      limit. Best of three per cycle; four builds an arm; a t-interval on
      per-build medians; a two-step read (two cycles, accept at +0.9% and
      close at +0.1%; between them six cycles, +0.5% with the lower bound
      above 0); a disturbance rule; two independent pools as the null; a pause
      after fresh builds, which antivirus scanning slows. `nps_ab.ps1`
      bootstraps readings, which Rarog showed is not the uncertainty.
      Implemented 2026-10-25, LOCAL_QUALIFIED:
      - `tools/nps_pool.ps1`, `tools/nps_read.ps1` and
        `tools/diag/nps_read.py`, as PROCESS *Build, bench and NPS* now
        states.
      - `test_nps_read.py` passes 18/18, and each of five mutations of the
        decision rules fails it. `test_nps_pool.ps1` passes 9/9.
      - A smoke read of two adopted pools ran step 1 and `-Extend` end to
        end, and a worktree build of `9c93d15` reproduced its fingerprint.
      - The leaf closes when BAS-P18, the null, reads zero. BAS-P19, the
        clang 22 to 23 read A.9.3 owed, runs after it. Both are
        maintainer-run with `tools/run_a94_nps_reads.ps1 -Read null|clang`.
      — CLOSED 2026-10-25.
      - BAS-P18, the null, read −0.19% [−0.66%, +0.27%], NO_CHANGE at step 1.
        That is inside its prediction, so the instrument is validated and
        the worktree build path costs nothing visible.
      - BAS-P19 read clang 23.1.3 at +0.89% [+0.23%, +1.54%] against 22.1.8
        at identical source: no regression. Rarog's −1.6% prior did not
        transfer, so A.9.3's owed compiler read is done.
      - The launcher's first run stopped before any reading on a pipeline
        bug, fixed in `5b6ea69`. Its pools were reused, and no reading was
        lost.
    - [x] **A.9.5** `[I1]` Profiler with inline attribution
      (D-13, D-75) B.7.2 and C.12 both require a profile, and Basilisk has no
      profiler. Rarog's reads per-region shares from ETW or a user-mode
      sampler with full inline chains over frozen cohorts, and validates its
      schema from the data: xperf with `-symbols` once silently charged
      inlined board work to `negamax`. The Clang/MinGW toolchain needs its own
      symbol path.
      — CLOSED 2026-10-25.
      - The research question was the symbol path, and it settled on two
        facts measured here:
        - `-gline-tables-only` leaves `.text`, `.rdata` and `.data`
          byte-identical (non-PGO, bench 8,437,148 both ways);
        - llvm-symbolizer reads full inline chains from MinGW's DWARF.
      - `PROFILE_SYMBOLS` (off by default; byte-identical when off) and
        `build_test.ps1 -Flavor Profile` build the binary. Its PGO build
        benches 8,437,148 and carries 23,871 inline records.
      - `tools/profile.ps1` is a user-mode sampler that needs no
        administrator rights. `tools/diag/profile_regions.py` charges the
        innermost engine frame by source file. Its tests pass 10/10.
      - On the head's `bench 13` × 5 (about 12,000 samples): board 23.5%,
        eval 21.4%, ordering 20.8%, search 16.7%, TT 6.8%, attacks 6.7%,
        system 4.1%. A repeat agrees in every region within its interval.
      - Charging by function instead (the trap) reads search 38.4%,
        board 12.8%, attacks 0.0% and TT 0.3%.
      - Refusals proven on real inputs: a symbolization without inline
        chains (exit 2), and a Release binary without line tables.
      - The sampler's first form read at 19 Hz, because Windows ignores a
        background process's timer request. Pollers' wake-ups also
        inflated `system` to 67%. A spin clock and a busy-thread filter
        fixed both before any recorded profile.
      - Cohorts beyond `bench 13` belong to B.7.2 and C.12, which define
        them. An EPD cohort run shows the path works.
- [x] **A.10** Delivery and repository repairs
  Added 2026-10-24. The delivery machinery matches Rarog's (CI on pull
  requests, tag-driven release, one fingerprint across the assets); these
  steps bring the practice and four checks up to it. Evidence:
  `analysis/rarog_audit_2026-10-24.md`, appendix D Part 2.
    - [x] **A.10.1** `[M]` Land dev and B.10 on master by squash PRs (ANY TIME) — before B.3
      (D-55) PROCESS *Delivery*'s transition said `dev` lands as one squash PR
      once B.2 closed; it has not (`origin/master` is still 1.10.2), and
      B.10's branch was cut from `dev`. Land B.2 (`dev`), then B.10, as one
      squash PR each with the fixed message shape and their results;
      afterwards new branches start from `master`. PRs, pushes and merges are
      the maintainer's.
      — CLOSED 2026-10-24 by the maintainer's merges: B.2 landed as `71a612b`
      (PR #5) and B.10 as `f4ebe79` (PR #6), one squash commit each, and
      branches now start from `master`. It closed out of turn, so it keeps
      its `(ANY TIME)` marker.
    - [x] **A.10.2** `[M]` CHANGELOG Unreleased caught up and kept
      (D-66) `[Unreleased]` is empty although B.2's cluster and B.10's repair
      were accepted. Write their user-facing entries, and add Rarog's rule to
      `agents/records.md`: a user-visible change gets its entry in the commit
      that records it.
      — CLOSED 2026-10-25.
      - B.10's entry had already landed with its fix, so only B.2's was owed.
        It is now written: the rebuilt selectivity core, at +136 ± 34 and
        +59 ± 21 Elo at `3+0.03`, and the head's BAS-S27 reads against
        1.10.2.
      - Nothing else user-visible has landed since 1.10.2. The release
        binaries' UCI option lists are identical apart from the version
        string, and PR #7 and PR #8 were roadmap and tooling only.
      - The rule, with what counts as user-visible, is in
        `agents/records.md`.
    - [x] **A.10.3** `[M]` PR refspec and required checks on master
      (D-59, D-58) Configure `remote.origin.fetch` with
      `+refs/pull/*/head:refs/remotes/origin/pr/*`, so a pruning fetch keeps
      the PR refs citations need (they vanished on 2026-10-23), and record it
      in `agents/records.md`. Make CI and Release required checks on `master`
      (repository settings, the maintainer's).
      Implemented 2026-10-25; the maintainer's setting is pending.
      - This clone maps the PR refs. A dry run showed that a pruning fetch
        without the mapping deletes all 8; with it, a real pruning fetch
        keeps all 8. The likely pruner is GitHub Desktop.
      - `agents/records.md`, PROCESS and `check_citations.py`'s hint now
        give the once-per-clone mapping, not a one-off fetch.
      - The required checks needed aggregators that cannot pass by being
        skipped. CI gains `CI complete` over its six jobs; Release's
        `Verify the complete matrix` now always runs and fails unless
        every build succeeded.
      - The step closes once the maintainer requires both checks on
        `master`, verified through the API. This branch's PR is their
        first live run, and A.10's closing check.
      — CLOSED 2026-10-25. Verified through the API: `master` requires
        `CI complete` and `Verify the complete matrix`, and branches up to
        date with it. Administrators included, force pushes and deletions
        blocked, squash merges only, as before.
    - [x] **A.10.4** `[I1]` Release and CI checks tightened
      (D-65, D-29, D-27, D-57) `release_check.py` also compares the released
      baseline's fingerprint. The Windows release cells pin and print the
      MSYS2 clang version: they install whatever MSYS2 ships, which is how
      clang moved from 22.1.8 to 23.1.3 on 2026-10-23. An ARM64 ISA contract
      with a required `prfm` class (`verify_isa.py` checks x86 tiers only).
      `ctest -R` filters in CI fail when they match nothing
      (`--no-tests=error`).
      — CLOSED 2026-10-25. Each check was proven live on known-bad input.
      - **Fingerprint (D-65).** GUIDE's *Released baseline* row states its
        `bench 13` count, and `release_check.py check` refuses a release
        whose row differs from the declared fingerprint. The two new test
        cases fail on the old checker.
      - **Compiler pin (D-29).** CI and Release had built Windows assets with
        clang 22.1.8, from the runner image's frozen MSYS2 database, while
        every measurement since 2026-10-23 used 23.1.3. By maintainer
        decision the pin is 23.1.3, in `.github/windows-clang-version`.
        Every Windows cell refreshes MSYS2, prints its compiler and fails
        on any other clang. The check passed on 23.1.3 and failed on a 22.1.8
        pin with this machine's clang.
      - **ARM64 contract (D-27).** `verify_isa.py --tier arm64` requires
        `prfm`, the TT prefetch, and forbids SVE, dot-product, AES and SHA.
        It reads AArch64's one-word objdump format, which the old parser
        skipped entirely; x86 counts are unchanged on the head's PEXT
        asset.
        - Freestanding objects for Linux and Windows ARM64 pass.
        - No-prefetch, SVE, dot-product and AES objects fail.
        - The contract runs on every ARM64 cell of CI and Release.
        - PR #9's first runs failed Windows ARM64 on one SVE instruction that
          Linux and macOS lack. The check now names every forbidden
          instruction's function.
          - The second run named it: `cntd x0` in LLVM libunwind's
            `Registers_arm64::getRegister`, not compiler-rt's
            `__arm_get_current_vg`, which I had guessed. libunwind reads the
            vector-granule register only when unwinding an SVE frame, and the
            engine has none.
          - The check allows it in that one function alone. Windows links
            LLVM libunwind, while Linux uses libgcc's unwinder.
        - `test_verify_isa.py` runs in the documents job.
      - **Empty filters (D-57).** The three `-R` filters and the
        sanitizer's `-LE strength` run pass `--no-tests=error`. Locally, a
        filter matching nothing exits 8 with the flag and 0 without it.
      - **Not yet run live.** The workflow steps first run on this branch's
        PR.
      - **Amended 2026-10-25 (maintainer decision).**
        - The pin is the LLVM major, `23`. Patch releases pass, because MSYS2
          ships several a month and an exact pin would stall CI on each.
        - `build_test.ps1` warns when the local clang is on another major. It
          warned on a `22` pin and stayed silent on `23`.
        - The workflow check, run verbatim on this machine, passes on `23`
          and fails on `22` and `2`.
    - [x] **A.10.5** `[I1]` Ledger index dispositions kept in step
      (D-61) `docs/EXPERIMENTS.md` lists BAS-M10 and BAS-M11 as not run while
      their entries read VOID and COMPLETE; the checker compares IDs, not
      dispositions. Fix the rows and make `check_roadmap.py` flag an index
      disposition that disagrees with its entry's.
      — CLOSED 2026-10-25.
      - Free-text dispositions are compared by state, open or closed. A
        verdict word anywhere means closed. A text that opens with
        registered, prepared, not (yet) run, maintainer-run or pending, and
        holds no verdict, is open.
      - The entry's state comes from its last `Disposition:` line, or from
        its `Disposition`, `Result / disposition` or `Result` field.
      - Entries with no such field, multi-part entries, and texts that state
        neither are not compared. The 52 entries with a state on both sides
        are compared.
      - On the real ledger the check refused exactly BAS-M10 and BAS-M11.
        Their rows now read VOID and COMPLETE. The self-test covers both
        directions, both entry forms, and prose that merely says "running".
    - [x] **A.10.6** `[M]` Tools index and retired numbers in comments
      (D-62, D-63) A `tools/README.md` with one line per tool and the
      procedure or leaf that uses it; retired step numbers removed from source
      comments (for example `// 8.7.6(c)` in `search_kernel_core.cpp`,
      "8.7.10" in `attacks.h`), behaviour-neutral.
      — CLOSED 2026-10-25.
      - `tools/README.md` indexes every tracked tool, one line each, with
        the procedure, step or ledger entry that uses it. A coverage check
        found none missing.
      - The retired numbering (Phases 5–15) is gone from engine comments:
        185 lines in 27 files. Each number became plain words, and
        history-only fragments were deleted. Two rewordings were corrected
        against the code: the search's clock does start at `recv_time`, and
        the bishop-scaler scope is deferred, not settled.
      - Behaviour-neutral, proven: against a non-PGO build of the code before
        the edit, `.text`, `.rdata`, `.data`, `.pdata`, `.tls` and `.reloc` are
        byte-identical; bench 8,437,148.
      - Left for the maintainer: 32 current-roadmap or ledger IDs in
        comments, and stale bench counts quoted in a few comments (for
        example 11,941,440 in `eval_params.h`).
    - [x] **A.10.7** `[I1]` Dev builds name their commit
      Added 2026-10-24 by maintainer decision. The exact shape is decided when
      this leaf opens. The proposal follows Reckless: a dev build's id name
      appends the short commit, as in `Basilisk 1.11.0-dev-<sha>`. That needs
      a build change, and `release.yml`'s id-name check must accept the suffix
      on candidate builds (next to A.10.4's release checks).
      — CLOSED 2026-10-25. Shape by maintainer decision: Stockfish's exact form.
      - A development build reads `Basilisk dev-<commit date>-<8 hash
        characters>`, with `-m` after the date when the working tree differs
        from the commit. Without git it reads `dev-<compile date>-nogit`. A
        release still reads `Basilisk X.Y.Z`. The release line was dropped:
        every dev build shares it, so it tells nothing the date and hash
        don't. `engineVersion` keeps `X.Y.Z-dev` for the release tooling.
      - One deliberate difference from Stockfish: the modified check uses
        the clone's own line endings, because Stockfish's
        `core.autocrlf=false` can read a CRLF working copy as modified.
      - A header regenerated at every build carries the commit, so a commit
        made without reconfiguring never leaves a stale name.
        `BASILISK_DEV_ID` fixes the name for byte-identity comparisons.
      - Release's smoke test now requires a candidate build to name exactly
        the commit it was built from, and a release to read `Basilisk X.Y.Z`.
        Run verbatim here, it passed both and refused a wrong commit, a
        suffixed name and a dev name on a release.
      - Verified: a clean tree names its commit; an uncommitted edit adds
        `-m`; the override works; a PGO build names its commit at bench
        8,437,148. CTest 21/21, and `test_uci_protocol` 85/85 with four new
        sections.
      - The first live run (PR #9) found every Windows build reading
        `dev-<date>-nogit`: the MSYS2 build shell has no git on its PATH, and
        MSYS2's own git reads this CRLF clone as modified. Configure now looks
        for Git for Windows first and hands the build that git. Under MSYS2
        it picks Git for Windows even with MSYS2's git present.
- [ ] **A.11** Correctness repairs from the Rarog audit
  Added 2026-10-24 by maintainer decision. Each repair is its own engine
  commit with a test that fails on the old code, bench accounting for any
  fingerprint move and CTest on both arms; a repair that changes play is gated
  by a registered `[-5,5]` repair SPRT unless its leaf argues none is owed.
  Evidence: `analysis/rarog_audit_2026-10-24.md`. The repairs change the
  default search only: the legacy kernel (`-DB2_CORE=OFF`) is neither fixed
  nor maintained, and B.8 deletes it (maintainer decision 2026-10-24).
    - [x] **A.11.1** `[I1]` Rule-50 damping out of the TT static eval
      (B-1, C-7) `evaluate()` damps the score by the halfmove clock
      (`eval.cpp:1210`, `:1985`); the kernel stores that value as the TT
      static eval and reuses it at any clock
      (`search_kernel_core.cpp:488-495`, qsearch `:184-187`), though the TT
      key has no clock. Rarog found the same defect in its own selectivity
      core (RAR-S73, `db92790`) and moved the damping into the search, applied
      once after the TT read. Test: an eval stored at clock 0 and read at
      clock 80 is served as a fresh evaluation would serve it. Moves the
      fingerprint; it precedes any fit and optimism (B.5).
      - Implemented 2026-10-25: `Evaluator::evaluate_undamped` is the eval
        without the clock, `evaluate()` damps it for every other caller,
        and the default kernel stores the undamped value and damps it after
        the read in negamax and qsearch. The legacy kernel is untouched.
      - The new `test_search` sections plant a clock-0 entry for a
        capture-free child at clock 81: the old kernel serves it undamped
        (root -748 against a fresh -443), the repair serves -443.
      - Qualified 2026-10-25 on `2c0cc76`: `bench 13` 9,260,429 (from
        8,437,148, +9.8%); the legacy arm 15,887,213, unchanged; CTest ON
        23/23 and OFF 20/20 in Release.
      - It changes play, so it owed the repair gate: BAS-S28, `[-5,5]`,
        launcher `tools/run_a111_gate.ps1`, maintainer-run.
      - Closed 2026-10-25: BAS-S28 reached its 4,000-pair cap at
        +1.5 ± 4.8 Elo (8,000 games, LLR +1.58, no faults) with equal NPS,
        and the repair stays by the registered rule on the maintainer's
        confirmation. The development fingerprint is `bench 13` 9,260,429;
        the legacy arm stays 15,887,213.
    - [x] **A.11.2** `[I1]` Singular search: lone excluded move returns alpha
      (B-4) When the singular search excludes a node's only legal move,
      `searched == 0` returns `in_check ? -(MATE_SCORE - ply) : 0`
      (`search_kernel_core.cpp:1297`). That scores far below singular beta, so
      the move extends, up to two plies off the PV, and the extensions chain:
      doubles are capped only by `DoubleExtMax`, and the check extension
      stacks on top. Stockfish returns `alpha` when the excluded move is the
      only one. In Rarog the chain left bench unfinished (seldepth 127 at 331M
      nodes, one search spinning 39 minutes) once IIR changed; Basilisk's IIR,
      at its rail, plausibly hides it today, and B.3 plans an IIR-off arm.
      Test: a forced position whose only move is the TT move does not extend;
      a counter for the lone-move return.
      - Implemented 2026-10-25 (`8802a85`): an exclusion search with no
        move left returns `alpha` (`no_move_score`, `src/search_types.h`),
        counted as `singular_lone_move` in diagnostic builds.
      - The leaf's "does not extend" is corrected to its mechanism: `alpha`
        is one below the singular beta, so a forced move still extends by
        one ply, as in Stockfish and Rarog, and never doubles at the
        default `SingularDoubleMargin` 4. The test checks exactly that on
        Rarog's forced `Kxg2`. `negamax` has no test seam, so the test
        holds the return's value and the Diag counter shows the kernel
        takes it: 2,719 returns over `suite_v1.epd` at depth 10.
      - Qualified: `bench 13` 8,753,339 (from 9,260,429, -5.5%); the legacy
        arm 15,887,213, unchanged; CTest ON 23/23 and OFF 20/20.
      - It changes play, so it owed the repair gate: BAS-S29, `[-5,5]`,
        launcher `tools/run_a112_gate.ps1`, maintainer-run.
      - Closed 2026-10-25: BAS-S29 reached its 4,000-pair cap at
        +1.7 ± 4.8 Elo (8,000 games, LLR +1.84, no faults) with equal NPS,
        and the repair stays by the registered rule on the maintainer's
        confirmation. The development fingerprint is `bench 13` 8,753,339;
        the legacy arm stays 15,887,213.
    - [x] **A.11.3** `[I1]` Own clock of zero without increment is bounded
      (A-55) `go wtime 0 btime 5000` with no increments: a negative clock
      clamps to 0 (`parameters.cpp:291-293`), `compute_time_limit` sets no
      soft or hard limit when `time <= 0 && inc <= 0`
      (`search_worker.cpp:74`), and `infinite` stays false because the other
      clock is set (`engine.cpp:92-98`), so the search runs to
      `MAX_SEARCH_DEPTH`. Stockfish budgets from `max(1, ...)`. A zero own
      clock gets a minimal budget. Test: the command answers `bestmove` within
      a bound.
      - Closed 2026-10-26 (`6de81be`). Time management is on when either
        side has a clock or an increment, as in Stockfish, so the budget
        formula runs for an empty own clock and its floors give one
        millisecond. A search with no clock at all keeps its depth, node or
        mate limit: the early return could not simply go, since it is also
        what leaves `go depth N` unbounded in time.
      - New `test_search` sections: `go wtime 0 btime 5000` answers a legal
        `bestmove` within 2 s, and a clock-free `go depth 6` still completes
        depth 6. On the old code the first is stopped only by the test's
        10 s watchdog. Through UCI, `wtime 0` and `wtime -50` answer at
        once at depth 4; `go depth 9` and a 2 s clock are unchanged.
      - `bench 13` 8,753,339 and the legacy arm 15,887,213, both unchanged;
        CTest ON 23/23 and OFF 20/20. The driver is shared, so the legacy
        arm carries the fix too.
      - No gate is owed: the change acts only when the side to move has no
        time and no increment, which no registered gate can reach (every
        one has an increment) and which in a game means the flag has
        already fallen.
    - [x] **A.11.4** `[I1]` Upper-case UCI move text accepted
      (A-9) `apply_uci_move` matches `move_to_uci(move) == token` exactly
      (`parameters.cpp:67`), so `E2E4` or `e7e8Q` is rejected, and a rejected
      `position` ends the process by design (BAS-C12). Rarog accepts
      upper-case move text (its tests come from Little Blitzer's output).
      Compare case-insensitively before the legality match; the fatal policy
      for a genuinely illegal move stays.
      - Closed 2026-10-26 (`d81efc7`): `fold_move_text` lower-cases the token
        before both legality matches, `position` and `searchmoves`
        (`src/parameters.cpp`); a folded token must still equal a legal
        move's spelling, and the diagnostics print the token as sent.
      - New `test_uci_protocol` sections: `E2E4` plays e2e4, `e7e8Q` and
        `E7E8N` promote to the piece named, `searchmoves E2E4 D2D4` is
        accepted, and `E2E5` is still rejected with the board unchanged.
        The first three fail on the old parser. Through UCI,
        `position startpos moves E2E4 E7E5 G1F3` now searches instead of
        ending the process.
      - `bench 13` 8,753,339 and the legacy arm 15,887,213, unchanged;
        CTest ON 23/23 and OFF 20/20. No gate is owed: the change is
        protocol only, and every harness sends lower-case moves.
    - [x] **A.11.5** `[I1]` Fatal errors from every thread reach stdout
      (A-35) Only the UCI thread catches (`main.cpp:86-93`); the engine thread
      (`engine_entry.cpp:49`) and the helper threads have no handler and no
      `std::set_terminate`, so an exception there aborts with nothing on
      stdout, where harnesses look. A concrete source: `setoption Hash` at the
      advertised maximum throws `bad_alloc` in `tt.h:130-131` after
      `clusters_.reset()`. Print `info string CRITICAL ERROR ...` on stdout
      from every thread, and keep a usable table when a Hash allocation fails.
      - Closed 2026-10-26 (`8289ceb`). `run_engine` installs a terminate handler
        (`report_terminate`, `src/uci_output.h`): an exception escaping any
        thread prints `info string CRITICAL ERROR: unhandled exception: ...;
        exiting.` on stdout and exits with status 1. It does not wait on
        the output mutex, which the dying thread may hold.
      - `TranspositionTable::resize` keeps the previous size, cleared, when
        the new table cannot be allocated, and returns false; the engine
        prints `info string Hash N MB could not be allocated; keeping M MB`.
        It does not halve toward a smaller table: zero-filling the largest
        one the system would commit could exhaust memory.
      - Tests: `fatal_report_from_a_thread` (a helper-thread exception
        yields exit 1 and the line on stdout; without the handler the
        process aborts with stdout empty) and `test_tt`'s failed-resize
        sections (the old `resize` threw `bad_alloc` and left no table
        behind its stale size; skipped under ASan, whose allocator aborts
        on an oversized request). Through UCI, `setoption name Hash value
        33554432` now keeps 64 MB and the engine searches.
      - `bench 13` 8,753,339 and the legacy arm 15,887,213, unchanged;
        CTest ON 24/24 and OFF 21/21. No gate is owed: nothing changes
        unless a thread fails or a Hash cannot be allocated.
    - [x] **A.11.6** `[I1]` Frontier nodes counted once
      (B-111) `negamax` counts a node at entry (`record_node()`,
      `search_kernel_core.cpp:335`) and at depth 0 or less hands off to
      `quiescence`, which counts it again (`:117`); `interior_nodes` is also
      counted before the hand-off (`:391`). `nodes`, NPS, bench and `go nodes`
      budgets are inflated, and the move loop's `(int(state_.nodes) & 14)`
      reads the count, so search behaviour depends on it. Rarog's counter
      specification holds `main_tt_probes == nodes` as an exact-mode
      invariant. Count each node once and re-declare the fingerprint; whether
      a gate is owed is decided with the maintainer (timed play changes only
      through that rule and the stop poll).
      - A precondition defect, found 2026-10-26: the tie rule also ran at
        the root. A later root move is searched with a null window at
        alpha, and the fail-hard quiescence search returns alpha exactly,
        so an equal score is usually a fail-low bound. With the count
        fixed, the free-queen test played Qa4 (worth −6) at Qxd5's
        `cp 1018`. Fixed first in its own commit (`0eadf03`, maintainer
        decision): the root is kept out of the rule, as Stockfish's root
        plays only a move that exceeded alpha. `bench 13` 8,753,339 →
        8,101,423; all 40 positions move, 39 in score. The legacy kernel
        has no such rule.
      - Implemented (`07a357f`): the depth-0 hand-off and razoring, which
        also re-entered quiescence on the same node, pass the node as
        counted, so quiescence skips the count and the stop, ply and draw
        checks negamax already ran. `interior_nodes` moved below the
        hand-off; `in_check_nodes` stays above it, keeping run_suite's
        `check_exts <= in_check_nodes`.
      - A new `test_search` section: a depth-1 search of a single quiet
        move counts 2 nodes (the old count, on the A.11.2 binary, is 3).
        Free-queen now plays Qxd5 at depths 1–3.
      - `bench 13` 7,233,823 with both commits (from 8,753,339); the
        legacy arm 15,887,213, unchanged; CTest ON 24/24, OFF 21/21.
      - Gate: one `[-5,5]` repair SPRT for both commits against `master`
        (maintainer decision 2026-10-26), since the root fix changes play
        and the count reshuffles every remaining tie: BAS-S30, launcher
        `tools/run_a116_gate.ps1`, maintainer-run.
      - The leaf's "timed play changes only through that rule and the stop
        poll" missed a third reader: the time manager's root-effort ratio
        (`search_worker.cpp`, `root_best_nodes / root_depth_nodes`). The
        gate's games show it did not move the clock (69.6 ms per move on
        both arms).
      - Closed 2026-10-26: BAS-S30 accepted H1, +4.3 ± 5.9 Elo (5,312
        games, LLR +2.95, no faults); both repairs stay by the registered
        rule. The development fingerprint is `bench 13` 7,233,823; the
        legacy arm stays 15,887,213.
    - [x] **A.11.7** `[I1]` Evaluation colour symmetry: tie-break and sweep
      (C-3, A-22, C-103) The pawnless "ahead by at most a minor" scaler picks
      `strong = (w_tot >= b_tot) ? WHITE : BLACK` (`eval.cpp:800`), so at
      equal totals White is always the strong side: a colour asymmetry, rare
      because the totals must tie exactly. Pick the side by the score's sign,
      as Stockfish does. `tests/test_eval.cpp` mirrors only five positions;
      Rarog's corpus-wide sweep with promoted material and the lazy gate open
      found twelve asymmetric evaluations (its C.13.1, among them a
      connected-rooks bug). Add a sweep over a versioned sample with promoted
      and odd material.
      - Implemented 2026-10-26 (`d5442f8`): the scaler takes the winning
        side from the score's sign (`score > 0`), as Stockfish's
        `eg > VALUE_DRAW`. Besides the tie, the totals could disagree with
        the score, and then the richer side's lack of pawns scaled the other
        side's advantage.
      - Tests in `test_eval`: an equal-totals position (R+N+B against Q and
        two pawns) equals its mirror, where the old scaler gave −9 against
        −150; a sweep over 3,149 positions (the bench and WAC positions, ten
        odd- and promoted-material positions, each with a seeded eight-ply
        walk, `std::mt19937_64`) finds none asymmetric, where the old scaler
        gave nine. No other asymmetry was found.
      - `bench 13` 7,384,101 (from 7,233,823); the legacy arm 17,606,287
        (from 15,887,213), since the evaluation is shared; CTest ON 24/24,
        OFF 21/21.
      - It changes the evaluation, so it owed the repair gate: BAS-S31,
        launcher `tools/run_a117_gate.ps1`, maintainer-run.
      - Closed 2026-10-26: BAS-S31 accepted H1, +5.9 ± 6.3 Elo (3,204
        games, LLR +2.96, no faults); the repair stays by the registered
        rule. The development fingerprint is `bench 13` 7,384,101; the
        legacy arm 17,606,287.
      - Found beside it, not changed: the same scaler's near-draw factor
        reads `npm(weak) > 0 ? 4 : 14` where the donor has
        `npm(weak) <= BishopValueMg ? 4 : 14`
        (`sf9587-ks/src/material.cpp`). Ours scales a weaker side holding a
        rook (KRBKR, KRNKR) to 4/64 where the donor gives 14/64, and our 14
        is unreachable.
    - [x] **A.11.8** `[I1]` Info lines: bestmove after a stop, mate scores as cp
      (A-26, A-31) After 3 s the aspiration loop prints a `lowerbound` line
      for a new move that is failing high (`search_worker.cpp:450-459`); a
      hard-limit or GUI stop then discards the iteration (`:515`), `bestmove`
      is the previous depth's move, and at Threads 1 nothing is printed after
      it (`search_thread_pool.cpp:193-194`; `needs_pool_line` compares
      results, not the last printed line). Rarog's fix (`97ea52d`) records the
      move its last line named and prints the result's line when they differ.
      At a tablebase root the bound line also prints the raw bound, not the
      tablebase score. Output only; exact bench.
      Mate scores printed as `cp` (BAS-S27, maintainer decision 2026-10-25):
      - Without tablebases, both 1.10.2 and the head sometimes print a
        mate-range score as `cp`, for example `cp -31742`, which is beyond
        the 128-ply mate window.
      - Both also print `cp` values in the tablebase band (`cp 20000 -
        plies`) in won endings: 111 move scores in 64 of the `ltc` read's
        games, all decisive.
      - The cause is not located yet. Find which values reach the UCI score
        unconverted.
      - Test: a score is `mate n` exactly when it is a mate, and a `cp` value
        is in the tablebase band only when a tablebase or tablebase-backed
        entry produced it.
      - If the cause is in the search rather than the display, the leaf
        owes bench accounting and A.11's repair SPRT.
      - Closed 2026-10-26 (`accaac0`), after A.11.7, output only, `bench 13`
        exact:
        - The search records the first move of its last line-1 `info` and,
          when `bestmove` names another, repeats the result's line (Rarog's
          rule). At a tablebase root a failed window prints the move's
          tablebase score without a bound. The bound lines' 3 s delay is a
          `SearchLimits` field so a test can open it.
        - The "mate scores as cp" premise was false. `cp -31742` is the
          rule-50 downgrade: `score_from_tt` turns a mate or tablebase
          result the clock may spoil into the strongest non-decisive score,
          as Stockfish's `value_from_tt` does. The `cp 20000 - plies`
          values without tablebases were evaluations, not tablebase results:
          won-ending evaluations print raw and run through the display band
          (the KBNK drive reaches 31,660). In the `ltc` read's games the
          first such score came from 1.10.2 in 62 games and from the head in
          2, both bishop endings at `cp -19873` and `-19883` that fresh
          searches show as ordinary evaluations. The repair is in the
          display: an evaluation prints at most ±19,871, below the band.
        - Tests in `test_search`: a stopped fail-high search ends on the
          line of the move played (the old code ended on `e2e3` while
          playing `h2h3`); a tablebase root prints no bound (old: 7
          bounded lines); evaluations of 25,000 and the rule-50 value print
          as ±19,871. CTest ON 24/24, OFF 21/21; `bench 13` 7,384,101 and
          the legacy arm 17,606,287, both exact. No gate is owed.
    - [x] **A.11.9** `[R2]` Override boundaries never reward shedding material
      (C-9, C-131) The KBNK and KXK overrides (`kbnk_score` base 15,600;
      `kxk_score = KNOWN_WIN + ...`, `eval.cpp:306`, `:520`) apply only
      without the winner's pawns, so KBNP-K to KBN-K, and KBP-K promoting to a
      knight or an opposite-colour bishop, score far above the generic score
      of the position before; BAS-E49 saw KBP-K convert "via knight
      promotion". Rarog's rule (RAR-E53, RAR-E54): a replacement score stays
      above every neighbour one capture or one promotion away, held by a unit
      test with a wire check. Research first (how often play meets these
      boundaries, and the cost in games), then a design. It changes the
      evaluation during the freeze, so a repair needs the maintainer's
      B.10-style admission.
      - Research 2026-10-26: over 26,574 gate and read games, about 1.3% of
        `3+0.03` games and 0.3% at `10+0.1` entered an override from a
        position with a pawn, almost always by underpromotion; the strong
        side then drew 17 of 352 (4.8%) positions a queen wins. The cause:
        queen and rook mates scored ordinarily (about 2,400 for KQB-K)
        under KBNK's 15,600 and KXK's 10,650. RAR-E53/E54 are not in the
        pinned snapshots, so the design rests on Basilisk's own rule that
        more material never scores lower against a bare king.
      - Admitted into the freeze and implemented (`fdf4277`, maintainer
        decision 2026-10-26): tiers by the winner's strongest piece, queen
        (28,700 to 31,742) above rook (26,100 to 28,699) above the
        minor-piece mates. The queen and rook tiers carry the ordinary
        score; KBNK keeps its weights with its base at `KNOWN_WIN`; pawns
        add 3 each within a minor tier.
      - Screens: tiers with only a small drive lost KR-K (2/4) and left a
        wandering queen in a tablebase analysis line, so the queen and rook
        tiers carry the ordinary score; then KQ-K 4/4, KR-K 4/4, KBB-K 6/6,
        KBN-K 6/6 and the WAC floor hold. On the 342 positions where games
        underpromoted, `master` still underpromotes in 337; the candidate
        in none (289 queen promotions). `bench 13` 7,857,713 (+6.4%); the
        legacy arm 23,696,970; CTest ON 24/24, OFF 21/21; a `test_eval`
        section on promotion order and pawn shedding fails on the old
        evaluation.
      - Gate BAS-S32, `[-5,5]`, launcher `tools/run_a119_gate.ps1`,
        maintainer-run.
      - Closed 2026-10-26: BAS-S32 reached its 4,000-pair cap at
        −0.3 ± 4.0 Elo (8,000 games, LLR −0.33, no faults); the repair stays
        by the registered rule on the maintainer's confirmation. The
        development fingerprint is `bench 13` 7,857,713; the legacy arm
        23,696,970.
      - On the same branch, A.11.8's repeated final line goes through
        `send_info` (`28b7018`), so in analysis it gets the tablebase
        extension other lines get; output only, `bench 13` exact.
      - Also on the branch (maintainer decision 2026-10-26), root
        underpromotions are judged against the queen (`1641e64`): a rook or
        bishop promotion beats the queen promotion on the same squares only
        by more than 200 of exact score, and the queen is searched first.
        A transposing rook line had displaced the queen in 59 of 228 gate
        positions (7 of them errors by Stockfish 18); now 1, with the
        stalemate escapes and Saavedra's `c8=R` kept. Pruning
        underpromotions below the root instead lost Saavedra. `bench 13`
        exact, NPS unchanged. Gate BAS-S33, `[-5,5]`, launcher
        `tools/run_a119_up_gate.ps1`, maintainer-run: H0 at 3,285 pairs,
        −2.7 ± 4.1, so the rule is reverted (`c1791b3`), keeping the
        stalemate and Saavedra tests. A replay found its choices no worse
        (Stockfish 18: 21 better, 148 equal, 8 worse), and the rule acts in
        only about 6% of games, so BAS-S34 re-tests it from the 3,159
        positions where the arms differ (`[-5,0]`, launcher
        `tools/run_a119_up_retest.ps1`, maintainer-run). It reached its
        3,159-pair cap at −0.3 ± 1.1 Elo on that book (about 0.02 Elo in
        ordinary play), so the rule is re-applied (`d7d2c34`) by its cap
        rule on the maintainer's confirmation; `bench 13` 7,857,713, the
        legacy arm 23,696,970; CTest ON 24/24, OFF 21/21.
    - [ ] **A.11.10** `[R2]` Fail-soft quiescence
      (B-19) Pulled forward from B.4 by maintainer decision 2026-10-26, so
      that B.3 is not fitted around a known defect. Quiescence fails hard:
      a fail-low returns `alpha` (delta pruning and the final return in
      `quiescence`, `search_kernel_core.cpp`). The exact `alpha` echoes up
      the tree. Root fail-highs then equal beta and drive aspiration
      cascades (93% of Rarog's), and inside the equal-score tie rule's
      window (`ply + 2 >= root_depth_`) an "equal" score is usually a
      bound, so below the root a refuted move becomes the node's best move
      one time in eight (A.11.6 kept the root out). Stockfish's quiescence
      is fail-soft throughout.
      - Research first. The only record of the earlier attempts is two
        comments in `quiescence`: a fail-soft delta-pruning return broke the
        KBNK fixed-depth conversion canary, and seeding the final store from
        the TT-tightened stand-pat turned a KQK mate in 5 into `mate 63`,
        because a stand-pat raised by a lower bound is not an upper bound.
        Each changed one return alone. Re-measure both, then specify every
        return, the stand-pat tightening and the stored bounds together,
        against Stockfish's.
      - Obligations: the KBNK and KQK fixtures and their mate distances, the
        conversion floors, WAC, bench accounting, and a diagnostic count of
        tie-rule promotions of a bound, read before and after.
      - Gate: a `[-5,5]` repair SPRT. RFP, razoring and aspiration were
        fitted on fail-hard values; B.3 to B.5 refit them on the repaired
        search.
- [ ] **A.12** `[M]` Roadmap rules from the audit
  Added 2026-10-24, for the maintainer to decide one by one: Rarog rule 7c's
  amendment of 2026-10-20 (a near-optimum surface may be tuned as one longer
  run with a larger perturbation and a smaller gain ratio); the standing
  contracts Rarog keeps that Basilisk's table lacks (the allocation guard once
  B.7.1 lands, diagnostic counter units, the Texel data contract once C.2.1
  writes it, board footprint assertions, cross-engine board benchmark parity);
  and whether `agents/research.md`'s snapshot-only rule for reading Rarog gets
  a standing exception for audits the maintainer asks for (waived for the
  2026-10-24 audit).

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

- [x] **B.0** Search programme investigation
  Investigation (`R3`): the Basilisk-versus-Stockfish mechanism map, cluster
  contents, scale ratio and seed rule, survivors, SPSA surfaces, B.2.2's
  registered screen numbers, frozen B.2 predictions and handoffs for B.1–B.3.
  No engine source changes.
  **Research delivered 2026-10-06:** `analysis/search_programme_2026-10-06.md`
  ends `READY_FOR_IMPLEMENTATION` for B.1 and B.2, B.3 contingent on the
  accepted B.2 head, with the donor pinned at Stockfish `sf_19` (`edb0d9db`).
  Measured on the 1.10.1 head (BAS-D20, BAS-D21): per-ply growth equals the
  oracle's (1.767 against 1.757) but the tree is a constant ×4 from depth 4
  on; with the shallow move-loop pruning family removed on both matched
  ablation builds the trees are equal at depth 4, so that family carries the
  multiplier (oracle 4.5× of selectivity against Basilisk's 2.2×); Basilisk's
  razoring costs 51 WAC positions at 100k nodes and its LMR 15, where the
  oracle's cost none; the evaluation scale is 0.282 of Stockfish 19's and
  0.557 of the oracle's with a residual over half its own magnitude, so the
  oracle column seeds every evaluation-unit constant. Phase 5's "width is a
  symptom" premise stays refuted; the check policy is not the differential
  (both extension families cost 1.9× nodes for no fixed-node tactics) and
  stays B.3's. Retry triggers recorded: BAS-S07/S10 fire at B.2, BAS-S08/S09/
  S11 at B.3, BAS-S12 not fired. Two cheap maintainer-run game tests were
  registered and spawned below; neither blocks B.1, and B.0 closes when both
  have been read into the packet's calibration.
  — CLOSED 2026-10-08: both read into packet §11, inside their 80%
  intervals; no falsifier fired, the cluster order and P2 stand.
  The deliverable covers:
  1. The donor revisions pinned.
  2. Zero-game measurements on the 1.10.1 head, with the A.5 instruments:
     - the branching profile and the depth reached at 300k nodes;
     - WAC at fixed nodes;
     - stride-1 counters, including the share of LMR reductions that land in
       qsearch, the re-search rate after reduced fail-highs, NMP conversion,
       the qsearch share, the check-extension population (BAS-S16 measured
       15.84% of interior nodes) and history-pruning activity (last measured
       near-dead).
  3. The scale ratios and the three-column seed rule.
  4. The mechanism map, per area: node entry (draw value, upcoming
     repetition as BAS-S12's retry, mate distance); TT; static eval and
     correction; node-level pruning; singular and extensions, including
     Basilisk's check extension; the picker and histories, including whether
     killers go; move-loop pruning and LMR; plus qsearch, root and time as
     boundaries.
  5. The interaction map and shared signals.
  6. Cluster contents confirmed or changed, and the board producers the
     clusters need.
  7. Survivors with local evidence (operating rule 4).
  8. SPSA surfaces, with categorical switches kept apart.
  9. Counter re-keying and the ablation-mask disposition.
  10. B.2.2's screen numbers: a branching window, WAC floors and targets,
      the agreement floor, the time-to-depth floor, and the paired-run floor
      and target.
  11. Frozen predictions for B.2.
  12. Handoffs for B.1, B.2 and, contingent on B.2's head, B.3.

  A.2.3's `analysis/parameter_inventory_v1.md` is an input: the live list,
  and no SPSA surface moves `LmrBase` while capture futility is in the tree.
  It also answers the four `DESIGN.md` questions for the programme. It
  tests Phase 5's "width is a symptom" premise, and it records which retry
  triggers the programme fires: BAS-S07, BAS-S10 and BAS-S12 name a history
  ownership change; BAS-S08 and BAS-S09 name a joint architecture and fit.
    - [x] **B.0.1** `[V]` Razoring depth reach in Elo (maintainer)
      BAS-S17, registered 2026-10-06: `RazorCoeff=500` against 243 on the
      B.0 Tune binary, 2,000 paired games at `3+0.03`, no adjudication,
      through the A.3.1 run-file path; a Tune-build diagnostic, never
      acceptance. Frozen: +8 Elo, 80% interval [−4, +20]. The reading rule
      in BAS-S17 decides whether B.2 seeds razoring from the oracle column
      (depth 1, 256 cp) without a categorical re-test. Prepare the run file
      and the one runnable command; append the calibration to BAS-S17 and
      to the packet's §11.
      Prepared 2026-10-08: `tools/colosseum.ps1 -Mode match`
      (`match-fixed.toml`), one binary two option sets, 1,000 paired
      openings, seed 20101, 14 slots, revision and bench pinned; dry run
      accepted.
      — CLOSED 2026-10-08: **+2.4 Elo**, 95% interval [−7.5, +12.3]
      (pentanomial [50, 224, 431, 252, 43], 0 faults, every game ended
      naturally). The interval holds 0, so neither branch of the reading rule
      fires and B.2 decides razoring's depth cap with the cluster.
      Calibration: inside the 80% interval [−4, +20], 5.6 below centre.
    - [x] **B.0.2** `[V]` Oracle move-loop pruning family in Elo (maintainer)
      BAS-S18, registered 2026-10-06: `oracle-1.10.1-ablate.exe` with
      `AblationMask=32` (its shallow move-loop pruning off) against Basilisk
      1.10.1 at equal time, the BAS-O05 recipe at 1,000 cycles (2,000
      games); G(32) = 1500 − Basilisk's rating. Frozen: G(32) = +110, 80%
      interval [+40, +180], so the family explains about 200 of the 312.6.
      G(32) ≥ +250 lowers B.2's prediction P2 and re-opens the cluster order
      before B.2.1. Prepare from `tools/run_a73_oracle_g0.ps1` with the
      ablate binary and option; append the calibration to BAS-S18 and §11.
      Prepared 2026-10-08: `tools/run_b02_oracle_g32.ps1`, BAS-O05's
      launcher with the ablate binary, `AblationMask=32`, 1,000 cycles and
      seed 20102; dry run accepted, and the mask is live (startpos depth 14:
      445,620 nodes against 131,089 at mask 0).
      — CLOSED 2026-10-08: **G(32) = +102.3 ± 15.9** (Basilisk 1397.7;
      oracle 868-838-294, 64.35%; 0 faults, every game ended naturally).
      G(0) − G(32) = **210.3 ± 23.9** of 312.6: the oracle's move-loop
      pruning family is worth about two thirds of its lead. At mask 0 the
      ablate oracle searches the same tree as G(0)'s oracle (967,078
      nodes) at the same NPS within 1%. No falsifier fired, so P2 and the
      cluster order stand. Calibration: inside [+40, +180], 7.7 below
      centre.
- [x] **B.1** `[I1]` Search restructure, behaviour-neutral
  `src/search.cpp` split into modules, node types as template parameters, a
  per-thread worker and stack separated from per-search configuration and
  engine-owned shared resources, inert parameters removed per A.2.3; exact
  fingerprint and pooled-PGO NPS within noise.
  The move table comes from A.6 and B.0. Release and sanitizer CTest, and
  exact `bench 13` at every commit.
  Handoff frozen by B.0 (packet §12.1, 2026-10-06): A.6's tickets 1–8 with
  `NodeType`, the A.2.3 removals and the two comment corrections; killers,
  countermove, low-ply history and every live mechanism stay byte-identical
  in behaviour; the B.0 zero-game baselines (packet §10) are reproduced on
  the B.1 binary exactly except NPS. Does not wait on B.0.1 or B.0.2.
  Implemented 2026-10-08, `456cf6a`..`bf44834`, one commit per ticket:
  - the A.2.3 removals: `time_limit_`, `RootMoveStat`, capture futility,
    quiet SEE pruning, qsearch quiet checks and the post-LMR nudge with
    their 7 coordinates, `KBNK Drive`; the `DoubleExtMax` note corrected
    (the capture-futility comment went with its code);
  - `search_types.h`, `search_root`, `search_diagnostics`,
    `search_history.cpp`, `move_picker`, `search_worker` with
    `SearchConfig` / `SearchShared` / `SearchState`, `NodeType`,
    `search_thread_pool`, and `search.h` as the facade.
  Bench 14,978,465 at every commit on PEXT, non-PEXT, Tune, Diag and
  Ablate; release and Tune CTest 19/19, sanitizer CTest 17/17 (`-LE
  strength`), the Texel build reconstructs exactly; Diag, decision trace,
  MultiPV, tablebase, pool and time-budget output match the pre-B.1 Tune
  build line for line. Packet §10 reproduced on the B.1 PGO builds
  (`tools/results/b1-qual/`): branching 1.767 and every position at every
  depth; WAC at 100k / 400k nodes 204 / 242, identical to the A.8 head in
  every field; LMR-off WAC 219 (+15); the mask 0 / 32 profiles cell for
  cell; counters and the oracle differential (66/105, median depth 14
  against 18) identical except the mated root `Fool's mate`, which the B.0
  binaries searched once per iteration and A.8.8 reports without
  searching; canaries 77/77.
  — CLOSED 2026-10-09: the NPS gate BAS-P16 (maintainer-run) reads B.1
  **+1.21%** against the A.8 head, 95% CI [+0.57%, +1.44%], with the self
  pair at −0.04%. It passes, and the gain is recorded unexplained. B.1's
  pooled median, 4.216M NPS, is B.2's NPS reference.
- [x] **B.2** Cluster 1 — the selectivity core
  Cluster 1 — the selectivity core; final contents fixed by B.0.
  Expected contents:
  - TT use: the TT-adjusted eval, raw eval stored on a miss, and the
    graph-history check on cutoffs;
  - the correction histories and the corrected eval, with the correction
    magnitude as an input to margins and reductions;
  - the history set and its update policy (tables, TT-cutoff bonus,
    eval-swing training, fail-low parent bonus, post-LMR update, malus
    decay; killers dropped if B.0 confirms);
  - the move picker;
  - move-loop pruning;
  - the LMR formula and re-search rules;
  - RFP and razoring on the corrected eval;
  - the hindsight depth adjustment and cutoff counting.

  NMP, ProbCut, singular and extensions keep Basilisk's forms in this
  cluster so that B.3 can measure them separately. Gated at `[0,10]`,
  because the prior is large.
  Contents fixed by B.0 (packet §5, §12.2, 2026-10-06), changing the list
  above in five places: razoring in the donor's return shape seeded from
  the oracle column (depth 1, 256 cp; the cap categorical); RFP in
  Stockfish 19's shape with the TT-move condition and the blended return;
  a persisted `tt_pv` bit taken from the age field; killers and countermove
  dropped, low-ply history kept; and the move-loop family (count pruning
  as a picker skip, history-adjusted `lmrDepth`, continuation-history
  pruning, quiet SEE pruning, capture futility, fail-soft futility) named
  as the primary target, since it carries the ×4 node multiplier
  (BAS-D21). Seeds, surfaces, screens and the frozen predictions P1–P6 are
  the packet's §7, §8, §10 and §11.
    - [x] **B.2.0** `[R2]` Architecture review and neutral upgrades
      Architecture review of the B.1 head: the seams the cluster needs, and
      behaviour-neutral upgrades landed at the exact fingerprint before the
      first mechanism ticket.
      Rarog's review landed twelve such upgrades.
      Reviewed 2026-10-10 on the B.1 head `8c706a8`:
      `analysis/b20_architecture_review_2026-10-10.md`. The head needs no
      structural rewrite; the weight sat where the cluster presses. Landed at
      the exact fingerprint (`c434ee4`..`9b51b46`): the node kernel split out
      of the driver behind three entry points (`root_search`,
      `kernel_begin_search`, `kernel_new_game`) with Searcher's kernel members
      in one fenced section; the `B2_CORE` umbrella option, inert, both arms
      at 14,978,465; the counter, parameter (root, legacy kernel, core kernel)
      and TT-layout tables made one-source; the driver's comments without
      retired numbers (391 → 288 references in `src/`); the SPSA surface
      regenerated; a source census tool. Release PEXT and plain, Tune, Diag,
      Ablate and the sanitizer build at the fingerprint; release CTest 19/19,
      sanitizer 17/17; the Tune option set, `Diag` and trace output
      byte-identical to the B.1 Tune build. The review's §5 refines B.2.1's
      contract to the code: the fence, the per-ply record, the tables and
      their 9.2 MiB footprint, the TT encoding with the miss-store's
      occupancy rule, the picker and threat-producer contracts, 51 seeded
      coordinates and 6 categorical switches, the tests. Remaining gate:
      BAS-P17, the pooled-PGO NPS pool against the B.1 builds (maintainer-run
      on an idle host, `tools/run_b20_nps_gate.ps1`), passing above −0.5%.
      — CLOSED 2026-10-10: BAS-P17 (maintainer-run) reads the B.2.0 pair
      **0.00%** against the B.1 pair, 95% CI [−0.22%, +0.28%], self pair
      −0.03% OK, fingerprints identical; the frozen 0.0% held at its centre.
      B.1's 4.216M pooled median stays B.2's NPS reference.
    - [x] **B.2.1** `[I2]` Implement behind the umbrella switch
      Implement to the B.0 handoff behind one umbrella CMake option, OFF by
      default, as ordered tickets; ticket 0 is the decision trace hook.
      Each ticket keeps the umbrella-off arm at the exact fingerprint. Unit
      tests cover every table's bounds and gravity, picker exhaustiveness, TT
      store and probe, and stack unwind. CTest passes on both arms: the
      mate-drive floors bind the on arm too (the WAC floor from ticket 7,
      Amendment 2), and a failure there returns the premise to research.
      Reviewer acceptance (a separate `R2` session) is
      recorded before B.2.2.
      Tickets 0–2 landed 2026-10-10 (`1dbf0d9`..`f179404`): the counters and
      trace version 2, the threat producer and the core's stack record, the
      TT encoding with the miss-store and the cutoff rules. The off arm stays
      at 14,978,465 at every commit; the ON arm reads 13,488,940; CTest
      passes on both arms.
      — RETURNED 2026-10-10 (Return 1, packet §16): Basilisk's `see_ge`
      answers `0 ≥ threshold` for every quiet move, so the picker's
      `see_ge(−75)` check gate and quiet SEE pruning are inert as written.
      Research amendment 1 decides before ticket 3; the implementation
      recommends the donor's quiet-move semantics for `see_ge`.
      — AMENDED 2026-10-10 (Amendment 1, packet §17): `see_ge` takes the
      donor's quiet-move semantics (option a); decisions 2–13 confirmed except
      razoring above depth 1, now linear `margin·d`; P5's sources re-registered.
      Implementation resumes at ticket 3.
      Ticket 3's first commit landed 2026-10-10 (`ea5c49f`, `60a610e`):
      `see_ge` evaluates quiet moves, tested against the legality oracle;
      neither arm's fingerprint moved.
      — RETURNED 2026-10-10 (Return 2, packet §18): the rest of ticket 3 (the
      core tables, the picker, the update routing; held as a patch) fails
      the ON arm's WAC floor, 119/300 at depth 6 against 130. The picker's
      SEE-gated check bonus costs 13 of the 19 positions under the legacy
      pruning and LMR still in place. Research amendment 2 decides; the
      implementation recommends a categorical switch for the gate.
      — AMENDED 2026-10-10 (Amendment 2, packet §19): the check gate stays
      as contracted, no switch; the depth-6 WAC floor binds the ON arm from
      ticket 7, recorded as a count per ticket before that with §10's
      100k-node reading beside it. Implementation resumes at ticket 3's
      remainder from the held patch.
      Ticket 3 landed 2026-10-10 (`9a632e6` count mode, `bcf63e8` tables,
      picker and routing; record in packet §20): ON arm 12,815,895
      (transient), off arm 14,978,465; CTest passes on both arms; the ON
      arm's WAC reads 119 at depth 6 and 200 at 100,000 nodes.
      Ticket 4 landed 2026-10-10 (`221afdf`; packet §20): the cluster's
      correction form and admission, `improving`, opponent worsening,
      hindsight and eval-difference training. ON arm 13,061,601, off arm
      14,978,465; CTest passes on both arms; WAC 127 at depth 6 and 193 at
      100,000 nodes.
      Ticket 5 landed 2026-10-11 (`bb73fb5`; packet §20): razoring at
      depth 1 with the quiescence value returned, and RFP in the cluster's
      shape. ON arm 25,182,694 (one bench position found a forced mate),
      off arm 14,978,465; CTest passes on both arms; WAC 167 at depth 6
      (+40: razoring +43, RFP +5, −8 interaction) and 237 at 100,000 nodes.
      Ticket 6 landed 2026-10-11 (`573add0`, `8337944`; packet §20): the
      move-loop pruning family on the history-adjusted reduced depth. ON
      arm 11,922,648, off arm 14,978,465; CTest passes on both arms; every
      member fires at `bench 13`; WAC 153 at depth 6 (−14: the legacy family
      removed +14, the new family −28, capture/check SEE −23 of it) and 235
      at 100,000 nodes.
      Ticket 7 landed 2026-10-11 (`b02fcb7`, `802ed5c`; packet §20): the
      cluster's LMR, its floor and re-search rules, `depth −= 3`, the
      fail-high blend. ON arm 10,739,013, off arm 14,978,465; CTest passes
      on both arms with the ON arm's depth-6 WAC floor binding again (176
      against 130; +23: LMR +29, `depth −= 3` −8); 241 at 100,000 nodes.
      Windows sanitizer builds now reserve an 8 MB stack, as Linux does.
      Ticket 8 landed 2026-10-11 (`4112db0`; packet §20): the cluster's
      update policy (exit-time training of every alpha-raising node, the
      fail-low parent bonus, the post-LMR update, the TT-move history) and
      the per-search decay. ON arm 6,337,226, off arm 14,978,465; CTest
      passes on both arms; WAC 185 at depth 6 and 249 at 100,000 nodes.
      The ablation bits are live (8 of 8), and the four decision traces are
      recorded.
      — IMPLEMENTED 2026-10-11: implementation record in packet §20, with
      the reviewer's checklist. Reviewer acceptance (a separate `R2`
      session) comes before B.2.2.
      — RETURNED 2026-10-11 (Return 3, review, packet §21): every
      fingerprint reproduced from clean sources (149 of 149), CTest on both
      arms, WAC 185 at depth 6 and 249 at 100k nodes; not accepted. Two
      defects: `stat_score` never reset at node entry, and the PV
      TT-move depth rule inert on the first move. One gap: the
      stack-unwind test. Two contract questions: the first child's
      node type, and two donor constants on evaluation units left
      unconverted. Research amendment 3 rules on the questions; then
      implementation lands all five and a delta review accepts them.
      — AMENDED 2026-10-11 (Amendment 3, packet §22): the first child at a
      non-PV node typed `!cut_node` with the donor's first-move step-down;
      three evaluation-unit conversions (`|corr|/7419`, the LMR alpha term
      and the eval-difference training on scaled differences); F1, F2 and
      F5 confirmed; two more defects (the per-ply move count surviving
      same-slot re-entries; the absent step-down). Implementation lands
      eleven items in order, then the delta review. B.2.2–B.2.4 registered
      as BAS-S21–S23.
      — RETURNED 2026-10-11 (Return 4, packet §23): items 1–5 landed
      (`31c80db`..`90d7db7`); ON arm 5,941,186, WAC 172 at depth 6 and 250
      at 100k nodes; the off arm stays at 14,978,465. Item 6's `|corr|/7419`
      never finishes `bench 13`: where the correction saturates it removes
      up to 9.3 plies of reduction, against the donor's 2.6. Research
      amendment 4 decides the divisor; items 6–11 resume after it.
      — AMENDED 2026-10-12 (Amendment 4, packet §24): item 6 is the converted
      slope under the donor's bound, `min(|corr|/7419, 2695)`; rule 4 gains a
      saturation clause; the item-2 deviation is accepted; the correction
      bound's own conversion is a registered B.2.3 question. Items 6–11
      resume in order, then the delta review.
      Return 4 resume landed 2026-10-12 (items 6–10: `45f4827`..`7f1b5bb`;
      items 1–5: `31c80db`..`90d7db7`; record in the packet's §20 addendum):
      ON arm 9,132,715 at `bench 13` on every flavour, `bench 10`
      1,678,409 release and sanitizer; off arm 14,978,465 on every flavour,
      sanitizer 3,190,673. CTest: ON 23/23 release and 21/21 sanitizer, off
      20/20 and 18/18. ON WAC 185 at depth 6 and 249 at 100k nodes.
      `test_stack_unwind` and the node-entry checks were proven live with
      two planted stale fields. The R2 delta review (§21.4) accepts before
      B.2.2.
      — ACCEPTED 2026-10-12 (delta review, packet §25): every fingerprint of
      items 1–10 reproduced from clean sources on both arms, CTest on both
      arms, WAC 249 / 204 at 100k nodes, the changed lines exact against
      §22.6 and §24.1, the planted proofs re-run; two follow-ups bind
      nothing here. CLOSED.
    - [x] **B.2.2** `[V]` Diagnostics and the unfitted paired run
      Screens at B.0's registered numbers; the ablation sweep if a floor
      fails; the unfitted 2,000-game paired run against the accepted head,
      maintainer-run, which governs. Categorical switches the review asks for
      are settled by 2,000-game paired runs on one tune build and never
      become SPSA coordinates.
      Screens read 2026-10-12 on the accepted head (packet §26, BAS-S21):
      two floors flagged. The canaries read 68/77; eight of the nine
      regressions come back with the move-loop pruning family removed, and
      there are 30 new passes, WAC.001 among them. Branching 4–14 reads
      1.6465 against the window's 1.65, carried by the LMR at depths 12–14.
      Nodes at depth 12 relative to the oracle read 1.86 (median 1.04); WAC
      249 at 100k and 276 at 400k nodes; the LMR-off delta +8; the
      shallow-pruning factor 3.46; agreement 66/105; time to depth 0.59×;
      pooled NPS 1.038× the B.1 head. P6 held; BAS-C09's trigger was not
      met. Prepared, not started: the paired run and the six categorical
      runs (`tools/run_b22_games.ps1`, `526a68d`; dry runs accepted). The
      paired run governs, and the review decides on the flagged floors
      with its result.
      Games 2026-10-13 (maintainer-run, packet §26.5, BAS-S21): the paired
      run reads **+130.9 ± 11.4 Elo** (ON against OFF), which proceeds to
      B.2.3; P1's −15 ± 30 missed high. `CoreLmpExemptChecks` is adopted at
      0 (+13.6 ± 9.7); the other five switches keep their defaults
      (`CoreCorrAdmission` = 0 costs −96.9). The fastchess cross-check
      agrees (+136.2 ± 11.0, 2026-10-13).
      — CLOSED 2026-10-13: `CoreLmpExemptChecks` defaults to 0 (`cd2e397`).
      The ON arm's `bench 13` is now 6,199,439 (`bench 10` 1,598,399
      release and sanitizer) and the off arm 14,978,465 on every flavour.
      CTest passes on both arms (ON 23/23 and 21/21, off 20/20 and 18/18);
      ON WAC reads 173 at depth 6 and 249 at 100k nodes. B.2.3 starts from
      this head.
    - [x] **B.2.3** `[V]` Curvature sweep, then SPSA in blocks
      Curvature sweep, then SPSA in blocks (rule 7c) over the cluster's live
      coordinates, maintainer-run; theta baked in one engine commit.
      Sweep read 2026-10-13 (`analysis/b23_sweep_2026-10-13.md`, classification
      frozen before the first point): `CoreRfpPerPly`, `CoreLmpSquareScale`,
      `CoreLmrOffset` and `CoreCorrUpdateScale` curved, `CoreQuietFutBase`
      monotone, so the tune runs. Every curve peaks at the seed (WAC 249 at
      100k), so the curvature is shallow. Block 1 is registered in BAS-S22
      and prepared, not started: the core surface's 50 coordinates (now
      generated, `e24914f`) on the ON Tune build at `526dd49`; runner
      preflight and dry run accepted.
      — AMENDED 2026-10-13 (Amendment 5, packet §27): before block 1, one `I1`
      commit gives the picker's capture score, its threat term and LMR's
      capture `statScore` the donor's `PieceValue` (history-unit sums),
      converts the good-capture split to `−score/47`, seeds
      `CoreRfpCorrMult` at 676 and fixes `CoreQuietSeeCoeff` off the surface
      (49 coordinates); then the sweep re-runs on the new binary and block 1
      is re-registered. The eight legacy coordinates stay B.3's.
      Amendment 5 landed 2026-10-13 (packet §28: `3eea3c5` engine; `e24114a`,
      `b5983c6` tooling). The ON `bench 13` is now 6,096,792 and the off arm
      14,978,465; CTest passes on both arms. ON WAC reads 177 at depth 6 and
      237 at 100k (from 249); first-move cutoffs rise 88.5% → 89.1% and
      good-capture cutoffs 21.0% → 24.9%. The sweep re-run on the new Tune
      binary finds three coordinates curved and the seed now at or near the
      lowest WAC point; block 1 is re-registered (49 coordinates) and
      prepared, not started.
      Owed before the bake (maintainer, 2026-10-14): a written analysis of
      the fitted constants, not only their gate. Every coordinate that
      moved a full step, ended at a range edge or moved against its seed
      column (§7.3) is read for what it says about implementation,
      design, unit or feature cohesion. Examples: a mechanism tuned toward
      off, two terms moving to cancel, a margin compensating for a
      mis-scaled input, a rail that hides a wanted value outside the range.
      Each finding gets a proposed fix and an owner leaf. Block 1 already
      shows two rails at iteration 1,530: `CoreIirMinDepth` 4 → 8 (the
      kept IIR barely wanted) and `CoreResearchShallower` 2 → 0.
      Block 1 complete 2026-10-14 (60,000 games, 0 faults): six coordinates
      moved a full step, so the movement rule says CONTINUE. The interim
      analysis is `analysis/b23_constants_2026-10-14.md`: IIR tuned off
      (B.3), the shallower re-search firing only on fail-hard bound echoes
      (B.4, B.8), RFP's seed column conservative (C, B.9), the `cutoffCnt`
      terms (B.3), and the correction rate (the registered `CoreCorrLimit`
      question). Block 2 is prepared (seed 23002, from block 1's centres),
      maintainer-run. After the last block: the final analysis, the bake,
      then B.2.4's gates whatever the analysis finds.
      Block 2 complete 2026-10-16 (60,000 games, 0 faults): two full-step
      movers, so the rule says STOP; theta is block 2's rounded centres.
      The final analysis (`analysis/b23_constants_2026-10-14.md`) carries
      seven findings with owners (IIR tuned off and the shallower re-search
      inside its dead zone, B.3 and B.4/B.8; RFP's seed, C and B.9; the
      correction wanting more weight, which argues against running the
      `CoreCorrLimit` categorical; histories weighing more, the `CoreHistoryFills`
      residual); none holds the bake.
      — CLOSED 2026-10-16: theta baked in one engine commit (`b777ba8`; 46
      of 49 defaults change), surface regenerated (`2ab17c7`). The ON
      `bench 13` is now 11,099,210 (`bench 10` 2,034,942) and the off arm
      14,978,465 on every flavour. CTest passes on both arms (ON 23/23 and
      21/21, off 20/20 and 18/18); ON WAC reads 181 at depth 6 and 241 at
      100k. The endgame cohort at 600k nodes holds every floor (KBN-K 23/24
      inside its slack; BAS-S23). B.2.4's gates are registered and
      dry-run.
    - [x] **B.2.4** `[V]` Cluster 1 gates
      Gate: (a) the unfitted arm against the umbrella-off arm at the same
      revision, `[0,10]`, once its PGO binaries exist; (b) the fitted arm
      against the unfitted one, `[0,10]`, after the bake.
      Each gate accepts one thing on its own evidence, so a weak tune cannot
      ride on the cluster's margin. The default flips to the accepted arm;
      the legacy search stays compilable until B.8.
      — CLOSED 2026-10-16 (BAS-S23): both gates accept H1. (a) the unfitted
      arm against the off arm, **+136.2 ± 33.6 Elo** (252 games); (b) the
      fitted arm against the unfitted, **+58.7 ± 20.9 Elo** (430 games).
      Chained, B.2 reads about +195 Elo over the B.1 head; P2's +40 missed
      high. `B2_CORE` now defaults ON (`8a10d14`): the development
      fingerprint is `bench 13` 11,099,210, and the legacy kernel builds
      with `-DB2_CORE=OFF` (14,978,465), tested in CI (`2853699`), until B.8.
- [x] **B.10** `[I1]` PST orientation repair (ANY TIME) — before B.3
  Correctness repair, admitted into the evaluation freeze by maintainer
  decision (2026-10-23): fix it before B.3 starts fitting to the eval. Seven
  piece-square tables have been stored rank-mirrored since v1.0.0: mg and eg
  knight, bishop and queen, plus eg king. They hold PeSTO's rank-8-first rows
  on a1-first squares, so, for example, a White knight on f3 gets f6's +110.
  Pawn, rook and mg king are correct. Evidence is in
  `analysis/pst_orientation_2026-10-23.md`, all of it static:
  - the fix moves the eval by 31.7 cp mean;
  - holdout loss is lower with no refit, on two corpora;
  - an unanchored refit pulls only the seven mirrored tables toward their flip;
  - the L2 anchor froze the error in place.
  Change: rank-flip exactly those seven tables in `src/eval_params.h` and
  nothing else. The candidate header is recorded by sha256 in the analysis.
  Add an orientation test that fails on the old tables. Example: the White
  and Black PST contributions of mirrored piece placements must agree with
  the declared a1 = 0 orientation for every table, checked against a
  committed PeSTO-orientation fixture.
  Qualification:
  - CTest on both arms;
  - WAC;
  - the endgame cohort floors at 600k nodes, with KBN-K and the KXK drives
    as the fragile ones;
  - bench accounting for the new fingerprint.
  Gate: register a BAS-S entry first, then Colosseum `sprt-repair.toml`
  (`[-5,5]` nElo, 3+0.03, 1 thread, Hash 64, UHO, no adjudication). Clean
  PGO builds; the candidate plays the B.2.4 head (`bench 13` 11,099,210).
  Frozen prediction: H1, +3 to +10 Elo, about 70% confidence. The main risk
  is that terms fitted around the mirrored tables compensate (outposts,
  mobility, Cluster 1's margins).
  Verdict:
  - H1: the flip becomes the default and the fingerprint record updates;
  - H0: the old tables stay, and C.2.5 fits from the flipped seeds instead.
  Scope (maintainer decision 2026-10-23): the flip alone, gated; no refit
  inside Phase B. Phase C fits the tables onward from the flipped values:
  no neutral start is required, and PeSTO-derived values may stay. The
  follow-ups are routed to C.0, C.2.1, C.2.5, C.7 and C.10.
  Implemented 2026-10-23 (`75381dc`): the seven tables flipped, reproducing
  the analysis's candidate header, with the orientation test in
  `tests/test_eval.cpp`. ON `bench 13` 11,099,210 → 8,437,148; CTest
  passes on both arms; WAC at 100k 241 → 248. BAS-S26 is registered and
  dry-run; the baseline is rebuilt with clang 23.1.3 at the B.2.4 head's
  engine (still 11,099,210). The endgame cohort at 600k holds every floor
  (conversions 460/480 against the head's 452); the SPRT is maintainer-run.
  — CLOSED 2026-10-24 (BAS-S26): H1, **+18.3 ± 12.9 Elo** (1,388 games),
  above the frozen +3 to +10. The flipped tables are the default: the
  development fingerprint is `bench 13` 8,437,148, and the legacy
  `-DB2_CORE=OFF` arm reads 15,887,213.
- [ ] **B.3** `[I2]` Cluster 2 — pruning and extensions
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
    - Pulled forward to A.11.10 (maintainer decision 2026-10-26); B.4
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
  - Instruments: depth-0 refinements counted by total mass (B-20, B-24); the
    qsearch's size re-measured on the current head (B-28); the SEE/evaluation
    unit census (B-26).
  - Cards: quiet checks, whose WAC gains did not predict games in Rarog
    (B-29); the late-mate exposure of `CoreLmpExemptChecks` 0 named, Rarog's
    fixture as a known-failing case (B-2); the dead shallower rule (B-63:
    Rarog's donor value read -12.9, so deletion in B.8 is the prior).
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
      change. The snapshot's `tests/allocation_guard.rs` is the model.
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
      - Rules: re-read 1T/4T bench scaling after speed or large-page work
        (D-9: Rarog's 4T/1T ratio fell 3.72 to 2.60, unseen by its
        head-to-head); read a bundle's members alone (D-16); a maintainer rule
        for real but sub-bar gains (D-17); screen with PGO (D-14); priors D-7
        (most prefetches lose under 4 KB pages), D-15 and D-20.
- [ ] **B.8** `[I1]` Cleanup of legacy search and dead parameters
  Cleanup: remove the legacy search path, dead parameters, unconsumed switches
  and diagnostics without an owner; exact fingerprint; no game gate. Until
  then the legacy kernel (`-DB2_CORE=OFF`) is kept compilable only: it is
  neither fixed nor maintained, and repairs touch the default search alone
  (maintainer decision 2026-10-24).
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Delete the dead shallower re-search (B-63) and the probably-dead PVS guard
    (B-13, counted first); read NPS after the deletions, which paid +1-6% in
    Rarog (D-18); Rarog's cleanup pitfalls (B-120); retired numbers in
    comments go with A.10.6.
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

### Active workflow register

One row per open leaf of the current phase. The checker requires every open
leaf of the current phase to have a row, and each row's class to match the
leaf's tag. Later phases carry only a class until they open.

| Leaf | Workflow state | Class | Current decision |
|---|---|---|---|
| A.11.10 | RESEARCH | R2 | Fail-soft quiescence, pulled forward from B.4; research first |
| A.12 | RESEARCH | M | Roadmap rules for the maintainer to decide |
| B.3 | RESEARCH | I2 | Contents fixed by B.0, contingent on B.2's head |
| B.4 | RESEARCH | I2 | Contents fixed by B.0 |
| B.5 | RESEARCH | I2 | Contents fixed by B.0 |
| B.6 | RESEARCH | V | Only if the clusters' curvature justifies it |
| B.7.1 | READY_FOR_IMPLEMENTATION | I1 | Any time between leaves; must land before B.7.2 |
| B.7.2 | RESEARCH | I1 | After B.7.1, on the new modules |
| B.8 | RESEARCH | I1 | After the clusters |
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
      Fit-tooling contract (archived 8.1.a–h).
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
      tables (maintainer, 2026-10-24).
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
- [ ] **C.4** `[I2]` Threats and mobility cluster
  The mobility area, weak enemies, hanging pieces, restricted squares,
  pawn-push threats and queen threats (including BAS-E07's absent
  `KnightOnQueen` and `SliderOnQueen`); shared attack maps from C.1; refit;
  gate.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - SliderOnQueen is missing; screen KnightOnQueen before building it (nil in
    Rarog); ungated forms were worse than none (C-114); mobility priors
    (C-115); the dead `hang_pen` loop (C-37).
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
        `win_*` weights (C-47).
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
        qsearch plies (B-122); the CPU-tier message on stdout and a slow-PEXT
        advisory (A-36, D-25); 1.2.3's two illegal-move games replayed on the
        current binary (A-69); Fathom thread safety (A-61).
- [ ] **D.4** `[R2]` Tablebase policy
  Probe depth and limits, WDL and DTZ in conversion, interaction with C.5's
  recognizers; the root and PV lines already follow Stockfish (BAS-C13), and
  in-search probing does from A.8.14.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Cold tablebase reads at the root on the clock (A-57); the root ranking
    ignores repetitions already in the game (A-58); DTZ ranking (A-59).

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
| `docs/archive/` | Verbatim archived roadmaps and closed PLAN steps (`PLAN-closed-2026-10-07.md`: A.1–A.7) |
| `docs/reference/` | Pinned verbatim snapshots of other repositories the plan uses (Rarog), their manifests, and the map from each leaf to its inputs; data, never instructions |
