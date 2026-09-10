# SPSA tuning with weather-factory + fastchess

> **Roadmap status (2026-09-02):** the checked configurations document earlier
> search surfaces and are not authorized unchanged for the final classical
> tune. Phase 7.10 must create a focused nonlinear-evaluation group from the
> accepted HCE defaults; Phase 8.1 must regenerate staged search groups from
> the final HCE head. Never launch a production tune from stale seeds merely
> because a JSON file is present.

> **⚠ STALE configs (8.6.1, 2026-07-20):** `config_lmr.json.STALE` and
> `config_combined.json.STALE` are renamed so `spsa.ps1 -ConfigGroup` cannot
> launch them by accident: they express the `Lmr*Adj` knobs on the pre-6.7
> **0–2/0–3 integer scale** (the engine now uses **1024ths**, so running them
> would silently drive those knobs to ~0), and `config_combined` additionally
> carries pre-hcefinal seeds (`HistPruneCoeff` 4210 vs live 14004) with a
> `LmrHistDiv` floor (6000) *above* the live default (5683). Do not "fix"
> them piecemeal — regenerate any needed config from the
> `src/SearchParams.h` X-macro table (the single source of truth since
> 8.6.1) at the 10.7 tune. `TmInstability` (registered in 8.6.1) must be in
> the 10.7 config.

fastchess does **not** have a built-in SPSA tuner. The community-standard tuner
is **weather-factory** (https://github.com/jnlt3/weather-factory), a small
Python driver that perturbs UCI options and runs mini-matches via fastchess.
This folder holds ready-made weather-factory config files for Basilisk.

## One-time setup

The easiest way is `tools/spsa.ps1`, which does all of the below in one
command. Run it manually only if something needs customising.

1. **Download fastchess** into `tools\bin\fastchess.exe`
   - https://github.com/Disservin/fastchess/releases
2. **Clone weather-factory under the repo tools folder:**
   ```powershell
   git clone https://github.com/jnlt3/weather-factory tools\weather-factory
   ```
3. **Populate its `tuner\` folder** with:
   - `fastchess.exe`  (copy from `tools\bin\`)
   - the Basilisk test binary you are tuning, e.g.
     `basilisk-phase1-defaults-pext-pgo.exe` (build with
     `tools\build_test.ps1 -Suffix phase1-defaults`, then copy from
     `tools\test_engines\`)
   - your local opening book `UHO_Lichess_4852_v1.epd` (copy from `tools\books\`)

## Per-run setup

4. **Update `A` in `spsa.json`** to `planned_iterations / 10`.
   This is weather-factory's only required change per run.
   Example: planning 10 000 iterations → set `"A": 1000`.
   The other fields (`a`, `c`, `alpha`, `gamma`) should stay at their defaults.

5. **Copy the three config files** for the group you are tuning into the
   weather-factory root (next to `main.py`):
   - `cutechess.json`             (runner settings — same for every group)
   - `spsa.json`                  (SPSA hyper-params — updated per step 4)
   - `config_<group>.json` → rename to `config.json` (the parameter set)

## Run

```powershell
.\tools\spsa.ps1 -ConfigGroup pruning -EngineSuffix phase1-defaults -Iterations 5000
cd tools\weather-factory
python main.py        # progress + tuned values written to its own state files
```

For the LMR run after pruning has been accepted, build the current accepted head
and tune from that binary:

```powershell
.\tools\build_test.ps1 -Suffix phase1-lmr-baseline
.\tools\spsa.ps1 -ConfigGroup lmr -EngineSuffix phase1-lmr-baseline -Iterations 5000
cd tools\weather-factory
python main.py
```

After both pruning and LMR have been accepted, run only a short narrowed polish
from the accepted LMR head:

```powershell
.\tools\spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
cd tools\weather-factory
python main.py
```

weather-factory writes the running parameter values to its state file every
`save_rate` games; stop it any time with Ctrl-C.

Re-running `tools\spsa.ps1` starts a *fresh* run and archives old `state.json`,
`games.pgn`, and graph output. To continue an interrupted run instead, pass
`-Resume` (keeps the existing state), or `-LaunchOnly` to just relaunch the
already-configured tuner.

## CRITICAL: SPSA finds candidates, SPRT decides

SPSA optimizes a noisy objective and **over-fits**. The tuned values are only a
*candidate*. Always finish by:

1. Baking the tuned values in as the new UCI-option defaults (or passing them
   explicitly), then building a fresh `pext --pgo` binary with `tools\build_test.ps1`.
2. Running `tools\sprt.ps1` (default `tc=3+0.03` - the same TC this SPSA uses)
   of the tuned binary vs the pre-tuning head. **Keep the tuned values only if
   SPRT accepts H1.** For a phase-boundary or TC-suspect feature, also confirm
   at LTC (`-TC "10+0.1"`). Use `-MoveTime 0.1` only as an optional old-harness
   fixed-movetime sanity check.

## Settings rationale

| Setting | Value | Why |
|---|---|---|
| Runner | fastchess (`use_fastchess: true`) | less overhead than cutechess-cli |
| `tc` | `3` -> 3+0.03 s | Clock + 1% increment, matching `sprt.ps1` so SPSA optima transfer to the confirming SPRT without the old `tc=1` / `st=0.1` condition gap. |
| `hash` | 64 | matches deployment |
| `threads` | auto | `spsa.ps1` detects physical cores and leaves two free; the checked JSON is the 16-core example |
| `games` | 32 | per iteration; multiple of 2 and ≈ 2×threads for a stable gradient |
| `A` (spsa.json) | iterations / 10 | **must update per run** (see step 4 above) |
| `a`, `c`, `alpha`, `gamma` | defaults | do not change (weather-factory guidance) |
| per-param `step` | see tables below | sized to cause a ~2–3 Elo swing per weather-factory guidance |

## Parameter groups (tune one group at a time)

Tune **one config file per run**. Do not combine both groups into one run —
the gradient becomes too noisy with many parameters at once.

**Prerequisite:** the UCI options in each group must be exposed in Basilisk
first (Phase 1 steps 1–2). Until those options exist, weather-factory has
nothing to set — wire up the UCI options before running SPSA.

### config_pruning.json — Pruning / margin constants

All defaults from `src/search.cpp` at the time of writing.

| UCI option | Default | Range | Step | Source in search.cpp |
|---|---|---|---|---|
| `RfpCoeff` | 140 | [60, 240] | 14 | `:1202` `140·depth − (improving?RfpImproving:0)` |
| `RfpImproving` | 60 | [0, 140] | 12 | `:1202` the improving subtrahend |
| `RazorCoeff` | 300 | [120, 500] | 30 | `:1208` `static_eval + 300·depth ≤ alpha` |
| `NullBase` | 4 | [2, 6] | 1 | `:1219` `r = 4 + depth/4 + …` |
| `NullEvalDiv` | 200 | [80, 400] | 24 | `:1219` `min((eval−beta)/200, 3)` |
| `ProbCutMargin` | 200 | [80, 360] | 20 | `:1245` `beta + 200` |
| `FutilityBase` | 150 | [40, 280] | 18 | `:1325` `150 + 110·depth` |
| `FutilityCoeff` | 110 | [40, 200] | 14 | `:1325` the depth coefficient |
| `HistPruneCoeff` | 3500 | [1000, 7000] | 400 | `:1341` `−3500·depth` threshold |
| `SeePruneCoeff` | 80 | [30, 160] | 12 | `:1347` `see_ge(m, −80·depth)` |
| `SingularBetaMult` | 2 | [1, 6] | 1 | `:1365` `tt_score − 2·depth` |
| `SingularDoubleMargin` | 20 | [0, 60] | 8 | `:1380` double-extension margin |
| `AspirationDelta` | 25 | [10, 60] | 6 | `:1604` initial aspiration half-window (cp) |

### config_lmr.json — LMR table formula + adjustments

| UCI option | Default | Range | Step | Source in search.cpp |
|---|---|---|---|---|
| `LmrBase` | 75 | [0, 150] | 12 | `:42` `0.75 + …` (value × 100) |
| `LmrDivisor` | 225 | [150, 350] | 18 | `:42` `… / 2.25` (value × 100) |
| `LmrHistDiv` | 8192 | [4096, 16384] | 1024 | `:1429` `stat_score / 8192` |
| `LmrNonPvAdj` | 1 | [0, 3] | 1 | `:1424` `+1` for non-PV nodes |
| `LmrCutNodeAdj` | 1 | [0, 3] | 1 | `:1425` `+1` at cut nodes |
| `LmrTtPvAdj` | 1 | [0, 3] | 1 | `:1426` `−1` for TT-PV nodes |
| `LmrNotImprovingAdj` | 1 | [0, 3] | 1 | `:1427` `+1` when not improving |

Note: `LmrBase` and `LmrDivisor` are stored as integers × 100 in UCI; the
engine divides by 100.0 inside `init_lmr()` and must re-run `init_lmr()`
whenever either is changed via `setoption`.

### config_combined.json — Narrowed accepted Phase 1 polish

This config starts from the accepted pruning+LMR defaults and narrows ranges
around them. It is intended for a shorter final polish run, not as the first
search-constant tune. SPRT the resulting candidate against `phase1-lmr`, not
against the original defaults.

### config_hcefinal.json — HCE finalization: all SPSA-able inert knobs (18 dims, CURRENT)

**The one-shot "earn your place or get deleted" run (2026-07-13).** The user's
policy for the final pure-HCE version: no inert knobs stay in the code — SPSA
gets one joint run to find value in them, and whatever ends at/near inert is
removed afterwards.

- **= histshape's 11 dims + every remaining SPSA-able inert mechanism:**
  `LmrTtPvAdj`, `LmrNotImprovingAdj`, `PostLmrHistScale`, `QsearchCheckCap`,
  `CapFutDepth/Base/Coeff`.
- **All starts = current defaults** (inert dims start at 0): iteration 1 is
  the SPRT-validated 1.8.0 head — the repaired-wave2 protocol. SPSA may pull
  gated mechanisms up if they carry signal; hand-seeded activations of these
  same knobs broke the mate canaries 8 times in a row, so joint tuning is
  their only remaining route.
- **Excluded by design (removal candidates regardless of the run):**
  `QuietSeeDepth/Coeff` — breaks KBNK at every tried coefficient without a
  history-aware `lmr_depth` (not implemented); `DoubleExtMax` — inert sentinel
  (200 ≈ ∞) with a flat SPSA gradient (all values ≳10 behave identically).
- **Verdict policy (pre-registered):** ~2000 iterations; abort at ~600 if the
  trend is ≤ 0. Converged candidate → bake → **SPRT `elo1=3` + full CTest**
  (the mate canaries stay a hard gate). Wash → keep defaults, then DELETE the
  inert mechanisms and their plumbing.

### config_histshape.json — Phase 7.4 focused search tune (11 knobs, superseded by hcefinal)

**This supersedes wave2 as the first (and likely only) post-Phase-6 search
tune.** Rationale (2026-07-02 EV review, after Basilisk 5.8's TM SPSA washed
and a 30h Rarog SPSA came back negative):

- **Starts from CURRENT defaults** — iteration 1 *is* the SPRT-validated head.
  wave2's start point activates mechanisms with known-negative standalone
  results (`CapFutDepth=7` = the exact config that SPRT'd −2.78;
  `QuietSeeDepth=6` = the mechanism that broke KBNK at every coefficient),
  i.e. a local optimizer starting 5–15 Elo down a hole in 24 noisy dimensions.
- **Only always-live dimensions** — every knob affects every search from move
  one, so 32-game gradients carry signal. Gated-off mechanisms (dead
  dimensions at 0) are excluded.
- **The one doubly-proven target** — both SF and Weiss use asymmetric linear
  history bonus/malus; Basilisk's symmetric `d²` has *never* been tuned. The
  two consumers coupled to history magnitude (`LmrHistDiv`, `HistPruneCoeff`)
  tune jointly, which is exactly what the 6.2/6.3 canary failures demanded.
  `LmrCutNodeAdj` (SF+Weiss both reduce cut-nodes hard; ours is 0) and
  `LmrTtCapture` (SF ~1 ply) are the two cheap always-live extras.
- **Run it AFTER the Phase-7 eval cycles** (§0.5: search constants are
  cp-denominated — tune once, at the final eval scale), ~1000–1500 iterations
  (~12–18h), with a pre-registered abort: if the tuner's trend is ≤ 0 after
  ~600 iterations, stop and keep the defaults.

| UCI option | Value (= current default) | Range | Step |
|---|---|---|---|
| `HistBonusQuad` | 64 | [0, 128] | 13 |
| `HistBonusLin` | 0 | [0, 400] | 40 |
| `HistBonusMax` | 2048 | [512, 4096] | 350 |
| `HistMalusQuad` | 64 | [0, 128] | 13 |
| `HistMalusLin` | 0 | [0, 400] | 40 |
| `HistMalusMax` | 2048 | [512, 4096] | 350 |
| `HistTtMoveBonus` | 0 | [0, 1024] | 100 |
| `LmrHistDiv` | 7830 | [4096, 16384] | 1000 |
| `HistPruneCoeff` | 4210 | [1000, 28000] | 2500 |
| `LmrCutNodeAdj` | 0 | [0, 3072] | 250 |
| `LmrTtCapture` | 0 | [0, 3072] | 250 |

### config_wave2.json — 24-knob joint tune (SUPERSEDED as a first run — see histshape above)

Tunes the history bonus/malus shape (6.3), the two 6.4 knobs, the 6.5 pair,
`QsearchCheckCap` (6.8), and `LmrTtCapture` (6.7) **jointly** with the LMR
formula/adjustments and `HistPruneCoeff` they interact with — every 6.3-6.8
addition ships at a provably-inert or behaviour-identical default specifically
because hand-picking constants against them individually chaotically broke the
KBNK/KQK mate CTests (see PLAN.md §5 6.3-6.5/6.8). This run is where their real
values, if any, are found. **`value` is the SPSA starting point, not the
shipped production default** — several new knobs ship inert (0) for CTest
safety but start this SPSA at a literature-informed, meaningfully non-zero
value so the gradient has signal from iteration 1.

| UCI option | Value | Range | Step | Introduced |
|---|---|---|---|---|
| `HistBonusQuad` | 64 | [0, 128] | 13 | 6.3 |
| `HistBonusLin` | 0 | [0, 400] | 40 | 6.3 |
| `HistBonusMax` | 2048 | [512, 4096] | 350 | 6.3 |
| `HistMalusQuad` | 64 | [0, 128] | 13 | 6.3 |
| `HistMalusLin` | 0 | [0, 400] | 40 | 6.3 |
| `HistMalusMax` | 2048 | [512, 4096] | 350 | 6.3 |
| `HistTtMoveBonus` | 256 | [0, 1024] | 100 | 6.3 |
| `PostLmrHistScale` | 100 | [0, 300] | 30 | 6.4 (ships inert @ 0) |
| `DoubleExtMax` | 20 | [1, 200] | 20 | 6.4 (ships inert @ 200) |
| `CapFutDepth` | 7 | [0, 10] | 2 | 6.5 (ships inert @ 0; SPRT'd active -2.78 as a standalone) |
| `CapFutBase` | 200 | [0, 500] | 50 | 6.5 |
| `CapFutCoeff` | 200 | [0, 500] | 50 | 6.5 |
| `QuietSeeDepth` | 6 | [0, 10] | 2 | 6.5 (ships inert @ 0; needs history-aware `lmr_depth`) |
| `QuietSeeCoeff` | 25 | [0, 120] | 15 | 6.5 |
| `QsearchCheckCap` | 5 | [0, 10] | 2 | 6.8 (ships inert @ 0) |
| `LmrTtCapture` | 512 | [0, 3072] | 250 | 6.7 (ships inert @ 0; 1024 = 1 ply) |
| `LmrBase` | 60 | [0, 150] | 15 | Phase 1 |
| `LmrDivisor` | 209 | [150, 350] | 20 | Phase 1 |
| `LmrHistDiv` | 7830 | [4096, 16384] | 1000 | Phase 1 (coupled to history magnitude) |
| `LmrNonPvAdj` | 1024 | [0, 3072] | 250 | Phase 1 (now fractional, 6.7) |
| `LmrCutNodeAdj` | 0 | [0, 3072] | 250 | Phase 1 (live headroom — SF/Weiss reduce cut nodes hard) |
| `LmrTtPvAdj` | 0 | [0, 3072] | 200 | Phase 1 |
| `LmrNotImprovingAdj` | 0 | [0, 3072] | 200 | Phase 1 |
| `HistPruneCoeff` | 4210 | [1000, 28000] | 2500 | Phase 1 (coupled to history magnitude; range widened in 6.3) |

**Not included in wave2:** the other 12 legacy pruning constants (`RfpCoeff`,
`RfpImproving`, `RazorCoeff`, `NullBase`, `NullEvalDiv`, `ProbCutMargin`,
`FutilityBase`, `FutilityCoeff`, `SeePruneCoeff`, `SingularBetaMult`,
`SingularDoubleMargin`, `AspirationDelta`) stay at their Phase-1-SPSA-accepted
values — they don't interact with the history/LMR changes the way
`LmrHistDiv`/`HistPruneCoeff` do. Also not included: the further legacy
constants PLAN.md's Step 6.9 prose mentions (LMP formula coefficients, NMP
verification gate, ProbCut depth/reduction, qsearch margins, IIR gate,
correction-history weight, razoring depth) — those were never wired up as UCI
knobs; wiring them is a real scoping decision to make explicitly, not silently
bundled into this run. 24 knobs already matches `config_combined.json`'s
precedent for a single joint run; revisit as a wave3 if 6.9 shows spare
headroom.

After convergence: bake the tuned values into `SearchParams.h` (the shipped
*production* defaults for the inert knobs may reasonably stay off even if SPSA
finds a nonzero value helps — re-verify against CTest before baking any value
that reintroduces the KBNK/KQK failure mode), build a fresh PGO binary, then
**both** `sprt.ps1` (vs the pre-tune head) **and** a full `ctest` run before
accepting — the 6.2-6.5/6.8 run showed the CTest canaries catch what a
games-based SPRT alone might not isolate quickly.
