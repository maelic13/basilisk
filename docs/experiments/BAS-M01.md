# BAS-M01

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M01 |
| Experiment and conditions | Historical unpinned fastchess runs were compared with fixed explicit physical-core placement on the Ryzen 9 5950X. |
| Result / disposition | Real scheduler offsets up to about ±10 Elo/run were found; the harness now discovers physical cores, pins explicitly and leaves two cores free. |
| Conditional lesson and retry trigger | On this Windows host, small unpinned results may be biased. Re-audit a borderline historical verdict only if it would affect a current decision. |
| Source | `CHANGELOG.md` 1.9.1; legacy plan at `8dc0a24^` |
