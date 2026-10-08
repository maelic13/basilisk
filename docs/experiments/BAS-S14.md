# BAS-S14

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.4.3 — reduction hypotheses, all refuted before games -->

| Field | Value |
|---|---|
| ID | BAS-S14 |
| Change | Raise the context magnitudes toward reference scale via the TUNE knobs: `LmrCutNodeAdj` 401→1024/2048, `LmrTtPvAdj` 23→1024. |
| Result | **Worse or flat.** Mean reduction essentially unmoved (2.354 → 2.229 / 2.338 / 2.294) and non-monotonic in the knob; depth at equal nodes **20.80 → 20.10** at the largest setting. Not adopted. |
| Conditional lesson | The reference's magnitudes do not transfer, and this is exactly why PLAN forbids importing constants: they were fitted to a different search. More importantly the response is non-monotonic, which says the aggregate is not a clean lever — changing reductions changes which nodes are eligible, so the ratio and its input move together. |
