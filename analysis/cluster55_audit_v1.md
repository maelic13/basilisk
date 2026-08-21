# Cluster 5.5 audit — static eval, TT and quiescence

**Date:** 2026-08-13. **Verdict: no candidate.** Every contract in this
cluster's scope is already equivalent to the reference or intentionally
different for a recorded reason. The engine is unchanged.

This is the lifted 5.3 item-7 inventory for this surface, run under the
re-audit's re-motivation: provenance correctness, the TT contracts selectivity
depends on, and qsearch — never previously examined.

## 1. Evaluation provenance — EQUIVALENT, and correct

Durable lesson 6 demands that raw evaluation, pruning evaluation and searched
bounds stay distinct. Basilisk already does this, at `search.cpp` ~1529:

| Value | Content | Consumers |
|---|---|---|
| `raw_static_eval` | evaluator output, or the TT's stored raw eval | **TT storage only** |
| `static_eval` | raw + correction history, clamped off mate range | `improving`, correction attribution, pruning gate |
| `eval` | `static_eval`, replaced by `tt_score` when the TT bound proves it tighter | **pruning decisions only** |

Two details worth noting as strengths rather than gaps:

- The TT refinement is clamped to non-mate scores, with the reason recorded in
  place: RFP returns `eval` directly, so an unclamped shallow mate bound would
  leak out as an unverified mate cutoff.
- Correction history is applied at probe time and never stored, so the TT
  holds raw values that stay valid as corrections evolve.

The reference at `9587eeeb` performs the same TT-bound refinement but has **no
correction history at all** — that mechanism postdates this revision. On this
contract Basilisk is ahead of its reference, not behind.

## 2. Qsearch provenance and structure — EQUIVALENT

Qsearch mirrors the main search exactly: TT probe with exact/alpha/beta
cutoffs, `raw_eval` from TT or evaluator, stand-pat = raw + correction,
clamped, then tightened by a TT bound when tighter, with the raw value stored.

Structure present: MVV plus capture history ordering with the TT move
preferred, delta/futility pruning on captured value plus promotion gain, SEE
pruning at a threshold, an extra SEE screen for late captures, and
`gives_check` guards on both prunes. In-check nodes are searched as evasions
with **no qsearch-depth cap** — deliberately, since a static eval of an
in-check position is not a valid bound and capping it masked mates in long
check chains, poisoning parent TT stores. A ply backstop bounds termination.

**Intentionally different:** qsearch quiet checks exist but are inert
(`qsearch_check_cap = 0`). Recorded reason: the "SF does this" claim tracked an
older Stockfish; `9587eeeb` restricts qsearch to captures and evasions, and the
`hcefinal` SPSA independently pinned the cap at 0. This matches the reference.

## 3. Qsearch share — measured, and ours is SMALLER

The re-audit flagged qsearch as 35% of our nodes and never compared. The
reference emits no qsearch counter, so one was added on a **derived branch,
`hybrid-diag`** — the frozen `hybrid` oracle stays at `01df815` with its
tournament binary untouched, which is the rule that branch exists to protect.

All three arms, `suite_v1.epd`, fixed 300,000 nodes:

| Arm | Qsearch share |
|---|---:|
| **Basilisk native** | **30.8%** |
| SF search + Basilisk HCE | 36.1% |
| SF search + SF HCE | 37.0% |

Our qsearch is **smaller** than the reference's, which spends a *larger* share
of its nodes there while still reaching 12 more plies. Qsearch is not a source
of wasted width, and the hypothesis that it might be is closed.

## 4. TT capabilities — one gap, already adjudicated

The entry is a dense 10-byte record — `key16`, `score`, `static_eval`,
`move16`, `depth8`, `flag_age8` with 2 bits of bound and 6 of generation —
three to a 32-byte cluster, lock-free by 8-byte payload alignment. Mature.

**Missing versus the reference: a persisted TT-PV bit.** It is not a free
addition: every bit of `flag_age` is spoken for, so it costs an age bit or a
wider entry, and PLAN 5.2 explicitly bars both "until a measured consumer needs
it". It has also already been tried — the 8.5.7 re-test measured **+51% nodes**
and found no good operating point through the LMR route, which is why
`lmr_tt_pv_adj` sits at a near-noise 23 against a reconstructed signal.

Retry trigger: a consumer other than LMR that demonstrably needs the persisted
bit. Not a new constant on the existing route.

## 5. Conclusion

Cluster 5.5 closes with **no candidate and no engine change**. That is a
finding rather than a failure: these contracts being sound is what makes the
remaining width attributable to cluster 5.6's mechanisms by elimination.

**Budget note.** PLAN's honesty clause says that if 5.5 and 5.6 both close
empty, re-open whether the remaining Phase-5 budget is better spent going to
NNUE. 5.5 is now one of those two. 5.6 carries a concrete, quantified anomaly —
history pruning firing 142 times in 15.1M interior nodes — so it has a real
target rather than a hopeful one. If it also closes empty, the clause should be
honoured rather than argued around.
