# Basilisk Strength Improvement Plan

> **CURRENT STATE (2026-07-09):** **1.8.0 RELEASED 2026-07-08** (bundles Phases
> 5+6+7: ~+93 Elo over 1.7.0 at `3+0.03`, **~+40 at `10+0.1` LTC**, colosseum
> field gauntlet). **The HCE line is CLOSED:** the post-release self-play cycle 6
> washed (**+1.37 ± 5.21, LLR 0.10 flat at 8,100 games** — the entire CI below
> the pre-registered ≥+8 stop threshold; candidate discarded). The Step-7.0
> ceiling analysis' residual estimate (+25–35 LTC) proved optimistic — the
> per-cycle taper (+21.0 → +19.5 → +18.3 → +15.3) **cliffed to ~+1** at cycle 6;
> the actual residual was ≈ 0 and 1.8.0 caught all of it. Dev head = 1.8.0,
> bench **12,661,251**.
>
> **▶ NEXT = Phase 9: NNUE (§4).** No further HCE tuning phases. Deferred /
> reopenable experiments live in §5.

This plan was executed 2026-05 → 2026-07 as Phases 0–7. The step-by-step
history lives in `CHANGELOG.md` and git history; this document keeps the
**process** (how work happens, §1), the **record** (what shipped, §2–3), the
**active phase** (NNUE, §4), the **deferred menu** (§5), and the **operational
discipline** (releases §6, commands §7).

---

## 1. The development process

### The working rhythm (user ↔ model)

```text
User  -> "Implement the next step of the plan."
Model -> Reads PLAN.md, inspects current state, implements, verifies locally
         (build, bench fingerprint, 9/9 CTest), commits on a candidate branch,
         and hands the user exactly one command to run (usually an SPRT).
User  -> Runs the long compute (SPRT / SPSA / gauntlet / datagen) and pastes
         the result. The model cannot honestly guess these results — the
         user's report is the decision input.
Model -> Acts on the verdict: merge + document, or revert + document. Then
         proposes the next step.
```

Division of labour, fixed by convention:
- **The model implements, verifies, documents, and prepares releases.** It
  never guesses game results, never tags, never pushes — `git tag` and
  `git push` are exclusively manual user actions (tagging triggers the GitHub
  release workflow).
- **The user runs all long compute** and makes scope decisions (skip a step,
  stop a line, cut a release).
- **Candidate branch discipline:** every SPRT candidate is committed on its own
  branch (`phaseXY-name`); `development` stays at the last validated head until
  the SPRT verdict. Accept → fast-forward merge + delete branch. Reject → the
  branch is deleted and the work is recorded as a null result.
- **After every step:** update `PLAN.md` + `user_dev_guide.md`, commit
  **without** a co-author trailer.
- **Releases:** user squashes `development` onto `master` as a single
  `Version X.Y.Z` commit, then tags manually. See §6.

### Non-negotiable gates (apply to every change)

1. **One candidate at a time** — never bundle unrelated feature work with tuned
   values.
2. **SPSA/Texel propose; SPRT decides.** Lower holdout loss that does not
   survive SPRT is overfitting; revert it. (Phase 7 proved the converse too:
   a *stalled* holdout MSE does not mean no Elo — the games are the only
   arbiter in both directions.)
3. **SPRT every kept strength change.** Standard gate `elo0=0, elo1=3` at
   `tc=3+0.03`, Hash 64, Threads 1, `SuperGM_4mvs.pgn` book (this matches the
   SPSA TC so optima transfer). `tc=10+0.1` for phase-boundary / LTC-suspect
   confirmation. Don't react to sub-3k-game trends — early SPRT noise has
   flipped sign more than once.
4. **PGO `pext` binaries for all local strength testing** (`build_test.ps1`).
5. **`bench` fingerprint discipline:** behaviour-identical refactors must match
   the recorded fingerprint exactly; behaviour changes record the new
   fingerprint but it is never interpreted as Elo.
6. **CTest canaries:** the KBNK/KQK mate-resolution tests (`test_endgames`,
   `test_search`) are fragile canaries for search-constant changes. The proven
   pattern: implement + expose a new mechanism at a **provably inert default**
   (mathematical argument — e.g. a `>0` gate at 0, a cap above `MAX_PLY`), pass
   9/9 CTest, and defer real values to SPSA. **Never hand-pick a
   canary-passing constant** (canary results are non-monotonic in the knobs).
7. **Preserve the eval boundary** — search calls
   `Evaluator::evaluate(const Board&)` and nothing deeper. This is what makes
   the NNUE swap (Phase 9) possible.
8. **Release UCI stays clean** — tuning knobs exist only behind `-DTUNE=ON`;
   releases run compiled-in defaults (accepted values are baked into headers,
   never shipped as config).

### The cost principle (§0.5, kept because it still governs §5)

- **Texel weight-fitting is CPU-minutes — run it freely.** SPSA and SPRT are
  **thousands of games each — conserved.**
- **Search constants are denominated in eval centipawns.** Any eval re-fit
  changes what a centipawn means, so a search-constant SPSA run before the
  eval is final is thrown away. This is why the search SPSA was deferred
  through Phase 7 — and why it is now a **post-NNUE** item (§5): NNUE re-scales
  the eval wholesale.
- New eval structure always enters as a behaviour-identical refactor (new
  terms seeded inert/equivalent, bench fingerprint unchanged, `--verify`
  exact, no games), and gets its values from data fits, not by hand.

### Model recommendations

Driving (implement, verify, document, releases): **Sonnet 5 medium**; log
reading / doc upkeep: Sonnet 5 low. Dense or interaction-risky engine work
(NNUE core, tuner-core changes, pruning-interaction ports): **Fable 5 high**
(alt: **Opus 4.8 high**); self-contained formula/infra ports and data-pipeline
work: Fable 5 / Opus 4.8 medium.

---

## 2. Version history (what shipped, with validated numbers)

| Version | Date | Content | Validated strength |
|---|---|---|---|
| 1.4.9 | pre-plan | baseline before this plan | — |
| **1.5.0** | 2026-06 | Phase 0 harness + Phase 1 search-constant SPSA (pruning +18.9, LMR +15.6) | **+27** vs 1.4.9 |
| **1.6.0** | 2026-06-20 | Phase 2 Texel infra + cheap eval scalar fits; Phase 2.9 robustness (SF-style TM port, time-forfeit fix) | **+54** vs 1.5.0 |
| **1.7.0** | 2026-06-29 | Phase 3 eval structure build-out (attack maps, threats pkg, KS-v2 danger funnel, per-count mobility, pawn refine, endgame scaling/KPK/KBNK, imbalance, lazy eval +16.6) + Phase 4 staged Texel campaign (KS **+65.5**, threats **+79.1**, positional **+57.2**, imbalance **+26.9**, mobility-area + PST/material **+6.5**, joint polish **+33.3**) | **+280.74** vs phase1-final |
| **1.8.0** | 2026-07-08 | Phase 5 TM (clock-at-`go`, +2.95; TM SPSA washed → reverted) + Phase 6 search wave (6.1 TT-bound pruning eval **+7.18**; 6.2 cont-hist6 rejected −7.70; 6.3–6.8 exposed-inert knob set; bundle 6.10 **+9.14**) + Phase 7 eval refresh (7.1 SF@60k distillation **+6.75**; on-policy self-play cycles **+21.02 / +19.51 / +18.29 / +15.32**) | **≈ +93 fast-TC / ~+40 LTC** vs 1.7.0 (colosseum 10+0.1 gauntlet) |
| *(post-1.8.0)* | 2026-07-09 | self-play cycle 6: **wash** (+1.37 ± 5.21, 8.1k games) → **HCE line closed**, candidate discarded | — |
| **2.0.0** | future | **Phase 9 NNUE** (§4) | target **+200…+400** |

Current dev head: 1.8.0, bench **12,661,251** (fixed-depth 40-position
harness; TM-independent).

Internal-gauntlet placement of 1.8.0 (10+0.1, 2,819 games): ≈ Rarog 2.2.0 /
Shredder 12 level; below Rybka 3/4, HIARCS 14, Critter 1.6a, SF-limited-2900+.
Basilisk holds no official CCRL rating — the internal gauntlet is the yardstick.

---

## 3. Durable lessons (measured on this project — read before proposing work)

1. **On-policy self-play refit was the biggest post-campaign lever** (~+15–21
   per ~2h cycle, five times over) — **but ~half of fast-TC eval gains compress
   at LTC** (+93 → ~+40). Size releases on the LTC number.
2. **The lever exhausts as a cliff, not a taper.** Cycle gains went
   21 → 19.5 → 18.3 → 15.3 → **+1.4 (wash)**. Do not extrapolate taper curves;
   run the next cycle and let the SPRT decide.
3. **Holdout-MSE-delta does not predict Elo in either direction** — cycles with
   near-zero linear-eval movement gained +15–20 (the KS `safety_table` funnel
   kept re-shaping); the distillation with the biggest MSE drop gained least.
4. **Datagen requires the diverse book** (`tools/texel/data/beast_seed.epd`,
   100k Beast openings, `-BookFormat epd`). Identical engines at fixed nodes
   are deterministic: the default 2.7k-opening book collapsed 200k games to
   31,880 unique positions; the diverse book yields ~1M+.
5. **Sequential joint-bake:** `--tune all` and `--tune-kingsafety` each write
   FULL dumps, so baking one after the other reverts the first. Correct order:
   bake `--tune all` → **rebuild `basilisk-texel`** → re-fit KS against the
   baked eval → bake KS. KS finite-diff: `--max-positions 100000` (~8 min).
   Always fit with `--l2 1e-6` (anchors the cp scale; material stays pinned).
6. **SPSA at maturity: 0-for-2** (Phase 5.8 TM wash; sibling Rarog's 30h
   negative). Only never-tuned constants ever paid (Phase 1). Never seed an
   SPSA start point with mechanisms whose standalone SPRT was negative.
7. **Benchmark Stockfish, not the sibling.** Rarog is a same-family peer that
   Basilisk passed — its 6-ply continuation history *lost* −7.70 on Basilisk.
   Design references come from the strongest engines; SPRT decides the local
   value.
8. **Gauntlets: trust only the within-gauntlet head-to-head** (candidate vs
   prior release under identical conditions). Absolute "estimated Elo" is
   anchored on stored opponent ratings and can read −100 while the engine
   gained +40. Strength-limited Stockfish crashes/plays illegal moves in
   already-lost positions — benign, but adjudication (or a real engine at that
   level) avoids the noise.
9. **The eval is feature-complete pre-NNUE** (2026-07-01 audit vs SF-classical /
   Ethereal / Weiss: nothing missing worth adding). King-bucketed PSTs are the
   NNUE input shape — ~80% of a net's work for a fraction of its gain — so the
   next structural step *is* the net.

---

## 4. Phase 9 — NNUE (THE ACTIVE PHASE; release = 2.0.0)

The ceiling-breaker: **+200–400 Elo** above any HCE. Everything before it was
preparation, and the preparation is done:

- **The eval boundary held** — `Evaluator::evaluate(const Board&)` is the
  single search↔eval interface; an NNUE accumulator hooks into
  `make_move`/`unmake_move` beside the existing incremental keys (`Board`
  already maintains pawn/minor/nonpawn keys).
- **The data pipeline exists and is battle-tested** (this was the expensive
  part): `datagen.ps1` produces 200k diverse games in ~45 min
  (`beast_seed.epd` seeding); `extract_parallel.py` quiet-filters (skips
  in-check + capture/promotion plies) and phase-balances; Hydra's
  `annotate_sf.py` provides SF-teacher labels at scale if wanted; the 1.8.0
  HCE itself is a strong data generator (~+400 over the engine that generated
  the first Texel sets).
- **The gates carry over unchanged:** SPRT harness, bench discipline, CTest
  canaries, the gauntlet slate, the release checklist.

Proven small-engine path (unchanged from the original plan):
**768 → (256×2) → 1 perspective net with SCReLU**, trained on self-play data
from the tuned HCE (tens of millions of positions — scale the existing datagen
up), using an external trainer (e.g. `bullet`). Quantized int16/int8 inference
with incremental accumulator updates; the HCE remains compiled-in as the
fallback/verification eval during bring-up.

Execution outline (each gate = SPRT vs the 1.8.0 head, then gauntlet):
1. **Data at scale** — scale datagen to 30–60M quiet positions from the 1.8.0
   head. **▶ IN PROGRESS 2026-07-09; labels DECIDED: blended score+result.**
   The fastchess PGNs already carry per-move search scores in comments, so both
   labels are free — `tools/texel/extract_nnue.py` (parallel, quiet-filtered,
   deduped) emits `FEN | cp(white-POV) | result(white-POV)`; sign convention
   verified (+100cp → 84.6% white score on 1.9M positions). The training blend
   λ is chosen at trainer time (9.2). Bootstrap: the five Phase-7 self-play
   PGNs (~1M games). Scale-up: fresh `beast_seed_2m.epd` (2M openings, seed
   777) → one big datagen from the 1.8.0 head (~2M games, ~8h).
2. **Trainer integration** — external trainer, export to a compact binary net
   format embedded in the binary (no runtime file dependency for releases).
3. **Inference core** — accumulator + affine layers, `make/unmake` hooks,
   perft/bench-verified incremental correctness; a `--verify`-style
   full-recompute cross-check in debug.
4. **Swap-in + SPRT** — net eval behind a compile flag first; SPRT vs HCE
   head; keep iterating data/net size until it clearly passes at LTC too.
5. **Search re-tune at the new eval scale** — this is where the deferred
   search-constant SPSA (§5) finally runs once, legitimately.
6. **Release 2.0.0** after the full §6 gate.

Model: **Fable 5 high (alt: Opus 4.8 high)** for the inference core and
trainer integration; Sonnet 5 medium driving.

---

## 5. Deferred / reopenable experiments (replaces the old Phase-8 menu)

The old Phase-8 "feature menu" was closed by the 2026-07-01 audit (EV ≈ 0 — no
missing pre-NNUE features; the list survives in git history). What remains
here is only what was **skipped, not rejected** — things with a real, if
small, chance of paying — plus the conditions under which they make sense.
Rejected-with-measurement items (6.2 cont-hist6 −7.70, capture-futility-active
−2.78, do_deeper margins, TM SPSA values) stay closed.

**The unifying rule (§1 cost principle): all search-constant tuning happens
ONCE, at the FINAL eval scale — which now means after NNUE (§4 step 5).**
Running any of these SPSAs against the 1.8.0 HCE would be spent games the net
invalidates.

| Deferred item | What it is | Est. value | When to run |
|---|---|---|---|
| **histshape SPSA** (was 7.4) | `config_histshape.json` — 11 always-live dims: history bonus/malus shape (quad/lin/max ×2), `HistTtMoveBonus`, `LmrHistDiv`, `HistPruneCoeff`, `LmrCutNodeAdj`, `LmrTtCapture`; starts from current defaults | ~+2–4 | fold into the post-NNUE search re-tune |
| **wave2 mechanisms** (6.4/6.5/6.8 exposures, inert) | `CapFutDepth`(~7) + margins, `QuietSeeDepth`(~8) + coeff, `QsearchCheckCap`(~6), `PostLmrHistScale`, `DoubleExtMax`, fractional-LMR `*_adj` dims | unknown; standalone attempts washed or broke canaries at HCE scale | post-NNUE only, and **fix the start point** — never seed with the known-negative values (`config_wave2.json` kept as a template, not a start) |
| **TM knobs** (9 `Tm*`, exposed) | time-management constants; 5.8 SPSA washed → at ceiling for the current search | ~0 now | only if the search's node economics change materially (NNUE) |
| **6.6 fail-low prior countermove bonus** | skipped on weak (SF-only) evidence during Phase 6 | small | as a one-off SPRT experiment post-NNUE, low priority |
| **fractional history** | quantization experiment flagged during 6.7 | likely wash | only alongside the post-NNUE history re-tune |
| **SPSA discipline** (pre-registered, from the 2026-07-02 EV review) | 1000–1500 iters, abort at ~600 if trend ≤ 0, converged candidate → bake → SPRT + full CTest; a wash → keep defaults, done | — | applies to every run above |

Explicitly **not** reopenable: more HCE self-play cycles (cycle 6 proved the
lever exhausted at the 1.8.0 head), king-bucketed PSTs (do the net instead),
the audit-closed feature menu.

---

## 6. Release discipline

### Versioning (SemVer-adapted; no public API, so it maps to strength)

- **MAJOR** — architecture swap: NNUE ships as **2.0.0**.
- **MINOR** — a phase/campaign banking SPRT + gauntlet-validated strength
  (1.5.0 → 1.8.0 were all this). The version reflects **cumulative content
  since the last tag**, not the latest step.
- **PATCH** — robustness/bugfix only, no strength claim.
- Releases stay rare and curated; don't ship bench-identical work, don't sit
  on validated Elo.

### Pre-tag gate

1. All content SPRT-accepted at `3+0.03`.
2. **LTC field gauntlet** at `10+0.1` — eval changes over-fit self-play and
   fast TC, so the gauntlet is the honest sizing. Tooling: **colosseum**
   (`D:\code\colosseum`, the user's GUI tournament manager — user drives it) or
   `tools/gauntlet.ps1`; optional LittleBlitzer cross-check. Calibrated slate
   (all in `D:\chess\engines`): prior Basilisk release (the head-to-head that
   matters), Fruit 2.1, Rarog 2.2.0, Rybka 3, Critter 1.6a, SF
   `UCI_LimitStrength` 2800/2900/3000, HIARCS 14, Shredder 12. Read the
   **head-to-head vs the prior release**; treat absolute estimates loosely.
3. Scan for illegal moves / forfeits / crashes (`t=`, `i=` counts ≈ 0 for
   Basilisk; throttled-SF incidents in lost positions are benign).
4. For a CCRL-style estimate: anchor Ordo to a published engine
   (`ordo -a <ccrl> -A "<name>"`), slower TC, treat as ±50–100.

### Release checklist (the model runs this when asked to "release X.Y.Z")

1. Confirm the gate above passed.
2. Bump the version in **both** places: `src/Constants.h` (`engineVersion`) and
   `CMakeLists.txt` (`project(basilisk VERSION …)` — drives the dist tag).
3. Update `CHANGELOG.md` (Keep-a-Changelog entry with SPRT + gauntlet numbers,
   honest fast-TC vs LTC framing) and `README.md` only if the feature list
   changed.
4. Verify the release build: no `BASILISK_TUNE` UCI options exposed, `bench`
   runs, 9/9 CTest.
5. Commit the prep on `development`. **Do not tag. Do not push.**
6. Produce copy-pasteable GitHub release notes (summary, strength vs prior tag,
   changes, honest caveats).
7. **User:** squash → `Version X.Y.Z` on `master`, push, `git tag vX.Y.Z`
   (tag push triggers `.github/workflows/release.yml`, which builds the
   platform asset matrix — CI assets are non-PGO by design; PGO is for local
   testing).

### Compute budget

Ryzen 9 5950X, shared. Texel fits = CPU-minutes, run freely. Datagen/SPSA at
concurrency ~24; SPRT/gauntlets via repo script defaults. NPS is not a
bottleneck (~2.7M nps single-thread; PEXT + LTO + PGO local builds) — profile
before any micro-optimisation.

---

## 7. Quick commands

```powershell
# One-time fresh-clone setup
.\tools\setup_tools.ps1

# Build a named pext-PGO+TUNE candidate into tools\test_engines\
.\tools\build_test.ps1 -Suffix mystep-name

# SPRT a gain candidate (defaults: elo1=3, tc=3+0.03, Hash 64, 1 thread)
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-candidate-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-baseline-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -Elo1 3
# Non-regression variant: -Elo0 -3 -Elo1 0 ; LTC confirmation: -TC "10+0.1"

# Self-play datagen (ALWAYS the diverse book — see §3 lesson 4)
.\tools\datagen.ps1 -Suffix <head> -Book tools\texel\data\beast_seed.epd `
    -BookFormat epd -Rounds 100000 -Nodes 8000 -OutputPgn tools\texel\data\selfplay_X.pgn

# Extract + phase-balance
python tools\texel\extract_parallel.py tools\texel\data\selfplay_X.pgn `
    --train train_X.csv --holdout holdout_X.csv --balance-phase 1.5

# Texel fit (sequential joint-bake — see §3 lesson 5)
cmake --build build\texel-verify --target basilisk-texel
.\build\texel-verify\basilisk-texel.exe --tune all tools\texel\data\train_X.csv `
    tools\texel\data\holdout_X.csv out_all.txt --l2 1e-6 --epochs 100
python tools\texel\bake.py out_all.txt --allow-pst
cmake --build build\texel-verify --target basilisk-texel   # rebuild before KS
.\build\texel-verify\basilisk-texel.exe --tune-kingsafety tools\texel\data\train_X.csv `
    tools\texel\data\holdout_X.csv out_ks.txt --max-positions 100000
python tools\texel\bake.py out_ks.txt

# SPSA (post-NNUE only — §5)
.\tools\setup_spsa.ps1 -ConfigGroup histshape -EngineSuffix <base> -Iterations 1500
cd tools\weather-factory && python main.py

# Fixed-game boundary gauntlet (or use colosseum)
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <prior-release>,<field...> -TC "10+0.1"
```

---

## 8. Bottom line

Phases 0–7 took Basilisk from 1.4.9 to 1.8.0 — roughly **+400 SPRT-validated
Elo** — by tuning search constants once, building the full SF11-class eval
structure bench-identically, fitting it in one staged campaign, hardening time
management, adding the one search feature that survived (TT-bound pruning
eval), and then riding the on-policy self-play refit loop until it measurably
exhausted. The HCE is done: feature-complete by audit, data-saturated by
measurement. **Everything from here is Phase 9 — the net.** The harness that
banked the first +400 is the same harness that gates the next +300.
