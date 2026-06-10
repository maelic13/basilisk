# Basilisk Strength Improvement Plan

> Current checkpoint: Basilisk is a feature-rich C++23 HCE engine at version
> `1.5.0` (includes the accepted Phase 1 search constants). Phase 0 and Phase 1
> are complete: pruning SPSA +18.87 +/- 8.81 Elo, LMR SPSA +15.63 +/- 8.02 Elo,
> combined polish rejected and reverted, validation vs 1.4.9 = +27.16 Elo.
> The immediate next work is **Phase 2 evaluation tuning (Texel pipeline)**.
> Do not start new search/eval features until those gates stop paying.
>
> 2026-06 measurement update (basis for Phase 2+ revision below):
> Basilisk's depth deficit vs Stockfish is **not** time management and **not**
> nps. Measured at `go movetime 1000`, single thread, same machine:
>
> | Engine | startpos | middlegame | nps | nodes for final depth |
> |---|---|---|---:|---:|
> | Basilisk 1.5.0 | depth 18 | depth 18 | ~2.8M | ~1.7-1.8M |
> | Stockfish 18 | depth 23 | depth 21 | ~1.0M | ~0.8-1.0M |
>
> Basilisk searches ~3x more nodes per second but needs ~2.3-3x more nodes per
> iteration of depth (effective branching factor ~2.2 vs ~1.8) and its HCE eval
> is far less accurate than Stockfish's NNUE. At 10 ms/move Basilisk reaches
> depth 10 using 8 ms (fine; Rarog only reaches depth 7 and stops at 4 ms -
> that is Rarog's bug, not a Basilisk problem). Conclusion: the levers, in
> order, are eval accuracy (Phase 2, Phase 4), search selectivity (Phase 3),
> and only then time management (Phase 5).
>
> Honest ceiling note: full-strength Stockfish is ~500-700 Elo above Basilisk
> 1.5.0. The strongest hand-crafted-eval engines ever built (Stockfish 11 era)
> sit ~200 Elo below today's Stockfish. Phases 2-6 realistically buy
> +150 to +350 Elo. Actual parity with modern Stockfish requires Phase 7
> (NNUE). The phases below are still the right order: a well-tuned HCE engine
> is also the best data generator and test harness for a future NNUE.
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

### 2026-06 audit addendum (code-level findings feeding Phases 2-5)

Search (`src/search.cpp`) is modern but leaves known Elo on the table:

- LMR table is integer-granularity (`init_lmr` truncates to int); all strong
  engines use fractional reductions accumulated in fixed-point.
- Static eval used for pruning ignores usable TT score bounds (Stockfish
  refines `eval` with `ttValue` when the bound allows).
- Qsearch never generates quiet checks; first-ply checking moves are a
  standard tactical-safety gain.
- History bonus is `min(depth*depth, 2048)` with equal malus; modern engines
  use larger, separately-tuned linear bonus/malus formulas.
- No "search deeper / search shallower" adjustment after an LMR re-search.
- Double singular extensions are uncapped along a line.
- Many second-wave constants are still inline (LMP formula, NMP verification
  depths, ProbCut depths, qsearch margins 150/200/-50, history caps,
  correction weights, IIR gates).

Eval (`src/eval.cpp`, 735 lines) is the weakest layer relative to the search:

- No attack-map infrastructure: `is_attacked_by()` is re-computed per square
  for hanging pieces and passed-pawn stops (slow and limits feature quality).
- No piece-on-piece threats beyond pawn attacks; no safe-check king terms; no
  weak-square logic; mobility is a linear per-square count, not per-count
  tables.
- Every weight outside the PeSTO PSTs is hand-guessed.
- Pawn-structure terms are cached in a per-Searcher pawn hash keyed only by
  `pawn_key` - **any tuner that changes eval weights at runtime must disable
  or clear this cache** (see Phase 2 pitfalls).

Time management (`Searcher::compute_time_limit`):

- Fixed `movetime` path is correct (verified at 10 ms/move).
- Clock path is simplistic: `soft = remaining/25 + 0.75*inc`,
  hard = 20/30/50% of remaining by tier. It ignores increment accumulation
  over the remaining game and per-move overhead accumulation. Adequate, but
  improvable - Phase 5.
- Adaptive iteration-stop (stability, score-drop, root-effort scaling) already
  exists and works.

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

- No `EvalParams` abstraction or eval-weight loader exists.
- No Texel dataset or tuner exists.
- Several second-wave search constants remain inline (now scheduled in
  Phase 3 together with the search-feature candidates above).

### Main Diagnosis

Unchanged, now backed by measurement: Basilisk has plenty of features, but the
eval constants are hand-guessed and the search is less selective than top
engines. The plan:

1. Fit the HCE evaluation to data (Phase 2 - biggest lever).
2. Close part of the branching-factor gap (Phase 3).
3. Add eval features the tuner can then fit (Phase 4).
4. Polish time management for increment games (Phase 5).
5. Keep NNUE as the final ceiling-raiser (Phase 7).

---

## 1. Non-Negotiable Gates

Apply these to every phase.

1. **One candidate at a time.** Do not bundle unrelated feature work with tuned
   values.
2. **SPSA/Texel propose; SPRT decides.** A tuned value set is only a candidate.
3. **SPRT every kept strength change.** Default strength gate:
   `elo0=0 elo1=5 alpha=0.05 beta=0.05`. Use `elo1=3` for small follow-up
   refinements and single search features.
4. **Use deployment-like conditions for the final gate.** `st=0.1`, Hash 64 MB,
   Threads 1, repo-local `SuperGM_4mvs.pgn`, concurrency no higher than physical
   cores minus one. Exception: time-management changes (Phase 5) must be gated
   at clock time controls (`tc=...`), because `st` does not exercise them.
5. **Use PGO `pext` binaries for local strength testing.** `tools/build_test.ps1`
   is the default build path for SPRT/SPSA candidates on this machine.
6. **Use `bench 13` correctly.** For pure refactors, the fingerprint must match
   the recorded baseline exactly. For tuned values or real behavior changes, the
   fingerprint may change; record it, but do not interpret it as Elo.
7. **Preserve the eval boundary.** Keep search calling
   `Evaluator::evaluate(const Board&)`; do not spread eval internals into
   search. This keeps Phase 2 manageable and Phase 7 possible.
8. **Keep release UCI clean.** Tuning options remain behind `-DTUNE=ON`; release
   builds should not expose development knobs.
9. **Bake accepted values into defaults.** Runtime loaders (eval file, UCI
   spins) are tuning vehicles only; SPRT gates and releases always run on
   compiled-in defaults.
10. **Texel candidates are judged by SPRT, not by loss.** Lower training/holdout
   loss that does not survive SPRT is overfitting; revert it.

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
- Calibration self-vs-self SPRT is intentionally skipped.

Do not spend more time here unless a script fails.

---

## 3. Phase 1 - Search Constant Tuning (Complete)

Status: complete. Kept for the record; see git history and `user_dev_guide.md`
for details.

- Pruning SPSA accepted: `+18.87 +/- 8.81 Elo`, 2930 games.
- LMR SPSA accepted: `+15.63 +/- 8.02 Elo`, 3714 games.
- Narrowed combined polish rejected (`-0.40 +/- 3.20 Elo`) and reverted.
- Validation vs 1.4.9 defaults: 2000 games, 53.90%, ~+27.16 Elo.
- Accepted head: `phase1-final` = current `master` defaults (shipped as 1.5.0).
- Conclusion recorded: currently exposed search constants are saturated;
  further search-constant work moved into Phase 3 where it rides on new
  features and newly exposed second-wave constants.

### Exposed Phase 1 Parameters

Already implemented as UCI spin options under `BASILISK_TUNE` (see
`src/SearchParams.h` for current accepted defaults): `RfpCoeff`,
`RfpImproving`, `RazorCoeff`, `NullBase`, `NullEvalDiv`, `ProbCutMargin`,
`FutilityBase`, `FutilityCoeff`, `HistPruneCoeff`, `SeePruneCoeff`,
`SingularBetaMult`, `SingularDoubleMargin`, `AspirationDelta`, `LmrBase`,
`LmrDivisor`, `LmrHistDiv`, `LmrNonPvAdj`, `LmrCutNodeAdj`, `LmrTtPvAdj`,
`LmrNotImprovingAdj`.

---

## 4. Phase 2 - Eval Tuning via Texel (Big Lever)

Goal: fit Basilisk's HCE weights to data while preserving the clean
`Evaluator::evaluate(const Board&)` boundary. Realistic target: +60 to +150
Elo self-play across the whole phase.

Method decision (2026-06): use a **linear-trace gradient tuner**
(Ethereal-style, "Evaluation & Tuning in Chess Engines" approach), not
coordinate descent and not SPSA. Basilisk's eval is almost entirely linear in
its weights: every term is `weight * count`, the PSTs and the king-safety
table are one-hot lookups, and tapering uses a position-derived phase that
does not depend on the weights. Gradient descent over a sparse trace fits all
~900 parameters in minutes on CPU, which coordinate descent cannot do for the
768 PST entries.

### Step 2.0 - `EvalParams` struct, default-equivalence (pure refactor)

1. Create `src/EvalParams.h` defining `struct EvalParams` containing **every**
   tunable weight in `src/eval.cpp`, with defaults exactly equal to the current
   inline constants:

   - Material: `mg_val[7] = {0,82,337,365,477,1025,0}`,
     `eg_val[7] = {0,94,281,297,512,936,0}`.
   - PSTs: `pst_mg[6][64]`, `pst_eg[6][64]` (PeSTO values, white perspective,
     a1=0 layout as in `eval.cpp`).
   - Passed pawns: `passed_mg[8] = {0,5,10,20,35,60,100,0}`,
     `passed_eg[8] = {0,10,17,35,62,100,170,0}`; supported-passer
     `pass_supp_mg = 8`, `pass_supp_eg_base = 6`, `pass_supp_eg_rank = 4`;
     candidate `cand_mg = 6`, `cand_eg = 10`; free-stop `pass_free_mg = 2`,
     `pass_free_eg = 6`; safe-stop `pass_safe_eg = 8`.
   - Pawn structure: doubled `(-10,-20)`, isolated `(-15,-20)`,
     connected `(7,5)`, backward `(-10,-15)`.
   - Bishop pair `(30,50)`.
   - Rook: open file `(25,10)`, semi-open `(12,8)`, 7th rank `(20,40)`,
     behind own passer `(15,25)`, enemy rook behind our passer `(-10,-20)`.
   - Knight outpost `(25,15)`.
   - Mobility: `mob_mg[7] = {0,0,4,5,2,1,0}`, `mob_eg[7] = {0,0,4,5,4,2,0}`.
   - Pawn threats: minor `(18,12)`, rook `(28,18)`, queen `(45,30)`.
   - King safety: `ks_unit[7] = {0,0,2,2,3,5,0}`, coordination bonus `4`,
     open-king-file units `2`, `safety_table[25]` (current values),
     no-queen scaling kept frozen as `*2/3`.
   - Shelter: missing-pawn penalty center `20` / flank `10`; distance-1 bonus
     `15`, distance-2 bonus `7`; storm weight king-file `7` / adjacent `4`.
   - Hanging pieces: `hang[7] = {0,0,45,45,60,80,0}`.
   - Passer king proximity: `prox_base = 2` (the `(2 + rel_r)` multiplier).
   - Space: `space_mg = 2`. Tempo: `tempo = 10`.
   - Trapped bishop `(60,40)`.
   - **Frozen (listed in the struct as `// frozen` comments, not tuned):**
     phase weights, mate-drive term (`5*lk_center + 4*(14-dist)`), OCB scale
     `32 + 4*pawns`, two-knights draw rule, 50-move damping, the
     `popcount(zone_hits)/2` and `n_attackers` king-zone composition logic.

2. Add an X-macro registry in the same header so loaders and the tuner can
   iterate parameters generically without reflection:

   ```cpp
   // EVAL_PARAM(name, field, length)  - length 1 for scalars, N for arrays
   #define EVAL_PARAM_LIST(X) \
       X(MgVal, mg_val, 7) \
       X(EgVal, eg_val, 7) \
       X(PstMgPawn, pst_mg[0], 64) \
       /* ... every field ... */
   ```

3. Rework `eval.cpp` to read every weight from a global
   `EvalParams g_eval_params;` (or a pointer member on `Evaluator`).
   `init_eval_tables()` becomes `init_eval_tables(const EvalParams&)` and must
   be re-run whenever params change (it bakes material+PST into
   `MG_TABLE`/`EG_TABLE`).

4. Acceptance: `bench 13` fingerprint **identical** to baseline
   (`4,972,548` nodes from the same parent commit), 8/8 CTest, and release
   `bench` wall-time within ~3% of the parent build (params indirection must
   not cost measurable nps; PGO usually erases it).

### Step 2.1 - Tune-time loader and dumper

Under `BASILISK_TUNE` only:

- On startup, if env var `BASILISK_EVAL_FILE` is set, load it. Format: one
  `name index value` per line (index 0 for scalars), unknown names = hard
  error, then re-run `init_eval_tables` and clear pawn caches.
- Add a plain `dumpeval` console command (like `bench`) that writes the
  current parameters in exactly that format to stdout - this defines the
  round-trip format the tuner emits.
- Acceptance: dump -> load -> dump round-trip is byte-identical; release build
  exposes neither.

### Step 2.2 - Trace instrumentation and tuner binary

1. Add compile flag `-DTEXEL_TRACE` (CMake option `TEXEL=ON`, separate target
   `basilisk-texel`; never enabled in release/tune builds). Under it,
   `eval.cpp` records into a thread-local `EvalTrace`:

   ```cpp
   // Same shape as EvalParams, but int8/int16 counts per color.
   // TR(field_expr, color, n) adds n to the white/black counter.
   ```

   Every `mg += sign * W * n` site becomes `mg += sign * W * n; TR(W, c, n);`
   via a macro so normal builds compile to the same code. One-hot lookups
   (PSTs, `passed_mg[r]`, `safety_table[u]`) trace the specific index.

2. **Pitfall (mandatory):** under `TEXEL_TRACE`, the pawn-structure cache in
   `Evaluator::eval_pawns` must be bypassed entirely (always compute), because
   cached entries don't re-emit trace counts and stale weights would poison
   reconstruction.

3. Per-position record produced by tracing one `evaluate()` call:
   - sparse `(param_index, white_count - black_count)` pairs split into mg/eg
     contributions, plus `phase` (0..24),
   - the multiplicative scale factor actually applied (OCB scaling, 50-move
     damping; store as a rational or float),
   - a `rest` constant = exact integer eval minus the reconstructed linear
     part at default weights (this absorbs all frozen non-linear terms such as
     the mate-drive bonus, making reconstruction exact at the defaults),
   - white-POV: store eval before the final side-to-move negation; tempo is a
     traced feature with count +1 (white to move) or -1.

4. Tuner (`tools/texel/tuner.cpp`, part of the `basilisk-texel` target):
   - Loads a dataset file of `FEN;result` lines (result from White's POV:
     `1.0`, `0.5`, `0.0`).
   - Traces every position once, holding the sparse records in memory
     (~1-2M positions is fine).
   - Fits sigmoid scale `K` first: minimize MSE of
     `result - sigmoid(K * default_eval)` by simple grid + refinement.
   - Optimizes parameters with Adam (lr ~0.05, full-batch gradients are fine;
     epochs until holdout loss stops improving). Predicted eval per position:
     `E(w) = scale * ( (mg(w)*phase + eg(w)*(24-phase))/24 + rest )`.
   - Supports `--tune <group>` masks (material / scalars / kingsafety / pst /
     all) - untuned params stay at their loaded values.
   - Optional small L2 toward the PeSTO values for PST entries
     (`lambda ~ 1e-6`), none for scalars.
   - Reports train and holdout loss every N epochs; writes
     `tools/texel/out/eval_params.txt` in the Step 2.1 format.
   - Acceptance test before any tuning run: for 10,000 random dataset
     positions, reconstructed `E(default)` equals `evaluate()` exactly.

### Step 2.3 - Dataset

1. Self-play generation (primary source), using the existing fastchess and the
   current head binary. New script `tools/datagen.ps1`:

   ```powershell
   # ~60k games, fixed small node count for diversity + speed, random book
   tools\bin\fastchess.exe `
     -engine cmd=tools\test_engines\basilisk-phase2-base-pext-pgo.exe name=A `
     -engine cmd=tools\test_engines\basilisk-phase2-base-pext-pgo.exe name=B `
     -each tc=inf nodes=8000 option.Hash=16 option.Threads=1 `
     -openings file=tools\books\SuperGM_4mvs.pgn format=pgn order=random `
     -rounds 30000 -games 2 -repeat -concurrency <cores-1> `
     -pgnout file=tools\texel\data\selfplay.pgn
   ```

   Adjudication defaults are fine; node-limited games are fast (~1-2 s/game)
   and produce labelings consistent with the engine's own play style.

2. Extraction script `tools/texel/extract.py` (python-chess):
   - Skip the first 8 full moves and the final 6 plies of each game.
   - Skip positions in check and positions where the **played move** was a
     capture or promotion (cheap quietness proxy).
   - Sample at most 12 random qualifying plies per game (decorrelation).
   - Deduplicate by FEN (first 4 fields).
   - Split holdout **by game** (5% of games, not positions) into
     `train.book` / `holdout.book`.
   - Target: at least 1.5M train positions. If short, generate more games
     (different `nodes=` values 5000-12000 add variety).

3. Optional refinement (only if Step 2.4's first SPRT fails): add a
   `quietfilter` mode to `basilisk-texel` that drops positions where
   `qsearch != static eval`, the rigorous quietness filter.

### Step 2.4 - Staged tuning, each stage SPRT-gated

Naming: build each accepted stage as `phase2-<stage>` via
`tools/build_test.ps1 -Suffix phase2-<stage>`; SPRT vs the previous accepted
head. After acceptance, bake values into `EvalParams.h` defaults, record
`bench 13`, run CTest.

| Stage | Tuned group | SPRT gate | Notes |
|---|---|---|---|
| 2.4a | Material values only (10 params) | elo1=5 | Cheap pipeline proof. If this fails, debug K-fit, dataset, reconstruction - do not proceed. |
| 2.4b | All scalar terms except king safety and PSTs (~85 params) | elo1=5 | Mobility, pawn structure, passers, piece terms, threats, hanging, space, tempo. |
| 2.4c | King safety block: `safety_table[25]`, `ks_unit`, shelter, storm (~45 params) | elo1=3 | Keep zone-composition logic frozen. |
| 2.4d | PSTs + material refit (778 params) | elo1=5 | Use L2 toward PeSTO; needs the full dataset. |
| 2.4e | Global polish: everything unfrozen, low lr | elo1=3 | Stop here regardless of outcome. |

Decision rules per stage: tuned values that are wildly implausible (signs
flipped on well-understood terms, passer table non-monotonic by hundreds)
mean dataset or trace bugs - fix before SPRT. A failed SPRT after a sane fit
means revert that stage and continue to the next; one retry only if a concrete
defect was found and fixed.

### Step 2.5 - Phase boundary validation

- `tools/gauntlet.ps1` 2000 games vs `phase1-final`.
- Optional external check: drop the new binary into the LittleBlitzer pool
  (`D:\chess\little blitzer`) against the Stockfish UCI_Elo anchors.
- Update `CHANGELOG.md`, tag the release per Section 10.

### Why PSTs Are Last

Unchanged rationale: largest parameter block, easiest to overfit, and the
scalar stages prove the pipeline transfers to Elo before spending the PST
budget. The gradient tuner makes PSTs *feasible*; the staging makes them
*safe*.

---

## 5. Phase 3 - Search Efficiency Wave (close the EBF gap)

Goal: reduce nodes-per-depth toward Stockfish's regime (measured gap: ~2.2 vs
~1.8 effective branching factor). This phase absorbs the old "Phase 1.5
second-wave constants" and adds specific search features. Realistic target:
+30 to +80 Elo self-play.

Rules: one item at a time, in the order below; each item is implemented,
CTest-ed, `bench 13`-recorded, then SPRT-gated (`elo1=3` unless noted).
Constants introduced by an item are exposed under `BASILISK_TUNE` immediately
but SPSA-tuned only in Step 3.9. Skip to the next item on a failed SPRT; do not
stack untested changes.

### Step 3.1 - TT-bound eval refinement (~10 lines, do first)

In `negamax` after computing `static_eval` (non-check, non-excluded path):
if a TT entry was found with a usable score
(`tt_flag == TT_EXACT`, or `TT_BETA && tt_score > static_eval`, or
`TT_ALPHA && tt_score < static_eval`), use `tt_score` in place of
`static_eval` for the **pruning decisions only** (RFP, razoring, NMP entry,
futility) - keep `ss->eval` as the raw corrected value used for `improving`
and correction-history updates. Mirror in qsearch stand-pat: if TT bound
applies, tighten `stand_pat` the same way.

### Step 3.2 - History formula upgrade

Replace `bonus = min(depth*depth, 2048)` in `update_all_histories` with
separately scaled linear forms:

```
bonus = std::min(p.hist_bonus_mul * depth - p.hist_bonus_sub, p.hist_bonus_max);
malus = -std::min(p.hist_malus_mul * depth - p.hist_malus_sub, p.hist_malus_max);
```

Initial values: `bonus_mul=170, bonus_sub=90, bonus_max=1700;
malus_mul=180, malus_sub=100, malus_max=1500`. Expose all six. SPRT.

### Step 3.3 - Fractional LMR

`lmr_table_` becomes `int` in 1024ths: `1024 * (base + log(d)*log(m)/div)`.
All adjustments scale accordingly (`lmr_non_pv_adj` etc. become ~1024-unit
values; `move_stat_score / p.lmr_hist_div` is already integer - multiply
numerator by 1024 before dividing). Final
`reduction = total_r >> 10` clamped as today. This is behavior-changing
(rounding), so SPRT it on its own with the old adjustment defaults converted
(`1 -> 1024`).

### Step 3.4 - LMR input: TT-move-is-capture

Track `tt_capture = tt_move != MOVE_NONE && tt_move is capture` at node entry;
add `+p.lmr_tt_capture (default 1024)` to quiet-move reductions when true.
SPRT (cheap; bundle-eligible with Step 3.5 only if both individually fail near 0).

### Step 3.5 - Post-LMR deeper/shallower re-search

Where the code currently re-searches at full depth after a reduced search
returns `score > alpha`: first compute
`do_deeper = score > best_score + p.deeper_margin (default 64) + 2*reduction`
and `do_shallower = score < best_score + p.shallower_margin (default 8)`;
re-search at `new_depth + (do_deeper ? 1 : do_shallower ? -1 : 0)`. SPRT.

### Step 3.6 - Qsearch quiet checks

At `qply == 0` and not in check, after the capture loop fails to raise alpha:
generate quiet moves, filter to `move gives check && see_ge(move, 0)`
(use `Board::check_squares(pt, side)` to pre-mask candidates cheaply), search
them like captures. Cap at the first 4-6 such moves. SPRT (`elo1=3`); if it
fails, retry once gating by `depth >= -1`-style recent-entry only.

### Step 3.7 - Double-extension cap

Add `int double_exts` to `SearchStack`, incremented when the singular path
extends by 2, inherited by children (`(ss+1)->double_exts = ss->double_exts`);
disallow the 2-ply extension when `double_exts >= p.double_ext_max
(default 8)`. Likely Elo-neutral but bounds tactical blowups; SPRT with
`elo0=-3 elo1=1` (non-regression gate).

### Step 3.8 - Razoring restriction experiment

Try `depth <= 1` (from `depth <= 3`) - RFP largely covers razoring's range
and the qsearch verification call is not free. SPRT; keep whichever passes.

### Step 3.9 - Second-wave constants SPSA

Only after Steps 3.1-3.8 verdicts are in. Expose in `SearchParams` (one coherent
group, ~12-16 knobs): LMP formula coefficients (the `3 + d*d` / `2 + d*d/2`
pair), NMP verification depth gate (10) and `depth/4` divisor, ProbCut depth
gate (5) and reduced depth (-4), qsearch margins (150 futility, 200/-800/200
SEE-threshold clamp, -50 late-SEE, the `i >= 6` gate), history-pruning depth
gate, IIR depth gate and TT-depth offset, correction-history `/5` weight and
update clamp, move-ordering check bonus (32000), plus the Phase 3 constants
introduced above. Default-equivalence `bench 13` first, then
`setup_spsa.ps1 -ConfigGroup wave2` (add the group to the script following
the existing pruning/lmr pattern), 5000 iterations, SPRT the result.

---

## 6. Phase 4 - Eval Features (attack-map era) + Retune

Goal: add the eval features that separate basic HCE from strong HCE, then let
the Phase 2 tuner fit them. Realistic target: +40 to +100 Elo self-play.
Prerequisite: Phase 2 pipeline working (the tuner is what makes new eval
features pay off).

Per-feature loop: implement -> trace-instrument -> CTest -> Texel-retune the
new group (+ affected neighbors) -> bake -> SPRT (`elo1=3`) -> keep/revert.

### Step 4.0 - Attack-map infrastructure (pure refactor, do first)

In `evaluate()`, compute once per call:

```cpp
Bitboard attacked_by[NCOLORS][PIECE_TYPE_NB]; // per piece type, pawns included
Bitboard attacked[NCOLORS];                   // union
Bitboard attacked2[NCOLORS];                  // attacked by 2+ pieces
```

Rewrite mobility, king safety, and hanging-piece terms to use these instead of
recomputing attacks / calling `is_attacked_by` per square. Identical eval
output required: `bench 13` must not change. Expect a small nps gain.

### Step 4.1 - Threats package (Stockfish-classical shapes)

New traced params (initial values from SF-classical, scaled to Basilisk's
pawn=82 base): `threat_by_minor[pt]` and `threat_by_rook[pt]` arrays (mg,eg
each), threat-by-king, hanging refinement (enemy piece attacked, not defended
or attacked twice and defended once), pawn-push threats (squares our pawns
could attack after one safe push), restricted-piece bonus. Replace the current
flat `hang[]` term with this package. Retune threat group + SPRT.

### Step 4.2 - King safety v2: safe checks and weak ring

Using Step 4.0 maps: add per-piece-type **safe-check** units (squares from
`check_squares(pt)` that the enemy attacks and we don't defend, or defend only
with the king/queen), and **weak-ring** units (king-ring squares attacked and
not solidly defended). Both feed the existing `attack_units -> safety_table`
funnel as additional tunable unit weights. Retune king-safety block + SPRT.

### Step 4.3 - Per-count mobility tables

Replace linear `mob * w` with one-hot tables indexed by popcount:
`mob_n[2][9]`, `mob_b[2][14]`, `mob_r[2][15]`, `mob_q[2][28]` (mg/eg).
Initialize from the current linear values (`table[i] = i * w`), refine the
mobility *area* to exclude squares occupied by own king and queen and own
blocked pawns (SF-classical definition). Retune mobility group + SPRT.

### Step 4.4 - Pawn-structure refinement

Connected/phalanx bonus by rank (SF formula `seed[r]` style, one-hot traced),
weak-unopposed penalty, blocked pawns on 5th/6th, king-pawn-distance endgame
term. Keep everything inside `eval_pawns`/pawn cache where it only depends on
pawns (+ king squares: add a small separately-cached king-pawn table keyed by
`pawn_key ^ king squares` if needed). Retune pawn group + SPRT.

### Step 4.5 - Endgame scaling rules

Cheap, high-value drawishness handling on top of the existing OCB rule:
winning side has no pawns and <= minor-piece advantage -> scale toward draw;
rook endings with all pawns on one flank -> mild scale-down; opposite-colored
bishops with other pieces -> mild scale-down (distinct from pure OCB). Frozen
constants first (not traced), SPRT directly; expose for tuning only if the
first SPRT is borderline.

---

## 7. Phase 5 - Time Management (clock games)

Goal: better budgets at increment time controls (the LittleBlitzer pool runs
base+increment; `st` SPRTs never exercise this code). Verified working today;
this is refinement, not repair. Realistic target: +5 to +20 Elo at clock TCs.

### Step 5.1 - Budget formula upgrade

Replace the body of `compute_time_limit` clock path with an
increment-and-overhead-aware budget:

```cpp
int mtg   = limits.movestogo > 0 ? std::min(limits.movestogo, 50)
                                 : p.tm_mtg;            // default 38
int64_t time_left = std::max<int64_t>(1,
      remaining + (int64_t)inc * (mtg - 1) - (int64_t)overhead * (2 + mtg));
int soft_ms = std::clamp<int64_t>(time_left * p.tm_soft_num / 1000, // default 41
                                  1, remaining * 8 / 10);
int hard_ms = std::clamp<int64_t>((int64_t)soft_ms * p.tm_hard_mul,  // default 4
                                  soft_ms, remaining * 8 / 10 - overhead);
```

Keep the existing 10 ms floors, the `movetime` path, and the adaptive
iteration-stop scaling (stability/score-drop/effort) exactly as they are.
Expose `tm_mtg`, `tm_soft_num`, `tm_hard_mul` under `BASILISK_TUNE`.

### Step 5.2 - Gating

Add a `-Tc` parameter to `tools/sprt.ps1` that switches fastchess from
`st=0.1` to `tc=<value>` (e.g. `-Tc "10+0.1"`). Gate Step 5.1 with two SPRTs,
both `elo0=0 elo1=3`:

- `tc=10+0.1` (base-time dominated), and
- `tc=1+0.5` (increment dominated, mirrors the LittleBlitzer pool shape).

Then scan the fastchess logs for time losses at full concurrency before
accepting (Section 10 discipline).

### Step 5.3 - Optional SPSA

Only if Step 5.1 passes: SPSA the three constants at `tc=10+0.1` (add a
`ConfigGroup tm`), then re-SPRT. Skip if gains look < 5 Elo - this lever is
small and the games are expensive.

---

## 8. Phase 6 - Remaining Feature Menu (only after Phases 2-5 plateau)

Only start this after the earlier phases no longer produce accepted
candidates. Candidates, roughly in order of expected value:

- Correction-history-driven pruning adjustments (use correction magnitude to
  gate RFP/futility aggressiveness).
- ProbCut TT interaction (skip when TT suggests it can't fail high; store
  ProbCut results at `depth-3` consistently).
- Singular refinement: TT-score-dependent margins, negative-extension width.
- History aging schedule (the `/2 every 2 searches` is coarse; try per-search
  decay factors).
- Continuation-history for captures; countermove-history merge.
- Eval: bishop long-diagonal, rook trapped-by-king, queen infiltration,
  pawn-storm vs castled-side detection, knight-distance-to-kings.
- Multi-threaded improvements (only if pool play moves beyond Threads=1):
  proper main-thread voting, depth-skip schedules.

Per-feature checklist unchanged: implement exactly one feature, tests, CTest,
`bench 13`, expose constants under `BASILISK_TUNE`, tune, SPRT vs current
head, keep only H1-accepted.

---

## 9. Phase 7 - NNUE (Future Project, the actual path to Stockfish parity)

NNUE remains the biggest long-term ceiling - several hundred Elo above any
HCE - but it stays out of scope until Phases 2-5 are exhausted. Everything
above feeds it: the tuned HCE engine generates better training data, and the
SPRT harness gates the swap.

Guardrails now (unchanged):

- Keep `Evaluator::evaluate(const Board&)` as the search boundary.
- Preserve cheap access to piece placement and incremental keys (`Board`
  already maintains pawn/minor/nonpawn keys; an NNUE accumulator hooks into
  `make_move`/`unmake_move` the same way).
- Do not design the Texel/eval loader in a way that search depends on HCE
  internals.
- Reuse the Phase 0 SPRT/validation harness when NNUE work starts.

When the time comes, the proven small-engine path is: 768->(256x2)->1
perspective net with SCReLU, trained on self-play data from the tuned HCE
(Phase 2's datagen pipeline scaled up to tens of millions of positions),
using an external trainer (e.g. bullet). Not now.

---

## 10. Release Discipline

After each accepted phase:

- Build fresh PGO assets for local testing (`pext`) and distribution (`avx2` or
  normal release as appropriate).
- Run fixed-game validation, not just SPRT self-play.
- Scan logs for illegal moves, timeouts, disconnects, crashes, and `bestmove
  0000` from legal positions.
- Update `CHANGELOG.md` and version metadata only after both self-play SPRT and
  fixed-game validation are acceptable.

---

## 11. Quick Commands

```powershell
# One-time fresh-clone setup
.\tools\setup_tools.ps1

# Build a named pext-PGO+TUNE binary into tools\test_engines\
.\tools\build_test.ps1 -Suffix phase2-base

# SPRT a gain candidate (st=0.1 default conditions)
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-phase2-material-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-phase2-base-pext-pgo.exe `
    -NameA "Phase2Material" -NameB "Phase2Base"

# Refactor/default-equivalence SPRT, only when needed
.\tools\sprt.ps1 ... -Elo0 -3 -Elo1 3

# Time-management SPRT (after Step 5.2 adds -Tc)
.\tools\sprt.ps1 ... -Tc "1+0.5" -Elo1 3

# Phase 2: dataset generation + extraction (after Steps 2.2/2.3 exist)
.\tools\datagen.ps1 -Suffix phase2-base -Rounds 30000
python tools\texel\extract.py tools\texel\data\selfplay.pgn

# Phase 2: build and run the tuner
cmake --preset release-pext -DTEXEL=ON
cmake --build build\release-pext --target basilisk-texel
.\build\release-pext\basilisk-texel.exe --data tools\texel\data\train.book `
    --holdout tools\texel\data\holdout.book --tune material

# SPSA (existing groups; Phase 3 adds wave2, Phase 5 adds tm)
.\tools\setup_spsa.ps1 -ConfigGroup wave2 -EngineSuffix phase3-base -Iterations 5000
cd tools\weather-factory
python main.py
```

---

## 12. Bottom Line

Phase 1 proved the harness converts tuning into real Elo (+27 validated).
Measurement shows the remaining gap to Stockfish is eval accuracy first and
search selectivity second - not speed (Basilisk is 3x faster in nps) and not
time management (verified sound down to 10 ms/move). So: build the Texel
pipeline and fit the eval (Phase 2), close part of the branching-factor gap
with specific, individually-gated search changes (Phase 3), add the eval
features the tuner can exploit (Phase 4), polish clock handling (Phase 5).
That is the maximum extractable from HCE; parity with modern Stockfish is
Phase 7's job, and every earlier phase makes that switch cheaper and better
tested.
