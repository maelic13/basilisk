# BAS-M06

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M06 |
| Experiment and conditions | Behaviour-neutral performance measurements with single builds/runs versus pooled PGO and identical-binary self-pairs. |
| Result / disposition | An apparent −0.5% regression was retracted; pooled measurements found +0.17 ± 4.41 Elo and NPS inside noise for the tested patch. |
| Conditional lesson and retry trigger | On this host, a single PGO build or unbalanced placement is insufficient for small speed claims. Retry only with the pooled/interleaved protocol. |
| Source | `CHANGELOG.md` 1.9.1 |
