# Basilisk Strength Improvement Plan

> Current checkpoint: Basilisk is a feature-rich C++23 HCE engine. **Release
> `1.6.0` has shipped** (Phase 2 eval scalars, ≈+54 Elo over 1.5.0, plus the
> Rarog-ported logarithmic-time-left clock time management with its time-safety
> reserve); the repo is on branch `v1.7.0`. Phase 0 and Phase 1 are complete
> (pruning SPSA +18.87, LMR SPSA +15.63, validation vs 1.4.9 = +27.16 Elo), and
> Phase 2 (Texel infra + cheap scalar fits: material/mobility/passers/
> pawn-structure/hanging) and Phase 2.9 robustness are complete and released.
> **Phase 3 (eval structure build-out) is COMPLETE (2026-06-27): Steps 3.0–3.11
> all done.** The whole enlarged HCE structure is in place, seeded inert and
> Texel-traceable; bench fingerprint is now `4,168,590` (re-baselined by 3.5
> endgame knowledge and 3.11 lazy eval). The two behaviour-changing steps cleared
> their SPRTs (3.5 endgame kept; 3.11 lazy eval **+16.64 ± 7.03 Elo, H1**). **The
> **Phase 4 (the eval data-fit campaign) is UNDERWAY.** Step 4.0 (tuner readiness
> gate) DONE; **Stage 4.1 = king safety ACCEPTED +65.48 ± 13.58 Elo (H1, 1514
> games)** via the new `--tune-kingsafety` finite-difference path — bigger than
> Rarog's +42.5. **Stage 4.2 = threats ACCEPTED +79.13 ± 14.82 Elo** (H1, 1264
> games; old flat `hang_pen` dropped into the new package). Phase-4 cumulative
> ≈ **+144** in two stages. **Next: Stage 4.4 (mobility)**, then pawn+small /
> imbalance / PST+material / polish. Phase 4 ships release 1.7.0. Every Phase 3 step was a behaviour-identical
> refactor that spent no games (gate = `bench 13` identity + `--verify` + CTest). Do not
> start the Phase 4 eval data-fit or any search SPSA until the Phase 3 structure
> exists (see §0.5).
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
> 1.5.0. The strongest hand-crafted-eval engine ever shipped — **Stockfish 11,
> ~3440 CCRL** — is the real proof of how far a classical eval can go (and it sits
> ~200 Elo below today's NNUE Stockfish). **Caution when sizing HCE targets:**
> Berserk/RubiChess/Stash are often quoted at 3300+ as "strong HCE," but those are
> their **NNUE** versions — their *classical* builds were ~3000–3150; do not size
> HCE expectations off NNUE-era ratings. Phases 2-6 realistically buy +150 to +350
> Elo. Actual parity with modern Stockfish requires **Phase 9 (NNUE)**. The phases
> below are still the right order: a well-tuned HCE engine is also the best data
> generator and test harness for a future NNUE. See **§7.5 (Phase 7)** for the full
> non-NNUE ceiling analysis and the post-Phase-5 eval-refresh multi-cycle grind.
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
search-constant SPSA (old search Step 3.9, now Step 5.9) *before* the Phase-4
eval refit rescales every centipawn-denominated margin. That is wasted self-play
compute. The order below removes the waste.

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
| done | **Phase 2** | Texel **infra + cheap, structure-independent scalar fits** | §4 Steps 2.0–2.4b (material/mobility/passers/pawnstruct/hanging **accepted**) |
| done (shipped `1.6.0`) | **Phase 2.9 — Robustness quick win** (bench-identical) | the time-safety floor (65 forfeits) — pulled before the eval work | §4.9 (new) |
| **done (2026-06-27)** | **Phase 3 — Eval structure build-out** (3.0–3.11) | attack maps, threats pkg, KS v2, per-count mobility, pawn refine, endgame scaling, lazy eval | **old §6 Phase 4** (moved *earlier*) |
| **NEXT** | **Phase 4 — Eval data-fit completion** (one campaign) | threats group, KS-v2 block, mobility tables, minors, **PST + material refit**, polish | old §4 Steps 2.4c–2.5 (the **deferred** tuning) |
| then | **Phase 5 — Time management hardening + tuning** | clock-at-`go` latency fix, anti-overshoot poll granularity, GUI-robust reserve, root fail-low extension, **TM-constant SPSA**, cross-TC validation | promoted ahead of search 2026-06-29 (recurring LB time-losses + untapped tuning) — see §7 |
| then | **Phase 6 — Search efficiency wave** (SPSA last) | TT-bound eval, history split, fractional LMR, deeper re-search, qsearch checks, **wave2 SPSA** | old §5 Phase 3 → search wave (moved *after* TM) |
| then | **Phase 7 — Non-NNUE ceiling: eval-refresh multi-cycle grind** | regen self-play with the stronger head, joint refit (1–3 cycles), bank the tuning-maturity Elo | §7.5 (new; ported from sibling Rarog's Phase 6) |
| then | **Phase 8 — Feature menu** | the plateau menu | old §8 Phase 6 |
| last | **Phase 9 — NNUE** (terminal option) | the ceiling-raiser | old §9 Phase 7 |

> **[HISTORICAL — superseded 2026-06-27. Current status: Phases 2/2.9/3 all DONE;
> NEXT = Phase 4. See the §0 checkpoint at the top and the §0.5 table above.]**
>
> **Where Basilisk was at 2026-06-18:** mid old-Step-2.4b. Accepted:
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
| **3** | eval structure build-out (bench-identical) + NPS recovery (3.10 hot-loop cleanup, 3.11 lazy eval) | 0 direct (enabler); **~−22 % NPS if unmitigated** (sibling Rarog, measured) → recovered by 3.10/3.11 |
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

NNUE stays the terminal option (Phase 9).

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
   search. This keeps Phase 2 manageable and Phase 9 (NNUE) possible.
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

## 4.9 Phase 2.9 - Robustness quick win (DONE — shipped in release 1.6.0)

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
LittleBlitzer probe showed was tighter to the margin than Rarog's logT-based one.
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
>    normal allocation (and the Elo from the logT-based TM) untouched.
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

## 5. Phase 6 - Search Efficiency Wave (close the EBF gap) — EXECUTE AFTER Phase 5 (TM)

> **Renumbered 2026-06-29: this search-efficiency wave is now Phase 6, executed
> AFTER the Phase 5 time-management hardening (§7 below).** TM was promoted ahead
> of it because LB still shows time-losses and the TM constants have never been
> tuned for Basilisk — both higher-confidence, lower-risk Elo than the search
> wave, and TM robustness should be solid before we change search shape. Step
> numbers below keep their `5.x` labels for now (history); treat them as Phase-6
> work.

> **Order (§0.5):** this runs **after** the eval is final (Phase 4). The Step 5.9
> (old search Step 3.9) **second-wave constants SPSA is the conserved compute** — its margins
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
eval, and Lambergar's HCE tuning notes (the §3.9 checklist). The conclusion is
**not** "add a large new feature family" — the plan already has the right strong-
HCE core. The remaining work is making these structures **tunable and
measurable**: nonlinear king-safety tuner support, feature-support diagnostics,
better data balance (Step 4.0), promotion-path passer safety (3.4), exact KPK
(3.5), space/winnable coupling (3.6), and a handful of cheap SF11-classical /
Ethereal positional terms (3.7/3.9). Keep that framing while building
Phase 3 — structure that the Phase-4 fitter cannot measure is dead weight.

**2026-06-24 SF16-classical addendum (corrected 2026-06-25 — there are *no*
post-SF11 HCE improvements to mine).** Verified against the Stockfish sources:
its classical eval was **frozen the moment NNUE landed (SF12, 2020)** and was
**removed in SF16 (commit `af110e0`, July 2023)** — by then it ran only on
nearly-material-decided positions where speed beat accuracy, worth ~**2 Elo**.
So **SF16's classical eval ≈ SF11's** — no terms were added or tuned in between.
The practical consequence: **SF11 remains the HCE benchmark**, and SF16 is **not**
a source of new ideas; it is only a convenient *faithfulness cross-check* that we
implemented SF11's terms completely. The items folded into Phase 3 below
(space-weighted-by-pieces + a single winnable/complexity coupling in 3.6; the
small minor/rook/queen-pressure terms in 3.7/3.9; multi-threshold lazy eval in
3.11) are **SF11-era terms Basilisk simply had not built yet — not deltas over
SF11.** Build them because they are good SF11 terms, not because SF16 "improved"
the HCE (it did not). For anything genuinely *post-SF11* in hand-crafted eval,
the real sources are the engines that kept developing HCE after SF froze it
(Ethereal, RubiChess, Koivisto-classical) — see the §3.9 checklist.

### Step 3.0 - Attack-map infrastructure (behaviour-identical refactor, do first) — Opus 4.8 medium / GPT-5.5 high

> **DONE 2026-06-24.** `Evaluator::evaluate` (`src/eval.cpp`) now builds, once
> per call: `attacked_by[NCOLORS][PIECE_TYPE_NB]` (per-type unions, pawns + king
> included), `attacked[NCOLORS]` (full union — provably identical to
> `is_attacked_by(sq, all_occ, c)` as a bitboard, since that predicate is exactly
> the OR of the same per-type attack sets), `attacked2[NCOLORS]` (2+ attackers),
> and `king_zone[NCOLORS]`. Mobility and king-attacker pressure are folded into a
> **single** knight/slider sweep (previously each non-pawn piece's slider attack
> was computed twice — once for own-color mobility, once as the enemy of the other
> king); king safety is split into that gather plus a finalization loop
> (coordination bonus, no-queen scaling, open-file penalty, table lookup —
> arithmetic unchanged because integer accumulation is order-independent). The
> hanging-piece term now tests membership in `attacked[them]`/`attacked[us]`
> instead of two per-piece `is_attacked_by()` calls. `attacked_by`/`attacked2`
> are seeded substrate for Steps 3.1/3.2 (no current consumer). **Gates passed:**
> `bench 13 = 4,033,379` nodes (byte-identical to the 1.6.0 baseline), 8/8 CTest,
> texel `--verify` reconstructs all 8,598 positions exactly. **Deviation from the
> spec below:** the `blockers_for_king` / pinned-piece masks are **deferred to
> Steps 3.2/3.3**, where they are first consumed — computing them in 3.0 with no
> consumer would be pure NPS cost against the §3.10/§3.11 NPS budget, with zero
> eval effect; they will be added to the same hot slider sweep when 3.2/3.3 need
> them, which still avoids a second sweep.

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

> **DONE 2026-06-25.** Added a threats package (attacker/victim-typed threat
> bonuses, hanging, queen-only-defended, restricted-square, safe-pawn-push) to `evaluate()`
> (`src/eval.cpp`, right after the existing flat pawn-threats block) consuming the
> Step 3.0 attack maps. New traced params (all **seeded 0** → bench unchanged), in
> `EvalParams.h` + the `EVAL_PARAM_LIST` registry: `threat_by_minor_mg/eg[7]` and
> `threat_by_rook_mg/eg[7]` (indexed by attacked piece type), `threat_by_king`,
> `threat_hanging` (weak piece undefended or hit twice), `weak_queen_prot` (weak
> piece whose only defender is its queen — the "defended once" refinement),
> `restricted` (squares the enemy attacks but cannot firmly hold), and
> `threat_push` (enemy non-pawn attackable by a safe single/double pawn push).
> Uses the SF `strongly_protected` / `defended` / `weak` decomposition from
> `attacked_by[]`/`attacked[]`/`attacked2[]`. The existing flat pawn-threat and
> `hang_pen` terms stay active until Phase 4.2 swaps in the tuned replacement.
> **Overloaded-defender (optional below) deferred** — it complicates the pass; add
> in Phase 4 only if cheap. **Gates passed:** `bench 13 = 4,033,379` (identical),
> 8/8 CTest, texel `--verify` reconstructs all 8,598 positions exactly. Per-term
> activation diagnostics are deferred to the Step 4.0 feature-support tooling.

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

> **DONE 2026-06-25.** Rebuilt the king-safety finalization in `src/eval.cpp` as
> a full danger model feeding the existing `attack_units → safety_table` funnel,
> every new input **seeded 0** (and the no-queen relief re-expressed as
> `ks_noqueen_num/den = 2/3`, byte-identical to the old `*2/3`) so bench is
> unchanged. New `EvalParams` (registered; **no linear trace** — they shape the
> index and are tuned by the Phase-4.3 finite-difference path): `ks_safe_check[7]`
> (per-type safe checks via `check_squares(pt, them) & attacked_by[them][pt] &
> safe`), `ks_weak_ring`, `ks_ring_pressure`, `ks_flank_attack`/`ks_flank_defense`
> (camp ∩ king-flank files), `ks_pawnless_flank`, `ks_king_blockers`,
> `ks_central_king` (central file + castling rights gone), `ks_shelter_storm`
> (open-files-near-king proxy folded into danger — the existing linear shelter/
> storm block stays active in parallel per the door-open note), `ks_noqueen_num/
> den`. **Also computed `blockers_for_king[NCOLORS]`** once after the attack-map
> sweep (own pieces pinned in front of their king, via empty-board snipers +
> single `BB_BETWEEN` occupant) — this **resolves the Step-3.0 deferral**; Step 3.3
> consumes the same masks for the mobility area. **Gates passed:** `bench 13 =
> 4,033,379` (identical), 8/8 CTest, texel `--verify` exact on all 8,598 positions.

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
  rights gone — folded in here rather than as a separate 3.7 term so it shares
  the danger funnel),
- **shelter/storm folded into the danger funnel** — Basilisk's pawn shelter
  (`shelter_missing_center/flank`, `shelter_close1/2`) and storm
  (`storm_weight_kf/adj`) terms are today scored **linearly in a separate block**
  (`eval.cpp` "King pawn shelter"), fully decoupled from the
  `attack_units → safety_table` funnel. **Sibling-engine evidence (Rarog,
  2026-06-24):** its Phase-4 fit drove the equivalent storm/shelter weights to
  **0**, because a linear term cannot capture that an *exposed* king is dangerous
  *in combination with* piece pressure — the danger interaction lives in the
  `safety_table` nonlinearity, not in a standalone linear penalty. So add a
  shelter/storm contribution as **additional danger-index inputs** (seeded inert /
  linear-equivalent so `bench 13` is unchanged), letting the Phase-4.3 KS re-eval
  fit learn the interaction the linear term can't. **Door open (Basilisk may
  differ):** *keep the existing linear shelter/storm term as a tunable parallel
  component* rather than deleting it — SF carries both, and if Basilisk's own
  linear fit lands plausible nonzero shelter/storm values the linear part still
  earns its place; let the fit decide the split, don't assume Rarog's exact
  zero-collapse repeats in a different eval,
- **no-enemy-queen scaling** of the whole danger sum (kept as the frozen `*2/3`
  seed, exposed as a tunable scalar).

Tune the king-safety block in Phase 4.3, behind the Step 4.0 nonlinear-safety
tuner support (these inputs are composite, not pure-linear).

### Step 3.3 - Per-count mobility tables — structure, seeded linear-equivalent (tuned in Phase 4.4) — Opus 4.8 high

> **DONE 2026-06-25 (table refactor; area refinement deferred to 4.4).** Replaced
> the linear `mob * mob_mg/eg[pt]` with one-hot per-count tables `mob_n_mg/eg[9]`,
> `mob_b_mg/eg[14]`, `mob_r_mg/eg[15]`, `mob_q_mg/eg[28]` in `EvalParams.h`
> (registry `MobNMg…MobQEg`; old `mob_mg/eg[7]` + `MobMg/MobEg` removed), seeded
> `table[i] = i * old_weight` (N/B/R/Q mg 5/5/1/2, eg 5/7/7/12) so eval is
> byte-identical. The eval loop indexes by the safe-mobility count and traces
> one-hot; `tools/texel/tuner.cpp` `mobility` group + clamps updated to the 8
> tables. **The mobility *area* is left exactly as today (`att & ~pawn_atk[them] &
> ~own_occ`).** Per sibling Rarog's precedent (its 3.7 did the same and explicitly
> deferred the area change), the mobility-area refinement — exclude own K/Q, own
> blocked pawns, and own pinned pieces (`blockers_for_king`, computed in 3.2) — is
> a **behaviour change that interacts with the fit, so it moves into Phase 4.4**
> (A/B it there alongside the table tuning), not Phase 3. **Gates passed:** `bench
> 13 = 4,033,379` (identical), 8/8 CTest, texel `--verify` exact on all 8,598.

Replace linear `mob * w` with one-hot tables indexed by popcount:
`mob_n[2][9]`, `mob_b[2][14]`, `mob_r[2][15]`, `mob_q[2][28]` (mg/eg).
Initialize from the current linear values (`table[i] = i * w`), refine the
mobility *area* to exclude squares occupied by own king and queen, own blocked
pawns (SF-classical definition), and own pinned/blocking pieces (from the Step
3.0 pin masks — a pinned piece's "mobility" is illusory). Tune the mobility group
in Phase 4.4.

### Step 3.4 - Pawn-structure refinement — structure, seeded inert (tuned in Phase 4.5) — Opus 4.8 medium

> **DONE 2026-06-25.** Added the refinement terms, **all seeded 0** (bench
> unchanged); the existing flat doubled/isolated/connected/backward/candidate and
> passer terms stay active. **Pawn-cache-safe terms in `eval_pawns`** (depend only
> on pawns): `connected_rank_mg/eg[8]` (connected = supported *or* phalanx,
> one-hot by relative rank), `weak_unopposed_mg/eg` (isolated/backward pawn on a
> half-open file), `pawn_lever_mg/eg` (own pawn attacking an enemy pawn),
> `blocked_pawn_mg/eg[2]` (rammed by an enemy pawn on rel rank 5/6),
> `pawn_majority_mg/eg` (own pawn majority per flank — the breakthrough proxy,
> distinct from the existing `cand`). **Piece-dependent terms in `evaluate()`**
> (use the attack maps / king squares, so outside the pawn cache):
> `passed_path_safe_eg` (whole promotion path free of enemy attack, ×rel rank),
> `passed_block_defended_eg` (block square defended by us), `passed_king_block_eg`
> (king distance to the block square), `blockader_knight_eg` (our knight directly
> in front of an enemy passer — Nimzowitsch ideal blockader). The Tarrasch
> rook-behind-passer term already exists and is left active. **Gates passed:**
> `bench 13 = 4,033,379` (identical), 8/8 CTest, texel `--verify` exact on all
> 8,598 positions.

Connected/phalanx bonus by rank (SF formula `seed[r]` style, one-hot traced),
weak-unopposed penalty, blocked pawns on 5th/6th, king-pawn-distance endgame
term, **pawn levers** (a pawn move that creates a lever), and a **candidate
passer / majority breakthrough** term — a pawn that becomes passed in 1–2 pawn
moves (own majority with no/fewer enemy pawns ahead), distinct from already-passed
and from the unstoppable rule-of-square logic (3.9). Upgrade passed-pawn safety
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

> **DONE 2026-06-25.** Added a `ScaleFactor` framework (0–64, `SCALE_NORMAL = 64`,
> `SCALE_DRAW = 0`) and an `apply_endgame()` pass that runs on the tapered score
> *before* the 50-move damping. Implemented: an exact **KPK bitbase**
> (retrograde fixed-point, `g_kpk`, lazily built once via `kpk_init`) returning a
> `KNOWN_WIN`-magnitude score for won KPK and `SCALE_DRAW` for drawn KPK incl. the
> rook-pawn/wrong-corner draw; **KBNK** corner mop-up (`kbnk_score`) that drives
> the bare king to the bishop-coloured corner (dark bishop → {a1,h8}, light →
> {a8,h1}); **KNNK** → draw; **no-pawn ≤ minor advantage** → scale toward draw;
> generalised **OCB** scaling folded into the framework (behaviour-identical to the
> old rule when it applies). The frozen generic mate-drive mop-up is kept for the
> general KXK case. All Step 3.5 terms are **frozen constants** (not Texel-traced),
> so they land in the tuner's `rest` term and `--verify` stays exact by
> construction.
>
> **Deviation from the original plan (important).** The spec assumed 3.5 would be
> `bench 13`-invisible "because the patterns are absent from the bench suite." That
> assumption is **wrong**: KPK / KRKR-scaling material is *reachable at the leaves
> of a normal deep search* of the bench's pawn/rook endings (position 3, the rook
> ending `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8`, in particular), so the new knowledge
> legitimately changes search scores. Step 3.5 is therefore **behaviour-changing**,
> unlike the bench-identical Steps 3.0–3.4. It is gated by the deterministic
> endgame suite (below), and because it touches common endings it should also pass
> a non-regression **SPRT** before merge — see the prepared command in the working
> notes. **Gates passed:** new fingerprint `bench 13 = 4,377,437` (was 4,033,379;
> only position 3 changed), 9/9 CTest, new `tests/endgames.epd` suite 18/18, texel
> `--verify` exact (terms are frozen/non-traced).
>
> **SPRT result (2026-06-25, `tc=3+0.03`, simplify gate `[-5,0]`):** `Phase35Endgame`
> (3.4+3.5+3.6 head; 3.6 is identity so this isolates 3.5) vs `Phase34Base` (pre-3.5
> head) — **−4.17 ± 5.20 Elo** (nElo −6.27 ± 7.82), LOS 5.79%, 7,586 games, LLR −1.18
> inside (−2.94, 2.94), stopped manually. **Read: NPS tax, not a chess bug.** The EPD
> suite passing (KBNK/KPK/dead-draws all correct) rules out a gross scaling
> regression; the small negative is the per-node cost of `apply_endgame()` running a
> full piece census on **every** node while only paying off in rare endgames — the
> harshest possible TC for endgame knowledge. This is the same NPS-tax shape the whole
> seeded-inert Phase-3 structure carries (Rarog: −22% NPS / −32.6 Elo at its inert
> head), which the plan **defers to NPS recovery (3.10/3.11) and to the Phase-4
> boundary** (live + tuned, ideally LTC), *not* a fast-TC standalone. **Decision: keep
> 3.5, do not revert** — correct, EPD-proven knowledge; its cost is recovered by the
> `apply_endgame` early-out guard in Step 3.10 (lazy eval 3.11 cannot skip endgame
> scaling — it must run on both paths). A minor possible second-order chess cost (the
> "no-pawn ≤ minor" rule flattens all KRKm to ~4/64, but a few KRKm are winnable) is a
> review item for 3.10, not a blocker. **Process note:** do **not** fast-TC-SPRT-gate
> the remaining seeded-inert structural steps and revert on small negatives — that
> contradicts §0.5 and discards structure that pays off in Phase 4; this SPRT was a
> one-time correctness check on the lone *live* step and it passed that purpose.

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

### Step 3.6 - Space + winnable coupling — structure, seeded equivalent/inert (tuned in Phase 4.5) — Sonnet 4.6 medium

> **DONE 2026-06-25 (bench-identical).** Both items added as seeded-inert
> structure; `bench 13 = 4,377,437` unchanged, 9/9 CTest, `--verify` exact.
> **Space refinement** (`eval.cpp`, traced, all weights seeded 0): kept the base
> `SpaceMg` term active and added three new traced mg features — `SpaceBehindMg`
> (safe central squares sheltered behind own pawns), `SpacePieceMg` (each side's
> safe-space count × that side's non-pawn piece count), `SpaceBlockedMg` (× own
> blocked-pawn count). Linear/traceable → tuned with the `misc` batch in Phase
> 4.5. **Winnable/complexity coupling** (`evaluate()`, **frozen, not traced**, all
> seeded 0): a complexity term over king outflanking, both-flanks pawns, king
> infiltration, pawn-only endgame, passed-pawn count and total-pawn count (with a
> bias offset), applied to `eg` with a **sign-preserving clamp**
> (`eg += sign(eg)·max(complexity, −|eg|)`) so it can never flip the eg sign. Its
> nonlinearity (the sign clamp) means it is *not* linearly traced — like the 3.2
> KS funnel, it is finite-difference tuned in Phase 4.5; seeded 0 → contributes
> exactly 0 today (so bench/`--verify` are untouched). New tuner groups/clamps:
> `misc` extended with the three space terms; a new `winnable` group registers the
> seven complexity knobs for the Phase-4.5 finite-difference pass. **Gates
> passed:** `bench 13 = 4,377,437` (identical), 9/9 CTest, `--verify` exact.

Promote two **SF11-era** items from the old survey menu — they are not throwaway
extras; they shape how the whole endgame score is interpreted (re-confirmed by the
SF16 cross-check, which found SF11 and SF16 carry the same terms — see the
addendum above):

- **Space upgrade** — replace the flat centre-files popcount (`eval.cpp:624-638`)
  with the SF shape: safe central squares, extra credit for safe squares behind
  own pawns, weighted by own piece count **and blocked-pawn count**. Seed it to
  the current flat `space_mg` behaviour where possible; any extra factors start
  at 0 so `bench 13` is unchanged. Tune with the small/pawn batch in Phase 4.5.
- **Winnable/complexity coupling** — wire initiative and scale-factor together
  rather than keeping them as independent nudges. Inputs: passed-pawn count, total
  pawns, both-flanks pawns, king outflanking/infiltration, no-non-pawn-material,
  and drawish one-flank/OCB signals. This depends on the 3.5 scale-factor
  framework. Seed linear-equivalent to today's no-initiative/no-extra-scale
  state, then tune in Phase 4.5. The important design constraint is that the
  complexity bonus must not flip the sign of the mg/eg score.

### Step 3.7 - Small positional terms — structure, seeded inert (tuned in Phase 4.5) — Sonnet 4.6 medium

> **DONE 2026-06-25.** Added a batch of small positional terms in `evaluate()`
> (new `for c` block before the space term, using the Step-3.0 attack maps +
> `king_zone`), **all seeded 0** so the eval is unchanged. New traced `EvalParams`:
> `bishop_outpost`, `reachable_outpost` (per outpost square a knight can hop to),
> `bad_bishop` (per own pawn on the bishop's colour), `minor_king_ring` /
> `rook_king_ring` (minor/rook attacking the enemy king ring), `rook_closed`
> (rook on a both-colour-pawn file), `rook_queen_file` (rook shares a file with
> the enemy queen), `connected_rooks`, `weak_queen` (enemy queen on a square we
> attack), `bishop_pair_pawns` (bishop pair × own pawn count — added alongside the
> existing flat `bp_mg/eg`, which stays active). **Deferred** (optional / lower
> value, to keep the pass cheap): long-diagonal, bishop x-ray on pawns,
> uncontested side outpost, and the Ethereal closedness / central-king terms.
> **Gates passed:** `bench 13 = 4,377,437` (identical to the post-3.5 fingerprint),
> 9/9 CTest, texel `--verify` exact on all 8,598 positions.

Pull the high-value items from the old Phase-6 menu forward as structure (each a
few lines, all seeded `0` so `bench 13` is unchanged; the tuner decides their
worth in Phase 4): **bishop outposts** (mirror the knight-outpost logic,
`eval.cpp:335-353`), **reachable knight outposts** (an outpost square a knight
can reach in one safe hop, not only occupy), **uncontested side outpost**
(knight on a flank outpost with few relevant targets), **trapped-rook-by-own-
uncastled-king**, **connected rooks**, **rook on a closed file**, **rook/bishop
pressure on the king ring**, **bishop on a long diagonal** bearing on the enemy
king, **bishop x-ray on enemy pawns**, **bad bishop** (own pawns on the bishop's
colour, optionally edge-file bucketed), and **bishop-pair scaled by pawn count**
(replacing the flat `bp_mg/bp_eg`). Add the cheap **queen-pressure** terms here
too — rook on the queen's file, weak/exposed queen on an enemy-attacked square,
**weak queen only protected by queen**, and slider-on-queen / pinner threat
**only if** it falls out of the attack maps cleanly. Trace each; batch into one
Phase-4 tuning stage.

Two non-SF-classical terms (from the Ethereal/RubiChess survey — see the HCE
source checklist after 3.9), both **optional and possibly redundant**, add only
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

### Step 3.8 - Material imbalance (optional) — structure, seeded 0 (tuned in Phase 4) — Opus 4.8 high

> **DONE 2026-06-25.** Added the symmetric quadratic material imbalance in
> `evaluate()` (right after the material/PST + phase block), **all coefficients
> seeded 0** → contributes exactly 0 today. Piece index 0 = bishop pair, 1 = pawn,
> 2 = knight, 3 = bishop, 4 = rook, 5 = queen. New `EvalParams`: `imb_linear[6]`,
> `imb_our[21]`, `imb_their[21]` (lower-triangular `t = i*(i+1)/2 + j`,
> own-vs-own and own-vs-enemy count-product coefficients). **Key simplification:
> the eval is *linear in the coefficients* (the feature is the count product), so
> it is traced normally (`TR_BOTH`, added equally to mg and eg as a non-tapered
> material correction) and fit by the **ordinary linear tuner** — no
> finite-difference path needed.** The `--verify` pass (exact on all 8,598) is the
> proof the quadratic count-product bookkeeping is correct. Added an `imbalance`
> tuner group (`tools/texel/tuner.cpp`). **Gates passed:** `bench 13 = 4,377,437`
> (identical), 9/9 CTest, texel `--verify` exact. *(Optional step — implemented;
> the tuner decides its worth in Phase 4.6, skip-able if it earns nothing.)*

Optional, lowest-yield, fiddliest. Symmetric quadratic material imbalance:
own-pair / opponent-pair coefficient matrices + redundancy terms (rook-pair,
knight-with-pawns, rook-with-pawns), all coefficients seeded `0`. Skip for a
leaner eval. Quadratic bookkeeping is error-prone → Opus 4.8 high.

### Step 3.9 - HCE source checklist + remaining survey additions (updated 2026-06-24, SF16 pass included)

> **DONE 2026-06-25.** Added the survey terms, **all seeded 0**: `unstoppable_passer_eg`
> (rule of the square — fires only when the enemy has no non-king pieces and the
> promotion path is clear; enemy king outside the square, defender-tempo aware) in
> the passer block; `minor_behind_pawn` + `king_protector` (per square of own-minor
> distance to our king) in the 3.7 bishop & knight loops; `queen_infiltration` (queen
> on rel rank ≥5 not attacked by an enemy pawn) as a new queen loop; `pawn_islands`
> (disconnected own-pawn file groups) in `eval_pawns` (cache-safe). The optional
> low-yield trio (bishop x-ray on pawns, rook+queen battery, slider-on-queen) was
> **skipped**. **Gates passed:** `bench 13 = 4,377,437` (identical), 9/9 CTest, texel
> `--verify` exact on all 8,598.

A fresh pass over Stockfish-classical (cross-checked against SF16 — which, being
SF11's frozen eval, surfaced **no new terms**; the additions below are SF11 terms
this plan had not yet listed) surfaced items missing from the list above. Add each
as inert-seeded structure (`bench 13` unchanged) and tune in the matching Phase-4
stage. **Models:** Opus 4.8 medium (endgame/passer items), Sonnet 4.6 medium
(small terms).

- **Unstoppable passed pawn (rule of the square)** — a passed pawn the enemy king
  cannot catch (king outside the promotion square, accounting for side-to-move
  and the double-step). Near-winning eg score; every strong HCE has it. Add to
  **3.4 / 3.5**. *Highest-value item here* — and Basilisk's high
  repetition-draw rate in the gauntlet hints at weak conversion this helps.
- **Minor behind pawn** (SF `MinorBehindPawn`): minor directly shielded by a
  friendly pawn → small mg bonus. Add to **3.7**.
- **Pawn islands**: penalty scaling with the count of disconnected own-pawn
  groups. Add to **3.4** (`eval_pawns`).
- **Queen infiltration / exposed queen** (SF `QueenInfiltration`): bonus for our
  queen safely deep in the enemy half; small penalty for a queen on an
  enemy-attacked square. Add to **3.7**.
- **King protector** (SF `KingProtector`): penalty ∝ each own minor's distance
  from our king. Pairs with king safety (3.2). Add to **3.7**.
- *(Optional, low yield)* bishop x-ray on pawns, rook+queen battery, slider-on-
  queen threat — into the **3.7** batch only if cheap.

**HCE source checklist — avoid SF-monoculture (do this term survey once, here).**
The list above (and Step 3.9) was derived almost entirely from
**Stockfish-classical**, which is exactly why the terms this plan was missing
(closedness, central-king danger, overloaded defender — now folded into 3.1/3.7)
are the *non-SF* ideas that live in other strong HCEs. Before freezing the
Phase 3 term list, cross-check it against a small fixed panel and pull in
anything material only one of them has:
- **Stockfish 11 / classical** (the clean SF HCE base — already here; **this is
  the SF reference**),
- **Stockfish 16 classical** (≈ SF11's *frozen* eval, removed July 2023 — adds
  **no** terms over SF11; use only as a faithfulness cross-check that SF11's terms
  are all present, never as a rating target or a source of new ideas),
- **Ethereal** (last strong HCE-era release) — cleanest tuned-HCE reference, the
  source of `Closedness`, and — unlike SF — a genuine *post-SF11* HCE that kept
  being developed,
- **RubiChess** HCE-era / classical eval — independent term set,
- **one independent current HCE** of choice (Igel-HCE, Lambergar) as a tiebreak.
This is a *term-selection* checklist (what to build), distinct from §10's
*strength* ladder (who to play). One survey pass, not a recurring gate.

### Step 3.10 - Eval hot-loop cleanup (behaviour-identical NPS recovery) — PROFILE-GATED (likely light) — Opus 4.8 medium

> **DONE 2026-06-25 (the `apply_endgame` guard — the one identified high-value
> win).** Restructured `apply_endgame()` (`src/eval.cpp`) so the 12-popcount piece
> census + the five known-endgame rules (KNNK / KPK / KBNK / KBP wrong-bishop /
> no-pawn-≤-minor scaling) run **only when** a side is a lone king or pawnless —
> the necessary condition for any of them to fire. In the opening/middlegame the
> whole block is skipped; the OCB scaling stays outside the guard and runs always.
> This is **behaviour-identical** (the guarded rules provably cannot fire when the
> condition is false), so `bench 13` is unchanged at **4,377,437** — confirming the
> guard only skips work that would have no-op'd. It removes the per-node census
> that was the dominant cost behind 3.5's −4.17-Elo fast-TC SPRT, so a re-SPRT of
> the post-3.10 head vs the pre-3.5 base should recover most/all of that. Per the
> plan's profile-gated stance, **no speculative micro-opt** was done beyond this
> identified win (the 3.8 imbalance loop already skips empty piece types; reuse of
> the attack-map substrate was already banked in 3.0). The KRKm-scaling
> over-aggressiveness review remains a Phase-4 item. **Gates passed:** `bench 13 =
> 4,377,437` (identical), 9/9 CTest, texel `--verify` exact on all 8,598.

> **Recalibrated from sibling Rarog's result (measured 2026-06-23).** Rarog ran
> the identical Phase-3 build-out; its end-of-phase NPS SPRT vs the pre-eval head
> failed (**−32.6 ± 9.3 Elo, H0**, 2694 games at `3+0.03`) because `bench` NPS
> fell **−22 %** — the enlarged seeded-inert eval computes every node, and the
> bench *fingerprint* (node count) is blind to NPS, so it surfaced only at the
> gate. **But Rarog then tried to recover that NPS behaviour-identically and could
> not:** (a) hot-loop micro-opt — its loops were already compiler-optimised and a
> hand "collapse imbalance" *regressed* throughput (307→324 ns/eval); (b)
> inert-block gating recovered +15 % NPS but **only at the throwaway seeded-0
> head** — once Phase 4 tunes weights nonzero every block reactivates and the
> gating does nothing in the shipping engine, so it was reverted. **Lesson: the
> durable NPS lever is 3.11 lazy eval, not 3.10.** So 3.10 is profile-gated, not
> mandatory, and the inert-block-gating dead end is **not** to be repeated.
>
> **Where Basilisk may differ — do not copy Rarog's verdict blindly.** Basilisk is
> C++/Clang+PGO with a *different* eval body, and it genuinely *did* carry a
> redundant double-compute that Rarog had already removed — **Step 3.0 just deleted
> it** (mobility and king safety each used to compute every slider's attacks
> separately). So **profile before concluding "nothing to do":** if the profiler
> shows real redundant work (per-passer `is_attacked_by` probes, the trapped-bishop
> slider recompute, threats full-board sweeps, imbalance nested loops), removing it
> is a free win Rarog's Rust verdict does not rule out. What Rarog's result
> *forbids* is **speculative** micro-opt and the **inert-block-gating** scaffold —
> not profiler-proven cleanup.

Profile the eval (a sampling profiler / VTune on the pext build) and, **only where
the profiler shows genuine redundancy**, make the hot blocks cheaper
**without changing the result**:
- reuse the **Step 3.0 attack-map substrate** everywhere (threats, mobility,
  king-safety, hanging) instead of recomputing `is_attacked_by()` per consumer —
  this is the single biggest win, since 3.0 replaces per-square recomputation;
- collapse the **imbalance** nested loops (skip empty piece types, precompute the
  count vectors once);
- trim redundant full-board sweeps in **threats** and per-passer attack probes
  (mask first, probe survivors only);
- hoist invariant lookups out of the per-piece loops.
- **`apply_endgame()` early-out guard (required — identified by the Step-3.5
  SPRT, see §3.5).** `apply_endgame()` currently runs a full 12-popcount piece
  census on **every** node; in the opening/middlegame all of its rules no-op, so
  it is pure overhead — this is the dominant cost behind 3.5's −4.17-Elo fast-TC
  result. Add a cheap guard (e.g. skip the census/known-endgame block when total
  non-pawn material or piece count is clearly above any endgame pattern; keep the
  OCB path it previously covered) so it costs ~nothing in non-endgame positions.
  This is behaviour-identical (it only skips work that would no-op), so the gate
  is `bench 13` unchanged from the 3.5 fingerprint. *Also* sanity-review the
  "no-pawn ≤ minor → scale toward draw" rule here for over-aggressive KRKm
  flattening (a few KRKm are winnable).
**Gate (when any cleanup is applied):** `bench 13` **unchanged** (byte-identical
refactor) + the Step-3.x trace/symmetry tests still pass + a **measured NPS gain**
(best-of ≥5 `bench`, pext). Note: full parity with the `phase1-final` head is
**not** reachable here — the seeded-inert terms are pure overhead until Phase 4
activates them (see the Phase-3 gate note below). 3.10 only shaves genuine
redundancy; the rest is recovered by 3.11 lazy eval and, ultimately, by Phase 4
turning the eval cost into Elo.

### Step 3.11 - Lazy eval (behaviour-changing, SPRT-gated) — the durable NPS lever; strongly recommended before Phase 4 — Opus 4.8 high

> **DONE 2026-06-25 (single checkpoint; SPRT pending).** Added a lazy-eval
> checkpoint in `evaluate()` right before the Step-3.0 attack-map substrate: once
> the cheap part (material/PST + imbalance + pawns + dynamic passers + bishop pair
> + rook/knight bonuses) is computed, if `|tapered| > LAZY_MARGIN` (`= 700`,
> conservative start, hardcoded — tune under SPRT) it **skips the whole expensive
> attack-map-driven block** (mobility, king safety, threats, hanging, shelter,
> small terms, space, winnable) and finishes with `apply_endgame()` + 50-move
> damping. **`apply_endgame` runs on the lazy path**, so KBNK/KPK/draw-scaling
> survive the skip (the `test_endgames` suite passes with lazy on). The early
> return is `#ifndef TEXEL_TRACE`, so the tuner traces the full eval and `--verify`
> stays exact. **Single checkpoint only** — the optional second checkpoint (skip
> just threats/space/small-terms after the attack maps) is a later refinement.
> **Behaviour-changing → bench re-baselined `4,377,437 → 4,168,590`** (≈−4.8 %
> node count; lazy skips positional eval in decisive leaves). **Gates passed:**
> 9/9 CTest (incl. `test_endgames`), texel `--verify` exact on all 8,598.
> **SPRT result (2026-06-27, `tc=3+0.03`, gate `[-3,0]`): ACCEPTED H1 — `LazyOn`
> vs `LazyOff` (3.10 head `d4cc5cf`) = +16.64 ± 7.03 Elo** (nElo +23.23 ± 9.80),
> LOS 100%, LLR 2.96, 4,828 games. **A clear net gain, not just non-regression**
> (well above Rarog's +4.4): the deeper search from the recovered NPS far
> outweighs the coarser eval in decisive positions, and it more than recovers the
> Phase-3 per-node eval-cost tax (so 3.5's −4 fast-TC result is recovered too).
> **LAZY_MARGIN=700 kept**; lowering it (more aggressive skipping) is a possible
> Phase-5 SPSA refinement, not needed now. The real strength verdict vs
> `phase1-final` still belongs at the **Phase-4 boundary**. **✅ This closes the
> Phase-3 eval-structure build-out.**

The big lever. When the **tapered material + PST margin** (cheap, already
computed) is so large that no positional term could flip the bound, **return
early**, skipping the king-safety / threats / mobility / imbalance / small-terms
block (lazy eval). Material/PST are not seeded inert, so the margin is
meaningful today.

A standard multi-threshold lazy shape (present in SF's classical eval since the
SF11 era — *not* an SF16 addition) is worth using: **two lazy checkpoints.**
Checkpoint 1 after material/PST+pawns can skip the full heavy eval in decisive
positions; checkpoint 2 after attack maps, mobility, king safety, and passers
can skip only threats/space/small terms when the result is already clear. Tune
Basilisk's margins under SPRT; do not copy SF constants because eval scale and
term order differ.

> **Sibling-engine evidence (Rarog, ACCEPTED 2026-06-23).** Rarog's lazy eval
> (margin 600) was **net +4.42 ± 3.90 Elo** in a 15,314-game lazy-on-vs-off SPRT
> `[-3,0]` — *not just a free speedup*: the deeper search from +11.8 % NPS
> outweighed the coarser eval (and at seeded-0 the skipped terms contribute ~0).
> It re-baselined its bench fingerprint. Expect a similar shape here; the margin
> Basilisk wants may differ (different eval scale, C++ vs Rust), so tune it under
> SPRT rather than copying 600.

Constraints (Rarog's lessons; Basilisk-adapted):
- **Disable lazy eval on the Texel trace/`--verify` path** — the tuner must fit
  the *full* eval; the lazy early-return is compiled out (or guarded) under
  `TEXEL_TRACE`. Keep the eval a **pure function of the position** so any
  whole-eval cache stays exact (Basilisk's available extra remedy — it has only
  `pawn_table_` today — is a positional-eval cache keyed by the full hash; if
  added, **note Rarog's bug**: never cache a term that depends on state outside
  the cache key, e.g. occupancy-dependent passed-pawn stop bonuses under a
  pawn-only key).
- **Mating technique must survive the skip.** Extract the endgame mate-drive /
  mop-up (and any KBNK/KXK corner logic from 3.5) so it runs on **both** the lazy
  and full paths — Rarog's KBNK regressed until it did this; gate with the 3.5
  endgame EPD suite, not SPRT.
- **Conservative margin**, widened then tightened under SPRT (too tight → wrong
  skips cost Elo; too loose → little NPS back).
- Changes the played eval for clearly-decided positions → **`bench 13` moves**
  (re-baseline, documented).
**Gate:** a **non-regression SPRT** `[-3,0]` of the lazy head vs the pre-lazy
head, plus the lazy head recovering the bulk of the enlarged-eval NPS cost
(best-of ≥5 `bench`, pext). The **real** strength/NPS comparison vs `phase1-final`
is **not** run here (it cannot pass at the seeded-inert head — see the gate note
below); it belongs at the **Phase-4 boundary**, where the terms are live.
**Door open:** if SPRT ever shows lazy eval costs Elo in Basilisk that Rarog
didn't see, a **whole-eval cache** (the C++ remedy Basilisk lacks today) is an
alternative durable NPS lever — pursue that instead rather than forcing lazy eval.

**Phase 3 gate — eval-cost budget (whole enlarged eval).** The new terms are
seeded inert but their *structure still computes* (attack-map reads, threat /
passer loops run, then multiply by 0), so the full eval cost is paid at the
Phase 3 head. **Important correction (sibling Rarog, measured): an NPS SPRT of the
seeded-inert Phase-3 head vs `phase1-final` cannot pass** — the new terms are
*strictly* pure overhead until Phase 4 gives them nonzero weight and Elo, so the
seeded-0 head is by construction slower with no compensating strength. Do **not**
make "beat `phase1-final` on NPS at the Phase-3 head" a pass/fail gate; it is
unachievable by design (this is what cost Rarog a confusing −32.6 Elo gate
result). Instead:
- **measure** the enlarged-eval NPS cost (best-of ≥5 `bench`, native+pext) and
  record it — a large regression (Rarog saw −22 %) flags how much 3.11 must recover;
- recover the **durable** part with **3.11 lazy eval** (gated lazy-on vs lazy-off
  `[-3,0]`), optionally a **whole-eval cache** (a real C++ remedy Basilisk lacks
  today — it has only `pawn_table_`; Rarog already had an eval cache and learned to
  never key a term on state outside the cache key), and only **profiler-proven 3.10**
  cleanup (speculative micro-opt is out — see Step 3.10);
- defer the **real** NPS/strength comparison vs `phase1-final` to the **Phase-4
  boundary**, where the terms are live (Rarog banked **+240 real Elo** there, which
  dwarfs the transient −22 % NPS).

**Bottom line:** the Phase-3 *close* condition is bench-fingerprint identity per
step + `--verify` exact + CTest/EPD suites + a *non-regression* lazy-eval SPRT —
**not** an NPS race the seeded-0 head cannot win.

> **MEASURED (record, 2026-06-27).** Best-of-5 `bench` NPS, pext-PGO: Phase-3 head
> (lazy-on) **2,988,236** vs `phase1-final` **3,333,606** = **−10.4 %** — better
> than Rarog's −22 % (3.10's `apply_endgame` guard + 3.11 lazy eval recovered most
> of the seeded-inert cost). Not gated; the lazy-on-vs-off SPRT already netted
> **+16.6 Elo**, so the raw −10.4 % costs no Elo. The real strength verdict is the
> Phase-4 boundary.

**Phase 3 gate — trace/eval regression tests (per inert term).** For **every**
new structural term, add a small deterministic CTest/`--verify` fixture that
proves: (a) the Texel trace reconstruction equals the direct eval delta, (b) eval
**symmetry** still holds (mirroring a position negates the score), (c) a
seeded-zero param changes its trace count but **not** the eval or `bench 13`, and
(d) the feature's activation count is **nonzero** on a curated position that
should trigger it. This is the guardrail that stops Phase 4 from spending SPRT
games tuning a broken or never-firing trace — it is cheap to run and it is what
makes the Step 4.0 feature-support diagnostics trustworthy.

> **Status (2026-06-27): (a)(b)(c) MET, (d) folds into Step 4.0.** (a) `--verify`
> reconstructs exactly on all 8,598 holdout positions at every step. (b) eval
> symmetry is guarded by `test_eval`'s mirror test (passing). (c) bench identity
> per inert step proves "changes trace but not eval/bench". **(d) the per-term
> activation-count check is the Step-4.0 feature-support diagnostic itself** — it
> is built as Phase 4's opening task (it is *required* before any fit, to freeze/
> merge sparse params and confirm every traced term fires), which retroactively
> closes this item. Building it twice would be wasteful, so it is done once, in
> 4.0, against the complete Phase-3 trace.

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

> **Dataset sizing (decided 2026-06-27).** Start with **~2–3M** on-policy
> self-play positions from the current (post-Phase-3) head — Rarog ran its whole
> campaign on 2.19M and it was enough, and Basilisk's param count is the same
> order (PSTs 768 dominate; ~1,100 total). **Raw count is not the binding
> constraint — per-term support is.** PSTs/material saturate by ~1M; the risk is
> entirely in the **rare terms** (KS funnel, endgame/scale-factor, queen-pressure,
> unstoppable passer, restricted). So: generate ~2–3M, **run feature-support
> first**, and **targeted-top-up only the sparse buckets** (filtered datagen/sample
> pass) rather than blanket-scaling — cheaper and more effective. Regenerate fresh
> from the current head (the existing 1.73M `beast_seed` set is from a
> pre-Phase-2-final head). The big "more/better data" lever is deliberately the
> **Phase-7 data-refresh ratchet** (regen with the much-stronger post-Phase-4/5
> head); do not over-invest in dataset size now.
>
> **Mid-Phase-4 regen timing (decided 2026-06-27, on phone):** do **not** regen
> per stage. The small stages (KS, threats, mobility, pawn/small, imbalance) are
> robust on the existing `beast_seed` data (KS landed +65 on it). Regenerate
> **once, after Stage 4.6 and before Stage 4.7** (the ~778-param PST + material
> refit — the one stage where stale/noisy labels actually bite), so the fresh
> self-play comes from a head that already has KS+threats+mobility+pawn+imbalance
> banked. That one regen serves 4.7 **and** 4.8. (Datagen saturates the CPU and
> can't overlap an SPRT — run it in an idle/overnight window.)

| Item | Requirement | Why |
|---|---|---|
| Nonlinear king-safety support | **Build a finite-difference re-eval tuner path** (perturb weight → re-`evaluate()` the dataset → ΔMSE, integer coordinate descent with a shrinking step + shape clamps) for the knobs that feed the `attack_units → safety_table` funnel: `ks_unit`, coordination, open-file, no-queen scaling, and the `safety_table` curve shape. **Sibling-engine evidence (Rarog, 2026-06-24):** its identical funnel made all 11 king-danger inputs report **0 activations** in the linear trace — the gradient is structurally blind to them — yet fitting them via a re-eval `--tune-kingsafety` path was its **single biggest stage, +42.5 Elo**. Basilisk's KS is the same shape today (`eval.cpp`: "ks_unit/ks_coord_bonus/ks_open_file affect only the index"). **Door left open (do not over-restructure):** any *new* 3.2 sub-term Basilisk can express as a **direct linear** mg/eg contribution (not routed through the index) should be traced linearly so the existing gradient fitter handles it for free — only the genuinely index-funnelled knobs need the re-eval path. Actionable requirement: build the re-eval path before 4.3, and minimise what must go through it by tracing whatever 3.2 can express linearly. | The linear-trace fitter cannot learn knobs whose coefficient is zero-seeded or depends on the safety-table index. Phase 4.3 must not pretend to tune untunable params. |
| Feature-support diagnostics — **BUILT 2026-06-27 (`basilisk-texel --feature-support <dataset>`)** | Streams the dataset, counts per-flat-param nonzero linear-trace activations with an opening/mid/endgame phase split, flags zero- and sparse-activation traced params, and reports the finite-diff/frozen knobs as expected-zero. **Run on the full 1.7M `train_beast_seed` set: every linearly-traced term fires; the only 61 zero-activation traced params are all structurally impossible** (pawn PSTs ranks 1/8, passer/connected-rank 0/7, the `ImbTheir` diagonal — own-vs-enemy of the same type cancels, so those 6 imbalance coeffs are dead and should be frozen, the piece-type none/king slots `ThreatBy*[0]/[6]`, `HangPen[0/1/6]`), and the 33 KS/winnable funnel knobs are expected-zero. 20 rare-but-nonzero terms = freeze/L2 candidates. **This closes Phase-3 trace-regression item (d).** | Stops rare HCE terms (endgame, queen-pressure) from learning random signs or giant values off a handful of positions. |
| Bucketed holdout | Report loss by game phase (opening / mid / endgame), material class (no-queens / OCB / rook / pawn endings), king-attack, passed-pawn, and quiet-threat buckets — not just the aggregate. Reuses the Step 2.2 trace counts, so it is a reporting layer, not new instrumentation. | A global loss drop can hide a regression in the domains HCE strength depends on. |
| Targeted-data policy | If one bucket is sparse/regressing, append *targeted* quiet positions for that bucket only (a filtered datagen/sample pass); reserve global regeneration for aggregate stagnation. | Keeps data work cheap and avoids washing out rare critical positions. |
| Phase/domain-balanced sampling | Datagen/extraction enforces quotas (or sampling probabilities) for opening/mid/endgame, pawn endings, passers, quiet threats, and king attacks. | Waiting for self-play to *naturally* supply rare terms leaves the tuner underdetermined. |
| Blended labels | Optionally train on `alpha*result + (1-alpha)*score_target` (a search-score / WDL teacher target). Keep engine output pure HCE — teacher labels are training data only. | Result-only labels are noisy; blended targets smooth the gradient and shrink the dataset needed. |
| Binary feature cache | Add a versioned trace/feature cache (schema + params hash + bucket metadata + labels) so repeated staged fits don't rebuild traces. | Phase 4 reruns many fits; a cache makes them fast and reproducible. |
| Regularization / shape constraints | L2-to-prior beyond PSTs, monotonic/smooth passed-pawn and safety curves, sign constraints on obvious penalties, optional PST smoothing. | The broad 2.4b scalar pass already produced implausible signs; constraints make the fit robust instead of decorative. |

> **Step 4.0 — COMPLETE (2026-06-27).** All readiness items are built or
> resolved; the gate is clear to stage Phase 4.
> - **Nonlinear king-safety FD path — BUILT (`basilisk-texel --tune-kingsafety
>   <train> <holdout> [out] [--epochs N] [--max-positions N] [--step S]`).** A
>   compact position snapshot restored into one reused `Board` (avoids storing
>   millions of heavy Boards), `K` fit by golden-section on the holdout via real
>   `evaluate()`, then integer coordinate descent (shrinking step) over the 43 KS
>   funnel knobs (`ks_unit[2..5]`, coord, open-file, `ks_safe_check[2..5]`,
>   weak-ring, ring-pressure, flank attack/defense, pawnless-flank, king-blockers,
>   central-king, shelter-storm, no-queen num/den, `safety_table[2..24]`), with
>   `safety_table` held **non-decreasing** by a neighbour-clamp. Smoke run (60k):
>   holdout MSE 0.09691→0.09619, and it activated the dead funnel knobs sensibly
>   (`ks_safe_check` N/B/R/Q = 12/4/8/4, king-blockers +4, lifted table tail) —
>   exactly the Rarog-style result. Lazy eval is off under `TEXEL_TRACE`, so it
>   fits the full eval.
> - **Feature-support diagnostics — BUILT** (see row above; closes Phase-3 (d)).
> - **Bucketed holdout — BUILT.** `--tune` now prints holdout loss split by phase
>   (opening / mid / endgame) at the end; `TuneSet` carries the traced phase.
> - **Regularization / shape constraints — BUILT.** Added `--l2 <λ>` (L2-to-prior,
>   pull toward default weights; off by default) to `--tune`; the existing
>   `clamp_weights` sign/shape/monotonic clamps stay, and the FD path keeps
>   `safety_table` monotonic.
> - **Phase/domain-balanced sampling — BUILT.** `extract.py` now computes game
>   phase (faithful to engine `PHASE_W`), always prints the train phase mix, and
>   takes `--balance-phase R` to downsample over-represented phase buckets to `R×`
>   the smallest.
> - **Blended labels — capability in place (no code change).** The tuner's
>   `parse_target` already accepts any float in `[0,1]`, so a `fen;0.62` blended
>   target works as soon as datagen emits a WDL/search-score column.
> - **Binary feature cache — DEPRIORITIZED** (per sibling Rarog): trace-build is
>   seconds, so a versioned cache saves little; revisit only if reruns bottleneck.
> - **Targeted-data policy — process, not code:** run `--feature-support`, then
>   filtered datagen/`sample_fens` to top up only the sparse buckets.

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
| 4.1 | Material + any leftover structure-independent scalars (`misc`) | elo1=5 | **Skipped / fold into 4.7** — material was already SPRT-accepted (+29) in Phase 2; low value to redo standalone. |
| 4.2 | Threats package (3.1) | elo1=3 | **✅ DONE +79.1 Elo** (executed 2nd). Dropped the old flat `hang[]` term in the accepted step. |
| 4.3 | King safety v2 (3.2): `safety_table[25]`, `ks_unit`, new safe-check / weak-ring / flank / danger inputs | elo1=3 | **✅ DONE +65.5 Elo** — the biggest lever, **executed FIRST** (promoted), so it carries execution label "Stage 4.1" / binary `phase41-ks` (see the Stage-4.1 note below). Used the Step-4.0 `--tune-kingsafety` path. |
| 4.4 | Per-count mobility tables (3.3) | elo1=3 | **DONE — no clean win, nothing banked** (table fit + SF area refinement both investigated 2026-06-27/28; see note below). Phase-2 linear mobility already well-tuned. Re-try the area refinement with fresh data at the 4.7 boundary. |
| 4.5 | Pawn refinement (3.4) + space/winnable (3.6) + small positional terms (3.7) + `minors` | elo1=5 | **✅ DONE +57.2 ± 15.5 Elo** (H1, LOS 100%, 956 games, 2026-06-28). One joint group `phase45` (incl. 3.9 survey adds + rooks — the other seeded-0 positional terms), **L2-to-prior from the start** (λ=1e-6: holdout −0.000608, survives regularization). 45 small sensibly-signed deltas; `--verify` exact, 9/9 CTest, startpos +52cp. New tooling `bake.py`. Head = `phase45-positional`. |
| 4.6 | Material imbalance (3.8) | elo1=3 | **✅ DONE +26.9 ± 8.4 Elo** (H1, LOS 100%, 3218 games, 2026-06-28). Linear quadratic fit, L2 λ=1e-6 (holdout −0.000344, survives regularization). The **6 structurally-dead `imb_their` diagonal coeffs** (i==j → `their_cnt ≡ 0`, t∈{0,2,5,9,14,20}) excluded from the fit and confirmed 0. 3 array members moved (max coeff 8); `--verify` exact, 9/9 CTest, startpos +48cp. Head = `phase46-imbalance`. |
| 4.6b | **Mobility-area refinement re-try (on fresh data)** | folded into 4.7 SPRT | **DONE 2026-06-28 — sane on fresh data (unlike 4.4).** Re-applied the SF mobility area in `eval.cpp` (exclude own K/Q, blocked/low-rank pawns, pinned `blockers_for_king` moved before the sweep; include own minor/rook squares) + monotonic mobility clamp in the tuner; re-fit on the 3.35M v17 set. **Startpos depth-12 = +32cp** (4.4 had +131 → failed; fresh diverse labels calibrated the magnitude, exactly as predicted). Holdout gain marginal (area −0.00003, refit +noise; tables moved only ±1). `--verify` exact, 9/9 CTest. Kept as the (more-correct, SF-matching) model; **validated folded into the 4.7 combined SPRT vs `phase46`** rather than spending a separate SPRT on a ~0 Elo change. Standalone binary `phase46b-mobarea` exists to isolate if 4.7 fails. |
| 4.7 | PSTs + material definitive refit (~778 params) | elo1=5 | **✅ DONE +6.45 ± 4.60 Elo** (H1, LOS 99.7%, 10402 games, 2026-06-28; combined with 4.6b vs `phase46`). Refit all 782 PST+material params on the fresh 3.35M v17 set, L2=1e-6 toward the PeSTO seeds (holdout −0.000149, converged epoch 9). Conservative: material barely moved (pawn 85→83, rest ±1), PSTs adjusted 4/6 tables each (42 params). `--verify` exact (dead pawn-rank squares stayed 0), startpos +50cp, 9/9 CTest. Baked via `bake.py --allow-pst`. Head = `phase47-pst`. |
| 4.8 | Global polish — everything unfrozen, low lr | elo1=3 | **✅ DONE +33.25 ± 9.40 Elo** (H1, LOS 100%, 2610 games, 2026-06-28). Joint refit of all 1179 params on v17 (lr 0.15, L2=1e-6; holdout −0.000695, all three phases improved). Far bigger than a typical polish — the 4.5 positional terms were fit on the *old* beast_seed and had real headroom on fresh data. Head = `phase48-polish`. Next: 4.8a prune, then the 2000-game validation vs `phase1-final`. |
| 4.8a | **Dead-feature prune** | bench-identical removal (no SPRT) | **✅ DONE 2026-06-29.** The 4.8 fit on fresh v17 confirmed all flagged terms stayed **exactly 0** even on endgame-heavy data, so they were removed: `pass_supp` (×3), `cand` (mg+eg), `pawn_lever` (mg+eg), `blockader_knight_eg`, `bishop_outpost` (mg+eg), `weak_queen` (mg+eg), `unstoppable_passer_eg`, `space_behind_mg` — **14 params / 8 features** across `eval.cpp` + `EvalParams.h` (struct + `EVAL_PARAM_LIST`) + `tuner.cpp` (groups + clamps). Behaviour-identical: **bench 3,764,539 unchanged**, `--verify` 10000/10000 exact (trace shrank consistently), 9/9 CTest, no orphaned vars. **Confirmed by simplification SPRT `[-5,0]`: +2.49 ± 4.19 Elo, H1 accepted, 11294 games** (not a regression; hair-positive NPS edge). KS re-fit (the other audit flag) deferred to Phase 7 (v17 endgame-heavy → wrong data). |

> **Stage 4.1 — KING SAFETY first (promoted; SPRT PENDING 2026-06-27).** Per the
> "biggest lever first" principle and Rarog's order (and because it exercises the
> new `--tune-kingsafety` path), king safety is run as the first fit instead of the
> low-value material re-confirm. Fit on the existing 1.7M `beast_seed` data:
> holdout MSE **0.09870 → 0.09780 (−0.0009)**, converged in 72 passes. The
> finite-difference path activated the linear-trace-blind funnel knobs sensibly —
> `ks_safe_check` N/B/R/Q = **7/6/8/6**, `ks_king_blockers` **4**, `ks_unit` reshaped
> to {N4,B0,R1,Q0}, `ks_open_file` 2→0, and a sharper monotonic `safety_table`
> (tail 147→**296**). `ks_weak_ring`/`ring_pressure`/`flank`/`pawnless`/`central`/
> `shelter_storm` stayed 0 (no measured gain). **Baked into `EvalParams.h`**
> (bench fingerprint 4,168,590 → **4,123,914**; 9/9 CTest). Candidate
> `tools/test_engines/basilisk-phase41-ks-pext-pgo.exe`. **SPRT ✅ ACCEPTED H1
> (2026-06-27, `tc=3+0.03`, `[0,3]`): +65.48 ± 13.58 Elo** (nElo +86.42 ± 17.50),
> LOS 100%, LLR 2.96, 1514 games — *bigger than Rarog's +42.5* from the equivalent
> stage. The KS-v2 structure + the `--tune-kingsafety` path are validated; **new
> accepted head = `phase41-ks`, bench `4,123,914`.** Next: Stage 4.2 (threats).
> *(Existing data adequate for KS; fresh ~2–3M regen recommended for later stages.)*

> **Stage 4.2 — THREATS (SPRT PENDING 2026-06-27).** Extended the tuner `threats`
> group to the new package (`ThreatBy*`, `ThreatHanging`, `WeakQueenProt`,
> `Restricted`, `ThreatPush`) + old flat pawn-threats + `hang_pen`, then linear
> `--tune` on the 1.7M data from the KS-tuned base: holdout MSE **0.09780 →
> 0.09681 (−0.001)**, all phase buckets improved, 40/51 params changed. The new
> threats-package terms activated (`ThreatByKing` 20/40, `ThreatHanging` 22/25,
> `WeakQueenProt` 16/7, `ThreatPush` 18/14, per-type `ThreatByMinor/Rook` tables),
> and **`hang_pen` was driven {22,39,33,36}→0 — the old flat hanging term dropped,
> absorbed by the package, exactly as specified.** Old flat pawn-threats clamped to
> 58/25. **Baked** (bench `4,123,914 → 3,929,330`; 9/9 CTest). Candidate
> `tools/test_engines/basilisk-phase42-threats-pext-pgo.exe`. **SPRT ✅ ACCEPTED H1
> (2026-06-27, `[0,3]`): +79.13 ± 14.82 Elo** (nElo +105.88), LOS 100%, LLR 2.94,
> 1264 games — bigger than Rarog's +45.2. **New head = `phase42-threats`, bench
> `3,929,330`.** Phase-4 cumulative so far ≈ **+144** (KS +65, threats +79).
> Next: Stage 4.4 (mobility).

> **Stage 4.4 — mobility table fit: NO GAIN on this data (2026-06-27, skipped).**
> Linear `--tune mobility` (per-count tables) from the threats base: the
> unregularized fit improved holdout −0.0005 but **over-valued mobility — the
> baked candidate failed CTest** (`startpos depth-5 |score| < 100`, and bench
> dropped 17%) because the sparse high-mobility table entries overfit. Re-fitting
> with L2-to-prior (`--l2` 1e-6 … 2e-5) showed the improvement was **almost
> entirely overfitting**: at λ=1e-6 the holdout gain collapses to −0.00002. The
> per-count tables have ~no true headroom over the Phase-2-tuned **linear** seed
> on this data, so the fit was **not baked**.
>
> **Mobility-area refinement attempted too (2026-06-28) — also no clean win, so
> Stage 4.4 is DONE with nothing banked.** Implemented the SF area in `eval.cpp`
> (exclude own K/Q, blocked/back-rank pawns, and pinned `blockers_for_king`;
> include own minor/rook squares — `blockers_for_king` moved before the sweep) and
> re-fit the tables on the new area with a **monotonic non-decreasing** clamp:
> holdout improved **−0.00064** (real headroom — the seed was miscalibrated for the
> larger-count area), **but the eval over-valued mobility** (startpos depth-5 =
> **+131 cp**, failing the CTest sanity bound); L2-to-prior collapsed the gain back
> to the seed (no λ sweet spot — gain and over-valuation are coupled when mobility
> is fit in isolation); the area change with seed tables sat startpos at exactly
> 100 (borderline) with noise-level holdout. **Reverted both.** Conclusion:
> **Phase-2 linear mobility is already well-tuned; mobility has no calibrated,
> sanity-passing win on 4.2-head data.** The SF-area refinement is structurally
> correct, so the re-try is **scheduled as its own step, Stage 4.6b**, on the
> freshly-regenerated ~2-3M dataset (the 4.7 boundary) — fresh on-policy labels
> may calibrate the magnitude. If 4.6b over-values again, it is dropped
> permanently. *(Discarded candidate: `phase44-mobility`.)*

**Stage-order & magnitude calibration (sibling Rarog, 2026-06-24 — priors, not
mandates).** Rarog ran the same campaign and banked, in order, KS **+42.5**,
threats **+45.2**, mobility **+24.1**, remaining scalars **+85.2**, imbalance
**+26.7**, material+PST **+27.6**, polish **+65.0** → ≈**+316 self-play / +240
real Elo** — well above this plan's deliberately-conservative **+80–160**
estimate. Discount for Basilisk: it already SPRT-tuned more scalars in Phase 2
than Rarog had pre-Phase-4 (material +29, mobility, passers, pawn-structure), so
part of Rarog's scalar/material gain is **pre-banked** here — but KS, threats, and
per-count mobility are still untouched, and those three alone were ~+112 in Rarog.
Two takeaways, **neither forced**:
- **King safety is the single biggest lever** and the one stage needing the new
  re-eval tuner. Consider promoting it ahead of the cheap material re-confirm
  (4.1) so the riskiest fit is de-risked early and its Elo banks sooner. Counter-
  argument to weigh: doing material first stabilises the centipawn scale the KS
  danger curve is fit against — if you keep 4.1 first, keep it *tiny* (material
  only) so it doesn't delay KS.
- **The 4.1 material re-confirm is low-value for Basilisk specifically** (material
  was already SPRT-accepted at +29 in Phase 2). It may be **merged into 4.7**
  rather than run as a standalone stage. Keep PST + material **last** regardless.

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

> **Eval data-refresh ratchet → promoted to its own Phase 7 (§7.5).** The
> post-Phase-5 multi-cycle eval-refresh grind (regen self-play with the stronger
> head, joint refit, repeat 1–3 cycles) used to live here as "Step 4.9"; it runs
> *after* Phase 5 (and Phase 6), so its numbering now matches its order — see
> **§7.5 Phase 7**. At the **end of Phase 4** the only forward-looking decision is
> the standard boundary gate (LTC confirm + gauntlet vs `phase1-final`); the
> refresh decision is made later, at the Phase 7 entry, gated on that gauntlet.

---

## 7. Phase 5 - Time Management hardening + tuning (EXECUTE BEFORE the Phase 6 search wave)

> **✅ PHASE 5 COMPLETE (2026-07-01).** Shippable content = **5.4 clock-at-`go`**
> (dispatch-latency fix, validated `+2.95 ± 6.74` non-regression vs 1.7.0). 5.5
> skipped / 5.6 dropped / 5.7 deferred (no real overshoot or forfeit to fix — the
> LB losses were GUI pipe latency). **5.8 SPSA was baked then REVERTED**: the 5.9
> gain SPRT was a wash (`+0.88 ± 4.03` over 12,262 games, LLR flat in `[0,3]`),
> confirming the TM was already at its ceiling — the "+8–25 Elo" target below did
> **not** materialise (the TM was already sound, not untuned). Net Phase-5 result:
> a correctness fix, no tuning gain. Bundled into the **1.8.0** dev line (ships
> with the Phase 6 search wave; see user_dev_guide.md §5.3–5.9 for detail).

> **Promoted to Phase 5 (2026-06-29).** Steps 6.1/6.1b/6.2 below (the Rarog-port
> budget formula + reserve + the 2026-06-20 LB validation) are **DONE** and stay
> as the foundation. But LB **still shows time-losses** post-Phase-3/4 (heavier
> eval per node) and the TM constants have **never been tuned for Basilisk** — so
> this phase is reopened with new steps **5.3–5.9** and executed before the search
> wave (now Phase 6). Goal: *generally* strong across the whole TC spectrum
> (bullet → slow), **not** a bullet or long-game specialist. Realistic target:
> reclaim the LB forfeits (reliability) **+ `+8`–`+25` Elo** from the first-ever
> TM-constant SPSA. The legacy `5.x` numbering in the §5 search section is
> unrelated history.

Goal: better budgets at increment time controls. The default SPRT now exercises
clock mode (`tc=3+0.03`), so the formula work is refinement, not repair —
**but the robustness work (5.3–5.6) IS repair** (the LB forfeits are real).

> **Step 6.1 was pulled forward to 2026-06-20**, ahead of Phases 3-5, because
> matching Rarog's proven Phase 2.2 + 2.9.1 fix required the full logT-based
> rewrite anyway — patching just the reserve onto Basilisk's old simpler
> formula (the original Step 2.9.1 patch) left the formula itself more
> aggressive than Rarog's at fast TCs. Step 6.2's gating SPRTs and the
> LittleBlitzer validation below are what's left before 1.6.0 can ship this.

### Step 6.1 - Budget formula upgrade — DONE (2026-06-20), ported from Rarog

Implemented as a **direct port of Rarog's Phase 2.2 logarithmic-time-left rewrite**
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

> **Step 6.3 superseded by Step 5.8 below.** The "likely skip the SPSA" call was
> made when LB showed `t=0` and Basilisk's eval was light. Both changed:
> Phase-3/4 made each node heavier (overshoot returned) and we now *want* the
> TM-constant SPSA as a deliberate Elo lever, not a contingency.

---

### TM HARDENING + TUNING (new, 2026-06-29) — reopened Phase 5

> **Root-cause analysis (2026-06-29, current `phase48-polish` head).** Three
> code-level defects + one untapped lever, in priority order:
> 1. **Clock starts late.** `start_time_` is set inside the worker
>    (`search.cpp` `Searcher::search`, after `cmdGo`→queue→dequeue→`init_lmr`).
>    The GUI charges wall-time from when it *sent* `go`; that dispatch/setup
>    latency is invisible to `elapsed_seconds()` → systematic undercount →
>    overshoot. Laggy GUIs (LittleBlitzer) expose it; fastchess masks it. **Most
>    likely the dominant LB-forfeit cause.**
> 2. **Coarse poll granularity.** `check_stop()` runs every 2048 nodes
>    (`search.cpp:982`, `:1131`). Phase-3/4 raised per-node eval cost, so a
>    2048-node batch is wall-time-longer than when this was validated at 1.5.1 →
>    larger overshoot past `hard_limit_`, worst at bullet where the whole budget
>    is tens of ms.
> 3. **Thin, static safety margin.** `reserve = 2*overhead`, default Move
>    Overhead `10 ms`; never re-checked after the eval got heavier, and too thin
>    for high-latency GUIs.
> 4. **Zero tuning.** Every constant (optConst/maxConst, the `0.06` stability
>    step, the `30 cp`/`÷100` score-drop, the `80/25`→`0.80/1.20` effort scales,
>    `0.8097`, the reserve factor) is a hardcoded SF/Rarog value — none fitted to
>    Basilisk. The biggest TM Elo lever, completely unexploited.
>
> Design principle: **generally strong, not a TC specialist.** Every change is
> validated at bullet (`1+0.01`), blitz (`3+0.03`), rapid (`10+0.1`) and a slow
> control (`60+0.6`); a change that helps one TC but regresses another is
> rejected.

#### Step 5.3 - Diagnose & instrument the recurring LB time-loss — Opus 4.8 medium

> **INSTRUMENTATION DONE 2026-06-29 (measurement pending).** `TM_Debug` UCI check
> (default off) advertised **only in tune/dev builds** (`#ifdef BASILISK_TUNE`) so
> harnesses/GUIs actually send the setoption — fastchess/LB skip *unadvertised*
> options (first attempt logged `Warning; doesn't have option TM_Debug`, zero
> data); release builds stay 9 options. `go`-receipt
> timestamp captured in `UciProtocol::cmdGo`, threaded `EngineCommand.recv_time`
> → `SearchLimits.go_recv_time`; `Searcher::search` emits one
> `info string tm soft_ms=.. hard_ms=.. elapsed_ms=.. dispatch_ms=..` per move on
> the reporting thread. Timing UNCHANGED (start_time_ still in-worker; go_recv
> only read for the line). Gate met: bench 3,764,539, 9/9 CTest, 9 options,
> debug-off identical. Diagnostic binary `basilisk-phase53-tmdebug-pext-pgo.exe`.
>
> **MEASURED 2026-06-29 (DONE).** 1+0.01 fastchess, 12,471 moves (`-log
> engine=true`), plus the LB forfeit log. **The engine's TM is sound:** overshoot
> negligible (14 moves over `hard` by ≤ 1 ms — poll rounding), allocation correct
> (`elapsed < hard`). Only harness-independent flaw: **dispatch ≤ 20 ms**
> (`go`→clock-start under bullet+concurrency) → 5.4 reclaims it. **LB forfeits are
> GUI/pipe latency, not a budget bug:** LB charged 959 ms for a move the engine
> spent 112 ms on (`dispatch_ms=0`) — ~847 ms in the LB↔engine pipe; fastchess
> (efficient I/O) never forfeits. **⇒ 5.5 unnecessary (no overshoot); LB is not a
> TM-budget repair; real levers are 5.4 (small, harness-independent) + 5.7 + 5.8.**

Make the forfeit measurable before changing anything. Add a **debug-only**
(`info string` behind a hidden `TM_Debug` UCI check, default off — so it's
bench-/play-identical when off) line per move logging: allocated `soft`/`hard`,
actual `elapsed` at bestmove, and `go`-receipt-to-`start_time_` delta. Reproduce
the forfeit in a controlled harness: `gauntlet.ps1` at `tc=1+0.01` and `0.3+0`
with a few hundred games, plus an LB short run, reading only `t=`. Confirm which
of defects 1–3 dominates (expected: #1 latency, then #2 at bullet). **Gate:**
debug-off path bench-identical (`4,168,590`/current), 9/9 CTest; deliverable is a
short findings note in this section. Output drives 5.4/5.5 sizing.

#### Step 5.4 - Start the clock at `go`-receipt (latency fix) — Opus 4.8 medium

> **DONE + VALIDATED 2026-06-29.** `Searcher::search` sets `start_time_ =
> limits.go_recv_time` (else `now()`). Bench 3,764,539, 9/9 CTest. Non-regression
> SPRT `[-3,0]` at 3+0.03 (`phase5tm` vs `v1.7.0`): **+2.95 ± 6.74 Elo** at 4,468
> games (LLR 0.66) — stopped early as a confirmed non-regression (a low-stakes
> safety change measuring positive). Kept.


Capture a `steady_clock` timestamp **in `UciProtocol::cmdGo`** (the moment the
`go` line is parsed) and thread it through `EngineCommand` → `SearchLimits` →
`Searcher`, using it as `start_time_` instead of the in-worker `now()`. Falls
back to in-worker `now()` if absent (e.g. bench, internal calls). This makes
`elapsed_seconds()` count the dispatch + `init_lmr` + thread-handoff latency the
GUI already charges. **Gate:** bench-identical (timing only, no search change);
5.3 harness shows the `go`→`start` delta now folded in and the bullet/LB `t=`
count drops. SPRT non-regression `[-3,0]` at `3+0.03` (must not cost Elo at clean
TCs). Concurrency-aware (timestamp is read-only, shared safely).

#### Step 5.5 - Anti-overshoot poll granularity — Sonnet 4.6 medium

> **LIKELY SKIP (5.3 data, 2026-06-29).** The premise — heavier eval per node
> overshooting the tiny bullet budget — did not materialize: measured overshoot
> is **≤ 1 ms over 12,471 bullet moves**, so the fixed `(nodes_ & 2047)` poll is
> already fine. Keep this step only as a contingency if a future change shows
> real overshoot. Do not implement now.

Replace the fixed `(nodes_ & 2047)` poll with an interval that **tightens as the
budget shrinks**: e.g. poll mask = `min(2047, max(255, hard_limit_nodes_est >> k))`,
or simpler, poll every `N` nodes where `N` is derived from `hard_limit_` and the
running NPS so one batch can never exceed a small fraction (~1–2%) of the budget.
Keep the cheap power-of-two mask on the hot path. Optionally add a coarse
wall-clock short-circuit so a single pathological node can't blow the deadline.
**Gate:** bench-identical; 5.3 harness shows bullet overshoot bounded; SPRT
non-regression `[-3,0]` at `3+0.03` **and** a `1+0.01` `t=`-only gauntlet.

#### Step 5.6 - GUI-robust reserve + Move Overhead default — Sonnet 4.6 medium

> **DROPPED (5.3 data, 2026-06-29).** No forfeits in robust harnesses (fastchess
> 1+0.01, 12k moves, 0 losses); the LB forfeit is ~847 ms of GUI-pipe latency a
> reserve cannot sanely absorb. Current `2*overhead` reserve is adequate.
> Revisit only if a real (non-LB) forfeit appears.


With 5.4/5.5 in, re-fit the safety margin: make the reserve `max(2*overhead,
abs_floor_ms)` (small absolute floor, e.g. 15–25 ms, covers fixed GUI latency
that doesn't scale with overhead), and reconsider the **default Move Overhead**
(10 ms → likely 20–30 ms) for out-of-the-box robustness on laggy GUIs. Trade a
hair of Elo at clean TCs for zero forfeits anywhere. **Gate:** full LB pool leg
re-run at `tc=3+0.03` **and** a `1+0.01` LB/gauntlet leg → `t=0` required; SPRT
non-regression `[-3,0]` at `3+0.03`. This is the step that *closes* the LB issue.

#### Step 5.7 - Root fail-low / instability time extension — Opus 4.8 medium

> **DEFERRED (2026-06-29).** The adaptive-stop block already does instability
> extension (best-move stability / score-drop / effort scaling), and 5.8 made
> those SPSA-tunable. A *mid-iteration* fail-low extension is a behaviour-changing
> search heuristic needing an SPRT to land — out of scope for the games-less
> autonomous pass. Revisit after the 5.8 SPSA or fold into the Phase-6 search wave.


Extend the soft limit when the position is unstable mid-search, not just between
iterations: if the **root best move fails low** (score crashing through the
aspiration window) or the PV is still changing late, grant extra time up to
`hard_limit_`. Complements the existing score-drop extension (which only fires
*between* completed iterations). **Gate:** SPRT `elo1=3` at `3+0.03` + a `10+0.1`
confirmation (this is a genuine strength lever, not just robustness).

#### Step 5.8 - Expose TM constants under `BASILISK_TUNE` + SPSA — Sonnet 4.6 medium

> **EXPOSURE DONE 2026-06-29 (SPSA pending — the maintainer's compute).** 9
> SPSA-tunable knobs added to `SearchParams` (defaults == baked, behaviour-
> identical): `TmOptMult`/`TmMaxMult` (budget ×100), `TmStability` (×1000),
> `TmScoreDropThr`/`Div`, `TmEffortHi`/`Lo`/`HiMult`/`LoMult`. `compute_time_limit`
> applies the multipliers; the iteration-stop block reads the rest. Guarded by
> `#ifdef BASILISK_TUNE` (release stays 9 options — verified), parsed in
> `Parameters.cpp`, wired into `setup_spsa.ps1` as group `tm`
> (`tools/spsa_configs/config_tm.json`). bench 3,764,539, 9/9 CTest. SPSA binary
> `basilisk-phase5tm-pext-pgo.exe`. Run `setup_spsa.ps1 -ConfigGroup tm` at
> `tc=10+0.1`, re-validate at `1+0.01`/`60+0.6`, bake, SPRT vs `phase5tm`.


The Elo lever. Expose the TM constants as a `ConfigGroup tm` (UCI spin options
under `BASILISK_TUNE`, like the existing search-constant groups): optConst,
maxConst and their slopes, the `0.8097` max factor, the stability step (`0.06`),
score-drop threshold/scale (`30`, `÷100`), effort thresholds/scales
(`80/25`,`0.80/1.20`), and the 5.7 extension knobs. **SPSA at `tc=10+0.1`** (the
clock condition that exercises the full budget), then **re-validate the tuned set
at `1+0.01` and `60+0.6`** to guarantee it didn't over-fit one TC. Bake; SPRT the
tuned set vs the pre-SPSA head at `3+0.03` and `10+0.1`. This is the first time
Basilisk's TM is fitted to *its own* eval/search rather than inheriting SF's.

#### Step 5.9 - Cross-TC validation + ship gate — Sonnet 4.6 medium

Final confirmation that the phase delivered "generally strong, no specialist":
gauntlet (or LB) at **`1+0.01`, `3+0.03`, `10+0.1`, `60+0.6`** vs a fixed field,
each requiring `t=0` and a non-negative aggregate vs the pre-Phase-5 head. Record
the per-TC Elo so any specialization is visible. Then Phase 5 ships (folds into
the next release alongside the Phase-6 search wave, or its own point release if
the LB fix alone warrants shipping sooner).

---

## 7.5 Phase 7 - Non-NNUE ceiling: eval-refresh multi-cycle grind (EXECUTE AFTER Phases 5–6; optional, evidence-driven) — Sonnet 4.6 medium (driving)

> **Ported from sibling Rarog's Phase 6 (2026-06-24), customized for Basilisk and
> kept as priors — Rarog is a different engine (Rust); verify against Basilisk's
> own gauntlet before spending a cycle.** This is the **post-search-wave HCE
> maturity phase**: the *multi-cycle tuning grind* that pushes a complete
> hand-crafted eval toward its ceiling without a net. It was previously buried as
> "Step 4.9" inside Phase 4; it runs *after* Phase 5 (and Phase 6), so it is now
> numbered to match its order. **It is optional and evidence-driven:** enter it
> only if the end-of-Phase-5 gauntlet shows the eval still has transfer headroom,
> and stop the moment a cycle stops paying.

Goal: bank the **tuning-maturity** Elo that a stronger, more on-policy dataset
unlocks — the data-fit bootstrap ratchet — plus a couple of cheap structural
ride-alongs that the same refit fits for free. Realistic target across the phase:
**+10–40 Elo** on the first cycle, diminishing after.

### Step 7.0 - Non-NNUE ceiling analysis (read first; it sets the stop point)

**Question:** how far can HCE Basilisk go without a net, and what is the highest-
value lever at this point? This sharpens the §0 ceiling note.

- **Reference points (CCRL facts, engine-agnostic).** The proof a hand-crafted
  eval can reach the high-3000s is **Stockfish 11 (~3440 CCRL)** — the strongest
  classical eval ever shipped — plus Ethereal (pre-NNUE), Komodo classical,
  Xiphos, Texel. **Caution:** Berserk/RubiChess/Stash are often quoted at 3300+ as
  "strong HCE," but those are their **NNUE** versions; their *classical* builds
  were ~3000–3150. **Do not size HCE targets off NNUE-era ratings.**
- **What this means.** For a *complete* SF11-class HCE, the gap to the top of the
  field is **search-depth/selectivity + tuning maturity, not missing eval
  features** — SF11 reached 3440 with essentially this feature list. So the ranked
  non-NNUE levers are: (1) the **Phase 5 search wave** (highest confidence,
  eval-scale-independent — it stays *before* this phase and is invalidated by
  nothing); (2) **this data-refresh ratchet** (Steps 7.1–7.2); (3) the
  **shelter/storm→danger** fold (best single new *eval* bet — built as structure
  in Phase 3.2, it lands its Elo on a refit here if Phase 4.3 left it weak);
  (4) the deferred low-yield Phase-3.7 terms.
- **Basilisk caveat (do NOT inherit Rarog's "features are done" framing yet).**
  Unlike Rarog, Basilisk is **not yet** a complete SF11-class HCE at the time of
  writing — Phase 3 is still building that structure. The "only tuning + search
  remain" framing applies **only after Phase 4 closes**. Until then the Phase 3→4
  build-out is itself a first-order lever; this phase is what comes *after* it.
- **King-bucketed PSTs = the NNUE gateway, DEFER.** `PST[piece][square][king_bucket]`
  is the strongest *structural* HCE expansion left, but it is literally the HalfKA
  NNUE input shape — SF11 did **not** use it, and committing to king-bucketed
  inputs + a fitting pipeline is ~80% of the work of a small net for a fraction of
  the strength. If Basilisk ever reaches for it, **do NNUE (Phase 9) instead.** It
  marks the boundary between this phase / Phase 8 (HCE menu) and Phase 9 (NNUE).

### Step 7.1 - Eval data-refresh, cycle 1 (the first refit on fresh data) — Sonnet 4.6 medium

Basilisk's Texel dataset (`tools/texel/data/*beast_seed*.csv`) was self-played by
a **pre-Phase-4 (indeed pre-Phase-2-final) head.** Once Phase 4 (eval fit) and
Phase 5 (search wave) make Basilisk ~+150–250 Elo stronger, regenerating self-play
and re-fitting **once** is a real, well-precedented lever (Ethereal/Texel got
strong this way).

- **Why it helps:** a stronger head gives (1) cleaner WDL labels — a weaker engine
  blunders won positions into draws/losses, mislabeling the Texel target — and
  (2) more on-policy, better-distributed positions. Better labels → a tighter fit
  on the *same* (now-activated) terms.
- **It is NOT a re-stage.** The Phase-4 staging (4.1–4.8) existed to *activate*
  seeded-inert terms and isolate per-group Elo (a one-time job). On fresh data, do
  **one joint low-lr refit** of the whole activated eval (like Step 4.8) **plus the
  king-safety `--tune-kingsafety` re-eval path** (built in Step 4.0), then **one
  SPRT** vs the head. Concretely, reuse the Phase-2/3 pipeline:
  `tools/datagen.ps1` (regen with the new head) → `tools/texel/extract.py` →
  `basilisk-texel --tune all` + `--tune-kingsafety` → bake → `sprt.ps1`.
  Cost: datagen + a minutes-long fit + one SPRT.
- **Exploit the dormant Step-4.0 capabilities on the regen:** turn on **blended
  labels** (`α·result + (1−α)·score_target`) and **phase-balanced sampling**
  (`extract.py` quota pass) — both directly attack label noise, exactly what a
  second iteration should target.
- **Ride-along structure (fit for free in this same refit):** the
  **shelter/storm → danger-funnel** fold (Step 3.2) is the prime candidate — if
  Phase 4.3 left the linear shelter/storm fit looking weak, this refresh is where
  the routed-into-danger version earns its Elo. Optionally activate the deferred
  low-yield Phase-3.7/3.9 trio (bishop x-ray on pawns, rook+queen battery,
  slider-on-queen) — seed inert, let the refit decide; include only if cheap.
- **Gate:** one SPRT vs the head at `tc=3+0.03` (`elo1=3`), then a boundary
  gauntlet at `tc=10+0.1` (eval refits over-fit self-play — the gauntlet is the
  real verdict). **Expected ~+10–40 Elo** for cycle 1.

### Step 7.2 - Iterate (cycles 2–3) and the stop condition — Sonnet 4.6 medium

The ratchet repeats: each cycle is *exactly* Step 7.1 again — a fresh self-play
regen with the now-stronger head, one joint low-lr refit (+ the KS re-eval path),
one SPRT, one boundary gauntlet. **Strong HCE evals do 1–3 such cycles;** the curve
flattens fast.

**Stop when (any one):** a cycle yields `< ~+8 Elo` by SPRT, holdout loss stops
dropping between regens, or the gauntlet shows no movement vs the field. Past that
point the only remaining classical lever is king-bucketed PSTs (Step 7.0) — which
is the NNUE input shape, so the door turns toward **Phase 9 (NNUE)**, not more HCE
data. The Phase 8 feature menu is a parallel option for any not-yet-built small
terms, but the data-refresh curve flattening is the signal that pure-HCE
*tuning* is spent.

**Honest bottom line (Rarog calibration; Basilisk's own number TBD by gauntlet).**
Rarog measured Phase 5 (+20–50) + one refresh cycle (+10–40) landing it around
3040–3110 CCRL — likely past a fair share of the field but *short of* a ~3187
Critter on the first pass, with further cycles closing more. Matching the very top
on pure HCE is *possible* (SF11 proves the ceiling is far higher) but is a
**multi-cycle grind, not one feature.** Decisively beating the top is NNUE
territory (Phase 9) — which is why the eval boundary (gate 7 in §1) is preserved
throughout, with king-bucketed inputs as the natural bridge to it.

### Release

This phase **banks SPRT + gauntlet-validated Elo**, so each accepted cycle is
releasable (a **minor** bump if it lands real gauntlet Elo, a patch if marginal) —
see the cadence table in §10. Don't sit on a validated cycle waiting for the next.

---

## 8. Phase 8 - Remaining Feature Menu (only after the eval + search phases plateau)

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

## 9. Phase 9 - NNUE (Future Project, the actual path to Stockfish parity)

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

- **MAJOR (`X.0.0`)** — architecture swap. Reserved for **NNUE (Phase 9)** → `2.0.0`.
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
| **after each Phase 7 cycle** | eval-refresh grind, per accepted cycle (**+10–40** cycle 1, diminishing) | **minor** if the cycle lands real gauntlet Elo, else patch | per-cycle SPRT + boundary gauntlet @ `tc=10+0.1` |
| Phase 8 (menu) | feature-menu items, batched | patch per small batch; minor if a batch lands large Elo | per-feature SPRT |
| **Phase 9** | NNUE | **`2.0.0`** | full revalidation |

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
SF; beyond it, NNUE (Phase 9) is the only lever.

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
with modern Stockfish is Phase 9's NNUE job, and every earlier phase makes that
switch cheaper and better tested.
