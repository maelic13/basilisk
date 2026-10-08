# BAS-S15

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.4.3 — reduction hypotheses, all refuted before games -->

| Field | Value |
|---|---|
| ID | BAS-S15 |
| Change | Hypothesis that the `new_depth - 1` ceiling caps mean reduction, making modulation unable to matter near the leaves. Tested by adding the `lmr_clamped_high` counter. |
| Result | **Refuted.** The ceiling binds on **4.8% of applied** reductions (145,854 of 3,014,380). |
| Conditional lesson | Not the constraint. The counter is retained: it permanently separates "our modulation is too small" from "our modulation cannot matter here", which are opposite repairs and were previously indistinguishable. |
