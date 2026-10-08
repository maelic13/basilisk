# BAS-D04

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D04 — history-pruning reachability** (`suite_v1.epd`, depth 12,
2026-08-13; `analysis/cluster56_audit_v1.md`). The live condition compares
`hist_prune_coeff * depth` against a sum of six **bounded** history channels
whose maximum magnitude is **81,920**. At `coeff = 14004` the depth-6 threshold
is 84,024 — **provably unsatisfiable**; depth 5 needs 85% of theoretical maximum
negative on every channel at once. The mechanism is live only at depths 1–2 and
fires **142 times in 5,355,599** tested quiet moves (0.003%). Loosening would
activate a real population — `coeff/2` 95,418, `coeff/4` 234,235, `coeff/8`
467,647 — but paired depth at 300k nodes is flat at every value (−0.019, +0.037,
−0.019).

*Conditional lesson.* A threshold that scales with depth against a signal that
does not is unreachable at the top of its own range; this one was stranded when
`hcefinal` re-scaled the history space. But reviving it is **not** a candidate:
it would prune 4–9% of tested quiets for no depth, and BAS-S16 measured a tree
that shrinks without gaining depth at −3.48 ±3.32 Elo. Move-count pruning
already fires 22.2M times against 15.1M interior nodes, so the quiets history
pruning would catch are largely gone before it is consulted.

*Retry trigger.* Only if move-count pruning is restructured so the surviving
quiet population changes materially, or if a diagnostic shows the pruned moves
carry quality cost rather than node cost. Not on a new coefficient alone.

*Also recorded.* ProbCut's 0.4% share is **correct rarity, not a defect** — it
succeeds 56,311 of 84,469 tries, a 67% hit rate.

*Harness defect found and fixed in the same work.* `print_diag` built its kv
line into `char buf[256]`; the grown line overflowed and `snprintf` truncated
silently, always losing the tail field, so corruption scaled with counter
magnitude. It was caught only because the threshold series has a monotonicity
invariant that made the result visibly impossible. Buffer now 512 with the
probe on its own line.
