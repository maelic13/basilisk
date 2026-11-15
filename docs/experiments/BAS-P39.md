# BAS-P39

**BAS-P39 — the continuation-correction cells prefetched before the make (B.8.9, item A2)**

- **Registration.** 2026-11-15, B.8.9, before the reads. Agent-run on the idle host.
- **Change.** In `Searcher::do_move`, before the make: prefetch
  `(ss - 1)->cont_corr->entry[pt][to]` and `(ss - 3)->cont_corr->entry[pt][to]`,
  `pt` the moved piece's type or the promotion's: the two cells the child's
  `correction_value` reads at its `(ss - 2)` and `(ss - 4)` rows. A sentinel row
  is prefetched harmlessly. Behaviour-neutral: a prefetch reads nothing the
  program observes.
- **Prediction (packet §8, frozen).** Bench +0.3% [0.0, +0.7] (80%); loaded
  +0.6% [0.0, +1.5] (80%); P(kept) 0.35.
- **Arms.** `b84-on` (the head before A2: A1 reverted; its release code is B.8.5's head) against `b89-a2`, four final-PGO builds
  of the head with A2, PEXT, clang 23.1.5, large pages on.
- **Reads.** As BAS-P38: the pooled `bench` read with its gain and no-regression
  contrasts; the loaded cohort read (gain); the same tool at one engine, reported.
- **Verdict rule (packet §4.1, frozen).** Kept when the pooled gain contrast
  ACCEPTs, or when the loaded read is +1.0% or more with its lower bound above 0
  and the pooled no-regression contrast PASSes. Otherwise reverted.
