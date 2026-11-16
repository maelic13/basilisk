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

#### RESULT

Idle host, 2026-11-15. Pools `b84-on` and `b89-a1` (four final-PGO builds of the
head with A1, `8440f8b`, `bench 13` 4,688,537). Archives `tools/results/b89-a1-bench`,
`b89-a1-loaded`, `b89-a1-single`.

| Read | `b84-on` | `b89-a1` | Delta | 95% interval | Verdict |
|---|---:|---:|---:|---|---|
| pooled `bench`, gain, six cycles | 2.3947M | 2.3798M | −0.62% | [−0.83%, −0.41%] | NO_CHANGE |
| pooled `bench`, no-regression | | | −0.62% | [−0.83%, −0.41%] | FAIL (step 1 EXTEND) |
| loaded cohort, 14 engines | 2.0900M | 2.1188M | +1.38% | [+1.00%, +1.76%] | loaded rule MET |
| cohort, one engine (reported) | 2.3917M | 2.4087M | +0.71% | [+0.52%, +0.90%] | — |

No cycle of any read disturbed. Exactness before the reads: `bench 13` 4,688,537 on
Release, Tune, Diag and the symbol PGO build; the cohort fingerprint 24,421,195
`8bc7f3c8d1dde0d2` on each; CTest 33/33 on Release and Diag; `hot_path_calls.py` 14
hits, 0 unknown; the Diag counters reproduce the packet's counts exactly (`bench`
4,592,276 predicted and 110,206 corrected, 97.66%; the cohort 145,097,634 and
2,187,425, 98.51%); the kernel pin walks 26,426 cohort moves and 837,423 in all.

- **Disposition: REVERTED by the frozen rule.** The loaded read meets its bar, but the
  pooled read's no-regression contrast fails, and the rule keeps a memory item on the
  loaded read only with that guard passing.
- The prediction: bench +0.8% [+0.2, +1.6] missed (−0.62%); loaded +3.5% [+1.5, +6.0]
  missed low (+1.38%) and is above the falsifier's +1.0%, so H1 stands for the
  table. The game cohort alone gains +0.71%; `bench 13` alone loses. The item's sign
  depends on the workload, not on the load: Return 1 in the packet.
