# BAS-X16

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X16 |
| Rarog evidence | Rarog RAR-M20 directly benchmarked Basilisk `d734766` against Rarog `ca03a46` and Reckless `91b56c2` on a Ryzen 9 5950X, native optimized non-PGO, three cyclic rounds. Basilisk led Rarog by **43.7% legal generation, 22.3% captures, 29.4% generation+make/unmake, 39.8% perft and 46.0% two-ply simulation**; RAR-M29's normalized SEE rerun was **58.335 vs 44.923 M captures/s (+29.9%)**. |
| Possible Basilisk implication | This measures Basilisk directly and rejects a broad board-throughput deficit as the default hypothesis. Active-desktop load and scatter prevent small-gain claims; microbenchmarks do not establish whole-search share or Elo. Profile HCE search before opening board work. |
| Existing PLAN coverage | 7.1–7.12; donor `analysis/board_audit_2026-09-05.md`, RAR-M20/M29 |
