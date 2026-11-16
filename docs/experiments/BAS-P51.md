# BAS-P51

**BAS-P51 — root_ordering tested once per stage (B.8.9, item S2)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** `score_captures` and `score_quiets` called `root_ordering` per move, which returns 0 below the root; the root's inputs are now added in a second loop only when the picker has them; the verify build agrees on both fingerprints (`19e337c`).
- **Prediction (packet §8, frozen).** S1 and S2 together +0.15% [0.0, +0.4] (80%), under D-17 (S1 reverted, BAS-P50).
- **Arms.** `b89-b3` (the head before S2, with B1, B2 and B3: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted, and the items kept since) against `b89-s2`, four
  final-PGO builds of the head with S2, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** Kept alone when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2). Otherwise, as an item that only removes, hoists or reorders work, it is kept under the bar when its step-1 estimate is −0.1% or above, and it then stands or falls with the sub-bar bundle's read (+0.5% or more with the lower bound above 0, eight builds an arm, six cycles). Below −0.1% at step 1 it is reverted.

#### RESULT

Idle host, 2026-11-15. Exactness before the read as BAS-P47's (the verify build
agreeing at every node). Archive `tools/results/b89-s2-bench`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| step 1 | +0.09% | [−0.16%, +0.34%] | NO_CHANGE |

- **Disposition: KEPT UNDER THE BAR (D-17)**, its step-1 estimate at −0.1% or above;
  it stands or falls with the sub-bar bundle (BAS-P52).
- **Final: REVERTED with the bundle** (BAS-P52: B1 and S2 together +0.05%).
