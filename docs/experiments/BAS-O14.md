# BAS-O14

**BAS-O14 — C.2.10: the matched evaluation ablation by family on the refitted head**

- Date / owner / category: registered 2026-11-24, before any game;
  maintainer-run, agent-read; measurement.
- Binary: `tools/test_engines/oracle-c29-e56f1e1-evalablate.exe`,
  SHA-256 `CAA8B70B…`, Stockfish `9587eeeb`'s search around the head's
  evaluation, played against itself with `Use Basilisk HCE=false` and
  `=true` at one `EvalAblationMask`.
- Research question: how much of the same-search evaluation deficit does
  each family, and each cluster, explain? M(mask) = E(0) − E(mask).
- Reads: masks 0, 7, 8, 48, 64, 127, 2, 1, 4, 16 and 32, in that order,
  3,000 games each.
- Registration, confounds, the decision rule, predictions and
  falsifiers: `analysis/c210_family_order_2026-11-24.md`, frozen in the
  commit that adds this file.
- Registered design and stop rule: no gate; one fixed run per read,
  never extended or re-run on the same binary.
- Full conditions: `python tools/run_c210_reads.py --read all`; Colosseum
  CLI, two-engine gauntlet, 1,500 cycles, UHO `UHO_Lichess_4852_v1.epd`
  in random order, seed `210001` for every read, `3+0.03`, margin 20 ms,
  1T, Hash 64, concurrency 14, no adjudication, no tablebases, zero
  permitted faults.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-24

Elo, with an 80% band.
- E(0): +160, +130 to +190.
- M(7), mobility, space and threats: +55, +25 to +90.
- M(8), king: +20, −5 to +45.
- M(48), pawns and passers: +12, −15 to +40.
- M(64), pieces: +5, −12 to +22.
- M(2), mobility: +35, +10 to +65.
- M(1), space: +12, −5 to +30.
- M(4), threats: +8, −12 to +28.
- M(16), pawn structure: +8, −12 to +28.
- M(32), passers: +5, −18 to +28.
- E(127): +70, +35 to +110.
- C.3's is the largest cluster meter: 0.7. The frozen rule leaves the
  order unchanged: 0.75. Mobility's single meter exceeds space's: 0.75.
  At least three single meters include zero: 0.8.
- Falsifiers: E(0) above +205; E(0) − E(127) below 40.

#### RESULT — append after exposure

- 2026-11-25. Eleven reads of 3,000 games, each complete: no engine
  fault, no time loss, none refused. Table and evidence:
  `tools/results/c210-table.md`, built by
  `python tools/diag/c210_table.py` from `tools/results/c210-<read>/`.

  | Read | Families removed | E(mask), 95% | M(mask), 95% | Score |
  |---|---|---|---|---|
  | `e0` | none | +146.2 [+137.5, +155.1] | — | 69.9% |
  | `m7` | space, mobility, threats | +198.2 [+188.1, +208.6] | −52.0 ± 13.5 | 75.8% |
  | `m8` | king | +103.1 [+93.3, +113.1] | +43.1 ± 13.2 | 64.4% |
  | `m48` | pawn structure, passers | +51.3 [+41.2, +61.5] | +94.9 ± 13.4 | 57.3% |
  | `m64` | piece terms | +184.0 [+174.4, +193.8] | −37.7 ± 13.1 | 74.2% |
  | `m127` | all seven | −16.6 [−27.4, −5.8] | +162.8 ± 13.9 | 47.6% |
  | `m2` | mobility | +192.2 [+182.3, +202.4] | −46.0 ± 13.3 | 75.1% |
  | `m1` | space | +148.3 [+139.5, +157.3] | −2.1 ± 12.5 | 70.1% |
  | `m4` | threats | +156.1 [+146.8, +165.6] | −9.9 ± 12.9 | 71.1% |
  | `m16` | pawn structure | +179.5 [+169.9, +189.3] | −33.2 ± 13.1 | 73.8% |
  | `m32` | passers | +20.9 [+11.5, +30.3] | +125.4 ± 12.9 | 53.0% |

- Checks:
  - Every arm is inside 20% to 80%.
  - Basilisk's arm runs within about 4% of its mask-0 speed on every
    read but `m127`, which is flagged (2.96M against 2.52M nodes a
    second).
  - Clamped elementary-mate draws: 16 to 40 a read.
- Not additive. The seven single meters sum to +39.6 and the four
  cluster meters to +48.3, against +162.8 together. Inside a cluster
  they nearly add: −58.0 against −52.0 for space, mobility and threats,
  and +92.1 against +94.9 for the pawns' two.
- Neither falsifier fired: E(0) is +146.2 and E(0) − E(127) is 162.8.
- The passers' single meter stands 30.5 above its cluster's read, over
  the 21 the packet asked to be explained. Pawn structure's own read,
  −33.2, is the explanation the packet named.
- The rule applied and the order: the packet's §11.

#### PREDICTION CALIBRATION

- Inside the 80% band, 4 of 11: E(0), M(8), M(1) and M(4).
- Missed, 7 of 11: M(7) −52.0 for +55; M(48) +94.9 for +12; M(64) −37.7
  for +5; M(2) −46.0 for +35; M(16) −33.2 for +8; M(32) +125.4 for +5;
  E(127) −16.6 for +70.
- All four ranked predictions missed:
  - C.3's is the smallest cluster meter, not the largest (0.7);
  - the rule changes the order (0.75);
  - space's single meter is above mobility's (0.75);
  - two single meters include zero, not three (0.8).
- What was wrong in the model:
  - It took the screen's ranking, the donor's family added to
    Basilisk's score, as the prior for games. The meter measures what
    each family is worth to its own evaluation, and the two rank the
    families differently: space leads the screen on gate games and
    reads −2.1 here; the passers add 5.4 in the screen and read +125.4
    here.
  - It assumed level would raise mobility's meter. The two mobility
    terms carry the same level: slope on own material 0.019 and 0.021
    (`tools/results/c210-close/level_check.py`, corpus A's validation
    rows).
- After the fact, and not a prediction: the static cost of removing
  each evaluation's own family (`own_ablation.py`, same folder) has the
  meters' signs for the passers, the king, pawn structure and mobility,
  and reads zero for space, threats and the piece terms.
