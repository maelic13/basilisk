# BAS-P50

**BAS-P50 — the draw verdict from the node's own evaluation (B.8.9, item S1)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** `Evaluator::evaluate_undamped(b, verdict)` hands back the verdict its evaluation computed; where the node has just evaluated, `corrected_eval` uses it instead of `draw_verdict`, which evaluated the position again; a 0 read from the table still asks; a node check (Diag) holds the kept verdict to the query's. One out-parameter on the evaluator's entry; no term changes (`6f573fa`).
- **Prediction (packet §8, frozen).** S1 and S2 together +0.15% [0.0, +0.4] (80%), under D-17.
- **Arms.** `b89-b3` (the head before S1, with B1, B2 and B3: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted, and the items kept since) against `b89-s1`, four
  final-PGO builds of the head with S1, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** Kept alone when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2). Otherwise, as an item that only removes, hoists or reorders work, it is kept under the bar when its step-1 estimate is −0.1% or above, and it then stands or falls with the sub-bar bundle's read (+0.5% or more with the lower bound above 0, eight builds an arm, six cycles). Below −0.1% at step 1 it is reverted.

#### RESULT

Idle host, 2026-11-15. Exactness before the read as BAS-P45's (the node check live in
the Diag build's `bench 13` and cohort fingerprint). Archive `tools/results/b89-s1-bench`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| step 1 | −0.28% | [−0.62%, +0.07%] | NO_CHANGE |

- **Disposition: REVERTED**, below D-17's −0.1% floor at step 1. The saving is 0.8% of
  evaluations (§1.2: 607,864 of 73.5M); the read's interval holds −0.28% to +0.07%.
