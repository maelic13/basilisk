# Basilisk experiment ledger

This is the indexed maintainer record of measured experiments and the lessons
that may inform later work. It is not a roadmap: [`PLAN.md`](PLAN.md) owns what
will be done and in what order. [`CHANGELOG.md`](CHANGELOG.md) remains the
user-facing release record.

Numbered references in this ledger are historical identifiers. The current
forward numbering and old-to-new map live in PLAN.md section 14; do not rewrite
measured records merely because the roadmap was reordered.

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

**BAS-E08 — 5.9.4 joint Texel refit of the enlarged surface** (2026-08-25,
`--tune scalars`, 348 active params, 1,520,109 train / 79,891 holdout rows from
`beast_sf_*`, `--l2 1e-6`, 200 epochs, K = 1.41868).

| | initial | tuned | delta |
|---|---:|---:|---:|
| holdout loss | 0.0703086 | **0.0659483** | **−6.2%** |
| opening (n=29,421) | 0.06477 | 0.05676 | −12.4% |
| early-mid (n=17,117) | 0.08900 | 0.08441 | −5.2% |
| middlegame (n=16,313) | 0.07211 | 0.07038 | −2.4% |
| **endgame** (n=13,932) | 0.05976 | 0.05951 | **−0.4%** |
| deep endgame (n=3,108) | 0.05761 | 0.05689 | −1.2% |

**Ablation — the new structure contributes essentially nothing.** Applying only
the twenty new-term values while leaving the 348 existing parameters at their
shipped values gives holdout **0.0703113** against the baseline **0.0703086** —
marginally *worse*, and inside noise. The entire 6.2% therefore comes from
refitting the pre-existing surface.

*Conditional lesson, and it is not comfortable.* That refit is close to what HCE
cycle 6 already did, and cycle 6 **washed at +1.37 ±5.21 over 8,100 games**. The
distinction this program relied on — that an enlarged surface is not the surface
that washed — is weakened by the ablation: the enlargement measures inert, so
what remains is largely cycle 7. Two recorded results say holdout loss cannot
rescue that read: durable lesson "holdout-MSE-delta does not predict Elo", and
BAS-X02, where Stockfish distillation improved holdout by 4.9% and lost −17.11
Elo in Rarog.

*Two individual terms are suspect.* `SliderOnQueen` fitted to exactly **0**
despite firing on 2,260 of 20,000 positions, and `LongDiagonalBishop` fitted
**negative** where the concept predicts a bonus. Both are the signature BAS-X11
describes of a term whose relations duplicate signal already priced elsewhere —
mobility and `bad_bishop` in this case.

*One clear success.* Splitting `king_protector` at 5.9.2 was justified: the fit
separated the pieces it could not previously distinguish, to `(−1, +4)` for
knights against `(−2, 0)` for bishops. The endgame divergence is the whole point
— a knight near its own king in the endgame is worth something a bishop is not.

*Cost carried into the gate — measured, after a first reading that was wrong.*
Bench moves **11,941,440 → 15,655,764**, +31% nodes at the bench's fixed depth
13, and this was first recorded here as a ~2.5× per-iteration deficit that the
evaluation gain would have to outrun. **BAS-E09 measured it directly and that
reading does not hold.** The real cost at realistic depths is about a quarter of
a ply. See BAS-E09.

*Correctness.* CTest 12/12 including the KBNK/KQK mate canaries — notable, since
this class of change tripped them eight consecutive times historically.

**BAS-E09 — the +31% bench does not buy a proportional depth loss** (2026-08-26,
diagnostic, no Elo claim). Paired depth-at-equal-nodes over the 107-position
`suite_v1.epd`, candidate `41797a7` against pre-bake baseline `8c5d7cc`, both
built plain Release from the same toolchain, **Hash 64 on both arms**.

| nodes/position | base mean depth | cand mean depth | paired Δ (mean) | Δ excl. mate runaways | deeper / shallower / equal |
|---:|---:|---:|---:|---:|---|
| 300,000 | 21.47 | 21.49 | **+0.019** | −0.010 | 27 / 32 / 48 |
| 1,000,000 | 26.29 | 26.12 | **−0.168** | −0.250 | 25 / 37 / 45 |

*Why the bench number misled.* Bench counts nodes to a **fixed depth 13** on a
different 40-position set. Two evaluators that disagree score a position
differently — the candidate returns 251 where the baseline returns 281 — so
aspiration windows, fail-high/low patterns and TT behaviour all diverge. A large
bench-node delta between two *different* evaluators is expected and is not by
itself evidence of a search-efficiency regression. NPS is unchanged (3.23M vs
3.31M), so the extra nodes are not a more expensive evaluator either.

*What the cost actually is.* Roughly **0.17–0.25 ply at 1M nodes**, and
indistinguishable from zero at 300k. That is a small single-digit Elo headwind
into 5.9.6, not the ~2.5× deficit first recorded. It is a real headwind and it
is the right size to state, but it does not on its own predict rejection.

*Method note, carried from BAS-O04.* The mean is reported alongside a
mate-runaway exclusion because BAS-O04's "12.07-ply gap" was a mean dominated by
ten positions running past depth 100. Here the two agree, so the conclusion does
not rest on the choice.

*Standing caution.* Depth at fixed nodes is a coarse instrument: one iteration
costs ~1.9× the previous (BAS-D08), so a 31% node difference is about a third of
an iteration and rounds to zero in most single positions. The median is 0 at
both operating points for that reason; the mean over 107 paired positions is
what carries the signal.

**BAS-E10 — 5.9.5 king-safety coordinate descent; the table reshape breaks
mating and is deferred** (2026-08-26, `--tune-kingsafety`, 43 knobs, 1,520,109
train / 79,891 holdout, coordinate descent from step 8, K = 1.49757).

Converged in 83 passes; best holdout restored from **pass 75**, after which
holdout rose — the overfit onset was caught by the restore, not by luck.
Holdout **0.0658991 → 0.0652499**, −0.99%.

*What it changed, and it is not what the early probe suggested.* A 2-pass probe
on 100k positions appeared to raise the safety table uniformly, and that reading
was recorded here as a direction before it converged. It was wrong. The
converged fit made the table sharply **convex** — low end roughly halved, high
end up ~45%, crossover near index 15:

| index | 2 | 6 | 10 | 12 | 15 | 17 | 20 | 22 | 24 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| old | 6 | 55 | 111 | 161 | 192 | 203 | 302 | 316 | 407 |
| new | 0 | 28 | 68 | 108 | 188 | 263 | 369 | 451 | 597 |

Small attacks are priced far lower, large attacks far higher. A linear fit
cannot express that reshaping, which is the case for using this instrument
rather than Texel — independent of whether the result ships.

*Two dead knobs found.* `ks_unit[ROOK]` and `ks_unit[QUEEN]` were both **0** —
rook and queen attacks contributed nothing to the attack-unit count. Both fitted
to 2. That is a standing coverage gap in king safety, not a tuning artifact.

*The canary failure, and its real cause.* Baking all 34 changed knobs fails
`test_eval`'s "mate-drive drives the pawnless defender to the edge": the edge
preference collapses from **29cp to 4cp** against a `>20` threshold. Bisection
isolated it to `safety_table` alone — reverting only the table restores 29cp and
passes 77/77 with all eleven scalar changes in place.

The cause is **corpus coverage, not the fitter**. The training corpus is
quiet-filtered self-play WDL and carries essentially no forced-mate positions,
so the objective has no signal for mating behaviour and freely halved the
low-end table. In the canary position the mate-drive's own designed contribution
is only ~15cp (`5 × lk_center`; the king-distance term cancels between the two
FENs) — the remaining ~14cp was king safety incidentally topping it up. **The
canary has therefore been passing partly by accident**, and the fit removed the
accidental part. That is worth stating plainly: a threshold met by a mechanism
plus an unrelated contribution is not a measurement of the mechanism.

*Disposition.* Variant A shipped — the eleven scalar knobs, `safety_table`
reverted. The table reshape is deferred to **5.9.12**, whose corpus (5.9.11)
must include mating and near-mating material so the objective has signal there.

*Cost.* Bench **15,655,764 → 18,228,447**, +16%. Paired depth at equal nodes is
**−0.196 ply at 1M** against the accepted head, statistically unchanged from
5.9.4 alone (−0.168). The king-safety scalars cost nothing measurable, and the
bench rise is once again a poor proxy — see BAS-E09.

*Score scale moved at 5.9.4, and this is the first record of it.* `K` fitted
**1.41868** at the start of 5.9.4 and **1.49757** at the start of 5.9.5, both on
the same holdout. Since each is fitted before its run, that is a ~5.6%
**compression of the eval scale caused by the 5.9.4 refit**. Our futility,
razoring and delta-pruning margins are centipawn constants calibrated to the old
scale and are now effectively ~5.6% more aggressive; BAS-M05 records the resign
threshold as engine-scale dependent for the same reason. Not repaired here — it
is registered as a known consequence carried into 5.9.6's verdict, and it is
exactly the failure mode 5.9.12's mandatory score-scale audit exists to catch.

**BAS-E11 — 5.9.6 REJECTED at −77.92 Elo; no measured mechanism explains it**
(2026-08-26). `basilisk-5.9.5-cand-pext-pgo` (`fd048497a5`, bench 18,228,447)
against `basilisk-1.9.3-baseline-pext-pgo` (`16eff201`, clean, bench
11,941,440). `3+0.03`, 1T, Hash 64 both arms, UHO, concurrency 14.

**Elo −77.92 ±15.32, nElo −99.71 ±18.94, LOS 0.00%, 1,292 games, LLR −2.95 →
H0.** Ptnml [117, 199, 219, 74, 37], PairsRatio 0.35. The loss is broad and
consistent, not a few catastrophic games.

Every hypothesis was then tested **without further games**. None of them, alone
or summed, accounts for the result.

| hypothesis | measurement | verdict |
|---|---|---|
| NPS regression | new-term code costs **4.1%** (3.358M → 3.221M) at *identical* bench nodes | real, ~2–4 Elo |
| tree growth | bench nodes +52.6% at fixed depth; time-to-depth **+60%** | real, maybe 10–15 Elo |
| depth at equal nodes | **−0.196 ply** at 1M, 107 paired positions | real, small |
| static eval quality | holdout **0.0656034** vs baseline 0.0703086 — **6.7% better** | refuted as cause |
| tactical ability | WAC@12: **49 fails baseline, 48 candidate** | refuted as cause |
| lazy-eval divergence | 0 sign flips both; crossings 2.191% → **1.365%** (better); mean abs delta 157.8 → 181.1 | refuted as cause |
| score-scale drift | `K` 1.41868 → 1.47613, **4.1% compression** | real, few Elo |

*The residual is the finding.* Identified costs sum to perhaps 15–20 Elo against
a measured 78. **The fitted values are simply worse in play while being better on
the corpus.** This is BAS-X02's lesson at roughly four times the magnitude —
there, Stockfish distillation improved holdout 4.9% and lost 17.11 Elo in Rarog;
here 6.7% better holdout costs 77.92.

*CORRECTION (2026-08-26, same day). The primary cause was misdiagnosed below.*
The `beast_sf_*` corpus is **Stockfish-distillation labelled**, not self-play
WDL. `import_beast.py` imports `FEN<TAB>target` where the target is a Stockfish
expected score, and the data confirms it: **427 distinct target values** in
200,000 rows, continuous on [0,1], with only ~18% at the discrete 0 / 0.5 / 1 a
game result would produce. 5.9.4 therefore fitted 348 coefficients to reproduce
**Stockfish's static evaluation**, and the "holdout loss" it improved by 6.7%
measures *agreement with Stockfish*, not agreement with winning.

**Our own ledger already priced this: BAS-X02 — Stockfish distillation improved
holdout 4.9% and lost −17.11 Elo in Rarog.** That is the recorded failure mode,
and 5.9.4 walked into it at four times the scale. I described this corpus as
"Basilisk self-play WDL" throughout 5.9 without once checking the label format.
The quiet-filter account below remains true and independently verified — it
explains the mate-drive canary (BAS-E10) and the missing endings (BAS-E14) —
but it is the **secondary** factor, not the primary one.

*The secondary reason, and it matches 5.9.5's canary failure exactly.* The
corpus is also **quiet-filtered and off-policy**. Quiet filtering removes positions
with live attacks on the king — precisely where king safety governs — so the
objective cannot see whether king-safety values are right where they matter, and
the fit is free to set them wrongly at no measured cost. BAS-E10 found the same
blindness for mating positions, which the corpus lacks entirely. Two independent
manifestations of one defect: **the corpus does not contain the position classes
these terms exist to price.**

*A structural blindness worth recording separately.* The tuner builds with
`TEXEL_TRACE`, which **disables the lazy-eval skip** (`eval.cpp`, `#ifndef
TEXEL_TRACE`). It therefore fits the full evaluation and is constitutionally
unable to see that every centipawn it adds to the skipped block widens the error
`LAZY_MARGIN = 700` silently accepts. Measured here as harmless — sign flips
stayed at zero — but it is a real gap between what is fitted and what is played.

*Retry trigger.* Do not re-gate any fit produced from a quiet-filtered
off-policy corpus. 5.9.11 must regenerate on-policy **and** carry the position
classes the terms govern — sharp king attacks and mating material — before
5.9.12 refits.

**BAS-E12 — recovering the NPS the new terms cost** (2026-08-26, speed only,
behaviour-identical). Bench stays **18,228,447** across the change and CTest is
12/12, so this is a pure throughput measurement, not a behaviour change.

The 4.1% NPS that BAS-E11 attributes to the 5.9.1/5.9.2 term code was paid from
the moment those terms landed and was **never measured** — 5.9.1 and 5.9.2 both
recorded "bench unchanged", which is a *node-count* identity and says nothing
about speed. Three fixes, all exact rather than approximate:

1. `bishop_xray_pawn` called `bishop_attacks(sq, 0ULL)` per bishop per eval. The
   empty-board ray set is a per-square **constant**; tabled once in
   `init_eval_tables`.
2. `trapped_rook` recomputed `rook_attacks(rsq, b.all_occ)` for a square whose
   attack set the attack-map substrate already caches in `slider_att[]` — the
   same 8.7.7(a) substitution already made for the king-ring and connected-rook
   probes.
3. `slider_on_queen` did four magic lookups whenever the enemy held a queen,
   including when we held no slider that could be counted. Now gated on holding
   the piece, each half independently. Exact: the popcount was zero regardless.

NPS **3.208M → 3.299M, +2.8%**, leaving 1.8% against pre-5.9.1's 3.358M. The
tuner still reconstructs all 10,000 verification positions exactly.

*Not done, and why.* `bishop_outpost` sits **before** the lazy checkpoint, so it
is paid on every eval including skips. Moving it after would recover more, but
it would change the lazy score and therefore behaviour — that is an SPRT-gated
change, not a free one.

*Second pass: zero-guards, after reverting the values.* With 5.9.6 rejected and
every new coefficient back at 0, an **interleaved** measurement (BAS-M06
protocol; the earlier non-interleaved reading understated this) put the retained
term code at **−3.73%** NPS against pre-5.9.1 — speed spent on arithmetic
multiplied by zero. Six terms now skip when both their coefficients are zero,
via `EVAL_TERM_ACTIVE`. The macro is **always true under `TEXEL_TRACE`**: the
tuner needs a term's feature counts precisely when its coefficient is 0, since
that is the state it fits from, so gating the trace would break `--verify` and
the fit itself. Deficit **−3.73% → −1.37%**; `--verify` still exact on 10,000
positions.

The residual −1.37% is the standing price of retaining the structure for
5.9.12's ablation. If the terms still measure inert once the PSTs are free, they
should be removed and that comes back.

**BAS-E13 — the revert is provably the 1.9.3 engine** (2026-08-26). An SPRT can
only *fail to detect* a difference; this proves there is none. At fixed depth
the search is deterministic, so identical best moves **and** identical node
counts imply the same engine. Over the 107-position `suite_v1.epd` at depth 12,
Hash 64 both arms, against `basilisk-1.9.3-baseline-pext-pgo`: **0 best-move
mismatches, 0 node-count mismatches**, and bench **11,941,440** exactly.

*Conditional lesson.* A confirmation SPRT here would spend ~20k games failing to
detect a difference already proven absent. Where a change is claimed
behaviour-neutral, the deterministic identity check is both cheaper and
**strictly stronger** than a null SPRT, and should be preferred. Only the
−1.37% NPS delta is real, and at roughly 1 Elo it sits far below what this
harness resolves (BAS-M06; BAS-M01's ±10 Elo placement floor).



**BAS-E14 — the resign threshold is why the corpus had no mating material**
(2026-08-26, 2 × 300 self-play games, `5.9.11-datagen` binary, 8,000 nodes/move,
Hash 16, SuperGM_4mvs, seed 42; the only variable is adjudication).

BAS-E10 and BAS-E11 both blamed corpus coverage. This measures the mechanism
directly rather than inferring it.

| | standard (resign 600/3) | **none** |
|---|---:|---:|
| games reaching **checkmate** | 4 (**1.3%**) | 187 (**62.3%**) |
| reaching bare-officer material | 75 (25.0%) | 194 (**64.7%**) |
| reaching phase ≤ 2 | 79 (26.3%) | 125 (41.7%) |
| median plies | 116 | 138 |
| ended by adjudication | **209 / 300** | **0** |

**Resign-at-600 truncates the game roughly 22 plies before the material the fit
needs ever appears.** 209 of 300 games never finished. A corpus built this way
cannot contain bare-king endings, so a static fit is free to destroy mate-drive
and endgame behaviour at zero measured cost — which is exactly what BAS-E10
caught in `safety_table` and what BAS-E11 could not otherwise explain.

*Cost of the fix:* ~19% more plies and ~14% more wall time. `datagen.ps1` now
takes `-Adjudication standard|none`; "none" omits the flags entirely rather than
passing disabled ones.

*Caveat, so this is not over-claimed.* Positions immediately before mate are
checks and will still be dropped by the quiet filter — correctly, since a static
eval cannot price a position with a forced tactic pending. What this recovers is
the **approach** to mate: quiet K+Q-v-K and K+R-v-K positions, which is the class
mate-drive and endgame knowledge actually need. It also does **not** by itself
fix the sharp-middlegame gap; that is what the UHO slice is for.

**BAS-E15 — rounds above book size buy nothing; deterministic self-play repeats
verbatim** (2026-08-26, corpus-design failure, caught before the fit).

The first 5.9.11 corpus requested 180,000 games: 80,000 and 60,000 rounds
against `SuperGM_4mvs.pgn`, plus 40,000 against `UHO_Lichess_4852_v1.epd`.
Measured over the first 40,000 games: **2,668 distinct games — exactly the book
size — each repeated 15 times, 93.3% duplicates.**

Fixed-node self-play with the same binary on both sides is deterministic, so
replaying an opening reproduces the game bit-for-bit. `datagen.ps1`'s own
`-Rounds` documentation says as much ("one game per opening"), and the round
counts were still chosen without checking how many openings the book holds.
Cost: ~2.5 hours of generation of which only the 40,000-game UHO slice carried
independent information.

*Rates once duplication is removed* (3,000 UHO games per arm; dedup falls to
**0.2%**, against 59% on the duplicated corpus):

| phase | UHO standard | UHO **none** |
|---|---:|---:|
| opening | 6.971 | 7.015 |
| early_mid | 4.691 | 5.050 |
| middlegame | 4.348 | 4.887 |
| endgame | 3.460 | **5.105** |
| deep_endgame | 1.387 | **2.612** |

Adjudication-none **doubles** deep-endgame yield and lifts endgame 47% at
unchanged opening/midgame rates, independently confirming BAS-E14 in the units
that matter — positions per game rather than games reaching a class.

*Conditional lessons.*

1. **Never request more rounds than the book has openings** unless something
   else breaks determinism (varied node counts, differing binaries, randomised
   play). Check book size first; `datagen.ps1` prints `book_openings` to the
   manifest, and the preflight would have shown it.
2. **A raw-vs-unique gap is the detector.** 137,815 raw against 56,985 unique
   was visible in the very first preflight and is what exposed this. Treat a
   large gap as a corpus defect until explained, not as normal dedup.
3. **Adjudication-none should be the default for datagen, not a special slice.**
   It yields strictly more of the scarce classes at identical rates for the
   abundant ones, and it removes adjudication's label error, for ~19% more time.

*Revised design.* One slice: **UHO, adjudication none**, 250,000 rounds. UHO
carries 2,632,036 openings, so duplication is structurally impossible at any
practical round count, and it is the book our SPRTs use — so the eval is fitted
on the distribution it is judged on. The separate "general/sharp" split is
dropped: `SuperGM_4mvs` cannot supply more than 2,668 distinct games and cannot
be the bulk of any corpus. Recorded bias to watch: UHO is unbalanced by
construction, so the corpus skews toward decisive positions.

**BAS-E16 — Beast positions as STARTS with self-play labels; adopting Manta's
design after wrongly rejecting it** (2026-08-26).

Manta's `docs/HCE_DATAGEN.md` states the contract plainly: *"uses Manta
self-play results rather than an external evaluation oracle. The Beast file
supplies starts only. Every retained row receives the White-perspective result
of its own game."* They sample 1M starts from the Beast position file with a
per-pawn-family cap and play each one out.

**That was reviewed during the Manta import and rejected — wrongly.** The
recorded reason was that their corpus is smaller, labelled by a weaker engine,
and that reusing their FENs "would still require replaying them, which is the
expensive half — there is no saving." Every clause is true and the conclusion
does not follow: the question was never whether to import their *data*, it was
whether to adopt their *method*. Compute cost is identical either way, because
you play the games regardless. What the method buys is start diversity and phase
coverage, which are free.

*Measured, 3,000 games per arm, adjudication none, 8,000 nodes, same binary:*

| phase | UHO opening book | **Beast starts** |
|---|---:|---:|
| opening | 7.015 | 2.060 |
| early_mid | 5.050 | 3.036 |
| middlegame | 4.887 | 4.025 |
| endgame | 5.105 | **5.335** |
| deep_endgame | 2.612 | **3.018** |
| **games needed for a 1M-row target** | **173,036** | **116,528** |

An opening book forces every game to traverse an opening before it can reach an
endgame, so the scarce classes are paid for at the price of ~60 plies each.
Beast starts span all phases, so endgames are entered directly. The binding
constraint moves off `deep_endgame` — the class we are starved of — and onto
`opening`, which we have in abundance and value least. A third fewer games for
the same corpus.

`tools/texel/data/beast_seed_2m.epd` already holds 2,000,000 sampled Beast
positions, so no sampling pass is needed; `sample_fens.py` and `audit_starts.py`
are present if a fresh seed is ever wanted.

*Conditional lesson.* Judge an imported idea on its **method**, not on whether
its artifacts are worth copying. "No saving" was the wrong criterion — the right
one was "does this produce better data for the same cost", and it does.

*What survives from the old corpus.* Nothing of its labels. The Beast **file**
remains the right start source; the Stockfish **targets** are the thing to
discard, per the BAS-E11 correction and BAS-X02.

**BAS-E17 — 5.9.11 label-source experiment, REGISTERED BEFORE RUNNING**
(2026-08-26). Three corpora differing in **one variable — which engine's games
produce the WDL labels.** Everything else is held identical: the same
`beast_seed_2m.epd` starts, adjudication none, the same extraction parameters,
the same 348-parameter `scalars` fit that 5.9.4 used, the same holdout split.

| arm | label engine | nodes/move | throughput | draws | rounds |
|---|---|---:|---:|---:|---:|
| **A** | Basilisk 1.9.3 | 8,000 | 48.4 g/s | 41% | 125,000 |
| **B** | Stockfish dev-20260716 | 8,000 | 15.2 g/s | 44% | 125,000 |
| **C** | Basilisk 1.9.3 | **25,000** | 18.75 g/s | 45% | 125,000 |

*Why the fit is `scalars` (348) and not the full 1,116.* 5.9.4 fitted exactly
that group on distillation labels and 5.9.6 lost **−77.92 Elo**. Holding the
group fixed makes this a one-variable replication: if changing only the labels
recovers the loss, the BAS-E11 correction is confirmed directly. The full-surface
fit is 5.9.12's job, using whichever label source wins here.

*Arms are matched, verified by pilot rather than assumed* (3,000/3,000/1,500
games). Phase yields per game are near-identical — opening 2.060 / 2.039 / 2.104,
deep_endgame 3.018 / 2.972 / 3.178 — so equal rounds give equal corpora and the
comparison is not confounded by phase mix.

*The draw-collapse risk was measured and did not materialise.* Stronger play
should draw more and flatten label variance; draws move only 41% → 44% → 45%.
Arm C exists precisely to separate **label quality** from **label source**: it
raises Basilisk's own strength instead of importing another engine's, so if the
oracle's advantage is merely less-noisy outcomes, C captures it without the
transfer risk.

**Registered decision rule, fixed before any game is played.**

1. Each arm gets **one SPRT against the current head**, nElo `[0, 3]`,
   alpha = beta = 0.05, `3+0.03`, 1T, Hash 64, UHO — the standard gate.
2. The winning label source is the arm that **passes with the highest nElo point
   estimate**. A-vs-B is run only if attribution is still ambiguous after (1).
3. **If no arm passes**, the label-source hypothesis is *not* supported, and the
   next suspect is the extraction contract or the fit itself — not another
   corpus. Say so rather than iterating on data.
4. No arm is re-fitted, re-extracted or re-tuned after seeing its SPRT. A failed
   gate is not repaired by adjusting its own inputs.

*Pre-registered prediction (mine, recorded so it cannot drift):* all three beat
the head, because all three fix the distillation defect; **C > A > B** on
ordering. High confidence on "all three beat the head", low confidence on the
ordering. If B wins, the independence question in PLAN becomes live — shipping
an evaluation shaped by Stockfish's play is a different thing from being
inspired by its ideas.

*Cost:* ~4h50m of datagen (43 min + 2h17m + 1h51m), plus extraction, three
fits, and three-to-four SPRTs.

**BAS-E18 — 5.9.11 results: the labels were the defect, and the scalar surface
is saturated** (2026-08-27, arm B still running). Conditions per BAS-E17: same
starts, same extraction, same 348-parameter `scalars` fit, `3+0.03`, 1T, Hash 64
both arms, UHO. Baseline `5911-base` = current head, bench 11,941,440.

| arm | labels | Elo | games | outcome |
|---|---|---:|---:|---|
| **A** | Basilisk self-play @ 8k | **−2.85 ±3.11** | 20,096 | H0 accepted — **rejected** |
| **C** | Basilisk self-play @ 25k | **+1.00 ±2.11** | 44,800 | **no verdict** — stopped by decision |
| **B** | Stockfish self-play @ 8k | **−7.30 ±4.76** | 9,852 | H0 accepted — **rejected, worst arm** |
| *5.9.6* | *Stockfish **evaluations*** | *−77.92 ±15.32* | *1,292* | *H0 — rejected* |

**The diagnosis is confirmed.** Arm A differs from 5.9.6 in exactly one thing —
the labels are game results instead of Stockfish evaluations — and the same fit
of the same 348 parameters moves from **−77.92 to −2.85**. Changing only the
label source recovered roughly **75 Elo**. The BAS-E11 correction stands: the
distillation was the primary cause, not the quiet filter.

**And the scalar surface is saturated.** Neither Basilisk-labelled arm gains
anything. That is now four independent results agreeing: cycle 6 washed at
+1.37 ±5.21; BAS-E08's ablation found the added structure inert; arm A −2.85;
arm C +1.00. Clean labels stop the fit doing harm; they do not extract anything
further from those 348 coefficients. Arm C beating arm A by ~3.9 Elo is the
predicted direction for stronger labels, but far too small to matter.

**A stronger oracle's outcomes made the evaluation WORSE.** Arm B is the worst
of the three and the fastest to reject — 9,852 games in under two hours, against
four to eight hours for the Basilisk-labelled arms. Same starts, same
extraction, same fit, same row count; only the engine that played the games
differs.

*This settles the oracle question, and it settles it against the oracle.* The
argument for Stockfish labels was that stronger play makes an outcome a
less-noisy estimate of a position's true value. Arm B's wider W/L skew (34.9% /
24.2% against arm A's 33.4% / 27.7%) confirms the labels really were cleaner in
that sense — and it still lost 4.5 Elo to arm A. The counter-argument is what
held: **a label should reflect what OUR search can realize.** A position
Stockfish converts and Basilisk cannot is, for our purposes, not that won, and
fitting to it teaches the evaluation to value what the search cannot cash.
Evaluation belongs to the search that consumes it.

*Convenient corollary:* PLAN's independence constraint and the strongest
measured option agree here, so no trade-off had to be made.

*Registered prediction, scored honestly.* BAS-E17 predicted **all three beat the
head, ordering C > A > B**. The **ordering was exactly right** — C (+1.00) > A
(−2.85) > B (−7.30). The **"all three beat the head" half was wrong**: none did.
Recorded as a half-hit; the ordering intuition was sound, the level was not.

**Arm C carries a protocol deviation and must be reported as an estimate.** Its
SPRT never reached a boundary — LLR sat at **+0.03**, dead centre of
(−2.94, 2.94) — and it was halted at 44,800 games by maintainer decision so that
arm B could run. Never quote it as a pass or a fail. The ±2.11 interval is
tighter than most accepted verdicts in this project; what is missing is a
stopping-rule decision, not precision.

*Why it could not simply be left to finish.* `sprt.ps1` honours `-Games` only in
`fixed`/`calibrate` modes — line 476, `$rounds = if ($fixedSize) {...} else
{ 50000 }`. A **gainer**-mode run silently discards the cap and stops only on an
LLR boundary or the 50,000-round (100,000-game) hard stop. The run was launched
with `-Games 16000` believing each arm was capped near 3 hours; arm A resolved
in 3h45m only because it was decisive, and arm C was ~10 hours from stopping
with no prospect of deciding.

**Decision-rule outcome, applied as registered.** BAS-E17 rule 3: *"If NO arm
passes, the label-source hypothesis is not supported, and the next suspect is
the extraction contract or the fit itself — not another corpus."* No arm passed,
so **no further corpus work is warranted**, and the winning label source for all
downstream work is **our own self-play**, at the highest node count affordable
(arm C's 25k beat arm A's 8k by ~3.9 Elo).

The hypothesis needs splitting to be stated accurately, though. Labels were
decisively the cause of the *−77.92 damage* — arm A proves that at 75 Elo of
recovery. Labels are **not** a source of *gain*, because the surface they feed
is saturated. Both halves are true and they are not in conflict.

*Lessons.*

1. **A parameter that is accepted and ignored is a defect.** `sprt.ps1` now
   throws when `-Games` is passed in a mode that cannot honour it, and the
   runner no longer passes it.
2. **A neutral SPRT does not stop.** Budget one long arm per night, not three.
   Decisiveness, not the cap, is what makes an arm cheap.
3. **A stronger labelling engine is not a better labelling engine.** Measured,
   not argued: −7.30 against −2.85 for our own engine at the same node count.
   Do not revisit an external oracle for evaluation labels without new evidence.
4. **Stopping a run early is legitimate when it cannot decide, and must be
   recorded as such.** The reason to stop was that the informative arm was
   waiting behind an uninformative one — not that the numbers looked settled.

**BAS-E19 — 5.9.15 LTC probe: no depth story** (2026-08-27). `5911-armC`
against `5911-base` at **`10+0.1`**, `-Mode fixed` 6,000 games, 1T, Hash 64
both arms, UHO. A fixed-N estimate by design — the harness makes no accept or
reject decision.

| | TC | Elo | 95% CI |
|---|---|---:|---|
| STC | `3+0.03` | +1.00 ±2.11 | 44,800 games |
| **LTC** | **`10+0.1`** | **+0.29 ±5.46** | 6,000 games |

Both time controls agree: **neutral.** The hypothesis that arm C is genuinely
better but hidden by shallow search is **not supported**. The probe was sized to
detect a +10–15 Elo depth effect and found nothing remotely that large; the LTC
point estimate is if anything *lower* than the STC one.

*Do not extend this run, despite the harness suggesting it.* Doubling to 12,000
games narrows the interval to roughly ±3.9, which still cannot resolve +1 — and
we already hold a **tighter** measurement of the same candidate at STC (±2.11).
More LTC games would buy precision on a null result we have already established
more cheaply elsewhere.

*The harness's canned classification does not apply here and was not followed.*
The `fixed` mode prints an 8.6.8A accept-audit reading — "A = removal side; a
NEGATIVE estimate means removing the feature lost", straddle ⇒ "default a single
to KEEP". That instrument answers a **removal** question, where the candidate is
the *simplification* and keeping the feature is the conservative default. Arm C
is an **addition**. For an addition, a straddle around zero is not a reason to
keep; it is the absence of a reason to ship. Read the number, ignore the verdict
line.

*Conditional lesson.* BAS-M07 warns that fast-TC gains compress at longer TC.
The converse — that a neutral STC result hides an LTC gain — is a real
possibility and was worth 3h50m to check, but it did **not** hold here. An
evaluation change that is neutral at STC should not be assumed to be waiting for
depth. Retry trigger: a candidate whose *mechanism* is specifically depth-gated
(deep endgame recognisers, tablebase-like knowledge), not a general refit.

**BAS-E20 — 5.9.14 king-safety refit on game-result labels; the reshape is real
but far milder than the distilled corpus claimed** (2026-08-27,
`--tune-kingsafety`, 43 knobs, arm C corpus: 1,000,000 train / 52,632 holdout,
coordinate descent from step 8, K = 1.94497).

Converged in 55 passes, best holdout restored from **pass 38**. Holdout
**0.0793025 → 0.0791382**, **−0.21%**.

*The distilled corpus overstated this by roughly 5×.* 5.9.5 ran the same fitter
on `beast_sf` and reported **−0.99%**; on real game outcomes the same procedure
finds **−0.21%**. A corpus of Stockfish evaluations rewards reshaping king
safety to look like Stockfish's king safety, which is a much larger apparent win
than reshaping it to win games. Another instance of BAS-E11's lesson, now
visible inside a single instrument rather than across an SPRT.

*The table reshape is the same shape, a fraction of the magnitude:*

| index | 2 | 6 | 10 | 14 | 17 | 20 | 24 |
|---|---:|---:|---:|---:|---:|---:|---:|
| baseline | 6 | 55 | 111 | 191 | 203 | 302 | 407 |
| **5.9.14 (game results)** | 0 | 45 | 101 | 177 | **281** | 307 | **406** |
| *5.9.5 (distilled)* | *0* | *28* | *68* | *160* | *263* | *369* | ***597*** |

Both are convex — low end down, high end up. But 5.9.5 nearly halved the middle
and inflated the top by 47%; 5.9.14 leaves the top **unchanged** (407 → 406) and
trims the middle by ~8%. The extreme reshaping was substantially an artifact of
the labels.

*The rook finding reproduces; the queen finding does not.* `ks_unit[ROOK]`
**0 → 2** again, independently, on a different corpus with different labels —
that is now two-for-two and worth taking seriously. `ks_unit[QUEEN]`, which
5.9.5 also moved to 2, **stays at 0 here**. The earlier claim that both were
dead knobs was over-read from a single fit on bad data; only the rook survives
replication.

**Candidate health — the best profile of the whole phase.**

| check | result |
|---|---|
| CTest | **12/12** |
| mate-drive canary | **on=37** — *stronger* than baseline's 29 (5.9.5 broke it to 4) |
| bench | 11,941,440 → **12,844,350**, +7.6% (arms were ~+30%) |
| paired depth at 1M nodes | **+0.093 ply**, 26 deeper / 26 shallower / 55 equal |

Zero depth cost, unlike every 5.9.11 arm (−0.196). The canary moving *up* is the
direct confirmation that BAS-E14's corpus fix worked: the same fitter that
destroyed mating behaviour on a corpus with no mating positions now strengthens
it on one that has them.

*Standing caution.* None of this is Elo. Holdout deltas have failed to predict
Elo repeatedly in this project, and −0.21% is a small one. The gate decides.

**BAS-E21 — 5.9.14 ACCEPTED: +2.64 Elo, the first gain of Phase 5.9**
(2026-08-28). `basilisk-5914-ks-pext-pgo` (`a534b1ab87`, bench 12,844,350)
against `basilisk-5911-base-pext-pgo` (`8e8f681ddb`, bench 11,941,440).
`3+0.03`, 1T, Hash 64 both arms, UHO, concurrency 14.

**Elo +2.64 ±2.05, nElo +4.05 ±3.15, LOS 99.42%, LLR 2.97 → H1 accepted** over
46,864 games in 8h44m. Ptnml [1036, 5599, 9849, 5869, 1079], PairsRatio 1.05.

*The LLR drifted rather than wandered, and that was the tell throughout.*

| games | Elo | LLR |
|---:|---:|---:|
| 10,636 | +2.74 ±4.30 | +0.72 |
| 22,998 | +2.99 ±2.93 | +1.76 |
| 46,864 | +2.64 ±2.05 | **+2.97 → H1** |

The point estimate held across a 4× increase in N while the interval tightened
by half. Contrast arm C, which sat at LLR +0.03 for 44,800 games: a hovering LLR
means neutral, a drifting one means real, and the distinction was visible long
before either finished.

**Time-forfeit warning investigated and cleared.** The harness flagged 9 lines
reporting losses on time and said to investigate before trusting the result.
Seven actual forfeits: **candidate 6, baseline 1**.

Two findings, both of which clear it:

1. **They are clustered, not systematic.** Every forfeit falls between games
   10,700 and 15,138 — a 4,438-game span, **9.5% of the run, containing 100% of
   the forfeits**, with none before and none after. Under a uniform rate the
   probability of that is ≈ 0.095⁷ ≈ **7×10⁻⁸**. This is a transient host
   condition, not a property of the engine.
2. **They penalise the CANDIDATE, so the result is conservative.** Removing all
   seven moves the estimate from **+2.639 to +2.713** Elo — the forfeits cost
   the winning side 0.074 Elo. A defect that biases *against* the accepted arm
   cannot manufacture the pass.

*Watch item, not a blocker.* The candidate searches ~7.6% more nodes per depth,
so under host load it may sit marginally closer to the clock than the baseline.
Worth re-checking if forfeits appear in a future run of this binary; nothing here
supports it as a real effect.

**What this result is, and what it is not.**

It is the **only accepted change in Phase 5.9**, and it came from the **capped
non-linear king-danger funnel** — precisely where BAS-E07 located the reference's
advantage, and precisely the surface a linear Texel fit structurally cannot
reach. Both halves of that prediction held.

It is **not** a new feature. Nothing was added. The gain came from two
corrections: fixing the corpus (5.9.11) and re-running a fit that had been
discarded for the wrong reason (5.9.5 → 5.9.14). The candidate had existed since
5.9.5 and was blocked by a canary failure caused by corpus blindness, not by
evidence against it.

*Conditional lesson.* A candidate withdrawn because it broke a canary deserves
re-examination once the canary's cause is understood. 5.9.5's withdrawal was
correct on the evidence available; keeping it as an open retry rather than a
closed rejection is what made this gain reachable. Retry trigger: any candidate
withdrawn for a reason later shown to be an artifact of its inputs.

**BAS-E22 — 5.9.12 full-surface fit: the PSTs move, and the added terms are
refuted** (2026-08-28/29, `--tune texel` 1,116 params + `--tune-kingsafety` 57,
two iterations, arm C corpus). The run completed in ~42 minutes — the
king-safety stage was ~20 min per iteration, not the 90 I estimated — and
survived a Windows-update reboot that destroyed only the console output. The
final bake is verified landed: re-baking `5912_ks_it2.txt` reports **0 changed**.

| | value |
|---|---|
| holdout | 0.0791382 → **0.0787951**, −0.43% |
| bench | 12,844,350 → **20,005,943**, +55.8% |
| paired depth vs the 5.9.14 head | **−0.037 ply** (27 deeper / 31 shallower / 49 equal) |
| CTest | 12/12 |
| mate canary | on=**35** (5.9.14 head 37, pre-5.9.14 baseline 29) |
| `K` score-scale audit | 1.94497 → **1.9107**, +1.8% scale expansion |

*Score-scale audit passes.* `K` moved 1.8%, against 5.6% at 5.9.4. Our
centipawn margins are effectively 1.8% *less* aggressive than calibrated —
small, recorded, not repaired. Bench is again a poor proxy: +55.8% nodes costs
**no depth at all** at equal nodes, exactly as BAS-E09 established.

**THE BAS-E08 ABLATION IS SETTLED, AND IT REFUTES THE FROZEN-PST EXPLANATION.**

BAS-E16 argued the 5.9.1/5.9.2 terms measured inert because they compete with a
piece-square table frozen since Phase 4.7 that already holds their geometric
signal. 5.9.12 unfroze it. Zeroing the twenty term values on the finished
surface gives holdout **0.0787873** against **0.0787951** with them active —
**removing them is very slightly BETTER**, and the difference is inside noise
either way.

The fit's own verdict is the same: **12 of the 20 values are exactly 0**, with
`trapped_rook`, `threat_safe_pawn` and `bishop_outpost` all zero in both phases.

So the terms are genuinely redundant, not shackled. The BAS-E16 hypothesis was
reasonable and is now dead. **Disposition: remove them**, per PLAN 5.9.12's
stated rule. That is a separate simplification, not part of this gate —
bundling "refit 768 PSTs" with "delete 20 terms" would confound two changes.
Removing them also recovers the NPS the zero-guards cannot reclaim while eight
of the values are non-zero.

*What 5.9.12 is worth on its own.* A −0.43% holdout improvement from unfreezing
the largest remaining surface, at no depth cost. Holdout has repeatedly failed
to predict Elo here, so this is a candidate for a gate and nothing more.

*Winnable remains outside all of this, and can stay outside.* Its 7 parameters
are independent of both the gradient set and the king-safety descent, so they
can be fitted later without redoing any of 5.9.12 — a finite-difference
instrument added at any point applies on top of the current state.

**BAS-E23 — 5.9.13 ACCEPTED: +9.52 Elo from unfreezing the piece-square tables**
(2026-08-29). `basilisk-5912-full-pext-pgo` (`e7e6b61211`, bench 20,005,943)
against `basilisk-5914-ks-pext-pgo` (`a534b1ab87`, bench 12,844,350). `3+0.03`,
1T, Hash 64 both arms, UHO.

**Elo +9.52 ±4.66, nElo +14.58 ±7.14, LOS 100.00%, LLR 2.95 → H1 accepted** in
**9,092 games / 1h42m**. Ptnml [189, 1026, 1903, 1203, 225], PairsRatio 1.18.
No time forfeits. The fastest and largest acceptance of the phase.

*Phase 5.9 cumulative: **+2.64** (BAS-E21, king safety) and **+9.52** here, so
roughly **+12 Elo** against the head the phase started from.*

**Holdout loss is not merely noisy as a predictor — it has now inverted twice.**

| change | holdout | Elo |
|---|---:|---:|
| 5.9.4 refit on distilled labels | **−6.2%** | **−77.92** |
| 5.9.12 full-surface refit | **−0.43%** | **+9.52** |

A 14× smaller holdout improvement produced an 87-Elo swing in the opposite
direction. Any future reasoning that leans on a holdout delta to predict, rank
or justify a candidate should be treated as unsupported. Only the gate decides.

*What actually produced the phase's gains.* Both accepted changes came from
**fitting surfaces we already had, with the right instrument, on correct data**:

- the capped non-linear king-danger funnel, reachable only by coordinate
  descent, which a linear fit had nothing to say about (BAS-E21);
- the 768 piece-square tables, frozen since **Phase 4.7** and never touched by
  any fit in this phase until 5.9.12.

**Neither gain came from new evaluation features.** The sixteen terms added at
5.9.1/5.9.2 were measured inert three separate times — BAS-E08, then again with
the PSTs free (BAS-E22), with the fit itself zeroing 12 of their 20 values. The
phase's lesson is that our evaluation was **mis-calibrated, not
under-featured**, and that the expensive part was discovering which surfaces had
never been fitted at all.

*Immediate consequences.*

1. The refuted terms should be **removed** — a separate simplification gate
   (`-Mode simplify`), never bundled with a gain.
2. Coverage now stands at 1,173 of 1,190 fittable parameters. The 7 `winnable`
   params are the only reachable set still unfitted, and they need a
   finite-difference instrument; they are independent of everything in 5.9.12
   and can be done at any time.
3. A third fit iteration is untested. The two-iteration run moved holdout
   0.0791382 → 0.0787951; whether a third buys anything is a ~40-minute
   question, and given the table above, only a gate could answer it.

**BAS-E24 — removing the refuted terms; the expected NPS recovery did not
appear** (2026-08-29, candidate `eb717a7033`). Eight terms and their 16
parameters removed, along with the `EVAL_TERM_ACTIVE` guard macro, the
`EVAL_BISHOP_EMPTY` table and the geometry constants that served only them.

*Registry integrity verified rather than assumed:* slots **1,194 → 1,178**
(exactly −16), texel group **1,116 → 1,100**, groups 161 → 145, and
`--verify` **PASS** on 10,000 positions — the trace and the evaluator still
agree exactly, which is the check that would catch a half-removed term.
CTest 12/12, canary unchanged at on=35, bench **20,005,943 → 13,981,020**.

**I predicted removal would recover NPS. Interleaved, both PGO, it measures
−6.70% — slower, not faster.**

| | median NPS |
|---|---:|
| `5912-full` (terms present) | 3,233,900 |
| `5912-slim` (terms removed) | **3,017,099** |

*This is not a clean speed claim in either direction, and should not be quoted
as one.* NPS is nodes per second, and the two engines do not search the same
nodes: the slim tree is 30% smaller to the same depth, so its node
**composition** differs — proportionally more full evaluations, fewer cheap
interior nodes. BAS-M06 already recorded that a single PGO build is
insufficient for a small speed claim, and this is that situation exactly, with
the added confound of a changed tree. What it does establish is that the
"removal buys back throughput" argument is unsupported, so it should not appear
in the case for this change.

*The case for removal is therefore narrower than I first put it:* the terms
were measured inert three times, the fit set 12 of 20 values to zero, and less
evaluation code is worth having for its own sake. Whether it costs strength is
a question only the gate answers.

**BAS-E25 — term removal ACCEPTED as a non-regression; Phase 5.9 closes**
(2026-08-29). `5912-slim` (`eb717a7033`, bench 13,981,020) against `5912-full`
(`e7e6b61211`, bench 20,005,943). `-Mode simplify`, H0 elo ≤ −5, H1 elo ≥ 0.

**Elo +0.49 ±2.96, nElo +0.76 ±4.59, LLR 2.97 → H1 accepted** in 21,990 games /
4h06m. Ptnml [470, 2682, 4661, 2711, 471], PairsRatio 1.01.

The eight refuted terms are gone with no measurable cost. The LLR drifted
steadily (+0.07 → +1.22 → +2.97) rather than hovering, and the point estimate
moved *toward* zero as the interval halved — the profile of a genuinely neutral
change.

*Forfeit note, and it points the other way this time.* One game (4111) was lost
on time by **`5912-full`, the baseline** — so it handed the candidate a free
win rather than penalising it. Worth ~0.03 Elo on one game in 21,990, and the
verdict holds without it, but unlike BAS-E21 this forfeit is **not**
conservative and should not be dismissed with the same reasoning.

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

**BAS-D09 — search diagnostics re-measured after Phase 5.9; one is stale**
(2026-08-30). Every search diagnostic that 5.7's candidate ranking rests on was
taken on the **pre-5.9 evaluation**, which has since moved ~+12 Elo. Re-measured
on head `e763a52`, same suite and protocol.

| metric | pre-5.9 | current | verdict |
|---|---:|---:|---|
| first-move cutoff | 89.101% | 88.872% | **holds** |
| mean cutoff index | 0.2137 | 0.216 | holds |
| LMR applied | 36.115% | 36.008% | holds |
| LMR mean reduction | 2.354 | 2.337 | holds |
| LMR re-search | 1.744% | 1.798% | holds |
| LMR clamp-0 | 16.217% | 16.458% | holds |
| branching b(4–11) | 1.692 | **1.754** | holds directionally (ref 1.894) |
| **qsearch share** | **30.8%** | **35.1%** | **STALE — see below** |

*Ordering and LMR are unmoved.* A 12-Elo evaluation change left every ordering
and reduction statistic within 0.3 percentage points. BAS-D01's "ordering is
healthy" and the LMR picture that 5.4.3 refuted candidates against both stand.

**BAS-D03 is superseded.** Qsearch share moved **30.8% → 35.1%** (suite-wide,
depth 12, same protocol — a first single-position reading of 35.0% was
discarded as non-comparable before this was concluded). BAS-D03 concluded "ours
is *smaller* than the reference's 36–37%; not a width source". **We are now
inside that band.** The conclusion no longer follows; qsearch share is a matched
quantity, not a favourable outlier. Nothing in 5.7 depends on it, but any future
argument citing BAS-D03's *smaller* must use this row instead.

*Not re-measured, and why that is defensible:* BAS-O01/O04's oracle attribution
(+322.7 Elo search, 95.9%/4.1% split) holds **Basilisk's evaluation constant on
both arms** — the hybrid runs SF's search against our eval versus our search
against our eval. An evaluation improvement lifts both arms, so the *search*
contrast is structurally insensitive to it. Re-measuring costs 2,400 games and a
rebuilt bridge to move a 322-Elo finding by at most a few Elo. Deferred with the
reason recorded; re-open if a decision ever turns on the exact split.

**BAS-D10 — `singularQuietLMR` implemented; the reference's magnitude is wrong
for us by 8×** (2026-08-30, step 5.7.2).

*The semantics were nearly misread, and the misreading would have shipped dead
code.* The flag is reset **once per node**, not per move, and set when the TT
move proves singular — so it relaxes LMR for **every later move at that node**,
not for the extended move itself. The extended move is the TT move, ordered
first, so `searched < 2` blocks LMR from ever reaching it. "Reduce the extended
move less" is unimplementable; "reduce its siblings less because the position is
sharp" is the mechanism.

*Plumbing verified before magnitude:* at `LmrSingularQuiet = 0` the bench is
**13,981,020**, matching the head **exactly**, so the mechanism is inert when
disabled and fires only through the intended path.

*Magnitude swept with `sweep.py`* (107 positions, 300k nodes, Hash 64, paired):

| value | mean depth | paired Δ | better / worse |
|---:|---:|---:|---|
| 0 | 21.30 | — | — |
| 128 | 21.38 | +0.084 | 32 / 24 |
| 256 | 21.41 | +0.112 | 26 / 32 |
| **401** | **21.42** | **+0.121** | **32 / 23** |
| **1024** (the reference's `r -= 1`) | 21.08 | **−0.215** | 26 / 33 |

**Porting the reference's constant would have been a regression**, and a large
one — a full ply doubled the bench (13.98M → 29.47M) and broke a mate-distance
test. Our LMR mean reduction is 2.337 plies and our singular gate is one ply
earlier than the reference's, so a full-ply relaxation removes ~43% of the
reduction at every singular node. Shipped at **401**; 128/256/401 are within
noise of each other, so this is a plausible operating point, not a tuned one.

*Instrument note.* `run_bench()` takes no `SearchParams` and therefore ignores
UCI options entirely — a parameter sweep driven by `bench` returns the identical
node count for every value, which is what it did here before the mistake was
caught. This is correct for a fingerprint and useless for a sweep. `sweep.py`
sets options on a real `go nodes` search and is the instrument for this.

**BAS-D11 — 5.7.3 REFUTED: the reference's extension exclusivity fails our
tactical floor** (2026-08-30). Three measurements, one implemented change
reverted, no games spent.

*First, how often the stack the audit flagged actually happens* — instrumented
with three new diag counters, 107-position suite, depth 12:

| | count | share |
|---|---:|---:|
| interior nodes | 14,154,475 | |
| singular extension fired | 70,790 | 0.50% of interior |
| …of which **+2 double** | 37,884 | **53.52% of fired** |
| …at an in-check node | 22,940 | 32.41% of fired |
| …**both → the full 3-ply stack** | 17,412 | 24.60% of fired = **0.123% of interior** |

**The double-extension rate is the surprise: more than half of all singular
extensions take +2.** `singular_double_margin` is **4** on a range of 0–60, so
the double fires when `s_val < s_beta - 4` — barely stricter than the singular
test itself. A mechanism meant to mark "one move is overwhelmingly best" is the
common case.

*But tightening it buys nothing.* Swept on `sweep.py` (107 positions, 300k
nodes, paired): margin 12 → **−0.178** ply, 25 → −0.009, 40 → +0.000. The
permissive setting looks wrong and does not measurably cost depth. Not pursued.

**`double_ext_max` is dead code.** Capping it at 16 instead of the 200 default
gives **0 better, 0 worse, 107 same** — `ss->double_exts` never reaches 16, so
the Phase 6.4 path cap has never once bound. Recorded for 5.7.6.

*Then the audit's actual candidate*, landed behind an inert knob and swept:

| `SingCheckMaxExt` | paired Δ depth | WAC @ depth 6 | floor 130 |
|---|---:|---:|---|
| **2 — compose (current)** | — | **137** | pass |
| 1 — no double when in check | +0.009 | 132 | pass |
| 0 — exclusive (the reference) | **+0.065** | **124** | **FAIL** |

**The reference's semantics do not transfer, and the knob was removed.**
Exclusivity fails the WAC floor outright; the intermediate setting costs 5
solved positions to buy noise. The reason is structural: our check extension is
**per-node and unconditional**, the reference's is **per-move and gated on
discovery-or-SEE**. Removing our composition therefore removes strictly more
extension than removing theirs would. Composition stays.

**Durable lesson — depth-at-fixed-nodes is not sufficient for extension work.**
That instrument ranked exclusivity **best** at +0.065 ply. WAC caught it as a
tactical regression that fails a correctness floor. Extensions exist to find
forcing lines, and a metric that averages depth over quiet and tactical
positions alike cannot see that. **Any future extension candidate must clear WAC
as well as the depth sweep before it is considered for a gate.** The two
instruments disagreeing is itself the finding.

*Kept:* the three probe counters (`sing_fired`, `sing_double`, `sing_in_check`,
`sing_triple`), as the evidence base for anything revisiting this interaction.
Bench restored to 12,709,666, CTest 12/12, WAC 137/300.

**BAS-D12 — 5.7.2 reading, not a verdict** (2026-08-30). `572-sqlmr` against
`5912-slim`, stopped by decision at **24,956 games**: **Elo +1.49 ±2.77, nElo
+2.32 ±4.31, LOS 83.52%, LLR +0.51 (17.3%)**.

Stopped because it could not resolve: the LLR drift implied ~149,000 further
games to reach a boundary, past the 100,000 hard stop, with the nElo estimate
straddling the upper bound of 3 — the classic indifference-region case. The
45,000-game criterion set beforehand was revised for that reason, stated rather
than quietly dropped; running on would have narrowed the interval to ±2.05
without changing the disposition.

**Read as: not a regression, plausibly a small positive.** Consistent with its
+0.121 ply sweep and a 9% smaller tree. It is **kept provisionally and is NOT
accepted** — PLAN's 5.7.7 gates the surviving set as one integrated contract,
and a +1.5 Elo change is below what a single SPRT resolves economically. If
5.7.7 fails, 5.7.2 has no independent claim to survival and comes back out.

**BAS-D13 — 5.7.4 REFUTED: the reference's verification search is neutral, and
its apparent gain was an outlier artifact** (2026-08-30). Implemented behind an
inert knob, measured on both instruments, reverted. No games spent.

*Reach first.* The `tt_score >= beta` branch fires **25,124** times on the
107-position suite at depth 12 — **0.1775% of interior nodes**, comparable to
the 3-ply stack 5.7.3 examined.

*Depth at equal nodes, verify(1) minus current(0):*

| | value |
|---|---|
| mean | **+0.383** |
| **median** | **+0.000** |
| better / worse / equal | **27 / 25 / 55** |
| largest swings | **+11, +11, +9** — all trivial pawn/king endgames |

**The mean is an artifact.** Three positions such as `8/8/8/4k3/8/4P3/8/4K3` and
`7k/8/7K/7P/8/8/8/8` carry it: shallow-material endgames where depth is cheap and
a small pruning gain compounds into double-digit plies. The median is zero and
the better/worse split is a coin flip. Excluding mate runaways does not help
(+0.433) because these are not mate runaways.

*WAC:* **138 vs 137** — one position, noise, both far above the floor of 130.

**Disposition: keep our negative extension.** No broad gain by either
instrument, and the reference's form costs an additional search at every firing.
This is a design decision taken on evidence rather than a gap left unclosed:
ours post-dates `9587eeeb`, and nothing here argues for trading it.

*Method note — BAS-D11's lesson generalised.* 5.7.3 was caught by WAC after the
depth sweep endorsed it; 5.7.4 was caught by the **median and the better/worse
split** after the depth *mean* endorsed it. The recurring failure is trusting a
single aggregate. For search work: report mean **and** median **and** the
directional split, and clear WAC as well. Both refutations came free.

*Kept:* the `sing_ttbeta` counter, since the branch still fires and its reach is
the evidence base for anything revisiting it.

**BAS-D14 — 5.7.5 singular gate depth: the two instruments measure the same
trade-off from opposite ends, and WAC-at-fixed-depth is biased** (2026-08-30).
`singular_min_depth` parameterised (default 5, unchanged) and swept.

| gate | depth mean | median | better/worse | WAC @6 |
|---:|---:|---:|---|---:|
| 4 | −0.121 | +0.0 | 18 / 38 | **162** |
| **5 (current)** | — | — | — | 137 |
| 6 (the reference) | +0.308 | +0.0 | 33 / 23 | 131 |
| 7 | +0.318 | +0.0 | 40 / 24 | **130** (at the floor) |

Lowering the gate runs more singular searches: worse depth, dramatically better
WAC. Raising it does the reverse. One trade-off, read from both ends.

**WAC at a fixed shallow depth is structurally biased toward more extension, and
this is the measurement that exposes it.** At depth 6, a gate of 4 lets singular
fire at depths 4–6 instead of 5–6, so critical lines are extended and more
tactics resolve *within the fixed depth*. That is a mechanical consequence of
the protocol, not evidence of strength. The +25 WAC positions at gate 4 are
largely this effect.

*This refines BAS-D11 rather than overturning it.* WAC remains a valid **floor**
— a candidate that drops below 130 has broken something. It is **not** a fair
comparator between settings that differ in how much they extend, because it
rewards extension mechanically. 5.7.3's WAC drop was partly this bias too; the
conclusion there still holds because it broke the floor outright, which is a
floor question rather than a comparison.

**Disposition: gate stays at 5.** Neither instrument can decide a trade-off it
is biased on, and nothing here justifies games. `singular_min_depth` is kept as
a tunable — unlike the refuted knobs of 5.7.3/5.7.4, its alternatives are not
measured worse, merely undecided, which makes it legitimate SPSA material later.

**BAS-D15 — 5.7.6 dead-code removal, behaviour-neutral** (2026-08-30).

| removed | why |
|---|---|
| `check_ext_path_cap` | added inert for 5.4.4, which closed **REJECTED** (BAS-S16, −3.48 ±3.32) |
| `lmr_allow_check` | same; current policy (never reduce checking moves) hardcoded |
| `SearchStack::check_exts` | existed only to feed the removed cap |
| stale 5.4.4 doc block | described both as "awaiting an experiment" that had already failed |

| changed | why |
|---|---|
| `double_ext_max` **200 → 16** | at 200 the cap **can never bind**; BAS-D11 measured 16 as behaviour-identical across all 107 positions, so this converts a decorative valve into a real one at no measured cost |

*The distinction that governed this step:* a parked switch whose trial already
**failed** is residue and comes out — leaving it implies an avenue is open when
it is closed. A safety valve that never fires is different: it should be made
capable of firing, not deleted. Deleting `double_ext_max` would have removed the
only bound on a pathological double-extension chain; setting it to 16 gives that
bound teeth for the first time.

Verified: bench **12,709,666** unchanged, CTest 12/12, WAC 137/300.

**BAS-D16 — 5.8.2 aspiration instrumentation, and 5.8.3/5.8.4 REFUTED**
(2026-08-30). Nothing had ever counted the aspiration path, so none of cluster
E's candidates could be sized before implementing them.

*Measured*, 107 positions at depth 14 — **events per window, not percentages
of windows**:

| | count | per window |
|---|---:|---:|
| root iterations using a window | 1,066 | — |
| fail-low events | 544 | **0.51** |
| fail-high events | 910 | **0.85** |
| total re-searches | 1,463 | **1.37** |
| `delta >= 900` give-up | 9 | 0.008 |

**More than one full root re-search per iteration on average**, and the give-up
hatch does fire — rarely, but it is not dead code.

*Then the two window candidates, swept at 300k nodes:*

| config | re-searches | depth mean | median | better / worse |
|---|---:|---:|---:|---|
| current | 1,305 | — | — | — |
| **5.8.3** fail-low narrows beta | **1,342** | +0.131 | +0.0 | **18 / 19** |
| **5.8.4** delta growth /4 (ref-like) | 1,419 | **−0.243** | +0.0 | 20 / 33 |
| both | 1,515 | −0.009 | +0.0 | 21 / 25 |

**5.8.3 refutes its own rationale.** The argument was that pulling beta to the
midpoint makes the fail-low re-search cheaper. It makes re-searches **more
frequent** — 1,305 to 1,342 — because a tighter window simply fails again.
The depth split is 18/19, no direction, so the +0.131 mean is the BAS-D13
pattern again.

**5.8.4 is clearly worse.** The reference's slower growth (`delta/4 + 5` against
our `delta/2`) costs 0.243 ply on a 20/33 split and adds re-searches. Our faster
escalation is the better trade here.

**BAS-D17 — 5.8.5 fail-high depth reduction REFUTED, fails the floor**
(2026-08-30). The reference re-searches shallower after each fail-high
(`failedHighCnt`).

| | re-searches | depth mean | better/worse | **WAC** |
|---|---:|---:|---|---:|
| current | 1,305 | — | — | **137** |
| reduce | **1,450** | +0.477 | 33 / 24 | **119** |

**WAC 137 to 119 against a floor of 130** — a floor failure, not a comparison,
so BAS-D14's bias caveat does not rescue it (the bias runs *against* less depth
here anyway). Re-searches also rose. A root failing high is often a tactical
shot; searching it shallower misses it. The +0.477 mean is again outlier-carried
(+11, +10, +6).

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

**BAS-D18 — 5.7.2 ACCEPTED BY MAINTAINER DECISION, not by a gate**
(2026-08-31). `singularQuietLMR` at 401 ships on the reading recorded in
BAS-D12: **Elo +1.49 ±2.77, nElo +2.32 ±4.31, LOS 83.52%**, LLR +0.51 at
24,956 games when the run was stopped.

**This is a deviation from "SPRT proposes, SPRT accepts", and is recorded as one
rather than dressed up as a pass.** The justification is that the gate cannot
decide it: the LLR drift implied ~149,000 further games against a 100,000 hard
stop, with nElo straddling the upper bound of 3. Re-running reaches the same
non-resolving state after ~8 hours. Spending that to re-learn BAS-D12 is not a
better use of the machine than accepting a small positive on the evidence in
hand.

*What the evidence actually supports:* not a regression (LOS 83.5%), consistent
with its +0.121 ply sweep and a 9% smaller tree, and clearing CTest and the WAC
floor. What it does **not** support is a claim of +1.49 Elo as a measured gain
— the interval spans [−1.28, +4.26].

*Standing caution.* Any future cumulative Elo claim must treat 5.7.2's
contribution as **unmeasured**, not as +1.49. The 5.13 release gate compares the
accepted head against 1.9.3 directly, which prices it correctly without relying
on this number.

**BAS-E26 — 5.9.7 endgame recogniser inventory: rook endings dominate, and
PLAN's own ordering under-ranked them** (2026-08-31). Full table in
`analysis/endgame_inventory_v1.md`. 20,000 games from `armC_basilisk25k.pgn`
(25k nodes, **adjudication off**, so endings are played out), counting games
that reach a class at least once with ≤7 pieces.

| class | % of games | covered? |
|---|---:|---|
| **`KRPP-KRP`** | **11.32%** | **no** |
| **`KRP-KR`** | **9.09%** | **no** |
| `KR-K` | 6.64% | mate-drive |
| **`KRP-KRP`** | **6.50%** | **no** |
| **`KR-KR`** | **6.11%** | **no** |
| **`KRPP-KR`** | **5.62%** | **no** |
| `KP-K` | 4.63% | KPK bitbase |
| **`KPP-KP`** | **4.04%** | **no** |
| **`KPPP-KPP`** | **3.42%** | **no** |
| **`KBPP-KBP`** | **2.74%** | partial |

**Five of the top seven classes are rook endings, and we recognise none of
them.** No rook-ending scaling, no Philidor/Lucena notion, nothing beyond the
generic rook-behind-passer positional term.

*PLAN's ordering was close but not right.* It named `KRPKR` first — correct
as far as it goes — but **`KRPP-KRP` is more frequent still**, and the drawn
**`KR-KR` at 6.11%** was not on its list at all. Measuring instead of assuming
changed the target.

Second family: **multi-pawn endings** (`KPP-KP`, `KPPP-KPP`, `KP-KP`,
`KPP-KPP`). Our only exact pawn knowledge is the single-pawn KPK bitbase;
everything with two pawns falls back to generic evaluation.

**Stated limitation.** Frequency is necessary, not sufficient: a class we reach
often but already evaluate correctly needs no recogniser. Value is
frequency × *error*, and error is **not** measured here. The refinement —
bucket the holdout by these classes and compare static evaluation against the
game result per bucket — is the right input to 5.9.8 and is cheap, since the
data exists and the classes are now defined. Recorded so the next step does not
silently assume frequency alone justifies the work.

**BAS-E27 — frequency was the wrong target: our endgame evaluation is fine
almost everywhere, and badly wrong in ONE class** (2026-08-31). 5.9.7 ordered
the classes by how often games reach them. This measures where we are actually
**wrong**, by bucketing the 52,632-row holdout by the same canonical classes and
comparing our static evaluation against the real game result. Fitted
K = 1.90906, global loss 0.078806.

| class | n | loss | vs global | mean eval | mean result | bias |
|---|---:|---:|---:|---:|---:|---:|
| **`KBN-K`** | 604 | **0.22558** | **2.86×** | **+8563** | **0.549** | **+0.451** |
| `KPPP-KPP` | 187 | 0.08232 | 1.04× | +219 | 0.840 | −0.121 |
| `KPP-KP` | 248 | 0.06238 | 0.79× | +220 | 0.821 | −0.096 |
| `KR-KP` | 183 | 0.05820 | 0.74× | +351 | 0.669 | +0.155 |
| `KRPP-KRP` | 260 | 0.05293 | **0.67×** | +182 | 0.660 | +0.029 |
| `KRP-KR` | 310 | 0.04084 | **0.52×** | +226 | 0.705 | +0.023 |
| `KR-K` | 601 | 0.02169 | 0.28× | +679 | 0.946 | +0.014 |

**This inverts 5.9.7's conclusion.** The rook endings that dominate by frequency
are **not** where we are wrong — `KRPP-KRP` at **0.67×** and `KRP-KR` at
**0.52×** are *better* than our global average, and `KR-KR` does not even
reach the worst sixteen. Building rook-ending recognisers would have been work
against a non-problem, and frequency alone would have sent us there.

**`KBN-K` is the finding.** We score it **+8563 on average** — verified
directly at **+10,845** and **+11,123** on two textbook positions — and the
side holding bishop and knight scores **0.549**. A theoretically forced win
converts barely better than a coin flip, and the evaluation asserts near-victory
throughout. That is a 2.86× loss ratio and a +0.451 probability bias, both far
outside anything else measured.

*Important nuance for 5.9.8's design.* This is **not a missing recogniser**. We
classify KBNK correctly — the corner drive exists and `apply_endgame` scores it
as won. The failure is **conversion**: technique inside the fifty-move limit.
PLAN's 5.9.8 says implement classification only, because MAN-E05's post-mortem
was grading conversion with no recogniser able to say whether an ending was
winnable. **Our position is the exact opposite of Manta's**, and applying
Manta's remedy unchanged would build recognisers we already have while leaving
the measured defect untouched.

*Secondary, and all minor:* multi-pawn endings are **under**-estimated
(`KPPP-KPP` −0.121, `KPP-KP` −0.096) and `KR-KP` is over-estimated (+0.155),
but every one of these sits at or below global loss.

**BAS-E28 — 5.9.17: KBNK conversion measured directly, and nearly doubled**
(2026-08-31).

*Baseline, and it is far worse than the game statistic suggested.* Random legal
KBNK positions, engine playing **both** sides at 60,000 nodes:

| outcome | n=40 |
|---|---:|
| **mated** | **8 (20.0%)** |
| fifty-move draw | 25 (62.5%) |
| other / stalemate | 7 (17.5%) |

BAS-E27's 0.549 game result understated the defect, because positions arising in
games are often already part-converted. **When it does mate, the median is 53
plies against a 100-ply limit** — so the technique is not slow, it fails to
find the plan at all.

*Cause.* `kbnk_score` was `KNOWN_WIN + (7 − corner_dist) × 250 + (8 −
king_dist) × 30` — **both terms price only the two kings.** Nothing rewarded
bringing the knight to bear, and in KBNK the knight is what removes the weak
king's escape squares. The search had no gradient toward the piece the mate
depends on.

*Sweep* (n=30, then the two survivors re-run at n=120 on identical positions):

| `kbnk_knight_prox` | mated (n=30) | mated (n=120) |
|---:|---:|---:|
| **0** | 23.3% | **11.7%** (14) |
| **20** | 33.3% | **21.7%** (26) |
| 60 | 23.3% | — |
| 150 | 20.0% | — |

**n=30 could not resolve it** — 7 mates against 10 is barely one standard
error, and I did not treat it as a result. At n=120 on the same positions, 14
against 26 is roughly **3 SE**, and conversion nearly doubles. Shipped at 20.

*What this does NOT claim.* **21.7% is still bad.** A correct KBNK
implementation converts near 100%. This fixes a missing gradient term, not the
technique: our corner drive is a coarse Chebyshev distance where the standard
formulation uses a corner-distance table. The class remains open work, and the
Elo available from finishing it is larger than what this step banks.

*Correctness.* CTest 12/12, mate-drive canary intact, KNNK draws intact, WAC
137/300, tuner reconstruction exact. Bench unchanged at 12,709,666 — the bench
suite contains no KBNK position, which is itself worth knowing.

**BAS-E29 — 5.9.17 continued: KBNK conversion 13.0% → 54.5%, and WHY the old
drive failed** (2026-08-31). Matched comparison, identical 200 random legal KBNK
positions, engine playing both sides at 60,000 nodes:

| outcome | pre-5.9.17 | after |
|---|---:|---:|
| **mated** | **26 (13.0%)** | **109 (54.5%)** |
| fifty-move draw | 144 (72.0%) | 62 (31.0%) |
| other / stalemate | 30 (15.0%) | 29 (14.5%) |

**A 4.2× improvement, roughly 12 SE.**

**The mechanism is the finding, and it generalises beyond this ending.** The old
drive was `(7 − chebyshev_corner) × 250 + (8 − king_dist) × 30`. Two defects:

1. **Chebyshev distance has large plateaus** — a whole L-shaped band of squares
   shares one value — so over most of the board the potential was flat and
   there was nothing for the search to follow. Manhattan distance has the same
   minimum with far fewer ties.
2. **The gradient was smaller than our own pruning margins.** A king-distance
   term of **30cp per step** sits far below futility, razoring and RFP margins
   of roughly 100–500cp, so the search *pruned away the very moves that made
   progress*. This is why scaling every weight up together helped, even though
   `apply_endgame` returns `kbnk_score` as an override and a pure rescaling
   should be behaviour-neutral: it is not the ratios that matter, it is whether
   the differences survive the pruning thresholds.

*Weight sweep, n=60 screening:*

| corner / edge / king / knight | mated |
|---|---:|
| 250 / 0 / 30 / 20 | 18.3% |
| 200 / 200 / 100 / 40 | 21.7% |
| 400 / 500 / 100 / 100 | 38.3% |
| 500 / 600 / 150 / 150 | 48.3% |
| **800 / 900 / 220 / 220** | **66.7%** |
| 900 / 1000 / 250 / 250 | 63.3% |
| 1000 / 1100 / 280 / 280 | 63.3% |

The plateau is real and the ceiling is hard: at 1000/1100/280/280 the maximum
evaluation reaches **31,780** against a mate boundary of **MATE_SCORE −
MAX_PLY = 31,872**. Shipped at **800/900/220/220**, whose maximum is 27,420 —
verified at **+18,056** on a textbook position, comfortably clear.

*n=60 over-estimated it at 66.7%; the honest number is the n=200 figure of
54.5%.* The first sixty positions of the fixed seed are easier than average, and
the screening sweep is a ranking instrument, not a measurement.

**Still not solved, and the remaining failures are two different problems.**
31% still reach the fifty-move rule — technique too slow. **14.5% end in
stalemate or otherwise, essentially unchanged from 15.0%** — nothing here
addressed stalemate avoidance, which is a distinct defect and the obvious next
target.

*Correctness.* CTest 12/12, mate-drive canary intact, all three KNNK draw
assertions intact, the KNK non-trigger assertion intact, WAC 137/300, tuner
reconstruction exact on 10,000 positions. Bench unchanged at 12,709,666 — the
bench suite still contains no KBNK position.

**BAS-E30 — which forced-win endings actually need work, and why KBBK is worse
than KBNK** (2026-08-31). Conversion measured across every bare-king family that
is a theoretical win. Rook-and-pawn endings are excluded: they are not forced
wins, so a conversion percentage there is meaningless and BAS-E27's
eval-versus-result comparison is the right instrument for them.

*Python harness, 100 positions per family, 60k nodes, persistent TT:*

| family | mated | fifty-move | stalemate/other |
|---|---:|---:|---:|
| **`KQ-K`** | **100.0%** | 0% | 0% |
| **`KR-K`** | **100.0%** | 0% | 0% |
| `KBB-K` | 87.0% | 10% | 3% |
| `KBN-K` | 54.0% | 37% | 9% |

**The two most frequent bare-king endings are already perfect.** `KQ-K` (5.88%
of games) and `KR-K` (6.64%) convert 100/100, consistent with BAS-E27 putting
them at 0.57× and 0.28× global loss. **There is no Elo waiting in "all the
endgames"** — only KBNK and KBBK need anything.

*Then the CTest harness (5.9.18) disagreed sharply, and the disagreement is the
finding:*

| family | python (persistent TT) | CTest (fresh TT per move) |
|---|---:|---:|
| `KBB-K` | 87% | **3/12 = 25%** |
| `KBN-K` | 54% | 14/16 = 88% |

Two causes, both worth recording. The harnesses differ because `best_move()`
builds a **fresh transposition table for every move**, where a real game keeps
one across the playout — so the CTest instrument is deliberately harsher and
its numbers are not comparable to the python ones.

**But the ordering inversion is real and is not a harness artifact.** Two
bishops is a far simpler mate than bishop-and-knight, so KBBK converting *worse*
points at something structural: **KBBK has no recogniser at all.** It falls
through to the generic mate-drive, `5 × lk_center + (14 − king_dist) × 4`
— **5 and 4 centipawns per step**, which is the exact sub-pruning-margin defect
BAS-E29 diagnosed in KBNK, an order of magnitude worse. KQK and KRK survive it
only because the material is overwhelming enough that the search finds mates
directly.

**BAS-E31 — 5.9.18: randomised conversion floors added to CTest**
(2026-08-31). The existing endgame suite gated a handful of hand-picked EPD
positions, which is precisely why it stayed green throughout the period KBNK
converted 13% of random positions. It tests *evaluation direction*, not whether
the engine can finish.

Four randomised per-family floors, fixed-seed LCG so the position set is
identical every run and a failure is reproducible. Floors set well below
measured rates — they are floors like WAC's, not rate assertions, so ordinary
search churn cannot trip them:

| family | measured | floor |
|---|---:|---:|
| `KQ-K` | 12/12 | **12** (deterministic; must stay perfect) |
| `KR-K` | 12/12 | **12** (deterministic; must stay perfect) |
| `KBB-K` | 3/12 | 1 |
| `KBN-K` | 14/16 | 10 |

Runtime 48s, against 2m53s for a first version that used a fixed depth 18.

*Three defects in that first version, all found by running it:* it generated
**same-coloured bishop pairs**, which are a genuine draw, so 7/16 was measuring
the generator rather than the engine; depth 18 made the suite far too slow; and
a fixed depth conflated knowledge with search effort — KBB-K scored 2/12 at
depth 10 *and* at depth 14. Switching to a node limit matched the measurement
instrument.

**Standing instruction recorded in the test itself:** raise each floor when the
matching conversion work lands. A floor left at an old rate silently stops
protecting the improvement that replaced it.

**BAS-E32 — CORRECTION to BAS-E27: the per-class mean hid a systematic error
on the drawn subset** (2026-08-31). BAS-E27 compared our evaluation against the
game result per endgame class, found the rook endings *better* than global loss,
and concluded they needed nothing. **That conclusion was wrong, and the method
was the reason.**

A scaling function exists to recognise that a materially-winning position is in
fact **drawn**. A per-class mean cannot see that: if a class is 40% decisive and
we score those correctly, the mean looks healthy while every drawn position is
called a win. Splitting each class by actual game result:

| class | drawn share | we predicted | bias | reference has? |
|---|---:|---:|---:|---|
| **`KBN-K`** | 90.2% | **1.000** | **+0.500** | `KBNK` |
| `KRPP-KR` | 25.1% | 0.848 | +0.348 | — |
| **`KR-KP`** | 66.1% | **0.780** | **+0.280** | **`KRKP`** |
| **`KRP-KR`** | 59.0% | **0.671** | **+0.171** | **`KRPKR`** |
| `KBPP-KBP` | 84.7% | 0.639 | +0.139 | `KBPKB` family |
| **`KRPP-KRP`** | 61.9% | **0.638** | **+0.138** | **`KRPPKRP`** |
| `KRP-KRP` | 90.1% | 0.498 | ~0 | — |
| `KPP-KPP` | 65.7% | 0.489 | ~0 | — |

**We systematically score drawn rook endings as won**, and the classes where we
are wrong are precisely the ones the reference implements scaling functions for.
Where material is symmetric — `KRP-KRP`, `KPP-KPP` — we are accurate. It is
the **up-a-pawn** cases that fail: our evaluation sees +1 pawn and has no notion
that Philidor exists.

*Recorded as a method lesson, because it is the second time in this phase.*
BAS-E27 replaced a frequency ordering with an error ordering and that was an
improvement; it then used a **mean** where the quantity of interest lived in a
**subset**. Frequency → mean error → error on the sub-population the mechanism
targets. When evaluating whether a *recogniser* is needed, measure the
population that recogniser would fire on.

**BAS-E33 — the reference's endgame table against ours** (2026-08-31). Not
previously enumerated; 5.9.7 measured our corpus but never asked what a mature
engine implements. The reference carries **18** endgame functions and **we have
6**.

| | reference | Basilisk |
|---|---|---|
| verdicts | `KNNK` `KNNKP` `KXK` `KBNK` `KPK` `KRKP` `KRKB` `KRKN` `KQKP` `KQKR` | `KNNK` `KXK` `KBNK` `KPK` |
| scaling | `KQKRPs` `KRPKR` `KRPKB` `KRPPKRP` `KPsK` `KPKP` | KBP, OCB, insufficient |

**Missing and measured to matter:** `KRPKR` (9.09% of games, +0.171 drawn bias),
`KRPPKRP` (11.32%, +0.138), `KRKP` (2.38%, +0.280).

**BAS-E34 — 5.9.19, the generic bare-king mate drive** (2026-09-01). The drive
was `5*lk_center + 4*(14 - king_dist)`: **5 and 4 cp/step** against futility and
razoring margins of **128-243**, so the search pruned the king walk before it
paid. Replaced for minor-piece mates by a `kxk_score` override in the shape
BAS-E29 proved on KBNK — Manhattan corner potential (Chebyshev has wide
plateaus with no gradient to follow), an explicit edge term, and weights above
the margins: edge 900, corner 250, kings 300, on a `KNOWN_WIN` base.

| | before | after |
|---|---|---|
| **KBB-K conversion** | **3/12** | **12/12** |
| KQ-K / KR-K | 12/12 | 12/12 |
| KBN-K | 14/16 | 14/16 |
| drawn-ending bias | 22/60 | 22/60 |
| **bench** | 12,709,666 | **12,709,666** |

**The finding is the scoping, not the gain.** The first version overrode every
bare-king mate, Q and R included. It converted KBB-K equally well and cost
**+20.5% bench nodes** (12,709,666 -> 15,315,269) across all positions — a
search-efficiency regression that would plausibly have outweighed the endgame
gain, and one no endgame test would ever have shown. Restricting the override to
minor-piece mates returned bench to the baseline **byte for byte**.

Why the heavy case cost so much and bought nothing: BAS-E30 had already measured
KQ-K and KR-K at **100/100**. Those mates are solved by search inside the
horizon, so they never used the gradient; overriding them only replaced ordinary
scores (~950) with `KNOWN_WIN`-scale ones (~10950), which churned aspiration
windows everywhere a deep line touched a won ending.

*Generalisation.* A recogniser is worth adding only where **search cannot
already solve the class**. "The eval term is too small" and "the eval term
matters" are different claims, and BAS-E30's conversion table is what separates
them. The same test that justified the change for KBB-K refuted it for KQ-K.

*Also confirmed:* two same-coloured bishops cannot mate, so the pair is tested
by square colour, not by count — the old inline gate used `more_than_one()`.

**BAS-E35 — 5.9.21 closes empty: the KBNK stalemates are not a defect**
(2026-09-01). BAS-E29 reported **14.5%** in a bucket labelled *"other /
stalemate"*, and PLAN 5.9.21 treated that as a stalemate rate. It is not one.
New instrument (`tools/diag/kbnk_outcomes.py`): the bucket is split, the
denominator is Syzygy-validated, and each event is recorded by **ply** rather
than halfmove clock.

| outcome | n | share of confirmed wins |
|---|---:|---:|
| mated | 94 | 47.5% |
| **fifty-move draw** | **77** | **38.9%** |
| stalemate | 16 | 8.1% |
| piece lost to the bare king | 11 | 5.6% |

*(198 of the 200 generated positions are true wins; the other 2 are genuine
draws — random placement can trap a piece. The first 16, which the CTest floor
uses, are 16/16 wins.)*

**Both failure modes are artefacts of the fifty-move boundary, not defects.**

| event | ply, min-median-max |
|---|---|
| stalemate | 91 · **95** · 99 |
| piece lost | 98 · **100** · 100 |

**Zero stalemates below ply 80.** Every one lands within 1-5 plies of a
fifty-move draw the engine was going to take anyway, so it costs nothing; the
same holds for the piece giveaways at ply 98-100. The engine is not blundering
into stalemate — it is running out of clock and then behaving indifferently,
correctly, in a position whose result is already settled.

**So 5.9.21 has nothing to implement, and the piece-loss finding earns no step
either.** The single remaining KBNK defect is **technique speed**: 38.9% hit the
fifty-move rule (5.9.22).

*Two measurement traps this cost, both already on the record in this project.*
First, an aggregate bucket ranked a step that does not exist — the same error as
grading endgames by frequency (5.9.7) and trusting a mean over an even split
(5.7.4). Second, my own first instrument used the **halfmove clock**, which a
capture resets, so every piece loss looked like it happened at move 0; the ply
axis reversed the reading. A third instrument bug caught in passing: reusing one
engine process across 200 games let the TT carry over and the run was not
reproducible — `ucinewgame` per position fixed it.

*Also corrected:* my first reading of the ply-1 stalemate `6Bk/8/7K/8/6N1/8/8/8 w`
called it a blunder. It is a **drawn** position: black's only legal move is
Kxg8, every bishop move on the g8-a2 diagonal covers g8 and stalemates, and
everything else drops the piece to a bare king. Syzygy confirms. The engine
played it correctly.

**BAS-E36 — 5.9.22: the KBNK failures are STUCK, not slow; two remedies
refuted** (2026-09-01). Tablebase DTZ at every white-to-move node, over the 198
Syzygy-confirmed wins.

| outcome | n | median optimal DTZ | median plies spent | median DTZ left | moves losing ground |
|---|---:|---:|---:|---:|---:|
| mated | 94 | 47 | **47** | 1 | 22.1% |
| fifty-move | 77 | 52 | 100 | **46** | **47.7%** |

**The result is bimodal, and that kills the framing the step was written with.**
When we convert we do it in **1.0x optimal** — the technique is not slow. When we
fail we spend 100 plies to travel 6, and still have 46 to go. We are not slow,
we are **stuck**, and on those games nearly half our moves lose ground.

*Mechanism, from the move traces.* Progress is real at first, then the engine
shuffles the bishop — `Ba4 Bd7 Be8 Bb5 Be2 Bg4 Bd3 Bc4` — while DTZ oscillates
in a band and the white king barely moves. Every term in `kbnk_score` is a
function of the weak king, our king or the knight; **nothing depends on the
bishop**, so all bishop moves score *exactly* alike. The potential has a flat
maximum that is not mate, and escaping it needs a multi-move plan whose
intermediate steps do not raise the potential.

**Arm B — bishop proximity to the weak king, weight 300. REFUTED.**

| | baseline | arm B |
|---|---:|---:|
| mated | 94 (47.5%) | 88 (44.4%) |
| stalemate | 16 | 20 |
| piece lost at ply < 10 | **0** | **2** |

94 -> 88 is inside one SE (±3.6pp), so this is "no gain", not a proven loss. It
is rejected on the second row: pulling a long-range piece next to a bare king
gets it captured, at ply 6 twice, which never happened before.

**Arm C — the weak king's escape-square count, weight 400. REFUTED, and harder.**
Chosen because it is what a mating net actually tightens: it moves with every
piece including the bishop, reaches zero exactly at mate, and cannot be gamed by
parking the bishop next to the king (an undefended bishop is not in our attack
set, so its square *counts as a flight square* and the term gets worse). KBN-K
conversion fell **14/16 -> 9/16**, failing the CTest floor — about 3.8 SE, a real
regression, so it was rejected without spending the 198-position run.

*Hypothesis for why, not a claim:* minimising flight squares rewards confinement
that is stalemate-adjacent rather than mating, and it competes with the corner
drive instead of reinforcing it.

**5.9.22 stays open.** The diagnosis is solid and reusable; the two obvious
bishop features are now closed off. Bench was byte-identical on both arms — the
bench suite contains no KBNK position — so only the endgame instruments can see
this work at all, which is exactly why it needs `tools/diag/kbnk_outcomes.py`
rather than an SPRT.

**Missing and measured NOT to matter:** `KPKP` — our `KP-KP` and `KPP-KPP`
predictions are already accurate on the drawn subset. `KRKB`/`KRKN`/`KQKR`/
`KQKP`/`KNNKP` are each below 2% of games and are not proposed.

**BAS-D08 — per-iteration cost; there is no shallow target** (same runs,
2026-08-25). Cumulative counts hide where the cost is. Differencing them gives
the cost of each iteration on its own:

| depth | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| per-iteration ratio | 1.56× | 2.56× | 2.05× | 1.31× | 2.37× | 1.85× | 1.75× | 1.69× |

Our **depth-19 iteration itself** costs about 1.7× theirs. The cumulative ratio
sits near 1.9× because the per-iteration ratio is near 1.9× at every depth — not
because a shallow overhead is carried forward.

**Depths ≤6 are 0.205% of a depth-19 search.** Removing that entire band
outright would change nothing.

*Correction.* BAS-D06 and BAS-D07 both concluded that depths 2–6 were the place
to attack — first because the excess peaked there, then because a saving there
would "propagate unchanged". **Both readings were wrong.** The depth-4 peak of
4.38× is an artifact of cumulative accounting: at shallow depths the early
iterations are most of the total, so their ratio dominates it. The propagation
argument was worse — it inferred a mechanism from a constant ratio when the
constancy has the simpler explanation that every iteration costs the same
multiple.

*Disposition.* **5.14 yields no localized target.** The deficit is a uniform
per-iteration cost multiplier of roughly 1.9× at all depths, with no band where
work would pay disproportionately. Any attack has to make the whole search
cheaper per unit depth, which is what clusters 5.4–5.6 already failed to do. The
~2.5-ply discrepancy from BAS-D07 remains open and is now the only concrete
unexplained quantity left in this line of enquiry.

**BAS-D07 — deep-segment branching; corrects BAS-D05 and BAS-O04** (16 suite
positions, depths 11–19, Hash 64 every arm, 2026-08-25). Measuring only to depth
11 was a blind spot: games reach 15–25 plies and the ratio was still moving.

| depth | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Basilisk / oracle | 1.99× | 1.78× | 2.06× | 2.06× | 1.75× | 1.95× | 1.92× | 1.86× | 1.80× |

| segment | Basilisk | oracle |
|---|---:|---:|
| b(4–11) | 1.692 | 1.894 |
| **b(11–19)** | **1.570** | **1.590** |
| b(11–19) per-position median | 1.548 | 1.558 |

**Correction 1 — deep branching is EQUAL, not better for us.** BAS-D05
concluded "our per-ply growth is better than the reference's" from the 4–11
segment alone. Over 11–19, which is where games actually run, the two are
indistinguishable. The shallow advantage was a shallow-band artifact and does
not describe our asymptotic behaviour. **The ratio therefore plateaus at
~1.8–2.0× and never closes; the crossover projected from BAS-D05 does not
happen.**

**Correction 2 — BAS-O04's 12.07-ply gap is an outlier artifact.** It was a mean
over a distribution containing forced mates that ran to depth 100 on our side
and 245 on the oracle's; 10 of 105 positions hit depth ≥100. The **median gap is
4.00 plies** (Basilisk 13, oracle 17), not 12.07. Every prior statement of "12
plies shallower at equal nodes" overstates by roughly three times and is
corrected here. The paired-delta comparisons used elsewhere were never affected,
because pairing is robust to exactly this; only the absolute means were.

*Open and stated as such.* A flat ~1.9× node cost at b≈1.55 predicts a gap of
about **1.5 plies**, but the measured median gap is **4.0**. The two measures do
not reconcile and roughly 2.5 plies is unexplained. Candidate causes not yet
separated: the 16-position summed branching sample is dominated by its most
expensive positions and may not represent the 107-position median; and the two
engines may differ in when an iteration is reported complete. This is recorded
as an open discrepancy rather than resolved by choosing whichever number suits.

*Consequence.* The real deficit is smaller than recorded and is a **persistent
multiplier**, not a shallow-band transient. Since deep branching is equal, any
node saving achieved by depth ~11 propagates unchanged through the playing
range — which keeps the shallow band worth attacking, but for a different reason
than BAS-D06 gave.

**BAS-D06 — shallow-depth node cost, step 5.14** (16 suite positions, Hash 64
every arm, fresh process per depth, 2026-08-25;
`analysis/step514_shallow_cost.md`). Cumulative nodes to reach each depth,
Basilisk against SF search driving our own evaluator:

| depth | 1 | 2 | 3 | **4** | 6 | 8 | 11 |
|---|---:|---:|---:|---:|---:|---:|---:|
| ratio | 1.50× | 3.12× | 3.41× | **4.38×** | 3.45× | 3.18× | 1.99× |

**Extended to depth 19 by BAS-D07, which corrects the conclusion below: the excess does NOT keep decaying — it plateaus at ~1.9× and deep branching is equal, not better for us.**

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
| BAS-E08 | Frozen 770-position Syzygy truth baseline: accepted head `294a3e2` versus Stockfish `dev-20260716-ebcea3ef`, both at 60k nodes, 1T, 16 MB, engine TB disabled and no score adjudication. | **Baseline observation:** Basilisk converted 293/480 clean wins (61.04%) versus 389/480 (81.04%); paired matrix 277 both, 16 Basilisk-only, 112 reference-only, 75 neither. | The 81.04% is an attained reference result, not a theoretical or empirical ceiling: Basilisk may surpass it, and need not equal it to be accepted. The 405/480 paired union is stretch evidence, not one engine's performance. Largest deficits were KBP-K, KQ-KR, KNN-KP, KBN-K and KQ-KRP. | `tools/diag/endgame_ceilings_v1.json`; PLAN 6.0.b–c |

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
| BAS-X15 | Rarog `4aea0c7`/RAR-E10 replaced its coarse KBNK corner metric with a bishop-colour-selected weak-king diagonal potential. At 60k nodes KBN-K conversion moved **19.4% -> 96.9%**, KBB-K 78.0% -> 100%, and 61 rule-50 failures fell to zero. A near-1:1 diagonal/king balance was ineffective; strong corner-pull dominance was essential. | **6.1.b correction:** Basilisk's existing Manhattan-to-nearest-correct-corner term is algebraically the same diagonal potential plus a constant. No geometry was missing. The transferable question is its dominance over Basilisk's edge/king/knight pulls, owned by 6.1.c; no Rarog constant or bishop-position term is licensed. | **6.1.a–f**; `analysis/kbnk_diagonal_port_v1.md` |

Apart from the search-oracle result and the KBNK diagonal potential, the
cross-review found no additional high-value Rarog item missing from the current
Basilisk plan. The remaining items are already covered, contradicted by local
evidence, or deliberately postponed to the NNUE/scaling phases.

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
