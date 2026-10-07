# BAS-D09

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D09 — search diagnostics re-measured after Phase 5.9; one is stale**
(2026-08-30). Every search diagnostic that 5.7's candidate ranking rests on was
taken on the **pre-5.9 evaluation**, which has since moved ~+12 Elo. Re-measured
on head `e763a52`, same suite and protocol.

| metric | pre-5.9 | current | verdict |
|---|---:|---:|---|
| first-move cutoff | 89.101% | 88.872% | **holds** |
| mean cutoff index | 0.2137 | 0.216 | holds |
| LMR applied | 36.115% | 36.008% | holds |
| LMR mean reduction | 2.354 | 2.337 | holds |
| LMR re-search | 1.744% | 1.798% | holds |
| LMR clamp-0 | 16.217% | 16.458% | holds |
| branching b(4–11) | 1.692 | **1.754** | holds directionally (ref 1.894) |
| **qsearch share** | **30.8%** | **35.1%** | **STALE — see below** |

*Ordering and LMR are unmoved.* A 12-Elo evaluation change left every ordering
and reduction statistic within 0.3 percentage points. BAS-D01's "ordering is
healthy" and the LMR picture that 5.4.3 refuted candidates against both stand.
