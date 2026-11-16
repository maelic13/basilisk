# BAS-P41

**BAS-P41 — the structure-correction entries prefetched after the make (B.8.9, item A3)**

- **Registration.** 2026-11-15, B.8.9, before the reads. Agent-run on the idle host.
- **Change.** In `Searcher::do_move`, after the make: prefetch the child's four
  structure-correction entries, `pawn_corr`, `minor_corr` and both `nonpawn_corr`
  rows, for the side now to move, at the board's new keys (`7699164`).
  Behaviour-neutral: a prefetch reads nothing the program observes.
- **Prediction (packet §8, frozen).** Bench +0.2% [−0.2, +0.6] (80%); loaded
  +0.5% [−0.2, +1.3] (80%); P(kept) 0.30.
- **Arms.** `b89-head` (BAS-P40's pool of the head before A3: A1 and A2 reverted)
  against `b89-a3`, four final-PGO builds of the head with A3, PEXT, clang 23.1.5,
  large pages on.
- **Reads.** As BAS-P38: the pooled `bench` read with its gain and no-regression
  contrasts; the loaded cohort read (gain); the same tool at one engine, reported.
- **Verdict rule (packet §4.1, frozen).** Kept when the pooled gain contrast
  ACCEPTs, or when the loaded read is +1.0% or more with its lower bound above 0
  and the pooled no-regression contrast PASSes. Otherwise reverted.

#### RESULT

Idle host, 2026-11-15. Pools `b89-head` and `b89-a3` (`7699164`'s engine). Exactness
before the reads as BAS-P38's (Release, Tune, Diag and the symbol PGO build at
4,688,537 and the cohort fingerprint; CTest 33/33 on Release and Diag;
`hot_path_calls.py` 0 unknown). Archives `tools/results/b89-a3-bench`,
`b89-a3-loaded`, `b89-a3-single`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| pooled `bench`, six cycles | −0.31% | [−0.64%, +0.01%] | gain NO_CHANGE; no-regression PASS (step 1 EXTEND) |
| loaded cohort, 14 engines | −0.44% | [−1.55%, +0.68%] | loaded rule NOT_MET (one cycle disturbed, absorbed) |
| cohort, one engine (reported) | −0.33% | [−0.62%, −0.05%] | — |

- **Disposition: REVERTED.** Neither the gain contrast nor the loaded rule is met.
- Prediction bench +0.2% [−0.2, +0.6] and loaded +0.5% [−0.2, +1.3]: both missed low.
  With A2 also reverted, A4 is owed (packet §6).
