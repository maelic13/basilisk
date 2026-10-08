# BAS-C15

| Field | Value |
|---|---|
| ID | BAS-C15 |
| Experiment or failure mode | **Frozen before any game, the 1.10.2 ponder-on, Threads 4 smoke run against 1.10.1 (maintainer-run).** `tools/ponder_match.py`, the referee 1.10.1's ponder gate used: real ponder protocol, a `ponderhit` sent the moment the opponent's move is read. Candidate `basilisk-1.10.2-rc-pext-pgo.exe` (SHA-256 `1234F3F6…`) against the **published** 1.10.1 asset `D:/chess/engines/basilisk/basilisk-v1.10.1-windows-x86_64-pext-pgo.exe` (SHA-256 `F7AAEF49…`, bench 14,978,465, `id name Basilisk 1.10.1`). 200 games, `10+0.1`, Threads 4, Hash 256, concurrency 2, tables `D:/chess/tablebases/syzygy3456`, the UHO book, seed 1102, time margin 100 ms. Wiring proven 2026-10-08: 2 games at `3+0.05`, 37 and 38 ponder hits, no failures. |
| Disposition | registered |
| Conditional lesson / coverage | **Pass:** zero failures of every kind for 1.10.2 (forfeit, hang, stop stall, early bestmove, illegal move, crash). The Elo it prints is not a gate. A failure stops the release for investigation. |
| Source | PLAN A.8.21; release 1.10.2 |
