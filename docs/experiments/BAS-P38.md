# BAS-P38

**BAS-P38 — the table prefetch from a predicted key (B.8.9, item A1)**

- **Registration.** 2026-11-15, B.8.9, before the reads. Agent-run on the idle host.
- **Change.** `Searcher::do_move` computes the child's key before `make_move`
  (`predicted_child_key`) and prefetches its cluster, then prefetches again after
  the make only when the made key differs (a promotion, an en passant capture, a
  castling, a double push that sets an en-passant square). `do_null_move`
  prefetches its exact key before the make. The kernel's three prefetches after
  the make leave. Quiescence's children are prefetched for the first time.
  Behaviour-neutral: a prefetch reads nothing the program observes.
- **Question.** Does fetching the child's cluster across the make, and at
  quiescence's children, shorten the probe's wait (packet §1.1, H1)?
- **Prediction (packet §8, frozen).** Bench +0.8% [+0.2, +1.6] (80%); loaded
  +3.5% [+1.5, +6.0] (80%); P(kept) 0.85. Falsifier: under +1.0% loaded with the
  instrument admitted means the probe's wait is not lead time, and the item
  returns to the packet.
- **Arms.** `b84-on` (B.8.5's head, BAS-P35's pool; its release code is the head's)
  against `b89-a1`, four final-PGO builds of the head with A1, PEXT, clang 23.1.5,
  large pages on.
- **Reads.**
  - The pooled `bench` read (`tools/nps_read.ps1`), two contrasts on the one pair:
    the gain form and the no-regression form.
  - The loaded cohort read (`tools/cohort_read.py run`, BAS-P37's instrument),
    gain form.
  - The same tool at one engine (`--engines 1`), reported, deciding nothing.
- **Verdict rule (packet §4.1, frozen).** Kept when the pooled read's gain contrast
  ACCEPTs; or when the loaded read is +1.0% or more with its lower bound above 0 and
  the pooled read's no-regression contrast PASSes. Otherwise reverted. If BAS-P37
  did not admit the loaded instrument, the pooled gain contrast alone decides.
