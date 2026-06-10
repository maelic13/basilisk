# Basilisk Strength Improvement Plan

> Current checkpoint: Basilisk is a feature-rich C++23 HCE engine at version
> `1.5.0` (includes the accepted Phase 1 search constants). Phase 0 and Phase 1
> are complete: pruning SPSA +18.87 +/- 8.81 Elo, LMR SPSA +15.63 +/- 8.02 Elo,
> combined polish rejected and reverted, validation vs 1.4.9 = +27.16 Elo.
> Phase 2 Texel infrastructure and the cheap, structure-independent scalar fits
> are also complete and baked: material (+29), mobility (+8.8), passers (+16.6),
> pawn-structure (+30.7), hanging (baked); rooks rejected. Phase 2.9 robustness
> is **implemented** — the clock-TC gauntlet fix (2.9.2) plus the time-forfeit
> fix (the 2.9.1 reserve, now folded into the Phase 6 clock-TM port; see §4.9 /
> Step 6.1b). The immediate next work is to **clear the forfeit gate (clock-TC
> gauntlet, `t=`≈0) and cut release `1.6.0` (Step 2.9.3)**, then start **Phase 3
> eval structure**. Do not start the Phase 4 eval data-fit or any search SPSA
> until the Phase 3 structure exists (see §0.5).
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
- `tools/sprt.ps1` runs fastchess SPRT at the unified SPSA/SPRT condition:
  `tc=3+0.03`, Hash 64 MB, Threads 1, repo-local book
  `tools/books/SuperGM_4mvs.pgn`. Use `-TC "10+0.1"` for LTC confirmation
  and `-MoveTime 0.1` only for the optional old fixed-movetime sanity gauntlet.
- `tools/gauntlet.ps1` runs fixed-game phase-boundary validation matches.
- `tools/setup_spsa.ps1` writes weather-factory configs for either the pruning
  or LMR parameter group and can select which built engine suffix to tune from.
- `src/SearchParams.h`, `Parameters::uciOptions()`, `Parameters::setOption()`,
  and `Engine::build_limits()` already wire search parameters into search under
  `BASILISK_TUNE`.
- `src/EvalParams.h`, the tune-only eval loader/`dumpeval`, the `basilisk-texel`
  target with `--verify` trace reconstruction, `tools/datagen.ps1`, and
  `tools/texel/{extract,import_beast,sample_fens}.py` all exist (Phase 2 infra).
  Phase 2 scalar tuning has banked the accepted values listed in §4.

### Remaining Gaps (updated 2026-06-20)

The Phase 2 infra is now built, so the old "Still Missing" list is obsolete.
What remains:

- The Texel tuner fits **linear** traces well, but the king-safety knobs are
  composite/nonlinear (zero-seeded units, safety-table index dependence). They
  need explicit tuner support (linearised traced inputs, or an Ethereal-style
  finite-difference path) before Phase 4.3 — see Step 4.0.
- The dataset/tuner has no **feature-support diagnostics**, **bucketed holdout**,
  **phase-balanced sampling**, **blended (result+teacher) labels**, **binary
  feature cache**, or **regularization/shape constraints** yet — all scheduled in
  Step 4.0 so rare HCE terms do not overfit or learn from sparse data.
- Several second-wave search constants remain inline; they are scheduled for the
  Phase 5 wave2 SPSA, run last at the final eval scale (see §0.5 / Step 5.9).

### Main Diagnosis

Backed by measurement and a 2026-06-18 code audit: Basilisk has plenty of
features, but **the eval is structurally thin in the terms that dominate a
strong HCE, and it has only been partly fitted to data.** The search is modern;
it is not where 200+ Elo hides. The gap is eval *resolution* — see §0.5.

---

## 0.5 Sequencing principle & execution order (READ THIS FIRST — revised 2026-06-18)

The original plan ordered the forward work **Phase 2 (tune eval) → Phase 3
(search) → Phase 4 (add eval features) → retune**. That order **tunes the eval
twice**: it fits the *toy* king-safety and *pawn-only* threats in Phase 2, then
rewrites those exact terms in Phase 4 and has to refit them — and it runs the
search-constant SPSA (old Step 3.9) *before* the Phase-4 eval refit rescales
every centipawn-denominated margin. That is wasted self-play compute. The order
below removes the waste.

### The principle

Two kinds of tuning, very different costs:
- **Texel weight-fitting** (`basilisk-texel`): a gradient fit over a fixed
  labelled dataset. **Minutes of CPU, zero games.** Re-running it after every
  eval change is cheap and expected — it is the per-feature inner loop, **not** a
  conserved resource.
- **SPSA** (search-constant tuning) and the **SPRT** game gates: **thousands of
  self-play games each.** These are conserved.

Search margins (RFP, razoring, futility, qsearch margins, parts of LMR) are
denominated in **eval centipawns**. Rewriting/expanding the eval changes what a
centipawn means, so an SPSA wave run before the eval is final is thrown away.
Therefore:

1. **Build all eval structure before the heavy eval tuning and before the search
   SPSA.** Every structural rewrite (attack maps, threats package, king-safety
   v2, per-count mobility, pawn refinement, endgame scaling) goes in as a
   **behaviour-identical refactor**: new structure + trace points, but new
   sub-terms **seeded inert** (zero-effect, or linear-equivalent to the term they
   replace), so `bench 13` is unchanged and the gate is the fingerprint +
   `--verify` reconstruction + CTest — **no games**.
2. **Fit the whole enlarged eval once** (one staged Texel campaign, biggest lever
   first, PSTs + material last), not as two campaigns.
3. **Run the one search-constant SPSA wave last**, at the final eval scale.

Texel is exempt from "conserve" because repeating it is cheap; the per-feature
SPRT gate is unavoidable (it banks Elo) but not wasted. The only avoidable
waste — premature SPSA and a double Texel campaign — is removed by this order.

### Execution order (new) and where the old sections map

| Execute | Phase (new) | Role | Built from old section |
|---|---|---|---|
| done | Phase 0, 1 | harness, search constants | §2, §3 (unchanged) |
| done / in progress | **Phase 2** | Texel **infra + cheap, structure-independent scalar fits** | §4 Steps 2.0–2.4b (material/mobility/passers/pawnstruct/hanging **accepted**) |
| **next** | **Phase 2.9 — Robustness quick win** (bench-identical) | the time-safety floor (65 forfeits) — pulled before the eval work | §4.9 (new) |
| then | **Phase 3 — Eval structure build-out** (bench-identical) | attack maps, threats pkg, KS v2, per-count mobility, pawn refine, endgame scaling | **old §6 Phase 4** (moved *earlier*) |
| then | **Phase 4 — Eval data-fit completion** (one campaign) | threats group, KS-v2 block, mobility tables, minors, **PST + material refit**, polish | old §4 Steps 2.4c–2.5 (the **deferred** tuning) |
| then | **Phase 5 — Search efficiency wave** (SPSA last) | TT-bound eval, history split, fractional LMR, deeper re-search, qsearch checks, **wave2 SPSA** | old §5 Phase 3 (moved *later*) |
| then | **Phase 6 — Time management** | increment-aware budget | old §7 Phase 5 |
| then | **Phase 7 — Feature menu** | the plateau menu | old §8 Phase 6 |
| last | **Phase 8 — NNUE** (terminal option) | the ceiling-raiser | old §9 Phase 7 |

> **Where Basilisk is right now (2026-06-18):** mid old-Step-2.4b. Accepted:
> material (+29), mobility (+8.8), passers (+16.6), pawn-structure (+30.7),
> hanging (baked). Rejected: rooks. **Stop the 2.4 tuning here.** Do **not** now
> tune old Step 2.4c (king safety — it's the toy model that Phase 3 rewrites) or
> the `threats`/`minors` pawn-threat scalars (Phase 3 replaces the threats term).
> Optional: the `misc` subgroup (prox/space/tempo) is structure-independent and
> may be finished cheaply. Everything else moves into the **Phase 4** campaign,
> run after the **Phase 3** structure exists. (One small sunk cost: mobility was
> fitted *linear*; those values become the per-count-table seeds in Phase 3 —
> seeded, not lost.)

### Honest expectation (estimates; SPRT is the only verdict; gains overlap)

| Phase | Work | Expected Elo |
|---|---|---|
| 2 (remaining) | finish cheap scalars (optional) | small |
| **3** | eval structure build-out (bench-identical) | 0 direct (enabler) + small NPS |
| **4** | eval data-fit completion (the multiplier) | **+80–160** |
| **5** | search-efficiency wave (SPSA once) | **+20–50** |
| 6 | time management | +5–20 (clock TCs) |

### Model recommendations (per phase; exact model + thinking mode)

The loop *driving* (build, SPRT, read verdict, bake, document) is **Sonnet 4.6
medium**. The *authoring* of dense C++ eval rewrites earns a larger model:
- **Opus 4.8 high** — king-safety v2 (3.2), threats package (3.1), endgame
  scaling framework incl. KPK/KBNK (3.5), per-count mobility (3.3, large tables),
  and any tuner-core change (nonlinear king-safety gradients / finite-difference
  path, binary trace-cache format — Step 4.0).
- **Opus 4.8 medium** / **GPT-5.5 high** — attack-map substrate (3.0),
  pawn-structure refinement (3.4).
- **Sonnet 4.6 medium** — the Phase 4 staged-campaign driving, the Phase 6 TM
  formula, release execution/checklists, and all loop-driving.
- **Codex 5.5 medium** / **GPT-5.5 high** — the dense Phase 5 search ports
  (fractional LMR, deeper/shallower re-search, wave2 SPSA wiring).

NNUE stays the terminal option (Phase 8).

---

## 1. Non-Negotiable Gates

Apply these to every phase.

1. **One candidate at a time.** Do not bundle unrelated feature work with tuned
   values.
2. **SPSA/Texel propose; SPRT decides.** A tuned value set is only a candidate.
3. **SPRT every kept strength change.** Default strength gate:
   `elo0=0 elo1=5 alpha=0.05 beta=0.05`. Use `elo1=3` for small follow-up
   refinements and single search features.
4. **Use representative clock conditions for the final gate.** Default SPRT is
   `tc=3+0.03`, Hash 64 MB, Threads 1, repo-local `SuperGM_4mvs.pgn`,
   concurrency no higher than physical cores minus one. This is also the SPSA
   TC (`tc=3`, weather-factory's 1% increment convention), so tuned optima do
   not have to transfer from a different test condition. Use `tc=10+0.1` for
   phase-boundary / TC-suspect LTC confirmation; use fixed `st=0.1` only as an
   optional old-harness sanity check.
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

### 2026-06-17 Harness-Corrected Retry Audit

Rarog's harness change reopened only failures with a plausible fixed-movetime
artifact. Basilisk now uses the same unified SPSA/SPRT clock TC, so place the
worthy retries as follows:

| Candidate | Placement | Decision |
|---|---|---|
| Narrowed Phase 1 combined polish | Step 5.9 | Worth one retry only as part of the post-eval second-wave constants SPSA. Do not rerun it before the eval phases finish: many included margins are centipawn-scale-coupled and the Phase-4 eval refit will invalidate them. |
| Post-LMR deeper re-search | Step 5.5 | Keep as planned. Rarog's equivalent loss was a likely old fixed-movetime artifact; gate here at `tc=3+0.03` and require LTC `tc=10+0.1` if the STC SPRT passes. |
| LMR formula/coefficients | No Basilisk retry | Basilisk's LMR tune already passed SPRT (`+15.63 +/- 8.02`). Rarog needs the retry; Basilisk only carries the lesson that LMR-like changes must use the clock harness. |

Do **not** reopen ProbCut, the singular double-extension cap, or no-aging
history solely because of the harness change. ProbCut lost too much, the cap is
not TC-sensitive, and history persistence needs the Step 5.2 bonus/malus formula
change before it is a meaningful candidate.

### Exposed Phase 1 Parameters

Already implemented as UCI spin options under `BASILISK_TUNE` (see
`src/SearchParams.h` for current accepted defaults): `RfpCoeff`,
`RfpImproving`, `RazorCoeff`, `NullBase`, `NullEvalDiv`, `ProbCutMargin`,
`FutilityBase`, `FutilityCoeff`, `HistPruneCoeff`, `SeePruneCoeff`,
`SingularBetaMult`, `SingularDoubleMargin`, `AspirationDelta`, `LmrBase`,
`LmrDivisor`, `LmrHistDiv`, `LmrNonPvAdj`, `LmrCutNodeAdj`, `LmrTtPvAdj`,
`LmrNotImprovingAdj`.

---

## 4. Phase 2 - Eval Texel Infra + Cheap Scalar Fits (infra done; heavy tuning deferred to Phase 4)

> **Execution note (2026-06-18):** Steps 2.0–2.3 (infra) and the
> structure-independent scalar fits in 2.4 (material, mobility, passers,
> pawn-structure, hanging) are **done/accepted**. **Stop here.** The remaining
> 2.4 stages — king-safety (2.4c, the *toy* model Phase 3 rewrites), PSTs (2.4d),
> global polish (2.4e), and the still-untuned `threats`/`minors` pawn-threat
> scalars — are **deferred into Phase 4**, the single data-fit campaign run
> *after* the Phase 3 structure build-out. Do **not** run them here. See §0.5.

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
   - Traces every position once, then holds only the default score and active
     coefficients needed by the selected group. This keeps material-only
     tuning small enough for the Beast/Stockfish dataset.
   - Fits sigmoid scale `K` first: minimize MSE of
     `result - sigmoid(K * default_eval)` by simple grid + refinement.
   - Optimizes parameters with Adam (group-specific lr; material currently
     uses `--lr 0.3`, full-batch gradients are fine; epochs until holdout loss
     stops improving). Predicted eval near the default is the exact default
     score plus the draw/halfmove-scaled active trace delta.
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

4. External evaluated-position source: `A:\Chess\Beast\data\evaluated\`
   contains Stockfish-evaluated unique positions as `FEN<TAB>target`. The raw
   target is Stockfish side-to-move WDL/expected-score, so
   `tools\texel\import_beast.py` converts black-to-move rows to white
   perspective before writing repo-local `tools\texel\data\beast_sf_train.csv`
   and `beast_sf_holdout.csv` in the `FEN;target` format accepted by
   `basilisk-texel`. Keep this as a distinct train/holdout source from
   self-play so SPRT can tell whether SF-supervised tuning transfers to
   Basilisk self-play.

### Step 2.4 - Staged tuning, each stage SPRT-gated

Naming: build each accepted stage as `phase2-<stage>` via
`tools/build_test.ps1 -Suffix phase2-<stage>`; SPRT vs the previous accepted
head. Bake candidate values into `EvalParams.h` on the candidate branch to
build the test binary; keep that bake only after SPRT acceptance. Record
`bench 13` and run CTest before SPRT.

| Stage | Tuned group | SPRT gate | Notes |
|---|---|---|---|
| 2.4a | Material values only (10 params) | elo1=5 | Cheap pipeline proof. If this fails, debug K-fit, dataset, reconstruction - do not proceed. |
| 2.4b | All scalar terms except king safety and PSTs (~85 params) | elo1=5 | Mobility, pawn structure, passers, piece terms, threats, hanging, space, tempo. |
| 2.4c | King safety block: `safety_table[25]`, `ks_unit`, shelter, storm (~45 params) | elo1=3 | Keep zone-composition logic frozen. |
| 2.4d | PSTs + material refit (778 params) | elo1=5 | Use L2 toward PeSTO; needs the full dataset. |
| 2.4e | Global polish: everything unfrozen, low lr | elo1=3 | Stop here regardless of outcome. |

Current command shape for 2.4a:

```powershell
.\build\release-pext\basilisk-texel.exe --tune material `
  tools\texel\data\beast_sf_train.csv `
  tools\texel\data\beast_sf_holdout.csv `
  tools\texel\out\phase2-material.txt `
  --epochs 200 --lr 0.3
```

When using the full Beast import rather than the 1.6M sampled import, pass
`--max-positions 0`; the tuner default intentionally caps each split at 5M
positions for memory safety:

```powershell
.\build\release-pext\basilisk-texel.exe --tune material `
  tools\texel\data\beast_sf_all_train.csv `
  tools\texel\data\beast_sf_all_holdout.csv `
  tools\texel\out\phase2-material-all.txt `
  --epochs 200 --lr 0.3 --max-positions 0
```

Full Beast material candidate tried and rejected by SPRT:

| Param | Old | Candidate |
|---|---:|---:|
| `mg_val` pawn, knight, bishop, rook, queen | 82, 337, 365, 477, 1025 | 78, 290, 359, 420, 1065 |
| `eg_val` pawn, knight, bishop, rook, queen | 94, 281, 297, 512, 936 | 131, 276, 297, 565, 990 |

SPRT result (2026-06-17, `tc=3+0.03`, `elo0=0`, `elo1=5`):
H0 accepted after 2,982 games, Elo `-16.56 +/- 9.56`, nElo
`-21.64 +/- 12.47`. The bake was reverted. Treat Stockfish WDL labels as a
proposal source only; do not use them directly for accepted material values.
The Beast FEN corpus remains useful as a diverse position source for Basilisk
self-play starts / quiet-position sampling, with Basilisk-generated labels.

Next material attempt: sample the FEN-only Beast corpus into a fastchess EPD
book, generate Basilisk-vs-Basilisk games from those starts, then extract
`FEN;game_result` labels with `--skip-start 0`:

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

Basilisk-labeled Beast-seed material candidate accepted by SPRT:

| Param | Old | Candidate |
|---|---:|---:|
| `mg_val` pawn, knight, bishop, rook, queen | 82, 337, 365, 477, 1025 | 85, 323, 363, 514, 1085 |
| `eg_val` pawn, knight, bishop, rook, queen | 94, 281, 297, 512, 936 | 96, 310, 319, 557, 996 |

Tune source: 200,000 Basilisk self-play games from `beast_seed.epd`, yielding
1,730,726 train and 92,433 holdout positions. Holdout loss improved
`0.101272 -> 0.100645` over 200 material-only epochs.

SPRT result (2026-06-17, `tc=3+0.03`, `elo0=0`, `elo1=5`):
H1 accepted after 1,930 games, Elo `+29.05 +/- 11.36`, nElo
`+39.83 +/- 15.50`. Keep the material bake and use it as the baseline for
Step 2.4b.

First broad 2.4b scalar tune on the same Basilisk-labeled dataset improved
holdout loss (`0.100610 -> 0.098716`) but was **not baked**: candidate values
had clear sign/shape problems (negative supported/candidate passer terms,
non-monotonic passed-pawn tables, negative queen-threat EG, negative space).
Proceed with narrower scalar subgroups and/or simple constraints rather than
SPRTing the broad scalar pass.

The tuner now supports scalar subgroups with basic sign/shape clamps:
`pawnstruct`, `passers`, `rooks`, `minors`, `mobility`, `threats`,
`hanging`, and `misc`. It restores the best holdout epoch before writing the
candidate file. First checked groups:

| Group | Holdout | Notes |
|---|---:|---|
| `mobility` | `0.100610 -> 0.100266` | Plausible; selected as first 2.4b SPRT candidate. |
| `pawnstruct` | `0.100610 -> 0.100314` | Plausible, but weaker and several terms clamp to zero. |
| `minors` | `0.100610 -> 0.100565` | Plausible but weak. |
| `hanging` | `0.100610 -> 0.100504` | Plausible but weaker than mobility. |
| `threats` | `0.100610 -> 0.100591` | Weak; MG threats collapse together under monotonic clamp. |

Mobility candidate accepted by SPRT vs `phase2-material`:

| Param | Old | Candidate |
|---|---:|---:|
| `mob_mg` knight, bishop, rook, queen | 4, 5, 2, 1 | 5, 5, 1, 2 |
| `mob_eg` knight, bishop, rook, queen | 4, 5, 4, 2 | 5, 7, 7, 12 |

SPRT result (2026-06-17, `tc=3+0.03`, `elo0=0`, `elo1=5`):
H1 accepted after 7,288 games, Elo `+8.77 +/- 5.68`, nElo
`+12.32 +/- 7.98`. Keep the mobility bake and use `phase2-mobility` as the
baseline for the next scalar subgroup.

Post-mobility scalar subgroup checks on the same Basilisk-labeled dataset:

| Group | Holdout | Notes |
|---|---:|---|
| `passers` | `0.100256 -> 0.099610` | Best loss; monotonic/nonnegative after clamps, but support/candidate/free terms mostly collapse to zero. Worth one SPRT. |
| `pawnstruct` | `0.100256 -> 0.099969` | Plausible and safer fallback if passers fails. |
| `misc` | `0.100256 -> 0.100021` | Mostly `prox_base`/tempo; lower priority because passers already includes `prox_base`. |
| `rooks` | `0.100256 -> 0.100052` | Some suspicious zeroed rook-file/7th-rank terms. |
| `hanging` | `0.100256 -> 0.100169` | Plausible but modest. |
| `minors` | `0.100256 -> 0.100203` | Plausible but weak. |
| `threats` | `0.100256 -> 0.100235` | Weak; MG/EG threat values collapse together. |

Passers candidate accepted by SPRT vs `phase2-mobility`:

| Param | Old | Candidate |
|---|---:|---:|
| `passed_mg[1..6]` | 5, 10, 20, 35, 60, 100 | 8, 8, 8, 25, 90, 90 |
| `passed_eg[1..6]` | 10, 17, 35, 62, 100, 170 | 10, 10, 53, 84, 106, 124 |
| `pass_supp_mg`, `pass_supp_eg_base`, `pass_supp_eg_rank` | 8, 6, 4 | 0, 0, 0 |
| `cand_mg`, `cand_eg` | 6, 10 | 0, 0 |
| `pass_free_mg`, `pass_free_eg`, `pass_safe_eg` | 2, 6, 8 | 2, 0, 16 |
| `prox_base` | 2 | 11 |

Built/tested: `bench 13 = 4,280,823`, 8/8 CTest.

SPRT result (2026-06-17, `tc=3+0.03`, `elo0=0`, `elo1=5`):
H1 accepted after 3,462 games, Elo `+16.57 +/- 8.28`, nElo
`+23.20 +/- 11.57`. Keep the passers bake and use `phase2-passers` as the
baseline for the next scalar subgroup.

Post-passers scalar subgroup checks on the same Basilisk-labeled dataset:

| Group | Holdout | Notes |
|---|---:|---|
| `pawnstruct` | `0.099561 -> 0.099316` | Best remaining loss; plausible values, selected for SPRT. |
| `rooks` | `0.099561 -> 0.099372` | Weaker and still zeroes open-file EG / 7th-rank MG terms. |
| `hanging` | `0.099561 -> 0.099472` | Plausible but modest. |
| `misc` | `0.099561 -> 0.099485` | Small gain, mainly tempo/space. |
| `minors` | `0.099561 -> 0.099517` | Plausible but weak. |
| `threats` | `0.099561 -> 0.099540` | Weak; threats collapse together. |

Pawn-structure candidate accepted by SPRT vs `phase2-passers`:

| Param | Old | Candidate |
|---|---:|---:|
| `doubled_mg`, `doubled_eg` | -10, -20 | -2, -8 |
| `isolated_mg`, `isolated_eg` | -15, -20 | -9, -17 |
| `connected_mg`, `connected_eg` | 7, 5 | 18, 1 |
| `backward_mg`, `backward_eg` | -10, -15 | 0, -12 |

Built/tested: `bench 13 = 3,877,336`, 8/8 CTest.

SPRT result (2026-06-17, `tc=3+0.03`, `elo0=0`, `elo1=5`):
H1 accepted after 1,836 games, Elo `+30.74 +/- 11.76`, nElo
`+41.76 +/- 15.89`. Keep the pawn-structure bake and use
`phase2-pawnstruct` as the baseline for the next scalar subgroup.

Post-pawnstruct scalar subgroup checks on the same Basilisk-labeled dataset:

| Group | Holdout | Notes |
|---|---:|---|
| `rooks` | `0.099315 -> 0.099120` | Best remaining loss; selected for one SPRT despite zeroed open-file EG / 7th-rank MG terms. |
| `hanging` | `0.099315 -> 0.099224` | Plausible but smaller. |
| `misc` | `0.099315 -> 0.099242` | Small gain, mainly tempo/space. |
| `minors` | `0.099315 -> 0.099276` | Plausible but weak. |
| `threats` | `0.099315 -> 0.099293` | Weak; threats collapse together. |

Rooks candidate tried vs `phase2-pawnstruct` and manually rejected:

| Param | Old | Candidate |
|---|---:|---:|
| `rook_open_mg`, `rook_open_eg` | 25, 10 | 41, 0 |
| `rook_semi_mg`, `rook_semi_eg` | 12, 8 | 8, 32 |
| `rook_7th_mg`, `rook_7th_eg` | 20, 40 | 0, 14 |
| `rook_behind_passer_mg`, `rook_behind_passer_eg` | 15, 25 | 13, 26 |
| `enemy_rook_passer_mg`, `enemy_rook_passer_eg` | 10, 20 | 25, 11 |

Built/tested: `bench 13 = 4,566,221`, 8/8 CTest.

SPRT stopped manually after 10,666 games: Elo `+3.13 +/- 4.74`, nElo
`+4.35 +/- 6.59`, LOS `90.21%`, LLR `0.82` inside `(-2.94, 2.94)`.
Result is inconclusive and below the `elo1=5` target; reject due marginal
effect plus awkward value shape (`rook_open_eg=0`, `rook_semi_eg=32`,
`rook_7th_mg=0`). Reverted rook constants to the accepted
`phase2-pawnstruct` baseline before trying the next subgroup.

Hanging candidate selected for SPRT vs `phase2-pawnstruct`:

| Param | Old | Candidate |
|---|---:|---:|
| `hang_pen` knight, bishop, rook, queen | 45, 45, 60, 80 | 22, 39, 33, 36 |

Built/tested: `bench 13 = 4,033,379`, 8/8 CTest. SPRT pending.

Decision rules per stage: tuned values that are wildly implausible (signs
flipped on well-understood terms, passer table non-monotonic by hundreds)
mean dataset or trace bugs - fix before SPRT. A failed SPRT after a sane fit
means revert that stage and continue to the next; one retry only if a concrete
defect was found and fixed.

**Decision lessons carried forward (into Phases 3–4):**
- Treat Stockfish/teacher labels as a *proposal* source only; accepted values
  still require a Basilisk self-play SPRT (the SF-WDL material fit lost −16.56).
- Broad scalar fits that produce implausible signs/shapes are data/trace/
  constraint bugs, not candidates — the broad 2.4b pass collapsed several terms
  to zero and inverted others. This is why Phase 4 adds **feature-support
  diagnostics, bucketed holdout, regularization, and shape constraints** before
  it widens the parameter set (Step 4.0).
- King safety was **not** tuned in Phase 2: the current model is too thin and
  partly **nonlinear** for the existing linear-trace fitter. It is rebuilt as
  structure in Phase 3.2 and tuned in Phase 4.3, behind the Step 4.0 nonlinear-
  safety tuner support.

### Step 2.5 - Phase boundary validation

> **Superseded (2026-06-20).** The Phase 2 scalar gains are already
> strength-valid; the release that ships them is now **Step 2.9.3 (release
> `1.6.0`)**, cut after the 2.9.1 forfeit gate passes. The broad **2000-game
> validation vs `phase1-final` moves to the Phase 4 boundary**, after the
> complete eval campaign (where eval over-fit risk is highest). The items below
> are kept as the validation recipe to reuse there.

- `tools/gauntlet.ps1` 2000 games vs `phase1-final` (clock TC `tc=10+0.1`).
- Optional external check: drop the new binary into the LittleBlitzer pool
  (`D:\chess\little blitzer`) against the Stockfish UCI_Elo anchors.
- Update `CHANGELOG.md`, tag the release per Section 10.

### Why PSTs Are Last

Unchanged rationale: largest parameter block, easiest to overfit, and the
scalar stages prove the pipeline transfers to Elo before spending the PST
budget. The gradient tuner makes PSTs *feasible*; the staging makes them
*safe*.

---

## 4.9 Phase 2.9 - Robustness quick win (THE NEXT PHASE — do before Phase 3)

**Why (added 2026-06-19).** The 35k-game gauntlet exposed one cheap,
eval-independent problem worth fixing *before* the eval campaigns, because it is
quick, low-risk, and it makes every later SPRT/gauntlet trustworthy:

- **Time forfeits.** Basilisk 1.5.1 (dev) lost **65 games on time** (`t=65`) at
  `tc=3+0.03` vs 1.5.0's 18 and 1.4.9's 13 — the dev work *tripled* forfeits.
  Pure lost Elo, one-sided vs non-forfeiting SF, so it **biases every external
  gauntlet.**

**Step 2.9.1 — Time-safety floor (highest priority) — Sonnet 4.6 medium.
DONE (2026-06-19), then SUPERSEDED (2026-06-20) by Phase 6 Step 6.1/6.1b.**
The standalone reserve patch (a `2x move_overhead` clamp plus a fix for an
unconditional `std::max(10, …)` floor bug that forced ≥10 ms of planned thinking
even when `remaining` was 0) sat on the old tiered-percentage formula, which a
LittleBlitzer probe showed was tighter to the margin than Rarog's SF-style one.
Rather than re-tune it, **Step 6.1 ports Rarog's formula wholesale** and Step
6.1b folds the reserve in with Rarog's exact 2x mechanics. **See Phase 6 (§7)
for the current, validated implementation;** this section is kept for the
regression history only.

> **Transferable findings from Rarog's completed 2.9.1 (apply when implementing
> this).** Rarog hit the same forfeit regression and fixed it; three engine-
> agnostic lessons:
> 1. **The forfeits are a *clock-path* problem, not a movetime one.** Rarog's
>    forfeits were all at `tc=3+0.03` (clock); fixed movetime never forfeited.
>    Put the remaining-time floor in the clock branch — an absolute reserve of
>    `~2 × move_overhead` bound only the genuine low-time scrambles and left
>    normal allocation (and the Elo from the SF-style TM) untouched.
> 2. **Don't subtract overhead in the movetime path** ("keep as is" above may be
>    over-conservative — check it). Rarog originally reserved a full
>    `move_overhead` from `go movetime T`, costing ~10 % of thinking depth:
>    measured `tpm=92.9` on a "100 ms/move" gauntlet vs Stockfish's `110.2`, both
>    `t=0`. The GUI tolerates ~10 % over nominal, so use the **full** movetime as
>    the hard limit (the SF/Reckless default). If Basilisk's movetime path
>    subtracts overhead, it is leaving depth on the table for no safety gain.
> 3. **Validate the forfeit fix in a second harness.** Little Blitzer mismeasures
>    time for some engines (Critter forfeits 100 % in LB, 0 % in Colosseum at the
>    same TC), so an LB `t=` count is not trustworthy alone — confirm with
>    `sprt.ps1`/Colosseum. (LB's *comparative* forfeits across engines in one run
>    are still meaningful.) See also Phase 6, whose movetime path should inherit
>    finding 2.

**Step 2.9.2 — Fix `tools/gauntlet.ps1` to support clock TC — Sonnet 4.6 medium.
DONE (2026-06-19).** The script hardcoded `-each st=0.1` (fixed 100 ms/move) and
printed `TC: st=0.1`, but §10 instructs running the phase-boundary gauntlet at a
**clock** TC (`tc=10+0.1`) — a plan/tooling inconsistency that silently ran every
gauntlet at the wrong (movetime) condition, which doesn't exercise the time
manager the way clock play does. Added a `-TC`/`-MoveTime` parameter pair
mirroring `sprt.ps1` (clock `tc=…`, default `"10+0.1"`, unless `-MoveTime` is
given) plus `-TimeMargin` (default 20 ms, also mirroring `sprt.ps1`) and updated
the banner. Pure tooling, no engine change, no bench impact. This unblocks the
Step 2.9.1 forfeit re-check, which is meaningless at fixed movetime since
movetime doesn't forfeit the way clock play does.

**Step 2.9.3 — Release `1.6.0` — Sonnet 4.6 medium.** Once the time-forfeit fix
(now the Phase 6 clock-TM port + Step 6.1b reserve) clears its forfeit gate
(clock-TC gauntlet shows `t=`≈0 and the +54 holds vs `phase1-final`
@ `tc=10+0.1`), prepare the next release: the unreleased Phase 2 scalar gains
are already strength-valid and the time-safety fix makes them shippable. Run
the Section 10 release checklist exactly — confirm the gate, bump the version
in **both** `src/Constants.h` and `CMakeLists.txt`, update `CHANGELOG.md`,
verify a clean non-tune release build (tuning options hidden, `bench 13`,
8/8 CTest), build distribution binaries, and commit. **Do not tag, do not
push** — the user creates the `v1.6.0` tag and pushes it themselves when ready;
hand back copy-paste GitHub release notes for when they do. **Version `1.6.0`,
not `1.5.1`** (§10): a patch number undersells the biggest eval jump so far. Do
**not** start Phase 3 until this release is done or explicitly deferred by the
user.

**Speed:** Basilisk is **not** NPS-bound (it already out-searches Rarog —
2.76 M nps / d13.8). Architecture use is already good (`-march=native` znver3 +
`USE_PEXT` + LTO + PGO). No speed work here; if ever wanted, profile first, after
the eval phases. *(The Rarog-specific micro-opts — `BadCapture` struct bloat,
`gives_check` clone, Rust bounds checks — are Rust-only and do not apply to the
C++ engine.)*

Expected: forfeits → 0 (reliability + a few Elo), near-zero risk. Then proceed to
**Phase 3**.

---

## 5. Phase 5 - Search Efficiency Wave (close the EBF gap) — EXECUTE AFTER Phase 4

> **Order (§0.5):** this runs **after** the eval is final (Phase 4). The Step 5.9
> (old 3.9) **second-wave constants SPSA is the conserved compute** — its margins
> are eval-centipawn-denominated, so it must be spent once, here, at the final
> eval scale. Steps are still individually SPRT-gated. **Models:** loop-driving
> Sonnet 4.6 medium; dense ports (fractional LMR, deeper/shallower re-search,
> wave2 SPSA wiring) Codex 5.5 medium / GPT-5.5 high. Expected: **+20–50 Elo**.

Goal: reduce nodes-per-depth toward Stockfish's regime (measured gap: ~2.2 vs
~1.8 effective branching factor). This phase absorbs the old "Phase 1.5
second-wave constants" and adds specific search features. Realistic target:
+20 to +50 Elo self-play (the §0.5 expectation table figure).

Rules: one item at a time, in the order below; each item is implemented,
CTest-ed, `bench 13`-recorded, then SPRT-gated (`elo1=3` unless noted).
Constants introduced by an item are exposed under `BASILISK_TUNE` immediately
but SPSA-tuned only in Step 5.9. Skip to the next item on a failed SPRT; do not
stack untested changes.

### Step 5.1 - TT-bound eval refinement (~10 lines, do first) — Sonnet 4.6 medium

In `negamax` after computing `static_eval` (non-check, non-excluded path):
if a TT entry was found with a usable score
(`tt_flag == TT_EXACT`, or `TT_BETA && tt_score > static_eval`, or
`TT_ALPHA && tt_score < static_eval`), use `tt_score` in place of
`static_eval` for the **pruning decisions only** (RFP, razoring, NMP entry,
futility) - keep `ss->eval` as the raw corrected value used for `improving`
and correction-history updates. Mirror in qsearch stand-pat: if TT bound
applies, tighten `stand_pat` the same way.

### Step 5.2 - History formula upgrade — Sonnet 4.6 medium

Replace `bonus = min(depth*depth, 2048)` in `update_all_histories` with
separately scaled linear forms:

```
bonus = std::min(p.hist_bonus_mul * depth - p.hist_bonus_sub, p.hist_bonus_max);
malus = -std::min(p.hist_malus_mul * depth - p.hist_malus_sub, p.hist_malus_max);
```

Initial values: `bonus_mul=170, bonus_sub=90, bonus_max=1700;
malus_mul=180, malus_sub=100, malus_max=1500`. Expose all six. SPRT.

### Step 5.3 - Fractional LMR — Codex 5.5 medium / GPT-5.5 high

`lmr_table_` becomes `int` in 1024ths: `1024 * (base + log(d)*log(m)/div)`.
All adjustments scale accordingly (`lmr_non_pv_adj` etc. become ~1024-unit
values; `move_stat_score / p.lmr_hist_div` is already integer - multiply
numerator by 1024 before dividing). Final
`reduction = total_r >> 10` clamped as today. This is behavior-changing
(rounding), so SPRT it on its own with the old adjustment defaults converted
(`1 -> 1024`).

### Step 5.4 - LMR input: TT-move-is-capture — Sonnet 4.6 medium

Track `tt_capture = tt_move != MOVE_NONE && tt_move is capture` at node entry;
add `+p.lmr_tt_capture (default 1024)` to quiet-move reductions when true.
SPRT (cheap; bundle-eligible with Step 5.5 only if both individually fail near 0).

### Step 5.5 - Post-LMR deeper/shallower re-search — Codex 5.5 medium / GPT-5.5 high

Where the code currently re-searches at full depth after a reduced search
returns `score > alpha`: first compute
`do_deeper = score > best_score + p.deeper_margin (default 64) + 2*reduction`
and `do_shallower = score < best_score + p.shallower_margin (default 8)`;
re-search at `new_depth + (do_deeper ? 1 : do_shallower ? -1 : 0)`. SPRT.
Before implementing the shallower arm, verify it is reachable in Basilisk's
loop invariants; if it is dead, implement and tune the deeper arm alone. This
is one of the harness-corrected retries from the Rarog audit: use the default
clock SPRT (`tc=3+0.03`, `elo1=3`) and, if it passes, confirm at
`-TC "10+0.1"` before keeping it.

### Step 5.6 - Qsearch quiet checks — Codex 5.5 medium

At `qply == 0` and not in check, after the capture loop fails to raise alpha:
generate quiet moves that give check, filtered to `see_ge(move, 0)`. **Start
with the existing `Board::gen_quiet_checks()` helper** (already in the codebase,
`Board.h:103`) for correctness, then profile and only replace it with a
`Board::check_squares(pt, side)` pre-mask if generation shows up hot. Search
them like captures. Cap at the first 4-6 such moves. SPRT (`elo1=3`); if it
fails, retry once gating by `depth >= -1`-style recent-entry only.

### Step 5.7 - Double-extension cap — Sonnet 4.6 medium

Add `int double_exts` to `SearchStack`, incremented when the singular path
extends by 2, inherited by children (`(ss+1)->double_exts = ss->double_exts`);
disallow the 2-ply extension when `double_exts >= p.double_ext_max
(default 8)`. Likely Elo-neutral but bounds tactical blowups; SPRT with
`elo0=-3 elo1=1` (non-regression gate).

### Step 5.8 - Razoring restriction experiment — Sonnet 4.6 medium

Try `depth <= 1` (from `depth <= 3`) - RFP largely covers razoring's range
and the qsearch verification call is not free. SPRT; keep whichever passes.

### Step 5.9 - Second-wave constants SPSA (the conserved compute) — Sonnet 4.6 medium

Only after Steps 5.1-5.8 verdicts are in. Expose in `SearchParams` (one coherent
group, ~12-16 knobs): LMP formula coefficients (the `3 + d*d` / `2 + d*d/2`
pair), NMP verification depth gate (10) and `depth/4` divisor, ProbCut depth
gate (5) and reduced depth (-4), qsearch margins (150 futility, 200/-800/200
SEE-threshold clamp, -50 late-SEE, the `i >= 6` gate), history-pruning depth
gate, IIR depth gate and TT-depth offset, correction-history `/5` weight and
update clamp, move-ordering check bonus (32000), plus the Phase 5 constants
introduced above. Default-equivalence `bench 13` first, then
`setup_spsa.ps1 -ConfigGroup wave2` (add the group to the script following
the existing pruning/lmr pattern), 5000 iterations, SPRT the result.
This is also the proper retry point for the rejected Phase 1 combined polish:
use `config_combined.json` as the reference seed/range source, but retune under
the unified `tc=3+0.03` harness after Phase 2's eval refit. Do not resurrect the
old tuned values directly.

---

## 6. Phase 3 - Eval Structure Build-Out (attack-map era) — EXECUTE FIRST of the forward phases

Goal: put the **entire enlarged eval structure** in place — the terms that
separate basic HCE from strong HCE — **without changing play**, so the Phase 4
campaign can fit it all at once. This is the highest-leverage block; its payoff
is realised in Phase 4 (+80–160 Elo), not here.

**Discipline (this whole phase spends no games):** every step is a
**behaviour-identical refactor**. New structure + Texel trace points go in, but
new sub-terms are seeded **inert** (zero-effect, or linear-equivalent to the
term they replace), so `bench 13` is **unchanged** and the gate is the
fingerprint + `--verify` reconstruction + CTest. **Ignore any "retune … + SPRT"
wording inherited from the old per-feature plan — all tuning is deferred to the
Phase 4 stage named on each step.** The one exception is endgame *behaviour*
(3.5, KBNK etc.), confined to material patterns absent from the bench suite and
gated by the endgame EPD suite, not SPRT.

Seeding rules: a *new* sub-term → weight seeded `0`; a *replaced* term (mobility
tables) → `table[i] = i * old_weight`; a *capped* term being extended → seed new
entries to the old cap value. `--verify` must stay exact after every step.

**2026-06-20 HCE-survey framing.** The term list below was cross-checked against
Stockfish 11/classical, Ethereal's HCE-era eval+tuner, RubiChess's classical
eval, and Lambergar's HCE tuning notes (the §3.8 checklist). The conclusion is
**not** "add a large new feature family" — the plan already has the right strong-
HCE core. The remaining work is making these structures **tunable and
measurable**: nonlinear king-safety tuner support, feature-support diagnostics,
better data balance (Step 4.0), promotion-path passer safety (3.4), exact KPK
(3.5), and a handful of cheap Stockfish/Ethereal positional terms (3.6/3.8). Keep
that framing while building Phase 3 — structure that the Phase-4 fitter cannot
measure is dead weight.

### Step 3.0 - Attack-map infrastructure (behaviour-identical refactor, do first) — Opus 4.8 medium / GPT-5.5 high

In `evaluate()`, compute once per call:

```cpp
Bitboard attacked_by[NCOLORS][PIECE_TYPE_NB]; // per piece type, pawns included
Bitboard attacked[NCOLORS];                   // union
Bitboard attacked2[NCOLORS];                  // attacked by 2+ pieces
```

Rewrite mobility, king safety, and hanging-piece terms to use these instead of
recomputing attacks / calling `is_attacked_by` per square. Identical eval
output required: `bench 13` must not change. Expect a small nps gain.

Also compute `blockers_for_king` / pinned-piece masks for both colours **while
the slider attack information is hot** (one extra pass over the slider rays).
Steps 3.2 and 3.3 consume these: 3.3 excludes pinned/blocking pieces from the
mobility area, and 3.2 scores king-danger pressure from pieces pinned or
blocking in front of the king. Computing them here, once, avoids a second slider
sweep later. Seeded-inert consumers keep `bench 13` unchanged.

### Step 3.1 - Threats package — structure, seeded inert (tuned in Phase 4.2) — Opus 4.8 high

New traced params (initial values from SF-classical, scaled to Basilisk's
pawn=82 base): `threat_by_minor[pt]` and `threat_by_rook[pt]` arrays (mg,eg
each), threat-by-king, hanging refinement (enemy piece attacked, not defended
or attacked twice and defended once), pawn-push threats (squares our pawns
could attack after one safe push), restricted-piece bonus. Seed every new weight
inert here; the current flat `hang[]` term stays active until Phase 4.2 accepts
the tuned replacement, then drop it in that same tuned step. Tune the threat
group in Phase 4.2.

*(Optional, higher-complexity)* **Overloaded defender** — an enemy piece that is
the *sole* defender of a more valuable attacked target (or is pinned and cannot
leave): a static-tactical term most strong HCEs carry but SF-classical expresses
only indirectly. The attacker/defender counts from the 3.0 maps make
"defended-exactly-once and that defender is itself needed elsewhere" cheap to
test. Seed 0, tuned in 4.2. Add only if it falls out of the threat loops already
built here; defer if it complicates the pass.

### Step 3.2 - King safety v2: full danger model — structure, seeded inert (tuned in Phase 4.3) — Opus 4.8 high

Using the Step 3.0 maps: add per-piece-type **safe-check** units (squares from
`check_squares(pt)` that the enemy attacks and we don't defend, or defend only
with the king/queen), and **weak-ring** units (king-ring squares attacked and
not solidly defended). Then extend into the full strong-HCE king-danger model,
every new input seeded inert and feeding the existing
`attack_units -> safety_table` funnel as additional tunable unit weights:
- **king-flank attack/defense counts** (attacker vs defender presence on the
  king's flank),
- **pawnless-flank penalty** (king on a flank with no friendly pawns),
- **blockers/pinned pieces near the king** (from the Step 3.0 pin masks — a
  pinned defender does not really defend),
- **enemy mobility pressure near the king** (enemy attacked squares in/around the
  ring),
- **lost-castling / central-king danger** (king stuck centrally with castling
  rights gone — folded in here rather than as a separate 3.6 term so it shares
  the danger funnel),
- **no-enemy-queen scaling** of the whole danger sum (kept as the frozen `*2/3`
  seed, exposed as a tunable scalar).

Tune the king-safety block in Phase 4.3, behind the Step 4.0 nonlinear-safety
tuner support (these inputs are composite, not pure-linear).

### Step 3.3 - Per-count mobility tables — structure, seeded linear-equivalent (tuned in Phase 4.4) — Opus 4.8 high

Replace linear `mob * w` with one-hot tables indexed by popcount:
`mob_n[2][9]`, `mob_b[2][14]`, `mob_r[2][15]`, `mob_q[2][28]` (mg/eg).
Initialize from the current linear values (`table[i] = i * w`), refine the
mobility *area* to exclude squares occupied by own king and queen, own blocked
pawns (SF-classical definition), and own pinned/blocking pieces (from the Step
3.0 pin masks — a pinned piece's "mobility" is illusory). Tune the mobility group
in Phase 4.4.

### Step 3.4 - Pawn-structure refinement — structure, seeded inert (tuned in Phase 4.5) — Opus 4.8 medium

Connected/phalanx bonus by rank (SF formula `seed[r]` style, one-hot traced),
weak-unopposed penalty, blocked pawns on 5th/6th, king-pawn-distance endgame
term, **pawn levers** (a pawn move that creates a lever), and a **candidate
passer / majority breakthrough** term — a pawn that becomes passed in 1–2 pawn
moves (own majority with no/fewer enemy pawns ahead), distinct from already-passed
and from the unstoppable rule-of-square logic (3.8). Upgrade passed-pawn safety
from today's single "safe stop square" test to a **promotion-path safety** model
(SF-classical `passed` shape): score the attacked/defended status of the squares
on the pawn's path to promotion, whether the immediate block square is defended,
the friendly/enemy king distance to the block and next squares, and rook/queen
support **behind** the passer (own → pushes it, enemy → restrains it). Keep the
pawn-only parts inside `eval_pawns`/pawn cache where they only depend on pawns
(+ king squares: add a small separately-cached king-pawn table keyed by
`pawn_key ^ king squares` if needed); the path-attack and rook-behind parts need
piece squares, so compute them in the piece-activity pass. Tune the pawn/passer
group in Phase 4.5.

**Rook behind passed pawn (Tarrasch) + ideal blockader** (eg-weighted) — a bonus
when our rook is on a passer's file *behind* it (own passer → supports the push;
enemy passer → restrains it), plus a small bonus for a **knight directly in front
of an enemy passer** (Nimzowitsch ideal blockader). These need piece squares, so
compute them in the piece-activity pass, *not* the pawn-only cache; seed 0, tuned
with passers in 4.5 and folded into 3.5 endgame scaling where relevant.

### Step 3.5 - Scale-factor framework + endgame knowledge (incl. KBNK) — Opus 4.8 high (framework/KBNK) · GPT-5.5 high (per-EG funcs)

Basilisk's endgame handling today is only OCB scaling + a two-knights draw +
the frozen mate-drive mop-up (`eval.cpp:623-679`). Introduce a proper
`ScaleFactor` (0–64, `NORMAL = 64`) applied to the eg side of the tapered score
before the 50-move damping, then add, behind it (firing only on material
patterns absent from the bench suite, so `bench 13` is unchanged):
- generalise the OCB rule into the framework, scaled by passed-pawn count;
- winning side has no pawns and ≤ minor-piece advantage → scale toward draw;
  rook endings with all pawns on one flank → mild scale-down; OCB-with-other-
  pieces → mild scale-down (distinct from pure OCB);
- exact **KPK** handling (a compact bitbase or an opposition / rule-of-the-square
  evaluator) so Syzygy stays optional and KPK is scored correctly without tablebases;
  KQKP / KRKP / KQKR heuristics; KBP wrong-bishop-wrong-corner draw;
- a **correct KX K mop-up incl. KBNK driving the bare king to the bishop-coloured
  corner** — the generic mop-up (`eval.cpp:623-638`) cannot win KBNK reliably.
  Keep the generic mop-up for the general case; add the KBNK-correct corner only
  for the KBNK pattern.

Each known-endgame function is small and self-contained, gated by a **permanent
endgame regression suite** (below), not SPRT — these positions are too rare for
self-play to measure. Frozen constants (not traced) are fine; expose for tuning
only if a function is borderline.

**Permanent endgame regression suite (`tests/endgames.epd` + a CTest target).**
Build once, keep forever — it protects mating technique against any future
eval/search change and is the gate for every 3.5 function. One EPD per line with
the expected verdict (KBNK win both corners, KPK win/draw by the rule, KRKP,
KQKP, KQKR, OCB-with-passers draws, rook-flank draws, KBP wrong-corner draw,
KNNK-vs-bare-king draw — ~30–60 positions). The CTest asserts (a) the static
eval sign/magnitude matches the verdict and (b) for the mating ones a short
fixed-depth search **actually delivers mate from the board** within a move
budget. *If a model cannot curate the FENs, paste textbook positions — the test
harness is the work; the positions are public knowledge.*

### Step 3.6 - Small positional terms — structure, seeded inert (tuned in Phase 4.5) — Sonnet 4.6 medium

Pull the high-value items from the old Phase-6 menu forward as structure (each a
few lines, all seeded `0` so `bench 13` is unchanged; the tuner decides their
worth in Phase 4): **bishop outposts** (mirror the knight-outpost logic,
`eval.cpp:335-353`), **reachable knight outposts** (an outpost square a knight
can reach in one safe hop, not only occupy), **trapped-rook-by-own-uncastled-king**,
**connected rooks**, **bishop on a long diagonal** bearing on the enemy king,
**bad bishop** (own pawns on the bishop's colour), **bishop-pair scaled by pawn
count** (replacing the flat `bp_mg/bp_eg`), and an **initiative/complexity** term
(a score nudge scaled by total pawns / passed pawns / both-flanks presence, like
SF's `Initiative`). Add the cheap **queen-pressure** terms here too — rook on the
queen's file, weak/exposed queen on an enemy-attacked square — and a
slider-on-queen / pinner threat **only if** it falls out of the attack maps
cleanly. Trace each; batch into one Phase-4 tuning stage.

Two non-SF-classical terms (from the Ethereal/RubiChess survey — see the HCE
source checklist after 3.8), both **optional and possibly redundant**, add only
if cheap and prune in Phase 4 if they don't earn their fit:
- **Closedness / locked-centre** (Ethereal `ClosednessKnight/ClosednessRook`) —
  adjust knight value *up* and rook value *down*, and dampen the
  complexity/initiative term, as the centre locks (count of fixed/rammed pawns).
  *Caveat: the per-count mobility tables (3.3) already penalise a closed-position
  knight's low mobility, so the marginal lever is the material-value swing only —
  keep it small.* Seed 0, tuned in 4.5.
- **Central-king / lost-castling danger** — a penalty for a king stuck in the
  centre with castling rights gone, *separate* from the king-ring attack model
  (3.2), which scores nothing until attackers arrive. *Caveat: the king PST
  already encodes much of "king on e1 vs g1"; this adds only the explicit
  rights-lost / development-stage signal, and could instead live as a king-danger
  input in 3.2.* Seed 0, tuned in 4.3/4.5.

### Step 3.7 - Material imbalance (optional) — structure, seeded 0 (tuned in Phase 4) — Opus 4.8 high

Optional, lowest-yield, fiddliest. SF-style symmetric quadratic imbalance:
own-pair / opponent-pair coefficient matrices + redundancy terms (rook-pair,
knight-with-pawns, rook-with-pawns), all coefficients seeded `0`. Skip for a
leaner eval. Quadratic bookkeeping is error-prone → Opus 4.8 high.

### Step 3.8 - Gauntlet-driven eval additions (added 2026-06-19, re-survey of SF-classical terms)

A fresh pass over the Stockfish-classical eval surfaced terms missing from the
list above. Add each as inert-seeded structure (`bench 13` unchanged) and tune
in the matching Phase-4 stage. **Models:** Opus 4.8 medium (endgame/passer
items), Sonnet 4.6 medium (small terms).

- **Unstoppable passed pawn (rule of the square)** — a passed pawn the enemy king
  cannot catch (king outside the promotion square, accounting for side-to-move
  and the double-step). Near-winning eg score; every strong HCE has it. Add to
  **3.4 / 3.5**. *Highest-value item here* — and Basilisk's high
  repetition-draw rate in the gauntlet hints at weak conversion this helps.
- **Minor behind pawn** (SF `MinorBehindPawn`): minor directly shielded by a
  friendly pawn → small mg bonus. Add to **3.6**.
- **Pawn islands**: penalty scaling with the count of disconnected own-pawn
  groups. Add to **3.4** (`eval_pawns`).
- **Space upgrade**: replace the flat centre-files popcount (`eval.cpp:591-605`)
  with the SF shape — safe squares behind own pawns in the centre, **weighted by
  own piece count**, extra credit behind 2–3 pawns. Add to **3.6**.
- **Queen infiltration / exposed queen** (SF `QueenInfiltration`): bonus for our
  queen safely deep in the enemy half; small penalty for a queen on an
  enemy-attacked square. Add to **3.6**.
- **King protector** (SF `KingProtector`): penalty ∝ each own minor's distance
  from our king. Pairs with king safety (3.2). Add to **3.6**.
- **Winnable/complexity coupling**: wire the 3.6 initiative term and the 3.5
  scale-factor framework together to mirror SF's single `winnable` adjustment
  (scale eg toward draw for OCB / one-flank / few pawns; up for both-flank passers
  + king outflanking), rather than two independent terms.
- *(Optional, low yield)* bishop x-ray on pawns, rook+queen battery, slider-on-
  queen threat — into the **3.6** batch only if cheap.

**HCE source checklist — avoid SF-monoculture (do this term survey once, here).**
The list above (and Step 3.8) was derived almost entirely from
**Stockfish-classical**, which is exactly why the terms this plan was missing
(closedness, central-king danger, overloaded defender — now folded into 3.1/3.6)
are the *non-SF* ideas that live in other strong HCEs. Before freezing the
Phase 3 term list, cross-check it against a small fixed panel and pull in
anything material only one of them has:
- **Stockfish 11 / classical** (the SF base — already here),
- **Ethereal** (last strong HCE-era release) — cleanest tuned-HCE reference, and
  the source of `Closedness`,
- **RubiChess** HCE-era / classical eval — independent term set,
- **one independent current HCE** of choice (Igel-HCE, Lambergar) as a tiebreak.
This is a *term-selection* checklist (what to build), distinct from §10's
*strength* ladder (who to play). One survey pass, not a recurring gate.

**Phase 3 gate — eval-cost budget (whole enlarged eval).** The new terms are
seeded inert but their *structure still computes* (attack-map reads, threat /
passer loops run, then multiply by 0), so the full eval cost is already paid at
the Phase 3 head — measure it. Compare fixed-depth wall-time / NPS of the Phase 3
head vs the `phase1-final` head (best-of ≥5 `bench`, native+pext). If the
enlarged eval costs **>10–15 % NPS** beyond what 3.0's attack-map substrate
saved, treat it as a defect to fix *before* Phase 4 spends games: profile
(`perf`/VTune on the pext build) and apply **lazy eval** (skip the king-safety /
threats / mobility block when the tapered material+PST margin already exceeds a
safe bound) or add a **whole-eval cache** (Basilisk has only the pawn cache
today — `pawn_table_` — so a positional-eval cache keyed by the full hash is a
real available remedy here, unlike Rarog which already has one) or hot-loop
cleanup. Under budget → carry on; only pay the lazy-eval/cache complexity if the
budget is breached.

**Phase 3 gate — trace/eval regression tests (per inert term).** For **every**
new structural term, add a small deterministic CTest/`--verify` fixture that
proves: (a) the Texel trace reconstruction equals the direct eval delta, (b) eval
**symmetry** still holds (mirroring a position negates the score), (c) a
seeded-zero param changes its trace count but **not** the eval or `bench 13`, and
(d) the feature's activation count is **nonzero** on a curated position that
should trigger it. This is the guardrail that stops Phase 4 from spending SPRT
games tuning a broken or never-firing trace — it is cheap to run and it is what
makes the Step 4.0 feature-support diagnostics trustworthy.

---

## 6.5 Phase 4 - Eval Data-Fit Completion (the campaign; EXECUTE after Phase 3) — Sonnet 4.6 medium (driving)

Goal: activate the entire Phase-3 structure by fitting it to data in **one**
staged Texel campaign. The tuner, dataset, and `--verify` all exist from Phase 2
(Steps 2.0–2.3); the method, K-fit, Adam, and per-stage SPRT discipline are
exactly old Step 2.4 — reuse them. This is where the **deferred** old stages
(2.4c king safety, 2.4d PST, 2.4e polish) and the **new** structural groups come
alive. Realistic target: **+80–160 Elo** across the phase.

Bake each accepted stage into `EvalParams.h`, record `bench 13`, run CTest,
SPRT vs the previous accepted head at `tc=3+0.03`; confirm the whole-phase gain
at `tc=10+0.1` at the boundary.

### Step 4.0 - Tuner/data readiness gate (do before any Phase 4 stage) — Sonnet 4.6 medium; Opus 4.8 high for tuner-core changes

Strong HCE evals are won as much by the **tuning process** as by term selection;
the Stockfish/Ethereal-style terms built in Phase 3 only pay off if the fitter
can actually measure and constrain them. Clear this gate before staging:

| Item | Requirement | Why |
|---|---|---|
| Nonlinear king-safety support | Either restructure Phase 3.2 into linear traced inputs, or add an Ethereal-style special / finite-difference tuner path for the composite knobs (`ks_unit`, coordination, open-file, no-queen, safety-curve shape). | The linear-trace fitter cannot learn knobs whose coefficient is zero-seeded or depends on the safety-table index. Phase 4.3 must not pretend to tune untunable params. |
| Feature-support diagnostics | Count nonzero observations per parameter and per bucket; warn / freeze / merge very sparse params before fitting. | Stops rare HCE terms (endgame, queen-pressure) from learning random signs or giant values off a handful of positions. |
| Bucketed holdout | Report loss by game phase (opening / mid / endgame), material class (no-queens / OCB / rook / pawn endings), king-attack, passed-pawn, and quiet-threat buckets — not just the aggregate. Reuses the Step 2.2 trace counts, so it is a reporting layer, not new instrumentation. | A global loss drop can hide a regression in the domains HCE strength depends on. |
| Targeted-data policy | If one bucket is sparse/regressing, append *targeted* quiet positions for that bucket only (a filtered datagen/sample pass); reserve global regeneration for aggregate stagnation. | Keeps data work cheap and avoids washing out rare critical positions. |
| Phase/domain-balanced sampling | Datagen/extraction enforces quotas (or sampling probabilities) for opening/mid/endgame, pawn endings, passers, quiet threats, and king attacks. | Waiting for self-play to *naturally* supply rare terms leaves the tuner underdetermined. |
| Blended labels | Optionally train on `alpha*result + (1-alpha)*score_target` (a search-score / WDL teacher target). Keep engine output pure HCE — teacher labels are training data only. | Result-only labels are noisy; blended targets smooth the gradient and shrink the dataset needed. |
| Binary feature cache | Add a versioned trace/feature cache (schema + params hash + bucket metadata + labels) so repeated staged fits don't rebuild traces. | Phase 4 reruns many fits; a cache makes them fast and reproducible. |
| Regularization / shape constraints | L2-to-prior beyond PSTs, monotonic/smooth passed-pawn and safety curves, sign constraints on obvious penalties, optional PST smoothing. | The broad 2.4b scalar pass already produced implausible signs; constraints make the fit robust instead of decorative. |

A stage is only "clean" when the aggregate holdout **and** the relevant buckets
hold or improve, the feature-support diagnostics are sane, and `--verify` still
reconstructs exactly. A bucket that regresses while global loss drops is the
signal to investigate *before* the SPRT — implausible signs/shapes are a
dataset/trace/constraint bug, not a candidate.

**Stages — biggest lever first to de-risk; PST +
material last so they balance against the complete term set (fit once, no
redo):**

| Stage | Group unfrozen | SPRT | Notes |
|---|---|---|---|
| 4.1 | Material + any leftover structure-independent scalars (`misc`) | elo1=5 | Cheap re-confirm at the new structure. |
| 4.2 | Threats package (3.1) | elo1=3 | Drop the old flat `hang[]` term in the accepted tuned step. |
| 4.3 | King safety v2 (3.2): `safety_table[25]`, `ks_unit`, new safe-check / weak-ring / flank / danger inputs, shelter, storm | elo1=3 | Biggest single lever; requires the Step 4.0 nonlinear-safety tuner support first. |
| 4.4 | Per-count mobility tables (3.3) | elo1=3 | |
| 4.5 | Pawn refinement (3.4) + small positional terms (3.6) + `minors` | elo1=5 | |
| 4.6 | Material imbalance (3.7) | elo1=3 | Skip if 3.7 was skipped. |
| 4.7 | PSTs + material definitive refit (~778 params) | elo1=5 | L2 toward PeSTO; full dataset. Last; balanced against everything above. |
| 4.8 | Global polish — everything unfrozen, low lr | elo1=3 | Stop here regardless of outcome. Then run old Step 2.5 (2000-game validation vs `phase1-final`). |

Per-stage sanity rule: implausible signs/shapes (as the old "broad scalar"
attempt showed) mean a dataset/trace bug — fix before SPRT, not after. `--verify`
must pass before any fit.

#### Step 4.3b - King-safety SPSA polish (optional; decide when reached) — Sonnet 4.6 medium

After stage 4.3 is accepted, optionally run a **small game-based SPSA** over the
few highest-leverage king-safety knobs (the `safety_table` curve shape and the
new safe-check / weak-ring / no-queen unit weights — expose them under
`BASILISK_TUNE` first). Texel optimises "static eval predicts result," not game
strength directly, and king safety is where that gap bites most; SF/Ethereal
both game-tune their king-safety scalars on top of the data fit. Keep the group
tiny (≤6 knobs). SPRT `elo1=3` vs the post-4.3 head; keep only if it passes.
This is the **one** place worth spending extra game-compute on the eval.

---

## 7. Phase 6 - Time Management (clock games)

Goal: better budgets at increment time controls. The default SPRT now exercises
clock mode (`tc=3+0.03`), so this is refinement, not repair. Realistic target:
`+5` to `+20` Elo at clock TCs.

> **Step 6.1 was pulled forward to 2026-06-20**, ahead of Phases 3-5, because
> matching Rarog's proven Phase 2.2 + 2.9.1 fix required the full SF-style
> rewrite anyway — patching just the reserve onto Basilisk's old simpler
> formula (the original Step 2.9.1 patch) left the formula itself more
> aggressive than Rarog's at fast TCs. Step 6.2's gating SPRTs and the
> LittleBlitzer validation below are what's left before 1.6.0 can ship this.

### Step 6.1 - Budget formula upgrade — DONE (2026-06-20), ported from Rarog

Implemented as a **direct port of Rarog's Phase 2.2 Stockfish-style rewrite**
(`D:\code\rarog\src\time_manager.rs`), not the simpler tiered-percentage
placeholder originally sketched here — Rarog's gauntlet run validated that
formula, so Basilisk now shares it rather than re-deriving an independent one.

`compute_time_limit` (`src/search.cpp`) clock path now takes a `game_ply`
parameter (`2*(fullmove_number-1) + (side_to_move==BLACK)`, passed from the
two call sites: `Searcher::search` and the ponderhit branch of `check_stop`)
and computes:

```cpp
const double mtg = explicit_mtg ? std::min(limits.movestogo, 50) : 50.0;
// SF: timeLeft = max(1, time + inc*(mtg-1) - overhead*(2+mtg))
const double time_left = std::max(1.0, time + inc*(mtg-1.0) - overhead*(2.0+mtg));
// explicit movestogo: opt = min((0.88 + ply/116.4)/mtg, 0.88*time/time_left); max = 1.3+0.11*mtg
// sudden death:        logT-based optConst/maxConst (SF timeman.cpp constants),
//                       opt = min(0.012112 + (ply+3.22713)^0.46866 * optConst, 0.19404*time/time_left)
//                       max = min(6.873, maxConst + ply/12.352)
optimum_ms = max(opt_scale * time_left, 1.0);
maximum_ms = max(min(0.8097*time - overhead, max_scale*optimum_ms), optimum_ms);
```

No `BASILISK_TUNE` knobs were added — the formula is hardcoded, matching
Rarog (which also exposes no tunables for it; only `Move Overhead` is a UCI
option for either engine). Keeps the `movetime` path (already fixed in 2.9.1)
and the adaptive iteration-stop scaling (stability/score-drop/effort) exactly
as they were.

### Step 6.1b - Match Rarog's 2.9.1 reserve exactly — DONE (2026-06-20)

Folded Basilisk's Step 2.9.1 time-safety reserve into the new formula, using
**Rarog's exact mechanics** rather than Basilisk's original (slightly more
conservative) version: the reserve clamp now uses the **raw, un-adjusted**
clock value (`time`), not a pre-overhead-subtracted one —
`hard_ceiling = max(time - 2*overhead, 1.0)`, `maximum_ms = min(maximum_ms,
hard_ceiling)`. (Basilisk's pre-rewrite 2.9.1 patch subtracted overhead once
from `remaining` *and* clamped against `remaining - 2*overhead`, an effective
3x-overhead margin — more cautious than Rarog's 2x, but inconsistent with the
"match Rarog" goal, so the formula now uses 2x throughout, identical to Rarog.)

Gate: `bench 13` unchanged (`4,033,379` nodes, non-PGO release-pext — TM
doesn't affect fixed-depth bench), 8/8 CTest passed. Manually verified across
`wtime` from 60000 down to 0 (with `winc=30`): smooth degradation, no hangs,
budgets at the gauntlet TC (`wtime=3000 winc=30`) land at ~68-78ms — in the
same range as the whole opponent field observed in the prior LittleBlitzer
snapshot (65.6-70.0ms tpm across SF/Rarog/Basilisk).

### Step 6.2 - Gating

**LittleBlitzer leg — DONE and PASSED (2026-06-20).** Full 35,000-game overnight
pool at `tc=3+0.03` (Base 100/Inc 30 in LB units), `Basilisk 1.5.1-sftm` at
**default Move Overhead** (matching Rarog's own entry), vs Stockfish 2500/2600/
2700-capped and Rarog 2.1.0:

| Engine | Rating | Score | `t=` (time losses) |
|---|---:|---:|---:|
| Stockfish 18-2700 | 2704.7 | 64.0% | 0 |
| **Basilisk 1.5.1-sftm** | **2696.8** | **62.7%** | **0** |
| Rarog 2.1.0 | 2620.0 | 49.8% | 0 |
| Stockfish 18-2600 | 2600.0 | 46.4% | 0 |
| Stockfish 18-2500 | 2479.5 | 27.1% | 0 |

**Zero time losses across all 14,000 Basilisk games** (and zero for every other
engine in the pool, confirming the harness itself is sound) — the strongest
possible confirmation of the Step 6.1/6.1b fix. Strength is essentially tied
with SF-2700-capped, far ahead of the next tier; consistent with (and likely
exceeding) the earlier +54-vs-1.5.0 self-play finding.

**fastchess leg — skippable, do NOT use `sprt.ps1` for it (corrected
2026-06-20).** This step originally said "two SPRTs"; that's the wrong tool.
Step 6.1/6.1b is a forfeit fix, not a strength change — its true Elo delta vs
`phase1-final` is expected to sit near 0, which is the *worst* case for an SPRT
(`elo0=0 elo1=3` converges slowest exactly when the true value sits on the H0
boundary; it can run tens of thousands of games without crossing either bound).
If a second-harness sanity check is still wanted (Rarog finding #3: LB can
mismeasure some engines), use `tools/gauntlet.ps1` (fixed games, no stopping
rule) at `tc=10+0.1` and `tc=1+0.5` and read only the `t=` count, ignoring the
score — the games and opponent don't matter, only whether the engine ever
forfeits. Given the LB result already showed `t=0` for **every** engine in the
pool (not a Basilisk-specific blind spot), this adds little; **skip straight to
Step 2.9.3 (release) unless extra confirmation is wanted.**

### Step 6.3 - Optional SPSA

Step 6.1's Rarog port is **hardcoded with no `BASILISK_TUNE` knobs** (matching
Rarog), so there is nothing to SPSA as-is. Only if Step 6.2's SPRTs are
inconclusive and the lever still looks worth it: first expose the SF
optConst/maxConst constants under `BASILISK_TUNE` (add a `ConfigGroup tm`), SPSA
at `tc=10+0.1`, then re-SPRT. Likely skip — the LittleBlitzer leg already passed
(`t=`0) and the formula is the validated Rarog one; this lever is small and the
games are expensive.

---

## 8. Phase 7 - Remaining Feature Menu (only after the eval + search phases plateau)

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
- **Threat-aware quiet history / pruning** *(only after the Phase 3 attack maps
  are stable)*: index quiet history by a threat context (the moved piece was
  threatened, the move creates a safe threat, or it resolves a king/queen
  threat), and optionally tweak pruning margins by that context. This couples
  search to eval, so keep it plateau-only and SPRT-gated (`elo1=3`); sibling
  Rarog carries the same experiment.
- **6-ply continuation history** — Basilisk has 1/2/4-ply (`cont_hist1/2/4_`);
  add a 6-ply table as a gated experiment (sibling Rarog already runs 1/2/4/6).
  A small history change, so it belongs here on the plateau menu, not before the
  eval work; SPRT `elo1=3`, keep only if it passes.
- **Selective extensions à la SF** *(risky tree-size changes, try last, one at a
  time)*: a **passed-pawn-push extension** (extend a safe push to the 6th/7th) and
  a **castling / king-move extension**. Basilisk already has singular extensions
  and a double-extension cap, so these are purely additive; SPRT `elo1=3` each,
  drop on H0.
- Eval: bishop long-diagonal, rook trapped-by-king, queen infiltration,
  pawn-storm vs castled-side detection, knight-distance-to-kings.
- Multi-threaded improvements (only if pool play moves beyond Threads=1):
  proper main-thread voting, depth-skip schedules.

Per-feature checklist unchanged: implement exactly one feature, tests, CTest,
`bench 13`, expose constants under `BASILISK_TUNE`, tune, SPRT vs current
head, keep only H1-accepted.

---

## 9. Phase 8 - NNUE (Future Project, the actual path to Stockfish parity)

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
- Run **fixed-game validation**, not just SPRT self-play — **eval changes
  (especially the Phase 4 tuned weights) over-fit self-play more than search
  changes do**, so the gauntlet matters most after Phase 4. Use
  `tools/gauntlet.ps1` at a clock TC (`tc=10+0.1`).
- Scan logs for illegal moves, timeouts, disconnects, crashes, and `bestmove
  0000` from legal positions.
- Update `CHANGELOG.md` and version metadata only after both self-play SPRT and
  fixed-game validation are acceptable.

### Release cadence & version numbers (SemVer-adapted)

Basilisk has no public API, so SemVer maps to strength/architecture:

- **MAJOR (`X.0.0`)** — architecture swap. Reserved for **NNUE (Phase 8)** → `2.0.0`.
- **MINOR (`1.Y.0`)** — a phase/campaign that banks net new **SPRT + gauntlet
  validated** strength (this is how Phase 1's +27 went `1.4.9 → 1.5.0`).
- **PATCH (`1.y.Z`)** — robustness/bugfix-only, or a single small feature with no
  campaign behind it.

**The version reflects cumulative content since the last tag, not the most recent
phase.** So the post-2.9 release is a *minor* bump (it ships all unreleased Phase 2
eval work, +54) even though Step 2.9 itself is only a fix.

Don't release Phase 3 (bench-identical, zero strength). Don't sit on validated
Elo waiting for the next phase — tag at each banked-strength boundary below.

| Release point | Cumulative content since prior tag | Version | Pre-tag gate |
|---|---|---|---|
| **after Phase 2.9** | Phase 2 eval scalars (**+54 vs 1.5.0**) + time-forfeit fix | **`1.6.0`** | `t=`≈0 forfeit gauntlet **and** +54 holds vs `phase1-final` @ `tc=10+0.1` |
| (Phase 3) | bench-identical refactors only | **no release** | — |
| **after Phase 4** | full eval data-fit campaign (**+80–160**) | **`1.7.0`** | per-stage SPRT + boundary gauntlet @ `tc=10+0.1` (eval over-fits self-play — gauntlet matters most here) |
| **after Phase 5** | search-efficiency wave (**+20–50**) + wave2 SPSA | **`1.8.0`** | per-item SPRT + gauntlet |
| **after Phase 6** | increment-aware time management | **`1.8.1`** (patch; TM-only, small) or `1.9.0` if it banks real Elo | clock-TC SPRTs (`10+0.1`, `1+0.5`) |
| Phase 7 (menu) | feature-menu items, batched | patch per small batch; minor if a batch lands large Elo | per-feature SPRT |
| **Phase 8** | NNUE | **`2.0.0`** | full revalidation |

> **Current decision (2026-06-19):** the working number `1.5.1` undersells the
> Phase 2 eval gain (a *patch* number for the biggest eval jump so far). Retag the
> post-2.9 release as **`1.6.0`**.

### Release checklist (the model runs all of this when asked to "release vX.Y.Z")

A release is a discrete, scripted task — **do not skip steps**. Given a target
version (e.g. `1.6.0`), the model must:

1. **Confirm the gate passed** — the pre-tag gate for that row in the cadence
   table above (SPRT + clock-TC gauntlet; for 1.6.0: `t=`≈0 forfeits and the
   strength delta holds vs the prior tag). Do not release on unvalidated work.
2. **Bump the version in both source locations** (they must match):
   - `src/Constants.h` → `engineVersion` string.
   - `CMakeLists.txt` → `project(basilisk VERSION X.Y.Z …)` (this drives the
     `v${PROJECT_VERSION}` dist asset tag automatically).
3. **`README.md`** — no longer carries a per-version "Current release" /
   strength-update section (deliberately removed 2026-06-19); nothing to update
   here unless a feature/UCI-option list actually changed.
4. **Update `CHANGELOG.md`** — a new `## [X.Y.Z] - YYYY-MM-DD` entry in the
   Keep-a-Changelog format already used (Changed / Added / Testing sections with
   the SPRT + gauntlet numbers), plus the `[X.Y.Z]: .../compare/...` link at the
   bottom.
5. **Verify the release build is clean** — non-tune `release` build does **not**
   expose `BASILISK_TUNE` UCI options; `bench 13` runs; CTest 8/8.
6. **Build distribution binaries** (`pext` for local, plus the distributable
   release target) into `build/dist/` per README. (The full platform matrix —
   Linux x86_64/aarch64, macOS aarch64, avx2 — is built by
   `.github/workflows/release.yml` on tag push, not locally.)
7. **Commit the version bump + CHANGELOG.** **Do not tag. Do not push.**
   Tagging (`git tag vX.Y.Z`) and pushing (branch or tag) are exclusively
   manual actions the user performs themselves when they're ready to actually
   cut the release — tagging is what triggers the GitHub release workflow, so
   it is never automatic.
8. **Produce GitHub release notes** in a single copy-pasteable fenced block for
   the user to paste into the GitHub release form (summary, strength delta vs
   prior tag with SPRT/gauntlet numbers, changes, known limitations).

### Recommended gauntlet opponent ladder

Pick the exact binaries by **measured** score on the test machine (5950X) — these
are the targets, not absolute truths:

| Opponent | Why | ~Elo (CCRL) |
|---|---|---|
| Basilisk 1.5.0 (`phase1-final`) + Rarog | own baseline + sibling | ~3100 / ~3015 |
| **Critter 1.6a** | a strong old HCE reference to clear | ~3150–3200 |
| **Stockfish capped** — `UCI_LimitStrength`/`UCI_Elo`, start **2900**, then **3100**, then **3300** | a tunable known-rating yardstick (Basilisk already ~3100, so start higher than Rarog's ladder) | as capped |
| One independent mid/high HCE (Lambergar / Peacekeeper / Igel HCE, or another) | non-SF, non-sibling check | ~3050–3300 |

**Pick the capped-SF level where Basilisk scores ~30–70%** — sharpest signal —
and raise it as the eval phases land. The HCE ceiling is ~200 Elo below modern
SF; beyond it, NNUE (Phase 8) is the only lever.

**Gauntlet result + calibration (2026-06-19, 35k-game overnight pool at
`tc=3+0.03`):** Basilisk 1.5.1 (dev, partial Phase-2 scalar tuning **only**)
finished **2nd of 9, +54 Elo over 1.5.0** and effectively tied with
"SF-18 capped 2700" — strong validation that the eval-fit lever is real and large
*before* king safety, PSTs, or any Phase-3 structure. **But SF `UCI_Elo` is
calibrated at 120s+1s anchored to CCRL 40/4 — it is NOT a reliable absolute
anchor at `tc=3+0.03`** (a "2700" cap topped the pool). Do not read CCRL off the
capped-SF labels. **For a real CCRL placement, run a separate small gauntlet at a
slower TC (`tc=15+0.1`/`30+0.3`) that includes Critter 1.6a** — at `tc=3+0.03`
Critter forfeited every game, so the one clean external HCE anchor was lost.

> **Time-forfeit regression — fix before the eval campaigns (high priority).**
> The same gauntlet showed **Basilisk 1.5.1 lost 65 games on time** (`t=65`) vs
> 1.5.0's 18 and 1.4.9's 13 — the dev eval/search work *tripled* forfeits at the
> fast clock. That is pure lost Elo and one-sided against non-forfeiting SF, so it
> **contaminates every external gauntlet.** Add a hard remaining-time safety floor
> to `compute_time_limit` (and watch `t=` in every run) **now** — do not wait for
> Phase 6. Re-verify forfeits drop to ~0 before trusting any gauntlet number.

### How to estimate a CCRL rating (the Ordo numbers are otherwise arbitrary)

Ordo's `-a 3000` just floats the pool *average* at 3000 — the absolute numbers
mean nothing alone; only the relative deltas are real. To put the pool on the
CCRL scale:
1. **Include ≥1 engine with a stable published CCRL 40/15 rating** that runs
   cleanly at your TC — **Critter 1.6a (~3180)** at a slower TC, a classic anchor
   like **Fruit 2.1 (~2783)**, or any released engine on the CCRL list. Two
   anchors spanning the range let you check the fit.
2. **Anchor Ordo to it:** `ordo-win64.exe -p Results.pgn -o rating.txt -a <ccrl>
   -A "<exact PGN engine name>"` — `-A` *pins* that engine to its CCRL number and
   scales the rest relative (instead of `-a 3000` floating). With two anchors,
   pin each in turn and average the offset; the spread is your error bar.
3. **Run the anchoring gauntlet at a TC close to CCRL 40/15** (e.g. `40/300` or a
   long `60+0.6`), single thread, on the 5950X.
4. **Treat the result as ±50–100 Elo** — an estimate, not an official rating
   (TC/hardware/book/pool differ); within-pool deltas stay exact. Enough to know
   whether Basilisk is ~3100, ~3300, or closing on the ~3380–3450 top-HCE band.

### Speed / architecture note

Basilisk's NPS is **not** a bottleneck (it already out-searches Rarog: 2.76 M
nps / depth 13.8 vs 2.31 M / 12.4 in the gauntlet). Architecture use is good:
the local `release-pext` build is `-march=native` (`znver3` tuning) + `USE_PEXT`
(PEXT sliders, ideal on Zen3) + LTO + PGO. Any further speed is low priority —
**profile first** (`perf`/VTune on the pext build) before micro-optimising, and
only after the eval phases. (Rarog, by contrast, has a real ~16% nps gap to close
— part of which is that its test build uses generic `x86-64-v3` tuning, not
`native`; see Rarog PLAN §9 step 4.)

### Time / compute budget (Ryzen 9 5950X, shared machine)

High total budget, moderate patience — keep runs usable: datagen and SPSA at
`-Concurrency 24` (leave ~8 threads free); SPRT/gauntlets via the repo scripts'
defaults. **Texel fits are CPU-minutes — run them freely** (that is why the
plan tunes per-stage). The long poles are the one-time datagen (already done for
the current dataset) and the per-stage SPRTs (Phase 4) / the wave2 SPSA
(Phase 5).

---

## 11. Quick Commands

```powershell
# One-time fresh-clone setup
.\tools\setup_tools.ps1

# Build a named pext-PGO+TUNE binary into tools\test_engines\
.\tools\build_test.ps1 -Suffix phase2-base

# SPRT a gain candidate (default tc=3+0.03 clock conditions)
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-phase2-material-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-phase2-base-pext-pgo.exe `
    -NameA "Phase2Material" -NameB "Phase2Base"

# Refactor/default-equivalence SPRT, only when needed
.\tools\sprt.ps1 ... -Elo0 -3 -Elo1 3

# Time-management / LTC SPRT
.\tools\sprt.ps1 ... -TC "1+0.5" -Elo1 3

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
time management (verified sound down to 10 ms/move). The eval is structurally
thin in exactly the strong-HCE terms (toy king safety, pawn-only threats, linear
mobility, no attack maps, almost no endgame knowledge) — and §0.5 reorders the
work so we **build all that structure first (Phase 3, bench-identical, no games),
fit the whole enlarged eval once (Phase 4), then spend the search-constant SPSA
once at the final eval scale (Phase 5)**, then polish clock handling (Phase 6).
That is the maximum extractable from HCE (~+100–200 Elo over Phases 3–5); parity
with modern Stockfish is Phase 8's NNUE job, and every earlier phase makes that
switch cheaper and better tested.
