# Basilisk Strength Improvement Plan

> **CURRENT STATE (2026-08-01):** **1.9.3 RELEASE READY** as a tooling-only
> patch on the published 1.9.2 engine; Phase 9 is CLOSED. Clang PGO now asks
> the selected compiler for its matching `llvm-profdata`, preventing mixed
> LLVM installations from discarding profile records. Engine behavior remains
> identical (bench 11,941,440); the frozen HCE/NNUE baseline is unchanged.
>
> **✅ 9.12 CLOSED:** the formal 4T fastchess gate passed, Colosseum boundary
> gauntlets confirmed positive direction against 1.9.1 at 1T fast (**+11 ±24**),
> 4T fast (**+14 ±34**) and 4T `10+0.1` (**+26 ±35**), and all 5,800 Colosseum
> games completed without a reported Basilisk forfeit, crash or illegal move.
> The user explicitly waived the redundant standalone 1T NPS and 4T scaling/
> diagnostic reruns: 1T is bench-identical, 9.6 already has pooled-PGO NPS
> evidence, and the completed 1T/4T games directly cover deployed behavior.
> By explicit user decision, the focused accepted scope ships as **1.9.2**, not
> the originally planned 1.10.0.
>
> **Then, unchanged:** the post-release **NNUE runway** (8.5.3 dirty-piece on
> the 8.6.10 structure, 8.5.14 TT graph-history [down-scoped], 8.5.15
> frozen-teacher benchmark, 8.5.16 `net_trainer` preflight) → rebase `nnue`
> once → **Phase 10: NNUE (§6)** on the existing `D:/code/net_trainer`
> Bullet/Rust/CUDA pipeline and its raw `quantised.bin` contract (do not
> resurrect the removed PyTorch/`MNN1` path).
> **Deferred to the 11.7 joint SPSA** (each over-widens/fragments as a one-off
> flip, not as a pre-release unit): TT-PV bit (8.5.7), history-v2 (8.5.11), the
> (d) prior-move cont-hist rebalance, and the inert-knob set. Post-NNUE:
> **Phase 11 search architecture + final tune, Phase 12 MT scaling beyond
> Phase 9, Phase 13 NNUE architecture/data iteration (§7)**. Deferred
> experiments also live in §7.

This plan was executed 2026-05 → 2026-07 as Phases 0–8.7. The step-by-step
history lives in `CHANGELOG.md` and git history; this document keeps the
**process** (how work happens, §1), the **record** (what shipped, §2–3), the
**closed pre-NNUE phases** (§4), the **closed Phase 9 record** (§5), the
**NNUE line** (§6), the **post-NNUE roadmap + deferred menu** (§7), and the **operational
discipline** (releases §8, commands §9).

---

## 1. The development process

### Document audience (fixed 2026-07-29 — check it before writing into any of them)

Every document in this repo is written for exactly one of two audiences, and
the split is a rule, not a habit. Content written at the wrong altitude is a
defect in that document even when every fact in it is true.

| Document | Audience | What belongs in it |
|---|---|---|
| `README.md` | **User** (plays/analyses with the engine) | what the engine is, where to download it, which asset to pick, how to load it in a GUI, UCI options in plain language, how to build it if they want to. No phases, no SPRT numbers, no roadmap. |
| `CHANGELOG.md` | **User** | what changed between released versions, in released-version terms. Strength claims stay, stated honestly (fast-TC vs LTC); internal step numbers only where they genuinely help a reader place a change. |
| **GitHub release notes** | **User** | the per-release summary: strength vs the prior tag, what changed, which asset to download, honest caveats. Derived from `CHANGELOG.md`, never from `PLAN.md`. |
| `PLAN.md` | **Developer / maintainer** | scope, phases, dependencies, gates, evidence, measured results, doctrine, rejected candidates. Authoritative for *what we do and why*. |
| `GUIDE.md` | **Developer / maintainer** | the at-a-glance companion to `PLAN.md`: where we are, what is next, what to run. |
| `docs/`, `analysis/` | **Developer / maintainer** | contracts, audits, deep-dives referenced from `PLAN.md`. |

Practical consequences: a user-facing document never gains a phase number, an
SPRT interval or a roadmap item; a developer-facing document never softens a
measured result to make it read better. When a release is cut, the notes are
written *from* `CHANGELOG.md` — translating `PLAN.md` into release notes is how
internal vocabulary leaks to users.

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
- **After every step:** update `PLAN.md` + `GUIDE.md`, commit
  **without** a co-author trailer.
- **Releases:** user squashes `development` onto `master` as a single
  `Version X.Y.Z` commit, then tags manually. See §8.

### Strength-first engineering doctrine (user decision 2026-08-01)

The project objective is the **strongest possible chess engine**, not merely
finishing the current checklist inside inherited constraints. This applies to
engine code, testing, tuning, data generation, tooling, and this plan itself.

- If the model finds something plausibly **wrong, weak, sub-optimal, obsolete,
  or badly implemented**, or sees that a current constraint is avoidable, it
  must tell the user before silently adapting the work around it.
- State the evidence, likely strength/correctness/reliability benefit, expected
  implementation and measurement cost, and whether it would invalidate or
  delay the active experiment. Distinguish a demonstrated defect from a
  promising hypothesis.
- If fixing it materially expands scope or changes a registered protocol,
  **pause and decide with the user first**. The user chooses whether to improve
  it now, schedule it, or deliberately accept the constraint. Discovery is not
  automatic authorization to bundle unrelated changes.
- Do not preserve a legacy design merely because Basilisk already uses it or
  because the current plan assumed it. Compare stronger available designs and
  repair the plan when the evidence warrants it.
- Once the decision is made, retain the normal discipline: one semantic
  candidate, reproducible evidence, and the appropriate gate. “Strongest
  possible” raises the quality bar; it does not excuse unmeasured changes.

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
   `tc=3+0.03`, Hash 64, Threads 1, **UHO book** (`UHO_Lichess_4852_v1.epd`,
   adopted 2026-07-17 — unbalanced-but-fair openings cut the draw rate so
   SPRTs resolve in far fewer games; the old balanced `SuperGM_4mvs.pgn`
   remains the gauntlet fallback for CCRL-comparable estimates). The TC
   matches the SPSA TC so optima transfer. `tc=10+0.1` for phase-boundary / LTC-suspect
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
   the NNUE swap (Phase 10) possible.
8. **Every tested artifact is reproducible.** `build_test.ps1`/`sprt.ps1`
   must emit and retain a manifest containing engine SHA + dirty-diff hash,
   compiler/linker versions, complete CMake/ISA/TUNE flags, PGO profile input
   and hash, bench signature, binary SHA-256, opening-book SHA-256, random
   seed/order policy, TC/hash/threads/concurrency/adjudication/SPRT bounds, and
   fastchess version. A PGN without this manifest is not a reproducible test.
9. **Release UCI stays clean** — tuning knobs exist only behind `-DTUNE=ON`;
   releases run compiled-in defaults (accepted values are baked into headers,
   never shipped as config).
10. **Deployment is 1T AND 4T (user decision 2026-07-28).** Both are
    first-class competition conditions (Colosseum/CCRL 1CPU and 4CPU), so
    multi-thread Elo counts exactly as much as single-thread Elo, and a change
    must regress neither. Practical consequences, all enforced by Phase 9:
    - a multi-thread item gates at **Threads=4** as a matter of course, and
      any 1T-visible part of it also carries a 1T non-inferiority check;
    - **hash scales with threads** — 4× the nodes needs 4× the table, so a 4T
      gate runs `Hash 256` against 1T's `Hash 64`; a fixed-`Hash` MT comparison
      measures table thrash, not the change;
    - at 4T, **nothing under ~10k games separates 0 from +3** (Rarog measured
      the same run at +1.78 @1.5k, +2.90 @3.1k and −0.81 @28.4k). Do not form
      an opinion from a 4T run's first few thousand games;
    - `-use-affinity` is **dropped** whenever Threads>1 (see 9.2), and every
      thread count needs its own null calibration before a verdict is trusted.
11. **SPSA doctrine (established 9.1, applies to every future tune).** SPSA is
    the most expensive thing this project runs; these rules are what make a
    tune worth its nights.
    - **≥5,000 iterations or don't start.** Below ~2,500 a tune barely beats
      its own seed — a 1,000-iteration SPSA is a null result with a bake
      attached. Every Basilisk tune to date sat in that range.
    - **Bake tail means (final ~500 iterations), never endpoints**, and bake
      the **whole vector**. SPSA estimates a *joint* optimum; reverting a
      subset yields a point the tuner never evaluated. A knob wandering on
      noise already has a tail mean ≈ its seed, so the tail mean *is* the
      filter. Decompose on a rejection, not at bake time.
    - **Merge groups.** Per-iteration cost is 2 evaluations regardless of
      dimension (p=6 and p=26 converge at nearly the same rate), so merging is
      free *and* captures interaction. One tune beats three sequential ones.
    - **Never pin a discrete A/B knob ON inside the tune that fits its own
      consumers.** Rarog pinned one such guard "so the signal is honest",
      the guard silently discarded 59.7% of its correction-history training,
      and 117,536 games produced a **−55.98** candidate that could not have
      detected the cause. Gate the discrete knob separately *first*.
    - **Kill-checkpoint at ~1,500 iterations:** seed two knobs a full step off
      their baked values; the schedule must visibly walk them back. If they
      wander, stop — the tuner has no resolving power at that noise level and
      the rest of the run cannot help either.
    - **Re-tuning an already-fitted group is low-EV.** Our accepted Elo has
      come overwhelmingly from mechanism and speed; prefer mechanism, speed and
      correctness work whenever machine time is contested.

### The cost principle (§0.5, kept because it still governs §7)

- **Texel weight-fitting is CPU-minutes — run it freely.** SPSA and SPRT are
  **thousands of games each — conserved.**
- **Search constants are denominated in eval centipawns.** Any eval re-fit
  changes what a centipawn means, so a search-constant SPSA run before the
  eval is final is thrown away. This is why the search SPSA was deferred
  through Phase 7 — and why it is now a **post-NNUE** item (§7): NNUE re-scales
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
| **1.9.1** | 2026-07-23 | **Phase 8.6** pre-NNUE hardening from the 2026-07-20 Rarog cross-review — param/TT/protocol hygiene, CI in Rarog's 9.2 shape (master-push + dispatch; sanitizers; cross-platform bench agreement), A/B compiler-equality enforcement, search telemetry, the check-extension-removal SPRT (Rarog evidence **+30.75**; **our SPRT rejected −10.17**) + **Phase 8.7 profile-guided speed pass** (**+4.34% NPS ≈ +8.69 ± 6.63 Elo**; search bit-identical to 1.9.0) | released pre-Phase-9 HCE baseline |
| **1.9.2** | **2026-08-01** | **Phase 9 (§5)** focused harness/SMP/durability wave — SPSA schedule repair, MT-capable SPRT harness, thread-count + node-counter safety, accepted SMP clock/helper safety, index-hoists II, removal of unproven helper-history blending, and upgraded five-phase data/Texel infrastructure. Coordination/TT/diversification/HCE-refit candidates were rejected and are not shipped. User chose a patch rather than the pre-registered 1.10.0 because the accepted production scope is focused. | **9.4: +30.42 ± 8.77 Elo @4T, 0 forfeits** (SPRT-stopped, optimistic, bundle value); boundary H2H vs 1.9.1: +11 @1T fast, +14 @4T fast, +26 @4T `10+0.1`; released |
| **1.9.3** | **release ready 2026-08-01** | Tooling-only patch: Clang PGO resolves `llvm-profdata` from the selected compiler rather than global `PATH`, preventing mixed LLVM toolchains. | Engine behavior unchanged; bench **11,941,440** |
| **2.0.0** | future | **Phase 10 NNUE** (§6) | target **+200…+400** |
| **2.x** | future | **Phase 11** final 1T search + tune, **Phase 12** MT scaling beyond Phase 9, **Phase 13** NNUE architecture/data frontier loop (§7) | prior **+15–40** 1T non-additive; NNUE scale evidence-driven |

Current release head: 1.9.3 release-ready, bench **11,941,440** (fixed-depth
40-position harness; TM-independent). Deployment conditions: **1T and 4T**
(gate 10).

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
10. **Harness bias is per-run persistent and does NOT average out with
    games** (2026-07-20/21 incident). Unpinned fastchess gave each run one
    scheduler-placement draw worth up to ~±10 Elo (nulls: +9.34, −10.78 on
    byte-identical binaries); more games only tighten the CI around the
    biased value, so **no historical accept inside the bias radius is
    validated by its sample size**. Fixes: `-use-affinity` with an explicit
    one-logical-CPU-per-physical-core list, ≥1.7.0 fastchess, null-pair
    calibration after every harness change judged by **estimate at fixed N,
    never LLR** (truth at a bound stalls SPRT). Corollaries: equivalence
    questions get fixed-N runs, not SPRTs; small pre-fix accepts need
    re-verification (8.6.8A); **never run two pinned harnesses at once** —
    they deterministically collide on the same core list.
11. **Bundle membership is not evidence about a component** (Rarog 2026-07-27,
    imported as doctrine). Rarog's SMP rework won **+102.78 @4T** as an
    undecomposed five-change bundle; its own later decomposition then showed
    the diversification half was worth **zero** (−0.81 over 28,362 games) and
    the coordination half carried everything. In the same project, treating
    "it was in the winning bundle" as proof of a component's value is exactly
    what let an untested guard ship and cost −56. **Consequence:** when a
    bundle is gated as a bundle, pre-register the decomposition order *before*
    the result, and never quote the bundle number as a component's EV.
12. **An early SPRT positive is the default shape of a null.** Three Rarog
    runs in two days peaked at +11.6 / +5.9 / +4.7 in their first 1–2k games
    and settled at +1.4 / −0.5 / −0.8. Combined with lesson 10 (per-run bias
    does not average out), the rule is: **the estimate at the CI you actually
    need is the only reading**, and at 4T that CI costs ~10k games.
13. **Cross-engine transfer is a prior, never a verdict — and it runs both
    ways.** Measured pairs so far: mop-up gating **+3.19 here / −7.37 there**;
    the node-invariant index hoist **+3.03% here / +3.76% there**; per-node
    `CheckInfo` **−1.8% here / +2.75% there**; `cutoffCnt` untested here /
    **−7.78 there**; fail-soft qsearch **canary-blocked here / −5.96 there**.
    Same family, opposite signs, in both directions. Import the *mechanism and
    the measurement method*, re-measure the *value*.

---

## 4. Phase 8 — board/search/eval correctness & hardening (CLOSED; shipped in 1.9.0/1.9.1)

Source: `analysis/infra_analysis.md` (external audit, 2026-07-13), **verified
claim-by-claim in-session 2026-07-13** — every repro re-run against the current
head. Verification verdicts that gate this phase:

- **Real, reproduced:** rule-50 draw overrides mate-in-1 (doc §4.1; live binary
  scores `cp 0` in `7k/5Q2/5K2/8/8/8/8/8 w - - 99 1`); null move advances the
  halfmove clock (§4.2, `board.cpp:644`); SEE accepts pinned recapturers
  (§4.3; `see(Bxc6)` = −200 where +100 is correct — fans out to all 9 `see_ge`
  call sites: staging, qsearch/main SEE pruning, ProbCut, LMR classification);
  EP square hashed when the EP capture is illegal (§4.5, missed-repetition
  identity); history capacity 1024 is assert-only (§4.6).
- **REFUTED — do not "fix":** doc §4.4 (SEE king recaptures). The
  `KING = 20000` sentinel + minimax fold handles every king case correctly
  (verified empirically on all three scenarios). Any explicit king branch
  would be dead code at best.
- **Doc errors to remember:** §7.4 is wrong that no TT prefetch exists
  (`search.cpp:1562` prefetches the child entry); §6.3's cost claim is inert
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
  checks in qsearch (the `search_params.h` 6.8 comment tracked an older SF) —
  bears on 8.9 and weakens the `QsearchCheckCap` prior in the hcefinal SPSA.
- **Recommendation split adopted here:** eval-independent structural items →
  the **Phase 8.5 search ladder on `development`**; cp-denominated /
  architecture-heavy items → **Phase 11 (§7)**; SMP → **Phase 12 (§7)**.
  Its Elo priors overlap heavily — never sum the rows.

The third same-day audit (`analysis/hce_analysis.md`, the HCE evaluator) was
verified to the same standard 2026-07-13: every claim checked against the
source — evaluator, `eval_params.h` values, both tuner paths, extraction /
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
  covers only KS knobs, no SPSA config exposes them; the `eval_params.h`
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
  Phase-13 fallback menu — NNUE learns those relationships directly, so they
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
   stale `search_params.h` 6.8 comment (SF no longer does qsearch quiet
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
8. **8.9 (LAST) Conditional direct quiet-check generation** (doc §6.3):
   **gate on the hcefinal SPSA outcome** — if `QsearchCheckCap` baked at 0,
   the `gen_quiet_checks()` path is provably inert → **skip, close the item**.
   If it baked > 0, first measure the generate-all-then-filter cost in the
   live path (qply==0 nodes only); implement direct generation (check-squares
   + discovered-check masks) only if the cost is measurable, then SPRT.
   Search-audit prior: current SF does **not** search quiet checks in
   qsearch, so the expected outcome is *skip* — the SPSA verdict decides.

### Phase 8.5 — NNUE-neutral board/search completion and data preparation (`development` ONLY)

> **Status 2026-07-28:** the pre-1.9.0 half is CLOSED and shipped. The four
> remaining items (8.5.3, 8.5.14, 8.5.15, 8.5.16) are the **NNUE runway** and
> now run **after Phase 9 (§5)**, not before it — they are data-prep for the
> `nnue` rebase, while Phase 9 is the last HCE-line work and fixes the
> measurement tooling the whole NNUE line depends on. Nothing in Phase 9
> touches the evaluator, so the runway is unaffected by it.

**Branch rule, fixed by the user on 2026-07-14:** all Basilisk Phase-8.5 work
lands on `development`. The `nnue` branch remains frozen while 8.5 runs. When
8.5 is complete and the accepted HCE head is tagged/recorded, rebase `nnue`
**once** onto that exact `development` SHA and begin Phase 10. Do not implement
parallel copies of StateInfo/search changes on `nnue`, and do not merge partial
NNUE code back into `development`.

Phase 8.5 contains only work that remains useful after the evaluator swap.
Actual Bullet `quantised.bin` loading, accumulator storage/update and SIMD
inference stay in Phase 10 because those already exist in partial form on
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
   Phase 10):** `8.5.3` dirty-piece contract → `8.5.15` frozen teacher
   benchmark (baselines the **released** final HCE head — consistent only
   because no Elo work is left for "after") → `8.5.16` `net_trainer` preflight
   → `nnue` rebase → Phase 10.

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
   **AMENDED 2026-07-20 (twice, same day) — final placement: PRE-release,
   as Phase 8.6.10 "structure era"** (first amendment put it in the
   post-1.9.1 runway; the user then decided all cleanups/refactors ship in
   1.9.1). Full spec at 8.6.10: 8.5.2's content + `HistoryTables` +
   per-ply `PlyContext` + make/unmake centralization + `RootMove` records,
   one board-surgery block (Rarog 11.0 pattern). Only the pure NNUE data
   prep (8.5.3 dirty-piece, which attaches to the new structure) stays in
   the post-release runway.
3. **8.5.3 [POST-1.9.0 NNUE runway] NNUE-neutral dirty-piece contract:** every real move records exact
   removed/added piece-square pairs for quiets, captures, EP, promotions and
   castling; null move records no piece delta. This is data only—no network
   weights or accumulator in `development`. Extend the 8.8 random walker to
   reconstruct the child board from each delta. Phase 10 consumes this contract
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
   naturally unblocked once Phase 10 NNUE replaces the HCE eval.** Reverted.
   ⚠ **EV DOWNGRADED 2026-07-28 (Rarog cross-review, §5 table).** Rarog got the
   same family to a gate and decomposed it: the tactical-outcome **guard** —
   the (a)-class semantics above — is worth **−56 Elo** (it silently discarded
   59.7% of all correction training), and everything else in the bundle
   measured **+1.43 ± 4.85** combined. So the guard is REFUTED, not merely
   untested, and must not be revived; what survives is the margin/weight half
   at ~+1.5, which belongs in 11.6/11.7's fit rather than its own gate.
6. **8.5.6 Qsearch in-check upgrade:** order evasions by TT move and contextual
   history, and store completed in-check qsearch bounds in TT. → SPRT.
   **ATTEMPTED 2026-07-15 → both parts negative on the bench (fixed-depth node
   count), not sent to SPRT:** the generator already emits king-moves-first,
   which cuts off *better* than either `score_moves` (capture-first: +54%
   nodes) or TT-move-first (+11%); and the TT store alone adds ~+25% because
   in-check evasion children are *non-check* qsearch nodes that return the
   non-provable fail-hard bounds documented at the 8.1f trap — aggregating
   them into a stored bound poisons later cutoffs. **The store part is BLOCKED
   on Phase 11.4 qsearch bound-shaping** (provable qsearch bounds) and cannot
   land safely before it. Reverted to the 8.1e baseline. Revisit the store
   only after 11.4; the reordering is simply not a win here.
7. **8.5.7 Persistent TT-PV bit:** store/propagate PV ancestry through all
   eligible TT bounds rather than reconstructing it only from deep exact
   entries. It is an enabler for Phase 11 even if neutral alone; validate age/
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
     play — the bench wrap is not a game regression. Requeue with 11.7 SPSA or a
     dedicated tune. Deferred in favor of the genuinely-clean cuckoo (8.5.13).
   - **Re-tested 2026-07-17 on the `instabtm` head → confirmed 11.7 sub-project,
     no SPRT spent.** Re-applied genBound8 (CTest 11/11) and measured the real
     node impact with a **fresh-TT fixed-depth** search (sidesteps the shared-TT
     gen-wrap that corrupts bench): depth-18 startpos **3.90M vs 2.57M = +51%
     nodes**. Confirms there is **no good LMR operating point** — the
     `lmr_tt_pv_adj` reduction *is* the cost, so any value that matters
     over-widens. TT-PV only pays via **pruning conservatism** (relax
     futility/LMP on tt_pv nodes), a consumer we don't have. Land it as
     bit + ttPv pruning guards + joint SPSA at 11.7. Reverted; head `instabtm`.
8. **8.5.8 Blanket check-extension removal:** isolated two-sided experiment,
   after 8.1 qsearch correctness and 8.8 canary split. Record telemetry,
   tactical/quiet/endgame node-budget suites and SPRT. Keep or close on games.
9. **8.5.9 Checking-move LMR under the existing model:** after the 8.5.8
   verdict, allow contextually bad late checks to reduce/SEE-prune rather than
   receive categorical protection. Do **not** make a later good-capture test
   conditional on this result; good-capture/all-move LMR belongs to 11.1 after
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
      a clean rung.** Requeue it under 8.5.11 (history rep v2) / 11.7 SPSA, not
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
    future 11.1 reduction model. Staged SPRTs, never one table explosion.
12. **8.5.12 Persistent root-move model, aspiration and TM inputs:** retain per
    root move score, previous score, running mean **and variance**, bound/
    completion, seldepth, nodes and PV; sort after each root result. Then test
    per-move uncertainty-aware aspiration, repeated-fail recovery depth, and
    time management based on best-move instability plus full root effort
    distribution. Separate state refactor from behavioral candidates. This is
    also the required input for Phase 12 voting.
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
      (also the Phase-12 voting input).
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
14. **8.5.14 TT graph-history semantics (DOWN-SCOPED 2026-07-20):** separately
    test cut-node-compatible TT cutoffs, TT-cutoff history feedback and a
    conservative near-rule-50 cutoff guard. Verify partial history cannot turn
    mate into draw. Prefetch-before-make/qsearch remains a measurement candidate
    because main-search child TT prefetch already exists. **The rule-50-adjusted
    TT key/bucket sub-item is PARKED** on Rarog's 7.3 verdict (2026-07-15,
    rationale transfers verbatim): our harness adjudicates draws at move 40
    (score < 10) and resigns at 600, so test games almost never reach high
    clocks — the fix's benefit is invisible at our gates while its de-tuning
    risk is not, and both of Rarog's draw-adjacent reworks lost 7–12 Elo.
    Re-entry triggers: LTC-era primary testing, an adjudication-policy change,
    or the post-NNUE recalibration.

#### Track C — external benchmark and `net_trainer` data contract

15. **8.5.15 [POST-1.9.0 NNUE runway] Frozen teacher benchmark:** create an evaluator test corpus
    independent of Basilisk adjudication, split by source game/trajectory with
    an untouched test set, enriched for endgames, king attacks, tactical
    cliffs and quiet positional drift. Record full/lazy/corrected HCE, qsearch
    and depth-N output; report residuals by phase, material, king danger,
    halfmove clock and tacticality. Baseline the **released 1.9.1 head**
    (the final accepted HCE, post-Phase-8.6), not 1.8.0 or 1.9.0, before
    training the release net.
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
    Bullet revision and CUDA toolchain needed to reproduce training. Phase 10
    adds trainer validation/resume and large-scale data.

#### Track D — durable general strength pulled forward to 1.9.0 (2026-07-15)

**Phase-number principle (user, 2026-07-15; amended 2026-07-20):** the number
tracks the NNUE boundary — Phase < 10 is **before** NNUE, Phase 10 **is** NNUE,
Phase ≥ 10 is **strictly after**. So a general (eval-agnostic) improvement that
strengthens the final HCE release *and* carries to NNUE **without re-SPSA/
re-fit** belongs before Phase 10, not in the post-NNUE Phase 11. Goal: make the
last HCE release as strong as possible, as a hedge if the NNUE project
underdelivers. *(2026-07-20: the final HCE release is now **1.9.1** — Phase
8.6 sits between 8.5 and 9, still pre-NNUE; the principle is unchanged.)*

Inclusion test for Track D: (a) NNUE-agnostic; (b) survives the evaluator swap
with no material re-tune/re-fit; (c) strengthens 1.9.0 and/or datagen; (d) does
**not** touch the won-endgame static eval (avoids the HCE mate-drive canary that
blocked 8.4/8.5.5).

1. **8.5.D1 TT density & replacement (pulled from old 11.3):** 32-byte
   cluster / partial-key layout for ~2× entries at equal hash, with safe
   lock-free publication and collision/replacement telemetry. Durable (a memory
   layout, not an eval constant), strengthens long-TC / large-hash play, and
   improves **fixed-node datagen label quality** (deeper search per node
   budget). → SPRT at several hash sizes and a genuinely long TC. **Top Track-D
   pick.**
2. **8.5.D2 = 8.5.13 upcoming repetition (cuckoo):** already pre-1.9.0;
   eval-agnostic, durable correctness+strength. Keep in the 1.9.0 set.
3. **8.5.D3 = 8.5.7 persistent TT-PV bit:** already pre-1.9.0; durable TT bit,
   enabler for the post-NNUE 11.1 reduction model. Non-inferiority gate.

**Eligible but lower-certainty:** the history ladder (8.5.10) and history v2
(8.5.11) are move-ordering (no eval touch → no fragility), so they can be tried
for 1.9.0; their tables/benefit are re-validated (not re-SPSA'd) after NNUE, so
durability is less certain than D1–D3.

**Optional big lever — SMP (Phase 12):** fully eval-agnostic and durable, and
would materially strengthen 1.9.0 for multi-thread users (a real hedge if NNUE
fails). But it is a large effort and does **not** accelerate datagen (which
parallelizes at the game level, not via Lazy SMP). Default: keep post-NNUE by
priority; pull to Track D only if a strong MT 1.9.0 is explicitly wanted.

**Excluded from 1.9.0** (need eval-calibrated re-tune or hit the mate-drive
fragility, so they stay in post-NNUE Phase 11): 11.1 unified reduction, 11.4
bound quality, 11.5 ProbCut/null/IIR margins, 11.6 correction-consumption v2,
11.7 final tune; and 8.5.5, 8.5.8, 8.5.9. *(2026-07-20 amendment: **8.5.8 is
pulled forward into Phase 8.6 as 8.6.7** — Rarog's 8.2(a) result (+30.75)
showed the removal needs no eval-calibrated re-tune and no SPSA; 8.5.9 and
the rest stay post-NNUE.)*

**Explicitly skipped on the HCE line:** incremental HCE material/PST fields,
NNUE accumulator code, refresh caches, threat inputs, and Chess960 (product
feature, no standard-chess Elo). OpenBench is not mandatory before Phase 10 on
one machine, but the manifest/result schema must be compatible with later
OpenBench adoption; frontier-scale +1–3 Elo work will eventually benefit from
persistent/distributed testing.

### Phase 8.6 — Pre-NNUE hardening & CI wave (CLOSED; shipped in 1.9.1)

**Added 2026-07-20 (user decision): one more HCE release before the NNUE
cutoff.** Source: the in-session cross-review of **Rarog's pre-NNUE program**
(its Phases 7–9, including the 2026-07-19/20 Phase-9 refinements: CI shape,
PGO-smoke, 10.5-A resolution, local-only manifests) against Basilisk 1.9.0,
verified by **four code audits** (UCI/FEN robustness, search bug-classes,
tests/CI extent, params/TT/bench infra) — every imported item was checked
against our code, never assumed. Mirrors Rarog's same-day reciprocal import
(its 8.4(e)/8.10/8.11 + two LMR dims came from our 1.9.0). Naming: "Phase
8.6" always carries the word *Phase*; the historical **step 8.6**
(mate-drive gating, shipped in 1.9.0) is unrelated. The NNUE runway
(8.5.3/8.5.14/8.5.15/8.5.16) moves after this phase; **1.9.1 replaces 1.9.0
as the final HCE release and frozen NNUE baseline** — read every
`[POST-1.9.0 NNUE runway]` tag as "after the final HCE release", i.e.
post-1.9.1.

**Verified NON-gaps (2026-07-20; recorded so future audits don't
re-litigate):** the aspiration loop terminates by construction (re-centers on
the *current* fail score, delta ×1.5, full-open fallback at delta ≥ 900,
mate-range pre-guard — `search.cpp:1853-1878`; Rarog's 7.0
infinite-fail-high class is impossible here); the TM score-drop signal reads
the prior-iteration snapshot *before* updating it (`search.cpp:1896` —
Rarog's 7.5 attenuation bug absent); FEN/`position` handling validates
strictly (incl. side-not-to-move-in-check) and retains the prior board with
no `exit()` — stronger than Rarog's own open 9.5-A behaviour; no
unstoppable-passer/square-rule term exists (Rarog's 7.4(iii) bug N/A); TT
allocation never exceeds `Hash` (Rarog's pre-9.4 2× bug absent; the residual
resize spike + pow2 waste is 8.6.2); `bench` already has repeats/best-of-N +
geomean EBF, and `board_performance` is median-of-11 + MAD (Rarog 9.7
parity). **Anti-lessons imported, do NOT copy from Rarog:** root-aware
repetition / null-clock repetition fences (genuine −7…−12 there), history
no-aging (rejected twice), standalone SF-style aspiration re-centering
(−4.52), TT-cutoff history rewards (rejected independently by both engines).

**Sequencing (order ≠ numbering; reworked 2026-07-20, user decision:
cleanups and refactors FIRST, so everything later lands on the final
shape):** 8.6.2a → 8.6.2b → 8.6.2c → **8.6.10 structure era** → 8.6.3 →
8.6.6 (telemetry — written once, against the restructured search) → 8.6.7;
the CI steps 8.6.4/8.6.5 interleave with the 8.6.7 SPRT wait. All of
8.6.2–8.6.10 need no games (each bench-identical or test-only, so 8.6.7's
SPRT baseline stays honest). 8.6.7 is the phase's **single planned SPRT**.
8.6.8 is opt-in only. 8.6.9 releases — gated on the whole wave being
resolved (Rarog 9.8 precedent). **2026-07-21 amendment:** the placement-
lottery incident retired "single planned SPRT" — the phase now also carries
the refactor-equivalence gate plus the **8.6.8A accept-audit** fixed-N probe
runs, and 1.9.1 waits for 8.6.8A. **Release principle, inverted from 1.9.0:
cleanups/refactors ship IN the release; only pure NNUE data prep
(8.5.3/8.5.14/8.5.15/8.5.16) stays after it.** **2026-07-22 amendment
(user decision): the release step 8.6.9 moved OUT of this phase into the
new Phase 8.7 speed pass (its final item) — 1.9.1 now waits for 8.6.8A
AND Phase 8.7.**

1. **8.6.1 Parameter-plumbing hygiene — ✅ DONE 2026-07-20 (no games; bench
   11,941,440 identical, CTest 11/11).** Scope upgraded by user decision:
   the fix IS the macro. `search_params.h` now holds a single
   `BASILISK_SEARCH_PARAMS(X)` table — 46 entries of `(field, UciName,
   default, min, max)` — that generates all three hand-synced sites: the
   struct field + compiled default, the TUNE-build UCI advertisement, and
   the setoption clamp arm (Rarog 9.0a `params!` equivalent; the drift
   class is dead by construction, incl. for every future NNUE-era knob).
   All historical doc comments preserved in the header's FIELD NOTES block.
   Fixes landed via the table: **(a)** `PostLmrHistScale` now advertises
   its true compiled 0 (was 104 — the audit's stale-default find); **(b)**
   `TmInstability` (the +10.79 knob, previously registered nowhere =
   untunable) now exposed 0–100 default 35 — TUNE builds advertise 46
   spins, the release list stays clean (5 spins + non-spin options).
   Verified: bench fingerprint identical, CTest 11/11, TUNE=OFF build
   clean, and the generated setter chain proven functional end-to-end
   (`setoption RfpCoeff 240` → `go depth 12` nodes 173k→370k; note `bench`
   by design never consumes setoptions — it is the fixed fingerprint/PGO
   workload, unchanged). **(c)** stale SPSA configs renamed
   `config_lmr.json.STALE` / `config_combined.json.STALE` so
   `spsa.ps1 -ConfigGroup` cannot launch them (both carry the pre-6.7
   **0–2/0–3 integer** LMR-adj scale vs the live 1024ths — running one
   would silently drive those knobs to ~0; `config_combined` additionally
   has pre-hcefinal seeds `HistPruneCoeff` 4210 vs live 14004 and a
   `LmrHistDiv` floor 6000 **above** the live default 5683), with a loud
   README warning: regenerate any needed config **from the table** at
   11.7, and `TmInstability` must be in the 11.7 config.
2. **8.6.2 — SPLIT 2026-07-20 into (a) TT contract + (b) clean-code package**
   after a four-audit deep dive (TT/cache sizing + 32-bit inventory;
   empirical lint/sanitizer scan; C++23 idiom review; architecture/
   duplication review). Headline verdicts, recorded: **no Rarog-class sizing
   bug anywhere** (every table sized from its own unit; the 2× Hash class is
   absent); `src/` is already fully `-Wconversion`/`-Wsign-conversion`/
   old-style-cast **clean** (Rarog had 245 warnings; we have 0 in src, 15 in
   two test files); zero TODO/dead code, no raw-memory/lifetime debt, no UB
   found, clean search↔eval boundary. BUT: **zero `static_assert`s in the
   tree** (the 32-byte TTCluster density contract is unenforced — the latent
   Rarog-bug form); the **debug ASan/UBSan gate is broken as shipped**
   (`_ALL_TARGETS` omits `test_wac`/`test_invariants` → the debug preset
   fails to link; `test_board`'s 8.1d capacity test trips the debug `assert`
   by design-conflict, so the debug suite cannot have passed since 8.1d —
   once bypassed, all tests pass with **zero** sanitizer reports); an
   **unconditional POST_BUILD dist copy** lets any debug/scratch build
   clobber release-named assets in `build/dist/` (empirically bitten during
   the audit; pext asset restored byte-identical); and **`-ffast-math` is set
   globally** on an integer-eval engine. Fair language-level label:
   disciplined C++17 under a C++23 flag — `<bit>`, `[[nodiscard]]`,
   constexpr enum operators all unused.
   - **8.6.2a TT contract — ✅ DONE 2026-07-20 (no games; bench 11,941,440
     identical, CTest 11/11, test_tt 52→64 assertions).** `clusters_.reset()`
     now precedes the reallocation in `resize()` — plain assignment kept both
     tables alive across `make_unique`, so growing 8→16 GB transiently needed
     ~24 GB at `setoption Hash` time, i.e. mid-game (Rarog's 9.4 class, milder
     form; nothing is carried across a resize, so dropping first costs
     nothing). New `[[nodiscard]] allocated_bytes()` accessor + two `test_tt`
     cases: the byte contract (**≤ budget and > budget/2**) across 9 `Hash`
     sizes, and the same contract across a grow/shrink resize pair plus a
     usability probe. **Both bounds negative-controlled and the controls
     reverted** (recorded in the test's doc comment): sizing the count in the
     WRONG UNIT — `bytes / 16` instead of `bytes / sizeof(TTCluster)`, i.e.
     precisely the sibling-engine bug shape — fails the upper bound (11
     assertions, exit 1); `bytes / (sizeof(TTCluster) * 4)` fails the lower
     bound. A first control attempt was discarded as *invalid* before use
     (over-allocating the array while `cluster_count_` stayed put doesn't move
     `allocated_bytes()`, which derives from the count — the same value
     `resize()` hands to `make_unique`, so it is the true footprint).
     **static_asserts — the tree had ZERO before this step:**
     `sizeof(TTCluster)==32` + `alignof==32` (pins the density contract that
     both the comments and `resize()`'s `bytes/sizeof` math depend on — a
     silently widened field was the latent Rarog-shaped failure), pow2 guards
     on `PAWN_TABLE_SIZE`/`CORR_SIZE`/`PAWN_HIST_SIZE` (all indexed
     `& (SIZE-1)`, where a non-pow2 value aliases silently rather than
     failing; verified firing by a compile-only control on
     `PAWN_TABLE_SIZE=16000`), and the **64-bit-only declaration**
     (`sizeof(size_t) >= 8` + `sizeof(void*) >= 8` in `types.h`, README
     prerequisite row, `release_tiers.md` paragraph). Vestigial 32-bit
     removals: 8 `_M_IX86`/`__i386__` CPUID guard arms (`main.cpp`) and
     the `i.86` clause (`CMakeLists.txt`). Formalization only, as scoped —
     Basilisk never had 32-bit fallback code (unconditional 64-bit popcount;
     all 8 shipped assets are x86-64/aarch64); the declaration also
     retroactively proves the ~14 `Key & (SIZE-1)` index sites and the
     `mb*1024*1024` math lossless. Full-budget (multiply-hi) indexing stays
     §7-deferred.
   - **8.6.2b Clean-code & build-hygiene package (no games; per-item
     bench-identity gate):**
     **P0 — ✅ DONE 2026-07-20** (commit `5b6c01a`; bench 11,941,440
     identical, release CTest 11/11, zero build warnings): `_ALL_TARGETS`
     made exhaustive — `test_wac`/`test_invariants` were missing, so they
     compiled against sanitized libs *without* `-fsanitize` and the debug
     preset failed to link (~150 undefined `__asan_*`/`__ubsan_*` symbols
     each); the gate had been unbuildable since those targets were added.
     `test_board`'s capacity guard aborted the debug suite: `board.cpp`
     asserted `history_size < MAX_HISTORY` directly above the release clamp
     handling that exact case — contradictory, and overflow is reachable
     from **external input** (an over-long `position ... moves` list), so a
     debug abort would be crash-on-demand from the GUI; assert removed, clamp
     kept + documented (8.6.10's growable history is the real fix). The dist
     POST_BUILD copy is now gated on an optimized/non-sanitized/non-PGO
     build — it previously fired for every configuration, so a Debug
     configure overwrote a release-named asset with a 10 MB ASan binary
     (observed during the audit); gate confirmed holding. Rider: the two test
     files are warning-clean (3 `test_invariants` warnings became visible
     under the `_ALL_TARGETS` fix; `board_performance`'s were pre-existing).
     **First-ever result: the full debug suite BUILDS and PASSES 11/11 under
     ASan+UBSan in 559 s with ZERO sanitizer reports** — the code was always
     clean; the gate was fiction.
     **P1 — ✅ DONE 2026-07-20** (3 commits; bench 11,941,440 identical
     throughout): `-ffast-math` **removed — measured free** (bench
     bit-identical with and without, so the float `init_lmr` table never
     moved; the flag was buying nothing behavioural on an integer-eval
     engine while implying `-ffinite-math-only`). `<bit>` adopted
     (`std::countr_zero`/`countl_zero`/`popcount`) — same codegen, fixes the
     real-MSVC `popcount` break; `more_than_one` deliberately **not**
     converted to `has_single_bit` (they differ on an empty board).
     `damp_rule50` + OCB/KNvK predicates + `EVAL_DARK_SQUARES` extracted to
     `eval.h`; the tuner's `linear_delta_scale` now calls them. Two
     complementary dark-square masks and a hand-copied rule-50 curve are
     gone — and the in-code claim that `--verify` would catch a divergence
     was **wrong**: it corrupts fitted gradients silently instead. Verified
     by tuner `--verify` 8598/8598 exact. Rider: 11 long-standing TEXEL-build
     warnings cleared.
     **P2 — ✅ DONE 2026-07-20** (3 commits; bench identical throughout):
     `[[nodiscard]]` on 14 pure queries (`pop_lsb` excluded — it mutates, so
     discarding is legitimate); constexpr `Square`/`Direction` operators +
     `flip_file()` in `types.h`, deleting **all 22** `Square(int(...))`
     casts; dead ray-stepping block removed (`bitboard.cpp` — it walked all
     8 rays then discarded them via `(void)ray;` while the next loop rebuilt
     the tables from scratch); `lmr_table_` `{}`-initialised; `Parameters`
     22 camelCase members/methods → snake_case across 6 files; the two
     trailing-underscore stragglers (`Searcher::evaluator_`, `Engine::tt_`)
     + a duplicated `private:` label; `setOption`'s 3 `std::regex` objects
     hoisted to function-local statics (they were recompiled per call —
     thousands per SPSA run); **test-only `gen_pseudo_legal*` oracle moved
     out of `board.cpp` (−302 lines) into `tests/movegen_oracle.h`**, with
     its castling helpers deliberately re-stated rather than exported (an
     oracle that borrows engine internals stops being independent — the
     header says never to de-duplicate it); stale script doc-examples
     refreshed. Test-file warnings were cleared earlier, with P0.
     **File renames stay declined** (Windows case-only-rename pain +
     `nnue`-rebase conflicts) — Phase-11 opportunistic.
     **C-items — resolved 2026-07-20:** **C9 ✅** TT `key16` store→`release`
     / probe load→`acquire`, so the documented payload-before-key invariant
     is now real (it was comment-only under all-`relaxed`: accidentally true
     on x86, not on the ARM targets we ship). **C10 ✅** debug asserts in
     `lsb`/`msb`/`sq_bb` (landed with `<bit>`; the full ASan suite passes
     with them live, so no call site violates the contracts). **C11 —
     triaged, mostly declined:** `memset`→`= {}` was implemented and
     **reverted** (raw multi-dimensional C arrays are not assignable; the
     alternatives are invasive or worse — `memset` is correct here, now
     recorded in-code); `perft` test-helper dedup declined (each test binary
     is standalone by design). **C12/C13 — BLOCKED, need a user decision:**
     neither clang-tidy nor `cl.exe` is installed, and installing a
     toolchain is a system change not taken unprompted. **C14 — no action:**
     `tools/weather-factory/` is a gitignored vendored clone (the user's
     scratch area), not repository content. *(Amended by 9.1: the clone is
     still gitignored, but the files we patch now live tracked in
     `tools/weather-factory-overlay/` and are hash-verified into it.)*
     **Recorded skips (decided 2026-07-20):** `evaluate()` decomposition —
     permanent (NNUE bypasses it; every hour there is sunk); `negamax`
     decomposition — Phase 11.1/11.2 rewrite that code anyway; file renames
     — never/Phase 11 (Windows case-only-rename pain + `nnue`-rebase
     conflicts); consteval table init and `std::format` — post-NNUE at best
     (`std::format` additionally risks breaking tool parsers that read
     "Nodes searched").
   - **8.6.2c C++23 modernization pass — ⚠ PARTIALLY DONE 2026-07-20; ONE
     OPEN DECISION (`std::expected`, see below).** Delivered: all bit
     primitives are now **constexpr** (enabled by P1's `<bit>` switch — the
     builtins they replaced were not portably constant-evaluable; this is
     the prerequisite for the deferred consteval tables). Evaluated and
     declined **with reasons recorded in-code**: `std::unreachable()` — no
     provably exhaustive switch exists (the `PieceType` defaults are
     genuinely reachable via `KING`/`NO_PIECE_TYPE` and return defined
     fallbacks, so marking them unreachable would convert correct code into
     UB); `std::to_underlying` — nothing to convert (no enum-class→int casts;
     the plain `enum : int` types convert implicitly *by design*);
     `std::span` for `eval_param_ptr/cptr` — the textbook case, but the
     lengths live in X-macro expansions in both `eval.cpp` and `tuner.cpp`,
     so it is deferred to 8.6.10 which rewrites that area anyway;
     `std::print`/`format` — unchanged skip (tool scripts parse
     "Nodes searched"). **OPEN: `std::expected` for `try_set_fen`'s
     bool+out-param error channel.** `<expected>` is confirmed available in
     this toolchain, but the conversion touches **19 call sites** across
     engine and tests, and the same argument used to decline file renames
     applies — every pre-rebase churn multiplies conflicts for the one-time
     `nnue` rebase. Needs a user call: do it now, or fold it into 8.6.10.
     Original scope, for reference:
     **(i)** `std::expected`-shaped error channels for the cold fallible
     APIs (`try_set_fen`'s internal fail plumbing, `load_eval_params`) —
     replaces bool+out-param; **(ii)** `std::unreachable()` in provably
     exhaustive switch defaults (replaces silent sentinel returns);
     **(iii)** `std::to_underlying` for cold enum→int (complements 8.6.2b's
     operators); **(iv)** `std::span` where pointer+length travels (test
     oracle interface, bench suite iteration); **(v)** `constexpr` widening
     of pure helpers (mirror/relative-square/distance) — enables future
     consteval tables without committing to them; **(vi)** ranges/algorithm
     upgrades in cold loops ONLY where they clarify; **(vii)** concepts/
     deducing-this: adopt only at natural sites — `gen_legal_impl<Us>`
     stays a plain template, decision recorded. Explicitly out (standing
     skips): `std::print`/`std::format` (tool parsers), hot-path ranges,
     consteval table init.
   Gate: bench fingerprint identical per item (one documented exception: a
   re-baseline if `-ffast-math` removal shifts the LMR table), CTest 11/11
   **including the repaired debug/sanitizer suite**, CI dry-run where
   workflow files are touched.
3. **8.6.3 — ✅ DONE 2026-07-20** (bench identical, CTest 11/11): **(a)**
   engine-level malformed-input survival test (garbage/bare/unknown
   `position` forms, illegal move list, 3 malformed setoption shapes →
   `isready` returns, `go` yields a legal STARTPOS move, rejections
   reported) with the reject-and-retain contract decision recorded in the
   test header — flipping to SF-style hard-exit must change that test,
   never happen silently; **(b)** strict packaged-FEN sweeps over
   endgames.epd + the 40 bench FENs (new `bench_fens()` accessor) + WAC-300
   (loader upgraded to strict); **(c)** parser fuzz mutates all 7
   SEED_FENS; **(d)** POSIX `SIGPIPE` ignored in main. Original spec: **(a)** UCI-layer malformed-input tests through the real
   `setPosition`/`setOption` path (garbage FEN, bare `position fen`, illegal
   move in `moves`, `position` with no args, unknown command, malformed
   `setoption`) pinning the documented contract: reject + `info string` +
   **prior board retained** + `isready` still answered. **Contract decision
   recorded (Rarog 9.5-A style; its 11-engine survey 2026-07-19):** SF-dev
   now exits 1 + diagnostic on both cases (the reference implementation is
   tightening); we **deliberately stay with reject-and-retain** (majority
   practice — Critter/SaberTooth/Hydra/us), accepting the recorded residual
   risk that a GUI which ignores the `info string` gets a legal move for the
   *previous* board. Any flip to SF-style hard-exit is a future deliberate
   decision, never a silent change — these tests are the guard. **(b)**
   packaged-FEN legality sweep: every FEN in `endgames.epd`, the WAC-300
   suite and the `bench` suite loads through **strict** `try_set_fen` — the
   gate that would have caught the 4 illegal `endgames.epd` positions
   (Rarog `canary_integrity` equivalent). **(c)** widen the parser-fuzz
   mutation seeds from startpos-only to the 7 `SEED_FENS` (Rarog fuzzes 22
   diverse roots; junk-string half stays as is). **(d)** POSIX `SIGPIPE`
   ignore for the shipped Linux/macOS binaries (a GUI pipe-close currently
   signal-kills there; Windows unaffected). Gate: CTest green incl. the new
   tests, bench-identical.
4. **8.6.4 — ✅ DONE 2026-07-20 (`8fc0ac1`); all job paths verified locally
   (portable Release CTest 11/11, TUNE build clean, ASan suite proven,
   portable==pext bench-10 agreement 3,342,261); remote dispatch dry-run
   fires with the next master push.** Original spec: 8.8's `ci.yml` was removed
   at the 1.9.0 release because push/PR triggers didn't fit the
   squash-to-`master` workflow; Rarog's 9.2 design does fit and is imported
   as is: **trigger on push to `master` + `workflow_dispatch` only** (the
   local loop already gates every `development` commit; a manual pre-merge
   check is only useful if it faithfully previews the master gate — so both
   triggers run the IDENTICAL job set). Jobs: Linux + Windows clang Release
   build + full CTest; Linux Debug **ASan+UBSan** CTest (the preset exists,
   nothing invokes it today); TUNE=ON/OFF build matrix; **cross-platform
   bench-agreement job hardcoding NO expected value** — all platforms must
   agree with each other (a frozen constant would need editing every
   behavioural commit; agreement must hold forever, catches unsound code /
   nondeterminism, and is the one thing local dev cannot verify — it is also
   our compiler-bump UB canary, Rarog 9.1's argument). Optional rider:
   scheduled nightly rotating-seed fuzz via the existing `BASILISK_FUZZ_SEED`
   hook. Gate: green `workflow_dispatch` dry-run.
5. **8.6.5 — ✅ DONE 2026-07-20 (pulled before 8.6.7 by user decision so the
   SPRT runs under the hardened harness; mismatch + equality controls both
   verified; `instabtm` predates manifests → warn-not-fail path).** Original
   scope: **(a)** `sprt.ps1` copies both engines' build manifests
   beside the result (Rarog 9.7) and **hard-fails when their `compiler:`
   lines differ** — the C++ analog of Rarog's 9.1 toolchain pin: with no
   `rust-toolchain.toml` equivalent, equality-by-check replaces
   identity-by-pin (a silent MSYS2 clang upgrade between an A/B pair folds
   compiler delta into a ±3-Elo result; recording has existed since 8.8,
   enforcement hasn't). Warn-not-fail for pre-8.6 binaries; add the loud
   dirty-tree console warning at build time. **(b)** `release.yml`: add a
   real `bench` node count to the artifact smoke test (Rarog 9.3; currently
   `uciok` + version only). **(c)** **Manifests stay local-only by design**
   (Rarog 9.7 user decision 2026-07-20: release provenance = tag SHA +
   artifact smoke test; `tools/test_engines/` and results stay gitignored) —
   this also ratifies the 1.9.0 removal of 8.7's per-asset manifest
   publishing. Gate: dispatch dry-run + a deliberate mismatched-compiler
   negative test of (a).
6. **8.6.6 Search telemetry counters — ✅ DONE 2026-07-20 (no games; bench
   11,941,440 identical with Diag OFF **and ON** — the lazy audit serves the
   remembered lazy score, so behaviour never changes; CTest 11/11; counter
   cost measured NIL by interleaved best-of-5 NPS vs the pre-telemetry
   commit, POST ≥ PRE both rounds).** `DiagCounters` per Searcher
   (always-counted plain int64s, printed only under the new TUNE-advertised
   `Diag` check option → 5 `info string diag` lines at search end), plus
   the 8.6.6b lazy dual-eval audit in the evaluator (skip runs the full
   tail for measurement, serves the lazy value).
   **Baseline harvest, pre-8.6.7 head (startpos d16, recorded for
   before/after):** interior 642,337 / qsearch 349,317; in-check share
   **4.09%**, `check_ext` **25,892** (the 8.6.7 must-read-0 counter);
   tt_pv **0.30%** (Rarog's 0.50% re-grade of TT-PV consumers confirmed
   here too); TT hits 31.2%, cutoffs 38,000; LMR applied 126,817 /
   re-searched **1.04%** (Rarog's 1.83% already meant "far too timid" —
   ours is MORE so; strong prior for the post-NNUE 11.1/11.7 LMR work);
   **`hist_prunes` = 2 — history pruning is de-facto DEAD** post-hcefinal
   (`HistPruneCoeff` 14004×depth exceeds any reachable combined history;
   legitimate SPSA output, recorded for 11.7, not "fixed"); razor 28k,
   RFP 84k, null 18.5k/38.6k, fut 32.5k, LMP 1.55M; **lazy audit: 21,689
   fires, ZERO sign flips, 1,056 crossings (4.9%), mean |Δ| 77 cp, max
   500** — mirrors Rarog's zero-flip result; lean retire for any
   lazy-margin conditioning, pending a game-conditions harvest. Original
   spec: reverses the 8.5.4 skip on evidence: at Rarog the counters re-graded a
   planned item **before any SPRT was spent** (`tt_pv` veto measured at
   0.50% of nodes → EV +2–8 re-graded to +0–2 and the item folded away) and
   identified LMR as its top live lever (re-search rate 1.83%) — and our
   Phase-11 acceptance criteria already assume this telemetry exists.
   Disabled-by-default (TUNE/UCI switch), end-of-search `info string diag`
   dump: in-check node share, check-extension count (must read 0 after
   8.6.7), LMR applied / re-search rate, prune attempts+successes by family
   (RFP/razor/null/ProbCut/futility/LMP/history/SEE), TT probe/hit/bound
   shares, history-update events by type, qsearch evasion/depth stats.
   **(b) rider (Rarog 9.6b):** lazy dual-eval audit under the same switch —
   lazy fires, |delta| sum/max, **sign flips**, margin crossings; one WAC
   harvest decides (Rarog's: 228k fires, **zero** sign flips → lean retire).
   Matters here because Phase-10.2 datagen labels come from this eval.
   Record a baseline dump at the pre-8.6.7 head. Gate: bench-identical with
   diag off.
7. **8.6.7 — ❌ REJECTED 2026-07-20, REVERTED (user-stopped at no-path-to-H1,
   Rarog-8.1b precedent).** SPRT vs `instabtm` (UHO 3+0.03): **−10.17 ±
   6.52, nElo −15.53, LOS 0.11%, LLR −1.97 @ 4,682 games** (Ptnml
   [114,627,989,504,107]). Implementation was correct and fully witnessed
   (bench 7,939,803 = −33.5%, `check_ext` 0, canaries green, equal-cost WAC
   no-collapse: 139/300 in 3.13M vs old 132/300 in 2.47M) — the LOSS is
   what's real. **Classification (Rarog lesson 15): plausibly a de-tuning
   victim, not proven a heuristic loss** — hcefinal tuned the LMR context,
   futility and RFP/razor/null margins *with* the extension present, on a
   tree the removal shrinks 33%; Rarog's +30.75 came *before* its LMR
   re-tune. The standalone gate therefore partly measured de-tuning. Also a
   fresh instance of lesson 7: Rarog priors do not transfer (cont-hist6
   −7.70 before this). **Verdict: reverted cleanly (only `d22e4c8` —
   restores the extension verbatim; bench back to 11,941,440, CTest 11/11,
   WAC floor green again, no re-baseline ever happened). NOT in 1.9.1.
   8.5.8 is CLOSED for the HCE line. Requeued as a lesson-15-fair bundle —
   removal + joint re-SPSA of its consumers (~16 dims: RFP/razor/null/
   futility/SEE margins + full LMR family) judged as ONE gate — at the
   post-NNUE recalibration (§7 deferred table), where those constants get
   re-fitted anyway.** Original spec: Remove the unconditional in-check `depth++`
   (`search.cpp:1247-1250`) — 8.5.8's spec, pulled forward on cross-engine
   evidence: Rarog 8.2(a) **+30.75 ± 8.83, LOS 100%** (its largest single
   gain ever; bench −57%, EBF 2.52 → 2.42). Safe here for the same reason:
   a checked node at depth 0 falls to qsearch, which generates the **full**
   legal evasion list and detects mate (8.1e). The other categorical
   in-check protections (pruning-block/futility/LMP/LMR exemptions) stay
   untouched — this is the extension only, exactly Rarog's (a) scope; the
   (b)/(c) LMR-of-checks variants washed there and stay post-NNUE (8.5.9).
   Methodological rules imported with it: **judge tactics at equal node
   cost, never equal depth** (Rarog's fixed-depth WAC *fell* 185→174 while
   the change was strongly positive; at equal nodes it rose 185→203); the
   WAC CTest floor may trip — leave it red through the gate and re-baseline
   only **after** the SPRT verdict, never to make the change pass; compare
   8.6.6's in-check share and check-extension counter before/after. Gate:
   SPRT `[0,3]` vs the 1.9.0 head (`instabtm` engine); robust endgame
   canaries apply (correctness core only — mate recognition + conversion
   floor). On H0: revert, close 8.5.8 for the HCE line, re-test at the
   post-NNUE recalibration. Prior: Rarog +30.75 on a same-family tree;
   honest expectation **+5…+30** (our extension feeds a smaller tree than
   Rarog's did).
8. **8.6.8 — ⏭ SKIPPED 2026-07-20 (user decision; content anchored as a
   MUST-INCLUDE in 11.1 [mechanism] and 11.7 [re-tune] so it cannot be
   forgotten — see those steps).** Original spec: Rarog's measured #1 remaining lever (its 8.6; SPSA in flight
   2026-07-20, carrying two dims imported from *our* hcefinal vector —
   `lmr_tt_capture` 301, `lmr_not_improving` 89). Eval-agnostic (LMR lives
   in depth/move-index space — Rarog lesson 2 — so it would survive the
   evaluator swap), hence pre-NNUE-legal; but it costs an SPSA + SPRT and
   delays the cutoff, and our plan already reserves LMR modernization for
   11.1/11.7 targeting the final net. Run **only on explicit user opt-in**
   after 8.6.7 resolves; otherwise read Rarog's 8.6 verdict as a free
   cross-review prior for 11.1/11.7.
9. **8.6.8A — ✅ CLOSED 2026-07-23: accept-audit — re-verified the small
   biased-harness accepts before 1.9.1 shipped (added after the placement-
   lottery incident; user decision). Outcome: every re-measured accept was
   REAL, nothing was removed, and the original headline numbers were ~40–55%
   bias-inflated.**

   **Why (the bias model, from the 2026-07-20/21 incident):** every SPRT
   before the affinity fix ran unpinned; each run inherited one persistent
   scheduler-placement offset of up to ~±10 Elo (measured null readings:
   **+9.34** and **−10.78** on byte-identical binaries; pinned nulls read
   **−0.10 ± 6.00 @ 3.4k**). Crucially the offset is **per-run persistent —
   it does NOT average out with more games**; a long run only tightens the
   CI around the *biased* value, so a large game count does not derisk an
   old accept. Any Elo-justified accept whose claimed margin sits inside
   the bias radius could be a zero-value feature that drew a lucky
   placement. Verdicts cannot be repaired retroactively; they can only be
   re-measured on the pinned harness.

   **Scope rule (pre-registered):** re-verify an accept iff (i) it was
   justified by Elo alone, not correctness (bugfixes are exempt — they ship
   on correctness grounds regardless of Elo); (ii) claimed margin ≤ ~12,
   i.e. inside bias reach; (iii) no SPSA has re-tuned on top of it —
   reverting under a later tune measures de-tuning, not the feature
   (lesson 15; the 8.6.7 trap). The `hcefinal` SPSA concluded 2026-07-14
   and **no SPSA has run since**, so the entire post-SPSA 1.9.0 accept
   chain (manifests 2026-07-15 → 07-17) qualifies cleanly; everything
   pre-hcefinal is excluded as a de-tune trap. (iv) revert cost sane.

   **The audit table (1.9.0 strength claims, triaged):**

   | Feature | Claimed | Verdict | Action |
   |---|---|---|---|
   | `hcefinal` SPSA | +35.94 ± 9.42 | beyond bias reach (survives a full draw) | SAFE — no action |
   | 8.3 eval refresh | +13.97 | **NOT safe-by-size** (one draw ≈ 11 → could be ~+3 true); **kept on correctness merit** — 8.3 fixed three genuine activation bugs (OCB whole-eval amplification eval.cpp:384, enemy-rook-behind-passer mis-nested eval.cpp:1139, attacked2 two-pawn double-attack eval.cpp:779). You do not revert bugfixes to audit Elo; also de-tune-confounded (hcefinal tuned search on this eval). Claim downgraded to *unverified* | no run; reclassify |
   | 8.5.12 instabtm | +10.79 | **✅ run A DONE 2026-07-21: REAL, KEEP** — off-switch costs −6.46 ± 4.12 (LOS 0.11%, 10k, clean pinned, 0 forfeits). Re-verified value +6.46 (statistically consistent with +10.79; point estimate ~4 lower = the favorable bias the audit hunts). Claim annotated, feature kept, no code change | done |
   | 8.5.10b' exact/PV reward-only hist | +4.90 | ✅ **REAL (pair), KEEP** — hist-pair (b'+e) off = **−3.06 ± 4.35**, LOS 8.4% @10k | done |
   | 8.5.D1 TT density | +4.27 | inside reach | **kept on structural merit** (2× entries/hash; 8.6.2a contract tests build on the 32-byte cluster; revert cost high). Claim downgraded to *unverified* in bookkeeping |
   | 8.4 rule-50 damping | +3.29 | ✅ **REAL (pair), KEEP** — eval-pair (8.4+8.6) off = **−4.24 ± 4.38**, LOS 2.9% @10k | done |
   | 8.6 mate-drive nudge | +3.19 | ✅ **REAL (pair), KEEP** — same eval-pair probe | done |
   | 8.5.10e eval-surprise hist | +2.50 | ✅ **REAL (pair), KEEP** — same hist-pair probe | done |
   | 8.2 SEE pin-awareness | +0.65 | claim ≈ 0 anyway | reclassified **correctness** (more accurate SEE); kept; Elo claim dropped |

   Bias is symmetric — **rejects** are equally suspect in the other
   direction (a −10 draw can kill a genuine +5): `cuckoo` (rejected twice
   2026-07-17), `postlmrhist` (2026-07-16) and 8.6.7 all carry it. These
   do NOT block 1.9.1; queued in §7 as opt-in re-trials.

   **Probe design (fixed-N estimate-judged, never open-ended SPRT — an
   equivalence question has truth at/near a bound, where SPRT stalls; the
   calibration doctrine applies):**

   - **(a) Harness prep — extend `sprt.ps1` — ✅ DONE + VALIDATED
     2026-07-21 (syntax + all three guards + arg-construction, and run A
     confirmed it live: manifest recorded `option.TmInstability=0`, zero
     option errors, zero forfeits, and the −6.46 result proves the knob
     took effect — a no-op reads ~0):** new `-Mode fixed` (fixed
     `-Games`, no SPRT stopping rule, final Elo/nElo CI report, manifest
     as usual, no pass/fail throw); new `-OptionsA` / `-OptionsB`
     (per-engine `option.Name=Value` lists passed to fastchess and
     recorded in the manifest); SHA-guard update: **identical binaries
     become legal iff the option sets differ** (calibrate still requires
     identical binaries AND identical options; a normal SPRT of identical
     binaries with identical options still throws). Validate with a
     ~200-game micro-run showing the options land (fastchess log/PGN
     headers) before any probe is trusted.
   - **(b) Run A — instabtm (+10.79 claim) — ✅ DONE 2026-07-21: REAL,
     KEEP.** `basilisk-v1.9.1-pext-pgo` (the PROPER fresh build) vs itself,
     `-OptionsA TmInstability=0` — code-verified full off-switch
     (search.cpp:1941: `instability_scale = 1.0` exactly at 0). Result:
     off-switch −6.46 ± 4.12, nElo −10.68 ± 6.81, LOS 0.11%, 10,000 games,
     **0 forfeits** on the quiet pinned box (Ptnml [187,1209,2364,1083,157]).
     Straddled −3 only at the optimistic CI edge (−2.34); every decision
     branch → KEEP, so recorded KEEP without the 20k extension (estimate
     −6.46 at LOS 0.11% is decisive). Also the live micro-validation of
     substep (a): manifest recorded `option.TmInstability=0`, no option
     errors, same binary both sides. Feature kept at default 35, no change.
   - **(c) Run B — ✅ DONE 2026-07-22/23: ALL FOUR REAL, KEEP (no removals).**
     Bundle (all four off) read **−6.6 ± 6.8 @4.2k** (user-stopped;
     composition unresolved), so the pre-registered split ran — each fixed
     10k vs `v1.9.1-pext-pgo`, **0 forfeits**:
     **hist-pair (b'+e) −3.06 ± 4.35, LOS 8.4%** (claimed +7.40);
     **eval-pair (8.4+8.6) −4.24 ± 4.38, LOS 2.9%** (claimed +6.48).
     *The pre-split hypothesis — history pair = the phantom — is REFUTED:
     both pairs pay, each at roughly HALF its claimed value.*
     **Decisive step: the splits are independent measurements of disjoint
     feature sets against a common baseline, so they COMBINE — total
     −7.30 ± 6.17 (errors in quadrature) → CI [−13.5, −1.1], EXCLUDING
     ZERO.** The sum also reconciles with the bundle's −6.6 (three
     consistent measurements). **No further splitting is useful: resolving a
     single ~+2 Elo feature from 0 needs error <1 Elo ≈ 200k games (~40 h)
     per feature — the measurement floor; even 20k only reaches ±3.1 and
     still straddles.** Per the unresolvable-straddle fallback (default
     KEEP) plus positive combined evidence: **keep all four, revert
     nothing.** Probe binaries (`probe-b`, `-hist`, `-eval`) kept in
     tools/test_engines with manifests; reverts were working-tree only,
     never committed. Original spec: one probe
     commit on `development` reverting all four cheap suspects — 8.5.10b'
     (skip the reward-only training call, search.cpp:~1726), 8.5.10e
     (`es` always 100, search.cpp:~1692), 8.4 (make `damp_rule50`
     identity — single definition, tuner follows automatically), 8.6
     (guard out the mate-drive `eg +=` block, eval.cpp:~1418). Build via
     `build_test.ps1` (manifest, compiler equality). Fixed 10,000 games
     vs `phase86-final`. Probe commit is reverted or kept per the verdict
     (retired-candidate-branch workflow: commit on dev, revert on reject).
   - **(d) Decision rules (pre-registered; A = removal side, negative
     estimate = removal loses = feature real):** 95% CI entirely **above
     −3** → noise → remove the feature(s). CI entirely **below −3** →
     real → keep (restore). CI straddles −3 → extend the same run once to
     20k games; if still straddling: a **bundle splits** (history pair
     b'+e claimed +7.4 vs eval pair 8.4+8.6 claimed +6.5, fixed 10k
     each), a **single defaults to KEEP** — conservative for a PATCH
     release — with the claim marked unverified. Judged **only at final
     N**; interim reads are for anomaly detection, never decisions.
   - **(e) Consolidation:** apply proven-noise removals as individual
     commits; history/eval removals **change the bench fingerprint** — the
     1.9.1 CHANGELOG "bit-identical" wording is amended to "bit-identical
     refactor + verified-neutral simplifications" listing each with its
     probe number (instabtm removal, if any, is TM-only: bench
     unaffected); CTest + endgame/WAC canaries re-run; tuner `--verify`
     re-run if eval was touched; 1.9.0's CHANGELOG strength table gets a
     one-line annotation of which claims survived re-verification (1.9.0
     itself stays released as-is — bookkeeping honesty only). Final
     confirmation on the cleaned head: fixed 10k vs 1.9.0 supersedes the
     refactor equivalence gate as the release evidence.
   - **(f) (optional, user call) Ladder re-baseline:** the shipped
     "1.9.0 ≈ +52 vs 1.8.0" head-to-head was one unpinned run (±bias).
     One pinned fixed 10k, 1.9.1 vs 1.8.0, replaces it in §2 honestly.

   **Compute:** expected 2 × 10k ≈ two ~4 h daytime runs; worst case ~5
   runs (extend + splits + confirmation). **One harness at a time on the
   box** — the 2026-07-21 collision (Basilisk + Rarog pinned to the same
   explicit core list → 2× oversubscription on those cores, forfeits,
   −58 Elo garbage) is the standing counterexample. Sequencing: the
   refactor-equivalence gate ✅ PASSED 2026-07-21 (fresh PROPER 1.9.1 build
   −0.5% NPS best-of-7 node-identical ≈ sub-1-Elo; clean 6k `-Mode fixed`
   probe +0.17 ± 4.41 — the mid-dev `phase86-final` first tested was a
   slower/wrong-versioned artifact whose −5 reading was ~−1 Elo speed +
   machine contention, NOT a real regression), so runs A/B start whenever
   the box is free. **Lesson for the audit: use a fresh proper release
   build, and separate NPS (best-of-N, node-identical) from game contention
   before trusting any small negative.**
10. **8.6.9 ⭐ Release 1.9.1 — MOVED 2026-07-22 into Phase 8.7 (user
   decision: a dedicated pre-release speed pass, motivated by Rarog's 10.3
   speed→Elo evidence, now precedes the release).** The full release step
   is the final item of **Phase 8.7** below; number kept (order ≠
   numbering). Everything already done under this step (version bump,
   CHANGELOG `[1.9.1]`, doc rename, refactor-equivalence gate ✅
   2026-07-21) carries over unchanged.
11. **8.6.10 Structure era — ✅ DONE 2026-07-20** (7 commits; bench
    **11,941,440 identical at every step**; NPS best-of-5 measured after
    every layout change against the pre-wave baseline 3,333,735 best /
    3,212,655 median — all within noise, the 16-bit-Move medians slightly
    BETTER):
    **(a) ✅** growable game history — `std::vector<UndoInfo>` with 2048
    reserve replaces the fixed array + release clamp; any `position` length
    is now simply correct, and the new test unwinds RESERVE+200 plies back
    to the exact starting hash (the property the clamp could never
    deliver); `operator=`'s hand-copy loop collapsed into vector assign.
    **EP TT-legality Board copy removed** — `is_legal`'s EN_PASSANT case
    deep-copied the Board and made the move; `ep_capture_legal`'s loop body
    is now the shared `ep_capture_legal_from()` core and both callers use
    it (one definition of the subtlest legality rule). **16-bit `Move`
    CONFIRMED and kept** — `uint16_t` (queen promo sets bit 15), zero new
    diagnostics because everything already flowed through the `make_*`
    constructors; MoveList/PV/countermove storage halved; two independent
    best-of-5 NPS runs medians 3,275,216 / 3,251,140 vs baseline 3,212,655.
    **(b) ✅** `HistoryTables` extracted to new `src/history.{h,cpp}` —
    storage + clear/age/blend lifecycle behind one struct, 103 references
    rewired; update POLICY (bonus formulas, what a cutoff trains) stays in
    search.cpp so Phase-11 edits policy without touching layout. age()
    keeps nested loops deliberately (a flattened pointer sweep over
    multi-dim arrays is formally UB).
    **(c) — SCOPE DECISION:** the full `PlyContext` AoS consolidation
    (folding PV/move buffers per-ply) was NOT done — it is an NPS-risky
    layout change with no present consumer, and the accumulator stack
    attaches at (d)'s seam regardless; `SearchStack` remains the per-ply
    context and the accumulator slot lands beside it in Phase 10. Revisit
    only if Phase-10 integration actually wants the AoS shape.
    **(d) ✅** make/unmake centralized — all search-side sites (6 move + 1
    null pair across negamax/quiescence/evasions/ProbCut) route through
    `Searcher::do_move/undo_move` (+null variants), inline in search.h,
    with the Phase-10 accumulator push/pop and 8.5.3 dirty-piece attachment
    points marked IN the wrappers. The three make_move calls on separate
    boards (ponder/PV-extraction/TT-child) deliberately stay direct.
    **(e) ✅** persistent `RootMoveStat` records — last/previous score,
    Welford mean+variance, cumulative nodes, seldepth, in-window flag, PV
    when leading; reset per `go`, written at the single root accounting
    point, consumed by nothing yet (Rarog 10.1 pattern — substrate first).
    **Also folded in: `std::expected<void,std::string>` for `try_set_fen`
    (8.6.2c's open item)** — the internal `fail` lambda kept all 46 error
    sites textually unchanged; 13 call sites adapted; 4 dead `err` locals
    fell out of the tuner; tuner `--verify` 8598/8598 exact.
    **NOT moved** (pure NNUE data prep, stays post-release): 8.5.3
    dirty-piece recording (attaches at (d)'s seam), 8.5.14/8.5.15/8.5.16.
    Forward-enabler check stands: with (d)+(e) landed, no known future
    improvement requires re-plumbing before its own phase.

### Phase 8.7 — Profile-guided speed pass (CLOSED; shipped in 1.9.1)

**Added 2026-07-22 (user decision): a dedicated speed wave before 1.9.1,
imported from Rarog's 10.3 result.** Naming: "Phase 8.7" always carries the
word *Phase*; the historical **step 8.7** (release PGO/manifests/tiers,
shipped in 1.9.0) is unrelated — same convention as Phase 8.6.

**Why (the Rarog evidence, read from its PLAN/dev guide 2026-07-22):**
Rarog's 10.3 profile-guided speed pass turned **+10.35% NPS into +20.31 ±
7.13 Elo** (nElo +33.06, LOS 100%, H1 on `[-3,0]` @ 3,460 games, 3+0.03,
both arms bench-identical — the cleanest speed→Elo datapoint either project
owns), and revised the planning constant to **≈ +2 Elo per 1% NPS at STC**
(~3× the old 0.7 Elo/1% estimate; do NOT transfer to LTC unmeasured —
deeper searches plausibly gain less per extra node, and our own 1.8.0
lesson says ~half of fast-TC gains compress at LTC). At that constant,
speed outperformed every search-mechanism item left in Rarog's queue (its
8.6 −7.78, 8.7 −7.29 vs speed +20.31) — the same shape as here, where
8.6.7 just failed at −10.17. Rarog's follow-up **8.12 "speed pass II"**
verdicts are imported as priors: incremental accumulators **REJECTED
−0.23%** under PGO; per-node SEE pin cache est +0.5–1.5%; profile-first
before speculation; PGO workload enrichment 0–2% "very cheap".

**Basilisk's starting point differs from Rarog's — verified by a
three-audit code review 2026-07-22 (search/movegen hot paths; eval/TT/
caches; build/PGO/tooling), so the honest target is smaller:**

- **No refactor debt to claw back.** Rarog's first +3.2% was recovering
  its own Phase-10 clean-code regression; our entire 8.6 wave measured
  **−0.5% NPS total** (best-of-7, node-identical — the refactor-
  equivalence gate, 2026-07-21).
- **Already in Rarog's "fixed" shape (verified — do NOT re-do):** move
  ordering's check bonus uses a per-node lazily-cached `check_squares`
  mask (search.cpp:617-652), not per-move `gives_check` (Rarog's CheckInfo
  fix, pre-done for *ordering*); `pick_next` is already the iterator+
  local-best selection scan with one swap (search.cpp:668-678 — Rarog's
  8d shape); movegen is **fully legal** (no per-move `is_legal`; Rarog's
  legality waste class absent); TT is header-only 32-byte clusters with a
  correctly-placed child prefetch right after `do_move` (search.cpp:1564);
  no allocations, virtuals, `std::function` or regex on the hot path;
  `bench <depth> <repeats>` best-of-N with median/min already exists
  engine-side (bench.cpp:107-212).
- **Confirmed live candidates** (the steps below): unconditional
  `checkers` recompute in `make_move`, full `gives_check` in the
  pruning/LMR path, pins recomputed per generation stage, SEE verdicts
  and `see_pins` recomputed per call, a transient `MoveList`+copy pass in
  the picker, per-move conthist guard/index recomputation, redundant
  slider lookups in the eval tail, a single-`bench 13` PGO workload, and
  a random magic search at every startup of the non-PEXT tiers.

**Collective target: +3–6% NPS ≈ +6–12 Elo at the STC constant** (honest
range — Rarog's +10.35% is not reachable here since several of its items
are pre-done). Order of steps = expected value; 8.7.2's profile findings
may re-order or add sub-steps.

**Gate protocol (Rarog's 10.3 shape + its rewritten NPS-measurement
protocol, adopted wholesale):**

- Every item lands **bench-identical** (fingerprint 11,941,440) unless
  the step says otherwise; CTest 11/11 per item; commit-per-item on
  `development`, revert on reject (retired-candidate-branch workflow).
- **NPS is judged only through the 8.7.1 instrument** — never a single
  best-of-N comparison. Imported rules: **validate the estimator on a
  SELF PAIR first** (same exe both arms, must read ~0.00% — two of
  Rarog's estimators were biased −0.2…−0.4% and produced two confident
  false rejections before the self pair caught them; bench NPS is
  left-skewed, so any design weighting the arms unequally against the
  slow tail manufactures bias); strictly alternate arms; compare
  arm-level **median and best-of** with a bootstrap CI; **pool ≥2 PGO
  builds per arm** (identical-source PGO builds differ ~0.36% — one
  build per arm cannot resolve a sub-1% effect); non-PGO builds are a
  cheap deterministic **screen that OVERSTATES** the shipped gain (Rarog
  8d: +6.35% non-PGO → +1.18% PGO) — screen non-PGO, confirm under PGO;
  idle pinned machine only.
- **Bench-identity does NOT imply NPS-identity** (Rarog's lesson: eight
  individually-"neutral" steps compounded to −3.2%): the phase closes
  with ONE end-to-end head-vs-phase-start NPS measurement on top of the
  per-item ones.
- An item that is a **strict work reduction AND bench-identical** but
  below the machine's ~1% resolution may be kept on the structural
  argument (Rarog items 5/8a precedent) — recorded as such and never
  counted toward the collective NPS claim.
- **Batch verdict (pre-registered):** collective NPS ≥ +2% → one batch
  `[-3,0]` STC SPRT vs the phase-start head (Rarog 10.3 shape, H1
  expected); < +2% → one fixed 10k `-Mode fixed` probe (calibration
  doctrine — equivalence questions stall SPRTs). Either way the 8.6.9
  cumulative release evidence follows on the final head.
- **No SPSA, no behavior changes anywhere in this phase** — pure speed;
  the no-games items interleave freely with the 8.6.8A probe runs
  (dev work daytime; **one pinned harness on the box at a time** — the
  2026-07-21 collision rule).

**Anti-items (decided now, from Rarog measurements + our own audits — do
NOT implement without new evidence):** incremental material/PST/phase
accumulators (Rarog 8.12(a) built BOTH shapes, bench-identical, **rejected
−0.23% under PGO** — bookkeeping outnumbered saved work 2:1; Basilisk is
structurally *worse* for it: the from-scratch walk in evaluate()
(eval.cpp:588-609) is the cheapest part of a ~950-line eval, evaluate()
itself is gated behind the TT `static_eval` cache (search.cpp:1264-1267)
and skipped in check, and the do_move seam is reserved for the Phase-10
NNUE accumulator — where this same design *becomes* correct);
**qsearch make_move check-hint** (Rarog measured −0.79%: qnodes make too
few moves to amortize a per-node mask build); **insufficient-material
per-node early exit** (Rarog sized the whole prize ≤ +0.23% with a probe
binary); **`pick_next` rewrite / ordering CheckInfo** (already in final
shape, see above); **`LAZY_MARGIN` sweep / lazy retire** (changes served
evals = strength item → 11.7, not speed; the 8.6.6b audit already reads
zero sign flips); **TT multiply-hi full-budget indexing** (stays §7:
changes the bench fingerprint and pays only at non-pow2 `Hash`, which our
harness never uses); **BOLT / `-fno-rtti` / x86-64-v2 tier** (menu-grade;
revisit only if 8.7.2's profile shows front-end stalls).

1. **8.7.1 NPS instrument + speed telemetry — 🔄 (a)(b)(c) ✅ DONE
   2026-07-23 (`c61543d`); (d) + self-pair validation PENDING AN IDLE BOX
   (both are NPS measurements; the user was on the machine).** Bench
   **11,941,440 identical**, CTest **11/11**.
   **First counter harvest (startpos d16) — numbers that did not exist
   before:** `eval` **51.10%/node**; pawn cache **89.03% HIT**
   (451,083/506,688 — the *inverse* of Rarog's 88.4% miss, so **8.7.8 is
   likely a dead end here** and should be re-scoped or dropped);
   `gives_check` **195.45%/node** (1,938,166 — ~2 full check-detections per
   node, the largest single number in the harvest ⇒ **8.7.3 is confirmed as
   the headline candidate**); `see_ge` **0.631/node** (625,587).
   `in_check` 4.09% and `check_ext` 25,892 reproduce the 8.6.6 baseline
   exactly, confirming behaviour is unchanged.
   **(d) PHASE-START BASELINE — ✅ RECORDED 2026-07-23, idle box, pinned
   CPU 30, bench 13 x3, 16 alternating rounds:**
   - **Estimator self-pair: −0.13%, CI [−0.31%, +0.19%], A 6/16 — PASSES.**
     Resolution ≈ **±0.25%**, so items ≥0.5% are individually resolvable and
     ≥1% comfortably so. (Two earlier self-pairs FAILED and fixed the tool:
     slot bias −0.29% at 8/8, then a ±4.5% CI from raw-repeat pooling. A
     third, +0.29% at 13/16, was traced to a YouTube video — the same run on
     an idle box reads −0.13%, so **background media alone invalidates a
     sub-1% NPS run**.)
   - **PGO build luck (headA vs headB, identical source): +0.09%, CI
     [−0.38%, +0.28%], 8/16.** ~4× tighter than Rarog's 0.36%.
   - **Head NPS (counters in): median ≈ 3.487M** (headA 3,487,060 / headB
     3,483,505). **Phase-start reference binary =
     `basilisk-v1.9.1-pext-pgo.exe`** (pre-counter; the 8.7.11 end-to-end
     measurement runs against it, so the telemetry cost is correctly counted
     inside the phase's net effect).
   - **Telemetry cost: NIL confirmed** — counter builds measured *+0.70%*
     (CI [0.29%, 0.88%]) vs pre-counter 1.9.1, i.e. certainly no slowdown.
     ⚠ **Unresolved: v1.9.1 sits 0.70% below BOTH head builds while those
     agree to 0.10%** — either v1.9.1 is an unlucky build or the added
     fields shifted layout favourably. **Consequence: the 2026-07-21
     "−0.5% refactor cost" (v1.9.1 vs 1.9.0) was single-build, ad-hoc-script
     and pinned to CPU 0 — treat it as measurement noise, NOT a real
     regression.** The game evidence (+0.17 ± 4.41) is unaffected. Settle by
     building 2 pre-counter binaries and comparing pooled-vs-pooled before
     relying on single-build arms.
   - **Counter baseline across position types (go depth 14, Diag on):**

     | position | eval/node | pawn hit | gives_check/node | see_ge/node |
     |---|---|---|---|---|
     | startpos | 52.2% | 88.7% | **196%** | 0.65 |
     | middlegame | 53.4% | 94.1% | **166%** | **1.04** |
     | R+P endgame | 42.5% | 99.6% | **141%** | 0.58 |
     | pawn endgame | 38.7% | 98.1% | **96%** | 0.43 |

     **Both phase conclusions now hold in EVERY position type, not just
     startpos: `gives_check` runs 1–2× per node everywhere ⇒ 8.7.3 confirmed
     headline; the pawn cache never drops below 88.7% hit ⇒ 8.7.8 is DEAD
     (drop or re-scope).** `see_ge` peaks in middlegames (1.04/node) ⇒ that
     is where 8.7.5 has most to gain. *(Bench-wide counters would need
     accumulation across its 40 positions since counters reset per `go`;
     deferred — these four positions already answer the routing questions.)*
   **Implementation note:** the `gives_check`/`see_ge` counters live in
   `Board` (mutable, one increment per query) rather than at the ~11 search
   call sites, so every caller is captured including movegen's own use;
   `Board` is per-searcher (`Searcher::search` takes it BY VALUE) so they
   cannot race, and they are snapshotted into `DiagCounters` at teardown
   because `board_ptr_` is nulled before `print_diag()` runs.
   Original spec:
   - **(a) `tools/nps_ab.ps1`** — the missing counterpart of `sprt.ps1`'s
     Elo self-pair calibration (sprt.ps1:234-246), which exists for games
     but not for NPS: drive `bench 13 <repeats>` on two engine builds in
     **strictly alternating arms** (reuse `harness_common.ps1` affinity/
     pinning helpers), report arm-level **median + best-of** with a
     bootstrap CI on the median, and support a **self-pair mode** (same
     exe both arms) whose ~0.00% reading is REQUIRED before the script's
     first real use (Rarog's estimator-bias lesson).
   - **(b) build pooling** — `nps_multibuild.ps1` or a `-Builds N` flag:
     ≥2 independent PGO builds per arm, per-build medians printed so
     non-overlap is visible (the 0.36% PGO-luck offset).
   - **(c) speed telemetry counters** (8.6.6 DiagCounters pattern —
     always-counted int64s, NIL-cost class, bench-identical Diag off+on):
     `evaluate()` call count (→ eval rate as % of nodes), pawn-cache
     probe/hit (eval.cpp:426-436), full-`gives_check` call count
     (board.cpp:1356), `see_ge` call count + repeat-calls-per-node.
     These are the counters 8.7.3/8.7.5/8.7.8 read before touching
     anything — Rarog's decisive "eval at 34.5% of nodes, 88.4% cache
     miss" style numbers are currently **unmeasurable here**.
   - **(d) phase-start baseline:** pooled-PGO NPS + a counter harvest
     (startpos d16 + bench), recorded next to 8.6.6's baseline.
   Gate: self-pair validation passes; counters bench-identical.
2. **8.7.2 Profile-first pass — ✅ DONE 2026-07-23 (REDONE: the first
   attempt was invalid, see below).**

   **RESULTS (PGO, dedicated idle box, 10 rounds/region, every probe
   self-identified and bench-identical at 11,941,440; control baseA-vs-baseB
   read +0.10%, CI [−0.24%, +0.30%] = noise floor ±0.3%):**

   | region | NPS when doubled | **share of runtime** | 95% CI |
   |---|---|---|---|
   | **eval** | −19.54% | **24.3%** | [24.0, 24.5] |
   | **score_moves** | −6.69% | **7.2%** | [6.9, 7.4] |
   | **make+unmake** | −5.55% | **5.9%** | [5.6, 6.0] |
   | **movegen** | −5.48% | **5.8%** | [5.6, 6.0] |
   | **see_ge** | −3.94% | **4.1%** | [3.8, 4.3] |
   | **gives_check** | −2.34% | **2.4%** | [2.1, 2.7] |

   Attributed 49.7%; the remaining ~50% is TT probe/store, history updates
   and diffuse search plumbing (no single optimisable region).

   **⇒ THE PHASE ORDER IN THIS PLAN IS WRONG. Re-ranking:**
   - **8.7.3 was the headline on the strength of `gives_check` running
     1–2×/node — but `gives_check` is the SMALLEST region at 2.4%.** High
     call count, cheap per call: the 8.7.1 counters alone would have sent us
     the wrong way, which is exactly what "profile before speculating" is
     for. 8.7.3 survives ONLY because its second half (threading the check
     hint into `make_move` to skip the unconditional checkers scan) draws on
     the **5.9% make+unmake** region. **Reframe: implement the make-side
     check hint FIRST; the `gives_check` replacement is the minor half.**
   - **`score_moves` (7.2%) is the largest search-side region and is
     currently only a "micro-sweep" (8.7.6). PROMOTE it.**
   - `movegen` 5.8% (8.7.4 pin sharing) is on par with make — keep.
   - 8.7.8 pawn-cache already dead from the 8.7.1 counters (89% hit).
   - **eval 24.3% dominates everything, but Phase 10 replaces the evaluator
     wholesale — HCE eval work is 1.9.1-only value. USER DECISION PENDING.**
     Note the asymmetry: `make_move` work *compounds* after NNUE (the
     accumulator attaches to that same `do_move` seam), so it is the most
     future-proof target on the list.
   - Search-side regions total 25.4%; capturing 20–30% of that is **+3–5%
     NPS, i.e. the phase target is reachable without touching eval.**

   **⚠ THE FIRST ATTEMPT WAS INVALID — two independent causes, both worth
   remembering:**
   1. **User load contaminated part of it** (reported; all its numbers
      discarded).
   2. **`cmake/pgo-build.cmake` silently drops `CMAKE_CXX_FLAGS`.** It
      re-configures its own `-pgo-generate`/`-pgo` build dirs forwarding
      only `COMP`/`PORTABLE_BUILD`/`TUNE`/PGO flags, so `-DBASILISK_PROBE=N`
      never reached the compiler and every "PGO probe" was a plain base
      build reading ~0%. **Workaround: set the `CXXFLAGS` environment
      variable** (the script wipes and re-configures both dirs, so a fresh
      configure picks it up).
   **Two false conclusions I drew from that bad data, recorded so they are
   not repeated: (a) "the optimizer is CSE-ing the duplicate away" — there
   was no folding, the probe simply was not compiled in; the argument
   laundering added to defeat it is a harmless no-op. (b) "differing SHA-256
   hashes prove the probes differ" — PGO builds are not byte-reproducible,
   so distinct hashes prove nothing.**
   **⇒ PERMANENT GUARD ADDED: every probe binary SELF-IDENTIFIES** —
   `bench` prints `PROBE REGION : N` under `#ifdef BASILISK_PROBE`. Never
   trust a probe number without that line. Note the null control
   (base-vs-base) CANNOT catch this class of error: with the treatment
   never applied, both arms are identical and the control passes happily.
   That needs a **positive** control, which the marker provides.
   Probe scaffolding is working-tree-only (never committed); the reusable
   patch + `probe.h` are saved in the session scratchpad.

   Original spec:
   **⚠ TOOLING BLOCKER FOUND 2026-07-23 — Rarog's ETW/xperf route does NOT
   port here.** `xperf` IS installed on this box
   (`C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit`)
   and Rarog's `tools/profile_etw.ps1` needs an **elevated** shell (kernel
   stack-walk sampling is admin-only, so a human runs it). But xperf/WPA
   resolve symbols from **PDB**, and our toolchain is MSYS2 clang targeting
   `x86_64-w64-windows-gnu` → **DWARF**, so our own frames would come back
   unresolved. Rarog never hit this: Rust on Windows builds through the
   MSVC toolchain and emits PDB natively. Getting PDBs here means a
   clang-cl/MSVC-ABI build, i.e. **different codegen from the binary we
   ship** — a profile of the wrong artifact.
   **⇒ Adopt Rarog's OTHER tool instead as our PRIMARY: `profile_attrib.ps1`
   duplication attribution.** Run a region's work TWICE in a probe binary;
   `f = NPS_base/NPS_dup − 1` is that region's share of search runtime.
   It needs **no symbols and no elevation**, runs on the **shipped PGO
   binary**, plugs straight into the 8.7.1 `nps_ab.ps1` instrument, and is
   immune to the inlining-attribution skew that makes sampling unreliable
   on PGO+LTO builds anyway. Cost is one probe build per region — screen
   with fast non-PGO builds to RANK regions, then confirm the top 2–3
   under PGO. Regions to probe (Rarog's list, mapped): eval total /
   pawns / activity, tt_probe, gen_captures, gen_quiets, score_quiets,
   **gives_check** and **make_move** (the two the 8.7.1 counters already
   implicate). ETW stays available as an optional breadth cross-check if
   the user runs it elevated AND we accept a PDB-bearing side build.
   Original spec: Profile the pext-PGO binary on bench + a WAC slice +
   endgame FENs (tool: whatever works on this box — VTune / AMD uProf /
   ETW+WPA / samply; record the working recipe in the dev guide).
   Confirm or kill, with shares: `gives_check` + `attackers_to` cost
   (feeds 8.7.3), `see_pins` share (8.7.5), movegen+copy share (8.7.6),
   eval-tail split (8.7.7), TT-probe stalls. Profile findings re-order
   the steps below and may add new sub-steps — Rarog's 10.3 found its
   biggest item (the pick_next scan) only by looking. No gate (no code
   change); output is a ranked table in the dev guide.
3. **8.7.3 Per-node CheckInfo + check-hinted make_move — ❌ REJECTED &
   REVERTED 2026-07-23, CLOSED PERMANENTLY (2nd failure; 8.5.1 was the
   1st).** Implemented per the Rarog spec: one `CheckInfo` per node built
   from just TWO magic lookups (the enemy-king bishop/rook attack sets serve
   both direct-check squares AND discovered-check candidates —
   `disc = (bishop|rook)_from_k & ours`); `gives_check_hinted` replacing the
   full routine (promo/EP/castling + discovered-candidate movers fall back);
   the answer threaded through `do_move`→`make_move(m, known_no_check)` to
   skip the unconditional `attackers_to` checkers scan on proven-quiet moves.
   **Verified CORRECT** — full ASan/UBSan CTest 11/11 with a live
   `assert(gives_check_hinted == gives_check)` on every move both directions,
   bench-identical 11,941,440. **But a NET SPEED LOSS under the 8.7.1
   instrument (pooled PGO, idle box, 12 rounds):**
   - eager (hint computed for every move at search_one top): **−2.67%**, CI
     [−3.25, −1.99], 0/12.
   - lazy (hint computed just before `do_move`, only for moves that survive
     pruning, reusing the guard lambda's value when present): **−1.83%**, CI
     [−2.14, −1.50], 0/12.
   The make-side scan skipped for non-checking moves does not outweigh the
   per-node `CheckInfo` build + `gives_check_hinted` cost, even when the hint
   is paid only for played moves. Matches the 8.5.1 result and the prior-art
   warning. **Per the pre-registered "negative → close permanently": DONE.**
   Not reopenable without a fundamentally different mechanism (e.g. a hint
   that is a genuine by-product of work already done, not an added compute).
   **Bonus find, KEPT (see §7 LMR-post-move-gives_check bug): the equivalence
   work exposed that the LMR gate consumes a post-move `gives_check`.**
   Original spec below (kept for the §7 bug context and any future retry):
   - Build ONE `CheckInfo` per node in negamax: `check_squares` per piece
     type (primitive exists — board.cpp:1344-1354, already used by
     ordering) + discovered-check blockers.
   - Replace full `gives_check` (board.cpp:1356-1416 — direct slider
     lookup PLUS two discovered-check slider scans from the enemy king)
     with the two-bitboard test at the pruning lambdas + LMR gate
     (search.cpp:1443-1500, :1581 — fires for essentially every searched
     quiet at depth ≥ 2) and the qsearch delta-prune site
     (search.cpp:1088); promo/EP/castling fall back to the full routine
     (Rarog's exact shape).
   - Thread the now-known `gives_check` through `do_move` → `make_move`
     (the 8.6.10(d) seam exists for exactly this — search.h:264):
     non-checking moves set `checkers = 0` and **skip the unconditional
     `attackers_to` scan** (board.cpp:678); checking moves keep the scan
     (Rarog's conservative shape — hint only the EMPTY case).
   - **Prior-art warning, recorded for honesty: 8.5.1 tried "cached check
     geometry + make-move hint" pre-1.9.0 and was a net NPS regression,
     reverted.** Why the retry is legitimate: 8.5.1 predates the 8.6.10
     do_move seam (its plumbing was invasive), predates this phase's NPS
     instrument (the verdict was a single unpooled comparison), and
     bundled a full per-ply StateInfo restructure. This retry is the
     narrow Rarog scope only. If it measures negative under 8.7.1's
     instrument, close permanently.
   - Equivalence: `assert` hint == fresh `attackers_to` in debug through
     the whole ASan suite (Rarog asserted both directions); bench-
     identical. Est here: **+1.5–3.5%** (ordering half is pre-done).
4. **8.7.4 Pin sharing across generation stages — ❌ REJECTED & REVERTED
   2026-07-23 (measured −0.16%, CI [−0.36, +0.05], 3/12; complexity not
   worth a neutral-to-negative result).** Implemented fully and VERIFIED
   correct (bench-identical 11,941,440; a debug stale-pin assert rode
   through 109 s of ASan/UBSan fuzz + search without firing), then measured
   neutral-to-slightly-negative. Why it doesn't pay here (unlike Rarog's
   ≲1%): the shared x-ray only benefits nodes reaching the QUIETS stage, but
   many nodes cut off during captures and never reach quiets — they pay the
   refactor overhead (extra indirection, pointer param that inhibits some
   inlining the inline path got) for no saving; and the x-ray is only a few
   magic lookups, cheap vs the rest of movegen. Per the user's stop rule
   (worth ≤ 0 for real complexity cost in delicate movegen), reverted. The
   extracted `compute_movegen_pins` helper + shared-pin overloads are gone;
   the one-line idea is documented here if a future movegen rewrite makes it
   free. Original spec:
   `pinned` is computed inline on every `gen_legal` call
   (board.cpp:1057-1078); the staged picker generates captures and quiets
   in separate calls (search.cpp:761-782), so nodes reaching the quiets
   stage pay it twice (qsearch pays it per `gen_legal_captures`). Compute
   once per node, pass into both stages; stale shares caught by a debug
   assert against a fresh compute (Rarog's guard). Gate: bench-identical;
   keep on the structural argument if below resolution.
5. **8.7.5 SEE work reduction — ✅ DONE 2026-07-23: (a) ACCEPTED +0.36%
   (`873de47`); (b) SKIPPED (not cleanly cacheable); (c) INVALID (see()
   is a test fixture, reverted).**
   - **(a) memoize the good/bad verdict — ✅ +0.36% NPS** (CI [−0.06, +0.67],
     10/12, bench-identical, memo assert clean through the ASan fuzz suite).
     Picker exposes `last_see_score()` (0 good / −1 bad capture by stage);
     search_one seeds `see_score` with it, skipping both duplicate
     `see_ge(m,0)` recompute sites. Original spec:
     the picker's `see_ge(m, 0)`
     classification (search.cpp:789) is thrown away, then recomputed at
     the capture-SEE-prune (search.cpp:1500) and the LMR/history
     `see_score` site (:1509/:1662 — which already has a `VALUE_NONE`
     memo channel; extend it to carry the picker's verdict).
   - **(b) per-node SEE pin geometry — ⏭ SKIPPED 2026-07-23 (not cleanly
     cacheable; escape hatch taken).** `see_ge` builds a move-specific
     occupancy `occ = all_occ ^ from [^ to]` and passes THAT to `see_pins`,
     so removing the mover/target changes which pieces are pinned — there is
     no from/to-independent part to cache. The only alternative (incremental
     pin maintenance across a 1–2-bit occ change inside the exchange loop) is
     high-risk complexity in the correctness-critical SEE path for a small
     slice of a 4.1% region that (a) already partly reclaimed. Fails the
     worth/price rule. Original spec: `see_pins` runs a both-color x-ray scan
     inside every `see_ge`; cache the from/to-independent part — if nothing
     is cleanly cacheable, skip (Rarog 8.12(b) est +0.5–1.5%, did not
     transfer).
   - **(c) rider — ❌ INVALID, NOT DONE (2026-07-23): `Board::see()` is
     NOT dead.** The audit claimed "zero call sites" but only checked `src/`
     — `tests/test_board.cpp:855-1128` uses `see()` as a SEE-correctness
     unit-test fixture (asserts exact signed values 100/300/900/−200 across
     a suite). Deleting it (committed `fe7a629`) broke the debug/ASan build;
     reverted (`ccf9727`). `see()` is test-only but provides real SEE
     coverage — KEEP it. Lesson: "dead code" audits must grep `tests/` too.
     Its production comment ("feeds ordering") is stale (ordering uses
     MVV+capture-history) — that could be corrected in place, but the
     function stays.
   Gate: bench-identical per sub-item.
6. **8.7.6 Picker + scoring micro-sweeps — ✅ DONE 2026-07-23. (b)+(d)
   ACCEPTED +3.03% NPS (`7f133de`); (c) TT prefetch at null/ProbCut kept
   NPS-neutral on consistency (+0.12%, sub-resolution, not counted,
   `e1475c7`); (a) generate-into-ScoredMove and (e) history-stack DEFERRED
   (user decision — the phase target was already banked by (b)).** Promoted to the top search-side target
   by the 8.7.2 profile (score_moves 7.2%). The conthist row-pointer hoist (b)
   is the headline and carries essentially all of the +3.03% — nearly the
   whole phase target from one sub-item. Incremental base for the remaining
   sub-items = `basilisk-876bd-scoreopt{A,B}` (the +3.03% head).
   (Rarog's "small sweeps" +1.18% analog; each measured, keep-or-revert):**
   - **(a)** generate moves **directly into the `ScoredMove` buffer**:
     `fill_tacticals`/`fill_quiets` build a transient 516-B `MoveList`
     then copy it (search.cpp:761-782) — twice per node.
   - **(b)** conthist row-pointer hoisting (the standard SF pattern):
     the `(ss-1/2/4)` guards, derefs and two array-dimension multiplies
     re-evaluate per scored quiet (search.cpp:452-470); hoist the three
     row base pointers once per node, and reuse the computed 4-table sum
     at the hist-prune (:1458-1461) and LMR-stat (:1554-1557) sites
     instead of recomputing it.
   - **(c)** TT prefetch after the ProbCut/null/singular `do_move` sites
     (search.cpp:1004, 1106, 1135, 1362 — the main move loop is already
     covered at :1564).
   - **(d)** `pick_next`: keep the running best score in a local (drops
     the per-iteration `best->score` reload; the loop shape stays).
   - **(e) measure-or-close:** `UndoInfo` (~72 B) `history.push_back`
     per make_move (board.cpp:600) vs a flat ply-indexed stack. 8.6.10(a)
     made history growable **deliberately** (any `position` length
     correct) — if this measures, the fix is a hybrid (flat search stack,
     vector only for the root `position ... moves` path), never a revert
     of the correctness property. Below resolution → close, keep vector.
   Gate: bench-identical per sub-item.
7. **8.7.7 Eval-tail redundancy — ✅ DONE 2026-07-23: (c) +0.89% + (a)
   +0.39% (both bench-identical 11,941,440, tuner --verify 8598/8598, CTest
   11/11; commits `6c30c58`, `0d90935`). (b) SKIPPED — not provably
   identical (the `attacked[]` map it would reuse is built ~100 lines after
   the dynamic-passer loop; per its own spec, skip unless provable).**
   (c) hoisted the two per-piece `switch(pt)` out of the mobility inner loop
   via a C++23 per-type template lambda (clang was NOT already specializing
   the range-for-over-initializer-list). (a) cached each swept bishop/rook
   attack set (`slider_att[sq]`) and reused it at the four eval-tail tests
   that recomputed `bishop/rook_attacks(sq, b.all_occ)` (minor/rook king-ring,
   connected-rooks, trapped-bishop). Original spec:
   (equivalence-class: values must not change, so bench-identical by
   construction).**
   - **(a)** reuse per-piece slider attacks from the main attack sweep
     (eval.cpp:853-854) at the king-ring tests (eval.cpp:1283, 1336,
     1357) and trapped-bishop test (:1409) — up to ~8 redundant magic
     lookups per non-lazy eval today.
   - **(b)** the dynamic-passer loop calls `is_attacked_by(stop, ...)`
     per passer (eval.cpp:670) while `attacked[them]` is built ~100 lines
     later — reorder/reuse ONLY if provably identical (the map must cover
     the exact attacker set the predicate tests); otherwise skip.
   - **(c)** hoist the loop-invariant `switch (pt)` out of the mobility
     inner loop (eval.cpp:851-882) into per-type loops so the inner
     `while (pcs)` carries no per-piece branch.
   Gate: bench-identical (any fingerprint change = a bug, full stop).
8. **8.7.8 Pawn-cache sizing — ❌ REJECTED 2026-07-23 (no code; killed by the
   8.7.1(c) counter harvest before spending a step).** Premise was Rarog's
   88.4% MISS; ours is the inverse — **88.7–99.6% HIT** across all four probe
   position types (startpos 89.0 / middlegame 94.1 / R+P endgame 99.6 / pawn
   endgame 98.1). At ≥~90% hit the direct-mapped 768 KB/thread cache is already
   near-optimal: enlarging it can reclaim at most the ~10% miss tail (below the
   ±0.25% instrument resolution) at real L2/L3 cost, and shrinking is strictly
   worse. No sweep worth running — the profile-first "confirm/kill with shares"
   discipline paying off. Original spec:
   `PawnEntry` is 48 B × `PAWN_TABLE_SIZE` 16384 = **768 KB per thread**,
   direct-mapped, embedded in `Evaluator` (eval.h:6-39). Read the 8.7.1(c)
   hit-rate counter under bench + a game-condition harvest FIRST; sweep
   the size only if the counter says so — **in both directions** (768 KB
   of L2 per thread is not free; smaller may win). Bench-identical by
   construction (the cache stores exact recomputable values). Below
   resolution → close.
9. **8.7.9 — (a) ❌ RETIRED 2026-07-23 (user decision: anti-pattern);
   (b)/(c) doc-honesty riders → folded into the 8.6.9 release checklist.**
   - **(a) PGO workload enrichment — RETIRED.** Training PGO on more than
     `bench` (WAC slice + endgame FENs + deeper bench) is an anti-pattern:
     bench is the single reproducible training source, its 40 positions
     already span opening/middlegame/endgame so every hot path is profiled,
     and enrichment risks over-fitting PGO to tactical position types for
     ~0 measurable gain. **This reverses the deliberate 1.8.0 decision**
     ("PGO training now uses the 40-position bench suite only; the separate
     depth-7 EPD set removed as redundant — the broadened suite is itself a
     representative execution profile") and is not reopened. Original spec:
     `cmake/pgo-build.cmake:36-41` trains every shipped binary on a single
     `bench 13` run; enrich with deeper bench + WAC slice + endgame FENs.
   - **(b)** rider: confirm + record the portable tier's POPCNT story
     (PORTABLE_BUILD compiles with no `-march` at all — release.yml:160
     — so `std::popcount` may lower to a software fallback on the
     baseline-x86-64 asset; that IS the tier contract, but it should be
     a documented fact, not an accident; the avx2/pext tiers are the
     fast path).
   - **(c)** rider (doc honesty, found by this audit): `release.yml`
     produces neither the `*.manifest.txt` (with per-tier NPS) nor the
     `*.sha256` that docs/release_tiers.md:4-8 promises — either add the
     generation step or amend the doc to match the 8.6.5 local-only
     manifest decision.
10. **8.7.10 Baked magics + startup — ✅ DONE 2026-07-23 (`bc93f26`):
    portable startup 603 ms → 38 ms (~16×; Rarog 192→19).** Measured the
    baseline first (none existed). Dumped the deterministic `find_magic`
    output (seed 1234567890123) to `src/magics.h`; `init_attacks` tries the
    baked magic via a new `commit_magic()` (validate against the full
    occupancy enumeration → build table), skipping the ~1e8-attempt search;
    a stale value falls back to the live search (`g_baked_magic_fallbacks++`
    — startup cost, never correctness). `test_magics`
    (`baked_magics_cover_every_square`) asserts 0 fallbacks, run under the
    non-PEXT debug/ASan build (the config that uses magics); vacuous on PEXT.
    Bench-identical both tiers (11,941,440 — same attack tables). Regenerate
    `src/magics.h` with a `-DBASILISK_DUMP_MAGICS` portable build if the
    search ever changes. Original spec:
    (Rarog: 192 ms → 19 ms on its generic build). The non-PEXT tiers — portable, avx2, and
    ALL aarch64, i.e. most shipped assets — run `find_magic`'s random
    search (up to 1e8 splitmix64 attempts per square × 128 squares) **at
    every launch** (attacks.cpp:102-125, 218-231). Measure startup first
    (no number exists today), then bake: the seeded search is
    deterministic, so the baked constants ARE what it computes; init
    verifies each baked value and falls back to searching (a stale
    constant costs startup time, never correctness), plus a test
    asserting the fallback stays unused (Rarog's
    `baked_magics_cover_every_square` shape). Pays in harness/tool
    invocations and ultra-fast TC, not game strength — cheap, do late.
11. **8.7.11 Phase close-out — ✅ DONE 2026-07-23.** End-to-end fresh
    pooled-PGO head-vs-phase-start (871-head): **+4.34% NPS, CI [4.08, 4.59],
    16/16 rounds**, both arms bench-identical 11,941,440 — close to the
    per-item sum (+4.8%), so NO hidden compounding regression (the Rarog
    "eight neutral steps → −3.2%" failure mode did not occur). Post-phase
    counter harvest (startpos d16): every node count identical to 8.7.1(d)
    (search tree unchanged); `see_ge` **0.631 → 0.514/node** (−18.5%, the
    real calls 8.7.5(a) killed); eval/pawn-cache/gives_check rates unchanged
    (those items cut cost-per-call, not call count). **Batch verdict:
    +4.34% ≥ +2% ⇒ the pre-registered [-3,0] STC SPRT** (Phase-8.7 head vs
    phase-start). **✅ CONFIRMED 2026-07-23: +8.69 ± 6.63 Elo, nElo
    +14.64, LLR +1.49 climbing toward H1, 95% CI [+2.06, +15.32] EXCLUDES
    ZERO, 0 forfeits @3,760 games — user-stopped (estimate-judged: CI off
    zero + lands on the ~+8–9 the +4.34% NPS predicted, so H1 is
    established and the exact LLR crossing is a formality).** The +4.34%
    NPS translates to a real ~+8.7 Elo game gain — **the speed pass is a
    genuine STRENGTH increase, not zero-change (feeds the 1.9.1-vs-1.10.0
    version call at 8.6.9).** Phase 8.7 CLOSED.
    Accepted-item summary: 8.7.6(b+d)
    +3.03%, 8.7.7(c) +0.89%, 8.7.7(a) +0.39%, 8.7.5(a) +0.36%, 8.7.6(c)
    neutral-kept, 8.7.10 startup 603→38 ms; rejected 8.7.3/8.7.4/8.7.8,
    retired 8.7.9, skipped 8.7.5(b), invalid 8.7.5(c). Original spec:
    One end-to-end head-vs-phase-start NPS
    measurement (pooled PGO, both arms rebuilt fresh); post-phase counter
    harvest next to the 8.7.1(d) baseline; the pre-registered batch
    verdict (≥ +2% → `[-3,0]` batch SPRT; else fixed 10k probe); per-item
    results recorded in the dev guide's Phase-8.7 block (Rarog's
    dev-guide 10.3 block is the template).
12. **8.6.9 ⭐ Release 1.9.1 — the final HCE release — ✅ PUBLISHED
   2026-07-24.** Moved here 2026-07-22;
   number kept — order ≠ numbering. Gated on 8.6.1–8.6.7,
   **the 8.6.8A accept-audit, and Phase 8.7 resolved** (Rarog 9.8
   precedent: the release waits for the whole wave, not its first passing
   item) — all satisfied. Model-side prep complete: version 1.9.1 in
   `constants.h` + `CMakeLists.txt` (verified), CHANGELOG `[1.9.1]` rewritten
   (Phase-8.7 speed summary +4.34% NPS ≈ +8.69±6.63 Elo, 8.6.8A audit note,
   "why still a patch"), README baked-magics note, `GUIDE.md`
   restructured to future-focus, PGO ISA matrix via the 8.6.5-hardened
   `release.yml`, release notes drafted. Cumulative `development`-vs-1.9.0
   evidence is the Phase-8.7 batch SPRT (+8.69±6.63).
   **Folded-in 8.7.9 doc riders — ✅ BOTH DONE 2026-07-23 (`95c1702`),
   no code-behaviour change:** (b) `docs/release_tiers.md` now documents the
   portable tier's software `popcount` (verified by disassembly: portable 0
   vs `-pext` 97 `popcnt` instructions — the pext/avx2 tiers are hardware
   POPCNT, so no speed is left on the table there); (c) the doc's promised
   per-asset `*.manifest.txt`/`*.sha256` (which `release.yml` never uploads)
   is amended to match reality (binary-only assets + CI smoke test; manifest
   stays local per 8.6.5). **user (manual):** squash `development` →
   `master` as `Version 1.9.1`, push, then **publish a GitHub Release**
   (`gh release create v1.9.1 --target master --notes-file <notes>` — a bare
   tag does NOT fire `release.yml`, which triggers on `release: published`).
   **1.9.1 then replaces
   1.9.0 everywhere as the frozen HCE baseline: 8.5.15 baselines it,
   Phase 10's HCE comparison is against it, and the `nnue` rebase targets
   the post-1.9.1 `development` SHA.** Prep already done 2026-07-20/21
   (version bump, CHANGELOG draft, doc rename, refactor-equivalence gate
   ✅) carries over.
   ⚠ **Amended 2026-08-01:** Phase 9 (§5) ran on `development` before the NNUE
   runway and now ships as **1.9.2** by explicit user decision. Its release
   gates passed on 2026-08-01, so 1.9.2 is the frozen HCE baseline and the
   handoff SHA is post-1.9.2. Version 1.9.2 was published on 2026-08-01;
   1.9.3 follows as a tooling-only PGO repair with the same engine behavior.

**Execution order across the NNUE boundary (explicit):**

1. **Pre-1.9.0 (HCE, `development`):** the Phase-8.5 pre-1.9.0 items — Track D
   (TT-density [SPRT-pending], cuckoo, TT-PV, history ladder, root/TM) and the
   **retry candidates** now unblocked by the robust canary (8.4, 8.5.5,
   PostLmrHistScale re-bake, inert-knob re-exam). Each SPRT/SPSA-gated; keep
   the accepted ones.
2. **⭐ 1.9.0 RELEASE — ✅ DONE 2026-07-17.** All accepted Phase-8/8.5 work
   squashed to `master` as the single `Version 1.9.0` commit; version → 1.9.0
   (constants.h + CMakeLists), CHANGELOG `[1.9.0]` written, CTest 11/11, engine
   reports `Basilisk 1.9.0`. `development` is reset to this `master` state to
   continue. **Still manual (user):** tag `v1.9.0` + push `master`; optional
   cumulative `instabtm`-vs-1.8.0 confirmation gauntlet (STC + `10+0.1`) for
   the shipped number.
3. **Phase 8.6 (added 2026-07-20) → Phase 8.7 (added 2026-07-22)
   → ⭐ 1.9.1 RELEASE — ✅ DONE.** The hardening/CI/telemetry wave + the
   8.6.8A accept-audit, then the profile-guided speed pass (Phase 8.7 — the
   release step 8.6.9 lives at its end); 1.9.1 is the last shipped HCE
   release.
4. **✅ Phase 9 (§5, CLOSED 2026-08-01) — harness, SMP
   and pre-NNUE durability, on `development`.** Inserted here, *ahead of* the NNUE runway,
   because 9.1/9.2 are measurement prerequisites for everything after them
   and because 1T+4T deployment (gate 10) turns the audited SMP defects into
   strength defects. The original 1.10.0 trigger was met by 9.4's MT result,
   but the user selected **⭐ 1.9.2** on 2026-08-01 because the final accepted
   production scope is focused. The 1T/4T boundary gates passed; only manual
   publication remains.
5. **Post-Phase-9 NNUE runway (`development` → `nnue`) — pure NNUE data
   prep only (structure era moved pre-release → 8.6.10):** 8.5.3
   dirty-piece (attaches to 8.6.10's centralized do_move/undo_move +
   `PlyContext`), 8.5.14 TT graph-history (down-scoped), 8.5.15 teacher
   benchmark (baselines the released **1.9.2** HCE head), 8.5.16 trainer
   preflight; record the exact handoff SHA and
   **rebase `nnue` onto it once**.
6. **Phase 10 → 11 → 12 → 13** (all post-NNUE), below.

---

## 5. Phase 9 — harness, SMP and pre-NNUE durability (CLOSED)

**Added 2026-07-28.** Two sources, kept separate on purpose so a claim can be
traced to its evidence:

- **(R) the Rarog Phase-8→10 cross-review** (read at `b5d88f2^`, i.e. before
  that repo condensed its completed phases). Rarog ran the SMP, harness and
  SPSA work we have not, and both its wins *and* its nulls are free evidence.
- **(A) an in-session audit of our own threading, time-management, harness and
  move-scoring code**, 2026-07-28. Every finding below carries the file and
  line it was read at, and none of them is a transcription of a Rarog item.

**Why this phase runs NOW, before the NNUE runway.** Three independent reasons,
in descending order of force:

1. **9.1 and 9.2 are measurement prerequisites.** Our SPSA schedule is
   dimensionally wrong (§9.1), so *every* tune we run from here — including
   the one big post-NNUE tune at 11.7 — under-anneals by construction. Our
   SPRT harness hardcodes `Threads=1` (§9.2), so we cannot gate a
   multi-thread change at all. Fixing measurement after spending machine time
   on it is the one ordering that is always wrong.
2. **Gate 10 (1T+4T deployment) reclassifies the SMP findings.** The audit
   found a latent 4T time-forfeit exposure, helpers that stop searching while
   the main thread is still extending, and an unbounded `Threads` cap. Under
   1T-only deployment those were curiosities; under 4T they are strength and
   robustness defects in a shipped configuration.
3. **It is all NNUE-durable.** Nothing here touches the evaluator. Harness,
   thread coordination, clock safety and node-invariant hoists survive the
   eval swap untouched, so doing them now costs the NNUE line nothing and
   makes every measurement it depends on trustworthy.

**Standing constraints for the whole phase**

- 1T behaviour is **bench-identical** unless a step says otherwise; every
  MT-only mechanism is gated on `shared_state`/`thread_count > 1` so the 1T
  fingerprint (11,941,440) cannot drift silently.
- ⚠ **Bench-identity is not NPS-identity** (Rarog measured a 3.2% NPS loss
  across a program of individually "bench-identical, NPS-neutral" refactors).
  Any step touching a hot path carries a pooled-PGO `nps_ab.ps1` reading, and
  9.11 runs ONE end-to-end 1T NPS check against the phase-start head.
- MT gates run at **Threads=4, Hash 256, ~10k games minimum** (gate 10), after
  a Threads=4 null calibration. 1T gates keep `Hash 64`.
- ⛔ **Engine-side thread/core pinning is out of scope, permanently** (matches
  Rarog's standing user decision). Harness-side affinity for SPRT/SPSA is a
  separate, legitimate thing and is unaffected.

**Execution order (dependencies, not value):**
`9.1 ∥ 9.2` (no games, either order, both block what follows) → `9.3`
(enabler: safety + counters + telemetry) → `9.4` (clock safety before any
mechanism that spends more time) → **`9.5` (the value item)** → `9.6 ∥ 9.7`
(interleave with 9.5's gate wait) → `9.8` → `9.9` → `9.10` (needs 9.5c) →
`9.11` (OPTIONAL Texel re-fit) → `9.12` close-out + release decision.
9.6 has no dependency on the SMP chain at all and can be pulled forward into
any gate wait — it is the only step that pays at both 1T and 4T with no games.
9.11's *datagen* is likewise free-floating (nothing in this phase changes 1T
fixed-node search), so if it is run at all, generate the corpus during an
earlier gate wait rather than idling the box in this slot.

**Not duplicated here — these stay where they are, deliberately:**

| Item | Lives at | Why not in Phase 9 |
|---|---|---|
| `cutoffCnt` reduction input | 11.1 mechanism / 11.7 re-tune | needs the unified `r`; its prior is now **negative** (R: −7.78) — see the correction recorded at 11.1 |
| LMR post-move-`gives_check` fix | 11.1 (by construction) / 11.7 (re-tune) | standalone fix already measured −21 here; it is a de-tuning victim, so it only makes sense inside the LMR re-tune |
| History-coverage residue (scoped capture malus, countermove aging) | 11.7 SPSA | a dedicated SPSA now would violate gate 11's "re-tuning a fitted group is low-EV"; R fitted the same items as *scaled* knobs inside one joint tune (+6.01) — that is the shape 11.7 should use |
| Correction-history update semantics | 8.5.5 | still canary-blocked pre-NNUE; **EV downgraded 2026-07-28** by R's decomposition: the tactical guard is worth **−56** and everything else in that family ≈ +1.4 combined. Do not revive the guard |
| Fail-soft qsearch prune exits | closed (8.1f) | measured here already: the fail-hard `alpha` echo is load-bearing for mate-distance bounds (KQK mate-in-5 → "mate 63"). R independently rejected its own version at −5.96 |
| do-deeper re-search | closed | rejected here (§7 deferred) and at R (−7.29) |
| TT multiply-hi full-budget indexing | §7 deferred / 12.x | large-hash/LTC item, belongs with the 12.x memory work |

---

1. **9.1 SPSA harness repair + doctrine — no games; blocks every future tune.**
   **(R + A.)** Rarog found the bug in its own tuner and explicitly relayed it;
   the audit confirmed our copy is affected **verbatim**, not merely similarly.
   - **The defect, verified here:** `tools/weather-factory/spsa.py:68` advances
     `self.t += self.cutechess.games` — `t` counts **games** (32 per iteration)
     — while `tools/spsa.ps1:193-194` writes `A = Iterations / 10` in
     **iterations** and `a: 1.0`. Both schedule terms read `t`
     (`a_t = a/(t+A)^alpha`, `c_t = c/t^gamma`), so `A` is effectively 1/32 of
     its intended value: the "damp the first 10% of the run" term damps
     essentially nothing, and the gain decays ~8× faster than the design
     implies. **Every Basilisk SPSA ever run, `hcefinal` included, annealed far
     too fast.** Accepted bakes each won a real SPRT and stand; what was lost is
     unrealized upside, and nothing is re-fitted retroactively without a gate.
   - **(a) Schedule fix:** convert `t` to iteration units inside
     `spsa.py::step` (`it = t / games`), keeping `t` and `tuner/state.json` in
     games so existing states resume correctly, and keeping `A = Iterations/10`
     in `spsa.ps1` — which is then dimensionally right.
   - **(b) ⚠ Do NOT ship (a) alone — R shipped it and retracted the same day.**
     For `k ≫ A/32` the old games-fed decay was the correct *shape* times a
     constant (`(32k)^-0.601 = 0.126·k^-0.601`), so `a=1.0` under the broken
     schedule behaved like `a ≈ 0.126` under the fixed one. Restoring the shape
     without the magnitude multiplies every step by ~8; R's
     trajectory-validated simulation read RMSE **0.78 fixed-with-a=1.0 vs 0.53
     broken vs 0.32 at a≈0.1**. So adopt fishtest's **end-state
     parameterization** instead of hand-picking `a`: take `c_end` and `r_end`
     per parameter and back-solve
     `c = c_end·N^gamma`, `a_end = r_end·c_end²`, `a = a_end·(A+N)^alpha`,
     `A = 0.1·N`. Because both constants derive from the planned horizon `N`,
     changing the horizon can no longer silently change end behaviour, and `a`
     can never be left stale. Default `-REnd 0.0031` (fishtest's own default is
     ~0.002; our current `a=1.0` corresponds to r_end ≈ 0.031, ~15× hotter than
     fishtest has ever defaulted to).
   - **(c) Multi-session hardening — all three hazards confirmed present.**
     `main.py:73` is `while True:` (the target iteration count lives only in
     the operator's head); `main.py:50-55` restores `spsa_params` from
     `state.json`, so **`A` is frozen at first launch** and re-passing
     `-Iterations` on a resume is silently ignored; and the run log truncates
     rather than appends on resume (R lost 1,086 of 3,670 iterations that way,
     and the trajectory is precisely what the tail-mean bake reads). Fix all
     three: env-var iteration target with self-stop, append-on-resume, and an
     iteration/percent/ETA line printed on every launch **plus a loud warning
     that `A` is fixed by the first launch**.
   - **(d) Doctrine → gate 11 (§1).** 5,000-iteration floor, tail-mean bake of
     the whole vector, merged groups, no pinned discrete knobs,
     1,500-iteration kill-checkpoint.
   - **Verification (no games):** reproduce the `a_t` table old-vs-new at
     iterations 1/100/600/3673 and check the invariance of `r_end` across
     `N = 1,000 / 2,500 / 5,000 / 10,000`; confirm an existing `state.json`
     still resumes. **Nothing is re-tuned in this step** — it is the
     instrument, not a candidate.

   > **✅ 9.1 DONE 2026-07-29.** All four parts landed, no engine code touched
   > (`bench` and CTest cannot move). Verification:
   > `python tools/verify_spsa_schedule.py` — 30 checks, all pass, and it is
   > the pre-registered evidence above plus two additions.
   >
   > **The overlay, and why the shape changed.** `tools/weather-factory/` is a
   > **gitignored clone**: an edit to `spsa.py` there is untracked, invisible to
   > review, and destroyed by the next re-clone. Only `cutechess.py`'s one-line
   > affinity insert was ever patched in place, and that survives because
   > `setup_tools.ps1` re-applies it. 9.1 rewrites far too much for an anchored
   > insert, so the changed files are kept whole in **`tools/weather-factory-overlay/`**
   > (tracked) and copied over the clone: `Install-WfOverlay` in
   > `harness_common.ps1` installs + `py_compile`s them, and **`Assert-WfOverlay`
   > hash-compares them at BOTH spsa.ps1 setup and launch** — a stale clone
   > silently reinstates the bug and nothing in the run output would say so.
   > Re-run `tools/setup_tools.ps1` after pulling.
   >
   > **(a)+(b) as implemented.** `SpsaParams.schedule()` is now the single
   > definition of `a_t`/`c_t` and converts games→iterations
   > (`it = t / games`); `t` and `state.json` stay in games, so old states
   > resume. The back-solve is `SpsaParams.from_end_state()`, and in
   > weather-factory's factorization (one global `a`/`c`, per-parameter
   > `param.step`) fishtest's per-parameter form collapses to
   > **`c = N^gamma`, `a = r_end·(A+N)^alpha`, `A = 0.1·N`** — `param.step`
   > *is* `c_end`, and it cancels out of the gain. Equality with fishtest's
   > update magnitude is exact at every iteration, not just at `N`; the probe
   > equals one `param.step` at `k = N` by construction. `spsa.ps1` no longer
   > writes `spsa.json` itself (it calls the overlay's `write_spsa_json.py`), so
   > the formula exists once. Measured: the historical schedule's `a_t` was
   > **1.05× → 7.60× too small** across iterations 1 → 3673 (the "~8× faster
   > decay"), `A`'s share of the denominator at iteration 1 goes 0.920 → 0.997,
   > and `c_t` was uniformly `32^gamma = 1.424×` too small. Shipping (a) alone
   > would have inflated the step **6.09×** at iteration 600 — the retraction R
   > made. PLAN's "`a=1.0` ≈ r_end 0.031" is confirmed **at N = 1,000**
   > (0.03007); the old form's implied `r_end` drifts **3.16×** across
   > N = 1k…10k, which is the staleness the parameterization removes.
   >
   > **(c) as implemented.** Self-stop at the horizon; the horizon is **recorded
   > in the schedule (`N`) and wins over the environment**, because a run
   > continued past `N` is annealing against a schedule that no longer describes
   > it — a disagreement prints rather than resolving silently. Resumes print
   > the frozen `a`/`c`/`A`/`N` and state that `-Iterations`/`-REnd` are ignored;
   > `spsa.ps1` repeats the warning before the tuner starts. `watch.ps1` gained
   > `-Append` (SPSA always passes it; SPRT keeps truncate) and a session-start
   > marker, and a fresh setup rotates the old log into `tuner/archive_*`
   > instead of overwriting it. Progress lines carry
   > iteration/target/percent/ETA. **Addition:** the parameter vector is
   > appended to **`tuner/trajectory.csv`** every iteration — the tail-mean bake
   > (gate 11) reads a trajectory, and having its only copy in a console log is
   > what made truncate-on-resume destructive in the first place.
   >
   > **(d)** is in §1 gate 11 already; `spsa.ps1` now **enforces** the floor —
   > `-Iterations` below 5,000 throws unless `-AllowShortRun` is passed.
   >
   > **Two findings worth keeping.** (1) PowerShell cannot read `state.json` at
   > all — `ConvertFrom-Json` rejects it because `a` and `A` collide under
   > case-insensitive keys (the same reason `spsa.json` was hand-written), so
   > the shell reads it through the overlay's `describe_state.py`, which emits
   > deliberately unambiguous key names (`gain_a`/`damp_A`). The first draft hit
   > exactly this collision and printed `a=500.0`; the verifier now pins it.
   > (2) `spsa.py`'s `cutechess` import is deferred under `TYPE_CHECKING` so the
   > schedule maths can be imported and verified without weather-factory's
   > runtime.
   >
   > **Not re-tuned, not re-fitted.** Every accepted bake stands on its SPRT.
   > What the repair buys is future tunes; the first user of it is 11.7.

2. **9.2 SPRT harness: multi-thread gating — no games; blocks every 4T gate.**
   **(A**, with R's incident as the cautionary evidence.**)**
   - **The defect:** `tools/sprt.ps1:359-360` hardcodes `option.Threads=1` on
     both engines and `:390` always passes `-use-affinity $AffinityCpus`.
     There is no way to run a multi-thread gate, and — critically — **removing
     only the hardcode would silently produce garbage**: fastchess 1.8.0 binds
     each *game* to ONE core regardless of the engine's `Threads`, so a
     `Threads=4 concurrency=3` run pins 3 cores for 12 engine threads. R hit
     exactly this and measured **−100 ± 27 purely from starvation** on a
     variant that inspects clean; that run was void.
   - **Changes:** add `-Threads` (plus `-ThreadsA`/`-ThreadsB` for asymmetric
     scaling runs); make the concurrency arithmetic threads-aware —
     `concurrency = floor((physical − 2) / max(ThreadsA, ThreadsB))`, throwing
     on oversubscription (16 physical: T1→14, T2→7, T4→3, T8→1); **drop
     `-use-affinity` entirely when threads > 1** and keep it at Threads=1 where
     one-core-per-game is correct and the pin removes the Zen-3 placement bias
     (lesson 10); default `Hash` to `64 × Threads` (gate 10) and print threads,
     hash and the affinity decision in the run header and manifest.
   - **Also:** teach `-Mode calibrate` to run at the requested thread count,
     and record in the script's help that a 4T verdict needs ~10k games and a
     preceding 4T null.
   - **Acceptance:** a Threads=4 null calibration on byte-identical binaries
     whose 95% nElo CI fits the tolerance, with **zero time forfeits** — that
     null is also 9.4's canary, so run it once and use it twice.

   > **✅ 9.2 IMPLEMENTED 2026-07-29; acceptance run is PENDING (it costs an
   > overnight).** No engine code touched. All of it lives in `tools/sprt.ps1`
   > plus two `harness_common.ps1` helpers.
   > - **`-Threads`** (both sides) plus **`-ThreadsA`/`-ThreadsB`** for an
   >   asymmetric scaling run. `option.Threads=1` is gone from both engine
   >   argument arrays.
   > - **Concurrency is threads-aware** via `Resolve-HarnessConcurrency
   >   -ThreadsPerGame`: `floor((physical − 2) / threads)`, verified on this box
   >   as **T1→14, T2→7, T4→3, T8→1** — exactly the table in this step. An
   >   explicit `-Concurrency` that would oversubscribe now throws with the
   >   arithmetic shown. At `ThreadsPerGame=1` the helper is byte-identical to
   >   its pre-9.2 behaviour, so `gauntlet.ps1`/`spsa.ps1` are unaffected.
   > - **`-use-affinity` is dropped whenever threads > 1** and kept at 1T. The
   >   header and manifest print *which* decision was taken and why, and the MT
   >   path prints a standing warning that an unpinned thread count needs its
   >   own null before any verdict there is trusted.
   > - **Hash defaults to 64 × that side's threads** (64 at 1T, 256 at 4T).
   >   Per-side rather than global so an asymmetric run configures both sides as
   >   deployed; `-Hash` forces them equal, `-HashA`/`-HashB` are explicit.
   > - **Time forfeits are counted and reported on every run**, and called out
   >   as run-invalidating at MT (9.4's canary, ready before 9.4 needs it). The
   >   pattern deliberately excludes fastchess's own `Timeouts: 0` summary line.
   > - **Two guards were widened, because threads are part of "same thing":**
   >   `-Mode calibrate` now also requires equal threads and hash (a null
   >   calibrates ONE thread count), and the degenerate-null throw no longer
   >   fires when the sides differ only by thread count — which is precisely
   >   the 9.12 scaling diagnostic. That diagnostic also gets its own reporting:
   >   the "A is the removal side" framing is replaced by a scaling reading, and
   >   the 8.6.8A −3 Elo classifier is suppressed as not applicable.
   > - **Robustness found while smoke-testing:** fastchess emits `inf`/`nan` for
   >   the estimate and/or error at tiny/degenerate samples (`nElo: inf +/- nan`
   >   on a clean sweep), which the old regexes could not match — the operator
   >   got "could not parse" instead of "your sample is too small". Both report
   >   paths now match those tokens and name the condition
   >   (`Test-HarnessFiniteNumber`); a zero-width interval is called out too.
   > - **Smoke-tested end to end** on real 4-game matches: fastchess confirmed
   >   `4t, 256MB` symmetric, `4t - 1t, 256MB - 64MB` asymmetric, and `1t, 64MB`
   >   pinned to 14 CPUs for the unchanged 1T path.
   >
   > **✅ ACCEPTANCE 2026-07-30 — accepted by the user on a SHORT null, with the
   > pre-registered ±5 bound NOT established.** Recorded plainly because it is a
   > deliberate amendment, not a pass:
   > - Run: 600 games, Threads=4, Hash 256, concurrency 3, 31 min.
   >   **Elo +4.63 ± 18.24, nElo +7.07 ± 27.80**, Ptnml [12,75,120,79,14].
   > - **Time forfeits: 0.** This is the reading that mattered most and it is
   >   clean — it is also 9.4's before-measurement, so 9.4 does not re-buy it.
   > - Mechanics confirmed under load: `4t, 256MB` reached both engines,
   >   concurrency 3 = 12 of 16 cores, no starvation, CPU ~35% as predicted for
   >   3 games × 4 threads on 32 logical.
   > - The script FAILED it: the CI is ±27.80 and the tolerance was ±25. That is
   >   a sizing error of mine, not a harness finding — I scaled from a past
   >   **Elo** CI and applied it to an **nElo** tolerance, and nElo runs ~1.5×
   >   wider here (27.80/18.24 = 1.52).
   > - **Measured throughput: 1,155 games/h at 4T**, so ±5 nElo needs ~18,000
   >   games (~16 h) and 10,000 games would only reach ±6.8. ⚠ `-Games 10000`
   >   with the DEFAULT tolerance 5 would therefore fail again after 9 hours.
   > - **User decision (2026-07-30): keep this null and proceed**, on the
   >   grounds that Rarog independently validated the same 4T configuration.
   >   **Consequence to carry:** we have bounded 4T harness bias at roughly
   >   ±28 nElo, not ±5. That does **not** exclude the ~±10 Elo per-run
   >   placement class of lesson 10, so a 4T result inside ±10 Elo is not
   >   separable from harness bias on this evidence alone. Where that matters —
   >   9.5's `[0,3]` in particular, whose honest prior is +5…+25 — either run
   >   the long null first or treat a sub-+10 reading as unresolved.

2b. **9.3 Thread-count safety, node-counter batching and MT telemetry.**

   > **✅ 9.3 DONE 2026-07-30** (`aba3941` + `0239014`). No games. Hard gates:
   > **bench 11,941,440 — identical**, **CTest 12/12**.
   > - **(a)** The cap is now defined **once**: the two sites computed
   >   `max(1024, 4*hw)` independently and could drift, so
   >   `Parameters::max_threads()` forwards to the definition in `search.cpp`.
   >   A test pins the invariant that survives any policy change — the `uci`
   >   string must quote the same number the pool actually honours.
   >   **⚠ VALUE AMENDED 2026-07-30 (user decision): flat `1024`, matching
   >   Stockfish's `Option(1, 1, 1024)`.** Picking a sane thread count is the
   >   operator's job, and `hardware_concurrency()` under-reports in exactly
   >   the containerised environments where a machine-derived cap would bite.
   >   The first implementation used `min(1024, 4×hw)` (128 on this box).
   >   **Honest correction to this step's framing:** with a flat 1024 the old
   >   `max(1024, 4*hw)` was *already* 1024 on any machine under 256 logical
   >   CPUs, so the "we advertise 1024 and accept it, ~2 GB of history tables"
   >   line overstated it — that is Stockfish's behaviour too, and the real
   >   backstop is the graceful "Threads reduced to N" path on creation
   >   failure, not the cap. What 9.3(a) genuinely repaired is the duplicated,
   >   incoherent formula (and its >1024 result on ≥256-logical machines).
   > - **(b)** Batched at 1024 nodes/thread; `shared_nodes`/`shared_tbhits` are
   >   `alignas(64)`. 1T never sets `shared_nodes`, hence the identical bench.
   >   ⚠ Self-review caught a regression pre-build: checking the limit only at
   >   batch boundaries would miss any `go nodes N` below one batch (~10×
   >   overshoot at 4T). The limit is now checked every node against local
   >   arithmetic — the *atomic* is what got batched, not the check.
   > - **(c)** Pool aggregation lands; see the archived baseline below.
   > - **NPS, stated honestly (`analysis/mt_baseline_9.3.md`):** neutral at both
   >   deployed conditions — 1T identical by construction, 4T 13.55 → 13.63
   >   M nps inside a ±10% spread, 8T flat — and **+12.8% at 16 threads**
   >   (37.04 → 41.79, no overlap across 3 reps). Mechanism-consistent: one
   >   shared cache line, contention scaling with thread count. Justified as
   >   removing a **Phase-12 scaling ceiling**, NOT as a 1T/4T speed win, and
   >   it is an indicative reading rather than the pooled-PGO `nps_ab` protocol
   >   a strength claim would need.
   > - **Instrument defect found and fixed:** `bench` could not measure MT NPS
   >   at all. `run_bench()` has always taken and documented a thread count, but
   >   `Engine::run_bench_command` parsed only depth/repeats and passed a
   >   hardcoded `1`, so `bench [depth] [repeats] [threads]` was unreachable.
   >   Now parsed; default still 1, fingerprint untouched. `README.md`'s claim
   >   that bench honours the `Threads` option was **wrong** (it predates this
   >   phase and was repeated in the 2026-07-29 rewrite) and is corrected.
   > - **MT baseline archived:** `analysis/mt_baseline_9.3.md` — Threads=4 diag
   >   run showing main at 25.6% of pool nodes, main TT hit 39.60% vs pool
   >   40.68% (the number Rarog's throttling experiment moved; our unthrottled
   >   writes keep main level with the pool), **same-key store share 33.90%**
   >   (the new counter, and the quantity 9.5 moves), per-thread depths
   >   16/15/15/16.

3. **9.3 Thread-count safety, node-counter batching and MT telemetry — no
   games; 1T bench-identical.** **(A.)** Pure enabler: 9.5 cannot be measured
   without (c), and (a)/(b) are defects found while reading the pool.
   - **(a) 🐛 The `Threads` cap is inverted.** `Parameters::max_threads()`
     (`parameters.cpp:190-195`) and `SearchThreadPool::normalize_thread_count()`
     (`search.cpp:2115-2120`) both compute `std::max(1024u, 4u * hw)` where
     `min` was clearly intended, so the UCI option advertises `max 1024` on
     every machine and `setoption name Threads value 1024` is accepted. Each
     `Searcher` owns ~2 MB of history tables (three `cont` tables at 512 KB
     each), so that is a ~2 GB allocation plus a 1024-thread creation storm on
     a 16-core box. Fix to a real cap, add a test that the advertised maximum
     tracks `hardware_concurrency`, and keep the existing graceful
     "Threads reduced to N" path for creation failure.
   - **(b) Batch the shared node counter.** `Searcher::record_node()`
     (`search.cpp:328-340`) does an atomic `fetch_add` on one shared cache line
     **at every node from every thread**, and `shared_nodes`/`shared_tbhits`
     are adjacent stack atomics (`search.cpp:2266-2267`) so they share a line.
     Accumulate thread-locally and flush every 1024 nodes (R batches at 128 and
     measured contention negligible *because* of the batching); pad the two
     counters apart. Node-limit semantics become granular to the batch — that
     is acceptable and matches every engine that does this. 1T path unchanged
     (it never touches `shared_nodes`), so bench is identical; measure 4T NPS
     before/after and record it.
   - **(c) MT-aware diagnostics.** `print_diag()` (`search.cpp:869`) is guarded
     by `info_cb_`, which only thread 0 has, so **at Threads>1 every diag
     counter we print describes the main thread alone** — sound, but blind to
     the pool. Add a pool aggregation pass (sum the helpers' `DiagCounters`
     after the join) plus three counters that 9.5 needs to be interpretable:
     main-thread TT probe/hit split, TT store same-key share, and per-thread
     completed depth. ⚠ Read them with R's corrections: divide aspiration
     re-search counts by thread count before comparing across T, and **do not
     use depth-at-fixed-time as a verdict** — its rep-to-rep spread is ±2
     iterations, the same size as the effect.
   - **Gate:** bench identical at 1T, CTest green, 4T NPS recorded, one
     Threads=4 diag run archived as the phase's MT baseline.

4. **9.4 SMP clock safety: helpers run to the stop + SMP-aware reserve —
   `[-3,0]` at 4T with a forfeit canary; 1T byte-identical.** **(A**, with R's
   measured fix as the prior.**)** Two defects, one gate, because both change
   only how long helpers search and share a single canary.
   - **(a) 🐛 Helpers stop searching while main is still extending.** Every
     thread runs the full time-management block (`search.cpp:2017-2036`) with
     its own `best_stability` / instability / score-drop scales, so a helper
     breaks its own iterative-deepening loop and then idles until the main
     thread finishes — exactly when main has *extended* on an unstable root and
     most needs the pool. Helpers must not own a clock: skip the soft-limit
     break for `thread_id > 0` and let `stop_` end them.
   - **(b) 🐛 Helpers inherit the depth limit.** `limits_for_thread()`
     (`search.cpp:2195-2208`) copies `limits` wholesale, so under `go depth N`
     every helper stops at `N` and idles instead of continuing to widen the
     shared TT. Clear `depth` for helpers; the main thread keeps the depth
     contract, so the reported result is unchanged. (R shipped exactly this as
     its "zero-risk" SMP fix.) Clock games are unaffected; bench is 1T so it
     cannot move.
   - **(c) SMP-aware time reserve.** We poll the clock every 2048 nodes
     (`search.cpp:1037`, `:1240`) and reserve a flat `2 × overhead`
     (`search.cpp:311`). That is the *identical* configuration in which R
     measured **10 time forfeits in 240 games at Threads=4**: under scheduler
     contention the 2048-node poll stretches from ≤1 ms to 50–100 ms and
     overruns the hard cap by up to 46 ms whenever the clock is low. Their fix
     — `min_reserve = 2·overhead + 30 ms` when `Threads > 1`, leaving Threads=1
     TM byte-identical — took it to 0/103. `timeBeginPeriod(1)` was measured
     **not** to be the fix. Adopt the same shape and verify the arithmetic
     against our own TM, not by transcription.
   - **Gate:** the 9.2 Threads=4 null doubles as the before-measurement; run
     `[-3,0]` at 4T on the bundle with **time forfeits as the hard canary**
     (any nonzero count fails the step regardless of Elo). 1T: byte-identical
     binary, no games. ⚠ (a) makes helpers search *longer*, which is a real
     behaviour change at MT — if the gate goes negative, decompose (a) first,
     since (b) and (c) have correctness arguments and (a) does not.

   > **🎉 9.4 ACCEPTED 2026-07-30 — `[-3,0]` @4T returned +30.42 ± 8.77 Elo
   > (nElo +47.97 ± 13.76), LOS 100%, LLR 2.95 → H1 @ 2,450 games, 2h08.
   > TIME FORFEITS: 0 — the hard canary is clean.** Ptnml
   > [33,230,525,364,73]. Gated as a *correctness* step expected to be neutral;
   > it is instead the first real MT strength in the phase.
   > - **The 95% CI [21.65, 39.19] Elo is ~3× the ±10 Elo placement class**, so
   >   the short-null caveat (bias bounded only at ~±28 nElo) does not threaten
   >   this reading. It would threaten a ±10 Elo reading; it does not threaten
   >   this one.
   > - **Mechanism-consistent with the pre-measurement:** main's share of pool
   >   nodes was drifting to 33% instead of 25%, i.e. roughly a quarter of the
   >   pool's throughput was being lost to idling helpers. Recovering that is
   >   worth about this much at 4T.
   > - ⚠ **Two qualifications on the number.** (1) It is an **SPRT-stopped
   >   estimate** — you stop when the LLR crosses, which correlates with a
   >   favourable stretch, so the magnitude is optimistically biased. Direction
   >   and significance are solid; the value is an upper-ish bound and only a
   >   fixed-N run would pin it (not worth 2 h — the decision is unambiguous).
   >   (2) It is a **BUNDLE number** (§3 lesson 11): (a)+(b)+(c) shipped
   >   together and the pre-registered decomposition was required only on a
   >   negative. **Never quote +30.42 as (a)'s value.** The mechanism evidence
   >   points at (a) as the likely carrier — inference, not measurement.
   > - 1T is byte-identical, so this is a pure-MT gain and needs no 1T games.
   >
   > **✅ 9.4 IMPLEMENTED 2026-07-30 (`2e1d632`).** All three
   > parts landed as one commit, one gate. **1T is byte-identical by
   > construction** — (a) gated on `thread_id_ == 0`, (b) unreachable at 1T
   > (`limits_for_thread` is only called on the multi-thread path), (c) gated
   > on `thread_count > 1`. Verified: bench **11,941,440**, CTest **12/12**.
   > - **(c) arithmetic checked, not transcribed:** our two poll sites fire
   >   every 2048 nodes (`search.cpp` qsearch + negamax), under a millisecond
   >   at 1T. The default `Move Overhead 10` makes the old reserve ~20 ms
   >   against a poll that stretches to 50–100 ms under contention, i.e. 30–80
   >   ms of exposure. `+30 ms` when `thread_count > 1` covers the common case.
   > - **Mechanism confirmed BEFORE spending the SPRT**, using 9.3's pool
   >   telemetry. `go wtime 20000` at Threads=4, five reps, main's share of pool
   >   nodes (25% = a fully-used 4-thread pool):
   >   `pre-9.4: 26.1 33.4 26.5 24.8 25.1` → `9.4: 24.9 24.9 24.9 25.2 25.1`.
   >   **The variance collapse is the signature** — main was picking up work the
   >   helpers had stopped doing. `go depth 16` @4T likewise rose 2.46M → 3.00M
   >   pool nodes for the same reported answer (that is (b)).
   > - Candidate `basilisk-phase94-clocksafety-pext-pgo` vs baseline
   >   `basilisk-phase93-base-pext-pgo` (both clang 22.1.8, both bench
   >   11,941,440, revisions `2e1d632` / `1ef2ab6`).
   > - ⚠ **Reading caveat from the short 9.2 null:** bias at 4T is bounded at
   >   ~±28 nElo, so this `[-3,0]` decides non-inferiority only if it lands
   >   clearly; the **forfeit count is the reading that is trustworthy
   >   regardless of bias**, and it is the canary that actually gates the step.

5. **9.5 SMP coordination wave — the value item. One `[0,3]` at Threads=4 plus
   an LTC confirm.** **(R mechanism, A gap analysis.)**
   **What we already have** (audited, do not re-implement): a shared atomic TT
   (`tt.h`), a pooled `RootMoveTable` consumed for root ordering
   (`search.cpp:116-125`, `:685-686`), staggered helper start depths
   (`search.cpp:1926-1927`), unthrottled helper TT writes — R re-measured
   throttling and found it costs main 12 points of TT hit rate at 4T and 27 at
   16T for 9–15% of nodes, i.e. *our* current behaviour is the measured-correct
   one — and per-thread `RootMoveStat` records with score/variance/effort/exact
   (`search.h:136-167`), which is the substrate this step publishes.
   **What is missing**, and is therefore this step:
   - **(a) Bound-typed publication of every searched root move.**
     `RootMoveTable::update()` (`search.cpp:85-100`) records only each thread's
     *iteration bestmove*; every other root move a thread proved something
     about is discarded. Publish each thread's `root_stats_` entries with their
     bound (Exact / Lower / Upper from the existing `exact` flag plus the
     window result), and have `ordering_score()` consume the bound — a proven
     fail-low demoted, an Exact preferred — instead of treating one move's
     depth as the whole pool's knowledge.
   - **(b) Pool-seeded aspiration windows.** Centre a thread's window on the
     pool's deepest Exact score when that is deeper than the thread's own
     progress. R's counters refuted the obvious objection: per-thread
     aspiration re-searches *fall* monotonically with thread count
     (21.5 → 17.9 from 1T to 16T), so pool seeding helps window placement
     rather than causing re-search storms.
   - **(c) Symmetric majority soft-stop.** Main's expiry becomes one vote; the
     pool can also **extend** main past its solo target, bounded by the
     existing `hard_limit_` and 9.4(c)'s reserve, with a strict majority
     (`votes·2 > thread_count`) and one latched vote per thread so a single
     helper cannot re-vote its way to a majority. ⚠ R's direct evidence on the
     direction: at Threads=2 requiring *both* votes beat requiring one by
     **+15.85 ± 6.12** — the pool was stopping too early, and under a real
     `maximum_ms` bound extra search time is worth more than fidelity to a
     heuristic estimate. Forfeit count is the canary.
   - **(d) Fix the result merge.** `merge_results()` (`search.cpp:2210-2242`)
     selects by depth-then-score, which our own Phase-12 spec says never to do,
     and ignores the effort/variance data `RootMoveStat` already carries. Use
     agreement- and effort-aware selection over the bound-typed pool from (a).
     Also tighten the mate branch, which can currently prefer a *shallower*
     mate result.
   - **Gate:** one `[0,3]` at Threads=4, Hash 256, ≥10k games, after the 9.2
     null. **Then an LTC confirm at `10+0.1` @4T**, because (c) touches time
     management and TM changes are TC-suspect by rule. 1T must be
     bench-identical (every mechanism gated on `thread_count > 1`); add a test
     that pins it, in the shape of R's `smp_machinery_is_inert_on_a_single_thread`.
   - **⚠ Expectation management, recorded before the run.** R's headline
     +102.78 @4T was a five-change bundle measured against its *original* SMP,
     and was never decomposed (lesson 11). Its one decomposed number is
     **+43.1 nElo for the coordination half alone**, and our baseline already
     contains two of those coordination pieces. So the honest prior for
     (a)–(d) is "the remainder", not the headline — a positive result in the
     +5…+25 range would be consistent with R's evidence; +100 would not.
   - **Pre-registered decomposition on H0**, so nothing is improvised after
     seeing data: drop **(c)** first (the TM piece, the only TC-suspect and
     forfeit-relevant one), re-gate (a)+(b)+(d); if that also H0s, drop **(b)**
     and gate (a)+(d) alone; (a)+(d) are the substrate the rest depends on and
     are kept only if they gate positive on their own.

   > **❌ 9.5 BUNDLE REJECTED (user-stopped) 2026-07-30.** `[0,3]` @4T vs the
   > 9.4 head: **Elo −41.45 ± 17.75** (nElo −63.53 ± 26.92), LOS 0.00%,
   > LLR −1.01 (34% toward H0), **640 games**, PairsRatio 0.55, Ptnml
   > [25,101,125,63,6], **zero forfeits**. Stopped before H0 to free the box;
   > the CI [−59.2, −23.7] excludes zero by >4σ, so unlike the 360-game read
   > this is a real negative, not a trend. Recorded as *rejected on a stopped
   > run*, not as a formal H0.
   >
   > **▶ Pre-registered decomposition executed: (c) dropped**, ballot plumbing
   > removed with it, `(a)+(b)+(d)` regated vs the same 9.4 baseline
   > (`basilisk-phase95abd-nostop`). ⚠ The −41.45 is a BUNDLE number and
   > **(c) is a suspect, not a convicted party** — (b) is a live alternative
   > (pool-seeded windows could centre a thread on a deeper-but-different
   > score and provoke re-searches). The regate discriminates: positive ⇒ (c)
   > was the problem; still negative ⇒ it was (b) and the suspicion was wrong.
   >
   > **⭐ IF `(a)+(b)+(d)` IS ACCEPTED, (c) GETS ONE REPAIR ATTEMPT** — gated
   > against the NEW accepted head, not against 9.4, and as its own single
   > candidate (gate 1). Suspected mechanism, reasoned but unmeasured: main
   > waited for a strict majority (itself + 2 helpers at 4T) while helpers
   > evaluate their own stability/instability state at staggered start depths
   > with no depth limit since 9.4(b); if they arrive late, main overspends on
   > every move. Zero forfeits is consistent with that — 9.4(c)'s reserve
   > prevents the flag fall, chronic overspend still loses games later.
   > Re-designs, in preference order:
   > 1. **Bound the extension** — break on majority *or* at
   >    `own_expiry × 1.25`, whichever comes first. Keeps the pool's say,
   >    makes the worst case a 25% overspend instead of unbounded-to-hard-limit.
   >    One line; addresses the suspected mechanism rather than its label.
   > 2. **Equalise the vote condition** — helpers vote on bare
   >    `elapsed >= soft_limit_`, without their per-thread scales, so votes
   >    arrive together and the majority forms near main's own expiry.
   > 3. **Lower the threshold rather than raise it.**
   > **⚠ Consequence for 9.10, which is now probably backwards.** Rarog's
   > supporting evidence was at **2 threads**, where "both votes" is
   > *unanimity*; 3-of-4 is a different point on that curve and PLAN already
   > warned the 2T result need not extrapolate. This run is evidence that
   > *waiting longer* is what hurt, so 9.10 — queued to test raising 3-of-4 to
   > 4-of-4 — should be **re-scoped to test lowering it, or closed**. Do not
   > run 9.10 as written.
   >
   > **CLOSED 2026-07-31 — no 9.5 candidate is banked.** The isolated `b`
   > pool-window gate washed: last report **+1.26 ± 5.28 Elo**, nElo
   > **+2.05 ± 8.57**, LLR **+0.09** at 6,320 games, zero forfeits; it was
   > removed as pure unproven surface area. The first `a` gate was invalid:
   > its root selector could choose a deeper Upper-bound (refuted) move and
   > lost every opening before the defect was fixed. Corrected sparse `a` was
   > user-stopped at 2,450 games on a sustained negative trend: last report
   > **−6.98 ± 8.51 Elo**, nElo **−11.31 ± 13.79**, LLR **−0.78**, zero
   > forfeits. Neither stopped gate is a formal H0, but neither is accepted.
   > `c` remains excluded: its repair was explicitly conditional on the
   > `(a)+(b)+(d)` gate accepting. The Phase-9 head is restored to **9.4**.

6. **9.6 Node-invariant index hoists II — bench-identical, NPS-gated, pays at
   1T and 4T.** **(A**, from R's class verdict.**)** Phase 8.7 hoisted the
   continuation-history rows out of the per-quiet-move loop for **+3.03%**;
   R took the same lead and got **+3.76%**, then swept for more and found the
   payoff scales with loop frequency — per-move and per-node loops pay, per-piece
   loops do not. Our per-quiet-move loop still re-derives three node-invariant
   row bases on **every** move (`search.cpp:648-680`):
   `pawn_hist_score()` re-masks `pawn_key & (PAWN_HIST_SIZE-1)` (`:472-474`);
   `hist_.main[b.side_to_move]` re-loads the side and re-indexes the colour
   row; `low_ply_score()` re-tests `ply < LOW_PLY_HISTORY_SIZE` and re-indexes
   the ply row (`:476-478`). The same three appear in the negamax move loop's
   history-prune and `stat_score` paths (`search.cpp:1543-1544`, `:1639-1640`).
   Resolve all three once per node beside the existing `check_info`/cont-row
   hoist, leaving only the per-move `piece/to` index varying.
   ⚠ **Do not assume the compiler already does this** — R predicted its
   compiler would hoist the identical pattern (the loads look hoistable) and
   was **wrong**; the measured gain was its single biggest speed win of the
   wave. Unit-test the decomposition (`row_base + piece_to_index == full
   index`). **Gate:** bench identical (11,941,440) + `nps_ab.ps1` pooled PGO,
   ≥2 builds per arm, self-pair validated first; accepted on NPS evidence per
   the 8.7 precedent (bench-identical ⇒ no search risk). At ≈2 Elo per 1% NPS
   this is the cheapest Elo in the phase.

   > **✅ ACCEPTED 2026-07-31: +1.29% pooled-PGO NPS**, 95% CI
   > **[−0.25%, +2.64%]**, best-of **+0.50%**, 9/16 rounds, identical
   > 11,941,440 fingerprints. The original
   > implementation was not source-isolated from the stopped 9.5(c) clock
   > experiment and omitted this item's focused decomposition test. Fixed by
   > committing a clean post-9.5 / exact-9.4 baseline (`206cd60`), then applying
   > only the three row hoists plus the required full-index/address-equivalence
   > test (`28dead9`). Bench is **11,941,440** and CTest is **12/12**. Two clean,
   > independent PGO builds exist per arm, all with distinct SHA-256 values.
   > The first pooled self-pair failed **−3.77%, CI [−10.91%, +2.84%]** with an
   > extreme 3.03–3.64M NPS range because an unrelated concurrency-14 Rarog
   > tuner was active across every physical core, including pinned CPU 30.
   > Per the idle-box rule this run is **INVALID and says nothing about 9.6**.
   > After the tuner stopped, the valid A/B used two independent PGO builds per
   > arm pinned to CPU 30. Per user direction the already-validated harness did
   > not need another self-pair. The CI narrowly crosses zero and the build
   > pairs disagree, so do not overstate precision; retention follows the 8.7
   > strict-work-reduction precedent (+0.36% was accepted with CI [−0.06%,
   > +0.67%]). This is a measured positive for a behavior-neutral simplification,
   > not a direct Elo result.

7. **9.7 Helper history blending: measure or delete.** **(A + R.)**
   `SearchThreadPool::search()` calls `blend_history_from()` for every helper
   at the end of every MT search (`search.cpp:2304-2305` →
   `history.cpp:69-105`), blending main + capture + **all three continuation
   tables** + pawn history — on the order of 800k `int16` blend operations per
   helper per search, i.e. ~1.6 MB touched per helper. **It has never been
   measured, in either direction.** R implemented the *cheap* version of this
   idea (ordering tables only, 21 KB) precisely because we had it, gated it at
   Threads=4, and measured **−0.52 ± 7.45 over 3,320 games** — a clean null —
   then wrote the explicit warning: *do not escalate to `cont_history` /
   `pawn_history`*, because helpers search the same root through the same TT so
   their tables are largely redundant, and the expensive version inherits the
   same prior at many times the cost. We were running the expensive version.
   - **Do:** put the blend behind a temporary UCI switch so both arms come from
     **one binary** (removing the ~0.36% per-build PGO offset), measure 4T NPS
     with it on and off, and run one `[-5,0]` simplify-shaped gate at 4T for
     the removal.
   - **Expected outcome:** deletion — free NPS at MT and ~35 lines gone. If it
     unexpectedly gates positive, keep it and delete the switch (temporary
     scaffolding must never ship, either way).

   > **✅ CLOSED 2026-07-31: BLENDING REMOVED (user-stopped simplify decision,
   > not a formal H1).** Added the temporary TUNE-only
   > `HelperHistoryBlend` check option (default `true`, so release behavior is
   > unchanged) and carried it through `Parameters` → `SearchLimits` → the
   > post-join pool merge. `bench` receives the same option, preventing the
   > silent split where SPRT would test the switch but NPS would ignore it.
   > `nps_ab.ps1` now accepts per-arm `Name=Value` options and an explicit
   > thread count; at Threads=4 it pins the process to four physical cores
   > rather than forcing four workers onto the old single-core affinity mask.
   > Exact 1T bench **11,941,440**; CTest **12/12**; TUNE-on focused tests pass.
   > Same-binary 4T NPS with removal (`false`) as A and current behavior (`true`)
   > as B measured **+0.48%**, 95% CI **[−1.66%, +3.45%]**, best-of **+0.97%**,
   > 9/16 rounds. This is directionally favorable but unresolved. Lazy-SMP node
   > totals differed 45,122,343 vs 36,193,107, so the reading includes tree and
   > scheduling effects and is not a pure merge-cost measurement. The 4T
   > simplify SPRT was user-stopped after its early favorable drift settled
   > into a wash: last complete report at 1,400 games was **+0.99 ±11.70 Elo**,
   > nElo **+1.54 ±18.20**, LLR **+0.23**, Ptnml
   > **[30,170,293,180,27]**, zero forfeits; another 34 games finished without
   > a report block. It was projected to need roughly 18k games at the current
   > estimate or ~28k if truly neutral. Removal was accepted by explicit user
   > decision on the combined evidence: no adverse strength trend, positive NPS
   > direction, Rarog's cheap analogue neutral, and substantial unconditional
   > memory traffic/code with no demonstrated value. The merge implementation
   > and all temporary switch plumbing are deleted; `nps_ab.ps1` keeps its
   > reusable same-binary option support and corrected Threads>1 affinity.

8. **9.8 Pruning-input provenance: `EvalPruneTtMinDepth` — inert knob now, one
   `[0,3]` at 1T, then it joins 11.7's group.** **(A + R.)** Our pruning eval
   (`search.cpp:1356-1362`) lets a TT score stand in for the corrected static
   eval whenever its bound proves it tighter — with **no minimum entry depth**.
   Qsearch stores at depth 0 (`search.cpp:1108`, `:1215`), so a qsearch entry
   can decide a reverse-futility cut at depth 8, which is hard to defend on
   principle. Add a neutral-seeded knob (0 = today, bench-identical) requiring
   a minimum TT depth before the substitution; the principled value is **1**,
   which excludes exactly the qsearch entries.
   ⚠ **R's warning, which is the whole reason this is a `[0,3]` and not a
   sweep:** larger values reduce node counts sharply, and **fewer nodes is not
   evidence of strength** — it is more aggressive pruning, the failure mode
   that made a self-play-tuned candidate win its SPSA and then lose −7.78 to
   the more accurate baseline. Gate value 1 only; larger values belong to
   11.7's pruning group, never a one-off A/B. R's related finding, recorded so
   we do not re-derive it: the qsearch **stand-pat** TT refinement
   (`search.cpp:1101-1104`) is the compounding path — a lower stored Upper
   bound lowers the stand pat, suppressing the primary qsearch exit — so if
   this step is interesting, the follow-up is gating *that* refinement on bound
   provenance, not widening this knob.

   > **❌ CLOSED / REJECTED 2026-07-31.** The TUNE-only 0-vs-1 gate was
   > user-stopped after the signal settled into a wash. Last complete report:
   > **4,120 games, −2.02 ±6.76 Elo, −3.18 ±10.61 nElo, LLR −0.48**,
   > Ptnml **[85,508,898,484,85]**, zero anomalies; 38 more games completed
   > without another report. Across the run LLR stayed between +0.27 and
   > −0.48. Formal resolution was projected to take many more hours, while the
   > candidate showed no strength evidence. The temporary option, main-search
   > guard and tests are removed; qsearch stand-pat was never changed. Larger
   > minimum-depth experiments remain deferred to 11.7's joint pruning group.

9. **9.9 Diversification retries — LOW PRIOR, one bundled 4T gate, revert on a
   wash.** **(R nulls; kept because our engine is not theirs — lesson 13.)**
   R shipped root-move rotation, per-thread LMR jitter and whole-tree quiet-
   ordering jitter, then measured the jitter pair at **−0.81 ± 2.55 over 28,362
   games** (a textbook null: pentanomial 559/538 and 3438/3414) and left
   rotation unresolved at −3.31 ± 10.62. Our diversification is different —
   staggered start depths only, and after 9.5 our threads will be *more*
   correlated than R's were (pool-seeded windows pull them together), so the
   question is genuinely re-opened rather than answered.
   - **Implement all three behind one switch** and gate the bundle once at 4T:
     (a) per-thread LMR reduction jitter from a **proper PRNG** — R's first
     attempt was `(nodes + id·27) % 128 − 59`, measured at lag-1
     autocorrelation **0.95** with a **+4.48/1024 mean reduction bias present
     only at Threads>1** (a pruning change wearing a diversification label);
     use a per-thread xorshift64 re-seeded from `thread_id`, taking the **top**
     bits (the low bits are xorshift's weakest and skew the mean);
     (b) whole-tree quiet-**ordering** jitter as a hash of (thread, move), not
     a stream — a PRNG would make a thread disagree with itself between the
     null-window and full-window passes of the same node — sized below the
     tier gaps so it can only reorder near-tied quiets, and hoisting the
     `thread_count > 1` test **out** of the per-move loop (R had to do this
     after leaving it in);
     (c) root-move rotation for helpers.
   - **Pre-registered:** wash ⇒ revert all three and delete the switch. None of
     them has a correctness argument, so "keep it anyway" does not apply.
     ⚠ Do not fold this into 9.5 — a null here must not be able to mask 9.5.

   > **❌ CLOSED 2026-07-31; user-stopped neutral, bundle removed.** This was
   > not a formal H0. Last complete report at 640 games: **+0.54 ±17.20 Elo**,
   > **+0.85 ±26.92 nElo**, **LLR −0.01**, Ptnml **[15,71,146,74,14]**, zero
   > anomalies; three more games completed. There was no useful trend, and the
   > experiment's special premise had disappeared: 9.5's pool-seeded windows
   > were rejected, so our threads are not made more correlated than Rarog's
   > robust null (**−0.81 ±2.55 over 28,362**). Per the pre-registration, all
   > three mechanisms and the temporary switch/tests were removed.

10. **9.10 Stop-threshold sweep — needs 9.5(c); one line, one 4T gate.**
    **(R lead, untested anywhere.)** R's 2T measurement says a pool that waits
    for *unanimity* beats one that stops on a bare majority by ~16 Elo, i.e.
    the vote fires too early. The cheapest well-motivated 4T experiment on the
    books is therefore raising the threshold from 3-of-4 to **4-of-4** at
    Threads=4: no tuning, one binary, one `[0,3]`. The prior is genuinely
    uncertain — 3-of-4 is already above-median, so the 2T result need not
    extrapolate — which is exactly why it is a cheap test and not a
    pre-decided change. **Do not bundle it with anything**, and re-check
    forfeits, since it can only make the pool search longer.

    > **CLOSED / NOT APPLICABLE 2026-07-31; no code, no games.** 9.5(c) was
    > never accepted, so there is no 3-of-4 threshold in the accepted head to
    > sweep. The bundle containing it was already strongly negative
    > (**−41.45 ±17.75 Elo at 640 games**). Rarog's Threads=2 evidence is not
    > a 4T extrapolation: 2-of-2 is unanimity, while 3-of-4 already waits past
    > a bare majority. Restoring rejected ballot plumbing solely to make the
    > pool wait still longer would be a new riskier feature, not this one-line
    > follow-up, so 9.10 closes with its dependency.

11. **9.11 ❌ CLOSED: USER-STOPPED WASH — one final HCE Texel re-fit on post-1.9.1 labels.**
    **(User request 2026-07-28, after a Colosseum Grand Bullet arena appeared
    to show 1.9.1 ~5–10 Elo below 1.9.0 against external opposition.)**

    **First, the premise — the arena does not show a regression.** Read at
    9,857/36,400 games, 3+0.03, 1T, 15 parallel games:
    - **Direct head-to-head, the only paired measurement in the table:
      1.9.1 beats 1.9.0 27–57–24** (108 games, ≈ +10 Elo ± 40). It points the
      *other* way, and §3 lesson 8 is explicit that the within-gauntlet
      head-to-head is what we trust, not the pool rating.
    - The pool-score gap is 763.0 vs 774.0 of 1,398 games = **54.58% vs
      55.36%, i.e. 0.79% ≈ 5.5 Elo**, with a per-engine standard error near
      1.3% — so the difference sits comfortably inside one SE, before even
      accounting for the shared-opponent correlation.
    - Per-opponent it is scatter, not a trend: 1.9.1 is **better** vs SF
      dev-3000 (+6.0 pts) and Rarog 2.2.0 (+4.5), **worse** vs SF dev-2800
      (−6.5) and Rybka 3 (−6.5), level vs the other eight.
    - **Mechanism says it cannot be real:** 1.9.1's search is *bit-identical*
      to 1.9.0 (bench 11,941,440 both) — the only delta is NPS, measured
      +4.34%, and the arena's own `Avg depth` column corroborates it
      (12.8 vs 12.7 at the same time/move). Our dedicated batch SPRT read
      **+8.69 ± 6.63 over 3,764 games**, which is a far tighter instrument
      than 108 head-to-head games inside a 15-way loaded arena (§3 lesson 10:
      a loaded box is exactly where placement bias lives).
    **Conclusion: nothing to recover — 1.9.1 ≥ 1.9.0.** This step therefore
    is not a repair; it is a separate, optional question about whether the
    *evaluator* has residual headroom.

    **The honest case for it.** Our own record says the on-policy self-play
    re-fit was the biggest post-campaign lever (+15…+21 per cycle) until
    **cycle 6 washed (+1.37 ± 5.21, 8.1k games)** and closed the HCE line
    (§3 lessons 1–2). The one thing that has changed since is real: cycle 6's
    labels were generated by the pre-1.9.0 head, and the label generator has
    since gained **≈ +52 Elo** (Phase 8/8.5 + `hcefinal`). Better search ⇒
    game outcomes correlate better with true eval, so the same fit on
    better-labelled data is a genuinely different experiment. Rarog filed the
    identical item (its 10.4.3) for the identical reason.

    **The honest case against, which is why this is OPTIONAL and last.**
    1. **The cliff.** Three measurements across two engines say a well-fit HCE
       has no *unchanged-representation* headroom: our cycle 6 wash, Rarog's
       off-policy −17.11 and its on-policy −1.28. Better labels do not add
       representational capacity.
    2. **It de-tunes the search.** `hcefinal` (+35.94) was fitted *after* the
       eval froze at cycle 5, so every cp-denominated margin is calibrated to
       the current weights. A re-fit is an eval rescale in the sense that
       matters, which makes a standalone gate a **lesson-15 de-tuning victim**
       — rigged low. The re-tune that would recover it is 11.7, and 11.7
       targets the *net*, not the HCE. **Pre-registered: we do NOT chase a
       wash here with a pre-NNUE search re-tune.**
    3. **Phase 10 deletes the evaluator.** Spending days on the thing we are
       about to replace is what the §1 cost principle exists to prevent.

    **What makes it cheap enough to be worth offering anyway: the datagen is
    dual-use.** `datagen.ps1` already writes a fastchess PGN with per-move
    score/depth comments, and that same corpus feeds **both**
    `tools/texel/extract_parallel.py` (Texel CSV) **and** `net_trainer`'s
    `extract_nnue.py` (Phase 10.2 training data). So generate the corpus once
    at the released head: if the fit washes, the games are not wasted — they
    become an early slice of the NNUE dataset that 10.2 needs regardless.
    Size it to 10.2's requirements, not to the Texel fit's.

    **⚙ Datagen configuration — 1T, and the reasoning matters:**
    - **Threads=1, `tc=inf nodes=N`** (what `datagen.ps1` already does).
      Labels are a function of *search quality per node*, and at a fixed node
      budget Lazy SMP is strictly worse than one thread — the pool duplicates
      work, so 4T at N nodes reaches a shallower, noisier result than 1T at N
      nodes. 4T would also make the corpus non-deterministic and therefore
      non-reproducible from its manifest.
    - **Throughput agrees.** Datagen is embarrassingly parallel at the *game*
      level; 15 concurrent 1T games saturate the box far better than 4
      concurrent 4T games, because SMP efficiency is below 1 and the fixed-node
      contract throws away the extra nodes anyway.
    - **The quality axis is nodes-per-move, not threads.** If we want better
      labels for the same wall clock, raise `-Nodes` and cut `-Rounds`; do not
      add threads. (Gate 10's 1T-and-4T rule is about *playing* conditions —
      it has no bearing on how labels are produced.)
    - **Always the diverse book** (`tools/texel/data/beast_seed.epd`) — §3
      lesson 4: a tiny book collapsed 200k games to 31,880 unique positions.
    - **Timing is free-floating.** Nothing in Phase 9 changes 1T fixed-node
      search — 9.5/9.9/9.10 are MT-only and 9.6 is pure speed, which a
      node-limited search cannot observe — so the label generator is *already*
      in its final pre-NNUE state and datagen can run in any earlier gate wait
      rather than waiting for this slot.
    **Protocol: ONE dataset, ONE fit, ONE gate.** Sequential joint bake
    (`--tune all` → bake → rebuild → `--tune-kingsafety` → bake, §3 lesson 5),
    `--verify` reconstruction, then a single `[0,3]` at 1T vs the phase head.
    **Pre-registered outcome:** if it washes, the HCE evaluator is closed
    **permanently** — no second cycle, no margin re-tune, no "one more
    dataset" — and we go to the NNUE runway with the corpus already in hand.

    > **DATA POLICY CORRECTED + EXACT DATASET COMPLETE 2026-08-01.** The first
    > 200,000-game corpus produced 3,508,199 unique positions and 3,334,121
    > natural train rows, but the 1:1.5:1.5 phase cap retained only **1,237,744**
    > (309,436 / 464,154 / 464,154). This exposed a registration arithmetic
    > error: the run was sized for 3.5M *before* lossy balance, despite the
    > extractor's 1.5M warning floor, the README's ≈3M target, and v17's actual
    > 3,351,075 train rows. The resulting fit and `805abb2` candidate were
    > declared void before arena games and reverted by `dfebea7`.
    >
    > A second audit corrected the correction rather than preserving legacy
    > policy. Rarog's current extractor and Hydra both use five equal material-
    > phase reservoirs; Rarog also samples inside each phase per game and applies
    > a winning-capture quiet filter. Basilisk now adopts those stronger
    > contracts: **700,000 each** of opening (20–24), early-mid (14–19), middle
    > (8–13), endgame (3–7), and deep endgame (0–2), plus an exact balanced
    > game-level holdout, global FEN dedup, bounded per-phase-per-game sampling,
    > quiet filtering, and atomic publication. There was no empirical basis for
    > the inherited 1:1.5:1.5 compromise, so it is retired before the valid fit.
    >
    > The 16-ply start skip was also wrong for this corpus: Basilisk's own
    > original Beast-EPD recipe specified `--skip-start 0`, because each supplied
    > FEN is itself a candidate position, not a played conventional book line.
    > The user stopped the registered append at **424,071 complete games**
    > (200,000 original + 224,071 disjoint extension starts). A header audit found
    > **424,071 exact unique starts, zero duplicates**; pawn-family concentration
    > is low (largest opening family 30 games, top 100 only 0.74%), though the
    > source files do not retain ICCF/MEGA game IDs and therefore cannot prove
    > source-game independence. No more games are generated unless the exact
    > five-reservoir extraction reports a real short bucket. This PGN
    > remains an initial NNUE slice; 10.2 extends it to 30–60M. The one-fit,
    > sequential bake/rebuild/KS-bake/rebuild/verify protocol is unchanged.
    > **Measured extraction:** 424,071 games, zero parse errors, 6,262,111 raw
    > candidates, 1,067,049 rejected by the quiet filter, and 6,202,261 unique
    > positions. The exact train set is **3,500,000 = 700,000 × 5** plus 184,211
    > balanced holdout rows. Eligible train streams were 875,444 / 1,196,846 /
    > 1,467,791 / 1,604,024 / 745,848; deep endgame was limiting but still had
    > 6.5% headroom. Therefore the stopped corpus is sufficient and datagen is
    > closed.
    >
    > **Registered fit complete 2026-08-01.** The joint fit used all 3.5M train
    > rows and 184,211 holdout rows, with 1,165 active parameters and
    > `L2=1e-6`. It restored the best holdout checkpoint at epoch 28:
    > **0.105861 → 0.105147**. The sequential king-safety refit then improved
    > its 100k-position holdout **0.105322 → 0.105126**. Exact score
    > reconstruction passed on 10k train and 10k holdout positions; all 12 C++
    > tests and all 13 Texel Python tests pass. The baked candidate changes 115
    > of 1,165 rounded joint parameters plus the registered king-safety pass;
    > bench fingerprint is **15,312,647**. The tuner now reports initial, tuned
    > and delta holdout loss with the same five phase boundaries as extraction;
    > the king-safety optimizer restores its best holdout checkpoint (pass 27,
    > rather than overfitting through pass 39); and the bake tool no longer
    > accumulates PST indentation. These are tooling corrections discovered
    > before games; the gated engine candidate remains one learned weight set.
    > **SPRT artifact ready:** clean PGO build
    > `basilisk-phase911-texel5-pext-pgo.exe` at revision `15ddd26`, bench
    > **15,312,647**, SHA-256 `A917BCDE687CF77A6C2F75D23D06F081EB1973170F8FEE965A9C09F373ABF765`.
    > Against the registered pre-fit `phase911-datagen` baseline, the complete
    > engine-source diff is exactly `src/eval_params.h`. The earlier PGO build
    > without king-safety best-checkpoint restoration is preserved only as
    > `VOID-no-best-checkpoint` and must not be tested.
    >
    > **VERDICT 2026-08-01: USER-STOPPED WASH; fitted weights rejected and
    > restored.** The valid 1T `[0,3]` SPRT was stopped at **6,398 games**:
    > **+1.52 ± 5.77 Elo, +2.24 ± 8.51 nElo**, W/L/D
    > **1,791/1,763/2,844**, LLR **+0.12 / +2.94**. Its trajectory sat in the
    > indifference region (+1.38 nElo at 3,004; +0.65 at 3,998; +1.03 at
    > 5,000), so more games risked a long midpoint stall and there was no
    > evidence for the registered +3 target. Two time losses occurred, one by
    > each engine; they add no directional explanation but make the run less
    > than perfectly clean. Per the pre-registration, there is **no second HCE
    > fit and no pre-NNUE search SPSA rescue**. The old Phase-9 eval weights are
    > restored (bench **11,941,440**). The five-phase extractor, fitter fixes,
    > manifests and 424,071-game dual-use corpus remain accepted infrastructure
    > and an initial Phase-10 NNUE data slice. The tested PGO artifact is renamed
    > `REJECTED-wash`; the HCE refit line is closed.

12. **9.12 ✅ CLOSED — Phase close-out and 1.9.2 release gate (2026-08-01).**
    - **Correctness/build:** release metadata reports 1.9.2; bench remains
      **11,941,440**; CTest **12/12** and Texel/Python **13/13** pass.
    - **Primary MT evidence:** 9.4's `[-3,0]` fastchess gate passed at 2,450
      games, **+30.42 ±8.77 Elo @4T**, with zero forfeits under the tight 20 ms
      margin. It is an optimistic SPRT-stopped estimate and a bundle value.
    - **Colosseum 1.0.1 boundary campaign:** 3,600 games at 1T `3+0.03`, 1,800
      at 4T `3+0.03`, and 400 at 4T `10+0.1`; UHO paired openings, ponder/TB
      off. Direct 1.9.2-vs-1.9.1 results were respectively **107-199-94
      (+11 ±24)**, **55-98-47 (+14 ±34)**, and **59-97-44 (+26 ±35)**.
      Every condition pointed positive. All 5,800 games completed without a
      reported Basilisk forfeit, crash or illegal move. Colosseum's generous
      timeout grace does not replace the tight fastchess clock evidence above.
    - **Field sanity:** against Rarog 2.3.0, 1.9.2 scored +66 ±29 at 1T fast,
      +10 ±40 at 4T fast and +28 ±40 at 4T `10+0.1`; the wider field contained
      Houdini 1.5a, Critter 1.6a, Rybka 3/4, SF-dev-2800, HIARCS 14 and Fruit
      2.1. Absolute field ratings remain secondary to the direct prior-release
      head-to-head.
    - **Explicit close-out waiver (user decision):** do not spend more machine
      time on the standalone end-to-end 1T NPS rerun, the 4T diagnostic/scaling
      rerun, a 1T `10+0.1` duplicate, or `60+0.6`. The relevant evidence already
      exists: 1T behavior is fingerprint-identical, 9.6 has pooled-PGO NPS,
      9.3/9.4 recorded MT utilization/scaling, and the deployment games above
      show no reversal. These items are waived, not falsely recorded as run.
    - **Version decision:** the original 1.10.0 trigger was met by 9.4, but the
      user explicitly chose **1.9.2** because the final accepted production
      scope is focused. Version 1.9.2 is the frozen HCE/NNUE baseline; 1.9.3
      changes only PGO tool selection and does not replace that engine baseline.

---

## 6. Phase 10 — NNUE baseline and 2.0.0 (`nnue` after the one Phase-8.5 rebase)

The strategic direction is NNUE. The +200–400 target is a hypothesis, not a
schedule commitment. The actual training implementation already exists in
**`D:/code/net_trainer`** (re-audited at `59d190e` on 2026-07-14): a Rust
trainer on pinned Bullet with CUDA training, BulletFormat packing, seeded
shuffle, a blended search-score/WDL target, cosine LR, and raw Bullet
`quantised.bin` export. The v1 network is
`chess768 -> (H×2 perspectives) -> 1×8 material buckets` with SCReLU,
H=1024 by default, QA=255, QB=64 and SCALE=400. NumPy, C++17 and Rust integer
references plus a committed H32 conformance net/vectors define exact engine
behavior. Phase 10 hardens and consumes this implementation. **Do not restore
the retired PyTorch/`MNN1` pipeline or invent a parallel trainer/format.**

Repository ownership:

| Work | Repository |
|---|---|
| Data generation/extraction, splits, BulletFormat conversion/shuffle, Rust/Bullet trainer, checkpoints and reference verification | `D:/code/net_trainer` |
| Board state, accumulator, raw `quantised.bin` loader/embedding, SIMD inference, UCI and search integration | `D:/code/basilisk` on `nnue` |
| Teacher annotation, if used | Existing Hydra tooling, imported through a versioned `net_trainer` dataset path |

The HCE comparison baseline is the **released 1.9.2 head** (the post-Phase-9
`development` handoff SHA), not 1.8.0/1.9.0/1.9.1. Every network is identified by
`quantised.bin` SHA-256 plus its
architecture constants, hidden size and training manifest. HCE remains
available as debug/full-recompute comparison and a
temporary UCI fallback during bring-up; the release default is the accepted
embedded net.

1. **10.0 Rebase and conformance inventory:** record the post-1.9.2 handoff
   SHA, rebase `nnue` onto it once, resolve the existing partial NNUE implementation,
   and map every retained component to `docs/nnue_format.md` at the recorded
   `net_trainer` SHA. No search/eval behavior change in this step. The v1 file
   deliberately has no magic/header: validate total size, 64-byte `bullet`
   padding, inferred/expected H, tensor dimensions and SHA before use. Future
   layouts receive a separately documented contract and conformance artifact;
   do not pretend a nonexistent `MNN1/MNN2` dispatch field is present.
2. **10.1 Harden `net_trainer` for real experiments:** add train/validation/
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
3. **10.2 Data at scale with controlled provenance:** use
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
   - **Candidate: select positions by BEHAVIOURAL disagreement, not eval
     error** (added 2026-07-30, external report + our own lesson 3). The same
     r/chessprogramming post measured two mining strategies on identical
     infrastructure: selecting training positions by **centipawn loss yielded
     0.5%**, selecting by **recall failure** — positions where the teacher's
     best move falls outside the model's top-K — **yielded ~30%**, and two
     fine-tuning rounds moved recall@4 from 68.6% to 78.1%. A ~60× difference
     in what the selection criterion was worth.
     **This is §3 lesson 3 arriving from an independent direction:** we
     measured that holdout-MSE delta does not predict Elo in either direction
     (the distillation with the biggest MSE drop gained least; cycles with
     near-zero linear movement gained +15–20). They measured that eval-error
     selection is near-worthless while behaviour-level disagreement is not.
     Same shape, different mechanism, so it is corroboration rather than a new
     claim — and it says our current selection (diversity via
     `beast_seed.epd`, and nothing else) is leaving the lever untouched.
     ⚠ **Define the analogue before spending machine time:** theirs is a
     *policy* net where "recall failure" is native; ours is a *value* net, so
     the transfer is "positions where the net's eval leads search to a
     different move than the teacher", NOT "positions with large eval error"
     — the latter is precisely the criterion that measured 0.5% for them.
     A/B it as one of 10.2's independent label/selection axes.
4. **10.3 Baseline training:** train the existing
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
5. **10.4 Scalar engine integration first:** validate the raw Bullet loader and
   a full-recompute scalar evaluator against `net_trainer`'s committed H32 net
   and vectors—covering both STM colors and all eight output buckets—and a
   large generated FEN corpus with exact integer equality. Validate inferred H,
   payload order and padding. Keep `Evaluator::evaluate(const Board&)` as the
   search boundary. The release net is embedded; optional `EvalFile` is
   allowed for development/custom nets but must validate raw size/H/padding
   fully and report the active net SHA.
6. **10.5 Incremental and optimized inference:** consume 8.5.3 dirty-piece
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
   **Rider — toolchain stabilization (do once, immediately before that PGO
   regen):** pin ONE LLVM major across all release/CI platforms so the compiler
   stops being an uncontrolled variable under hand-written SIMD: Linux = apt
   `clang-NN` (already pinned at 19 by the 1.9.1 CI fix — clang-18's
   `__cpp_concepts 201907L` disables libstdc++'s `<expected>`, so never float
   back to distro default), Windows = an exact-tagged **llvm-mingw** release
   (replaces rolling MSYS2 CLANG64/CLANGARM64; same clang+libc++/UCRT ABI, one
   toolchain for x64+aarch64), macOS = Homebrew `llvm@NN` through the existing
   `COMP=llvm` path (drops floating AppleClang). Hold the major in a single
   `LLVM_MAJOR`-style variable per workflow, stamp the toolchain into release
   metadata, and accept the pin/bump via CI green + the 3-way (ci.yml) and
   9-way (release.yml) bench-agreement jobs — no other gate needed.
7. **10.6 Swap-in and iterate:** compare NNUE against the released **1.9.2** HCE baseline
   at the standard SPRT, `10+0.1`, the frozen teacher cohorts and tactical/
   endgame suites. Iterate data amount, label blend, H=512/1024, WDL weight,
   learning rate and training duration one variable at a time until the net
   passes both game gates and
   has acceptable NPS. A failed baseline triggers contract/data/trainer/
   architecture diagnosis—not a claim that HCE is the frontier path.
8. **10.7 Provisional search-scale safety pass, not the final tune:** inspect
   score distribution, correction magnitude, pruning telemetry and tactical
   regressions. Change only margins required to prevent gross NNUE-scale
   miscalibration, as isolated SPRTs. **Do not run the comprehensive histshape/
   wave2/TM SPSA here:** Phase 11 still changes the search architecture and
   would invalidate it.
9. **10.8 Release 2.0.0:** full §8 gate against the released HCE head and field
   slate, exact embedded-net SHA plus H/bucket/constants/trainer/Bullet revision
   in release metadata, production PGO/ISA assets smoke-tested, zero
   incremental/full-recompute mismatches, and STC + LTC strength stated
   separately. Phase 11 produces the final search architecture and tune for
   the next 2.x strength release.

Model: **Fable 5 high (alt: Opus 4.8 high)** for StateInfo/accumulator/SIMD and
trainer-core changes; Sonnet 5 medium for orchestration, data runs and docs.

---

## 7. Post-NNUE roadmap — execution order is Phase 11 → 12 → 13

### Phase 11 — final single-thread search architecture and tune

Phase 10.7 performs only emergency NNUE-scale safety calibration. Phase 11
builds the final search architecture **first**, then runs the comprehensive
tune **once at the end**. Search telemetry from 8.5.4 is an acceptance input
for every candidate; total nodes alone are not a verdict. Working prior:
**+15–40 at 1T**, heavily non-additive with Phase 8.5.

> **11.0 (prep, no games) — ordering/pruning recall telemetry.** Added
> 2026-07-30 from an r/chessprogramming post (a 39M policy-net + alpha-beta
> engine; the *result* does not transfer — see below — but the method does).
> They expanded only the policy's top-4 moves per node and never checked
> whether the reference best move was in that beam; it was absent **29.4%** of
> the time (recall@1 33.7%, @4 70.6%, @16 96.3%). Their line is the useful
> one: *no amount of depth or pruning recovers a move you never generate.*
>
> **Our analogue is pruning, not generation.** The staged MovePicker emits
> every legal move, so nothing is structurally absent — but LMP, futility and
> history pruning *discard* moves, and we count how many
> (`lmp_prunes`/`fut_prunes`/`hist_prunes`) without ever recording **whether
> the move that turned out best was among them**. Same blind spot, one layer
> over.
>
> **Why this is worth building here specifically:** the whole 8.5.10 ladder was
> adjudicated on bench node counts — (a) +82%, (c) +30%, (d) +22…60%, all
> vetoed without an SPRT — and the one rung that did reach a gate read **−26%
> nodes and −84 Elo**. Node count conflates *orders better* with *prunes
> harder*, and that is exactly the distinction all four turned on. Add: a
> cutoff-rank histogram (move index at beta cutoff, capped ~8 + overflow),
> first-move-cutoff rate, and a best-move-was-pruned counter. Pure counting,
> bench-neutral, slots into the 8.6.6/9.3(c) telemetry. It is a **diagnostic,
> never a gate** (§1: SPRT decides), and it is the acceptance input 11.1–11.6
> and the 8.5.10 residue at 11.7 should be read against.
>
> ⚠ Do **not** import their Elo. ~2000 → ~2192 is an engine repairing a
> structural hole far below our level; the fix was *enabling LMR*, which we
> have had since Phase 1 and have SPSA-tuned repeatedly; and it was measured
> against fixed SF-limited brackets, not an SPRT (§3 lesson 8 — and "two
> independent brackets agreed within 1 Elo" is not a credible precision claim
> at that sample size).

1. **11.1 Unified contextual reduction:** compute one signed `r` for every
   move after the first from PV/cut status, TT-PV/depth/bound/move class,
   improving/opponent-worsening, move count, check/capture state, accepted
   quiet/capture/continuation histories, correction uncertainty and prior
   reduction. Derive `lmrDepth = newDepth - r` once and reuse it for futility,
   history and SEE pruning. Stage separately: shared calculation with behavior
   parity; second-move eligibility; checks; good/bad captures; negative
   reductions/extensions; then removal of obsolete categorical exceptions.
   Each behavior step gets its own SPRT. **`cutoffCnt` (8.6.8, skipped
   pre-1.9.1 by user decision 2026-07-20) — DOWNGRADED 2026-07-28 from
   MUST-INCLUDE to *candidate input*.** The free cross-review prior this item
   asked for came back **negative**: Rarog gated the SF `cutoffCnt` term inside
   a full LMR-family re-tune and lost **−7.78 ± 8.00**, abandoned with no
   retry, on the first clean run after its harness-affinity fix (the candidate
   ran in the historically *favoured* slot and still lost, so the −8 is if
   anything conservative). Its root cause is the one to fear here: the tuned
   candidate searched **16% more aggressively**, won its self-play SPSA, and
   then lost to the more accurate baseline. Rarog also corrected the record on
   our side — the "+15.6" we remembered is Basilisk 1.5.0's re-tune of an
   *untuned* LMR formula under the pre-pinning harness, not a `cutoffCnt`
   validation — so no engine has yet shown this term positive. It may still
   enter as one input among the others (our telemetry does say the LMR family
   is very timid, re-search rate 1.04%), but it enters on its own merits with a
   negative prior, is never auto-included, and **must be gated against the
   accepted head, never against a sibling of its own tuning run.**
   **MUST INCLUDE #2 (the LMR post-move-`gives_check` bug — found 2026-07-23,
   standalone fix REJECTED −18/−19 Elo @1.1k, §7): the unified `r` computes
   the check state from the CORRECT pre-move answer BY CONSTRUCTION, so the
   bug is designed out here rather than patched onto the old code. The
   standalone fix lost because the hcefinal LMR constants were tuned WITH the
   bug's accidental over-reduction (lesson-15 de-tuning); the loss is a
   re-tune candidate, NOT proof correct check-handling is bad. Re-measured
   here as part of the reduction rework and re-tuned in 11.7 — the aggressive-
   reduction benefit the bug supplied must be reclaimed through the proper
   knobs (LMR base/divisor/context) or via `cutoffCnt`. If it STILL H0s after
   the joint re-tune, the accident was near-optimal and we keep the current
   behavior; pre-register that outcome so it is not re-litigated.**
2. **11.2 Result-dependent verification:** choose deeper/shallower full-search
   depth from the reduced result, node confidence and prior reduction; train
   post-LMR history from both outcomes. → staged SPRTs.
3. **11.3 TT density and replacement → PULLED to pre-1.9.0 as 8.5.D1**
   (2026-07-15): it is eval-agnostic and durable, so it strengthens the final
   HCE release and carries to NNUE with no re-tune. See Phase 8.5 Track D.
   (Slot kept to preserve 11.4–11.7 references.)
4. **11.4 Bound quality:** blend RFP/qsearch proof values conservatively toward
   beta, preserve fail-soft futility bounds, and finish near-rule-50 TT cutoff
   safeguards from 8.5.14. → separate SPRTs.
5. **11.5 ProbCut/null/IIR:** staged ProbCut MovePicker with TT/capture-history/
   SEE ordering and TT-disproof skip; null-move verification min-ply region;
   audit IIR against PV/cut/all-node and TT-PV semantics. → standalone SPRTs.
6. **11.6 Correction-history consumption v2:** fit per-source weights instead
   of `/5`, add accepted 2-/4-ply continuation-correction contexts, and use
   absolute correction as uncertainty in selected margins. → staged fit +
   SPRT, with collision/support telemetry.
7. **11.7 Final search tune:** only after 11.1–11.6 are decided, generate a
   fresh configuration containing accepted histshape/wave2/correction/TM and
   pruning-margin dimensions — **regenerated from the `search_params.h`
   X-macro table (8.6.1), and it MUST carry the full LMR family incl. the
   11.1 `cutoffCnt` term (8.6.8's re-tune half), the **11.1 pre-move-
   `gives_check` LMR fix (its de-tuned −18 loss is re-tuned HERE — reclaim the
   aggressiveness through the knobs, or pre-registered-close if it still H0s)**,
   and `TmInstability`**. Exclude dead, rejected and redundant knobs;
   pre-register ranges and stop rule; SPSA → bake → CTest/telemetry → SPRT.
   Confirm at `10+0.1`, a genuinely longer TC, several hash sizes, and both
   production TUNE=OFF and test TUNE=ON binaries. This is the final tune the
   old plan incorrectly scheduled in 10.5.

**Phase-11 completion:** no known unsound bound, every search mechanism has a
telemetry/semantic test, final constants target the final net and architecture,
and 1T strength is validated against the prior 2.x head and current open-source
frontier opponents.

### Phase 12 — MT scaling beyond Phase 9 (what Phase 9 deliberately did not do)

**RE-SCOPED 2026-07-28.** The core of this phase — the MT harness, per-thread
root state, coordination, voting/stopping, clock safety, shared-state ownership
and diversity — **moved to Phase 9 (§5) and runs pre-NNUE**, because deployment
is 1T+4T (gate 10) and those are the two conditions we actually compete in.
What remains here is genuinely post-NNUE: the ≥8-thread regime, the NUMA/memory
work that only pays there, and the re-validation everything needs once the
evaluator has been replaced. Enter this phase only with Phase 9's MT harness
and 4T baseline already in hand.

1. **12.1 Re-validate the Phase-9 SMP design on the net.** NNUE changes the
   node/eval cost ratio, which is exactly what the coordination and
   time-allocation decisions were tuned against: re-measure 9.5's mechanisms
   (bound-typed publication, pool-seeded windows, majority stop) and 9.10's
   threshold at 4T on the NNUE head before assuming they still hold.
   ⚠ Do not re-open 9.9's diversification question here unless 12.2 gives a
   *new* reason — it was measured at 4T on the HCE head and its prior is a
   null in two engines.
2. **12.2 The ≥8-thread regime.** Our own 1T→8T smoke reading was **5.72×
   aggregate nodes for 1.49× wall-clock**, and the 2026-07-25 cross-engine set
   put Basilisk 1.9.1 at 13.44×/84% NPS scaling at 16T (vs Stockfish 14.42×/90%,
   Reckless 13.37×/84%). ⚠ Rarog chased its own 16T deficit and found it was
   **not** in the engine: with the whole table resident in L3 the deficit
   survived (bandwidth ≈3%), turbo contributed ≈0%, and ~6% was OS/SMT
   placement — i.e. the lever the item was written around did not exist, and
   the TT design was exonerated (64-byte aligned, prefetched, batched
   counters). Read that as the prior before spending time here, and note that
   scaling *percentage* correlates inversely with per-thread NPS across
   engines, so it is a confounded metric: chase shared-memory traffic and
   absolute throughput, never the percentage.
3. **12.3 Voting and stopping at scale.** Phase 9.5(d)/9.10 settle 4T; revisit
   only the parts that are genuinely N-dependent (a strict majority is a very
   different rule at 16 threads than at 4). ⚠ Depth-weighted votes are
   **closed, not deferred**: they exist to stop a late deep thread holding the
   search open, and that is the direction Rarog measured as costing **−15.85**
   at 2T. Do not re-derive them.
4. **12.4 Shared-state ownership at the NNUE scale:** which histories,
   corrections and accumulator caches are thread-local versus shared, and
   false-sharing/cache-line placement for the structures NNUE adds. Helper
   history blending is already resolved by 9.7 — do not re-add it.
5. **12.5 Topology and memory:** NUMA policy, TT sharing and first-touch,
   large-page support where portable, and scaling at realistic hash sizes
   (the 12.x home of the deferred TT multiply-hi full-budget indexing).
   Preserve deterministic 1T behavior.
   ⛔ **Engine-side thread/core pinning stays out of scope** (gate 10). It is
   not a lever we are allowed to pull, however good the placement number
   looks; the ~6% Rarog measured is diagnostic evidence about the OS, not a
   proposal. Harness-side affinity for SPRT/SPSA is unaffected.

**Phase-12 completion:** no 1T regression; statistically positive fixed-time
strength at the supported MT targets; scaling report for 1/2/4/8/16 threads;
zero races under sanitizer/stress; exact release-binary tests on the primary
Ryzen platform and at least one different CPU family.

### Phase 13 — NNUE architecture, data and frontier loop

Chess768 is the bring-up baseline, not the expected final frontier evaluator.
NNUE remains the strategic path even if the first net underperforms; failure
triggers contract/data/training/architecture diagnosis. The HCE remains a
debug/datagen reference and optional maintenance fallback, not the default
route back to top-engine strength.

1. **13.0 Evidence review:** record STC/LTC/MT Elo, NPS, net/data learning
   curves, quantization loss and 8.5.15 cohort residuals. Select the next net
   feature from measured residuals; do not use a single arbitrary +150-Elo
   threshold to declare the evaluator finished.
2. **13.1 Versioned architecture ladder:** each layout change updates the raw
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
3. **13.2 Data/training ladder:** scale beyond 60M according to held-out and
   Elo learning curves; A/B Basilisk versus stronger-teacher scores, search
   depth/node mix, WDL lambda, natural-end share, hard-position mining,
   endgame/king-attack/tactical cohort balance, optimizer/schedule and hidden
   size. Keep untouched test data and manifests immutable; never select a
   release net on test loss.
4. **13.3 HCE recovery menu (closed by default):** only if explicitly chosen
   for product fallback or a benchmark cohort where HCE must remain useful:
   winnability/material scaling, king safety/shelter, specialized endgames,
   passer/threat semantics, lazy-eval train/serve study and phase specialization.
   These do not replace the NNUE frontier loop.
5. **13.4 Frontier acceptance:** compare current Basilisk against full-strength
   contemporary Stockfish, Reckless, PlentyChess and at least one other current
   independent engine, using direct games where informative and calibrated
   node odds otherwise. Validate 1T and target MT, multiple hash sizes, STC,
   `10+0.1` and a genuinely long TC. Seek external-list testing (CCRL or
   equivalent) before claiming top-tier proximity.

### Deferred / reopenable experiments

**🐛 LMR gate reads a POST-MOVE `gives_check` (found 2026-07-23 during 8.7.3;
latent, pre-existing).** In `negamax`'s `search_one` the `move_gives_check()`
lambda is lazy, and the LMR reduction gate (search.cpp ~1602) is the first
caller for any move that skipped every earlier pruning branch — but that call
happens AFTER `do_move`, so it evaluates `gives_check(m)` on the CHILD
position, for a move already played. The value is deterministic (hence bench
was stable) but semantically wrong: the LMR "don't reduce checking moves"
exemption has been keyed off a meaningless bit for exactly the moves that
reach it. Fixing it (evaluate the pre-move `gc_premove` that 8.7.3 already
computes correctly, and feed it to the LMR gate) moved bench 11,941,440 →
16,365,460 — a real search change. **Deferred as a STRENGTH item (own SPRT):
Phase 8.7 is pure-speed by contract, so 8.7.3 preserves the quirk verbatim.
Likely positive (correct LMR data on checking moves), plausibly a wash;
worth a `[0,3]` SPRT after the speed pass or folded into 11.1's LMR rework.
Prior art: none — this is a fresh find.**

> **STANDALONE FIX ❌ REJECTED 2026-07-23 (−19.64 ± 13.43, LLR trending H0,
> CI excluded 0 @1.1k games, user-stopped); RELOCATED to 11.1 + 11.7, NOT
> abandoned.** The one-line fix (`(void)move_gives_check();` before
> `do_move(ss, m)` in `search_one`, forcing the lazy lambda pre-move) took
> bench 11,941,440 → 16,365,460, CTest 11/11 — but lost ~18-19 Elo standalone.
> **Why it lost, and why it is NOT abandoned:** the garbage bit usually reads
> "not a check," so the buggy engine OVER-reduces those moves → smaller tree →
> more depth at fixed time. The accident is a net-positive aggressive-LMR
> heuristic, and the hcefinal SPSA tuned the LMR constants WITH it present, so
> the standalone fix is a textbook lesson-15 de-tuning victim — the loss does
> NOT prove correct check-handling is bad. **The fix is therefore designed
> INTO 11.1's unified reduction (the pre-move answer by construction) and
> re-tuned in 11.7 (reclaim the aggressiveness through LMR base/divisor/
> context/`cutoffCnt`). Pre-registered: if it still H0s after the joint
> re-tune, the accident was near-optimal → keep current behavior, close.**
> User decision 2026-07-23: move to the later step, do NOT ship the standalone
> fix. Final SPRT −21.55 ± 9.83, LOS 0.00%, LLR −1.72 @2,002 (user-stopped).
> The fix is a documented one-liner (rebuild at 11.1, not worth keeping a
> pre-NNUE binary). Development stays at 11,941,440; the Phase-8.7 speed pass
> continues unaffected.

(The old Phase-8 "feature menu" was closed by the 2026-07-01 audit — EV ≈ 0,
list survives in git history. The **Phase 8 label now names the 2026-07-13
hardening phase, §4**.) What remains
here is only what was **skipped, not rejected** — things with a real, if
small, chance of paying — plus the conditions under which they make sense.
Rejected-with-measurement items (6.2 cont-hist6 −7.70, capture-futility-active
−2.78, do_deeper margins, TM SPSA values) stay closed.

**The unifying rule (§1 cost principle): comprehensive search-constant tuning
happens once, in Phase 11.7, after NNUE and after the final Phase-11 search
architecture.** Phase 10.7 permits only isolated safety calibration required to
make the net searchable; it is not the histshape/wave2/TM campaign.

> **⚑ HCE-line note (2026-07-13):** after the HCE/NNUE branch split, the
> final-scale condition is met on `master`/`development` (the HCE eval is
> final there), so these items became runnable for the HCE finalization. The
> user chose to spend only a small budget: staged LMR context-adjustment
> SPRTs (the four inert `Lmr*Adj` knobs) — the rest of this table stays
> deferred. The "post-NNUE" framing still governs the `nnue` branch.

| Deferred item | What it is | Est. value | When to run |
|---|---|---|---|
| **histshape SPSA** (was 7.4) | history bonus/malus shape plus accepted history/LMR dimensions; regenerate the config after 8.5.10/8.5.11/11.1 so it contains no obsolete tables or knobs | unknown | Phase 11.7 final tune only |
| **wave2 mechanisms** | accepted `CapFutDepth`/`QuietSeeDepth`/post-LMR/fractional-LMR dimensions; `QsearchCheckCap` is excluded unless 8.9 made quiet qchecks live | unknown | Phase 11.7, after history-aware `lmrDepth`; never seed known-negative values |
| **TM knobs** | time-management constants; the old SPSA washed under the old root model | unknown after NNUE/root v2 | Phase 11.7 only, after 8.5.12 supplies variance/instability/root-effort inputs |
| **6.6 fail-low prior countermove bonus** | skipped during Phase 6 | small | absorbed into 8.5.10(d) on `development` |
| **fractional history** | quantization experiment flagged during 6.7 | likely wash | only if still meaningful after 8.5.11, then Phase 11.7 |
| **full-budget TT indexing** (mul-hi instead of the pow2 mask; from the 2026-07-20 TT audit, recorded at 8.6.2c) | use the entire `Hash` budget — pow2 flooring wastes up to ~50% at unlucky sizes | LTC/large-hash relevant (cf. TT-density +4.27) | Phase 12.5, with the topology/memory work |
| **cutoff-count LMR + LMR-family re-tune** (8.6.8 opt-in) | SF `cutoffCnt`. **VERDICT IN 2026-07-28:** the cross-review prior we were waiting for came back **negative** — Rarog gated it inside a full LMR-family re-tune and lost **−7.78 ± 8.00** (abandoned, no retry; its candidate searched 16% more aggressively, won its self-play SPSA and then lost to the accurate baseline). The "+15.6" prior was our own 1.5.0 re-tune of an *untuned* LMR formula, not a `cutoffCnt` validation | **negative** (was: Rarog EV +3–8) | 11.1 as a *candidate input only*, never auto-included; re-tuned at 11.7. No standalone opt-in |
| **check-extension-removal BUNDLE** (8.6.7 standalone ❌ −10.17 @ 4.7k, 2026-07-20; 8.5.8 closed for HCE) | removal + joint re-SPSA of the consumers it de-tunes (RFP/razor/null/futility/SEE + LMR family, ~16 dims), ONE gate — the standalone gate vs the hcefinal-tuned head was partly rigged (lesson 15); Rarog's +30.75 predates its LMR re-tune; **the −10.17 also carries one unpinned-harness bias draw (lesson 10)** | unknown; +30.75 at Rarog | post-NNUE recalibration (13.x-era / with 11.7), where these constants are re-fitted anyway; pre-registered: if the bundle H0s there too, closed permanently |
| **cuckoo re-trial** (rejected twice 2026-07-17, unpinned harness) | cuckoo-hash repetition detection; both rejects predate the affinity fix and each carries one ±10 bias draw — two same-sign draws are unlikely but possible; lesson-10 candidate for one clean fixed-N probe | probably wash (two rejects) | post-1.9.1 opt-in only; one fixed 10k probe, keep-or-close permanently |
| **postlmrhist re-trial** (rejected 2026-07-16, unpinned harness; `PostLmrHistScale` inert at 0) | post-LMR continuation-history nudge; single biased-harness reject, knob still registered — probe is options-only (no build), same protocol as 8.6.8A run A | unknown, single reject | post-1.9.1 opt-in only; options-probe fixed 10k, keep-or-close permanently |
| **SPSA discipline** (pre-registered, from the 2026-07-02 EV review) | 1000–1500 iters, abort at ~600 if trend ≤ 0, converged candidate → bake → SPRT + full CTest; a wash → keep defaults, done | — | applies to every run above |

Explicitly **not** reopenable by default: more HCE self-play cycles (cycle 6
proved that loop exhausted at the 1.8.0 head), HCE king-bucketed PSTs (build
NNUE king buckets instead), and the audit-closed HCE feature-name menu.

---

## 8. Release discipline

### Versioning (SemVer-adapted; no public API, so it maps to strength)

- **MAJOR** — architecture swap: NNUE ships as **2.0.0**.
- **MINOR** — a phase/campaign banking SPRT + gauntlet-validated strength
  (1.5.0 → 1.8.0 were all this). The version reflects **cumulative content
  since the last tag**, not the latest step.
- **PATCH** — focused maintenance/robustness/speed work without a broad new
  engine campaign. A modest measured gain may still be reported honestly
  (1.9.1 and 1.9.2 are explicit user-chosen precedents).
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
   `tools/gauntlet.ps1`. **Colosseum is the tournament manager** (LittleBlitzer
   is out of scope as of 2026-07-28 — Colosseum supersedes it; historical
   LittleBlitzer numbers in `CHANGELOG.md` stay as record). Per gate 10 a
   boundary gauntlet runs at **both 1T and 4T**. Calibrated slate
   (all in `D:\chess\engines`): prior Basilisk release (the head-to-head that
   matters), Fruit 2.1, Rarog 2.2.0, Rybka 3, Critter 1.6a, SF
   `UCI_LimitStrength` 2800/2900/3000, HIARCS 14, Shredder 12. Read the
   **head-to-head vs the prior release**; treat absolute estimates loosely.
   From 2.0 onward add full-strength contemporary Stockfish, Reckless,
   PlentyChess and another current independent engine; use calibrated node
   odds when direct scores are too one-sided.
4. Scan for illegal moves / forfeits / crashes (`t=`, `i=` counts = 0 for
   Basilisk; throttled-SF incidents in lost positions are benign). **Do not use
   Colosseum 1.0.2's zero-forfeit count as the sole tight clock-safety gate:**
   its hard-coded 2 s timeout tolerance masks smaller overruns. Preserve a
   fastchess clock run at 20 ms margin until that tolerance is configurable.
5. For a CCRL-style estimate: anchor Ordo to a published engine
   (`ordo -a <ccrl> -A "<name>"`), slower TC, treat as ±50–100.
6. **Every release from Phase 9 on validates BOTH deployment conditions**
   (gate 10): the 1T gate above **and** a Threads=4 run at `Hash 256` with
   recorded topology/hash/manifests and **zero time forfeits**. A 1T SPRT
   cannot see MT strength or MT clock safety, and 4T is a shipped condition,
   not a claim we opt into. Thread counts beyond 4 are reported as scaling
   diagnostics, not gates, until Phase 12.

### Release checklist (the model runs this when asked to "release X.Y.Z")

1. Confirm the gate above passed.
2. Bump the version in **both** places: `src/constants.h` (`engineVersion`) and
   `CMakeLists.txt` (`project(basilisk VERSION …)` — drives the dist tag).
3. Update `CHANGELOG.md` (Keep-a-Changelog entry with SPRT + gauntlet numbers,
   honest fast-TC vs LTC framing) and `README.md` only if something a *user*
   sees changed — a feature, a UCI option, an asset tier, a build instruction.
   Both are **user-facing** documents (§1 audience table): no phase numbers, no
   step-level bookkeeping, no roadmap.
4. Build the complete production ISA matrix with fresh revision-matched PGO.
   Test the **exact files to upload**: no `BASILISK_TUNE` UCI options, all
   CTest/sanitizer-required gates pass, bench runs, manifests/checksums match,
   and the generic binary runs on its declared baseline CPU. For NNUE, record
   embedded-net SHA/architecture and require incremental/full/reference parity.
5. Commit the prep on `development`. **Do not tag. Do not push.**
6. Produce copy-pasteable GitHub release notes (summary, strength vs prior tag,
   changes, which asset to download, honest caveats). **User-facing** — write
   them from `CHANGELOG.md`, not from this plan (§1 audience table).
7. **User:** squash → `Version X.Y.Z` on `master`, push. Then **publish a
   GitHub Release** — e.g. `gh release create vX.Y.Z --target master --title
   "Basilisk X.Y.Z" --notes-file <notes>` — which creates the tag (if absent)
   and publishes in one step. **⚠ `release.yml` fires on `release: published`,
   NOT on a bare tag push**, and its upload step (`gh release upload`) requires
   the release to already exist — so `git tag && git push` ALONE builds and
   uploads NOTHING (verified 2026-07-23). Publishing the Release fires the
   workflow, which builds/tests all tiers and uploads the documented **PGO
   production assets** (PGO binaries named WITHOUT `-pgo`; no `*.manifest.txt`
   or `*.sha256` shipped). Local and public binaries may use different
   portability tiers but must share source, defaults, embedded net and
   recorded behavior. (Manual re-run if a build flakes: the `workflow_dispatch`
   input takes the existing release tag.)

### Compute budget

Ryzen 9 5950X, shared. Texel fits = CPU-minutes, run freely. Datagen/SPSA at
concurrency ~24; SPRT/gauntlets via repo script defaults. NPS is not a
bottleneck (~2.7M nps single-thread; PEXT + LTO + PGO local builds) — profile
before any micro-optimisation.

---

## 9. Quick commands

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

# 4-THREAD gate (deployment condition #2). Everything follows from -Threads 4:
# Hash 256 (64 x threads), concurrency 3 (floor((cores-2)/threads)), and
# -use-affinity dropped by the script itself. Run the Threads=4 null FIRST and
# require zero forfeits (the script counts them), then the candidate:
.\tools\sprt.ps1 -EngineA <copy.exe> -EngineB <same.exe> `
    -NameA Self -NameB Self2 -Mode calibrate -Threads 4 -Games 10000
.\tools\sprt.ps1 -EngineA <cand.exe> -EngineB <base.exe> `
    -NameA Candidate -NameB Baseline -Elo1 3 -Threads 4
# ~10k games minimum at 4T — nothing shorter separates 0 from +3 (gate 10) —
# and at concurrency 3 that is a ~10-12 h run. Pass -Games explicitly for a
# null: the 30k calibrate default is sized for 1T's concurrency 14.
# MT scaling number (a diagnostic, not a gate; one binary, equal TC):
.\tools\sprt.ps1 -EngineA <head.exe> -EngineB <head.exe> `
    -NameA "4T" -NameB "1T" -ThreadsA 4 -ThreadsB 1 -Mode fixed -Games 4000

# NPS A/B (self-pair validate first; pool >=2 PGO builds per arm; idle box)
.\tools\nps_ab.ps1 -EngineA <cand.exe> -EngineB <cand.exe>            # self pair
.\tools\nps_ab.ps1 -EngineA <cand.exe> -EngineB <base.exe> -Rounds 12

# Self-play datagen (ALWAYS the diverse book — see §3 lesson 4)
.\tools\datagen.ps1 -Suffix <head> -Book tools\texel\data\beast_seed.epd `
    -BookFormat epd -Rounds 100000 -Nodes 8000 -OutputPgn tools\texel\data\selfplay_X.pgn

# Extract + phase-balance
python tools\texel\extract_parallel.py tools\texel\data\selfplay_X.pgn `
    --train train_X.csv --holdout holdout_X.csv --balance-phase 1.5 `
    --target-train 3500000

# Texel fit (sequential joint-bake — see §3 lesson 5)
cmake --build build\texel-verify --target basilisk-texel
.\build\texel-verify\basilisk-texel.exe --tune all tools\texel\data\train_X.csv `
    tools\texel\data\holdout_X.csv out_all.txt --l2 1e-6 --epochs 100
python tools\texel\bake.py out_all.txt --allow-pst
cmake --build build\texel-verify --target basilisk-texel   # rebuild before KS
.\build\texel-verify\basilisk-texel.exe --tune-kingsafety tools\texel\data\train_X.csv `
    tools\texel\data\holdout_X.csv out_ks.txt --max-positions 100000
python tools\texel\bake.py out_ks.txt

# NNUE Bullet baseline pipeline (Phase 10; NVIDIA CUDA training)
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

# SPSA (post-9.1 schedule: -Iterations sets the horizon, the run stops itself
# there, and it is FROZEN at first launch; <5000 throws without -AllowShortRun;
# -REnd 0.0031 sets the end-of-run step ratio; resume with -Resume, which
# ignores a new -Iterations/-REnd by design and says so).
.\tools\spsa.ps1 -ConfigGroup <group> -EngineSuffix <base> -Iterations 5000
.\tools\spsa.ps1 -ConfigGroup <group> -Resume
python .\tools\verify_spsa_schedule.py     # schedule self-test (no games)
# Bake the TAIL MEAN of the whole vector from
# tools\weather-factory\tuner\trajectory.csv — not endpoints, not a subset.
# The comprehensive tune is Phase 11.7 ONLY, after the final search architecture.

# Fixed-game boundary gauntlet — Colosseum is the tournament manager;
# gauntlet.ps1 is the scripted fallback. Run BOTH 1T and 4T (gate 10).
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <prior-release>,<field...> -TC "10+0.1"
```

---

## 10. Bottom line

Phases 0–7 took Basilisk from 1.4.9 to 1.8.0 by building a serious test harness,
search baseline and mature HCE. The remaining route is explicit:

```text
Phase 8 on development                                        [✅ shipped in 1.9.0]
  correctness + release/CI infrastructure
Phase 8.5 on development                                      [✅ shipped in 1.9.0]
  eval-independent strength + NNUE data preparation
Phase 8.6 on development                                      [✅ shipped in 1.9.1]
  pre-NNUE hardening: param/TT/protocol hygiene, CI (Rarog 9.2 shape),
  A/B compiler equality, search telemetry, check-extension SPRT (rejected),
  8.6.8A accept-audit
Phase 8.7 on development                                      [✅ shipped in 1.9.1]
  profile-guided speed pass, +4.34% NPS ≈ +8.69 Elo, search bit-identical
  → Release 1.9.1 — the last shipped HCE release
Phase 9 on development                                        [✅ shipped in 1.9.2]
  harness first (SPSA schedule repair, MT-capable SPRT), then the audited
  SMP defects (thread cap, node counter, helper clock, coordination),
  index-hoists II, blending/pruning hygiene
  → Release 1.9.2 (explicit user version choice; focused accepted scope;
    1T+4T boundary gates passed)
  → Release 1.9.3 (tooling-only matching-llvm-profdata repair)
post-Phase-9 NNUE runway (8.5.3 / 8.5.14 / 8.5.15 / 8.5.16)
one rebase of nnue onto the released handoff SHA
Phase 10
  harden D:/code/net_trainer and integrate its Bullet 1x8 quantised.bin baseline
Phase 11
  final 1T search architecture, then the one comprehensive search tune
Phase 12
  MT scaling beyond Phase 9: the >=8-thread regime, NUMA/memory, NNUE re-validation
Phase 13
  versioned NNUE architecture/data ladder and frontier validation
```

“Done” does not mean every item was copied from Stockfish or every constant was
tuned. It means no known correctness/unsound-bound defect remains; search and
board behavior are observable and independently tested; public PGO/ISA assets
are the artifacts actually validated; 1T and MT gains survive appropriate time
controls; every network/data/test result is reproducible; and top-tier claims
are made against contemporary full-strength engines or external lists. The HCE
remains useful for fallback and diagnosis, but NNUE is the frontier path.
