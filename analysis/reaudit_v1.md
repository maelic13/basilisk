# Phase 5 re-audit — triggered by cluster 5.4

**Date:** 2026-08-13. **Trigger:** PLAN cluster discipline rule 7 — two failed
hypotheses in cluster 5.4 with no accepted gain.
**Scope:** re-audit 5.2 (harness) and 5.3 (inventory) against everything
measured since they closed.

## 1. The hypothesis I proposed at 5.4's close is refuted

On closing cluster 5.4 I recorded a working hypothesis: that tree width is a
*symptom* of evaluation quality — a search that cannot trust its margins must
stay wide — and that the value therefore sat in cluster 5.5 and the 5.9 HCE
track rather than in selectivity.

The oracle can test that directly, because it holds one side constant while the
other changes. Measured on `suite_v1.epd` at a fixed 300,000 nodes:

| Arm | Mean depth | EBF |
|---|---:|---:|
| Basilisk native — our search, our eval | 20.80 | 1.834 |
| SF search + **our** eval (oracle) | 32.87 | 1.468 |
| SF search + SF eval (control) | 33.38 | 1.459 |

| Attribution | Ply | Share |
|---|---:|---:|
| **Search** (our evaluator held constant) | **+12.07** | **95.9%** |
| **Evaluation** (SF search held constant) | +0.51 | 4.1% |

Swapping in an evaluator **+232.8 Elo stronger** (BAS-O02) buys **0.51 ply**,
and the paired split is 36 positions better against 42 worse — not even
consistently deeper. Evaluation quality is essentially *not* what makes our
tree wide.

**The hypothesis is wrong and is withdrawn.** Width is a search-policy
property, by a factor of roughly 23 to 1.

## 2. Reconciling that with BAS-S16

Cluster 5.4 targeted the right subsystem and used the wrong levers.

- Width is search policy (§1), so 5.4 aimed correctly.
- Yet narrowing our tree with our own knobs **lost** −3.48 ±3.32 Elo, on a tree
  28% smaller (BAS-S16), and scaling the reduction knobs did not move the
  metric at all (BAS-S14, non-monotonic in the knob).

The consistent reading: the reference is narrow **because its decisions are
better informed at the point of pruning**, not because its margins are more
aggressive. Turning our existing knobs harder does not reproduce that. It
prunes the *same* decisions more, which is simply blindness — and games priced
it accordingly.

This is durable lesson 5 in its sharpest form yet: our width is close to
correctly priced *for the decision quality we currently have*. Buying depth
requires new information at the decision point, not a bigger constant.

## 3. What this changes in the 5.3 inventory

| 5.3 item | Was | Now | Why |
|---|---|---|---|
| 1 reduction modulation | rank 1 | **closed** | BAS-S13/S14/S15. Magnitude is not a lever; retry trigger recorded, do not reopen on a new constant. |
| 2 eligibility scope | rank 2 | **closed with 3** | Rejected inside BAS-S16. |
| 3 check extension | rank 3 | **closed** | BAS-S16, −3.48 ±3.32. |
| 4 history pruning dead | rank 4 | **rank 1** | 142 fires in 15.1M interior nodes. A whole width mechanism contributing nothing, in the family that owns 96% of the gap. |
| 5 move ordering | equivalent | unchanged | BAS-D01, no work. |
| 6 reduction context we lack | rank 5 | **demoted** | These are modulation terms, and modulation is closed. |
| 7 **not yet inventoried** | deferred | **rank 2 — now the main body of work** | The uninventoried selectivity, qsearch and TT contracts are where the remaining width lives, by elimination. |

Item 7 was deferred at 5.3 on the reasoning that those surfaces were about to
move under cluster 5.4. **Cluster 5.4 changed nothing**, so that reason has
expired and the deferral must be lifted rather than renewed.

## 4. What this changes in the cluster order

**5.6 (main selectivity) gains, it does not lose its slot.** At 5.4's close I
flagged that 5.6 might be redundant because its levers looked inert. That was
the refuted hypothesis talking. Selectivity is where the uninventoried width
mechanisms live, and §1 says width is 96% search policy.

**5.5 is retained but re-motivated.** Its value is no longer "fix the margins'
information quality" — §1 kills that. It is retained for provenance
correctness (durable lesson 6), for TT contracts that selectivity depends on,
and because **qsearch is 35% of all nodes** (8.09M of 23.2M) and has never been
inventoried against the reference at all.

**Order is unchanged: 5.5 then 5.6.** 5.6's preconditions genuinely require
5.5 — every margin is measured against a pruning evaluation, and separation of
raw / pruning / searched values has to exist before margins mean anything.

## 5. Our mechanism populations, for the 5.5/5.6 audit

From `baseline_v1.json`, per interior node:

| Mechanism | Fires | Share |
|---|---:|---:|
| move-count pruning (per move) | 22,211,538 | 146.7% |
| check extensions | 2,398,982 | 15.8% |
| reverse futility | 2,148,693 | 14.2% |
| futility | 1,032,317 | 6.8% |
| razoring | 872,839 | 5.8% |
| SEE pruning | 667,957 | 4.4% |
| null-move cuts | 317,065 | 2.1% |
| ProbCut | 56,311 | 0.4% |
| **history pruning** | **142** | **0.001%** |

ProbCut at 0.4% and history pruning at 0.001% are the two obvious anomalies.
Neither is evidence of a defect on its own — a mechanism may be correctly rare
— but both are unexamined, and one of them is indistinguishable from absent.

## 6. What 5.2's harness cannot yet answer

The harness measures **our** counters well and the identity check keeps it
honest. Its blind spot is that it cannot attribute the *reference's*
narrowness: Stockfish emits no equivalent counters, so every cross-engine
comparison reduces to depth-at-equal-nodes.

That was sufficient while the question was "how wide are we". It is not
sufficient for "which mechanism closes the gap", which is the question the
remaining clusters ask. Two options, neither free:

1. Instrument the frozen oracle with matching counters. It is our own build, so
   this is possible — but it edits a branch that is deliberately frozen, and
   the counters would have to mean the same thing on both sides to be
   comparable at all.
2. Accept per-mechanism A/B on our side only, using depth-at-equal-nodes as the
   cross-engine yardstick and our counters purely to explain our own arm.

Option 2 is what 5.4 used. It was adequate to *reject* — and rejecting cheaply
is most of the value. Decide between them at the start of 5.5 rather than
drifting into option 1 by habit.

## 7. Conclusions

1. Width is search policy, ~96/4 against evaluation. My 5.4-close hypothesis is
   withdrawn.
2. Cluster 5.4 aimed at the right subsystem with the wrong levers; magnitude
   and check-depth are both closed with recorded retry triggers.
3. The deferral of 5.3 item 7 has expired and is lifted. The uninventoried
   selectivity/qsearch/TT contracts are the remaining work.
4. Cluster order 5.5 → 5.6 stands, with 5.6 promoted in expected value and 5.5
   re-motivated on provenance and qsearch rather than on margin quality.
5. History pruning being effectively absent is the single most concrete
   anomaly and enters 5.6 as its first candidate.
6. Phase 5 has now spent three sub-steps for no strength. That is within the
   plan's tolerance — it is what the stop rules are for — but if 5.5 and 5.6
   also close empty, the honest move is to re-open the question of whether the
   remaining Phase-5 budget is better spent going to NNUE, not to invent a
   fourth cluster.
