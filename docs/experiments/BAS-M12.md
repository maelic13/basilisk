# BAS-M12

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M12 |
| Experiment and conditions | **Houdini 3 crashes at 4T: diagnosis (observation, 2026-10-05).** Every failure in BAS-M10 and BAS-M11, and one in a Rarog gauntlet of 2026-09-11, has one shape: after `Threads=4` and `isready`, Houdini allocates its large-page hash and dies (`0xc0000005`) or hangs while starting its helper threads, before or within its first searches. A stress harness that launches it exactly as Colosseum does (suspended, affinity set, then `uci`, `Hash`, `Threads`, `isready`): alone, any slot, any hash, three at once, **0 of 800**; with two pinned 8-thread searches loading the other slots, **7 of 900** at `Numa` default, **1 of 600** with `Numa=false`, **0 of 600** at `Threads=1`. |
| Result / disposition | A race in Houdini 3's multi-thread start-up, triggered by CPU contention from concurrent games; its NUMA binding raises the rate but `Numa=false` is not a cure, and it never appears at one thread. Not Basilisk, Colosseum or hash size. Colosseum 0.2.0 has no setup retry and scores such a game as a win for the opponent. |
| Conditional lesson and retry trigger | Exclude Houdini setup crashes from any reading (no move was played, so nothing chess-relevant is lost); expect about 1% of its 4T games under load. Retry: a Colosseum setup-retry option, or a wrapper that restarts the engine before move 1, would make exclusion unnecessary. |
| Source | `tools/results/a72-gauntlet-4t`; the app's incident logs |
