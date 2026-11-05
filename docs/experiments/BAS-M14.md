# BAS-M14

**BAS-M14 — where Basilisk loses: every move of stored games scored by a
judge, by position class, clock and material**

- Date / owner / calibration category: registered 2026-11-05, agent;
  measurement (PLAN B.16.3, Triumviratus TT-1). Maintainer-run.
- Baseline source revision / candidate source revision / dirty-diff identity:
  no engine change. Tool `tools/diag/loss_by_cell.py` and launcher
  `tools/run_b163_loss.ps1`, committed with this registration on
  `b16-instruments`.
- Binary / compiler / flags / PGO identity and hashes (the launcher checks
  every one):
  - judge: Stockfish 19,
    `D:\chess\engines\stockfish\stockfish-19-windows-x86-64-universal.exe`,
    SHA-256 `45BC8E4969147DB9C2EB533810637994619BFF0EACC81CCFD9854394901BCBD0`;
  - re-search, m11: `tools/test_engines/basilisk-1.10.1-rc4-pext-pgo.exe`
    (`C1F9DC2D…`, revision `aca23806`, bench 14,978,465). The gauntlet
    played the 1.10.1 release asset (`F7AAEF49…`, not on this host), which
    reproduces the same bench; a single-thread search at fixed nodes
    depends only on the search, so rc4 stands in for it;
  - re-search, b36: the gate's own arms,
    `basilisk-b36-colour-on-pext-pgo.exe` (`0E2C93E2…`) and
    `basilisk-b36-colour-off-pext-pgo.exe` (`21CF9C56…`).
- Sources:
  - **m11**: BAS-M11's 2,400 games (Basilisk 1.10.1, 4T, `3+0.03`, against
    Critter 1.6a, Fritz 16, Houdini 3, Rarog 2.4.0 and 2.5.0, Rybka 4.1),
    exported read-only from the Colosseum app's database by
    `tools/diag/export_tournament_pgn.py` to
    `tools/results/b163-loss/sources/m11-gauntlet-4t.pgn`, SHA-256
    `E4E3379A4C179F85CC807522376CCB24CC6A278EAF361C3B7AD318861BBC89FB`;
    annotated with each move's time and nodes;
  - **b36**: the first 2,000 games (1,000 pairs) of B.3.6's gate
    (`tools/results/b36-gate/games.pgn`, `A38F1459…`, seed 46001), 1T
    `3+0.03`, both arms today's engine except for the coloured histories;
    annotated;
  - **srt**: Basilisk 1.10.0's 8,200 games of the Super Rating Tournament
    (BAS-M08, `D:\chess\results\super_rating_tournament.pgn`, `4E87A36A…`),
    1T `3+0.03`; no annotations, so no clock cells and no re-search.
- Research question: in which positions does Basilisk lose its points, and
  is a loss a matter of the clock (depth) or of the evaluation? The answer
  feeds B.9's checkpoint and the order of Phases C and D.
- Hypothesis / proposed mechanism: none to accept; an observation.
- Interacting mechanisms / consumers: D.1's time budget (a loss with a
  short clock), C's evaluation work (a loss the deeper search keeps).

#### Instrument

The judge scores every position of every selected game at 150,000 nodes, one
thread, Hash 16, its table cleared at each game and each position given with
the game's moves (repetitions known). A move's loss is the judge's score
before it plus the opponent's score after it, counted between 0 and 300 cp;
a mate in n is 10,000 − n. Cells: the mover (ours: a player named Basilisk;
the opponent as the control), the class by the judge's score before the move
(defending at −50 or below, ahead at +50 or above, level), the clock left
before the move as a share of the base (under 10%, 10–30%, 30% and more;
rebuilt from the TimeControl header and each move's `t=`), and the men on the
board (24+, 16–23, 10–15, 9 or fewer). Per cell: moves, mean counted loss,
the share of moves losing 50 cp or more, and the cell's share of the side's
total loss; and ours' excess over the opponents' rate in the same cell
(our moves × the difference of the means).

Re-search: of ours' moves losing 100 cp or more from a position the judge
did not already score as lost (above −300), the 100 worst per source (by
counted loss, then from the better position) are searched again from a
fresh process (Threads 1, Hash 64) at 1, 4 and 16 times the nodes the game's
search used; each new move, the game's move and the position before are
judged afresh. An error mended at 4× or 16× (the new move loses under 50
cp) is a matter of depth; one that stays at 16× is the evaluation's. The 1×
search shows how often a fresh process repeats the game's move (the game's
table, histories and, for m11, four threads are not reproduced).

Validation (2026-11-05): 9 unit tests (`tools/diag/test_loss_by_cell.py`)
on synthetic games (scores, mates, cells, clock rebuilding, the excess, the
selection); a planted game in which the side to move gives its queen for a
pawn (`Qxf7+`) read 868 cp, counted 300, and its other moves 3 to 57 cp; six
m11 games judged twice on different worker counts gave identical scores in
all 761 positions; a launcher dry run (two games a source at 20,000 nodes)
passed every stage. Cost: about 0.15 s a position a judge, so about five
hours on 15 cores, three of them for srt, which runs last.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Exposure before freezing: the six-game timing sample of m11 (its 85 moves
of 30 cp or more were listed, six re-searched) and the dry run's two games
a source; no table of a full source has been seen.

- m11, ours (1.10.1) against the field: mean counted loss per move above the
  opponents' (probability 0.9); the defending cells carry at least half of
  ours' total excess (0.6); the cell defending with under 10% of the clock
  holds at least 20% of ours' total loss (0.4; Triumviratus read 33.8%);
  ours' excess per move grows as material falls, largest at 9 men or fewer
  (0.6).
- srt, ours (1.10.0): the same defending-heavy shape by class and material
  (0.6).
- b36 (today's engine against itself): mean counted loss per move below
  m11's ours (0.85; a confounded comparison, recorded as a diagnostic).
- Re-search: in m11 and in b36, at least 80 of the 100 errors are still
  errors when judged afresh; at least 40% of those are mended at 4× or 16×
  (0.6; Triumviratus: half vanished with two to four times the nodes).
- Most likely failure mode: a source too small for the defending-with-short-
  clock cell (fewer than 200 of ours' moves), read then as no finding.
- Registered reading rule: the reading names (a) the class × clock and the
  class × material cells with the largest excess of ours over the opponents
  in m11 and in srt, and (b) for m11 and b36 the share of the re-judged
  errors mended at 4× or 16×. A share of 50% or more points D (the clock)
  ahead of C; 25% or less points C (the evaluation) first; between, no
  order. Both are pointers for B.9 and the maintainer's ordering, not
  decisions.
- Full conditions / provenance: one command,
  `pwsh -NoProfile -File tools/run_b163_loss.ps1`, resumable; archive
  `tools/results/b163-loss/judged/` with `host.json`.

#### RESULT — append after exposure

- Diagnostics (not the verdict):
- Deciding result:
- Disposition:

#### PREDICTION CALIBRATION — preserve the original prediction above

- Original prediction summary:
- Observed result:
