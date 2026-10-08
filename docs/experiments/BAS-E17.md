# BAS-E17

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E17 — 5.9.11 label-source experiment, REGISTERED BEFORE RUNNING**
(2026-08-26). Three corpora differing in **one variable — which engine's games
produce the WDL labels.** Everything else is held identical: the same
`beast_seed_2m.epd` starts, adjudication none, the same extraction parameters,
the same 348-parameter `scalars` fit that 5.9.4 used, the same holdout split.

| arm | label engine | nodes/move | throughput | draws | rounds |
|---|---|---:|---:|---:|---:|
| **A** | Basilisk 1.9.3 | 8,000 | 48.4 g/s | 41% | 125,000 |
| **B** | Stockfish dev-20260716 | 8,000 | 15.2 g/s | 44% | 125,000 |
| **C** | Basilisk 1.9.3 | **25,000** | 18.75 g/s | 45% | 125,000 |

*Why the fit is `scalars` (348) and not the full 1,116.* 5.9.4 fitted exactly
that group on distillation labels and 5.9.6 lost **−77.92 Elo**. Holding the
group fixed makes this a one-variable replication: if changing only the labels
recovers the loss, the BAS-E11 correction is confirmed directly. The full-surface
fit is 5.9.12's job, using whichever label source wins here.

*Arms are matched, verified by pilot rather than assumed* (3,000/3,000/1,500
games). Phase yields per game are near-identical — opening 2.060 / 2.039 / 2.104,
deep_endgame 3.018 / 2.972 / 3.178 — so equal rounds give equal corpora and the
comparison is not confounded by phase mix.

*The draw-collapse risk was measured and did not materialise.* Stronger play
should draw more and flatten label variance; draws move only 41% → 44% → 45%.
Arm C exists precisely to separate **label quality** from **label source**: it
raises Basilisk's own strength instead of importing another engine's, so if the
oracle's advantage is merely less-noisy outcomes, C captures it without the
transfer risk.

**Registered decision rule, fixed before any game is played.**

1. Each arm gets **one SPRT against the current head**, nElo `[0, 3]`,
   alpha = beta = 0.05, `3+0.03`, 1T, Hash 64, UHO — the standard gate.
2. The winning label source is the arm that **passes with the highest nElo point
   estimate**. A-vs-B is run only if attribution is still ambiguous after (1).
3. **If no arm passes**, the label-source hypothesis is *not* supported, and the
   next suspect is the extraction contract or the fit itself — not another
   corpus. Say so rather than iterating on data.
4. No arm is re-fitted, re-extracted or re-tuned after seeing its SPRT. A failed
   gate is not repaired by adjusting its own inputs.

*Pre-registered prediction (mine, recorded so it cannot drift):* all three beat
the head, because all three fix the distillation defect; **C > A > B** on
ordering. High confidence on "all three beat the head", low confidence on the
ordering. If B wins, the independence question in PLAN becomes live — shipping
an evaluation shaped by Stockfish's play is a different thing from being
inspired by its ideas.

*Cost:* ~4h50m of datagen (43 min + 2h17m + 1h51m), plus extraction, three
fits, and three-to-four SPRTs.
