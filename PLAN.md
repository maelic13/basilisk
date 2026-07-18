# Basilisk Strength Improvement Plan

> **CURRENT STATE (2026-07-17):** **1.9.0 RELEASED** — the last pure-HCE release,
> squashed to `master` as the single `Version 1.9.0` commit. It bundles **Phase 8**
> (correctness & infrastructure) and **Phase 8.5** (pre-NNUE strength) on top of
> 1.8.0. Accepted SPRT gains: hcefinal SPSA **+35.94**, eval 8.3 **+13.97**,
> instability-TM **+10.79**, exact/PV reward-only history **+4.90**, TT density
> **+4.27**, rule-50 damping **+3.29**, mate-drive **+3.19**, surprise-scaled
> history **+2.50**, SEE pin-awareness **+0.65**. Engine reports `Basilisk 1.9.0`,
> bench **11,941,440**, CTest 11/11.
>
> **▶ NEXT:** `development` is reset to the `master` 1.9.0 state and continues from
> there — the **Phase 8.5 post-1.9.0 NNUE runway** (8.5.3 dirty-piece contract,
> 8.5.14 TT graph-history, 8.5.15 frozen-teacher benchmark, 8.5.16 `net_trainer`
> preflight), then rebase `nnue` once and resume **Phase 9: NNUE (§5)** on the
> existing `D:/code/net_trainer` Bullet/Rust/CUDA pipeline and its raw
> `quantised.bin` contract (do not resurrect the removed PyTorch/`MNN1` path).
> **Deferred to the 10.7 joint SPSA** (each over-widens/fragments as a one-off
> flip, not as a pre-1.9.0 unit): TT-PV bit (8.5.7), history-v2 (8.5.11), the (d)
> prior-move cont-hist rebalance, and the inert-knob set. Post-NNUE: **Phase 10
> search architecture + final tune, Phase 11 mandatory SMP, Phase 12 NNUE
> architecture/data iteration (§6)**. Deferred experiments also live in §6.

This plan was executed 2026-05 → 2026-07 as Phases 0–7. The step-by-step
history lives in `CHANGELOG.md` and git history; this document keeps the
**process** (how work happens, §1), the **record** (what shipped, §2–3), the
**active phases** (hardening §4, NNUE §5), the **post-NNUE roadmap + deferred
menu** (§6), and the **operational discipline** (releases §7, commands §8).

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
- **Branch discipline (simplified 2026-07-14, user decision):** candidates are
  committed **directly on `development`** — that branch is the working line and
  may carry SPRT-pending work. A rejected candidate is reverted with a recorded
  null result. (`master` still only receives squashed `Version X.Y.Z` commits.)
- **After every step:** update `PLAN.md` + `user_dev_guide.md`, commit
  **without** a co-author trailer.
- **Releases:** user squashes `development` onto `master` as a single
  `Version X.Y.Z` commit, then tags manually. See §7.

### Non-negotiable gates (apply to every change)

1. **One semantic candidate at a time** — never bundle unrelated behavior
   changes or tuned values. Mandatory correctness campaigns may share one final
   non-regression run, but every defect is still a separate commit with its own
   regression test and before/after diagnostic so a failure can be isolated.
2. **SPSA/Texel propose; SPRT decides.** Lower holdout loss that does not
   survive SPRT is overfitting; revert it. (Phase 7 proved the converse too:
   a *stalled* holdout MSE does not mean no Elo — the games are the only
   arbiter in both directions.)
3. **Use the gate that matches the claim.** A strength candidate uses the
   standard SPRT `elo0=0, elo1=3` at
   `tc=3+0.03`, Hash 64, Threads 1, `SuperGM_4mvs.pgn` book (this matches the
   SPSA TC so optima transfer). `tc=10+0.1` for phase-boundary / LTC-suspect
   confirmation. Don't react to sub-3k-game trends — early SPRT noise has
   flipped sign more than once. Mandatory correctness fixes and behavior-neutral
   enablers use correctness evidence plus a pre-registered **non-inferiority**
   bound; do not pretend that `elo1=3` decides whether an objectively wrong
   rule may remain wrong. If a mandatory fix regresses, isolate and repair or
   retune the exposed dependency.
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
   **LANDED with 8.8 (2026-07-15):** the canary suite now splits into a
   **hard correctness core** (gating: the won endgame is not misevaluated as
   a draw AND still converts to a legal checkmate under a generous budget —
   the class that reverted 8.4) and **non-gating trajectory diagnostics**
   (the exact mating-ply count / route at the fixed search depth is printed
   and tracked, never a veto) — eight consecutive standard mechanisms were
   vetoed by tree-*shape* trajectories, not by correctness; SPRT is the
   strength arbiter. Implemented in `test_endgames` (`mate_w`/`mate_b`);
   `test_invariants` adds the perft/legality/make-unmake/fuzz correctness
   gates the audit called for.
   **HARDENED 2026-07-15 — the 8.8 split was incomplete and over-fired.** Two
   fixes: (a) `endgames.epd` had **4 illegal positions** (side-not-to-move in
   check) — replaced with verified-legal ones; (b) the "converts under a
   generous budget" gate was still **single-position full conversion at fixed
   depth 18** — a search-*shape* trajectory that vetoed benign eval/search/TT
   changes (8.4/8.5.5/8.5.6/TT-density) while the eval still saw the win.
   Now the hard core is **robust and search-shape-independent**:
   eval-recognizes-win (per position) + a **conversion floor** (≥ total−1,
   tolerates one fragile trajectory) + **near-mate recognition** (KQK/KRK/KBNK
   return a mate score, incl. a KQK **stalemate trap**). `test_search`'s
   shortest-mate gate likewise changed from exact `≤5` to "found a mate
   `< 8` (not stuck on the longer one)". Exact conversion/ply/distance are
   diagnostics. **Consequence:** changes the brittle canary wrongly refused are
   **retry candidates** (8.4, 8.5.5, TT-density [accepted-canary, SPRT-pending],
   PostLmrHistScale re-bake, and re-examining the 6.x inert knobs). The
   inert-default discipline still applies only to constants that touch the
   *robust* core, which is now much smaller.
7. **Preserve the eval boundary** — search calls
   `Evaluator::evaluate(const Board&)` and nothing deeper. This is what makes
   the NNUE swap (Phase 9) possible.
8. **Every tested artifact is reproducible.** `build_test.ps1`/`sprt.ps1`
   must emit and retain a manifest containing engine SHA + dirty-diff hash,
   compiler/linker versions, complete CMake/ISA/TUNE flags, PGO profile input
   and hash, bench signature, binary SHA-256, opening-book SHA-256, random
   seed/order policy, TC/hash/threads/concurrency/adjudication/SPRT bounds, and
   fastchess version. A PGN without this manifest is not a reproducible test.
9. **Release UCI stays clean** — tuning knobs exist only behind `-DTUNE=ON`;
   releases run compiled-in defaults (accepted values are baked into headers,
   never shipped as config).

### The cost principle (§0.5, kept because it still governs §6)

- **Texel weight-fitting is CPU-minutes — run it freely.** SPSA and SPRT are
  **thousands of games each — conserved.**
- **Search constants are denominated in eval centipawns.** Any eval re-fit
  changes what a centipawn means, so a search-constant SPSA run before the
  eval is final is thrown away. This is why the search SPSA was deferred
  through Phase 7 — and why it is now a **post-NNUE** item (§6): NNUE re-scales
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
| **1.9.0** | 2026-07-17 | Phase 8 correctness/infra (rule-50/mate precedence, null clock, legal-EP hashing, SEE pin-awareness **+0.65**, eval 8.3 **+13.97**, mate-drive **+3.19**, assert_ok/fuzz/CI, robust canaries) + `hcefinal` SPSA **+35.94** + Phase 8.5 strength (TT density **+4.27**, rule-50 damping **+3.29**, exact/PV reward history **+4.90**, surprise-scaled history **+2.50**, instability-TM **+10.79**) | cumulative fast-TC SPRT gains over 1.8.0 (last pure-HCE release) |
| *(post-1.8.0)* | 2026-07-09 | self-play cycle 6: **wash** (+1.37 ± 5.21, 8.1k games) → **HCE line closed**, candidate discarded | — |
| **1.8.1 / 1.9.0** | next | **Phases 8 + 8.5 on `development`**: correctness, infra/state hardening, eval-independent search and NNUE data preparation (§4) — PATCH only for a correctness-only cut; MINOR if strength banks | — |
| **2.0.0** | future | **Phase 9 NNUE** (§5) | target **+200…+400** |
| **2.x** | future | **Phase 10** final 1T search + tune, **Phase 11** mandatory SMP, **Phase 12** NNUE architecture/data frontier loop (§6) | prior **+15–40** 1T non-additive; SMP +10–30 at 8T; NNUE scale evidence-driven |

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
9. **"Feature-complete" held for feature *names*, not activations.** The
   2026-07-01 audit (vs SF-classical / Ethereal / Weiss) found no missing term
   worth adding — that stands. But the 2026-07-13 `hce_analysis.md` audit
   (verified in-session) found three **activation defects inside existing
   terms** (OCB amplification, dead enemy-rook-behind-passer, `attacked2` pawn
   doubles — fixed in 8.3) plus a winnability block that is all-zero *and* has
   no tuning path. Durable form: adding feature names stays EV ≈ 0, **semantic
   audits of existing activations can still pay**, and the conditionality /
   representation gaps are the net's job — king-bucketed PSTs are the NNUE
   input shape, so the next structural step *is* the net.

---

## 4. Phase 8 — board/search/eval correctness & hardening (ACTIVE on `development`)

Source: `analysis/infra_analysis.md` (external audit, 2026-07-13), **verified
claim-by-claim in-session 2026-07-13** — every repro re-run against the current
head. Verification verdicts that gate this phase:

- **Real, reproduced:** rule-50 draw overrides mate-in-1 (doc §4.1; live binary
  scores `cp 0` in `7k/5Q2/5K2/8/8/8/8/8 w - - 99 1`); null move advances the
  halfmove clock (§4.2, `Board.cpp:644`); SEE accepts pinned recapturers
  (§4.3; `see(Bxc6)` = −200 where +100 is correct — fans out to all 9 `see_ge`
  call sites: staging, qsearch/main SEE pruning, ProbCut, LMR classification);
  EP square hashed when the EP capture is illegal (§4.5, missed-repetition
  identity); history capacity 1024 is assert-only (§4.6).
- **REFUTED — do not "fix":** doc §4.4 (SEE king recaptures). The
  `KING = 20000` sentinel + minimax fold handles every king case correctly
  (verified empirically on all three scenarios). Any explicit king branch
  would be dead code at best.
- **Doc errors to remember:** §6.4 is wrong that no TT prefetch exists
  (`search.cpp:1562` prefetches the child entry); §5.3's cost claim is inert
  at defaults (`qsearch_check_cap = 0` gates the whole quiet-check loop off).

The same-day **search audit** (`analysis/search_analysis.md`) was verified to
the same standard 2026-07-13: every checkable claim confirmed against the
code, and all three of its measurements reproduced exactly on this machine
(bench 12,661,251 / EBF 2.829 / median 253,845; 1T depth-17 startpos 658,707
nodes; the rule-50 FEN scoring `cp 0` on the 1.8.0 binary — the same §4.1 bug
both audits found independently). Verdicts that shape this phase:

- **Additional real defects → folded into 8.1:** qsearch returns *static eval
  while in check* at the 10-ply qsearch cap (`search.cpp:1022` — an unsound
  bound that can mask mates in long check chains and poisons parent TT
  stores); qsearch delta pruning returns bare `alpha`, discarding the
  fail-soft best value (`search.cpp:1072` — degrades exactly the TT-bound
  quality the measured +7.18 Step-6.1 gain relies on).
- **Doc nuance to remember:** its §10 repro wording is off — `g6h5` *is* the
  mate-in-1 (Qh5#); the engine plays it by ordering luck while *scoring* it
  0.00. The defect is the score, not the move choice.
- **Our stale comment confirmed:** current SF does **not** search quiet
  checks in qsearch (the `SearchParams.h` 6.8 comment tracked an older SF) —
  bears on 8.9 and weakens the `QsearchCheckCap` prior in the hcefinal SPSA.
- **Recommendation split adopted here:** eval-independent structural items →
  the **Phase 8.5 search ladder on `development`**; cp-denominated /
  architecture-heavy items → **Phase 10 (§6)**; SMP → **Phase 11 (§6)**.
  Its Elo priors overlap heavily — never sum the rows.

The third same-day audit (`analysis/hce_analysis.md`, the HCE evaluator) was
verified to the same standard 2026-07-13: every claim checked against the
source — evaluator, `EvalParams.h` values, both tuner paths, extraction /
datagen / import scripts, tests, and the SPSA configs. Verdicts that shape
this phase:

- **Real, verified eval defects → step 8.3:** (1) the OCB "draw" scaler
  **amplifies** the whole eval above four total pawns (`eval.cpp:384`,
  `scale = 32 + 4·pawns` over 48 → ×2.0 at 16 pawns) — and it is *worse than
  the doc claims*: the rule is not phase-gated, it fires in any position where
  each side has exactly one bishop and they stand on opposite colours;
  (2) the enemy-rook-behind-passer penalty (live weights 10/20) is nested
  inside the friendly-rook loop (`eval.cpp:1139`), so it fires **only when we
  also have a rook on the passer's file** and double-counts with stacked
  rooks; (3) `attacked2` never records two-pawn double attacks
  (`eval.cpp:779`) and its "no consumer depends on this" comment is stale.
- **Verified process facts:** all seven winnability params are zero **and
  untunable** — the block is untraced (zero linear gradient), the FD tuner
  covers only KS knobs, no SPSA config exposes them; the `EvalParams.h`
  "tuned via the finite-difference path in Phase 4.5" comment describes a
  path that was never built. The KS fit is degenerate-looking (`ks_unit`
  B/R/Q = 0; `storm_weight_kf = −1` *rewards* the defender) — identifiability
  symptoms of the single-axis tuner, not chess conclusions.
- **Doc errors to remember:** its `attacked2` consumer list is half wrong —
  the weak-king-ring term is provably unaffected (`~attacked_by[us][PAWN]`
  already excludes every pawn-covered square) and king-flank pressure is
  inert (`ks_flank_attack = 0`); only the threats package is live-affected.
  Its hedge that "current SPSA configuration may expose" winnability params
  is false (`config_hcefinal.json` is search-only). Its Elo priors skew high
  against this project's measured history — the Texel constants were fitted
  *with* these bugs present (the tuner duplicates the OCB formula at
  `tuner.cpp:354`), so fixes are SPRT questions, not free Elo.
- **Recommendation split adopted here:** verified eval bugs → **8.3**; cheap
  eval-semantics repairs → **8.4/8.6**; teacher benchmark + data hygiene
  (NNUE-enabling) → **Phase 8.5 Track C**; the big HCE reworks (king safety,
  winnability model, threat/passer semantics, phase buckets) remain a closed
  Phase-12 fallback menu — NNUE learns those relationships directly, so they
  are not the default frontier path.

**Sequencing:** 8.1/8.2/8.3/8.4/8.6 change bench and engine behaviour — do
**not** land them while the hcefinal SPSA is in flight; land after its verdict
is baked or discarded. 8.4 and 8.6 run **after 8.3 is merged** (they perturb
terms the bundle touches, and the post-bundle head is their honest baseline).
8.7/8.8 are engine-content-neutral and can go any time.

> **✅ hcefinal SPSA pre-step DONE 2026-07-14 — ACCEPTED +35.94 ± 9.42 (H1,
> LOS 100%, 2,270 games, Ptnml [30,216,462,344,83]), merged `7c3a857`, bench
> 12,661,251 → 15,008,100. The largest single tune in project history**
> (repaired protocol vindicated: validated-defaults start, joint dims, 1,894
> iterations). Landed vector: asymmetric-linear history (BonusLin 120 /
> MalusLin 143 / MalusMax 1304) + rescaled consumers (HistPruneCoeff 14004,
> LmrHistDiv 5683) + live LMR context (CutNode 401, TtCapture 301,
> NotImproving 89, TtPv 23) + HistTtMoveBonus 29 + CapFut 1/198/283.
> **One documented exclusion:** the raw optimum's `PostLmrHistScale=104`
> (≈ Weiss full weight) failed the KBNK *correctness core* — no conversion
> inside the 50-move rule even at 200ms/move — matching 6.4's solo
> measurement of that mechanism; excluded (0), the 16-dim remainder passes
> 9/9 CTest incl. the full KBNK playout. **`QsearchCheckCap` baked at 0 →
> step 8.9 resolves to SKIP.** `LmrTtPvAdj=23` is near-noise against the weak
> reconstructed tt_pv signal — 8.5's TT-PV bit re-checks it. Phase 8 starts
> from this head; 8.1's non-inferiority baseline = `hcefinal-tuned`.

Steps (each = own candidate branch, §1 gates apply):

1. **8.1 Correctness campaign** — separate semantic commits, followed by one
   combined **non-inferiority SPRT (`-Elo0 -3 -Elo1 0`)**:
   rule-50/mate precedence (draw at clock ≥ 100 only if not in check or a
   legal move exists, SF-style — fix **both** `is_draw` overloads: the search
   path *and* the no-arg overload the Syzygy DTZ walk uses); stop
   `halfmove_clock++` in `make_null_move` (reset only `plies_from_null`);
   set/hash `ep_sq` only when at least one **legal** EP capture exists;
   replace the release-unsafe fixed game-history limit with checked/growable
   pre-search history (never silently clamp an undo index). From the search audit:
   **qsearch in-check cap fix** (never return static eval while in check —
   drop the 10-ply cap for evasion nodes, `MAX_PLY` remains the guard) and
   **fail-soft qsearch** (preserve the best futility-derived value through
   delta/SEE pruning instead of returning bare `alpha`); rider: correct the
   stale `SearchParams.h` 6.8 comment (SF no longer does qsearch quiet
   checks). Regression tests for every reproduced position (infra §4.1–4.6
   test lists; FENs at clock 99/100/101; a deep check-chain qsearch case).
   Every subfix gets its own regression and commit. Expected Elo ≈ 0 — these
   are correctness, mandatory before further strength work; if the combined
   run fails non-inferiority, bisect the commits rather than reverting the bugs.
2. **8.2 SEE pin legality** — separate change, **SPRT run #2
   (non-inferiority first; report the ordinary Elo interval too)**: exclude
   absolutely pinned attackers in `see()`/`see_ge()`
   (pinned-along-ray captures stay legal; pinner-removed x-ray cases per doc
   §4.4 test list). Add the slow independent legal-exchange oracle test —
   the existing `see_ge ↔ see` test only proves the two share bugs. If the
   run is flat, accept after confirming non-inferiority. Honest expectation:
   **+0…+4** fast-TC.
3. **8.3 Eval-correctness campaign (HCE audit)** — three semantic commits,
   each with activation counts and focused tests, then **SPRT run #3 against
   the combined corrected evaluator**. Correct semantics are mandatory; the
   SPRT determines whether dependent weights need refitting, not whether the
   defects are restored:
   **OCB scaling cap** — minimal fix `scale = min(48, 32 + 4·total_pawns)`
   (draw scaler can never amplify; behaviour identical at ≤ 4 pawns), and
   **co-fix the duplicated formula in the tuner's `linear_delta_scale`
   (`tuner.cpp:354`) in the same commit** or the `--verify` reconstruction
   breaks; **enemy-rook-behind-passer decoupling** — independent pass keyed
   off passers so the penalty fires without a friendly rook on the file and
   never double-counts stacked rooks (note: the 10/20 weights were fitted
   while the feature almost never fired — the SPRT arbitrates the
   re-activated term); **`attacked2` pawn seeding** — seed left/right pawn
   attack sets separately so two-pawn double attacks are recorded (live
   effect is threats-package classification only — see the verification
   note above). Riders (0-Elo hygiene, same series): delete the false
   "tuned via FD path" winnability comment, the stale `attacked2`
   "no consumer" comment, the stale 2/3 no-queen-scaling comments. Tests:
   OCB never amplifies at any pawn count + sign preserved + same-colour
   bishops untouched; enemy rook behind passer **without** a friendly rook
   (+ mirrored + stacked-rook dedupe); two pawns converging on one square
   enter `attacked2`. Honest expectation: **+0…+8 bundled** — the fit
   ratified these bugs, so a wash is a real outcome; flat = keep as
   correctness.
4. **8.4 Rule-50 eval damping curve** — own branch after 8.3 merges, **SPRT
   run #4 (standard gate; keep-or-revert)**: the eval damps
   `score · (100 − clock)/100` from clock 1 — only 1% retained at clock 99
   where SF-era practice retains ~50% (`1 − clock/199`-style). One-line
   change in a hot path that shapes RFP/null/stand-pat through static eval;
   genuinely unstudied here. Try the gentler curve (or damp only above a
   clock threshold); prior **0…+4**, cheap to ask.
5. **8.6 ("8.5" stays skipped so the *Phase 8.5* label below remains
   unambiguous) Mate-drive gating** — own branch after 8.3 merges, **SPRT
   run #5 (standard gate; keep-or-revert)**: the frozen mate-drive
   (`eval.cpp:1404`) adds up to ~80 eg-cp of push-the-king-to-the-edge
   geometry in *any* phase ≤ 6 position with |eval| > 200 — including rook
   endings and pawn races where that geometry is wrong. Gate it to material
   signatures where mate-driving is valid (defender bare or pawnless,
   attacker with mating material); the KBNK override stays intact. Prior
   **0…+4**.
6. **8.7 Shipped-binary strength and ISA contract** (no games; measured bench
   + NPS + exact-artifact smoke): define coherent, accurately named release
   tiers rather than independent PEXT/AVX2 toggles — portable x86-64,
   x86-64-v2/POPCNT, x86-64 AVX2+BMI2/PEXT, and aarch64/NEON where CI support
   exists. Make `release.yml` build **PGO** assets from the exact revision and
   production flags, run CTest + bench on the files that will be uploaded, and
   publish their manifests/checksums. Regenerate PGO after NNUE so the profile
   exercises accumulator update, refresh and inference paths. The audit's
   5–10% public-binary estimate is a hypothesis: record actual per-tier NPS,
   binary size and supported CPU contract; do not promise an unmeasured gain.
7. **8.8 CI, invariants, fuzz/property tests, benchmark repair & canary split**
   (0 direct Elo, high safety/velocity value):
   - Push/PR Linux + Windows Clang Release builds, CTest and bench signature.
   - Linux Debug ASan+UBSan; nightly GCC/Clang, deeper perft and randomized
     differential runs; aarch64 when a runner is available.
   - Add debug-only `Board::assert_ok()` covering mailbox/bitboards, occupancy,
     kings, castling/EP, hashes, rule-50/repetition and cached StateInfo.
   - Reproducible random-walk make/unmake property tests with full unwind and
     independent differential perft/move sets.
   - Fuzz FEN/UCI parsing, move sequences, TT move decoding, EP/castling and
     SEE against the slow legal-exchange oracle.
   - Repair `board_performance`: no Board copies inside timed regions, replace
     the trivial cached-check benchmark, use warm-up + 7–15 samples, median +
     MAD/CI, affinity where available, and add staged generation, SEE, EP,
     make/unmake and full-search NPS workloads.
   - Make `build_test.ps1` and `sprt.ps1` produce the §1 artifact/test manifest.
   This is the class of testing that would have caught 8.1/8.2:
   **253/253 existing board tests passed while three real bugs existed.**
   Finally apply the **canary policy split** (search doc §14): KBNK/KQK keep
   a **hard correctness core** (endgame still won/converted under generous
   limits, legal play, no false draws, mate recognized) while the exact
   fixed-depth mating-ply/route expectations become **non-gating
   diagnostics** (printed, tracked, never a veto). Rationale: eight
   consecutive standard mechanisms (6.2–6.8, H2) were vetoed by tree-*shape*
   trajectories, not correctness; SPRT is the strength arbiter. Fold the 8.3
   eval-semantic tests into the CTest suite here. This step is the
   prerequisite for 8.5.8/8.5.9 below; §1 gate 6 is updated when it lands.
8. **8.9 (LAST) Conditional direct quiet-check generation** (doc §5.3):
   **gate on the hcefinal SPSA outcome** — if `QsearchCheckCap` baked at 0,
   the `gen_quiet_checks()` path is provably inert → **skip, close the item**.
   If it baked > 0, first measure the generate-all-then-filter cost in the
   live path (qply==0 nodes only); implement direct generation (check-squares
   + discovered-check masks) only if the cost is measurable, then SPRT.
   Search-audit prior: current SF does **not** search quiet checks in
   qsearch, so the expected outcome is *skip* — the SPSA verdict decides.

### Phase 8.5 — NNUE-neutral board/search completion and data preparation (`development` ONLY)

**Branch rule, fixed by the user on 2026-07-14:** all Basilisk Phase-8.5 work
lands on `development`. The `nnue` branch remains frozen while 8.5 runs. When
8.5 is complete and the accepted HCE head is tagged/recorded, rebase `nnue`
**once** onto that exact `development` SHA and begin Phase 9. Do not implement
parallel copies of StateInfo/search changes on `nnue`, and do not merge partial
NNUE code back into `development`.

Phase 8.5 contains only work that remains useful after the evaluator swap.
Actual Bullet `quantised.bin` loading, accumulator storage/update and SIMD
inference stay in Phase 9 because those already exist in partial form on
`nnue`. Each behavior
candidate gets its own branch and matching SPRT/non-inferiority gate. Pure
refactors/enablers must be bench-identical and pass the strengthened 8.8 tests;
they are not discarded merely for producing 0 Elo alone when a documented
downstream phase requires them.

**RESHUFFLED 2026-07-15 (user decision) — Elo before the release, NNUE runway
after.** The original linear 8.5.1→8.5.16 order made the 1.9.0 release wait on
pure NNUE data prep that adds no Elo to the shipped engine. New order:

1. **Pre-1.9.0 (all the strength + the enablers it rests on):** `8.5.1` state
   substrate, then **all of Track B** (`8.5.4`–`8.5.14`, the Elo, each
   SPRT-gated), with `8.5.2` NPS cleanup folded in where convenient. This
   makes the released head the **final accepted HCE head**.
2. **Cut 1.9.0** — measure the cumulative `development`-vs-1.8.0 gain
   (gauntlet fast + LTC spot check), bump version, CHANGELOG, then the manual
   user tag/push via the 8.7 PGO `release.yml`.
3. **Post-1.9.0 NNUE runway (0 Elo, done as one contiguous block feeding
   Phase 9):** `8.5.3` dirty-piece contract → `8.5.15` frozen teacher
   benchmark (baselines the **released** final HCE head — consistent only
   because no Elo work is left for "after") → `8.5.16` `net_trainer` preflight
   → `nnue` rebase → Phase 9.

Rule that keeps 8.5.15 coherent: **no Elo-bearing (Track B) work is deferred
past the release** — only the three pure-NNUE items (`8.5.3`, `8.5.15`,
`8.5.16`) land after it. The sub-steps below keep their numbers; the tags
`[PRE-1.9.0]` / `[POST-1.9.0 NNUE runway]` record which side of the release
each falls on.

#### Track A — position/state architecture

1. **8.5.1 [PRE-1.9.0] Per-ply `StateInfo` + cached check geometry:** separate physical
   position from reversible per-ply state; cache checkers, blockers/pinners for
   both kings and check squares once per child. Reuse them across capture/quiet
   move-generation stages; pass the already known `gives_check` result into
   make-move so non-checking children use the zero-checker fast path. Gate:
   8.8 invariants/random walks, unchanged legal move sets and fixed-depth
   nodes, then NPS and SPRT if behavior changes.
2. **8.5.2 State/layout cleanup:** remove EP TT-legality Board copies using the
   shared occupancy legality helper; use dynamic pre-search game history plus
   a bounded per-thread search `StateInfo` stack; convert `Move` to 16 bits if
   full-search benchmarks confirm the layout; preserve a scalar/portable path.
3. **8.5.3 [POST-1.9.0 NNUE runway] NNUE-neutral dirty-piece contract:** every real move records exact
   removed/added piece-square pairs for quiets, captures, EP, promotions and
   castling; null move records no piece delta. This is data only—no network
   weights or accumulator in `development`. Extend the 8.8 random walker to
   reconstruct the child board from each delta. Phase 9 consumes this contract
   after the rebase.

#### Track B — search observability and eval-independent architecture

4. **8.5.4 Search telemetry + benchmark baseline (no Elo):** add a disabled-by-
   default diagnostic mode counting PV/cut/all nodes, in-check nodes and check
   chains, every prune attempt/cutoff, LMR size/reduced cutoff/re-search result,
   TT probe/hit/bound/PV/replacement data, qsearch captures/evasions/max depth,
   null/ProbCut verification, root nodes per move/thread, and thread overlap.
   Capture a mixed fixed-node corpus baseline, not only startpos/total bench.
5. **8.5.5 Correction-history update semantics:** exclude capture/promotion
   tactical outcomes; update only when the bound direction is informative
   relative to raw static eval, including eligible fail-low evidence. → SPRT.
   **ATTEMPTED 2026-07-15 → BREAKS the KBNK conversion canary (hard gate),
   not sent to SPRT.** Both the informative-incl-fail-low condition and the
   tactical-exclusion-only bisect leave KBNK non-converging at the depth-18
   playout — correction-history is *eval-adjacent*, and any perturbation of
   the won-endgame static eval breaks the fragile frozen HCE mate-drive
   (same class that reverted 8.4). **Blocked by HCE mate-drive fragility;
   naturally unblocked once Phase 9 NNUE replaces the HCE eval.** Reverted.
6. **8.5.6 Qsearch in-check upgrade:** order evasions by TT move and contextual
   history, and store completed in-check qsearch bounds in TT. → SPRT.
   **ATTEMPTED 2026-07-15 → both parts negative on the bench (fixed-depth node
   count), not sent to SPRT:** the generator already emits king-moves-first,
   which cuts off *better* than either `score_moves` (capture-first: +54%
   nodes) or TT-move-first (+11%); and the TT store alone adds ~+25% because
   in-check evasion children are *non-check* qsearch nodes that return the
   non-provable fail-hard bounds documented at the 8.1f trap — aggregating
   them into a stored bound poisons later cutoffs. **The store part is BLOCKED
   on Phase 10.4 qsearch bound-shaping** (provable qsearch bounds) and cannot
   land safely before it. Reverted to the 8.1e baseline. Revisit the store
   only after 10.4; the reordering is simply not a win here.
7. **8.5.7 Persistent TT-PV bit:** store/propagate PV ancestry through all
   eligible TT bounds rather than reconstructing it only from deep exact
   entries. It is an enabler for Phase 10 even if neutral alone; validate age/
   replacement behavior explicitly. → non-inferiority + ordinary Elo report.
   - **Prototyped 2026-07-16 (SF `genBound8`) then reverted — NOT a clean flip;
     do it as bit-infra + companion re-tune.** Implemented correctly: stole
     `flag_age` bit 2 for the PV flag, age → 5 bits (32 generations, step 8,
     mask 0xF8, wrap handled), stored/probed at all sites, `ss->tt_pv` from the
     persisted bit with a singular-search guard. Two problems surfaced: **(1)**
     the persisted bit makes `tt_pv` *common* (vs the old rare TT_EXACT
     approximation), so the existing `lmr_tt_pv_adj=23` — tuned when the signal
     was rare — now widens the tree ~+19% nodes; the knob **must** be re-tuned
     with the bit (as its own param comment always warned). **(2)** halving
     generations 64→32 makes the 40-position shared-TT **bench wrap at pos 33**
     (`tt.clear()` is per-repeat, not per-position), inflating bench ~+13% as a
     pure artifact → **bench is an unreliable pre-filter for this change.**
     Verdict: land TT-PV as *enable-bit + SPSA the `tt_pv`-gated LMR/pruning
     knobs together* (and add SF-style `ttPv` pruning-conservatism consumers),
     not a one-shot flip. 32 generations is standard (SF) and fine for real
     play — the bench wrap is not a game regression. Requeue with 10.7 SPSA or a
     dedicated tune. Deferred in favor of the genuinely-clean cuckoo (8.5.13).
   - **Re-tested 2026-07-17 on the `instabtm` head → confirmed 10.7 sub-project,
     no SPRT spent.** Re-applied genBound8 (CTest 11/11) and measured the real
     node impact with a **fresh-TT fixed-depth** search (sidesteps the shared-TT
     gen-wrap that corrupts bench): depth-18 startpos **3.90M vs 2.57M = +51%
     nodes**. Confirms there is **no good LMR operating point** — the
     `lmr_tt_pv_adj` reduction *is* the cost, so any value that matters
     over-widens. TT-PV only pays via **pruning conservatism** (relax
     futility/LMP on tt_pv nodes), a consumer we don't have. Land it as
     bit + ttPv pruning guards + joint SPSA at 10.7. Reverted; head `instabtm`.
8. **8.5.8 Blanket check-extension removal:** isolated two-sided experiment,
   after 8.1 qsearch correctness and 8.8 canary split. Record telemetry,
   tactical/quiet/endgame node-budget suites and SPRT. Keep or close on games.
9. **8.5.9 Checking-move LMR under the existing model:** after the 8.5.8
   verdict, allow contextually bad late checks to reduce/SEE-prune rather than
   receive categorical protection. Do **not** make a later good-capture test
   conditional on this result; good-capture/all-move LMR belongs to 10.1 after
   the shared contextual reduction exists. → SPRT.
10. **8.5.10 History-coverage ladder:** independently test (a) TT-cutoff quiet
    rewards and prior-quiet maluses, (b) exact/PV best-move training,
    (c) searched-but-failed capture maluses, (d) fail-low refutation/
    countermove learning, and (e) static-eval-difference history. Each is its
    own candidate. No arbitrary “two washes close unrelated remaining items”
    rule; stop only when a dependency or evidence applies to that item.
    - **(a) TT-cutoff quiet reward — ❌ BENCH-VETOED 2026-07-17, no SPRT spent.**
      Rewarded the TT move (reward-only, via the `reward_only` path) on a TT beta
      cutoff. Bench exploded +82% (19951050 vs 10922796): TT beta-cutoffs are far
      more frequent than exact nodes, so crediting the (already-good) TT move at
      every one saturates the position-independent `main_hist[color][from][to]`
      across many move keys — a move great in one position gets over-ordered
      everywhere it shares from→to, degrading ordering globally. This is exactly
      why SF does not train history on TT cutoffs. Reverted before commit. Do not
      retry without per-position gating that avoids the main-hist over-generalization.
      Reused `update_all_histories` at exact nodes gated `best_score > orig_alpha
      && best_score < beta`. CTest 11/11, bench 8329726 (−26% nodes), but SPRT vs
      `rule50-retry`: **−84.21 ± 18.85 Elo, LOS 0%, H0 accepted @ 652 games.**
      **Diagnosis:** at an exact node we searched *all* moves, and
      `update_all_histories` maluses every non-best sibling. On a cutoff that
      malus is earned (those quiets failed the cutoff the best move made); at an
      exact node best-vs-second-best is often a few cp, so blanket-maluing every
      sibling poisons history → sharper-but-wrong ordering (−26% nodes, −84 Elo).
      **Refinement for a future retry (own candidate):** reward the exact-node
      best move *only*, no sibling malus (or a heavily-attenuated malus). Do not
      reuse `update_all_histories` unchanged for exact nodes.
    - **(b′) exact/PV best-move training, REWARD-ONLY — ✅ ACCEPTED 2026-07-16.
      New head `exacthist-rewardonly` (commit `b0b6097`).** The (b) refinement:
      `update_all_histories` gained a `reward_only` flag that boosts only the PV
      move's graded history (quiet/pawn/low-ply/cont, or cap-hist) and skips the
      sibling malus, the bad-capture malus, and the killer/countermove slots
      (those are cutoff semantics). Bench 10922796 (−2.9%). SPRT vs `rule50-retry`:
      **+4.90 ± 3.71 Elo, nElo +7.67, LOS 99.52%, LLR +2.95 → H1 @ 13,768 games.**
      Vindicates the (b) diagnosis exactly: the sibling malus (which drove (b) to
      −84) was the poison; reward alone is a real +4.9 gain. Durable, NNUE-agnostic
      (pure move ordering). Engine `basilisk-phase8510b2-exacthist-rewardonly-pext-pgo`.
    - **(d) fail-low prior-move continuation bonus — ⏸ DEFERRED 2026-07-16 (not
      a drop-in; reverted before SPRT, never handed off).** Added a `SearchStack
      is_capture` flag + `update_cont_for_move(ss-1, …)` to credit the prior move
      on a non-fail-high result (SF's "prior countermove that caused the fail
      low"). **Local bench was the veto — never spent an SPRT:** full depth-bonus
      → +60% nodes; /4 → +41%; /4 + meaningful-drop gate (`best_score ≤
      static_eval − 50`, fail-low only) → still +22%. Our gravity-capped cont
      tables lack SF's counterbalancing prior-move **malus on fail-high
      refutations** + SF's conditional-magnitude formula, so a one-sided prior
      bonus drifts entries toward +MAX and flattens ordering at any useful
      magnitude. **Verdict: (d) is a continuation-history *rebalancing*
      sub-project (bonus on restrict + malus on refute, jointly SPSA-tuned), not
      a clean rung.** Requeue it under 8.5.11 (history rep v2) / 10.7 SPSA, not
      here. `is_capture` infra reverted with it (no dead code).
    - **(c) searched-but-failed capture maluses — ❌ BENCH-VETOED 2026-07-17, no
      SPRT.** Tracked EVERY searched capture (not just SEE<0) so a cutoff maluses
      all non-best captures in cap_hist (SF-standard). Bench +30% (14210259):
      cap_hist feeds capture ordering (MVV·16 − atk + cap_hist), so maluing
      good-SEE captures pushes them below history-favoured quiets → capture
      ordering degrades. Reverted. Same consumer-imbalance class as (a): a
      one-off cap_hist-magnitude change without retuning the ordering consumers.
    - **(e) static-eval-difference (surprise-scaled) history — ✅ ACCEPTED
      2026-07-17 (by decision). New head `evaldiff` (bench 11941440).** Added a
      `bonus_scale` arg to `update_all_histories`; at a cutoff, boost the reward
      to 125% when the node's static eval was below beta (the search found a good
      move the eval did not credit). Bench +9.3% — "legitimate" (like cuckoo's
      node increase) but, unlike cuckoo, it converts to Elo. SPRT vs
      `exacthist-rewardonly` (UHO 3+0.03): peaked **+3.84** (~12.5k), settled
      **+2.50 ± 3.81, LOS ~90%, nElo 3.78** @ ~13.9k; LLR peaked 1.72 then
      receded to 0.74 → a genuine small win whose true value (~+2.5) sits well
      below `elo1=5`, so H1 was unreachable and it was accepted by decision.
      Durable/NNUE-agnostic (history/ordering survives the eval swap), low-risk
      (no eval/correctness touch). Engine
      `basilisk-phase8510e-evaldiff-pext-pgo`. Second clean pre-1.9.0 win after
      (b′) +4.9. The (a)/(c)/(d) magnitude-change rungs stay bench-vetoed; (e)
      differs because the 125% boost is a *bounded* nudge on a *subset* of
      cutoffs, not a table-wide magnitude shift.
    context and in-check/capture continuation context; measure table support,
    collision, saturation and aging. Test replacing fixed categorical killer/
    countermove priority with contextual evidence, or at minimum age the
    countermove table. Feed the accepted histories into both ordering and the
    future 10.1 reduction model. Staged SPRTs, never one table explosion.
12. **8.5.12 Persistent root-move model, aspiration and TM inputs:** retain per
    root move score, previous score, running mean **and variance**, bound/
    completion, seldepth, nodes and PV; sort after each root result. Then test
    per-move uncertainty-aware aspiration, repeated-fail recovery depth, and
    time management based on best-move instability plus full root effort
    distribution. Separate state refactor from behavioral candidates. This is
    also the required input for Phase 11 voting.
    - **instability-extension TM — ✅ ACCEPTED 2026-07-17. New head `instabtm`.**
      First (behavioral) slice, no state refactor: a decaying `best_move_changes`
      (SF totBestMoveChanges) + `tm_instability` knob (35) that raises the
      soft-limit threshold ×(1 + min(changes,2)·0.35) when the root best move
      thrashes — the TM previously only *shrank* time on stability, never
      *extended* on instability. Bench-inert (fixed depth), CTest 11/11 → SPRT
      only. SPRT vs `evaldiff` (UHO 3+0.03): **+10.79 ± 6.13, nElo +17.20, LOS
      99.97%, H1 @ 4862** — largest single pre-1.9.0 gain; refutes "TM at ceiling"
      (ceiling was for existing signals, not the missing extension). Durable/
      NNUE-agnostic. Engine `basilisk-phase8512-instabtm-pext-pgo`. The per-move
      variance/aspiration/root-effort refactor remains as a later extension
      (also the Phase-11 voting input).
13. **8.5.13 Upcoming repetition:** cuckoo reversible-move lookup, validated
    against a slow legal oracle and graph-history/rule-50 edge suites. → SPRT.
    - **❌ TESTED & REVERTED 2026-07-17.** Full SF/Hagen-van der Tak cuckoo was
      implemented correctly (3668-entry delta table, `has_upcoming_repetition`
      with path-clear + side-to-move + `is_legal` validation; the legality check
      proved essential — without it, restricted endgame kings gave illegal
      matched moves falsely claimed as forced draws). CTest 11/11. Bench +16%
      (12669287 vs 10922796) — legitimate (reveals draws the search prunes), not
      a bug. **SPRT'd two ways, both wash-to-negative:** SuperGM `10+0.1` **−4.58
      ± 8.46** (LLR → H0, stopped); UHO `3+0.03` **−1.64** (LLR drifting → H0).
      It forces plenty of draws (doing its job) but against a near-equal opponent
      that doesn't convert to Elo, while the +16% node cost is always paid.
      Reverted (`git revert fac536b`) — Elo-negative **and** node-costly, and not
      needed for correctness (normal repetition detection already fires, one ply
      later). Head stays `exacthist-rewardonly`. Re-open only if a future need
      (e.g. fortress/anti-3fold at LTC in a real match, or an NNUE eval that
      makes the draw-claim pay) justifies the node cost.
14. **8.5.14 TT graph-history semantics:** separately test cut-node-compatible
    TT cutoffs, TT-cutoff history feedback, a conservative near-rule-50 cutoff
    guard, and a rule-50-adjusted TT key/bucket after draw/null correctness is
    fixed. Verify partial history cannot turn mate into draw. Prefetch-before-
    make/qsearch remains a measurement candidate because main-search child TT
    prefetch already exists.

#### Track C — external benchmark and `net_trainer` data contract

15. **8.5.15 [POST-1.9.0 NNUE runway] Frozen teacher benchmark:** create an evaluator test corpus
    independent of Basilisk adjudication, split by source game/trajectory with
    an untouched test set, enriched for endgames, king attacks, tactical
    cliffs and quiet positional drift. Record full/lazy/corrected HCE, qsearch
    and depth-N output; report residuals by phase, material, king danger,
    halfmove clock and tacticality. Baseline the **final accepted Phase-8.5
    HCE head**, not 1.8.0, before training the release net.
16. **8.5.16 [POST-1.9.0 NNUE runway] `net_trainer` preflight (`D:/code/net_trainer`):** retain the
    current Bullet-based Rust trainer at the audited handoff (currently
    `59d190e`), its pinned Bullet revision and raw `quantised.bin` contract:
    chess768, H×2 perspectives, SCReLU and eight material output buckets. Add
    source-game train/validation/test splitting before position extraction,
    global dedup across splits, stable source/game IDs, and dataset/book/
    command/SHA/seed manifests. Replace “played move was quiet” as the sole
    tactical filter with a position-level qsearch/tactical criterion; retain
    both search-score and result labels. Add tests for extraction, conversion,
    deterministic shuffle, feature indices, all eight output buckets, raw
    payload/padding/H inference, truncation-toward-zero on negative division,
    export/load and committed conformance vectors. Record the Rust/Cargo,
    Bullet revision and CUDA toolchain needed to reproduce training. Phase 9
    adds trainer validation/resume and large-scale data.

#### Track D — durable general strength pulled forward to 1.9.0 (2026-07-15)

**Phase-number principle (user, 2026-07-15):** the number tracks the NNUE
boundary — Phase ≤ 8.5 is **before** 1.9.0/NNUE, Phase 9 **is** NNUE, Phase
≥ 10 is **strictly after**. So a general (eval-agnostic) improvement that
strengthens the final HCE release *and* carries to NNUE **without re-SPSA/
re-fit** belongs in Phase 8.5, not in the post-NNUE Phase 10. Goal: make
1.9.0 — the last HCE release — as strong as possible, as a hedge if the NNUE
project underdelivers.

Inclusion test for Track D: (a) NNUE-agnostic; (b) survives the evaluator swap
with no material re-tune/re-fit; (c) strengthens 1.9.0 and/or datagen; (d) does
**not** touch the won-endgame static eval (avoids the HCE mate-drive canary that
blocked 8.4/8.5.5).

1. **8.5.D1 TT density & replacement (pulled from old 10.3):** 32-byte
   cluster / partial-key layout for ~2× entries at equal hash, with safe
   lock-free publication and collision/replacement telemetry. Durable (a memory
   layout, not an eval constant), strengthens long-TC / large-hash play, and
   improves **fixed-node datagen label quality** (deeper search per node
   budget). → SPRT at several hash sizes and a genuinely long TC. **Top Track-D
   pick.**
2. **8.5.D2 = 8.5.13 upcoming repetition (cuckoo):** already pre-1.9.0;
   eval-agnostic, durable correctness+strength. Keep in the 1.9.0 set.
3. **8.5.D3 = 8.5.7 persistent TT-PV bit:** already pre-1.9.0; durable TT bit,
   enabler for the post-NNUE 10.1 reduction model. Non-inferiority gate.

**Eligible but lower-certainty:** the history ladder (8.5.10) and history v2
(8.5.11) are move-ordering (no eval touch → no fragility), so they can be tried
for 1.9.0; their tables/benefit are re-validated (not re-SPSA'd) after NNUE, so
durability is less certain than D1–D3.

**Optional big lever — SMP (Phase 11):** fully eval-agnostic and durable, and
would materially strengthen 1.9.0 for multi-thread users (a real hedge if NNUE
fails). But it is a large effort and does **not** accelerate datagen (which
parallelizes at the game level, not via Lazy SMP). Default: keep post-NNUE by
priority; pull to Track D only if a strong MT 1.9.0 is explicitly wanted.

**Excluded from 1.9.0** (need eval-calibrated re-tune or hit the mate-drive
fragility, so they stay in post-NNUE Phase 10): 10.1 unified reduction, 10.4
bound quality, 10.5 ProbCut/null/IIR margins, 10.6 correction-consumption v2,
10.7 final tune; and 8.5.5, 8.5.8, 8.5.9.

**Explicitly skipped on the HCE line:** incremental HCE material/PST fields,
NNUE accumulator code, refresh caches, threat inputs, and Chess960 (product
feature, no standard-chess Elo). OpenBench is not mandatory before Phase 9 on
one machine, but the manifest/result schema must be compatible with later
OpenBench adoption; frontier-scale +1–3 Elo work will eventually benefit from
persistent/distributed testing.

**Execution order across the NNUE boundary (explicit):**

1. **Pre-1.9.0 (HCE, `development`):** the Phase-8.5 pre-1.9.0 items — Track D
   (TT-density [SPRT-pending], cuckoo, TT-PV, history ladder, root/TM) and the
   **retry candidates** now unblocked by the robust canary (8.4, 8.5.5,
   PostLmrHistScale re-bake, inert-knob re-exam). Each SPRT/SPSA-gated; keep
   the accepted ones.
2. **⭐ 1.9.0 RELEASE — ✅ DONE 2026-07-17.** All accepted Phase-8/8.5 work
   squashed to `master` as the single `Version 1.9.0` commit; version → 1.9.0
   (Constants.h + CMakeLists), CHANGELOG `[1.9.0]` written, CTest 11/11, engine
   reports `Basilisk 1.9.0`. `development` is reset to this `master` state to
   continue. This is the last pure-HCE release and the frozen HCE baseline for
   8.5.15. **Still manual (user):** tag `v1.9.0` + push `master` (the PGO
   `release.yml` assets + manifests fire on the tag); optional cumulative
   `instabtm`-vs-1.8.0 confirmation gauntlet (STC + `10+0.1`) for the shipped
   number.
3. **Post-1.9.0 NNUE runway (`development` → `nnue`):** 8.5.3 dirty-piece,
   8.5.14 TT graph-history, 8.5.15 teacher benchmark, 8.5.16 trainer preflight;
   record the exact handoff SHA and **rebase `nnue` onto it once**.
4. **Phase 9 → 10 → 11 → 12** (all post-NNUE), below.

---

## 5. Phase 9 — NNUE baseline and 2.0.0 (`nnue` after the one Phase-8.5 rebase)

The strategic direction is NNUE. The +200–400 target is a hypothesis, not a
schedule commitment. The actual training implementation already exists in
**`D:/code/net_trainer`** (re-audited at `59d190e` on 2026-07-14): a Rust
trainer on pinned Bullet with CUDA training, BulletFormat packing, seeded
shuffle, a blended search-score/WDL target, cosine LR, and raw Bullet
`quantised.bin` export. The v1 network is
`chess768 -> (H×2 perspectives) -> 1×8 material buckets` with SCReLU,
H=1024 by default, QA=255, QB=64 and SCALE=400. NumPy, C++17 and Rust integer
references plus a committed H32 conformance net/vectors define exact engine
behavior. Phase 9 hardens and consumes this implementation. **Do not restore
the retired PyTorch/`MNN1` pipeline or invent a parallel trainer/format.**

Repository ownership:

| Work | Repository |
|---|---|
| Data generation/extraction, splits, BulletFormat conversion/shuffle, Rust/Bullet trainer, checkpoints and reference verification | `D:/code/net_trainer` |
| Board state, accumulator, raw `quantised.bin` loader/embedding, SIMD inference, UCI and search integration | `D:/code/basilisk` on `nnue` |
| Teacher annotation, if used | Existing Hydra tooling, imported through a versioned `net_trainer` dataset path |

The HCE comparison baseline is the **final accepted Phase-8.5 handoff SHA**,
not 1.8.0. Every network is identified by `quantised.bin` SHA-256 plus its
architecture constants, hidden size and training manifest. HCE remains
available as debug/full-recompute comparison and a
temporary UCI fallback during bring-up; the release default is the accepted
embedded net.

1. **9.0 Rebase and conformance inventory:** record the Phase-8.5 handoff SHA,
   rebase `nnue` onto it once, resolve the existing partial NNUE implementation,
   and map every retained component to `docs/nnue_format.md` at the recorded
   `net_trainer` SHA. No search/eval behavior change in this step. The v1 file
   deliberately has no magic/header: validate total size, 64-byte `bullet`
   padding, inferred/expected H, tensor dimensions and SHA before use. Future
   layouts receive a separately documented contract and conformance artifact;
   do not pretend a nonexistent `MNN1/MNN2` dispatch field is present.
2. **9.1 Harden `net_trainer` for real experiments:** add train/validation/
   untouched-test handling, validation loss and best-checkpoint selection,
   deterministic run manifests, dataset/output hashes and CI/unit tests for
   the Python data tools, Rust converter/shuffler/trainer contract and all
   three reference implementations. Replace the permissive hand-rolled CLI
   parsing: reject unknown flags, missing values and parse failures instead of
   silently using defaults; make conversion failures counted/manifested and
   fatal above a registered tolerance. Expose and record every controllable
   initialization/data seed and prove repeatability on a pilot. Implement and
   test exact resume using Bullet checkpoint APIs, including optimizer/LR/RNG/
   data-cursor state; if exact continuation cannot be proven, explicitly
   forbid resume and run each registered experiment uninterrupted. Capture
   Rust/Cargo, pinned Bullet revision, CUDA/driver/GPU and every CLI setting.
   Report training-domain float loss and exported-quantized integer loss
   separately; verify exact arithmetic on a large FEN sample, including
   truncation toward zero for negative values and accumulator/output range
   bounds. Static loss selects checkpoints within one run; it never replaces
   SPRT.
3. **9.2 Data at scale with controlled provenance:** use
   `net_trainer/tools/datagen.py` and a one-opening-per-pair diverse EPD book.
   Generate an initial 30–60M **unique** positions from the Phase-8.5 HCE head,
   retaining both PGN search scores and final WDL. Start with the trainer's
   documented `--wdl 0.3` only as a baseline; A/B label blend, node budget and
   teacher-labelled subsets independently. Include a natural-end/no-
   adjudication validation slice so the current 10cp draw/600cp resignation
   loop cannot silently define the target. Record learning curves versus data
   scale; 60M is a first serious run, not a permanent ceiling. Seed and record
   shuffle order; the built-in shuffle loads the whole file, so switch to
   Bullet's chunked shuffle tooling when the dataset exceeds available RAM.
4. **9.3 Baseline training:** train the existing
   `chess768 -> (H×2 perspectives) -> 1×8 material buckets` SCReLU model,
   H=1024 baseline for the 30–60M run (H=512 for sub-20M pilots or as a
   measured speed/strength candidate), using the Rust `net-trainer` CLI.
   Compare at least two seeds before attributing a result to architecture;
   choose checkpoints by validation,
   evaluate once on the untouched test/8.5.15 cohorts, export `quantised.bin`,
   generate vectors and pass training→exported-integer verification. Keep the
   complete manifest:
   trainer SHA, engine/datagen SHA, data/book hashes, split IDs, commands,
   seed, Rust/Cargo/Bullet/CUDA/driver/GPU versions, hyperparameters,
   checkpoint and net hash.
5. **9.4 Scalar engine integration first:** validate the raw Bullet loader and
   a full-recompute scalar evaluator against `net_trainer`'s committed H32 net
   and vectors—covering both STM colors and all eight output buckets—and a
   large generated FEN corpus with exact integer equality. Validate inferred H,
   payload order and padding. Keep `Evaluator::evaluate(const Board&)` as the
   search boundary. The release net is embedded; optional `EvalFile` is
   allowed for development/custom nets but must validate raw size/H/padding
   fully and report the active net SHA.
6. **9.5 Incremental and optimized inference:** consume 8.5.3 dirty-piece
   deltas in a per-thread/per-ply accumulator stack; cover quiet, capture, EP,
   promotion, castling, null and full unwind. Debug/sanitizer builds compare
   incremental accumulators and eval against full recomputation after every
   randomized move. Use int16 accumulators exactly as contracted, prove their
   bounds from trainer clipping and use int64 for the scalar output reference;
   do not narrow/vectorize until the committed vectors arbitrate every path.
   Add exact-parity kernels for the supported release tiers (AVX2/BMI2 and
   portable scalar; NEON when shipped),
   benchmark refresh/update/eval separately and full-search NPS, then regenerate
   the production PGO profile with the final net embedded.
7. **9.6 Swap-in and iterate:** compare NNUE against the Phase-8.5 HCE baseline
   at the standard SPRT, `10+0.1`, the frozen teacher cohorts and tactical/
   endgame suites. Iterate data amount, label blend, H=512/1024, WDL weight,
   learning rate and training duration one variable at a time until the net
   passes both game gates and
   has acceptable NPS. A failed baseline triggers contract/data/trainer/
   architecture diagnosis—not a claim that HCE is the frontier path.
8. **9.7 Provisional search-scale safety pass, not the final tune:** inspect
   score distribution, correction magnitude, pruning telemetry and tactical
   regressions. Change only margins required to prevent gross NNUE-scale
   miscalibration, as isolated SPRTs. **Do not run the comprehensive histshape/
   wave2/TM SPSA here:** Phase 10 still changes the search architecture and
   would invalidate it.
9. **9.8 Release 2.0.0:** full §7 gate against the Phase-8.5 HCE head and field
   slate, exact embedded-net SHA plus H/bucket/constants/trainer/Bullet revision
   in release metadata, production PGO/ISA assets smoke-tested, zero
   incremental/full-recompute mismatches, and STC + LTC strength stated
   separately. Phase 10 produces the final search architecture and tune for
   the next 2.x strength release.

Model: **Fable 5 high (alt: Opus 4.8 high)** for StateInfo/accumulator/SIMD and
trainer-core changes; Sonnet 5 medium for orchestration, data runs and docs.

---

## 6. Post-NNUE roadmap — execution order is Phase 10 → 11 → 12

### Phase 10 — final single-thread search architecture and tune

Phase 9.7 performs only emergency NNUE-scale safety calibration. Phase 10
builds the final search architecture **first**, then runs the comprehensive
tune **once at the end**. Search telemetry from 8.5.4 is an acceptance input
for every candidate; total nodes alone are not a verdict. Working prior:
**+15–40 at 1T**, heavily non-additive with Phase 8.5.

1. **10.1 Unified contextual reduction:** compute one signed `r` for every
   move after the first from PV/cut status, TT-PV/depth/bound/move class,
   improving/opponent-worsening, move count, check/capture state, accepted
   quiet/capture/continuation histories, correction uncertainty and prior
   reduction. Derive `lmrDepth = newDepth - r` once and reuse it for futility,
   history and SEE pruning. Stage separately: shared calculation with behavior
   parity; second-move eligibility; checks; good/bad captures; negative
   reductions/extensions; then removal of obsolete categorical exceptions.
   Each behavior step gets its own SPRT.
2. **10.2 Result-dependent verification:** choose deeper/shallower full-search
   depth from the reduced result, node confidence and prior reduction; train
   post-LMR history from both outcomes. → staged SPRTs.
3. **10.3 TT density and replacement → PULLED to pre-1.9.0 as 8.5.D1**
   (2026-07-15): it is eval-agnostic and durable, so it strengthens the final
   HCE release and carries to NNUE with no re-tune. See Phase 8.5 Track D.
   (Slot kept to preserve 10.4–10.7 references.)
4. **10.4 Bound quality:** blend RFP/qsearch proof values conservatively toward
   beta, preserve fail-soft futility bounds, and finish near-rule-50 TT cutoff
   safeguards from 8.5.14. → separate SPRTs.
5. **10.5 ProbCut/null/IIR:** staged ProbCut MovePicker with TT/capture-history/
   SEE ordering and TT-disproof skip; null-move verification min-ply region;
   audit IIR against PV/cut/all-node and TT-PV semantics. → standalone SPRTs.
6. **10.6 Correction-history consumption v2:** fit per-source weights instead
   of `/5`, add accepted 2-/4-ply continuation-correction contexts, and use
   absolute correction as uncertainty in selected margins. → staged fit +
   SPRT, with collision/support telemetry.
7. **10.7 Final search tune:** only after 10.1–10.6 are decided, generate a
   fresh configuration containing accepted histshape/wave2/correction/TM and
   pruning-margin dimensions. Exclude dead, rejected and redundant knobs;
   pre-register ranges and stop rule; SPSA → bake → CTest/telemetry → SPRT.
   Confirm at `10+0.1`, a genuinely longer TC, several hash sizes, and both
   production TUNE=OFF and test TUNE=ON binaries. This is the final tune the
   old plan incorrectly scheduled in 9.5.

**Phase-10 completion:** no known unsound bound, every search mechanism has a
telemetry/semantic test, final constants target the final net and architecture,
and 1T strength is validated against the prior 2.x head and current open-source
frontier opponents.

### Phase 11 — mandatory SMP and hardware scaling

If Basilisk ships `Threads > 1`, SMP is part of engine quality, not optional.
The local startpos depth-17 smoke measurement was **5.72× aggregate nodes for
1.49× wall-clock speedup at 8 threads**; it is diagnostic, not a general
benchmark. The ordinary 1T SPRT cannot see SMP Elo, so first build a fixed-time
paired harness at 1/2/4/8/16 threads with recorded affinity, hash per engine,
NUMA/topology and manifests.

1. **11.1 Per-thread root state:** extend 8.5.12; retain each thread's complete
   root scores/PVs/variance/nodes and completed depth.
2. **11.2 Controlled diversity:** independently test aspiration, reduction/
   depth jitter and root-order perturbation; measure root/key overlap so
   diversity is demonstrated rather than assumed.
3. **11.3 Voting and stopping:** score/depth/effort-weighted best-thread voting,
   agreement-aware soft stop, then bounded hard stop; never select solely by
   depth then score.
4. **11.4 Shared-state ownership:** define which histories/corrections are
   thread-local or shared, stop repeatedly re-blending stale helper tables,
   and test false sharing/cache-line placement.
5. **11.5 Topology and memory:** thread pinning/NUMA policy, TT sharing and
   first-touch, large-page support where portable, and scaling at realistic
   hash sizes. Preserve deterministic 1T behavior.

**Phase-11 completion:** no 1T regression; statistically positive fixed-time
strength at the supported MT targets; scaling report for 1/2/4/8/16 threads;
zero races under sanitizer/stress; exact release-binary tests on the primary
Ryzen platform and at least one different CPU family.

### Phase 12 — NNUE architecture, data and frontier loop

Chess768 is the bring-up baseline, not the expected final frontier evaluator.
NNUE remains the strategic path even if the first net underperforms; failure
triggers contract/data/training/architecture diagnosis. The HCE remains a
debug/datagen reference and optional maintenance fallback, not the default
route back to top-engine strength.

1. **12.0 Evidence review:** record STC/LTC/MT Elo, NPS, net/data learning
   curves, quantization loss and 8.5.15 cohort residuals. Select the next net
   feature from measured residuals; do not use a single arbitrary +150-Elo
   threshold to declare the evaluator finished.
2. **12.1 Versioned architecture ladder:** each layout change updates the raw
   Bullet format document, reference inference, conformance net/vectors,
   accumulator tests and scalar/SIMD parity before training at scale. Dispatch
   by an explicit engine/network contract (or separate compiled evaluator),
   not nonexistent magic in v1: (a) mirrored king-bucket inputs plus refresh/
   Finny cache; (b) pairwise multiplication plus a second small dense layer
   and its int8 quantization; (c) explicit attacker/victim threat inputs with
   measured dirty-threat propagation only if residuals support them; (d)
   direct PSQT or pawn-pair/structure inputs only when evidence justifies the
   engine complexity. Eight material output buckets are already part of v1,
   not a future milestone. Train → NPS → frozen benchmark → STC/LTC/MT SPRT
   against the accepted current net, one architecture change at a time.
3. **12.2 Data/training ladder:** scale beyond 60M according to held-out and
   Elo learning curves; A/B Basilisk versus stronger-teacher scores, search
   depth/node mix, WDL lambda, natural-end share, hard-position mining,
   endgame/king-attack/tactical cohort balance, optimizer/schedule and hidden
   size. Keep untouched test data and manifests immutable; never select a
   release net on test loss.
4. **12.3 HCE recovery menu (closed by default):** only if explicitly chosen
   for product fallback or a benchmark cohort where HCE must remain useful:
   winnability/material scaling, king safety/shelter, specialized endgames,
   passer/threat semantics, lazy-eval train/serve study and phase specialization.
   These do not replace the NNUE frontier loop.
5. **12.4 Frontier acceptance:** compare current Basilisk against full-strength
   contemporary Stockfish, Reckless, PlentyChess and at least one other current
   independent engine, using direct games where informative and calibrated
   node odds otherwise. Validate 1T and target MT, multiple hash sizes, STC,
   `10+0.1` and a genuinely long TC. Seek external-list testing (CCRL or
   equivalent) before claiming top-tier proximity.

### Deferred / reopenable experiments

(The old Phase-8 "feature menu" was closed by the 2026-07-01 audit — EV ≈ 0,
list survives in git history. The **Phase 8 label now names the 2026-07-13
hardening phase, §4**.) What remains
here is only what was **skipped, not rejected** — things with a real, if
small, chance of paying — plus the conditions under which they make sense.
Rejected-with-measurement items (6.2 cont-hist6 −7.70, capture-futility-active
−2.78, do_deeper margins, TM SPSA values) stay closed.

**The unifying rule (§1 cost principle): comprehensive search-constant tuning
happens once, in Phase 10.7, after NNUE and after the final Phase-10 search
architecture.** Phase 9.7 permits only isolated safety calibration required to
make the net searchable; it is not the histshape/wave2/TM campaign.

> **⚑ HCE-line note (2026-07-13):** after the HCE/NNUE branch split, the
> final-scale condition is met on `master`/`development` (the HCE eval is
> final there), so these items became runnable for the HCE finalization. The
> user chose to spend only a small budget: staged LMR context-adjustment
> SPRTs (the four inert `Lmr*Adj` knobs) — the rest of this table stays
> deferred. The "post-NNUE" framing still governs the `nnue` branch.

| Deferred item | What it is | Est. value | When to run |
|---|---|---|---|
| **histshape SPSA** (was 7.4) | history bonus/malus shape plus accepted history/LMR dimensions; regenerate the config after 8.5.10/8.5.11/10.1 so it contains no obsolete tables or knobs | unknown | Phase 10.7 final tune only |
| **wave2 mechanisms** | accepted `CapFutDepth`/`QuietSeeDepth`/post-LMR/fractional-LMR dimensions; `QsearchCheckCap` is excluded unless 8.9 made quiet qchecks live | unknown | Phase 10.7, after history-aware `lmrDepth`; never seed known-negative values |
| **TM knobs** | time-management constants; the old SPSA washed under the old root model | unknown after NNUE/root v2 | Phase 10.7 only, after 8.5.12 supplies variance/instability/root-effort inputs |
| **6.6 fail-low prior countermove bonus** | skipped during Phase 6 | small | absorbed into 8.5.10(d) on `development` |
| **fractional history** | quantization experiment flagged during 6.7 | likely wash | only if still meaningful after 8.5.11, then Phase 10.7 |
| **SPSA discipline** (pre-registered, from the 2026-07-02 EV review) | 1000–1500 iters, abort at ~600 if trend ≤ 0, converged candidate → bake → SPRT + full CTest; a wash → keep defaults, done | — | applies to every run above |

Explicitly **not** reopenable by default: more HCE self-play cycles (cycle 6
proved that loop exhausted at the 1.8.0 head), HCE king-bucketed PSTs (build
NNUE king buckets instead), and the audit-closed HCE feature-name menu.

---

## 7. Release discipline

### Versioning (SemVer-adapted; no public API, so it maps to strength)

- **MAJOR** — architecture swap: NNUE ships as **2.0.0**.
- **MINOR** — a phase/campaign banking SPRT + gauntlet-validated strength
  (1.5.0 → 1.8.0 were all this). The version reflects **cumulative content
  since the last tag**, not the latest step.
- **PATCH** — robustness/bugfix only, no strength claim.
- Releases stay rare and curated; don't ship bench-identical work, don't sit
  on validated Elo.

### Pre-tag gate

1. Every strength behavior is SPRT-accepted; mandatory correctness and pure
   enablers satisfy their documented correctness + non-inferiority gates.
   Every result has the §1 manifest and matching binary/book hashes.
2. **Time-control ladder:** standard `3+0.03`, phase-boundary `10+0.1`, and for
   major/search/SMP releases a genuinely longer confirmation (target
   `60+0.6` or an equivalent agreed workload). Eval and selectivity gains can
   compress or reverse with depth; `10+0.1` alone is not called real LTC.
3. **Field gauntlet** — tooling: **colosseum**
   (`D:\code\colosseum`, the user's GUI tournament manager — user drives it) or
   `tools/gauntlet.ps1`; optional LittleBlitzer cross-check. Calibrated slate
   (all in `D:\chess\engines`): prior Basilisk release (the head-to-head that
   matters), Fruit 2.1, Rarog 2.2.0, Rybka 3, Critter 1.6a, SF
   `UCI_LimitStrength` 2800/2900/3000, HIARCS 14, Shredder 12. Read the
   **head-to-head vs the prior release**; treat absolute estimates loosely.
   From 2.0 onward add full-strength contemporary Stockfish, Reckless,
   PlentyChess and another current independent engine; use calibrated node
   odds when direct scores are too one-sided.
4. Scan for illegal moves / forfeits / crashes (`t=`, `i=` counts = 0 for
   Basilisk; throttled-SF incidents in lost positions are benign).
5. For a CCRL-style estimate: anchor Ordo to a published engine
   (`ordo -a <ccrl> -A "<name>"`), slower TC, treat as ±50–100.
6. For releases claiming MT strength, repeat the target 4/8-thread fixed-time
   gate with recorded affinity/topology/hash; 1T SPRT is insufficient.

### Release checklist (the model runs this when asked to "release X.Y.Z")

1. Confirm the gate above passed.
2. Bump the version in **both** places: `src/Constants.h` (`engineVersion`) and
   `CMakeLists.txt` (`project(basilisk VERSION …)` — drives the dist tag).
3. Update `CHANGELOG.md` (Keep-a-Changelog entry with SPRT + gauntlet numbers,
   honest fast-TC vs LTC framing) and `README.md` only if the feature list
   changed.
4. Build the complete production ISA matrix with fresh revision-matched PGO.
   Test the **exact files to upload**: no `BASILISK_TUNE` UCI options, all
   CTest/sanitizer-required gates pass, bench runs, manifests/checksums match,
   and the generic binary runs on its declared baseline CPU. For NNUE, record
   embedded-net SHA/architecture and require incremental/full/reference parity.
5. Commit the prep on `development`. **Do not tag. Do not push.**
6. Produce copy-pasteable GitHub release notes (summary, strength vs prior tag,
   changes, honest caveats).
7. **User:** squash → `Version X.Y.Z` on `master`, push, `git tag vX.Y.Z`.
   The tag workflow builds, tests and uploads the documented **PGO production
   assets**; local and public binaries may use different portability tiers but
   must share source, defaults, embedded net and recorded behavior.

### Compute budget

Ryzen 9 5950X, shared. Texel fits = CPU-minutes, run freely. Datagen/SPSA at
concurrency ~24; SPRT/gauntlets via repo script defaults. NPS is not a
bottleneck (~2.7M nps single-thread; PEXT + LTO + PGO local builds) — profile
before any micro-optimisation.

---

## 8. Quick commands

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

# NNUE Bullet baseline pipeline (Phase 9; NVIDIA CUDA training)
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
cd D:\code\basilisk

# Comprehensive SPSA (Phase 10.7 only, after final search architecture)
.\tools\spsa.ps1 -ConfigGroup histshape -EngineSuffix <base> -Iterations 1500
cd tools\weather-factory && python main.py

# Fixed-game boundary gauntlet (or use colosseum)
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <prior-release>,<field...> -TC "10+0.1"
```

---

## 9. Bottom line

Phases 0–7 took Basilisk from 1.4.9 to 1.8.0 by building a serious test harness,
search baseline and mature HCE. The remaining route is explicit:

```text
Phase 8 on development
  correctness + release/CI infrastructure
Phase 8.5 on development
  StateInfo/dirty-piece contract + observable eval-independent search
  + frozen teacher/data contract
one rebase of nnue onto the accepted Phase-8.5 SHA
Phase 9
  harden D:/code/net_trainer and integrate its Bullet 1x8 quantised.bin baseline
Phase 10
  final 1T search architecture, then the one comprehensive search tune
Phase 11
  mandatory SMP/hardware scaling
Phase 12
  versioned NNUE architecture/data ladder and frontier validation
```

“Done” does not mean every item was copied from Stockfish or every constant was
tuned. It means no known correctness/unsound-bound defect remains; search and
board behavior are observable and independently tested; public PGO/ISA assets
are the artifacts actually validated; 1T and MT gains survive appropriate time
controls; every network/data/test result is reproducible; and top-tier claims
are made against contemporary full-strength engines or external lists. The HCE
remains useful for fallback and diagnosis, but NNUE is the frontier path.
