# BAS-D17

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D17 — 5.8.5 fail-high depth reduction REFUTED, fails the floor**
(2026-08-30). The reference re-searches shallower after each fail-high
(`failedHighCnt`).

| | re-searches | depth mean | better/worse | **WAC** |
|---|---:|---:|---|---:|
| current | 1,305 | — | — | **137** |
| reduce | **1,450** | +0.477 | 33 / 24 | **119** |

**WAC 137 to 119 against a floor of 130** — a floor failure, not a comparison,
so BAS-D14's bias caveat does not rescue it (the bias runs *against* less depth
here anyway). Re-searches also rose. A root failing high is often a tactical
shot; searching it shallower misses it. The +0.477 mean is again outlier-carried
(+11, +10, +6).
