# Basilisk Development Workflow Guide

This is the short operational roadmap. Detailed rationale, contracts, gates
and lessons live in [`PLAN.md`](PLAN.md).

## Current checkpoint

| Item | State |
|---|---|
| Branch | `development`; `master`/`v1.9.3` at `d737123` |
| Engine | bench **12,709,666** · CTest **12/12** · WAC **137/300** |
| Baseline for gates | `basilisk-5912-slim-pext-pgo` (bench 13,981,020) |
| Cumulative vs 1.9.3 | **≈ +12 Elo** — 5.9.14 +2.64, 5.9.13 +9.52 |
| Current phase | **Phase 5**, step **5.9.19** next |
| Evaluation | HCE **unfrozen** since 2026-08-25; corpus is on-policy self-play WDL |
| Corpus | `armC_basilisk25k_*` — game-result labels only, never engine evaluations |
| Reference | Stockfish `9587eeeb` — idea source only; 7 of 8 of its search ideas did not transfer |
| Oracle | branch `hybrid` `01df815`, frozen |
| Diagnostics | `Diag`=true ⇒ `info string diag kv`; suite `tools/diag/suite_v1.epd` |
| Portability | `origin/arm_fix` unmerged, owned by 5.11 |
| Next release | **1.9.4** at 5.13 on current evidence; 1.10.0 needs +40 Elo |

### Independence — binding, not aspirational

**Basilisk stays an independent engine.** Stockfish is an idea source, a
diagnostic oracle and evidence of what a mature search achieves — never
something to become. Both are GPLv3, so copying would be legal; that is not the
constraint. A transcribed engine inherits decisions we cannot explain and
discards work that already measured well.

| Do | Don't |
|---|---|
| Learn what problem a mechanism solves, then design our answer | Copy source, or paraphrase it into a translation |
| Reimplement in our idiom, types, structure and parameter table | Mirror upstream file layout, decomposition or naming |
| Treat reference constants as starting points to validate | Import constant tables verbatim — they were fitted to another search and scale |
| Record deliberate divergence as *intentionally different* | Delete a Basilisk-original mechanism because upstream lacks it |
| Attribute the idea in source and `README.md` | Accept a cluster because its trace looks more Stockfish-like |

**The test:** state why Basilisk does it this way *without* saying "because
Stockfish does". No such answer ⇒ not understood well enough to ship.

### Program order — Phase 5 is not an NNUE shortcut

```text
Phase 5  build the engine up: search → HCE → correctness/platform/SMP → release
Phase 6  NNUE runway: corpus, state contract, trainer preflight
Phase 7  train and integrate the baseline NNUE → 2.0.0
Phase 8  the engine adjustments NNUE makes necessary, then the single SPSA
```

Phase 5 ends when **its own** release gate passes, not when NNUE looks
reachable. No datagen or trainer work during Phase 5. A stronger engine is a
better NNUE teacher, so skipping ahead costs twice.

Equally, this is not a reason to abandon NNUE: search work here is
evaluator-agnostic and survives intact, and HCE work here improves the 7.1
teacher. The historical engine ladder still moves to 8.4 and still does not
block NNUE.

## Closed phases

### Phase 1 — Foundations and first strength line — ✅ 1.0.0–1.8.0

Built the board/UCI/search/TT/history/SEE/Syzygy/SMP stack, serious testing and
the accepted HCE. Repeated HCE self-play fitting stopped transferring.

### Phase 2 — Correctness and search architecture — ✅ 1.9.0

Banked state, repetition/rule-50, TT/mate/SEE correctness, staged ordering,
correction/history, root-instability TM and dense TT improvements.

### Phase 3 — Hardening, CI and PGO speed — ✅ 1.9.1

Centralized parameters, expanded invariants/CI/telemetry and shipped a
behaviour-identical **+4.34% NPS** PGO speed pass.

### Phase 4 — SMP durability and release tooling — ✅ 1.9.2/1.9.3

Repaired SPSA/MT harnesses, helper clock/node/thread safety and data tooling;
accepted +30.42 ±8.77 Elo at 4T with zero forfeits. 1.9.3 fixed PGO tool
matching without changing search.

## Forward phases

### Phase 5 — Search and evaluation acceleration (→ 1.10.0 or higher)

Steps run in the order listed. `EXPERIMENTS.md` holds the evidence for every
line; this is the tracker only.

**Evidence and instrumentation**

- [x] **5.0** Baseline — clean 1.9.3 reproduced.
- [x] **5.1** Search oracle — search **+322.7 ±36**, HCE **+232.8 ±32**.
- [x] **5.2** Differential diagnostic harness — 107-position suite, `diag kv`.
- [x] **5.3** Idea inventory and cluster split.

**Search clusters — closed**

- [x] **5.4 A** ordering, histories, LMR — no accepted change.
- [x] **5.5 B** static eval, TT, qsearch — contracts already sound.
- [x] **5.6 C** main selectivity — history-pruning defect recorded, not gated.
- [x] **5.14** shallow-depth node cost — target withdrawn after measuring.

**5.9 HCE maturity — IN PROGRESS (≈ +12 Elo so far)**

- [x] **5.9.1** coverage close-out — 7 terms added.
- [x] **5.9.2** mechanism search — bishop outpost, `king_protector` split.
- [x] **5.9.3** structure freeze — endgame named as the real gap.
- [x] **5.9.4** joint Texel refit — ran on a distilled corpus; the defect.
- [x] **5.9.5** king-safety fit — scalars only, table deferred.
- [x] **5.9.6** gate — **REJECTED −77.92**; labels were Stockfish evaluations.
- [x] **5.9.11** corpus rebuilt on-policy — 3 label sources, none passed.
- [x] **5.9.15** LTC probe at `10+0.1` — no depth story.
- [x] **5.9.14** king-safety reshape — **ACCEPTED +2.64 ±2.05**.
- [x] **5.9.12** full-surface fit — 768 PSTs unfrozen.
- [x] **5.9.13** gate — **ACCEPTED +9.52 ±4.66**.
- [x] **5.9.16** remove the 3×-refuted 5.9.1/5.9.2 terms — accepted, neutral.
- [x] **5.9.7** recogniser inventory — rook endings dominate (5 of top 7).
- [~] **5.9.17** KBNK conversion — **13.0% → 54.5%** mated; stalemates still open.
- [x] **5.9.18** endgame conversion floors in CTest — 4 families, 48s.
- [ ] **5.9.19 KBNK stalemate avoidance** — 14.5%, unchanged so far. ← **NEXT**
- [ ] **5.9.20** KBNK fifty-move cases — 31%.
- [ ] **5.9.21** generic mate-drive gradient — 5 and 4 cp/step; KBBK 3/12.
- [ ] **5.9.22** KBBK conversion, after 5.9.21.
- [ ] **5.9.8** classification recognisers — the second hypothesis.

  `KQ-K` and `KR-K` convert **100/100** — no step, no attention needed.
- [ ] **5.9.9** grading on top, only if 5.9.8 holds.
- [ ] **5.9.10** endgame gate — one SPRT, TC ladder required.

**5.7 extensions — reopens after 5.9**

- [x] **5.7.1** contract inventory.
- [x] **5.7.2** `singularQuietLMR` @ 401 — **ACCEPTED by decision**, not a gate.
- [x] **5.7.3** check/singular exclusivity — refuted, fails WAC floor.
- [x] **5.7.4** `ttValue >= beta` verification search — refuted.
- [x] **5.7.6** dead-code removal — behaviour-neutral.
- [ ] **5.7.5** singular gate depth 5 vs 6 — **undecided**, needs games or SPSA.
- [x] **5.7.7** integrated gate — skipped; only 5.7.2 survived.

**5.8 root and clock — reopens after 5.9**

- [x] **5.8.1** contract inventory.
- [x] **5.8.2** aspiration instrumentation — 1.37 re-searches per iteration.
- [x] **5.8.3** fail-low narrows beta — refuted.
- [x] **5.8.4** delta growth rate — refuted.
- [x] **5.8.5** fail-high depth reduction — refuted, fails WAC floor.
- [x] **5.8.6** document the `reported_score`/`score` split.
- [ ] **5.8.7** clock and time allocation — not yet opened.

**Consolidation and release**

- [ ] **5.10** correctness and safety repairs only.
- [ ] **5.11** portability and ISA baseline — `origin/arm_fix` unmerged.
- [ ] **5.12** SMP effectiveness checkpoint.
- [ ] **5.13** cumulative checkpoint and release gate — **1.10.0** needs +40 Elo
      over 1.9.3; at ≈ +12 the fallback **1.9.4** is the likely release.

### Phase 6 — NNUE runway and branch convergence

- [ ] **6.0** inventory old `origin/nnue`, then reimplement/cherry-pick only
      useful seams; do not rebase its obsolete `.mnn` contract wholesale.
- [ ] **6.1** per-ply state and complete dirty-piece make/unmake contracts.
- [ ] **6.2** frozen teacher/residual/search-disagreement corpora.
- [ ] **6.3** pinned `D:/code/net_trainer` data/manifests/resume preflight.
- [ ] **6.4** bench-identical runway gate and integration branch.

### Phase 7 — Baseline NNUE (→ 2.0.0)

- [ ] **7.0** harden trainer CLI, splits, manifests, determinism/conformance.
- [ ] **7.1** controlled 30–60M initial data and label/mining A/Bs.
- [ ] **7.2** H=512 pilot/H=1024 baseline with at least two seeds.
- [ ] **7.3** strict scalar loader/embedded net and exact references.
- [ ] **7.4** incremental accumulators and exact portable/x86/ARM64 kernels.
- [ ] **7.5** baseline data/architecture iteration one variable at a time.
- [ ] **7.6** gross NNUE-scale search safety calibration only.
- [ ] **7.7** HCE/STC/LTC/4T/external/parity gates and release 2.0.0.

### Phase 8 — NNUE frontier and final search fit

- [ ] **8.0** residual and search-disagreement analysis.
- [ ] **8.1** scale/deduplicate data, natural finishes and hard-position mining.
- [ ] **8.2** evidence-led king/threat/material/width architecture ladder.
- [ ] **8.3** resolve deferred search architecture with categorical A/Bs, then
      run the single planned post-NNUE search SPSA and ablations.
- [ ] **8.4** contemporary frontier plus contextual historical-engine ladder.

### Phase 9 — Scaling, platforms and product completeness

- [ ] **9.0** continue the 5.4 baseline into high-thread/NUMA/root/TT/
      accumulator scaling; no staggering retry without new Basilisk evidence.
- [ ] **9.1** advanced memory, full-budget TT and runtime ISA dispatch.
- [ ] **9.2** demanded product or additional-platform work; baseline ARM64 and
      NNUE/NEON parity are already release gates in 5.3 and 7.4/7.7.
- [ ] **9.3** scaling/platform release matrix.

### Phase 10 — Optional HCE fallback

Enter only after serious NNUE integration/data/architecture retries fail and
the user explicitly abandons that program.

- [ ] **10.0** document NNUE failure and approve HCE scope.
- [ ] **10.1** select a small residual-driven HCE program.
- [ ] **10.2** run one HCE fit and full external release matrix.

## Where we are

| | |
|---|---|
| Engine | bench **12,709,666** · CTest **12/12** · WAC **137/300** |
| Cumulative vs 1.9.3 | **≈ +12 Elo** |
| Next step | **5.9.19** — KBNK stalemate avoidance |

Evidence and reasoning for every step live in `EXPERIMENTS.md`; scope and
rationale live in `PLAN.md`.

## Decision rules

| Situation | Action |
|---|---|
| Behaviour-neutral | Exact bench plus correctness/performance evidence |
| Strength candidate | Registered SPRT; H1 accepts, otherwise revert behaviour |
| Root/TM/SMP | 1T STC/LTC plus 4T LTC, zero forfeits |
| Mechanism de-tunes consumers | Fix it inside its 5.4–5.8 cluster and fit jointly; defer to 8.3 only if no cluster owns it |
| Reference contract differs | Adopt the *idea*, implement it our way. "Looks more like the reference" never accepts a cluster |
| Cross-evaluator cohort | Adjudication **off** — score-based adjudication moved a headline estimate ~75 Elo |
| HCE proposal | Structural gap vs the reference ⇒ 5.9. Another constant refit ⇒ refused; cycle 6 washed |
| SPSA | Phase 8.3 only. A cluster's own small local refit is part of the cluster, not a tune |
| NNUE baseline loses | Diagnose contract/data/training/architecture; do not jump to HCE |
| Historical target unavailable | Record the gap; it does not block Phase 5 or NNUE |

## Working rhythm

```text
You   -> Paste completed long-job artifacts or ask for the next step.
Model -> Implements, verifies, updates PLAN + GUIDE and commits without push.
You   -> Run only the requested SPSA/SPRT/gauntlet/datagen job.
```

## Common commands

```powershell
.\tools\setup_tools.ps1
.\tools\build_test.ps1 -Suffix <name>
.\tools\sprt.ps1 -EngineA <candidate> -EngineB <baseline> `
  -NameA Candidate -NameB Baseline -Elo1 3
.\tools\sprt.ps1 -EngineA <copy> -EngineB <same> `
  -NameA Self -NameB Self2 -Mode calibrate -Games 30000
.\tools\nps_ab.ps1 -EngineA <candidate> -EngineB <baseline> -Rounds 12
.\tools\gauntlet.ps1 -Engine <candidate> -Opponents <list> -TC "10+0.1"
```

The fastchess harness pins one physical core per **game**, so `Threads=1`
resolves to **14 concurrent games** on this 16-core host — the two engines in a
game alternate and share the core. `harness_common.ps1` owns topology
discovery, the `-use-affinity` core list and the fastchess >= 1.7.0 gate that
BAS-M01 required.

**The Colosseum CLI is not adopted.** It was trialled and reverted: it
allocates a disjoint physical core to *each engine*, so 14 slots would need 28
physical cores and the pinned ceiling is 7 — half throughput for no measurement
benefit. The Colosseum **GUI** remains the tournament tool.
`tools/colosseum/` keeps converted profiles and tune vectors for when the CLI
is ready; nothing in the current workflow reads them.
