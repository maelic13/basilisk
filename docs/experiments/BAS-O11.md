# BAS-O11

**BAS-O11 — corpus A's tablebase labels: local relabel built, whole-game adjudication gets no arm (C.2.4)**

- Date / owner / category: 2026-11-23, agent overnight on the maintainer's
  leave; a descriptive audit with a design verdict, no prediction registered
  (it decides whether an arm exists, not whether one wins).
- Question (PLAN C.2.4, archived 8.4–8.5): how wrong are corpus A's game
  results where Syzygy can judge them? Does whole-game tablebase adjudication
  deserve a third corpus arm?
- Instrument: `tools/texel/relabel.py` (`fbeceaf`) on corpus A
  (`tools/texel/data/corpusA/rows`, 1,000,000 games of
  `basilisk-c2-corpusA-40ae560` at 8,000 nodes, natural terminations), Syzygy
  `D:/chess/tablebases/syzygy3456`. The verdict is Stockfish's root_probe
  rule under the row's rule-50 clock. Packet:
  `analysis/c24_label_arms_2026-11-23.md`.
- **Results:**
  - Local (corpus B): 577,305 of 3,770,000 train rows are eligible, and
    10,584 change (0.28% of rows). The changes are symmetric: 5,525 draws
    become wins or losses, 5,008 decisive labels become draws, and 51 flip
    between win and loss. The clock draws 380 more rows than Syzygy alone
    would. No probe failed. Basilisk's own root probe agrees with every
    changed validation and test row (1,204) and 400 unchanged ones.
  - Whole-game: 553,735 games reach six men or fewer. At that first
    position, 24,099 (4.35%) end against the verdict: 13,223 drawn endings
    were decided, and 10,833 won or lost ones were not finished. A
    whole-game relabel would change 97,274 train rows (2.58%), 62,259 of
    them above six men.
  - The C-62 audit on 20,000 of these games: 2.54% of clean wins not won,
    against BAS-E46's 19.77% at the same nodes on 1.9.3.
- **Verdict: no whole-game arm now, for power, not for confound.**
  - A post-hoc relabel of A's own games would isolate the label exactly.
  - But it moves 2.6% of rows with errors in both directions, and the local
    relabel's 0.28% is itself expected near null. Such a small gain waits for
    Phase H.
  - Corpus B stays C.2.8's matched arm.
- **Retry trigger** for a whole-game arm, any of:
  - C.2.8's fits of A and B differ beyond noise, or B passes its gate;
  - a corpus whose contradicted share of adjudicable games exceeds 10%;
  - Phase H's small-gain programme.
