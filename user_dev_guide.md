# Basilisk Development Workflow Guide

How to drive the improvement plan with an AI coding model and know what to run,
what to report, and when a decision is yours. Read alongside `PLAN.md`, which
holds the technical rationale.

---

## Current Checkpoint

As of this guide, the repo is not at the beginning of the plan.

- Phase 0 harness is complete.
- Phase 1 search-parameter plumbing is complete.
- `SearchParams` exists and is exposed as tune-gated UCI options with
  `-DTUNE=ON`.
- Default-equivalence has been recorded:
  `bench 13 = 4,972,548 nodes`, full CTest passed.
- Phase 1 pruning SPSA was SPRT-accepted:
  `+18.87 +/- 8.81 Elo`, 2930 games, H1 accepted.
- Phase 1 LMR SPSA completed:
  **4034 iterations**, 129,088 games.
- Phase 1 LMR SPRT was accepted:
  `+15.63 +/- 8.02 Elo`, 3714 games, H1 accepted.
- Narrowed combined Phase 1 polish was rejected:
  `-0.40 +/- 3.20 Elo`, 23,210 games, H0 accepted.
- Phase 1 validation completed:
  - vs Basilisk 1.4.9/defaults: 2000 games, 53.90%, approx +27.16 Elo.
- **Phase 1 is complete.** The accepted search-constant head is
  `tools\test_engines\basilisk-phase1-final-pext-pgo.exe` (shipped as 1.5.0).
- A 2026-06 analysis re-baselined the plan: the depth deficit vs Stockfish is
  eval accuracy + search selectivity, not nps and not time management. PLAN.md
  Phases 2-7 were rewritten accordingly (Texel pipeline, search efficiency
  wave, eval features, time management, feature menu, NNUE last).
- The next useful strength step is **Phase 2, Step 2.0: `EvalParams`
  default-equivalence refactor**.

So if you say:

> "Implement the next step of the plan."

the model should not create more Phase 1 SPSA work. It should check the current
state, keep `phase1-final` as the accepted search-constant head, and implement
PLAN.md Step 2.0 (then 2.1, 2.2, ... in order, with the gates listed there).

---

## The Basic Rhythm

Most work is a short ping-pong:

```text
You   -> "Implement next step of the plan."
Model -> Reads PLAN.md, inspects current state, makes only needed edits, and
         tells you exactly what to run.
You   -> Run the command and paste the short result.
Model -> Acts on the result: keep, revert, rerun, or move to the next gate.
```

For SPSA and SPRT, the model cannot honestly guess the result. Your report from
the long-running command is the decision input.

---

## Next Commands

Phase 1 is complete. The next implementation work is Phase 2 (PLAN.md
Steps 2.0-2.5): make evaluation weights tunable without changing default
behavior (`EvalParams`, bench-identical), add the trace-based Texel tuner
target, generate the self-play dataset, then tune in the staged order
(material -> scalars -> king safety -> PSTs -> polish), SPRT-gating each
stage.

For a fresh clone, the historical Phase 1 commands are:

```powershell
# One-time setup on a fresh clone
.\tools\setup_tools.ps1

# Build the original default tune baseline, only needed for pruning from scratch
.\tools\build_test.ps1 -Suffix phase1-defaults

# Configure pruning SPSA
.\tools\setup_spsa.ps1 -ConfigGroup pruning -Iterations 5000

# Run the tuner
cd tools\weather-factory
python main.py
```

Stop SPSA with Ctrl-C when values look stable or the planned run is complete.
State is saved under `tools\weather-factory\tuner\state.json`, so rerunning
`python main.py` resumes. Running `setup_spsa.ps1` again starts a fresh run and
archives old tuner state unless you pass `-Resume`.

After pruning is accepted, build the current accepted head as the LMR parent and
run LMR SPSA from that engine:

```powershell
cd ..\..
.\tools\build_test.ps1 -Suffix phase1-lmr-baseline
.\tools\setup_spsa.ps1 -ConfigGroup lmr -EngineSuffix phase1-lmr-baseline -Iterations 5000
cd tools\weather-factory
python main.py
```

After both pruning and LMR are accepted, run the narrowed combined polish:

```powershell
cd ..\..
.\tools\setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
cd tools\weather-factory
python main.py
```

For the current checkpoint, start Phase 2 by asking:

> "Implement the next step of the plan."

The model should begin with `EvalParams` / eval-default equivalence work, not
another search SPSA run.

---

## What To Report Back

### SPSA Result

Minimal:

> "Pruning SPSA stopped at 5,000 iterations. Final values: RfpCoeff=128,
> RazorCoeff=276, NullEvalDiv=180, AspirationDelta=21, ..."

Helpful extras:

> "HistPruneCoeff sat at the max for the last 1,000 iterations."

The model will decide whether to bake the values, rerun with adjusted ranges, or
discard the noisy group.

### SPRT Result

Minimal:

> "SPRT: H1 accepted after 1,840 games."

or:

> "SPRT: H0 accepted after 2,210 games."

Helpful extras:

> "Score 53.1%, LLR crossed +2.94."

H1 usually means keep the candidate. H0 usually means revert or retry once if
the SPSA run was obviously flawed.

### Bench Result

For pure refactors:

> "bench 13 = 4,972,548 nodes."

For tuned candidates:

> "bench 13 = 4,812,903 nodes."

A changed bench fingerprint is expected after tuning. It is a behavior
fingerprint, not an Elo score.

### Errors

Paste the important error line:

> "fastchess exited with: engine option RfpCoeff not found."

or:

> "bench 13 returned 0 nodes; engine crashed on startup."

The model can diagnose from that.

---

## Decision Points

| Situation | Usual decision |
|---|---|
| SPSA values are stable and plausible | Bake candidate values, build, test, SPRT |
| SPSA values are noisy | Run longer or reduce the group/ranges |
| One plausible value hits a boundary | Widen that range once and rerun |
| Many values hit boundaries | Treat the run as suspect; narrow the problem |
| SPRT accepts H1 | Keep, record, move to next group |
| SPRT accepts H0 | Revert; retry once only if the run was flawed |
| End of Phase 1 | Run fixed-game validation before release work |
| Phase 2 asks for Texel data | Build dataset and holdout before tuning |

Do not keep running repeated SPRTs against tiny changes until one passes. That
is just statistical fishing wearing a little hat.

---

## Phase Progress Tracker

Update this when work completes.

### Phase 0 - Harness

- [x] `tools/setup_tools.ps1` exists for fastchess/weather-factory setup.
- [x] `tools/sprt.ps1` exists and uses repo-local book/default paths.
- [x] `tools/gauntlet.ps1` exists for fixed-game phase-boundary validation.
- [x] `tools/build_test.ps1` builds pext PGO tune binaries into
      `tools\test_engines`.
- [x] `tools/setup_spsa.ps1` writes pruning/LMR weather-factory configs.
- [x] Baseline `bench 13` fingerprint recorded:
      **4,972,548 nodes**.
- [x] Calibration self-vs-self SPRT skipped intentionally.

### Phase 1 - Search Constants

- [x] `SearchParams` struct exists.
- [x] Search params are exposed as UCI spin options under `-DTUNE=ON`.
- [x] Engine passes tuned params into search.
- [x] Default-equivalence verified:
      **bench 13 = 4,972,548 nodes**, 8/8 CTest passed.
- [x] Pruning SPSA run completed.
- [x] Pruning candidate SPRT-confirmed:
      **+18.87 +/- 8.81 Elo**, 2930 games, H1 accepted.
- [x] LMR SPSA run completed:
      **4034 iterations**, 129,088 games.
- [x] LMR candidate SPRT-confirmed:
      **+15.63 +/- 8.02 Elo**, 3714 games, H1 accepted.
- [x] Optional narrowed combined polish SPSA completed:
      **2863 iterations**, 91,616 games.
- [x] Optional narrowed combined polish SPRT-confirmed rejected:
      **-0.40 +/- 3.20 Elo**, 23,210 games, H0 accepted; reverted.
- [x] Phase 1 validation completed:
      - vs Basilisk 1.4.9/defaults: 2000 games, 638 wins, 482 losses,
        880 draws, 53.90%, approx +27.16 Elo.

Phase 1 is complete. Keep `phase1-final` as the accepted head.

### Phase 2 - Eval Tuning (Texel pipeline; PLAN.md Section 4)

- [ ] 2.0 `EvalParams` defaults reproduce current eval (bench identical).
- [ ] 2.1 Tune-only loader + `dumpeval` round-trip works.
- [ ] 2.2 `basilisk-texel` target: trace reconstruction exact on 10k positions.
- [ ] 2.3 Self-play dataset generated, extracted, deduplicated; holdout split
      by game; >= 1.5M train positions.
- [ ] 2.4a Material tuned and SPRT-confirmed (pipeline proof).
- [ ] 2.4b Scalar terms tuned and SPRT-confirmed.
- [ ] 2.4c King safety block tuned and SPRT-confirmed.
- [ ] 2.4d PSTs (+material refit) tuned and SPRT-confirmed.
- [ ] 2.4e Global polish attempted; kept only if SPRT-confirmed.
- [ ] 2.5 2000-game validation vs `phase1-final` recorded.

### Phase 3 - Search Efficiency Wave (PLAN.md Section 5)

- [ ] 3.1 TT-bound eval refinement SPRT verdict recorded.
- [ ] 3.2 History bonus/malus formula SPRT verdict recorded.
- [ ] 3.3 Fractional LMR SPRT verdict recorded.
- [ ] 3.4 TT-capture LMR input SPRT verdict recorded.
- [ ] 3.5 Deeper/shallower re-search SPRT verdict recorded.
- [ ] 3.6 Qsearch quiet checks SPRT verdict recorded.
- [ ] 3.7 Double-extension cap non-regression gate recorded.
- [ ] 3.8 Razoring restriction experiment verdict recorded.
- [ ] 3.9 Second-wave constants exposed, SPSA run, SPRT verdict recorded.

### Phase 4 - Eval Features (PLAN.md Section 6)

- [ ] 4.0 Attack-map refactor with identical bench.
- [ ] 4.1 Threats package retuned and SPRT-confirmed.
- [ ] 4.2 King safety v2 (safe checks, weak ring) retuned and SPRT-confirmed.
- [ ] 4.3 Per-count mobility tables retuned and SPRT-confirmed.
- [ ] 4.4 Pawn-structure refinement retuned and SPRT-confirmed.
- [ ] 4.5 Endgame scaling rules SPRT verdict recorded.

### Phase 5 - Time Management (PLAN.md Section 7)

- [ ] 5.1 Increment-aware budget formula implemented.
- [ ] 5.2 `sprt.ps1 -Tc` support added; both clock-TC SPRTs recorded.
- [ ] 5.3 Optional TM SPSA decision recorded.

### Later

- [ ] Phase 6 feature menu only after Phases 2-5 plateau.
- [ ] Phase 7 NNUE remains future work; eval boundary stays clean.

---

## Common Commands

```powershell
# Build a named pext-PGO+TUNE binary
.\tools\build_test.ps1 -Suffix <name>

# SPRT a gain candidate
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-<candidate>-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-<baseline>-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline"

# SPRT a small/tighter candidate
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-<candidate>-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-<baseline>-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -Elo1 3

# Refactor/default-equivalence SPRT, only when needed
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-refactor-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-baseline-pext-pgo.exe `
    -NameA "Refactor" -NameB "Baseline" -Elo0 -3 -Elo1 3

# Phase-boundary fixed-game validation
.\tools\gauntlet.ps1 `
    -Engine tools\test_engines\basilisk-phase1-final-pext-pgo.exe `
    -Opponents tools\test_engines\basilisk-phase1-defaults-pext-pgo.exe `
    -Name Phase1Final `
    -Games 2000

# Configure SPSA
.\tools\setup_spsa.ps1 -ConfigGroup pruning -Iterations 5000
.\tools\setup_spsa.ps1 -ConfigGroup lmr -Iterations 5000
.\tools\setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000

# Run/resume SPSA
cd tools\weather-factory
python main.py
```

Bench is best run interactively:

```text
.\build\release-pext\basilisk.exe
bench 13
quit
```

---

## Why Tuning Options Exist

weather-factory can only perturb the engine through UCI. For example, it sends:

```text
setoption name RfpCoeff value 128
```

That is why search constants are UCI options in tune builds. They are hidden in
normal release builds through the `BASILISK_TUNE` compile definition.

Before any public release, verify a non-tune build does not expose the tuning
option list.

---

## Ground Rules

- Do not accept a tuned value set without SPRT.
- Do not interpret lower or higher node count as strength.
- Do not bundle feature work with tuning defaults.
- Do not skip fixed-game validation at phase boundaries.
- Do not start Phase 3 features while Phase 1 or Phase 2 still has obvious
  tuning work left.
- Keep `Evaluator::evaluate(const Board&)` as the boundary between search and
  evaluation.

The process is the strength engine here: tune, test, keep only what survives.
