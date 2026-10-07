# BAS-E35

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E35 — 5.9.21 closes empty: the KBNK stalemates are not a defect**
(2026-09-01). BAS-E29 reported **14.5%** in a bucket labelled *"other /
stalemate"*, and PLAN 5.9.21 treated that as a stalemate rate. It is not one.
New instrument (`tools/diag/kbnk_outcomes.py`): the bucket is split, the
denominator is Syzygy-validated, and each event is recorded by **ply** rather
than halfmove clock.

| outcome | n | share of confirmed wins |
|---|---:|---:|
| mated | 94 | 47.5% |
| **fifty-move draw** | **77** | **38.9%** |
| stalemate | 16 | 8.1% |
| piece lost to the bare king | 11 | 5.6% |

*(198 of the 200 generated positions are true wins; the other 2 are genuine
draws — random placement can trap a piece. The first 16, which the CTest floor
uses, are 16/16 wins.)*

**Both failure modes are artefacts of the fifty-move boundary, not defects.**

| event | ply, min-median-max |
|---|---|
| stalemate | 91 · **95** · 99 |
| piece lost | 98 · **100** · 100 |

**Zero stalemates below ply 80.** Every one lands within 1-5 plies of a
fifty-move draw the engine was going to take anyway, so it costs nothing; the
same holds for the piece giveaways at ply 98-100. The engine is not blundering
into stalemate — it is running out of clock and then behaving indifferently,
correctly, in a position whose result is already settled.

**So 5.9.21 has nothing to implement, and the piece-loss finding earns no step
either.** The single remaining KBNK defect is **technique speed**: 38.9% hit the
fifty-move rule (5.9.22).

*Two measurement traps this cost, both already on the record in this project.*
First, an aggregate bucket ranked a step that does not exist — the same error as
grading endgames by frequency (5.9.7) and trusting a mean over an even split
(5.7.4). Second, my own first instrument used the **halfmove clock**, which a
capture resets, so every piece loss looked like it happened at move 0; the ply
axis reversed the reading. A third instrument bug caught in passing: reusing one
engine process across 200 games let the TT carry over and the run was not
reproducible — `ucinewgame` per position fixed it.

*Also corrected:* my first reading of the ply-1 stalemate `6Bk/8/7K/8/6N1/8/8/8 w`
called it a blunder. It is a **drawn** position: black's only legal move is
Kxg8, every bishop move on the g8-a2 diagonal covers g8 and stalemates, and
everything else drops the piece to a bare king. Syzygy confirms. The engine
played it correctly.
