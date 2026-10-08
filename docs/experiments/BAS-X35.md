# BAS-X35

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X35 |
| Rarog evidence | **Rarog 2.4.0→2.5.0, the final import** (snapshot `docs/reference/rarog-2026-10-06/` at `dcf15c51`, 2026-10-06). Output and robustness repairs, each confirmed on Basilisk before it entered PLAN: the pool reports the winning thread's line (`da6d8f9`; Rarog measured 4 of 24 at Threads 8, Basilisk 20 of 120) and the last line always describes `bestmove` (`97ea52d`); a tablebase PV extension starts only with ten move overheads of clock left (`b2c98c8`, after a 54 ms cold DTZ read lost RAR-S94's round 745 at 58 ms); single-PV bound lines, per-iteration `seldepth`, `multipv` on every line and a `depth 0` line at a terminal root (`8a76826`, `5a8d58b`, `45779ee`, `5cba881`); MultiPV with identity at 1; a tablebase band of its own with `cp ±(20000 − plies)` display and Stockfish's in-search probe semantics, **+11.4 ± 8.5** with tables (RAR-S94), of which moving band values by ply in the table changed Rarog's bench without tables and was not adopted (`84b1f74`); the won-endgame time sink (rec1 32,305 ms at `60000+600`, Stockfish 19 5.6–6.4 s); the tag-driven release flow and its first-run defect (the fingerprint read from the wrong GUIDE row, `5dc9f0e`); CI on pull requests to `master` only; merge commits and a continual changelog (`1f8d0fb`); documents under `docs/`, closed phases archived and the ledger split into entries (`15e78fd`, `ec014d2`, `ded841f`). |
| Basilisk implication | Each item is a separate A.8 leaf, qualified on Basilisk alone; the +11.4 is a prior for A.8.13's prediction, not acceptance. Rarog's other 2.5.0 fixes were already in Basilisk 1.10.1 (BAS-C10–C13) or do not apply (`analysis/rarog_parity_review_2026-10-06.md`). |
| PLAN coverage | A.8.2–A.8.19 |
