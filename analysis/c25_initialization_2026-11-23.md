# C.2.5 initialization control (2026-11-23)

- State / class: research in progress, `R2`. The control has been run
  (BAS-O12). Two decisions wait on the maintainer, and the production rule
  is registered once they are made.
- Owner / date: agent, overnight on the maintainer's leave, PLAN C.2.5.
- Decision needed: the maintainer's, on §4's two items, before C.2.8
  registers.

**Recommendation.** Seed C.2.8 from Stockfish 16's tables at 0.557, and
register the start as a variable: fit both the header start and the
Stockfish 16 start, and keep the better on validation. The tables' scale
barely matters, because seeds at 0.557 and 0.613 converge 2.4e-6 apart.
Where the data identifies a shape, the endgame tables, it converts at
0.57, which is B.0's 0.557. BAS-O12's frozen rule says 0.35, but that
number is an artefact of midgame shapes the fit cannot identify.

The main counter-argument is the frozen rule itself. Overriding a frozen
rule after exposure is what the protocol forbids, and so the override is
the maintainer's call, not mine. Before C.2.8 bakes anything, its fit
report must explain why the tables re-level effective piece values by up
to 179 cp (§3).

## 1. What was built and run

- **The seed** (`tools/texel/seed_sf16.py`, `c423754`):
  - It reads `sf_16`'s `psqt.cpp` at the pinned commit `68e1e9b3`.
  - It expands the half-board piece tables (`edge_distance`) and takes
    the full-width pawn table.
  - It keeps each table's level and transplants only its shape, scaled.
    A table's level is its piece's effective value against the fixed
    material (C-19), and Stockfish 16's piece values belong with its own
    imbalance terms, which Basilisk does not have.
  - Spot values follow Stockfish 16's indexing: knight a1 −175, d4 +49,
    the king's first rank 271 and its eighth 59.
- **B.10's PeSTO-distance test cannot judge this seed** (C-22). It fails
  the correctly oriented queen and king tables, and different tables in
  the rank-mirrored arm. So orientation went to BAS-O12.
- **The tuner** gains `--min-delta` (`1f16f4c`, tested): patience counts
  only gains above it. The registered 3,000-epoch schedule did not
  converge, and Amendment 1 re-ran every arm with a tolerance of 1e-6.
- **BAS-O12:** four whole-surface linear fits on corpus A at the frozen K
  2.009866, identical but for the start. The arms are:
  - H, the header;
  - S557 and S613, Stockfish 16's shape at those scales;
  - F613, S613 mirrored by rank.

## 2. Results (BAS-O12)

| Arm | Best holdout | Pooled `b` | `b` mg | `b` eg |
|---|---:|---:|---:|---:|
| H | 0.0961773 | 0.355 | 0.252 | 0.578 |
| S557 | 0.0962005 | 0.393 | 0.314 | 0.567 |
| S613 | 0.0961981 | 0.398 | 0.320 | 0.569 |
| F613 | 0.0962145 | 0.300 | 0.173 | 0.576 |

The slope `b` comes from a least-squares fit of each arm's fitted tables,
each table demeaned, on Stockfish 16's demeaned shape.

- **The start matters, slightly.** H ends 2.1e-5 below S613 (3.5e-5 at
  equal epochs in the first run). That is 0.02% to 0.04% of the loss, and
  outside the registered 2e-5.
- **The seed's scale does not matter:** S557 and S613 end 2.4e-6 apart.
- **The eg shapes are identified.** From every start, the flipped one
  included, the fitted eg tables regress on Stockfish 16's at 0.567 to
  0.578. The fit turns a mirrored eg table back the right way up.
- **The mg shapes are not.** Their `b` stays near where each start put
  it, 0.17 to 0.32.
- **The orientation is right** (inference). The flipped start reaches a
  holdout 1.6e-5 worse and travels further in shape (29.3 against 27.8 cp).
  On the registered read it travelled less (76.1 against 77.9 cp), because
  that read is dominated by the level moves every arm shares.

## 3. Finding for C.2.8: the tables re-level

Every arm moves each table's level by about the same large amount against
the header:

| Table | Level change (cp) |
|---|---|
| mg pawn | −126 to −163 |
| eg pawn | +32 to +39 |
| eg knight | +85 to +90 |
| eg bishop | +90 to +95 |
| eg rook | +169 to +179 |
| eg queen | +127 to +141 |

It is data-driven, since it does not depend on the start. Inference, two
readings:
- the effective piece values really want re-levelling against
  Basilisk's other count-like terms (imbalance, mobility, pawn structure);
- or a near-collinear direction survives `rank`'s support threshold.

An effective mg pawn near −50 cp before its other terms is implausible
enough to need explaining before a bake. Scheduled where it is read: C.2.8's
fit report, beside the registered king-table tree read (C-56).

## 4. Decisions for the maintainer

1. **The seed scale.**
   - **0.557, my recommendation:** the eg tables, where the data
     identifies a shape, convert at 0.57.
   - **0.35:** BAS-O12's frozen rule, applied mechanically.
   - **0.613:** C.0's outcome ratio. No table read supports it.
2. **The start in C.2.8:** both arms, H and Stockfish 16, under one
   registration, and the better on validation is taken (recommended). Or
   Stockfish 16 only, as decided on 2026-11-18, accepting the 2e-5.

Once these are decided, C.2.5 registers the production rule before the
test split opens, and closes.
