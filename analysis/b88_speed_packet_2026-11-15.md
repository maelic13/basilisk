# B.8.8 — where can the search get faster at an identical tree?

- State / class: `READY_FOR_IMPLEMENTATION` for B.8.9 / `R3`
- Owner / date: Fable, 2026-11-15, on the head `ea3b2e0` (`bench 13` 4,688,537)
- Decision needed from the maintainer: the rule for gains under the bar (§4.4,
  D-17), and whether the one evaluator-seam item (S1, §6) is in scope.
  **Decided 2026-11-15: both accepted as proposed.**

**Recommendation.** Build the prefetch from a predicted key first and read it
under load: the table's first load is 7% of a game's node, an eighth of the
probes have no prefetch at all, and Stockfish, Rarog and Triumviratus each
measured this change at +0.7% to +2.2% alone. Nothing else in the search is
that large; the rest of the pass is seven smaller items and a build read.

Every item below is behaviour-neutral: no node count, score or move changes.
No engine code changed in this leaf. The evidence is B.8.5's archived samples
read again by line, deterministic counts from a scratch build, and the donors'
records.

## 1. Known evidence

### 1.1 Where a game's node goes (B.8.5's samples, read by line)

B.8.5's two sample sets (`tools/results/b85-profile-games`, `-bench`) symbolized
again with line numbers (`tools/results/b88/prof_lines.py`, `prof_view.py`).
Shares are of all samples; the profiler's per-position clear is 5.96% of the
cohort's (`TranspositionTable::clear` 4.53%, `CoreHistoryTables::clear` 1.44%),
so a share of the search alone is the figure times 1.063. Line numbers are
those of `28006a8`, the profiled build.

| Where | Cohort | Bench | What it is |
|---|---:|---:|---|
| `tt.h:209`, the probe's first load of the cluster | 6.56% | 2.23% | memory wait |
| `score_quiets`, the history loads (lines 101 to 105) | 5.14% | 5.27% | memory wait |
| `partial_insertion_sort` | 3.72% | 3.16% | cohort: insertion pass 2.68%, partition 1.02% |
| `CoreMovePicker::next`, own frame | 4.27% | 4.34% | the stage `switch` alone 1.54% and 1.43% |
| the history `update` (`history_core.h:58`) | 1.96% | 1.90% | read-modify-write of cold rows |
| `correction_value` | 1.74% | 2.36% | cohort: continuation cells 0.85%, structure cells 0.71% |
| `lmr_reduction` | 1.26% | 1.36% | holds a division by `root_delta_` |
| `fill` (list to buffer) | 1.14% | 1.20% | the per-stage copy |
| `gives_check` | 1.94% | 2.21% | the board's; B.7 closed it |
| `evaluate_full` and its frames | 18.1% | 22.6% | not a target |

- **The probe's wait is split evenly**: 3.39% under `negamax`, 3.39% under
  quiescence, although quiescence probes 0.41 times a node against `negamax`'s
  0.67. A quiescence probe waits 1.7 times as long.
- **Why (evidence):** `negamax` prefetches the child's cluster after the make at
  its three sites (the move loop, the null move, ProbCut). Quiescence's two
  loops make their moves with no prefetch: `quiescence<NonPV>` holds no
  `prefetch` instruction in the disassembly.
- **Inference:** at 400 ns a node (2.50M alone, B.8.5), each half is 14.4 ns a
  node. An interior probe then waits 21 ns, although prefetched: the prefetch
  leaves only the parent's last lines and the child's entry tests as lead.
  Quiescence's own children are 31% of its probes (0.127 a node, counted in
  §1.2); if its prefetched probes wait the same 21 ns, an unprefetched one
  waits 66 ns, a whole miss.
- **What A1 can reach, by that model:** 5.7 ns a node from prefetching
  quiescence's children at all, and about 6 ns more from the make's own time
  as extra lead on every probe (the make is about 1.5% of a node): 3% of a
  game's node, alone, at Hash 64.
- The search's own code is spread thin: no line of `negamax` outside
  `lmr_reduction` holds more than 0.3%.

### 1.2 Counts (new, deterministic)

A scratch build of the head with counters (`tools/results/b88/counters.patch`,
SHA-256 143C7459…, applied by `instrument.py` and `instrument_2.py`; plain
release, no PGO; the counts are `counts_cohort.txt` and `counts_bench.txt`
beside it). It reproduces `bench 13` 4,688,537 and the cohort's reported node
total 111,624,167. "A node" is a move made, the engine's count.

| Per node unless named | Cohort | Bench |
|---|---:|---:|
| interior nodes | 0.674 | 0.650 |
| quiescence entries | 0.407 | 0.453 |
| moves quiescence itself makes | 0.127 | 0.165 |
| moves picked at a move loop (`search_one`) | 1.583 | 1.619 |
| picker `next()` calls | 1.773 | 1.801 |
| stage dispatches per `next()` | 1.87 | 1.80 |
| quiet generations | 0.237 | 0.208 |
| quiets scored per generation | 18.93 | 21.93 |
| sorted prefix per generation | 7.53 | 7.72 |
| insertion shifts per generation | 28.42 | 30.51 |
| quiets delivered per generation | 5.21 | 5.76 |
| lazy selection's element visits per generation | 67.09 | 76.05 |
| evaluations | 0.499 | 0.557 |
| interior table cutoffs, per interior node | 0.09 | 0.06 |
| quiescence table cutoffs, per entry | 0.20 | 0.19 |
| predicted key exact, of moves made | 98.51% | 97.66% |
| missed: promotion, en passant or castling | 1.29% | 1.93% |
| missed: a double push that sets a legal en-passant square | 0.19% | 0.41% |

- Quiets delivered per generation, cohort: none 1.5%, one 39.0%, two 13.1%,
  three or four 13.3%, five to eight 14.9%, nine to sixteen 9.9%, more 8.3%.
- The evaluator's draw-verdict query evaluates the position a second time:
  1,700,022 times in the cohort against 73,532,185 evaluations (2.3%), of
  which 607,864 follow an evaluation the same node had just made.
- A profile's reported node count is the last `info` line's, not the search's:
  the cohort made 147,285,059 moves. B.8.5's NPS figures count the limit (its
  §2), so they stand.
- **The cohort fingerprint.** `go depth 10` on each of the 1,000 positions,
  Hash 16, `ucinewgame` before each: **24,421,195 nodes**, digest
  `8bc7f3c8d1dde0d2` over each position's node count and best move
  (`tools/results/b88/cohort_depth.py`; the PGO head and the scratch build
  agree). It reaches the endings `bench 13`'s forty positions do not: 267 of
  the positions have ten men or fewer.

### 1.3 The donors and the sibling

Stockfish, the local checkout at `1e7e6bde` (2026-11-14):

- `Search::Worker::do_move` prefetches the table's cluster from
  `Position::prefetch_key(move)` before the make, and the three
  continuation-correction cells the child will read (`4150d22b`, about +0.5%
  on Zen 3 as its message reports). Quiescence shares that function.
- `Position::do_move` prefetches the child's pawn-history cell and the four
  structure-correction entries once the keys are final.
- The main search computes `correction_value` before `tt.probe`
  (`search.cpp:832` and `:837`), so its loads overlap the probe's.
- `ed2f9bdc` removed a second table prefetch inside `Position::do_move`;
  `c1bca289` hoists the history pointers in the quiet scoring loop.
- Its sort is the scalar insertion sort unless AVX-512 is built.
- Its build: `-flto=full`, IR-level PGO (`-fprofile-generate`), an inline
  threshold of 500. Not verified here, from the fetch: the predicted-key
  prefetch read +0.69% [0.47, 0.91] on top of an in-make prefetch (PR 6911),
  and a four-thread PGO training read +1.6% to +3.4% and was not merged (PR
  6543).

Rarog, the live repository at `fae1d0a`, read at the maintainer's request
(`analysis/c12_speed_pass_2026-10-17.md` and `c12_speed_followup_2026-10-17.md`
there):

- The probe's first load was 6.89% of its samples with the prefetch after the
  make; the prefetch from a hinted key before the make read +1.69% [+1.56,
  +1.89] (its RAR-P30 and RAR-P31).
- Search parameter sets as compile-time constants outside tune builds: kept,
  inside a +3.15% bundle of four commits. One set stayed run-time because a
  test sets a switch of it.
- Its conclusion after a second day in which nothing else paid: "bound by
  memory latency, not by instruction count". What read flat or negative
  there: prefetching every quiet's history entries (−3.1%), the correction
  prefetch after the make (−0.4% and −0.6% on 4 KB pages, +0.27% [−0.29,
  +0.83] on large pages), vectorised and interleaved selection (−3.8% to
  +0.06%), one SEE per move answering every threshold (−0.2%), history rows
  as fixed arrays (−0.59%), quiets scored as generated (flat).
- Its two largest items do not exist here: the by-value list copies (D-10)
  and the scan under `skip_quiets` (D-11).

Triumviratus, the pinned snapshot (`docs/reference/triumviratus/`):

- The same predicted key at the top of its make, exact for plain moves and
  captures, corrected after the make when it differs: +2.21% NPS. Issued
  earlier, as the move leaves the queue, it gained nothing.
- The correction entries prefetched in the make: +2.04%.
- The next move's prefetch at pick time lost 1.6% on a Ryzen alone and 4.2%
  with seven engines.
- Ranking a move by the count of larger keys replaced an insertion sort that
  caused 11% of its mispredicted branches: −0.70% cycles for the AVX-512 step.
- ext-tsp block layout and the cmov converter off: −0.6% (with another
  change) and −0.15% to −0.29%. Rarog read the cmov switch at −0.34%.

Basilisk's own ledger: BAS-P02 (history row hoists, +3.03% in the kernel
deleted since), BAS-P03 (SEE memo, +0.36%), BAS-P04 and BAS-P31 (check info
and the hinted make refused, the last at −0.49% on 2026-11-14), BAS-P06 (a
richer PGO corpus retired), BAS-P26 (flat slider tables stay).

### 1.4 The build as it is

Read from a configured release build: `-O3 -flto=thin -mavx2 -mbmi2 -static`,
clang 23.1.5 for `x86_64-w64-windows-gnu`. PGO is front-end instrumentation
(`-fprofile-instr-generate`), trained on `bench 13` at one thread, the training
build without LTO. `negamax<NonPV>` is 7,003 instructions with nine `idiv`;
its move lambda is inlined. No hot function holds a stack-protector check.

## 2. Unknowns and hypotheses

- **H1 (leading).** The engine is bound by memory latency where Rarog was: a
  change pays when it shortens a wait or removes work on a wait's path, and
  reads flat when it only removes instructions beside one.
- **H2.** The gates' conditions multiply memory items. B.8.5 read the table's
  traffic at 4% of a node alone and about 15% with fifteen engines; a
  single-engine `bench` read at Hash 16 sees a third of the cohort's probe
  wait and none of the load.
- **H3 (alternative to H1 for the code-shape items).** The picker's branches
  and the search's divisions cost what their samples say, so removing them
  pays even with no wait behind them. The sort and the stage dispatch are the
  two tests of it.

## 3. Interaction map

- **Prefetch → cache line → probe.** A prefetch changes no value any reader
  sees. The key it uses is never stored or compared. Under SMP it adds no
  write. It fetches no line the child's probe would not fetch: one prefetch
  for each move made, and a second for the 1.5% whose key is corrected. This
  is not the next-move prefetch that lost on AMD.
- **Correction prefetches → the child's `correction_value`.** The cells are
  the ones the child reads; the child's training writes them later through
  its own path.
- **A1, A2, A3 and A4 overlap.** Each shortens the same chain (make, entry
  tests, probe, correction). Their gains are not additive, and A4 is only the
  fallback for A2 and A3.
- **B1 and B2 overlap** on `lmr_reduction`'s two parameter reads; each is
  read alone.
- **B3, B4 and B5 share the picker.** One verify switch covers B3 and B4
  (§6); B5 changes arithmetic's form, not its operations.
- **Build arms move every other item's baseline.** They are read after the
  memory items and before the code-shape items, which then fit the build that
  ships.
- **PGO trains on `bench 13`, which the pooled read also measures.** A build
  or training change can fit the bench and not the game; the cohort read is
  the held-out workload.

## 4. Method

### 4.1 Memory items are decided under load

- **Instrument, built first in B.8.9:** `tools/cohort_read.py` (Python). It
  runs fourteen engines at once, each pinned to its own physical core, seven
  from each of two pools, the cores swapped between the arms every cycle.
  Each engine searches the first 200 cohort positions to 150,000 nodes, Hash
  64, one `ucinewgame` at its start and none between positions (a game clears
  the table once). A reading is an engine's nodes over its summed search
  time, the nodes counted as B.8.5 counted them (the limit, or the last
  line's when a search ends early). The estimate and the verdict rule are
  `nps_read.py`'s, on per-build medians over six cycles. Two physical cores
  stay free for the harness.
- **Why concurrent arms:** both arms see the same load and the same clock in
  the same seconds, which is what the four-thread read lacked.
- **Its null first** (a new BAS-P entry): `b84-on` against `b84-on-null`,
  both built already. The instrument is admitted when the null reads within
  ±0.5% with an interval that holds 0 and a half-width of 1.0% or less.
- **The rule for a memory item (A1 to A4):** kept when the pooled `bench`
  read accepts it by the standing rule; or when the loaded read is +1.0% or
  more with its lower bound above 0 and the pooled read passes the
  no-regression form. The same tool at one engine is reported beside it and
  decides nothing.
- **If the null fails:** memory items are read as every other item (the
  pooled `bench` read at +0.5%), and the loaded predictions stay unverified.

### 4.2 The four-thread read

No second design in this pass. One thread, the loaded read and B.8.4's pinned
scaling check decide: the loaded read exercises the shared caches a
multi-thread search loads, and a 1% four-thread read needs the clock held,
which is a host setting. The multi-thread PGO training goes to D.2 with it.

### 4.3 Exactness, for every item

- `bench 13` 4,688,537 on Release, Tune and Diag builds.
- The cohort fingerprint, 24,421,195 and its digest (§1.2), which
  `cohort_read.py --fingerprint` prints.
- CTest, B.8.3's allocation guard, `tools/diag/hot_path_calls.py`.
- The item's verify switch (§6) on `bench 13` and the cohort fingerprint
  where it replaces a computation.

### 4.4 Gains under the bar (D-17) — the maintainer's decision

Accepted by the maintainer, 2026-11-15, as proposed:

- An item that adds a prefetch, a table, a vector path or a build flag clears
  its own bar alone, or is not kept.
- An item that only removes, hoists or reorders work and adds no state (B1,
  B2, B3, B5, S1, S2) is kept under the bar when it reads alone at −0.1% or
  above at step 1, **and** the kept sub-bar items together read +0.5% or more
  with a lower bound above 0, eight builds an arm, six cycles. Otherwise
  they leave together.
- Why: Rarog's two +0.2% changes summed to +0.41% [+0.28, +0.54] and its
  bundle of three read +0.15% [−0.35, +0.65]; a sub-bar gain is real only as
  far as a read can show it, and code that removes work costs nothing to keep
  when it does.

## 5. The candidates, ranked

By share of a game's node at stake (cohort samples), then by the prior.

| # | Candidate | At stake | Prior | Verdict |
|---|---|---:|---|---|
| A1 | Table prefetch from a predicted key, every make | 6.56% | +0.7% to +2.2%, three engines | **kept, first** |
| B4 | Sort's insertion pass by rank (AVX2) | 2.68% | −0.70% cycles; Stockfish's AVX-512 sorter | **kept** |
| B3 | Stage machine falls through | 1.54% | none measured | **kept** |
| B1 | LMR's window division hoisted; LMP's halving | 1.26% | none measured | **kept** |
| A2 | Continuation-correction cells prefetched before the make | 0.85% | +0.5% on Zen 3 | **kept** |
| A3 | Structure-correction entries prefetched after the make | 0.71% | +2.04%; flat in Rarog | **kept** |
| B2 | Search parameters as constants outside Tune | 91 reads, 5 `idiv` | in a +3.15% bundle | **kept** |
| C | Build: full LTO, IR PGO, both | whole binary | Stockfish's build; sign unknown | **kept, one read** |
| B5 | Quiet scoring's rows hoisted per piece | part of 5.14% | +3.03% here once; −0.59% in Rarog | **kept, last** |
| S1 | Verdict kept from the node's own evaluation | 0.16% | exact by construction | **kept under D-17** |
| S2 | `root_ordering` tested once per stage | 0.26% | — | **kept under D-17** |
| A4 | Correction computed before the probe | part of 1.74% | Stockfish's form | **only if A2 and A3 fail** |

## 6. The kept items

Each names its mechanism, why it is exact, its semantics, its verify switch
and its read. Predictions are in §8.

### A1 — the table prefetch from a predicted key

- **Mechanism.** `Searcher::do_move` computes the child's key before
  `make_move` and prefetches its cluster, then prefetches again after the make
  only when the true key differs. The lead grows by the whole make, and
  quiescence's children get a prefetch for the first time.
- **Semantics.** With `piece` on the origin and `victim` on the destination:
  `key ^ SideKey ^ PieceKeys[piece][from] ^ PieceKeys[piece][to]`, the
  victim's key at `to` when there is one, `CastlingKeys[rights] ^
  CastlingKeys[rights & CASTLING_MASK[from] & CASTLING_MASK[to]]`, and the
  current en-passant file's key when one is set.
  - The `[NO_PIECE_TYPE]` rows of `PieceKeys` are random, so the victim term
    stays conditional.
  - `do_null_move` does the same with its exact key (`key ^ SideKey` and the
    en-passant file's key).
  - The three `shared_.tt.prefetch` lines in the kernel leave; every make
    goes through the two wrappers.
- **Why exact.** A prefetch reads nothing the program can observe.
- **Verify.** Two Diag counters (predicted, corrected) and a node check that a
  corrected key belongs to a promotion, an en passant, a castling or a pawn's
  double push. A test walks every legal move of the cohort's positions and
  holds the predicted key to the made one outside those four kinds. The
  counts to reproduce: 98.51% exact in the cohort, 97.66% on `bench`.
- **Read.** §4.1's rule.
- **Not this item:** a prefetch at pick time or for the next move; the entry's
  size or the cluster's layout; a branch-free probe (−2.0% in Rarog).

### A2 — the continuation-correction cells, before the make

- **Mechanism.** In `do_move`, before the make: prefetch
  `(ss - 1)->cont_corr->entry[pt][to]` and `(ss - 3)->cont_corr->entry[pt][to]`,
  where `pt` is the moved piece's type, or the promotion's.
- **Why these.** The child's `correction_value` reads its `(ss - 2)` and
  `(ss - 4)` rows at the piece now on `to`; those are these two addresses.
- **Why exact.** As A1. A sentinel row is prefetched harmlessly.
- **Read.** §4.1's rule, alone on the head with A1.

### A3 — the structure-correction entries, after the make

- **Mechanism.** In `do_move`, after the make: prefetch the child's four
  entries, `pawn_corr`, `minor_corr` and both `nonpawn_corr` rows for the side
  now to move, at the board's new keys.
- **Read.** §4.1's rule, alone on the head with what A1 and A2 kept.

### A4 — the correction before the probe (fallback)

- **Only if A2 and A3 both fail.** In `negamax`, `correction_value(ss)` moves
  above `tt_probe`. It reads the board and the records at `ss - 1` to
  `ss - 4`, which the probe does not write. Not in quiescence, where a fifth
  of the entries return on the table before they need it.

### B1 — LMR's window division, and LMP's halving

- **Mechanism.** `lmr_window_term` divides `KLIT(lmr_window_null)` by
  `root_delta_` for every move picked at a null-window node, 1.58 times a
  node, and `root_delta_` changes only in `root_search`. The quotient becomes
  a member set there. The PV form keeps its division.
- **Second line.** The count rule's `/ (2 - improving)` becomes `improving ?
  x : x / 2`, which the compiler shifts.
- **Why exact.** The same integer division, done once; and a divisor that is
  1 or 2.
- **Verify.** A node check that the member equals the quotient at every use.
- **Read.** Pooled `bench` read, the standing rule.

### B2 — search parameters as constants outside Tune

- **Mechanism.** In a build without `BASILISK_TUNE`, an X line of the tables
  the kernel reads (`BASILISK_CORE_KERNEL_PARAMS`, `_B3_PARAMS`, `_B4_PARAMS`)
  declares `static constexpr int` instead of a field, so `kp.name` folds at
  every read site without changing one. The root's and Cluster 4's lines,
  which the driver reads, stay run-time.
- **The exception.** A line a test or the driver assigns stays a field. The
  compiler names them: an assignment to a constant does not build. Known so
  far: quiescence's lines that `test_b4_qsearch` sets, `b3_nmp_verify_depth`,
  and those `test_b3_proof` sets.
- **What it removes.** Five of `negamax`'s nine divisions have a parameter as
  divisor (the two capture-history divisors, the null move's two, the
  singular correction's); the kernel's 91 reads of `kp.` become immediates.
- **Why exact.** The constants are the X lines' defaults, the same tokens.
  The Tune build is unchanged and must reproduce both fingerprints.
- **Check before building:** `parameters.cpp` must not take a member's address
  outside Tune.
- **Read.** Pooled `bench` read, alone, on the head before B1 or after it,
  never bundled with it.

### B3 — the stage machine falls through

- **Mechanism.** `CoreMovePicker::next` re-enters its `switch` after every
  stage change: 1.87 dispatches a call. Each stage instead falls into the
  next (`[[fallthrough]]`), as Stockfish's picker does, so a call dispatches
  once. The `last_src_` stores stay only where a build reads them.
- **Why exact.** The stages, their order, their conditions and their reads of
  `skip_quiets_` are unchanged; only the jump between them is.
- **Verify.** `BASILISK_VERIFY_PICKER`: the kernel runs the old picker beside
  the new one on `move_buffers[ply][1]` (free once ProbCut's picker is done),
  mirrors `skip_quiets`, and aborts when a delivered move differs. It serves
  B4 too, and it is removed when B.8.9 closes with the old picker's code.
- **Read.** Pooled `bench` read.

### B4 — the sort's insertion pass by rank

- **Mechanism.** After the partition pass, which stays, the prefix of length
  `L` is placed by rank instead of insertion: `rank(i)` is the count of scores
  greater than `score[i]`, plus the count of equal scores at an earlier index.
  Each move goes to `out[rank(i)]`.
- **Why exact.** The insertion pass shifts only past strictly smaller scores,
  so it is a stable descending sort of the prefix in arrival order; that rank
  is the stable sort's position. The first element stays in the prefix
  whatever its score, as now.
- **Form.** AVX2, eight scores a compare; scores copied to a padded array
  (padding `INT_MIN`, which is never greater than a score and is excluded
  from the earlier-index mask). For `4 <= L <= 64`; outside it, and on every
  tier without AVX2, the insertion pass as it is. The order is the same on
  every tier, so the fingerprint stays one number.
- **Not** a clamped or packed key: scores are compared as they are.
- **Verify.** B3's switch, and a test of random lists with many ties, `L` from
  0 to 80, against the insertion pass.
- **Read.** Pooled `bench` read. A frame check beside it: `next()` stays
  under 4 KiB (792 bytes now; the two scratch arrays add about 800).

### B5 — quiet scoring's rows, once per piece type

- **Mechanism.** `score_quiets` finds, for every quiet, the pawn-history row
  and five continuation rows of its piece. The six row pointers are kept per
  piece type, filled at the type's first quiet, as `checks_for` keeps its
  squares.
- **Why exact.** The same entries enter the same sums in the same order.
- **Read.** Pooled `bench` read. The prior is split, so it is last.

### C — the build

- **Arms, one read sharing the head's pool:** `-flto=full`; IR-level PGO
  (`-fprofile-generate`, `-fprofile-use`); both, which is Stockfish's build.
- **Second read, on the winner only if one clears +0.5%:** an inline
  threshold of 500 at the compile and at the link (`-mllvm
  -inline-threshold=500`, `-Wl,--mllvm=-inline-threshold=500`) and ext-tsp
  block placement (`-Wl,--mllvm=-enable-ext-tsp-block-placement=1`). A
  spelling is confirmed when the link accepts it and the option is seen to
  act (the binary differs).
- **Why exact.** The source is unchanged; both fingerprints must hold in
  every arm, and an arm whose fingerprint moves is a compiler finding, not a
  candidate.
- **Read.** Pooled `bench` read, and the cohort read at one engine beside it
  (the build trains on `bench`). An arm is kept at +0.5% on `bench` with the
  cohort read not below −0.5%.
- **Owed with a kept arm:** CI's and the release's builds take the same
  flags; the universal binary's tiers are rebuilt and `check_universal.py`
  run.

### S1 — the verdict from the node's own evaluation

- **Mechanism.** `corrected_eval` asks `draw_verdict` whenever the raw
  evaluation is 0, and that evaluates the position again. Where the node has
  just evaluated, `evaluate_undamped` hands back the verdict it already
  computed. A 0 read from the table still asks.
- **Why exact.** Both calls run the same function on the same position; a
  verdict implies a 0.
- **Scope.** It adds one out-parameter to the evaluator's entry point and
  changes no term. In scope (maintainer, 2026-11-15).
- **Verify.** A node check that the kept verdict equals the query's.

### S2 — `root_ordering` tested once per stage

- `score_captures` and `score_quiets` call it per move and it returns 0 below
  the root. The test moves outside the loops.

## 7. Closed candidates

| Candidate | Why closed | Retry trigger |
|---|---|---|
| Prefetch at pick time, or the next move's (D-6) | Large pages removed its reason; the next-move form lost 1.6% on a Ryzen and 4.2% under load | none on AMD |
| Per-stage list copy (D-10) | Rarog's was six 4 KB copies a node; here `fill` is 1.14% and fusing it read flat there | a profile with `fill` above 2% |
| Survivor count under `skip_quiets` (D-11) | The mechanism is absent: both quiet stages stop at once | — |
| Attack tables behind a pointer (D-12) | The board's, and BAS-P26 kept the flat tables | B.15's board |
| Lazy selection for the sort | 67 element visits a generation against 28 shifts and 7.5 inserts | — |
| Scoring fewer quiets | 72% of scored quiets are never delivered, but the best is unknown until all are scored; no exact form | a behaviour change, Phase H |
| History prefetch before quiet scoring | Up to 72 lines a generation; −3.1% in Rarog; Triumviratus's prior | a profile showing one dominant row |
| Lazy correction | Wasted at 8.7% of interior nodes: 0.08% of a node | — |
| Store reusing the probe's slot | Not exact: the subtree rewrites the cluster between the two | D.2's table work |
| Hinted make, check info per node | BAS-P31 on 2026-11-14 (−0.49%); `attackers_to` under the make is 0.50% | B.15's board |
| `gives_check` asked later in quiescence | 0.36% at stake | — |
| SEE verdict carried per move | Ceiling 0.6%, state per move; +0.36% once (BAS-P03) | Phase H |
| Captures generated twice | 0.06 ProbCut generations a node, about 0.25% | Phase H |
| The table move validated once | 0.19 calls a node | — |
| Variable divisors (history divisor table, all-node scale, fail-high blend) | Under 0.3% each; a reciprocal table is new state | — |
| Staged table move in quiescence (NODE-5) | Closed by B.8.5: 1.2% of nodes | — |
| Stack probes (TB-11) | Closed by B.8.5: no frame reaches 4 KiB | B4's frame check |
| Evaluation cache (TB-15) | None exists | — |
| Imbalance cache (A.17) | Built already (`imbalance_table_`); 0.21% of samples | — |
| Material and PST incremental (A.17) | The evaluation's and the board's; leaves with NNUE | Phase C |
| The evaluation's safe-check loop (B.7's frontier) | The evaluation's: its two lines hold 0.81% and the queen's repeated lookups are at most half | Phase C, with the term |
| cmov converter off | −0.34% in Rarog, −0.15% to −0.29% in Triumviratus | — |
| Richer PGO corpus | BAS-P06; the two cohorts differ in the table's share, which is memory, not branches | a cohort read showing an arm of C losing there |
| Multi-thread PGO training | No four-thread instrument (BAS-P36) | D.2, with one |

## 8. Prospective prediction — frozen before exposure

Each is the item alone on the head before it. Bench figures are the pooled
read's; loaded figures are §4.1's read. Intervals are 80%.

| Item | Bench | Loaded | P(kept) |
|---|---|---|---:|
| A1 | +0.8% [+0.2, +1.6] | +3.5% [+1.5, +6.0] | 0.85 |
| A2 | +0.3% [0.0, +0.7] | +0.6% [0.0, +1.5] | 0.35 |
| A3 | +0.2% [−0.2, +0.6] | +0.5% [−0.2, +1.3] | 0.30 |
| B1 | +0.5% [+0.1, +1.0] | — | 0.50 |
| B2 | +0.5% [0.0, +1.2] | — | 0.50 |
| B3 | +0.4% [0.0, +0.8] | — | 0.40 |
| B4 | +0.8% [+0.2, +1.5] | — | 0.65 |
| B5 | +0.2% [−0.3, +0.7] | — | 0.25 |
| C, the best arm | +0.8% [−0.5, +2.5] | — | 0.50 |
| S1 and S2 together | +0.15% [0.0, +0.4] | — | under D-17 |
| **The pass** | **+3.5% [+1.5, +6.0]** | **+6% [+3, +10]** | |

- **In games:** about +7 to +12 Elo at `3+0.03` in the gates' conditions, by
  BAS-P01's 2 Elo per 1%. No game is owed: the tree is identical.
- **Falsifiers.**
  - A1 under +1.0% loaded with the instrument admitted: the probe's wait is
    not lead time (H1 is wrong about the table), and the item returns here.
  - B3 and B4 both under +0.2%: H3 is wrong, the picker's samples are waits
    charged to its branches, and B5 is not built.
  - The pass under +1.5% on `bench` with A1 kept: the bench is the wrong
    workload for this engine's speed, which the closing cohort read says
    outright.
- **Likely failure mode:** a step-1 accept that is a high draw. Rarog's quiet
  SEE shortcut read +1.13% alone and its bundle −0.11%; the closing read of
  the whole pass is the guard.
- **Stop rule:** B.8.9 stops an item at its read's verdict; no item is read
  twice except on a disturbed read.

## 9. Decision

`READY_FOR_IMPLEMENTATION`. The profile, the counts and three engines' records
agree on A1; the others are each small, exact and separately readable. D-17
(§4.4, accepted 2026-11-15) governs S1, S2 and the sub-bar outcomes of B1,
B2, B3 and B5.

## 10. Implementation handoff (B.8.9)

- **Goal:** the items of §6, exact, each read alone, in this order:
  1. `tools/cohort_read.py`, its fingerprint mode and its null (§4.1).
  2. A1, then A2, then A3 (A4 only by §6's rule).
  3. C, its first read; its second only on a winner.
  4. B1, B2, B3, B4, B5.
  5. S1 and S2, then the sub-bar bundle's read, by the maintainer's D-17.
  6. The closing reads PLAN B.8.9 names: the whole pass against B.8.5's head
     (`b84-on`) on `bench` and under load, B.8.4's pinned scaling check for
     D-9, the profile on both cohorts.
- **Per item:** the verify switch agrees on `bench 13` and the cohort
  fingerprint; both fingerprints on Release, Tune and Diag; CTest, the
  allocation guard and `hot_path_calls.py`; a BAS-P entry that cites §8's row
  before the read; the read; kept or reverted by its rule; one commit.
- **Producer → state → consumer:** A1 to A3 add no state. B1 adds one member
  written in `root_search` and read in `lmr_reduction`. B2 turns fields into
  constants. B4 adds two stack arrays inside the sort. B5 adds six pointers
  per piece type to the picker.
- **Files:** `src/search_worker.h` (the make seam), `src/search_kernel_core.cpp`,
  `src/search_b5.h`, `src/search_params.h`, `src/move_picker_core.{h,cpp}`,
  `src/eval.{h,cpp}` for S1 only, `CMakeLists.txt` and `cmake/pgo-build.cmake`
  for C, `tools/`.
- **Invariants:** DESIGN §3's, untouched; the fingerprint identical on every
  tier and platform; no allocation in the search; `next()` under 4 KiB.
- **A false premise returns the item** with its evidence as a *Return* section
  here.
- **Non-goals:** the board, the evaluation's terms, the table's layout or
  replacement, any history's size or index, any pruning or ordering rule, the
  PGO corpus, anything in §7.

## 11. Findings outside the leaf

- **The verdict query on a table hit.** 1,092,158 of the cohort's verdict
  queries follow a 0 read from the table, each a full evaluation the hit was
  meant to save (1.5% of evaluations). A verdict test that runs the
  recognisers alone would remove them; that is the evaluator's structure and
  Phase C's.
- **PLAN B.8.8's inputs name the imbalance cache as open.** It is built.
- **`profile.ps1` reports the last `info` line's nodes**, a quarter below the
  search's count on a node-limited cohort. Any NPS derived from a profile's
  report would be low by that much; none is recorded.

## Return 1 — A1 gains on the game's workload and loses on `bench` (B.8.9, 2026-11-15)

- **What was built.** A1 exactly as §6 states it (`8440f8b`, reverted by the next
  commit): the predicted key, both prefetches, the null move's exact key, the
  kernel's three post-make prefetches removed, two Diag counters, the node check,
  and a kernel pin over the cohort's legal moves. Exactness held everywhere and the
  counters reproduce §1.2's 98.51% and 97.66% to the move.
- **Evidence (BAS-P38).**

  | Read | Delta | 95% interval |
  |---|---:|---|
  | pooled `bench 13`, alone, Hash 16 | −0.62% | [−0.83%, −0.41%] |
  | game cohort, one engine, Hash 64 | +0.71% | [+0.52%, +0.90%] |
  | game cohort, 14 engines, Hash 64 | +1.38% | [+1.00%, +1.76%] |

- **Which premise failed.** §8's bench prediction (+0.8% [+0.2, +1.6]) and §4.1's
  premise that the pooled `bench` read is a fair no-regression guard for a memory
  item. On `bench` the item costs more than it saves: the key's computation and its
  table loads are paid at every one of 1.6 makes a node, and at Hash 16 the cluster
  is mostly in the L3 cache, so the lead buys little. On the cohort the same binary
  gains alone, so the sign is the workload's (the table's size and the positions),
  not the load's. H1's falsifier did not fire (+1.38% is above +1.0%), but the
  loaded gain is under half of §8's +3.5%.
- **Disposition by the frozen rule:** reverted. Nothing was rescued or reshaped.
- **Options I see** (the research's to decide):
  1. Keep the rule and the result: A1 stays out.
  2. Change the guard for memory items to the cohort read at one engine (here
     +0.71%), which is the held-out game workload §3 names; A1 would be kept.
  3. A cheaper A1: keep the post-make prefetch at the negamax sites and add one in
     quiescence's two loops only (the half of §1.1's wait that has no prefetch);
     no predicted key, no cost at the 1.5 interior makes a node.
  4. Hash 64 for the pooled read's `bench`, so the guard sees a table out of L3.
- **Interaction for the later items.** A2 and A3 are read on the head without A1,
  as §6's order reads "alone on the head with what A1 kept". A2's and A3's
  pooled guards meet the same workload question.
- **Added after BAS-P40.** A second read of A1 against the head's own pool (a read
  that was not owed; BAS-P40) gave −0.43% [−0.56, −0.30] on `bench`, which passes
  the no-regression guard, with +1.54% [+1.11, +1.97] under load and +0.81% alone
  on the cohort. A1's verdict is the first read's; but the two reads straddle the
  guard's −0.5%, so option 2 or 4 above is a question of the rule's design, not of
  this item alone.

## 12. Implementation record (B.8.9, 2026-11-15)

Every item was read alone on the head before it, in §10's order, on the idle host;
before each read `bench 13` was 4,688,537 on Release, Tune and Diag builds and the
symbol PGO build, the cohort fingerprint 24,421,195 `8bc7f3c8d1dde0d2` on each (and on
the picker check's build for B3 to S2), CTest 33/33 on Release and Diag (the
allocation guard among them), and `hot_path_calls.py` reported nothing unknown. Each
read has its BAS-P entry; the bench reads are `nps_read.ps1`'s standing method, the
loaded reads BAS-P37's instrument.

**The instrument.** `tools/cohort_read.py` (`064dac9`, `c845994`): the loaded read,
its one-engine form and `fingerprint`. Its null read −0.13% [−0.35, +0.10] (half-width
0.23%) and its liveness, large pages off against on, +2.38% [+1.89, +2.87]: admitted
(BAS-P37). One change from §4.1 before any registered read: a reading's time is the
engine's own CPU time over its counted searches, because the harness's reply latency
ran 4.7% over it on average and up to 20%.

| Item | Predicted (§8, 80%) | Bench (95%) | Loaded (95%) | Kept | Commit |
|---|---|---|---|---|---|
| A1 | +0.8% [+0.2, +1.6]; loaded +3.5% [+1.5, +6.0] | −0.62% [−0.83, −0.41], no-regression FAIL | +1.38% [+1.00, +1.76] | reverted (Return 1) | `8440f8b`, `2bc4d2b` |
| A2 | +0.3% [0.0, +0.7]; +0.6% [0.0, +1.5] | −0.93% [−1.07, −0.79], FAIL | −0.87% [−1.23, −0.52] | reverted | `887289d`, `cfc543a` |
| A3 | +0.2% [−0.2, +0.6]; +0.5% [−0.2, +1.3] | −0.31% [−0.64, +0.01] | −0.44% [−1.55, +0.68] | reverted | `7699164`, `ef1deef` |
| A4 | no row | −0.15% [−0.44, +0.14] | −0.16% [−0.44, +0.12] | reverted | `004b6f4`, `5e68524` |
| C, full LTO | best arm +0.8% [−0.5, +2.5] | +3.23% [+2.94, +3.52] | cohort alone +2.56% [+2.29, +2.84] | **kept** | `a662fd9`, `4612bbf` |
| C, IR PGO | | −0.70% [−1.03, −0.37] | cohort alone −0.68% | not kept | — |
| C, both | | +2.53% [+2.26, +2.80] | cohort alone +1.93% | not the winner | — |
| C, inline 500 and ext-tsp | — | +0.92% [+0.63, +1.21] | cohort alone +0.66% [+0.35, +0.98] | **kept** | `354fef4` |
| B1 | +0.5% [+0.1, +1.0] | +0.23% [−0.01, +0.46] | — | under the bar, then out with the bundle | `1a4549a`, `33591d7` |
| B2 | +0.5% [0.0, +1.2] | +0.94% [+0.49, +1.39] | — | **kept** | `6172370` |
| B3 | +0.4% [0.0, +0.8] | +0.81% [+0.60, +1.02] | — | **kept** | `33ba08e` (check), `1e82067` |
| B4 | +0.8% [+0.2, +1.5] | −1.57% [−1.95, −1.19] | — | reverted | `fc45d32`, `396278a` |
| B5 | +0.2% [−0.3, +0.7] | −1.41% [−1.60, −1.22] | — | reverted | `671c79e`, `321eeab` |
| S1 | S1 and S2 +0.15% [0.0, +0.4] | −0.28% [−0.62, +0.07] | — | reverted | `6f573fa`, `67ca790` |
| S2 | (with S1) | +0.09% [−0.16, +0.34] | — | under the bar, then out with the bundle | `19e337c`, `8f31cbb` |
| B1 + S2 (D-17) | — | +0.05% [−0.11, +0.21], six cycles, eight builds | — | both out | BAS-P52 |
| **The pass** | +3.5% [+1.5, +6.0]; loaded +6% [+3, +10] | **+6.05% [+5.55, +6.55]** | **+4.66% [+4.49, +4.83]** | | head `d17a62d` |

The one-engine cohort reads of the memory items, reported: A1 +0.71%, A2 −1.09%,
A3 −0.33%, A4 −0.02%. The kept items' product, 3.23, 0.92, 0.94 and 0.81 percent,
is +6.0%, the pass's read.

**The falsifiers (§8).**
- A1 under +1.0% loaded with the instrument admitted: did not fire (+1.38%, and
  +1.54% in BAS-P40's unowed second read). The table's wait is lead time, but half of
  what §1.1's model gave, and the item's `bench` cost decided it.
- B3 and B4 both under +0.2%: did not fire (B3 +0.81%). H3 holds for the dispatch;
  the sort's rank pass lost 1.6%, so H3 does not hold for the sort, and B5 lost too.
- The pass under +1.5% on `bench` with A1 kept: cannot fire, A1 was not kept; the
  pass read +6.05%.

**What the pass says about H1 to H3.** The memory items lost on `bench` (every one),
and only A1 gained under load; the gains came from the build (+4.2% together) and
from removing work in the search's code (B2, B3). H2 held: A1's loaded read was about
twice its cohort-alone read. The packet's ranking put the memory items first; the
reads put them last.

**Deviations from the handoff.**
- §4.1's reading time is the engine's CPU time (above, BAS-P37).
- BAS-P40, about 80 minutes: a pool of the head itself, built because A2's offset
  looked like a wrong base; the base was valid, so its re-reads of A1 and A2 were not
  owed. Their verdicts stayed the first reads'; A1's second read would have passed its
  guard (Return 1's added note).
- Two disturbed step-1 reads repeated, as the method says (B1, B4).
- B4's first build spilled its prefix copy into a `memcpy` the hot-path scan refused
  before any read; the read version permutes in place.
- B5's row pointers are locals of the scoring pass, not picker members (one reader).
- D-17's six cycles needed `nps_read.ps1 -Extend -Force` (`239fcff`).
- `hot_path_calls.py`'s KNOWN list gained the null move's undo-stack path, which full
  LTO inlines (`1255e75`), in its own commit.
- The C arms that were not kept, the second read's arm and the bundle's base are
  throwaway revisions, cited by their recipes in BAS-P43, BAS-P44 and BAS-P52.
- The picker check (`VERIFY_PICKER`) left with the pass (`d17a62d`), as §6 says.
- The closing one-engine cohort read was not run; §10's closing reads name `bench`
  and the load only. BAS-P53's registration was committed during its first read's
  warm-up, after the final pool was built and before the first counted cycle.

**Owed elsewhere.** The universal binary builds with full LTO and its tiers reproduce
4,688,537 here; `check_universal.py` under Intel SDE needs SDE and runs in the release
workflow. CI and the release take the preset's default, so they build with full LTO
and the two link options with no flag change.

## Amendment 1 — the guard for a memory item (research, 2026-11-16)

Answers Return 1. No engine code changed and no read was run; every figure is a
recorded one, recomputed from the archives where named. B.8.9 stays closed and
BAS-P38's verdict stands.

**Recommendation.** Replace the pooled `bench` guard for a memory item with the
cohort read at one engine, and read A1 again on the head in a small leaf: `bench`
prices such an item's instructions and cannot see its waits, and A1 gained on the
game's workload in both sessions, alone and under load.

### Am1.1 What the records say

| Read of A1 | BAS-P38 | BAS-P40 | Builds |
|---|---|---|---|
| pooled `bench`, Hash 16 | −0.62% [−0.83, −0.41] | −0.43% [−0.56, −0.30] | every A1 build below every base build, both sessions |
| cohort, one engine, Hash 64 | +0.71% [+0.52, +0.90] | +0.81% [+0.46, +1.16] | every A1 build above every base build, both sessions |
| cohort, 14 engines, Hash 64 | +1.38% [+1.00, +1.76] | +1.54% [+1.11, +1.97] | the same |

About −0.5% on `bench`, +0.75% alone on the cohort, +1.45% under load. The two
sessions share A1's pool, so they are two reads of one set of binaries, not two
samples of the change.

### Am1.2 Hypotheses, and what each predicts

- **Noise at the threshold.** Predicts that one pair of pools read twice differs by
  about what the two verdicts differ.
  - Evidence: BAS-P40's session also holds `b84-on`, BAS-P38's base. From its
    per-build medians the same pair, `b84-on` against `b89-a1`, reads −0.47% there
    and −0.62% in BAS-P38. A1's own four binaries read 0.31% apart between the
    sessions and the base's 0.15%.
  - So a delta moves about 0.15% between sessions, which a session's interval
    (taken over its builds) does not hold. A1's cost on `bench` is about −0.5%,
    and the line is at −0.5%.
  - **Held.** The two verdicts are one measurement on its threshold. No threshold
    is stable for an effect that sits on it; moving the line would move the coin,
    not remove it.
- **Workload: the table's reach.** `bench`'s table is 16 MiB, inside one CCD's
  32 MiB L3, and its first load is 2.23% of `bench`'s samples against 6.56% of the
  cohort's (§1.1). Predicts that a wait-shortening item reads lowest on `bench`,
  higher alone on the cohort, highest under load, and that an item which only
  removes instructions reads the other way.
  - Evidence, memory items: A1 −0.5%, +0.75%, +1.45%; large pages +0.91% on `bench`
    and +2.38% under load (BAS-P35, BAS-P37).
  - Evidence, code items: full LTO +3.23% on `bench` and +2.56% alone on the
    cohort; the inline threshold and ext-tsp +0.92% and +0.66%; the pass +6.05% and
    +4.66% under load. IR PGO, which is neither, −0.70% and −0.68%.
  - **Held, in both directions.**
- **Cost.** The predicted key is computed at every one of 1.6 makes a node,
  whatever the probe then waits. This is the same model as the workload's: a read
  is the wait saved on that workload less the key's cost. `bench`'s −0.5% is the
  cost less whatever a third of the wait pays back, so the cost alone is −0.5% or
  worse. **Held; the split is not measured.**
- **Positions.** The cohort's endings, not its table, turn the sign. No record
  separates it: `bench` differs from the cohort in the table's size, the positions,
  the depth and the table's history at once. **Open.**

### Am1.3 Is a test needed before deciding?

No. The guard's question is whether an item kept by the loaded read slows the
engine a user runs alone. That is answered by changing one thing from the loaded
read, the load, and `bench` changes four. This holds whether the table or the
positions turn A1's sign, so no outcome of a table-size read would change the
guard. The split between cost and payoff decides A1's form, not the rule; it is
read in the leaf that consumes it, on that leaf's own pools (§Am1.6).

### Am1.4 Decision: the guard

A memory item (a prefetch, an entry's size, a probe saved) is kept when its pooled
`bench` read accepts it by the standing rule, or when all three hold:

1. **The decider, unchanged.** The loaded cohort read is +1.0% or more with its
   lower bound above 0.
2. **The guard, replaced.** The cohort read at one engine (`cohort_read.py
   --engines 1`, Hash 64, six cycles) shows no resolved loss: its estimate is
   above −0.5% and its interval's upper bound is at 0 or above.
3. **The cost's ceiling, new.** The pooled `bench` read's estimate is above −1.0%.
   Above that it is recorded as the item's cost and decides nothing.

The one-engine form guards only once it has passed a null by BAS-P37's admission
rule (`b84-on` against `b84-on-null`, both built). Its record so far is BAS-P40's
level pair, −0.02% [−0.41, +0.37].

Why this is not a way to keep items that cost the engine speed:

- **Two reads must agree on the game's workload**, one alone and one loaded. An
  item that gains only because fourteen engines contend fails the guard. A4, at
  −0.02% alone, passes the guard and fails the decider: neither read keeps an item
  by itself.
- **The guard is stricter where it applies.** The old form passed any loss short
  of −0.5% with its upper bound under 0; this one refuses any resolved loss,
  however small.
- **The workload is held out.** The build trains on `bench 13`; neither cohort read
  is trained on.
- **`bench` is still read.** Its estimate is on the item's record, and a cost of
  1% of a compute-bound node refuses the item whatever it saves in waits.
- **No deployment runs `bench`'s table.** The option's default is 64 MiB and the
  gates use 64.
- **Applied to B.8.9's items as a check, not a re-score:** A2 (−1.09% loaded), A3
  (−0.44%) and A4 (−0.16%) fail the decider as they did; A1 passes in both
  sessions. Only the item whose two verdicts disagreed changes.

The options of Return 1:

1. *Keep the rule.* Refused: under it no memory item with a cost in instructions
   of 0.5% can be kept whatever it saves, and D.2's table work and the network's
   prefetches inherit it.
2. *The one-engine cohort read as the guard.* Adopted, in the form above.
3. *A prefetch after the make at quiescence's children only.* Not adopted as the
   answer: after the make the lead is only the child's entry tests, where
   `negamax`'s prefetched probes still wait 21 ns behind a longer lead (§1.1). It
   is one arm of the leaf below, since it costs two lines.
4. *`bench` at Hash 64.* Refused: `bench`'s table is fixed in `bench.cpp`, so it
   is an engine change; it would still be the workload the build trains on; and
   the one-engine cohort read is that guard already.

### Am1.5 What this amends

- **Predictions (substantive).** §8's A1 row is superseded for any new read by
  §Am1.6's table. Its calibration: `bench` predicted +0.8% and read −0.5%, because
  §1.1's model had no cost term for the key; loaded predicted +3.5% and read
  +1.45%, the model's lead worth 2.4 times what the reads show.
- **§4.1's rule, old:** "kept when the pooled `bench` read accepts it by the
  standing rule; or when the loaded read is +1.0% or more with its lower bound
  above 0 and the pooled read passes the no-regression form."
- **New:** §Am1.4.
- **Proposed for PROCESS *NPS*, for the maintainer to approve:** "A memory item is
  kept by the loaded cohort read (+1.0% or more, lower bound above 0) when the same
  read at one engine shows no resolved loss (estimate above −0.5%, upper bound at 0
  or above) and the pooled `bench` read is above −1.0%. `bench`'s 16 MiB table sits
  in the cache: it prices such an item's instructions and not its waits (BAS-P38,
  BAS-P40)."

### Am1.6 A1 again: a leaf proposed, not added

**B.8.10 `[I1]` The table prefetch across the make, read on the game** — after
B.8.9, before B.9.1, so that B.11.1's checkpoint and every later gate run on it.

- **Why now and not Phase H.** The gain is read twice, about +1.45% in the gates'
  conditions; `probe_copy` is the head's largest single wait, 7.16% of the cohort's
  samples (BAS-P53); the code exists (`8440f8b`); no game is owed. If the maintainer holds it to the
  small-gains rule it goes to H.2 unchanged.
- **Arms, each alone on the head, against `b89-final`** (the head's release code is
  `d17a62d`'s):
  - X0: A1 as §6 states it, `8440f8b`'s engine change applied again.
  - X2: the predicted key and its corrected prefetch at quiescence's two loops
    only; `negamax`'s three prefetches after the make stay. The key is then paid
    at 0.13 to 0.17 makes a node, not 1.6.
  - X1: Return 1's option 3, a prefetch after the make in quiescence's two loops.
- **Exactness, per arm:** §4.3, and A1's pin and counters for X0 and X2.
- **Reads, in order, each once:**
  1. The one-engine form's null (`b84-on`, `b84-on-null`); admission as BAS-P37's.
  2. One pooled `bench` read of the four pools: the cost, and the ceiling.
  3. One one-engine cohort read of the four pools: the guard.
  4. Three loaded reads, one per arm: the decider.
  5. X0 against the base at one engine, Hash 1 and Hash 256: reported, and the
     model's falsifier.
- **Keep rule.** By §Am1.4. Of the arms that pass, the best loaded estimate is kept;
  a smaller arm (X1, then X2) is kept instead when its loaded estimate lies inside
  the best one's interval.
- **Prediction, frozen (80% intervals):**

  | Arm | `bench` | One engine | Loaded | P(kept) |
  |---|---|---|---|---:|
  | X0 | −0.5% [−0.9, −0.2] | +0.8% [+0.4, +1.2] | +1.5% [+1.0, +2.1] | 0.80 |
  | X2 | −0.1% [−0.3, +0.1] | +0.4% [0.0, +0.9] | +0.8% [+0.2, +1.5] | 0.35 |
  | X1 | 0.0% [−0.2, +0.2] | +0.2% [−0.1, +0.5] | +0.3% [−0.1, +0.9] | 0.10 |
  | X0 at Hash 1, one engine | | −0.7% [−1.2, −0.3] | | |
  | X0 at Hash 256, one engine | | +0.9% [+0.5, +1.4] | | |

- **Falsifiers.**
  - X0's one-engine delta at Hash 1 is not below its delta at Hash 64: the gain
    does not grow with the table, so it is not the probe's wait. Nothing is kept,
    and the leaf returns here as Return 2.
  - X0's loaded read under +1.0% on the head: the faster node left no lead to buy.
    A1 closes; retry with D.2's table.
- **Stop rule.** One registered read each; a second only after a disturbed one.
- **Cost.** Twelve PGO builds and the reads, about two hours of idle host by
  B.8.9's pace; no games.
- **Class.** `I1`: the change is written and its contract fixed; the leaf's work is
  builds and reads by committed tools under frozen rules.
- **Non-goals.** The table's layout or size, any other prefetch, `bench`'s table,
  the PGO corpus.

### Am1.7 Verdict

- **The guard:** decided as §Am1.4; it takes effect, with PROCESS's line, on the
  maintainer's approval.
- **A1:** `READY_FOR_IMPLEMENTATION` as the proposed B.8.10, on that approval. Until
  then `NO_CHANGE`: the head is B.8.9's.
- **Approved by the maintainer, 2026-11-16,** who left the choice to the
  research: both. The rule is in PROCESS *NPS* and the leaf is PLAN B.8.10.

### Am1.8 Found on the way

- **The pooled `bench` read over-reads a code item against the game by about a
  quarter** (+3.23% against +2.56%, +0.92% against +0.66%, the pass +6.05% against
  +4.66% loaded): a faster instruction is a larger share of a node that waits
  less. The standing rule is unchanged by this; a pass's closing reads should hold
  the one-engine cohort read too, which B.8.9's did not.
