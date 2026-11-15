# BAS-P52

**BAS-P52 — the sub-bar bundle under D-17: B1 and S2 together (B.8.9)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **What.** The items kept under the bar by D-17 (packet §4.4): B1 (BAS-P45, +0.23% at
  step 1) and S2 (BAS-P51, +0.09%). The others of D-17's list: B2 and B3 cleared their
  own bar; B5 and S1 fell below −0.1% and were reverted.
- **Arms.** `b89-sub-off`, eight final-PGO builds of a throwaway revision, the head
  (`18f86a1`) with S2 and B1 reverted (`git revert 19e337c 1a4549a`, cited by that recipe), against
  `b89-sub-on`, eight final-PGO builds of the head; PEXT, clang 23.1.5, large pages on,
  full LTO with the inline threshold and ext-tsp, `bench 13` 4,688,537.
- **Read.** The pooled `bench` read, gain form, six cycles (`nps_read.ps1`, then
  `-Extend -Force` if step 1 decides, so the estimate is six cycles' as D-17 says).
- **Prediction.** §8's "S1 and S2 together +0.15% [0.0, +0.4]"; with B1's +0.5%
  [+0.1, +1.0] beside it. No new prediction is added.
- **Verdict rule (D-17, frozen).** The bundle stays when its six-cycle estimate is
  +0.5% or more with the lower bound above 0. Otherwise B1 and S2 leave together.
