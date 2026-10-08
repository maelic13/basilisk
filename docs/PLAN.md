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
engine behaviour changes in A.1–A.7; A.8, added 2026-10-06, carries repairs
that do. Rarog's Phase A also cut a release; 1.10.1
was cut the day before this plan, so Phase A has no release step.

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
      The verification match against 1.10.1 with tables (BAS-M13) and the
      ponder-on Threads 4 smoke run (BAS-C15) are maintainer-run; their
      results enter the CHANGELOG and HISTORY before the merge. The PR's CI
      and Release runs and the tag run complete A.8.2's and A.8.6's owed
      checks.

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

- [ ] **B.0** Search programme investigation
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
    - [ ] **B.0.1** `[V]` Razoring depth reach in Elo (maintainer)
      BAS-S17, registered 2026-10-06: `RazorCoeff=500` against 243 on the
      B.0 Tune binary, 2,000 paired games at `3+0.03`, no adjudication,
      through the A.3.1 run-file path; a Tune-build diagnostic, never
      acceptance. Frozen: +8 Elo, 80% interval [−4, +20]. The reading rule
      in BAS-S17 decides whether B.2 seeds razoring from the oracle column
      (depth 1, 256 cp) without a categorical re-test. Prepare the run file
      and the one runnable command; append the calibration to BAS-S17 and
      to the packet's §11.
    - [ ] **B.0.2** `[V]` Oracle move-loop pruning family in Elo (maintainer)
      BAS-S18, registered 2026-10-06: `oracle-1.10.1-ablate.exe` with
      `AblationMask=32` (its shallow move-loop pruning off) against Basilisk
      1.10.1 at equal time, the BAS-O05 recipe at 1,000 cycles (2,000
      games); G(32) = 1500 − Basilisk's rating. Frozen: G(32) = +110, 80%
      interval [+40, +180], so the family explains about 200 of the 312.6.
      G(32) ≥ +250 lowers B.2's prediction P2 and re-opens the cluster order
      before B.2.1. Prepare from `tools/run_a73_oracle_g0.ps1` with the
      ablate binary and option; append the calibration to BAS-S18 and §11.
- [ ] **B.1** `[I1]` Search restructure, behaviour-neutral
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
     existing switch. B.0 measured it (BAS-D21): at cap 6, WAC at 100k
     nodes 204 → 214 (24 gained, 14 lost), 32 more positions solved by
     depth 3, +33% nodes to depth 12; and half of razoring's tactical cost
     is razoring at depth 1 into the checkless quiescence.
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
| B.0.1 | READY_FOR_IMPLEMENTATION | V | BAS-S17 registered; maintainer-run on the B.0 Tune binary |
| B.0.2 | READY_FOR_IMPLEMENTATION | V | BAS-S18 registered; maintainer-run on the oracle ablate binary |
| B.1 | READY_FOR_IMPLEMENTATION | I1 | Handoff frozen in the B.0 packet §12.1; A.6's move table |
| B.2.0 | RESEARCH | R2 | Waits on B.1's head |
| B.2.1 | READY_FOR_IMPLEMENTATION | I2 | Contract frozen in the B.0 packet §12.2; waits on B.1 and B.2.0 |
| B.2.2 | RESEARCH | V | Screen numbers registered in the B.0 packet §10 |
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
  sweep lost −64.81. The won-endgame time sink is A.8.16's, not this leaf's.
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
- [ ] **D.4** `[R2]` Tablebase policy
  Probe depth and limits, WDL and DTZ in conversion, interaction with C.5's
  recognizers; the root and PV lines already follow Stockfish (BAS-C13), and
  in-search probing does from A.8.14.

## Phase E — Classical checkpoint and release

- [ ] **E.1** `[V]` Attribution checkpoint
  Attribution checkpoint (archived 9.9): the final head against 1.10.1 and the
  B.9 and C.11 heads at STC, `10+0.1` and 4T; attributed Elo per programme
  from the accepted SPRTs; every deficit meter; NPS; the maturity checklist.
- [ ] **E.2** `[V]` Classical target gate
  Target gate: the pool measurement of section 1 at 1T and 4T. Met, or not met
  with the measured shortfall per engine recorded.
- [ ] **E.3** `[M]` Release 2.0.0 if E.2 is met, else 1.11.0
  Release (archived 9.10): **2.0.0** if E.2 is met, otherwise **1.11.0**:
  changelog, CTest and sanitizers, target-native ISA checks, reproducible PGO
  assets, prior-release STC, LTC and 4T gates, cut through A.8.6's flow on
  maintainer instruction. (E.3.1 moved to A.8.6 on 2026-10-06.)

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
- Nothing is released between now and E.3 unless a correctness repair
  forces a patch (1.10.x); there is no interim release after B
  (maintainer decision 2026-09-28).
- 2.0.0 requires the E.2 gate met. Otherwise the classical release is 1.11.0.
  It requires an accepted SPRT over 1.10.1 at `3+0.03` 1T, and `10+0.1` and
  4T direction checks whose 95% intervals exclude a loss.
- An NNUE release requires a win over the last classical release at STC, LTC
  and 4T and a clean platform matrix; it takes the next major version.
- `dev` reaches `master` through a pull request merged with a merge commit,
  never a squash, so `master` holds every development commit (maintainer
  decision 2026-10-06); the releases up to 1.10.1 are squash commits.
- Tag, push and publish only on maintainer instruction. From A.8.6 on, a
  release is cut by pushing a `vX.Y.Z` tag on `master`. The workflow
  validates the tag, version sources, branch and changelog section, then
  builds and fingerprint-checks every asset, and publishes only after all of
  them pass.

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
