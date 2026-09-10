# PLAN 6.4.b — the four endgame measurement layers, and what each may license

**Date:** 2026-09-04
**Status:** frozen. Documentation leaf; no engine runs.

Phase 6 produced, in a single stretch of work, a case where each pair of these
layers disagreed with the other. This file names the layers, fixes what each
one may and may not be used to claim, and cites the case that forced the rule,
so the distinctions are anchored in evidence rather than in taste.

## The layers

| # | Layer | Question | Instrument | Unit |
|---|---|---|---|---|
| 1 | **Theory truth** | Did the engine throw a theoretically won position? | Syzygy WDL per move, `first_discard_ply` | one move |
| 2 | **Move quality** | Are the moves making progress toward the goal? | DTZ progress rate, win-preservation rate | one move |
| 3 | **Conversion** | Did the engine finish the win inside the rules? | outcome `mated` on a cohort root | one position |
| 4 | **Game strength** | Does it win more games? | SPRT at a real time control | one game pair |

A fifth quantity is not a layer but gates whether layers 1-3 can ever reach
layer 4: **occurrence**, how often the family arises at all
(`endgame_search_occurrence.py`, BAS-E43).

## Precedence

1. **Theory truth is an absolute veto and outranks everything below it.** A
   candidate that introduces a live clean-win discard is rejected however good
   its conversion is. *Case:* 6.1.e rejected `15600,1750,0,340,0`, which
   converted **103/138 against the accepted vector's 98/138**, on a single live
   discard — `KBNK0061`, where 2.Nc2 allows 2...Kd1 forking bishop and knight
   (BAS-E41).
2. **Conversion never establishes strength.** Only layer 4 does. *Case:* 6.1
   moved conversion 95→144 of 198 and the no-adjudication SPRT returned
   −1.40 ± 4.07 Elo, practical equivalence (BAS-E44). The conversion gain was
   real; it simply had almost no surface to act on, because KBN-K occurs zero
   times in search trees from real roots (BAS-E43).
3. **Move quality and conversion are different measurements and can move in
   opposite directions.** *Case:* at 200,000 nodes the accepted vector converts
   one *fewer* KBPP-KB root than legacy while carrying one *fewer* live truth
   discard (BAS-E51). Neither number is wrong; they answer different questions.
4. **Strength never overrides truth.** A positive SPRT does not license a
   candidate that fails a correctness gate. This direction has not yet been
   exercised in Phase 6 and is stated so it is not discovered by argument later.

## Confounds, each found the hard way

**Conversion is confounded by DTZ slack across families.** Restricting to roots
whose |DTZ| fits the remaining budget equalises *feasibility*, not *margin*. At
budget 50, a KQ-K root of median DTZ 10 has forty halfmoves spare while KBN-K's
median 50 has none, so any imperfection fails. Cross-family conversion
comparisons are therefore mostly a slack comparison unless slack is matched —
and on this cohort it cannot be, because the DTZ distributions barely overlap
(BAS-E52). **Move quality has no such problem**, being per-move: each family is
its own control across conditions.

**Conversion is confounded by the node budget.** Every 6.1 coefficient was
chosen at 60,000 nodes while a 3+0.03 game spends roughly 250,000–350,000. The
decision 6.1.e reached on a live truth discard does not reproduce at 200,000 or
600,000 nodes, where that arm has no live discard at all: the losing move is a
two-ply fork the search sees once it has a real budget (BAS-E45). **Node budget
is a first-class run condition and must be stated and justified against the
deployment time control.**

**Truth can be faked by an instrument that conflates material with winning.**
`endgame_truth.py` once ended a game whenever the strong side's piece count
fell, which is right for KBN-K and wrong for pawn technique, where giving a
pawn to promote another is the winning method. It aborted 139 of 148 such games
before the engine had played a single non-win-preserving move, and those aborts
were being read as conversion failures (BAS-E47). A layer-3 number produced by
a layer-1 category error is not a weak measurement, it is a different
measurement wearing the wrong label.

## Reporting rules

- State the layer, the instrument, the node budget and the position set for
  every number. A bare conversion percentage is not a result.
- Never aggregate layers into a single score. There is no exchange rate between
  a truth failure and a conversion gain.
- When two layers disagree, say so and say which one decides. The disagreement
  is usually the finding — 6.4.a's conclusion is precisely that DTZ progress
  and conversion disagreed and only one of them was clean.
- Bench identity belongs to none of these layers. It is a provenance
  fingerprint: necessary to show nothing unintended moved, and evidence of
  nothing about play. The bench suite contains no KBNK position, so it was
  blind to the whole of 6.1 by construction.
- Occurrence gates the ceiling on layer 4 and should be measured before
  investing in layers 1-3 for a family. It is not itself evidence of value.
