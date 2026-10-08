# BAS-S17

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search programme investigation (B.0) -->

| Field | Value |
|---|---|
| ID | BAS-S17 |
| Experiment and conditions | **Registered, not yet run (B.0.1, maintainer).** Razoring depth reach in Elo: `RazorCoeff=500` against the default 243 on one Tune binary `basilisk-b0-tune-pext-tune-pgo.exe` (`6c5ee63`, bench 14,978,465, SHA-256 `D99C8425…7563`), 2,000 paired games, `3+0.03`, 1T, Hash 64, UHO paired, no adjudication, 14 pinned slots, Colosseum run file; a Tune-build diagnostic, never acceptance. **Prediction (frozen 2026-10-06):** +8 Elo, 80% interval [−4, +20]; probability the point estimate is positive 70%; confidence moderate. Basis: BAS-D21 (razoring off +51 WAC at 100k for +13% nodes; 500 recovers 32 of 59 for +5%). Counter-argument: the `hcefinal` SPSA fitted 243 with 500 as its range maximum. |
| Result / disposition | **Prepared 2026-10-08, not yet run.** `tools/colosseum.ps1 -Mode match` with `match-fixed.toml`: arms `Razor500` (`RazorCoeff=500`) and `Razor243` (`RazorCoeff=243`) on the one binary, 2,000 games on 1,000 paired openings, seed 20101, concurrency 14, `-ExpectRevision 6c5ee630` and bench 14,978,465 on both arms, results in `tools/results/b01-razor-reach/`. Dry run accepted (`tools/results/colosseum_match_b01-razor-reach_20261008_195149.*`, dry-run JSON SHA-256 `2808FE1D…070E`). **Reading rule:** a 95% interval wholly above 0 confirms the reach as a defect and B.2 seeds razoring from the oracle column (depth 1, 256 cp) without a categorical re-test; wholly below 0 keeps 243 as a seed column and B.2.2 settles the depth cap by its own paired run; otherwise the cluster decides. |
| Conditional lesson and retry trigger | Fixed-node tactical gain is not Elo; this is the cheapest Elo-layer reading of the single largest fixed-node defect B.0 found. Retry: none; one run, one read. |
| Source | PLAN B.0.1; packet §11 |
