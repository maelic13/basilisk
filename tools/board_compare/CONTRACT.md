# cross-engine-board-v2

The contract every engine's board harness meets, so that Basilisk's board
can be timed beside Rarog's and Reckless's on the same work. Basilisk's
side is `tests/board_micro.cpp` (its other columns are Basilisk-only);
the other engines' adapters live beside this file, and
`tools/board_compare/build.py` builds them from pinned commits.
`tests/board_performance.cpp` keeps the older five-position contract,
cross-engine-board-v1, unchanged.

Why a second contract: v1 times five roots with no side to move in check,
no legal en passant and no promotion, and its SEE column read each
engine's own piece values, which Rarog's RAR-M29 showed is not
comparable. v2 times 512 positions of
real games and injects one value vector.

## Input

`tests/data/board_micro_positions.fen`: lines starting with `#` are
comments, every other line is one FEN. The line

    # Oracle: positions 512 legal 14085 tactical 927 checking 657

holds python-chess's counts for the file. A harness parses it and refuses
to time (exit 1) unless its board reproduces `positions`, `legal` and
`checking` exactly; `tactical` is checked by the capture column's rule
below.

Everything is built before any timing: the boards from the FENs, each
board's legal-move list, the list of every (position, legal move) pair,
and the tactical list -- the legal moves that capture (en passant
included) or promote, every promotion piece -- filtered from the legal
lists at setup. No timed loop allocates; lists are reused.

## Columns

Names, units and work are fixed. A column an engine cannot meet is left
out and the harness prints `noncomparable <column> <reason>`; it is never
approximated.

| Column | One operation | Work per pass |
|---|---|---|
| `legal_gen` | generate every legal move of one position into a reused list | 512 positions |
| `capture_gen` | the engine's own tactical generator for one position: what its search uses for captures and promotions | 512 positions |
| `make_unmake` | make one legal move, then unmake it | 14,085 moves |
| `see_captures` | threshold SEE at 0 of one tactical move | 927 moves |
| `gives_check` | whether one legal move gives check, without making it | 14,085 moves |
| `board_perft` | one leaf of perft with the legal generator and make/unmake, bulk-counted at depth 1 | 969,767 leaves |

- `capture_gen` is comparable only when the engine's tactical generator
  yields exactly the tactical set (927 moves over the file). If it yields
  another set (say queen promotions only), the harness still times it and
  prints `noncomparable capture_gen <its count> of 927` so the reader
  knows. It never filters inside the timed loop.
- `make_unmake` does all the work the engine's search make does to the
  board, keys and per-ply state, but no network accumulator work: an
  engine whose make takes an observer or accumulator hook gets its null
  form. Copy-make engines copy.
- `see_captures` uses the values P 100, N 300, B 300, R 500, Q 900, and the
  engine's own king sentinel. An engine whose SEE values cannot be set
  without changing its source is `noncomparable`; a one-line constant
  swap in the adapter's build copy is allowed and must be stated.
- `gives_check` must reproduce `checking` (657); an engine with no such
  query leaves the column out.
- `board_perft` roots and depths: the start position to depth 4,
  Kiwipete (`r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`)
  to depth 3, CPW position 3 (`8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`)
  to depth 5: 197,281 + 97,862 + 674,624 = 969,767 leaves.

## Output

Before timing, the preflight runs every column twice and requires the same
operation count and checksum, then prints, in this format:

    profile: cross-engine-board-v2
    work <column> <operations> <unit> checksum <16 hex digits>
    semantic positions <n>
    semantic legal <n>
    semantic tactical <n>          (the capture generator's total)
    semantic checking <n>
    semantic see_ge0 <n>           (tactical moves whose SEE >= 0 holds)
    semantic perft <n>
    preflight: PASS

`work` checksums fold the engine's own results and differ between engines;
`semantic` lines are engine-independent and must agree, except `see_ge0`,
where engines' exchange rules (pins, promotions, the king) may differ: a
difference there is reported, not refused.

Timing, after `--preflight-only` is absent:

- arguments `<fen file> [--samples N] [--sample-ms MS] [--columns a,b]
  [--preflight-only]`, defaults 15 samples of 200 ms;
- each column warms up for one sample's time, which fixes its passes per
  sample;
- samples go round-robin over the columns; each prints
  `sample %2d %-16s %10.3f ns/op`;
- a final table: a header line, then one row per column,
  `<column> <median ns/op> <MAD ns> <MAD %>% <passes/sample>`;
- results pass through a compiler barrier (`std::hint::black_box`,
  `asm volatile`).

`tools/diag/board_micro_run.py` runs any of these binaries, alternating
them, one process per run pinned to one logical CPU, and refuses a busy
host.

## Builds

The same LLVM for every arm (Clang 23.1.5 and rustc 1.100's LLVM 23.1.5),
no PGO on any arm, each engine's own release profile:

- Basilisk: `build/release` (the PEXT tier's fixed flags, `-O3`, LTO);
- Rarog: `RUSTFLAGS="-C target-cpu=native --cfg rarog_pext"`, its release
  profile (fat LTO, one codegen unit, panic abort);
- Reckless: `RUSTFLAGS="-C target-cpu=native"`, its release profile,
  `--no-default-features` (no Syzygy), the network not loaded.

## Reading it

- A column is a screen for a component, never a speed verdict. Rarog's
  RAR-M44(c) bundle won 12.7% on legal generation and cost 0.55% of
  pooled-PGO NPS; pooled NPS decides.
- Cross-engine, differences under about 5% on one column were not
  resolved by Rarog's sessions (round ranges up to 6–9%). Read the
  alternating rounds' spread before any gap.
- A gap is a lead to a technique, read in the other engine's source, not
  a number to chase.
