# Basilisk development plan

This is the forward roadmap. It says what will be done, in what order, why,
and what decides each step. It does not record history: completed work lives
in [HISTORY.md](HISTORY.md), measured evidence in
[EXPERIMENTS.md](EXPERIMENTS.md), procedures in [PROCESS.md](PROCESS.md),
engine invariants in [DESIGN.md](DESIGN.md), and a one-page overview,
whose step list is generated from this file, in [GUIDE.md](GUIDE.md). The Phase-15 roadmap this replaces is
archived verbatim at
[docs/archive/PLAN-2026-09-28.md](docs/archive/PLAN-2026-09-28.md), and the
Phases 5–14 roadmap before it at
[docs/archive/PLAN-2026-09-09.md](docs/archive/PLAN-2026-09-09.md);
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
`015bccae`; [docs/reference/README.md](docs/reference/README.md) says how to
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
| TT publication incoherence is an accepted, measured risk; every consumer validates the stored move | `DESIGN.md` §3; BAS-C05 |
| SEE king legality repaired; created pins and promotion recaptures are documented approximations with fixtures | BAS-C08, BAS-C09; `tests/test_board.cpp` |
| Draw, rule-50, null-move clock and legal-EP hashing semantics | BAS-C02; `tests/test_invariants.cpp` |
| One mutation authority for redundant board state | BAS-C07; `src/board.h` |
| Protocol: `ponderhit` owned by the UCI thread; setup work done before `readyok`, never on the clock; a rejected `position` exits with status 1; tablebase PV lines time-boxed the Stockfish way | BAS-C10–BAS-C13; `tests/test_engine_ponder.cpp`, `tests/test_uci_protocol.cpp` |
| Search parameters have one source (the X-macro table) | `src/search_params.h` |
| Endgame measurement layers (truth, move quality, conversion, strength) | `analysis/endgame_measurement_layers_v1.md`; `DESIGN.md` §4 |
| Behaviour-neutral change = exact fingerprint plus targeted checks plus pooled NPS | `AGENTS.md` |

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

## Phase A — Reset: documents, harness, instruments, baselines

**Goal:** put the documents, harness and instruments of the method in place
on the released 1.10.1 head, and measure the starting point with them. No
engine behaviour changes in Phase A. Rarog's Phase A also cut a release; 1.10.1
was cut the day before this plan, so Phase A has no release step.

- [x] **A.1** Document reset
  CLOSED 2026-09-28.
  New PLAN, GUIDE and PROCESS; AGENTS merged with Rarog's rules; DESIGN,
  HISTORY and the ledger's live sections brought up to date; the Phase-15
  roadmap archived verbatim (`6543ccf`); `check_roadmap.py` adapted to
  lettered IDs, the `(ANY TIME)` exemption, the register and the fingerprint
  check (`01cc84a`); `dev` recreated from `master`. Amended the same day:
  everything the plan takes from Rarog pinned as a verbatim snapshot in
  `docs/reference/rarog/` (`e5ad70b`), with the checker verifying its
  manifest (`e948979`), so the plan needs no live Rarog repository.
- [x] **A.2** Repository and inventory
  CLOSED 2026-09-28.
    - [x] **A.2.1** `[M]` Tracked-file cleanup
      Every one-off or superseded tracked file named with its last commit and
      removed when nothing consumes it. — CLOSED 2026-09-28.
      A consumer is a build, CI, test or tool that reads the file, or a live
      document or open leaf that directs its use; a closed record's citation
      is provenance and resolves through the commit named here. Removed, each
      recoverable as `git show 23f5557:<path>`:
      `tools/run_5911_experiment.ps1` (BAS-E17 launcher);
      `tools/diag/kbnk_{sweep,upper,refinement,holdout}_summary.py`
      (archived 6.1.c/6.1.e KBNK screens);
      `tools/diag/damping_resolution_summary.py` (archived 6.4.a probe);
      `tools/diag/freeze_group_a_head.py` and `group_a_head_v1.json`
      (the retired Group A head; GUIDE's checkpoint now defines the head);
      `tools/diag/eval_term_firing.cpp` (archived 5.9.1 firing check; C.1's
      `EvalTrace` coverage owns activation). Kept because an open leaf owns
      them: the endgame instruments, cohorts and label tools (C.2.4, C.5),
      `passer_king_geometry.py` (C.6), `rook_ending_failure_profile.py` and
      `narrow_node_probe.py` (C.5.4), `tools/texel/phase911.ps1` (C.2.1),
      `tools/spsa_configs/config_*.json` (retired by A.3.2's generated
      surface), `docs/release_tiers.md` (A.4.2, E.3); `analysis/` records,
      logos and the UCI specification are not one-off files. The remnants of
      the withdrawn August Colosseum adoption (`3cbf90b`) were removed so A.3
      starts clean: the Texel README's datagen recipe and
      `phase911.ps1`'s header point to `datagen.ps1` again (the script
      always required its manifest), and `build_test.ps1`'s TUNE comment
      names weather-factory.
    - [x] **A.2.2** `[M]` Branch and tag disposition
      Each `archive/*` and `oracle/*` tag gets its citing document, reason and
      retirement condition. — CLOSED 2026-09-28.
      Tags whose condition has fired are proposed for removal; deleting a
      remote tag stays the maintainer's command. The register is HISTORY's
      *Preserved commits*: `oracle/hybrid`, `oracle/hybrid-diag` and
      `archive/nnue-local` are kept with their retirement conditions (F.4
      now cites the last); `archive/backup` and `archive/nnue-origin` are
      ancestors of kept tags and `archive/arm_fix` is BAS-P07's rejected
      diff, so all three are proposed for removal. Only `master` and `dev`
      exist as branches. The ledger's stale branch pointers (BAS-D03,
      BAS-P07) now name the tags.
    - [x] **A.2.3** `[R2]` Feature, option and parameter inventory
      Every `search_params.h` coordinate, evaluation parameter, TUNE-only
      option and `Diag` counter classified live, inert-with-owner or dead.
      The "exposed but inert" knobs in `search_params.h` are listed with
      their owners: capture futility, SEE-quiet pruning, qsearch quiet
      checks, the double-extension cap and `PostLmrHistScale`. The inventory
      is the input to B.0's survivor list and B.1's removals. — CLOSED
      2026-09-28: `analysis/parameter_inventory_v1.md`, BAS-D19. Four of the
      five were inert; the double-extension cap is a live bound (16). Capture
      futility is dead at the default LMR table but wakes at `LmrBase` >= 100.
      B.1 removes 7 coordinates and the drifted `KBNK Drive` option.
- [x] **A.3** Colosseum CLI as the main harness
  Harness: Colosseum CLI as the main path (maintainer decision 2026-09-28). —
  CLOSED 2026-09-29.
    - [x] **A.3.1** `[I1]` Run files, `colosseum.ps1` and the shared guards
      Run files and wrapper: port the snapshot's `tools/colosseum.ps1`, run
      files and guard suite, adapted to Basilisk's CMake manifests; the
      snapshot's `tools/colosseum/colosseum.pin.json` (`cli-v0.2.0`) is the
      starting pin.
      Every guard is shared with `sprt.ps1` and `spsa.ps1` through
      `harness_common.ps1`, so the two paths cannot drift. Guards: refuse a
      dirty or wrong-revision candidate; require the binary, hash, compiler,
      flavor and bench manifest (`-ExpectBench`); refuse an option the engine
      does not advertise; refuse a busy host or an unpinned runner (SHA-256
      pin); require natural termination. These absorb the archived 6.6.f.
      Checks: the resolved configuration matches a recorded `sprt.ps1`
      manifest field by field; a guard suite breaks one input per case and
      gets the refusal that names it; a short live match completes with zero
      faults and an independent recount from the PGN. — CLOSED 2026-09-29:
      the pinned `cli-v0.2.0` wrapper and committed run files share strict
      CMake-sidecar, option, host-idle and fault guards with the backup paths;
      15/15 negative/control guard cases behaved, fastchess and Colosseum
      agreed on 26/26 comparable resolved fields, and the two-game live smoke
      had zero faults with its `[0,0,1,0,0]` pentanomial reproduced from PGN.
      Ignored evidence: `tools/results/sprt_A31A_vs_A31B_20260929_095323.*`,
      `tools/results/colosseum_match_a31-parity_20260929_095349.*`, and
      `tools/results/a31-live-smoke-process/` with
      `tools/results/colosseum_match_a31-live-smoke-process_20260929_095951.*`;
      their manifests carry the runner, engine, book, configuration and
      artifact hashes.
    - [x] **A.3.2** `[I1]` SPSA tune path from the X-macro
      Tune path: the X-macro table in `src/search_params.h` generates the SPSA
      surface and the Colosseum tune file. — CLOSED 2026-09-29.
      The tune shape is 15 slots × 30 games per iteration (RAR-M62, 1.65×
      the old throughput). `-SeedFrom` chains the blocks of PLAN rule 7c; the snapshot's
      `tools/spsa_config_to_colosseum.py` and `tools/spsa_block_rule.py` are
      the models.
      `spsa.ps1`'s fixed config-group list gains the generated path, and its
      5,000-iteration floor, which cites the archived "PLAN gate 11", is
      reconciled with rule 7c's 2,000-iteration blocks. A one-iteration tune
      on a temporary surface is the smoke. The hand-written
      `tools/spsa_configs/config_*.json` and the README's warnings about them
      (which name `.STALE` files no longer in the tree) retire in the same
      change.
      `generate_spsa_surface.py` now emits the ordered 48-coordinate
      weather-factory JSON and the 2,000-iteration Colosseum tune/run files;
      perturbations are `max(2, round(range / 16))`, and `--check` rejects
      drift. `spsa_colosseum.ps1` fixes production blocks at 15 × 30 and
      accepts `-SeedFrom`; `spsa_block_rule.py` applies the registered
      three-mover rule against each block's own seeds. The backup
      `spsa.ps1` consumes the generated surface at 30 games/iteration and
      uses the rule-7c 2,000-iteration floor. Five hand-written historical
      vectors retired. A fresh TUNE build reproduced **14,978,465**; the
      one-iteration/30-game temporary-surface smoke completed with 30 normal
      terminations and zero engine, time or infrastructure faults, and a
      second dry run proved `-SeedFrom` bound the completed result. The
      registered 15-slot shape targets the maintainer's Ryzen 9 5950X; this
      hybrid host exposes five eligible P-cores, so its smoke deliberately
      used one slot. Ignored evidence:
      `tools/results/a32-smoke/` and
      `tools/results/colosseum_spsa_a32-smoke_20260929_100945.*`.
    - [x] **A.3.3** `[M]` PROCESS *Harness* section
      PROCESS *Harness* section finalised: Colosseum main, fastchess and
      weather-factory maintained as backup and second opinion until at least
      the classical release, with the cross-check triggers. — CLOSED
      2026-09-29. The procedure names both guarded Colosseum entry points,
      ownership of run conditions, pre/post-run proof, resume and tune-chain
      semantics, the three backup triggers, topology rules and main/backup
      commands.
- [x] **A.4** Build and toolchain
  CLOSED 2026-09-29
    - [x] **A.4.1** `[I1]` Toolchain refresh and freeze
      Toolchain refresh and freeze (archived 7.0): inventory the compiler, C++
      library, CMake, Ninja and profile tools on Windows, Linux CI and macOS.
      Compare the current and newest stable versions one axis at a time.
      Require CTest, sanitizers, exact search agreement, ISA checks and
      pooled release/PGO throughput before selecting the faster
      non-regressing line. — CLOSED 2026-09-29. Retained the deployed
      compiler/library lines because forward Linux variants had no native
      pooled result and Windows already used current LLVM 22. The audit found
      and repaired a false PEXT-tier contract, added an ISA verifier with a
      known-bad control, kept the startup launcher at baseline ISA, and pinned
      CI runner generations. Release and sanitizer CTest passed 13/13 with
      the exact **14,978,465** fingerprint; repaired final-PGO PEXT measured
      **+7.45%** [**+7.22%, +7.75%**] over the pre-repair tier. Evidence:
      `analysis/toolchain_refresh_v1.md`.
    - [x] **A.4.2** `[I1]` Build flavors and manifests for arms
      Build flavors for arms: `build_test.ps1` builds and manifests release,
      tune, diag and umbrella-switch arms.
      Each manifest carries the hash, source revision, compiler, flavor and
      bench. CI covers the CMake option and ISA combinations rather than
      single options, and distinguishes production from diagnostic builds
      (archived 6.6.g). The matrix includes the `TEXEL` target, repaired first:
      `tools/texel/tuner.cpp` does not compile at `aecbd93` (A.2.3). — CLOSED
      2026-09-29. TEXEL now uses public board reads plus an opaque coherent
      position snapshot and reconstructs the seven-position fixture exactly.
      Final-PGO release, tune, diagnostic and umbrella-probe builds each
      reproduced **14,978,465** with clean schema-2 manifests; their UCI
      surfaces carried 9, 60 and 11 options respectively, and both PGO phases
      retained the umbrella value. CI exercises TUNE across every existing
      ISA/platform row, diagnostic and TEXEL PEXT combinations, and known-bad
      overlapping flavor/ISA controls. Release CTest passed 13/13 and the
      final PGO PEXT ISA contract passed.
- [x] **A.5** Instruments for the search programme
  CLOSED 2026-09-30.
    - [x] **A.5.1** `[I1]` Fixed-budget probe
      WAC solved at 100k and 400k nodes and at a fixed PV depth, plus oracle
      best-move agreement on `suite_v1.epd` at 300k nodes, with per-position
      records. — CLOSED 2026-09-29.
      Ported from the snapshot's `tools/diag/fixed_budget_probe.py` onto
      Basilisk's `wac` command and `tools/diag/run_suite.py`. `wac nodes N`
      records completed and total work separately; `wac depthpv N` records
      every completed PV head plus first/stable solution depths. The oracle
      path rejects missing options, ignores incomplete aspiration bounds and
      records comparable best-move agreement per position. Parser falsifiers,
      three live WAC command forms, a live 300k same-binary agreement control,
      release CTest 13/13 and bench **14,978,465** passed.
    - [x] **A.5.2** `[I1]` Reference-anchored branching profile
      Branching profile: the reference-anchored geometric branching factor
      over depths 4–14, one fresh process per depth. — CLOSED 2026-09-29.
      Basilisk against the classical oracle and modern Stockfish, with
      per-position ratios and the median beside the aggregate: one position
      of forty once decided an endpoint measure (BAS-X13).
      `tools/diag/branching.py` is the starting point, and the snapshot's
      `tools/branching_profile.ps1` the model. The v2 JSON binds both reference
      binaries and the corpus by SHA-256, fixes Hash 64 and Threads 1, records
      cumulative and iteration growth plus every position, and compares all
      arms on one common completed-position set with explicit exclusions. Six
      parser, outlier, common-denominator and lifecycle tests plus a real
      Basilisk UCI option handshake passed. No profile was measured on the
      busy host; B.0 performs the first three-arm run on an idle host.
    - [x] **A.5.3** `[I1]` Counter summation and decision trace
      Counters and decision trace: `Diag` counters summed per position at
      sampling stride 1 with their units named, and a diag-only decision
      trace. — CLOSED 2026-09-30.
      The trace is bounded to plies 1–2 under `searchmoves` and prints every
      prune, reduction and extension with its inputs. Rarog's trace found
      two seed defects that no counter could see: a static margin overriding
      a mate in one, and a count-based skip dropping a mating quiet move.
      The snapshot holds the counter tools (`tools/diag/bench_counters.py`,
      `tools/diag/phase4_differential.py`, `analysis/phase4_counter_spec.md`)
      and the oracle's counters (`refs/oracle-hybrid-diag.patch`).
      Every counter becomes machine-readable: 16 of the 57 are printed only
      as prose today (A.2.3). The leaf also decides the 73 `diag_`
      increments that run in release builds with `Diag` off, priced by
      PROCESS's NPS method.
      All 57 core counters now have named units, strict per-position records
      and checked sum identities at stride 1. `DecisionTrace` is compiled only
      into diagnostic/tune builds, requires `Diag=true`, one thread and one
      `searchmoves` root, records plies 1–2 into fixed-capacity storage, and
      rejects overflow. The release-counter arm was bench-identical at
      **14,978,465**. Its pooled two-build PGO comparison found compiling the
      increments out worth **+0.90% median NPS**, 95% CI **[+0.54%, +1.02%]**,
      best-of **+0.75%**, faster in **16/16** alternating rounds; the preceding
      two-build self-pair passed at **−0.17%**, 95% CI **[−0.36%, +0.20%]**.
      Production therefore compiles the increments out; diagnostic and tune
      builds retain them, and `RELEASE_DIAG_COUNTERS` remains only as the
      reproducibility arm.
    - [x] **A.5.4** `[I1]` Matched ablation mask
      One bit order on Basilisk and on the `oracle/hybrid` build, compiled
      away in production, every bit proven live by a moved node count; the
      oracle side exists as the snapshot's `refs/oracle-hybrid-ablate.patch`,
      written for the same classical-Stockfish search. — CLOSED 2026-09-30
      `AblationMask` uses bits 0–7 for razoring, reverse futility, null move,
      ProbCut, IIR, shallow move pruning, extensions and LMR. It exists only
      in the `Ablate` flavor; the final-PGO builder propagates that flavor
      through both child builds, while production advertises no option.
      Mask 0 and production both reproduce **14,978,465** at `bench 13`.
      `tools/diag/ablation_liveness.py` ran eight suite-v1 positions at depth
      9 and proved every single bit live in both engines. Against mask 0,
      Basilisk's aggregate node deltas were **+78,530, +123,314, +5,496,
      −19,430, +40,431, +363,230, −112,186, +771,104**; the patched pinned
      oracle's were **+24,158, +26,339, −10,116, −24,741, +2,875, +557,562,
      −44,400, +115,520**. The hash-bound record is
      `tools/results/a54-ablation-liveness.json`.
    - [x] **A.5.5** `[I1]` PGN conversion instrument
      Conversion instrument over PGN: port the snapshot's
      `tools/diag/conversion_audit.py` and
      `tools/diag/export_tournament_pgn.py`. — CLOSED 2026-09-30
      The hash-bound `5e539523` export contains 39,600 games, skips none and
      reproduces the snapshot's `conversion_release_basilisk_1.9.3_v1.json`
      exactly apart from path and generation time: against the six anchors,
      **94 draws and 12 losses after a persistent piece-up in 3,600 games**
      (**26.1 / 3.3 per 1,000**), with all counts, parameters and termination
      classes identical. PGN SHA-256 is
      `fe0cf072acaeba95427a9fa549ca50a9662350380c664f9071dfa0ffe0b863d5`.
      Seven synthetic tests cover perspective, persistence, material
      signatures, aggregation, filtering, PGN round-trip and read-only SQLite
      export/header rewriting. The earlier **17.5 / 5.0** text belonged to the
      separate Super Rating Tournament rather than the named frozen JSON; it
      remains an A.7.1 result to reproduce from that tournament's PGN.
    - [x] **A.5.6** `[R2]` Reference-anchored canaries
      WAC positions, including quiet key moves and quiet mate threats,
      anchored at the depth classical Stockfish solves them. — CLOSED
      2026-09-30.
      A changed canary is recorded with its cause and never re-blessed.
      The hash-bound `canary_v1.json` freezes **126** local classical-oracle
      anchors: oracle stable depth at most 6 plus WAC.001, with an independent
      oracle solve at 100k nodes. Of these, **77** are required because the
      1.10.1 baseline also stays correct by oracle depth + 2 and solves at
      100k; **24** have quiet key moves. The other **49** remain named gaps:
      a later pass is reported but never silently added to the gate. WAC.001
      is the quiet mate-threat gap, anchored locally at oracle depth 10 with a
      depth-12 allowance; the baseline fails both depth and node conditions.
      The imported Rarog counts (116/242 and WAC.001 at depth 9) were not
      reused because they were produced under Rarog's evaluation. Two complete
      runs matched in every non-time field, the baseline check passed 77/77,
      eight unit tests passed, a missing UCI option was rejected live, and a
      known regression fails while a new pass is diagnostic only. A frozen
      manifest refuses overwrite; changing the cohort requires a new version.
- [x] **A.6** `[R2]` Codebase consolidation analysis
  `src/search.cpp` (3,125 lines) and `src/eval.cpp` (2,148 lines) mapped into
  target modules; the B.1 and C.1 move tables; dead code; the seams a cluster
  needs. Refactors nothing. — CLOSED 2026-10-01.
  `analysis/codebase_consolidation_v1.md` separates engine-shared,
  per-thread, per-search configuration and mutable search state; fixes the
  target modules and ordered move tables; and preserves the hot recursive
  worker and accumulator boundaries behind pooled-PGO floors. The earlier
  2,848-line search count predated A.5. `time_limit_` and the unconsumed
  `RootMoveStat` collection are dead; the latter also allocates from the root
  recursive path. B.1 removes those, A.2.3's seven inert coordinates and the
  drifted `KBNK Drive` option. C.1 retains the 23 zero evaluation groups for
  C.0 rather than mistaking traced, consumed zero weights for dead code.
- [x] **A.7** Baselines on the 1.10.1 binary
  CLOSED 2026-10-06; Phase A is complete.
    - [x] **A.7.1** `[V]` 1T pool baseline from the Super Rating Tournament
      1T pool baseline, zero games: census of Basilisk 1.10.0's head-to-heads
      in the Super Rating Tournament PGN
      (`D:/chess/results/super_rating_tournament.pgn`, SHA-256 in BAS-X34),
      and its conversion rate with A.5.5 on the same file. — CLOSED
      2026-10-01.
      The exact 172,200-game PGN reproduced BAS-X34's hash. Direct records
      were Houdini 3 15-26-159, Critter 1.6a 25-35-140, Fritz 16 25-45-130
      and Rybka 4.1 44-38-118, 200 games per pair, balanced colours and zero
      unfinished. Against the six conversion anchors, 1.10.0 recorded 20
      draws and 8 losses after a persistent piece-up advantage in 1,200 games:
      **16.67 / 6.67 per 1,000**. BAS-M08 replaces HISTORY's unsourced
      2026-09-04 figures; raw PGN and JSON outputs remain ignored.
    - [x] **A.7.2** `[V]` 4T gauntlet against the targets and Rarog 2.4.0
      4T gauntlet against the four targets and Rarog 2.4.0, 400 games per
      pair, no adjudication, maintainer-run; a null pair first if the 4T setup
      changed since BAS-M02.
      BAS-M09's duplicate null was withdrawn before exposure by maintainer
      decision 2026-10-01: Basilisk uses the same pinned Colosseum 0.2.0 binary
      qualified in the Rarog snapshot, and no Rarog engine verdict transfers.
      BAS-M10 freezes the remaining 2,000-game gauntlet as 200 cycles of two
      colour-reversed games against each opponent. Its clean dry run resolves
      1,000 distinct openings, all five fixed ratings, heterogeneous 4T option
      names, zero permitted faults and symmetric four-core placement.
      Amendment 1 (2026-10-05, before any game) moves the run to the
      production 5950X: concurrency 3, the Basilisk and Rarog pins rebuilt
      for that host, everything else unchanged; its clean dry run is at
      `943f3f8`.
      BAS-M10 was voided at 659 games by two Houdini 3 crashes against its
      zero-fault rule. BAS-M11 re-runs it in the Colosseum desktop app with
      Hash 512 MB per engine and crashes left to maintainer judgement, read from
      Basilisk's performance against the five fixed ratings. Rarog 2.5.0 joins
      as an unrated sixth opponent for comparison (2,400 games), outside the
      verdict. — CLOSED 2026-10-05: 4T performance **3040** [3026, 3054]
      against the five fixed 1T ratings (1T: 2994); Houdini −203, Critter
      −129, Fritz −177, Rybka −56, Rarog 2.4.0 +14; Rarog 2.5.0 −312. Five
      Houdini setup crashes excluded (BAS-M12); deviations in BAS-M11.
    - [x] **A.7.3** `[V]` Oracle deficit meter G(0)
      Rebuild `oracle/hybrid` with the 1.10.1 evaluation, then 3,000 paired
      games at equal time, no adjudication, maintainer-run.
      A prediction is frozen first; BAS-O01's +322.7 is the prior, on a
      coarser estimator.
      Prepared 2026-10-05 as BAS-O05 while A.7.2's re-run was paused by the
      maintainer: the oracle rebuilt from tag `oracle/hybrid` with `v1.10.1`'s
      `src/` by `tools/oracle/build_oracle.ps1` (conformance 0 mismatches,
      known-bad control fails), 3,000 paired games by
      `tools/run_a73_oracle_g0.ps1`; clean dry run at `a7bc05b`. — CLOSED
      2026-10-05: **G(0) = +312.6 ± 17.8** over 3,000 paired games, 0 faults,
      inside the frozen +250 to +350; the deficit is unchanged from 1.9.3.
    - [x] **A.7.4** `[V]` Pooled-PGO NPS baseline
      Pooled-PGO NPS baseline with `nps_ab.ps1`: a self-pair validated first,
      at least two PGO builds per arm, interleaved, idle host.
      Prepared 2026-10-05 as BAS-P12: `tools/run_a74_nps_baseline.ps1`
      builds four final-PGO 1.10.1 binaries at `3e5294be`, runs a self pair
      over all four and builds 1–2 against 3–4; wiring smoke-tested. — CLOSED
      2026-10-06: **4.127M NPS** pooled; self pair +0.11%, build pools +0.04%;
      all three BAS-P12 predictions held.

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

**What "Stockfish-shaped" means, concretely.** The source of record is pinned
by B.0; the default is official master `0a215d6c` (2026-09-22), the revision
BAS-C13 already follows. The files are `src/search.cpp`, `movepick.cpp`,
`history.h`, `tt.cpp`, `timeman.cpp` and `thread.cpp`. Read them; do not copy
them.

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

- [ ] **B.0** `[R3]` Search programme investigation
  Investigation: the Basilisk-versus-Stockfish mechanism map, cluster
  contents, scale ratio and seed rule, survivors, SPSA surfaces, B.2.2's
  registered screen numbers, frozen B.2 predictions and handoffs for B.1–B.3.
  No engine source changes.
  The deliverable is `analysis/search_programme_<date>.md`. It covers:
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
- [ ] **B.1** `[I1]` Search restructure, behaviour-neutral
  `src/search.cpp` split into modules, node types as template parameters, a
  per-thread worker and stack separated from per-search configuration and
  engine-owned shared resources, inert parameters removed per A.2.3; exact
  fingerprint and pooled-PGO NPS within noise.
  The move table comes from A.6 and B.0. Release and sanitizer CTest, and
  exact `bench 13` at every commit.
- [ ] **B.2** Cluster 1 — the selectivity core
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
    - [ ] **B.2.0** `[R2]` Architecture review and neutral upgrades
      Architecture review of the B.1 head: the seams the cluster needs, and
      behaviour-neutral upgrades landed at the exact fingerprint before the
      first mechanism ticket.
      Rarog's review landed twelve such upgrades.
    - [ ] **B.2.1** `[I2]` Implement behind the umbrella switch
      Implement to the B.0 handoff behind one umbrella CMake option, OFF by
      default, as ordered tickets; ticket 0 is the decision trace hook.
      Each ticket keeps the umbrella-off arm at the exact fingerprint. Unit
      tests cover every table's bounds and gravity, picker exhaustiveness, TT
      store and probe, and stack unwind. CTest passes on both arms: the
      mate-drive floors bind the on arm too, and a failure there returns the
      premise to research. Reviewer acceptance (a separate `R2` session) is
      recorded before B.2.2.
    - [ ] **B.2.2** `[V]` Diagnostics and the unfitted paired run
      Screens at B.0's registered numbers; the ablation sweep if a floor
      fails; the unfitted 2,000-game paired run against the accepted head,
      maintainer-run, which governs. Categorical switches the review asks for
      are settled by 2,000-game paired runs on one tune build and never
      become SPSA coordinates.
    - [ ] **B.2.3** `[V]` Curvature sweep, then SPSA in blocks
      Curvature sweep, then SPSA in blocks (rule 7c) over the cluster's live
      coordinates, maintainer-run; theta baked in one engine commit.
    - [ ] **B.2.4** `[V]` Cluster 1 gates
      Gate: (a) the unfitted arm against the umbrella-off arm at the same
      revision, `[0,10]`, once its PGO binaries exist; (b) the fitted arm
      against the unfitted one, `[0,10]`, after the bake.
      Each gate accepts one thing on its own evidence, so a weak tune cannot
      ride on the cluster's margin. The default flips to the accepted arm;
      the legacy search stays compilable until B.8.
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
     existing switch.
- [ ] **B.5** `[I2]` Cluster 4 — root and aspiration
  Cluster 4 — root, aspiration and iterative deepening: aspiration centred on
  the root move's average score with width from its mean squared score,
  optimism, root-move effort, the `pvIdx` root structure with a `MultiPV`
  option, and deep-iteration line growth; SPRT `[0,3]`.
  Basilisk has no MultiPV today; the option is identity-gated at
  `MultiPV 1`, and BAS-C13's tablebase PV contract is kept. Time management
  proper is D.1's.
- [ ] **B.6** `[V]` Joint search SPSA, if justified
  Joint search SPSA over the coordinates the four clusters left live, only if
  their curvature evidence and results justify it; registered surface; PGO
  bake; SPRT `[0,3]`.
- [ ] **B.7** Search speed pass
    - [ ] **B.7.1** `[I1]` Allocation guard (ANY TIME) — before B.7.2
      Allocation guard (ANY TIME) — must land before B.7.2: a test with a
      counting global `operator new` searches fixed positions at two depths at
      Threads 1 and 4.
      It asserts that allocations grow per iteration, never per node, while
      the node count grows by at least an order of magnitude. A planted
      per-node allocation must fail it, checked once and reverted. No engine
      change. The snapshot's `tests/allocation_guard.rs` is the model.
    - [ ] **B.7.2** `[I1]` Speed pass
      Behaviour-neutral throughput work on the new modules, with a pooled-PGO
      floor of +0.5% per change and B.7.1's guard as the other floor.
      The archived board-speed leaves (7.2–7.8: generation and list
      delivery, fused relocation, pin/check sharing, the SEE kernel,
      representation) open only if a profile of board work inside HCE
      search makes them hot, and default to no-change.
- [ ] **B.8** `[I1]` Cleanup of legacy search and dead parameters
  Cleanup: remove the legacy search path, dead parameters, unconsumed switches
  and diagnostics without an owner; exact fingerprint; no game gate.
- [ ] **B.9** `[V]` Search programme checkpoint
  Checkpoint: re-measure every deficit meter.
  The meters are G(0) against the oracle, fixed-node depth and branching,
  pooled NPS, the conversion instrument and the endgame truth cohort, plus a
  1T pool gauntlet against the four targets and Rarog (400 games each).
  Record the attributed Elo per accepted cluster and the checkpoint against
  the budget table, remove the ablation build, then freeze the search head
  for C.

### Active workflow register

One row per open leaf of the current phase. The checker requires every open
leaf of the current phase to have a row, and each row's class to match the
leaf's tag. Later phases carry only a class until they open.

| Leaf | Workflow state | Class | Current decision |
|---|---|---|---|
| B.0 | RESEARCH | R3 | Next leaf: the search programme investigation and its handoffs |
| B.1 | RESEARCH | I1 | Waits on B.0's move table and A.2.3's removal list |
| B.2.0 | RESEARCH | R2 | Waits on B.1's head |
| B.2.1 | RESEARCH | I2 | Waits on B.0's cluster 1 handoff |
| B.2.2 | RESEARCH | V | Screen numbers come from B.0 |
| B.2.3 | RESEARCH | V | Curvature sweep after B.2.2 |
| B.2.4 | RESEARCH | V | Gates after B.2.3 |
| B.3 | RESEARCH | I2 | Contents fixed by B.0, contingent on B.2's head |
| B.4 | RESEARCH | I2 | Contents fixed by B.0 |
| B.5 | RESEARCH | I2 | Contents fixed by B.0 |
| B.6 | RESEARCH | V | Only if the clusters' curvature justifies it |
| B.7.1 | READY_FOR_IMPLEMENTATION | I1 | Any time between leaves; must land before B.7.2 |
| B.7.2 | RESEARCH | I1 | After B.7.1, on the new modules |
| B.8 | RESEARCH | I1 | After the clusters |
| B.9 | RESEARCH | V | Programme checkpoint |

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
- [ ] **C.1** `[I1]` Evaluation restructure, behaviour-neutral
  `src/eval.cpp` split into family modules, one attack-map and mobility-area
  producer, `EvalTrace` unchanged in meaning, lazy-path semantics preserved;
  exact fingerprint; pooled NPS within noise.
- [ ] **C.2** Fit pipeline, corpus and label contract
  Fit pipeline, corpus and label contract (archived 8.1–8.8).
    - [ ] **C.2.1** `[I1]` Fit-tooling contract and Texel handbook
      Fit-tooling contract (archived 8.1.a–h).
      - K is fitted once and frozen across stages and arms.
      - Initial vectors are complete and explicit.
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
    - [ ] **C.2.3** `[V]` Generate and publish corpus A
      Generate and publish corpus A with the B.9 head at a fixed node budget,
      natural termination, maintainer-run; audit the returned composition and
      publish it immutably (archived 8.3).
    - [ ] **C.2.4** `[R2]` Matched label arms
      Matched label arms (archived 8.4–8.5): corpus B row-identical to A
      except eligible tablebase rows relabelled by Syzygy WDL, with cursed
      results as draws and missing tables as unknown.
      Whole-game tablebase adjudication is analysed separately as its own
      registered arm or a no-arm verdict. Tablebase adjudication never enters
      a strength gate. BAS-E46 sizes the opportunity: at 8,000 nodes, 19.77%
      of clean tablebase wins were not won.
    - [ ] **C.2.5** `[R2]` Initialization control
      Initialization control (archived 8.6): fit from the accepted and from
      neutral starts under identical K, data and schedule; register the
      production rule before the test split opens.
    - [ ] **C.2.6** `[V]` Matched fits and the label-contract gate
      Matched whole-surface fits of A and B on the current surface, then the
      registered label-contract gate (archived 8.7–8.8); fit loss explains,
      the SPRT decides.
- [ ] **C.3** `[I2]` King safety cluster
  King safety cluster in the donor's shape: attacker units and weights, safe
  and unsafe checks by piece, weak ring squares, flank attack and defence,
  shelter and storm with the castling alternative, the queenless reduction and
  the nonlinear danger map; refit; gate.
  Opens as sub-steps (research, implement, refit, gate) when C.0's handoff
  reaches it; the same holds for C.4, C.6 and C.7.
- [ ] **C.4** `[I2]` Threats and mobility cluster
  The mobility area, weak enemies, hanging pieces, restricted squares,
  pawn-push threats and queen threats (including BAS-E07's absent
  `KnightOnQueen` and `SliderOnQueen`); shared attack maps from C.1; refit;
  gate.
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
    - [ ] **C.5.2** `[R3]` Occurrence, classification and ranking
      Occurrence, classification and ranking (archived 6.7.a–d).
      - Board and tree occurrence with per-root concentration.
      - Each family classified as exact recognizer, scale, move-quality or
        conversion work, with its deciding instrument.
      - Every dispatcher's promotion and material-shed closure.
      - A re-rank by local defect × occurrence × deployment budget.
    - [ ] **C.5.3** `[I2]` Generic winnability and scaling
      Generic winnability and scaling in the donor's form: pawn-count scaling,
      opposite-bishop scaling, rule-50 inside the scale and a complexity term;
      floored so a misfire discounts rather than throws a win.
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
    - [ ] **C.5.6** `[R2]` Lower-yield remainder
      Lower-yield remainder (archived 6.10.a–d): KPs-K and KP-KP first; KQ-KP
      and KQ-KR reconciled across budgets; KR-KN, KNN-KP and KNN-K theory and
      rule 50; implement only what has a measured defect, occurrence and a
      valid closure.
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
- [ ] **C.6** `[I2]` Pawns and passers cluster
  Passer king proximity, blocker ownership, path safety, unstoppable and
  rook-behind conditions; structure conditionality (doubled, isolated,
  backward, connected, levers); refit; gate. The dispositions of the archived
  6.3 passer study are its prior.
- [ ] **C.7** `[I2]` Material, imbalance, phase and pieces cluster
  Imbalance in the donor's quadratic form, the phase interpolation, bishop
  pair and bishop-pawn colour, outposts (including BAS-E07's absent
  `BadOutpost`), long diagonal, rook files, trapped rook, queen weakness and
  king-protector distance; refit; gate.
- [ ] **C.8** `[V]` Refit cycles
  Refit cycles (archived 8.9): regenerate data with the accepted head, refit
  the whole surface with a new untouched test split, and gate each cycle;
  repeat while a cycle accepts; stop at the first that does not.
- [ ] **C.9** `[V]` Nonlinear HCE SPSA or a written skip
  Nonlinear HCE SPSA (archived 8.10): only live, frequent terms Texel cannot
  price, such as the king-danger map, from a zero-game activation and
  curvature screen; a written skip if the surface is flat.
- [ ] **C.10** `[V]` Search re-fit after the new evaluation
  One joint SPSA over the registered cp-valued search coordinates, including
  B.4's value surface if it exists, then PGO and SPRT `[0,3]`.
- [ ] **C.11** `[V]` Freeze the classical evaluation
  Checkpoint and freeze (archived 8.11).
  - Re-measure the same-search deficit against a fresh classical-Stockfish
    hybrid at the C head, plus conversion, the truth cohort, NPS and a 1T
    pool gauntlet.
  - Revalidate the score scale, mate bounds and every endgame floor.
  - Ablate low-information mechanisms.
  - Freeze the classical evaluation.
- [ ] **C.12** `[I2]` Evaluation throughput, bit-exact
  Evaluation throughput, bit-exact (archived 8.12).
  - Profile terms, phases and cache paths first.
  - Optimize only measured hot paths, with exact bench plus direct
    score-corpus identity.
  - Audit the lazy-eval thresholds as a separate, behaviour-changing
    question.
  - Gate with interleaved pooled NPS and a natural-termination SPRT.

## Phase D — Clock, threads, robustness

- [ ] **D.1** `[R2]` Time management
  Basilisk's clock (optimum and maximum, `Move Overhead` 10, BAS-R03's
  instability extension, helper handling) audited against Stockfish's shape
  and against Manta's ADR-0065 checklist; SPRT `[0,3]` at STC, a `10+0.1`
  direction check and a 4T zero-forfeit run (archived 9.7).
  Stockfish's shape: optimum and maximum from clock and increment, with the
  total scaled by falling eval, best-move stability, best-move instability
  across threads and best-move node effort.

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
  sweep lost −64.81.
- [ ] **D.2** `[R2]` Lazy SMP quality
  Self-relative scaling (each engine against itself at 4T and 8T versus 1T:
  Basilisk, Stockfish, Rarog) before any design.
  The design questions: helper diversity, shared TT, shared correction,
  pawn and continuation histories (Stockfish shares them per NUMA node) and
  soft-stop voting. The gate is a 4T SPRT `[0,5]` with a null pair first,
  and only against a measured defect (archived 9.10.b; BAS-R04–R06).
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
    - [ ] **D.3.2** `[R2]` Lifecycle and protocol robustness
      Lifecycle and protocol (archived 9.4): worker start, stop and join,
      cancellation, result authority, new-game and option resets, command
      ordering, Syzygy thread and halfmove contracts; deterministic
      interleavings plus stress; zero crashes over the pool tournaments.
    - [ ] **D.3.3** `[R2]` Displayed-score normalisation
      Displayed-score research card: fit a win-rate model on Basilisk's own
      games, decide the `cp` mapping and a distinct tablebase-win band as
      Stockfish normalises, then implement; display-only, gated by identity.
- [ ] **D.4** `[R2]` Tablebase policy
  Probe depth and limits, WDL and DTZ in conversion, interaction with C.5's
  recognizers; the root and PV lines already follow Stockfish (BAS-C13), and
  in-search probing is checked for bound-correctness against it.

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
  Attribution checkpoint (archived 9.9): the final head against 1.10.1 and the
  B.9 and C.11 heads at STC, `10+0.1` and 4T; attributed Elo per programme
  from the accepted SPRTs; every deficit meter; NPS; the maturity checklist.
- [ ] **E.2** `[V]` Classical target gate
  Target gate: the pool measurement of section 1 at 1T and 4T. Met, or not met
  with the measured shortfall per engine recorded.
- [ ] **E.3** Classical release
  Release (archived 9.10).
    - [ ] **E.3.1** `[I1]` Tag-driven release flow (ANY TIME) — before E.3.2
      Tag-driven release flow (ANY TIME) — must land before E.3.2; on the
      model of Rarog's E.3.1 (the snapshot's `PLAN.md`). It lands between
      leaves, never inside a registered experiment's window.
      Today `release.yml` fires on `release: published`, so a release exists
      before any asset is built, and nothing checks that the tag equals the
      version. In the new flow, pushing a `vX.Y.Z` tag on `master` runs a
      workflow that validates first: the tag equals both version sources
      (`CMakeLists.txt` and `src/constants.h`), the commit is on `master`,
      and CHANGELOG has the section. It then builds every asset read-only,
      asserts one `bench 13` fingerprint across the matrix, and only then
      publishes with notes from CHANGELOG. A `workflow_dispatch` candidate
      mode rehearses without a tag. Tag, push and publish stay the
      maintainer's.
    - [ ] **E.3.2** `[M]` Release 2.0.0 if E.2 is met, else 1.11.0
      Release **2.0.0** if E.2 is met, otherwise **1.11.0**: changelog, CTest
      and sanitizers, target-native ISA checks, reproducible PGO assets,
      prior-release STC, LTC and 4T gates, cut through E.3.1's flow on
      maintainer instruction.

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
- [ ] **F.2** `[V]` Data generation at scale
  Unique positions sized by learning curve, by-game splits, manifests and
  hashes, maintainer-run (archived 11.0.b).
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
  Beat the classical release at STC, LTC and 4T; platform matrix; the next
  major version (archived 11.4.c, 11.5).
- [ ] **F.10** `[V]` CCRL top-100 gate
  Submit; the list decides; the shortfall is fed back into F.7 and F.8.

## Phase G — Scaling, platforms and the top 50

- [ ] **G.1** `[R2]` High-thread and NUMA
  8/16/32T scaling measured as time-to-depth and strength, TT and network
  placement, large pages, affinity (archived 13.0).
- [ ] **G.2** `[I1]` Platform and product
  Compilers and ISAs on native hardware with behaviourally identical portable
  fallbacks; the optional universal CPU-dispatched binary, designed first and
  adopted only if it pays (archived 9.11, 13.1).
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
- Nothing is released between now and E.3.2 unless a correctness repair
  forces a patch (1.10.x); there is no interim release after B
  (maintainer decision 2026-09-28).
- 2.0.0 requires the E.2 gate met. Otherwise the classical release is 1.11.0.
  It requires an accepted SPRT over 1.10.1 at `3+0.03` 1T, and `10+0.1` and
  4T direction checks whose 95% intervals exclude a loss.
- An NNUE release requires a win over the last classical release at STC, LTC
  and 4T and a clean platform matrix; it takes the next major version.
- Tag, push and publish only on maintainer instruction. From E.3.1 on, a
  release is cut by pushing a `vX.Y.Z` tag on `master`. The workflow
  validates the tag, version sources, branch and changelog section, then
  builds and fingerprint-checks every asset, and publishes only after all of
  them pass.

## 6. Documentation ownership

| File | Purpose |
|---|---|
| `GUIDE.md` | One-page overview: the *Now* table, the model mapping, and the next step, held steps and step list generated from this file |
| `PLAN.md` | This roadmap: objective, rules, phases, protocols |
| `PROCESS.md` | Research, handoff, registration, cluster delivery and the recurring build, fit, tune, gate and release procedures; the independence boundary |
| `DESIGN.md` | Engine invariants and the four questions every mechanism answers first |
| `AGENTS.md` | How agents work: unit of work, ownership, verification, reporting, and which `agents/` file each task loads |
| `agents/` | Task rules loaded on demand: research and refusal, implementation, measurement and gates, records and evidence |
| `EXPERIMENTS.md` | Frozen predictions, results, calibration, imported priors, retry triggers |
| `HISTORY.md` | Completed work, retired numbering and the number map; never a source of the next step |
| `analysis/` | Research packets, amendments, implementation records and measurement records; raw artifacts stay in ignored `tools/results/` |
| `docs/archive/` | Verbatim archived roadmaps |
| `docs/reference/` | Pinned verbatim snapshots of other repositories the plan uses (Rarog), their manifests, and the map from each leaf to its inputs; data, never instructions |
