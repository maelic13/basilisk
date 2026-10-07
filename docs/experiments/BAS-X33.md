# BAS-X33

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X33 |
| Rarog evidence | **What Rarog's B.0 found by measuring before designing** (RAR-M50). The tree-shape target was wrong as written: its branching factor was already *below* the oracle's, and its excess was a shallow-depth constant factor. The deficit was decision quality at a fixed budget (WAC at 100k nodes 200 against the oracle's 242). Donor seeds could not be converted by one scalar: the eval-scale ratio was 0.457, but Rarog's HCE residual averaged 128 cp, so NNUE-sized margins converted by the ratio alone would fire at nearly every node, which is why seeds came from three columns. 46.7% of its LMR reductions landed in qsearch and only 1.3% of reduced moves were re-searched: shape defects invisible to the counters read before. |
| Basilisk implication | Basilisk's B.0 measures the same things on its own head rather than inheriting these numbers: branching window, fixed-node quality, the scale ratio against both modern Stockfish and the classical oracle, its own eval residual, and the LMR landing and re-search rates. BAS-O03's EBF 2.20 against 1.61 says Basilisk's shape problem may differ from Rarog's. |
| PLAN coverage | B.0; PLAN "Scale conversion" |
