# Basilisk Development Workflow Guide

Quick human-side companion to `PLAN.md`. The plan is authoritative for scope,
dependencies and gates; this guide answers what branch is active, what comes
next, what to run, and what evidence to return.

## Current checkpoint

> ### ⏱ LIVE STATUS — 2026-07-17
> **🎉 1.9.0 RELEASED** — the pre-1.9.0 campaign is complete and the release is cut.
>
> **Release engine / head:** `instabtm` = `basilisk-phase8512-instabtm-pext-pgo`
> (bench **11941440**). Version **1.9.0** (Constants.h + CMakeLists), CHANGELOG
> `[1.9.0]` written, CTest 11/11, UCI reports `Basilisk 1.9.0`.
>
> **Branches:** `master` = single `Version 1.9.0` commit (all Phase 8 + 8.5 work
> squashed, no co-author). `development` has been reset to that `master` state and
> **development continues from here**.
>
> **Still manual (user):** tag `v1.9.0` + push `master` (PGO `release.yml` assets +
> manifests fire on the tag); optional cumulative `instabtm`-vs-1.8.0 confirmation
> gauntlet (fast + `10+0.1`) for the shipped number.
>
> **✅ Pre-1.9.0 gains banked** (each accepted vs the prior head; cumulative over the Phase-8 base `phase86-matedrive`, ≈ **+25 Elo** fast-TC):
>
> | Step | Elo | New head |
> |---|---|---|
> | 8.5.D1 TT density | +4.27 | ttdensity |
> | 8.4 rule-50 damping | +3.29 | rule50-retry |
> | 8.5.10(b′) exact/PV reward-only history | +4.90 | exacthist-rewardonly |
> | 8.5.10(e) surprise-scaled history | +2.50 | evaldiff |
> | 8.5.12 instability-TM | **+10.79** | **instabtm** ← current |
>
> **❌ Tested → rejected / deferred (no gain):** 8.5.1 check-geometry (NPS), 8.5.6 qsearch-in-check, 8.5.5 corrhist (robust-canary reject), 8.5.13 cuckoo (wash-neg), **8.5.7 TT-PV — re-tested 2026-07-17, node-vetoed +51% (fresh-TT), → 10.7 sub-project** (needs ttPv pruning-conservatism + SPSA, not a flip), 8.5.10(a) TT-cutoff reward (bench-veto +82%), 8.5.10(b) exact+malus (−84), 8.5.10(c) capture-maluses (bench-veto +30%), 8.5.10(d) prior-move cont (deferred), PostLmrHistScale (wash), inert 6.x knobs (batch: none gain), **8.5.11 threatened-history (bench-veto +167% — context-split fragments main-history; → 10.7)**, **8.5.2 layout/16-bit Move (0-Elo → NNUE line/Phase 10)**, **8.5.4 telemetry (0-Elo dev tool, skipped)**.
>
> **📋 Next dev work (post-1.9.0, on `development`):** the NNUE runway — 8.5.3
> dirty-piece, 8.5.14 TT graph-history, 8.5.15 frozen-teacher benchmark, 8.5.16
> `net_trainer` preflight → rebase `nnue` → Phase 9. Deferred to 10.7 joint SPSA:
> TT-PV (8.5.7), history-v2 (8.5.11), (d) cont-hist rebalance, inert knobs.
>
> _(The dated narrative below is the historical record up to 2026-07-15 and is superseded by this block for anything "current".)_

**Basilisk 1.8.0** was released 2026-07-08. It gained approximately +93 Elo at
`3+0.03` and +40 at `10+0.1` over 1.7.0. The sixth post-release HCE self-play
cycle washed, so repeated HCE result-label fitting is closed. Verified HCE
semantic defects still get corrected in Phase 8; new HCE feature development
is not the frontier path.

**✅ Pre-Phase-8 hcefinal SPSA DONE 2026-07-14: ACCEPTED +35.94 ± 9.42** (H1,
LOS 100%, 2,270 games), merged `7c3a857`, bench **15,008,100** — the largest
single tune in project history (asymmetric-linear history + rescaled
consumers + live LMR context; `PostLmrHistScale` excluded on the documented
KBNK-conversion failure, stays 0; `QsearchCheckCap` baked 0 → **8.9 = skip**).
All canaries pass on the merged head (9/9 CTest incl. the full KBNK playout).

**Active:** **Phase 8 COMPLETE & FINAL (2026-07-15)** on `development` — 8.1
accepted, 8.2 accepted, 8.3 accepted (+13.97), 8.4 reverted (KBNK canary),
8.6 accepted (+3.19), 8.7/8.8 done (release PGO/manifests/tiers; CI +
invariants/fuzz + assert_ok + canary split + benchmark repair), **8.9
confirmed SKIP**, and the **8.8-followup `see()` X-ray fix REJECTED −3.80 and
reverted**. **Final accepted head = `phase86-matedrive` (bench 10411042).**
**Bug audit 2026-07-15: NO correctness bugs remain.** Every verified defect
(infra §4.1–4.5, HCE 1–3) is fixed; §4.4 SEE-king refuted; §4.6 history is
clamp-safe. The 8.8 fuzz (assert_ok/2.2M states, differential perft, see_ge
invariants, parser fuzz) confirms no board/movegen/SEE bugs. Remaining audit
items are architectural gaps (eval could be *better*, not *wrong*) or dead code
(winnability). **Two test-data/gate bugs were found and fixed:** endgames.epd
had 4 *illegal* positions (side-not-to-move in check), and the KBNK-conversion
canary made single-position fixed-depth conversion a hard gate — a search-
*shape* trajectory that over-fired on 8.4/8.5.5/8.5.6/TT-density while the eval
still saw the win. **Full gate sweep done 2026-07-15 — TWO brittle canaries
fixed** (both `test_endgames` conversion and `test_search`'s exact
mate-distance ≤5; all other gates — perft, fuzz, TT round-trip, WAC floor,
`--verify`, SPRT — are objective/robust). The endgame canary is now robust
(eval-recognizes-win + conversion floor + near-mate recognition incl. a
stalemate trap; exact conversion is a diagnostic). **This reopens candidates
the brittle canaries wrongly blocked** — TT-density (8.5.D1) passes and is
committed (SPRT-pending, **next action**); **8.4 and 8.5.5 are now retry
candidates**, plus re-baking PostLmrHistScale (excluded from the hcefinal bake
on the old KBNK canary) and re-examining the inert 6.x knobs. So the earlier
"HCE is exhausted, release now" call was premature — there are real shots at a
stronger 1.9.0 first. See the reorganized tracker below (pre-1.9.0 / ⭐ RELEASE
/ post-1.9.0).

**Phase 8.5 ACTIVE** (on `development`), **RESHUFFLED**: Elo/Track-B work
before the 1.9.0 release, the three NNUE-only items (8.5.3/8.5.15/8.5.16)
after it (see PLAN §8.5). Early attempts on the "cheap" items came up empty:
**8.5.1** (Track A cached check-geometry) was a net NPS regression → reverted;
**8.5.6** (qsearch in-check upgrade) was negative on both parts and its TT-store
half is **blocked on Phase 10.4** bound-shaping (evasion children return the
non-provable 8.1f fail-hard bounds). Neither was the main Elo vein. **User
decision 2026-07-15: push on into the history-based Track-B items** (the
classic Elo source): 8.5.5 correction-history → 8.5.10 history ladder → 8.5.11
history v2 → 8.5.12 aspiration/TM, each SPRT-gated. If those wash too, cut
1.9.0 on the banked Phase-8 gains and move the harder search work into the
NNUE line. _(Head at the time of this 2026-07-15 note was `phase86-matedrive`,
bench 10411042; it has since advanced through 8.5.D1/8.4/(b′)/(e) — see the LIVE
STATUS block at the top for the current head.)_

Branch sequence is fixed:

```text
development: Phase 8 → Phase 8.5 → record accepted handoff SHA
nnue:        stays frozen during 8/8.5
              → rebase once onto the Phase-8.5 handoff SHA
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
| 1.8.1/1.9.0 | Phases 8 + 8.5: correctness, infra/state, eval-independent search and NNUE preparation |
| 2.0.0 | Phase 9: accepted embedded baseline NNUE using `net_trainer` |
| 2.x | Phase 10 final 1T search+tune, Phase 11 SMP, Phase 12 NNUE architecture/data ladder |

## Phase tracker

- [x] **Phases 0–7:** harness, search baseline, HCE structure/tuning, TM and
  1.8.0 release. See `PLAN.md` and `CHANGELOG.md` for measured history.
- [x] **Phase 8 — correctness and infrastructure — ✅ COMPLETE 2026-07-15
  (`development`):**
  - [x] **8.1 — ✅ ACCEPTED 2026-07-14** (six commits `0492739..e87023f` + tests
    `8279fff`, merged to `development`): rule-50/mate precedence, null clock,
    legal-EP-only hashing, history guard 2048+clamp, qsearch-in-check
    termination, fail-soft qsearch cap return. The two stronger fail-soft
    changes were implemented, **measured to corrupt mate resolution, bisected,
    and deferred to 10.4** (traps documented in code). Non-inferiority run
    stopped at 13.5k games by decision: **−2.06 ± 3.82, LLR −0.56** — inside
    the pre-registered −3 tolerance, true-value-between-bounds non-convergence;
    the ≈−2 cost is 8.1e's extra evasion nodes, **recovery planned at 8.5.5**
    (ordered/TT-stored in-check qsearch). Board tests 253→273, search 110.
    Calibration note recorded for 8.8: the generous-limit KBNK playout fails on
    every version incl. released 1.8.0 — the fixed-depth playout is the real gate.
  - [x] **8.2 — ✅ ACCEPTED 2026-07-14** (SPRT run #2, non-inferiority
    [−3, 0] vs `phase81-correctness`, stopped by decision at 23,462 games:
    **+0.65 ± 2.90**, nElo +1.00 ± 4.45, LOS 67%, LLR **+1.46 drifting
    toward accept** — measured value dead-center in the pre-registered
    +0…+4 expectation, entire CI clear of the −3 tolerance): `see()`/
    `see_ge()` now exclude absolutely pinned attackers via a shared
    `Board::see_pins()` scan computed against the **exchange occupancy**
    (mover + target removed — a first cut at a static scan against `all_occ`
    missed pins *revealed by the capture being scored* and was caught by the
    new oracle); a pinned piece re-enters the exchange when its pinner leaves
    `occ` or when the target square lies on the pin line, and kings are never
    filtered (audit §4.4's SEE-king claim stays REFUTED — sentinel verified by
    two new king-recapture tests). `see()`'s LVA selection was restructured to
    run *before* the gain write, since pin filtering can empty a nonempty
    attacker set. Tests: independent slow legal-exchange oracle
    (make/unmake + `gen_legal`, exact pins/checks) cross-checked on every
    curated case; audit repro `4k3/4n3/2p5/1B6/8/8/8/K3R3 w` Bxc6 = **+100**;
    pinned N/B/P recapturers, discovered-pin twins, pinner-departs vs
    pinner-stays pair, king-recapture legal/illegal pair. Board tests
    273→305, full CTest 9/9 green. Bench **11,694,451** (was 10,883,689 —
    SEE decisions changed, expected). Engine:
    `tools\test_engines\basilisk-phase82-see-pext-pgo.exe`. Gate per PLAN §4:
    non-inferiority first (`-Elo0 -3 -Elo1 0`) vs
    `basilisk-phase81-correctness-pext-pgo.exe`, report the Elo interval;
    honest expectation +0…+4.
  - [x] **8.3 — ✅ ACCEPTED 2026-07-14** (SPRT run #3 vs `phase82-see`,
    completed by LLR: **+13.97 ± 6.22**, nElo +21.15 ± 9.40, LOS 100%,
    LLR +2.96 → H1 at 5,250 games — a clear genuine gain, above the +0…+8
    prior; the corrected semantics beat the weights fitted to the bugs
    even before any refit. Refit consideration → Phase 8.5 re-tune notes.)
    Three semantic commits + hygiene rider, each CTest-green with
    `--verify` 10000/10000 exact:
    **8.3a OCB cap** — `ocb_draw_scale(p) = min(48, 32+4p)` shared eval/tuner
    helper (pre-fix reached 2x amplification at 16 pawns); identical at ≤4
    pawns. **8.3b rook/passer decoupling** — enemy-rook-behind-passer is its
    own pass: fires without a friendly rook on the file, counted once per
    enemy rook (was once per stacked friendly rook); "behind" stays
    geometric. **8.3c attacked2 pawn pairs** — left/right pawn capture sets
    seeded separately so two-pawn double attacks enter `attacked2`
    (consumers: strongly_protected/hanging, king-ring/flank). **Rider** —
    stale comments fixed (winnability was never actually tuned: zero
    gradient in the linear-trace optimizer; no-queen 2/3 seeds retuned
    since). Tests use param-perturbation activation counting; two traps
    documented in tests: the 700cp lazy margin skips the dynamic tail, and
    the frozen phase≤6 mate-drive adds a ~44cp step past |eval|=200 (8.6's
    subject) — test positions stay balanced and inside ±200. test_eval
    55→73. Bench **11555879** (8.3a: 15047169, 8.3b: 13486660, 8.3c/rider:
    11555879). Engine: `tools\test_engines\basilisk-phase83-eval-pext-pgo.exe`.
    Gate per PLAN §4: SPRT run #3 vs `basilisk-phase82-see-pext-pgo.exe`,
    non-inferiority (`-Elo0 -3 -Elo1 0`), report the Elo interval; honest
    expectation +0…+8 bundled (the fit ratified these bugs — a wash is a
    real outcome; flat = keep as correctness).
  - [x] **8.4 — ❌ REVERTED 2026-07-15 (canary, pre-SPRT):** the gentler
    `(199−clock)/199` damping curve **broke the KBNK fixed-depth-18
    conversion canary** (test_endgames `8/8/8/8/3k4/8/3KBN2/8 w`: 17/18 vs
    18/18 without it — bisect-confirmed against the parent commit). Root
    cause class: the harsh old curve is *load-bearing conversion pressure* —
    decaying eval punishes shuffling in pawnless mating endings, same canary
    family as 6.2–6.5/8.1f. **I initially missed it by running CTest against
    a stale test_endgames binary** (only test_eval+basilisk were rebuilt);
    process fix: full rebuild before the CTest gate, always. Reverted in
    `3139703`; SPRT run #4 was stopped. If revisited, PLAN's threshold
    variant (damp only above ~clock 20, keep a steep tail) is untested and
    could preserve the conversion pressure — file under 8.6-adjacent
    experiments. `basilisk-phase84-rule50-pext-pgo.exe` is dead; the head
    stays `phase83-eval` (bench 11555879).
  - [x] **WAC diagnostic (2026-07-15, rider):** `wac [depth]` command +
    CTest floor test mirrored from Rarog (SaberTooth CI pattern). Three-way
    reference run at SaberTooth's CI conditions (fixed depth 9, all 300):
    SaberTooth **280/300** (~33s), Basilisk **208/300** (15.0M nodes,
    3.8s), Rarog **177/300** (19.5M nodes, 6.4s). NOT comparable as
    strength: nominal depth buys wildly different trees per engine —
    Basilisk/Rarog's aggressive LMR/pruning make depth 9 a far thinner
    tree than SaberTooth's. Use each engine's own solved count vs ITSELF
    across candidates (the floor test); cross-engine verdicts stay with
    SPRT gauntlets.
  - [x] **8.6 — ✅ ACCEPTED 2026-07-15** (SPRT run #5, standard gate `[0, 3]`,
    stopped by decision at 9,158 games: **+3.19 ± 4.48**, nElo +5.06, LOS
    **91.8%**, LLR +0.81 drifting toward accept — trend favours H1, no path
    to revert, and correctness-justified with all canaries green; measuring
    better than 8.2 did when kept). New head = `phase86-matedrive`. Gated
    the frozen endgame mate-drive
    (`eval.cpp`) to bare-king mating signatures — fires only when the
    defender has no pawns AND the attacker holds forcing-mate material (Q, R,
    bishop pair, or B+N). Previously it added push-to-edge/close-the-kings
    geometry in *any* phase≤6 position with |eval|>200, wrong in pawn races,
    rook-and-pawn endings, and king-opposition pawn endings (audit
    hce_analysis 7). KBNK/KNNK stay handled by `apply_endgame`'s overrides —
    the gate never touches them, so the KBNK conversion canary (which killed
    8.4) is unaffected; full CTest 10/10 confirms. **No tuner co-fix**: the
    mate-drive is captured in the untraced residual, `--verify` 10000/10000
    exact. Testing this frozen, lazy-shadowed term needed a difference-of-
    differences (KQ-vs-KR in the (200,700) active band — lazy eval skips the
    term above 700cp, which every fully-cornered lone king trips — with the
    White king equidistant from both defender squares and an equidistant
    defender pawn toggling the gate): ~38cp isolated contribution, present
    pawnless, gone with a pawn; plus a no-leak check on zeroed KN-vs-K.
    test_eval 73→77. Bench **10411042** (eval changes in gated endgames,
    expected). Engine:
    `tools\test_engines\basilisk-phase86-matedrive-pext-pgo.exe`, SPRT vs
    `basilisk-phase83-eval-pext-pgo.exe` at `-Elo1 3` (standard gain gate —
    revert on H0). Prior +0…+4; a wash is a real outcome since the geometry
    was mostly lazy-shadowed already.
  - [x] **8.7 — ✅ DONE 2026-07-15 (no games; config + local measurement):**
    `release.yml` now builds **PGO** assets (the `pgo` target) for every tier,
    runs CTest + a smoke/bench on the exact uploaded file, and publishes a
    per-asset `manifest.txt` (revision, compiler, bench, NPS, size, SHA-256) +
    `.sha256`. `docs/release_tiers.md` documents the accurate CPU contract
    (portable / `-avx2` / `-pext` needs fast BMI2 (Zen 3+/Haswell+) / aarch64
    NEON), records a measured reference point (pext-pgo ~2.27 MB, ~2.75 M nps),
    and declines to promise an unmeasured speedup or a not-yet-built
    x86-64-v2 tier. **CI-only — cannot self-verify the workflow; run a release
    dry-run to confirm the matrix.**
  - [x] **8.8 — ✅ DONE 2026-07-15:** the testing class that would have caught
    8.1/8.2. **`ci.yml`** (added in 8.8, then **removed at the 1.9.0 release** —
    only `release.yml` ships now, doing PGO binary build + attach; the CTest/
    sanitizer/fuzz suite it ran stays a local gate): push/PR Linux+Windows Clang
    Release + full CTest + deterministic-bench fingerprint; Linux Debug
    ASan+UBSan; nightly GCC cross-check + 30 rotating-seed fuzz runs.
    **`Board::assert_ok()`**:
    mailbox↔bitboards, occupancy, kings, castling/EP plausibility, incremental
    keys vs recompute, cached checkers. **`test_invariants`** (in CTest):
    ~2.2M-state random-walk make/unmake + assert_ok + full-unwind; differential
    perft (legal vs pseudo-legal+is_legal vs published); see_ge threshold
    invariants (~205k moves); FEN round-trip + malformed-FEN robustness fuzz.
    **Canary split** (search doc §14): mate conversion + no-false-draw gate;
    exact mating-ply/route is now a printed non-gating diagnostic. **Manifests**
    in build_test.ps1 / sprt.ps1 (gate 8). **board_performance repaired**
    (no in-timed-region copies, see_ge workload replaces the trivial cached
    check, median-of-11 + MAD). CTest 10→11 targets. **The fuzz found a real
    bug**: `see()` (ordering-only) disagrees with `see_ge()`/brute-force on
    deep multi-recapture X-ray chains — flagged for a separate SPRT-gated fix
    (task chip `task_85937f34`); `see_ge()` (pruning) is correct. Partial
    gaps: no dedicated TT-move-decoding or full-UCI-command fuzz yet.
  - [x] **8.8-followup — `see()` X-ray fix ❌ REJECTED & REVERTED 2026-07-15**
    (non-inferiority `[−3, 0]` vs `phase86-matedrive`: **−3.80 ± 2.63**, nElo
    −6.17, LOS 0.23%, LLR −2.95 → **H0 at 25.4k games**). Removing `see()`'s
    gain-array early-exit prune made it exact (matched `see_ge()`/brute force on
    the deep-X-ray repro, −100→−200), but running the full swap on *every*
    ordering call cost ~−3.8 Elo — an NPS tax for zero decision benefit, since
    `see()` is ordering-only and `see_ge()` (pruning) was always exact and
    pin-aware. **Durable lesson:** SEE is a heuristic, not a correctness
    quantity; "exact SEE" only matches the SEE *definition* (itself an
    approximation), so exactness for ordering isn't worth an NPS loss — the
    fast X-ray-approximate prune is standard practice. Reverted `fa432d2`;
    intent documented in-code at the prune (do-not-refix) and the coupled
    `see()==see_ge()` fuzz check removed (fuzz keeps the `see_ge` invariants).
    The 8.8 fuzz infra still earns its keep: it *found* the discrepancy; the
    SPRT decided exactness wasn't worth it. Head stays `phase86-matedrive`
    (bench 10411042).
  - [x] **8.9:** ~~direct quiet-check generation~~ — **SKIP, re-confirmed by
    analysis 2026-07-15.** All gate conditions hold: `QsearchCheckCap` baked 0
    (`SearchParams.h`), the `gen_quiet_checks()` block is provably inert
    (`search.cpp` guards it with `qsearch_check_cap > 0` → dead at 0), and the
    setoption is `BASILISK_TUNE`-only so release can't enable it. Doing it
    anyway has no target: the direct-generation optimization would speed up a
    path that never runs (cap 0 → zero cost), and enabling quiet checks was
    already SPSA-rejected (hcefinal drove the dim to 0) — modern SF dropped
    them too. Inert infrastructure kept under `TUNE` for post-NNUE re-tuning.
    **Item closed.**
### ════ BEFORE 1.9.0 — HCE line, on `development` ════

- [x] **Phase 8.5 (pre-1.9.0 part) — strengthen the final HCE release — DONE
  2026-07-17.** Every strength-relevant item tested to a verdict; 5 accepted
  wins ≈ +25 Elo (head `instabtm`). Only the ⭐ 1.9.0 release gauntlet remains.
  - [x] **8.5.1 cached check-geometry — ❌ REVERTED** (net NPS regression;
    scans already cheap/amortized, eager caching wastes cutoff/leaf work).
  - [x] **8.5.2 state/layout cleanup — ⏸ DEFERRED 2026-07-17 (0-Elo).** 16-bit
    `Move` is a broad NNUE-prep refactor (0 Elo, only NPS/prep) → moved to the
    NNUE line / Phase 10 where its value is realized; the EP-copy micro-opt is
    dropped (0 Elo, not worth release risk). (8.5.3 dirty-piece is POST-1.9.0.)
  - [x] **8.5.4 telemetry — ⏸ SKIPPED (0-Elo dev tool).** "Do only if a
    candidate needs it"; nothing did. Not a release feature.
  - [x] **Track D — durable, eval-agnostic, no-re-fit strength — DONE (all
    sub-items resolved: TT-density ✅, cuckoo ❌, TT-PV → 10.7, history v2 → 10.7,
    root/TM ✅):**
    - [x] **8.5.D1 TT density & replacement (was 10.3) — ✅ ACCEPTED 2026-07-15**
      (SPRT vs `phase86-matedrive`, stopped by decision at 13,336 games:
      **+4.27 ± 3.82**, nElo +6.61, LOS **98.6%**, LLR +1.69 → accept; entire
      CI > 0, above elo1=3, durable textbook win). New head = `ttdensity`.
      32-byte partial-key cluster, ~2× entries. **Fast-TC is a conservative
      read — density helps more at longer TC/large hash; folded into the
      release-time cumulative-vs-1.8.0 LTC gauntlet rather than a separate run.**
    - [x] **8.5.13 cuckoo upcoming-repetition — ❌ TESTED & REVERTED 2026-07-17.**
      Implemented correctly (3668-entry delta table, `has_upcoming_repetition`
      with path/side/`is_legal` validation; CTest 11/11, bench +16% legitimate).
      SPRT'd two ways, both wash-to-negative: SuperGM `10+0.1` **−4.58 ± 8.46**
      (→ H0, stopped); UHO `3+0.03` **−1.64** (drifting → H0). Forces draws but
      doesn't convert to Elo vs a near-equal opponent, and the +16% node cost is
      always paid. Reverted (`git revert fac536b`); head stays
      `exacthist-rewardonly`. Not needed for correctness (normal rep detection
      already fires one ply later). See PLAN §8.5.13.
    - [x] **8.5.7 TT-PV bit — ❌ re-tested & re-reverted 2026-07-17; → 10.7
      sub-project (not a pre-1.9.0 flip).** Re-applied SF `genBound8` (steal
      `flag_age` bit 2, 32 generations) on the `instabtm` head, CTest 11/11.
      Measured the real node impact cleanly with a **fresh-TT fixed-depth search**
      (avoids the shared-TT gen-wrap that makes bench unreliable here): depth-18
      startpos **3.90M vs 2.57M nodes = +51%**. The persisted bit makes `tt_pv`
      common, and the only consumer (`lmr_tt_pv_adj=23`, reduce LMR on tt_pv)
      then over-widens massively — and there's **no good operating point** (knob
      big enough to matter over-widens; knob small enough not to is inert). The
      reduction *is* the cost. TT-PV's real value in SF is **pruning
      conservatism** (relax futility/LMP on tt_pv nodes → catch tactics on
      important lines), a *different* consumer we don't have. Doing it right =
      persisted bit + ttPv pruning-conservatism guards + joint SPSA → **10.7
      sub-project**. Reverted; head stays `instabtm`. No SPRT spent.
    - [x] **8.5.11 history-v2 (threatened-history) — ❌ BENCH-VETOED 2026-07-17
      (+167%), → 10.7.** Re-keyed `main_hist` by whether the from-square is
      attacked by an opponent pawn (`main_hist_[color][threatened][from][to]`;
      magnitude-neutral, cheap 1-shift-pair/node threat). Plumbing verified
      correct — forcing the threat bit to 0 reproduces the head exactly
      (11941440) — so the +167% is the *real* effect: splitting main-history by a
      per-position-varying context **fragments** the signal (a move's history
      scatters across buckets by a context that mostly doesn't determine its
      quality). Any further history-v2 context-dimension fragments the same way.
      Verdict: history-v2 needs bigger tables / more data / careful design →
      10.7, not a pre-1.9.0 flip. Reverted; head `instabtm`.
    - [x] **8.5.12 root/TM inputs — instability-extension slice ✅ ACCEPTED
      +10.79 (below).** The per-move variance / uncertainty-aware-aspiration /
      full-root-effort refactor remains as a later extension (also the Phase-11
      voting input) — not needed for 1.9.0.
      - [x] **instability-extension TM — ✅ ACCEPTED 2026-07-17. New head
        `instabtm` (bench 11941440).** The TM shrank time when the best move was
        stable but never *extended* when it thrashed. Added a decaying
        `best_move_changes` signal (SF's totBestMoveChanges: ×0.5/iter, +1 per
        flip) and a `tm_instability` knob (35): threshold ×= 1 + min(changes,2)·
        0.35, so a thrashing root buys up to +70% time. Bench identical (inert at
        fixed depth), CTest 11/11 → SPRT was the only judge. SPRT vs `evaldiff`
        (UHO 3+0.03): **+10.79 ± 6.13, nElo +17.20, LOS 99.97%, LLR +2.95 → H1 @
        4862 games** — the **largest single pre-1.9.0 win**. Refutes the Phase-5
        "TM at ceiling" call: the ceiling was for the *existing* signals; adding
        the missing instability-*extension* opened real Elo. Durable/NNUE-agnostic
        (pure TM). Engine `basilisk-phase8512-instabtm-pext-pgo`. (The per-move
        variance/aspiration refactor remains as a later 8.5.12 extension.)
  - **RETRY candidates re-tested 2026-07-15 against the robust canaries — the
    redesign correctly distinguishes benign from real:**
    - [x] **8.4 rule-50 damping curve — ✅ ACCEPTED 2026-07-15** (SPRT vs
      `ttdensity`, stopped by decision at 27,242 games: **+3.29 ± 2.68**, nElo
      +5.06, LOS **99.2%**, LLR +2.41 → accept; the early descending trend
      reversed and firmed above +3). New head = `rule50-retry` (commit
      `cd61288`). **Was reverted at run #4 purely on the brittle canary** — the
      canary-fix thesis vindicated: +4.27 (TT-density) + +3.29 (8.4) ≈ **+7.5
      Elo** recovered on the "exhausted" HCE base. NB: 8.4 is HCE-eval-specific
      (NNUE will re-decide the damping curve), so it does not carry to the NNUE
      line — pure pre-1.9.0 strength.
    - [x] **8.5.5 correction-history — ❌ CORRECTLY REJECTED by the *robust*
      canary (not a canary victim).** Re-applied → still fails the robust
      test_search mate gate: the engine returns a corrupted `cp 31851` for a
      KQK that is mate-in-5 (correction-history corrupts near-mate scores) — a
      **real** regression, not trajectory brittleness. Reverted. Would need a
      corrhist fix (bound the correction away from mate range) before any retry.
      *This validates the redesign: it allowed 8.4 and blocked 8.5.5.*
    - [x] **Inert-knob re-examination — DONE 2026-07-17 (none gain pre-1.9.0):**
      - [x] **PostLmrHistScale 0→104 (hcefinal SPSA value) — ❌ WASH, reverted
        to 0.** Excluded from the hcefinal bake on the brittle KBNK canary;
        passes the robust canary 11/11, so re-tested vs `rule50-retry`: **+0.87
        ± 4.92, LLR ≈ 0 @ ~8k** — a marginal SPSA dimension contributes ~0 when
        re-added to an already-baked head (SPSA non-additivity). Reverted; value
        is re-decided at 10.7 post-NNUE, so no durable reason to keep a neutral
        HCE-tuned change. Head stays `rule50-retry`.
      - [x] **Inert-knob batch re-examined 2026-07-17 vs the robust canary — NO
        pre-1.9.0 gain; all are 10.7 joint-SPSA material.**
        - **capture futility (cap_fut_depth):** already SPRT'd −2.78 at value 7
          (wash-loss); a one-off re-enable just re-confirms. → 10.7 SPSA.
        - **singular double-ext cap (double_ext_max):** robust canary NOW allows
          it (CTest 11/11 at cap 6 — the old "broke KBNK" was brittle-canary
          over-fire), **but capping HURTS**: cap 6 → bench +18.5%, cap 12 →
          +30%, both above the uncapped head. Basilisk's double-extensions are
          *productive*, so there's no explosion to cap — keep it inert (200).
          Bench-vetoed.
        - **SEE-quiet (quiet_see_depth):** naive enable broke KBNK completely;
          needs the history-aware `lmr_depth` wired first → not a flip, → 10.7.
        - **cont-hist6:** a magnitude-adding continuation table (same class as
          the (a)/(c)/(d) bench-vetoes) and needs the ss-array sentinels expanded
          4→6; predicted veto as a one-off → 10.7 joint SPSA.
        - **qsearch_check_cap / post_lmr_hist_scale:** already settled (SPSA-
          pinned 0 / tested WASH).
    - [x] **8.5.10(b) exact/PV best-move history training — ❌ REJECTED
      2026-07-16, reverted.** Reused `update_all_histories` at exact nodes
      (`best_score > orig_alpha && best_score < beta`). CTest 11/11, bench 8329726
      (−26% nodes), but SPRT vs `rule50-retry`: **−84.21 ± 18.85 Elo, LOS 0%, H0
      @ 652 games.** The −26% node drop was ordering going *sharper but wrong*:
      exact nodes search every move, and the reused updater maluses every
      non-best sibling — earned on a cutoff, but at an exact node best-vs-second
      is a few cp, so blanket-maluing siblings poisons history. Head stays
      `rule50-retry` (bench 11251808). **Retry idea (separate candidate):**
      reward-only at exact nodes, no sibling malus. Next rungs: (a) TT-cutoff
      quiet rewards, (c) failed-capture maluses, (d) fail-low countermove, (e)
      static-eval-diff history.
    - [x] **8.5.10(b′) exact/PV reward-only — ✅ ACCEPTED 2026-07-16. New head
      `exacthist-rewardonly` (commit `b0b6097`).** An
      `update_all_histories(reward_only)` flag boosts only the exact-node PV
      move's graded history and skips the sibling malus, bad-capture malus, and
      killer/countermove (cutoff-only). Bench 10922796 (−2.9%). SPRT vs
      `rule50-retry`: **+4.90 ± 3.71, nElo +7.67, LOS 99.52%, LLR +2.95 → H1 @
      13,768 games.** The (b) diagnosis confirmed — the sibling malus was the
      poison (−84), reward alone is +4.9, durable/NNUE-agnostic. Engine
      `basilisk-phase8510b2-exacthist-rewardonly-pext-pgo`.
    - [x] **8.5.10(e) surprise-scaled history — ✅ ACCEPTED 2026-07-17 (by
      decision). New head `evaldiff` (bench 11941440).** `update_all_histories`
      `bonus_scale`: at a cutoff, reward 125% when `static_eval < beta` (search
      found a move the eval missed). SPRT vs `exacthist-rewardonly` (UHO 3+0.03):
      peaked +3.84, settled **+2.50 ± 3.81, LOS ~90%** @ ~13.9k; LLR peaked 1.72
      then receded (true value ~+2.5 ≪ elo1=5 → H1 unreachable, accept by
      decision). Small but real, durable/NNUE-agnostic. Engine
      `basilisk-phase8510e-evaldiff-pext-pgo`. Second clean pre-1.9.0 win.
    - [x] **8.5.10(a)/(c) — ❌ BENCH-VETOED (no SPRT).** (a) TT-cutoff reward
      +82% nodes (main-hist over-generalisation); (c) all-capture maluses +30%
      (degrades capture ordering). Both are table-wide magnitude changes needing
      consumer retune → 10.7 joint SPSA, not pre-1.9.0 flips. See PLAN §8.5.10.
    - [x] **8.5.10(d) fail-low prior-move continuation bonus — ⏸ DEFERRED
      2026-07-16, reverted before SPRT (bench veto).** SF's "prior countermove
      that caused the fail low." Bench killed it before any SPRT: full bonus
      +60% nodes, /4 +41%, /4 + meaningful-drop gate (`best_score ≤ static_eval
      − 50`, fail-low only) still +22%. Our gravity-capped cont tables lack SF's
      counterbalancing prior-move malus on fail-high refutations, so a one-sided
      prior bonus saturates entries and flattens ordering. **It's a cont-hist
      rebalancing sub-project (bonus on restrict + malus on refute, SPSA-tuned
      jointly), not a clean rung** → requeued under 8.5.11 / 10.7. Remaining
      clean rungs: (a) TT-cutoff quiet reward, (e) static-eval-diff history.
- [x] **⭐ 1.9.0 RELEASE — ✅ DONE 2026-07-17.** Version → 1.9.0 (Constants.h +
  CMakeLists), CHANGELOG `[1.9.0]`, all Phase 8 + 8.5 work squashed to `master`
  as the single `Version 1.9.0` commit (no co-author), CTest 11/11, UCI reports
  `Basilisk 1.9.0`. `development` reset to this `master` state to continue.
  **Still manual (user):** tag `v1.9.0` + push `master` (PGO `release.yml`
  assets + manifests fire on the tag); optional cumulative `instabtm`-vs-1.8.0
  confirmation gauntlet (fast + `10+0.1`) for the shipped number. This is the
  last pure-HCE release and the frozen HCE baseline the NNUE net is measured
  against (8.5.15).

### ════ AFTER 1.9.0 — NNUE line ════

- [ ] **Phase 8.5 (post-1.9.0 NNUE runway) — on `development`, then rebase:**
  - [ ] **8.5.3 dirty-piece contract** (accumulator input data; 0 Elo).
  - [ ] **8.5.14 TT graph-history semantics** (eval-adjacent parts easier post-NNUE).
  - [ ] **8.5.15 frozen teacher benchmark** (baselines the *released* 1.9.0 HCE).
  - [ ] **8.5.16 `net_trainer` preflight** (split/dedup/filter/manifest/tests).
  - [ ] **Handoff:** record exact `development` SHA; rebase `nnue` once.
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
    SIMD parity, supported ISA kernels and new PGO profile.
  - [ ] **9.6:** net versus final Phase-8.5 HCE at STC, `10+0.1`, frozen
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
> done — so the plan keeps them after NNUE.

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

# Non-inferiority / phase-boundary confirmation
.\tools\sprt.ps1 ... -Elo0 -3 -Elo1 0
.\tools\sprt.ps1 ... -TC "10+0.1"

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
release notes. The user squashes `development` to `master`, pushes and tags.
The tag workflow must upload the same documented production PGO/ISA assets that
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
