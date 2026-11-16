# BAS-P58

**BAS-P58 — X0 at one engine, Hash 1 and Hash 256: the model's falsifier (B.8.10)**

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
- **Read.** Two one-engine cohort reads of `b810-x0` against `b89-final`, `--hash 1` then `--hash 256`, reported. Predicted (80%): Hash 1 −0.7% [−1.2, −0.3]; Hash 256 +0.9% [+0.5, +1.4].
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

Idle host, 2026-11-16, CPU 30, one engine. Archives `tools/results/b810-x0-hash1`,
`b810-x0-hash256`; no cycle disturbed.

| Hash | `b89-final` | X0 | Delta | 95% interval | Predicted (80%) |
|---|---:|---:|---:|---|---|
| 1 MiB | 2.6495M | 2.6225M | −1.02% | [−1.36%, −0.68%] | −0.7% [−1.2, −0.3] |
| 64 MiB (BAS-P56) | 2.5177M | 2.5318M | +0.56% | [+0.41%, +0.71%] | +0.8% [+0.4, +1.2] |
| 256 MiB | 2.4783M | 2.5062M | +1.13% | [+0.86%, +1.39%] | +0.9% [+0.5, +1.4] |

- **The model's falsifier did not fire:** at Hash 1 X0's delta is below its Hash 64
  delta (−1.02% against +0.56%), and it rises again at Hash 256. The gain grows with
  the table, so it is the probe's wait.
- **Cost and payoff, as the model reads them** (an inference, not a measurement of
  either part alone): at 1 MiB the table sits in the cache and the prefetch buys
  nothing, so the key's cost is about 1.0% of a node. The payoff is then about 1.6%
  at Hash 64 alone, 2.1% at Hash 256 and 2.3% under load at Hash 64; `bench`'s
  16 MiB table gives back about 0.2%.
