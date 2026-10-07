# BAS-D18

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-D18 — 5.7.2 ACCEPTED BY MAINTAINER DECISION, not by a gate**
(2026-08-31). `singularQuietLMR` at 401 ships on the reading recorded in
BAS-D12: **Elo +1.49 ±2.77, nElo +2.32 ±4.31, LOS 83.52%**, LLR +0.51 at
24,956 games when the run was stopped.

**This is a deviation from "SPRT proposes, SPRT accepts", and is recorded as one
rather than dressed up as a pass.** The justification is that the gate cannot
decide it: the LLR drift implied ~149,000 further games against a 100,000 hard
stop, with nElo straddling the upper bound of 3. Re-running reaches the same
non-resolving state after ~8 hours. Spending that to re-learn BAS-D12 is not a
better use of the machine than accepting a small positive on the evidence in
hand.

*What the evidence actually supports:* not a regression (LOS 83.5%), consistent
with its +0.121 ply sweep and a 9% smaller tree, and clearing CTest and the WAC
floor. What it does **not** support is a claim of +1.49 Elo as a measured gain
— the interval spans [−1.28, +4.26].

*Standing caution.* Any future cumulative Elo claim must treat 5.7.2's
contribution as **unmeasured**, not as +1.49. The 5.13 release gate compares the
accepted head against 1.9.3 directly, which prices it correctly without relying
on this number.
