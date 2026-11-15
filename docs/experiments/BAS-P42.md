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
