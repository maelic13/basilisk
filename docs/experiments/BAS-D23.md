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
