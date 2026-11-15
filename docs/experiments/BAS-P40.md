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
