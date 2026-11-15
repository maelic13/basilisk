# BAS-P45

**BAS-P45 — LMR's window division hoisted, LMP's halving (B.8.9, item B1)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** lmr_window_term's null-window quotient `KLIT(lmr_window_null) / root_delta_` becomes a member set in `root_search` (the PV form keeps its division), with a node check that it equals the quotient at every use; the count rule's `/ (2 - improving)` becomes `improving ? x : x / 2` (`1a4549a`).
- **Prediction (packet §8, frozen).** Bench +0.5% [+0.1, +1.0] (80%); P(kept) 0.50.
- **Arms.** `b89-c2` (the head before B1: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted) against `b89-b1`, four
  final-PGO builds of the head with B1, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** Kept alone when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2). Otherwise, as an item that only removes, hoists or reorders work, it is kept under the bar when its step-1 estimate is −0.1% or above, and it then stands or falls with the sub-bar bundle's read (+0.5% or more with the lower bound above 0, eight builds an arm, six cycles). Below −0.1% at step 1 it is reverted.

#### RESULT

Idle host, 2026-11-15. Exactness before the read: `bench 13` 4,688,537 on Release,
Tune, Diag (the node check live) and the symbol PGO build; the cohort fingerprint on
each; CTest 33/33 on Release and Diag; `hot_path_calls.py` 0 unknown.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| step 1 (`tools/results/b89-b1-bench`) | +0.79% | [−1.73%, +3.31%] | REPEAT_STEP_1: the base's cycle 1 disturbed (one build read 2.413M against 2.49M) |
| step 1, repeated (`b89-b1-r2-bench`) | +0.23% | [−0.19%, +0.65%] | EXTEND |
| step 2 | +0.23% | [−0.01%, +0.46%] | NO_CHANGE |

- **Disposition: KEPT UNDER THE BAR (D-17),** its step-1 estimate +0.23% being −0.1%
  or above; it stands or falls with the sub-bar bundle's read.
- Prediction +0.5% [+0.1, +1.0]: inside the interval's low half.
