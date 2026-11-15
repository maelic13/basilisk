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
