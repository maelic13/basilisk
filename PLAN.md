# Basilisk Strength Improvement Plan

> Current checkpoint: Basilisk is a feature-rich C++23 HCE engine at version
> `2.1.0`. The repo already has the Phase 0 harness and the Phase 1
> `SearchParams`/UCI tuning surface. The first pruning SPSA candidate has been
> SPRT-accepted at +18.87 +/- 8.81 Elo over the original defaults. The LMR SPSA
> candidate was also SPRT-accepted at +15.63 +/- 8.02 Elo over the pruning head.
> A narrowed combined polish failed its SPRT gate and was reverted. The Phase 1
> external gauntlet is complete. The immediate next work is Phase 2 evaluation
> tuning. Do not start new search/eval features until those gates stop paying.
>
> Companion: `user_dev_guide.md` is the human workflow and command cheat sheet.
> This file is the technical plan and decision record.

---

## 0. Current State Audit

### Engine

Basilisk is not a young engine missing basic chess-engine machinery. Current
source already contains:

- Search: iterative deepening, PVS alpha-beta, aspiration windows, Lazy SMP,
  shared clustered TT, TT prefetch/replacement aging, null-move pruning with
  verification, RFP, razoring, ProbCut, IIR, LMR, singular extensions,
  qsearch pruning, SEE/threshold SEE, legal PV/bestmove sanitization, Syzygy
  probing, and broad history/correction-history machinery.
- Move ordering: staged MovePicker, TT move, captures split by tactical quality,
  quiet history, capture history, killers, countermove, 1/2/4-ply continuation
  history, pawn-structure keyed history, low-ply history, root feedback, and
  tablebase root ordering.
- Eval: tapered HCE with PeSTO material/PST anchors, mobility, pawn structure,
  king safety, shelter/storm, passed-pawn dynamics, rook/knight/bishop terms,
  pawn threats, hanging pieces, space, tempo, endgame/draw scaling, and
  50-move damping.
- Tests: 8 CTest targets covering board, move, TT, eval, search, UCI, threading,
  and ponder behavior.

### Tooling Already Present

These are complete in the repo:

- `tools/setup_tools.ps1` downloads `tools/bin/fastchess.exe` and clones
  `tools/weather-factory/`.
- `tools/build_test.ps1` builds a `release-pext` PGO `-DTUNE=ON` binary and
  copies it to `tools/test_engines/`.
- `tools/sprt.ps1` runs fastchess SPRT at the intended fixed deployment
  condition: `st=0.1`, Hash 64 MB, Threads 1, repo-local book
  `tools/books/SuperGM_4mvs.pgn`.
- `tools/gauntlet.ps1` runs fixed-game phase-boundary validation matches with
  the same time/hash/book conditions.
- `tools/setup_spsa.ps1` writes weather-factory configs for either the pruning
  or LMR parameter group and can select which built engine suffix to tune from.
- `src/SearchParams.h`, `Parameters::uciOptions()`, `Parameters::setOption()`,
  and `Engine::build_limits()` already wire search parameters into search under
  `BASILISK_TUNE`.

### Still Missing

- Pruning and LMR search values have been accepted by SPRT and are now the
  Phase 1 head.
- No `EvalParams` abstraction or eval-weight loader exists.
- No Texel dataset or tuner exists.
- Several second-wave search constants remain inline and should only be exposed
  after the first SPSA cycle proves the harness is producing SPRT-positive
  changes.

### Main Diagnosis

The highest-value path is still sound: Basilisk has plenty of features, but the
constants are mostly hand-guessed. More features now would probably add
regression risk before the existing search/eval machinery is fitted to itself.

The plan therefore is:

1. Fit the exposed search constants.
2. Fit the HCE evaluation.
3. Only then consider new search/eval features.

---

## 1. Non-Negotiable Gates

Apply these to every phase.

1. **One candidate at a time.** Do not bundle unrelated feature work with tuned
   values.
2. **SPSA/Texel propose; SPRT decides.** A tuned value set is only a candidate.
3. **SPRT every kept strength change.** Default strength gate:
   `elo0=0 elo1=5 alpha=0.05 beta=0.05`. Use `elo1=3` for small follow-up
   refinements.
4. **Use deployment-like conditions for the final gate.** `st=0.1`, Hash 64 MB,
   Threads 1, repo-local `SuperGM_4mvs.pgn`, concurrency no higher than physical
   cores minus one.
5. **Use PGO `pext` binaries for local strength testing.** `tools/build_test.ps1`
   is the default build path for SPRT/SPSA candidates on this machine.
6. **Use `bench 13` correctly.** For pure refactors, the fingerprint must match
   the recorded baseline exactly. For tuned values or real behavior changes, the
   fingerprint may change; record it, but do not interpret it as Elo.
7. **Preserve the eval boundary.** Keep search calling
   `Evaluator::evaluate(const Board&)`; do not spread eval internals into
   search. This keeps Phase 2 manageable and Phase 4 possible.
8. **Keep release UCI clean.** Tuning options remain behind `-DTUNE=ON`; release
   builds should not expose development knobs.

---

## 2. Phase 0 - Harness (Complete)

Status: complete.

Done items:

- Repo-local fastchess/weather-factory setup script exists.
- Repo-local SPRT runner exists.
- Repo-local SPSA setup script and pruning/LMR configs exist.
- PGO test build script exists and builds with `-DTUNE=ON`.
- Baseline `bench 13` fingerprint recorded:
  `4,972,548 nodes` on release-pext/MSYS2 Clang.
- Calibration self-vs-self SPRT is intentionally skipped. The harness is
  straightforward UCI plus fastchess/weather-factory; the expensive H0 formal
  acceptance does not buy enough signal before the first real candidate.

Do not spend more time here unless a script fails.

---

## 3. Phase 1 - Search Constant Tuning (Complete)

Goal: improve strength without adding new search behavior.

Current status:

- `SearchParams` exists.
- Tune-gated UCI options exist.
- `Engine::build_limits()` passes `parameters_.search_params` into search.
- Default-equivalence has been recorded in the guide as:
  `bench 13 = 4,972,548 nodes`, 8/8 tests passed.
- Pruning SPSA was SPRT-accepted:
  `Elo +18.87 +/- 8.81`, 2930 games, H1 accepted for `[0.00, 5.00]`.
- LMR SPSA completed:
  4034 iterations, 129,088 games. Candidate values have been baked and are
  SPRT-accepted: `Elo +15.63 +/- 8.02`, 3714 games, H1 accepted for
  `[0.00, 5.00]`.
- Narrowed combined Phase 1 polish config exists:
  `tools/spsa_configs/config_combined.json`.
- Combined polish SPSA completed:
  2863 iterations, 91,616 games. SPRT rejected it:
  `Elo -0.40 +/- 3.20`, 23,210 games, H0 accepted for `[0.00, 5.00]`.
  Combined values were reverted; keep `phase1-lmr` as the accepted Phase 1 head.
- External gauntlet completed:
  - vs Basilisk 1.4.9/defaults: 2000 games, 53.90%, approx +27.16 Elo.
  - vs Rarog 2.0.2 release: 2000 games, 65.03%, approx +107.73 Elo.
  - vs local Rarog 2.1.0 development binary: 2000 games, 64.38%,
    approx +102.78 Elo.
- SPSA configs exist for pruning, LMR, and the rejected combined polish record:
  `config_pruning.json`, `config_lmr.json`, and `config_combined.json`.

### Immediate Sequence

1. Build the default tune baseline:
   ```powershell
   .\tools\build_test.ps1 -Suffix phase1-defaults
   ```
2. Run pruning SPSA:
   ```powershell
   .\tools\setup_spsa.ps1 -ConfigGroup pruning -Iterations 5000
   cd tools\weather-factory
   python main.py
   ```
3. Inspect the final SPSA values:
   - If most values are noisy or snap to implausible boundaries, rerun with
     narrower ranges or more iterations before touching defaults.
   - If only one or two values hit a plausible boundary, widen that one range
     and rerun once.
4. Bake the candidate pruning values into defaults, build a candidate binary,
   run CTest, and record `bench 13`.
5. SPRT pruning candidate vs. `phase1-defaults`:
   ```powershell
   .\tools\sprt.ps1 `
       -EngineA tools\test_engines\basilisk-phase1-pruning-pext-pgo.exe `
       -EngineB tools\test_engines\basilisk-phase1-defaults-pext-pgo.exe `
       -NameA "Phase1Pruning" -NameB "Defaults"
   ```
6. Pruning H1 accepted. Keep the pruning defaults.
7. Build the LMR tuning parent from the current accepted head:
   ```powershell
   .\tools\build_test.ps1 -Suffix phase1-lmr-baseline
   ```
8. Run LMR SPSA on top of the accepted pruning defaults:
   ```powershell
   .\tools\setup_spsa.ps1 -ConfigGroup lmr -EngineSuffix phase1-lmr-baseline -Iterations 5000
   cd tools\weather-factory
   python main.py
   ```
9. LMR H1 accepted. Keep the LMR defaults.
10. Run a short narrowed combined polish SPSA around the accepted values:
   ```powershell
   .\tools\setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
   cd tools\weather-factory
   python main.py
   ```
11. Combined polish H0 accepted. Revert the combined values and keep
   `phase1-lmr`.
12. End Phase 1 with an external gauntlet against at least:
   - Basilisk 1.4.9 release/default head
   - one comparable external engine
   - Rarog if available locally
   ```powershell
   .\tools\gauntlet.ps1 `
       -Engine tools\test_engines\basilisk-phase1-final-pext-pgo.exe `
       -Opponents tools\test_engines\basilisk-phase1-defaults-pext-pgo.exe,D:\chess\engines\rarog-v2.0.2-windows-pext-pgo.exe,D:\code\rarog\target\dist\rarog-v2.1.0-windows-pext-pgo.exe `
       -Name Phase1Final `
       -Games 2000
   ```

### Exposed Phase 1 Parameters

These are already implemented as UCI spin options under `BASILISK_TUNE`.

| Group | UCI option | Default | Range | Step | Meaning |
|---|---:|---:|---:|---:|---|
| Pruning | `RfpCoeff` | 160 | 60..240 | 14 | RFP depth coefficient |
| Pruning | `RfpImproving` | 72 | 0..140 | 12 | RFP improving reduction |
| Pruning | `RazorCoeff` | 243 | 120..500 | 30 | Razoring margin coefficient |
| Pruning | `NullBase` | 3 | 2..6 | 1 | Null-move base reduction |
| Pruning | `NullEvalDiv` | 192 | 80..400 | 24 | Eval surplus divisor in NMP |
| Pruning | `ProbCutMargin` | 189 | 80..360 | 20 | ProbCut beta margin |
| Pruning | `FutilityBase` | 180 | 40..280 | 18 | Move-loop futility base |
| Pruning | `FutilityCoeff` | 128 | 40..200 | 14 | Move-loop futility depth coeff |
| Pruning | `HistPruneCoeff` | 4210 | 1000..7000 | 400 | Quiet history pruning threshold |
| Pruning | `SeePruneCoeff` | 73 | 30..160 | 12 | Bad-capture SEE pruning coeff |
| Pruning | `SingularBetaMult` | 4 | 1..6 | 1 | Singular beta depth multiplier |
| Pruning | `SingularDoubleMargin` | 4 | 0..60 | 8 | Double singular extension margin |
| Pruning | `AspirationDelta` | 19 | 10..60 | 6 | Initial aspiration half-window |
| LMR | `LmrBase` | 60 | 0..150 | 12 | LMR formula base, stored x100 |
| LMR | `LmrDivisor` | 209 | 150..350 | 18 | LMR formula divisor, stored x100 |
| LMR | `LmrHistDiv` | 7830 | 4096..16384 | 1024 | History-to-reduction divisor |
| LMR | `LmrNonPvAdj` | 1 | 0..3 | 1 | Extra reduction at non-PV nodes |
| LMR | `LmrCutNodeAdj` | 0 | 0..3 | 1 | Extra reduction at cut nodes |
| LMR | `LmrTtPvAdj` | 0 | 0..3 | 1 | Reduction decrease near TT/PV |
| LMR | `LmrNotImprovingAdj` | 0 | 0..3 | 1 | Extra reduction when not improving |

### Expected Result

Realistic expectation: +5 to +30 Elo if one or both groups pass. A failed group
is not alarming; it still tells us the next large lever is eval tuning.

---

## 4. Phase 1.5 - Second-Wave Search Tuning (Conditional)

Do this only after Phase 1 has produced at least one accepted candidate, or
after Phase 1 clearly fails and the harness has been proven stable.

Potential second-wave tunables still inline in `src/search.cpp`:

- RFP, LMP, futility, SEE, ProbCut, and singular depth gates.
- Null-move verification depth and reduced verification depth.
- ProbCut reduced search depth.
- LMP formula coefficients.
- History bonus cap and continuation-history scaling.
- Correction-history weight/clamp.
- Qsearch capture-futility margin and threshold-SEE clamp.
- Aspiration reset threshold.

Rules:

- Add only a small coherent group at a time.
- Default-equivalence first: unchanged `bench 13` before tuning.
- Do not expose dozens of additional knobs in one SPSA run.
- If Phase 1 results were weak, skip most of this and go to Phase 2.

---

## 5. Phase 2 - Eval Tuning (Big Lever)

Goal: fit Basilisk's HCE weights to data while preserving the clean
`Evaluator::evaluate(const Board&)` boundary.

Current status: not started. Eval weights are still mostly inline constants in
`src/eval.cpp`; there is no `EvalParams`, no loader, no dataset, and no Texel
tuner.

### Implementation Sequence

1. Add `EvalParams` with defaults matching current `eval.cpp` exactly.
2. Make evaluator construction/read access use `EvalParams` without changing
   behavior. Verify default-equivalence with `bench 13`.
3. Under `BASILISK_TUNE`, add a simple text/JSON loader for eval params. Prefer
   an env var such as `BASILISK_EVAL_FILE` so release UCI remains clean.
4. Build a quiet-position dataset:
   - positions from self-play plus external/tournament PGNs where available;
   - label by game result;
   - deduplicate by hash/FEN;
   - filter positions in check, positions with obvious high-value captures,
     extreme eval positions, illegal FENs, and tablebase-trivial endings;
   - keep a holdout set that is never used for fitting.
5. Build or adapt an external Texel tuner. It should report training and holdout
   loss, write `EvalParams`, and support tuning subsets.
6. Tune in small groups, SPRT-gating each accepted group:
   1. Scalar material values and phase weights.
   2. Mobility weights.
   3. Pawn structure and passed-pawn terms.
   4. King safety, shelter, and storm.
   5. Piece-specific terms: bishop pair, rook files/7th/behind passer, knight
      outposts, trapped bishops.
   6. Threats, hanging pieces, space, tempo, draw scaling.
   7. PSTs last, because they are high-dimensional and need the most data.
7. Run a final global Texel polish over accepted weights, then final SPRT.

### Why PSTs Are Last

The original Claude plan put material + full PSTs first. That is plausible, but
it is also the largest parameter block and easiest to overfit without a large,
clean dataset. A better sequence is to first tune lower-dimensional scalar
terms, prove the Texel pipeline transfers to Elo, then tackle PSTs with stronger
regularization and more data.

### Expected Result

Eval tuning is likely the largest HCE lever remaining. If the dataset is clean
and the SPRT gates are respected, multi-tens of Elo is a realistic target.

---

## 6. Phase 3 - New Features (Only After Tuning Plateaus)

Only start this after Phases 1 and 2 no longer produce accepted candidates.

Likely feature candidates:

- Eval: piece-on-piece threats beyond pawn threats, safe-check king-safety
  terms, weak-square/hole detection, better mobility area, king-ring attack
  quality, more precise endgame scaling.
- Search: correction-history-driven reductions, tuned qsearch checks, more
  selective ProbCut/SEE interaction, refined singular/multi-cut logic, improved
  history aging.
- Time management: tune only if target tournaments use clock controls where
  time management matters. The current SPRT deployment condition `st=0.1` does
  not exercise it.

Per-feature checklist:

- Implement exactly one feature.
- Add or update focused tests.
- Build and run CTest.
- Record `bench 13`.
- Expose new constants under `BASILISK_TUNE` if they need tuning.
- Tune constants.
- SPRT vs. current integration head.
- Keep only H1-accepted features.

---

## 7. Phase 4 - NNUE (Future Project)

NNUE remains the biggest long-term ceiling, but it is out of scope until the HCE
pipeline is exhausted.

Guardrails now:

- Keep `Evaluator::evaluate(const Board&)` as the search boundary.
- Preserve cheap access to piece placement and incremental keys.
- Do not design the Texel/eval loader in a way that search depends on HCE
  internals.
- Reuse the Phase 0 SPRT/gauntlet harness when NNUE work starts.

---

## 8. Release Discipline

After each accepted phase:

- Build fresh PGO assets for local testing (`pext`) and distribution (`avx2` or
  normal release as appropriate).
- Run external gauntlets, not just self-play.
- Scan logs for illegal moves, timeouts, disconnects, crashes, and `bestmove
  0000` from legal positions.
- Update `CHANGELOG.md` and version metadata only after both self-play SPRT and
  external gauntlet are acceptable.

---

## 9. Quick Commands

```powershell
# One-time fresh-clone setup
.\tools\setup_tools.ps1

# Build default tune baseline into tools\test_engines\
.\tools\build_test.ps1 -Suffix phase1-defaults

# Run pruning SPSA
.\tools\setup_spsa.ps1 -ConfigGroup pruning -Iterations 5000
cd tools\weather-factory
python main.py

# Run LMR SPSA
cd ..\..
.\tools\setup_spsa.ps1 -ConfigGroup lmr -Iterations 5000
cd tools\weather-factory
python main.py

# SPRT candidate vs baseline
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-phase1-pruning-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-phase1-defaults-pext-pgo.exe `
    -NameA "Phase1Pruning" -NameB "Defaults"

# Refactor/default-equivalence SPRT, only when needed
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-refactor-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-baseline-pext-pgo.exe `
    -NameA "Refactor" -NameB "Baseline" -Elo0 -3 -Elo1 3
```

---

## 10. Bottom Line

Claude's original direction was mostly right: tune before adding features. The
important correction is that the repo has already completed the harness and
search-parameter plumbing, so the next meaningful work is not another refactor.
It is to run pruning SPSA, SPRT it, run LMR SPSA, SPRT it, then move to a
carefully staged eval-tuning pipeline.
