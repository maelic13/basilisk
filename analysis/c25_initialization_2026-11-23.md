# C.2.5 initialization control (2026-11-23)

- State / class: closed, `R2`. The control is run (BAS-O12, with its
  Correction 1), and the production rule is registered in §4 before the
  test split opens.
- Owner / date: agent, PLAN C.2.5. The run was made overnight on the
  maintainer's leave. The maintainer left both decisions to the agent,
  asking for the reasoning to be checked, and the check changed it (§2).
- Decision: C.2.8 seeds the piece-square tables from Stockfish 16 at
  0.35, as BAS-O12's frozen rule gives, from one start.

**Decision.** C.2.8 has one start: `sf_16`'s table shapes at 0.35, level
held, and every other slot from the header.
- Why 0.35: the frozen rule gives it, and the data agree with the rule's
  branch. No single slope converts a Stockfish 16 table into a fitted
  Basilisk one: the converged tables' own slopes run from −1.21 to +3.0.
- Why it costs nothing: seeds at 0.557 and 0.613 end inside the
  instrument's resolution of each other.
- Why one start: the header start's advantage, 2.1e-5 on the tuner's
  model, is 1e-7 on the fitted vectors' realized losses. That is below
  what one linear stage resolves. The Stockfish 16 start is the
  maintainer's 2026-11-18 decision, which ends the PeSTO derivation.
- **The first recommendation is withdrawn:** 0.557, with both starts
  fitted in C.2.8. It rested on two misreadings that a re-read found:
  - the eg tables' 0.57 is mostly the eg king table's slope;
  - the flipped start was not the negated shape.

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

## 2. Results (BAS-O12 and its Correction 1)

| Arm | Model loss | Realized loss | Pooled `b`, start → fit | `b` mg | `b` eg |
|---|---:|---:|---|---|---|
| H | 0.0961773 | 0.0962263 | 0.354 → 0.355 | 0.296 → 0.252 | 0.482 → 0.578 |
| S557 | 0.0962005 | 0.0962093 | 0.557 → 0.393 | 0.557 → 0.314 | 0.556 → 0.567 |
| S613 | 0.0961981 | 0.0962262 | 0.612 → 0.398 | 0.612 → 0.320 | 0.613 → 0.569 |
| F613 | 0.0962145 | 0.0962452 | 0.168 → 0.300 | −0.009 → 0.173 | 0.553 → 0.576 |

- **The model loss is not the vector's loss.**
  - The linear stage holds each row's lazy checkpoint and endgame scale
    where its seed put them. Each arm is its own linear model, and the
    fitted vectors send 5,500 to 5,900 more validation rows through the
    lazy checkpoint than the header does.
  - Realized (`--loss --init`), the model is off by 0.9e-5 to 4.9e-5.
    That is larger than the start effect, so one linear stage resolves
    arms no better than about 2e-5.
  - By band, S613 against H swings between −4.3e-5 and +5.0e-5, with
    mixed signs.
- **The tables are identified, all but the mg king's.**
  - Between arms, every mg table but the king's ends within 0.4 to
    2.6 cp RMS of every other arm's. Every eg table ends within 4 cp
    except the queen (up to 10.3) and the pawn (up to 11.8, against the
    flipped arm).
  - The mg king ends 17.5 cp (H against S613) and 36.5 cp (S613 against
    F613) apart.
  - The eg king converges (2.0 to 3.7 cp). The eg queen and eg pawn
    converge more slowly.
  - So the starts differ mainly in the one table the data cannot pin
    down.
- **No single slope converts a table.** Each converged table's slope on
  Stockfish 16's shape is the same in every arm, and the slopes range
  from −1.21 (mg queen) to +3.0 (eg pawn).
  - The pooled `b` weights each table by Stockfish 16's spread, and the
    king tables carry 63% of it in mg and 56% in eg.
  - So the pooled slope, and the eg 0.57 that matched B.0, are mostly the
    king's. The rule's branch, "neither ratio describes a table", is
    right.
- **Orientation.** A rank flip barely moves most eg tables, so F613 was a
  weak test, and its registered read failed. The identified
  rank-asymmetric tables (pawn, rook) reach the same shapes from the
  flipped start. The mg king keeps much of its seed's orientation, and
  the eg pawn some. There the seed is right by construction: in the
  a1-first White view, the mg king's first rank is 14 to 93 cp and its
  eighth −53 to −108.

## 3. Finding for C.2.8: the tables re-level

Every arm moves each table's level a long way against the header:

| Table | Level change (cp) |
|---|---|
| mg pawn | −126 to −163 (H the lowest) |
| eg pawn | +32 to +39 |
| eg knight | +85 to +90 |
| eg bishop | +90 to +95 |
| eg rook | +169 to +179 |
| eg queen | +127 to +141 |

With material held at 83, the effective mg pawn ends between −28 and
−65 cp before its other terms. That needs explaining before a bake.
Inference, with two leads from the fitted vectors:
- Queen mobility at 0 and 1 squares sits on the tuner's −200 clamp, mg
  and eg, in every arm. A mobility table's usage-weighted level is a
  piece level, so the queen's PST level may be compensating for a railed
  table.
- `ConnectedRankMg` index 6 goes from 0 to 208 to 215, a pawn-count-like
  term against the mg pawn level.

It is read where it is used: C.2.8's fit report, beside the registered
king-table tree read (C-56).

## 4. The production rule (registered 2026-11-23, before the test split opens)

- **Start.** C.2.8's piece-square tables start from
  `tools/texel/seed_sf16.py --scale 0.35`. That is `sf_16`'s shapes at
  0.35 Basilisk centipawns per Stockfish unit, level held, and every
  other slot from the header at C.2.8's root. There is one start, and no
  selection among starts.
- **The start is a named variable.** BAS-O12 bounds its effect below
  2e-5 on corpus A. A header-start arm is not part of C.2.8.
- **Losses are read realized.** Arms are compared by the fitted vectors'
  `--loss --init`, which `fit.py` already does, never by one stage's
  model loss.
- **Thresholds.** A ported donor threshold, which no refit absorbs, is
  converted by the leaf that ports it. The candidates are B.0's 0.557 by
  magnitude and C.0's 0.613 by outcome.
- **Retry trigger.** A header-start fit becomes the cheapest
  discriminating arm in either case:
  - C.2.8's king-table read (C-56) flags the mg king, the table the data
    does not identify;
  - its gate fails while the tables' loss improves.
