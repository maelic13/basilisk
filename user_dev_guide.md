# Basilisk Development Workflow Guide

Quick human-side companion to `PLAN.md`. The plan is authoritative for scope,
dependencies and gates; this guide answers what branch is active, what comes
next, what to run, and what evidence to return.

## Current checkpoint

> ### ⭐ 1.9.1 FINALIZED — 2026-07-23 (release-ready; user publishes the Release)
>
> 1.9.1 bundles two pre-NNUE waves on 1.9.0, shipped as a **PATCH**: Phase 8.6
> (hardening / CI / telemetry + the structure-era NNUE-runway refactor —
> strength-neutral, verified +0.17 ± 4.41) and Phase 8.7 (profile-guided
> speed pass — **+4.34% NPS ≈ +8.69 ± 6.63 Elo**, batch-SPRT-confirmed;
> plus baked-magic startup 603→38 ms). The search algorithm is **bit-identical
> to 1.9.0** (bench 11,941,440) — the ~+8.7 Elo is pure speed. Version stays
> 1.9.1 (grey-zone, algorithm unchanged, NNUE supersedes it soon — user
> decision 2026-07-23). 1.9.1 is the frozen HCE baseline the NNUE line measures
> against.
>
> **▶ NEXT — the future (this guide's focus): post-1.9.1 NNUE runway →
> Phase 9.** Pure NNUE data-prep (the structure era already shipped in 8.6.10):
> 8.5.3 dirty-piece at the do_move seam · 8.5.14 TT graph-history (down-scoped)
> · 8.5.15 frozen-teacher benchmark (baselines 1.9.1) · 8.5.16 `net_trainer`
> preflight → rebase `nnue` onto the post-1.9.1 SHA → Phase 9 (768→(256×2)→1
> SCReLU perspective net, ships as **2.0.0**, +200–400 target). Full spec:
> PLAN §5.
>
> **To publish 1.9.1 (user, manual):** squash `development` → `Version 1.9.1`
> on `master` + push (fires ci.yml = the 8.6.4 dispatch dry-run), then
> **publish a GitHub Release** — `gh release create v1.9.1 --target master
> --notes-file <notes>`. A bare tag fires nothing; `release.yml` triggers on
> `release: published` and uploads PGO binaries named without `-pgo` (no
> manifest / sha256).
>
> **Durable measurement rules (Phase 8.7, apply going forward):** NPS via
> `tools/nps_ab.ps1` — self-pair-validate to ~0.00%, pin a HIGH-numbered core
> (CPU 0 carries interrupt work), pool ≥2 PGO builds/arm, idle box (a YouTube
> video alone invalidated a sub-1% run); never two pinned harnesses at once
> (the 2026-07-21 Basilisk+Rarog core-list collision); bench-identity ≠
> NPS-identity.

**Completed release history** (measured detail in `CHANGELOG.md` / `PLAN.md`):
**1.8.0** (2026-07-08) ≈ +93 Elo/1.7.0 at 3+0.03 (~+40 LTC) from time-management
+ a search-efficiency pass + a from-scratch eval refresh; the sixth HCE
self-play cycle washed, closing result-label refitting. **1.9.0** (2026-07-17)
≈ +52 Elo/1.8.0 from Phase 8 correctness/infra + Phase 8.5 strength + the
`hcefinal` SPSA (+35.94) — the last pure-HCE eval and the frozen NNUE baseline.

Branch sequence is fixed:

```text
development: Phase 8 → Phase 8.5 → 1.9.0 → Phase 8.6 → Phase 8.7 → 1.9.1
             → NNUE runway (8.5.3/14/15/16) → record handoff SHA
nnue:        stays frozen during 8/8.5/8.6
              → rebase once onto the post-1.9.1 handoff SHA
              → continue Phase 9
```

Do not implement Phase-8.5 StateInfo/search changes independently on `nnue`.
Do not merge partial NNUE implementation back into `development`.

NNUE training uses the existing **`D:/code/net_trainer`** repository. It
now provides a Rust trainer on pinned Bullet, CUDA training, BulletFormat data,
seeded shuffle, the blended search-score/WDL objective and raw
`quantised.bin` export. Its baseline is chess768 → (H×2 perspectives) → 1×8
material buckets with SCReLU (H=1024 default), with NumPy/C++/Rust integer
references and H32 conformance vectors. Phase 9 hardens and integrates this
pipeline; do not restore the retired PyTorch/`MNN1` implementation.

| Version | Intended content |
|---|---|
| 1.9.0 ✅ | Phases 8 + 8.5: correctness, infra/state, eval-independent search and NNUE preparation |
| 1.9.1 ✅ | Phase 8.6 (pre-NNUE hardening/CI/reproducibility/telemetry, strength-neutral +0.17±4.41; check-extension SPRT rejected) + Phase 8.7 (profile-guided speed pass, **+4.34% NPS ≈ +8.69±6.63 Elo**) — the final HCE release / frozen NNUE baseline |
| 2.0.0 | Phase 9: accepted embedded baseline NNUE using `net_trainer` |
| 2.x | Phase 10 final 1T search+tune, Phase 11 SMP, Phase 12 NNUE architecture/data ladder |

## Completed phases

Phases 0–8.7 are complete. Per-step measured history lives in `PLAN.md` (§4)
and `CHANGELOG.md`; the version → content → strength map is the table above.
This guide's active content is the **post-1.9.1 NNUE line** below.

### ════ Completed pre-release work — summaries (full record: PLAN §4 / CHANGELOG) ════

- **Phase 8 + 8.5 → 1.9.0** (≈ +52 Elo/1.8.0): audit-driven correctness/infra
  (rule-50/mate precedence, null clock, legal-EP hashing, SEE pin-awareness) +
  the pre-NNUE strength ladder + the `hcefinal` SPSA (+35.94). The frozen HCE
  baseline the NNUE line measures against.
- **Phase 8.6 → 1.9.1 (strength-neutral):** X-macro params, TT contract +
  static_asserts, the sanitizer gate repaired, the **structure-era refactor**
  (vector history, 16-bit Move, one `do_move`/`undo_move` seam = the Phase-9
  NNUE-accumulator attach point), a C++23 pass, clang-tidy at zero, search
  telemetry, CI in Rarog's shape. Verified neutral vs 1.9.0 (+0.17 ± 4.41);
  the check-extension removal (8.6.7) was rejected −10.17 and requeued post-NNUE.
- **8.6.8A accept-audit (bookkeeping, no code change):** after the placement-
  lottery incident (unpinned SPRTs carried a ~±10 Elo per-run bias that does
  NOT average out), every small 1.9.0 accept was re-measured pinned — all
  REAL, nothing removed, headlines ~40–55% bias-inflated. Two reusable methods:
  disjoint probes combine (errors in quadrature); a ~+2 Elo feature needs
  ~200k games to separate from 0 (the measurement floor).
- **Phase 8.7 → 1.9.1 (profile-guided speed, +4.34% NPS ≈ +8.7 Elo):** built an
  NPS instrument + profiler FIRST, then extracted only measured wins —
  conthist-hoist +3.03%, mobility switch-hoist +0.89%, eval slider-reuse +0.39%,
  SEE-verdict-memo +0.36% (all bench-identical) + baked-magic startup 603→38 ms.
  Batch SPRT +8.69 ± 6.63 confirmed. Rejected/retired: CheckInfo check-hint
  (−1.8%, = the 8.5.1 result), pin-sharing (−0.16%), pawn-cache (dead — 89–99%
  hit), PGO-enrichment (anti-pattern; training stays on `bench` only). A latent
  LMR post-move-`gives_check` bug was found; its standalone fix lost −21 Elo
  (de-tuning) and is relocated to the post-NNUE LMR rework (10.1/10.7).

### ════ AFTER 1.9.1 — NNUE line ════

- [ ] **Phase 8.5 (post-release NNUE runway) — pure NNUE data prep only
  (structure era moved pre-release → 8.6.10) — on `development`, then
  rebase:**
  - [ ] **8.5.3 dirty-piece contract** (accumulator input data; 0 Elo;
    attaches to 8.6.10's centralized do_move/undo_move + PlyContext).
  - [ ] **8.5.14 TT graph-history semantics** (down-scoped 2026-07-20: the
    rule-50-adjusted TT key is parked on Rarog's 7.3 verdict — invisible at
    our adjudication, de-tuning risk is not; eval-adjacent parts easier
    post-NNUE).
  - [ ] **8.5.15 frozen teacher benchmark** (baselines the *released* **1.9.1**
    HCE).
  - [ ] **8.5.16 `net_trainer` preflight** (split/dedup/filter/manifest/tests).
  - [ ] **Handoff:** record exact post-1.9.1 `development` SHA; rebase `nnue`
    once.
- [ ] **Phase 9 — baseline NNUE (`nnue`, ships as 2.0.0):**
  - [ ] **9.0:** rebase/inventory existing partial NNUE against the raw Bullet
    `quantised.bin` contract; validate payload, padding, inferred H and SHA.
  - [ ] **9.1:** `net_trainer` validation, best checkpoint, resume, manifests,
    hashes, explicit seeds, strict CLI/error handling and CI; prove exact
    continuation or explicitly forbid resume.
  - [ ] **9.2–9.3:** 30–60M unique-position first dataset, controlled label/node/
    adjudication experiments, reproducible H1024 chess768/1×8 training (H512
    for sub-20M pilots or a measured speed candidate).
  - [ ] **9.4:** scalar loader/full-recompute exact H32/all-bucket conformance
    first.
  - [ ] **9.5:** incremental accumulator, property verification, exact scalar/
    SIMD parity, supported ISA kernels and new PGO profile. **Includes the
    toolchain-stabilization rider (PLAN §5 9.5): pin one LLVM major on all
    platforms — Linux apt clang-NN (19 since the 1.9.1 CI fix), Windows
    llvm-mingw exact tag (replaces rolling MSYS2), macOS brew `llvm@NN` via
    `COMP=llvm` — one `LLVM_MAJOR` variable per workflow, toolchain stamped
    into release metadata, accepted by the CI/release bench-agreement jobs.**
  - [ ] **9.6:** net versus the released 1.9.1 HCE at STC, `10+0.1`, frozen
    teacher cohorts and NPS; iterate one variable at a time.
  - [ ] **9.7:** only emergency search-scale safety calibration. The final
    search tune waits until Phase 10.7.
  - [ ] **9.8:** 2.0.0 release with embedded-net SHA and production asset gate.
- [ ] **Phase 10 — final 1T search architecture + tune** (prior **+15–40 @1T**,
  heavily non-additive; build the architecture first, then tune once at the end.
  8.5.4 telemetry is an acceptance input for every candidate):
  - [ ] **10.1 Unified contextual reduction:** one signed `r` per non-first
    move from PV/cut, TT-PV/depth/bound/move-class, improving, move count,
    check/capture, accepted histories, correction uncertainty; derive
    `lmrDepth = newDepth − r` once and reuse it for futility/history/SEE
    pruning. Staged (parity → 2nd-move → checks → good/bad captures →
    neg-reductions → remove old exceptions), each its own SPRT.
    **MUST INCLUDE: (1) 8.6.8 `cutoffCnt` reduction input; (2) the LMR
    post-move-`gives_check` fix — the `r` uses the correct pre-move check by
    construction. Its standalone fix lost −18/−19 (2026-07-23, §6): the
    hcefinal LMR constants were tuned WITH the bug's accidental
    over-reduction (lesson-15 de-tune), so it is re-tuned in 10.7, not
    shipped alone. (rebuild the one-liner at 10.1).**
  - [ ] **10.2 Result-dependent verification:** deeper/shallower full-search
    depth from the reduced result + node confidence; train post-LMR history
    from both outcomes. Staged SPRTs.
  - [x] **10.3 TT density & replacement → PULLED to pre-1.9.0 as 8.5.D1**
    (eval-agnostic + durable; strengthens the final HCE release). Slot kept so
    10.4–10.7 references don't shift.
  - [ ] **10.4 Bound quality:** blend RFP/qsearch proof values conservatively
    toward beta, keep fail-soft futility bounds, finish near-rule-50 TT-cutoff
    safeguards. **← this is the "provable qsearch bounds" that unblocks the
    8.5.6 in-check-qsearch TT store.** Separate SPRTs.
  - [ ] **10.5 ProbCut/null/IIR:** staged ProbCut MovePicker (TT/cap-hist/SEE
    ordering + TT-disproof skip); null-move verification min-ply region; IIR
    audit vs PV/cut/all-node + TT-PV. Standalone SPRTs.
  - [ ] **10.6 Correction-history consumption v2:** per-source weights (not
    `/5`), accepted 2-/4-ply continuation-correction contexts, absolute
    correction as uncertainty in margins. Staged fit + SPRT. *(Eval-adjacent —
    like 8.5.5, easier once NNUE removes the HCE mate-drive fragility.)*
  - [ ] **10.7 Final search tune:** ONE comprehensive SPSA after 10.1–10.6 are
    decided — histshape/wave2/correction/TM/pruning-margin dims, dead knobs
    excluded, pre-registered ranges/stop rule; confirm at `10+0.1`, long TC,
    several hash sizes, TUNE=OFF and TUNE=ON. **Targets the final net** (this
    is the tune the old plan wrongly scheduled at 9.5).
- [ ] **Phase 11 — mandatory SMP** (fixed-time 1/2/4/8/16-thread paired harness
  with recorded affinity/hash/NUMA/manifests first; 1T SPRT can't see SMP Elo.
  Startpos smoke: 5.72× nodes for 1.49× speed @8T — diagnostic only):
  - [ ] **11.1 Per-thread root state** (extend 8.5.12: each thread's
    scores/PVs/variance/nodes/completed depth).
  - [ ] **11.2 Controlled diversity** (aspiration / reduction-depth jitter /
    root-order perturbation; measure overlap so diversity is shown not assumed).
  - [ ] **11.3 Voting & stopping** (score/depth/effort-weighted best-thread
    voting, agreement-aware soft stop then bounded hard stop).
  - [ ] **11.4 Shared-state ownership** (thread-local vs shared histories/
    corrections; stop re-blending stale helper tables; false-sharing tests).
  - [ ] **11.5 Topology & memory** (pinning/NUMA/first-touch, large pages,
    scaling at realistic hash; preserve deterministic 1T).
- [ ] **Phase 12 — NNUE architecture, data & frontier loop** (chess768 is the
  bring-up baseline, not the final evaluator):
  - [ ] **12.0 Evidence review** (STC/LTC/MT Elo, NPS, learning/quant curves,
    8.5.15 cohort residuals; pick the next net feature from residuals).
  - [ ] **12.1 Versioned architecture ladder** (one change at a time, each with
    format doc + reference inference + conformance vectors + scalar/SIMD
    parity): mirrored king buckets/refresh cache → pairwise mult + small dense
    layer → evidence-backed threat inputs → PSQT/structure. (8 output buckets
    already in v1.)
  - [ ] **12.2 Data/training ladder** (scale beyond 60M by learning curves;
    A/B teacher scores, node mix, WDL λ, cohort balance, optimizer/hidden size;
    immutable test set).
  - [ ] **12.3 HCE recovery menu** (closed by default; only for product
    fallback / benchmark cohorts).
  - [ ] **12.4 Frontier acceptance** (vs full-strength SF/Reckless/PlentyChess
    + one more; 1T & MT, multiple hashes, STC/`10+0.1`/long TC; seek CCRL).

> **Why Phases 10–11 come after NNUE (they *are* general engine work — the
> sequencing is deliberate, not because they're NNUE-specific):** (1) NNUE is
> the dominant lever (+200–400 vs Phase 10's +15–40, non-additive) — get the
> big win first; (2) search is *calibrated to the eval* — NNUE changes the
> eval's scale, noise and tactical character, so the search architecture
> decisions (10.1–10.6, SPRT-judged) and especially the single final tune
> (10.7) should target the *final* net, not the HCE that's about to be
> replaced; (3) the eval-adjacent items (10.4 bound quality, 10.6 correction
> v2) hit the same **HCE mate-drive fragility** that blocked 8.5.5 — they get
> *easier* after NNUE deletes the mate-drive. The genuinely eval-agnostic
> subset (10.1 reduction, 10.3 TT, 11.x SMP) *could* be front-loaded before
> NNUE for a stronger pre-NNUE engine, but that trades speed-to-the-big-lever
> for a smaller non-additive gain that carries over regardless of when it's
> done — so the plan keeps them after NNUE. *(2026-07-20 exception, by user
> decision: Phase 8.6 front-loads exactly two evidence-backed, no-re-tune
> items — the check-extension removal (Rarog +30.75, one SPRT, no SPSA) and
> the no-games hardening/CI wave — into a final HCE release 1.9.1. The
> SPSA-costly LMR modernization stays post-NNUE unless 8.6.8 is explicitly
> opted into.)*

## Development rhythm

```text
You   → “Implement the next step.”
Model → inspect current state, create one semantic candidate, build/test,
        commit on its candidate branch, provide exactly the long-run command.
You   → run SPRT/SPSA/gauntlet/datagen and paste the result.
Model → merge+document or reject+document, then advance the plan.
```

Use the correct decision gate:

| Claim | Gate |
|---|---|
| Strength gain | `elo0=0, elo1=3` SPRT |
| Mandatory correctness | Regression/oracle tests + pre-registered non-inferiority |
| Behavior-neutral enabler | Exact bench/default parity + correctness/performance evidence |
| Eval/network checkpoint | Validation selects within a run; SPRT decides engine strength |
| SMP | Fixed-time paired MT games; 1T SPRT cannot see it |

Never bundle unrelated behavior changes. A correctness campaign may share a
final non-inferiority run, but each defect remains a separate commit/test so a
regression is bisectable.

## Common commands

```powershell
# Basilisk candidate build and strength gate
cd D:\code\basilisk
.\tools\build_test.ps1 -Suffix mystep-name
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-candidate-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-baseline-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -Elo1 3

# Phase 8.6.7 (check-extension removal) — the phase's one SPRT
.\tools\build_test.ps1 -Suffix phase867-nocheckext
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-phase867-nocheckext-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-phase8512-instabtm-pext-pgo.exe `
    -NameA "NoCheckExt" -NameB "Basilisk-1.9.0" -Elo1 3

# Non-inferiority / phase-boundary confirmation
.\tools\sprt.ps1 ... -Elo0 -3 -Elo1 0
.\tools\sprt.ps1 ... -TC "10+0.1"

# Phase 8.7 NPS A/B (script lands in 8.7.1; Rarog 10.3 protocol).
# ALWAYS validate on a self pair first — same exe both arms, must read ~0.00%.
# Screen non-PGO (deterministic, OVERSTATES), confirm with ≥2 pooled PGO
# builds per arm. Engine-side primitive: bench 13 5  (best of 5 + median/min).
.\tools\nps_ab.ps1 -EngineA <cand.exe> -EngineB <cand.exe>          # self pair
.\tools\nps_ab.ps1 -EngineA <cand.exe> -EngineB <base.exe> -Rounds 12

# Current net_trainer Bullet baseline pipeline (Phase 9; NVIDIA CUDA training)
cd D:\code\net_trainer\trainer
cargo build --release --features cuda
cd ..
python tools\datagen.py --engine <phase85-hce.exe> `
    --book data\books\openings.epd --rounds 500000 --out data\pgn\selfplay.pgn
python tools\extract_nnue.py data\pgn\selfplay.pgn --out data\txt\train.txt
.\trainer\target\release\net-trainer.exe convert data\txt\train.txt data\txt\train.bf
.\trainer\target\release\net-trainer.exe shuffle data\txt\train.bf `
    data\txt\train_shuffled.bf --seed 42
.\trainer\target\release\net-trainer.exe train data\txt\train_shuffled.bf `
    --hidden 1024 --id run1 --out trainer\checkpoints --superbatches 160 --wdl 0.3
python -m net_trainer.nnue.vectors `
    trainer\checkpoints\run1-160\quantised.bin `
    --out trainer\checkpoints\run1-160\vectors.json

# Field gauntlet
cd D:\code\basilisk
.\tools\gauntlet.ps1 -Engine <candidate> `
    -Opponents <prior-release>,<field...> -TC "10+0.1"
```

Phase 9.1 will extend the trainer command with explicit split/resume/manifest
options. Use the exact command recorded for that revision rather than silently
reusing this baseline after the CLI changes.

## What to report back

- **SPRT:** Elo ± error, LLR, games, pentanomial counts, H0/H1 verdict, and the
  generated manifest path.
- **SPSA:** run manifest, final values, iteration count, trend/stop reason and
  state path.
- **NNUE:** engine/trainer SHAs, pinned Bullet revision, Rust/Cargo/CUDA/driver/
  GPU versions, H/buckets/constants, dataset/book/net hashes, command/config,
  seed, train+validation+untouched-test metrics, quantization/parity and NPS.
- **Gauntlet:** PGN plus manifest/cross-table; prioritize the paired head-to-
  head against the prior release.
- **Errors:** engine/net identity and incident log. Basilisk must have zero
  illegal moves, time forfeits, crashes and incremental-eval mismatches.

## Release gate

The model prepares version changes, changelog, exact artifact verification and
release notes. The user squashes `development` to `master` and pushes, then
**publishes a GitHub Release** (`gh release create v<x> --target master
--notes-file <notes>`). `release.yml` fires on `release: published`, not on a
bare tag, and must upload the same documented production PGO/ISA assets that
were smoke-tested—not an unprofiled substitute.

Every release needs:

- all applicable strength/non-inferiority/correctness gates;
- standard, `10+0.1`, and major-release genuinely longer confirmation;
- exact binary/book/net/test manifests and SHA-256 values;
- all CTest/property/sanitizer requirements;
- zero Basilisk incidents;
- prior-release head-to-head and contemporary field comparison;
- 4/8-thread validation when claiming multithread strength.

The operating rule is simple: retain evidence, keep only sound improvements,
and tune only after the structure whose constants describe is final.
