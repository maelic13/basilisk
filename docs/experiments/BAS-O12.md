# BAS-O12

**BAS-O12 — C.2.5's initialization control: does the PST start change the fit, and what scale converts a Stockfish 16 table?**

- Date / owner / category: registered 2026-11-23 before any fit was run.
  Exposure so far: the four seeds' unfitted validation losses at the frozen
  K, which decide nothing here. Agent, overnight on the maintainer's leave;
  measurement.
- Question (PLAN C.2.5; C-23, C-65): C.2.8 fits the piece-square tables
  from classical Stockfish 16's (maintainer, 2026-11-18). Three questions:
  - Does that start converge where the accepted start does?
  - Is a donor table converted at 0.557 (B.0, by magnitude) or at 0.613
    (C.0, by outcome)?
  - Is the seed's orientation right?
- Instrument:
  - The seeds are written by `tools/texel/seed_sf16.py`. Each takes
    `sf_16`'s `psqt.cpp` shape (pinned `68e1e9b3`) and keeps each table's
    current level, so material and effective piece values are the header's.
  - The fits use `basilisk-texel --tune surface` on corpus A
    (`tools/texel/data/corpusA/rows`): train against validation, the
    surface `surface_v1.tsv`, K frozen at 2.009866 (validation at the
    header), `--l2 0`, `--lr 0.3`, `--epochs 3000`, `--patience 50`,
    `--max-positions 0`.
  - There are four arms, identical but for `--init`:
    - **H**, the header (the accepted start);
    - **S557** and **S613**, Stockfish 16's shape at those scales;
    - **F613**, S613's tables mirrored by rank.
- Reads:
  - each arm's best holdout loss, and how far its tables travelled from
    its start (mean absolute change over the PST slots);
  - **the scale:** pool the twelve tables' live squares, each table
    demeaned, and fit H's tables by least squares on Stockfish 16's
    demeaned shape. The slope `b` is the outcome-equivalent conversion of a
    table, read from a fit that never saw Stockfish 16's values. The same
    is read for mg and eg apart, and on S613's fit.
- **PRE-REGISTERED PREDICTION (frozen 2026-11-23):**
  - Start-independence: H and S613 converge within 2e-5 holdout loss of
    each other, probability 0.7.
  - Slope: H's `b` lies in 0.30 to 0.80 at 80%, point 0.55. It is closer
    to 0.613 than to 0.557 at 0.45.
  - Orientation: F613 travels further than S613, probability 0.9.
- **Verdict rule, frozen:**
  - *Scale:* the program converts a donor table at whichever of 0.557 and
    0.613 is nearer H's pooled `b`, and C.2.8 seeds at it.
  - If `b` is under 0.40 or over 0.80, neither ratio describes a table:
    the seed uses `b` rounded to two places, and the 10% question for
    thresholds is left to the leaf that ports them.
  - *Start:* if H and S6xx (the chosen scale) converge within 2e-5, the
    start is immaterial to the fit, and C.2.8 seeds from Stockfish 16 as
    decided. If not, the packet states the difference by band and C.2.8's
    registration names its start as a variable.
  - *Orientation:* the seed is accepted if S613 travels less than F613 and
    reaches no worse a holdout loss.
- Confounds:
  - A fit that stops on patience is a floor, not an optimum. Each arm
    reports its last-epoch improvement.
  - The surface's other terms co-adapt with the tables, so `b` is the
    tables' scale under Basilisk's other terms, not under Stockfish 16's.
- Stop rule: one run per arm, never re-run on the same inputs.

- **Amendment 1 (2026-11-23, after the first run, before any read of a
  fitted vector).**
  - *What failed:* the registered schedule did not converge. H, S557 and
    S613 were still improving at epoch 3,000, by about 3e-8 an epoch, so
    the tuner exited 3 and wrote no vector; F613 wrote one.
  - *Exposed:* the best holdout losses at epoch 3,000, which are floors: H
    0.0961693, S557 0.0962042, S613 0.0962045, F613 0.0962163.
  - *The instrument:* the tuner gains `--min-delta` (`1f16f4c`, tested in
    `test_fit.py`). Patience then counts only gains above it, and the
    travel guard still refuses a run its schedule carried.
  - *The amended schedule,* every arm re-run with nothing else changed:
    `--epochs 20000 --patience 50 --min-delta 1e-6`. That stops when 50
    epochs gain under 1e-6, a twentieth of the start-independence
    tolerance.
  - The predictions and the verdict rule stand as frozen. The exposed
    floors already put H ahead of S613 by 3.5e-5, outside the 2e-5
    tolerance, and the converged run decides.

#### RESULT — append after exposure

- **Run 2026-11-23 under Amendment 1** (`tools/results/c25/`, tuner `1f16f4c`;
  reads by `tools/results/c25/read_o12.py`, `o12_report.json`). Every arm
  stopped on patience, so all four converged under the tolerance:

  | Arm | Stopped | Best holdout | Pooled `b` | `b` mg | `b` eg |
  |---|---:|---:|---:|---:|---:|
  | H | 2,699 | 0.0961773 | 0.355 | 0.252 | 0.578 |
  | S557 | 3,109 | 0.0962005 | 0.393 | 0.314 | 0.567 |
  | S613 | 3,200 | 0.0961981 | 0.398 | 0.320 | 0.569 |
  | F613 | 3,074 | 0.0962145 | 0.300 | 0.173 | 0.576 |

  The header's own tables, unfitted, regress on Stockfish 16's shape at
  0.354.
- **Start: it matters, slightly.** H converges 2.1e-5 below S613, and the
  3,000-epoch floors of the first run put it 3.5e-5 below. Both are
  outside the 2e-5 tolerance, so by the frozen rule C.2.8's registration
  names its start as a variable. The two scales converge 2.4e-6 apart: a
  fit does not care whether the seed is converted at 0.557 or 0.613.
- **Scale: by the frozen rule, 0.35; the read's premise failed.**
  - H's pooled `b` is 0.355, under 0.40, so the rule seeds at 0.35.
  - But the rule assumed a fit that travels to a data-determined shape,
    and the mg shapes do not. `b` stays where each start put it: H goes
    from 0.354 to 0.355, S613 from 0.613 to 0.398, and F613 from −0.613
    to 0.300.
  - The eg shapes do converge: 0.567 to 0.578 from all four starts, the
    flipped one included. That is B.0's 0.557, not C.0's 0.613.
  - So the pooled 0.35 measures the mg tables' weak identification, not a
    conversion. The scale question goes back to the maintainer
    (`analysis/c25_initialization_2026-11-23.md`).
- **Orientation: the registered read failed; the seed is right
  (inference).**
  - On the registered read, mean absolute PST change, F613 travelled less
    than S613 (76.1 against 77.9 cp), against the prediction.
  - Post-hoc, the travel splits into level and shape. Each table's level
    moves the same in every arm, 66 to 69 cp on average. Shape travel is
    S613 27.8 and F613 29.3.
  - S613 also reaches a holdout 1.6e-5 better.
  - From the flipped start, the fit turns the eg tables back to Stockfish
    16's orientation (`b` eg +0.576) and the mg ones part of the way
    (+0.173 from −0.613).
- **A finding for C.2.8:** every arm re-levels the tables by the same large
  amounts. Against the header, mg pawn moves −126 to −163 cp, eg rook +169
  to +179, eg queen +127 to +141, and eg knight and bishop +85 to +95. The
  data moves effective piece values a long way against the fixed material.
  That is either a real re-valuation, or a near-collinear direction the
  rank check passes. C.2.8 reads it before any bake.
- **Against the prediction:**
  - Start-independence (0.7): false, by 2.1e-5.
  - `b` 0.355 is inside the 80% band (0.30 to 0.80), well below the point
    0.55, and near neither ratio.
  - The orientation prediction (0.9) failed on its registered read.
  - The wrong part of the model: a converged linear fit was taken to
    identify every table's shape. It identifies the eg shapes and the
    levels, not the mg shapes.
