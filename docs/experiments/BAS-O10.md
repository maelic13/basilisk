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

- **Run 2026-11-22** by the agent on the maintainer's leave, overnight,
  `python tools/diag/h_distribution.py run --out tools/results/c22/hdist`,
  exit 0, 22:02 to 22:34. The tuner is the head's `TEXEL=ON` build
  (`build/texel/basilisk-texel.exe`, `3cf0903f…`);
  `positions.csv` `8a66c229…`, `evals.csv` `a75843f2…`, the report
  `tools/results/c22/hdist/report.json`.
- **Kept pairs:** 9,490 of 10,000 (Stockfish 16 prints no evaluation in
  check); 4,747 fit `k`, 4,743 read skill.
- **Skill** (1 − cross-entropy / the constant predictor's):

  | Evaluation | G | O |
  |---|---:|---:|
  | Basilisk | 0.2868 | 0.0445 |
  | `9587eeeb` (d95) | 0.3058 | 0.0663 |
  | Stockfish 16 classical (d16) | 0.3015 | 0.0459 |

- **Gaps to Basilisk and deltas, 95% bootstrap intervals:**

  | Contrast | Point | 95% interval |
  |---|---:|---|
  | gap(d95, G) | +0.0190 | +0.0128 to +0.0246 |
  | gap(d16, G) | +0.0146 | +0.0086 to +0.0202 |
  | gap(d95, O) | +0.0219 | +0.0095 to +0.0331 |
  | gap(d16, O) | +0.0015 | −0.0099 to +0.0122 |
  | delta(d95) | +0.0029 | −0.0106 to +0.0156 |
  | delta(d16) | −0.0132 | −0.0255 to −0.0011 |

- **Verdict: valid, NOT SUPPORTED.** Both game-line gaps have lower bounds
  above 0, so the instrument sees C.0's deficit. Neither delta's lower bound
  is above 0, and Stockfish 16's is below it: one move off the line, the
  deficit is no wider, and against Stockfish 16 it narrows. Corpus A uses pool
  positions as starts only; the question stays with C.11's deficit re-read.
- **Against the prediction:** gap(d16, G) +0.015 sits inside its 80% band
  (+0.01 to +0.05), below the point +0.03. delta(d16) −0.013 sits inside its
  band (−0.02 to +0.04), below the point +0.01. SUPPORTED had been given 0.35.
- **What else it shows (inference):** skill collapses one move off the line
  for every evaluation, from about 0.30 to about 0.05, and the fitted scale
  `k` rises about 1.6 times. A random move mostly leaves material en prise,
  which no static evaluation sees, so O is a population about hanging
  material more than about positional terms. That is the registered confound
  and why the read is the contrast, not the level. A quiescence-settled O
  would be the sharper question; it is not this corpus's (§5, C-77).
