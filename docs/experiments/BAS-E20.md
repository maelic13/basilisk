# BAS-E20

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E20 — 5.9.14 king-safety refit on game-result labels; the reshape is real
but far milder than the distilled corpus claimed** (2026-08-27,
`--tune-kingsafety`, 43 knobs, arm C corpus: 1,000,000 train / 52,632 holdout,
coordinate descent from step 8, K = 1.94497).

Converged in 55 passes, best holdout restored from **pass 38**. Holdout
**0.0793025 → 0.0791382**, **−0.21%**.

*The distilled corpus overstated this by roughly 5×.* 5.9.5 ran the same fitter
on `beast_sf` and reported **−0.99%**; on real game outcomes the same procedure
finds **−0.21%**. A corpus of Stockfish evaluations rewards reshaping king
safety to look like Stockfish's king safety, which is a much larger apparent win
than reshaping it to win games. Another instance of BAS-E11's lesson, now
visible inside a single instrument rather than across an SPRT.

*The table reshape is the same shape, a fraction of the magnitude:*

| index | 2 | 6 | 10 | 14 | 17 | 20 | 24 |
|---|---:|---:|---:|---:|---:|---:|---:|
| baseline | 6 | 55 | 111 | 191 | 203 | 302 | 407 |
| **5.9.14 (game results)** | 0 | 45 | 101 | 177 | **281** | 307 | **406** |
| *5.9.5 (distilled)* | *0* | *28* | *68* | *160* | *263* | *369* | ***597*** |

Both are convex — low end down, high end up. But 5.9.5 nearly halved the middle
and inflated the top by 47%; 5.9.14 leaves the top **unchanged** (407 → 406) and
trims the middle by ~8%. The extreme reshaping was substantially an artifact of
the labels.

*The rook finding reproduces; the queen finding does not.* `ks_unit[ROOK]`
**0 → 2** again, independently, on a different corpus with different labels —
that is now two-for-two and worth taking seriously. `ks_unit[QUEEN]`, which
5.9.5 also moved to 2, **stays at 0 here**. The earlier claim that both were
dead knobs was over-read from a single fit on bad data; only the rook survives
replication.

**Candidate health — the best profile of the whole phase.**

| check | result |
|---|---|
| CTest | **12/12** |
| mate-drive canary | **on=37** — *stronger* than baseline's 29 (5.9.5 broke it to 4) |
| bench | 11,941,440 → **12,844,350**, +7.6% (arms were ~+30%) |
| paired depth at 1M nodes | **+0.093 ply**, 26 deeper / 26 shallower / 55 equal |

Zero depth cost, unlike every 5.9.11 arm (−0.196). The canary moving *up* is the
direct confirmation that BAS-E14's corpus fix worked: the same fitter that
destroyed mating behaviour on a corpus with no mating positions now strengthens
it on one that has them.

*Standing caution.* None of this is Elo. Holdout deltas have failed to predict
Elo repeatedly in this project, and −0.21% is a small one. The gate decides.
