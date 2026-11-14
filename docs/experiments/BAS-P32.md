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
- **Status.** Registered; both reads wait for the idle host.
