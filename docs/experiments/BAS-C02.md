# BAS-C02

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C02 |
| Experiment or failure mode | Rule-50/mate precedence, null-move halfmove preservation and legal-EP hashing defects. |
| Disposition | Fixed and covered by board/search invariants. |
| Conditional lesson / coverage | Draw and hash semantics influence TT, repetition and pruning together; retain deterministic coverage before strength testing. |
| Source | `CHANGELOG.md` 1.9.0 |
