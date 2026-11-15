# BAS-P47

**BAS-P47 — the picker's stages fall through (B.8.9, item B3)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** `CoreMovePicker::next` falls from each stage into the next (`[[fallthrough]]`) instead of re-entering its `switch` after every stage change (1.87 dispatches a call); stages, order, conditions and `skip_quiets_` reads unchanged. The verify build (`VERIFY_PICKER`, `33ba08e`) runs the pre-pass picker beside it and agrees on `bench 13` and the cohort fingerprint; a planted change aborts it (`1e82067`).
- **Prediction (packet §8, frozen).** Bench +0.4% [0.0, +0.8] (80%); P(kept) 0.40. Falsifier with B4: both under +0.2% means H3 is wrong and B5 is not built.
- **Arms.** `b89-b2` (the head before B3, with B1 and B2: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted, and the items kept since) against `b89-b3`, four
  final-PGO builds of the head with B3, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** Kept alone when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2). Otherwise, as an item that only removes, hoists or reorders work, it is kept under the bar when its step-1 estimate is −0.1% or above, and it then stands or falls with the sub-bar bundle's read (+0.5% or more with the lower bound above 0, eight builds an arm, six cycles). Below −0.1% at step 1 it is reverted.
