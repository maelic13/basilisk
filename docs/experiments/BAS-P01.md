# BAS-P01

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 6. Throughput, build and platforms -->

| Field | Value |
|---|---|
| ID | BAS-P01 |
| Experiment and conditions | Phase-8.7 profile-guided, bench-identical optimization wave. |
| Result / disposition | **Accepted, +4.34% pooled-PGO NPS; batch result +8.69 ± 6.63 Elo at `3+0.03`.** |
| Conditional lesson and retry trigger | On the tested x64 PEXT path, several small hot-path gains compounded and translated at roughly 2 Elo per 1% NPS, with a wide CI. Do not assume that ratio at LTC or another ISA. |
| Source | `CHANGELOG.md` 1.9.1 |
