# Basilisk experiment ledger

This is the index of the maintainer record of measured experiments and the
lessons that may inform later work. Each entry is its own file,
[`experiments/<ID>.md`](experiments/), headed `# <ID>`; this file keeps the
ledger's structure, prose and retry map, and one line per entry with a link,
a short title and a disposition word taken from the entry. The entry is the
record; an index line is only a pointer, and a `BAS-*` ID resolves in its file.
The split was made on 2026-10-07 (PLAN A.8.5) and rebuilt the former single
file byte for byte before it was written. It is not a roadmap: [`PLAN.md`](PLAN.md) owns what
will be done and in what order. [`CHANGELOG.md`](../CHANGELOG.md) remains the
user-facing release record.

Numbered references in this ledger are historical identifiers. The current
roadmap uses lettered phases (`A.2.1`, `B.3`); [`HISTORY.md`](HISTORY.md)
resolves every retired number to its archived roadmap and maps retired open
leaves to their current identifiers. Rows from BAS-X30 on cite the lettered
roadmap. Rarog identifiers (`RAR-*`) and Rarog paths cited anywhere in this
ledger resolve in the pinned snapshot `docs/reference/rarog/` (Rarog
`015bccae`), or for BAS-X35 in `docs/reference/rarog-2026-10-06/`;
`docs/reference/README.md` says how. Do not rewrite measured records merely because the roadmap was
reordered; the retry map in section 9 is live routing and carries current
destinations.

Three identifier collisions predate this contract and are preserved rather
than silently renumbered: `BAS-E08` names both the frozen Syzygy truth baseline
and the 5.9.4 Texel refit; `BAS-X08` both the Windows include-ownership check
and a Rarog parity-audit prior; `BAS-X11` both the 12,000-game current-standing
observation and a Manta HCE prior. Each file holds both records, in ledger
order; cite them by ID plus short title or source. A new ID must be unused: the
file name is the ID, and `check_roadmap.py` fails when the index and the entry
files disagree (a link without a file, a file without its index line, an extra
index line, a heading that does not name the file's ID, or an entry written
inline in this file).

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
  - [Search programme investigation (B.0)](#search-programme-investigation-b0)
  - [Accepted or retained](#accepted-or-retained)
  - [Rejected, neutral or deferred](#rejected-neutral-or-deferred)
- [4. Root search, time management and SMP](#4-root-search-time-management-and-smp)
- [5. Evaluation and data experiments](#5-evaluation-and-data-experiments)
- [6. Throughput, build and platforms](#6-throughput-build-and-platforms)
- [7. Correctness and protocol lessons](#7-correctness-and-protocol-lessons)
- [8. Cross-engine evidence imported from Rarog](#8-cross-engine-evidence-imported-from-rarog)
- [8b. Cross-engine evidence imported from Manta](#8b-cross-engine-evidence-imported-from-manta)
- [9. Open retry map](#9-open-retry-map)
- [10. Prediction calibration review](#10-prediction-calibration-review)
- [11. Template for a new experiment](#11-template-for-a-new-experiment)

## 1. How to use this ledger

Search the contents by subsystem before proposing a mechanism, tune or retry,
then open the entries the index points to.
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

Register the entry before result exposure, then update the same entry when the
experiment reaches a verdict. The prospective section is frozen at exposure;
only an explicitly labelled clerical correction may alter it. Historical
entries without preregistration remain without it: never manufacture a
prediction after the fact.

Record baseline/candidate/dirty-diff and binary/compiler/PGO identity; research
question, mechanism, interacting consumers and competing hypotheses; the
cheapest prior falsifier and whether implementation remained justified; full
book/corpus/split/seed/tablebase/TC/thread/hash/concurrency/affinity/
adjudication provenance; and a registered gate/stop rule.

Before exposure, freeze expected diagnostic movement, a defensible Elo
sign/range (or `not defensible`), probability the candidate is positive/useful,
confidence, most likely failure mode and falsification criteria. After exposure,
record diagnostics separately from the deciding result, disposition, prediction
calibration, postmortem, conditional lesson, objective retry trigger and
artifacts/commits.

Use dispositions `accepted`, `retained`, `rejected`, `neutral/inconclusive`,
`observation`, `no-change` or `deferred`. Useful calibration categories include
search/selectivity, evaluation/HCE, endgames, TT/cache, SMP/time,
board/performance, tooling/instrumentation, data/tuning and NNUE.

Use cautious language: “under these conditions this suggests …”, not “feature
X is good/bad”. If conditions or artifacts are unknown, say so.

## 2. Measurement, harness and tuning

| ID | Title | Disposition |
|---|---|---|
| [BAS-M01](experiments/BAS-M01.md) | Historical unpinned fastchess runs were compared with fixed explicit physical-core placement on | Real scheduler offsets up to about ±10 Elo/run |
| [BAS-M02](experiments/BAS-M02.md) | Identical-binary null testing after harness changes. | The old symmetric `[-3,+3]` setup had zero expected |
| [BAS-M03](experiments/BAS-M03.md) | Opening-book migration to `UHO_Lichess_4852_v1.epd`. | Retained as the common SPRT/SPSA/gauntlet book because it |
| [BAS-M04](experiments/BAS-M04.md) | SPSA schedule audit: iteration/game units, PowerShell `$A`/`$a` collision and perturbation resoluti… | Several schedule defects were repaired; the old runs |
| [BAS-M05](experiments/BAS-M05.md) | Resignation threshold replay against historical score streams. | `400/3` one-sided was too aggressive for the engine's |
| [BAS-M06](experiments/BAS-M06.md) | Behaviour-neutral performance measurements with single builds/runs versus pooled PGO and identical-… | An apparent −0.5% regression was retracted; pooled measurem… |
| [BAS-M07](experiments/BAS-M07.md) | Fast-TC release gains compared with longer gauntlets. | The 1.8.0 cycle measured about +93 fast-TC but |
| [BAS-M08](experiments/BAS-M08.md) | A.7.1 local replay of the Super Rating Tournament PGN: 42-engine Colosseum round | 15-26-159 (14.00%, -315.35 logistic Elo) |
| [BAS-M09](experiments/BAS-M09.md) | A.7.2 4T Colosseum null, withdrawn before exposure by maintainer decision 2026-10-01. | No games run; no null result exists. |
| [BAS-M10](experiments/BAS-M10.md) | Frozen before exposure, A.7.2 4T baseline gauntlet. | Prepared, not yet run. |
| [BAS-M11](experiments/BAS-M11.md) | Frozen before its first game, A.7.2 4T baseline re-run after BAS-M10's void. | Registered, not yet run. |
| [BAS-M12](experiments/BAS-M12.md) | Houdini 3 crashes at 4T: diagnosis (observation, 2026-10-05). | A race in Houdini 3's multi-thread start-up, triggered |

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

| ID | Title | Disposition |
|---|---|---|
| [BAS-O01](experiments/BAS-O01.md) | Search isolated. | 302-88-10, 86.5%, ≈ +322.7 ±36 Elo. |
| [BAS-O02](experiments/BAS-O02.md) | Evaluation isolated. | 264-106-30, 79.2%, ≈ +232.8 ±32 Elo. |
| [BAS-O03](experiments/BAS-O03.md) | Mechanism. | Basilisk 15.6 / 2.20 |

- [BAS-O04](experiments/BAS-O04.md) — Width attribution. — 20.80
- [BAS-O05](experiments/BAS-O05.md) — Frozen before any game, A.7.3 search deficit meter G(0) on 1.10.1. — COMPLETE (2026-10-05): G(0) = +312.6 ± 17.8

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

| ID | Title | Disposition |
|---|---|---|
| [BAS-D01](experiments/BAS-D01.md) | Where the width is not. | First-move cutoffs 89.10% |
| [BAS-D02](experiments/BAS-D02.md) | Where the width is. | 36.1% |

- [BAS-E08](experiments/BAS-E08.md) — 5.9.4 joint Texel refit of the enlarged surface — see entry

- [BAS-E09](experiments/BAS-E09.md) — the +31% bench does not buy a proportional depth loss — see entry

- [BAS-E10](experiments/BAS-E10.md) — 5.9.5 king-safety coordinate descent; the table reshape breaks mating and is deferred — see entry

- [BAS-E11](experiments/BAS-E11.md) — 5.9.6 REJECTED at −77.92 Elo; no measured mechanism explains it — REJECTED

- [BAS-E12](experiments/BAS-E12.md) — recovering the NPS the new terms cost — see entry

- [BAS-E13](experiments/BAS-E13.md) — the revert is provably the 1.9.3 engine — see entry



- [BAS-E14](experiments/BAS-E14.md) — the resign threshold is why the corpus had no mating material — see entry

- [BAS-E15](experiments/BAS-E15.md) — rounds above book size buy nothing; deterministic self-play repeats verbatim — see entry

- [BAS-E16](experiments/BAS-E16.md) — Beast positions as STARTS with self-play labels; adopting Manta's design after wrongly rejecting it — see entry

- [BAS-E17](experiments/BAS-E17.md) — 5.9.11 label-source experiment, REGISTERED BEFORE RUNNING — see entry

- [BAS-E18](experiments/BAS-E18.md) — 5.9.11 results: the labels were the defect, and the scalar surface is saturated — see entry

- [BAS-E19](experiments/BAS-E19.md) — 5.9.15 LTC probe: no depth story — see entry

- [BAS-E20](experiments/BAS-E20.md) — 5.9.14 king-safety refit on game-result labels; the reshape is real but far milder than the distill… — see entry

- [BAS-E21](experiments/BAS-E21.md) — 5.9.14 ACCEPTED: +2.64 Elo, the first gain of Phase 5.9 — ACCEPTED

- [BAS-E22](experiments/BAS-E22.md) — 5.9.12 full-surface fit: the PSTs move, and the added terms are refuted — see entry

- [BAS-E23](experiments/BAS-E23.md) — 5.9.13 ACCEPTED: +9.52 Elo from unfreezing the piece-square tables — ACCEPTED

- [BAS-E24](experiments/BAS-E24.md) — removing the refuted terms; the expected NPS recovery did not appear — PASS

- [BAS-E25](experiments/BAS-E25.md) — term removal ACCEPTED as a non-regression; Phase 5.9 closes — ACCEPTED

---

## Phase 5.9 closing summary

| step | change | verdict |
|---|---|---|
| 5.9.1–5.9.6 | 16 new evaluation terms, fitted on a distilled corpus | **−77.92 Elo, reverted** |
| 5.9.11 | corpus rebuilt on-policy; three label sources | none passed |
| 5.9.15 | LTC probe at `10+0.1` | no depth story |
| **5.9.14** | **king-safety funnel refit** | **ACCEPTED +2.64** |
| **5.9.13** | **full-surface refit, 768 PSTs unfrozen** | **ACCEPTED +9.52** |
| cleanup | the 16 added parameters removed | accepted, neutral |

**Net ≈ +12 Elo, and not one point of it came from a new feature.** Both gains
came from fitting surfaces the engine already had:

- the **capped non-linear king-danger funnel**, reachable only by coordinate
  descent — a linear fit had nothing to say about it;
- the **768 piece-square tables**, frozen since **Phase 4.7** and untouched by
  every fit in this phase until 5.9.12.

The sixteen terms added at 5.9.1/5.9.2 were measured inert three times and then
deleted. **The evaluation was mis-calibrated, not under-featured.**

**Durable lessons from the phase:**

1. **Verify what a data file contains before fitting it.** The −77.92 came from
   fitting Stockfish *evaluations* believing they were game results, because a
   summary said so and nobody checked. `parse_target` now aborts on any target
   that is not 0, 0.5 or 1.
2. **Holdout loss does not predict Elo — it has now inverted twice** (−6.2% →
   −77.92; −0.43% → +9.52). Never rank or justify a candidate by it.
3. **Check which parameters a fit actually reaches.** `--audit-coverage` exists
   because 348 of 1,190 were fitted while the number was reported as if it were
   the surface.
4. **A hovering LLR means neutral; a drifting one means real.** Visible in every
   run here long before it finished, and a better early signal than the Elo
   estimate.
5. **A candidate withdrawn over a canary deserves re-examination once the
   canary's cause is understood.** 5.9.14's +2.64 was blocked since 5.9.5 by a
   corpus artifact, not by evidence.
6. **A stronger engine is not a better labeller.** Stockfish outcomes lost 7.30
   Elo to our own; an evaluation belongs to the search that consumes it.

- [BAS-D09](experiments/BAS-D09.md) — search diagnostics re-measured after Phase 5.9; one is stale — see entry

- [BAS-D03](experiments/BAS-D03.md) — is superseded. — see entry

- [BAS-D10](experiments/BAS-D10.md) — `singularQuietLMR` implemented; the reference's magnitude is wrong for us by 8× — see entry

- [BAS-D11](experiments/BAS-D11.md) — 5.7.3 REFUTED: the reference's extension exclusivity fails our tactical floor — REFUTED

- [BAS-D12](experiments/BAS-D12.md) — 5.7.2 reading, not a verdict — see entry

- [BAS-D13](experiments/BAS-D13.md) — 5.7.4 REFUTED: the reference's verification search is neutral, and its apparent gain was an outlier… — REFUTED

- [BAS-D14](experiments/BAS-D14.md) — 5.7.5 singular gate depth: the two instruments measure the same trade-off from opposite ends, and W… — see entry

- [BAS-D15](experiments/BAS-D15.md) — 5.7.6 dead-code removal, behaviour-neutral — REJECTED

- [BAS-D16](experiments/BAS-D16.md) — 5.8.2 aspiration instrumentation, and 5.8.3/5.8.4 REFUTED — REFUTED

- [BAS-D17](experiments/BAS-D17.md) — 5.8.5 fail-high depth reduction REFUTED, fails the floor — REFUTED

## Cluster 5.8 closing summary

| step | outcome |
|---|---|
| 5.8.1 inventory | done — all divergences are in the aspiration window |
| 5.8.2 instrumentation | done — 1.37 re-searches per root iteration |
| 5.8.3 fail-low narrows beta | **REFUTED** — more re-searches, no direction |
| 5.8.4 delta growth | **REFUTED** — −0.243 ply, 20/33 |
| 5.8.5 fail-high depth reduction | **REFUTED** — WAC 119, fails floor |
| 5.8.6 documentation | done — `reported_score`/`score` split stated in place |
| 5.8.7 clock | not opened; PLAN's precondition never met |

**No candidate survived. No games were spent.** Combined with 5.7 — one
reading kept, three refuted, one undecided — **seven of the eight
reference-derived search candidates across both clusters do not transfer.**

That is the cluster's actual finding, and it is consistent: `9587eeeb` is the
last pure-HCE master and is **older than large parts of our search**. Where we
diverge, we are frequently the later idiom, not the deficient one. Both audits
independently reached "blend of eras, not behind", and the measurements bore it
out.

*Retained:* the five aspiration counters and the singular probes. Everything
future work needs to re-open either cluster is now counted rather than guessed.

- [BAS-D18](experiments/BAS-D18.md) — 5.7.2 ACCEPTED BY MAINTAINER DECISION, not by a gate — ACCEPTED

- [BAS-D19](experiments/BAS-D19.md) — A.2.3 parameter liveness probe: observation — see entry

- [BAS-E26](experiments/BAS-E26.md) — 5.9.7 endgame recogniser inventory: rook endings dominate, and PLAN's own ordering under-ranked them — see entry

- [BAS-E27](experiments/BAS-E27.md) — frequency was the wrong target: our endgame evaluation is fine almost everywhere, and badly wrong i… — see entry

- [BAS-E28](experiments/BAS-E28.md) — 5.9.17: KBNK conversion measured directly, and nearly doubled — see entry

- [BAS-E29](experiments/BAS-E29.md) — 5.9.17 continued: KBNK conversion 13.0% → 54.5%, and WHY the old drive failed — see entry

- [BAS-E30](experiments/BAS-E30.md) — which forced-win endings actually need work, and why KBBK is worse than KBNK — see entry

- [BAS-E31](experiments/BAS-E31.md) — 5.9.18: randomised conversion floors added to CTest — see entry

- [BAS-E32](experiments/BAS-E32.md) — CORRECTION to BAS-E27: the per-class mean hid a systematic error on the drawn subset — see entry

- [BAS-E33](experiments/BAS-E33.md) — the reference's endgame table against ours — see entry

- [BAS-E34](experiments/BAS-E34.md) — 5.9.19, the generic bare-king mate drive — see entry

- [BAS-E35](experiments/BAS-E35.md) — 5.9.21 closes empty: the KBNK stalemates are not a defect — see entry

- [BAS-E36](experiments/BAS-E36.md) — 5.9.22: the KBNK failures are STUCK, not slow; two remedies refuted — REFUTED

- [BAS-E37](experiments/BAS-E37.md) — 6.1.c: KBNK diagonal dominance screen selects `1000,0,220,0` — see entry

- [BAS-E38](experiments/BAS-E38.md) — corrected 6.1.c refinement improves 26/60 -> 47/60, but the boundary leaders fail live truth — see entry

- [BAS-E39](experiments/BAS-E39.md) — 6.1.c upper-range screen selects `15600,1750,0,340,0`; the plateau, not the peak, is the finding — see entry

- [BAS-E41](experiments/BAS-E41.md) — 6.1.e: the held-out set rejects the 6.1.c winner and confirms the plateau probe — REJECTED

- [BAS-E42](experiments/BAS-E42.md) — 6.1.f: the accepted KBNK vector is provably isolated, and the mate-band bound is exact — see entry

- [BAS-E43](experiments/BAS-E43.md) — 6.0.g census: KBNK does not occur in search trees from real positions — see entry

- [BAS-E44](experiments/BAS-E44.md) — 6.2.a: the 6.1 KBNK change is indistinguishable in play; stopped undecided at 7,720 games — see entry

- [BAS-E45](experiments/BAS-E45.md) — the 6.1 mechanism transfers across a 10x node budget, but the 6.1.e rejection does not — REJECTED

- [BAS-E46](experiments/BAS-E46.md) — 8,000-node datagen mislabels a fifth of won endings, concentrated in the Group B families — see entry

- [BAS-E47](experiments/BAS-E47.md) — the endgame instrument aborted correct pawn play; 6.0.b corrected — see entry

- [BAS-E48](experiments/BAS-E48.md) — 6.3.a: no king-to-passed-pawn approach signature exists in Basilisk's own failures — see entry

- [BAS-E49](experiments/BAS-E49.md) — the current head on the corrected 6.0.b cohort, and an unexpected KBNK/KBP-K interaction — see entry

- [BAS-E50](experiments/BAS-E50.md) — 6.0.c ceilings regenerated; the hard residue is 13 positions, not 75 — see entry

- [BAS-E51](experiments/BAS-E51.md) — 6.1.f redone: the KBNK vector reaches every bishop-plus-pawn family, and the original test could no… — see entry

- [BAS-E52](experiments/BAS-E52.md) — 6.4.a: the pruning-margin hypothesis is refuted, and refuted backwards — see entry

- [BAS-X08](experiments/BAS-X08.md) — `ca89031` include-what-you-use verified on Windows — see entry

- [BAS-E53](experiments/BAS-E53.md) — current 6.8.a: the narrow-window rook failure IS resolvable, and not by depth — see entry

- [BAS-E54](experiments/BAS-E54.md) — 6.5.a: the reference's rook-ending scaling ports, but only with a floor on the discount — see entry

- [BAS-E55](experiments/BAS-E55.md) — 15.1.a release gate: the SEE-repaired dev head against the 1.9.3 release binary - CLOSED, BOTH LEGS… — CLOSED

- [BAS-E55](experiments/BAS-E55.md) — CLOSED 2026-09-10: BOTH LEGS PASS. — CLOSED

- [BAS-E56](experiments/BAS-E56.md) — 15.0.a isolated cost gate: the SEE king-legality repair against its own parent - CLOSED, REPAIR KEPT — CLOSED

- [BAS-E57](experiments/BAS-E57.md) — 15.0.b harness reserve sweep: `Move Overhead` 40 against 10 on the same binary - RUN 2026-09-10, RE… — REJECTED

- [BAS-X11](experiments/BAS-X11.md) — current standing, 12,000-game Colosseum round robin — see entry

- [BAS-E40](experiments/BAS-E40.md) — 6.1.d: both bishop-dependent KBNK remedies stay closed, and the reason is now stronger than their r… — REFUTED

- [BAS-E39](experiments/BAS-E39.md) — removes their motivation. — see entry

- [BAS-D08](experiments/BAS-D08.md) — per-iteration cost; there is no shallow target — see entry

- [BAS-D07](experiments/BAS-D07.md) — deep-segment branching; corrects BAS-D05 and BAS-O04 — see entry

- [BAS-D06](experiments/BAS-D06.md) — shallow-depth node cost, step 5.14 — see entry

- [BAS-D05](experiments/BAS-D05.md) — consecutive-depth branching; the leading diagnosis is overturned — see entry

- [BAS-D04](experiments/BAS-D04.md) — history-pruning reachability — see entry

- [BAS-D03](experiments/BAS-D03.md) — qsearch share, all three arms — see entry

### Cluster 5.4.3 — reduction hypotheses, all refuted before games

Three attempts to act on the 5.3 inventory's item 1 ("reduction modulation is
nearly inert — raise it"). All were measured on `tools/diag/suite_v1.epd` and
all moved the target metric the **wrong way**, so none reached an SPRT. Target
metric is mean depth at a fixed 300,000-node budget, where the 5.2 baseline is
20.80 against the oracle's 32.87.

| ID | Title | Disposition |
|---|---|---|
| [BAS-S13](experiments/BAS-S13.md) | Fractional history response: `(stat/div)*1024` → `stat*1024/div`, removing the whole-ply quantisati… | Worse. |
| [BAS-S14](experiments/BAS-S14.md) | Raise the context magnitudes toward reference scale via the TUNE knobs: `LmrCutNodeAdj` | Worse or flat. |
| [BAS-S15](experiments/BAS-S15.md) | Hypothesis that the `new_depth - 1` ceiling caps mean reduction, making modulation | Refuted. |

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

- [BAS-S16](experiments/BAS-S16.md) — Cluster 5.4.4 — check-move depth policy, REJECTED — see entry

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

### Search programme investigation (B.0)

Zero-game measurements on the 1.10.1 head and two registered game tests,
2026-10-06. The packet is `analysis/search_programme_2026-10-06.md`; every
artifact is under `tools/results/b0-20261006/` with hashes in its §15. The
positions suite is `tools/diag/suite_v2.epd` (suite_v1 minus one illegal
position the 1.10.1 board rejects; 106 positions). Binaries: the A.7.4
release `basilisk-a74-nps-pgo1-pext-pgo.exe` (`3e5294be`, bench 14,978,465),
Tune and Ablate builds of `6c5ee63` (same bench), `oracle-1.10.1.exe`,
`oracle-1.10.1-ablate.exe` (the A.7.3 recipe plus the snapshot's
`oracle-hybrid-ablate.patch`; conformance 471,519 positions, 0 mismatches;
bench 967,078 as the unpatched oracle) and the official Stockfish 19
`x86-64-universal` binary (tag `sf_19`, `edb0d9db`, the pinned donor).

- [BAS-D20](experiments/BAS-D20.md) — the deficit is a constant ×4 node multiplier, not per-ply growth — see entry

- [BAS-D21](experiments/BAS-D21.md) — matched per-family ablation screens: the ×4 is the move-loop pruning family; razoring and LMR cost… — see entry

| ID | Title | Disposition |
|---|---|---|
| [BAS-S17](experiments/BAS-S17.md) | Registered, not yet run (B.0.1, maintainer). | Reading rule: |
| [BAS-S18](experiments/BAS-S18.md) | Registered, not yet run (B.0.2, maintainer). | Reading rule: |
| [BAS-S19](experiments/BAS-S19.md) | Frozen before any game, PLAN A.8.15: the tablebase band and probes with tables configured. | ACCEPTED: +31.7 ± 13.4 with tables |
| [BAS-S20](experiments/BAS-S20.md) | Frozen before any game, PLAN A.8.18: the won-endgame time-sink repair. | INCONCLUSIVE at the cap: −0.7 ± 4.5; maintainer to decide |

### Accepted or retained

| ID | Title | Disposition |
|---|---|---|
| [BAS-S01](experiments/BAS-S01.md) | TT-bound-aware pruning evaluation used a proving TT bound for RFP, razoring, NMP, | Accepted, +7.18 Elo |
| [BAS-S02](experiments/BAS-S02.md) | A jointly exposed search bundle after several knobs had landed inert at | Accepted, +9.14 Elo |
| [BAS-S03](experiments/BAS-S03.md) | Exact/PV-node best-move quiet-history reward, without sibling maluses. | Accepted, +4.90 Elo |
| [BAS-S04](experiments/BAS-S04.md) | Static-eval-surprise scaling of history. | Accepted, +2.50 Elo |
| [BAS-S05](experiments/BAS-S05.md) | Denser 32-byte partial-key TT cluster and replacement changes. | Accepted, +4.27 Elo |
| [BAS-S06](experiments/BAS-S06.md) | SEE excluded absolutely pinned attackers through a shared exchange-occupancy pin scan. | Retained for correctness; +0.65 Elo claim unverified |

### Rejected, neutral or deferred

| ID | Title | Disposition |
|---|---|---|
| [BAS-S07](experiments/BAS-S07.md) | Added a 6-ply continuation-history channel to the then-current history stack. | Rejected, −7.70 Elo. |
| [BAS-S08](experiments/BAS-S08.md) | Blanket removal of the check extension against the `hcefinal`-tuned head. | Rejected, −10.17 ± 6.52 over 4,682 games; reverted. |
| [BAS-S09](experiments/BAS-S09.md) | Corrected an LMR reduction gate that consumed `gives_check` after making the move. | Standalone rejected, about −20 Elo; reverted/deferred. |
| [BAS-S10](experiments/BAS-S10.md) | Exact-node history update reused cutoff-style sibling maluses. | Rejected, −84.21 ± 18.85 over 652 games. |
| [BAS-S11](experiments/BAS-S11.md) | Cutoff-count LMR was considered after Rarog tested a full LMR-family retune. | Not implemented standalone; imported Rarog result was −7.78… |
| [BAS-S12](experiments/BAS-S12.md) | Cuckoo repetition and post-LMR history candidates were tested on the old unpinned | Rejected/closed for the old state; measurements carried har… |

## 4. Root search, time management and SMP

| ID | Title | Disposition |
|---|---|---|
| [BAS-R01](experiments/BAS-R01.md) | Start the move clock at receipt of `go`, including GUI-to-worker dispatch latency. | Retained non-regression, +2.95 ± 6.74 Elo |
| [BAS-R02](experiments/BAS-R02.md) | SPSA of nine time-management constants on the old root model. | Neutral, +0.88 ± 4.03 over 12,262 games; reverted. |
| [BAS-R03](experiments/BAS-R03.md) | Best-move-instability time extension using a decaying root-change signal. | Accepted; +10.79 pre-affinity, +6.46 ± 4.12 on fixed harnes… |
| [BAS-R04](experiments/BAS-R04.md) | Phase-9 helper clock/node/thread safety bundle at 4T. | Accepted, +30.42 ± 8.77 at 4T, zero forfeits in 2,450 games. |
| [BAS-R05](experiments/BAS-R05.md) | Shared-node batching/scaling change. | Retained: |
| [BAS-R06](experiments/BAS-R06.md) | Extra helper coordination, aspiration sharing, diversification, TT variants and HCE refit during | Rejected or removed; not shipped. |

## 5. Evaluation and data experiments

The rows below preserve the historical decision to freeze HCE and route the
remaining gap to NNUE. That scheduling conclusion is superseded by the current
Plan 8.0-8.12: Rarog's later controlled HCE/data evidence now justifies a
bounded whole-surface audit, matched label experiment and iterative Texel
cycles. The old results remain binding priors and retry constraints; they do
not accept any new feature, vector or label policy by themselves.

- [BAS-E07](experiments/BAS-E07.md) — HCE maturity audit, 2026-08-13 — see entry

| ID | Title | Disposition |
|---|---|---|
| [BAS-E01](experiments/BAS-E01.md) | Staged Texel scalar/structural fits against successive accepted heads. | Material +29.05, mobility +8.77, passed pawns +16.57 and |
| [BAS-E02](experiments/BAS-E02.md) | Larger staged HCE campaign with king safety, threats, positional, imbalance, mobility/PST and | Each stage accepted; cumulative +280.74 versus the early |
| [BAS-E03](experiments/BAS-E03.md) | Stockfish-at-60k quiet-position distillation, with material scale pinned. | Accepted, +6.75 Elo. |
| [BAS-E04](experiments/BAS-E04.md) | Successive on-policy self-play refresh cycles with phase balancing and joint linear/king-safety ref… | Accepted: |
| [BAS-E05](experiments/BAS-E05.md) | `hcefinal` joint history/eval SPSA. | Accepted, +35.94 ± 9.42. |
| [BAS-E06](experiments/BAS-E06.md) | Five-phase corpus/tooling upgrade and attempted Phase-9 HCE refit. | +1.52 ± 5.77 Elo |
| [BAS-E08](experiments/BAS-E08.md) | Frozen 770-position Syzygy truth baseline: accepted head `294a3e2` versus Stockfish `dev-20260716-e… | Baseline observation: |

## 6. Throughput, build and platforms

| ID | Title | Disposition |
|---|---|---|
| [BAS-P01](experiments/BAS-P01.md) | Phase-8.7 profile-guided, bench-identical optimization wave. | Accepted, +4.34% pooled-PGO NPS; batch result +8.69 ± 6.63… |
| [BAS-P02](experiments/BAS-P02.md) | Continuation-history row/index hoists in move scoring. | Accepted, +3.03% NPS |
| [BAS-P03](experiments/BAS-P03.md) | SEE memoization/classification reuse. | Accepted, about +0.36% NPS |
| [BAS-P04](experiments/BAS-P04.md) | Per-node `CheckInfo`, check-hinted `make_move`, and pin-sharing candidates. | Rejected: |
| [BAS-P05](experiments/BAS-P05.md) | Pawn-cache resize at observed 89–99% hit rate. | Closed as dead. |
| [BAS-P06](experiments/BAS-P06.md) | Enrich PGO training beyond the expanded bench suite. | Rejected/retired; bench-only PGO retained. |
| [BAS-P07](experiments/BAS-P07.md) | `origin/arm_fix` aligned the TT allocation to 64-byte cache lines. | Rejected hypothesis: |
| [BAS-P08](experiments/BAS-P08.md) | Windows ARM64 Clang PGO selected `llvm-profdata` from global `PATH`. | Correctness/tooling repair in 1.9.3; search unchanged, benc… |
| [BAS-P09](experiments/BAS-P09.md) | Windows x64 PEXT+PGO compiler comparison at `5b92458`, dirty build-support patch `33b60cc9`: MSVC | MSVC rejected as the Windows production default; build supp… |
| [BAS-P10](experiments/BAS-P10.md) | GCC PGO advertised `-fprofile-generate/-use`, but the orchestrator always searched for Clang `.prof… | Tooling repair accepted; search unchanged. |
| [BAS-P11](experiments/BAS-P11.md) | Main CI ran only after pushes to `master` or manual dispatch, while | Delivery repair accepted; search unchanged. |
| [BAS-P12](experiments/BAS-P12.md) | Frozen before the run, A.7.4 pooled-PGO NPS baseline of 1.10.1. | COMPLETE (2026-10-05, 23:16–23:30): baseline 4.127M NPS |
| [BAS-P13](experiments/BAS-P13.md) | Frozen before the run, PLAN A.8.9's pooled-PGO NPS read against 1.10.1. | ACCEPTED: +1.13%, unexplained |
| [BAS-P14](experiments/BAS-P14.md) | Frozen before the run, PLAN A.8.12's pooled-PGO NPS read against the head before it. | ACCEPTED: −0.11%, neutral |

## 7. Correctness and protocol lessons

| ID | Title | Disposition |
|---|---|---|
| [BAS-C01](experiments/BAS-C01.md) | Brittle fixed-depth endgame conversion canaries rejected benign eval/search/TT changes. | Replaced with eval recognition, tolerant conversion floors… |
| [BAS-C02](experiments/BAS-C02.md) | Rule-50/mate precedence, null-move halfmove preservation and legal-EP hashing defects. | Fixed and covered by board/search invariants. |
| [BAS-C03](experiments/BAS-C03.md) | TT, parser, PV and SMP could expose stale or illegal moves. | Pseudo-legality validation, PV truncation, differential per… |
| [BAS-C04](experiments/BAS-C04.md) | Helper/private clocks and fixed-depth inheritance did not match deployed multi-thread semantics. | Repaired in 1.9.2 with aggregate accounting and zero-forfeit |
| [BAS-C05](experiments/BAS-C05.md) | A TT slot published its 64-bit payload separately from a plain 16-bit | CLOSED 2026-09-07: the defect is real, BOTH repairs were re… |
| [BAS-C06](experiments/BAS-C06.md) | Positive Syzygy tests depended on `D:\\chess\\Syzygy345`; when that private directory was absent | Test-instrument repair accepted; search unchanged. |
| [BAS-C07](experiments/BAS-C07.md) | `Board` exposed writable bitboards, occupancies, mailbox, king/check caches, incremental keys and u… | API containment accepted; search unchanged. |
| [BAS-C08](experiments/BAS-C08.md) | SEE king legality (15.0.a). | Repaired and closed 2026-09-09. |
| [BAS-C09](experiments/BAS-C09.md) | Created pins and recapture promotions (15.0.c). | CLOSED 2026-09-10 as a documented approximation, no engine… |
| [BAS-C10](experiments/BAS-C10.md) | Lost `ponderhit` after an instant opponent reply (1.10.1). | Fixed in 1.10.1; bench unchanged at 14,978,465. |
| [BAS-C11](experiments/BAS-C11.md) | Setup work charged to the clock (1.10.1). | Repaired 2026-09-27; bench unchanged at 14,978,465. |
| [BAS-C12](experiments/BAS-C12.md) | Rejected `position` searched the previous board (1.10.1). | Policy flipped by maintainer decision 2026-09-27; bench unc… |
| [BAS-C13](experiments/BAS-C13.md) | Tablebase PV lines restored the Stockfish way (1.10.1). | Implemented 2026-09-27; bench unchanged at 14,978,465. |

## 8. Cross-engine evidence imported from Rarog

These are ideas, warnings or ordering priors already incorporated where useful
in Basilisk's forward plan. Listing an item here does not by itself create a
roadmap item.

The 2026-09-07 refresh audited Rarog `dev` at `881c821` (clean and matching
`origin/dev`), including PLAN Phase 4, GUIDE, EXPERIMENTS, PROCESS,
SPSA_IMPROVEMENTS and the linked board/endgame/SEE/Texel analyses. Later donor
changes require a new import record rather than silently updating these claims.

**BAS-X09/X10 and BAS-X16 are the exceptions.** X09/X10 changed Phase 5's
purpose from bounded pre-NNUE hardening to a search and evaluation acceleration
program. X16 contains measurements of Basilisk itself made by Rarog's
cross-engine harness. Even there, the host/build/protocol limits remain and the
result cannot accept a Basilisk change.

Note what they do **not** license. They establish that a mature search is worth
roughly 200 Elo to us and that a mature HCE is worth more again. They say
nothing about which specific mechanism earns it, and they are not permission to
transcribe Stockfish. Basilisk remains an independent engine; the reference is
an idea source and an oracle. See PLAN's operating contract.

| ID | Title | Disposition |
|---|---|---|
| [BAS-X01](experiments/BAS-X01.md) | Check-extension removal was +30.75 Elo in Rarog but −10.17 ± 6.52 in | import |
| [BAS-X02](experiments/BAS-X02.md) | Stockfish distillation improved holdout loss by 4.9% yet lost −17.11 Elo in | import |
| [BAS-X03](experiments/BAS-X03.md) | Rarog's full `cutoffCnt`/LMR-family candidate lost −7.78 ± 8.00 despite its tuning trajectory. | import |
| [BAS-X04](experiments/BAS-X04.md) | Rarog gained +22.13 from history bonus/malus work and +6.01 from a broader | import |
| [BAS-X05](experiments/BAS-X05.md) | Rarog's accepted SMP rework was +102.78 ± 16.38 at 4T. Historical Rarog | import |
| [BAS-X06](experiments/BAS-X06.md) | Rarog's bench-identical speed wave gained +10.35% NPS and +20.31 ± 7.13 Elo | import |
| [BAS-X07](experiments/BAS-X07.md) | Rarog's AArch64 TT prefetch later measured +1.42% NPS on M4 with 12/12 | import |
| [BAS-X08](experiments/BAS-X08.md) | Rarog's parity audit emphasizes shared `MoveEvidence`, prospective depth and correction attribution. | import |
| [BAS-X09](experiments/BAS-X09.md) | Rarog RAR-O02 (no adjudication, 1,238 games, 982 natural mates, `3+0.03`, 1T): Stockfish | import |
| [BAS-X10](experiments/BAS-X10.md) | In the same run, the exact-revision Stockfish HCE beat that hybrid by | import |
| [BAS-X15](experiments/BAS-X15.md) | Rarog `4aea0c7`/RAR-E10 replaced its coarse KBNK corner metric with a bishop-colour-selected weak-k… | import |
| [BAS-X16](experiments/BAS-X16.md) | Rarog RAR-M20 directly benchmarked Basilisk `d734766` against Rarog `ca03a46` and Reckless `91b56c2` | import |
| [BAS-X17](experiments/BAS-X17.md) | Rarog RAR-M21 reran one fixed cohort at 60k/200k/600k nodes: its net reference | import |
| [BAS-X18](experiments/BAS-X18.md) | Rarog's 36,400-game occurrence census corrected prior zeroes, found KRPPKRP in 5.40% of | import |
| [BAS-X19](experiments/BAS-X19.md) | Rarog RAR-M22 found raw first-clean-win contradictions in 4.39% of `hce-v2` games and | import |
| [BAS-X20](experiments/BAS-X20.md) | Rarog 4.10 found a material-shed termination bug, zero-overlap cohorts compared as if | import |
| [BAS-X21](experiments/BAS-X21.md) | Rarog RAR-M25's board-v2 corpus covers canonical FEN/legal/capture identities, checks/evasions, pin… | import |
| [BAS-X22](experiments/BAS-X22.md) | Rarog RAR-M27/M28 found three SEE defects (selected-king legality, created pins, recapture promotio… | import |
| [BAS-X23](experiments/BAS-X23.md) | Rarog's complete HCE refit RAR-E06 accepted +22.04 ±7.51 Elo; matched TB relabel | import |
| [BAS-X24](experiments/BAS-X24.md) | Rarog's fitting handbook records why phase-balanced starts were inefficient: only opening starts | import |
| [BAS-X25](experiments/BAS-X25.md) | Rarog canceled a prepared 320k-game broad SPSA after expected-value review; its durable | import |
| [BAS-X26](experiments/BAS-X26.md) | Rarog's Phase-4 subsystem audit contract requires producer/state/consumer/invalidation inventories,… | import |
| [BAS-X27](experiments/BAS-X27.md) | Rarog `881c821` adds exact board-call/work counters for its pending 4.11b.7 HCE-search profile, | import |
| [BAS-X28](experiments/BAS-X28.md) | Independent review of the 2026-09-07 fix stack (`bdb828a`..`6dd9ada`) from a session that | import |
| [BAS-X29](experiments/BAS-X29.md) | Rarog's 2026-09-09 review of the shared SEE lineage: Basilisk's `see_ge` has no | import |

The 2026-09-07 re-audit supersedes the earlier conclusion that no additional
high-value Rarog item was missing. Rarog's newer Phase-4 work added substantial
instrument, board, endgame-ranking, HCE-cycle, subsystem-audit and delivery
knowledge after the original cross-review. BAS-X16–X27 record that delta. Each
is an imported prior or method unless it explicitly measured Basilisk; none is
an acceptance verdict for a future Basilisk change.

**2026-09-28 import: Rarog's search programme** (Rarog `dev` at `08c3a23`,
snapshotted verbatim at `015bccae`, which differs only in RAR-M63's note on
Basilisk 1.10.1;
PLAN, GUIDE, AGENTS, PROCESS, the B-phase ledger rows and
`analysis/search_programme_2026-09-13.md`). The rewritten Basilisk roadmap
adopts Rarog's method and order; these rows are the evidence it cites. They
are priors and methods, never acceptance: Rarog's donor was Reckless, its
evaluator is weaker than Basilisk's, and its verdicts do not transfer
(BAS-X01).

| ID | Title | Disposition |
|---|---|---|
| [BAS-X30](experiments/BAS-X30.md) | Donor-shaped search clusters, fitted, then gated. | import |
| [BAS-X31](experiments/BAS-X31.md) | Tunes in blocks. | import |
| [BAS-X32](experiments/BAS-X32.md) | Harness for the programme. | import |
| [BAS-X33](experiments/BAS-X33.md) | What Rarog's B.0 found by measuring before designing | import |
| [BAS-X34](experiments/BAS-X34.md) | Rarog's measurements of Basilisk and of the target pool | import |
| [BAS-X35](experiments/BAS-X35.md) | Rarog 2.4.0→2.5.0, the final import | import |

## 8b. Cross-engine evidence imported from Manta

Manta is a Zig engine by the same maintainer, developed against the same
reference snapshot. These are **imported priors**: they order and warn, and they
never accept a Basilisk change.

| ID | Title | Disposition |
|---|---|---|
| [BAS-X11](experiments/BAS-X11.md) | `MAN-E05` (endgame conversion grading, −16.32 Elo) and `MAN-E07` (nonlinear material imbalance, −7.… | import |
| [BAS-X12](experiments/BAS-X12.md) | `MAN-S18` and `MAN-S20`: two selectivity clusters, 12,000 games each, that grew the | import |
| [BAS-X13](experiments/BAS-X13.md) | `MAN-S23`: a registered pre-gate branching filter refuted a candidate "in minutes of | import |
| [BAS-X14](experiments/BAS-X14.md) | Manta's fit catalogue classifies coefficients free / fixed / excluded, excluding nonlinear | import |

Manta also supplied the **method** that produced BAS-D05, which is recorded
there rather than here because it is our own measurement.

## 9. Open retry map

| Prior IDs | Retry condition | PLAN destination |
|---|---|---|
| BAS-S08, BAS-S09, BAS-S11 | Unified pre-move evidence and prospective-depth model implemented; consumers included in a single justified joint fit; post-fit ablations registered. | B.0 (2026-10-06) records: the trigger fires at B.3 on the accepted B.2 head, where pruning is history-informed (BAS-D21) and the check policy, `gives_check` evidence and `cutoffCnt` are fitted jointly; not before |
| BAS-S07, BAS-S10, BAS-S12 | Diagnostics show a distinct source/consumer gap that existing histories cannot represent. | B.0 (2026-10-06) records: fires for BAS-S07 and BAS-S10 at B.2, which re-indexes the continuation histories by in-check and capture context at plies 1–6 and adopts the donor's exact-node update policy as a unit; BAS-S12's cuckoo repetition is not fired (no missing consumer named) and stays with D.3/D.4 |
| BAS-R02, BAS-R03 | Root-confidence inputs or evaluator score scale materially change. | D.1; re-audit at F.6 |
| BAS-P04, BAS-P05, BAS-P06 | A new profile demonstrates changed reuse, cache pressure or PGO coverage. | B.7.2, C.12 or B.2 (TT and caches) by owner |
| BAS-C05 (both encodings) | A measured 4T-only strength anomaly traced to TT publication, OR a TT redesign that widens the slot for an independent reason (e.g. the multiply-hi indexing deferred in PLAN section 6), OR a pooled `nps_ab.ps1` run showing a coherent layout inside +/-0.5% of plain-key. Reasoning alone does not reopen it: both prior repairs were correct and both lost Elo. | B.0 and B.2 (the TT is part of cluster 1) |
| BAS-P07, BAS-X07 | Production ARM64 artifacts show missing prefetch or measured hot-state contention; isolate one valid variant per target-native A/B. | A.4.1, E.3.2, G.2 |
| BAS-E03, BAS-E04, BAS-E06 | NNUE data/teacher experiment, not another unchanged-surface HCE constant fit; frozen teacher and holdout are available. | C.2.6 and C.8 only on a changed search and surface; F.2–F.3 for teacher data |
| BAS-E36 Arm B (bishop proximity) | A current instrument run shows bishop shuffling remains the dominant residual failure mode and the candidate clears promotion closure and all mate floors. | C.5.2/C.5.6 only if re-ranked |
| BAS-E36 Arm C (escape-square count) | The stalemate-adjacency hypothesis is tested directly, shown to reinforce rather than compete with the corner drive, and clears deterministic KBN-K/promotion-closure floors. | C.5.2/C.5.6 only if re-ranked |

Anything not meeting its trigger stays closed. A retry is a new experiment with
a new ID and manifest; it does not overwrite the historical row. Destinations
were remapped to the lettered roadmap on 2026-09-28; the retired destinations
resolve through HISTORY's number map.

## 10. Prediction calibration review

Periodically review only experiments that actually froze a prospective
prediction. Do not score older records by reconstructing what somebody "must
have believed." Keep the review lightweight:

| Question | Review |
|---|---|
| Sign | Was the predicted direction usually correct? |
| Magnitude | Are intervals systematically too optimistic or too narrow? |
| Confidence | Are high-confidence predictions more reliable than low-confidence ones? |
| Mechanism | Which categories repeatedly surprise, and which interaction is repeatedly missed? |
| Retry discipline | Was any rejected mechanism retried before its objective trigger fired? |

Summarize patterns and changed forecasting rules; never rewrite the frozen
prediction. A good retrospective story is not proof that the result was
predicted. After a surprise ask: **which part of the original causal model was
wrong?**

## 11. Template for a new experiment

Save a new entry as `experiments/BAS-<area><number>.md` and, before any game,
add one index line to the section that owns it: a bullet whose link text is the
ID and whose target is the file, then ` — <short name> — registered`, as the
other bullets read. When the verdict lands, append it to the entry and change
the line's last word to the disposition.

```markdown
# BAS-<area><number>

**BAS-<area><number> — <short name>**

- Date / owner / calibration category:
- Baseline source revision / candidate source revision / dirty-diff identity:
- Binary / compiler / flags / PGO identity and hashes:
- Research question:
- Hypothesis / proposed mechanism:
- Interacting mechanisms / consumers:
- Credible competing hypotheses:

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected diagnostic movement:
- Expected Elo sign/range, or `not defensible`:
- Probability candidate is positive/useful:
- Confidence and basis:
- Most likely failure mode:
- Falsification criteria:
- Cheapest prior falsifier: test / result / did implementation remain justified?
- Registered gate / stop rule:
- Full conditions / provenance: book or corpus and hash, split, seed, tablebases,
  TC or node budget, threads, hash, concurrency, affinity, adjudication:

#### RESULT — append after exposure

- Diagnostics (not the verdict): nodes, EBF, NPS, depth, counters, suites:
- Deciding result: games, W-D-L, Elo/nElo and CI, LLR, or relevant proof/gate:
- Disposition: accepted / retained / rejected / neutral-inconclusive /
  observation / no-change / deferred:

#### PREDICTION CALIBRATION — preserve the original prediction above

- Original prediction summary:
- Observed result:
- Sign correct? / magnitude reasonable?:
- Proposed causal mechanism supported?:
- Important missed interaction?:
- Confidence over- or under-calibrated?:
- Postmortem: causal assumption changed / what did not change / alternatives:
- Conditional lesson:
- Objective retry trigger or `CLOSED`:
- Artifacts / commits:
```
