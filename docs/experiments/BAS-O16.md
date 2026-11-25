# BAS-O16

**BAS-O16 — C.3.4: the rebuilt evaluation against the donor's in one search**

- Date / owner / category: registered 2026-11-25, before any game;
  maintainer-run, agent-read; measurement.
- Binary: one oracle built by `tools/oracle/build_oracle.ps1` around
  the `src/` of C.3.4's prepared commit with `EVAL_REBUILD` on, pinned
  by SHA-256 in the launcher. Stockfish `9587eeeb`'s search. This entry
  is completed with the hash before the run.
- Arms: `Use Basilisk HCE=false`, the donor's own evaluation, against
  `Use Basilisk HCE=true`, the rebuilt evaluation at its seed values
  through the bridge. `EvalAblationMask=0` on both.
- Research question: is the port the donor's evaluation in play? The
  fixture proves the value on 13,217 positions. This read asks the same
  of the positions a search visits. In the same search the donor's
  evaluation led the head's by +146.2 (BAS-O14).
- Registration and the rule: `analysis/c31_rebuild_2026-11-25.md`,
  Amendment 7.
- Registered design and stop rule: no gate; one fixed read of 3,000
  games, never extended or replaced.
- Full conditions: as BAS-O14's pair at mask 0. Colosseum CLI,
  two-engine read, 1,500 cycles, UHO `UHO_Lichess_4852_v1.epd` in random
  order, seed `210005`, `3+0.03`, margin 20 ms, 1T, Hash 64, concurrency
  14, no adjudication, no tablebases, zero permitted faults. The
  launcher is C.3.4's.
- Known differences between the arms, which keep the read off zero:
  the rebuilt evaluation has no contempt and no grain, serves
  Basilisk's endgame layer and rule-50 curve, and reaches the search
  about a quarter larger than the donor's (620/1024 centipawns a unit,
  then the bridge's 206/100).
- Completed 2026-11-25, before any game (C.3.4's preparation):
  - Binary: `tools/test_engines/oracle-c34-7d68e25-rebuild.exe`, SHA-256
    A53A9BA17F04248531F4892619354A9EF5B699F202965FF0B0FED0331EE1BD08.
  - It was built by `tools/oracle/build_oracle.ps1` with
    `oracle-eval-ablate.patch`, `oracle-eval-transplant.patch`,
    `oracle-eval-reference.patch` and `oracle-eval-rebuild.patch`, in
    that order, around `7d68e25`'s `src/`, byte-identical. That is the
    commit BAS-M25's arms are built from.
  - Conformance: 471,519 positions, 0 mismatches. Oracle bench 955,525,
    equal to C.3.2's rebuilt oracle.
  - At mask 0 its Basilisk value equals the new arm's served value on
    all 13,217 fixture rows, through the bridge's ×206/100, with 0
    mismatches. The head's oracle mismatches on 12,667 of them.
  - Arms: `Oracle 9587 eval` (`Use Basilisk HCE=false`) first, and
    `Oracle rebuilt eval` (`=true`) fixed at 1500. The first arm's
    rating less 1500 is the read. Each arm carries its switch,
    `EvalAblationMask=0`, Hash 64 and Threads 1, and nothing else; the
    dry run's launch record shows this.
  - Command: `python tools/run_c34_reads.py --read o16`. The table:
    `python tools/diag/c34_table.py`.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-25

- The donor's evaluation minus the rebuilt one: +15 Elo, 80% band −5 to
  +40.
- Rule: above +50 the port is suspect in play, and the leaf returns to
  research before any fit.
- An arm over 5% faster in nodes a second is flagged, and a flagged
  read decides nothing alone.

#### RESULT — append after exposure

- 2026-11-25. Complete: 3,000 games (`tools/results/c34-o16/`, PGN
  SHA-256 `70CEBB22…61045493`), zero engine faults, zero time losses,
  recount agrees. Summary table: `tools/results/c34-table.md`.
- **Oracle 9587 eval minus Oracle rebuilt eval: +42.4 Elo [+35.1, +49.7]**
  (pentanomial 95% CI; trinomial [+33.3, +51.5]), score 56.07%
  (1,682.0 / 3,000), 989 wins, 1,386 draws, 625 losses, pentanomial
  [21, 232, 676, 504, 67].
- **Flagged (`an arm >5% faster`):** `Oracle 9587 eval` ran 2.86M
  nodes/s (183,745 nodes/move, mean non-mate depth 18.27) against
  `Oracle rebuilt eval`'s 2.53M nodes/s (165,070 nodes/move, mean
  non-mate depth 18.22), ratio 1.129 (+12.9% NPS, matching BAS-O14's
  1.143 bridge overhead).
- Clamped bare-king draws: 41 for `Oracle rebuilt eval` (30 KRvK, 4
  KRPvK, 3 KBNvK, 2 KRBvK, 1 KQPvK, 1 KBBvK) against 2 for `Oracle
  9587 eval`, a net 39 draws (~4.5–5 Elo) from the oracle bridge's clamp
  artifact (`BAS-M25` had 0 on the rebuilt arm). Terminations: 1,614
  checkmates, 742 threefold, 381 insufficient material, 245 fifty-move,
  18 stalemate.
- Closes 103.8 Elo of BAS-O14's +146.2 gap inside `9587eeeb`'s search,
  consistent with BAS-M25's +99.8 in Basilisk's search.
- Rule applied: below +50 and speed-flagged (decides nothing alone);
  does not return C.3.4 to research.

#### PREDICTION CALIBRATION

- Missed the 80% band (+42.4 for +15, band −5 to +40; magnitude miss,
  with the speed flag active).
- What the model underestimated: the combined drag of the 12.9% bridge
  NPS overhead (~10–15 Elo), 39 net bridge-clamped bare-king draws
  (~4.5–5 Elo), the 1.247× score inflation through the bridge (mean
  |score| 439 cp against 395 cp), and unported `endgame.cpp`, contempt
  and grain (245 fifty-move draws against 18 in BAS-M25, led by BPvB,
  RPvR, RNvR, BPvP and RBvR).


