# BAS-S13

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.4.3 — reduction hypotheses, all refuted before games -->

| Field | Value |
|---|---|
| ID | BAS-S13 |
| Change | Fractional history response: `(stat/div)*1024` → `stat*1024/div`, removing the whole-ply quantisation. Motivated by BAS-D02's 16.2% reduce-to-zero rate. |
| Result | **Worse.** applied 36.1%→32.5%, clamp-to-zero 16.2%→**19.8%**, depth 20.80→20.70. Reverted. |
| Conditional lesson | The quantisation is not only a resolution defect, it is also a **threshold**. History *subtracts* from `r` and most moves carry positive history, so a continuous response shaves a little off nearly every reduction, where the integer form shaved a whole ply off only the `|stat| ≥ div` minority. Retry trigger: base and context reductions are materially larger, so there is enough `r` to modulate rather than erase. |
