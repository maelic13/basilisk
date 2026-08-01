# Phase 9 multi-thread baseline (recorded at 9.3, 2026-07-30)

> **Audience: developer / maintainer** (PLAN §1). This is the "before" record
> the Phase-9 SMP steps are read against — 9.4 (clock safety), 9.5 (the
> coordination wave) and 9.10 (stop threshold) all move numbers on this page.
> Nothing here is a gate; it is the reference state.

Head: `aba3941` (9.3) + the bench thread-argument repair. Binary:
`release-pext`, `TUNE=ON` for the `Diag` option. Box: Ryzen 9 5950X, 16
physical / 32 logical, otherwise idle.

## Thread-count safety (9.3a)

The advertised `Threads` maximum is a flat **1024**, matching Stockfish's
`Option(1, 1, 1024)` (user decision 2026-07-30 — an interim implementation used
`min(1024, 4 x hw)` = 128 here). The repair that matters is that the cap is
defined **once** rather than computed independently in two files where it could
drift; `test_engine_threading` asserts the `uci` string quotes the same number
the pool honours.

## NPS vs the pre-9.3 head (9.3b node-counter batching)

`bench 13 1 <threads>`, alternating arms, same non-PGO build flags on both
sides. Median of the readings; MT bench node counts vary run to run, so this is
an **indicative** reading, not the pooled-PGO `nps_ab.ps1` protocol a strength
claim would need.

| Threads | pre-9.3 (M nps) | 9.3 (M nps) | delta |
|---:|---:|---:|---|
| 1 | *(identical binary path — `shared_nodes` is null at 1T)* | — | bench **11,941,440**, unchanged |
| 4 | 13.55 | 13.63 | within noise (±10% spread) |
| 8 | 25.21 | 25.04 | within noise |
| 16 | 37.04 | **41.79** | **+12.8%**, no overlap across 3 reps |

Reading: the batching removes a contention bottleneck that **scales with thread
count** — one shared cache line touched at every node by every thread. At the
two deployed conditions (1T, 4T) it measures **neutral**, which is the honest
claim; the gain appears at the ≥8-thread regime that Phase 12 targets. It is
justified as removing a scaling ceiling and as a prerequisite, not as a 1T/4T
speed win.

## Diagnostic baseline at Threads=4 (9.3c)

`setoption name Diag value true` / `Threads 4` / `position startpos` /
`go depth 16`:

```
pool threads 4 nodes 2460395 (main 628674 = 25.6%) | main tt 198260/500683 (39.60% hit) pool tt 794949/1954341 (40.68% hit)
pool tt_stores 1255496 same_key 425581 (33.90%) | main stores 319323 same_key 105605 (33.07%)
pool depths main=16 t1=15 t2=15 t3=16  (diagnostic only: +/-2 iterations rep-to-rep at fixed time)
```

What each line is for, and how to read it later:

- **Node share.** Main does 25.6% of pool nodes, i.e. an even quarter across
  four threads. A 9.5 change that makes main's share move a lot is changing who
  does the work, which is worth knowing before reading its Elo.
- **TT hit split.** Main 39.60% vs pool 40.68% — main tracks the pool almost
  exactly. This is the number Rarog's throttling experiment moved (they measured
  helper-write throttling costing main 12 points of hit rate at 4T); our helper
  writes are unthrottled, and this baseline says that is working.
- **Same-key store share, 33.9%.** A third of TT stores update an entry that
  already holds that position rather than evicting a different one. This is the
  new 9.3c counter and the quantity 9.5's coordination work moves; read it as a
  share, never as an absolute.
- **Per-thread depth 16/15/15/16.** Spread of one iteration here. ⚠ Never a
  verdict: its rep-to-rep spread at fixed time is ~±2 iterations, the same size
  as any effect worth measuring. Likewise divide aspiration/re-search counts by
  thread count before comparing across thread counts.

## Instrument note

`bench` was **structurally unable to measure multi-thread NPS** before this
step. `run_bench()` has always taken and documented a thread count, but
`Engine::run_bench_command` parsed only depth and repeats and passed a
hardcoded `1`, so `bench [depth] [repeats] [threads]` was unreachable and the
`Threads` UCI option is deliberately not consulted (bench must stay
reproducible regardless of how a GUI left the engine). The third argument is
now parsed. The default remains 1, so the fingerprint is untouched; above 1 the
node total is non-deterministic by design and is a speed reading only.

`README.md` claimed `bench` honoured the `Threads` option. It never did; that
claim is corrected.
