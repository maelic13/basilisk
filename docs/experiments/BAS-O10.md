# BAS-O10

**BAS-O10 — H-distribution: is the evaluation gap wider one move off the game line than on it? (C.2.2's first read)**

- Date / owner / calibration category: registered 2026-11-22, before any
  measurement beyond a 40-pair dry run of the instrument; maintainer-run,
  agent-read; measurement.
- Research question (C.0 packet §5.4, §7.5; Rarog's C-124): C.0 found the
  evaluation deficit visible on quiet game positions. Is it wider on the
  positions a search visits and a game does not? If so, a corpus of game
  positions fits the wrong population, and corpus A needs starts off the
  game line.
- Instrument: `tools/diag/h_distribution.py` (committed with this file).
  - **Positions:** 10,000 pairs, seed `20261122`. **G** is a position
    sampled from C.0's gate-game positions (`tools/results/c0-screen/positions.csv`,
    254,486 rows). **O** is its sibling one random legal move away that does
    not end the game.
  - **Reference:** Stockfish 19 (`stockfish-19-windows-x86-64-universal.exe`,
    network), `go nodes 1000000`, Hash 64, one thread, `ucinewgame` each
    position, `UCI_ShowWDL`. The label is the side to move's expected score,
    `(W + D/2) / 1000`.
  - **Evaluations, static, the side to move's view:**
    - Basilisk's full traced evaluation (`basilisk-texel --dump-eval`, the
      head's source);
    - `9587eeeb`'s (`oracle-b111-4d2742f.exe`, `Use Basilisk HCE=false`);
    - Stockfish 16's classical one (`Use NNUE=false`).

    A pair is kept only when every evaluation and both references exist;
    Stockfish 16 prints none in check.
  - **Score:** per evaluation and population, the logistic scale `k` is
    fitted on even pairs by cross-entropy, and skill
    (1 − cross-entropy / the constant predictor's) is read on odd pairs.
    `gap(E, P) = skill(E, P) − skill(Basilisk, P)`, and
    `delta(E) = gap(E, O) − gap(E, G)`, each with a 95% interval from 2,000
    bootstrap resamples of pairs.
- Command (about 30 minutes on 14 cores):
  `python tools/diag/h_distribution.py run --out tools/results/c22/hdist`,
  with `--tuner` naming a head `TEXEL=ON` build.
- **PRE-REGISTERED PREDICTION (frozen 2026-11-22):**
  - `gap(d16, G)` +0.03, 80% band +0.01 to +0.05: the static deficit C.0
    saw.
  - `delta(d16)` +0.01, 80% band −0.02 to +0.04.
  - SUPPORTED (below) at 0.35.
- **Verdict rule, frozen:**
  - *Validity.* `gap(d16, G)` and `gap(d95, G)` have lower 95% bounds above
    0: the instrument sees the deficit C.0 measured on game positions.
    Otherwise the read is uninformative and corpus A is designed without
    it.
  - *SUPPORTED* when `delta(d16)` and `delta(d95)` both have lower bounds
    above 0. Corpus A's book then takes 20% of its starts one random legal
    move off a sampled pool position, and the game played from there labels
    it (`analysis/c22_corpus_design_2026-11-22.md` §5).
  - *NOT SUPPORTED* otherwise. Corpus A uses pool positions as starts only,
    and the question stays with C.11's deficit re-read.
- Confounds:
  - The reference is a network, not Basilisk's own re-search (C-91); it is
    the stronger and the common one.
  - A random move often leaves material en prise, so O is harder for every
    evaluation; the read is the contrast between evaluations, not their
    level.
  - Loss against a reference is not Elo.
- Stop rule: one run, never re-run on the same binaries and positions.

#### RESULT — append after exposure
