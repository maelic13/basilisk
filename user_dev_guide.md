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
verified correctness bugs + release/CI hardening** from the **three**
2026-07-13 audits — is queued there, now including the search audit's qsearch
in-check/fail-soft fixes, the canary policy split, **and the HCE audit's
three verified eval bugs** (OCB scaling that *amplifies* the eval, an
enemy-rook-behind-passer penalty that almost never fires, missing two-pawn
double attacks) **plus rule-50-damping and mate-drive SPRT experiments** —
its steps run 8.1–8.9). This `nnue` branch carries all Phase-9 work; its
prerequisite is **Phase 8.5** (PLAN §4.2): Track A = StateInfo/cached
geometry + accumulator hooks, **Track B = the pre-NNUE search ladder**
(verified search audit: correction-history gating, qsearch ordering/TT
store, TT-PV bit, check-extension experiment, history coverage, root-move
state, upcoming-repetition — one SPRT each), **Track C = the frozen teacher
benchmark + data-pipeline hygiene** (HCE audit — no games, but must exist
before the first full-size net trains). Run Tracks A/B **after rebasing onto
the Phase-8 head** (the SEE/draw/qsearch/eval fixes there change bench);
Track C can run any time.

**▶ NEXT = Phase 9: NNUE** (PLAN §4; release will be **2.0.0**, +200–400
expected). After the net: **Phase 10 (search architecture v2)**, conditional
**Phase 11 (SMP)**, and **Phase 12 (eval v2)** — PLAN §5. Phase 12 opens
with the **12.0 eval-direction decision**: from the 9.4 SPRT + the 8.5.12
benchmark we decide *on evidence* whether to iterate the net's architecture
(king buckets → material buckets → threat inputs) or reopen HCE feature work
(the audit's king-safety/winnability/endgame ideas wait there, closed by
default). Nothing else HCE-side is worth games before that decision.

| Version | Content | Validated |
|---|---|---|
| 1.5.0 | Phase 0–1: harness + search-constant SPSA | +27 vs 1.4.9 |
| 1.6.0 | Phase 2–2.9: eval scalar fits + TM robustness | +54 vs 1.5.0 |
| 1.7.0 | Phase 3–4: eval structure + staged Texel campaign | +280.74 vs phase1-final |
| **1.8.0** | Phase 5–7: TM fix + TT-bound eval + SF-distill + 5 self-play cycles | **+93 fast / ~+40 LTC** vs 1.7.0 |
| 2.0.0 (next) | **Phase 9: NNUE** (after Phase 8.5 board + search ladder + data prep here) | target +200–400 |
| 2.x | **Phase 10–12:** post-NNUE search architecture v2, conditional SMP, eval v2 (net ladder / HCE recovery — decided at 12.0) | +15–40 at 1T; SMP pays only at 8T |

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
- [ ] **Phase 8 — Correctness & infra hardening: runs on `development`, not here** (development's PLAN §4; steps 8.1–8.9: board/search fixes in 8.1/8.2, **the HCE audit's eval-correctness bundle in 8.3 + rule-50-damping/mate-drive SPRTs in 8.4/8.6**, infra in 8.7/8.8 — the canary split is 8.8 — and conditional 8.9). Rebase this branch onto its head when it lands.
- [ ] **Phase 8.5 — NNUE-ready board + pre-NNUE search ladder + data prep (THIS branch; PLAN §4.2), Tracks A/B after the rebase.** Track A = 8.5.1–8.5.3 (board, feeds 9.3); Track B = 8.5.4–8.5.11 (search, eval-independent — survives the net swap; one SPRT each, in this order); Track C = 8.5.12–8.5.13 (data/benchmark prep — no games, no rebase needed, before 9.1's big training run):
  - [ ] **8.5.1 StateInfo + cached check geometry** — pins/checkers/checkSquares once per node, reused across generation stages; `gives_check` into `make_move`. Node-count + NPS gated; SPRT if behaviour shifts.
  - [ ] **8.5.2 Layout cleanups** — EP legality without the Board copy, 16-bit Move, history restructure.
  - [ ] **8.5.3 Eager accumulator updates in make/unmake** — chess768 = no refresh machinery; validated by the randomized property test. Feeds directly into 9.3's performance layer.
  - [ ] **8.5.4 Correction-history gating** *(Track B starts here)* — no capture contamination, direction-gated updates (+2–8).
  - [ ] **8.5.5 Qsearch in-check upgrade** — ordered evasions + TT store (+2–8).
  - [ ] **8.5.6 TT-PV bit** — real PV ancestry in the TT; enabler for LMR context + Phase 10 (+0–8 alone).
  - [ ] **8.5.7 Check-extension removal** — two-sided experiment (−5…+15), needs Phase 8's 8.1 + 8.8; SPRT decides keep-or-close.
  - [ ] **8.5.8 Checking-move LMR** — checks become reducible; after 8.5.7's verdict.
  - [ ] **8.5.9 History-coverage ladder (a–d)** — TT-cutoff bonus, exact-node training, failed-capture malus, fail-low countermove; own SPRT each, stop after two washes (+10–30 combined).
  - [ ] **8.5.10 Per-root-move state** — scores/PV/nodes per root move; feeds ordering, aspiration, TM, SMP (+5–15).
  - [ ] **8.5.11 Upcoming-repetition (cuckoo)** — both audits recommend; self-contained (+1–5).
  - [ ] **8.5.12 Frozen teacher benchmark** *(Track C starts here)* — SF-labelled corpus independent of our own adjudication, game-level splits, untouched test set; residuals reported by phase/material/king-danger cohorts for full/lazy/corrected HCE (and later the net). This is how we'll *know* the net is genuinely better, and it feeds the 12.0 decision. Deliverable: the baseline report for the 1.8.0 HCE.
  - [ ] **8.5.13 Data-pipeline hygiene** *(Track C)* — game/trajectory-level train-holdout splits everywhere (`import_beast.py` today splits per position — leaks trajectories; same rule for the net_trainer split on top of `extract_nnue.py` dedup); confirm `.mnn` header versioning covers the Phase-12 feature ladder so later nets need no format break.
- [ ] **Phase 9 — NNUE (ACTIVE; PLAN §4; ships as 2.0.0):**
  - [~] **9.1 Data at scale — ▶ IN PROGRESS 2026-07-09; label decision MADE: blended score+result.** Discovery: our fastchess PGNs already carry per-move search scores (`{+0.28/6 ...}`), so blended labels are free — no annotation pass. New extractor (now `tools/extract_nnue.py` in the shared `D:/code/net_trainer` repo; parallel, quiet-filtered, dedup) emits `FEN | cp(white-POV) | result(white-POV)` bullet-convertible text; verified on 200k games (1.9M unique in 80s; sign check: +100cp → 84.6% white score). Bootstrap from the five Phase-7 PGNs (~1M games) + fresh 2M-opening book (`beast_seed_2m.epd`, seed 777) generated. **Remaining: the big datagen run** (~2M games from the 1.8.0 head, ~8h background) → extract → 30M+ unique target. **Before the first full-size training run: 8.5.12 benchmark frozen + 8.5.13 hygiene applied.**
  - [ ] **9.2 Trainer integration — ✅ trainer BUILT 2026-07-12 in the shared `D:/code/net_trainer` repo** (PyTorch `net_trainer.nnue`: 768→(H×2)→1 perspective SCReLU, blended-label training, quantized **`.mnn`** export + parity verifier — the cross-engine format all five engines consume, spec in its `docs/mnn_format.md`). Remaining: the full-size training run once 9.1 data lands.
  - [~] **9.3 Inference core — ▶ bring-up layer DONE 2026-07-13, performance layer remaining.** Shipped: `src/nnue.{h,cpp}` .mnn loader + full-recompute eval (conformance-exact vs the net_trainer reference — the vectors caught a real ±1 floored-vs-truncated division mismatch, now pinned in the contract), `Evaluator::evaluate` dispatch, UCI `UseNNUE` (default false) + `EvalFile` (runtime load / `<embedded>`), CMake `-DBASILISK_NNUE_FILE=net.mnn` bake-in, `tests/test_nnue` (24/24, in CTest → suite now 10/10). **Provably inert by default: bench 12,661,251 unchanged.** Remaining: incremental accumulators in `make/unmake` + AVX2 (full recompute would cost ~90% NPS — see PLAN §4.1). *(Fable 5 high / Opus 4.8 high.)*
  - [ ] **9.4 Swap-in + SPRT** — net eval behind a compile flag, HCE kept as fallback during bring-up; SPRT vs the 1.8.0 head at `3+0.03`, confirm at `10+0.1`; iterate data/net size until it clearly passes both. Record the 8.5.12 benchmark residuals beside the SPRT — they feed 12.0.
  - [ ] **9.5 Search re-tune at the new eval scale** — the ONE deferred search-constant SPSA (histshape + wave2 mechanisms + TM knobs, PLAN §5), now legitimate because the eval scale is final; SPRT-gated.
  - [ ] **9.6 Release 2.0.0** — LTC field gauntlet vs the calibrated slate (head-to-head vs 1.8.0 is the number), full release checklist (PLAN §6).
- [ ] **Phase 10 — Search architecture v2 (post-NNUE; PLAN §5):** unified contextual reduction driving all pruning (10.1, staged SPRTs), result-dependent verification depth (10.2), denser TT (10.3, LTC-tested), bound shaping (10.4), ProbCut/null/IIR refinements (10.5), correction-history consumption v2 — weighted sources + uncertainty into margins (10.6). Runs after 9.5 so the one big SPSA tunes the final architecture. Prior +15–40 at 1T.
- [ ] **Phase 12 — Eval v2: the direction decision (post-NNUE; PLAN §5).** Right after 2.0.0 (or after a repeated 9.4 failure):
  - [ ] **12.0 Decision** — no games; from the 9.4 SPRT margins + the 8.5.12 benchmark cohorts, decide: net cleared (≳+150 LTC) → iterate the net, HCE menu stays closed; net passed but underwhelmed → net ladder first, HCE items only where the benchmark shows the HCE still wins; net failed → HCE recovery becomes the active line. Recorded as a dated verdict in the PLAN.
  - [ ] **12.1 Net architecture ladder** — king buckets → material/output buckets + PSQT → threat inputs → pawn pairs; each stage = train + NPS + benchmark + SPRT (STC and LTC) vs the current net.
  - [ ] **12.2 HCE recovery menu (closed unless 12.0 opens it)** — winnability (needs the missing tuner built first), king-safety/shelter rework, endgame scaling families, passer/threat semantics, lazy-eval study, phase specialization. Every item: benchmark residual delta + ablation + SPRT.
- [ ] **Phase 11 — SMP scaling (conditional; PLAN §5):** only if multi-thread rating becomes a target — 8 threads currently burn ~5.5× the nodes for ~1× the speed. Per-thread root state, thread diversity, voting merge, history ownership. 0 Elo at 1T; +10–30 at 8T. Needs an MT test harness first.

---

## The Basic Rhythm

Most work is a short ping-pong:

```text
You   -> "Implement next step of the plan."
Model -> Reads PLAN.md, inspects current state, implements, verifies locally
         (build + bench fingerprint + 10/10 CTest), commits on a candidate
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
