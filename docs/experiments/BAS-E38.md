# BAS-E38

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E38 — corrected 6.1.c refinement improves 26/60 -> 47/60, but the
boundary leaders fail live truth** (2026-09-03). The five-coordinate run
reproduced both earlier controls exactly, proving the refactor itself neutral.
At fixed base 15,600 with edge and knight removed, conversion rose strongly
with the diagonal slope:

| vector (`base,D,E,K,N`) | converted | paired gain/loss | discards | live discard before ply 80 |
|---|---:|---:|---:|---:|
| legacy `15600,800,900,220,220` | 26/60 | — | 10 | 0 |
| `15600,1250,0,140,0` | 39/60 | 19/6 | 6 | **1** |
| `15600,1450,0,220,0` | 46/60 | 27/7 | 2 | **1** |
| `15600,1450,0,300,0` | **47/60** | 24/3 | 5 | **1** |

The headline leaders are **rejected**, not accepted. On the theoretically won
`KBNK0039` (initial DTZ 50), all diagonal-1450 arms play `Bc1-e3` immediately
and Syzygy changes White's verdict from clean win to draw; the king-220 and
king-300 arms then lose a piece at ply four. Aggregate conversion cannot excuse
a newly introduced live truth failure. The independent base axis at
diagonal-1250/king-220 scored 35/36/31/36 conversions for bases
14200/15600/17000/18400, refuting base inflation as the sole cause and showing
that absolute score still perturbs search non-monotonically.

Because the best aggregate result was also the highest tested diagonal, the
optimum was not bracketed. A final 1350-1900 upper-range screen is registered,
but only vectors that preserve the KBNK0039 preflight and every other clean win
before ply 80 may compete. Summary SHA-256:
`FAF6D1CC1A4AB43AF67DAD4D97B9CA2E247AC65F36807A3260C3D3F55814F5FA`.
