# Clustering of the first-layer outputs: 92442 evaluations (25000, 25000, 25000, 17442 per band)
Check: piece count from the position = recorded piece count in 100.00% of the records.
PCA: the first 32 components explain 18.4% of the variance (first 5: 2.2%, 1.6%, 1.3%, 1.0%, 0.7%).

## How much each criterion explains the network's clusters (NMI, 0 = nothing, 1 = all)
| criterion | k = 4 | k = 6 | k = 8 | k = 12 |
|---|---:|---:|---:|---:|
| N5 SF P4 (pieces-1)//8 x queens 0/1/2+ | 0.345 | 0.429 | 0.375 | 0.349 |
| band x queens | 0.339 | 0.428 | 0.370 | 0.346 |
| N8 total NPM 4 bands x queens 0/1/2+ | 0.363 | 0.385 | 0.370 | 0.336 |
| N7 queens (side to move, other) | 0.328 | 0.408 | 0.369 | 0.346 |
| N3 pair of NPM classes (side to move, other) | 0.407 | 0.374 | 0.343 | 0.312 |
| queens: 0 / 1 / 2+ | 0.301 | 0.389 | 0.350 | 0.327 |
| N4 opponent NPM x opponent queen | 0.336 | 0.364 | 0.340 | 0.306 |
| N2 NPM of the side to move's opponent, 4 classes | 0.363 | 0.342 | 0.310 | 0.280 |
| N1 total NPM 4 bands (<=12/13-28/29-46/47+) | 0.363 | 0.329 | 0.305 | 0.276 |
| N6 queen of the side to move's opponent (K32Q2) | 0.261 | 0.361 | 0.314 | 0.290 |
| material band (today) | 0.325 | 0.338 | 0.283 | 0.262 |
| non-pawn pieces: <=2 / 3-6 / 7-10 / 11+ | 0.345 | 0.282 | 0.272 | 0.242 |
| pawns: <=6 / 7-10 / 11-13 / 14+ | 0.256 | 0.288 | 0.238 | 0.227 |
| rooks and minors: rooks 0/1+/3+ x minors 0-1/2-3/4+ | 0.245 | 0.202 | 0.223 | 0.205 |
| closed structure (blocked pawns: 0-1 / 2-3 / 4+) | 0.056 | 0.066 | 0.108 | 0.098 |
| opposite castling | 0.004 | 0.007 | 0.017 | 0.018 |

## Inside each band: clusters at k = 3 and the criterion that explains them best
| band | N1 total NPM 4 bands (<=12/13-28/29-46/47+) | N2 NPM of the side to move's opponent, 4 classes | N3 pair of NPM classes (side to move, other) | N4 opponent NPM x opponent queen | N5 SF P4 (pieces-1)//8 x queens 0/1/2+ | N6 queen of the side to move's opponent (K32Q2) | N7 queens (side to move, other) | N8 total NPM 4 bands x queens 0/1/2+ | queens: 0 / 1 / 2+ | pawns: <=6 / 7-10 / 11-13 / 14+ | non-pawn pieces: <=2 / 3-6 / 7-10 / 11+ | opposite castling | closed structure (blocked pawns: 0-1 / 2-3 / 4+) | rooks and minors: rooks 0/1+/3+ x minors 0-1/2-3/4+ |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 0.002 | 0.093 | 0.197 | 0.088 | 0.077 | 0.062 | 0.182 | 0.086 | 0.104 | 0.001 | 0.002 | 0.013 | 0.000 | 0.016 |
| 1 | 0.006 | 0.059 | 0.211 | 0.054 | 0.087 | 0.020 | 0.124 | 0.080 | 0.087 | 0.006 | 0.008 | 0.003 | 0.001 | 0.016 |
| 2 | 0.081 | 0.069 | 0.151 | 0.083 | 0.136 | 0.111 | 0.193 | 0.127 | 0.162 | 0.022 | 0.046 | 0.015 | 0.001 | 0.010 |
| 3 | 0.036 | 0.028 | 0.031 | 0.039 | 0.029 | 0.035 | 0.035 | 0.046 | 0.036 | 0.058 | 0.021 | 0.068 | 0.193 | 0.031 |

## Composition of the 12 clusters
| cluster | share | mean band | mean queens | mean pawns | opposite castling | mean blocked pawns |
|---|---:|---:|---:|---:|---:|---:|
| 0 | 22.4% | 0.39 | 0.09 | 0.16 | 14% | 0.04 |
| 1 | 14.7% | 2.53 | 1.45 | 2.05 | 5% | 0.23 |
| 2 | 5.0% | 2.24 | 1.78 | 1.49 | 27% | 0.40 |
| 3 | 10.8% | 2.35 | 1.84 | 1.58 | 17% | 0.19 |
| 4 | 8.3% | 1.06 | 0.08 | 0.75 | 23% | 0.19 |
| 5 | 5.8% | 2.69 | 1.65 | 2.37 | 20% | 1.15 |
| 6 | 8.8% | 1.14 | 1.87 | 0.67 | 24% | 0.18 |
| 7 | 3.7% | 0.89 | 0.79 | 0.61 | 25% | 0.14 |
| 8 | 4.9% | 0.61 | 0.28 | 0.30 | 18% | 0.05 |
| 9 | 7.8% | 0.81 | 0.09 | 0.54 | 26% | 0.15 |
| 10 | 3.9% | 0.91 | 0.24 | 0.62 | 11% | 0.16 |
| 11 | 3.9% | 1.15 | 0.85 | 0.79 | 23% | 0.16 |
