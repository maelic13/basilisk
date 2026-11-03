# BAS-X39

| Field | Value |
|---|---|
| ID | BAS-X39 |
| Triumviratus evidence | **Clock, protocol and time-management findings** (`analysis/triumviratus_review_2026-11-03.md` §4.4–§4.6, TT-1 to TT-10, TP-1 to TP-6, TR-1 to TR-5). Unbuffered output and a 15.6 ms clock, present since its version 3.0, cost +29.8 ± 7.4 at 5+0.05 with an identical tree, found by timing `go nodes 1000` from outside (6.15 ms against Stockfish's 1.24). A judge-engine loss analysis put 33.8% of its loss against Stockfish 19 in defence with under 10% of the clock. Rules on top of a Stockfish-shaped clock: the surprise rule (+6.23 ± 6.75 at 40+0.4, flat at 16+0.16), less time when behind (+5.23 ± 4.72 at 6+0.06, flat at 30+0.3), time in defence with early saving (+7.27 ± 7.08 at 40+0.4); a lower cap on the longest thinks lost −27.1 ± 17.0. A 16-byte, XOR-verified TT entry four to a cache line read +6.30 ± 5.06 at Hash 16 against its old layout. Protocol defects: a search before any `position`, the final PV after a stop, stale per-ply state in a ProbCut child |
| Basilisk implication | Basilisk's code passes the clock and stream reading (steady clock, unsynced streams, persistent threads, Hash re-sends ignored); D.1 verifies it with the wall-clock read and builds the loss analysis as an instrument. The time rules wait in H.3: each was about zero at short controls. The TT layout is a candidate for BAS-C05's retry trigger in D.2. The protocol defects are D.3.2 tests |
| PLAN coverage | D.1, D.2, D.3.2, G.1, H.3 |
| Disposition | import |
