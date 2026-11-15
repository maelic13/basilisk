# BAS-P40

**BAS-P40 — the head's own pool, and A1 and A2 read again against it (B.8.9)**

- **Registration.** 2026-11-15, B.8.9, after BAS-P38 and BAS-P39's reads and before
  this one. Agent-run on the idle host.
- **Why.** BAS-P38 and BAS-P39 read A1 and A2 against `b84-on`, the pool of
  `60db617`, taken as the head's because the head's Release code differs from it
  only in Diag counters. That premise is in doubt: `ee6312d` added three counters to
  `DiagCounters`, which every build carries inside `SearchState`, so the search
  stack's and buffers' offsets moved in Release as well; and A2, two prefetches,
  read −0.93% on `bench` and −0.87% under load, the same offset in both reads, which
  two prefetches do not explain. Those two reads were made against a base that was
  not the head before the item, so they are read again here; their archives stay as
  they are, and their verdicts are replaced by this entry's.
- **Arms.** `b84-on` (`60db617`); `b89-head`, four final-PGO builds of `2bc4d2b` (the
  head with A1 reverted and before A2: B.8.5's head with the tools and documents of
  this leaf), by `nps_pool.ps1 -Revision`; `b89-a1` (BAS-P38's pool, `8440f8b`'s
  engine); `b89-a2` (BAS-P39's pool). All PEXT, clang 23.1.5, large pages on,
  `bench 13` 4,688,537.
- **Reads.**
  - One pooled `bench` read of the four arms, contrasts: `head` = `b84-on` →
    `b89-head` (no-regression, reported as the base check); `a1`, `a1-noreg` =
    `b89-head` → `b89-a1` (gain and no-regression); `a2`, `a2-noreg` = `b89-head` →
    `b89-a2` (gain and no-regression).
  - Loaded cohort reads, one per pair: `b84-on` → `b89-head` (reported),
    `b89-head` → `b89-a1`, `b89-head` → `b89-a2`.
  - The one-engine cohort read of the four arms, reported.
- **Verdict rules.** A1 and A2 by the packet's §4.1, as BAS-P38 and BAS-P39 froze
  them, against `b89-head`. The base check decides nothing about an item; if `head`
  reads within ±0.5% the earlier base stood and the earlier verdicts are confirmed
  by these reads; otherwise these reads decide.

#### RESULT

Idle host, 2026-11-15. `b89-head` built from `2bc4d2b` (`bench 13` 4,688,537).
Archives `tools/results/b89-p40-bench`, `b89-p40-loaded-{head,a1,a2}`,
`b89-p40-single`.

| Contrast | Read | Delta | 95% interval | Verdict |
|---|---|---:|---|---|
| `b84-on` → `b89-head` | pooled `bench`, six cycles | −0.03% | [−0.24%, +0.18%] | PASS (base check) |
| | loaded | −0.06% | [−0.49%, +0.36%] | reported |
| | one engine | −0.02% | [−0.41%, +0.37%] | reported |
| `b89-head` → `b89-a1` | pooled `bench`, six cycles | −0.43% | [−0.56%, −0.30%] | gain NO_CHANGE; no-regression PASS (step 1 EXTEND) |
| | loaded | +1.54% | [+1.11%, +1.97%] | loaded rule MET |
| | one engine | +0.81% | [+0.46%, +1.16%] | reported |
| `b89-head` → `b89-a2` | pooled `bench`, six cycles | −0.82% | [−0.95%, −0.69%] | gain NO_CHANGE; no-regression FAIL |
| | loaded | −1.09% | [−1.44%, −0.74%] | loaded rule NOT_MET |
| | one engine | −1.03% | [−1.38%, −0.69%] | reported |

The one-engine read's sixth cycle was disturbed in every arm (the host's level fell
1%), absorbed; no other cycle disturbed.

- **The base stood.** The head's own pool reads level with `b84-on` on every
  workload, so the premise that prompted this entry was false, and BAS-P38's and
  BAS-P39's reads were made against a valid base.
- **Disposition.** These reads were not owed: the leaf's rule allows a second read
  of an item only after a disturbed one, and nothing disturbed BAS-P38 or BAS-P39.
  Their verdicts stand. A2: reverted (both reads agree). A1: reverted by BAS-P38.
  This entry's A1 reads would keep it (the guard passes at −0.43% where BAS-P38's
  failed at −0.62%), which says the guard's −0.5% threshold sits inside the spread
  of two reads of one pair; recorded in the packet's Return 1 for the research.
- **Cost of the deviation:** about 80 minutes of reads; no verdict changed by it.
