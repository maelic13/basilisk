# Block 6: the inference of the network

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). This block covers the code that runs
the Consilium network in the engine: the accumulators, the forward pass through the layers after the accumulator,
the SIMD kernels, and the loading of the network file.

Status: **done** (18 October 2026). The engine evaluates the network with a module of our own, `source/consilium/`,
written from a specification of the network. The evaluations are identical bit for bit to those of the previous
inference. With block 7 the engine needs 5.5% fewer cycles per node in the middlegame and 4.7% fewer in endgames on
AVX-512, 2.0% and 1.5% fewer on AVX2 (section 6.9). The previous inference code (`source/nnue/nnue/`) is removed; with
block 7 the whole folder `source/nnue/` is gone (section 6.9), and no file of Stockfish origin remains outside the
search.

## 6.1 Starting point

For Consilium the part of the inference that runs at every node had already been largely rewritten before 8.1
(accumulators, the four experts, our input blocks, the link with the search). The forward pass of the layers, its
SIMD kernels and the loading of the network still came from the inference code the project started from, about 60
to 70% of the lines of `source/nnue/` by a line-by-line count.

## 6.2 Method: a clean room

The new code was written without reading the previous inference code.

1. **A specification first.** What the network computes was written down from the trainer alone: the PyTorch model,
   the quantization, the serializer and the C++ data loader, together with the network file. The specification
   covers the file format, the integer scales, the index of every input feature, the accumulator and the integer
   forward pass (section 6.3).
2. **Each part of the specification checked on its own.** A reader and writer of the file in Python rewrites
   `nn-consilium.nnue` (170,288,864 bytes) identical byte for byte. The feature indices agree with the trainer's own
   data loader on 3,000 positions, for both perspectives and all four input blocks. A Python reference of the integer
   forward pass gives the same two network outputs as the engine on 3,000 positions from games.
3. **The previous engine only as a black box.** A copy of the engine with one added command (`evalbatch`, which
   prints the two network outputs and the final evaluation for a list of positions) served as an oracle: values out,
   no code read. Probe networks, with one input block on and the others set to zero, checked the PSQT of each block
   separately (500 positions each).
4. **A golden set.** 2,000,124 positions from real games, without duplicates, with the oracle's three numbers for
   each. A sensitivity test showed that the set catches small errors: a variant that squares the activation after
   clipping instead of before is wrong on 998 positions out of 1,000.
5. **The module written from the specification**, then checked against the golden set, the bench, and the
   incremental accumulators against a full refresh at every node of a search (section 6.6).

Since the evaluations are the same, strength is unchanged by construction, and no SPRT is needed. The only open
question was speed, and a slower module would not have been adopted.

## 6.3 The specification

All values are little endian. Square numbering of the trainer: a1 = 0 to h8 = 63; the engine uses a8 = 0, so the
module builds its tables once in the engine's numbering (trainer square = engine square XOR 56).

### File format

| field | type |
|---|---|
| version | u32, `0x6A448AFA` |
| network hash | u32, hash of the layers XOR hash of the transformer XOR (L1 × 2) |
| description | u32 length, then UTF-8 text |
| transformer hash | u32 |
| transformer bias | int16 [1024] |
| for each input block, in order: weights, then PSQT | int8 or int16 [rows][1024], int32 [rows][8] |
| for each of the 8 output buckets: layer hash, then l1, l2, output | u32, then (int32 bias, int8 weights) × 3 |

A tensor is stored either raw or compressed: the 17 ASCII bytes `COMPRESSED_LEB128`, a u32 byte count, then signed
LEB128 values (7 bits per byte from the least significant, bit 0x80 = more bytes follow, bit 0x40 of the last byte =
sign extension). In the Consilium file the transformer bias, the piece-square weights and all four PSQT tensors are
compressed; the int8 weights are raw. The reader accepts both forms for every tensor.

Hashes, all modulo 2^32:

- block hashes: threats `0x2E6B9D04`, piece-square `0x7F234CB8 ^ 0x50484134`, pawn pairs `0x86F2B1DD`, passed pawns
  `0x50535344`;
- transformer: `h = 0`, then for each block in file order `h = rotl32(h, 1) ^ block hash`; result `0x9346EEEE`, written
  as `h ^ (L1 × 2)`;
- layers: from `prev = 0xEC42E90D ^ (L1 × 2)`, for the layers with 32, 32 and 1 outputs,
  `h = (0xCC03DAE4 + outputs) ^ (prev >> 1) ^ (prev << 31)`, plus `0x538D24C7` when outputs ≠ 1, then `prev = h`;
  result `0x63337116`, repeated in front of each of the 8 buckets.

The layers of each bucket are stored row by row, `[outputs][inputs]`: l1 is int32 [32] bias and int8 [32][1024]
weights, l2 int32 [32] and int8 [32][64], the output int32 [1] and int8 [1][128]. A permutation of the 512 neurons of
each half of the accumulator, chosen at export to group the zeros of l0, is already applied to the file; the
inference does not see it.

### Input blocks

| # | block (trainer name) | rows | weights |
|---|---|---|---|
| 0 | threats (`Full_Threats`) | 59,808 | int8 |
| 1 | piece-square with four phase bands (`HalfKAv2_hm_P4`) | 90,112 = 4 bands × 32 king squares × 704 | int16 |
| 2 | pawn pairs (`PP_3Wide`) | 4,560 | int8 |
| 3 | passed pawns (`PassedPawns`, ours) | 96 | int8 |

The total is 154,576 inputs. Piece types P N B R Q K are 0 to 5, white is colour 0 and black colour 1. For a
perspective p with its king on `ksq`, two different orientations are used:

- piece-square block: `oH = (p black ? 56 : 0) ^ (file(ksq) ≤ d ? 7 : 0)`, a mirror when the king is on files a to d;
- threat, pawn-pair and passed-pawn blocks: `oT = (p black ? 56 : 0) ^ (file(ksq) ≥ e ? 7 : 0)`, a mirror when the
  king is on files e to h.

An oriented square is `s ^ oH` or `s ^ oT`.

**Piece-square.** One feature per piece, kings included.

- Phase band by the number n of pieces on the board, kings and pawns included: 0 for n ≤ 9, 1 for 10 to 15, 2 for 16
  to 23, 3 for 24 or more.
- King bucket of the oriented king square, which always lies on files e to h: `KB = 31 − 4 × rank − (file − 4)`, so
  e1 = 31, h1 = 28, e8 = 3, h8 = 0.
- Plane: `2 × type + (colour ≠ p)` for pawn to queen (0 to 9), and 10 for both kings.
- Index = band × 22,528 + KB(ksq ^ oH) × 704 + plane × 64 + (s ^ oH).

**Threats.** For each colour c, first p, then the opponent: every diagonal pawn attack on a piece that is neither
king nor pawn, then every square attacked by a knight, bishop, rook or queen of c (real attacks, with the current
occupancy) that holds a piece other than a king. Kings never attack and are never targets.

For an attacker A of colour cA on `f` and a target D of colour cD on `t`:

1. orient the squares, `f' = f ^ oT`, `t' = t ^ oT`, and for the black perspective swap the colours (`cA ^= 1`,
   `cD ^= 1`);
2. take the slot of the pair of types, by attacker: pawn {knight 0, rook 1}; knight {P 0, N 1, B 2, R 3, Q 4};
   bishop and rook {P 0, N 1, B 2, R 3}; queen {P 0, N 1, B 2, R 3, Q 4}; every other pair has no feature (a pawn on
   a bishop or a queen, a bishop or a rook on a queen);
3. a pair of the same type, enemies or not pawns, is counted once: it has no feature when `f' < t'`;
4. `att` = the attacks of A from `f'` on an empty board (for a pawn, its two captures for colour cA);
5. with `NV` = {P 4, N 10, B 8, R 8, Q 10} and the ten coloured attackers x = cA × 5 + type, let `off[x][s]` be the
   number of empty-board attacks from the squares before s (pawns only from ranks 2 to 7), `tot[x]` their total, and
   `base[x]` the sum of `NV × tot` over the attackers before x; the grand total is 59,808;
6. index = `base[x] + (cD × NV/2 + slot) × tot[x] + off[x][f'] + popcount(att & ((1 << t') − 1))`.

**Pawn pairs.** For every pawn on `f` (colours in the order p, opponent) and every pawn on `t` on the same or an
adjacent file, ranks 2 to 7, `t ≠ f`, with `f > t` on the raw squares so that each unordered pair is counted once:
with `id(colour, s) = 48 × (colour ^ p) + (s ^ oT) − 8`, `hi` and `lo` the larger and smaller of the two ids, the
index is `hi × (hi − 1) / 2 + lo`. Total 96 × 95 / 2 = 4,560.

**Passed pawns.** A pawn with no enemy pawn ahead on its own or the adjacent files and no own pawn ahead on its own
file (the rear pawn of a doubled pair is not passed). Index = `(colour ≠ p ? 48 : 0) + (s ^ oT) − 8`. Total 96.

### Integer scales

| quantity | integer value of 1.0 | type |
|---|---|---|
| transformer weights and bias | 256 | int16 (int8 for threats, pairs, passed pawns) |
| PSQT | 9,600 | int32 |
| activations after the transformer (l0) | 128, at most 127 | uint8 |
| l1 weights / bias | 128 / 16,384 | int8 / int32 |
| l2 weights / bias | 64 / 8,192 | int8 / int32 |
| output weights / bias | 128 / 16,384 | int8 / int32 |

### Accumulator and forward pass

For each perspective p: `acc[p][j] = bias[j] + Σ W[f][j]` over the active features (j = 0..1023) and
`psqt[p][k] = Σ P[f][k]` (k = 0..7, no PSQT bias). The sums are exact in int16 arithmetic modulo 2^16, so the order
of the rows does not matter and an incremental update gives the same accumulator as a refresh.

With `us` the side to move and bucket b = (n − 1) / 4:

1. **l0** (1024 uint8): for q in (us, them) and j = 0..511,
   `l0[512 i + j] = (clamp(acc[q][j], 0, 255) × clamp(acc[q][512 + j], 0, 255)) >> 9`.
2. **l1** (32 int32): `x1 = b1 + w1 · l0`; the skip term is `x1[30] − x1[31]`, taken before any activation.
3. **l1 activations** (64 uint8): the 32 squares first, `min(x1² >> 21, 127)`, then the 32 values,
   `clamp(x1 >> 7, 0, 127)`. The square is of the signed pre-activation. |x1| can reach 77,939, so it is clipped to
   16,383 before squaring, which is exact and keeps the product in 32 bits.
4. **l2** (32 int32), then its activations: `min(x2² >> 19, 127)` and `clamp(x2 >> 6, 0, 127)` (|x2| clipped to 8,191).
5. **Output**: `out = b3 + w3 · (l1 activations, l2 activations)`.
6. **Network outputs**, in internal units (600 = 1.0 of the trainer), with division truncated toward zero:
   `positional = (out + skip) × 600 / 16384`, `psqt = (psqt[us][b] − psqt[them][b]) / 32`. Flooring, or dividing by
   16 before the multiplication, is wrong on 27 to 55% of positions.

There is no PSQT bias, no factorized term (it is folded into the weights at export), and no rounding to nearest:
only arithmetic shifts and truncated divisions. Every scale comes from the trainer: l0 is (1/256)² brought to 1/128
by the shift of 9; x1 is in units of 1/16384, so its value in 1/128 needs a shift of 7 and its square a shift of 21;
x2 is in 1/8192 (shifts 6 and 19); the trainer's output quantization, multiplied by 600, is exactly step 6.

### What a move changes

- Piece-square rows: the moved piece (removed and placed), a captured piece (removed), a promotion (pawn removed,
  piece placed), castling (two pieces). Perspective p needs a full recomputation when its king changes bucket or
  crosses between files d and e (the orientation changes, so every row changes), and both perspectives do when the
  phase band changes.
- Threat, pawn-pair and passed-pawn rows depend on `oT`, so a king crossing between the halves of the board changes
  them all. Threats also change for pieces that did not move, since a moving piece opens or closes lines.
- Only the set of active features matters: an incremental update equals a refresh by construction.

### Numbers that guided the design

Measured on 4,000 consecutive positions from games:

| block | active per perspective (mean / 99th percentile / max) | rows changed per half-move (mean / p99 / max) |
|---|---|---|
| threats | 12.9 / 37 / 41 | 4.54 / 14 / 21 |
| piece-square | 17.3 / 32 / 32 | 2.20 / 3 / 4 |
| pawn pairs | 13.1 / 36 / 38 | 1.29 / 11 / 14 |
| passed pawns | 1.1 / 5 / 5 | 0.14 / 2 / 3 |

11% of half-moves force a recomputation of a perspective. On average 141 of the 1024 bytes of l0 are nonzero
(13.8%), but they fall in 90.7 of the 256 groups of four bytes (35%), which sets the work of the sparse first layer.

## 6.4 The module

| file | role |
|---|---|
| `consilium/cn.h` | network shape, the weight block (`Net`), the per-thread state (`Handle`), the public calls |
| `consilium/cn.cpp` | index tables, row lists, accumulator kernels, forward pass, loader, `evaluate` |
| `consilium/cn_thrperm.inc` | a generated order of the threat rows by frequency (off by default) |
| `consilium_bridge.cpp/.h` | the engine side: loading the network, the per-thread handles, the evaluation scale |

The module reads the engine's board as it is (a8 = 0, piece codes 0 to 11, mailbox) and the changes of each move that
the engine's make already writes on its stack (`nn_dirty.h`). It returns the two network outputs.

**Weight block.** All weights live in one block of fixed size (about 170 MB), so that it can be copied byte for byte
into memory shared between engine processes. Each feature row holds its 1024 weights followed by its 8 PSQT values,
padded to whole 64-byte lines, so the PSQT values arrive with their row. The l1 weights are stored a second time in
the order in which the SIMD forward pass produces l0.

**Per-thread state.** One accumulator per stack state and perspective (1024 int16 sums and 8 int32 PSQT sums), and a
refresh cache with one entry per perspective, king square and phase band, which holds the bias plus the piece-square
rows of the pieces it last saw. A refresh brings an entry to the current board with only the pieces that changed.

**Evaluation.** For each perspective the stack is walked back to the nearest computed state, unless a move on the way
forces a refresh (a king move of that perspective, or a change of phase band). Then one of four paths:

1. both perspectives one move from a computed parent: the rows of both in one pass over the move (each threat decoded
   once), then one pass of the kernels per perspective;
2. a king step inside the same half of the board: the child's accumulator is the parent's minus the parent's cache
   entry plus the child's, plus the rows of the move; the threat and pawn part is not recomputed, because its
   orientation depends only on the half;
3. a refresh: the cache entry brought to the board, then all threat and pawn rows in one pass, then the chain filled
   back down so that nearby branches of the search start from a computed state;
4. a chain: the missing states one move at a time.

**Kernels.** The accumulator is updated in tiles of registers (two tiles of 16 AVX-512 registers for an update or a
refresh, four tiles of 8 for the king step). Each tile loads its source once, applies every row of the list and stores
once; the first tile also carries the PSQT sums. The sign of a row costs no branch: every row enters both lists and
only the right counter moves.

**Forward pass.** l0 is computed with high-half multiplications on the two halves of each accumulator; the packing
order of the vector instructions is absorbed into the stored l1 weights at load. The nonzero groups of four bytes of
l0 are compacted in registers, and l1 is a sparse product over them (VNNI dot products where available). The small
layers are fully vectorized. AVX-512, AVX2 and a scalar path are written; the scalar path also serves as the reference.

## 6.5 From the network outputs to the evaluation

The engine turns the two outputs into its static evaluation in five steps, now written as five named functions in
`consilium_bridge.cpp`: blend (a weighted sum of the two outputs, with a weight of its own in endgames), agree (damped
when the two outputs disagree), material (stretched with the material left, and the search's optimism added on the
same scale), fifty (pulled toward zero as the fifty-move counter grows) and scale (a factor per output bucket, then
the bounds of a normal score). The constants were tuned by our SPSA runs on this network.

The default shape keeps the mathematical form of Stockfish's scaling, rewritten in our own code, with constants
tuned on our network. A shape of our own is written behind `EvalShape = 1` and is off: the agreement step becomes
v × A / (A + d), which never changes the sign of the value however large the disagreement d, and the fifty-move step
keeps the full value for the first `EvalShapeR50` plies of reversible play, then falls linearly to zero at ply 100.
It will be judged on games, then tuned (`EvalShapeAgree`, `EvalShapeR50`).

## 6.6 Verification

On every build of the module:

- the golden set: the three numbers identical on all 2,000,124 positions;
- the bench unchanged (126352 before the block 1 values were written into the source, 280323 after);
- `nnueverify` during the bench: at every evaluation the incremental accumulators are compared with a full refresh,
  with no difference;
- `nnperft 4` on five positions (including castling, promotions and en passant): 0 differences on 6.6 million
  evaluations.

The script is `tools/verify_consilium.ps1` in the development folder.

## 6.7 Speed

Measured with hardware counters (xperf), engine pinned to one logical processor, search threads only, 30 middlegame
and 30 endgame positions at 400,000 nodes each, 4 rounds in alternating order, on a Xeon Gold 6138 (AVX-512 without
VNNI), PGO builds. The nodes are identical across engines, so cycles per node is the verdict. Each session contains
a second build of the previous engine as a null test.

| engine | middlegame, large pages | endgame, large pages | middlegame, no large pages |
|---|---|---|---|
| previous inference | 4,765 cycles per node | 4,460 | 4,962 |
| new module, build 1 | 4,505 (−5.5%) | 4,236 (−5.0%) | 4,727 (−4.7%) |
| new module, build 2 | 4,493 (−5.7%) | 4,226 (−5.3%) | 4,716 (−5.0%) |

The new module needs about 5% fewer cycles per node, about 5.5% more nodes per second, in the middlegame and in
endgames, with and without large pages. The two builds of each source agree within 0.25 points.

These measurements were made on AVX-512 builds only. The AVX2 build of block 6 turned out to be about 14% slower than
the previous inference (6,290 against 5,534 cycles per node in the middlegame); block 7 rewrote the AVX2 forward pass
and recovered it, ending 2% ahead (section 6.9).

The largest gains came from fewer mispredicted branches: summing the PSQT values in the first tile instead of in
separate loops, prefetching each row when it enters the list instead of in a separate pass, and a threat row computed
without a branch. They added about 3.5 points; the forward pass about 0.5.

## 6.8 Ideas measured and rejected

| idea | measured | verdict |
|---|---|---|
| separate accumulators for the piece-square block and for the rest | extra stores on every update | one accumulator |
| int8 rows widened to int16 at load | +60% L3 misses, −10% in the middlegame | rejected |
| prefetch of every line of every row | saturated the miss buffers | first line and PSQT line only |
| separate PSQT table | one more scattered line per row, about 4% of the engine | PSQT at the end of the row |
| cancelling rows added and removed along a chain | 7.6 to 7.5 rows per update | no gain |
| several moves of a chain in one pass | holes in the stack, repeated refreshes | one move at a time |
| threat rows ordered by frequency | 90% of the reads in 12 MB of the 65 MB block, but one more lookup per threat: −0.8 points on the Xeon | off, to measure on other processors |

## 6.9 Block 7: the last files of Stockfish origin outside the search

After block 6, four small groups of files of `source/nnue/` remained, with their original licence headers, because
other parts of the engine used them: the vector slider attacks (`attacks.*`, on the hot path), the tables they filled
at startup (`bitboard.*`), large-page allocation (`memory.*`) and helpers and types for these (`misc.*`, `types.h`,
`tune.h`). Block 7 rewrote what the engine needs from them in the same clean-room way as block 6: first the interface
and the behaviour (which calls, which types, which results), then code written from public descriptions of the
techniques and from the Windows and Linux documentation, without reading the previous code. The previous files served
only as a black box, to compare results and speed. `source/nnue/` no longer exists.

| new file | replaces | what it does |
|---|---|---|
| `sliders.h/.cpp` | `nnue/attacks.*`, `nnue/bitboard.*` | bishop and rook attacks from the bit geometry, both from one computation |
| `large_pages.h/.cpp` | `nnue/memory.*` | memory on 2 MB pages for the network, the transposition table, the shared search tables and the per-thread data |
| `incbin/` | `nnue/incbin/` | moved: the public-domain header that embeds the network in GCC and clang builds |

Removed without replacement: `nnue/misc.*`, `nnue/types.h`, `nnue/tune.h` (only the files above used them),
`evaluation.h` and the table `nnue_pieces` (a conversion of piece codes that nothing used any more: the network reads
the engine's own codes). The `setoption` parser of `uci_mt.cpp`, which followed the previous engine's, is written
again: the line is read as words, the option name is rebuilt up to the word `value` and the value with one space
between words, so a GUI that sends extra spaces is understood the same way.

### Slider attacks

Every square lies on four lines: two diagonals, its file and its rank. Three kernels give the same attacks:

- **AVX-512: obstruction difference on the four lines in one register (new).** The obstruction difference is a
  published technique (chessprogramming wiki): on one line, with `lower` the occupied squares below the slider and
  `upper` those above, the attacked squares are `(upper ^ (upper - b)) & line`, where `b` is the highest bit of
  `lower | 1`. The formula needs the highest set bit of a word, which vector instructions could not give until
  AVX-512 added a lane-wise leading-zero count (`vplzcntq`, AVX-512 CD). Our kernel puts the four lines of a square
  in the four 64-bit lanes of one register and computes them together, with no table and no byte reversal; lanes
  ordered (diagonal, file, antidiagonal, rank) give bishop and rook by or-ing the two 128-bit halves. **This
  combination is ours: we have not found it described elsewhere.**
- **AVX2: hyperbola quintessence in three lanes (our variant).** On a line with at most one square per rank,
  reversing the bytes of a bitboard reverses the line, so the attacks in both directions come from two subtractions.
  Our form reverses the line occupancy inside the register with one shuffle (no reversed masks, no byte swap of the
  occupancy in a general register) and loads twice the slider's bit and its reversed copy from a per-square table, so
  the only value moved from a general register into a vector register is the occupancy. The rank, which has no such
  reversal, comes from a table of 512 bytes read in parallel.
- **Scalar** (no AVX2, ARM): the obstruction difference line by line.

Correctness: every kernel against an independent ray walk on every square, every subset of the squares that matter
and 2 million random boards (5,538,944 cases each, 0 differences), on the Xeon and on a Core 7 150U; in the engine,
`-DTRIUMV_VERIFY_SLIDERS` compares every attack with the PEXT tables. Speed of the kernels alone (Xeon Gold 6138,
nanoseconds per call, independent calls / dependent chain):

| kernel | throughput | latency |
|---|---|---|
| previous (vector hyperbola quintessence) | 2.72 | 5.31 |
| obstruction difference, AVX-512 | 1.69 (−38%) | 6.73 (+27%) |
| hyperbola quintessence of our own, AVX-512 build | 2.46 (−10%) | 5.64 (+6%) |
| hyperbola quintessence of our own, AVX2 build | 2.64 (−11%) | 6.16 (+4%) |

On the Xeon the leading-zero count costs 4 cycles of latency; on newer processors (Zen 4, Ice Lake and later) it is
cheaper, which favours the new kernel. Which balance of throughput and latency the engine needs is decided by the
measurement of the whole engine below.

### Large pages

Windows: the lock-memory privilege is enabled in the process token (advapi32 loaded at run time, so no build has to
link it), `VirtualAlloc` asks for `MEM_LARGE_PAGES` rounded to the large-page size, the token is put back as it was,
and the allocation falls back silently to normal pages. Linux and Android: memory aligned to 2 MB with
`madvise(MADV_HUGEPAGE)`, so that transparent huge pages back it. Same behaviour as before: privilege, silent
fallback, transparent huge pages.

### Checks

| check | result |
|---|---|
| bench | 280323; `CorrPhaseQueen=1` 451959; `EvalShape=1` 300816 (all as before) |
| golden set (2,000,124 positions), `nnueverify`, `nnperft 4` | identical, clean, 0 differences |
| Chess960 perft suite, depth 4, `perft` | 960 positions, 0 errors |
| Chess960 perft suite, depth 3, `tdperft` | 960 positions, 0 errors |
| universal binary (`-NoPgo`) | sse41, avx2-nopext, avx2, avx512: bench 280323; vnni512, avx512icl, avxvnni built, not runnable on the Xeon |
| every attack against the PEXT tables (`-DTRIUMV_VERIFY_SLIDERS`) | bench 280323, no difference |
| MinGW builds sse41, avx2, avx512 | bench 280323 |

### Speed of the whole engine

Same method as section 6.7 (xperf, engine pinned, search threads only, 30 middlegame and 30 endgame positions at
400,000 nodes, 4 rounds), PGO builds of one batch: the engine before block 7 (base) and the new sources. Cycles per
node; negative = faster than the base of the same instruction set. Null test (a second copy of the base): −0.19%
middlegame, −0.18% endgame.

| build | kernel | middlegame | endgame |
|---|---|---|---|
| AVX-512, base (previous slider attacks) | | 4,543 | 4,321 |
| **AVX-512, new** | **obstruction difference, four lines in one register** | **4,499 (−0.98%)** | **4,269 (−1.20%)** |
| AVX-512, new, forced | hyperbola quintessence of our own | 4,517 (−0.59%) | 4,296 (−0.58%) |
| AVX2, base (previous slider attacks) | | 6,280 | 5,849 |
| AVX2, new | hyperbola quintessence of our own | 6,302 (+0.35%) | 5,863 (+0.24%) |

On AVX-512 the new kernel makes the whole engine about one point faster, and it is the default of the three AVX-512
builds (avx512, vnni512, avx512icl). On AVX2 the plain hyperbola quintessence is slightly slower than the previous
code. A second form was written for AVX2, `pair_hqs`: the two diagonals in an xmm register while the rook is computed
in general registers (the file with the same formula and a scalar byte swap, the rank from the table), so the two
halves run on different execution ports and nothing crosses the halves of a ymm register. Alone it answers in 4.56 ns
instead of 5.87 (latency −22%), with 10% lower throughput. In the engine (second session, AVX2 builds):

| build | middlegame | endgame |
|---|---|---|
| AVX2, base | 6,303 | 5,862 |
| AVX2, `pair_hq` | 6,313 (+0.16%) | 5,892 (+0.50%) |
| **AVX2, `pair_hqs`** | **6,288 (−0.23%)** | **5,859 (−0.05%)** |

`pair_hqs` is the AVX2 default (at least level with the previous code, slightly ahead in the middlegame); `pair_hq`
stays as `-DSLIDERS_FORCE_HQ`. On the Xeon the kernel of the AVX-512 builds is the one that matters for speed;
measurements on processors with AVX2 only (the Core 7 150U ran the kernel test, correct, speed partial) and on Zen 4
are still to come.

### AVX2 forward pass and AVX-VNNI (found and fixed in this block)

The AVX2 forward pass of block 6 lagged behind the AVX-512 one, and the AVX-VNNI instructions of the previous
inference were missing. Rewritten, identical results: l0 kept in the order the pack instruction leaves it (the l1
weights follow it, as on AVX-512), the nonzero groups compacted without branches (a table of 2 KB), two dependency
chains in the sparse first layer, l2 and the output in registers instead of plain C++, and `vpdpbusd` in its VEX form
with AVX-VNNI. Checked on the AVX2 build: golden set identical, bench 280323, `nnueverify`, `nnperft` clean. The
`avxvnni` build compiles; it cannot run on the Xeon, so its bench and speed on an Intel processor with AVX-VNNI are
still to do.

Against the AVX2 build of block 6 the new forward pass (with `pair_hqs`) needs **13.8% fewer cycles per node in the
middlegame and 15.2% fewer in endgames**, with a quarter fewer mispredicted branches. Measured against the engine
before block 6, this was mostly a recovery: block 6 had been measured on AVX-512 only, and its AVX2 build was about
14% slower than the previous inference.

### Total since the start of blocks 6 and 7

The engine before block 6 against the final sources, PGO builds of one batch, cycles per node (2 rounds):

| build | middlegame | endgame |
|---|---|---|
| AVX-512 | 4,786 → 4,524 (**−5.5%**, about 5.8% more nodes per second) | 4,491 → 4,279 (**−4.7%**) |
| AVX2 | 5,534 → 5,425 (**−2.0%**) | 5,100 → 5,026 (**−1.5%**) |

Evaluations and search tree are the same (bench 280323 on all four builds).

## 6.10 Open items

Instruction sets not yet used by the inference module (found on 18/10/2026; the speed measurements so far were on the
Xeon only, AVX-512 without VNNI):

- **AVX-VNNI** (variant `avxvnni`: Intel Alder Lake, Raptor Lake and later without AVX-512): the AVX2 forward pass
  still uses three instructions where `vpdpbusd` in its VEX form does one, with the same result. The previous
  inference used it (+4.55% nodes per second on an i9-14900KF). First item to do.
- **NEON and the dot-product instructions** (Android and Apple Silicon builds): the module has no NEON path, so
  accumulator and forward pass are scalar there, much slower than the previous inference.
- **SSE4.1** (variant `sse41`): scalar forward pass and accumulator.
- `avx512icl`: no instruction specific to that variant (the previous use measured +0.04%).

Speed ideas, in order of expected gain (estimates, each to be measured with the rimisura kit on PGO builds, with the
usual checks: bench, golden set, perft):

- **Prefetch of the piece-square rows in the engine's make** (estimate 1-2%): the make knows the piece and the squares
  of a move at once, and the hash probe and the other work before the evaluation would cover the memory latency of
  the 2 KB rows, which today are requested only when the evaluation starts.
- **Slider attacks of two squares per call** (to be measured, possibly 0.5-1% on AVX-512): a 512-bit register holds
  the four lines of two squares, so the loops over all sliders (move generation, the threats in `11_queue`,
  `06_nndirty`) would make half the calls. It plays to the strength of the new kernel, its throughput.
- **The remaining mispredicted branches of the inference** (about 1.6 per node more than the previous inference,
  estimate 0.5%): sample them again.
- **A shared pass over the threat changes of both perspectives in multi-move chains** (estimate 0.5-1%; count first
  how often chains occur).
- **Sparse first layer without VNNI**: pairs of four-byte groups whose products cannot overflow 16 bits could share
  one `madd` (the pairs can be classified at load; estimate 0.3-0.5%).
- **Done on 18 October:** three dependency chains of `vpdpbusd` in the sparse first layer of the VNNI builds
  (avxvnni, vnni512, avx512icl), and int8 rows stored with interleaved bytes in the avx2-nopext and avxvnni builds, so
  that a row widens to int16 with `vpmaddubsw` and `vpsraw` on two ports instead of `vpmovsxbw` on one. Both ideas are
  A Tin Man's, measured by him on the previous inference (+4.5% on Zen+, +1.1% on an i9-14900KF, −2.2% on Zen 3, which
  is why the avx2 build keeps the plain form); the code in our module is new. Identical results (golden set, bench).
  Speed on those processors still to measure.
- **Zen 4** (VNNI, AVX-512 at 256 bits per cycle): measure the inference and the slider kernels there (the
  leading-zero count is cheaper there than on the Xeon), and the threat-row order.
- **A smaller refresh cache** (512 entries of about 2 KB per thread, often outside L2 when read): small gain, mostly
  with many threads.

Checked (not speed): a clean MinGW `nopext` build gives bench 280323 and the same perft as the other builds; an
earlier 267502 came from object files left by another build (the Makefile does not rebuild when the options change).

Other items:

- `EvalShape = 1`: SPRT, then SPSA of its two parameters.
- The Linux and Android branch of `large_pages.cpp` is not compiled yet (no NDK on the test machine).
- Historical comments that cite Stockfish for comparison (`tt.h`, `profile.h`, `io.cpp` and others): not code; to be
  rewritten in one pass.
