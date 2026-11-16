# BAS-P46

**BAS-P46 — search parameters as constants outside Tune (B.8.9, item B2)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** outside `BASILISK_TUNE`, the X and F lines of the core kernel's, Cluster 2's and Cluster 3's tables declare `static constexpr int` members of `SearchParams`, so `kp.name` folds at every read site; the lines a test assigns stay fields (`b3_nmp_verify_depth`, `b4_qs_fut_base`, `b4_qs_fail_high_blend`, `b4_qs_move_count`, `b4_qs_see_margin`); the root's and Cluster 4's tables stay fields; the Tune build is unchanged. `negamax<NonPV>` goes from 8 `idiv` to 3 (`6172370`).
- **Prediction (packet §8, frozen).** Bench +0.5% [0.0, +1.2] (80%); P(kept) 0.50.
- **Arms.** `b89-b1` (the head before B2, with B1: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted, and the items kept since) against `b89-b2`, four
  final-PGO builds of the head with B2, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** Kept alone when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2). Otherwise, as an item that only removes, hoists or reorders work, it is kept under the bar when its step-1 estimate is −0.1% or above, and it then stands or falls with the sub-bar bundle's read (+0.5% or more with the lower bound above 0, eight builds an arm, six cycles). Below −0.1% at step 1 it is reverted.

#### RESULT

Idle host, 2026-11-15. Exactness before the read as BAS-P45's (Release, Tune, Diag and
the symbol PGO build at 4,688,537 and the cohort fingerprint; CTest 33/33 on Release
and Diag; `hot_path_calls.py` 0 unknown). Archive `tools/results/b89-b2-bench`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| step 1 | +0.94% | [+0.49%, +1.39%] | ACCEPT |

- **Disposition: KEPT on its own bar.** Prediction +0.5% [0.0, +1.2]: inside.
