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
  of Phase 3.9 after eval tuning; keep the post-LMR deeper re-search in
  Phase 3.5 and gate it with the clock harness plus LTC if it passes.
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
- **The next implementation work is Phase 3.0 — the attack-map substrate**
  (a behaviour-identical refactor; the gate is `bench 13` identity, no games).

### The program in one table (overview · model picker · Elo)

Read PLAN.md §0.5 for *why* this order: build all eval structure first
(Phase 3, no games), fit the eval once (Phase 4), run the search SPSA once last
(Phase 5). Texel fits are cheap; SPSA/SPRT games are the conserved resource.

| Phase | What | Gate | Model(s) | Elo |
|---|---|---|---|---|
| **2** (done) | Texel infra + cheap scalar fits | SPRT per subgroup | Sonnet 4.6 medium | banked |
| **2.9** Robustness quick win (**NEXT**) | time-safety floor (65 forfeits) — pulled before the eval work | `bench 13` identical; `t=`→0 in a gauntlet | Sonnet 4.6 medium | reliability + a few |
| **3** Eval structure build-out | attack maps, threats, KS v2, mobility tables, pawn refine, endgames (+KBNK), small terms | `bench 13` identical + `--verify` + CTest (**no games**) | **Opus 4.8 high** (KS/threats/endgames/mobility); Opus 4.8 medium (attack maps/pawns); Sonnet 4.6 medium (small terms) | 0 direct (enabler) |
| **4** Eval data-fit completion | one staged Texel campaign over the full structure; KS/threats/mobility/minors/**PST+material**/polish | SPRT per stage | Sonnet 4.6 medium (driving) | **+80–160** |
| **5** Search-efficiency wave | TT-bound eval, history split, fractional LMR, deeper re-search, qsearch checks, **wave2 SPSA last** | SPRT per item; SPSA once | Sonnet 4.6 medium; dense ports Codex 5.5 medium / GPT-5.5 high | **+20–50** |
| **6** Time management | increment-aware budget | clock SPRTs | Sonnet 4.6 medium | +5–20 |
| **7 / 8** Feature menu / NNUE | plateau menu / ceiling | — | — | — |

Per-step models are on each PLAN step header. **SPRT is the only verdict.**

> **Gauntlet validation (2026-06-19, 35k games @ `tc=3+0.03`):** Basilisk 1.5.1
> (dev, partial scalar tuning only) was **2nd of 9, +54 over 1.5.0**, tied with
> "SF-capped-2700" — the eval lever is confirmed, with KS/PST/structure still to
> come. **Two cautions:** (1) SF `UCI_Elo` is calibrated for 120s+1s/CCRL-40/4, so
> it is **not** a true anchor at this fast TC — for a real CCRL number run a
> slower-TC gauntlet that includes **Critter 1.6a** (it forfeited at 3+0.03).
> (2) **1.5.1 lost 65 games on time** (vs 1.5.0's 18) — add a hard time-safety
> floor to `compute_time_limit` **now**; forfeits contaminate every gauntlet.

So if you say *"implement the next step,"* the model should know that the time
forfeit fix has moved: **2.9.2 is done** (`gauntlet.ps1 -TC`), and **2.9.1's
original patch was superseded by Phase 6 Step 6.1/6.1b (2026-06-20)** — a small
LittleBlitzer probe showed the old formula was tighter to the margin than
Rarog's already-rewritten one, so rather than re-tune it, **Basilisk's clock-path
TM is now a direct port of Rarog's Stockfish-style formula**, with the 2.9.1
reserve folded in using Rarog's exact mechanics. **What's left before 1.6.0:**
Step 6.2 gating (SPRT + a LittleBlitzer overnight run at default Move Overhead,
matching Rarog's own entry) confirming `t=`≈0, then **2.9.3** (release `1.6.0`).
Only after that does **Phase 3.0** (the attack-map refactor) start — **not** old
Step 2.4c.

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

**Phase 6 Step 6.1 / 6.1b — done (2026-06-20).** A first LittleBlitzer probe
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
belongs to Phase 3.9, after the Phase 2 eval refit, using the unified clock
harness:

```powershell
cd ..\..
.\tools\setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
cd tools\weather-factory
python main.py
```

Treat that command as a historical reference until Phase 3.9 rewrites/extends
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

### Phase 2.9 - Robustness Quick Win (NEXT; bench-identical, PLAN.md §4.9)

- [x] 2.9.2 Add `-TC`/`-MoveTime`/`-TimeMargin` to `tools/gauntlet.ps1` (clock `tc=10+0.1` default, mirrors `sprt.ps1`; banner fixed). Pure tooling, no engine change, parse-verified.
- [x] 2.9.1 *(superseded by Phase 6 Step 6.1/6.1b, 2026-06-20 — see below)* — original standalone reserve patch on the old formula; replaced rather than re-tuned after a LittleBlitzer probe showed it binding more than Rarog's equivalent.
- [x] **2.9.3 Release `1.6.0` prep done (2026-06-20)** — version bumped in `src/Constants.h` + `CMakeLists.txt`, `CHANGELOG.md` entry added, clean non-tune release build verified (options hidden, `bench 13` matches, 8/8 CTest), dist binaries built, committed. **Pending (user-only): create tag `v1.6.0` and push** — the model never tags or pushes.
- [ ] (calibration, anytime) Slower-TC gauntlet with a CCRL-rated anchor (Critter 1.6a / Fruit 2.1), pin with `ordo -A "<name>" -a <ccrl>` (PLAN.md §10).

### Phase 3 - Eval Structure Build-Out (PLAN.md §6; EXECUTE FIRST — bench-identical, no games)

- [ ] 3.0 Attack-map substrate + blockers/pin masks (refactor; bench identical, `--verify` exact). — Opus 4.8 medium
- [ ] 3.1 Threats package structure (seeded inert). — Opus 4.8 high
- [ ] 3.2 King safety v2 — full danger model (safe checks, weak ring, flank attack/defense, pawnless flank, blockers/pins near king, lost-castling, no-queen scaling; seeded inert). — Opus 4.8 high
- [ ] 3.3 Per-count mobility tables (seeded `i*old_weight`; area excludes own pinned/blocking pieces). — Opus 4.8 high
- [ ] 3.4 Pawn-structure refinement + promotion-path passer safety (seeded inert). — Opus 4.8 medium
- [ ] 3.5 Scale-factor framework + endgame knowledge (exact KPK + KBNK) + `tests/endgames.epd` suite. — Opus 4.8 high
- [ ] 3.6 Small positional terms (bishop + reachable knight outposts, trapped rook, connected rooks, long-diagonal, bad bishop, queen-pressure, initiative; seeded 0). — Sonnet 4.6 medium
- [ ] 3.7 Material imbalance structure (optional; seeded 0). — Opus 4.8 high
- [ ] 3.8 Gauntlet-driven additions (unstoppable passer, minor-behind-pawn, pawn islands, space upgrade, queen infiltration, king protector, winnable coupling). — Opus 4.8 medium / Sonnet 4.6 medium
- [ ] Phase 3 gate: bench identical, `--verify` exact, CTest + endgame EPD suite pass.
- [ ] Phase 3 trace/eval regression tests: per inert term, prove trace reconstruction == eval delta, eval symmetry holds, seeded-zero changes trace counts but not eval/bench, and activation counts are nonzero on curated positions (PLAN.md §6).
- [ ] Phase 3 eval-cost budget: fixed-depth NPS of the Phase 3 head within ~10–15% of `phase1-final` (inert terms still compute). If breached, profile + lazy-eval / whole-eval cache before Phase 4 spends games (PLAN.md §6).
- [ ] HCE source checklist done once before freezing the term list: cross-check terms vs SF-classical + Ethereal + RubiChess + one independent HCE (avoid SF-monoculture; PLAN.md §3.8).

### Phase 4 - Eval Data-Fit Completion (PLAN.md §6.5; the campaign, SPRT per stage)

- [ ] 4.0 Tuner/data readiness gate (before any stage): nonlinear king-safety tuner support (linearise or finite-difference path), feature-support diagnostics (freeze/merge sparse params), bucketed holdout (phase / material class / king-attack / passers / quiet-threat — a bucket regressing while global loss drops is a stop-and-investigate signal), targeted-data policy (regen *only* the offending bucket), phase-balanced sampling quotas, blended (result+teacher) labels, binary feature cache, regularization/shape constraints (PLAN.md §6.5 Step 4.0).
- [ ] 4.1 Material + leftover scalars re-confirm.
- [ ] 4.2 Threats group (drop the old flat `hang[]`).
- [ ] 4.3 King safety v2 group.
- [ ] 4.3b King-safety SPSA polish (**optional**; decide when reached). — Sonnet 4.6 medium
- [ ] 4.4 Mobility tables.
- [ ] 4.5 Pawn refinement + small terms + `minors`.
- [ ] 4.6 Material imbalance (skip if 3.7 skipped).
- [ ] 4.7 PSTs + material definitive refit.
- [ ] 4.8 Global polish, then 2000-game validation vs `phase1-final` + gauntlet.

### Phase 5 - Search Efficiency Wave (PLAN.md §5; SPSA last — driving Sonnet 4.6 medium, dense ports Codex 5.5 medium / GPT-5.5 high)

- [ ] 5.1 TT-bound eval refinement SPRT verdict recorded.
- [ ] 5.2 History bonus/malus formula SPRT verdict recorded.
- [ ] 5.3 Fractional LMR SPRT verdict recorded.
- [ ] 5.4 TT-capture LMR input SPRT verdict recorded.
- [ ] 5.5 Post-LMR deeper retry: shallower reachability checked, STC SPRT
      recorded, LTC `tc=10+0.1` recorded if STC passes.
- [ ] 5.6 Qsearch quiet checks (start with the existing `Board::gen_quiet_checks()` helper) SPRT verdict recorded.
- [ ] 5.7 Double-extension cap non-regression gate recorded.
- [ ] 5.8 Razoring restriction experiment verdict recorded.
- [ ] 5.9 Second-wave constants SPSA (wave2, the conserved compute) + rejected
      combined-polish retry folded in; SPSA and SPRT verdict recorded.

### Phase 6 - Time Management (PLAN.md §7) — pulled forward to 2026-06-20

- [x] 6.1 Increment-aware budget formula implemented — direct port of Rarog's
      Phase 2.2 SF-style rewrite (logT-based optConst/maxConst, ply-aware
      sudden-death + explicit-movestogo branches); `compute_time_limit` gained
      a `game_ply` parameter. `bench 13` unchanged (`4,033,379` nodes), 8/8
      CTest passed.
- [x] 6.1b Time-safety reserve matched to Rarog's exact mechanics (`raw_time -
      2*overhead` hard ceiling, not the original 2.9.1 patch's 3x-effective
      double subtraction).
- [x] 6.2 Harness support exists: `sprt.ps1 -TC` and `-MoveTime`, plus
      `gauntlet.ps1 -TC`/`-MoveTime`/`-TimeMargin` (Step 2.9.2).
- [x] **6.2 LittleBlitzer leg — PASSED (2026-06-20).** Full 35,000-game
      overnight pool at `tc=3+0.03`, default Move Overhead: `t=0` for
      Basilisk 1.5.1-sftm (and every other engine in the pool). Rating
      2696.8, 62.7%, 2nd of 5 — essentially tied with SF-2700-capped, far
      ahead of Rarog/SF-2600/SF-2500.
- [ ] 6.2 fastchess leg (optional, recommended before tagging) — `tc=10+0.1`
      and `tc=1+0.5` SPRTs via `tools/sprt.ps1 -TC`, `elo0=0 elo1=3`. Lower
      priority now that LB has already settled the `t=`≈0 question
      conclusively; confirms the fix transfers to a second harness before
      Step 2.9.3 (release `1.6.0`).
- [ ] 6.3 Optional TM SPSA decision recorded.

### Later

- [ ] Phase 7 feature menu only after the eval + search phases plateau.
- [ ] Phase 8 NNUE remains the terminal option; eval boundary stays clean.

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

Last released: **1.5.0** (Phase 1 search constants). When to tag the next ones
(full rationale + gates in PLAN.md §10):

| Tag after | What ships (cumulative since last tag) | Version |
|---|---|---|
| **Phase 2.9** | Phase 2 eval scalars (**+54 vs 1.5.0**) + time-forfeit fix | **`1.6.0`** |
| Phase 3 | bench-identical only — **nothing to release** | — |
| **Phase 4** | full eval data-fit campaign (**+80–160**) | **`1.7.0`** |
| **Phase 5** | search-efficiency wave (**+20–50**) | **`1.8.0`** |
| **Phase 6** | increment-aware time management | `1.8.1` (or `1.9.0` if it banks Elo) |
| **Phase 8** | NNUE | **`2.0.0`** |

Rules: **minor** bump (`1.Y.0`) for each phase that banks SPRT+gauntlet-validated
strength; **patch** (`1.y.Z`) for fix-only releases; **major** (`2.0.0`) for the
NNUE architecture swap. The version tracks **everything new since the last tag,
not the last phase** — that's why the post-2.9 release is `1.6.0` (it ships all of
Phase 2's eval gains), even though Step 2.9 itself is just a fix.

> **Released as `1.6.0`, not `1.5.1`.** `1.5.1` (a patch) would have undersold
> the biggest eval gain so far. The release was gated on the time-safety fix
> (Phase 6 Step 6.1/6.1b) clearing `t=`≈0 and the +54 holding vs `phase1-final`
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
