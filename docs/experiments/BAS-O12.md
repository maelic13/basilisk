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

- **Correction 1 (2026-11-23, a re-read before C.2.5 closed;
  `tools/results/c25/verify_o12.py`, `real_losses.py` and their `.txt`
  outputs).** Four statements in the RESULT are wrong. The verdicts are
  restated below.
  - *F613 did not start at* `b` *−0.613.* A rank flip is not a negation.
    The flipped seed's `b` was +0.168 pooled, −0.009 mg and +0.553 eg:
    most eg tables are nearly rank-symmetric, so the flip left them in
    place. "The fit turns a mirrored eg table back" and "F613 from
    −0.613" are withdrawn.
  - *The mg shapes are identified, all but the king's.* Between arms, the
    RMS shape distance falls to 0.4 to 2.6 cp in every mg table but the
    king's, which goes from 51.4 to 17.5 cp (H against S613) and from 90.2
    to 36.5 (S613 against F613). Every eg table ends within 4 cp, except
    the queen (up to 10.3, H against S613) and the pawn (up to 11.8,
    against F613). The pooled `b` moved with the start because Stockfish 16's
    king tables carry 63% of the mg spread and 56% of the eg: the pooled
    slope is mostly the king's.
  - *No single* `b` *converts a table.* The converged tables' own slopes
    run from −1.21 (mg queen) to +3.0 (eg pawn), the same in every arm.
    The eg 0.57 that matched B.0 is mostly the eg king's 0.55 and the eg
    queen's 1.33, averaged; it is not a conversion.
  - *The losses read were the tuner's model, not the fitted vectors'.*
    The linear stage holds each row's lazy checkpoint and endgame scale
    where its own seed put them, so four arms are four different models.
    Realized by `--loss --init` at K 2.009866 on validation (the header
    sends 21,034 rows through the lazy checkpoint):

    | Arm | Model | Realized | Gap | Lazy-checkpoint rows |
    |---|---:|---:|---:|---:|
    | H | 0.0961773 | 0.0962263 | +4.9e-5 | 26,909 |
    | S557 | 0.0962005 | 0.0962093 | +0.9e-5 | 26,680 |
    | S613 | 0.0961981 | 0.0962262 | +2.8e-5 | 26,693 |
    | F613 | 0.0962145 | 0.0962452 | +3.1e-5 | 26,528 |

    | Band, realized | H | S557 | S613 | F613 | S613 − H |
    |---|---:|---:|---:|---:|---:|
    | opening | 0.1644140 | 0.1643879 | 0.1644304 | 0.1644418 | +1.6e-5 |
    | early_mid | 0.1317463 | 0.1316984 | 0.1317035 | 0.1317666 | −4.3e-5 |
    | middlegame | 0.0841586 | 0.0841608 | 0.0841694 | 0.0841994 | +1.1e-5 |
    | endgame | 0.0411445 | 0.0411037 | 0.0411137 | 0.0410905 | −3.1e-5 |
    | deep_endgame | 0.0441991 | 0.0442315 | 0.0442488 | 0.0442550 | +5.0e-5 |

    Realized, H and S613 end 1e-7 apart, and S557 ends 1.7e-5 below
    both, where the model put S557 and S613 2.4e-6 apart. One linear
    stage resolves arms no better than about 2e-5. `fit.py` re-linearizes
    at every stage and reads its losses realized, so C.2.8 does not
    inherit this.
- **The verdicts, restated:**
  - *Scale: the frozen rule stands, and C.2.8 seeds at 0.35.* H's pooled
    `b` is 0.3547, under 0.40, so neither ratio describes a table. The
    correction strengthens that branch's premise, and it withdraws the
    RESULT's case for 0.557. Seeds at 0.557 and 0.613 end inside the
    instrument's resolution of each other, so the scale costs the fit
    nothing measurable. A ported threshold, which no refit absorbs, is
    left to the leaf that ports it, as the rule says.
  - *Start: below this instrument's resolution.* It is outside 2e-5 on
    the registered read (2.1e-5) and 1e-7 realized. The rule's branch
    for a difference is followed: it is stated by band above, and
    C.2.8's registration names its start.
  - *Orientation: the registered read failed, and could not decide it;
    the seed is right by construction.* The flip barely moves the eg
    tables, and the travel read is dominated by level moves every arm
    shares.
    - The identified tables reach the same shapes from the flipped
      start, so their orientation is set by the data.
    - The mg king keeps much of its seed's orientation (36.5 cp from the
      flipped arm's fit), and the eg pawn some (8.2 cp). There the seed
      is right by construction: in Basilisk's a1-first White view the mg
      king's first rank holds 14 to 93 cp and its eighth −53 to −108.
    - The flipped start ends worse on both losses (1.6e-5 model, 1.9e-5
      realized), at the resolution.
  - *The level finding stands,* with H's mg pawn 35 cp below the
    others. Two leads for C.2.8:
    - Queen mobility at 0 and 1 squares sits on the tuner's −200 clamp,
      mg and eg, in every arm.
    - `ConnectedRankMg` index 6 goes from 0 to 208 to 215.
- **Against the prediction, amended:** three parts of the model were
  wrong, not one.
  - A single linear stage's model loss was taken for the fitted
    vector's loss.
  - The flipped start was taken for the negated shape.
  - One pooled slope was taken to describe twelve tables.
