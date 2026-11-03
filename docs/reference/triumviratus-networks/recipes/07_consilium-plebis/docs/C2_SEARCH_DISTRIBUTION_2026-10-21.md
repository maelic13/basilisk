# Consilium Plebis: expert pairs in the search against the training stream (21/10/2026, F plan step 1, first run)

First run of the measurement the F-phase data weighting needs (C2_F_PLAN_2026-10-21.md). Engine: Triumviratus_8.1
of 21/10 22:20 with -DTRIUMV_C2 -DC2POP (routing cell at every network evaluation), network ep47, one search thread,
Hash 64, `go nodes`. Roots in chunks of 8 (a chunk = one engine process = the bootstrap unit). Tools and raw data:
<work>\c2eng\pop2 (make_roots.py, run_search.py, analyze_dist.py, out\, DISTRIBUZIONE_ep47_weights.npz).

## Conclusions

- **The search is far from the stream.** The same expert on both sides: 67.6% of the search evaluations against 83.7%
  of the training positions. The unbalanced pairs (e1-e2, e2-e4, e0-e1, e5-e6, e0-e2) are 2-3.5 times more frequent
  in the search; e7-e7 0.42 times. Per perspective: e7 9.5% of the search against 21.4% of the stream; e0-e2 above.
- **Weights** (clamp 0.5-2, mean 1 over the stream): 8 pairs at the upper clamp, 6 at the lower one; effective sample
  size of the weighted stream 82%. With so many pairs at the clamps, the clamp range is itself a decision for F.
- **Precision not yet enough:** of the 28 pairs with a search share of at least 0.1%, the relative standard error has
  median 4.9% and maximum 19.7% (e3-e6); the target is 5% for all. About four times the roots (~12,000) are needed.
- **Depth:** 10M against 1M nodes, total variation distance 4.6%; the largest differences (e0 pairs, e3-e3) at about
  2 standard errors: a mild shift toward queen positions with depth. The final run should search at a depth closer to
  the games' (about 4M nodes) or check the shift at two depths again.
- **Sources:** book exits are almost all full material (e7-e7 59%) and the endgame book almost all low material; the
  right source is positions of real games at every ply (`games`), which hold openings and endgames in their natural
  proportions. The weights below use that source only.
- **Training side:** measured with the P skip configuration (3.3M positions, multinomial s.e. negligible); to be
  measured again with the F configuration before F.

## Final run (before F)

Final P export; ~12,000 game positions (chunks of 8), ~4M nodes per root; the engine version F will be judged with
(after SPSA if it ends before F); the stream with the F configuration (c2_cell_sample.py, 10M+ positions). Check: the
final weights should fall inside the intervals of this run, otherwise the search or the net moved the distribution.

## Tables of this run

| source | chunks (8 roots) | nodes per root | evaluations |
|---|---|---|---|
| games | 375 | 1M | 1,300,917,944 |
| deep | 50 | 10M | 1,735,718,251 |
| books | 63 | 1M | 282,631,519 |
| endgames | 33 | 1M | 110,787,741 |
| training stream (P configuration) | | | 3,276,800 positions |

## Pairs: search (games, 1M nodes) against training, and the weight

Pair (a, b) = (expert of one side, expert of the other), both colours together. Shares in %; s.e. by bootstrap over chunks (search) or multinomial (training); rel = s.e. / share of the search.

| pair | search % | s.e. | rel | training % | search / training | weight | 95% interval |
|---|---|---|---|---|---|---|---|
| e7-e7 | 8.874 | 0.446 | 5.0% | 20.925 | 0.42 | 0.51 | 0.51-0.52 |
| e6-e6 | 15.580 | 0.425 | 2.7% | 20.644 | 0.75 | 0.77 | 0.73-0.81 |
| e4-e4 | 8.785 | 0.296 | 3.4% | 10.575 | 0.83 | 0.85 | 0.80-0.90 |
| e5-e5 | 7.658 | 0.258 | 3.4% | 9.692 | 0.79 | 0.81 | 0.76-0.86 |
| e2-e2 | 11.708 | 0.379 | 3.2% | 9.456 | 1.24 | 1.26 | 1.18-1.34 |
| e0-e0 | 7.535 | 0.291 | 3.9% | 5.922 | 1.27 | 1.30 | 1.21-1.40 |
| e1-e1 | 5.789 | 0.249 | 4.3% | 5.166 | 1.12 | 1.14 | 1.05-1.24 |
| e1-e2 | 4.847 | 0.210 | 4.3% | 2.038 | 2.38 | 2.04 | 2.02-2.06 |
| e2-e4 | 4.241 | 0.198 | 4.7% | 1.911 | 2.22 | 2.04 | 2.02-2.06 |
| e0-e1 | 3.943 | 0.171 | 4.3% | 1.888 | 2.09 | 2.04 | 1.96-2.06 |
| e5-e6 | 3.649 | 0.120 | 3.3% | 1.816 | 2.01 | 2.04 | 1.92-2.06 |
| e4-e5 | 2.982 | 0.116 | 3.9% | 1.561 | 1.91 | 1.95 | 1.80-2.05 |
| e3-e3 | 1.717 | 0.133 | 7.7% | 1.303 | 1.32 | 1.34 | 1.15-1.55 |
| e0-e3 | 1.101 | 0.073 | 6.7% | 1.118 | 0.98 | 1.00 | 0.88-1.14 |
| e0-e2 | 3.062 | 0.156 | 5.1% | 0.877 | 3.49 | 2.04 | 2.02-2.06 |
| e6-e7 | 1.265 | 0.056 | 4.5% | 0.844 | 1.50 | 1.53 | 1.39-1.68 |
| e3-e4 | 1.118 | 0.104 | 9.3% | 0.706 | 1.58 | 1.62 | 1.33-1.92 |
| e0-e4 | 0.704 | 0.041 | 5.8% | 0.584 | 1.21 | 1.23 | 1.09-1.38 |
| e2-e3 | 0.935 | 0.108 | 11.5% | 0.554 | 1.69 | 1.72 | 1.36-2.05 |
| e0-e5 | 0.241 | 0.018 | 7.5% | 0.412 | 0.58 | 0.60 | 0.52-0.69 |
| e1-e3 | 0.680 | 0.071 | 10.4% | 0.351 | 1.93 | 1.97 | 1.61-2.06 |
| e4-e6 | 0.980 | 0.034 | 3.4% | 0.318 | 3.08 | 2.04 | 2.02-2.06 |
| e3-e5 | 0.626 | 0.049 | 7.8% | 0.316 | 1.98 | 2.02 | 1.74-2.06 |
| e1-e4 | 0.381 | 0.046 | 11.9% | 0.180 | 2.12 | 2.04 | 1.71-2.06 |
| e0-e6 | 0.031 | 0.003 | 10.7% | 0.166 | 0.19 | 0.51 | 0.51-0.52 |
| e2-e5 | 0.771 | 0.031 | 4.0% | 0.152 | 5.06 | 2.04 | 2.02-2.06 |
| e1-e5 | 0.449 | 0.034 | 7.5% | 0.142 | 3.17 | 2.04 | 2.02-2.06 |
| e3-e6 | 0.150 | 0.029 | 19.7% | 0.124 | 1.21 | 1.23 | 0.88-1.78 |
| e2-e6 | 0.123 | 0.008 | 6.3% | 0.096 | 1.28 | 1.31 | 1.16-1.47 |
| e1-e6 | 0.028 | 0.004 | 14.1% | 0.064 | 0.44 | 0.51 | 0.51-0.58 |
| e5-e7 | 0.041 | 0.003 | 7.0% | 0.047 | 0.88 | 0.89 | 0.78-1.02 |
| e4-e7 | 0.004 | 0.000 | 11.3% | 0.027 | 0.15 | 0.51 | 0.51-0.52 |
| e2-e7 | 0.000 | 0.000 | 19.8% | 0.012 | 0.03 | 0.51 | 0.51-0.52 |
| e0-e7 | 0.000 | 0.000 | 38.3% | 0.006 | 0.00 | 0.51 | 0.51-0.52 |
| e1-e7 | 0.000 | 0.000 | 48.4% | 0.004 | 0.01 | 0.51 | 0.51-0.52 |
| e3-e7 | 0.000 | 0.000 | 22.8% | 0.001 | 0.05 | 0.51 | 0.51-0.52 |

Pairs with a search share >= 0.1%: 28 of 36; their relative s.e.: max 19.7%, median 4.9% (target <= 5%).
Effective sample size of the weighted stream: 82.4% of the unweighted one.
Same expert on both sides: search 67.6%, training 83.7%.

## Expert of one perspective (%), per source

| set | e0 | e1 | e2 | e3 | e4 | e5 | e6 | e7 |
|---|---|---|---|---|---|---|---|---|
| training | 8.4 | 7.5 | 12.3 | 2.9 | 13.2 | 11.9 | 22.4 | 21.4 |
| search games | 12.1 | 11.0 | 18.7 | 4.0 | 14.0 | 12.0 | 18.7 | 9.5 |
| search deep | 10.6 | 10.8 | 19.2 | 5.2 | 14.1 | 13.1 | 17.9 | 9.0 |
| search books | 0.0 | 0.0 | 0.3 | 0.0 | 1.8 | 4.7 | 31.4 | 61.7 |
| search endgames | 15.4 | 9.7 | 26.7 | 4.2 | 27.6 | 12.4 | 3.8 | 0.1 |

## Stability: other sources against games at 1M nodes

- **deep**: total variation distance 4.6%; largest differences: e0-e0 5.94% vs 7.54% (z -2.6), e0-e5 0.44% vs 0.24% (z +2.3), e0-e1 3.26% vs 3.94% (z -2.2), e0-e3 1.54% vs 1.10% (z +2.0), e3-e3 2.42% vs 1.72% (z +1.8)
- **books**: total variation distance 65.8%; largest differences: e7-e7 59.47% vs 8.87% (z +48.3), e2-e2 0.13% vs 11.71% (z -30.5), e0-e0 0.01% vs 7.54% (z -25.9), e6-e7 4.26% vs 1.26% (z +25.3), e4-e4 1.14% vs 8.79% (z -23.8)
- **endgames**: total variation distance 31.9%; largest differences: e7-e7 0.00% vs 8.87% (z -19.9), e6-e7 0.15% vs 1.26% (z -17.2), e6-e6 2.68% vs 15.58% (z -15.8), e5-e7 0.00% vs 0.04% (z -14.3), e4-e7 0.00% vs 0.00% (z -8.8)
