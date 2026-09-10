# HCE maturity — verdict at the 5.9.3 structure freeze

**Date:** 2026-08-25. **Question:** is the HCE now mature, or does it need more?
**Answer: term coverage is mature; endgame knowledge is deliberately partial.**

## 1. Named terms — complete

Every one of the **31 named `constexpr Score` terms** in the reference now has a
Basilisk counterpart. The six that BAS-E07 found absent were added at 5.9.1, and
5.9.2 repaired the two that existed in a weaker functional form.

The shapes match too, which matters more than presence: threats are graded
arrays over the attacked piece type, mobility is a per-count table per piece,
rook-on-file is decomposed into open and semi-open, hanging is indexed, and
king-protector is now per-piece. Nothing is a scalar standing in for a table.

**On this axis the evaluator is at reference parity and the freeze is safe.**

## 2. Endgame knowledge — partial, and deliberately so

This is the real gap. We carry six rules:

| rule | kind |
|---|---|
| KNNK | exact draw |
| KPK | exact bitbase |
| KBNK | mate drive to the correct corner |
| KBP vs K, wrong rook-file bishop | exact draw |
| no pawns, ≤ minor advantage (KmK, KmKm, KRKm) | scale |
| opposite-coloured bishops | scale |

The reference carries roughly **29** classes. Genuinely absent here: rook-ending
scaling (`KRPKR`, `KRPKB`), rook versus pawn (`KRKP`), queen versus rook
(`KQKR`), rook versus minor (`KRKB`, `KRKN`), the bishop-pawn scale family
(`KBPKB`, `KBPKN`, `KBPPKB`), and a generic bare-king drive (`KXK`) beyond the
KBNK special case.

**They are not being added before the fit, and that is a decision rather than an
oversight.** Four reasons, in order of weight:

1. **They are not linearly fittable.** Endgame knowledge is scale factors and
   exact evaluations — caps, multipliers and early returns. BAS-X14 records
   precisely why a linear count model misprices that class, so they would sit in
   the *excluded* set at 5.9.4 and gain nothing from the fit. The entire
   rationale for landing 5.9.1/5.9.2 inert was that the fit would price them;
   that rationale does not extend here.
2. **This exact class lost Elo in the sibling project.** `MAN-E05` graded
   endgame conversion and lost **−16.32 Elo**, and its post-mortem was that it
   "graded conversion while having no recogniser able to say whether an ending
   was winnable".
3. **Our own canary history is hostile to hand-seeded endgame constants.** Eight
   consecutive mechanisms tripped the KBNK/KQK mate tests at their literature
   seeds. An endgame rule cannot be seeded inert in the way an additive term can:
   a scale factor of 1.0 is inert, but the *recogniser* that selects it is
   behaviour from the first line.
4. **The six we have cover the highest-frequency drawish classes.** OCB and
   no-pawn scaling catch most of what a fast-TC game actually reaches.

## 3. What this means for the freeze

**Structure is frozen for the linear surface**, which is what 5.9.4 fits. The
endgame gap is recorded as a **separate, later program** with its own risk
profile, not folded into this one.

Freezing an incomplete structure would normally be the error ADR-0050 warns
about — a well-calibrated incomplete evaluator, needing a refit after every
later addition. That warning applies to *fittable* structure, and the linear
surface is complete. Endgame rules sit outside the fit entirely, so adding them
later does not invalidate the coefficients this fit produces.

**Retry trigger for the endgame program:** open it only after 5.9.6 returns a
verdict, and design it recogniser-first — a rule that decides whether an ending
is winnable before anything grades how well it converts. That is the correction
`MAN-E05`'s own post-mortem demanded and never received.

## 4. Verdict

| axis | state |
|---|---|
| Named term coverage | **mature** — 31/31 |
| Term functional form | **mature** — no scalar standing in for a table |
| Trace/fit readiness | **mature** — `--verify` exact, every term fires |
| Endgame knowledge | **partial** — 6 rules against ~29, deferred with cause |
| Calibration | **not yet** — that is 5.9.4 and 5.9.5 |

The evaluator is ready to be fitted. It is not yet a finished classical
evaluator, and the remaining distance is endgame knowledge plus the calibration
that BAS-E07 identified as the real gap in the first place.
