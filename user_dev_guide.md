# Basilisk Development Workflow Guide

The human-side companion to `PLAN.md`. That file holds the process detail,
history, and the active plan; this one is the quick reference for day-to-day
work: what state we're in, what to run, and what to paste back.

---

## Current Checkpoint

**1.8.0 RELEASED 2026-07-08** — bundles Phases 5+6+7 (~+93 Elo over 1.7.0 at
`3+0.03`, **~+40 at LTC** per the colosseum gauntlet). The post-release
self-play cycle 6 **washed** (+1.37 ± 5.21 over 8,100 games, LLR flat) — the
HCE tuning line is **closed**; 1.8.0 captured all of it. Dev head = 1.8.0,
bench **12,661,251**.

**▶ BRANCH STRUCTURE (2026-07-13):** `master`/`development` = the HCE line
(finalization concluded 2026-07-13, engine content stays 1.8.0; **Phase 8 —
verified correctness bugs + release/CI hardening** — is queued there). This
`nnue` branch carries all Phase-9 work; its board prerequisite is
**Phase 8.5** (PLAN §4.2: StateInfo/cached geometry + accumulator hooks), to
run **after rebasing onto the Phase-8 head** (the SEE/draw fixes there change
bench).

**▶ NEXT = Phase 9: NNUE** (PLAN §4; release will be **2.0.0**, +200–400
expected). Deferred/reopenable experiments (post-NNUE SPSA etc.) are in
PLAN §5 — nothing else HCE-side is worth games.

| Version | Content | Validated |
|---|---|---|
| 1.5.0 | Phase 0–1: harness + search-constant SPSA | +27 vs 1.4.9 |
| 1.6.0 | Phase 2–2.9: eval scalar fits + TM robustness | +54 vs 1.5.0 |
| 1.7.0 | Phase 3–4: eval structure + staged Texel campaign | +280.74 vs phase1-final |
| **1.8.0** | Phase 5–7: TM fix + TT-bound eval + SF-distill + 5 self-play cycles | **+93 fast / ~+40 LTC** vs 1.7.0 |
| 2.0.0 (next) | **Phase 9: NNUE** | target +200–400 |

---

## Phase Progress Tracker

- [x] **Phase 0 — Harness:** fastchess/SPRT/SPSA tooling, books, build scripts; calibration verified.
- [x] **Phase 1 — Search constants:** pruning + LMR SPSA, +27 validated → **1.5.0**.
- [x] **Phase 2 (+2.9) — Texel infra + cheap scalar fits:** tuner/datagen pipeline + material/mobility/passers/pawn fits (+54) and the time-forfeit fix → **1.6.0**.
- [x] **Phase 3 — Eval structure build-out:** attack maps, threats, KS-v2 danger funnel, per-count mobility, endgame scaling, lazy eval — all bench-identical, tuned later.
- [x] **Phase 4 — Eval data-fit campaign:** staged Texel fits over the full structure, +280.74 vs phase1-final → **1.7.0**.
- [x] **Phase 5 — Time management:** clock-at-`go` fix (+2.95); TM SPSA washed — at ceiling.
- [x] **Phase 6 — Search efficiency wave:** TT-bound pruning eval +7.18 (bundle +9.14); the rest of the wave shipped as inert knobs for a post-NNUE SPSA.
- [x] **Phase 7 — Eval refresh (HCE endgame):** SF@60k distillation +6.75, then five on-policy self-play cycles (+21/+19.5/+18.3/+15.3); LTC gauntlet validated ~+40 → **1.8.0**. Cycle 6 washed → **HCE closed**.
- [ ] **Phase 8 — Correctness & infra hardening: runs on `development`, not here** (development's PLAN §4). Rebase this branch onto its head when it lands.
- [ ] **Phase 8.5 — NNUE-ready board architecture (THIS branch; PLAN §4.2), after the rebase:**
  - [ ] **8.5.1 StateInfo + cached check geometry** — pins/checkers/checkSquares once per node, reused across generation stages; `gives_check` into `make_move`. Node-count + NPS gated; SPRT if behaviour shifts.
  - [ ] **8.5.2 Layout cleanups** — EP legality without the Board copy, 16-bit Move, history restructure.
  - [ ] **8.5.3 Eager accumulator updates in make/unmake** — chess768 = no refresh machinery; validated by the randomized property test. Feeds directly into 9.3's performance layer.
- [ ] **Phase 9 — NNUE (ACTIVE; PLAN §4; ships as 2.0.0):**
  - [~] **9.1 Data at scale — ▶ IN PROGRESS 2026-07-09; label decision MADE: blended score+result.** Discovery: our fastchess PGNs already carry per-move search scores (`{+0.28/6 ...}`), so blended labels are free — no annotation pass. New extractor (now `tools/extract_nnue.py` in the shared `D:/code/net_trainer` repo; parallel, quiet-filtered, dedup) emits `FEN | cp(white-POV) | result(white-POV)` bullet-convertible text; verified on 200k games (1.9M unique in 80s; sign check: +100cp → 84.6% white score). Bootstrap from the five Phase-7 PGNs (~1M games) + fresh 2M-opening book (`beast_seed_2m.epd`, seed 777) generated. **Remaining: the big datagen run** (~2M games from the 1.8.0 head, ~8h background) → extract → 30M+ unique target.
  - [ ] **9.2 Trainer integration — ✅ trainer BUILT 2026-07-12 in the shared `D:/code/net_trainer` repo** (PyTorch `net_trainer.nnue`: 768→(H×2)→1 perspective SCReLU, blended-label training, quantized **`.mnn`** export + parity verifier — the cross-engine format all five engines consume, spec in its `docs/mnn_format.md`). Remaining: the full-size training run once 9.1 data lands.
  - [~] **9.3 Inference core — ▶ bring-up layer DONE 2026-07-13, performance layer remaining.** Shipped: `src/nnue.{h,cpp}` .mnn loader + full-recompute eval (conformance-exact vs the net_trainer reference — the vectors caught a real ±1 floored-vs-truncated division mismatch, now pinned in the contract), `Evaluator::evaluate` dispatch, UCI `UseNNUE` (default false) + `EvalFile` (runtime load / `<embedded>`), CMake `-DBASILISK_NNUE_FILE=net.mnn` bake-in, `tests/test_nnue` (24/24, in CTest → suite now 10/10). **Provably inert by default: bench 12,661,251 unchanged.** Remaining: incremental accumulators in `make/unmake` + AVX2 (full recompute would cost ~90% NPS — see PLAN §4.1). *(Fable 5 high / Opus 4.8 high.)*
  - [ ] **9.4 Swap-in + SPRT** — net eval behind a compile flag, HCE kept as fallback during bring-up; SPRT vs the 1.8.0 head at `3+0.03`, confirm at `10+0.1`; iterate data/net size until it clearly passes both.
  - [ ] **9.5 Search re-tune at the new eval scale** — the ONE deferred search-constant SPSA (histshape + wave2 mechanisms + TM knobs, PLAN §5), now legitimate because the eval scale is final; SPRT-gated.
  - [ ] **9.6 Release 2.0.0** — LTC field gauntlet vs the calibrated slate (head-to-head vs 1.8.0 is the number), full release checklist (PLAN §6).

---

## The Basic Rhythm

Most work is a short ping-pong:

```text
You   -> "Implement next step of the plan."
Model -> Reads PLAN.md, inspects current state, implements, verifies locally
         (build + bench fingerprint + 9/9 CTest), commits on a candidate
         branch, and tells you exactly what to run.
You   -> Run the command and paste the short result.
Model -> Acts on the result: merge + document, or revert + document.
```

For SPSA and SPRT the model cannot honestly guess the result — your pasted
report is the decision input. `development` always stays at the last validated
head until a verdict; the model never tags or pushes (that's how releases are
triggered, and it's yours).

---

## Common Commands

```powershell
# Build a named pext-PGO+TUNE candidate into tools\test_engines\
.\tools\build_test.ps1 -Suffix mystep-name

# SPRT a gain candidate (defaults: elo1=3, tc=3+0.03, Hash 64, 1 thread)
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-candidate-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-baseline-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -Elo1 3

# LTC confirmation                       # Non-regression variant
.\tools\sprt.ps1 ... -TC "10+0.1"        # ... -Elo0 -3 -Elo1 0

# Self-play datagen — ALWAYS the diverse book (default book collapses to dupes)
.\tools\datagen.ps1 -Suffix <head> -Book tools\texel\data\beast_seed.epd `
    -BookFormat epd -Rounds 100000 -Nodes 8000 -OutputPgn tools\texel\data\selfplay_X.pgn

# Boundary gauntlet: colosseum (D:\code\colosseum, you drive the GUI) or:
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <prior-release>,<field...> -TC "10+0.1"
```

The full Texel-fit recipe (sequential joint-bake) and the SPSA setup live in
PLAN §7; the model runs those itself and only hands you the game-playing steps.

---

## What To Report Back

- **SPRT:** paste the final block (`Elo: X +/- Y`, `LLR`, `Games`, `Ptnml`,
  H0/H1 verdict). If it's grinding flat near LLR 0 for thousands of games,
  paste the interim block — a CI entirely below the threshold is already a
  verdict.
- **SPSA:** the final parameter values + iteration count (state saves to
  `tools/weather-factory/tuner/state.json`).
- **Gauntlet:** the PGN path — the model parses the cross-table itself. Trust
  the head-to-head vs the prior release, not absolute Elo estimates.
- **Errors:** engine name + the incident log path (colosseum:
  `%APPDATA%\colosseum\data\logs\incidents\`). Throttled-Stockfish crashes in
  lost positions are known-benign.

---

## Releases

The model prepares everything on request ("release X.Y.Z"): version bump in
`src/Constants.h` + `CMakeLists.txt`, `CHANGELOG.md`, clean-build verification,
copy-pasteable GitHub release notes. **You** squash `development` →
`Version X.Y.Z` on `master`, push, and `git tag vX.Y.Z` (the tag triggers the
CI release build). Full checklist: PLAN §6.

---

## Ground Rules

- No tuned value set is accepted without SPRT; no strength claim without games.
- Holdout-MSE movement (or its absence) is not a verdict in either direction.
- Never bundle feature work with tuning defaults; one candidate at a time.
- Bench node counts are fingerprints, not strength.
- Fixed-game LTC gauntlet at every release boundary — fast-TC eval gains
  compress (~half survived at LTC in Phase 7).
- Keep `Evaluator::evaluate(const Board&)` as the search↔eval boundary — it is
  the NNUE socket.

The process is the strength engine here: tune, test, keep only what survives.
