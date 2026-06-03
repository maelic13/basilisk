# Basilisk Development Workflow Guide

How to drive the improvement plan with Claude (or any AI model) and know exactly
what to say, what to run, and when a decision is yours to make. Read alongside
`PLAN.md`, which holds all the technical detail.

---

## The basic rhythm

Every step is a ping-pong between you and the model:

```
You  →  "Implement next step of the plan."
Model→  Writes code, verifies build + bench, tells you exactly what to run.
You  →  Run the command, come back with the result.
Model→  Acts on the result. Either commits and moves on, or flags a decision.
```

Most iterations cost you **one message**. You report the result; the model
handles everything else.

---

## How to start a session

Opening message when continuing work:

> "Implement next step of the plan."

The model reads `PLAN.md`, checks the current branch + progress tracker below,
and knows where we left off. You do not need to re-explain context.

For a specific phase or step instead of "next":

> "Implement Phase 1 step 2 — expose the search constants as UCI options."
> "Start the Phase 2 king-safety Texel tuning."

---

## After the model writes code — your turn

The model ends its response with an explicit instruction, e.g.:

> **Build and test:**
> `.\tools\build_test.ps1 -Suffix phase1`
> `.\tools\sprt.ps1 -EngineA ... -EngineB ... -NameA "Phase1" -NameB "Head"`

Run those, then come back with one of these short reports:

### SPRT result
> "SPRT: **H1 accepted** after 1,840 games."
> "SPRT: **H0 accepted** after 2,210 games."

That is all the model needs. It will commit (H1) or discard (H0) and move on.

### SPSA result
After weather-factory finishes (or you stop it at a reasonable point):
> "SPSA done (~8,000 iterations). Tuned: RfpCoeff=128, RazorCoeff=276,
> NullEvalDiv=180, AspirationDelta=21."

The model bakes those in as new defaults, builds a test binary, and gives you the
SPRT command to confirm them.

### Texel result (Phase 2)
> "Texel pass on king-safety done. New weights written to king_safety.txt,
> training loss 0.0712 → 0.0689."

The model folds the weights into the defaults and gives you the SPRT command.

### Bench fingerprint check
When the model asks you to verify a refactor didn't change behavior:
> "bench 13 = **X nodes** ✓" (matches baseline — safe refactor)
> "bench 13 = **Y nodes**"   (changed — expected for real re-fits)

---

## Decision points — when the model will stop and ask you

| Situation | The question | Typical answer |
|---|---|---|
| **SPRT returns H0** | Discard and move on, or try a second SPSA pass? | Usually: discard and move on |
| **SPSA converges to a range boundary** | Accept the outlier, widen the range and re-run, or discard? | Widen + re-run if plausible; discard if it looks wrong |
| **End of a phase** | Run the external gauntlet before moving on? | Yes, always recommended |
| **Phase 3 (new features)** | Start adding features, or keep tuning? | Only if Phases 1–2 have plateaued |
| **Phase 4 (NNUE)** | Begin the NNUE project? | Far-future; not until HCE tuning is exhausted |

Just answer in plain English; the model proceeds accordingly.

---

## Prerequisites — complete these before Phase 1

Phase 0 tooling must be created and verified once:

- [x] **`tools/` created** — scripts, configs, opening book, and `.gitkeep`
      sentinels all committed. Self-contained: run `.\tools\setup_tools.ps1`
      on a fresh clone to download fastchess + clone weather-factory.
- [x] **fastchess** at `tools\bin\fastchess.exe` ✓
- [x] **Baseline `bench 13` fingerprint recorded:** `4,972,548 nodes`
      (release-pext, MSYS2 Clang). Recorded in `PLAN.md` §3.
- Calibration self-vs-self SPRT **skipped** — fastchess + weather-factory is
  proven by rarog. Self-vs-self with symmetric bounds needs thousands of games
  for a formal H0 acceptance; the cost exceeds the benefit here.

**Phase 0 is complete. Phase 1 steps 1–3 are also complete (SearchParams struct,
UCI options, default-equivalence verified). SPSA is wired and ready to run.**

The external Texel tuner is only needed from Phase 2.

---

## Quick command reference

```powershell
# Fresh-clone one-time setup (downloads fastchess, clones weather-factory)
.\tools\setup_tools.ps1

# Build a named pext-PGO+TUNE test binary into tools\test_engines\
.\tools\build_test.ps1 -Suffix <name>

# SPRT — test a gain (default H0=0, H1=5)
.\tools\sprt.ps1 -EngineA <new>.exe -EngineB <head>.exe -NameA "X" -NameB "Head"

# SPRT — small feature (tighter bound, faster conclusion)
.\tools\sprt.ps1 -EngineA <new>.exe -EngineB <head>.exe -NameA "X" -NameB "Head" -Elo1 3

# SPRT — refactor / default-equivalence (symmetric bounds, H0 in ~1-3k games)
.\tools\sprt.ps1 -EngineA <refactor>.exe -EngineB <head>.exe `
    -NameA "Refactor" -NameB "Head" -Elo0 -3 -Elo1 3

# SPSA tuning — see tools/spsa_configs/README.md
.\tools\setup_spsa.ps1          # wire up weather-factory for the run
cd tools\weather-factory; python main.py

# Bench fingerprint — run the binary interactively (piping is unreliable):
#   .\build\release-pext\basilisk.exe
#   bench 13
#   quit
# Record the baseline value in PLAN.md §3.
```

---

## Phase progress tracker

Update as each step completes.

### Phase 0 — Harness
- [x] `tools/sprt.ps1` adapted from Rarog
- [x] `tools/build_test.ps1` (CMake `pgo` target) adapted
- [x] `tools/spsa_configs/` (configs + setup) adapted with Basilisk option names
- [x] fastchess at `tools\bin\fastchess.exe` ✓
- [x] Baseline `bench 13` fingerprint recorded: **4,972,548 nodes** (release-pext, MSYS2 Clang)
- [x] ~~Calibration test~~ skipped — harness proven by rarog; self-vs-self takes thousands of games for H0

### Phase 1 — Expose + SPSA-tune search constants
- [x] `SearchParams` struct, defaults == current inline literals
- [x] §1a constants exposed as `tune`-gated UCI spin options (`-DTUNE=ON`)
- [x] Default-equivalence verified — bench 13 = **4,972,548 nodes** ✓, 8/8 tests pass ✓
- [ ] SPSA group: pruning / margins
- [ ] SPSA group: LMR terms
- [ ] SPRT confirmation of tuned set vs. 1.4.9 head
- [ ] (Tune flag already gated — release build shows 0 tune options ✓)

### Phase 2 — Texel-tune eval
- [ ] `EvalParams` struct + file loader (`tune` flag) — default-equivalent
- [ ] Texel dataset built (quiet positions, result-labeled)
- [ ] External Texel tuner working
- [ ] Material + PST tuned + SPRT confirmed
- [ ] Mobility tuned + SPRT confirmed
- [ ] King safety tuned + SPRT confirmed
- [ ] Passed pawns tuned + SPRT confirmed
- [ ] Piece terms (bishop pair / rook / knight) tuned + SPRT confirmed
- [ ] Threats / tempo / draw-scaling tuned + SPRT confirmed
- [ ] Global Texel re-pass + final SPRT

### Phase 3 — New features (only if tuning plateaus)
- [ ] (deferred — list items as they are chosen)

### Phase 4 — NNUE (far-future, not scheduled)
- [ ] Eval interface kept clean throughout Phases 1–3 (ongoing guardrail)

### Release gates (after each phase)
- [ ] External gauntlet vs. 1.4.9, Stockfish, Rarog 2.x
- [ ] CHANGELOG updated
- [ ] Version bumped, PGO asset rebuilt (`pext` + `avx2`)

---

## What makes a good result report

**Minimal (always sufficient):**
> "H1 accepted after 2,100 games."

**Helpful extras:**
> "H1 accepted after 2,100 games. Score 53.1%. LLR crossed +2.94."

**For SPSA:**
> "Stopped at 6,000 iterations. Final: RfpCoeff=128 RazorCoeff=276
> FutilityBase=140 SeePruneCoeff=72 AspirationDelta=22."

**If something looks wrong:**
> "fastchess exited immediately with: [paste error]"
> "bench 13 returned 0 nodes — engine crashed on startup."

The model diagnoses and fixes; you don't need to understand the error.

---

## Why constants become UCI options (and when to remove them)

weather-factory (the SPSA driver) has no interface to the engine other than UCI.
To perturb `RfpCoeff`, it sends `setoption name RfpCoeff value 128` before each
mini-match. UCI options are the only mechanism.

This is standard practice: Stockfish, Ethereal, and most modern engines expose
constants during development behind a compile-time flag, then strip them from
release builds. Basilisk does the same via the `tune` CMake flag.

**During development:** build with `tune` on so the options exist.
**Before any public release:** build with `tune` off so production binaries don't
show a cluttered UCI option list to GUIs.

---

## Ground rules (keep the model honest)

- **Never accept a change without a bench-13 check first.** For a pure refactor
  the fingerprint must be unchanged; for a real re-fit it will change (record
  it). An unchanged fingerprint alone proves a refactor is equivalent — a full
  SPRT H0 is not required (but if you run one, use `-Elo0 -3 -Elo1 3`).
- **Never skip the SPRT gate.** If the model says "this is clearly good, let's
  skip the test" — refuse. The whole point is that "clearly good" changes can
  still lose Elo.
- **SPSA/Texel propose; SPRT decides.** Tuned values are *candidates*. The
  `st=0.1` SPRT is the final word.
- **One change per commit.** If the model bundles two, ask it to split them.
- **Run the external gauntlet at the end of each phase**, not just per feature —
  self-play over-fits; external opponents catch it.
- **Protect the eval interface.** Don't let a tuning refactor weld eval internals
  into search — that would foreclose a future NNUE swap (`PLAN.md` §7).
