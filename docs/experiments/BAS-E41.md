# BAS-E41

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E41 — 6.1.e: the held-out set rejects the 6.1.c winner and confirms the
plateau probe** (2026-09-03). Full frozen 198-position cohort, one run per arm,
verdict read on positions 61-198 only. Both control fingerprints reproduced, so
the run is valid.

| arm | dev 1-60 | held-out 61-198 | paired g/l held-out | z | live discards | verdict |
|---|---:|---:|---:|---:|---:|---|
| legacy `15600,800,900,220,220` | 26/60 | 69/138 | — | — | 0 | control |
| `15600,1750,0,340,0` | 51/60 | 103/138 | 47/13 | +4.39 | **1** | **REJECTED** |
| **`15600,1900,0,460,0`** | 46/60 | **98/138** | 42/13 | **+3.91** | **0** | **CONFIRMED** |

**The rejection is a chess blunder, not a statistical shortfall.** On held-out
`KBNK0061` (`8/8/8/2K5/8/3k4/8/N1B5 w`, clean win at DTZ 52) the 6.1.c winner
plays 1.Kd5 Ke2 2.**Nc2??**, when Syzygy flips White from +2 to 0. Black
answers 2...Kd1, forking the undefended bishop on c1 and knight on c2, and
after 3.Ke4 Kxc1 the game is K+N versus K, dead drawn by ply 5. The confirmed
vector plays 1.Nb3 instead, bringing the knight out of the corner, and
converts with DTZ falling 52-51-50-46-45. The cause is the same flat-maximum
pathology BAS-E36 identified for the bishop: with edge and knight weights both
zero, no term in `kbnk_score` refers to the knight, so every knight move scores
alike and the tiebreak can be a losing one. A steeper diagonal does not fix
that; it only changes which arbitrary move wins the tie.

The confirmed vector is also the cleanest arm on truth. Across all 198 it takes
just 10 discarded clean wins against the legacy 32, its earliest is at ply 96 —
inside the benign rule-50 cleanup band BAS-E35 established — and it holds
99.8467% of clean-win moves against the legacy 99.5750%. Conversion rises 95 to
144 of 198, rule-50 draws fall 87 to 53, stalemates 16 to 1, and DTZ progress
improves from 0.4897 to 0.5880.

**The registered caveat was correct in both directions.** Head-to-head on the
held-out rows the two candidates are 24/19 discordant, z +0.76: statistically
**indistinguishable**, exactly the plateau BAS-E39 predicted and could not
resolve. Shrinkage was real too: 85.0% to 74.6% for the 6.1.c winner and 76.7%
to 71.0% for the probe. Had 6.1.e carried only the selected vector, the step
would have ended with a rejected candidate and no replacement; carrying the
second arm is what produced a result. Engine SHA-256
`0539993784847A70C89460D245C50558CCEFB251FAC50500F3D2276297DA0403`; summary
SHA-256 `FD09E1E1E872144E7476DE045D65A439C4F17D69496340F49B371EF0CCC437A1`.

*Two things this does not claim.* Conversion on a truth cohort is not Elo; the
Group A SPRT in 6.2 owns that. And `bench 13` is unchanged at 12,709,666
because the bench suite contains no KBNK position, so bench identity here is
bookkeeping and carries no behavioural information about this change.
