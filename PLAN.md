# Basilisk Strength Improvement Plan

> Implementation guide for taking Basilisk from its current `1.4.9` state to a
> measurably stronger engine by **tuning the search and evaluation constants it
> already ships**, under a proper SPRT + SPSA/Texel testing discipline.
>
> This document is meant to be handed to an implementation model (see
> "Recommended model" at the bottom). Work **one phase at a time, one change at
> a time**, and never merge a change that does not pass its SPRT gate.
>
> Companion: `user_dev_guide.md` (the human workflow). This file holds the
> technical detail; that one holds the ping-pong rhythm and decision points.

---

## 0. Background — why this plan exists

Basilisk is a mature C++23 hand-crafted-eval (HCE) engine (~7.7k LOC). Unlike a
young engine, **it is not missing features** — its search already has PVS,
aspiration windows, persistent Lazy SMP, a clustered atomic TT, null-move
pruning with high-depth verification, RFP, razoring, ProbCut, LMR, IIR, singular
extensions (with a multi-cut branch), SEE / threshold-SEE pruning, a staged
MovePicker, full continuation (1/2/4-ply) and correction (pawn / minor / non-pawn
/ continuation) histories, and Syzygy probing. Its evaluation is a rich tapered
HCE (PeSTO material + PST, mobility, full pawn structure, king-safety attack
table, shelter/storm, rook/knight/bishop terms, threats, hanging pieces, draw
scaling, tempo).

**The bottleneck is not features — it is two things:**

1. **Every constant is hand-guessed.** Search margins and eval weights are
   inline magic numbers (PeSTO defaults + values picked by eye). **None have
   ever been fit to this engine's own search.** This is the single most common
   source of left-on-the-table Elo in a feature-complete HCE.
2. **Testing has been statistically weak.** Strength has been judged with small
   ad-hoc Cutechess matches (e.g. the 1.4.9 CHANGELOG records "120 games,
   +23 ± 42 Elo" — the error bar swamps the signal). There is **no SPRT, no SPSA,
   no Texel tuner, and no fixed test harness** in the repo.

This mirrors exactly the lesson learned in the sibling engine **Rarog**
(`D:/code/rarog/PLAN.md`): *speed and features are not the bottleneck; untuned
constants are the #1 regression source, and the prerequisite for everything is a
fast, statistically valid test/tune loop.* Rarog's plan spends two whole phases
**porting** features that Basilisk already has — so Basilisk gets to **skip the
feature-porting work and go straight to fitting the constants it already ships.**

**Therefore: build the harness first, expose and tune the search constants, then
Texel-tune the eval. New features only after tuning plateaus. NNUE is noted as a
far-future ceiling (Phase 4) but is explicitly out of scope for now.**

---

## 1. Inventory — what exists, and where

### 1a. Search constants (all inline in `src/search.cpp`)

These are the SPSA targets for Phase 1. Line numbers are at the time of writing —
re-grep if they drift.

| Concept | Constant(s) | Location |
|---|---|---|
| Reverse futility pruning | `140·depth − (improving?60:0)`, gated `depth ≤ 9` | `search.cpp:1201-1204` |
| Razoring | `static_eval + 300·depth ≤ alpha`, `depth ≤ 3` | `search.cpp:1208` |
| Null-move reduction | `r = 4 + depth/4 + min((eval−beta)/200, 3)`, `depth ≥ 3` | `search.cpp:1219` |
| Null-move verification | fires at `depth ≥ 10` | `search.cpp:1231` |
| ProbCut margin | `beta + 200`, `depth ≥ 5`, reduced search `depth − 4` | `search.cpp:1244-1245` |
| IIR | `depth ≥ 4`, stale-TT `tt_depth < depth − 3` | `search.cpp:1274` |
| LMP threshold | `improving ? 3 + d² : 2 + d²/2` | `search.cpp:1287` |
| Move-loop futility | `static_eval + 150 + 110·depth ≤ alpha`, `depth ≤ 6` | `search.cpp:1325` |
| History pruning | `hist < −3500·depth`, `depth ≤ 6` | `search.cpp:1341` |
| SEE pruning (bad caps) | `see_ge(m, −80·depth)`, `depth ≤ 8` | `search.cpp:1347` |
| Singular extension | `s_beta = tt_score − 2·depth`, `s_depth = (depth−1)/2`, double-ext margin `s_beta − 20` | `search.cpp:1365-1380` |
| LMR base table | `0.75 + log(d)·log(m)/2.25` | `search.cpp:42` |
| LMR adjustments | `+1` non-PV, `+1` cut-node, `−1` tt-pv, `+1` !improving, `−stat_score/8192` | `search.cpp:1424-1429` |
| Aspiration window | initial `delta = 25`, grow `delta += delta/2`, reset `delta ≥ 900` | `search.cpp:1604-1619` |
| History bonus | `min(depth², 2048)`; 4-ply cont. uses `bonus/2` | `search.cpp:469, 511` |
| Correction history | weight `min(depth+1, 16)`, clamp `±1024` | `search.cpp:557-559` |

### 1b. Eval weights (all inline in `src/eval.cpp`)

These are the Texel targets for Phase 2. Grouped roughly by historical value:

- **Material / PST** — `MG_VAL`/`EG_VAL` (`eval.cpp:6-7`) + the eight PeSTO PST
  tables (`eval.cpp:16-135`). The single largest weight set.
- **Mobility** — `MOB_MG`/`MOB_EG` per piece (`eval.cpp:402-403`).
- **King safety** — attacker `UNIT` weights (`:457`), zone construction, coordination
  bonus (`:477`), no-queen scaling (`:482`), `SAFETY_TABLE[25]` (`:492-496`).
- **Pawn structure** — `PASSED_MG`/`PASSED_EG` rank tables (`:215-216`), doubled/
  isolated/connected/backward penalties (`:250-278`), dynamic passed-pawn terms
  (`:326-344`), passed-pawn king proximity (`:626-639`).
- **Pawn shelter & storm** — (`:500-559`).
- **Piece terms** — bishop pair (`:347-353`), rook open/semi/7th (`:355-379`),
  knight outpost (`:381-399`), rook-behind-passer (`:561-602`), trapped bishop
  (`:657-674`).
- **Threats / misc** — pawn threats (`:427-442`), hanging pieces (`:604-623`),
  space (`:641-655`), tempo (`:692-694`).
- **Draw scaling** — OCB scaling, KNNvK, 50-move damping (`:699-731`).

### 1c. Existing tooling to reuse (don't reinvent)

- **`bench [depth]`** — `bench 13` runs a fixed 16-position suite and prints a
  total node count. In single-thread mode that total is a **behavior
  fingerprint**: any change to search/eval/movegen moves it; a pure refactor
  keeps it. Use it as the default-equivalence check. (`src/bench.cpp`.)
- **CMake presets** — `release`, `release-avx2`, `release-pext`, plus a `pgo`
  build target. `cmake --build --preset release-pext --target pgo` produces a
  PGO binary and copies it to `build/dist/` with a `-pgo` suffix. **`pext` is the
  correct arch for local testing on this BMI2 machine; `avx2` is for
  distribution.** (`README.md` § Profile-guided release builds.)
- **UCI options** are declared in `Parameters::uciOptions()` and parsed in
  `Parameters::setOption()` (`src/Parameters.cpp:142-309`). SPSA
  (weather-factory) sets parameters **through UCI options**, so Phase 1 must
  expose the tunables here.

### 1d. What Basilisk does NOT have yet (new work, vs. Rarog which already had it)

- **No tunable params struct.** `Parameters` is only the UCI options/limits
  parser. Phase 1 must add a `tune`-gated `SearchParams` and wire it in.
- **No Texel tuner.** Phase 2 must make `EvalParams` loadable from a text file
  (mirroring Rarog's `tune.rs` pattern) and drive it with an **external** Texel
  optimizer.
- **No SPRT/SPSA scripts.** Phase 0 copies them from Rarog's `tools/`.

---

## 2. Guiding principles (apply to every phase)

1. **SPRT-gate everything.** No change is "good" until it passes a sequential
   probability ratio test in self-play. Default bounds: `elo0=0 elo1=5
   alpha=beta=0.05`. Tighten to `elo1=3` for a single tuned constant.
2. **One change at a time.** Never bundle. Tune one group, test it, keep or drop,
   then move on. (Rarog regressed precisely by merging untuned bundles.)
3. **Tune on entry; SPSA proposes, SPRT decides.** SPSA/Texel over-fit a noisy
   objective — their output is a *candidate*. The `st=0.1` SPRT at the deployment
   condition is the authority.
4. **Default-equivalence first.** When hoisting a constant into a struct/UCI
   option, the default must reproduce current behavior exactly — verify with the
   `bench 13` fingerprint before tuning.
5. **Commit each kept step separately** with a descriptive message. Surgical
   revert-ability matters more than tidy history.
6. **Always test PGO `pext` builds.** Build with the `pgo` target (Phase 0
   `build_test.ps1`); never SPRT a plain `release` binary — PGO shifts hot-path
   timing enough to move measured NPS/Elo.
7. **Test conditions mirror deployment:** `st=0.1` (100 ms/move, fixed), Hash 64
   MB, Threads 1, `SuperGM_4mvs.pgn` book, concurrency = physical cores − 1.
8. **Keep the eval behind a clean interface.** Do not let tuning refactors weld
   eval internals into search — preserve the `Evaluator::evaluate(const Board&)`
   boundary so a future NNUE (Phase 4) can be swapped in without a rewrite.

---

## 3. Phase 0 — Testing & tuning harness (prerequisite, do this first)

**Goal:** a one-command SPRT self-play test plus an SPSA tuning loop. Nothing
else proceeds until SPRT reproduces a known null result.

### Tooling (copied/adapted from `D:/code/rarog/tools/`)

- **SPRT / match runner: [fastchess](https://github.com/Disservin/fastchess)**
  at `tools\bin\fastchess.exe`. Built-in SPRT, no Qt. (fastchess has
  **no** built-in SPSA.)
- **SPSA tuner: [weather-factory](https://github.com/jnlt3/weather-factory)** at
  `tools\weather-factory`. Perturbs UCI options, runs fastchess mini-matches.
- **Texel tuner (Phase 2):** an external gradient-descent tuner that reads quiet
  labeled positions and writes an `EvalParams` weights file. (Decision: external
  + file-loadable weights, not a built-in subcommand.)

### Steps

1. **Create Basilisk's own `tools/` folder** by copying from Rarog and adapting:
   - `tools/sprt.ps1` — **near-verbatim** from Rarog. It drives binaries purely
     over UCI, so the only changes are default names (`Basilisk`) and leaving
     the book/fastchess paths as-is (shared `D:\chess\...` locations).
   - `tools/build_test.ps1` — **rewrite the build command.** Replace Rarog's
     `cargo xtask build --arch pext --pgo` with:
     ```powershell
     cmake --preset release-pext -DCOMP=clang
     cmake --build --preset release-pext --target pgo
     ```
     then copy the newest `build/dist/basilisk-*-pext*-pgo*` to
     `D:\chess\engines\test_engines\basilisk-<suffix>-pext-pgo.exe`.
   - `tools/spsa_configs/` — copy `cutechess.json`, `spsa.json`,
     `setup_spsa.ps1`, `README.md` as-is; **rewrite `config_pruning.json` /
     `config_lmr.json`** to Basilisk's option names from §1a (see §4).
2. **Install fastchess** → `tools\bin\fastchess.exe`.
3. **Record the baseline `bench 13` fingerprint** of the current `1.4.9` build:
   `bench 13 = 4,972,548 nodes` (release-pext, MSYS2 Clang). This is the
   default-equivalence anchor — any pure refactor must reproduce this exactly.
4. **Calibration smoke-test:** ~~skipped~~. The fastchess + weather-factory
   harness is proven working by the rarog project. Self-vs-self with symmetric
   bounds can take thousands of games to formally accept H0 — not worth the
   time when the toolchain is already validated. Proceed directly to Phase 1.

### Done when
Baseline `bench 13` fingerprint is recorded and the harness scripts exist.
(Calibration self-vs-self SPRT skipped — harness proven by rarog.)

---

## 4. Phase 1 — Expose and SPSA-tune the existing search constants

**Goal:** strength gain with **zero new search behavior** — only re-fitting the
inline constants from §1a. Lowest risk, highest confidence.

### Steps

1. **Add a `SearchParams` struct** (new `src/SearchParams.h`, or extend
   `Parameters`) holding every §1a constant, with `DEFAULT` values equal to the
   current inline literals. Replace the inline magic numbers in `search.cpp` with
   reads from this struct.
2. **Expose each as a UCI spin option** in `Parameters::uciOptions()` /
   `setOption()`, default = current value, sensible min/max (see table below).
   **Gate the whole tunable set behind a compile-time `tune` flag** (e.g.
   `-DTUNE=ON` CMake cache var → `#ifdef BASILISK_TUNE`) so production builds
   keep a clean UCI option list. Development builds compile with `tune` on.
3. **Verify default-equivalence:** `bench 13` fingerprint unchanged vs. the
   recorded baseline; SPRT vs. `1.4.9` returns ~0 Elo (`-Elo0 -3 -Elo1 3`).
4. **SPSA-tune in batches** (never all 20+ at once — the gradient gets too
   noisy). Two ready groups:
   - `config_pruning.json` — RFP / razor / null-move / LMP / futility / history /
     SEE / aspiration / singular margins.
   - `config_lmr.json` — LMR base/divisor, the four reduction adjustments, the
     history divisor.
   Run each group to convergence (tens of thousands of games at `tc=1`).
5. **SPRT-confirm** the tuned set vs. `1.4.9` head at `st=0.1` (the deployment
   condition) as the gate to keep it.

### Suggested option set (defaults from §1a)

| UCI option | Default | Range | Step | Source |
|---|---|---|---|---|
| `RfpCoeff` | 140 | [60, 240] | 14 | `:1202` |
| `RfpImproving` | 60 | [0, 140] | 12 | `:1202` |
| `RazorCoeff` | 300 | [120, 500] | 30 | `:1208` |
| `NullBase` | 4 | [2, 6] | 1 | `:1219` |
| `NullEvalDiv` | 200 | [80, 400] | 24 | `:1219` |
| `ProbCutMargin` | 200 | [80, 360] | 20 | `:1245` |
| `FutilityBase` | 150 | [40, 280] | 18 | `:1325` |
| `FutilityCoeff` | 110 | [40, 200] | 14 | `:1325` |
| `HistPruneCoeff` | 3500 | [1000, 7000] | 400 | `:1341` |
| `SeePruneCoeff` | 80 | [30, 160] | 12 | `:1347` |
| `SingularBetaMult` | 2 | [1, 6] | 1 | `:1365` |
| `SingularDoubleMargin` | 20 | [0, 60] | 8 | `:1380` |
| `AspirationDelta` | 25 | [10, 60] | 6 | `:1604` |
| `LmrBase` (×100) | 75 | [0, 150] | 12 | `:42` |
| `LmrDivisor` (×100) | 225 | [150, 350] | 18 | `:42` |
| `LmrHistDiv` | 8192 | [4096, 16384] | 1024 | `:1429` |

> `LmrBase`/`LmrDivisor` are floats — expose as integers ×100 and divide inside
> `init_lmr()`. Re-run `init_lmr()` whenever they change via `setoption`.

### Expected
+10–30 Elo, low risk. **The first real strength milestone.**

---

## 5. Phase 2 — Texel-tune the evaluation (the big lever)

**Goal:** richer, properly-fit HCE. Basilisk's eval weights are PeSTO defaults +
hand-picked terms that have never been fit to game results — historically the
largest single gain on a PST-derived eval short of NNUE.

### Steps

1. **Hoist eval weights into an `EvalParams` struct** (new `src/EvalParams.h`),
   defaults equal to the current `eval.cpp` literals. `Evaluator` reads from it.
   Verify default-equivalence (`bench 13` fingerprint unchanged before any
   re-fit; SPRT ~0 vs. head).
2. **Make `EvalParams` file-loadable** behind the `tune` flag: read weights from
   a text file named by an env var (e.g. `BASILISK_EVAL_FILE`), one param per
   line, arrays space-separated. (Mirror Rarog's `tune.rs` loader.)
3. **Build a Texel data set:** extract quiet positions from large self-play /
   tournament PGNs (`D:\chess\...`), label by game result. Filter out positions
   in check or with a hanging capture (quiescence filter).
4. **External Texel tuner** (a small standalone program/script) minimizes the
   sigmoid loss over the data set, writing an `EvalParams` file. **Tune one term
   group at a time**, highest historical value first:
   1. Material + PST (global anchor)
   2. Mobility
   3. King safety (attacker weights, `SAFETY_TABLE`, shelter, storm)
   4. Passed pawns (rank tables + dynamic terms + rook-behind-passer)
   5. Bishop pair / rook open-semi-7th / knight outposts
   6. Pawn threats, tempo, draw-scaling factors
5. **SPRT-gate each group** in self-play (Texel loss reduction ≠ Elo — the game
   test is the authority).
6. **Final global Texel pass** over all kept weights, then a final SPRT
   confirmation.

### Expected
The largest HCE lever short of NNUE; a well-fit material/PST + mobility +
king-safety + passed-pawn set is typically a solid multi-tens-of-Elo gain.

---

## 6. Phase 3 — New features (only if tuning plateaus)

Basilisk is already feature-rich, so this phase is **optional and deferred**.
Only after Phases 1–2 stop yielding SPRT-positive results, consider — one at a
time, SPSA/Texel-tuned on entry, SPRT-gated:

- Eval gaps Basilisk lacks: piece-on-piece threats (currently only pawn
  threats), explicit safe-check king-safety terms, weak-square / hole detection,
  pawn-structure-aware mobility area.
- Search refinements: more aggressive history-based LMR, dedicated double/triple
  extensions, correction-history-driven reductions, deeper ProbCut/SEE
  interplay.

Each item follows the per-feature checklist below.

### Per-feature checklist
- [ ] Implement the single feature on a fresh branch.
- [ ] `cmake --build` clean; full CTest suite passes.
- [ ] `bench 13` runs (fingerprint *will* change — record it).
- [ ] Expose new constants as `tune`-gated UCI options.
- [ ] SPSA/Texel-tune the new constants.
- [ ] **SPRT vs. current integration head** (`elo0=0 elo1=3`).
- [ ] Accept-H1 → commit + merge. Accept-H0 → discard, document why.

---

## 7. Phase 4 — NNUE (far-future ceiling, NOT scheduled)

NNUE is the single biggest remaining strength lever versus Stockfish, and the
**eventual** intended direction for Basilisk — but it is explicitly **out of
scope for Phases 0–3**. It is recorded here so the earlier work does not paint us
into a corner:

- **Keep the eval interface clean.** All Phase 1–3 refactors must preserve a
  single entry point (`Evaluator::evaluate(const Board&)` returning a
  side-to-move score). Do not leak eval internals into search. A future NNUE
  evaluator should be droppable behind the same signature.
- **Keep board state NNUE-friendly.** Basilisk already maintains incremental
  Zobrist + piece keys and per-square piece lookup (`board_sq`); preserve cheap
  incremental access to piece placement on `make_move`/`unmake_move`, since NNUE
  needs incremental accumulator updates keyed on exactly that.
- **Don't over-fit infrastructure to HCE.** The Texel weights file (Phase 2) and
  the `tune` flag are HCE-specific and can simply be ignored by an NNUE build.
- **When the time comes**, NNUE becomes its own multi-phase project (net
  architecture, training data pipeline, SIMD accumulator, quantization) gated by
  the same SPRT harness built in Phase 0 — which is reusable as-is.

---

## 8. Release & regression discipline

- Keep `1.4.9` (and `master`) as the **gauntlet baseline**. After each phase run
  a **multi-opponent gauntlet** (vs. 1.4.9, Stockfish at a comparable level,
  Rarog 2.x) to confirm self-play SPRT gains transfer against external opponents.
- Rebuild the PGO asset (`pext` for local, `avx2` for distribution) before any
  gauntlet — tuning changes the hot paths.
- Bump version + CHANGELOG only when a phase clears **both** SPRT and the
  external gauntlet.

---

## 9. Risks & gotchas

- **Untuned constants are the #1 failure mode** — never SPRT-judge a re-fit
  before tuning converges.
- **Self-play over-fit** — confirm against external engines each phase.
- **SPSA needs UCI-exposed params** — wire the option first, or weather-factory
  has nothing to set.
- **`bench` fingerprint is identity, not strength** — a changed fingerprint is
  neither good nor bad; only SPRT decides. Use it solely to prove a refactor was
  behavior-preserving.
- **Float LMR coefficients** must re-trigger `init_lmr()` on `setoption`.
- **Don't weld eval into search** — it forecloses Phase 4 (see §7).

---

## 10. Quick command reference

```powershell
# Record / check the behavior fingerprint (run interactively; piping is flaky):
#   .\build\release-pext\basilisk.exe
#   bench 13
#   quit

# Build a named pext-PGO test binary into D:\chess\engines\test_engines\
.\tools\build_test.ps1 -Suffix phase1

# SPRT — calibration / refactor (symmetric bounds, fast H0)
.\tools\sprt.ps1 -EngineA <refactor>.exe -EngineB <head>.exe `
    -NameA "Refactor" -NameB "Head" -Elo0 -3 -Elo1 3

# SPRT — test a real gain
.\tools\sprt.ps1 -EngineA <new>.exe -EngineB <head>.exe -NameA "X" -NameB "Head"

# SPRT — small single-constant feature (tighter bound)
.\tools\sprt.ps1 -EngineA <new>.exe -EngineB <head>.exe -NameA "X" -NameB "Head" -Elo1 3

# SPSA tuning (after Phase 1 UCI options + weather-factory setup)
cd tools\weather-factory; python main.py
```

---

## 11. Recommended model for implementation

**Primary driver: Sonnet 4.6 (medium).** The work is incremental, plan-driven,
and test-gated — port one change, run the harness, read the verdict, decide,
repeat. Most steps are mechanical (hoist constants into a struct, expose UCI
options, write/adapt scripts). The SPRT/`bench` gates do the quality control, so
the model executes the loop faithfully rather than having to be right in one
shot.

**Optional specialist for the Texel tuner and dense search internals: a
strong-algorithmic-code model (e.g. Codex/Opus-class).** The external Texel
optimizer and any Phase 3 search internals benefit from algorithmic density. Hand
those isolated pieces over, then return to Sonnet for the test/tune loop.

**Bottom line:** drive the whole plan with Sonnet 4.6 medium; escalate the
gnarly bits only if needed. Never let any model merge a change that hasn't passed
its SPRT gate — the process, not the model, guarantees the result.
