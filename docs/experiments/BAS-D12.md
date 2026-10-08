# BAS-D12

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D12 — 5.7.2 reading, not a verdict** (2026-08-30). `572-sqlmr` against
`5912-slim`, stopped by decision at **24,956 games**: **Elo +1.49 ±2.77, nElo
+2.32 ±4.31, LOS 83.52%, LLR +0.51 (17.3%)**.

Stopped because it could not resolve: the LLR drift implied ~149,000 further
games to reach a boundary, past the 100,000 hard stop, with the nElo estimate
straddling the upper bound of 3 — the classic indifference-region case. The
45,000-game criterion set beforehand was revised for that reason, stated rather
than quietly dropped; running on would have narrowed the interval to ±2.05
without changing the disposition.

**Read as: not a regression, plausibly a small positive.** Consistent with its
+0.121 ply sweep and a 9% smaller tree. It is **kept provisionally and is NOT
accepted** — PLAN's 5.7.7 gates the surviving set as one integrated contract,
and a +1.5 Elo change is below what a single SPRT resolves economically. If
5.7.7 fails, 5.7.2 has no independent claim to survival and comes back out.
