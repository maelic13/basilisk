# Basilisk experiment ledger

This is the indexed maintainer record of measured experiments and the lessons
that may inform later work. It is not a roadmap: [`PLAN.md`](PLAN.md) owns what
will be done and in what order. [`CHANGELOG.md`](CHANGELOG.md) remains the
user-facing release record.

Every lesson below is conditional. A result describes one engine state, test
protocol, time control, compiler and machine population; it does not establish
a universal chess-programming rule. An experiment from Rarog is only a prior
for Basilisk and never bypasses Basilisk's own gates.

## Contents

- [1. How to use this ledger](#1-how-to-use-this-ledger)
  - [Result and evidence vocabulary](#result-and-evidence-vocabulary)
  - [Recording contract](#recording-contract)
- [2. Measurement, harness and tuning](#2-measurement-harness-and-tuning)
- [3. Search and selectivity](#3-search-and-selectivity)
  - [Accepted or retained](#accepted-or-retained)
  - [Rejected, neutral or deferred](#rejected-neutral-or-deferred)
- [4. Root search, time management and SMP](#4-root-search-time-management-and-smp)
- [5. Evaluation and data experiments](#5-evaluation-and-data-experiments)
- [6. Throughput, build and platforms](#6-throughput-build-and-platforms)
- [7. Correctness and protocol lessons](#7-correctness-and-protocol-lessons)
- [8. Cross-engine evidence imported from Rarog](#8-cross-engine-evidence-imported-from-rarog)
- [8b. Cross-engine evidence imported from Manta](#8b-cross-engine-evidence-imported-from-manta)
- [9. Open retry map](#9-open-retry-map)
- [10. Template for a new experiment](#10-template-for-a-new-experiment)

## 1. How to use this ledger

Search the contents by subsystem before proposing a mechanism, tune or retry.
Use the stable IDs in commit messages and `PLAN.md` when a prior result changes
a future decision. Do not copy the tables into `PLAN.md`.

### Result and evidence vocabulary

| Term | Meaning in this document |
|---|---|
| **Accepted** | Passed the registered gate and entered an accepted baseline. |
| **Retained** | Kept for correctness, infrastructure or structural value; any Elo figure may be unresolved. |
| **Rejected** | Failed its registered gate or had a clear adverse measurement and was reverted. |
| **Neutral/inconclusive** | Evidence did not distinguish a useful effect at the tested resolution. |
| **Observation** | Diagnostic evidence, not an acceptance verdict. |
| **Imported prior** | Evidence from Rarog; useful for ordering or designing a Basilisk test, never for accepting it. |

Unless a row says otherwise, historical strength tests used paired UHO games at
fast time control. Results before the pinned-harness repair of 2026-07-21 may
carry a persistent scheduler-placement offset of roughly ±10 Elo per run. Fast
TC deltas are non-additive and may compress or reverse at longer TC.

### Recording contract

For every experiment that reaches a verdict, update this file in the same
commit that accepts, reverts or closes it. Record:

1. baseline and candidate source SHAs, dirty-diff hash if applicable;
2. hypothesis and interactions expected to move;
3. binary/compiler/PGO, book, TC, threads, hash, concurrency, affinity and
   adjudication profile;
4. registered gate, games, W-D-L, estimate/CI and LLR where available;
5. diagnostics separately from the verdict;
6. disposition, conditional lesson and an objective retry trigger.

Use cautious language: “under these conditions this suggests …”, not “feature
X is good/bad”. If conditions or artifacts are unknown, say so.

## 2. Measurement, harness and tuning

| ID | Experiment and conditions | Result / disposition | Conditional lesson and retry trigger | Source |
|---|---|---|---|---|
| BAS-M01 | Historical unpinned fastchess runs were compared with fixed explicit physical-core placement on the Ryzen 9 5950X. | Real scheduler offsets up to about ±10 Elo/run were found; the harness now discovers physical cores, pins explicitly and leaves two cores free. | On this Windows host, small unpinned results may be biased. Re-audit a borderline historical verdict only if it would affect a current decision. | `CHANGELOG.md` 1.9.1; legacy plan at `8dc0a24^` |
| BAS-M02 | Identical-binary null testing after harness changes. | The old symmetric `[-3,+3]` setup had zero expected LLR drift at equality; current policy is fixed-N 30k at 1T and 10k at 4T, requiring the full 95% nElo CI inside ±5. | Equivalence needs a calibration design, not an ordinary gain SPRT. Repeat after runner, scheduler, topology or adjudication changes. | `PLAN.md` §2 |
| BAS-M03 | Opening-book migration to `UHO_Lichess_4852_v1.epd`. | Retained as the common SPRT/SPSA/gauntlet book because it raised decisive-game signal. | The improved signal applies to this engine/protocol combination; do not compare raw Elo across old and new books without a bridge test. | `CHANGELOG.md` 1.9.0 |
| BAS-M04 | SPSA schedule audit: iteration/game units, PowerShell `$A`/`$a` collision and perturbation resolution. | Several schedule defects were repaired; the old runs annealed faster than intended. Accepted bakes remain accepted because they passed independent SPRTs. | A converged SPSA trajectory is not proof that its schedule was well calibrated. Assert every derived emitted value and validate integer perturbations across the whole horizon. | legacy plan at `8dc0a24^`; `CHANGELOG.md` 1.9.2 |
| BAS-M05 | Resignation threshold replay against historical score streams. | `400/3` one-sided was too aggressive for the engine's score scale; `600/3` one-sided became the shared `strength-v1` profile. | Adjudication scores are engine-scale dependent. Recalibrate after a material score-scale change such as NNUE integration. | legacy plan at `8dc0a24^`; `PLAN.md` §2 |
| BAS-M06 | Behaviour-neutral performance measurements with single builds/runs versus pooled PGO and identical-binary self-pairs. | An apparent −0.5% regression was retracted; pooled measurements found +0.17 ± 4.41 Elo and NPS inside noise for the tested patch. | On this host, a single PGO build or unbalanced placement is insufficient for small speed claims. Retry only with the pooled/interleaved protocol. | `CHANGELOG.md` 1.9.1 |
| BAS-M07 | Fast-TC release gains compared with longer gauntlets. | The 1.8.0 cycle measured about +93 fast-TC but about +40 at `10+0.1`; later release gates also showed wide external intervals. | Search/eval gains observed at `3+0.03` may compress with depth. Major boundaries require the TC ladder in `PLAN.md`. | `CHANGELOG.md` 1.8.0, 1.9.2 |

## 3. Search and selectivity

### Search-oracle observations (Basilisk's own)

These size Phase 5's two tracks. They are **observations**, not acceptance
verdicts: they identify where strength is available and in what order to
pursue it, and they credit no individual mechanism with any Elo. Elo below is
the ordinary logistic estimate from the score with a trinomial CI, not the
project's paired-pentanomial SPRT estimator.

Shared conditions for BAS-O01–O03 — Colosseum "Basilisk Oracle", 2026-08-12,
round robin ×200, **2,400 games**, 400 per pair, `3+0.03`, 1T, Hash 64, paired
`UHO_Lichess_4852_v1.epd`, concurrency 14, ponder/tablebases off, and **every
adjudication off** (max-moves, draw and resign). Terminations were entirely
natural: 1,919 checkmates, 316 threefold, 89 insufficient material, 74
fifty-move, 2 stalemate. **Zero forfeits.** Instrument: branch `hybrid`
`01df815`, binary SHA-256 `7E2433C3…5B99B06C`, Basilisk rev `39f1563` with
`src/` clean, Stockfish `9587eeeb`, Clang 22.1.8.

| ID | Experiment | Result | Conditional lesson |
|---|---|---|---|
| BAS-O01 | **Search isolated.** Stockfish `9587eeeb` search driving Basilisk's own unmodified 1.9.3 HCE, against native Basilisk 1.9.3. Only the search differs. | **302-88-10, 86.5%, ≈ +322.7 ±36 Elo.** The oracle won while running *fewer* nodes per move (170k vs 226k) and lower NPS (2.4M vs 2.9M). | Basilisk's dominant deficit is search coordination, and it is not a throughput artifact — the oracle was handicapped on speed and still won by a wide margin. Far larger than the ~+196 Rarog measured, partly because our direct-link adapter costs ~14% where their DLL cost ~37%. Sizes the Phase-5 search track. |
| BAS-O02 | **Evaluation isolated.** The same binary's exact-revision Stockfish HCE against Basilisk's HCE, with Stockfish's search held identical. | **264-106-30, 79.2%, ≈ +232.8 ±32 Elo.** | A second large but *smaller* deficit in HCE feature coverage. Notably below the +328.6 Rarog measured for their evaluator on the same instrument, so Basilisk's HCE is materially the better of the two — indicatively by ~96 Elo, though that is a cross-run comparison and not a controlled one. Sizes the Phase-5 HCE track and confirms it ranks second. |
| BAS-O03 | **Mechanism.** Average completed depth and effective branching factor at equal time, from the same tournament. | Control 29.7 plies / EBF 1.51; oracle 25.2 / 1.61; **Basilisk 15.6 / 2.20**; Rarog 15.5 / 2.21. Basilisk searched *more* nodes per move than the oracle and finished **9.6 plies shallower**. | The gap is tree shape, not speed: our effective branching factor is ~2.20 against ~1.61. This is the single most actionable number the experiment produced and it points squarely at ordering, reductions and selectivity — i.e. cluster 5.4 first. It also matches durable lesson 5 in reverse: our tree is not too small, it is too wide. |

| BAS-O04 | **Width attribution.** Mean completed depth over `suite_v1.epd` at a fixed 300,000 nodes, three arms, holding one side constant at a time. Run 2026-08-13 during the cluster-5.4 re-audit. | Basilisk native **20.80** (EBF 1.834); SF search + **our** eval **32.87** (1.468); SF search + SF eval **33.38** (1.459). Attribution: **search +12.07 ply = 95.9%**, evaluation +0.51 ply = 4.1%. **CORRECTED 2026-08-13** — those arms ran at unequal hash (Basilisk defaults to 64, the oracle to 16) and mixed two estimators. Re-measured with every arm at Hash 64 and one estimator: Basilisk 21.47, oracle 32.88, control 33.07 — **search 98.4%, evaluation 1.6%**. Conclusion unchanged and slightly strengthened. The EBF figures in this row are superseded by BAS-D05. The evaluation arm's paired split was 36 better / 42 worse. | Tree width is a **search-policy** property, not a symptom of evaluation quality — an evaluator +232.8 Elo stronger (BAS-O02) buys half a ply and is not even consistently deeper. This **refutes** the working hypothesis recorded when cluster 5.4 closed, which is withdrawn. Combined with BAS-S16 it says the reference is narrow because its decisions are better informed at the point of pruning, not because its margins are more aggressive: pruning the same decisions harder is blindness, and games price it as such. |

**Internal consistency.** Measured directly, full Stockfish beat Basilisk 1.9.3
by 364-33-3, ≈ **+516.1 ±59**. Composing the two isolated legs gives
+232.8 + 322.7 = +555.5. The 39-Elo shortfall sits inside the combined
interval and is the expected direction for non-additive Elo, so the two legs
and the whole measure the same thing. A large disagreement here would have
meant the isolation was leaking; it did not.

**Also recorded.** Basilisk 1.9.3 − Rarog 2.3.2 measured 137-143-120,
≈ **+14.8 ±27** — consistent with, but weaker than, the +30.4 from Rarog's run,
and not distinguishable from zero at this sample. Do not treat the two engines
as separated at STC on this evidence.

### Differential-harness observations

Conditions for BAS-D01–D02 — `tools/diag/suite_v1.epd` (107 positions: 40 UHO
openings, 40 WAC tactics, 17 endgames, 6 zugzwang, 4 in-check), Basilisk
`development` with the 5.2 counters, 1T, `Diag` on, driven by
`tools/diag/run_suite.py`. Internal counters at fixed **depth 12**;
differential at fixed **300,000 nodes** against the 5.1 oracle (`hybrid`
`01df815`, `Use Basilisk HCE=true`, so the evaluator is ours and only the
search differs). Baseline artifact `tools/diag/baseline_v1.json`.

| ID | Measurement | Result | Conditional lesson |
|---|---|---|---|
| BAS-D01 | **Where the width is not.** Move-ordering quality at the cutoff. | **First-move cutoffs 89.10%**, mean cutoff index **0.214**. Cutoff sources: TT 24.6%, good captures 49.3%, quiets 25.4%, bad captures ~0%. | Ordering is already strong and is **not** the cause of the EBF gap. This refutes the second-priority hypothesis carried into 5.2 and removes move-picker rework from cluster 5.4's likely content — a saving, since ordering work is expensive and would have been measured against an unmoving baseline. |
| BAS-D02 | **Where the width is.** LMR gate accounting; the identity `eligible = applied + clamped_zero + Σ blocked` holds exactly on every run. | Of eligible moves only **36.1%** are reduced; **16.2%** pass every gate and then compute a reduction of **zero**; mean reduction when applied is 2.354 plies; and the **re-search rate is 1.744%**. | Reductions are far too conservative. A re-search rate near 1.7% means reductions almost never need undoing, which is the signature of under-reduction rather than of a well-tuned policy — a healthy LMR pays for its depth with visibly more re-searches. Combined with a sixth of eligible moves being reduced by zero, this is where the tree width is created. Points directly at the reduction/re-search contract, cluster **5.4.3**. |

**BAS-D06 — shallow-depth node cost, step 5.14** (16 suite positions, Hash 64
every arm, fresh process per depth, 2026-08-25;
`analysis/step514_shallow_cost.md`). Cumulative nodes to reach each depth,
Basilisk against SF search driving our own evaluator:

| depth | 1 | 2 | 3 | **4** | 6 | 8 | 11 |
|---|---:|---:|---:|---:|---:|---:|---:|
| ratio | 1.50× | 3.12× | 3.41× | **4.38×** | 3.45× | 3.18× | 1.99× |

The excess is not startup overhead: it **rises** to a peak at depth 4 and then
decays, which with our better branching ratio (BAS-D05) describes a search that
pays a large penalty in a narrow band and then grows more slowly. Interior and
quiescence carry the **same** ratio at every depth (4.69×/4.05× at depth 4,
1.95×/2.05× at depth 11) and converge together.

*Conditional lesson.* One cause, not two — a multiplier applied above the
subtree shows up identically in interior and quiescence, and it rules out both
"qsearch is expensive" and "interior pruning is weak" as separate diagnoses.
The target band is **depths 2–6**, where our shallow pruning lives (razoring
`<= 3`; futility, LMP and history pruning `<= 6`). Phase 5 has never tested that
band: clusters 5.4 and 5.6 judged candidates on depth reached at 300,000 nodes,
a metric dominated by deep search that a shallow-band saving barely moves.
BAS-D04's "no depth" verdict on history pruning is consistent with this and is
not contradicted — it was measured against the wrong band.

*Caveat.* Absolute node counts are never comparable across engines; the shape of
the ratio is. The engine-agnostic evidence of a real deficit remains the
equal-time 15.6 against 25.2 plies (BAS-O01/O03).

*Disposition.* Diagnostic complete, no candidate proposed. Whether to open a
cluster against the shallow band is part of the pending budget decision.

**BAS-D05 — consecutive-depth branching; the leading diagnosis is overturned**
(16 suite positions, depths 4–11, **Hash 64 on every arm**, 2026-08-13;
`analysis/manta_import_v1.md`). Method imported from Manta `MAN-S23`: branching
is the ratio between consecutive depths, because a single-depth
`nodes^(1/depth)` estimate folds in the fixed cost of the first plies. Every EBF
figure previously recorded here used that folded estimator.

| | Basilisk | SF search + our eval |
|---|---:|---:|
| b(4–11) aggregate | **1.692** | 1.894 |
| per-position median | 1.699 | 1.899 |
| nodes at depth 4 | 29,482 | 6,732 (**4.38×**) |
| nodes at depth 11 | 1,170,224 | 588,190 (1.99×) |

**Our per-ply growth is BETTER than the reference search's.** The deficit is a
constant factor — 4.4× at depth 4 decaying to 2.0× by depth 11, which is what a
better ratio does to a worse starting point.

*Conditional lesson.* Phase 5 spent three clusters on the premise that our tree
is too wide per ply, and every attempt to cut harder failed (BAS-S13/S14
neutral, BAS-S16 −3.48 Elo, BAS-D04 no depth). Those failures now share one
explanation: **the growth rate was never the deficit**, so cutting harder could
not help. What is deficient is what a shallow subtree costs us. The 12-ply
equal-node gap and the ~9.6-ply equal-time gap (BAS-O01/O03, engine-agnostic)
both stand; only their attribution changes. Absolute node counts are never
comparable across engines, but both count interior and quiescence nodes, so the
ratio is sound in kind.

*Disposition.* Supersedes the EBF framing in BAS-O03/O04. Registered as PLAN
5.14.

**BAS-D04 — history-pruning reachability** (`suite_v1.epd`, depth 12,
2026-08-13; `analysis/cluster56_audit_v1.md`). The live condition compares
`hist_prune_coeff * depth` against a sum of six **bounded** history channels
whose maximum magnitude is **81,920**. At `coeff = 14004` the depth-6 threshold
is 84,024 — **provably unsatisfiable**; depth 5 needs 85% of theoretical maximum
negative on every channel at once. The mechanism is live only at depths 1–2 and
fires **142 times in 5,355,599** tested quiet moves (0.003%). Loosening would
activate a real population — `coeff/2` 95,418, `coeff/4` 234,235, `coeff/8`
467,647 — but paired depth at 300k nodes is flat at every value (−0.019, +0.037,
−0.019).

*Conditional lesson.* A threshold that scales with depth against a signal that
does not is unreachable at the top of its own range; this one was stranded when
`hcefinal` re-scaled the history space. But reviving it is **not** a candidate:
it would prune 4–9% of tested quiets for no depth, and BAS-S16 measured a tree
that shrinks without gaining depth at −3.48 ±3.32 Elo. Move-count pruning
already fires 22.2M times against 15.1M interior nodes, so the quiets history
pruning would catch are largely gone before it is consulted.

*Retry trigger.* Only if move-count pruning is restructured so the surviving
quiet population changes materially, or if a diagnostic shows the pruned moves
carry quality cost rather than node cost. Not on a new coefficient alone.

*Also recorded.* ProbCut's 0.4% share is **correct rarity, not a defect** — it
succeeds 56,311 of 84,469 tries, a 67% hit rate.

*Harness defect found and fixed in the same work.* `print_diag` built its kv
line into `char buf[256]`; the grown line overflowed and `snprintf` truncated
silently, always losing the tail field, so corruption scaled with counter
magnitude. It was caught only because the threshold series has a monotonicity
invariant that made the result visibly impossible. Buffer now 512 with the
probe on its own line.

**BAS-D03 — qsearch share, all three arms** (`suite_v1.epd`, fixed 300,000
nodes, 2026-08-13). Basilisk native **30.8%**; SF search + Basilisk HCE
**36.1%**; SF search + SF HCE **37.0%**. Our qsearch is *smaller* than the
reference's, which spends a larger share of its nodes there while still
reaching 12 more plies. **Qsearch is not a source of wasted width** and the
hypothesis that it might be is closed. Measured by adding a behaviour-neutral
qsearch counter to the vendored Stockfish on a **derived branch `hybrid-diag`**;
the frozen `hybrid` oracle stays at `01df815` with its tournament binary
untouched.

**Differential at equal nodes.** Given the same 300,000-node budget, Basilisk
reaches mean depth **20.80** and the oracle **32.87** — **+12.07 plies** on
identical evaluation. This reproduces BAS-O03's time-based EBF finding under a
node budget, so it is not a throughput artifact in any form.

**What this does not yet say.** The counters localize the width; they do not
prove that reducing more would gain Elo, and they credit no specific change.
Durable lesson 5 cuts both ways — a smaller tree can also be worse. Cluster 5.4
must gate any reduction change on games, and prune recall (would a pruned move
have been best?) is not yet instrumented, so "reduce more" remains a hypothesis
with a mechanism, not a finding.

### Cluster 5.4.3 — reduction hypotheses, all refuted before games

Three attempts to act on the 5.3 inventory's item 1 ("reduction modulation is
nearly inert — raise it"). All were measured on `tools/diag/suite_v1.epd` and
all moved the target metric the **wrong way**, so none reached an SPRT. Target
metric is mean depth at a fixed 300,000-node budget, where the 5.2 baseline is
20.80 against the oracle's 32.87.

| ID | Change | Result | Conditional lesson |
|---|---|---|---|
| BAS-S13 | Fractional history response: `(stat/div)*1024` → `stat*1024/div`, removing the whole-ply quantisation. Motivated by BAS-D02's 16.2% reduce-to-zero rate. | **Worse.** applied 36.1%→32.5%, clamp-to-zero 16.2%→**19.8%**, depth 20.80→20.70. Reverted. | The quantisation is not only a resolution defect, it is also a **threshold**. History *subtracts* from `r` and most moves carry positive history, so a continuous response shaves a little off nearly every reduction, where the integer form shaved a whole ply off only the `|stat| ≥ div` minority. Retry trigger: base and context reductions are materially larger, so there is enough `r` to modulate rather than erase. |
| BAS-S14 | Raise the context magnitudes toward reference scale via the TUNE knobs: `LmrCutNodeAdj` 401→1024/2048, `LmrTtPvAdj` 23→1024. | **Worse or flat.** Mean reduction essentially unmoved (2.354 → 2.229 / 2.338 / 2.294) and non-monotonic in the knob; depth at equal nodes **20.80 → 20.10** at the largest setting. Not adopted. | The reference's magnitudes do not transfer, and this is exactly why PLAN forbids importing constants: they were fitted to a different search. More importantly the response is non-monotonic, which says the aggregate is not a clean lever — changing reductions changes which nodes are eligible, so the ratio and its input move together. |
| BAS-S15 | Hypothesis that the `new_depth - 1` ceiling caps mean reduction, making modulation unable to matter near the leaves. Tested by adding the `lmr_clamped_high` counter. | **Refuted.** The ceiling binds on **4.8% of applied** reductions (145,854 of 3,014,380). | Not the constraint. The counter is retained: it permanently separates "our modulation is too small" from "our modulation cannot matter here", which are opposite repairs and were previously indistinguishable. |

**Disposition.** Cluster 5.4.3 has **no supported candidate**. Nothing was
handed to an SPRT, because our own harness predicts all three arms are worse
than the accepted head — spending games to confirm that would be waste.

Applying PLAN's negative-result triage: preconditions were healthy (BAS-D01
ordering, live histories), the changes were small and self-contained rather
than incomplete clusters, and reference constants were used only as targets to
validate — which is what showed they do not transfer. That leaves reason 4: for
**reduction magnitude specifically**, the mechanism does not transfer to
Basilisk. The 12-ply gap at equal nodes is real and unexplained, but it is not
explained by how much each late move is reduced.

**Where this points.** The untested lever in this cluster with a large measured
population is 5.4.4: check extensions fire on **15.84% of interior nodes**, each
adding a ply, and those same moves are barred from reduction. That is a direct,
unconditional depth cost that no reduction knob can reach. Cluster 5.5's
qsearch is the other candidate — qsearch is 8.09M of 23.2M total nodes.

### Cluster 5.4.4 — check-move depth policy, REJECTED

| ID | BAS-S16 |
|---|---|
| **Verdict** | **Rejected.** SPRT `[0,3]` nElo accepted H0 at the bound: LLR **−2.95** against −2.94, at **17,058 games** in 3h09. Reverted — the switches were never committed active, so the accepted head is untouched at bench 11,941,440. |
| **Result** | Elo **−3.48 ±3.32**, nElo **−5.47 ±5.21**, LOS **2.00%**. W 4,351 / L 4,522 / D 8,185, 49.50%. Ptnml(0-2) [377, 2074, 3767, 1965, 346], pairs ratio 0.94, WL/DD 0.82, draw ratio 44.17%. |
| **Candidate** | `CheckExtPathCap=2` + `LmrAllowCheck=1`, adjudicated jointly per PLAN 5.4.4. Two binaries from revision `ce572a7` differing only in those defaults; candidate bench 8,611,045 against baseline 11,941,440. |
| **Methodology note — CORRECTED 2026-08-13** | The two-binary design was chosen because `sprt.ps1` was believed to lack per-arm UCI options, and one PGO profile's worth of variance between arms was recorded as its unavoidable cost. **That was wrong.** `sprt.ps1` has had `-OptionsA`/`-OptionsB` since before this phase (lines 221–222), documented as letting "a single binary be A/B-tested on a UCI knob without a rebuild"; a grep for `[string]` did not match `[string[]]`. The two binaries and their variance were unnecessary. The verdict is unaffected. Future single-binary gates use `-OptionsA`/`-OptionsB`. |
| **Conditions** | `tools/sprt.ps1` on fastchess 1.8.0, `3+0.03`, 1T, Hash 64, paired `UHO_Lichess_4852_v1.epd`, concurrency 14 pinned one core per game, standard `strength-v1` adjudication (valid — both arms share Basilisk's evaluator). |
| **Triage** | PLAN's ordered check: (1) preconditions were healthy — BAS-D01 ordering, five live history channels; (2) the change was dependency-complete, both halves of the check-depth question moved together as 5.4.4 requires; (3) no reference constant was imported — `cap=2` came from our own sweep. So **reason 4**: for Basilisk, this mechanism does not transfer. |
| **Conditional lesson** | Trading tree width for depth **loses** here, and durable lesson 5 lands on its other side: our width is buying something real. Note the magnitude — a **28% smaller tree** cost only **−3.48 Elo**, so the depth-for-tactics trade is nearly balanced but sits on the wrong side of zero. The pre-registered rule forbids re-running with a softer cap as though it were the same experiment; a milder setting is a new hypothesis needing a new ID and a new reason to believe it. |
| **Retry trigger** | Only if a later cluster changes the *information* pruning decisions are made on — static-eval separation, TT provenance or qsearch quality. Not on a different cap value. |
| **Not ablated** | The stop rule allowed isolating either switch ("if either"). Declined: the components' own priors were smaller than the bundle's (cap alone +0.411 ply, `LmrAllowCheck` alone +0.140), so each would cost ~3h to measure a likely smaller loss. Recorded rather than silently skipped. |

**Cluster 5.4 is now exhausted and the re-audit trigger has fired.** Its two
hypotheses both failed: 5.4.3 reduction magnitude (BAS-S13/S14/S15, refuted on
the harness before games) and 5.4.4 check-move depth (BAS-S16, refuted by
games). Ordering was already equivalent (BAS-D01). PLAN cluster discipline
requires stopping to re-audit rather than continuing by sunk cost.

**What the cluster established, taken together.** The 12-ply gap at equal nodes
is real, but it is not reachable by pruning harder on the same decisions —
every attempt to narrow the tree either failed to move it or measured worse.
The reference is narrow *and* strong, so its narrowness cannot be aggression;
it must come from making better-informed decisions, which lets it prune safely
where we cannot.

That is a hypothesis, not a result, but it is coherent with BAS-O02: our HCE
measured **−232.8 Elo** against the reference's under an identical search. An
evaluator that is materially worse produces pruning and reduction decisions
that are materially less trustworthy, and a search that cannot trust its own
margins must stay wide to be safe. If that is right, width is a *symptom* and
cluster 5.5 (static eval / TT / qsearch separation) and the 5.9 HCE track carry
the value that 5.4 did not.

### Accepted or retained### Accepted or retained

| ID | Experiment and conditions | Result / disposition | Conditional lesson | Source |
|---|---|---|---|---|
| BAS-S01 | TT-bound-aware pruning evaluation used a proving TT bound for RFP, razoring, NMP, futility and qsearch stand pat while preserving raw/corrected eval for improving and correction updates. | **Accepted, +7.18 Elo** in the 1.8.0 search state. | In that state, separating pruning evidence from training evidence improved their cooperation. Preserve this separation in the 5.1 shadow census and any 8.3 result-evidence redesign. | `CHANGELOG.md` 1.8.0 |
| BAS-S02 | A jointly exposed search bundle after several knobs had landed inert at defaults. | **Accepted, +9.14 Elo** in the 1.8.0 campaign. | Mechanisms that are neutral alone may become measurable after related consumers exist and are jointly fitted. This does not justify bundling without diagnostics and ablations. | legacy plan at `8dc0a24^` |
| BAS-S03 | Exact/PV-node best-move quiet-history reward, without sibling maluses. | **Accepted, +4.90 Elo** before harness re-audit; paired with surprise history it reverified at +3.06 ± 4.35. | In that search state, result semantics mattered to history training. Plan 5.1 shadows attribution; any consumer redesign belongs to 8.3. | `CHANGELOG.md` 1.9.0 |
| BAS-S04 | Static-eval-surprise scaling of history. | **Accepted, +2.50 Elo** pre-affinity; retained after the paired fixed-harness audit. | Eval surprise was a useful confidence signal in that baseline, but must be regenerated when evaluation scale changes. | `CHANGELOG.md` 1.9.0 |
| BAS-S05 | Denser 32-byte partial-key TT cluster and replacement changes. | **Accepted, +4.27 Elo** pre-affinity; retained with structural merit, not re-run on the corrected harness. | More effective TT capacity appeared useful at the tested hash/TC; Plan 8.3/9.1 must separate semantic consumers, density, replacement and indexing before generalizing. | `CHANGELOG.md` 1.9.0 |
| BAS-S06 | SEE excluded absolutely pinned attackers through a shared exchange-occupancy pin scan. | **Retained for correctness; +0.65 Elo claim unverified** after harness repair. | Correct exchange legality is a valid prerequisite even when the isolated strength effect is below resolution. | `CHANGELOG.md` 1.9.0 |

### Rejected, neutral or deferred

| ID | Experiment and conditions | Result / disposition | Conditional lesson and retry trigger | Source |
|---|---|---|---|---|
| BAS-S07 | Added a 6-ply continuation-history channel to the then-current history stack. | **Rejected, −7.70 Elo.** | In that stack, the extra channel likely duplicated or distorted existing evidence. Retry only after history ownership/indexing changes and with a post-fit ablation. | legacy plan at `8dc0a24^` |
| BAS-S08 | Blanket removal of the check extension against the `hcefinal`-tuned head. | **Rejected, −10.17 ± 6.52 over 4,682 games; reverted.** | The result may reflect consumers tuned around the extension, not a universal need for it. Retry only in the post-NNUE joint architecture/fit at 8.3; Rarog's opposite result is a prior, not a verdict. | `CHANGELOG.md` 1.9.1 |
| BAS-S09 | Corrected an LMR reduction gate that consumed `gives_check` after making the move. | **Standalone rejected, about −20 Elo; reverted/deferred.** | The incorrect path may have acted as an aggressive-reduction heuristic and the surrounding constants were fitted with it. Reintroduce only inside unified pre-move evidence plus the post-NNUE joint fit in Plan 8.3. | `CHANGELOG.md` 1.9.1; `analysis/search_analysis.md` |
| BAS-S10 | Exact-node history update reused cutoff-style sibling maluses. | **Rejected, −84.21 ± 18.85 over 652 games.** | Under this history design, sibling malus semantics did not transfer from cutoffs to exact nodes. Any retry must distinguish reward-only exact evidence from cutoff evidence. | legacy plan at `8dc0a24^` |
| BAS-S11 | Cutoff-count LMR was considered after Rarog tested a full LMR-family retune. | **Not implemented standalone; imported Rarog result was −7.78 ± 8.00.** | `cutoffCnt` is not a mandatory improvement. It may be tested only as a diagnosed coordinate within Plan 8.3, followed by ablation. | legacy plan at `8dc0a24^`; Rarog `CHANGELOG.md` |
| BAS-S12 | Cuckoo repetition and post-LMR history candidates were tested on the old unpinned harness. | **Rejected/closed for the old state; measurements carried harness uncertainty.** | The old conditions leave residual uncertainty, but repeated retries have low expected value. Reopen in 8.3 only if the 5.1 shadow census identifies the exact missing consumer and pre-registers one terminal test. | legacy plan at `8dc0a24^` |

## 4. Root search, time management and SMP

| ID | Experiment and conditions | Result / disposition | Conditional lesson and retry trigger | Source |
|---|---|---|---|---|
| BAS-R01 | Start the move clock at receipt of `go`, including GUI-to-worker dispatch latency. | **Retained non-regression, +2.95 ± 6.74 Elo** versus 1.7.0. | At bullet TC with concurrent engines, dispatch latency was material to safety. Recheck on materially different UCI scheduling architectures. | `CHANGELOG.md` 1.8.0 |
| BAS-R02 | SPSA of nine time-management constants on the old root model. | **Neutral, +0.88 ± 4.03 over 12,262 games; reverted.** | The tested constants appeared near a local ceiling in that root model. A retry is justified only after post-NNUE Plan 8.3 changes the confidence inputs. | `CHANGELOG.md` 1.8.0 |
| BAS-R03 | Best-move-instability time extension using a decaying root-change signal. | **Accepted; +10.79 pre-affinity, +6.46 ± 4.12 on fixed harness.** | Root instability was useful at the tested TC/model. Preserve the signal but recalibrate it when root confidence or score scale changes. | `CHANGELOG.md` 1.9.0 |
| BAS-R04 | Phase-9 helper clock/node/thread safety bundle at 4T. | **Accepted, +30.42 ± 8.77 at 4T, zero forfeits in 2,450 games.** Boundary H2H was +11 1T fast, +14 4T fast and +26 4T `10+0.1`, all with wide intervals. | Most measured value belonged to the combined deployed SMP condition, not a single isolated mechanism. Keep 1T and 4T claims separate and require topology/hash manifests. | `CHANGELOG.md` 1.9.2 |
| BAS-R05 | Shared-node batching/scaling change. | **Retained:** neutral at 1T/4T and +12.8% in an indicative 16T throughput check; no Elo claim. | A scaling fix can be useful beyond the tested release topology without proving strength. Re-measure under Phase 9 high-thread/NUMA work. | `CHANGELOG.md` 1.9.2 |
| BAS-R06 | Extra helper coordination, aspiration sharing, diversification, TT variants and HCE refit during Phase 9. | **Rejected or removed; not shipped.** | Additional shared signals can create correlated work or overwrite useful diversity. Retry only when diagnostics identify a specific scaling bottleneck. | `CHANGELOG.md` 1.9.2; `analysis/mt_baseline_9.3.md` |

## 5. Evaluation and data experiments

New HCE strength work is frozen. These rows remain relevant to NNUE data,
teacher and measurement design; they do not authorize Plan-10 HCE work unless
NNUE is explicitly abandoned.

**BAS-E07 — HCE maturity audit, 2026-08-13** (`analysis/hce_maturity_v1.md`).
Term coverage against Stockfish `9587eeeb` is near-complete: only `BadOutpost`,
`BishopXRayPawns`, `LongDiagonalBishop`, `KnightOnQueen`, `SliderOnQueen` and
`TrappedRook` are genuinely absent, and every term that once shipped seeded-inert
is now non-zero. Both evaluators over `suite_v1.epd` (103 scored positions):
correlation **r = 0.790** (r² 0.624), regression slope SF-on-Basilisk **1.749**,
std-dev 8.67 against 19.19 pawns, median absolute difference 0.94 pawns, and a
**17% sign-disagreement rate** — one position in six where the two evaluators
disagree about which side stands better.

*Conditional lesson.* The −232.8 Elo evaluation gap (BAS-O02) is **not** missing
features; six minor terms cannot carry it. It is the values assigned to features
we already have, and our fitting lever is exhausted — cycle 6 washed at
+1.37 ±5.21 over 8,100 games and holdout MSE never predicted Elo. The reference's
weights were fitted by game outcome at fishtest scale; ours by a static
objective, which cannot price a term whose value is realised through search.
That is a difference in method, not effort.

*Consequence.* Phase 5.9 is scoped as structural convergence with constant
refitting barred, so as written it cannot close a gap that is almost entirely
constants. Its realistic yield is the six terms above. Recorded so that a small
5.9 result is not later misread as failure, and so the barred constant refit is
not reopened on the strength of it. The evaluation gap is NNUE's to close.

*Retry trigger.* Only if a game-outcome fitting capability at scale becomes
available, which NNUE supersedes anyway.

| ID | Experiment and conditions | Result / disposition | Conditional lesson and retry trigger | Source |
|---|---|---|---|---|
| BAS-E01 | Staged Texel scalar/structural fits against successive accepted heads. | Material +29.05, mobility +8.77, passed pawns +16.57 and pawn structure +30.74 Elo; rook terms ended +3.13 ± 4.74 and were reverted. | Data-fit movement can transfer unevenly by feature group. Each stage still needs games; a lower fitting loss alone is not acceptance. | `CHANGELOG.md` 1.6.0 |
| BAS-E02 | Larger staged HCE campaign with king safety, threats, positional, imbalance, mobility/PST and joint polish. | Each stage accepted; cumulative +280.74 versus the early phase baseline, with smaller external/longer-TC transfer. | Sequential fitting worked strongly on that corpus, but self-play stage deltas should not be added or projected directly to external ratings. | `CHANGELOG.md` 1.7.0 |
| BAS-E03 | Stockfish-at-60k quiet-position distillation, with material scale pinned. | **Accepted, +6.75 Elo.** | Teacher labels helped this stale Basilisk HCE/corpus combination. Rarog's −17.11 result shows that teacher quality or lower fit loss alone does not guarantee playing gain. | `CHANGELOG.md` 1.8.0; Rarog legacy plan |
| BAS-E04 | Successive on-policy self-play refresh cycles with phase balancing and joint linear/king-safety refit. | **Accepted:** +21.02, +19.51, +18.29 and +15.32 Elo; the next cycle washed at +1.37 ± 5.21 and was discarded. | On-policy refresh paid repeatedly until this line saturated. Retry only after a material policy, representation or evaluator change—not by extending the same HCE loop. | `CHANGELOG.md` 1.8.0; legacy plan at `8dc0a24^` |
| BAS-E05 | `hcefinal` joint history/eval SPSA. | **Accepted, +35.94 ± 9.42.** | A large joint fit helped that coupled HCE/search state; it also means standalone consumer changes can be de-tuned. The exact constants are not priors for NNUE scale. | `CHANGELOG.md` 1.9.0 |
| BAS-E06 | Five-phase corpus/tooling upgrade and attempted Phase-9 HCE refit. | Tooling retained; refit stopped at **+1.52 ± 5.77 Elo** and not shipped. | Better tooling is separable from a candidate's strength. With HCE frozen, reuse the phase-balanced data concepts in NNUE work rather than retrying this fit. | `CHANGELOG.md` 1.9.2 |

## 6. Throughput, build and platforms

| ID | Experiment and conditions | Result / disposition | Conditional lesson and retry trigger | Source |
|---|---|---|---|---|
| BAS-P01 | Phase-8.7 profile-guided, bench-identical optimization wave. | **Accepted, +4.34% pooled-PGO NPS; batch result +8.69 ± 6.63 Elo at `3+0.03`.** | On the tested x64 PEXT path, several small hot-path gains compounded and translated at roughly 2 Elo per 1% NPS, with a wide CI. Do not assume that ratio at LTC or another ISA. | `CHANGELOG.md` 1.9.1 |
| BAS-P02 | Continuation-history row/index hoists in move scoring. | **Accepted, +3.03% NPS** within BAS-P01. | Hoisting repeated table-address work paid in this profile. Re-profile after layout or NNUE changes before copying the optimization. | `CHANGELOG.md` 1.9.1 |
| BAS-P03 | SEE memoization/classification reuse. | **Accepted, about +0.36% NPS** within BAS-P01. | Reusing already-required tactical evidence can pay when coverage is proven; verify no fallback or stale-state path. | `CHANGELOG.md` 1.9.1 |
| BAS-P04 | Per-node `CheckInfo`, check-hinted `make_move`, and pin-sharing candidates. | **Rejected:** roughly −1.8% to −2.7% for check caching, −0.16% for pin sharing. | Caching is not free when computation is lazy or consumers are sparse. Retry only after profile evidence shows changed reuse. | `CHANGELOG.md` 1.9.1 |
| BAS-P05 | Pawn-cache resize at observed 89–99% hit rate. | **Closed as dead.** | Under those hash sizes/workloads, capacity was not the bottleneck. Reopen only with measured collision/miss pressure in a new evaluator. | `CHANGELOG.md` 1.9.1 |
| BAS-P06 | Enrich PGO training beyond the expanded bench suite. | **Rejected/retired; bench-only PGO retained.** Earlier depth-8/9 EPD profiles increased build time without meaningful speed gain. | A larger training workload need not improve the deployed profile. Reopen only after coverage/profile evidence shows a material blind spot. | `CHANGELOG.md` 1.9.1, 1.4.9 |
| BAS-P07 | `origin/arm_fix` aligned the TT allocation to 64-byte cache lines. | **Rejected hypothesis:** no AArch64 evidence; a 32-byte-aligned 32-byte cluster cannot straddle a 128-byte boundary, and Rarog found no material Apple 4T false-sharing case. | Close the wrapper in Plan 5.3. Retain target-native cache/atomic measurement, ISA contracts and emitted-prefetch verification rather than benchmarking the invalid geometry. | `PLAN.md` 5.3; branch `origin/arm_fix` |
| BAS-P08 | Windows ARM64 Clang PGO selected `llvm-profdata` from global `PATH`. | **Correctness/tooling repair in 1.9.3; search unchanged, bench 11,941,440.** | Tool identity is part of reproducibility. Validate compiler/profdata compatibility for every asset rather than treating a successful compile as a PGO proof. | `CHANGELOG.md` 1.9.3 |

## 7. Correctness and protocol lessons

| ID | Experiment or failure mode | Disposition | Conditional lesson / coverage | Source |
|---|---|---|---|---|
| BAS-C01 | Brittle fixed-depth endgame conversion canaries rejected benign eval/search/TT changes. | Replaced with eval recognition, tolerant conversion floors and near-mate checks. | A correctness test should assert the invariant, not one search trajectory. Keep tactical strength diagnostics separate. | legacy plan at `8dc0a24^` |
| BAS-C02 | Rule-50/mate precedence, null-move halfmove preservation and legal-EP hashing defects. | Fixed and covered by board/search invariants. | Draw and hash semantics influence TT, repetition and pruning together; retain deterministic coverage before strength testing. | `CHANGELOG.md` 1.9.0 |
| BAS-C03 | TT, parser, PV and SMP could expose stale or illegal moves. | Pseudo-legality validation, PV truncation, differential perft, fuzzing and sanitizers retained. | Shared or aliased evidence is untrusted at consumption boundaries. Correctness gates remain necessary even if normal games rarely hit the path. | `CHANGELOG.md` 1.4.x–1.9.0 |
| BAS-C04 | Helper/private clocks and fixed-depth inheritance did not match deployed multi-thread semantics. | Repaired in 1.9.2 with aggregate accounting and zero-forfeit tests. | UCI timing and SMP cannot be validated only at 1T. Repeat the full matrix after root, stop or pool changes. | `CHANGELOG.md` 1.9.2 |

## 8. Cross-engine evidence imported from Rarog

These are ideas, warnings or ordering priors already incorporated where useful
in Basilisk's forward plan. Listing an item here does not by itself create a
roadmap item.

**BAS-X09/X10 are the exception.** They did not refine an existing plan item —
they changed Phase 5's purpose from bounded pre-NNUE hardening to a search and
evaluation acceleration program. They remain *imported priors*: they size and
order Basilisk's work and can never accept a Basilisk change.

Note what they do **not** license. They establish that a mature search is worth
roughly 200 Elo to us and that a mature HCE is worth more again. They say
nothing about which specific mechanism earns it, and they are not permission to
transcribe Stockfish. Basilisk remains an independent engine; the reference is
an idea source and an oracle. See PLAN's Independence contract.

| ID | Rarog evidence | Possible Basilisk implication | Existing PLAN coverage |
|---|---|---|---|
| BAS-X01 | Check-extension removal was +30.75 Elo in Rarog but −10.17 ± 6.52 in Basilisk. | Search mechanisms can be jointly de-tuned; copy the experiment design, not the verdict. | 8.3 |
| BAS-X02 | Stockfish distillation improved holdout loss by 4.9% yet lost −17.11 Elo in Rarog, while Basilisk gained +6.75. | Teacher transfer depends on corpus, representation, scale and current policy; games remain the gate. | 6.2–6.4, 7.0–7.2 |
| BAS-X03 | Rarog's full `cutoffCnt`/LMR-family candidate lost −7.78 ± 8.00 despite its tuning trajectory. | Tuner success can select a self-play-local optimum. Treat `cutoffCnt` as an optional diagnosed coordinate, not required parity. | 8.3 |
| BAS-X04 | Rarog gained +22.13 from history bonus/malus work and +6.01 from a broader history bundle. | Result-source attribution and consumer normalization may unlock history value, but Basilisk's 6-ply channel already showed duplication risk. | 5.2 diagnostics, then the 5.4 history cluster; residue to 8.3 |
| BAS-X05 | Rarog's accepted SMP rework was +102.78 ± 16.38 at 4T. Historical Rarog 2.3.0 minus Basilisk 1.9.1 pool Elo was 1T STC −55 ± 21, 1T LTC −38 ± 27, 4T STC −32 ± 50 and 4T LTC **+34 ± 24**. Rarog later showed ~12.3× 16T NPS but no fixed-time depth gain. | The old matrix suggests a thread × TC crossover, not general Basilisk LTC inferiority: Basilisk led both 1T cells. Since Basilisk subsequently changed SMP, repeat the current-version 2×2 with uncertainty and pair it with internal time-to-depth diagnostics before assigning the cause or changing code. | 5.12, 9.0 |
| BAS-X06 | Rarog's bench-identical speed wave gained +10.35% NPS and +20.31 ± 7.13 Elo at STC. | It corroborates that wall-clock speed can convert to strength near this host/TC, while leaving LTC and ISA transfer unknown. | 5.11, 5.12, 9.1 |
| BAS-X07 | Rarog `arm_fix` adds an AArch64 prefetch path and a runtime-hoist idea; x64 evidence was bundled and ARM untested. | Basilisk already uses compiler prefetch on ARM. Verify emitted code; do not copy the Rust implementation, wrapper or constants. | 5.11 |
| BAS-X08 | Rarog's parity audit emphasizes shared `MoveEvidence`, prospective depth and correction attribution. | These abstractions may reduce contradictory consumers in Basilisk if the telemetry confirms the same failure modes. | 5.2 diagnostics, 5.10 safety, then the 5.4-5.8 clusters; residue to 8.3 |
| BAS-X09 | Rarog RAR-O02 (no adjudication, 1,238 games, 982 natural mates, `3+0.03`, 1T): Stockfish `9587eeeb` search driving Rarog's 2.3.2 HCE beat **Basilisk 1.9.3 by ~+196.5 Elo** while running 1.5M NPS against Basilisk's 2.5M. Basilisk − Rarog was +30.4 in the same pool. | Basilisk's dominant measurable deficit is search coordination, not evaluation capacity or NNUE absence, and it is not a throughput artifact. Logistic point estimates from a stopped run — they size a target and are never added or quoted as a release claim. Basilisk-specific magnitude is unknown until 5.1 measures our own evaluator under the same search. | **5.1** oracle, then 5.2–5.8 convergence |
| BAS-X10 | In the same run, the exact-revision Stockfish HCE beat that hybrid by **~+328.6 Elo** with the search held identical. RAR-O01, the adjudicated variant, inflated the sibling contrast from +196.5 to +270.9. | A second large deficit exists in HCE structural coverage, isolated cleanly because only the evaluator changed. Justifies unfreezing HCE for structural convergence (5.9) while keeping the constant-refit freeze. The adjudication delta is a standing warning: cross-evaluator cohorts run with adjudication off. | **5.9**; adjudication rule in PLAN durable lesson 14 |

Apart from the search-oracle result, the cross-review found no additional
high-value Rarog item missing from the current Basilisk plan. The remaining
items are already covered, contradicted by local evidence, or deliberately
postponed to the NNUE/scaling phases.

## 8b. Cross-engine evidence imported from Manta

Manta is a Zig engine by the same maintainer, developed against the same
reference snapshot. These are **imported priors**: they order and warn, and they
never accept a Basilisk change.

| ID | Manta evidence | Basilisk implication | Coverage |
|---|---|---|---|
| BAS-X11 | `MAN-E05` (endgame conversion grading, −16.32 Elo) and `MAN-E07` (nonlinear material imbalance, −7.00) — two faithful reference-family evaluation concepts with hand-reasoned coefficients, about **−23 Elo between them**. Manta's conclusion: adopting reference concepts with reasoned constants "reproduces its structure without its calibration". | A direct warning about **5.9 as currently scoped** — six absent terms, each hand-set and individually gated, is precisely the design that lost twice there. Manta's answer was to land structure on deterministic evidence and promote the block through **one** joint fit and **one** gate. Our HCE freeze bars that, which is further support for BAS-E07's conclusion that the evaluation gap is NNUE's to close rather than 5.9's. | 5.9 |
| BAS-X12 | `MAN-S18` and `MAN-S20`: two selectivity clusters, 12,000 games each, that **grew** the tree (755,581→772,203 and 744,899→761,703 nodes) while losing. The direction of travel was toward more protective search while the tree was already too wide. | Converges with BAS-S13, where a more principled continuous history response also made reductions *smaller*. Selectivity work drifts toward protection unless a tree-shape measurement is checked at design time, not after. | 5.4, 5.6 |
| BAS-X13 | `MAN-S23`: a registered pre-gate branching filter refuted a candidate "in minutes of arithmetic" where the two clusters above had cost 12,000 games each. Its `b(4-12)` measure was then found to have been decided by **one position of forty** exploding 41.5% at the endpoint depth. | Independent validation of the harness-before-SPRT discipline this phase used to reject BAS-S13/S14/S15 and BAS-D04 without spending games. The endpoint-measure trap is adopted directly: `tools/diag/branching.py` reports per-position ratios and a median beside the aggregate. | 5.2, 5.14 |
| BAS-X14 | Manta's fit catalogue classifies coefficients **free / fixed / excluded**, excluding nonlinear king danger, capped winnability and truncated tables because "a linear count model would misrepresent their caps, squares, per-application truncation or dispatch". | Independent support for BAS-E07: our king-safety funnel is exactly that class of term, so a static linear objective cannot price it. Reinforces that our fitting method — not our effort — is what the evaluation gap reflects. | 5.9, 7.x |

Manta also supplied the **method** that produced BAS-D05, which is recorded
there rather than here because it is our own measurement.

## 9. Open retry map

| Prior IDs | Retry condition | PLAN destination |
|---|---|---|
| BAS-S08, BAS-S09, BAS-S11 | Unified pre-move evidence and prospective-depth model implemented; consumers included in the single joint fit; post-fit ablations registered. | 5.4 and 5.6 clusters; residue to 8.3 |
| BAS-S07, BAS-S10, BAS-S12 | Diagnostics show a distinct source/consumer gap that the existing history tables cannot represent. | 5.2, then the 5.4 cluster; residue to 8.3 |
| BAS-R02, BAS-R03 | Root-confidence inputs or NNUE score scale materially change. | 7.6, 8.3 |
| BAS-P04, BAS-P05, BAS-P06 | A new profile demonstrates changed reuse, cache pressure or PGO coverage. | 9.1 |
| BAS-P07, BAS-X07 | Production ARM64 artifacts show missing prefetch or measured hot-state contention; isolate one valid variant per target-native A/B. | 5.11 |
| BAS-E03, BAS-E04, BAS-E06 | NNUE data/teacher experiment, not another HCE **constant** fit; frozen teacher and holdout are available. Structural HCE coverage is a different question owned by 5.9. | 5.9, 6.2–7.2 |

Anything not meeting its trigger stays closed. A retry is a new experiment with
a new ID and manifest; it does not overwrite the historical row.

## 10. Template for a new experiment

```markdown
### BAS-<area><number> — <short name>

- Date / owner:
- Baseline SHA / candidate SHA / dirty-diff hash:
- Hypothesis and interacting consumers:
- Registered gate and stop rule:
- Build: compiler, flags, PGO manifest, binary hashes:
- Games: book/hash, TC, threads, hash, concurrency, affinity, adjudication:
- Result: games, W-D-L, Elo/nElo and CI, LLR:
- Diagnostics: nodes, EBF, NPS, depth, counters, suites (not the verdict):
- Disposition: accepted / retained / rejected / neutral / observation:
- Conditional lesson:
- Retry trigger or `closed`:
- Artifacts / commits:
```
