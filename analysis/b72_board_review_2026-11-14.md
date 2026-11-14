# B.7.2 — board review against Stockfish 19 and Triumviratus (2026-11-14)

**Status: done; handoff frozen (§6).**

- **What it rests on:** each component was read against both donors. The
  idle-host reads of 2026-11-14 (`python tools/run_b72_reads.py`) decided
  each proposal by the condition stated before them: the microbenchmark
  beside Rarog's and Reckless's boards (§1.1), and the head's profile at
  `bench 13` (§1.2).
- **What B.7.3 gets:** six components to rebuild, each behaviour-neutral
  (§6). B.7.4 then prices them on pooled-PGO NPS.

Inputs: Stockfish 19 at `edb0d9db` (`position.cpp`, `position.h`,
`movegen.cpp`, `attacks.h`), the Triumviratus snapshot
(`docs/reference/triumviratus/source/`, mapped by a Sonnet read whose
citations are kept below), `analysis/b7_board_speed_inputs_2026-11-14.md`
(the donor inventory), and B.7.1's packet
(`analysis/b71_board_contract_2026-11-14.md`).

## 1. Instruments

| Instrument | What it reads |
|---|---|
| `tests/board_micro.cpp` (`board_micro_bench`, profile basilisk-board-micro-v1) | each primitive alone on 512 positions of BAS-S63's games: legal, capture, staged and quiet-check generation; make/unmake; SEE on captures (threshold 0) and on quiets (−75); `gives_check`; the check information Stockfish keeps per node; the table-move test, offered the node's own moves and another position's; perft through the staged picker and through the board's generator |
| `tests/data/board_micro_positions.fen` | frozen by `tools/diag/board_micro_positions.py` from BAS-S63's PGN (SHA-256 recorded); its oracle line holds python-chess's counts (legal 14,085, tactical 927, checking 657), which every harness must reproduce before it times |
| `tools/board_compare/` | the cross-engine-board-v2 contract (`CONTRACT.md`), Rarog's and Reckless's adapters as patches on pinned commits (Rarog `611307e`, Reckless `8d7d1dd`), and `build.py`, which rebuilds both from the patches |
| `tools/diag/board_micro_run.py` | runs pinned copies of any harnesses, alternating, one process at a time on one logical CPU, refuses a busy host, keeps every sample, checks the engine-independent counts agree |
| `tools/profile.ps1` | the head's Profile build (`basilisk-b72-head-prof-pext-profile-pgo.exe`, `9ed7043`, `bench 13` 4,247,965) |

Every column folds its results into a checksum. The checksum is the
column's semantic signature, so a behaviour-neutral rewrite in B.7.3 must
keep it (`board_micro_run.py` reports any change). The CTest
`board_micro_preflight` runs the preflight. `tests/board_performance.cpp`
(cross-engine-board-v1) is unchanged.

**How the comparison is made fair.** It follows Rarog's three earlier
comparisons (RAR-M20, M43, M44):

- **The same work, checked:** the same positions and work counts, and a
  preflight against python-chess's counts.
- **SEE on one value vector:** 100/300/300/500/900 for every engine. RAR-M29
  showed that comparing each engine's own values says nothing.
- **Reckless without its network:** its make runs with its null NNUE
  observer.
- **The same compiler and flags:** every arm on LLVM 23.1.5 (Clang 23.1.5
  and rustc 1.100), each engine's release profile, no PGO.
- **The same host conditions:** cyclic order, one CPU, a load gate.

Two limits carry over:

- Rarog's sessions could not resolve single-column gaps under about 5%.
- A board column is not search speed. Rarog's RAR-M44(c) bundle gained
  12.7% on legal generation and cost 0.55% of pooled NPS.

**What the harnesses found before any timing:**

- **Capture generation is not comparable.** Rarog and Reckless both yield
  885 of the 927 tactical moves: their quiet stage generates the 42
  non-capturing underpromotions. The column is timed and marked.
- **Reckless has no exact `gives_check`.** Its `is_direct_check` ignores
  discovered checks, so the column is left out for Reckless.
- **Rarog's `gives_check` column uses its full form.** The fast form, which
  takes a `CheckInfo`, is crate-private. Its make is the hinted make, as in
  its search.
- **SEE ≥ 0 holds on 325 of the 927 tactical moves for Basilisk and
  Reckless, and on 361 for Rarog and both Stockfishes.** The legal exchange
  tree (B.7.1's `see_oracle.py`) gives 325, and so does its
  least-valuable-legal-attacker chain.
  - The 36 are exactly the promotions and en-passant captures whose exchange
    loses.
  - Stockfish's `see_ge` passes every move that is not a normal move without
    an exchange, and Rarog follows it.
  - It is the donor's deliberate rule, not a defect. Basilisk's exact
    treatment of those moves is BAS-C09's departure.
  - It also makes Stockfish's SEE column cheaper on those moves.

**Static reads of the release engine** (`basilisk-b71-fix-pext-pgo.exe`, the
head's code):

- **Slider lookups are inlined.** No slider lookup is called out of line; the
  `bishop_attacks` symbol has no caller. A.17's 1.58% "function" was
  attribution, not a call, which settles the inventory's D-12 item.
- **No hot function probes its stack.** No search, picker, board or
  evaluation function calls `___chkstk_ms`, which settles TB-11 (B.8.5's
  item).

### 1.1 The microbenchmark, three engines

`tools/results/b72-micro/` (manifest and 15 raw outputs), 2026-11-14:

- **Conditions:** five alternating rounds, 15 samples of 200 ms per
  column, one logical CPU.
- **Host load:** 3.8% busy before, 0.3% after; no competing process.
- **Arms:**
  - Basilisk `board_micro_bench` at `e2e762a` (the board's source as at
    `9ed7043`);
  - Rarog `611307e`, `bin/rarog-board-v2.exe` SHA-256 `F8F115D9…`;
  - Reckless `8d7d1dd`, `53B1FEEA…`.

The table gives the median ns per operation over the five runs, with the
range between runs.

| Column | Basilisk | Rarog | Reckless | Rarog / Basilisk | Reckless / Basilisk |
|---|---:|---:|---:|---:|---:|
| `legal_gen` | 48.09 (2.8%) | 61.85 (0.8%) | 51.35 (0.5%) | 1.286 | 1.068 |
| `capture_gen` † | 17.68 (7.1%) | 19.81 (4.2%) | 16.31 (1.8%) | 1.121 | 0.923 |
| `make_unmake` | 17.34 (0.6%) | **14.66** (1.0%) | 40.37 (2.0%) | **0.846** | 2.327 |
| `see_captures` ‡ | 15.50 (1.9%) | 13.60 (2.9%) | 18.31 (3.0%) | 0.878 | 1.181 |
| `gives_check` § | 6.53 (0.6%) | 7.55 (0.4%) | — | 1.155 | — |
| `board_perft` | 3.34 (2.8%) | 4.18 (4.4%) | 5.39 (1.4%) | 1.252 | 1.613 |

† Rarog and Reckless generate 885 of the 927 tactical moves, so the
column is not comparable.

‡ Rarog's SEE passes promotions and en passant unexamined, as Stockfish's
does (§1).

§ Rarog's full form, not its search's fast form.

Basilisk-only columns:

| Column | Median ns/op | Range between runs |
|---|---:|---:|
| `staged_gen` | 56.91 | 2.9% |
| `quiet_check_gen` | **229.49** | 1.7% |
| `see_quiets` | 18.35 | 0.5% |
| `check_info` | 5.91 | 3.1% |
| `table_move_test` | 5.20 | 3.2% |
| `search_perft` | 21.24 | 0.6% |

Read against the ranges:

- **Basilisk leads where it has a lead:** legal generation by 7% to 29%,
  and perft by 25% to 61%.
- **Rarog's make/unmake is 15.4% faster**, against ranges of 1%. That is
  the one resolved gap against Basilisk, and §1.2 explains it.
- **Rarog's SEE column is not quite the same work:** like Stockfish's, it
  passes 36 of the moves without an exchange.
- **Quiet-check generation costs four times a full legal generation**
  (229 against 57 ns for the staged pair).

### 1.2 The profile, by board entry point

`tools/results/b72-profile/`:

- **Conditions:** the Profile build at `9ed7043`, `bench 13` × 5;
  9,185 samples, 67.4% with an inline chain, nothing unresolved.
- **Region shares:** board 24.0% [23.2%, 24.9%] (27.9% inline-inclusive),
  attacks 5.8%, ordering 23.2%, evaluation 20.4%, search 15.3%, TT 5.6%.
- **The map:** `search_b5.h` was added to `tools/diag/profile_regions.json`.
  The region tool had refused the profile for it, and it was re-attributed
  from the kept samples.

Each board sample was then charged to the outermost board frame of its
inline chain: the entry point the search called. Shares are of all
samples.

| Entry point | Share | Inside it |
|---|---:|---|
| `see_ge` | **6.13%** | own 2.22; `attackers_to` 1.21; `see_pins` 1.12; `see_departure_opens_line` 0.59; slider lookups 0.58 |
| generation, out of line (`gen_legal_impl`) | 3.28% | own 1.12; `is_attacked_by` (king targets, castling) 0.85; push 0.33; lookups 0.33 |
| `gen_legal_captures` | 3.12% | generator 1.51; lookups 0.61 |
| `make_move` | 3.48% | `move_piece` 1.20; own 1.19; `attackers_to` (the checkers) 0.39; `remove_piece` 0.35; lookups 0.25 |
| `gives_check` | **3.33%** | own 2.23; slider lookups 1.03 |
| `unmake_move` | 2.68% | own 1.07; `move_piece` 0.90; `put_piece` 0.35 |
| `is_legal` | 0.54% | |
| `is_draw` and `upcoming_repetition` | 0.81% | |
| `check_squares` | 0.38% | |

### 1.3 Both Stockfishes, and the cost per search node

The maintainer asked for the donor's board in both of its forms:

- **Stockfish 19** (`edb0d9db`): its `do_move` writes the network's
  dirty-piece, threat and pawn-pair records.
- **Classical Stockfish** (`9587eeeb`, tag `SF_classical`): its `do_move`
  keeps the evaluation's incremental piece-square score.

How the Stockfish arms were built:

- **Build:** each is built by its own Makefile with Basilisk's compiler
  (Clang 23.1.5), COMP=clang ARCH=x86-64-bmi2, no PGO. The adapters are
  patches in `tools/board_compare/`, rebuilt by `build.py`.
- **SEE values:** both take the contract's values by a constant swap under
  a bench-only define.
- **Make:** both are given `givesCheck` precomputed, as their searches
  pass it. Stockfish 19's TT and history prefetch pointers are null.
  Classical Stockfish's `do_move` always prefetches its TT entry, which
  cannot be switched off without changing its code.
- **Generation:** both generate pseudo-legal moves. Their `capture_gen` is
  pseudo-legal and unfiltered (1,150 and 1,104 moves against 927), so it is
  not comparable. Their legality test is paid at delivery, which no column
  times.

All five arms ran in one session, `tools/results/b72-micro5/`:

- **Conditions:** five alternating rounds, 15 samples of 200 ms per
  column, one logical CPU.
- **Host load:** 3.7% busy before, 1.6% after; no competing process.

**Median ns per operation, and each engine's time as a percentage of
Basilisk's** (above 100% is slower):

| Column | Basilisk | Stockfish 19 | Stockfish classical | Rarog | Reckless |
|---|---:|---:|---:|---:|---:|
| `legal_gen` | **48.1** | 76.6 (159%) | 89.4 (186%) | 62.0 (129%) | 51.1 (106%) |
| `make_unmake` | 17.3 | 52.7 (305%) | 35.5 (205%) | **14.7 (85%)** | 40.4 (234%) |
| `see_captures` | 15.3 | **6.6 (43%)** | 6.6 (43%) | 13.8 (91%) | 18.2 (119%) |
| `gives_check` | 6.5 | **3.2 (49%)** | 3.2 (49%) | 7.6 (116%) | — |
| `board_perft` (per leaf) | **3.35** | 6.19 (185%) | 5.14 (154%) | 4.18 (125%) | 5.41 (162%) |
| `capture_gen` (not comparable) | 17.4 | 10.2 | 13.4 | 19.8 | 16.3 |

The spread between rounds was 0.3–3.5% per column, except Basilisk's
`capture_gen` (5.9%) and classical Stockfish's SEE (4.7%).

**Calls per search node.** The search's mix was counted by a throwaway
build (counters at the entry of each board function, `bench 13`
unchanged at 4,247,965):

| Operation | Calls per node |
|---|---:|
| make, and the matching unmake | 0.92 |
| null move | 0.04 |
| capture-stage generation | 0.53 |
| quiet-stage generation | 0.20 |
| SEE | 1.42 |
| `gives_check` | 1.44 |
| `check_squares` | 4.19 |
| repetition test | 1.00 |
| cuckoo repetition test | 0.50 |
| TT-move legality | 0.14 |
| full legal generation | 0.004 |
| quiet-check generation | **0** |

Quiet checks in quiescence are off at the defaults
(`b4_quiet_checks` 0), so quiet-check generation is never called.

**Board cost per search node under Basilisk's call mix.** Each engine's
time per operation is weighted by these rates:

    0.92 make_unmake + 0.53 capture_gen + 0.20 legal_gen + 1.42 see + 1.44 gives_check

`legal_gen` stands in for the quiet stage, an overestimate for every arm.

| | Basilisk | Stockfish 19 | Stockfish classical | Rarog | Reckless |
|---|---:|---:|---:|---:|---:|
| ns per node | **65.8** | 83.2 | 71.7 | 67.0 | — |
| against Basilisk | 100% | 126% | 109% | 102% | — |
| ns per node, without `gives_check` | 56.4 | 78.6 | 67.1 | **56.1** | 82.0 |
| against Basilisk | 100% | 139% | 119% | 99% | 145% |

How to read it:

- **On the work Basilisk's search does, its board is the fastest of the
  five.** Rarog is within 2%, inside the composite's resolution.
- **Classical Stockfish costs 9% more and Stockfish 19 26% more.**
  Stockfish 19's make pays for the network's records.
- **Stockfish's design moves cost from the queries into make.**
  `set_check_info` stores blockers, pinners and check squares at every
  make, so `gives_check` and SEE are half Basilisk's cost, and make is two
  to three times dearer.
  - At Basilisk's mix, about 0.9 makes against 2.9 queries per node, the
    trade loses: Basilisk's composite is 9% below classical Stockfish's.
  - That is BAS-P04's −1.8% to −2.7% seen from the other side, and it
    confirms keeping the lazy check information.
- **What remains to take from Stockfish is the queries' cheapness without
  its make cost.** That is C2 and C3: a bit test before the discovered-check
  lookups, and a pin pre-test before `see_pins`. Stockfish's SEE gap also
  includes the 36 promotions and en-passant moves it does not examine.
- **The limit, as before:** these are primitives on real positions, not
  search speed. The composite favours the pseudo-legal generators, whose
  legality test at delivery is not timed. Pooled NPS (B.7.4) decides any
  change to Basilisk.

## 2. Scope

PLAN moved the board-speed items that are not the board's to B.8.5 on
2026-11-05. The inventory ranks two of them highest, and they stay there:

- the child TT prefetch before make (TB-12, D-5);
- the picker's second pass over the generated list (D-10, TB-8's picker
  half).

They are not proposed here. The board half of TB-8 (the generator) is
B.7.2's.

## 3. Components

Each component gives the three forms, the evidence and a proposal. "Neutral"
means the bench fingerprint and the microbenchmark checksums are unchanged.

### 3.1 Check information (TB-4)

- **Stockfish:** `set_check_info` runs at every `do_move`. It fills both
  colours' blockers and pinners and the five check-square sets into
  `StateInfo`. `gives_check` and `legal` then read them.
- **Triumviratus:** checkers and pins are computed once per node for the
  legal generator (`movegen_legal.h:244-281`, cached in `MoveQueue`). The
  check squares are built lazily at the first move (`07_makemove.inc:68-112`).
  The child's in-check status is the parent's `gives` (`13_search.inc:1120-1124`).
- **Basilisk:** the picker caches check squares lazily per type
  (`CoreMovePicker::checks_for`). Generation computes pins per call. Every
  `make_move` computes the checkers with `attackers_to(king)`, two slider
  lookups (`board.cpp:741`).
- **Evidence:** BAS-P04's eager per-node `CheckInfo` plus a hinted make read
  −1.8% to −2.7%. Pin sharing between stages read −0.16%. The search calls
  `gives_check` 1.43 times per node.
- **Proposal: keep the lazy form; measure one part alone.** That part is the
  child's checkers from the parent's `gives_check`, which the search already
  computes for most moves it makes. It is the half of BAS-P04 that was never
  read alone, and BAS-P04's retry trigger is a profile showing changed reuse.
  - The profile's share of `make_move`'s `attackers_to` decides whether it
    is a candidate. Below 0.5% of samples, it is refused.
  - It would be neutral: checkers would be identical.
- **Verdict: accepted as component C6, measured alone.** Make's checkers
  computation is 0.39% of samples, and 0.39–0.64% with its lookups.
  - Its result duplicates `gives_check`, which the search computes before
    the make for most moves (3.33% of samples).
  - That is the changed reuse BAS-P04's trigger asks for: the hint adds no
    computation, where P04's eager `CheckInfo` did.
  - Where the search has not computed `gives_check`, make computes the
    checkers as now.
  - It also accounts for most of Rarog's 15% `make_unmake` lead (Rarog's
    make is hinted).

### 3.2 Make and unmake (TB-5, TB-10)

- **Stockfish:**
  - `do_move` computes the new keys in a local and in `StateInfo`.
    `put_piece`, `remove_piece` and `move_piece` touch only the board
    arrays, never a key.
  - `undo_move` moves the pieces back and restores the state by pointer; it
    does no key work.
- **Triumviratus:** make copies a 64-byte block of keys, rule-50 and
  occupancies (`03_tables.inc:312-314`, `07_makemove.inc:320-321`). The
  partial keys go through a branch-free routing table
  (`07_makemove.inc:261-290`). Unmake restores the keys and occupancies by
  `memcpy`.
- **Basilisk:**
  - `put_piece`, `remove_piece` and `move_piece` update the main key and
    the pawn, minor and non-pawn keys, with three type tests each
    (`board.cpp:17-68`).
  - Unmake runs the same helpers, which XOR every key again, and then
    overwrites the keys from `UndoInfo` (`board.cpp:793-800`). Each unmake
    computes four to eight key updates it discards.
- **Evidence:** Triumviratus's unmake alone read −0.61% cycles per node, and
  the whole package −1.33%.
- **Proposal: adapt.** Split the helpers into a board-only form and a keyed
  form. Unmake uses the board-only form and restores the keys by copy.
  - It is neutral.
  - Expected: `make_unmake` faster on the microbenchmark (unmake is about
    half of the column), and 0.3–0.6% of search.
  - The falsifier is `make_unmake` not moving by more than its run spread.
- **Verdict: accepted as component C1.** The helpers inside unmake are
  1.25% of samples (`move_piece` 0.90, `put_piece` 0.35), and about half of
  their work is the discarded key updates.
- **Dirty-piece record (TB-10): fix the contract here, build it in F.1 with
  its consumer.**
  - The record holds the moved piece, from, to, the captured piece and its
    square (the en-passant pawn's), and the added piece and square
    (promotion, castling rook). Make writes it into the ply's `UndoInfo`;
    null moves write none.
  - The accumulator replays pending records lazily before an evaluation,
    which is Triumviratus's variant (c): +0.60% / +1.13% instructions
    against +2.68% / +3.40% for the eager form.
  - Writing it now, with no reader, costs speed for nothing. B.7.3 leaves
    `UndoInfo` so the record slots in.

### 3.3 Legality and the table-move test

- **Stockfish:** pseudo-legal generation; `legal` reads the stored blockers;
  `pseudo_legal` validates table moves.
- **Triumviratus:** a legal generator (`movegen_legal.h`); the table-move
  test `td_is_pseudo_legal` (`10_order.inc:13-182`); no killer stage.
- **Rarog and Reckless:** both generate legal moves.
- **Basilisk:**
  - Generation is strictly legal (`gen_legal_impl`).
  - `is_legal` does the pseudo-legality and the king test in one function:
    two slider lookups for a non-king move, with no pin information.
  - It runs at most once per node, on the TT move.
- **Proposal: keep.** It is called less than once per node, and the
  microbenchmark prices it. Revisit only if `table_move_test` is above 2% of
  a node in the profile.
- **Verdict: kept.** `is_legal` is 0.54%.

### 3.4 `gives_check`

- **Stockfish:** the direct check is one bit test on the stored check
  squares. The discovered check is a test of the stored blockers, then
  `line_bb`. Only promotion, en passant and castling compute attacks.
- **Triumviratus:** `checks_before` reads lazily built check squares and a
  discovered-check set (`12_quiesce.inc:204-239`).
- **Basilisk:** the direct check is one lookup at the destination. The
  discovered check is always two slider lookups from the enemy king
  (`board.cpp:1471-1477`).
- **Proposal: adapt.** Pre-test the discovered check with one bit test
  before the two lookups: the origin must lie on a line from the enemy king
  that holds one of our sliders of that line's kind. It is the same kind of
  superset test as `see_departure_opens_line`.
  - It is neutral; B.7.1's differential (9,498 checks against play) guards
    it.
  - Expected: most calls skip both lookups, at 1.43 calls per node.
- **Verdict: accepted as component C2.** `gives_check` is 3.33%, and its
  slider lookups 1.03%. Basilisk's form is already 13% faster than Rarog's
  full form; the pre-test removes most of the lookups.

### 3.5 SEE (TB-6)

- **Stockfish:** the pins are the stored blockers of the original position.
  It is not exact when a pin opens during the exchange, which is the donor's
  intent.
- **Triumviratus:** the pins are stored once per position. The next
  attacker is found by index (`see.cpp:375-382`). Both bounds are tested
  before any lookup. A quiet move skips SEE when the enemy attack map
  decides it.
- **Basilisk:**
  - The pins are exact, computed on the exchange occupancy (BAS-C08/C09).
    It agrees with the exchange tree on every case of B.7.1's corpus and on
    the 927 game moves here.
  - `see_pins` costs eight slider lookups whenever an enemy attacker exists
    (`board.cpp:1663`).
- **Proposal: keep the exact model, and adapt its cost.**
  - Pre-test the pins: if no enemy slider stands on an empty-board line of
    either king, no pin exists and `see_pins` is skipped. It is neutral.
  - Next attacker by index: Triumviratus's −0.38% / −0.14% is small. It is
    a candidate only if `see_ge` is above 3% in the profile.
  - The quiet-SEE skip: priors conflict (Rarog's member read −0.58% inside
    its bundle), so it is deferred.
- **Verdict: the pin pre-test is accepted as component C3.** `see_pins` and
  its line test are 1.71% of samples, inside SEE's 6.13%.
  - The pre-test: an enemy slider on an empty-board line of a king. It
    needs the empty-board bishop and rook tables (TB-7), 1 KB.
  - Next attacker by index: SEE is above its 3% condition, but the prior is
    0.14–0.38%, under B.7.4's +0.5% floor alone. It is deferred to B.8.5's
    pass, where small gains are read together.

### 3.6 Attack tables (TB-7)

- **Stockfish 19:** on AVX2 it uses a dual hyperbola quintessence (one
  256-bit computation for bishop and rook together, a 64-byte rank table);
  without AVX2, magic tables. It has no PEXT path.
- **Triumviratus:** its default is a hyperbola-quintessence pair on AVX2
  (`sliders.h:106-155`), its PEXT tables are the reference, and it keeps
  empty-board tables.
- **Basilisk:** flat PEXT tables (840 KB) on the PEXT tier and baked magics
  below it, all inlined.
- **Proposal:** keep the flat PEXT tables on the PEXT tier (BAS-P26
  measured compression −0.98%). For the AVX2 tier only, the dual hyperbola
  quintessence is a candidate, read on an AVX2-tier build of the
  microbenchmark. Low priority: the release's fast hosts take the PEXT tier.
- **Verdict: kept;** the AVX2-tier read is deferred. Every lookup is
  inlined, and the attacks region is 5.8%. A rewrite for a tier the
  maintainer's hosts do not run is not worth a B.7.3 component now. It is
  recorded for G.2's platform work.

### 3.7 Generation (TB-8, archived 7.2–7.8)

- **Basilisk:**
  - `gen_legal_impl<Us>` is templated on colour, with capture and quiet
    selection as two runtime flags.
  - The staged pair (captures, then quiets) recomputes the king moves and
    pins.
  - `gen_quiet_checks` generates every legal move and then filters by
    `gives_check` (`board.cpp:1482`).
- **Proposals:**
  - **Quiet checks: replace** `gen_legal` by `gen_legal_quiets` inside
    `gen_quiet_checks`. The quiets come in the same relative order (one code
    path, masked), so it is neutral, and it skips the captures and
    promotions it threw away. Expected: `quiet_check_gen` much faster.
    Quiescence calls it at its first ply, after the captures, when quiet
    checks are enabled (`search_kernel_core.cpp:580`); the profile gives
    its weight.
  - **Stage as a template parameter:** a candidate measured alone, never
    bundled. Rarog's colour-templated, force-inlined bundle measured −0.55%.
  - **Pin sharing between the stages: refused.** It is BAS-P04's −0.16%.
- **Verdict: quiet checks withdrawn from the handoff.** They cost 229 ns
  per position against 57 for the staged pair, but the search never calls
  them: quiet checks are off at the defaults (§1.3's counts). The change
  waits for a leaf that turns quiet checks on. The stage template stays a
  candidate outside B.7.3.

### 3.8 Chess960 (TB-9)

- **Stockfish:** `castlingRookSquare`, `castlingPath` and
  `castlingRightsMask` are per-position tables.
- **Triumviratus:** `castle_rook_sq` and `castle_king_sq` tables, with an
  XOR occupancy update in which both origins are cleared before both
  destinations are filled.
- **Basilisk:** the castling squares are hard-coded (E, H and A files,
  `CASTLING_MASK`) in make, unmake, `gives_check`, `is_legal` and the
  generator.
- **Proposal:** B.7.3 replaces the hard-coded squares by tables filled at
  `set_fen`, still for standard chess alone. It is neutral, and the feature
  (X-FEN, king takes rook) is G.2's.
- **Verdict: accepted as component C7, held to cost-neutral rather than
  +0.5%.** It buys representability, not speed (§6).

### 3.9 Repetition

- **Proposal: keep the scan on demand.** Its one measured share is 0.30%.
- **Cleanup:** `is_repetition` and `is_draw` start their scan at distance 2,
  which can never match. The loop starts at distance 4, which is neutral.
- **Verdict: accepted as component C5,** as its own commit. It is too small
  to price (0.81% for both draw tests).

### 3.10 Representation

- **Proposal: keep** 12 piece bitboards, the occupancies, the mailbox,
  `king_sq` and the compact `UndoInfo` on a reserved vector.
- **Why:** Rarog rejected six type boards on counters, untimed (RAR-M39);
  its trigger is a single region above 12%.

## 4. Departures table (kept Basilisk forms)

| Component | Basilisk's form | Donor form | Why kept |
|---|---|---|---|
| Check information | lazy, per picker | eager in `StateInfo` (Stockfish) | BAS-P04 −1.8% to −2.7% |
| Legality | legal generation | pseudo-legal plus `legal` (Stockfish) | Triumviratus, Rarog and Reckless all generate legal moves |
| SEE pins | exact, on the exchange occupancy | stored blockers (Stockfish, Triumviratus) | exact against the tree; BAS-C08/C09 |
| Attacks (PEXT tier) | flat PEXT | dual hyperbola quintessence (Stockfish AVX2) | BAS-P26; the read in 3.6 for the AVX2 tier |
| Repetition | scan on demand | distance stored at every make (Stockfish) | 0.30% share; no make-time cost |
| Representation | 12 bitboards and a mailbox | type and colour boards (Stockfish) | RAR-M39's trigger not met |

## 5. Findings outside B.7.2

- **Rarog's SEE:** it passes promotions and en passant without an
  exchange, as Stockfish does (36 of the 927 game moves lose their
  exchange). It is a policy choice for Rarog's own board review, not a
  defect.
- **Inventory row:** `analysis/b7_board_speed_inputs_2026-11-14.md` §8 still
  lists F1 as "for the maintainer". It was repaired and gated (BAS-S63)
  after the inventory was written.

## 6. Handoff to B.7.3 (frozen 2026-11-14)

**Rules for every component:**

- **One commit each**, in this order.
- **Neutral:** `bench 13` stays 4,247,965, and every checksum of
  `board_micro_bench` stays as the reads recorded it:
  `tools/results/b72-micro/manifest.json`, `work`.
- **Tests pass:** B.7.1's corpus (`test_board_v2`) and B.6.3's search-path
  perft (`test_search_path`).
- **An old-beside-new check (TM-10):** each replaced form keeps, behind a
  build switch, a run of the old form beside the new at every call that
  aborts on the first difference. It is run once under `test_search_path`
  and `bench 13`.
- **Nothing rides along:** no search, evaluation or parameter change.
- **Each component's microbenchmark column** is read before and after, with
  `board_micro_run.py` alternating the two builds. A column that does not
  move by more than its range between runs is reported as unmoved.
  B.7.4's pooled read decides.

| # | Component | Change | Columns it should move | Prior |
|---|---|---|---|---|
| C1 | Unmake without key work | `put_piece`, `remove_piece` and `move_piece` gain a board-only form; unmake uses it and restores every key from `UndoInfo` | `make_unmake`, `board_perft`, `search_perft` | Triumviratus −0.61% cycles/node; 1.25% of samples in the helpers |
| C2 | `gives_check` discovered pre-test | the two slider lookups only when the origin is on a line from the enemy king that holds one of our sliders of that line's kind | `gives_check` | 1.03% of samples in its lookups |
| C3 | SEE pin pre-test | `see_pins` skipped for a colour with no enemy slider on its king's empty-board lines (new 1 KB tables) | `see_captures`, `see_quiets` | 1.71% of samples in pin work |
| C5 | Repetition scan from distance 4 | `is_repetition` and `is_draw` | none | cleanup |
| C6 | Hinted make | `make_move` takes the parent's `gives_check` when the search computed it: checkers are 0 when false, `attackers_to` when true, unchanged when unknown | `make_unmake` (hinted column added) | 0.39–0.64% of samples; Rarog's 15% |
| C7 | Castling squares from tables | rook origins and destinations, paths and rights masks filled at `set_fen`; standard chess only | none | representability (TB-9) |

**B.7.4's floor:**

- C1–C3 and C6 are each held to PLAN's +0.5% pooled-PGO floor. C2 and C3
  may not clear it alone. Their pooled read is still per component, and
  one that falls short returns to Basilisk's form.
- C5 and C7 are held to cost-neutral: no loss beyond the read's
  resolution.
- **Maintainer's decision: C7's floor.** PLAN asks +0.5% of every
  component, and C7 buys representability, not speed.

**Not handed over:**

- **To B.8.5:** next attacker by index, the stage template parameter, the
  quiet-SEE skip.
- **To G.2:** the AVX2 tier's dual hyperbola quintessence.
- **To F.1:** the dirty-piece record, with its consumer (§3.2 fixes its
  contract).
- **Refused:** eager check information, and pin sharing between stages
  (BAS-P04).
