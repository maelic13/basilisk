# BAS-S20

| Field | Value |
|---|---|
| ID | BAS-S20 |
| Experiment and conditions | **Frozen before any game, PLAN A.8.18: the won-endgame time-sink repair (maintainer-run).** Candidate `tools/test_engines/basilisk-a818-sink-new-pext-pgo.exe` (`9c93d15`, SHA-256 `FC01A88A…`); baseline `basilisk-a815-tb-new-pext-pgo.exe` (`b0a078a`, the BAS-S19 candidate, SHA-256 `C502B6DA…`). Between them only A.8.17's stop changes the engine; both are clean PGO builds at bench 14,978,465. Colosseum CLI 0.2.0 through `tools/colosseum.ps1 -Bracket repair` (`sprt-repair.toml`): repair bracket `[-5, 5]` nElo, `3+0.03`, Hash 64, Threads 1, concurrency 14, the standard UHO book, no tables, no adjudication, seed 818, cap 4,000 pairs. Dry run accepted 2026-10-07 (`tools/results/colosseum_sprt_a818-sink-gate-dry_20261007_174845.*`). Activation is shown off the games, by the clock probes recorded under PLAN A.8.17, since the PGN does not record when the stop fires. |
| Result / disposition | registered |
| Conditional lesson and retry trigger | **Prediction (frozen, from the A.8.16 packet):** +1 Elo, 80% interval [−2, +4]; zero time losses. **Reading:** H1 or a run to the cap with the interval above −5 accepts the repair, which cannot change a move when it fires; H0, or any time loss, returns A.8.17 to research. |
| Source | PLAN A.8.18; `analysis/a816_time_sink_2026-10-07.md` |
