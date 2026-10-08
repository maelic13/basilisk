# BAS-X07

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X07 |
| Rarog evidence | Rarog's AArch64 TT prefetch later measured +1.42% NPS on M4 with 12/12 paired wins and exact search identity; a proposed 128-byte TT wrapper was flat because allocations were already aligned. |
| Possible Basilisk implication | Verify emitted instructions and the mechanical premise of a layout change. Basilisk already has ARM prefetch; target-native evidence, not donor code, decides any retry. |
| Existing PLAN coverage | 7.0, 9.10, 13.1 |
