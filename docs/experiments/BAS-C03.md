# BAS-C03

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C03 |
| Experiment or failure mode | TT, parser, PV and SMP could expose stale or illegal moves. |
| Disposition | Pseudo-legality validation, PV truncation, differential perft, fuzzing and sanitizers retained. |
| Conditional lesson / coverage | Shared or aliased evidence is untrusted at consumption boundaries. Correctness gates remain necessary even if normal games rarely hit the path. |
| Source | `CHANGELOG.md` 1.4.x–1.9.0 |
