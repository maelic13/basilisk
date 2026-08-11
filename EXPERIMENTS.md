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

### Accepted or retained

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
in Basilisk's forward plan. No additional roadmap item is created merely by
listing them here.

| ID | Rarog evidence | Possible Basilisk implication | Existing PLAN coverage |
|---|---|---|---|
| BAS-X01 | Check-extension removal was +30.75 Elo in Rarog but −10.17 ± 6.52 in Basilisk. | Search mechanisms can be jointly de-tuned; copy the experiment design, not the verdict. | 8.3 |
| BAS-X02 | Stockfish distillation improved holdout loss by 4.9% yet lost −17.11 Elo in Rarog, while Basilisk gained +6.75. | Teacher transfer depends on corpus, representation, scale and current policy; games remain the gate. | 6.2–6.4, 7.0–7.2 |
| BAS-X03 | Rarog's full `cutoffCnt`/LMR-family candidate lost −7.78 ± 8.00 despite its tuning trajectory. | Tuner success can select a self-play-local optimum. Treat `cutoffCnt` as an optional diagnosed coordinate, not required parity. | 8.3 |
| BAS-X04 | Rarog gained +22.13 from history bonus/malus work and +6.01 from a broader history bundle. | Result-source attribution and consumer normalization may unlock history value, but Basilisk's 6-ply channel already showed duplication risk. | 5.1 shadow, then 8.3 |
| BAS-X05 | Rarog's accepted SMP rework was +102.78 ± 16.38 at 4T, much larger than Basilisk's Phase-9 result. | The gap suggests different baseline defects, not a transferable Elo budget. Rarog later showed ~12.3× 16T NPS but no fixed-time depth gain; compare throughput, time-to-depth, root ownership and TT interaction before changing code. | 5.4, 9.0 |
| BAS-X06 | Rarog's bench-identical speed wave gained +10.35% NPS and +20.31 ± 7.13 Elo at STC. | It corroborates that wall-clock speed can convert to strength near this host/TC, while leaving LTC and ISA transfer unknown. | 5.3, 5.4, 9.1 |
| BAS-X07 | Rarog `arm_fix` adds an AArch64 prefetch path and a runtime-hoist idea; x64 evidence was bundled and ARM untested. | Basilisk already uses compiler prefetch on ARM. Verify emitted code; do not copy the Rust implementation, wrapper or constants. | 5.3 |
| BAS-X08 | Rarog's parity audit emphasizes shared `MoveEvidence`, prospective depth and correction attribution. | These abstractions may reduce contradictory consumers in Basilisk if the telemetry confirms the same failure modes. | 5.1 shadow, 5.2 safety, then 8.3 |

The cross-review found no additional high-value Rarog item missing from the
current Basilisk plan. Items above are already covered, contradicted by local
evidence, or deliberately postponed to the NNUE/scaling phases.

## 9. Open retry map

| Prior IDs | Retry condition | PLAN destination |
|---|---|---|
| BAS-S08, BAS-S09, BAS-S11 | Unified pre-move evidence and prospective-depth model implemented; consumers included in the single joint fit; post-fit ablations registered. | 5.1 shadow, then 8.3 |
| BAS-S07, BAS-S10, BAS-S12 | Diagnostics show a distinct source/consumer gap that the existing history tables cannot represent. | 5.1 shadow, then 8.3 |
| BAS-R02, BAS-R03 | Root-confidence inputs or NNUE score scale materially change. | 7.6, 8.3 |
| BAS-P04, BAS-P05, BAS-P06 | A new profile demonstrates changed reuse, cache pressure or PGO coverage. | 9.1 |
| BAS-P07, BAS-X07 | Production ARM64 artifacts show missing prefetch or measured hot-state contention; isolate one valid variant per target-native A/B. | 5.3 |
| BAS-E03, BAS-E04, BAS-E06 | NNUE data/teacher experiment, not another HCE fit; frozen teacher and holdout are available. | 6.2–7.2 |

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
