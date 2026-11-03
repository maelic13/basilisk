# Blending two experts at a boundary (8.0 development, closed on 11 October 2026)

This note records an idea that was implemented, measured and removed during the development of Triumviratus 8.0. The
code is no longer in `source/`; it can be found in the history of this repository (commit `5aa71a2` added the
option, the commit that adds this note removes it). The section of `DEVELOPMENT_8.0.md` that covers the day refers
here.

## The idea

Consilium, the 8.0 network, has four experts on its first layer, chosen by the number of pieces on the board: up to
9, 10 to 15, 16 to 23, and 24 or more. When a capture crosses one of the three boundaries (9|10, 15|16, 23|24 pieces),
the evaluation moves from one expert to the next, even though the position has hardly changed. A study of the
boundaries (agent Z1) found that two neighbouring experts agree on average at every boundary, within ±1.3 cp, but
differ by 22 to 42 cp position by position, and that a weighted mean of the two predicts the deep search, the game
result and Stockfish 19 about 1% better than either expert alone.

The blend computes that mean in the positions next to a boundary. Besides the expert that the piece count selects,
the engine evaluates the same position with the neighbouring expert and returns a weighted mean (weights 256, 384 and
448 out of 1024 for the three boundaries, options `EvalExpertBlendLo/Mid/Hi`). The accumulator of the neighbouring
expert was rebuilt from the cache of king-bucket accumulators (the "finny table"), starting from the accumulator
already computed for the position. The question behind the test was the author's: are the experts really experts, or
would the network be better as one smooth function?

## Measurements

All builds were deterministic PGO builds of the same source; costs are cycles per node measured with hardware
counters (xperf), engine pinned to one core, four alternating rounds, middlegame and endgame position sets.

| test | conditions | result |
|---|---|---|
| Tree at fixed depth | 160 positions, depth 13 and 16 | nodes ×1.01 and ×0.98; best move changed in 16% of positions |
| Quality without the speed cost | 60,000 nodes per move, UHO book | **+5.27 ± 7.86** over 1,978 games |
| Cost of the full blend | xperf | **+15.4% middlegame, +17.8% endgame** |
| Full blend at a time control | UHO 8+0.08 | **−24.23 ± 11.12** over 1,034 games |
| Cheaper variants (cost) | xperf | blend only at depth ≥ 2: +5.3% / +7.0%; only at depth ≥ 3: **+3.9% / +3.9%**; only on one side of the boundary, depth ≥ 1: +6.5% / +10.3% |
| Blend only at depth ≥ 3 | UHO 15+0.15 | **−3.06 ± 9.59** over 1,248 games |

The cost comes from the rows of the accumulator that the rebuild has to read, about 28% more than a normal update. A
variant that updated the accumulator in blocks held in registers ("tiling") executed fewer instructions but lowered
the instructions per cycle from 1.38 to 1.30, so it cost more cycles, not fewer. A cost-free alternative, a correction
learned from the difference between the two experts, was studied on data and dropped: the difference is almost
unpredictable from simple features (R² ≈ 0.02).

## Why it was closed

- The gain in evaluation quality, measured at fixed nodes where speed does not count, is at most a few Elo and not
  distinguishable from zero.
- The full blend costs 15 to 18% of the cycles per node, worth roughly 8 to 10 Elo at short time controls; at 8+0.08
  it lost 24.
- The cheapest variant removes three quarters of the cost but keeps only part of the quality: at 15+0.15 it is flat.

The experts behave as experts: averaging them at a boundary does not improve the evaluation enough to pay for a second
evaluation. If the jump at the boundaries is worth addressing, the place is training, with overlapping material bands
for the experts, which needs a new network.
