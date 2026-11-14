# B.7.2 — board review against Stockfish 19 and Triumviratus (2026-11-14)

**Status: draft.** The instruments are built and the components are read
against both donors. The decisions below are proposals, each with what
would confirm or refute it. They wait for the idle-host reads
(`python tools/run_b72_reads.py`): the microbenchmark beside Rarog's and
Reckless's boards, and the head's profile at `bench 13`. The frozen handoff
to B.7.3 is written after those reads, not before.

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
  Reckless, and on 361 for Rarog.** The legal exchange tree (B.7.1's
  `see_oracle.py`) gives 325, and so does its least-valuable-legal-attacker
  chain. Rarog's SEE disagrees with the tree on at least 36 game positions.
  That is Rarog's finding, reported to the maintainer.

**Static reads of the release engine** (`basilisk-b71-fix-pext-pgo.exe`, the
head's code):

- **Slider lookups are inlined.** No slider lookup is called out of line; the
  `bishop_attacks` symbol has no caller. A.17's 1.58% "function" was
  attribution, not a call, which settles the inventory's D-12 item.
- **No hot function probes its stack.** No search, picker, board or
  evaluation function calls `___chkstk_ms`, which settles TB-11 (B.8.5's
  item).

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

### 3.9 Repetition

- **Proposal: keep the scan on demand.** Its one measured share is 0.30%.
- **Cleanup:** `is_repetition` and `is_draw` start their scan at distance 2,
  which can never match. The loop starts at distance 4, which is neutral.

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

- **Rarog's SEE:** with the cross-engine values it holds SEE ≥ 0 on 36 more
  of the 927 game moves than the legal exchange tree does. Rarog's session
  can check it with the adapter.
- **Inventory row:** `analysis/b7_board_speed_inputs_2026-11-14.md` §8 still
  lists F1 as "for the maintainer". It was repaired and gated (BAS-S63)
  after the inventory was written.

## 6. To complete after the reads

1. Record the microbenchmark per column and the three-engine table, with
   the round spread.
2. Record the profile's board shares: `make_move`, `unmake_move`,
   `attackers_to`, `gives_check`, `see_ge`, `see_pins`, generation,
   `gen_quiet_checks`, `is_legal`.
3. Confirm or refuse each proposal by its stated condition.
4. Freeze the B.7.3 handoff: one commit per accepted component, the
   microbenchmark checksums and fingerprint unchanged, a TM-10 old-beside-new
   check per replaced component.
