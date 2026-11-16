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

#### RESULT (against `b84-on`; superseded by BAS-P40)

Idle host, 2026-11-15. Pools `b84-on` and `b89-a2` (`5b325e8`'s engine, A2 alone).
Exactness before the reads: `bench 13` 4,688,537 on Release, Tune, Diag and the
symbol PGO build; the cohort fingerprint on each; CTest 33/33 on Release and Diag;
`hot_path_calls.py` 0 unknown. Archives `tools/results/b89-a2-bench`,
`b89-a2-loaded`, `b89-a2-single`.

| Read | `b84-on` | `b89-a2` | Delta | 95% interval | Verdict |
|---|---:|---:|---:|---|---|
| pooled `bench`, step 1 | 2.3990M | 2.3767M | −0.93% | [−1.07%, −0.79%] | gain NO_CHANGE, no-regression FAIL |
| loaded cohort, 14 engines | 2.0977M | 2.0794M | −0.87% | [−1.23%, −0.52%] | loaded rule NOT_MET |
| cohort, one engine (reported) | 2.3917M | 2.3655M | −1.09% | [−1.43%, −0.76%] | — |

- The same offset on every workload and load, for two prefetches, is what a base
  that is not the head's would read. BAS-P40 reads the head's own pool and decides
  A2 against it; this table's verdicts are not used.
- **Disposition (with BAS-P40): REVERTED.** The base stood (BAS-P40), so this table's
  verdict holds: the pooled no-regression contrast fails and the loaded read is below
  0. Prediction bench +0.3% [0.0, +0.7] and loaded +0.6% [0.0, +1.5]: both missed,
  at about −0.9% on every workload.
