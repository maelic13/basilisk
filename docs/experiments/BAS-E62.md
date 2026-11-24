# BAS-E62

**BAS-E62 — C.2.10: the family screen both ways on the refitted head**

- Date / owner / category: registered 2026-11-24, before any read;
  agent; measurement, static layer.
- Tool: `tools/diag/family_screen.py`, `collect` then `screen`, with the
  head's tuner.
- Inputs: corpus A's validation split
  (`tools/texel/data/corpusA/rows/validation.csv`), 60,000 positions;
  donors `donor-sf9587-stock.exe` and `donor-sf16-stock.exe`.
- Research question: per family and phase band, what does the donor's
  family add to Basilisk's whole score, and what does Basilisk's add to
  the donor's? It is the tiebreak for what games cannot resolve, and one
  half of C-110's `NO_CHANGE` test.
- Registration, its place in the decision rule and predictions:
  `analysis/c210_family_order_2026-11-24.md` §2.3, §4 and §5.
- Stop rule: one read on the registered inputs. A refused join or a
  donor table that does not sum is reported, never worked around.
- Evidence: `tools/results/c210-screen/`.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-24

Gains in 1e-4 nats, donor into Basilisk, at phase 18–24:
- space still has the largest gain: 0.6;
- space's gain is at least 25 (BAS-E58 read 47.0 on the 1.11.0 head):
  0.6.

Over all rows:
- the passers' gain includes zero: 0.7.

Basilisk into the donor:
- at least one family's gain excludes zero: 0.8.

A static gain is not Elo. The games of BAS-O14 decide the order.

#### RESULT — append after exposure
