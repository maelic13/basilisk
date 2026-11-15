# BAS-P42

**BAS-P42 — the correction computed before the table probe (B.8.9, item A4)**

- **Registration.** 2026-11-15, B.8.9, before the reads; owed because A2 (BAS-P39)
  and A3 (BAS-P41) both failed (packet §6). Agent-run on the idle host.
- **Change.** In `negamax`, `correction_value(ss)` moves above `tt_probe` (`004b6f4`).
  It reads the board and the records at `ss - 1` to `ss - 4`, which the probe does
  not write, so the value is the same; its loads overlap the probe's. Not in
  quiescence.
- **Prediction.** The packet's §8 has no row for A4 (it is the fallback); none is
  frozen here after the fact.
- **Arms.** `b89-head` (the head before A4: A1 to A3 reverted) against `b89-a4`, four
  final-PGO builds of the head with A4, PEXT, clang 23.1.5, large pages on.
- **Reads.** As BAS-P38: the pooled `bench` read with its gain and no-regression
  contrasts; the loaded cohort read (gain); the same tool at one engine, reported.
- **Verdict rule (packet §4.1, frozen).** Kept when the pooled gain contrast
  ACCEPTs, or when the loaded read is +1.0% or more with its lower bound above 0
  and the pooled no-regression contrast PASSes. Otherwise reverted.

#### RESULT

Idle host, 2026-11-15. Pools `b89-head` and `b89-a4` (`004b6f4`'s engine). Exactness
before the reads as BAS-P38's. Archives `tools/results/b89-a4-bench`,
`b89-a4-loaded`, `b89-a4-single`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| pooled `bench`, six cycles | −0.15% | [−0.44%, +0.14%] | gain NO_CHANGE; no-regression PASS (step 1 EXTEND) |
| loaded cohort, 14 engines | −0.16% | [−0.44%, +0.12%] | loaded rule NOT_MET |
| cohort, one engine (reported) | −0.02% | [−0.38%, +0.34%] | — |

- **Disposition: REVERTED.** The correction's loads already overlap the probe's,
  or cost nothing on its path: no read moves.
