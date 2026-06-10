# SPSA tuning with weather-factory + fastchess

fastchess does **not** have a built-in SPSA tuner. The community-standard tuner
is **weather-factory** (https://github.com/jnlt3/weather-factory), a small
Python driver that perturbs UCI options and runs mini-matches via fastchess.
This folder holds ready-made weather-factory config files for Basilisk.

## One-time setup

The easiest way is `tools/setup_spsa.ps1`, which does all of the below in one
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
   - your local opening book `SuperGM_4mvs.pgn` (copy from `tools\books\`)

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
.\tools\setup_spsa.ps1 -ConfigGroup pruning -EngineSuffix phase1-defaults -Iterations 5000
cd tools\weather-factory
python main.py        # progress + tuned values written to its own state files
```

For the LMR run after pruning has been accepted, build the current accepted head
and tune from that binary:

```powershell
.\tools\build_test.ps1 -Suffix phase1-lmr-baseline
.\tools\setup_spsa.ps1 -ConfigGroup lmr -EngineSuffix phase1-lmr-baseline -Iterations 5000
cd tools\weather-factory
python main.py
```

After both pruning and LMR have been accepted, run only a short narrowed polish
from the accepted LMR head:

```powershell
.\tools\setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
cd tools\weather-factory
python main.py
```

weather-factory writes the running parameter values to its state file every
`save_rate` games; stop it any time with Ctrl-C.

Run `python main.py` again to resume the same configured run. Re-running
`tools\setup_spsa.ps1` starts a fresh run and archives old `state.json`,
`games.pgn`, and graph output unless `-Resume` is passed.

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
| `threads` | 15 | concurrency = physical cores (16) − 1 |
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
