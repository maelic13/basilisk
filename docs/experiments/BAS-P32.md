# BAS-P32

**BAS-P32 — the final board beside the donors (contract v3), and B.7.6's speed in hardware counters (B.7.9)**

- **Registration.** 2026-11-14, B.7.9, before either read. Agent; the counter read is
  maintainer-run (it needs an elevated shell). The board is B.7.6's head (`0c978c2`):
  B.7.7 kept nothing and B.7.8 closes NO_CHANGE.
- **Questions.**
  1. Measured with hardware counters at an identical tree, does B.7.6's set show the
     speed BAS-P30's pooled read found?
  2. Pricing the search's own call mix without the quiet-stage stand-in, where does
     the final board stand against Rarog and both Stockfishes, and which column does a
     donor still lead?
- **Counter read.** `tools/run_b164_pmc.ps1 -BasePool tools/test_engines/b76-base.pool.json
  -CandPool tools/test_engines/b76-head.pool.json -ExpectNodes 4247965 -Name b79 -Contrast
  b76`, BAS-P28's instrument on BAS-P30's pools, run after BAS-P28 itself in the same
  elevated session. It counts only if BAS-P28 accepts the instrument.
- **Five-engine read.** `tools/diag/board_micro_run.py`, five alternating rounds of 15
  samples of 200 ms, CPU 2, as B.7.2's five-engine session:
  - Basilisk's `board_micro_bench` at the final head;
  - the four v3 adapters, `tools/results/board-compare/bin/*-board-v3.exe`.
- **Composite** (PLAN B.7.9):
  - Basilisk's counted call mix: `0.92 make_unmake + 0.53 capture_gen + 0.20 quiet_gen
    + 1.42 see_captures + 1.44 gives_check`, with no stand-in for the quiet stage.
  - Charges: an engine whose make is given `gives_check` pays a `gives_check` for the 57% of
    makes Basilisk's search does not ask about (`+ 0.53 gives_check`), so Rarog and both
    Stockfishes. Basilisk pays its draw tests (1.8 ns per node) and the picker's check
    squares (0.53 ns per node), the replay's stub-corrected figures (B.7.5).
  - Reckless has no `gives_check` query and is priced without that term, as in B.7.2.
- **Predictions, frozen.**
  1. Counters: the search thread's cycles per node −3.1% (BAS-P30's +3.21% in time),
     the interval overlapping [−3.4%, −2.8%]; instructions per node lower by 2–5%.
  2. Composite: Basilisk the cheapest of the five, at least 5% under Rarog and
     classical Stockfish and at least 20% under Stockfish 19 (probability 0.7). The
     donors still lead `see_captures` and `gives_check` (Stockfish's stored check
     information), and Rarog leads `make_unmake`.

#### RESULT

Idle host, CPU 2, 2026-11-14.

**1. Counters** (`tools/results/b79-pmc/`, maintainer-run, elevated): `b76` −2.587%
cycles per node [−3.663%, −1.510%], instructions −2.738%, branch misses −3.364%; the
null −0.112% [−0.682%, +0.459%]. **Not counted:** BAS-P28 did not accept the
instrument (its null's half-width 0.76% against a 0.5% rule). For information, the
point estimate lies 0.5 points short of BAS-P30's −3.11% in time, with intervals
overlapping. Read again with BAS-P33's null (`tools/results/b79b-pmc/`): −2.698%
[−3.285%, −2.112%], instructions −2.740%. Still not counted: BAS-P33 did not accept
the instrument either.

**2. Five engines on contract v3** (`tools/results/b79-micro5/`, five alternating
rounds; Basilisk's harness at `0c978c2`, host 1.4% before, 4.8% after). Every
engine-independent count agrees, the new `quiet_positions` 477 and `quiet` 13,029
included. Medians, ns per operation:

| Column | Basilisk | Stockfish 19 | Stockfish classical | Rarog | Reckless |
|---|---:|---:|---:|---:|---:|
| `legal_gen` | 48.79 | 76.69 | 90.06 | 64.27 | 52.60 |
| `capture_gen` † | 17.89 | 10.50 | 14.21 | 21.54 | 16.62 |
| `quiet_gen` † | 47.75 | 29.30 | 30.12 | 44.88 | 40.40 |
| `make_unmake` | 16.04 | 53.33 | 35.34 | 14.68 | 40.28 |
| `see_captures` | 10.61 | 6.53 | 6.62 | 14.09 | 18.45 |
| `gives_check` | 6.64 | 3.46 | 3.25 | 8.08 | — |
| `board_perft` | 3.33 | 6.28 | 5.12 | 4.11 | 5.42 |

† Basilisk's generators yield exactly the legal sets. Both Stockfishes generate
pseudo-legal moves and test legality at delivery, untimed (13,713 and 13,714 quiets,
1,150 and 1,104 captures). Rarog and Reckless leave 42 underpromotions to the quiet
stage (13,085 and 13,071).

Composite, ns per node, by the frozen rule:

| | Basilisk | Stockfish 19 | Stockfish classical | Rarog | Reckless |
|---|---:|---:|---:|---:|---:|
| ns per node | **60.7** | 76.6 | 61.9 | 69.8 | — |
| against Basilisk | 100% | 126% | 102% | 115% | — |
| without `gives_check` | 51.2 | 69.8 | 55.5 | 53.9 | 80.1 |

- **The final board is the cheapest of the five on the work Basilisk's search does.**
  Classical Stockfish is within 2%, inside the composite's resolution (B.7.2's ±5%).
  Rarog is 15% dearer once its make pays for the check answer its search provides.
- **The columns a donor still leads, and their mechanisms:**
  - `see_captures` and `gives_check`: Stockfish's check information stored at every
    make, paid in its make (35 and 53 ns against 16).
  - `make_unmake`: Rarog's hinted make. B.7.7's p5 measured that form in Basilisk's
    search as −0.49%.
  - `capture_gen` and `quiet_gen`, Stockfish: legality deferred to delivery.
  - `capture_gen` and `quiet_gen`, Reckless: threats and pins kept in its board at
    every make (its make 40 ns), which decide king targets, castling and pinned
    pieces for free. B.7.7's p6, an enemy attack map per node, measured that form in
    Basilisk's search as −1.32%.
  - Each lead is work moved into make or out of the timed column, and each of the
    two forms that could be moved into Basilisk was read in Basilisk's search, and
    lost.

**Calibration** against the frozen predictions:
- Counters: not counted; the estimate (−2.6%) was a little short of the predicted
  −3.1%, inside the interval's overlap.
- Composite: Basilisk cheapest, more than 5% under Rarog and more than 20% under
  Stockfish 19, as predicted. Missed: classical Stockfish is 2%, not 5% or more,
  above it. The donors' leads are the predicted three, plus both generation columns.
- **Disposition.** COMPLETE.
