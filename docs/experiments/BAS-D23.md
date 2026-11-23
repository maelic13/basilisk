# BAS-D23

**BAS-D23 — C.2.7.3: deployed nodes per search at the gate's conditions, and the endgame budget bracket they set**

- Date / owner / calibration category: registered 2026-11-23, before any
  read at the gate's conditions; agent, PLAN C.2.7.3; run condition (no
  playing verdict). Exposure so far: one dry run at 1+0.01 over two games
  (`tools/results/c273/dry/`), a tool smoke that decides nothing.
- Binary: the head `901e519` (engine source last changed in `4f61426`),
  `tools/test_engines/basilisk-c273-head-pext-pgo.exe`, `EDDBA36E38C2…`,
  final PGO, PEXT, clean, bench 4,777,412, Clang 23.1.5.
- Question: what does the engine really search per move in a gate, and so
  which three fixed-node budgets must the endgame truth vetoes use (C-25,
  C-137, B-3)?
- Instrument: `tools/diag/nodes_per_move.py` (`901e519`). Conditions are the
  Colosseum gate's (`tools/colosseum/common.toml`, PROCESS *Harness*):
  - 3+0.03, one thread, Hash 64, Syzygy off;
  - 150 games from the UHO book (`tools/books/UHO_Lichess_4852_v1.epd`,
    seeded offsets, seed `0xC273`);
  - two engine processes per game, 15 games at once, on an otherwise idle
    host;
  - natural termination, at most 400 plies;
  - the engine's own clock path, charged with measured wall time.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-23

- Overall median nodes per search: in [50,000, 300,000] at 80%, point
  130,000.
- With 3 to 6 men on the board, the median is below the overall median, at
  0.7: those positions come late, on a short clock (the dry run read 14.6k
  against 30.8k).
- At least 300 moves with at most 6 men are sampled: 0.75.
- The bracket's middle budget is below Rarog's 200,000: 0.65.
- No game forfeits on time: 0.9. Any forfeit is reported, and questions
  the run before its numbers are used.

**The rule, frozen** (`nodes_per_move.bracket_from`):
- M is the median nodes per search over moves made with at most 6 men on
  the board, when there are at least 300 such moves; otherwise the median
  over every move.
- The budgets are M/3, M and 3M, each rounded to two significant figures.
- They become the bracket for C.2.7.4's baselines and for every
  evaluation gate's endgame vetoes, under the veto rule of C.2.7.1's
  audit, §4.

**Stop rule:** one run. A run on a host found busy is discarded and run
once more, never kept. No re-run on the same binary and conditions to move
the number.

#### RESULT — append after exposure

- **Run 1, 2026-11-23, by the maintainer: DISCARDED** under the stop rule
  (see below; `tools/results/c273/nodes_per_move_run1_discarded.json`).
  Kept here as the record of what it read. Its checks hold:
  - the binary hash `EDDBA36E…` is the registered one;
  - all 150 games ended by the rules, none on time or the ply cap;
  - the run read 17,467 searches.

  | Moves | n | p25 | Median | p75 | p90 |
  |---|---:|---:|---:|---:|---:|
  | All | 17,467 | 67,725 | **106,873** | 156,974 | 218,437 |
  | Ply 0–29 | 4,490 | 115,602 | 150,536 | 203,539 | 257,250 |
  | Ply 30–79 | 6,946 | 86,446 | 116,563 | 163,298 | 222,564 |
  | Ply 80+ | 6,031 | 38,376 | 60,008 | 92,229 | 132,506 |
  | 3–6 men | 1,634 | 35,723 | **55,259** | 84,420 | 117,007 |
  | 7–12 men | 4,339 | 42,130 | 68,269 | 105,298 | 155,774 |
  | 13–32 men | 11,494 | 92,798 | 128,156 | 177,493 | 237,348 |

- **The bracket, by the frozen rule:** 1,634 moves had at most 6 men (300
  needed), so M is 55,259. The budgets are **18,000, 55,000 and
  170,000**.
- **What follows:**
  - Every truth read so far ran at 60,000 nodes. That is near the deployed
    endgame median, but it is not one of the bracket's budgets. BAS-D22's
    60k report therefore cannot stand as an arm, and C.2.7.4 runs all
    three budgets on the head.
  - Deployed endgame searches are short: the median at 3 to 6 men is
    about half the median over all moves, because those positions come
    late on a short clock. A tactic the 600k arm of Rarog's bracket sees
    is mostly beyond what a gate game searches.
- **Against the prediction:** every part held.
  - The overall median, 106,873, is inside [50,000, 300,000], against a
    point of 130,000.
  - The 3-to-6-man median is below the overall one (0.7).
  - At least 300 endgame moves were sampled (0.75).
  - The middle budget is below 200,000 (0.65).
  - No game forfeited (0.9).

- **Run 1 discarded (maintainer, 2026-11-23, after the result above).** The
  host was not fully idle during run 1, so the stop rule applies: the run
  is discarded and the read is made once more, on the same binary and
  conditions, and that run is kept whatever it reads.
  - Run 1's bracket and its calibration above are withdrawn. Nothing reads
    them.
  - Run 2 writes `tools/results/c273/nodes_per_move.json`.

- **Run 2, 2026-11-23, by the maintainer on an idle host: KEPT**
  (`tools/results/c273/nodes_per_move.json`). Its checks hold:
  - the binary `EDDBA36E…`, the conditions and the starts (`e64575c7…`,
    the same 150 positions) are run 1's;
  - all 150 games ended by the rules, none on time or the ply cap;
  - the run read 18,177 searches.

  | Moves | n | p25 | Median | p75 | p90 | Run 1 median |
  |---|---:|---:|---:|---:|---:|---:|
  | All | 18,177 | 78,246 | **121,819** | 176,936 | 244,078 | 106,873 |
  | Ply 0–29 | 4,499 | 133,399 | 169,743 | 228,443 | 292,806 | 150,536 |
  | Ply 30–79 | 7,100 | 100,030 | 134,873 | 185,833 | 250,094 | 116,563 |
  | Ply 80+ | 6,578 | 47,301 | 71,338 | 106,378 | 157,207 | 60,008 |
  | 3–6 men | 2,129 | 42,922 | **63,876** | 94,303 | 133,910 | 55,259 |
  | 7–12 men | 4,440 | 54,242 | 83,270 | 124,869 | 185,744 | 68,269 |
  | 13–32 men | 11,608 | 107,768 | 146,217 | 201,036 | 264,149 | 128,156 |

- **The bracket, by the frozen rule:** 2,129 moves had at most 6 men (300
  needed), so M is 63,876. The budgets are **21,000, 64,000 and
  190,000**.
- **The busy host cost run 1 12% of its nodes** overall, and 11% to 18% by
  band. The stop rule moved every budget by one rounding step or more.
- **What follows:**
  - Every earlier truth read ran at 60,000 nodes. That is near the
    deployed endgame median, but it is not one of the budgets, so C.2.7.4
    runs all three on the head.
  - Deployed endgame searches are short: the median at 3 to 6 men is
    about half the median over all moves, because those positions come
    late on a short clock.
- **Against the prediction (run 2):** every part held.
  - The overall median, 121,819, is inside [50,000, 300,000], near the
    point of 130,000.
  - The 3-to-6-man median is below the overall one (0.7).
  - At least 300 endgame moves were sampled (0.75).
  - The middle budget is below 200,000 (0.65).
  - No game forfeited (0.9).
