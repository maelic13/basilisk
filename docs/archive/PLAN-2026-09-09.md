# Basilisk development plan

This is the maintainer-facing source of truth. [GUIDE.md](GUIDE.md) is its
short checklist mirror; [EXPERIMENTS.md](EXPERIMENTS.md) remains the detailed,
historical evidence ledger.

## 1. Current checkpoint

| Item | State |
|---|---|
| Branch | development |
| Released baseline | Basilisk 1.9.3; bench-13 fingerprint 11,941,440 |
| Accepted engine head | Group A plus 6.5.a rook scaling; bench 12,568,898; CTest 12/12 |
| Strength baseline | 6.5.a accepted at +3.29 +/- 4.61 Elo over frozen Group A; accepted HCE line previously about +12 Elo versus 1.9.3 |
| Current phase | Phase 6, step 6.6.a |
| Evaluation | HCE is unfrozen for structural improvement and complete, controlled refits |
| Corpus rule | Game-result labels only; no engine-evaluation labels |
| Match/data rule | Natural termination by default; score-based adjudication requires explicit opt-in and registration |
| Long job | None |
| Release target | Classical release after Phase 9; NNUE 2.0.0 after Phase 11 |

Step 6.5.a is accepted. Its truth-floored rook-ending scaling candidate was
implemented at `489c331`, benches 12,568,898 against the Group-A 12,709,666,
and passed its registered no-adjudication `[-5,+3]` SPRT at +3.29 +/- 4.61 Elo
over 6,332 games. The next unchecked leaf is 6.6.a.

## 2. Operating contract

`DESIGN.md` holds the engine invariants and the four questions every mechanism
must answer before it is implemented. `AGENTS.md` makes reasoned refusal and
refutation deliverables of equal standing to a diff.

- Work strictly in numbered order. A later step may be prepared, but may not
  change engine policy or consume experimental budget before its dependencies close.
- Commit each completed step with PLAN and GUIDE synchronized.
- Run python tools/diag/check_roadmap.py before committing either roadmap file.
- Consult and update EXPERIMENTS before retrying a mechanism.
- Preserve source, compiler, binary, book, corpus, split, seed, tablebase and
  command provenance. Hash immutable inputs and outputs.
- A behavior-neutral change needs the relevant static checks, CTest and exact
  bench. A playing change additionally needs its registered game gate.
- Strength tests use paired UHO openings and normalized Elo. Default SPRT is
  [0,3] nElo and broad/risky bundles use [-3,3]. **Non-regression uses [-5,0],
  which is what `tools/sprt.ps1 -Mode simplify` actually runs (`Elo0=-5,
  Elo1=0`);** this line previously said [-3,0] and disagreed with both the tool
  and 6.2's own text. A run may widen the upper bound deliberately -- 6.5.a used
  [-5,+3] -- but must record the bracket it used.
- **Accepting H1 is a decision, not an effect size.** H1 at [-5,+3] means the
  evidence favours "not a regression" over "a real loss"; it does not assert that
  the gain exceeds the upper bound. Report the point estimate and its interval
  beside the verdict, and never quote a bound as the measured Elo.
- Score-based draw/resign adjudication is off by default in every tool. An
  opt-in compatibility run must record the exact adjudication policy and must
  never be mixed with natural-termination evidence.
- Do not run competing CPU-heavy work while a long tournament, tune, datagen
  or fit occupies the machine.
- Long jobs are run by the user after the model prepares and verifies the
  instrument. The model analyzes returned artifacts and applies the registered verdict.
- Reference engines teach mechanisms and experimental design. Reimplement in
  Basilisk's idiom and fit Basilisk's scale; never copy constants as acceptance evidence.
- Texel fits linear HCE coordinates. A bounded HCE SPSA prices only nonlinear,
  capped or contextual evaluation terms after the linear fit. Search SPSA runs
  only after the final HCE freezes; the later NNUE transition gets its own tune.
- State the measurement layer, position set and node budget beside every
  endgame number. Theory truth, move quality, conversion, drawn-share bias and
  game strength answer different questions and are never aggregated.
- A guard is not qualified until a known-bad input makes it fail. Prove option
  wires with an absurd value, refuse mismatched cohorts/revisions/configurations,
  and treat an unchanged bench as necessary provenance rather than complete
  behavioral proof.
- Before a subsystem optimization, inventory producer -> stored state ->
  consumer -> invalidation/undo/reset, profile realistic search, classify the
  finding, and finish the analysis with a bounded implementation/no-change
  decision. A faster operation can still build a worse tree.

### Development states, ownership and capability classes

The normal playing-change path is:

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

The path is descriptive, not bureaucracy. Research can close with `NO_CHANGE`
or `NOT_WORTH_PURSUING`; documentation/provenance may close after
implementation; correctness and behavior-neutral work substitute the relevant
proof or performance gate. A playing-strength change normally may not bypass
`GAME_GATE`.

`READY_FOR_IMPLEMENTATION` is the critical boundary. Promotion requires a
measured local defect/opportunity, an explicit mechanism and semantics, credible
competing explanations, interacting mechanisms, invariants, the cheapest
falsifier, required instrumentation, deterministic/cheap qualification, an
expensive gate where applicable, acceptance/rejection rules and explicit
non-goals. The implementation owner may make normal engineering decisions but
does not redesign that research decision. A false premise returns the leaf to
`RESEARCH`.

PLAN owns current state, dependencies, readiness and deciding gates.
EXPERIMENTS owns frozen predictions, conditions, results, dispositions,
calibration and retry triggers. Linked `analysis/` packets hold detail; their
format is defined in [`analysis/README.md`](analysis/README.md). Measured
evidence outranks roadmap assumptions. Diagnostics explain; the registered gate
decides. No fit, depth, NPS, node, conversion or donor result silently converts
into Elo.

The maintainer owns long tournaments/SPRTs, large datagen, expensive tuning,
large PGO/profiling campaigns and other machine-saturating jobs. Agents prepare
and validate them, then analyze returned artifacts. Cheap local qualification
does not require command-by-command maintainer direction.

Open executable leaves carry an advisory capability class:

| Class | Use |
|---|---|
| `R3` | frontier research; unresolved causal or architecture work |
| `R2` | bounded but correctness-sensitive architecture/reasoning |
| `I2` | difficult implementation requiring strong reasoning |
| `I1` | well-specified implementation |
| `M` | mechanical documentation, manifests or provenance |
| `V` | verification or measurement work |

Classes are not workflow states or evidence. GUIDE owns the editable mapping to
current models. Completed historical model tags remain untouched.

## 3. Required evidence

| Change | Minimum gate |
|---|---|
| Tool/docs/refactor | Syntax/static checks; focused tests; full tests/bench when execution semantics can change |
| Endgame evaluator | Deterministic truth corpus, paired WDL/DTZ evidence, conversion floors, tactical regression, SPRT |
| HCE refit | Frozen corpus/splits/surface, fixed K, untouched test set, source restore proof, clean PGO SPRT |
| Search change | Deterministic regression/telemetry, 1T STC SPRT, relevant LTC/4T confirmation |
| Time/root/SMP | 1T STC, 1T 10+0.1, 4T 10+0.1, zero forfeits, topology/hash recorded |
| Release | Reproducible PGO assets, correctness matrix, prior-release and external-cohort games |
| Behavior-neutral hot path | Exact immediate fingerprint, targeted parity, pooled/interleaved NPS on an idle-enough host |
| Correctness repair | Independent invariant that fails on the old behavior; strength gate when deployed play changes materially |
| SPSA | Live interacting surface, prospective immutable horizon/estimator, perturbation proof, fresh PGO bake and independent SPRT |

Use tools/books/UHO_Lichess_4852_v1.epd for paired gates. SPRT decides strength;
node counts, traces, tactical suites, WDL/DTZ and fit loss explain the result.

### Active Phase 6 readiness map

Only the earliest unchecked leaf is active. Later Phase 6 leaves remain
`RESEARCH` until predecessor evidence supplies a valid handoff; imperative old
wording does not make them implementation-ready. The checklist line supplies
the leaf goal, roadmap order supplies the default dependency, and this table
supplies current state, evidence basis and promotion/deciding gate. **No open
Phase 6 implementation leaf is `READY_FOR_IMPLEMENTATION` today.**

| Leaves | State / class | Evidence and unresolved decision | Ready-to-advance condition / deciding gate |
|---|---|---|---|
| 6.6.a | `RESEARCH` / `R2` | BAS-X20 plus Basilisk's BAS-E47/E50/E51 instrument failures; exact donor deltas and affected history are unclassified | Complete the registered static audit below; each delta classified with historical dependants and no instrument edit |
| 6.6.b, 6.6.e, 6.6.f, 6.6.g | `RESEARCH` / `I2` | BAS-X20 demonstrates the failure classes, but 6.6.a must establish local semantics and adapters | 6.6.a produces exact implementation handoffs; deterministic parity/refusal/combination tests decide local qualification |
| 6.6.c | `RESEARCH` / `V` | Known-bad cases are named but depend on the selected 6.6.b schema/lifecycle | Implemented guards exist; every named mutant fails while the valid reference passes |
| 6.6.d | `RESEARCH` / `R2` | BAS-E45 and BAS-X17 show budget sensitivity; Basilisk deployment bracket is unmeasured | Real-clock measurement defines a justified bracket; provenance and repeat rule frozen |
| 6.6.h | `RESEARCH` / `V` | Which baselines are invalidated is an output of 6.6.a-g | Dependency map names only affected artifacts; matched reruns and explicit supersession refreeze Group A |
| 6.7.a-6.7.d | `RESEARCH` / `R3` (`6.7.d` `R2`) | BAS-E43/BAS-E49/BAS-X18 are priors; current local occurrence, kind, closure and budget ranking are unresolved | Local censuses/classification close with uncertainty and a ranked no-change-permitted decision; no implementation gate |
| 6.8.a | `RESEARCH` / `R3` | BAS-E53 proves an only-move defect and refutes depth/piece-choice accounts, but supplies no HCE/search/knowledge mechanism | Linked packet yields a held-out, falsifiable mechanism handoff or `NO_CHANGE`; research evidence decides promotion |
| 6.8.b-6.8.c | `RESEARCH` / `R2` (`6.8.c` `R3`) | BAS-E48/E49 and future 6.7 classifications establish questions, not mechanisms | Per-family local mechanism, interaction/closure map, falsifier and acceptance rule exist |
| 6.8.d | `RESEARCH` / `I2` | Tests cannot be specified honestly before 6.8.a-c select mechanisms | Accepted handoffs name deterministic theory/truth/closure cases and known-bad mutants |
| 6.8.e | `RESEARCH` / `R3` | No dependency-complete fitted surface exists yet | 6.8.a-d define the covariant cluster, frozen train/validation split and stop rule |
| 6.9.a | `RESEARCH` / `R2` | Gate tier depends on 6.7 occurrence and the complete 6.8 candidate | Candidate and occurrence are frozen; prospective tier, prediction and stop rule registered |
| 6.9.b | `RESEARCH` / `V` | No locally qualified Group B candidate exists | Required truth/no-veto/deterministic/tactical checks pass; maintainer-owned clean-PGO game gate decides closure |
| 6.10.a-6.10.c | `RESEARCH` / `R3` (`6.10.a` `R2`) | Current deficits are measured, but budget/theory/scale causes remain unresolved | Each packet reaches a falsifiable local mechanism or `NO_CHANGE` under 6.7 ranking and 6.6 instruments |
| 6.10.d | `RESEARCH` / `I2` | Implementation is conditional on 6.10.a-c evidence and valid promotion closure | Exact semantics and handoff exist; local qualification passes without adjacent changes |
| 6.10.e | `RESEARCH` / `V` | Gate is undefined until the dependency-complete remainder exists | 6.9 tier rule and marginal-value stop are preregistered; maintainer-owned gate decides |
| 6.11.a | `RESEARCH` / `M` | Final accepted evaluator/artifact set does not exist | All Phase 6 dispositions closed; immutable artifacts and hashes agree |
| 6.11.b | `RESEARCH` / `R2` | Rejects and gaps remain prospective | Ledger contains alternatives, calibration and objective retry triggers |
| 6.11.c | `RESEARCH` / `V` | Reconciliation depends on all family results | Every layer and hard veto reconciles without aggregate masking |
| 6.11.d | `RESEARCH` / `M` | Phase closure is not yet authorized | PLAN/GUIDE/EXPERIMENTS agree on accepted head; documentation check is the deciding gate |

#### Current research card: 6.6.a

- **Research question:** Which Rarog 4.10 instrument, lifecycle and provenance
  changes transfer to Basilisk without changing the meaning of existing
  evidence?
- **Known evidence:** BAS-X20 names the donor failures and repairs; BAS-E47,
  BAS-E50 and BAS-E51 show that Basilisk has already suffered material-shed,
  cohort and closure mistakes. Donor revision is fixed at `881c821`.
- **Leading hypothesis:** generic schema/cohort/provenance/refusal contracts are
  reusable, while process lifetime, UCI, language/build and report-field details
  need Basilisk adapters.
- **Competing hypotheses:** a repair is already covered locally; it is
  inapplicable; or changing it would invalidate a historical consumer and needs
  versioning plus explicit supersession rather than a port.
- **Interactions:** truth/report schemas, family seeding and sharding, holdout
  identity, engine lifecycle, build/revision manifests and every baseline or
  floor consuming those artifacts.
- **Cheapest discriminating work:** static source/contract delta plus a
  producer -> artifact -> consumer dependency map. Do not modify an instrument
  during this pass.
- **Falsifiers:** no semantic local equivalent exists, or an apparently direct
  port changes a recorded historical condition that cannot be isolated by a
  new version.
- **Prospective prediction (frozen before the audit):** most generic validation
  and provenance guards will be directly portable or already covered; runtime
  adapters and build matrices will need local design. Confidence: medium (70%).
- **Stop / READY condition:** stop after every donor delta and historical
  dependant is classified. Promote follow-on leaves only when exact semantics,
  non-goals, tests and supersession consequences are written; `NO_CHANGE` is
  valid for already-covered or inapplicable deltas.

## 4. Closed release phases

- [x] **1.0** Foundations and first strength line — 1.0.0 through 1.8.0.
  - [x] **1.0.a** Board, move generation, UCI, PVS/qsearch, TT, histories, SEE and Syzygy.
  - [x] **1.0.b** Time management, Lazy SMP, reproducible tests and accepted HCE.

- [x] **2.0** Correctness and search architecture — 1.9.0.
  - [x] **2.0.a** State, repetition/rule-50, TT/mate and SEE/pin correctness.
  - [x] **2.0.b** Staged ordering, correction/history, root-instability timing and dense TT.

- [x] **3.0** Hardening, CI and PGO speed — 1.9.1.
  - [x] **3.0.a** Centralized parameters, invariants, fuzzing, CI and telemetry.
  - [x] **3.0.b** Behavior-identical PGO speed pass accepted at +4.34% NPS.

- [x] **4.0** SMP durability and release tooling — 1.9.2/1.9.3.
  - [x] **4.0.a** SPSA/MT harness and helper clock/node/thread safety repaired.
  - [x] **4.0.b** Four-thread bundle accepted; PGO tool matching fixed without search change.

## 5. Completed development foundation

Everything complete remains here. Phase 6 is the first open engine phase, so no
completed item has a number after unfinished work.

- [x] **5.0** Reproduce the 1.9.3 baseline.
  - [x] **5.0.a** Freeze benchmark, test and compiler evidence.

- [x] **5.1** Measure search/evaluation authority.
  - [x] **5.1.a** Search oracle measured +322.7 +/-36 Elo.
  - [x] **5.1.b** HCE oracle measured +232.8 +/-32 Elo.

- [x] **5.2** Build differential diagnostics and inventory.
  - [x] **5.2.a** Add the 107-position diagnostic suite and diag-kv telemetry.
  - [x] **5.2.b** Split candidate work into dependency-complete clusters.

- [x] **5.3** Close search cluster A: ordering, histories and LMR.
  - [x] **5.3.a** Reject reduction magnitude on harness evidence.
  - [x] **5.3.b** Reject check-depth change by games.

- [x] **5.4** Close search cluster B: static-eval, TT and qsearch contracts.
  - [x] **5.4.a** Confirm existing contracts; accept no engine change.

- [x] **5.5** Close search cluster C: main selectivity.
  - [x] **5.5.a** Record the history-pruning defect and exhausted budget boundary.

- [x] **5.6** Close completed extension and root evidence.
  - [x] **5.6.a** Retain only mechanisms supported by completed gates.
  - [x] **5.6.b** Defer singular-extension depth and clock work to the final classical-search phase (now Phase 9).

- [x] **5.7** Audit shallow-depth node cost.
  - [x] **5.7.a** Measure rather than assume a width deficit.
  - [x] **5.7.b** Withdraw the target after the constant-factor diagnosis.

- [x] **5.8** Enlarge and freeze the HCE feature surface.
  - [x] **5.8.a** Add seven coverage terms.
  - [x] **5.8.b** Add bishop outpost and split king-protector structure.
  - [x] **5.8.c** Identify endgame technique as the remaining structural gap.

- [x] **5.9** Diagnose the first joint-fit failure.
  - [x] **5.9.a** Run the distilled-corpus refit.
  - [x] **5.9.b** Trace mate-drive loss to score-adjudicated corpus truncation.
  - [x] **5.9.c** Establish on-policy self-play WDL labels as the fit contract.

- [x] **5.10** Accept the repaired HCE line.
  - [x] **5.10.a** Accept king-safety refit.
  - [x] **5.10.b** Accept full-surface refit and freeze its baseline artifact.

- [x] **5.11** Remove redundant HCE terms.
  - [x] **5.11.a** Gate simplification without losing accepted strength.

- [x] **5.12** Inventory and improve endgames.
  - [x] **5.12.a** Inventory twenty reference endgame families.
  - [x] **5.12.b** Improve KBNK conversion from 13% to 54.5%.

- [x] **5.13** Add deterministic conversion floors.
  - [x] **5.13.a** Cover KQK, KRK, KBBK and KBNK with fixed-seed tests.
  - [x] **5.13.b** Record denominators and avoid treating tiny percentage changes as truth.

- [x] **5.14** Repair basic mate drive and diagnose KBNK residue.
  - [x] **5.14.a** Complete KXK/KBBK drive.
  - [x] **5.14.b** Classify KBNK failures: stalled drive, bishop-move ties and rule-50 loss.
  - [x] **5.14.c** Preserve the 198-position cohort for paired follow-up.

- [x] **5.15** Port the generalized endgame-truth instrument.
  - [x] **5.15.a** Support named <=6-man families with family-stable deterministic seeds.
  - [x] **5.15.b** Separate Syzygy WDL truth, WDL preservation, DTZ progress and conversion.
  - [x] **5.15.c** Emit per-position records and honest denominators for paired analysis.
  - [x] **5.15.d** Record engine identity/hash and prevent the tested engine from using Syzygy.

- [x] **5.16** Make no-adjudication the toolchain default.
  - [x] **5.16.a** Default SPRT, fixed gauntlet and datagen to natural termination.
  - [x] **5.16.b** Default weather-factory SPSA and its reinstall patch to natural termination.
  - [x] **5.16.c** Remove adjudication from all Colosseum strength, SPSA, tournament and datagen profiles.
  - [x] **5.16.d** Retain explicit opt-in only for registered legacy-compatibility runs.

- [x] **5.17** Synchronize the roadmap mechanically.
  - [x] **5.17.a** Reorder phases around endgame-first HCE development.
  - [x] **5.17.b** Add a PLAN/GUIDE checklist consistency checker.

## 6. Endgame maturity and truth

### 6.0 Evidence contract

- [x] **6.0** Establish the truth baseline before another evaluator edit.
  - [x] **6.0.a** Freeze 770 Syzygy-verified positions across 21 endgame families.
  - [x] **6.0.b** Measure the accepted Basilisk head and a strong reference at identical nodes.
  - [x] **6.0.c** Record attained family reference results; neither 100% nor the reference result is a finite-node ceiling.
  - [x] **6.0.d** Define paired confidence rules: aggregate beyond 2 SE reports, family beyond 3 SE blocks.
  - [x] **6.0.e** Add hard theory vetoes for clean-win discard, illegal play, crash and rule-50 regression.
  - [x] **6.0.f** Census disagreements between self-play WDL labels and Syzygy on every <=6-man corpus row.
  - [x] **6.0.g** Port Rarog's exact search-tree occurrence census for the 20 reference endgame families.

Step 6.0.a artifact: tools/diag/endgame_cohort_v1.epd and its manifest use
seed 0x4E9A2 and SHA-256
3CCF28EA3C8BC6C7E995BDA0BDD4833496B4C46758FF55C0B812E30E8AC6BF1B.
The 770 unique records contain 480 clean wins, 289 draws and one cursed win;
each carries family-stable seed, FEN, exact WDL and signed DTZ. The requested
24-clean-win/16-rule-draw family mix is left explicitly short where theory
does not supply that class. KRPP-KRP is seven men and remains outside the
available six-man tables.

Step 6.0.b used 60,000 nodes/move, one engine thread, 16 MB hash, a
100-ply diagnostic limit, disabled engine tablebases and no score adjudication.
All 770 IDs, FENs and theory labels paired exactly. Accepted head `294a3e2`
(binary SHA-256 D0E558F8A113CD9D17905B6EF701040CF5B0FB450FC058EFB9C49DF2329F8430)
converted 293/480 clean wins (61.04%); Stockfish
`dev-20260716-ebcea3ef` (binary SHA-256
91AE61DFCAEF1A5FDFEE9722EDE0591DA1FCB124D1DE7FBD44DD8786BD6531E3)
converted 389/480 (81.04%). The paired conversion matrix was 277 both, 16
Basilisk only, 112 reference only and 75 neither. Aggregate move-level
win-preservation was 98.08% versus 99.78%, and absolute-DTZ progress was
43.46% versus 56.41%. These variable-length move samples are descriptive and
autocorrelated; 6.0.c sets finite-budget ceilings and 6.0.d owns confidence.
The largest conversion deficits were KBP-K (6/24 versus 22/24), KQ-KR
(11/24 versus 24/24), KNN-KP (1/24 versus 14/24), KBN-K (12/24 versus 24/24)
and KQ-KRP (13/24 versus 23/24).

**6.0.b correction (2026-09-03, BAS-E47).** The figures above were produced by
an instrument that ended a game whenever the strong side's piece count dropped,
which aborts correct pawn technique. In those artifacts 178 of the Basilisk
arm's 193 `material_lost` outcomes, and 173 of the reference arm's 174,
occurred with no non-win-preserving move played. Both arms were re-run under
the same registered conditions with both binaries verified by SHA-256, only the
termination rule changed: Basilisk converts **361/480 (75.21%)**, not 293/480,
and the reference **466/480 (97.08%)**, not 389/480. The gap widens slightly
from 96 to 105 positions, so the deficit motivating 6.3-6.7 is real. The
original numbers are left above as the historical record and are superseded;
artifacts are in `tools/results/endgame-truth-6.0.b-refixed/`. 6.0.c's frozen
ceilings derive from the contaminated reference arm and must be re-checked
before reuse.

Step 6.0.c freezes `tools/diag/endgame_ceilings_v1.json`. Its historical field
name `attained_single_engine_ceiling` means only the best conversion count
observed in the two complete 60k runs. The accurate term is **attained reference
result**: it is neither a theoretical nor empirical upper bound, Basilisk may
surpass it, and failure to equal it is not itself a rejection. The paired union
is a stretch diagnostic only: it proves that each included position was
converted by at least one engine, not that one engine can convert the union.
A future accepted head may ratchet the recorded reference result upward through
another complete run under the identical cohort and search contract.
Conversion is not applicable to KNN-K because the frozen family contains no
clean theoretical wins. Move-level WDL/DTZ rates remain explanatory because
their variable game lengths make the samples dependent; 6.0.d owns their
comparison rule.

| Family | Clean wins | Accepted | Attained 60k reference | Paired union |
|---|---:|---:|---:|---:|
| KQ-K | 24 | 24 | 24 (100%) | 24 |
| KR-K | 24 | 24 | 24 (100%) | 24 |
| KBB-K | 24 | 24 | 24 (100%) | 24 |
| KBN-K | 24 | 12 | 24 (100%) | 24 |
| KNN-K | 0 | n/a | n/a | n/a |
| KP-K | 24 | 24 | 24 (100%) | 24 |
| KPP-K | 24 | 10 | 12 (50.0%) | 13 |
| KBP-K | 24 | 6 | 22 (91.7%) | 23 |
| KR-KP | 24 | 23 | 24 (100%) | 24 |
| KR-KB | 24 | 21 | 24 (100%) | 24 |
| KR-KN | 24 | 19 | 23 (95.8%) | 23 |
| KQ-KP | 24 | 24 | 24 (100%) | 24 |
| KQ-KR | 24 | 11 | 24 (100%) | 24 |
| KNN-KP | 24 | 1 | 14 (58.3%) | 15 |
| KRP-KR | 24 | 3 | 9 (37.5%) | 10 |
| KRP-KB | 24 | 5 | 9 (37.5%) | 12 |
| KBP-KB | 24 | 7 | 16 (66.7%) | 18 |
| KBP-KN | 24 | 13 | 15 (62.5%) | 19 |
| KP-KP | 24 | 24 | 24 (100%) | 24 |
| KQ-KRP | 24 | 13 | 23 (95.8%) | 23 |
| KBPP-KB | 24 | 5 | 6 (25.0%) | 9 |

Across all families the attained reference result is 389/480 (81.04%); the non-additive
paired union is 405/480 (84.38%). `tools/diag/endgame_ceilings.py` validates
the source schemas, hashes, identical contract and exact ID/FEN/theory pairing
before reproducing the artifact, so neither number can silently mix cohorts.
The frozen artifact SHA-256 is
19E43A2E7EEF9069E1EE8575ABF0E622BDF97887557A64023ED15F8C7E46508D.

**6.0.c correction (2026-09-03, BAS-E50).** The table above and
`endgame_ceilings_v1.json` were derived from the contaminated 6.0.b arms.
Regenerated from the corrected re-run with the identical two binaries -- so the
instrument fix is the only delta -- as `tools/diag/endgame_ceilings_v2.json`:
the attained 60k reference result rises from 389/480 to **466/480** and the
paired union from 405/480 to **467/480**. Seven family ceilings were
understated: KBPP-KB 6 to 23, KRP-KR 9 to 24, KRP-KB 9 to 24, KPP-K 12 to 24,
KBP-KN 15 to 24, KBP-KB 16 to 23, KBP-K 22 to 24.

The paired matrix moves too, and its shape is the more interesting part. "Both
converted" rises 277 to 360, "neither" collapses 75 to **13**, and
"accepted-only" falls 16 to 1. Most of the old `neither` bucket was not a pair
of engines failing a hard position; it was two aborted games. Of 480 clean
wins, 467 are now demonstrably convertible by at least one engine at 60,000
nodes, so the residual genuinely-hard set is 13 positions, not 75.

v1 is left in place as the historical artifact and is superseded, not deleted.
`endgame_ceilings.py` now defaults to the corrected inputs and to a v2 output,
so a regeneration cannot silently restore v1's numbers. No new games were
required; both corrected arms already existed.

Step 6.0.d is implemented by `tools/diag/endgame_compare.py`. The independent
unit is one paired frozen position, not one engine move. Conversion is binary;
win-preservation and DTZ progress are first reduced to one rate per position,
giving long games no artificial extra weight. A clean-win position whose
candidate discards before any DTZ comparison receives zero DTZ progress rather
than disappearing from the sample. Aggregate movement at or beyond 2 SE is
reported and must be explained but is not by itself a veto. Within a family,
movement at or beyond 2 SE is reported and a regression at or beyond 3 SE
blocks the candidate. Improvements never establish strength or bypass SPRT;
they nominate a later accepted result for ceiling/floor ratcheting. Invalid or
unpaired reports fail closed, while the absolute theory rules remain 6.0.e's
separate responsibility.

Step 6.0.e is implemented by `tools/diag/endgame_vetoes.py` and fail-closed
reporting in `endgame_truth.py`. Engine crash/error, illegal move and a no-move
response from a nonterminal position are absolute vetoes anywhere in the
cohort. Existing accepted-head theory debt is grandfathered, but a candidate
may not introduce a clean-win discard or rule-50 failure on a clean-win
position where the accepted baseline avoided it. These position-level vetoes
are objective trajectory regressions and cannot be traded against aggregate
improvement or a positive SPRT. The accepted report passes itself with zero
vetoes. As an intentional independence check, the stronger reference is not an
acceptable Basilisk candidate under this contract: despite its aggregate gain,
it newly discards clean wins at KNN-KP EG0459/EG0460 and newly reaches rule 50
at EG0464. This confirms that the ceiling oracle and the candidate correctness
gate answer different questions.

Step 6.0.f is implemented by `tools/diag/endgame_label_census.py` and freezes
`tools/diag/endgame_label_census_v1.json`. It scanned all 1,052,632 rows of the
accepted Arm C corpus (1,000,000 train and 52,632 holdout), verified exact
`0/0.5/1` White-perspective labels, and probed every one of the 173,750 rows
with at most six pieces. There were 24,530 self-play/Syzygy disagreements
(14.12%): 22,392 are self-play draws in tablebase-decisive positions, 2,128
are decisive game labels on tablebase draws, and only 10 reverse the decisive
winner. KBN-K alone contributes 6,117/6,547 disagreements and K-KBN contributes
4,778/5,248; together, all 10,895 disagreements are natural-termination draws
where Arm C failed to convert the theoretical KBNK win. This directly supports
doing 6.1 before the next full HCE refit. Syzygy's seven cursed and fourteen
blessed rows remain draws in the label domain. The census deliberately uses
zeroing-clock WDL and does not authorize Phase 8.4 relabeling without its
separate halfmove-clock and row-domain analysis. Corpus hashes and parent
no-adjudication PGN provenance are embedded in the artifact; its SHA-256 is
609E60489838F6708ADF83D851B8CDB5D1E963102A2C599879ECC9C9F2E5CB46.

Step 6.0.g adds tune-build-only exact counters at the start of every full
evaluation and `tools/diag/endgame_search_occurrence.py` to aggregate a fixed
suite at fixed depth. The runner defaults to 30 independent workers on this
32-thread machine, while every engine remains `Threads=1`; worker count changes
wall time, not the deterministic tree. The report records engine and suite
hashes, nodes, full evaluations and the <=7-man denominator. Counts are a
candidate-priority screen, never an Elo gate. KPK overlaps the KPsK aggregate
and KBNK overlaps KXK. Unlike the source instrument, Basilisk's KXK counter
requires actual mating material: queen, rook, bishop+knight, or a bishop pair
tested by SQUARE COLOUR rather than by count, matching `apply_endgame`'s own
KXK gate. Dead KBK, KNK and same-coloured KBBK are therefore excluded. Queen
and rook stay in the family census even though the engine deliberately leaves
their search-solved mates unoverridden (BAS-E30, BAS-E34): the instrument
measures family frequency, not `kxk_score` firings.

### 6.1 Complete KBNK mate drive

- [x] **6.1** Implement and tune the missing KBNK technique (historical step 5.9.22).
  - [x] **6.1.a** Start from Rarog's useful finding: bishop-color corner diagonal potential can solve the drive without a bishop-position term.
  - [x] **6.1.b** Make the bishop-colour diagonal mechanism explicit and prove the existing Manhattan form was algebraically identical.
  - [x] **6.1.c** Scale coefficients to Basilisk and test the required diagonal dominance against its existing edge, king-distance and knight-distance terms.
  - [x] **6.1.d** Do not retry bishop proximity or escape-square count unless new evidence overturns their earlier failure.
  - [x] **6.1.e** Compare on all 198 positions, with positions 61-198 as the held-out confirmation set; report WDL preservation, rule-50 failures, conversion and mate efficiency.
  - [x] **6.1.f** Require KQK/KRK/KBBK non-regression, tactical stability and bench accounting; exact bench identity is necessary but cannot prove this path-dependent evaluation change behaviorally neutral.

Step 6.1.a is frozen in `analysis/kbnk_diagonal_port_v1.md`. Rarog commit
`4aea0c7` replaced a coarse corner drive with a weak-king diagonal potential
selected only by the winning bishop's square colour. At 60k nodes it moved
KBN-K conversion from 19.4% to 96.9% and eliminated 61 rule-50 failures; the
successful sweep also showed that the corner pull must dominate the king pull.
For Basilisk, a dark-squared bishop uses `abs(7-rank-file)` toward a1/h8 and a
light-squared bishop uses `abs(rank-file)` toward a8/h1. Basilisk's existing
exact-material dispatcher already supplies the required bishop colour, weak
king square, score sign and narrow activation. Therefore 6.1.b owns only the
geometry port; 6.1.c owns Basilisk-scale constants and interaction with the
current edge, king and knight pulls. No bishop-position term is licensed.

Step 6.1.b found that the proposed geometry was already present. For the a1/h8
corner complex, Basilisk's existing `14 - min(Manhattan)` is algebraically
`7 + abs(7-rank-file)`; for a8/h1 it is `7 + abs(rank-file)`. The constant seven
cannot affect ordering. `kbnk_score()` now spells those diagonal formulas
directly while retaining the constant and Basilisk's existing weight, making
the rewrite score- and bench-identical. Both bishop-colour orientations have
focused regression coverage. Consequently no new geometry candidate exists:
6.1.c owns the remaining Rarog finding—scale and dominance relative to the
edge, friendly-king and knight terms. Release `test_eval` and `test_endgames`
pass, and `bench 13` remains exactly 12,709,666 nodes. No conversion gain is
claimed because the score is unchanged.

Step 6.1.c used a tune-build-only atomic `KBNK Drive` option in
`diagonal,edge,king,knight` order. The option rejects malformed, negative and
combined vectors whose largest legal KBNK score would enter the mate-score
band. The release UCI remains clean. The historical BAS-E35 LCG source is
frozen as `tools/diag/kbnk_cohort_v1.*`: exactly 198 Syzygy clean wins retained
from the original 200 generated positions, with source indices, WDL/DTZ labels
and tablebase inventory provenance.

The registered screen runs baseline, diagonal 600/1000, each competing-term
ablation, edge+knight ablation, and three increasingly Rarog-shaped dominant
diagonal vectors. It uses the first 60 frozen positions, 60,000 nodes/move,
100 plies, 30 independent one-thread workers, persistent TT only within each
game, engine tablebases disabled, and natural termination with no score
adjudication. All variants use the same tune binary and positions. Reject any
engine/protocol anomaly; then rank conversion on the paired positions, using
clean-win preservation, DTZ progress and mate efficiency diagnostically and
preferring the simpler vector when practically tied. The two leaders both
converted 31/60 versus baseline 26/60. `dominant-diagonal` (`1000,0,220,0`)
won the tie: it discarded only 3 clean wins versus 12 for
`diagonal-1000` (`1000,900,220,220`), had no hard anomaly, raised clean-win
move preservation from 99.5800% to 99.8676%, reduced stalemates 7 to 2, and
removed the edge and knight pulls. Its 15 paired gains and 10 losses made it a
promising first-pass bundle rather than proof; the correction below supersedes
its provisional completion. Summary SHA-256:
`ED0A554855D9B61273E968EDF73A5FBEE96046AA5E8F5D0DBCC1616A77FAAEDF`.

**6.1.c correction (2026-09-03):** that first-pass comparison did not isolate
the claimed mechanism. The implementation was `(7 + diagonal) * weight`, so
800 -> 1000 strengthened the position-dependent slope but also raised every
KBNK score by 1,400. Absolute score changes can alter pruning and search even
though they do not alter static move ordering inside the class. The selected
vector also removed edge and knight pulls, so its 31/60 cannot be attributed
to diagonal dominance alone. The completion was revoked before 6.1.e.

The corrected parameterization is `base + diagonal*slope + edge*E +
king-distance*K + knight-distance*N`, controlled atomically in tune builds as
`base,diagonal,edge,king,knight`. Its current defaults reproduce the provisional
first-pass score exactly, so the refactor itself is behavior-neutral. The old
four-field form remains accepted and maps to `base = 10000 + 7*diagonal`,
allowing the original artifacts and script to remain reproducible.

The registered refinement first requires the exact old baseline
`15600,800,900,220,220` and first-pass winner `17000,1000,0,220,0` to reproduce
their earlier paired results. It then holds base at 15,600 while testing a
3-by-3 diagonal/king grid, adds a lower diagonal boundary, and independently
tests base 14,200/17,000/18,400 at the centre vector. All other cohort, node,
ply, worker, tablebase and no-adjudication conditions remain identical. Any
control drift or hard anomaly invalidates the run. Otherwise rank paired
conversion first, clean-win discards second, and use WDL preservation, DTZ
progress and mate efficiency diagnostically. Because these 60 positions select
among 15 arms, they are development data, not confirmation: 6.1.e must make
positions 61-198 its primary held-out verdict and report the all-198 aggregate
only as secondary context. The refinement completed with summary SHA-256
`FAF6D1CC1A4AB43AF67DAD4D97B9CA2E247AC65F36807A3260C3D3F55814F5FA`.
Both behavior-neutral controls reproduced their original per-position
fingerprints exactly.

The result rose strongly through the diagonal boundary: legacy converted
26/60; the best 1250 arm converted 39/60; and diagonal 1450 converted 42, 46,
and 47 with king weights 140, 220, and 300. The independent base axis at
diagonal 1250/king 220 converted 35, 36, 31, and 36 at bases 14200, 15600,
17000, and 18400, so base-score inflation neither explains nor monotonically
drives the gain. However, all three diagonal-1450 arms discarded the clean win
at ply zero on `KBNK0039`; the 220 and 300 arms then lost a piece at ply four.
They are correctness failures regardless of aggregate conversion. The current
provisional default therefore remains only a control, not an accepted choice.

The final upper-range screen retained exact legacy and 1450/300 fingerprints,
tested diagonal 1350-1900 against king weights 340/460 where mate-band safety
allowed, and admitted no candidate with a new truth discard before ply 80. A
one-position 60k preflight excluded combinations already known to discard
`KBNK0039`; diagonal 2000 also fails that gate, so 1900 plus the static
mate-score bound closed the useful upper search interval.

Both registered controls reproduced exactly: legacy converted 26/60 at
99.5800% clean-win preservation with seven stalemates, and the 1450/300
boundary control converted 47/60 while still discarding `KBNK0039` at ply
zero into `insufficient_material`. The run is therefore valid. Its engine was
built from `12b40b6`, before the pending 6.0.g diagnostic landed; engine
SHA-256 `34AB7B682425020D8BBD97C80924BF2140119D127A57532811D09FDC5D2963B8`
and summary SHA-256
`12D7B76C69662C1B91A01AAE02E4CD6FEAD4792C30DB73762EA083FE7E6D4B59`.

The truth veto removed exactly two arms, and no arm produced a hard anomaly:
the 1450/300 control (`KBNK0039`, ply 0) and candidate `d1650-k460`
(`KBNK0023`, ply 6). Ranking the survivors by paired conversion selects
`15600,1750,0,340,0`, now the provisional default. It converted 51/60 against
the legacy 26/60 for 27 paired gains and 2 losses, cut discarded clean wins
from 10 to 3, raised clean-win preservation to 99.8448%, posted the best
admissible DTZ progress at 0.6192, reduced stalemates from 7 to 1 and
fifty-move draws from 27 to 8 -- the next best arm reaches only 14 -- and
solved `KBNK0039` in 43 plies, faster than any other variant.

Registered design caveat carried into 6.1.e: the winner is separated from the
legacy baseline overwhelmingly (paired z approximately +4.6) but is not
separated from its own plateau. Against `d1750-k460`, `d1850-k340` and
`d1900-k460` the paired z values are only +1.61, +1.70 and +1.21, and the
king-340 row is non-monotone across diagonal 1350-1900 (37, 33, 41, 51, 44,
40). These 60 positions have now selected across three rounds and 42 arms, so
51/60 is a winner's-curse point estimate and the held-out result should be
expected to fall. The defensible claim from 6.1.c is that the
diagonal-dominant region as a whole beats the legacy vector decisively, not
that (1750, 340) is its optimum. Accordingly 6.1.e must treat positions
61-198 as its primary verdict, and should carry `15600,1900,0,460,0` (46/60,
2 discards, 99.8987% preservation) as a second arm so that a noisy peak can
be distinguished from the true plateau level.

Step 6.1.d records the closure rather than implementing anything. BAS-E36
diagnosed the KBNK failures as stuck rather than slow and traced the cause
correctly: nothing in `kbnk_score` depends on the bishop, so every bishop move
scores alike and the potential has a flat maximum that is not mate. The
inference drawn at the time was to add a bishop-dependent term, and both
attempts failed. Bishop proximity (weight 300) moved conversion 94 to 88,
inside one SE, but took piece loss before ply 10 from zero to two: a long-range
piece parked beside a bare king gets captured. Escape-square count (weight 400)
dropped KBN-K conversion from 14/16 to 9/16, roughly 3.8 SE, failing the
deterministic CTest floor.

BAS-E39 has since removed the motivation as well as leaving the refutations
standing. The flat-maximum diagnosis was right, but the remedy was never a
bishop term: steepening the diagonal gradient and deleting the competing edge
and knight pulls fixed most of the stuckness with the bishop still absent from
the potential, cutting fifty-move draws from 27/60 to 8/60 and stalemates from
7 to 1. A bishop feature would now pay its known costs against a much smaller
residual. Both mechanisms are entered in the EXPERIMENTS.md retry map with
narrow triggers: proximity only if an instrument against the accepted vector
still shows bishop shuffling dominant, gated on zero clean-win loss and zero
piece loss before ply 10; escape-square count only if stalemate adjacency is
tested directly, the term is shown to reinforce rather than compete with the
corner drive, and it clears the KBN-K floor it broke. Neither is licensed for
6.1.e or 6.1.f.


Step 6.1.e runs the complete frozen 198-position cohort once per arm and
decides on positions 61-198 only. Three arms: the legacy control
`15600,800,900,220,220`, the 6.1.c selection `15600,1750,0,340,0`, and the
plateau probe `15600,1900,0,460,0` carried forward because BAS-E39 could not
separate them. Conditions are identical to the screens it confirms: 60,000
nodes/move, 100-ply limit, 16 MB hash, 30 independent one-thread workers,
engine tablebases disabled, natural termination, no score adjudication.

Two preparation measurements license the design. First, batch size does not
perturb per-position results: the first 60 records of a 198-position legacy
run reproduce the published BAS-E39 legacy fingerprint
`A28D2843263A8DDCB760C1133D5F105AE08E1F110AE4BB410210C683A15F1BF8`
byte-for-byte, so one run can be split into development and held-out halves
without changing what the development half means. Second, two independent
198-position legacy runs produced the identical fingerprint
`8CCB2C20380879C2025B017A661167CC60309A469C996F0C69F545D985CFD539`, so the
instrument is deterministic at this worker count. Both are hard gates in
`tools/diag/kbnk_holdout_summary.py`; either mismatch voids the run before any
candidate may be read. The drift gate was verified to fire on a perturbed
control, and the truth veto to override a large positive conversion result.

Disclosed pre-exposure: those preparation runs were the legacy control, whose
all-198 conversion is 95/198 at 0.9958 win preservation, giving a held-out
baseline of 69/138. The control is not a candidate and the verdict rule below
was registered before any candidate arm was run.

Pre-registered verdict. Reject any arm with a hard anomaly, or with a live
truth discard before ply 80 on either split, regardless of conversion. The
primary verdict is paired conversion against the legacy control on held-out
positions 61-198, accepted at McNemar z >= 2.0. Two surviving candidates count
as separated only at |z| >= 2.0 between them; otherwise prefer fewer discarded
clean wins, then higher DTZ progress, then the simpler vector. Positions 1-60
are reported solely to quantify shrinkage from the selection estimate and can
never rescue a failed held-out verdict. The all-198 aggregate, rule-50
failures, stalemates, WDL preservation and mate efficiency are diagnostic
context, not the verdict. Expect roughly four minutes at 30 workers. Run:
`powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\diag\run_kbnk_holdout.ps1`.


Step 6.1.e result: the held-out set rejected the 6.1.c winner and confirmed
the plateau probe, so the accepted vector is now `15600,1900,0,460,0`. Both
control fingerprints reproduced. On held-out `KBNK0061`
(`8/8/8/2K5/8/3k4/8/N1B5 w`, clean win at DTZ 52) the 6.1.c winner plays
1.Kd5 Ke2 2.Nc2, which Syzygy scores from +2 to 0; Black replies 2...Kd1
forking the undefended bishop on c1 and knight on c2, and after 3.Ke4 Kxc1 the
game is K+N versus K by ply 5. That is a correctness veto and outranks its
higher raw conversion of 103/138. The accepted vector plays 1.Nb3 and converts,
scoring 98/138 held out for 42 paired gains against 13 losses at z +3.91, with
no live discard anywhere in the 198.

The cause is the flat-maximum pathology BAS-E36 named for the bishop, now
visible for the knight: with edge and knight weights zero, no term in
`kbnk_score` refers to the knight, so all knight moves score alike and the
search tiebreak can pick a losing one. Steepening the diagonal does not remove
that degree of freedom. It is not licensed for repair inside 6.1 — 6.1.d closed
the bishop analogue and any knight-referencing term is a new mechanism needing
its own registration.

The registered caveat held in both directions. Head-to-head on held-out rows
the candidates are 24/19 discordant at z +0.76, statistically
indistinguishable, which is the plateau BAS-E39 predicted; and shrinkage was
real, 85.0% to 74.6% for the rejected arm and 76.7% to 71.0% for the accepted
one. Carrying the second arm is the only reason 6.1.e ends with a replacement
rather than an empty result. Diagnostics across all 198: conversion 95 to 144,
rule-50 draws 87 to 53, stalemates 16 to 1, clean-win preservation 99.5750% to
99.8467%, DTZ progress 0.4897 to 0.5880, discarded clean wins 32 to 10 with the
earliest at ply 96. Full record in EXPERIMENTS.md as BAS-E41; summary SHA-256
`FD09E1E1E872144E7476DE045D65A439C4F17D69496340F49B371EF0CCC437A1`.

Two limits carried into 6.1.f and 6.2. Conversion on a truth cohort is not Elo.
And the accepted vector's largest static KBNK score is 31,660 against the
31,872 mate-band floor, only 212 points of headroom, so the diagonal and king
weights are now effectively bounded from above by that check rather than by
evidence.


**Step 6.1.f was redone on 2026-09-04; see the correction below. The paragraph
that follows is the original, superseded accounting.**

Step 6.1.f closes 6.1 with acceptance accounting rather than new games.
Non-regression is exact rather than merely absent: KQ-K, KR-K and KBB-K at 100
positions each, seed 424242, produce byte-identical per-position records under
the accepted vector, the legacy vector and the rejected 6.1.c winner — KQ-K
98/100 at 99.91% preservation, KR-K 97/100 at 99.93%, KBB-K 99/100 at 99.93%.
`g_kbnk_drive` feeds only `kbnk_score`, which the dispatcher reaches only for
bishop and knight against a bare king, so those families cannot see the change;
the run confirms that rather than assuming it. WAC at depth 12 is likewise
identical across all three vectors at 244/300 and 83,444,716 nodes with the
same failure list.

The mate-band bound was checked against the search rather than re-read from the
validator. Every mate comparison in the engine — TT store and probe adjustment,
null-move verification, razoring, futility, singular extension and root
reporting — uses `MATE_SCORE - MAX_PLY`, with no lower threshold anywhere, so
32000 - 128 = 31872 is exactly `STATIC_MATE_FLOOR`. The accepted vector's
largest legal static score is 31,660, leaving 212 points of headroom, and a new
test pins the boundary behaviourally: `15600,1900,0,495,0` reaches 31,870 and is
accepted while `15600,1900,0,496,0` reaches 31,876 and is rejected, so a future
change to `MATE_SCORE` or `MAX_PLY` fires a test instead of silently letting an
evaluation be stored as a mate score.

Bench accounting: `bench 13` is 12,709,666 nodes at geomean EBF 2.876, identical
to the pre-6.1 head, and it proves nothing here because the bench suite contains
no KBNK position. The behavioural evidence is BAS-E41's held-out cohort. Full
CTest 12/12, test_eval 112/112, test_endgames 47/47, and the release default
plays 1.Nb3 on `KBNK0061`. Recorded as BAS-E42. Conversion on a tablebase cohort
is still not Elo; 6.2 owns that.

One residual defect is documented and deliberately not repaired inside 6.1. The
accepted vector sets edge and knight weights to zero, so no term in
`kbnk_score` refers to the knight and all knight moves score alike, which is the
flat-maximum pathology BAS-E36 named for the bishop. It cost the rejected
candidate a clean win on `KBNK0061`. The accepted vector happens to break that
tie safely on all 198 rows, but nothing makes it do so. A knight-referencing
term is a new mechanism needing its own registration, and 6.1.d closed the
bishop analogue on evidence, so it does not belong to this step.


**6.1.f correction (2026-09-04, BAS-E51).** The accounting above tested KQ-K,
KR-K and KBB-K and argued they were unreachable because `g_kbnk_drive` feeds
only `kbnk_score`. The argument is correct, and that is why the test was
worthless: those families were chosen *because* they are provably unreachable,
so passing confirmed only that the reasoning was self-consistent.

Redone without pre-selection -- one binary, the frozen 770-position cohort, all
21 families, only the UCI vector changed. **Exactly five families' records
differ: KBN-K, KBP-K, KBP-KB, KBP-KN and KBPP-KB.** Every one holds a bishop
and a promotable pawn, or is KBN-K itself; that is the knight-promotion
reachability closure, the only route by which a KBN-K node can appear in a tree
whose root has no knight. The other sixteen, including every pawnless family
and KP-K, KPP-K and KP-KP, are byte-identical, which is the part of the
original claim that survives.

| budget | KBN-K | KBP-K | KBP-KB | KBP-KN | KBPP-KB | net |
|---|---|---|---|---|---|---|
| 60,000 | 12 to 20 | 9 to 17 | 16 to 15 | 17 to 18 | 14 to 14 | +16 |
| 200,000 | 11 to 23 | 19 to 21 | 21 to 22 | 19 to 20 | 18 to 17 | +15 |

Every regression is `ply_limit` with a null `first_discard_ply` -- `EG0573`,
KBP-KB at DTZ 21, is mated on ply 53 by legacy and unfinished inside the
100-ply cap by the accepted vector. The engine never left a won position in any
of them, so these are conversion-speed regressions, not correctness ones.

Truth is equal or better at both budgets: live discards before ply 80 are
identical position-for-position at 60,000, and at 200,000 the accepted vector
has one fewer in KBPP-KB. KBN-K win-preservation rises 0.9955 to 0.9987 at
60,000 and 0.9935 to 1.0000 at 200,000, where it converts 23/24 against the
legacy 11/24 with twelve gained and none lost.

The step is accepted with the leakage documented rather than denied. The
original assertion that the change is confined to KBN-K is false; the corrected
claim is narrower and testable, and it holds.

**Method rule this establishes:** a non-regression set chosen by the same
argument that predicts safety cannot test that argument. Choose the set the
mechanism could plausibly reach, or test everything and let the data name the
affected families.

### 6.2 Gate endgame Group A

- [x] **6.2** Gate KBNK and accepted mate-drive changes.
  - [x] **6.2.a** Run a fresh no-adjudication [0,3] nElo SPRT against the accepted head.
  - [x] **6.2.b** Treat the old approximately 5,860-game adjudicated Group A run as preliminary only.
  - [x] **6.2.c** Never resume that run if the KBNK candidate or match policy changes.

Step 6.2 expectation, revised on evidence before the run (BAS-E43). The 6.0.g
census was executed for the first time at depth 13 on `suite_v1.epd`. Its
headline number is an artifact and must not be quoted: 21 of that suite's 107
roots are already seven men or fewer, and those roots are themselves bare-king
endgames, so they generated essentially every KBNK evaluation. Restricted to
the 86 roots with eight or more men, KBNK occurs **zero times** in 30.5M nodes
and 14.1M evaluations, and the whole <=7-man band is 0.283% of evaluations.

KBN-K therefore cannot reach the score sheet through search-tree influence; it
pays only when the position is actually reached on the board, which is rare.
**6.2 is a non-regression gate, not a strength gate.** The pre-registered
expectation is that a [0,3] nElo test accepts H0, and H0 acceptance here means
"no measurable gain", not "the change is harmful"; it must not be read as a
rejection of 6.1, whose evidence is BAS-E41's held-out truth cohort. Retention
is licensed by that cohort plus the absence of a strength regression, so if the
[0,3] run accepts H0 the follow-up that matters is a non-regression bound, not
another attempt at a gain.

The maintainer approved the bounds change on 2026-09-03, so 6.2.a's registered
primary is now the [-5,0] nElo non-regression test and the [0,3] gainer is
superseded. The gainer form remains available as `-Gainer` for the record, but
accepting H0 on it would license nothing: it asks a question the occurrence
census already answered.

A deterministic anchor for the 6.1.e failure was added to `test_endgames` at
the same time. It replays `KBNK0061` under the cohort's exact conditions and
asserts only that the strong side keeps both minors. Its first form used a
fresh table per move and a depth cap, and it passed under the rejected vector
too -- guarding nothing. Reproducing the cohort's conditions (60,000 nodes, no
depth cap, one table persisting across the game) makes it fail under
`15600,1750,0,340,0` and pass under the accepted vector, so it is a real guard
rather than a decorative one.

Step 6.2.a is prepared as a single-variable A/B that isolates 6.1. The baseline
is commit `6accfe6` (Complete 6.1.b), the last revision whose KBNK scoring is
the pre-6.1 one -- 6.1.b was proven score- and bench-identical, so everything
outside `kbnk_score` matches the candidate exactly. That deliberately differs
from the superseded Group A run's `5.9.18-base`: 6.2.b already declares that
approximately 5,860-game adjudicated run preliminary, and re-running a stale
multi-change comparison would not isolate what 6.2 gates. Adjudication is off,
as the default requires.

Harness note found during preparation: `tools/sprt.ps1` and
`tools/build_test.ps1` are UTF-8 without a BOM and contain non-ASCII em-dashes,
which Windows PowerShell 5.1 mis-decodes into unterminated string literals, so
both fail to parse there. They run correctly under `pwsh` 7. Every prepared
command therefore names `pwsh` explicitly.


Step 6.2.a outcome (BAS-E44). The gate ran as prepared -- no adjudication,
tc=3+0.03, 1 thread, 64 MB, concurrency 14, both benches 12,709,666 -- and the
maintainer stopped it at 7,720 games with LLR -0.05, that is 1.7% of the way to
a bound. Elo -1.40 +/- 4.07, nElo -2.66 +/- 7.75, LOS 25.09%, PairsRatio 0.97,
Ptnml [88, 736, 2236, 719, 81].

The claim this licenses is deliberately narrow: the candidate is not
distinguishable from the pre-6.1 baseline in play, and a regression worse than
about -5.5 Elo is excluded at 95%. It is practical equivalence, not a
demonstrated null and not a pass of the registered bound; the interval still
admits a real two-to-three Elo regression. The item is closed on the
maintainer's decision to stop, and reopening it costs only machine time if that
judgement is revisited.

An earlier interim reading at 3,484 games showed -4.29 Elo with LLR -0.82 and
was treated as a possible regression. It was noise and returned to the origin.
The standing lesson is that an SPRT a quarter of the way to its bound is not
evidence, and it should not have been discussed as though it were.

The PGO speed confound was measured rather than assumed, and does not flatter
the candidate. A first measurement was discarded because a maintainer job was
running concurrently; re-measured idle with five interleaved `bench 13` repeats
per side, the candidate averages 3,604,125 nps against 3,565,130, or +1.09%
(t = +1.70, inside a 1.6-3.0% within-engine spread). The candidate is if
anything faster, so speed cannot manufacture a negative result; if that edge is
real it is worth one to two Elo here, which would put the behavioural effect
nearer -3 than -1.4.

This is the outcome BAS-E43 predicted. A KBNK-only change has almost no surface
on which to move a 3+0.03 result, and 6.1's value stays where BAS-E41 measured
it -- conversion 95 to 144 of 198 when the family is actually reached.

Two follow-ups are prepared and unrun, both needing an idle machine.
`tools/diag/run_kbnk_budget_transfer.ps1` asks whether the 6.1.e ranking
survives a game-representative node budget, bracketing it at 200k and 600k
against the existing 60k result. `tools/diag/datagen_label_audit.py` asks
whether 8,000-node datagen mislabels won endings as draws, which would bias
Phase 8's refit exactly where 6.1 works; because `armA` at 8,000 nodes and
`armC` at 25,000 nodes already exist, that is a measurement rather than a
guess. A 60-game smoke read 25.9% of clean wins unconverted, well above the
6.0.f 14.12% baseline, but n=60 settles nothing.

Step 6.2.b records why the earlier Group A run is preliminary only. That run,
`5.9.23-groupA` against `5.9.18-base` on 2026-09-01, stopped at 5,840 games
with Elo -0.59 +/- 4.88, nElo -1.09 +/- 8.91, LLR +0.34 -- 11.6% of the way to
a bound. Three separate reasons bar it from being cited as the Group A verdict,
and only the third is about its statistics.

First, **it was adjudicated**: `draw(mn=40,mc=8,score=10)` and
`resign(mc=3,score=600,twosided)`. Score-based adjudication is off by default in
this project precisely because it substitutes the engine's own evaluation for
play, and its log shows games ending "by adjudication" throughout. For a change
that alters endgame evaluation, adjudication is not merely noisy, it is
circular: the candidate's own scores help decide the games that are supposed to
judge the candidate.

Second, **its candidate no longer exists**. It tested the 5.9.23 Group A bundle
against a 5.9.18 base, whereas the accepted KBNK vector was chosen in 6.1.c,
revoked, re-chosen, rejected on held-out data and finally settled at
`15600,1900,0,460,0` in 6.1.e. 6.2.c already forbids resuming that run for
exactly this reason, and its baseline differs too.

Third, it never reached a bound, so even on its own terms it decided nothing.

Its one legitimate use is as weak corroboration that the Group A direction was
not catastrophic. BAS-E44 supersedes it on every count: no adjudication, a
single-variable A/B against `6accfe6`, and a candidate that is the shipped one.

**Correction to 6.1.e, pending the maintainer's decision (BAS-E45).** The
budget-transfer run shows that 6.1.e's rejection of `15600,1750,0,340,0` was an
artifact of its 60,000-node budget. At 200,000 and 600,000 nodes that arm has no
live truth discard: 2.Nc2 on `KBNK0061` is a two-ply tactic the search sees once
it has a game-representative budget. The verdict rule was applied correctly to
the data it had, but the data came from a budget the engine never plays at.

The shipped vector still needs no change, and that is luck rather than
vindication. The two candidates are indistinguishable at every budget (116
against 119 at 200k, 133 against 132 at 600k), so `15600,1900,0,460,0` remains
defensible on the secondary criteria it won at 60,000. What is invalidated is
the justification, not the choice.

6.1.e is therefore NOT reopened, and the reason is substantive rather than
procedural. Reopening would mean re-deciding the held-out comparison at a
game-representative budget -- but that comparison has now been run, at both
200,000 and 600,000 nodes, and it returns the same answer: the two vectors are
statistically indistinguishable and `15600,1900,0,460,0` remains at least as
good on every secondary criterion. A reopened leaf would consume machine time
to arrive at the vector already shipped. The correction stands recorded here
and in BAS-E45, which is where it does its work.

The `KBNK0061` anchor added in 6.1.f is consequently a low-budget canary. It
still fires under the rejected vector and still guards the historical failure,
but it is not evidence of game-level safety, since at game budgets neither
vector fails it.

What the run does establish, and this is the stronger half: the diagonal
dominance mechanism transfers across a 10x budget span, beating legacy at paired
z +4.13 to +5.00 everywhere. At 600,000 nodes legacy converts 109/138 against
the candidates' 133 and 132, with fifty-move draws 22 against 4 and 5, so 6.1's
gain is real technique rather than compensation for a shallow search.

Registered consequence for future work: node budget is a first-class run
condition. Any fixed-node endgame screen must state its budget, justify it
against the deployment time control, and treat a result that only appears at a
low budget as provisional until it is shown to survive a game-representative
one.

Step 6.2.c makes the non-resumption rule explicit and states its trigger, so
that a future agent finding 5,840 games of apparently usable data cannot treat
them as a head start.

**The rule.** The `5.9.23-groupA` run is closed permanently. It must never be
resumed, extended, pooled with a later run, or cited as the Group A verdict.
Its log and PGN stay on disk as a historical record only.

**Its triggers, all three already fired.** Any one of them is sufficient on its
own. The KBNK candidate changed -- repeatedly, ending at `15600,1900,0,460,0`,
which that run never played. The match policy changed: that run adjudicated
with `draw(mn=40,mc=8,score=10)` and `resign(mc=3,score=600,twosided)`, and
6.2.a required no adjudication. The baseline changed, from `5.9.18-base` to
`6accfe6`.

**Why resumption is not merely stale but invalid.** Games from two different
match policies cannot be pooled: pooled Elo assumes one sampling process, and
adjudicated and natural terminations are different processes with different
draw rates, so the combined estimate would be biased by the mixing ratio rather
than by the engines. Resuming an SPRT after its candidate changed is worse
still, because the likelihood ratio accumulated under the old candidate is
carried into a test of a different one. And for an endgame-evaluation change
specifically, score adjudication is circular: the candidate's own evaluation
helps decide the games meant to judge that evaluation.

**The general rule this instantiates**, for any future gate: an SPRT is
invalidated by a change to either engine binary, either engine's options, the
book, the time control, the adjudication policy, or the hardware conditions
under which it is scored. When any of those move, the run is finished and a new
experiment ID begins. Partial results from the old run are diagnostics at best
and are never evidence for the new candidate.

With 6.2.a, 6.2.b and 6.2.c closed, step 6.2 is complete. The Group A gate
stands as BAS-E44: no adjudication, single-variable against `6accfe6`, stopped
by the maintainer at 7,720 games as practical equivalence, with a regression
worse than about -5.5 Elo excluded at 95% and no claim of a gain.

### 6.3 Passed-pawn king approach

- [x] **6.3** Add general king-to-passed-pawn approach logic.
  - [x] **6.3.a** Derive the feature from Basilisk truth failures, not reference constants.
  - [x] **6.3.b** Verify KP-K, KPP-K, KBP-K and mixed rook/minor pawn families.
  - [x] **6.3.c** Gate the isolated candidate with no adjudication.

**6.3.a is BLOCKED by an instrument defect, found while gathering its
evidence.** The leaf requires the feature to be derived from measured Basilisk
truth failures. The measurement is not currently trustworthy for pawn families,
so no derivation is written and the checklist item stays open.

`endgame_truth.py` ends a game as `material_lost` whenever the strong side's
piece count falls below its starting value:

    def strong_material(b): return chess.popcount(b.occupied_co[chess.WHITE])
    ...
    if strong_material(board) < initial_material: outcome = "material_lost"

That is a sound proxy for "threw the win" in KBN-K, where losing either minor
converts the win to a dead draw. It is false wherever the strong side can
correctly shed material -- which is most pawn technique, because sacrificing
one pawn to promote another, or trading into a won king-and-pawn ending, is the
winning method rather than a mistake.

Measured on the frozen 6.0.a cohort at 200,000 nodes over ten pawn families,
216 clean-win roots produced 148 `material_lost` aborts, and **139 of those
occurred without the engine ever playing a single non-win-preserving move**.
Example `EG0171`, KPP-K at DTZ 3: `8/8/8/3K4/8/5Pk1/6P1/8 w`, aborted at ply 2
because Black captured a pawn the winning line gives up. Only 9 of the 148 came
after a genuinely losing move.

The reported conversion rates are therefore artifacts. KPP-K reading 12/24 with
100% win preservation is the signature: the engine never erred, it was stopped.

*Scope of the damage.* KBN-K is unaffected, because there any minor loss really
does end the win, so 6.1's results and BAS-E39/E41/E45 stand. The families at
risk are exactly those where the strong side holds more than one unit or a
trade is part of the technique: KPP-K, KBP-KB, KBP-KN, KBPP-KB, KRP-KR, KP-KP,
KBP-K and KQ-KP. Any earlier baseline that scored those families through this
instrument -- 6.0.b in particular -- needs re-checking before its numbers are
reused.

*Proposed fix, not applied.* The tablebase already knows the answer the
heuristic is guessing at. Terminate on truth rather than on material: keep
playing unless the position's WDL for the strong side drops below a clean win,
which the harness already tracks for `first_discard_ply`. `material_lost` then
becomes a recorded reason rather than a stopping rule. This changes the meaning
of every family baseline the instrument has produced, so it is a maintainer
decision, not a silent repair.

`tools/diag/passer_king_geometry.py` is committed with this note. It tests
whether king-to-passer geometry actually separates converted from unconverted
roots, which is what 6.3.a needs in order to derive rather than assume, and it
is ready to re-run once the instrument is trustworthy.

Step 6.3.a is complete with a **negative** derivation, frozen as
`analysis/passed_pawn_king_approach_v1.md` and recorded as BAS-E48. The blocker
reported above is resolved: the instrument was fixed, 6.3.a's evidence
re-measured, and 6.0.b re-run.

The leaf demanded the feature be derived from Basilisk's failures rather than
imported. Measured properly, those failures carry no king-approach signature.
Aggregated over 288 clean-win roots at 60,000 nodes there is a weak trend --
82.2% conversion at king distance 0-1 down to 56.8% at distance 5 -- but it is
non-monotone and recovers to 70.0% at 6-7. Conditioning on family destroys it:
the within-family delta between a closer and a further strong king runs +25.0,
+20.0, +9.2, +9.1, 0.0, -3.5, -7.1, -11.9, -27.1 and -35.7 percentage points,
four families one way and six the other, mean -2.2pp. The aggregate trend was
family composition. At 200,000 nodes it is absent even in aggregate.

The deficit itself is real and material-specific: KBP-K 15 positions behind the
reference, KNN-KP 13, KQ-KR 13, KBN-K 12 against the pre-6.1 head, KQ-KRP 10,
KBPP-KB 9. Those are now distributed across 6.8.b/6.8.c and 6.10.b/6.10.c. A
general king-approach term
touches none of them, and adding one anyway would import a reference constant.

Consequently 6.3.b and 6.3.c have no candidate to verify or gate. They are left
open rather than ticked, because closing them is a scope decision for the
maintainer: either 6.3 closes empty in the manner of 5.9.21, or it is re-scoped
around the family-specific gaps this measurement actually found.

**6.3 closes empty (maintainer decision, 2026-09-03), in the manner of
5.9.21.** 6.3.a's derivation found no king-approach signature that survives
conditioning on family, so there is no candidate for 6.3.b to verify or 6.3.c
to gate. Both are marked complete as vacuous rather than left dangling, and no
king-to-passed-pawn term is implemented or licensed.

The one large addressable gap the measurement found, **KBP-K**, is absorbed by
6.8.c, which owns bishop-pawn families including wrong-bishop and
rook-pawn draw logic. Its size at the current head is 7 positions (17/24
against the reference's 24/24); the 15 quoted from the pre-6.1 arm was stale,
because the 6.1 KBNK vector unexpectedly moved KBP-K from 9/24 to 17/24
(BAS-E49). It must be attacked as bishop-pawn technique, not king geometry --
6.3.a's within-family evidence specifically rules out the latter as the cause.

Closing empty costs nothing that was ever demonstrated. Nothing in 6.3 was ever
supported by Basilisk's own evidence; the step existed because strong engines
have such a term, which is the reasoning 6.3.a was written to prevent.

**Current-head baseline (BAS-E49).** The 6.0.b arms measure `294a3e2`, which
predates 6.1. Re-measured on the same cohort and conditions, the current head
converts **377/480 (78.54%)** against the reference's 466/480 (97.08%), closing
the gap from 105 positions to 89. 6.1 is worth +16 there: KBN-K 12 to 20,
KBP-K 9 to 17, KBP-KN +1, KBP-KB -1.

The KBP-K movement was not intended and is not a build artifact: on one binary
with only the UCI vector changed, the legacy weights give 9/24 and the accepted
weights 17/24, and the old PGO binary independently reproduces 9/24. Since
KBP-K contains no knight and `kbnk_score` fires only on bishop-plus-knight
against a bare king, every KBN-K node in a KBP-K tree must come from a knight
promotion, and the drive's larger magnitude reprices those lines. **6.4.a
inherits this as its first concrete case**: an evaluation term with a
31,660-point ceiling is reaching families it was never scoped to touch.

Families level with the reference and needing no further work: KQ-K, KR-K,
KBB-K, KP-K, KPP-K, KQ-KP, KP-KP. Remaining deficits at the current head are
KQ-KR 13, KNN-KP 13, KQ-KRP 10, KBPP-KB 9, KBP-KB 8, KRP-KR 7, KBP-K 7,
KBP-KN 6, KR-KN 4, KRP-KB 4, KBN-K 4, KR-KB 3, KR-KP 1.

## Reopened work, 2026-09-03

Two completed leaves were invalidated by the endgame-instrument defect found
while gathering 6.3.a's evidence. **Both were reworked and closed -- 6.0.c on
2026-09-03 and 6.1.f on 2026-09-04 -- so no checklist item carries the
`(REOPENED)` marker today and `check_roadmap.py` reports none.** The section is
kept for the marker's definition and the record of what was redone:
`check_roadmap.py` understands the marker, and a reopened
leaf is exempt from the sequential-ordering rule, because a measurement defect
can invalidate an early step after later ones have closed, but it is never
exempt from being open, and an accidental un-tick still fails the check.

**6.0.c - DONE 2026-09-03 (BAS-E50), left here for the record.** The file
`tools/diag/endgame_ceilings_v1.json` records the reference arm's attained 60k
conversion, and that arm aborted correct pawn play. Seven families are wrong,
by 77 positions in aggregate: KBPP-KB 6 to 23, KRP-KR 9 to 24, KRP-KB 9 to 24,
KPP-K 12 to 24, KBP-KN 15 to 24, KBP-KB 16 to 23, KBP-K 22 to 24. These
ceilings are used downstream as acceptance targets, so leaving them stands is
worse than having none: they are far too lenient in exactly the families 6.8
and 6.10 may implement. Redo from
`tools/results/endgame-truth-6.0.b-refixed/`, which
already contains both corrected arms; no new games are required.

**6.1.f - DONE 2026-09-04 (BAS-E51), left here for the record.** It verified isolation on
KQ-K, KR-K and KBB-K, and argued they could not be affected because
`g_kbnk_drive` feeds only `kbnk_score`, which the dispatcher reaches only for
bishop-plus-knight against a bare king. That reasoning is sound for those three
families, which is exactly why testing them proves nothing: they were chosen
because they are provably unreachable. BAS-E49 then showed KBP-K moving 9/24 to
17/24 and KBP-KB 16/24 to 15/24 on the same change, through knight-promotion
lines the argument did not consider. The step must re-run its non-regression
accounting over families that CAN reach `kbnk_score` through promotion --
bishop-pawn and pawn families -- and record the small KBP-KB regression rather
than omit it.

Deliberately NOT reopened, with reasons, so tomorrow does not relitigate them:

- **6.0.b** has already been re-measured under its own registered conditions;
  the corrected figures and a superseded-notice sit with the original text.
- **6.0.d and 6.0.e** define comparison and veto RULES, not data, and both were
  **re-verified on 2026-09-04 against the corrected arms** rather than left as
  an assumption. `endgame_vetoes.py` keys on `first_discard_ply`, never on
  `material_lost`, and the recorded artifacts are outputs rather than inputs, so
  nothing consumed them. Regenerated from
  `tools/results/endgame-truth-6.0.b-refixed/`, the accepted head still passes
  itself with zero hard vetoes and zero confidence movement, which is exactly
  6.0.e's registered self-check. The reference arm still trips three
  position-level vetoes against the accepted baseline (KNN-KP `EG0459`,
  `EG0460` clean-win discards and `EG0464` rule-50), which is the intended
  independence check rather than a failure: 6.0.e states that the stronger
  reference is not an acceptance target. Neither leaf needs reopening.
- **6.0.f** censused datagen labels against Syzygy on the Arm C corpus and never
  used the playout instrument, so it is independent of this defect.
- **6.1.e** is corrected but not reopened; re-deciding it at a
  game-representative budget has already been done in BAS-E45 and returns the
  same vector.
- **6.2.a** was stopped undecided by maintainer decision, which is recorded as
  such rather than as a pass.

### 6.4 Magnitude and coverage audit

- [x] **6.4** Audit every endgame term before broadening the evaluator.
  - [x] **6.4.a** Test score resolution, saturation and interaction at Basilisk's scale.
  - [x] **6.4.b** Keep theory truth, move quality, conversion and game strength separate.
  - [x] **6.4.c** Freeze the accepted Group A head and truth report. Includes the 6.4.a carry-over that has no other owner: lift `STATIC_MATE_FLOOR` to file scope and `static_assert` the compiled `g_kbnk_drive` default against it, so the mate-band guarantee holds in release builds too. Today it is enforced only by `set_kbnk_drive_weights`, which is `#ifdef BASILISK_TUNE`, so the shipped constant is unchecked (BAS-E52).

Step 6.4.a is complete. The static audit is frozen as
`analysis/endgame_magnitude_audit_v1.md` and the measurement it registered has
run (BAS-E52); the result **refutes the audit's own hypothesis**, which is
recorded below rather than quietly dropped.

**Saturation.** No term collides with the 31,872 mate-band floor.
`kxk_score` tops out near 17,320 even with promotion-added bishops, leaving
14,552 points of headroom and needing no guard. `kbnk_score` sits 212 points
below the floor, and the check that keeps it there lives in
`set_kbnk_drive_weights`, which is `#ifdef BASILISK_TUNE`: **a release build's
compiled default is validated by nothing**, and 6.1.f's test exercises the
validator rather than the constant. Recommended and not applied: lift
`STATIC_MATE_FLOOR` to file scope and `static_assert` the compiled default, so
the guarantee holds at compile time in both build types.

**Resolution, and this is the substantive finding.** `kxk_score`'s comment
claims every weight clears the 243 razoring margin so the walk survives
pruning. That holds for the raw weights and fails for the scores the search
sees, because `damp_rule50` multiplies by `(199 - clock) / 199` after
`apply_endgame` and therefore scales the override band too. The kxk king step
(300) is already below the margin at clock 50, at 224, and the corner step
(250) at 187; the kbnk king step (460) falls below at clock 99, at 231.
Neither component is wrong on its own -- damping toward zero is deliberate, and
the weights were chosen against the undamped margin -- but the interaction was
never audited, and it means the gradient a mate drive depends on is weakest
exactly when conversion is most urgent. Ordering inside the class survives,
since damping is monotone; the pruning margin does not.

**Interaction.** BAS-E51's promotion leakage generalises. A family term's blast
radius is not its dispatcher condition but that condition's promotion closure:
KBP-K promoting to a knight reaches `kbnk_score`, and promoting to a bishop
reaches `kxk_score`, which was never checked. Every new family term in 6.8 and
6.10 must state which families reach it by promotion and include them in its
non-regression set.

**Result (BAS-E52): the hypothesis is refuted backwards.** Three arms at
starting clocks 0, 25 and 50 -- 80 was abandoned because it leaves KBN-K one
eligible root out of 24 -- gave DTZ progress falling 0.5962 to 0.5946 for the
kxk families, a drop of 0.0016, against 0.6357 to 0.4900 for KBN-K. The
families whose steps cross the 243 margin show no measurable move-quality
degradation; the family whose gradient never approaches it degrades most. The
pruning-margin mechanism does not bind in practice, and `damp_rule50` is left
alone.

Two cautions on how far to read that. DTZ progress is per-move, so each family
is its own control across clocks and that comparison is clean. The conversion
row is **not**: the eligibility cut equalises feasibility but not slack -- at
budget 50 a KQ-K root with median DTZ 10 has 40 halfmoves spare while KBN-K's
median 50 has none -- so cross-family conversion gaps are mostly slack, and a
slack-matched comparison is impossible here because the DTZ distributions
barely overlap. The leading explanation for KBN-K's within-family collapse is
BAS-E35's indifference effect, the evaluation being damped toward zero until
the engine stops distinguishing winning continuations, which is the curve
working as designed.

The audit's other finding is untouched by this and still stands: the release
build's compiled KBNK default is validated by nothing, and a file-scope
`static_assert` remains the recommended fix.

Step 6.4.b is frozen as `analysis/endgame_measurement_layers_v1.md`. It is a
documentation leaf, and Phase 6 happened to supply a case where each pair of
these layers disagreed, so the rules are anchored in evidence rather than in
taste.

**The four layers.** Theory truth asks whether a won position was thrown, per
move, from Syzygy WDL and `first_discard_ply`. Move quality asks whether the
moves progress, per move, from DTZ progress and win-preservation. Conversion
asks whether the win was finished inside the rules, per position, from the
cohort outcome. Game strength asks whether it wins more games, per game pair,
from an SPRT at a real time control. **Occurrence** is not a layer but gates
whether the first three can ever reach the fourth.

**Precedence, with the case that forced each rule.** Theory truth is an
absolute veto: 6.1.e rejected the arm converting 103/138 in favour of one
converting 98/138, on a single live discard (BAS-E41). Conversion never
establishes strength: 6.1 moved conversion 95 to 144 of 198 and the SPRT
returned -1.40 +/- 4.07 Elo (BAS-E44), because the family occurs zero times in
trees from real roots (BAS-E43). Move quality and conversion can move in
opposite directions: at 200,000 nodes the accepted vector converts one fewer
KBPP-KB root while carrying one fewer live truth discard (BAS-E51). And
strength never overrides truth -- stated now, though Phase 6 never tested that
direction, so it is not discovered by argument later.

**Three confounds, each found the hard way.** Conversion is confounded by DTZ
slack across families, because an eligibility cut equalises feasibility and not
margin, and on this cohort slack cannot be matched since the distributions
barely overlap (BAS-E52). Conversion is confounded by node budget: 6.1.e's
verdict does not reproduce at 200,000 or 600,000 nodes (BAS-E45). And truth
itself can be faked by an instrument that conflates losing material with losing
the win, which is what aborted 139 of 148 pawn games before any bad move was
played (BAS-E47) -- a layer-3 number produced by a layer-1 category error is
not a weak measurement but a different one wearing the wrong label.

**Reporting rules now binding on 6.5 onward.** State layer, instrument, node
budget and position set for every number; a bare conversion percentage is not a
result. Never aggregate layers into one score, because there is no exchange rate
between a truth failure and a conversion gain. When two layers disagree, say so
and say which decides -- that disagreement is usually the finding. Bench
identity belongs to no layer: it is a provenance fingerprint, and it was blind
to the whole of 6.1 by construction because the bench suite contains no KBNK
position.

Step 6.4.c closes 6.4 and Group A. It does two things.

**The shipped constant is now guarded at compile time, in both build types.**
6.4.a found the mate-band bound enforced only inside `set_kbnk_drive_weights`,
which is `#ifdef BASILISK_TUNE`, so a release build's compiled default was
checked by nothing and 6.1.f's test exercised the validator rather than the
constant. `KBNK_STATIC_MATE_FLOOR` now lives in `eval.h`, one `constexpr`
`KBNK_DRIVE_DEFAULT` serves both builds, and a `static_assert` rejects any
default whose largest legal score reaches the floor. Because `eval.h`
deliberately does not depend on `search.h`, the two definitions could still
drift, so `tests/test_endgames.cpp` -- which legitimately includes both --
carries a second `static_assert` tying `KBNK_STATIC_MATE_FLOOR` to
`MATE_SCORE - MAX_PLY`. Both were verified to FIRE, not merely written: a
default of king 496 (maximum 31,876) fails the build with the intended message,
and moving the floor to 31,800 fails the cross-header assert. The change is
compile-time only, and the 770-position truth report is byte-identical at
377/480 before and after it.

**The accepted head and its truth report are frozen** as
`tools/diag/group_a_head_v1.json`, generated by
`tools/diag/freeze_group_a_head.py` so it is reproducible rather than
hand-made. It is content-addressed, not commit-addressed: a commit SHA would
name the revision that wrote the freeze rather than the state being frozen, and
would go stale whenever documentation moved without the engine changing. It
records the accepted vector `15600,1900,0,460,0`, bench 12,709,666, both binary
hashes, the truth report's hash and conditions, per-family conversion,
win-preservation and DTZ progress, and the corrected v2 ceilings.

The artifact states its own limits, so a later reader cannot over-read it: it
licenses a fixed baseline for the 6.5 candidate and the binary identity the
6.9 Group B gate must start from. It licenses **no Elo claim**, because 6.2.a stopped
undecided at practical equivalence; **no further SELECTION on the 198-position
KBNK cohort**, which is spent; and **no cross-family conversion comparison
without matching DTZ slack** (BAS-E52).

Group A therefore closes measured, gated, audited and frozen -- with its
strength contribution honestly recorded as not demonstrated rather than
assumed.

### 6.5 Prepared rook-ending scale gate

- [x] **6.5** Finish the already-prepared rook-ending scale candidate.
  - [x] **6.5.a** `[Sol/H]` Analyze the registered no-adjudication SPRT for the truth-floored KRPKR/KRPPKRP draw-scaling candidate. **Accepted H1:** +3.29 +/- 4.61 Elo, 6,332 games, LLR 2.98 through the `[-5,+3]` upper bound.

**6.5.a implementation status (2026-09-07, BAS-E54).** KRPKR and KRPPKRP draw
scaling is implemented in `apply_endgame` and prepared for a gate; the leaf
stays open until the SPRT returns. Three findings worth carrying forward:

* **The gate is free but the rules are not.** A cheap bitboard gate benches
  identically to baseline. A faithful unfloored port of the reference's rules
  costs **+79% bench nodes for KRPKR and +108% for KRPPKRP**, and one rule --
  the loosest condition carrying the most aggressive scale, 10 of 64 -- is the
  entire KRPKR figure. Aggressive scale factors flatten the evaluation across
  the region they cover, and this engine's search pays for that in a way the
  reference's does not.
* **The rules are heuristics and must never assert a draw.** Against Syzygy on
  3,000 random KRPKR positions the unfloored draw rules call 18 genuinely WON
  positions dead draws. `SCALE_FLOOR = 24` takes that to zero. That correctness
  argument, not the bench curve, is why the floor exists: the floor sweep is
  chaotic (32 costs +28.5%, 24 and 40 are near baseline) and selecting on it
  would be fitting to noise.
* **What it buys is real but partial.** KRP-KR +0.302 -> +0.242 and KRPP-KRP
  +0.262 -> +0.231, at bench -1.1% and NPS +1.24%. That is roughly a fifth of
  one bias and an eighth of the other, so it is a down payment on 6.5.a rather
  than a fix, and it is far too small to assume Elo from.

**6.5.a completion (2026-09-07, BAS-E54).** The maintainer-returned
no-adjudication SPRT accepted H1 at **+3.29 +/- 4.61 Elo** and **+6.11 +/- 8.56
nElo**, LOS 91.91%, after 6,332 games (1,641 wins, 1,581 losses, 3,110 draws;
56.10% draw ratio; pentanomial `[67,606,1776,634,83]`). LLR reached **2.98**
against bounds `(-2.94, 2.94)` for the registered `[-5,+3]` test. Conditions:
`3+0.03`, 1 thread, Hash 64, paired `UHO_Lichess_4852_v1.epd`, tablebases off
and no score adjudication. The candidate is retained and the leaf is closed.
The returned paths named `_rookscale_vs_base_20260907_094516.pgn` and `.log`,
but those raw artifacts were not present under `D:/code` when closure was
performed; this limitation is recorded rather than silently treating the raw
files as re-verified.

**2026-09-07: historical 6.5.a was split into current 6.5.a and 6.8.a, with the maintainer's
sign-off.** BAS-E53 established that the two problems bundled here are
unrelated. The drawn-share bias is a scaling question, needs no tablebase
truth, and is implementable now -- it keeps 6.5.a. The only-move precision
defect is a knowledge question with no known mechanism and its own gate, so it
is now 6.8.a rather than blocking the scaling work behind open research.
Everything below this line was written before that split and is kept as the
record of how it was reached; where it says "6.5.a" of the precision defect,
read 6.8.a.

**Superseded 2026-09-07: 6.5.a is CLOSED and its candidate accepted.** The two
lines below say "no implementation yet". They were written before the split and
before the candidate existed, and are kept only so the reasoning that follows
still reads in order. Nothing from here to the end of this section describes
current status.

> **6.5.a is SPLIT: one half is derived, the other is not measurable.** No
> implementation yet; the leaf stays open.

**Correction (2026-09-04): the KRPP-KRP blocker below was my framing error, not
a real obstacle.** It assumed the family needs a tablebase-validated WIN
recognizer. It does not. In the reference taxonomy this roadmap is derived from,
`KRPPKRP` is a **scaling** function, not an evaluation function -- its job is to
recognise DRAWISHNESS, and its historical shape is "if either strong pawn is
passed, do not scale; otherwise, if the weak king is near the pawns, scale down
by pawn advancement". A draw-scaling heuristic is validated by drawn-share bias
and by game strength, neither of which needs seven-man truth.

The evidence for it is already in the repository and is game-outcome based.
BAS-E32 measured `KRPP-KRP` at **11.32% of games** with the class **61.9%
drawn** while Basilisk predicts **0.638**, a **+0.138** bias, and recorded the
conclusion directly: we systematically score drawn rook endings as won, and the
classes where we are wrong are precisely the ones the reference gives scaling
functions to. `KRP-KR` is the same story at 9.09% of games and +0.171. BAS-E43
agrees from the other direction: `KRPPKRP` occurs **352 times** in trees from
real non-endgame roots, where `KBNK` occurs **zero** times.

So 6.5.a needs no seven-man tablebase and no Lichess dependency. It needs a
draw-scaling mechanism, validated on the six-man cousins that ARE installed --
`KRPvKRP`, `KRPvKR`, `KRPPvKR` are all present locally -- plus the drawn-share
bias metric and an SPRT. The paragraph below remains as the record of the wrong
turn.

**Superseded framing: KRPP-KRP cannot be validated with the installed
tablebases.** It is seven men,
and `D:/chess/tablebases/syzygy3456` stops at six -- `KRPPvKRP.rtbw` is absent,
and the largest file in the set carries six pieces. 6.0.a already recorded this
when the cohort was frozen, and it is the reason the family has no cohort rows:
the frozen 770 positions cover twenty-one families and KRPP-KRP is not among
them. Nothing can be derived from its truth failures, nothing can gate on them,
and 6.6.a's "paired truth improvement" requirement is unsatisfiable for it. The
options are to install seven-man tables, which is a very large download, to
validate the family by a non-tablebase basis that 6.6 would have to accept, or
to drop it from 6.5 and say so. That is a scope decision, not something to
assume.

**KRP-KR is measurable and the failure mode is derived.** At the frozen head it
converts 17/24 against the reference's 24/24. The seven failures are not slow
technique: **six are live truth discards**, at plies 0, 0, 2, 8, 10 and 10, so
the engine leaves a won position outright rather than running out of clock.

Two hypotheses were tested and one survives. *Only-move precision is NOT the
explanation*: the engine converts five positions that have exactly one winning
root move, and fails `EG0497`, which has fourteen. Median winning root moves are
3.0 for converted and 2.0 for failed, which is not a separation.

Replaying each failure to the exact losing move gives the pattern. In
`EG0497`, `EG0501` and `EG0512` **the losing move is the passed-pawn push**,
and in every one of those the winning alternatives are rook or king
improvements -- `Kd6 Kd7 Ke7 Kf6 Kf7 Rd8 Rh7` instead of `h7`; `Rf4` instead of
`d4`; `Kg5 Rd1 Rd3 Rd4 Rd5` instead of `e5`. In `EG0496` the engine prefers a
check, `Rf7+`, where the quiet `Kd5` wins. The candidate mechanism is therefore
**premature passer advance in rook endings**: the pawn must be escorted, the
rook or king improved first, and the engine's general passed-pawn advancement
reward has no rook-ending brake.

That is a derived hypothesis with a named mechanism and six worked cases, not a
ported constant, so it satisfies the standard 6.3.a set. It is deliberately not
implemented yet: it is a playing change, it needs its own registered gate, and
committing to it while half the leaf is unmeasurable would fix the scope by
accident.

**KRPP-KRP is now measurable and it is the worst family yet seen.** The
KRPPvKRP table pair is installed in `D:\chess	ablebases\syzygy7` and
verified two ways: both files match the server's Content-Length, and five
positions spanning draws and wins at differing DTZ agree exactly with the
independent Lichess tablebase API on both WDL and DTZ. The frozen cohort is
`tools/diag/krppkrp_cohort_v1` (seed 0x6B1AC, 24 clean wins and 16 draws,
clean-win DTZ 3 to 19, book SHA-256 8DA2D418...).

First measurement at 60,000 nodes: **17/24 converted (70.8%)**, win-preservation
**0.9610** -- the worst of any family measured, against KRP-KR's 0.9876 and
KBN-K's 0.9987 -- and DTZ progress 0.3857. **Twelve of the 24 clean wins carry a
live truth discard**, five of them on the very first move. Separately,
`EG0039` (`8/8/5r2/3R4/4P3/8/p1k3KP/8 w`) is a theoretical **DRAW that Basilisk
LOST**, recorded as `wrong_mate` at ply 52; the graded-move machinery does not
count that as a discard because it only grades moves from clean wins, which is
exactly why the cohort carries 16 drawn rows.

**The KRP-KR mechanism does not transfer, and assuming it would have been the
error.** In KRP-KR the losing move was the passed-pawn push in three of six
cases. Across the five KRPP-KRP first-move throws the losing move is a pawn
move **zero** times out of five: every one is a rook or king placement error,
and every winning alternative is also a rook or king move. Several involve an
enemy pawn near promotion that the engine declines to stop -- in
`7R/6r1/P7/8/5P2/8/K1p3k1/8 w` the only winning move is `Kb2`, stopping the c2
pawn, and the engine plays `Rc8` instead. Two others require the rook to hold a
specific rank or file that the engine abandons.

That was a suggestive family of causes rather than one mechanism, and five
cases did not establish it. The characterisation below supersedes it and
corrects two claims made above.

**Characterisation with a control (`tools/diag/rook_ending_failure_profile.py`).**
Listing features of losing moves proves nothing, because anything common in the
family is common in its failures too. So every White-to-move node where the
tablebase still says WIN was classified as PRESERVED or THREW and each feature
compared across both groups. One feature separates them, and it separates
enormously:

| feature | KRP-KR preserved / threw | KRPP-KRP preserved / threw |
|---|---|---|
| **winning moves available** | **14.09 / 2.67** | **13.48 / 2.55** |
| engine chose a pawn move | 0.108 / 0.500 | 0.198 / 0.364 |
| enemy passer distance | n/a | 2.54 / 2.71 |
| king closes on enemy passer | 0.000 / 0.000 | 0.021 / 0.000 |
| rook to enemy passer's file | 0.000 / 0.000 | 0.039 / 0.182 |

**Correction 1.** The claim that the two families do not share a fix is wrong.
They share the dominant signature exactly: the engine holds the win while about
fourteen of roughly twenty legal moves preserve it, and throws it almost only
where about two and a half do.

**Correction 2.** The earlier finding that only-move precision is NOT the
explanation for KRP-KR is also wrong, and wrong for a specific reason worth
keeping: that test counted winning moves at the ROOT. The engine navigates
several correct moves and then errs at a narrow node, so root width is nearly
uninformative. Measured at the node where the engine actually chose, precision
is the dominant factor in both families. The enemy-passer hypothesis is not
supported; the pawn-move tendency is real but secondary, and is what reaching
for the natural move in a narrow position looks like.

**Superseded by BAS-E53 (2026-09-07): the impossibility claim below is false.**

**What this implies for the mechanism, and it is not an evaluation term.** A
static evaluation supplies a gradient over many moves; it cannot reliably pick
two exact moves out of twenty. That is the same shape of conclusion 6.3.a
reached, and it points away from the KRPKR/KRPPKRP scaling term as a fix for
THIS defect -- draw scaling addresses the separate drawn-share bias of BAS-E32,
which remains real and unaddressed.

**Scope question resolved by the maintainer's 2026-09-04 tournament: the
strength metric runs with TABLEBASES OFF.** `configure_engine` clears
`SyzygyPath` so the harness measures the evaluator rather than the tables, and
the natural worry was that this made the failure class an artifact. It does
not. The 12,000-game Colosseum run that defines current standing -- 3s+30ms,
round robin, two games per pair, no draw or resign adjudication,
UHO_Lichess_4852_v1 openings -- also has **tablebases off**. The harness is
therefore blind in the same way the games are, so every narrow-window rook
failure profiled above costs real measured Elo. The earlier hedge that this was
"largely an artifact" is withdrawn.

Two numbers from that run size the opportunity. **510 of 12,000 games (4.25%)
end by the fifty-move rule**, which is the game-level signature of exactly this
conversion failure -- though an unknown share of them are genuine theoretical
draws, so it bounds the prize rather than measuring it. And the overall drawn
share is 33.0%, the population in which BAS-E32's drawn-rook-ending bias lives.

This does not resurrect an evaluation term as the fix. The profile still says
the engine errs where about two of twenty moves win, which no gradient can
reliably resolve. What changes is that the defect is worth solving, and that a
search-side or knowledge-side answer must be judged against a TB-off metric.

**The narrow-window class IS resolvable, and depth is not the answer
(BAS-E53, `tools/diag/narrow_node_probe.py`).** The superseded paragraph above
asserted that no gradient can pick two exact moves out of twenty. It was never
measured, and it is false. All 48 clean wins of the two families were replayed
once by the frozen head to freeze **841 decision nodes** with their
winning-move counts; every arm then answered those SAME nodes with one move,
fresh TT per node, `SyzygyPath` cleared.

| arm | narrow (161) | mid (133) | wide (547) |
|---|---|---|---|
| basilisk@60k | **91.3%** | 98.5% | 99.6% |
| basilisk@300k | **92.5%** | 98.5% | 100.0% |
| basilisk@600k | **93.8%** | 99.2% | 100.0% |
| stockfish@60k | **99.4%** | 100.0% | 100.0% |

Paired on narrow nodes against basilisk@60k: **stockfish@60k 13-0, z = 3.61**;
basilisk@600k 6-2, z = 1.41. Three things follow, and they redirect the leaf.

**One: the class is resolvable at the deployment budget.** A strong evaluator
takes 160 of 161 narrow nodes on the same 60,000 nodes where we take 147.

**Two: it is not a search-volume problem.** Ten times the nodes buys 4 of 14
errors and does not approach stockfish@60k. This is NOT deferrable to Phase 9.

**Three: it is an only-move problem, and not a piece-selection problem.** At
`win_moves == 1` (68 nodes) we preserve 85%, at 600k 90%, Stockfish 100%; every
bucket at four or more winning moves is at ceiling for everyone. At the 13
discordant nodes our losing moves are 8 rook, 4 king, 1 pawn against Stockfish's
8 rook, 5 king -- **the right piece to the wrong square**. That kills premature
passer advance and any wrong-piece account outright.

**What it does not license.** Stockfish evaluates with an NNUE, so this removes
the impossibility argument without supplying a mechanism, and must not be read
as licensing an HCE rook coefficient. Thirteen discordant nodes are not a
mechanism either; that is the sample size that already misled this leaf once.

**The only-move question is now 6.8.a, separate from the prepared 6.5.a scale
gate.** It asks "what knowledge resolves this, and can this engine express it".
The failure lives in 68 only-move nodes -- small enough to study exhaustively,
too small to fit coefficients against without a held-out split. BAS-E32's
drawn-share bias remains the separate subject of the already-prepared 6.5.a
candidate. Analysis: `analysis/rook_narrow_node_resolvability_v1.md`.

- [ ] **6.6** Upgrade instrument and gate integrity before selecting another endgame candidate.
  - [ ] **6.6.a** `[R2]` Audit Basilisk's current tools against Rarog 4.10 at donor revision `881c821`; classify every delta as directly portable, adapter-specific, already covered or inapplicable. Do not change an instrument before recording which historical results depend on its semantics.
  - [ ] **6.6.b** `[I2]` Add a versioned truth-report schema, per-family FEN-sequence digests and an overall cohort identity; seed families by name, refuse cross-cohort comparisons, and add sharded workers whose ordered output is byte-identical to the serial reference. Port/adapt Rarog's `endgame_truth.py`, `endgame_floors.py` and lifecycle tests where that is cheaper than reimplementation.
  - [ ] **6.6.c** `[V]` Prove every guard fails on a known-bad input: material-shed/truth confusion, changed FEN cohort, thin denominator, broken regression anchor, serial/parallel drift and an engine that retains cross-position state. Stamp layer, budget, ply cap and cohort on every report.
  - [ ] **6.6.d** `[R2]` Measure Basilisk's deployed nodes per move through its real clock path, then justify a fixed-node bracket around deployment rather than inheriting Rarog's 60k/200k/600k values. Re-run after material time-management or NPS changes.
  - [ ] **6.6.e** `[I2]` Generalize held-out tooling: assign by FEN hash, prospectively freeze the deciding half and runner-up, use paired discordances/McNemar with a thin-sample refusal, report plateaus, and mark a selected cohort spent except as a safety veto.
  - [ ] **6.6.f** `[I2]` Harden SPRT/build provenance: refuse dirty or wrong-revision candidates by default, require binary/hash/compiler/flavor/bench manifests, reject options a selected mode cannot honor, record natural termination, and test each refusal live. Adapt the Rarog guards; retain Basilisk's C++/CMake manifest format.
  - [ ] **6.6.g** `[I2]` Add a supported CMake/feature/ISA matrix that covers combinations rather than only single options, distinguishes production from diagnostic builds, and verifies required instruction classes when behavior cannot reveal a missing optimization.
  - [ ] **6.6.h** `[V]` Re-run only the endgame baselines invalidated by 6.6, preserve old artifacts as superseded, derive new floors from matched cohorts, and freeze the accepted Group-A head under the repaired contract.

Rarog's exact tools may be copied because both projects are owned by the
maintainer, but their conclusions do not transfer automatically. Rust feature
matrices, UCI invocation shape, report fields and process lifetime need Basilisk
adapters. The required behavior is the contract above, not source identity.

### 6.7 Local endgame ranking and family contracts

- [ ] **6.7** Rank and classify remaining work from Basilisk evidence.
  - [ ] **6.7.a** `[R3]` Recompute board occurrence over a large current rated corpus and search-tree occurrence over a root-balanced suite. Report per-root concentration, board versus tree rates and uncertainty; occurrence prioritizes work but never proves value.
  - [ ] **6.7.b** `[R3]` Classify every remaining family as exact recognizer, scale/drawn-overclaim, move-quality or conversion/guidance work, and pre-register the deciding instrument. A scale that leaves won-position conversion unchanged has not failed its intended test.
  - [ ] **6.7.c** `[R3]` For every dispatcher, record its exact runtime condition, direct root-material matches and full promotion/material-shed closure. Test all families when the closure cannot be proved. Report paired gains and losses, not only net conversion.
  - [ ] **6.7.d** `[R2]` Re-rank on local defect x board occurrence x tree occurrence, qualified by the deployment node bracket. Record seven-man truth availability separately; a missing table is unknown, not agreement, but does not block independently labelled draw-scaling work.

Rarog's 36,400-game census changed two apparent zero-occurrence families into
real targets and found its seven-man KRPPKRP family in 5.40% of games. Those
numbers are donor priors only (BAS-X18); 6.7 must measure Basilisk's own policy.

### 6.8 High-value Group B families

- [ ] **6.8** Resolve, then implement only locally demonstrated, dependency-complete Group B work.
  - [ ] **6.8.a** `[R3]` Resolve the won-rook-ending only-move precision defect from BAS-E53. Study all 68 one-winning-move nodes with a held-out split, compare search/evaluation/knowledge explanations, and produce a mechanism or a justified no-change verdict. More depth is already refuted.
  - [ ] **6.8.b** `[R2]` Cover KR-KP, KQ-KRP and KR-KB according to 6.7's kind classification; protect winning rook endings while correcting drawn overclaim.
  - [ ] **6.8.c** `[R3]` Cover bishop-pawn families, including wrong-bishop/rook-pawn fortresses and promotion races. Absorb the current KBP-K deficit as bishop-pawn technique, not king geometry (BAS-E48/BAS-E49).
  - [ ] **6.8.d** `[I2]` Add deterministic theory, WDL, move-quality and promotion-closure cases before fitting. Each test must be shown to fail on the defect or known-bad mutant it claims to catch.
  - [ ] **6.8.e** `[R3]` Group mutually covariant family value, scale and generic guidance terms; fit the complete local cluster on training/validation only. Never freeze old coefficients while adding a new term or SPRT every recognizer alone.

### 6.9 Group B qualification

- [ ] **6.9** Qualify Group B against the frozen Group-A head.
  - [ ] **6.9.a** `[R2]` Register gates by occurrence: normal no-adjudication STC for common families, endgame-start cohorts for medium-frequency families, and theory/WDL/DTZ plus a loss-permitting no-regression SPRT for tails that cannot move whole-match Elo at feasible budget.
  - [ ] **6.9.b** `[V]` Require paired truth/move-quality improvement, zero family vetoes, deterministic/tactical passes and the registered clean-PGO game verdict. A correctness failure cannot be traded for aggregate conversion or Elo.

### 6.10 Group C and marginal-value stop

- [ ] **6.10** Evaluate remaining lower-yield families in the local 6.7 order.
  - [ ] **6.10.a** `[R2]` Measure KPs-K and KP-KP scale/technique gaps before implementation.
  - [ ] **6.10.b** `[R3]` Reconcile KQ-KP and KQ-KR at deployment-representative budgets; do not preserve a low-budget deficit that disappears with search.
  - [ ] **6.10.c** `[R3]` Audit KR-KN and KNN-KP/KNN-K theoretical scaling, fortress residue and rule-50 behavior.
  - [ ] **6.10.d** `[I2]` Implement and locally fit only mechanisms with measurable defect, sufficient occurrence and a valid promotion closure.
  - [ ] **6.10.e** `[V]` Gate the dependency-complete remainder by the 6.9 tier rules and stop when marginal value no longer pays for complexity.

### 6.11 Endgame closure

- [ ] **6.11** Close classical endgame maturity.
  - [ ] **6.11.a** `[M]` Freeze the accepted evaluator, versioned truth cohorts, reports, budgets, floors and family gate dispositions.
  - [ ] **6.11.b** `[R2]` Record every rejected mechanism, alternative explanation, unresolved seven-man gap and exact retry trigger.
  - [ ] **6.11.c** `[V]` Reconcile all family layers and confirm that no correctness, mate, rule-50 or tablebase veto remains hidden by an aggregate result.
  - [ ] **6.11.d** `[M]` Authorize board/toolchain work and post-endgame corpus generation only after PLAN, GUIDE and EXPERIMENTS agree on the accepted head.

## 7. Toolchain and board CORRECTNESS (not a speed phase)

**Maintainer direction, 2026-09-07: this phase hunts defects. It does not chase
throughput. Basilisk already has the speed.** Rarog RAR-M20 measured the current
Basilisk board source (`d734766`, still the board at this roadmap revision)
directly on the same Ryzen 9 5950X workload: Basilisk beat Rarog by 43.7% in
legal generation, 22.3% in legal captures, 29.4% in generation plus
make/unmake, 39.8% in perft and 46.0% in two-ply simulation, and RAR-M29's
normalized SEE rerun put Basilisk 29.9% ahead. Those are active-desktop,
non-PGO microbenchmarks with material scatter, so they do not measure
whole-search share or Elo -- but they are more than enough to retire
"Basilisk's board is slow" as a motivation for this phase.

What justifies the phase instead is that a peer audit of the same subsystem
found **three real SEE defects** in Rarog (BAS-X22) -- selected-king legality,
created pins, recapture promotions -- one of which moved its bench tree by
10.14%. Correctness defects in generation, SEE, make/unmake and repetition are
invisible to NPS and to ordinary tests, and they cost Elo silently. That is the
prize here.

Consequences for how the leaves are read:

* **7.1, 7.9 and 7.10 are the phase.** Contract auditing, policy separation and
  qualification are unconditional, because they are what finds defects.
* **7.3-7.8 are conditional and default to no-change.** They may only open if
  7.2 proves the component hot in real HCE search, and closing one with "profiled,
  not hot, no change" is a SUCCESS, not an unfinished leaf. Do not treat them as
  a speed backlog to work through.
* **A speedup is not a reason to accept a diff here.** Any behaviour change found
  while optimizing is a playing change and goes to 7.11's registered gate; a
  behaviour-neutral speedup with no defect behind it needs no gate and also
  earns no credit against this phase's purpose.
* **Report defects found, not percent gained.** The phase closes on 7.12 with an
  explicit no-change disposition for every conditional leaf that stayed shut.

- [ ] **7.0** `[I1]` Refresh and freeze the classical toolchain before performance work. Inventory Windows/MSYS2, Linux CI and macOS compiler, C++ library, CMake, Ninja and profile-tool versions; compare current versus newest stable one axis at a time; require CTest, sanitizers, exact search agreement, ISA checks and pooled release/PGO throughput before selecting the faster non-regressing line.
- [ ] **7.1** `[R2]` Port/adapt Rarog's board-v2 instrument and audit Basilisk's current contracts. Freeze canonical FEN, sorted legal/capture sets, perft/divides, keys, occupancy, normal/hinted/staged/null/clone/unwind restoration, checks/evasions, pinned EP, every castle and quiet/capture underpromotions. Add negative controls and an allocation guard. Audit malformed UCI, maximum move counters and every production SEE caller; use an independent legal same-square capture oracle rather than `see == see_ge` as the sole oracle.
- [ ] **7.2** `[R3]` Profile board work in actual HCE search on opening, middlegame, check-heavy, promotion and sparse-endgame cohorts at realistic budgets. Attribute generation, legality, pin/check queries, SEE, make/unmake, key/history work and allocations as shares of full-search time; prove instrumentation-off identity and each counter wire. Set a written component-time budget before opening optimization work.
- [ ] **7.3** `[R3]` Optimize legal generation and move-list delivery only if 7.2 makes them hot. Examine color/mode specialization, setwise pawns, pin discovery, king safety and repeated list initialization/copying in emitted code. Preserve exact legal sets, underpromotions and capture/quiet partitions; treat changed move order as a search change, not a neutral speedup.
- [ ] **7.4** `[I1]` Measure a fused ordinary-piece relocation path against remove-plus-add only if make/unmake is hot. Preserve mailbox, both occupancies, keys, evaluator state and every special move through make and unmake; retain only a robust whole-search benefit.
- [ ] **7.5** `[R3]` Share pin/check/attack information only where 7.2 proves duplicate work. Specify node/ply ownership, both-king semantics and real/null/undo/clone invalidation; never reuse original-position pin masks inside an evolving SEE exchange.
- [ ] **7.6** `[R2]` Optimize the corrected SEE kernel only after 7.1's independent contract passes. Carry attackers/rays incrementally where profitable while preserving king legality, created/released pins, en passant, recapture promotions and threshold equality at real production thresholds; compare kernel and whole-search performance separately.
- [ ] **7.7** `[I1]` Make game/search history capacity and mutation contracts explicit. Preserve arbitrary legal game history, reserve search headroom before hot paths and worker clones, prove no mid-search growth, and narrow unsafe mutable-derived-state APIs without adding hot validation.
- [ ] **7.8** `[R3]` Decide whether king-square caching or a larger board representation change pays only after earlier measured work. Compare complete alternatives, cache footprint and HCE/NNUE seams; keep mailbox, hashes, compact moves and portable slider backends; stop after one bounded architecture comparison fails.
- [ ] **7.9** `[R3]` Audit rule-50, repetition, null boundaries, mate precedence and TT/evaluation identity as separate policies. Preserve accepted invariants and historical rejected-policy retry conditions; never bundle a semantic change into a speed refactor.
- [ ] **7.10** `[I1]` Qualify the integrated board candidate with debug/release state and SEE tests, randomized independent comparison, sanitizers, all slider/ISA backends, exact fingerprint accounting and interleaved pooled-PGO NPS. Correctness changes and behavior-neutral speed changes retain separate baselines.
- [ ] **7.11** `[R2]` Register one dependency-complete no-adjudication playing gate for any board/SEE semantic change; use a symmetric bracket for an unknown-sign repair and do not gate each neutral optimization separately.
- [ ] **7.12** `[R2]` Refresh only endgame evidence affected by the accepted board head, version changed baselines/floors/rankings, and preserve unchanged reference arms. Close the phase with source/binary recipes and an explicit no-change disposition for every conditional optimization.

Directly reusable Rarog sources include `tools/diag/board_v2_oracle.py`,
`board_v2_run.py`, `see_contract_oracle.py`, `normalized_see_compare.py` and the
board-call counters introduced at donor `881c821`. The last commit only lands
Rarog instrumentation; it has no recorded profile verdict yet (BAS-X27), so
Basilisk must qualify both the wire and the findings independently.

## 8. Post-endgame corpus and complete HCE refit

This phase incorporates Rarog 4.13-4.14's now-measured experimental shape:
post-hoc tablebase relabeling and whole-game tablebase adjudication are
distinct arms and must not be conflated.
The target is a mature final classical evaluator, not merely another fit.

### 8.0 HCE maturity and feature-completeness audit

- [ ] **8.0** Define and close the final handcrafted-evaluation surface.
  - [ ] **8.0.a** `[R3]` Compare Basilisk conceptually with strong maintained HCE engines in D:/code; pin revisions and learn contracts/interactions rather than treating feature parity as an objective.
  - [ ] **8.0.b** `[R3]` Audit material/imbalance, PST, mobility, pawn structure, passers, outposts, threats, space, king safety, initiative/winnability, draw scaling, rule-50 damping and score saturation.
  - [ ] **8.0.c** `[R3]` Trace every term through EvalTrace, caches, qsearch stand-pat, correction, pruning margins and TT score storage; measure activation, phase/material coverage, correlation, clipping and factorial interactions on representative cohorts.
  - [ ] **8.0.d** `[R2]` Identify dead, duplicate, saturated, mutually cancelled and uncovered terms; classify each as repair, structural candidate, fit issue, neutral cost change or no-change with a deciding test.
  - [ ] **8.0.e** `[I1]` Add deterministic tests for every accepted categorical mechanism, expand the roadmap with any evidence-backed implementation clusters, and freeze the architecture before production datagen.

### 8.1 Fit-tooling contract

- [ ] **8.1** Harden and document the complete fit pipeline before generating expensive data.
  - [ ] **8.1.a** `[I1]` Fit K once on training/validation data and freeze it across all compared stages and arms.
  - [ ] **8.1.b** `[I1]` Accept an explicit complete initial vector, record every surface coordinate and reject partial/missing-by-accident vectors.
  - [ ] **8.1.c** `[I1]` Freeze by-game train/validation/test splits and atomically claim the test set so it can be opened once after model selection.
  - [ ] **8.1.d** `[R2]` Enforce an exact free/fixed/excluded surface partition, gauge anchors, reconstruction and semantic bounds; every production scalar has one instrument or a written invariant/unidentifiable disposition.
  - [ ] **8.1.e** `[M]` Hash corpora, row order, splits, configs, binaries, source revisions, tablebases, vectors and reports; restore source and the production binary byte-for-byte on success or failure.
  - [ ] **8.1.f** `[I1]` Enforce labels exactly in {0, 0.5, 1}; publish accepted counts and every rejection reason, and fail on suspiciously large rejection fractions.
  - [ ] **8.1.g** `[I1]` Give materially different corpus semantics a named versioned contract. Port/adapt Rarog's `build_book.py`, `book_yield.py`, `relabel_tb.py`, complete-fit driver, confirmation driver and manifest writers where useful; never widen a gate until unknown data silently passes.
  - [ ] **8.1.h** `[R2]` Write a Basilisk Texel handbook containing canonical paths, immutable resources, supported commands, surface partition, pipeline, traps, reference timings and recovery procedure. Rarog's handbook is a template, not a source of Basilisk constants.

### 8.2 Corpus design

- [ ] **8.2** Design a phase-efficient, natural-termination corpus.
  - [ ] **8.2.a** `[I1]` Locate the actual source position store under D:/chess; record canonical path, format, row count, duplicate rate, material-phase distribution and content hash rather than importing Rarog's path or counts.
  - [ ] **8.2.b** `[R3]` Define buckets from Basilisk's evaluator/material phase, never nominal ply. Measure rows/game conditioned on start-position phase; only opening starts can feed an opening bucket, while direct later-phase starts can improve independence.
  - [ ] **8.2.c** `[I1]` Freeze one extractor contract across every arm: start/end skips, per-phase and per-game caps, tactical filters, deduplication, ordering and by-game split seed. Change one causal axis at a time.
  - [ ] **8.2.d** `[R2]` Derive corpus size from effective independent rows per identifiable coordinate and label quality; confirm with a held-out learning curve instead of treating raw rows as capacity.
  - [ ] **8.2.e** `[M]` Register phase/material targets, start-book composition, independent starts, duplicate/rejection ceilings, learning-curve stop conditions, maximum generation budget and immutable publication names before launch.

### 8.3 Generate and freeze the source corpus

- [ ] **8.3** Generate self-play with the accepted post-endgame/board head.
  - [ ] **8.3.a** `[I1]` Prepare and verify the exact PGO generator, fixed-node budget, book segment, seed, concurrency and no-score-adjudication profile; then hand the single long-run command to the maintainer.
  - [ ] **8.3.b** `[R2]` Audit returned termination mix, natural mates, duplicate/leakage rate, by-phase yield, family coverage and <=6-man yield; reject a run whose manifest or registered contract drifted.
  - [ ] **8.3.c** `[M]` Publish corpus A atomically under a new name with immutable row order, splits and hashes before relabeling.

### 8.4 Build matched label arms

- [ ] **8.4** Create the matched tablebase-relabel comparison. BAS-E46 already sizes the opportunity: at 8,000 nodes 19.77% of clean tablebase wins were not won, falling to 13.65% at 25,000; more nodes recovered only about a third at 3.1x cost.
  - [ ] **8.4.a** `[M]` Keep corpus A's original white-perspective self-play game-result labels unchanged.
  - [ ] **8.4.b** `[I1]` Create corpus B as a row/order/split-identical copy except eligible local-tablebase rows receive Syzygy WDL labels.
  - [ ] **8.4.c** `[R2]` Treat cursed wins/losses as draws under rule 50 and analyze halfmove-clock semantics before relabeling.
  - [ ] **8.4.d** `[I1]` Prove identical FENs, ordering and by-game membership; reject missing tables as unknown rather than agreement.
  - [ ] **8.4.e** `[R2]` Decide row-local eligibility explicitly; never propagate an ending verdict backward into >tablebase positions without a separately registered causal rule.
  - [ ] **8.4.f** `[M]` Publish changed-row count/fraction, family and phase distribution, before/after label matrix, probe failures and a derived-from manifest.

### 8.5 Analyze whole-game tablebase adjudication separately

- [ ] **8.5** Decide whether a whole-game Syzygy-adjudicated corpus deserves a third arm.
  - [ ] **8.5.a** `[R3]` Audit semantics/provenance and game-to-row lineage: tablebase termination changes the result label of every sampled row in that game, unlike local relabeling.
  - [ ] **8.5.b** `[I1]` Pilot corpus C only if its starts, extraction and non-label semantics can be matched closely enough for a useful comparison.
  - [ ] **8.5.c** `[R2]` Register C as a separate experiment with its own hypothesis and gate; never merge it into A-versus-B or use tablebase adjudication in strength games.
  - [ ] **8.5.d** `[R2]` Record a no-arm verdict if the design cannot isolate whole-game termination from changed sampling distribution.

### 8.6 Initialization control

- [ ] **8.6** Measure optimizer dependence before the production fit.
  - [ ] **8.6.a** `[I1]` Fit identical targets from accepted-head and neutral/gauge-correct initial vectors under the same K, data and schedule.
  - [ ] **8.6.b** `[R2]` Compare validation convergence, parameter distance, covariance and untouched held-out loss; account for regularization toward the stage prior and local nonlinear search.
  - [ ] **8.6.c** `[M]` Register the production initialization rule before opening the test set.

### 8.7 Complete matched fits

- [ ] **8.7** Refit every relevant Texel-tunable HCE coordinate.
  - [ ] **8.7.a** `[I1]` Use the same complete surface, fixed K, optimizer budget and registered initialization for A and B.
  - [ ] **8.7.b** `[R2]` Use an alternating schedule when nonlinear selectors gate linear features: nonlinear prefit, complete linear fit, nonlinear refit and linear polish, with stage budgets validated for Basilisk rather than copied from Rarog.
  - [ ] **8.7.c** `[I1]` Verify perturbation/reconstruction, activation, gauge anchors, bounds, settled trajectories and every plus/minus wire.
  - [ ] **8.7.d** `[M]` Produce independently applicable vectors, source patches, candidate binaries and hash-complete manifests.
  - [ ] **8.7.e** `[I1]` Reject missing/frozen-by-accident coordinates, source drift, repeated test access or a fit that cannot restore the original source/binary.

### 8.8 Registered label-contract gate

- [ ] **8.8** Test whether tablebase relabeling transfers to Basilisk strength.
  - [ ] **8.8.a** `[R2]` Register exact candidates, common accepted pre-fit baseline, bracket/cap/stop rule and the causal A-versus-B comparison before games.
  - [ ] **8.8.b** `[I1]` Build clean PGO arms and run natural-termination gates under identical conditions.
  - [ ] **8.8.c** `[R2]` Use fit loss and truth reports to explain phase/family effects; only the registered SPRT decides strength.
  - [ ] **8.8.d** `[R2]` Accept one label policy/vector by the prospective rule; preserve rejected vectors and alternative explanations.

### 8.9 Iterative refresh

- [ ] **8.9** Run iterative whole-surface Texel cycles; at least one refresh from the accepted fitted head is mandatory.
  - [ ] **8.9.a** `[R2]` Prospectively cap the loop and define "repeat while the previous independent gate accepts; stop at the first non-acceptance" before cycle 1.
  - [ ] **8.9.b** `[I1]` Generate and publish a new no-adjudication corpus from the accepted engine under the winning label contract.
  - [ ] **8.9.c** `[I1]` Refit the complete surface with a new untouched test and exact manifests; never tune only the coordinates that looked promising in the prior test.
  - [ ] **8.9.d** `[R2]` Independently gate each cycle and stop at the first rejection/cap; never average a rejected vector into the head.
  - [ ] **8.9.e** `[R2]` Publish the cycle table (corpus, composition, test loss, gate, cumulative strength) and repeat the residual/cohort audit to decide whether structure stays closed.

### 8.10 Nonlinear HCE tuning

- [ ] **8.10** Conditionally tune evaluation terms Texel cannot price correctly.
  - [ ] **8.10.a** `[R3]` Inventory nonlinear, capped, thresholded and contextual terms after the accepted linear cycles; measure activation, interaction and local curvature with zero games.
  - [ ] **8.10.b** `[R2]` Include only live, sufficiently frequent coordinates such as a demonstrated king-danger funnel or validated contextual scale; exclude sparse recognizer switches, exact truth rules and Texel-owned linear weights.
  - [ ] **8.10.c** `[I1]` Wire the selected bounded options from exact accepted defaults and prove plus/minus perturbations reach every consumer. Close SPSA no-change if the surface is flat, monotone, unidentifiable or not worth its prospective gate.
  - [ ] **8.10.d** `[R2]` If justified, register a small sensitivity pilot and immutable full horizon/estimator. Pilot theta diagnoses only; the full tune starts from accepted defaults and HCE never shares a tune with search.
  - [ ] **8.10.e** `[I1]` Run natural-termination SPSA and accept only a fresh clean-PGO bake through an independent SPRT plus truth/correctness gates.

### 8.11 HCE closure

- [ ] **8.11** Freeze the classical evaluator.
  - [ ] **8.11.a** `[I1]` Revalidate score scale, calibration, tactics, mate bounds and all endgame floors under the final vector.
  - [ ] **8.11.b** `[R2]` Ablate new mechanisms and low-information fitted coordinates on held-out data and registered playing tests where needed.
  - [ ] **8.11.c** `[R3]` Compare held-out loss, truth quality and game strength against the pre-Phase-8 head and selected HCE references; reconcile disagreements without inventing an exchange rate.
  - [ ] **8.11.d** `[M]` Archive final surface, corpus/label policy, cycles, fit/tune artifacts, source/binary hashes and retry triggers.

### 8.12 Evaluation throughput optimisation

- [ ] **8.12** Optimize the frozen evaluator's cost without changing what it computes.
  - [ ] **8.12.a** `[R3]` Profile full evaluations and cache hit/miss paths on phase-balanced and narrow activation cohorts; attribute cost per term/phase and repeated attack/board work.
  - [ ] **8.12.b** `[I2]` Optimize only measured hot terms while keeping every evaluation bit-exact; exact bench plus direct score-corpus identity is required, and any score change returns the work to 8.10 or earlier.
  - [ ] **8.12.c** `[R2]` Audit lazy-eval and early-exit thresholds against the final surface as a separate behavior-changing question.
  - [ ] **8.12.d** `[I1]` Require interleaved pooled-PGO NPS on an idle-enough host and a no-adjudication SPRT; record but do not ship a speed gain that fails to convert.

Step 8.12 exists because evaluation cost is only worth attacking once the
surface is frozen: optimising a moving evaluator wastes the work twice. It is
deliberately a **bit-exactness** step. Anything that changes a score belongs to
8.10 or earlier, and exact bench identity is what separates the two -- with the
6.1.f caveat that identity is necessary and not sufficient, since a bench suite
covers only the positions it contains.

## 9. Classical search consolidation and release

The final HCE changes centipawn scale, qsearch share and pruning populations.
Old counters and old SPSA seeds become priors. This phase audits composition
before tuning, separates HCE/search/clock surfaces, and spends SPSA only where
activation, interaction and curvature justify it.

### 9.0 Search implementation, interaction and authority audit

- [ ] **9.0** Audit the complete current search before proposing another mechanism.
  - [ ] **9.0.a** `[R3]` Inventory iterative deepening, aspiration/PVS, move-picker stages, all histories and updates, ordering-versus-pruning reuse, qsearch, pruning, null move, reductions, extensions/singular verification, re-search and cutoff semantics. Trace node/ply/depth/window/bound, mate/draw/abort, excluded-move and TT evidence contracts through every consumer.
  - [ ] **9.0.b** `[R3]` Test interaction hypotheses with bounded ablation/factorial screens: baseline, A, B and A+B where justified. Look for cancellation, masking, stale evidence and a faster node that expands a worse tree; honor every prior retry trigger.
  - [ ] **9.0.c** `[R2]` Re-run the revision-matched oracle differential at sample stride 1 and profile cumulative/per-iteration nodes at shallow, mid and playing depth. Report per-position distributions and same-unit denominators; prove every UCI/counter wire with an absurd value. End with evidence-backed candidate/no-change leaves.

### 9.1 Categorical search work

- [ ] **9.1** Revisit singular-extension gate depth and any unique defects isolated by 9.0.
  - [ ] **9.1.a** `[R3]` Re-measure extension/depth authority on the frozen HCE/toolchain at fixed nodes, fixed depth and equal node cost; correctness canaries veto while aggregate disagreement remains inconclusive until per-position analysis resolves it.
  - [ ] **9.1.b** `[R2]` Implement the smallest dependency-complete categorical candidate, prove switch-off identity and gate it before tuning related constants. Close without code if 9.0 isolates no unique defect.

### 9.2 SEE and move-ordering value scale

- [ ] **9.2** Audit and, only if justified, fit Basilisk's production SEE/value policy.
  - [ ] **9.2.a** `[R2]` Zero-game audit the dedicated 100/300/300/500/900/20000 SEE vector and `search.cpp`'s same legacy values against the final HCE scale. Count changed `see_ge` verdicts at real thresholds, MVV-LVA order changes and qsearch delta-margin decisions; unequal values alone are not a defect.
  - [ ] **9.2.b** `[I2]` If decisions move materially, expose a board/search-owned injectable value surface with exact-default identity, independent special-move fixtures and no dynamic hot-path cost. Keep normalized benchmark values separate from playing values.
  - [ ] **9.2.c** `[R2]` Fit the live policy separately from HCE, using SPSA only if 9.6's prerequisites hold; use a symmetric unknown-sign gate and revalidate normalized SEE timing after acceptance.

### 9.3 TT, caches, hashing and hot memory

- [ ] **9.3** `[R3]` Audit TT/eval/pawn cache keys, replacement, publication/atomicity, aging, collisions, mate normalization, bounds/depth, rule-50/repetition identity, parameter invalidation, resets, sharing, alignment, prefetch and real hit/miss populations. Derive bounded repair/optimization leaves before 9.6; deleting validation or density is not a neutral speedup.

### 9.4 Threading, UCI lifecycle and tablebases

- [ ] **9.4** `[R3]` Audit worker start/stop/join, cancellation, completed-result authority, shared TT/root state, new-game/position/options resets, clone/resource lifetime, UCI command ordering and Syzygy FFI/thread/halfmove/WDL/DTZ contracts. Use deterministic interleavings plus stress; leave high-thread NUMA designs to Phase 13.

### 9.5 Diagnostic, harness and build-delivery audit

- [ ] **9.5** `[R2]` Re-audit counter sampling/units, parser/timeout/exit handling, benchmark barriers and corpus, instrument-off identity, game pairing/clock/stop rules, PGO/ISA provenance and artifact reproducibility after all preceding tool changes. Suspend and correct any gate whose instrument is invalid; preserve historical values as superseded.

### 9.6 Conditional post-HCE search SPSA

- [ ] **9.6** Tune only a demonstrated displaced interacting optimum.
  - [ ] **9.6.a** `[R3]` Select live cp-valued RFP, null, futility, ProbCut, qsearch, correction, aspiration, SEE or history/LMR coordinates from 9.0-9.5 telemetry. Exclude categorical switches, mate/endgame constants, TT/hash/thread settings and clock policy.
  - [ ] **9.6.b** `[I1]` Replace stale seeds with exact accepted defaults; verify plus/minus perturbations and semantic rails at start/mid/end; register dimensions, ranges, steps, immutable horizon, games, book, seed, tail estimator and independent gates.
  - [ ] **9.6.c** `[R2]` Run a bounded sensitivity pilot only when needed. Pilot theta is neither candidate nor seed; re-audit the complete active interaction surface before the full run. Skip with a recorded reason if flat, monotone, unidentifiable or low-value.
  - [ ] **9.6.d** `[I1]` Run natural-termination SPSA in dependency-complete stages only when separate surfaces are justified; complete the registered horizon, bake the registered endpoint/tail estimator into fresh PGO and gate each accepted stage independently.
  - [ ] **9.6.e** `[R2]` Preserve rejected tunes and convergence diagnostics; never add apparent gains or post-select a checkpoint/coordinate subset.

Rarog's SPSA workflow is a source of tooling ideas, not a package to merge:
transactional resume, immutable run metadata, deterministic non-overlapping
opening cursors, whole-vector tail extraction, rail/near-off diagnostics and
dependency pinning each require an independent Basilisk review and disposable
interrupt/resume test before use.

### 9.7 Time management

- [ ] **9.7** Complete clock/root-confidence work separately from search SPSA.
  - [ ] **9.7.a** `[R3]` Trace units, overhead/reserve, soft/hard stop, node polling, completed-root scores, aspiration instability, worker results and deadline behavior at 1T/4T; separate deadline correctness from think-time strength policy.
  - [ ] **9.7.b** `[R2]` Size any overhead/forfeit sweep prospectively on a null pair. Zero forfeits is a precondition, not a strength verdict.
  - [ ] **9.7.c** `[R3]` Diagnose remaining root-instability/confidence consumers from completed authoritative snapshots only; tune or remove unowned paths.
  - [ ] **9.7.d** `[I1]` If continuous parameters need tuning, run a separate clock-based tune and clean-PGO gate; never mix clock policy into fixed-node/search SPSA. Pass 1T and 4T clock gates with zero forfeits.

### 9.8 Correctness, cleanup and checkpoint

- [ ] **9.8** Close implementation debt before final claims.
  - [ ] **9.8.a** `[R2]` Reconcile module/audit ownership, dead/unreachable paths, dormant switches and retry triggers. Remove unconsumed alternatives; keep diagnostics only with a named future owner.
  - [ ] **9.8.b** `[I1]` Run state, repetition/rule-50, TT/mate, SEE/pin, qsearch, parser, sanitizer, debug/release and supported-feature matrices; add regressions for every demonstrated defect.
  - [ ] **9.8.c** `[I1]` Reproduce exact benchmark, pooled-PGO NPS, fixed-time/fixed-node profiles and accepted game verdicts on the clean checkpoint.

### 9.9 Final classical checkpoint

- [ ] **9.9** `[R2]` Compare the final head with the pre-Phase-8 head and 1.9.3 using revision-matched clean-PGO binaries and no adjudication. Record HCE and post-HCE-search attribution, NPS, fixed-node behavior, STC/LTC/4T direction, re-run the oracle split, ablate surprising contributors and close every maturity classification.

### 9.10 Portability, SMP and classical release

- [ ] **9.10** Release the strongest qualified classical line.
  - [ ] **9.10.a** `[I1]` Validate target-native execution, exact same-target search agreement, emitted ISA, sanitizer/CTest/UCI behavior and reproducible PGO assets/manifests on every supported platform.
  - [ ] **9.10.b** `[R3]` Revalidate SMP node/thread/helper-clock safety, contention and time-to-depth at 1/2/4/8 threads. Gate throughput and strength independently; do not infer scaling value from NPS alone.
  - [ ] **9.10.c** `[I1]` Pass prior-release STC, LTC `10+0.1`, 4T and external-cohort gates with zero hard-correctness/time failures; record topology, affinity and hash.
  - [ ] **9.10.d** `[M]` Publish the warranted version and user-facing compiler/ISA/tablebase manifests from measured cumulative strength.

### 9.11 Universal binary investigation

- [ ] **9.11** Determine whether startup CPU dispatch is worth adopting.
  - [ ] **9.11.a** `[R3]` Pin reference implementations and map Basilisk's current tiers, PEXT/magic tables, startup guard, PGO/linking and future NNUE ownership. Compare whole-engine variant dispatch with function-level dispatch; specify CPU plus OS checks and a test override.
  - [ ] **9.11.b** `[I2]` Build one isolated HCE prototype only after the design audit; keep ordinary release artifacts unchanged and record exact build/binary hashes.
  - [ ] **9.11.c** `[I1]` Force every tier on suitable hardware, verify baseline-safe startup and specialized regions, exact chess identity, target-native performance, startup, size and memory against separate binaries.
  - [ ] **9.11.d** `[R2]` Adopt, defer or reject with explicit hardware gaps and NNUE/release owners. Any adopted executable repeats affected 9.10 gates.

## 10. NNUE runway

- [ ] **10.0** Hand off the final classical measurement/data contract.
  - [ ] **10.0.a** `[M]` Freeze Phase-8 corpus splits, label manifests, final HCE/search score scale, board fingerprints and reference binaries for trainer and inference comparison.
- [ ] **10.1** Add factual per-ply state and dirty-piece deltas without NNUE inference.
  - [ ] **10.1.a** `[R3]` Freeze board baselines, including Rarog/Reckless reference contracts where useful, and define piece-add/remove/move, capture, off-target EP victim, promotion, castling, null and refresh timing from board facts rather than evaluator assumptions.
  - [ ] **10.1.b** `[I2]` Add the update interface and ownership seam; verify every transition against board reconstruction and qualify HCE identity plus move-event cost.
- [ ] **10.2** Add evaluator-owned accumulator scaffolding.
  - [ ] **10.2.a** `[R2]` Define per-thread/per-ply storage, perspective, validity, refresh, null, clone and unwind semantics without committing to a network architecture.
  - [ ] **10.2.b** `[I2]` Implement scaffold/full-refresh differential tests, exact HCE behavior and measured scaffold overhead.
- [ ] **10.3** Prepare the trainer and corpus path.
  - [ ] **10.3.a** `[R3]` Audit D:/code/net_trainer, Bullet/toolchain/GPU, serialization, quantization, buckets, deterministic splits, seeds and resume against the frozen contract.
  - [ ] **10.3.b** `[I1]` Run a pilot corpus/training/reload smoke and produce a reproducibility manifest; no large training before this passes.
- [ ] **10.4** Close the runway gate.
  - [ ] **10.4.a** `[I1]` Pass debug/release, scalar-oracle, every move transition, refresh/unwind, PGO fingerprint and bounded performance gates; reserve threat/relation hooks only if the selected network demonstrably needs them.

## 11. Baseline NNUE and 2.0.0

- [ ] **11.0** Harden the trainer and controlled data contract.
  - [ ] **11.0.a** `[I1]` Enforce strict CLI, deterministic by-game splits, hashes, seeds, checkpoints/resume, malformed-row refusals and exact feature/network metadata.
  - [ ] **11.0.b** `[R2]` Size unique training positions by learning curve and architecture capacity; generate, deduplicate and publish the corpus under the accepted label/sampling policy rather than inheriting a donor's 30-60M target blindly.
- [ ] **11.1** Train registered baseline networks.
  - [ ] **11.1.a** `[R3]` Compare one architecture axis at a time and at least two seeds per serious width/bucket configuration; select on frozen validation only and open test once.
- [ ] **11.2** Integrate scalar inference and packaging.
  - [ ] **11.2.a** `[I2]` Define the quantized network file/version/hash contract, integer bounds and scalar evaluator; require trainer-versus-engine integer-exact conformance.
- [ ] **11.3** Integrate incremental and SIMD inference.
  - [ ] **11.3.a** `[R2]` Prove actual-network incremental/full-refresh parity across every move/null/clone/unwind transition.
  - [ ] **11.3.b** `[I1]` Qualify SIMD/scalar parity, integer overflow bounds and every supported target/ISA.
  - [ ] **11.3.c** `[R2]` Attribute board updates, accumulator refresh/update and inference cost separately; gate whole-search NPS and memory.
- [ ] **11.4** Adapt search to NNUE.
  - [ ] **11.4.a** `[R3]` Re-audit score authority, qsearch, correction and every eval-coupled margin under NNUE; categorical mechanisms still gate before constants.
  - [ ] **11.4.b** `[R2]` Run one reserved post-NNUE search SPSA only on demonstrated displaced live coordinates, under the Phase-9 SPSA contract.
  - [ ] **11.4.c** `[I1]` Pass 1T STC/LTC, 4T, tactics, endgames, time and network-fallback gates.
- [ ] **11.5** Release 2.0.0.
  - [ ] **11.5.a** `[I1]` Reproduce network and binaries from manifests; pass correctness, provenance, portable fallback and packaging checks.
  - [ ] **11.5.b** `[R2]` Beat the final classical head and prior release at registered STC/LTC/4T conditions and confirm against the external cohort.

## 12. Post-NNUE frontier

- [ ] **12.0** Improve data and architecture only from measured residuals.
  - [ ] **12.0.a** `[R3]` Analyze disagreement/residuals by phase, material, king bucket, family, confidence and search outcome; distinguish representation limits from data/label defects.
  - [ ] **12.0.b** `[R3]` Evaluate scale, deduplication, hard-position mining and larger/sparser architectures one axis at a time with multiple seeds and held-out tests.
  - [ ] **12.0.c** `[I1]` Refresh data only under a registered changed-policy hypothesis and immutable label/sampling contract.
- [ ] **12.1** Extend search selectively.
  - [ ] **12.1.a** `[R3]` Revisit rejected classical mechanisms only when NNUE changes their recorded retry trigger; re-audit interactions and fit only displaced continuous consumers.
  - [ ] **12.1.b** `[R2]` Require isolated clean-PGO gates and preserve Basilisk-specific design.

## 13. Scaling and platform

- [ ] **13.0** Improve high-thread/NUMA scaling.
  - [ ] **13.0.a** `[R3]` Profile split points/iteration diversity, cutoff-usable TT traffic, contention, false sharing and memory bandwidth at 2/4/8/16 threads; measure time-to-depth and strength, not NPS alone.
  - [ ] **13.0.b** `[R2]` Optimize topology, TT/network placement and large-page policy only from measured causes; gate throughput and strength independently.
- [ ] **13.1** Expand platforms and delivery.
  - [ ] **13.1.a** `[I1]` Validate compilers, ISAs, universal/specialized dispatch and packaging on target-native hardware.
  - [ ] **13.1.b** `[I2]` Keep scalar/base and magic/portable fallbacks behaviorally identical and independently packaged.

## 14. Optional classical fallback

- [ ] **14.0** Reopen HCE only if NNUE is abandoned or a release blocker demands it.
  - [ ] **14.0.a** `[R3]` Require a new structural residual or changed data contract; never refit the unchanged surface again.
  - [ ] **14.0.b** `[R2]` Register mechanism, fit, budget and acceptance before work; preserve the Phase-8 frozen classical head.

## 15. Historical number map

Old references remain valid in EXPERIMENTS and git. Use this map rather than
rewriting historical evidence.

| Historical work | Current location |
|---|---|
| 5.0–5.3 | 5.0–5.2 |
| 5.4–5.6 | 5.3–5.5 |
| completed 5.7/5.8 evidence | 5.6 |
| 5.14 shallow cost | 5.7 |
| 5.9.1–5.9.6 | 5.8–5.9 |
| 5.9.11/5.9.15 | 5.9 |
| 5.9.12–5.9.14 | 5.10 |
| 5.9.16 | 5.11 |
| 5.9.7/5.9.17–5.9.21 | 5.12–5.14 |
| open 5.9.22–5.9.39 | 6.1–6.11 |
| pre-2026-09-07 6.5.b/6.5.c/6.5.d/6.5.e | 6.8.b/6.8.c/6.8.d/6.8.a |
| pre-2026-09-07 6.6–6.8 | 6.9–6.11 |
| pre-2026-09-07 Phase 7 | Phase 8 |
| pre-2026-09-07 8.0 compiler/build refresh | 7.0 |
| pre-2026-09-07 8.1 singular work | 9.1 |
| pre-2026-09-07 8.2–8.3 search SPSA | 9.6 |
| pre-2026-09-07 8.4 authority remeasurement | 9.0 |
| pre-2026-09-07 8.5 clock work | 9.7 |
| pre-2026-09-07 8.6–8.9 closure/release | 9.8–9.10 |
| pre-2026-09-07 7.12/8.10 throughput | 8.12 and Phase 7 |
| pre-2026-09-07 Phases 9–13 | Phases 10–14 |
