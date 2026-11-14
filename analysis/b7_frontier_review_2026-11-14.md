# B.7 — frontier review of the board work (2026-11-14)

**Status: research packet (R3). No engine source changed. The maintainer
decided on it the same day (§10): the amendments are applied with three
changes, and the leaf numbers of §6 and §7 are superseded by §10's.**

**Recommendation: amend B.7.3's handoff before it runs (§6): keep C1 and
C5, replace C3's pre-test by SEE pins read from aligned sliders, drop C2
and C6, move C7 to G.2, and build the search replay first as the board's
instrument.** In the engine itself the two kept speed components read
+1.86% and +1.56% (a non-PGO screen, 16 alternating rounds, every round
faster), where the handoff's C2 and C3 read no gain and the hint's nearest
form loses 0.74% (§3). All of them are exact on 58.6 million recorded board
results and keep `bench 13` at 4,247,965.

It reviews `analysis/b72_board_review_2026-11-14.md` (the B.7.2 packet) and
its instruments, at the maintainer's request: is the method sound, what
would make the board clearly better than Stockfish's, and what must the
board guarantee before NNUE (PLAN F.1).

## 1. What this rests on

- **Read:** the B.7.2 packet, the donor inventory
  (`analysis/b7_board_speed_inputs_2026-11-14.md`), B.7.1's packet, the
  board source, the microbenchmark and the four adapters, the raw runs of
  `tools/results/b72-micro5/`, the profile report, BAS-P04, BAS-P26, Rarog's
  RAR-M31 and RAR-M35 records, Stockfish 19's `position.cpp` at `edb0d9db`,
  and the shared trainer's `docs/architecture.md`.
- **Not repeated:** no read on record was timed again. The composite of
  §2.3 is recomputed from the kept runs.
- **New, all throwaway** (ignored, under `tools/results/b7-frontier/`;
  recipes in §8):
  - **Counters** inside SEE, `gives_check`, make and the generator, over
    `bench 13`. They were taken 19 commits before the head, on this branch
    (`5f61c94`, `bench 13` 4,278,307, before the root-repetition repair).
    The rates they share with the head's trace agree within 4%.
  - **A search replay.** A trace of every board operation the search makes
    during `bench 13` at the head (`2231b50`, 4,247,965 nodes, 58,598,640
    records), and a replayer that runs it on one board (§2.5).
  - **Nine prototype kernels** behind macros, each verified against the
    trace and timed on it; six of them also built into the engine and read
    on `bench 13` (§3).
  - **A cold-board test** of the five harness binaries of B.7.2's read
    (§2.6).
- **Host:** the 5950X. Checked idle before the replay reads (2.5% and 3.7%
  busy, no engine, harness or build); the cold-board runner gates itself.
  Builds ran between timed reads, never during one.
- **Time per node:** the head's PGO release build
  (`basilisk-b71-fix-pext-pgo.exe`) runs `bench 13` at 2,332,764 nodes per
  second, 429 ns per node. One run, used only to turn profile shares into
  nanoseconds.

## 2. Method review

### 2.1 What the microbenchmark times

`tests/board_micro.cpp` is a sound per-primitive screen, and its checksums
are a good guard for a rewrite. It does not time what its header says, "the
population the search meets":

- **Game positions, not search nodes.** The 512 positions are every ninth
  ply of about 35 games. The search's nodes differ: the side to move is in
  check at 6.8% of the file's positions, at 12.6% of the search's capture
  generations and at 21.2% of its quiet generations.
- **Every legal move, equally.** `make_unmake` and `gives_check` run over
  all 14,085 legal moves. The search's makes are 39% captures and 9% checks.
- **SEE at threshold 0 on tactical moves.** The search's SEE calls are 60.5%
  captures, 37.0% quiets and 2.5% promotions; 71% have a negative
  threshold, 10% zero and 19% positive. A quarter leave before any lookup (§2.4).
- **One fixed order, repeated.** Each pass replays the same moves in the
  same order, in generation order (all of a piece type's moves together),
  so the branch predictor sees a learnable stream.
- **512 boards.** Each operation lands on another board, where a search has
  one board that stays in the first-level cache.

These pull in different directions and partly cancel (§2.5): the micro's
make/unmake is close to the replay's, its generation is half the real cost.

### 2.2 Fairness between engines

| Issue | Checked how | Effect |
|---|---|---|
| **Hinted make.** Stockfish's and Rarog's makes are given `gives_check` precomputed for every move | adapters read | Fair for their own searches. Under Basilisk's call mix it is not: Basilisk's search knows the answer for 43% of its makes (§2.4). A hinted board would owe a `gives_check` for the rest, 0.53 per node: about 1.7 ns per node for Stockfish and 4.0 for Rarog that the composite does not charge |
| **Quiet stage stand-in.** `legal_gen` stands in for the quiet stage | recomputed, §2.3 | Decides the comparison with classical Stockfish by itself. Stockfish's `legal_gen` is its full pseudo-legal generation plus a legality filter, which its search never runs |
| **Pseudo-legal captures.** Stockfish's column generates 1,104–1,150 moves, unfiltered, and all evasions when in check | adapters read; the packet says so | Its legality test at delivery is untimed, about one test per made move. The composite still weights the column |
| **Work the composite leaves out.** Stockfish's make stores the repetition distance and the check squares; Basilisk computes both on demand | replay, §2.5 | Basilisk's draw tests (1.8 ns per node) and the picker's check squares are uncharged; Stockfish's are inside its make |
| **SEE.** One value vector for all, but threshold 0 on tactical moves only; Stockfish and Rarog pass 36 of the 927 moves unexamined | counters | The column prices 10% of the search's SEE calls by threshold class |
| **Prefetch.** Classical Stockfish's make always prefetches its table entry | adapter read | Small, against classical Stockfish |
| **Cold boards.** Stockfish's `Position` is larger than Basilisk's `Board`; 512 of them may cost it more | §2.6, measured | Small: nothing on make, 5% on Stockfish's SEE, 11.5% on classical Stockfish's `legal_gen` |
| **Flags.** Rarog and Reckless are built `target-cpu=native`, Basilisk and Stockfish with fixed BMI2 flags; no arm has PGO | CONTRACT.md | BAS-P26 read native tuning 1% slower for Basilisk; the shipped engine is a PGO build, which the micro is not |

### 2.3 The composite, recomputed

The packet's table reproduces from the kept runs: Basilisk 65.8 ns per
node, Rarog 67.0, classical Stockfish 71.7, Stockfish 19 83.1.

| Term (rate per node) | Basilisk | Stockfish 19 | Stockfish classical | Rarog |
|---|---:|---:|---:|---:|
| make/unmake (0.92) | 15.9 | 48.5 | 32.6 | 13.5 |
| capture stage (0.53) | 9.2 | 5.4 | 7.1 | 10.5 |
| quiet stage, `legal_gen` (0.20) | 9.6 | 15.3 | 17.9 | 12.4 |
| SEE (1.42) | 21.7 | 9.3 | 9.4 | 19.6 |
| `gives_check` (1.44) | 9.4 | 4.6 | 4.6 | 10.9 |
| **total** | **65.8** | **83.1** | **71.7** | **67.0** |
| total without the quiet stage | 56.2 | 67.8 | 53.8 | 54.6 |

- **Without the stand-in, classical Stockfish and Rarog are ahead of
  Basilisk** (53.8 and 54.6 against 56.2). Classical Stockfish ties
  Basilisk if its quiet stage costs 60 ns; Rarog if its costs 56 ns. Their
  stand-ins are 89 and 62 ns; Basilisk's own quiet stage is 39 ns
  (`staged_gen` less `capture_gen`), not the 48 ns the composite charges it.
- **Stockfish's stored check information is a wash in this table, not a
  loss.** Against classical Stockfish, Basilisk's queries cost 17.0 ns per
  node more and its make 16.8 ns less. Stockfish's make also carries work
  that is not check information (piece lists, the incremental score, the
  repetition scan, the prefetch).
- **What the table supports:** Stockfish 19's board is dearer (its make
  writes the network's records). Against Rarog and classical Stockfish the
  order is not resolved by this instrument.

### 2.4 The search's own call population

Counters over `bench 13` (4,278,307 nodes), per call class:

| Question | Answer |
|---|---|
| **SEE: where calls end** (5.85 million) | before any lookup 24.2%; no enemy attacker, after `attackers_to` 28.0%; the pin computation is reached by 47.9%; the exchange loop by 46.5%, 1.19 steps on average |
| **SEE: would C3's pre-test skip the pins?** | at 61.2% of the pin computations neither king has an enemy slider on its empty-board lines, at 33.2% one king has, at 5.6% both: 77.8% of the per-colour work is skipped |
| **SEE: do pins matter?** | a pinned piece exists at 17.9% of the pin computations, and is one of the attackers at 1.16% of them: 0.56% of all SEE calls |
| **SEE: recomputation** | 2.15 million departure tests, 48,134 recomputations |
| **`gives_check`: lookups** (5.94 million) | the mover is a slider in 61.2% of calls (one or two lookups for the direct test); 91.8% reach the discovered test (two lookups, always) |
| **`gives_check`: would C2's pre-test skip?** | it passes in 1.33% of the calls that reach it; a discovered check exists in 0.14% |
| **make: is the hint known?** (3.95 million) | the search asked `gives_check` for the same move in 43.0% of makes: 38.5% answered no, 4.5% yes. 9.1% of makes give check |
| **make: kinds** | captures 39.3%, promotions 1.2%, castling 0.7%, en passant 0.1%; an en-passant square is set in 0.39% |
| **generation: pins** | a piece is pinned at 10.6% of capture generations and 9.1% of quiet ones; no enemy slider is on the king's empty-board lines at 68.3% and 63.6% |
| **generation: king targets** | 4.0 attack tests per quiet generation (0.78 per node), 0.12 per capture generation |
| **repetition** | `is_draw` 1.00 per node, 88.2% leave at a clock under 4; 0.29 loop steps per node. The cuckoo test 0.49 per node, 84% leave early |
| **`check_squares`** | 4.19 per node, of which 3.64 are the evaluation's and 0.56 the picker's |

Read against the handoff:

- **C2 and C3 pre-test the right thing.** Both skips are frequent.
- **C2 leaves the direct test's lookups** (61% of calls).
- **C3 keeps four lookups per colour** in the 22% it does not skip.
- **C6's hint reaches 38.5% of makes**, not most of them.
- **C5 saves nothing measurable:** 88% of repetition tests never enter the
  loop.

### 2.5 A better instrument: search replay

**Design.** A trace build records every board operation the search makes
on its own board, in order: make, unmake, null moves, each generation
stage, SEE with its threshold, `gives_check`, `check_squares`, the table
move test and the draw tests, each with its result. The replayer runs the
trace on one board and does nothing else. So the positions are search
nodes, the order and the mix are the search's, the board stays hot, and the
branch history is real.

**Prototype** (`tools/results/b7-frontier/replay/`, §8):

- The trace build keeps `bench 13` at 4,247,965 nodes.
- `--verify` replays all 58,598,640 records and compares every result: the
  key after each make, each SEE and `gives_check` answer, each generated
  count. It passes, and fails at once on a planted wrong result.
- The trace's rates per node equal the packet's counted mix (make 0.922,
  capture stage 0.527, quiet stage 0.199, SEE 1.424, `gives_check` 1.441).

**Its own cost, controlled.** The replayer's dispatch is an unpredictable
switch, and that cost is not the board's. Each read is therefore paired
with a stub run: the same records and the same switch, every case calling
an empty out-of-line function. Board cost is the timed run less the stub.
The stub over-corrects, because a real board call hides part of the next
misprediction, so the two figures bracket the truth. An A/B of two boards
on one trace needs no correction: the dispatch is identical.

**The head, in real search order** (ns per node; three alternating rounds
of five passes, one pinned CPU; spreads 0.5–2%, one `none` round 17%):

| Class | Micro composite | Replay, less stub | Replay, uncorrected | Profile share × 429 ns |
|---|---:|---:|---:|---:|
| make, unmake, null | 15.9 | 17.2 | 26.1 | 26.4 |
| generation | 16.9 to 18.8 | 25.9 | 29.5 | 27.5 |
| SEE | 21.7 | 22.2 | 28.8 | 26.3 |
| `gives_check` | 9.4 | 11.1 | 16.9 | 14.3 |
| table-move test | — | 1.2 | 2.0 | 2.3 |
| draw tests | — | 1.8 | 5.0 | 3.5 |
| all recorded operations | 65.8 | 80.0 | 131.0 | 103.0 |

- **The replay and the profile agree.** The profile's figure lies between
  the replay's two for each of the four large classes. They are independent
  instruments: one samples the PGO engine, the other times a plain build of
  the board alone.
- **The composite underprices the board by 22% to 57%.** Generation costs
  about half as much again in real order as on the game positions.
- **Limits.** The replay's board is a plain release build, not the PGO
  engine's inlining. Nothing else competes for the caches, so the 840 KB
  slider tables sit better than in a search. It prices the board alone;
  pooled NPS still decides what ships.

**What a kept version needs** (a leaf, §7): the recorder behind a build
option; the replayer as a test target with `--verify`; a Python driver that
alternates binaries on a pinned CPU, refuses a busy host and records hashes
(the shape of `board_micro_run.py`); a short trace regenerated on demand
rather than committed (469 MB for `bench 13`; 40 roots at a node cap give
the same mix at a tenth of the size). A cross-engine form replays the same
trace through each adapter, moves resolved at load; it is the only way to
compare boards on the search's real work.

**What it gets wrong, measured (§3).** Both components that gain in the
engine gain less on the replay (4.3 against 6.9 ns per node, 2.1 against
8.2): with nothing else running, the memory a change stops touching was
already cheap. The direction agreed for the five kernels read both ways in
one form. So the
replay is a conservative screen with the right sign, and the alternating
engine read of §3 is the stronger one where a change touches table reads.

### 2.6 Cold boards

The same 32 positions once (32 boards) and sixteen times over (512 boards,
the full file's footprint), four disjoint subsets, the five binaries of
B.7.2's read. Time on 512 boards over time on 32, median of the subsets:

| Column | Basilisk | Stockfish 19 | Stockfish classical | Rarog | Reckless |
|---|---:|---:|---:|---:|---:|
| `make_unmake` | 1.012 | 1.019 | 1.008 | 1.009 | 1.017 |
| `see_captures` | 1.014 | 1.049 | 1.053 | 1.016 | 1.032 |
| `gives_check` | 1.003 | 1.015 | 1.012 | 1.001 | — |
| `legal_gen` | 1.007 | 1.026 | 1.115 | 1.000 | 1.003 |

Holding 512 boards does not inflate anyone's make. It costs Stockfish 5%
on SEE and classical Stockfish 11.5% on `legal_gen`, the column that stands
in for its quiet stage.

### 2.7 Which of the packet's conclusions stand

| Conclusion in the B.7.2 packet | Verdict |
|---|---|
| Basilisk's board is the fastest of the five on its search's work | **Stands against Stockfish 19 and Reckless. Not resolved against Rarog and classical Stockfish:** the order turns on the quiet-stage stand-in, the uncharged hint and the uncharged on-demand work (§2.2, §2.3) |
| Stored check information loses at Basilisk's mix; this confirms the lazy form | **Not supported by the table**, which shows a wash. The lazy form is still right, for a measured reason: the cheap queries are not where the time is (§3) |
| C2: most calls skip both lookups | The skip is real (98.7%). **The gain is not:** no gain in the engine (§3) |
| C3: pin work is 1.71% of samples, so the pre-test pays | The skip is real (77.8%). **The gain is not:** the cost is the code around the lookups, which the pre-test keeps (§3) |
| C6: the hint is BAS-P04's untested half and accounts for most of Rarog's make lead | **The hint is known for 38.5% of makes.** No gain on the replay; its nearest form loses 0.74% in the engine. Not built with search wiring |
| C1: about half of the unmake helpers' work is discarded key work | **Stands, and is larger than priced:** +1.86% in the screen |
| C5 is too small to price | Stands: 88% of repetition tests never enter the loop |
| Each column is the population the search meets | **No:** game positions, uniform moves, threshold 0 (§2.1) |
| The dirty-piece record: make writes it into `UndoInfo` | **Wrong owner, and redundant** (§5) |

## 3. Prototypes: what was measured

Nine kernels, each behind a macro in one throwaway copy of `board.cpp`
(`prototype_final.patch`, §8). For every one:

- **Exact.** `board_replay --verify` passes on all 58,598,640 records, and
  every microbenchmark checksum is unchanged. The six built into the engine
  give `bench 13` 4,247,965.
- **Replay:** timed difference to the head on the class the kernel changes,
  three alternating rounds of five passes. The classes a kernel does not
  touch are its null controls; they read within ±0.4 ns of the head.
- **Engine screen:** plain release builds (no PGO), `bench 13` on one
  pinned CPU, 16 alternating rounds, the per-round time ratio to the head.
  The head runs 445.6 ns per node in this build.

| Kernel | Replay, ns per node | Engine: NPS, median of 16 (range) | Verdict |
|---|---:|---|---|
| **C1** unmake without key work | −2.1 | **+1.86%** (+0.48 to +2.43) | keep |
| **SEE pins from aligned sliders** (below) | −4.3 | **+1.56%** (+0.32 to +2.10) | adopt, in place of C3 |
| C3 as handed off: pre-test, then `see_pins` | −0.1 | −0.16% (−1.05 to +0.32) | no gain |
| C2 as handed off: pre-test before the two lookups | +1.5 † | −0.32% (−1.97 to +0.53) | no gain |
| `gives_check` without slider lookups | +0.9 | −0.21% (−1.05 to +0.42) | refused |
| make derives the checkers from the move | +1.0 | **−0.74%** (−1.46 to −0.11) | refused |
| C6 as handed off: hinted make | +0.2; +0.5 with no hint | not built ‡ | no gain |
| SEE attackers from aligned sliders | +2.2 over the pins alone | not built | refused |
| generator pins from aligned sliders | −0.5 | not built | NO_CHANGE |
| C1 + SEE pins + C2 together | — | **+2.46%** (+1.29 to +3.42) | |

† With a first test of the line before the pre-test; the engine arm is the
single-branch form.
‡ The hint needs the search to pass it. The replayer passes it wherever the
last `gives_check` on the position was for the move made.

**The new SEE pins.** `see_ge` no longer computes pins up front. A piece of
colour `c` is pinned, on the exchange occupancy `occ`, when it alone stands
between its king and an enemy bishop, rook or queen that is on an
empty-board line of its kind from that king:

    snipers(c) = (rook lines of king c  & enemy rooks, queens)
               | (bishop lines of king c & enemy bishops, queens)
    pinned(c)  = { s : BB_BETWEEN[king c][p] & occ == {s}, p in snipers(c) & occ }

and it may still capture when the target is on that line. It is read where
an attacker is chosen, on the occupancy of that moment. That is the same
set `see_pins` returns, and re-reading it replaces both the pinner test and
the recomputation after a departure (`see_departure_opens_line`). At 61% of
the calls that reach it, both sniper sets are empty and nothing more runs.

**What the nine say together.** A slider lookup is not the cost; the code
around it is. Every kernel that only removed or skipped lookups gained
nothing or lost (C3's pre-test, C2, the lookup-free `gives_check` and make,
the aligned attackers, the generator's pins: Rarog's RAR-M31 again). The
two that removed a whole path gained: the pin array, its filter loop and
the departure test in SEE; the discarded key updates in unmake. On this
host a PEXT lookup is three cycles and a cached load, and it does not
branch; a pre-test is a branch.

**Limits of the screen.** The engine arms are not PGO builds. PGO may
already drop part of what C1 removes (the helpers are inlined into unmake
there), so C1's pooled figure can be smaller. B.7.4's pooled read decides.

**Prediction for the pooled read, frozen here.** C1 and the SEE pins
together: **+2.0%**, 80% interval [+0.8%, +3.2%]. Under +0.5% falsifies the
screen as a predictor of pooled NPS, and the replay with it.

## 4. Frontier ideas, ranked

Ranked by expected gain at Basilisk's call mix, measured ones first. The
recorded losses are respected: BAS-P04 (eager check information, pin
sharing), BAS-P26 (compressed tables), RAR-M35 (carried attacker set),
RAR-M44(c) (side templates with forced inlining). RAR-M31 asked for a
floor before a retry; the generator pins were read against one here and
did not clear it.

**1. SEE pins from aligned sliders. Measured: +1.56%.**
- **Mechanism:** §3. It wins because it removes a 256-byte array, a call,
  a filter loop and a test per exchange step, not because it removes eight
  lookups.
- **Touches:** `Board::see_ge`, `Board::see_pins`,
  `Board::see_departure_opens_line` (`src/board.cpp`); two 64-entry
  empty-board tables. `Board::see` has no caller (B.8.2 deletes it).
- **Must hold:** SEE exact against B.7.1's exchange-tree oracle (BAS-C08,
  BAS-C09), the king-legality rule's unfiltered opponent set, the
  fingerprint.
- **Falsifier:** done. In B.7.3: the old form beside the new at every call.

**2. C1, unmake without key work. Measured: +1.86%.** As handed off.

**3. Carry SEE verdicts per move. Ceiling measured: 10.2% of SEE calls.**
- **Mechanism:** the picker asks `see_ge(m, t1)` and the search asks again
  with another threshold: 20.6% of SEE calls are a second question about a
  move already asked at that node. `see_ge` is monotone in its threshold,
  so an earlier yes at or above the new threshold, or an earlier no at or
  below it, answers 0.145 calls per node without a call.
- **Touches:** `CoreMovePicker::next` (the good-capture test),
  `ProbCutPicker`, the pruning tests of `search_kernel_core.cpp` (lines
  303, 527, 537, 587, 1511, 1577). Not the board: B.8.5's.
- **Must hold:** the answers, so the fingerprint.
- **Expected:** up to 0.145 × 18.5 ns = 2.7 ns per node (0.6%) less the
  cost of carrying the verdict. Prior: 1.9.1's memo read +0.36%.
- **Falsifier:** the engine screen of §3 on a build that carries one
  verdict per move; under +0.3% it is dropped.

**4. Captures not generated twice. Measured share: 8.7% of capture
generations** repeat at the same node (the ProbCut picker, then the main
picker). About 1 ns per node. B.8.5's TB-8 item; the trace gives its size.

**5. Undo as one block. Unmeasured.**
- **Mechanism:** make saves the changing state as one image (the twelve
  bitboards and occupancies, 136 bytes, and the keys and counters, 72
  bytes) and unmake copies it back: no branch on the move's kind, no
  read-modify-write. The mailbox is restored by two to four stores, or
  joins the image once `Piece` is one byte (64 bytes instead of 256). The
  undo stack becomes a fixed array, and the key history a dense array of
  its own.
- **Touches:** `UndoInfo`, `Board::make_move`, `unmake_move`, the null
  moves, `history` (`src/board.h`), and every reader of `history[i].hash`
  (`is_repetition`, `upcoming_repetition`, `is_draw`).
- **Must hold:** B.7.1's restore-every-field walk, the history capacity
  contract (no allocation in a search), repetition and the cuckoo test.
- **Expected:** 0 to +1% beyond C1. C1 already takes the largest part.
- **Falsifier:** the engine screen against the C1 build; under +0.5% it is
  dropped. H.4, not B.7.

**6. One enemy attack map per node, built on demand. Unmeasured; priors
conflict.**
- **Mechanism:** the quiet stage tests 4.0 king targets per generation
  with up to two lookups each, the quiet scorer builds threat maps, and
  28% of SEE calls end at "nobody attacks the square" after two lookups.
  One map of the enemy's attacks answers all three.
- **Touches:** `gen_legal_impl` (king moves, castling), `threats_against`
  (`src/threats.cpp`), `CoreMovePicker::score_quiets`, `Board::see_ge`.
- **Must hold:** the generated order, SEE exactness (an attack through the
  mover's origin is not in the map, so the exit needs a line test).
- **Expected:** −0.3% to +0.5%. Triumviratus −0.35% cycles for the shared
  map; Rarog −0.58% for the SEE exit; BAS-P04 −0.16% for sharing between
  stages. §3's lesson argues against it.
- **Falsifier:** the engine screen; under +0.5% dropped. H.4.

**Refused, with the evidence and what would reopen each:**

| Idea | Why not | Reopens when |
|---|---|---|
| Check and pin information stored at make (Stockfish) | a wash on B.7.2's own table (§2.3); BAS-P04; SEE now needs no stored pins | never on this evidence |
| Check information cached per node on first use | the lookup-free queries it would feed lose (§3) | a profile of the network head (H.4) shows the tables cold |
| Lookup-free `gives_check`, make and SEE attackers | measured, §3 | the same |
| Smaller slider tables, or Stockfish 19's table-free form | BAS-P26 −0.98% | H.4, under the network's cache load; G.2 for the AVX2 tier |
| Attack tables kept incrementally | Stockfish 19's make pays about 17 ns more than classical's for threat records alone (52.7 against 35.5 ns) | F.7 adopts a threat-input network; then one table serves the network, SEE and legality |
| A byte mailbox read by vector permutes (Rose, Clockwork) | needs AVX-512 VBMI; the development host and the AVX2 tier have none; no measured comparison with PEXT exists | G.2 has an AVX-512 host |
| Side or stage as a template parameter | RAR-M44(c) −0.55% | its own trigger |
| Pseudo-legal generation with a late legality test | under 0.5% by B.7.2's columns, and it reorders the unsorted tail, so it moves the tree | never for speed alone |
| Copy-make | idea 5 takes its one advantage (a free unmake) without a board per ply | — |
| Pins shared between the two generation stages | BAS-P04 −0.16%; the pins cost 0.5 ns per node in all (§3) | never |

**Against Stockfish.** What Stockfish's board does better than Basilisk's
is one thing, its SEE (6.6 against 15.3 ns in B.7.2's column), and idea 1
is the part of that gap that is not the 36 moves it leaves unexamined.
Everything else in its design costs more at Basilisk's mix or is a wash.
The board is 24% of a node; the measured set is about 2% of a node. No idea
found here is worth more than that on this hardware, and the larger
throughput items are not the board's: the picker's second pass and the
prefetch (B.8.5), and ideas 3 and 4.

## 5. The NNUE question

**Will Basilisk lose speed at F.1?** Under the B.7.2 packet's §3.2, a
little, for nothing: make would write seven fields into a larger
`UndoInfo` at every node while the classical evaluation reads none of them
(Triumviratus's cheapest form cost +0.6% to +1.1% of instructions). Under
the contract below, no: make writes nothing new.

**Research amendment 1 to the B.7.2 packet, §3.2 (the dirty-piece
record).** The record is not written by make and does not live in
`UndoInfo`:

1. **The delta is a triple the search already holds at every make:** the
   move, the piece that moves and the piece captured (`Board::piece_on`
   before the make, or `Board::captured_piece` after it). F.1 stores it in
   the evaluator's own per-ply stack at the make call site. PLAN F.1
   already says the stacks are the evaluator's.
2. **The triple is complete.** Quiet move: one piece leaves `from` and
   arrives on `to`. Capture: the captured piece also leaves `to`. En
   passant: the captured pawn leaves the square behind `to`. Promotion: a
   pawn leaves `from`, the promoted piece arrives. Castling: king and rook,
   the rook's squares from the castling geometry. At most four changes.
3. **It is reversible:** the chain of triples from any earlier ply and the
   current board give every position between.
4. **A null move adds an empty entry,** so the stack index stays the ply.
5. **Unmake does nothing for the evaluator:** its stack is indexed by ply.

**What the board guarantees, from B.7.3 on:**

- **No evaluator state in `Board` or `UndoInfo`,** no observer call and no
  network branch in make.
- **The twelve piece bitboards stay directly readable**
  (`Board::piece_bitboards`). A refresh, and a refresh cache keyed by king
  bucket, reads exactly those twelve; this is a second reason to keep the
  representation, beside RAR-M39.
- **`king_square` and the occupancy count stay O(1):** the king bucket and
  the output bucket are read from them.
- **Lazy updates need no board.** The accumulator of a ply is computed on
  demand by walking the triples back to the last computed ply. When a king
  crossed a bucket on the way, that perspective is refreshed at the current
  ply from the current board, never at a ply between.
- **Threat features, if a later network has them, are computed the same
  way,** from the triples and the current board, outside make.

**What to re-read once the network runs** (H.4, on its profile): the
network's weights and accumulators take the second-level cache the slider
tables now sit in, so the forms §3 refused for lack of gain (the
lookup-free `gives_check` and make, the aligned SEE attackers, BAS-P26's
compressed tables) are read again there. Their patch is kept (§8).

**Not the board's, for F.5:** the triple is known before the make, so the
weight rows it will touch can be prefetched with the child's table entry.
Unmeasured; the memory share Triumviratus reports (47% of last-level
misses in the update) is the reason to try it.

## 6. How to proceed

**Position: (b), amend the handoff, with one instrument leaf before it;
then close B.7.** No new research leaf in B.7: ideas 5 and 6 and the
re-reads are small or conditional on the network, and by the Phase H rule
they wait in H.4, which already owns "the board, SEE, picker" on the
network head. Ideas 3 and 4 are B.8.5's.

**The amended handoff** (research amendment 2 to the B.7.2 packet, §6):

| # | Component | Change | Evidence |
|---|---|---|---|
| C1 | Unmake without key work | as handed off | replay −2.1 ns; screen +1.86% |
| C3′ | SEE pins from aligned sliders | §3's definition; `see_pins` and `see_departure_opens_line` leave `see_ge` | replay −4.3 ns; screen +1.56% |
| C5 | Repetition scan from distance 4 | as handed off | cleanup; no measurable effect |

- **Dropped:** C2 and C6 (no gain measured), C3's pre-test form (replaced).
- **C7 goes to G.2, whole.** Tables alone do not make Chess960
  representable: the move encoding (the king's destination) cannot express
  a castle whose king stays or lands on the rook's square. G.2 owns X-FEN,
  the king-takes-rook encoding and the perft suite, and the tables belong
  with them. B.7.3 adds no new hard-coded castling square.
- **Rules as handed off:** one commit each, the fingerprint, B.7.1's
  corpus, the search-path perft, the old form beside the new behind a
  build switch.

**The gate (B.7.4), amended.** The +0.5% floor per component cannot be
read by an instrument whose resolution is ±0.5%. Proposed:

1. Each component is read on the replay against the commit before it, and
   kept when its own class is faster by more than twice the spread of the
   untouched classes and no other class is slower by that much.
2. One pooled-PGO read of the rebuilt board against the head, floor +0.5%
   for the set. §3's prediction is its registered expectation.
3. The microbenchmark's checksums stay as the semantic guard; its timings
   are no longer the per-component read.

**Cost.** The replay leaf is the prototype made permanent (about 900 lines
exist as a patch). B.7.3 shrinks from six components to three. One pooled
read, as planned.

## 7. Proposed PLAN amendments (not applied)

The open leaves of B.7 renumbered in the order they run:

    - [ ] **B.7.3** `[I1]` Search-replay board benchmark
      Added 2026-11-14 (`analysis/b7_frontier_review_2026-11-14.md` §2.5):
      a trace build that records every board operation of a search with
      its result, a replayer that runs the trace on one board
      (`--verify` compares every result; timed runs are paired with a
      stub control), and a Python driver that alternates binaries on a
      pinned CPU and refuses a busy host. Behaviour-neutral: the trace
      build keeps `bench 13`. Done when the verify pass fails on a
      planted wrong result and the head's class table reproduces the
      review's within its spread. A cross-engine form is not part of
      this leaf.
    - [ ] **B.7.4** `[I2]` Rebuild behind the board contract
      (was B.7.3) B.7.2's handoff as amended by the frontier review §6:
      C1, C3′ and C5, one commit each; C2 and C6 dropped on measurement;
      C7 moved to G.2. Rules unchanged.
    - [ ] **B.7.5** `[V]` Board speed read and gate
      (was B.7.4) Each component read on the replay against the commit
      before it; one pooled-PGO read of the set against the head, floor
      +0.5%, expected +2.0% [+0.8%, +3.2%] (frontier review §3). BAS-P28
      first, as before.

Additions to existing leaves:

- **B.7.2:** a line recording research amendments 1 and 2 and this packet.
- **B.8.5:** ideas 3 and 4 with their measured ceilings (0.145 SEE calls
  per node answered by an earlier call; 8.7% of capture generations
  repeated).
- **F.1:** the dirty-piece contract of §5 in place of "per-ply dirty-piece
  deltas" written by make.
- **G.2:** C7, with the reason it is not separable.
- **H.4:** ideas 5 and 6, and the re-reads of §5, each with its floor.

## 8. Recipes and evidence

Everything is under `tools/results/b7-frontier/` (ignored). First 16 hex
digits of each SHA-256:

| File | SHA-256 | What |
|---|---|---|
| `counters/counters.patch` | CED30C28D58F1663 | the counters of §2.4, on `5f61c94`; `counters.err` holds every count |
| `replay/prototype_final.patch` | A7C836C81DAB6F0E | on `2231b50`: the recorder, `tests/board_replay.cpp`, and the nine kernels behind `PROTO_*` macros |
| `replay/bench13.trace` | 4C6643CD9CB43460 | `BASILISK_BOARD_TRACE_FILE=... basilisk bench 13` on a `-DBASILISK_BOARD_TRACE=ON` build |
| `replay/bin/replay2_replay.exe` | F377543AF76C95E9 | the head's replayer |
| `engine/bin/basilisk_replay.exe` | 452477BEC80661CE | the head's engine for the screen |
| `engine/bin/basilisk_p_see.exe`, `_p_um.exe`, `_p_set.exe` | 1171625E4EE02C8F, 8C6F6F7974E6E4E3, 8E69D30DFF9517A0 | the SEE pins, C1, and the set |

- **Builds:** `cmake -S . -B build/<name> -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DPORTABLE_BUILD=ON -DUSE_PEXT=ON "-DCMAKE_CXX_FLAGS=-DPROTO_..."`, then
  targets `board_replay`, `board_micro_bench` and `basilisk`. Clang 23.1.5.
  Macros: `PROTO_UNMAKE_NOKEY` (C1), `PROTO_SEE_SNIPERS` (C3′),
  `PROTO_SEE_C3`, `PROTO_GC_C2`, `PROTO_GC_NOLOOKUP`, `PROTO_MAKE_CHECKERS`,
  `PROTO_MAKE_HINT`, `PROTO_SEE_ALIGNED`, `PROTO_GEN_SNIPERS`.
- **Replay reads:** `replay/replay_ladder.py`; outputs `head_ladder_v2.json`,
  `proto_ladder.txt` and `proto_ladder2.txt` with their JSON samples.
- **Engine screen:** `engine/bench_ab.py`; output `bench_ab.txt`.
- **Trace counts:** `replay/trace_census.py`; output `trace_census.txt`.
- **Cold boards:** `instrument/make_subsets.py`, `cold_board_test.py`.
- **Composite:** the medians of `tools/results/b72-micro5/run*_*.txt`,
  weighted by the packet's rates.

## 9. Findings outside this review

- **DESIGN §5 contradicts the maintainer's direction of 2026-11-14.** It
  says the board is not a bottleneck and "make it faster" is not a
  motivation. The board is 24% of a node and B.7's point is its speed. The
  bullet should be rewritten when B.7 closes.
- **`check_squares` is mostly the evaluation's:** 3.64 of its 4.19 calls
  per node, two lookups for the queen where the bishop's and rook's are
  already computed. The evaluation is frozen; noted for C.
- **`Board::is_in_check` and `has_non_pawn_material` are out-of-line calls**
  made 1.2 times per node each. LTO may inline them; a header definition
  would not depend on it. B.8.5.
- **Two throwaway worktrees** held the prototypes. They were removed the
  same day; their content is the two patches of §8.

## 10. Maintainer decisions, 2026-11-14

The review was accepted with three changes, and PLAN was reworked to match.

- **B.7 ends with the fastest board that can be measured here.** So the
  unmeasured ideas of §4 (undo as one block, the byte mailbox, the attack
  map) do not wait in H.4: they are a second round inside B.7, with keys
  by table, next attacker by index and the hinted make with the search
  wired. Only the forms §3 measured as no gain wait for H.4.
- **Chess960 is not a goal.** C7 is withdrawn, not moved to G.2, and G.2's
  Chess960 item with it. The board is built for standard chess.
- **The separate findings are fixed or placed:** DESIGN §5 is rewritten;
  the two out-of-line accessors are a component of the rebuild (C8); the
  evaluation's `check_squares`, the carried SEE verdicts and the repeated
  capture generation are B.8.5's, with their counts.

**Leaf numbers.** §6 and §7 wrote B.7.3, B.7.4 and B.7.5. The roadmap
retires B.7.3 and B.7.4 instead (HISTORY's number map):

| In §6 and §7 | In PLAN | Leaf |
|---|---|---|
| B.7.3 (new) | B.7.5 `[I2]` | Board instruments: replay and engine screen |
| B.7.4 | B.7.6 `[I2]` | Rebuild behind the board contract: C1, C3′, C5, C8 |
| — | B.7.7 `[R2]` | Second round: candidates on the instruments |
| — | B.7.8 `[I2]` | Second rebuild: the candidates that cleared |
| B.7.5 | B.7.9 `[V]` | Board speed read and gate |

**One more read, taken for the decision:** the two kept kernels on the
microbenchmark's own columns (`tools/results/b7-frontier/micro_protos/`,
three alternating rounds). `see_captures` 15.25 to 10.42 ns with the new
pins (Stockfish 6.6), `see_quiets` 18.43 to 15.77, `make_unmake` 17.89 to
15.98 with C1 (Rarog 14.7). On B.7.2's composite without its quiet-stage
term that is 56.2 to about 47.6 ns per node, against 53.8 for classical
Stockfish and 54.6 for Rarog. B.7.9 reads it properly, on contract v3.
