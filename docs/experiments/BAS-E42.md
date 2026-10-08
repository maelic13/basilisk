# BAS-E42

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E42 — 6.1.f: the accepted KBNK vector is provably isolated, and the
mate-band bound is exact** (2026-09-03). No games decided anything here; this
is the acceptance accounting for the 6.1 change.

*Non-regression is exact, not merely absent.* KQ-K, KR-K and KBB-K were run at
100 positions each, seed 424242, 60,000 nodes/move, under three KBNK vectors:
the accepted `15600,1900,0,460,0`, the legacy `15600,800,900,220,220` and the
rejected `15600,1750,0,340,0`. All three produced **byte-identical**
per-position records in every family — KQ-K 98/100 at 99.91% preservation,
KR-K 97/100 at 99.93%, KBB-K 99/100 at 99.93%. That is stronger than a
non-regression bound: `g_kbnk_drive` feeds only `kbnk_score`, which the
dispatcher reaches only for bishop+knight against a bare king, so those
families cannot see the change at all, and the run confirms it rather than
assuming it.

*Tactical stability is likewise exact.* WAC at depth 12 gives 244/300 and
83,444,716 nodes under all three vectors, with an identical failure list.

*The mate-band bound was verified against the search, not just re-read.* Every
mate comparison in the engine — TT store/probe adjustment, null-move
verification, razoring, futility, singular extension, root reporting — uses
`MATE_SCORE - MAX_PLY`, and there is no lower threshold anywhere; 32000 - 128
= 31872 is exactly the validator's `STATIC_MATE_FLOOR`. The accepted vector's
largest legal static score is 15600 + 7*1900 + 6*460 = **31,660**, leaving 212
points of headroom, so no KBNK evaluation can be ply-adjusted as a mate score
on its way into the TT. A new test pins the boundary behaviourally:
`15600,1900,0,495,0` reaches 31,870 and is accepted, `15600,1900,0,496,0`
reaches 31,876 and is rejected. If `MATE_SCORE` or `MAX_PLY` ever moves, that
test fires instead of the engine silently storing an evaluation as a mate.

*Bench accounting.* `bench 13` is 12,709,666 nodes at geomean EBF 2.876,
identical to the pre-6.1 head. As the checklist item itself states, this is
necessary bookkeeping and proves nothing about behaviour here: the bench suite
contains no KBNK position, so the change is invisible to it by construction.
The behavioural evidence for this change is BAS-E41's held-out cohort, not
this number. Full CTest suite 12/12; test_eval 112/112; test_endgames 47/47.
The release default now plays 1.Nb3 on `KBNK0061`, the position that rejected
the 6.1.c winner.

*What is still not established.* Conversion and truth preservation on a
tablebase cohort are not Elo. 6.2 owns the no-adjudication Group A SPRT, and
nothing in 6.1 should be read as a strength claim.
