# BAS-E37

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E37 — 6.1.c: KBNK diagonal dominance screen selects `1000,0,220,0`**
(2026-09-03). Ten coefficient vectors in `diagonal,edge,king,knight` order were
run at 60,000 nodes/move on the identical first 60 positions of the frozen
BAS-E35 198-win cohort. Each arm used 30 independent one-thread engine workers,
engine tablebases disabled, a 100-ply limit, and no score adjudication.

| vector | converted | paired gain/loss vs baseline | clean wins discarded | DTZ progress | hard anomalies |
|---|---:|---:|---:|---:|---:|
| baseline `800,900,220,220` | 26/60 | — | 10 | 47.45% | 0 |
| diagonal `1000,900,220,220` | **31/60** | 19/14 | 12 | 49.74% | 0 |
| **dominant diagonal `1000,0,220,0`** | **31/60** | **15/10** | **3** | **49.85%** | **0** |
| no edge `800,0,220,220` | 30/60 | 10/6 | 7 | 49.87% | 0 |

The preregistered rule breaks the conversion tie in favour of
`1000,0,220,0`: it preserves substantially more clean wins and is the simpler
shape, removing the generic edge pull and knight-distance pull while retaining
the strong-king approach term. It also reduced stalemates from 7 to 2 and
raised per-move clean-win preservation from 99.5800% to 99.8676%. This is a
**provisional candidate**, not a 60-position claim of general superiority:
15 baseline failures converted but 10 baseline successes regressed. Step 6.1.e
must confirm it on all 198 frozen wins before the mechanism is accepted.
Summary SHA-256:
`ED0A554855D9B61273E968EDF73A5FBEE96046AA5E8F5D0DBCC1616A77FAAEDF`.

**Correction (2026-09-03): not accepted; 6.1.c reopened.** The first-pass
parameterization was `(7 + diagonal) * weight`. Raising 800 to 1000 therefore
changed both the diagonal slope and a class-wide offset (+1,400), while the
selected arm simultaneously removed the edge and knight pulls. Its result is
valid as a measured bundle but cannot establish which mechanism caused it.
The default was re-expressed without behavior change as
`base=17000, diagonal=1000, edge=0, king=220, knight=0`; a corrected paired
refinement now varies base, diagonal and king independently. This correction
must close before the 198-position confirmation.
