# Basilisk Development Workflow Guide

How to drive the improvement plan with an AI coding model and know what to run,
what to report, and when a decision is yours. Read alongside `PLAN.md`, which
holds the technical rationale.

---

## Current Checkpoint

As of this guide, the repo is not at the beginning of the plan.

- Phase 0 harness is complete.
- SPSA and SPRT now share the representative clock TC: `tc=3+0.03`.
  Use `.\tools\sprt.ps1 -TC "10+0.1"` for LTC confirmation and
  `-MoveTime 0.1` only for the optional old fixed-movetime sanity check.
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
- Harness-corrected retry audit: retry the combined polish only later as part
  of Phase 6.9 after eval tuning; keep the post-LMR deeper re-search in
  Phase 6.5 and gate it with the clock harness plus LTC if it passes.
- Phase 1 validation completed:
  - vs Basilisk 1.4.9/defaults: 2000 games, 53.90%, approx +27.16 Elo.
- **Phase 1 is complete.** The accepted search-constant head is
  `tools\test_engines\basilisk-phase1-final-pext-pgo.exe` (shipped as 1.5.0).
- A 2026-06 analysis re-baselined the plan: the depth deficit vs Stockfish is
  eval accuracy + search selectivity, not nps and not time management. PLAN.md
  Phases 2-7 were rewritten accordingly (Texel pipeline, search efficiency
  wave, eval features, time management, feature menu, NNUE last).
- **Step 2.0 is complete.** `src/EvalParams.h` created: `struct EvalParams`
  holds all ~900 tunable eval weights with defaults identical to the old inline
  constants; `EVAL_PARAM_LIST` X-macro enumerates every group. `eval.cpp`
  updated to read from `g_eval_params`; `init_eval_tables` takes
  `const EvalParams&`. Bench fingerprint on `release-pext` (non-PGO) on this
  machine: **4,283,684 nodes** (the recorded 4,972,548 was the PGO build;
  4,283,684 is the correct non-PGO baseline for refactor verification).
  8/8 CTest passed.
- **Step 2.1 is complete.** Under `BASILISK_TUNE`: `load_eval_file_if_set()`
  reads `BASILISK_EVAL_FILE` env-var path on startup; `run_dumpeval()` prints
  all 900 parameters in `name index value` format. `dumpeval` console command
  added to UciLoop. Round-trip verified byte-identical (dump → load → dump).
  Release build ignores both; bench fingerprint unchanged: **4,283,684 nodes**,
  8/8 CTest passed.
- **Step 2.2 is complete.** CMake `TEXEL=ON` option added; `basilisk-texel`
  target built from `src/eval.cpp` (TEXEL_TRACE+BASILISK_TUNE) +
  `tools/texel/tuner.cpp`. `EvalTrace` struct holds `int16_t mg[900]` and
  `eg[900]` flat count arrays; `reconstruct()` computes tapered dot product.
  All 900 linear eval parameters instrumented with `TR_MG`/`TR_EG`/`TR_BOTH`
  macros; pawn cache bypassed under `TEXEL_TRACE`; king safety traced as
  one-hot `SafetyTable` lookup. Acceptance test: `basilisk-texel --verify`
  reconstructs exactly on all positions (OCB/50-move/mate-drive absorbed into
  `rest`). Release bench **4,283,684 nodes**, 8/8 CTest passed.
- **Step 2.3 is complete.** `tools/datagen.ps1` and `tools/texel/extract.py`
  created. datagen.ps1 runs fastchess self-play (nodes=8000, 30k rounds = 60k
  games) writing `tools/texel/data/selfplay.pgn`. extract.py uses python-chess
  to skip first 16 plies + last 6 plies, drops in-check and capture/promotion
  moves, samples ≤12 positions per game, deduplicates by FEN (4 fields), splits
  by game (5% holdout), writes `train.csv` / `holdout.csv`. Round-trip
  pipeline verified: extracted positions pass `basilisk-texel --verify`.
- **Where the plan stands (restructured 2026-06-18, PLAN.md §0.5):** Phase 2
  texel infra (2.0–2.3) and the cheap, structure-independent scalar fits are
  **done/accepted** (material +29, mobility +8.8, passers +16.6, pawn-structure
  +30.7, hanging baked; rooks rejected). The plan was **reordered** so eval
  *structure* is built before the heavy eval tuning and the search SPSA. The
  remaining old 2.4 tuning (king safety 2.4c, PSTs 2.4d, polish 2.4e, plus the
  untuned pawn-threats) is **deferred into the new Phase 4**.
- **`1.6.0` shipped** (commit `Version 1.6.0`); the repo is on branch `v1.7.0`.
- **Phase 3 (eval structure build-out, Steps 3.0–3.11) is ✅ COMPLETE
  (2026-06-24…27).** The whole enlarged HCE structure is in place, seeded inert
  and Texel-traceable; bench fingerprint is now **`4,168,590`** (re-baselined by
  the two behaviour-changing steps). Per-step gates were green throughout
  (bench identity for the inert steps, 9/9 CTest incl. `test_endgames` 18/18,
  texel `--verify` exact 8,598/8,598, `test_eval` symmetry).
  - **3.0–3.4, 3.6–3.9 behaviour-identical** (seeded-inert structure; bench
    unchanged at each step): attack maps + pins, threats, king-safety v2 danger
    model, per-count mobility tables, pawn/passer refinement, space + winnable
    coupling, small positional terms, material imbalance, HCE survey adds.
  - **3.5 (endgame knowledge) behaviour-changing** — exact KPK bitbase, KBNK
    corner mate, KNNK/no-pawn-minor/OCB draw scaling. Bench → `4,377,437`.
    **SPRT −4.17 ± 5.20 (KEPT)** = the per-node NPS tax (EPD-proven correct), not
    a chess bug; recovered by 3.10 + 3.11 below.
  - **3.10 `apply_endgame` guard** (behaviour-identical): skip the endgame census
    in the opening/middlegame — removes the cost behind 3.5's −4.
  - **3.11 lazy eval** (behaviour-changing): skip the heavy positional block when
    the material/PST margin is decisive (`LAZY_MARGIN=700`; `apply_endgame` runs
    on both paths so KBNK/KPK survive). Bench → `4,168,590`. **SPRT +16.64 ± 7.03
    Elo, LOS 100%, H1** (LazyOn vs LazyOff) — a clear net gain that recovers the
    whole Phase-3 NPS tax and then some.
- **The next implementation work is Phase 4 — the eval data-fit campaign
  (+80–160 Elo)**, starting at **Step 4.0** (build the finite-difference
  `--tune-kingsafety` re-eval path — the biggest lever — plus the feature-support
  / bucketed-holdout tuner-readiness gate). None of the Phase-3 structure is tuned
  yet; Phase 4 is where it activates. Ships as release **1.7.0**.

### The program in one table (overview · model picker · Elo)

Read PLAN.md §0.5 for *why* this order: build all eval structure first
(Phase 3, no games), fit the eval once (Phase 4), harden time management
(Phase 5), then run the search SPSA once last (Phase 6). Texel fits are cheap;
SPSA/SPRT games are the conserved resource.

| Phase | What | Gate | Model(s) | Elo |
|---|---|---|---|---|
| **2** (done) | Texel infra + cheap scalar fits | SPRT per subgroup | Sonnet 4.6 medium | banked |
| **2.9** Robustness quick win (done) | time-safety floor — shipped in `1.6.0` | `bench 13` identical; `t=`→0 in a gauntlet | Sonnet 4.6 medium | reliability + a few |
| **3** Eval structure build-out (**✅ COMPLETE** — 3.0–3.11; lazy eval +16.6 Elo H1) | attack maps, threats, KS v2, mobility tables, pawn refine, endgames (+KBNK), space/winnable, small terms | `bench 13` identical + `--verify` + CTest (**no games**; **3.5 is behaviour-changing → endgame EPD suite + SPRT**) | **Opus 4.8 high** (KS/threats/endgames/mobility); Opus 4.8 medium (attack maps/pawns); Sonnet 4.6 medium (small terms) | 0 direct (enabler) |
| **4** Eval data-fit completion | one staged Texel campaign over the full structure; KS/threats/mobility/minors/**PST+material**/polish | SPRT per stage | Sonnet 4.6 medium (driving) | **+80–160** |
| **5** Time management hardening + tuning (**promoted 2026-06-29**) | clock-at-`go` latency fix, anti-overshoot poll, GUI-robust reserve, root fail-low extension, **TM-constant SPSA**, cross-TC validation | bench-identical (robustness) + SPRT/SPSA + `t=0` at bullet→slow | Opus 4.8 medium (latency/fail-low/diagnosis); Sonnet 4.6 medium (granularity/reserve/SPSA) | **reliability + +8–25** |
| **6** Search-efficiency wave | TT-bound eval, history split, fractional LMR, deeper re-search, qsearch checks, **wave2 SPSA last** | SPRT per item; SPSA once | Sonnet 4.6 medium; dense ports Codex 5.5 medium / GPT-5.5 high | **+20–50** |
| **7** Non-NNUE ceiling: eval-refresh grind | regen self-play with the stronger head, joint refit (1–3 cycles), bank tuning-maturity Elo | SPRT + boundary gauntlet per cycle | Sonnet 4.6 medium | **+10–40** (cycle 1) |
| **8 / 9** Feature menu / NNUE | plateau menu / ceiling | — | — | — |

Per-step models are on each PLAN step header. **SPRT is the only verdict.**

> **Gauntlet validation (2026-06-19, 35k games @ `tc=3+0.03`):** Basilisk 1.5.1
> (dev, partial scalar tuning only) was **2nd of 9, +54 over 1.5.0**, tied with
> "SF-capped-2700" — the eval lever is confirmed, with KS/PST/structure still to
> come. **Two cautions:** (1) SF `UCI_Elo` is calibrated for 120s+1s/CCRL-40/4, so
> it is **not** a true anchor at this fast TC — for a real CCRL number run a
> slower-TC gauntlet that includes **Critter 1.6a** (it forfeited at 3+0.03).
> (2) **1.5.1 lost 65 games on time** (vs 1.5.0's 18) — add a hard time-safety
> floor to `compute_time_limit` **now**; forfeits contaminate every gauntlet.

So if you say *"implement the next step,"* the model should know: **`1.6.0` has
shipped** (Phase 2 eval scalars + the Rarog-ported Stockfish-style clock TM with
the time-safety reserve), the repo is on branch `v1.7.0`, and **Phase 3 is
underway** — **Steps 3.0–3.6 are done** (2026-06-24/25), so the next step is
**Phase 3.7 (small positional terms)**. Steps 3.0–3.4 and 3.6 were bench-identical;
**3.5 (endgame knowledge) is behaviour-changing** (`bench 13` → 4,377,437,
gated by the endgame EPD suite + a maintainer-run SPRT, not bench identity).

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

**Phase 2.9.2 — done (2026-06-19).** `tools/gauntlet.ps1` now has a `-TC`/
`-MoveTime`/`-TimeMargin` parameter set mirroring `sprt.ps1`: clock `tc=10+0.1`
by default (the phase-boundary condition from §10), fixed `st=…` only when
`-MoveTime` is given, and the console banner reports the real TC used. Pure
tooling, no engine change.

**Phase 5 Step 5.1 / 5.1b (TM foundation; old labels 6.1/6.1b) — done (2026-06-20).** A first LittleBlitzer probe
(521 games) of the original Step 2.9.1 patch (a reserve clamp bolted onto the
old tiered-percentage formula) showed an oh1/oh10 tpm gap and a lone forfeit on
the aggressive `Move Overhead=1` variant, traced to the *old formula* being
tighter to the margin than Rarog's already-rewritten SF-style one. Rather than
re-tune that formula's margins, `compute_time_limit`'s clock path is now a
**direct port of Rarog's Phase 2.2 Stockfish-style rewrite**
(`D:\code\rarog\src\time_manager.rs`) — logT-based `optConst`/`maxConst`,
ply-aware sudden-death and explicit-movestogo branches, same constants, no new
`BASILISK_TUNE` knobs (matching Rarog, which exposes none either). The 2.9.1
reserve is folded in using Rarog's exact mechanics: `hard_ceiling = raw_time -
2*overhead` (Basilisk's original patch used 3x effective margin from double
overhead subtraction; now it's 2x, identical to Rarog). `compute_time_limit`
also gained a `game_ply` parameter, threaded from both call sites
(`Searcher::search` and the ponderhit branch of `check_stop`).

`bench 13` unchanged (`4,033,379` nodes, non-PGO release-pext — TM doesn't
touch fixed-depth bench), 8/8 CTest passed. Manually verified `wtime` from
60000 down to 0 (`winc=30`): smooth degradation, no hangs; at the gauntlet TC
(`wtime=3000 winc=30`) budgets land ~68-78ms, in the same range as the whole
opponent field's tpm (65.6-70.0) from the prior LittleBlitzer snapshot.

**LittleBlitzer validation — PASSED (2026-06-20).** Full 35,000-game overnight
pool at `tc=3+0.03`, default Move Overhead: **`t=0` for Basilisk 1.5.1-sftm**
(and every other engine in the pool — confirms the harness itself is sound, not
just this engine). Rating 2696.8, 62.7% score, 2nd of 5 — essentially tied with
Stockfish-2700-capped, well clear of Rarog 2.1.0 (2620.0) and SF-2600/2500.
This conclusively answers the time-safety question; no remaining doubt that the
Step 6.1/6.1b fix works.

**Skippable, not recommended: a second-harness forfeit check.** Step 6.1/6.1b
is a forfeit fix, not a strength change — its true Elo delta vs `phase1-final`
is expected to sit near 0, which is the *worst* case for an SPRT (`elo0=0
elo1=3` converges slowest exactly when the true value sits on the H0 boundary;
it can run tens of thousands of games without crossing either bound — do
**not** use `sprt.ps1` here). If you still want a second-harness sanity check
per Rarog finding #3 (LB can mismeasure some engines), use `gauntlet.ps1`
(fixed games, no stopping rule) and read the `t=` count only — ignore the
score:

```powershell
.\tools\gauntlet.ps1 `
    -Engine tools\test_engines\basilisk-phase6-sf-tm-pext-pgo.exe `
    -Opponents tools\test_engines\basilisk-phase1-final-pext-pgo.exe `
    -Name Phase6SfTm -TC "10+0.1" -Games 1000
```

Given the LB result already showed `t=0` for **every** engine in the pool
(not just Basilisk, so it's not a Basilisk-specific harness blind spot), this
adds little. Skip straight to the release unless you want extra peace of mind.

**Then 2.9.3** (release `1.6.0`). **Then** the eval work
begins with **Phase 3.0 — the attack-map substrate** (PLAN.md §6): a
behaviour-identical refactor that computes `attacked_by[color][pt]`,
`attacked[color]`, `attacked2[color]` once per `evaluate()` and routes
mobility, king safety, and hanging through them. **No games** — the gate is the
bench fingerprint.

Ask the model:

> "Build a PGO test binary for the time-safety fix and run a clock-TC gauntlet
> vs phase1-final to confirm time forfeits drop to ~0."

Then verify nothing changed:

```powershell
# Rebuild and check the refactor is behaviour-preserving.
cmake --build build\release-pext --target basilisk
.\build\release-pext\basilisk.exe   # then: bench 13   (expect the recorded baseline)
# And keep --verify exact:
cmake --build build\release-pext --target basilisk-texel
.\build\release-pext\basilisk-texel.exe --verify tools\texel\data\holdout.csv
```

Most of Phase 3 is the same shape: implement a structural step with new
sub-terms **seeded inert** → confirm `bench 13` unchanged + `--verify` exact +
CTest → move on. **The games (SPRT) only start in Phase 4**, when the staged
Texel campaign activates the new terms.

The **texel toolchain and datasets already exist** (Phase 2): `tools/datagen.ps1`,
`tools/texel/{sample_fens,extract,import_beast}.py`, the `basilisk-texel`
target, and `train.csv`/`holdout.csv` (self-play) plus `beast_sf_*.csv`
(Stockfish-labelled). Reuse them in Phase 4 — no new data needed unless holdout
loss stalls.

This creates about 1.52M train / 80k holdout Stockfish-target positions in
`FEN;target` format. The importer treats the raw Stockfish WDL target as
side-to-move perspective and converts black-to-move rows to white perspective,
which is what `basilisk-texel` trains against. Keep it separate from the
self-play data when reporting results.

After the material tune finishes, inspect the printed material deltas before
baking them into `EvalParams.h`; plausible values still need SPRT before they
are accepted.

For the full Beast import (`beast_sf_all_train.csv` /
`beast_sf_all_holdout.csv`), add `--max-positions 0` to the tune command.
Without it, `basilisk-texel` intentionally loads only 5M positions from each
split to protect later, wider tune groups from accidental huge memory use.

After the failed SF-supervised material SPRT, use Beast FENs as start positions
only and let Basilisk self-play generate the labels:

```powershell
python tools\texel\sample_fens.py A:\Chess\Beast\data\txt\positions.txt `
  --out tools\texel\data\beast_seed.epd --count 100000

.\tools\datagen.ps1 -Suffix phase2-base `
  -Book tools\texel\data\beast_seed.epd -BookFormat epd `
  -OutputPgn tools\texel\data\selfplay_beast_seed.pgn `
  -Rounds 75000 -Nodes 8000

python tools\texel\extract.py tools\texel\data\selfplay_beast_seed.pgn `
  --train train_beast_seed.csv --holdout holdout_beast_seed.csv `
  --skip-start 0 --max-per-game 12
```

The Phase 2 scalar tuning is **complete** (§0.5): material, mobility, passers,
pawn-structure, and hanging are baked into `EvalParams.h`; rooks was rejected.
Do **not** continue the old 2.4 subgroups (king safety, PSTs, threats, minors) —
they are deferred into the Phase 4 campaign that runs *after* the Phase 3
structure exists.

---

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

The narrowed combined polish already ran and failed under the old split
harness. Do **not** run it as a Phase 1 continuation. Its one justified retry
belongs to Phase 6.9, after the eval refit, using the unified clock
harness:

```powershell
cd ..\..
.\tools\setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
cd tools\weather-factory
python main.py
```

Treat that command as a historical reference until Phase 6.9 rewrites/extends
the wave2 SPSA config.

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

### Texel / Tuner Result (Phase 4)

Minimal:

> "Threats fit: train loss 0.0981 → 0.0972, holdout 0.0986 → 0.0979."

Helpful extras (these are what the model acts on — loss alone is never the
verdict, SPRT is):

> "Bucket losses: all held or improved except king-attack (0.094 → 0.097);
> feature-support flagged `threat_by_minor[Q]` with only 31 observations;
> the new safe-check unit weights fit with plausible signs."

A regressing bucket, a sparse-feature warning, or an implausible sign/shape is a
stop-and-investigate signal **before** spending SPRT games, not after.

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

Full per-candidate record (commands, intermediate holdout losses, build
fingerprints, PGN paths) lives in **PLAN.md §4** — this is the outcome summary.

- [x] 2.0–2.3 infra: `EvalParams` (bench-identical, **4,283,684** non-PGO
      baseline); tune-only loader + `dumpeval` (900 params, round-trip exact,
      hidden in release); `basilisk-texel` (TEXEL=ON, 900-param trace, `--verify`
      exact incl. frozen-term edges); `datagen.ps1` + `extract.py`; Beast/SF
      import verified (1,520,109 train / 79,891 holdout).
- [x] 2.4a material — **SF-WDL labels rejected** (`-16.56 ± 9.56`, 2,982 games),
      so pivoted to Basilisk-labeled Beast-seed self-play; that candidate
      **accepted** `+29.05 ± 11.36` (1,930 games). Baked
      `mg_val={85,323,363,514,1085}`, `eg_val={96,310,319,557,996}`.
- [x] 2.4b — tuner gained scalar subgroups with sign/shape clamps + best-epoch
      restore (the broad all-scalar pass was **not baked**: implausible
      signs/shapes). Per-subgroup SPRT outcomes:
  | Subgroup | SPRT verdict (games) | Baked values |
  |---|---|---|
  | mobility | **accepted** `+8.77 ± 5.68` (7,288) | `mob_mg={0,0,5,5,1,2,0}`, `mob_eg={0,0,5,7,7,12,0}` |
  | passers | **accepted** `+16.57 ± 8.28` (3,462) | `passed_mg[1..6]={8,8,8,25,90,90}`, `passed_eg[1..6]={10,10,53,84,106,124}`, support/cand/free-EG ≈0, `pass_safe_eg=16`, `prox_base=11` |
  | pawnstruct | **accepted** `+30.74 ± 11.76` (1,836) | `doubled=(-2,-8)`, `isolated=(-9,-17)`, `connected=(18,1)`, `backward=(0,-12)` |
  | hanging | baked, **SPRT pending** | `hang_pen={0,0,22,39,33,36,0}` |
  | rooks | **rejected** `+3.13 ± 4.74` (10,666; inconclusive, awkward shape) — reverted | — |
- [x] 2.4b cheap structure-independent scalars done (mobility/passers/pawnstruct/hanging accepted; rooks rejected). **Stop 2.4 here** (§0.5).
- [x] 2.4b `threats`/`minors` pawn-threat scalars — **DEFERRED to Phase 4.2/4.5** (Phase 3 rewrites the threats term).
- [x] 2.4c King safety block — **DEFERRED to Phase 4.3** (Phase 3.2 rewrites king safety first; do not tune the toy model).
- [x] 2.4d PSTs (+material refit) — **DEFERRED to Phase 4.7**.
- [x] 2.4e Global polish — **DEFERRED to Phase 4.8**.
- [x] 2.5 2000-game validation vs `phase1-final` — **moved to the Phase 4 boundary**.

### Phase 2.9 - Robustness Quick Win (DONE — shipped in 1.6.0; PLAN.md §4.9)

- [x] 2.9.2 Add `-TC`/`-MoveTime`/`-TimeMargin` to `tools/gauntlet.ps1` (clock `tc=10+0.1` default, mirrors `sprt.ps1`; banner fixed). Pure tooling, no engine change, parse-verified.
- [x] 2.9.1 *(superseded by Phase 5 Step 5.1/5.1b, old labels 6.1/6.1b, 2026-06-20 — see below)* — original standalone reserve patch on the old formula; replaced rather than re-tuned after a LittleBlitzer probe showed it binding more than Rarog's equivalent.
- [x] **2.9.3 Release `1.6.0` prep done (2026-06-20)** — version bumped in `src/Constants.h` + `CMakeLists.txt`, `CHANGELOG.md` entry added, clean non-tune release build verified (options hidden, `bench 13` matches, 8/8 CTest), dist binaries built, committed. **Pending (user-only): create tag `v1.6.0` and push** — the model never tags or pushes.
- [ ] (calibration, anytime) Slower-TC gauntlet with a CCRL-rated anchor (Critter 1.6a / Fruit 2.1), pin with `ordo -A "<name>" -a <ccrl>` (PLAN.md §10).

### Phase 3 - Eval Structure Build-Out (PLAN.md §6; EXECUTE FIRST — bench-identical, no games)

- [x] **3.0 Attack-map substrate — DONE 2026-06-24.** `evaluate()` now builds `attacked_by[c][pt]`, `attacked[c]` (== `is_attacked_by` union), `attacked2[c]`, `king_zone[c]` once; mobility + king-attacker pressure folded into ONE knight/slider sweep (was two); hanging-piece detection reads `attacked[]` instead of `is_attacked_by()`. `attacked_by`/`attacked2` seeded for 3.1/3.2. **blockers/pin masks deferred to 3.2/3.3** (no consumer in 3.0 → dead NPS now; add in the same sweep when consumed). Gates: `bench 13 = 4,033,379` (identical), 8/8 CTest, `--verify` exact (8598/8598). — Opus 4.8 medium
- [x] **3.1 Threats package — DONE 2026-06-25.** SF-style threats added to `evaluate()` on the Step-3.0 maps, all weights **seeded 0** (bench unchanged): `threat_by_minor/rook[7]` (by attacked type), threat-by-king, hanging refinement (`threat_hanging` + `weak_queen_prot`), `restricted`, `threat_push`. Uses the SF `strongly_protected`/`weak` decomposition; existing flat pawn-threat + `hang_pen` stay active until the Phase-4.2 swap; overloaded-defender deferred. Gates: `bench 13 = 4,033,379` (identical), 8/8 CTest, `--verify` exact (8598/8598). — Opus 4.8 high
- [x] **3.2 King safety v2 — DONE 2026-06-25.** Rebuilt the KS finalization as a full danger model feeding the `attack_units → safety_table` funnel, all inputs **seeded 0** (no-queen relief re-expressed as `ks_noqueen_num/den=2/3`, byte-identical): `ks_safe_check[7]`, `ks_weak_ring`, `ks_ring_pressure`, `ks_flank_attack`/`defense`, `ks_pawnless_flank`, `ks_king_blockers`, `ks_central_king`, `ks_shelter_storm` (open-files-near-king proxy; existing linear shelter/storm stays active in parallel). No linear trace (index-shaping → Phase-4.3 finite-difference tuner). **Also computed `blockers_for_king[]`** (resolves the deferred 3.0 pin masks; 3.3 reuses them). Gates: `bench 13 = 4,033,379` (identical), 8/8 CTest, `--verify` exact (8598/8598). — Opus 4.8 high
- [x] **3.3 Per-count mobility tables — DONE 2026-06-25.** Replaced linear `mob*w` with one-hot tables `mob_n[9]/mob_b[14]/mob_r[15]/mob_q[28]` (mg/eg), seeded `table[i]=i*old_w` (byte-identical); old `mob_mg/eg[7]` removed; tuner `mobility` group + clamps updated. **Mobility area left exactly as today** — the SF-style area refinement (exclude own K/Q, blocked pawns, pinned) is a behaviour change deferred to Phase 4.4 (Rarog precedent), where it's A/B'd with the table fit. Gates: `bench 13 = 4,033,379` (identical), 8/8 CTest, `--verify` exact (8598/8598). — Opus 4.8 high
- [x] **3.4 Pawn-structure refinement — DONE 2026-06-25.** All seeded 0 (existing flat pawn/passer terms stay active). Pawn-cache-safe in `eval_pawns`: `connected_rank[8]`, `weak_unopposed`, `pawn_lever`, `blocked_pawn[2]` (rel rank 5/6), `pawn_majority` (per flank). Piece-dependent in `evaluate()` (attack maps / kings): `passed_path_safe_eg`, `passed_block_defended_eg`, `passed_king_block_eg`, `blockader_knight_eg`. Gates: `bench 13 = 4,033,379` (identical), 8/8 CTest, `--verify` exact (8598/8598). — Opus 4.8 medium
- [x] **3.5 Scale-factor framework + endgame knowledge — DONE 2026-06-25 (behaviour-changing).** Added a `ScaleFactor` framework (0–64, `SCALE_NORMAL=64`, `SCALE_DRAW=0`) + `apply_endgame()` run before the 50-move damping: exact **KPK bitbase** (retrograde fixed-point, lazy-built once), **KBNK** corner mop-up driving the bare king to the bishop-coloured corner ({a1,h8} dark / {a8,h1} light), **KNNK** draw, **no-pawn ≤ minor** draw-scaling, and the generalised **OCB** rule folded into the framework (behaviour-identical when it applies). Generic mate-drive mop-up kept for the general KXK case. All terms **frozen** (non-traced) → `--verify` exact by construction. **Unlike 3.0–3.4 this is NOT bench-identical:** KPK/KRKR-scaling material is reachable at the leaves of the bench's pawn/rook endings, so the score of bench position 3 changes. New fingerprint **`bench 13 = 4,377,437`** (was 4,033,379). Gated by the new `tests/endgames.epd` suite (18/18) + `test_endgames` CTest. **SPRT (2026-06-25, `tc=3+0.03`, simplify `[-5,0]`): −4.17 ± 5.20 Elo, LOS 5.79%, 7,586 games, stopped manually. KEPT (not reverted).** Read as NPS tax, not a chess bug: EPD passing rules out a scaling regression; the small negative is `apply_endgame()`'s per-node census on every node (worst TC for endgame knowledge). Same NPS-tax shape as the whole seeded-inert Phase-3 structure → recovered by the `apply_endgame` guard in 3.10 + the Phase-4-boundary verdict (lazy eval 3.11 can't skip endgame scaling). Gates: bench 4,377,437, 9/9 CTest, endgame suite 18/18, `--verify` exact. — Opus 4.8 high
- [x] **3.6 Space + winnable coupling — DONE 2026-06-25 (bench-identical).** **Space refinement** (traced, seeded 0): base `SpaceMg` kept active + `SpaceBehindMg` (safe central squares behind own pawns), `SpacePieceMg` (per-side safe-space × own non-pawn piece count), `SpaceBlockedMg` (× own blocked-pawn count) — tuned with `misc` in Phase 4.5. **Winnable/complexity** (frozen, **not traced**, seeded 0): complexity over king outflanking / both-flanks pawns / king infiltration / pawn-only endgame / passed count / total pawns / bias, applied to `eg` with a sign-preserving clamp `eg += sign(eg)·max(complexity,−|eg|)` (never flips eg sign); nonlinear → finite-difference tuned in Phase 4.5 like the 3.2 KS funnel; new tuner `winnable` group registers the 7 knobs. Gates: `bench 13 = 4,377,437` (identical), 9/9 CTest, `--verify` exact. — Sonnet 4.6 medium
- [x] **3.7 Small positional terms — DONE 2026-06-25.** Seeded-0 batch in `evaluate()` (attack maps + king_zone): bishop_outpost, reachable_outpost, bad_bishop, minor/rook_king_ring, rook_closed, rook_queen_file, connected_rooks, weak_queen, bishop_pair_pawns (flat bp stays). Deferred (optional/cheap-scope): long-diagonal, bishop x-ray, uncontested outpost, Ethereal closedness/central-king. Gates: bench 4,377,437 (identical), 9/9 CTest, `--verify` exact (8598/8598). — Sonnet 4.6 medium
- [x] **3.8 Material imbalance — DONE 2026-06-25.** SF-style symmetric quadratic imbalance, all coeffs seeded 0: `imb_linear[6]`, `imb_our[21]`, `imb_their[21]` (lower-triangular count-product coefficients). Linear in its coefficients → traced normally (`TR_BOTH`, mg=eg) and fit by the ordinary linear tuner (no finite-diff needed); `--verify` exact proves the bookkeeping. Added `imbalance` tuner group. Gates: bench 4,377,437 (identical), 9/9 CTest, `--verify` exact (8598/8598). — Opus 4.8 high
- [x] **3.9 HCE survey additions — DONE 2026-06-25.** Seeded-0: unstoppable_passer (rule of the square, enemy-pieceless + clear path), minor_behind_pawn, king_protector (minor→king distance), queen_infiltration, pawn_islands (eval_pawns). Optional trio (bishop x-ray, R+Q battery, slider-on-queen) skipped. Gates: bench 4,377,437 (identical), 9/9 CTest, `--verify` exact (8598/8598). — Opus 4.8 medium / Sonnet 4.6 medium
- [x] **3.10 Eval hot-loop cleanup — DONE 2026-06-25 (apply_endgame guard).** Restructured `apply_endgame()` so the 12-popcount census + the five known-endgame rules run only when a side is a lone king or pawnless (the necessary condition); the opening/middlegame skips the whole block, OCB stays outside and always runs. Behaviour-identical (guarded rules can't fire when the condition is false) → bench unchanged. Removes the per-node census behind 3.5's −4 SPRT → a re-SPRT should recover it. No speculative micro-opt (per the plan). Gates: bench 4,377,437 (identical), 9/9 CTest, `--verify` exact (8598/8598). — Opus 4.8 medium
- [x] **3.11 Lazy eval — DONE 2026-06-25 (single checkpoint; SPRT owed).** Checkpoint before the attack-map substrate: if `|tapered(material/PST/imbalance/pawns/minor-bonuses)| > LAZY_MARGIN` (=700, conservative, tune under SPRT) skip the whole attack-map block and finish with `apply_endgame` (so KBNK/KPK survive — `test_endgames` passes). Early return `#ifndef TEXEL_TRACE` → `--verify` exact. **Bench re-baselined 4,377,437 → 4,168,590.** Single checkpoint (2nd checkpoint = later refinement). Gates: 9/9 CTest, `--verify` exact. **SPRT ✅ ACCEPTED H1 (2026-06-27, `[-3,0]`): +16.64 ± 7.03 Elo, LOS 100%, 4,828 games** (LazyOn vs LazyOff/`d4cc5cf`) — a clear net gain, far above Rarog's +4.4; recovers the Phase-3 NPS tax (incl. 3.5's −4). LAZY_MARGIN=700 kept (lowering it = a Phase-5 SPSA option). **This closes Phase 3.** — Opus 4.8 high
- [x] **Phase 3 gate — MET.** Every step: `--verify` exact (8,598/8,598), CTest 9/9 incl. `test_endgames` (endgame EPD suite 18/18). `bench` identical for the inert steps (3.0–3.4, 3.6–3.10); the two behaviour-changing steps re-baselined per their SPRT gates (3.5 → `4,377,437`, 3.11 → `4,168,590`).
- [x] **Phase 3 trace/eval regression — DONE (the Phase-3-scoped checks).** (a) reconstruction == eval delta: `--verify` exact (8,598/8,598) every step. (b) eval symmetry: `test_eval` mirror test (5 positions incl. piece-rich middlegames + the endgame-scaling rook ending), passing. (c) seeded-zero changes trace but not eval/bench: bench identity per inert step + the `TR_` traces fire (exercised by `--verify`). **(d) per-term activation counts — CLOSED 2026-06-27** by `basilisk-texel --feature-support` on the full 1.7M train set: every linearly-traced term fires; the only 61 zero-activation traced params are all structurally impossible (pawn PSTs ranks 1/8, passer/connected-rank 0/7, the `ImbTheir` diagonal, piece-type none/king slots), and the 33 KS/winnable funnel knobs are expected-zero. (PLAN.md §6)
- [x] **Phase 3 eval-cost budget — MEASURED (record-only, not a gate).** Best-of-5 `bench` NPS, pext-PGO: Phase-3 head **2,988,236** vs `phase1-final` **3,333,606** = **−10.4%** — *better* than Rarog's −22%, because 3.10's `apply_endgame` guard + 3.11 lazy eval recovered most of the seeded-inert cost. Per the plan this is **not** gated (terms are overhead until Phase 4 activates them); the real strength verdict is the Phase-4 boundary. (The lazy-on-vs-off SPRT already showed **+16.6 Elo net**, so the raw −10.4% does not cost Elo.)
- [x] **HCE source checklist — DONE at planning (2026-06-20).** The term list was cross-checked against SF11/classical (SF16 ≈ SF11 frozen — faithfulness check only) + Ethereal (source of `Closedness`) + RubiChess + Lambergar; the non-SF terms that pass surfaced (closedness, central-king danger, overloaded defender) were folded into 3.1/3.7. (PLAN.md §3.9)

### Phase 4 - Eval Data-Fit Completion (PLAN.md §6.5; the campaign, SPRT per stage)

- [x] **4.0 Tuner/data readiness gate — DONE 2026-06-27.** All built/resolved: `--feature-support` (closes box 2d; full-train run = every traced term fires, 61 zeros all structurally impossible), **`--tune-kingsafety`** finite-difference path (snapshot+reused Board, coord descent over 43 KS funnel knobs, `safety_table` non-decreasing; smoke 60k: holdout MSE 0.09691→0.09619, activates `ks_safe_check` 12/4/8/4 etc.), **`--l2`** L2-to-prior, **bucketed holdout** (phase split in `--tune`), **`extract.py --balance-phase`** (phase-faithful mix + downsample). Blended labels = capability already in place; binary cache deprioritized; targeted-data = process. `--verify` still exact. (PLAN.md §6.5 Step 4.0). *(Original spec retained below.)* — BUILD the finite-difference re-eval path (perturb weight → re-`evaluate()` → ΔMSE, coordinate descent + shape clamps) for the `attack_units→safety_table` funnel knobs; the linear trace is structurally blind to them (Rarog: all 11 danger inputs = 0 activations, and fitting them was its **biggest stage, +42.5 Elo**). Trace linearly whatever 3.2 can express as a *direct* mg/eg contribution so only the funnel knobs need the re-eval path. Then: feature-support diagnostics (**run first** — it's what reveals the dead KS inputs; freeze/merge sparse params; **this also performs the per-term activation-count check that closes Phase-3 trace-regression item (d)** — i.e. confirm every linearly-traced term fires nonzero, and that the zero-activation ones are exactly the finite-diff/frozen funnel knobs, not bugs), bucketed holdout (phase / material class / king-attack / passers / quiet-threat — a bucket regressing while global loss drops is a stop-and-investigate signal), targeted-data policy (regen *only* the offending bucket), phase-balanced sampling quotas, blended (result+teacher) labels, binary feature cache, regularization/shape constraints (PLAN.md §6.5 Step 4.0). **Dataset: ~2–3M on-policy self-play from the post-Phase-3 head; raw count isn't the constraint (PSTs saturate by ~1M) — per-term support is, so run feature-support first and targeted-top-up only sparse buckets. Big refresh = Phase 7, not now.**
- [x] **4.1 = KING SAFETY — ACCEPTED +65.48 ± 13.58 Elo (H1, LOS 100%, 1514 games, 2026-06-27).** Ran KS first (biggest lever) via `--tune-kingsafety` on the 1.7M `beast_seed` data: holdout MSE 0.09870→0.09780, activated `ks_safe_check` 7/6/8/6 + `ks_king_blockers` 4, sharper monotonic `safety_table` (tail→296). Baked; bench 4,168,590→**4,123,914**, 9/9 CTest. Bigger than Rarog's +42.5. New head = `phase41-ks`. (Material re-confirm low-value — folded into 4.7.)
- [x] **4.2 Threats — ACCEPTED +79.13 ± 14.82 Elo (H1, LOS 100%, 1264 games, 2026-06-27).** Linear `--tune threats` from the KS base; new SF threats activated, **`hang_pen` dropped to 0** (absorbed by the package). Baked; bench→**3,929,330**, 9/9 CTest. Bigger than Rarog's +45.2. New head = `phase42-threats`. Phase-4 cumulative ≈ +144 (KS +65, threats +79).
- [x] **4.3 King safety v2 group — DONE (this IS the king-safety fit, executed FIRST as "Stage 4.1" above; +65.48 Elo accepted).** Numbering note: the plan table reserves 4.3 for king safety, but I promoted it to the first fit ("biggest lever first"), so it carries the execution label 4.1 / binary `phase41-ks`. Same work — not a separate outstanding stage.
- [ ] 4.3b King-safety SPSA polish (**optional**, not yet done) — a small game-based SPSA over a few KS knobs on top of the data fit; decide if worth it after the campaign. — Sonnet 4.6 medium
- [x] **4.4 Mobility — DONE 2026-06-28, no clean win (kept threats head).** Investigated thoroughly: (1) per-count table fit on the existing area — no true headroom over the Phase-2 linear seed; unregularized fit over-valued mobility and failed CTest (startpos depth-5 +131), L2 confirmed the "gain" was overfitting (collapses to seed). (2) **SF-style mobility-area refinement** (exclude own K/Q, blocked/back-rank pawns, pinned `blockers_for_king`; include own minor/rook squares) — implemented + re-fit tables (monotonic clamp): improved holdout −0.00064, but still over-valued (startpos +131, failed CTest); L2 collapsed the gain; area-change with seed tables landed startpos at exactly 100 (borderline) with noise-level holdout. Reverted both. **Conclusion: Phase-2 linear mobility is already well-tuned; mobility has no calibrated, sanity-passing improvement on 4.2-head data.** The area refinement is structurally correct, so the re-try is **scheduled as its own step, 4.6b**, on fresh data (the 4.7 boundary). (The CTest startpos sanity bound earned its keep twice.)
- [x] **4.5 = POSITIONAL DATA-FIT — ACCEPTED +57.21 ± 15.48 Elo (H1, LOS 100%, 956 games, 2026-06-28).** One joint Texel group `phase45` (120 active params) over the full positional structure: pawn refinement (3.4), small positional terms (3.7), HCE survey adds (3.9), minors, rooks, space (3.6), winnable (3.6) — excludes material/PST (4.7), imbalance (4.6), mobility/threats/KS (done). **Fit with L2-to-prior from the start** (mobility lesson). λ-sweep on 1.73M `beast_seed`: λ=1e-6 holdout −0.000608 (48 moved) / 2e-6 −0.000450 / 5e-6 −0.000287. Baked **λ=1e-6** — biggest gain *and* it survives regularization (the test mobility failed). 45 members moved, all small & sensibly-signed (`bad_bishop_eg −1`, `blocked_pawn −2`, `space_mg`→0, `tempo +3`, `passed_king_block_eg +7`). Gate: `--verify` exact (10000/10000), 9/9 CTest, startpos depth-12 **+52cp** (mobility hit +131), NPS unchanged. New head = `phase45-positional` (bench 3,987,976). New tooling: `tools/texel/bake.py` (dump→header, self-limiting to moved params) + tuner group `phase45`. — Opus 4.8 high
- [x] **4.6 = MATERIAL IMBALANCE — ACCEPTED +26.94 ± 8.38 Elo (H1, LOS 100%, 3218 games, 2026-06-28).** SF-style quadratic imbalance (3.8) fit linearly on 1.73M `beast_seed`, on top of the 4.5 head, with L2-to-prior. λ-sweep: 1e-6 holdout −0.000344 (25 moved) / 2e-6 −0.000239 / 5e-6 −0.000134. Baked **λ=1e-6**. The **6 structurally-dead `imb_their` diagonal coeffs** (when i==j the feature `their_cnt = ic_W·ic_B − ic_B·ic_W ≡ 0`; flat t∈{0,2,5,9,14,20}, flagged by 4.0 feature-support) were **excluded from the active set** and verified to stay 0. 3 array members moved, all small (max coeff 8): `imb_linear`→[0,0,0,1,1,1], plus modest `imb_our`/`imb_their` quadratics. Gate: `--verify` exact (10000/10000), 9/9 CTest, startpos depth-12 **+48cp**, NPS fine. New head = `phase46-imbalance`. Baked via `bake.py`. — Sonnet 4.6 medium
- [x] **4.6b Mobility-area refinement — DONE 2026-06-28, sane on fresh data (kept).** Re-applied the SF mobility area in `eval.cpp` (exclude own K/Q, blocked/low-rank pawns, pinned `blockers_for_king` — moved before the sweep; include own minor/rook squares) + a monotonic non-decreasing clamp on the mobility tables in the tuner; re-fit on the 3.35M v17 set. **The 4.4 over-valuation is GONE: startpos depth-12 = +32cp** (4.4 had +131 and failed the CTest bound) — fresh diverse labels calibrated the magnitude, exactly as the 4.4 note predicted. Holdout gain is marginal (area itself −0.00003; the refit moved tables only ±1, +noise), so rather than spend a separate likely-inconclusive SPRT, the SF area is **kept as the more-correct model and validated folded into the 4.7 combined SPRT** vs `phase46`. Gates: `--verify` exact (10000/10000 fresh holdout), 9/9 CTest, bench re-baselined 4,150,316. Standalone binary `phase46b-mobarea` exists to isolate if 4.7 regresses. — Opus 4.8 high
- [x] **4.7 = PST + MATERIAL REFIT — ACCEPTED +6.45 ± 4.60 Elo (H1, LOS 99.7%, 10402 games, 2026-06-28; combined with 4.6b vs `phase46`).** Definitive refit of all 782 PST+material params on the fresh **3.35M v17** set, L2=1e-6 anchored toward the PeSTO seeds. λ-sweep: 1e-6 holdout −0.000149 (42/778 moved, converged epoch 9) / 5e-6 −0.000059. Baked **1e-6**. Conservative as expected (PSTs were already PeSTO-quality, material already tuned): material barely moved (pawn mg 85→83, eg 96→94, rest ±1), PSTs adjusted 4/6 piece tables each. Gate: `--verify` exact (10000/10000 fresh holdout; the regenerated PST block kept the structurally-dead pawn-rank squares at 0), startpos depth-12 **+50cp**, 9/9 CTest, bench 3,763,657. Baked via `bake.py --allow-pst` (first use of the 2-D path). New head = `phase47-pst`. The modest gain validates 4.6b too (one combined SPRT). — Sonnet 4.6 medium
- [x] **4.8 = GLOBAL POLISH — ACCEPTED +33.25 ± 9.40 Elo (H1, LOS 100%, 2610 games, 2026-06-28).** Joint refit of all 1179 params (material + PST + all positional + threats + mobility + imbalance) on the fresh 3.35M v17 set, lr 0.15, L2=1e-6. λ-sweep: 1e-6 holdout −0.000695 (117 moved, epoch 55) / 3e-6 −0.000365 (34). Baked **1e-6**. Far bigger than a typical "polish" because the 4.5 positional terms were fit on the *old* beast_seed — reconciling them (+ everything else) to fresh v17 recovered real headroom; **all three phases improved** (opening 0.1460→0.1450, mg 0.0950→0.0940, eg 0.0625→0.0618), not endgame over-tuning. Deltas small/sensible (material ±1). Gate: `--verify` exact, startpos +60cp, 9/9 CTest, bench 3,764,539. New head = `phase48-polish`. **KS re-fit deferred to Phase 7** (v17 is endgame-heavy → wrong data to re-fit king safety). Then: 4.8a prune + 2000-game validation vs `phase1-final` + gauntlet. (The post-Phase-5 eval-refresh grind is its own **Phase 7** — see below.)
- [x] **4.8a Dead-feature prune — DONE 2026-06-29 (behaviour-identical).** The 4.8 global-polish fit on fresh v17 confirmed every flagged term stayed **exactly 0** even on endgame-heavy data → genuinely redundant, removed. Pruned **14 params / 8 features**: `pass_supp` (×3), `cand` (mg+eg), `pawn_lever` (mg+eg), `blockader_knight_eg`, `bishop_outpost` (mg+eg), `weak_queen` (mg+eg), `unstoppable_passer_eg`, `space_behind_mg` — across `eval.cpp` (compute + trace blocks), `EvalParams.h` (struct members + `EVAL_PARAM_LIST` X-macros), and `tuner.cpp` (the `phase45`/`misc` group ranges + clamp lines). Gated on **byte-identical bench (3,764,539 unchanged)** + `--verify` 10000/10000 exact (trace layout shrank consistently) + 9/9 CTest; builds clean with no orphaned variables (verified `path`/`q_them`/`outpost_sqs`/`wp`/`bp` are all still used by live terms). **Confirmed by a simplification SPRT** (`[-5,0]`, 3+0.03): **+2.49 ± 4.19 Elo, H1 accepted, 11294 games** — i.e. not a regression, hair-positive from the leaner-eval NPS edge, exactly as expected. The KS re-fit (the other audit flag) is **deferred to Phase 7** — v17 is endgame-heavy (59%), the wrong data to re-fit king safety. — Opus 4.8 high

### Phase 5 - Time Management hardening + tuning (PLAN.md §7) — **promoted to Phase 5, executed BEFORE the Phase 6 search wave (2026-06-29)**

> The 5.1/5.1b/5.2 foundation (Rarog-port formula + reserve + the 2026-06-20 LB
> validation) is **done** and stays. Reopened (steps 5.3–5.9) because LB **still**
> shows time-losses post-Phase-3/4 (heavier eval per node) and the TM constants
> were never tuned for Basilisk. Root cause (2026-06-29): (1) the clock starts
> inside the worker, missing `go`-receipt→search dispatch latency the GUI charges
> (likely the main LB forfeit); (2) fixed 2048-node poll overshoots the tiny
> bullet budget now that nodes are heavier; (3) thin static `2*overhead` reserve
> + 10 ms default Move Overhead; (4) zero TM-constant tuning. Aim: **generally
> strong across bullet→slow, not a TC specialist.**

- [x] 5.1 Increment-aware budget formula implemented — direct port of Rarog's
      Phase 2.2 SF-style rewrite (logT-based optConst/maxConst, ply-aware
      sudden-death + explicit-movestogo branches); `compute_time_limit` gained
      a `game_ply` parameter. `bench 13` unchanged (`4,033,379` nodes), 8/8
      CTest passed. *(implemented 2026-06-20 under its old label Step 6.1.)*
- [x] 5.1b Time-safety reserve matched to Rarog's exact mechanics (`raw_time -
      2*overhead` hard ceiling, not the original 2.9.1 patch's 3x-effective
      double subtraction). *(old label 6.1b.)*
- [x] 5.2 Harness support exists: `sprt.ps1 -TC` and `-MoveTime`, plus
      `gauntlet.ps1 -TC`/`-MoveTime`/`-TimeMargin` (Step 2.9.2). *(old label 6.2.)*
- [x] 5.2b **LittleBlitzer validation leg — PASSED (2026-06-20).** Full 35,000-game
      overnight pool at `tc=3+0.03`, default Move Overhead: `t=0` for
      Basilisk 1.5.1-sftm (and every other engine in the pool). Rating
      2696.8, 62.7%, 2nd of 5 — essentially tied with SF-2700-capped, far
      ahead of Rarog/SF-2600/SF-2500. *(The optional fastchess second-harness
      leg and the old "6.3 optional SPSA" are absorbed into 5.9 / 5.8 below.)*
- [x] **5.3 Diagnose & instrument the LB time-loss — DONE 2026-06-29 (instrumented + measured).** **Findings (1+0.01 fastchess, 12,471 moves + the LB forfeit log):** the engine's TM is *sound* — overshoot is negligible (14/12471 moves over `hard` by ≤ **1 ms**, pure poll-rounding) and allocation is correct (`elapsed < hard` always). The only harness-independent imperfection is **dispatch ≤ 20 ms** (`go`-receipt→clock-start, under bullet+concurrency) — what 5.4 reclaims. The **LB forfeits are GUI/pipe latency, not a budget bug**: the LB log caught a forfeit with `elapsed_ms=112` but LB charging **959 ms** (`dispatch_ms=0`) — ~847 ms in the LB↔engine pipe (LB is a fragile 2012 single-threaded GUI choking on verbose bullet output). fastchess (efficient I/O, `timemargin=40`) never forfeits. **Consequences: 5.5 is unnecessary (no real overshoot); the LB issue is not a TM-budget repair.** — Opus 4.8 medium `TM_Debug` UCI check (default off) — **advertised only in tune/dev builds** (`#ifdef BASILISK_TUNE`) so a harness/GUI actually sends the `setoption` (fastchess/LB silently skip *unadvertised* options — the first attempt logged `Warning; doesn't have option TM_Debug` and produced no data); release builds keep a clean 9-option list. When on, `Searcher::search` emits one `info string tm soft_ms=.. hard_ms=.. elapsed_ms=.. dispatch_ms=..` per move. `dispatch_ms` = `go`-receipt (captured in `UciProtocol::cmdGo`) → `start_time_`, the latency the GUI charges but `elapsed_seconds()` doesn't yet count (defect #1). Timing unchanged; bench 3,764,539, 9/9 CTest, debug-off play-identical. Diagnostic binary: `tools/test_engines/basilisk-phase53-tmdebug-pext-pgo.exe`. **Pending (maintainer):** run it under LB / a `1+0.01` fastchess gauntlet with `option.TM_Debug=true` + engine logging, capture the `info string tm` lines, and confirm which defect dominates (expect `dispatch_ms` > 0 under LB and `elapsed_ms` > `hard_ms` overshoot at bullet) → sizes 5.4/5.5. — Opus 4.8 medium
- [~] **5.4 Start the clock at `go`-receipt — IMPLEMENTED 2026-06-29, SPRT pending.** `Searcher::search` now sets `start_time_ = limits.go_recv_time` (the `cmdGo` timestamp threaded in 5.3) instead of the in-worker `now()`, falling back to `now()` when unset (internal/bench). The engine now accounts for the ~20 ms dispatch latency 5.3 measured, tightening it against the GUI clock. `dispatch_ms` reads 0 by construction now (confirms it). Bench 3,764,539 unchanged, 9/9 CTest. Candidate `phase54-clockatgo`. **Gate (maintainer):** non-regression SPRT `[-3,0]` at `3+0.03` vs `phase48a-pruned` (1.7.0). Expect ~neutral — at robust harnesses 5.4 trades a few ms of search for clock safety; keep if within `[-3,0]`. — Opus 4.8 medium
- [~] **5.5 Anti-overshoot poll granularity — LIKELY SKIP (5.3 data).** Premise was that heavier eval per node would overshoot the tiny bullet budget; measured overshoot is **≤ 1 ms over 12,471 bullet moves**, so the fixed `(nodes_ & 2047)` poll is already fine. Revisit only if a future change (or a real harness) shows actual overshoot. — Sonnet 4.6 medium
- [ ] **5.6 GUI-robust reserve + Move Overhead default** — reserve `max(2*overhead, abs_floor_ms≈15–25)`; default Move Overhead 10→~20–30 ms. Gate: LB pool re-run at `3+0.03` **and** `1+0.01` → `t=0`; SPRT non-regression `[-3,0]`. **Closes the LB issue.** — Sonnet 4.6 medium
- [ ] **5.7 Root fail-low / instability time extension** — extend soft→hard when the root best fails low or the PV is still changing late (complements the between-iterations score-drop extension). Gate: SPRT `elo1=3` at `3+0.03` + `10+0.1` confirm. — Opus 4.8 medium
- [ ] **5.8 Expose TM constants under `BASILISK_TUNE` + SPSA** (the Elo lever; supersedes the old optional "6.3" SPSA) — `ConfigGroup tm` (optConst/maxConst+slopes, `0.8097`, stability `0.06`, score-drop `30`/`÷100`, effort `80/25`→`0.80/1.20`, the 5.7 knobs); SPSA at `10+0.1`, re-validate at `1+0.01` and `60+0.6`, bake, SPRT vs pre-SPSA head. First TM fit to Basilisk's own eval/search. — Sonnet 4.6 medium
- [ ] **5.9 Cross-TC validation + ship gate** — gauntlet/LB at `1+0.01`, `3+0.03`, `10+0.1`, `60+0.6`; each `t=0` and non-negative vs the pre-Phase-5 head; record per-TC Elo to expose any specialization. — Sonnet 4.6 medium

### Phase 6 - Search Efficiency Wave (PLAN.md §5; **renumbered to Phase 6 on 2026-06-29 — executed AFTER Phase 5 time management.** SPSA last — driving Sonnet 4.6 medium, dense ports Codex 5.5 medium / GPT-5.5 high)

- [ ] 6.1 TT-bound eval refinement SPRT verdict recorded.
- [ ] 6.2 History bonus/malus formula SPRT verdict recorded.
- [ ] 6.3 Fractional LMR SPRT verdict recorded.
- [ ] 6.4 TT-capture LMR input SPRT verdict recorded.
- [ ] 6.5 Post-LMR deeper retry: shallower reachability checked, STC SPRT
      recorded, LTC `tc=10+0.1` recorded if STC passes.
- [ ] 6.6 Qsearch quiet checks (start with the existing `Board::gen_quiet_checks()` helper) SPRT verdict recorded.
- [ ] 6.7 Double-extension cap non-regression gate recorded.
- [ ] 6.8 Razoring restriction experiment verdict recorded.
- [ ] 6.9 Second-wave constants SPSA (wave2, the conserved compute) + rejected
      combined-polish retry folded in; SPSA and SPRT verdict recorded.

### Phase 7 - Non-NNUE ceiling: eval-refresh multi-cycle grind (PLAN.md §7.5; EXECUTE AFTER Phases 5–6, optional/evidence-driven — Sonnet 4.6 medium)

- [ ] 7.0 Non-NNUE ceiling analysis read & stop-point understood: SF11 (~3440 CCRL) is the clean classical proof; SF16 ≈ SF11's *frozen* eval (removed July 2023, ~2 Elo), so it adds **no** terms over SF11 — a faithfulness cross-check only, not a source of post-SF11 ideas. Don't size off NNUE-era "HCE" ratings (Berserk/Rubi/Stash 3300+ are their NNUE builds, classical ~3000–3150); ranked levers = search wave (Phase 6) > data-refresh > shelter/storm→danger > deferred small terms; king-bucketed PSTs = NNUE gateway → do Phase 9 instead. **Basilisk caveat:** "features done, only tuning + search left" applies only AFTER Phase 4 closes.
- [ ] 7.1 Eval data-refresh cycle 1: the dataset was self-played by a pre-Phase-4 head; once Phase 4+5 strengthen it ~+150–250, regen self-play (`datagen.ps1` → `extract.py`) and do **one joint low-lr refit** (`basilisk-texel --tune all` + `--tune-kingsafety`) + **one SPRT** + boundary gauntlet — cleaner WDL labels, NOT a re-stage. Turn on blended labels + phase-balanced sampling; ride the shelter/storm→danger fold here. Expected ~+10–40 Elo.
- [ ] 7.2 Iterate cycles 2–3; **stop** when a cycle yields `<~+8 Elo`, holdout loss stalls between regens, or the gauntlet doesn't move. Each accepted cycle is releasable (minor if real gauntlet Elo).

### Later

- [ ] Phase 8 feature menu only after the eval + search phases (and the Phase 7 refresh) plateau.
- [ ] Phase 9 NNUE remains the terminal option; eval boundary stays clean.

### External gauntlet (opponents) + time budget

After each phase — **especially after Phase 4 (eval), which over-fits self-play
most** — run `tools/gauntlet.ps1` at `tc=10+0.1`. Recommended opponents (pick by
**measured** score): Basilisk 1.5.0 (`phase1-final`) + Rarog; **Critter 1.6a**
(~3150–3200, a target to clear); **Stockfish capped** via `UCI_LimitStrength`,
**start 2900 → 3100 → 3300** (Basilisk is already ~3100, so start high) at the
level where Basilisk scores ~30–70%; one independent mid/high HCE
(Lambergar / Peacekeeper / Igel). Time budget on the 5950X: datagen/SPSA at
`-Concurrency 24`; **Texel fits are CPU-minutes — run them freely.**

### Release points & version numbers

Last released: **1.6.0** (Phase 2 eval scalars + time-forfeit fix). **`1.7.0`
prepared 2026-06-29** (Phase 4 eval data-fit campaign; version/CHANGELOG/README
done, awaiting the maintainer's tag). When to tag the next ones (full rationale +
gates in PLAN.md §10):

| Tag after | What ships (cumulative since last tag) | Version |
|---|---|---|
| **Phase 2.9** | Phase 2 eval scalars (**+54 vs 1.5.0**) + time-forfeit fix | **`1.6.0`** |
| Phase 3 | bench-identical only (**except 3.5 endgame knowledge — behaviour-changing, SPRT-gated**) — nothing to release | — |
| **Phase 4** | full eval data-fit campaign (**+80–160**) | **`1.7.0`** |
| **Phase 5** | time-management hardening + tuning (LB forfeit fix + **+8–25** TM SPSA) | **`1.7.1`** if the LB fix ships alone, else folds into `1.8.0` |
| **Phase 6** | search-efficiency wave (**+20–50**) | **`1.8.0`** |
| **Phase 7** (per cycle) | eval-refresh grind (**+10–40** cycle 1) | **minor** per cycle that banks gauntlet Elo, else patch |
| **Phase 8** | feature menu, batched | patch/minor per batch |
| **Phase 9** | NNUE | **`2.0.0`** |

Rules: **minor** bump (`1.Y.0`) for each phase that banks SPRT+gauntlet-validated
strength; **patch** (`1.y.Z`) for fix-only releases; **major** (`2.0.0`) for the
NNUE architecture swap. The version tracks **everything new since the last tag,
not the last phase** — that's why the post-2.9 release is `1.6.0` (it ships all of
Phase 2's eval gains), even though Step 2.9 itself is just a fix.

> **Released as `1.6.0`, not `1.5.1`.** `1.5.1` (a patch) would have undersold
> the biggest eval gain so far. The release was gated on the time-safety fix
> (Phase 5 Step 5.1/5.1b, old labels 6.1/6.1b) clearing `t=`≈0 and the +54 holding vs `phase1-final`
> — the forfeit fix is what made the eval gains releasable.

**To prepare a release, say:** *"Release 1.6.0."* The model runs the checklist
in PLAN.md §10 — bumps the version in **both** `src/Constants.h` and
`CMakeLists.txt`, updates `CHANGELOG.md` (README no longer carries per-version
content), verifies the non-tune release build hides the tuning options +
passes CTest, builds the dist binaries (the full platform matrix comes from
`.github/workflows/release.yml` on tag push), and commits. **It will not tag
and will not push — those are yours.** Create the tag and push whenever you're
actually ready to cut the release:

```powershell
git tag -a v1.6.0 -m "Version 1.6.0"
git push origin <branch>
git push origin v1.6.0
```

It will refuse to even commit if the version's pre-release gate (SPRT/gauntlet)
hasn't passed — so settle that first.

---

## Common Commands

```powershell
# Build a named pext-PGO+TUNE binary
.\tools\build_test.ps1 -Suffix <name>

# SPRT a gain candidate (default tc=3+0.03 clock)
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-<candidate>-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-<baseline>-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline"

# SPRT a small/tighter candidate
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-<candidate>-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-<baseline>-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -Elo1 3

# LTC confirmation / TC-suspect candidate
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-<candidate>-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-<baseline>-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -TC "10+0.1" -Elo1 3

# Optional old fixed-movetime sanity gauntlet
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-<candidate>-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-<baseline>-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -MoveTime 0.1

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
