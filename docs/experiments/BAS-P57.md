# BAS-P57

**BAS-P57 — B.8.10's arms under load: the decider (B.8.10)**

- **Registration.** 2026-11-16, B.8.10, before the read; BAS-P55 to BAS-P58 are
  registered together, after the arms' pools and before the first of them runs.
  Agent-run on the idle host.
- **Arms.** `b89-final` (B.8.9's head, `d17a62d`; the head's engine code is unchanged
  since) and three pools of four final-PGO builds, each arm alone on the head: `b810-x0`
  (`63b9420`, A1 as the packet's §6 states it), `b810-x2` (`898a337`, the predicted key
  and its corrected prefetch at quiescence's two loops only), `b810-x1` (`f7d208a`, a
  prefetch after the make in quiescence's two loops). PEXT, clang 23.1.5, full LTO with
  the inline threshold and ext-tsp, large pages on; every build reproduces `bench 13`
  4,688,537 and the cohort fingerprint 24,421,195 `8bc7f3c8d1dde0d2`.
- **Exactness before the reads:** `bench 13` on Release, Tune, Diag and the symbol PGO
  build of each arm; the cohort fingerprint on each; CTest 33/33 on Release and Diag
  (the allocation guard among them); `hot_path_calls.py` 0 unknown; A1's kernel pin
  for X0 and X2; the Diag counters reproduce B.8.8's counts for X0 (bench 4,592,276
  and 110,206; cohort 145,097,634 and 2,187,425) and count quiescence's own makes for
  X2 (cohort 18,357,926 and 402,322, the 18,760,248 of §1.2).
- **Read.** Three loaded cohort reads (`cohort_read.py run`, fourteen engines, BAS-P37), one per arm against `b89-final`, gain form, in the order X0, X2, X1.
- **Prediction (Amendment 1 §Am1.6, frozen, 80%).**

  | Arm | `bench` | One engine | Loaded | P(kept) |
  |---|---|---|---|---:|
  | X0 | −0.5% [−0.9, −0.2] | +0.8% [+0.4, +1.2] | +1.5% [+1.0, +2.1] | 0.80 |
  | X2 | −0.1% [−0.3, +0.1] | +0.4% [0.0, +0.9] | +0.8% [+0.2, +1.5] | 0.35 |
  | X1 | 0.0% [−0.2, +0.2] | +0.2% [−0.1, +0.5] | +0.3% [−0.1, +0.9] | 0.10 |
- **The keep rule (PROCESS *NPS*, Amendment 1 §Am1.4 and §Am1.6, frozen).** An arm
  passes when its pooled `bench` read ACCEPTs by the standing rule, or when all three
  hold: the decider (BAS-P57, +1.0% or more with the lower bound above 0), the guard
  (BAS-P56, estimate above −0.5% and upper bound at 0 or above) and the ceiling
  (BAS-P55, `bench` estimate above −1.0%). Of the arms that pass, the best loaded
  estimate is kept; a smaller arm (X1 before X2 before X0) instead when its loaded
  estimate lies inside the best one's interval. No arm passing closes the leaf
  NO_CHANGE. Falsifiers: X0's one-engine delta at Hash 1 (BAS-P58) not below its delta
  at Hash 64 (BAS-P56) keeps nothing and writes Return 2; X0's loaded read under
  +1.0% closes A1 (retry with D.2's table).

#### RESULT

Idle host, 2026-11-16, CPUs 4 to 30. Archives `tools/results/b810-loaded-{x0,x2,x1}`.

| Arm | `b89-final` | Arm | Delta | 95% interval | Decider | Disturbed |
|---|---:|---:|---:|---|---|---|
| X0 | 2.2012M | 2.2304M | +1.32% | [+1.00%, +1.64%] | met | none |
| X2 | 2.2019M | 2.2143M | +0.57% | [+0.14%, +0.99%] | not met | one cycle, absorbed |
| X1 | 2.1954M | 2.2011M | +0.26% | [−0.25%, +0.76%] | not met | one cycle, absorbed |

- **The keep rule.** X0 passes all three conditions (decider here, guard BAS-P56,
  ceiling BAS-P55); X2 and X1 fail the decider. X2's +0.57% lies outside X0's
  interval, so no smaller arm replaces it. **X0 is kept** (`63b9420`, re-applied on the head by
  `dd5a5d4`).
- Predictions: X0 +1.5% [+1.0, +2.1], X2 +0.8% [+0.2, +1.5], X1 +0.3% [−0.1, +0.9]:
  all inside. The falsifier "X0's loaded read under +1.0%" did not fire.
