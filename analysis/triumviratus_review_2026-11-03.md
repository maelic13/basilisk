# Triumviratus review, 2026-11-03: a third donor, read for mechanisms

- State / class: delivered; the source of the Triumviratus inputs added to
  PLAN on 2026-11-03 (B.3.3, B.4, B.5, B.6.1, B.7.2, the new B.15 and
  B.16, C.0, D.1–D.3, F.0–F.8, G.1, G.2 and the new Phase H), `R2`.
  Amended the same day at the maintainer's request: the instruments that
  help now became B.16 (§4.7), and Triumviratus's NNUE path was recorded
  for self-play training (§4.12).
- Owner / date: agent, 2026-11-03, at maintainer request ("take what is
  useful for us and place it in the proper place of the plan ... mechanisms,
  not code ... fine-tuning after we switch to NNUE and after all our bigger
  gains are exhausted ... register it as third donor")
- Decisions taken by the maintainer, 2026-11-03:
  - Triumviratus is Basilisk's third donor, for mechanisms only;
  - gains of about 5 Elo or less on Triumviratus's own measurement wait
    until after the NNUE switch and the bigger gains (the new Phase H);
  - a board review and rebuild, on Stockfish's board and Triumviratus's
    ideas, keeping Basilisk's form wherever it measures better.
- Placement in this packet is the agent's; the maintainer may move any item.

## 1. Sources, pins and how to read them

**Snapshots** (verbatim, manifest-checked by `check_roadmap.py`; see
`docs/reference/README.md`):

| Snapshot | Source | Holds |
|---|---|---|
| `docs/reference/triumviratus/` | `Tors3/Triumviratus` `main` at `0c18907` (2026-11-03), GPL-3.0 | README, `docs/` (NOVELTIES, HISTORY, CONTRIBUTING, NETWORKS, the 6.0, 7.0 and 8.0 development logs, the expert-blend record, the tests README), `source/` without Fathom and incbin, `build/` (its measurement and build tools), `.github/` |
| `docs/reference/triumviratus/removed/e876fa3/` | the parent of `5b36328`, which deleted them | the 8.1 development log and its block documents (ordering, reductions, pruning, extensions, core, time management, the clean-room inference specification, two code maps) |
| `docs/reference/triumviratus/removed/dfa5a6b/` | the parent of `9efeed2`, which deleted them | FUTURE_DIRECTIONS (the residual and regime studies), the experimental-grafts record, the 4.2–7.0 network record |
| `docs/reference/triumviratus-networks/` | `Tors3/Triumviratus-Networks` `main` at `5f3dc60` (2026-10-28) | every Markdown file (recipes 01–07, the Consilium Plebis run log, data list and design documents) and the trainer patches |

The full history of Triumviratus (every branch and tag) is in the ignored
bundle `tools/results/triumviratus-reference/triumviratus-0c18907-all-refs.bundle`,
SHA-256 `79523ab592280776754dc4ea939d387a986fa3a2a4443c3c9a55dd5920cd1ca4`.
Commit messages there carry the 8.1 measurements that no document holds; this
packet quotes the ones it uses, so it does not depend on the bundle.

**Short references used below:**

| Short | Path under `docs/reference/` |
|---|---|
| D6, D7, D8 §n | `triumviratus/docs/development/archive/DEVELOPMENT_6.0.md`, `_7.0.md`, `_8.0.md`, section n |
| D81 §n | `triumviratus/removed/e876fa3/DEVELOPMENT_8.1.md` |
| S1–S5 | `triumviratus/removed/e876fa3/docs/development/8.1/SEARCH_1_ORDERING.md` … `SEARCH_5_CORE.md` |
| NOV | `triumviratus/docs/project/NOVELTIES.md` |
| NET | `triumviratus/docs/networks/NETWORKS.md` |
| FD §n | `triumviratus/removed/dfa5a6b/docs/project/FUTURE_DIRECTIONS.md` |
| R07 | `triumviratus-networks/recipes/07_consilium-plebis/README.md` |
| commit `x` | a Triumviratus commit message, in the bundle |

**What Triumviratus is.** A GPL-3 C++ engine by Francesco Torsello, with
Maurizio Platino as tester and tuner, developed with heavy AI assistance
(Claude agents are named in its logs). It began from BBC in 2024; up to 8.0
it carried Stockfish's NNUE inference and, from 4 October 2026, a search
"on the algorithmic base of Stockfish 19"; 8.1 rewrote both in its own form.
On the CCRL 40/15 list of 2026-10-30, 8.0 is first at 4 CPUs (3650 ± 28,
214 games) and among single-CPU engines (3633 ± 21, 392 games). Its networks
are its own weights trained from random initialisation, but on **public
Leela and Stockfish data** re-labelled with Leela's BT4 (NET, *What
"own-lineage" means*), which Basilisk's own-data rule excludes.

**Why it is useful to Basilisk.** It is the nearest documented case to
Basilisk's own road: a Stockfish-19-shaped search rebuilt and refitted on a
different evaluation, every change SPRT-measured and recorded, negatives
included. More than a hundred search, time, speed and network ideas carry a
measured result on that base.

**Reading its evidence.** Its verdicts are priors, never acceptance:

- one test machine (two Xeon Gold 6138 sockets, one with half the memory
  bandwidth), self-play on UHO 2024, often with hyperthreading. Its own log
  found that hyperthreading cost 1.6 plies at 5 s and biased SPSA vectors
  (D8 §45.5), that NUMA placement hid a network's gain (D8 §9), and that
  sockets disagreed by up to 20 Elo on the same pair (D8 §29);
- many results are early-stopped SPRTs of 1,000–8,000 games. The project
  itself found that seven of thirteen tests peaked at +5 to +17 and ended
  between −2 and +2 (D8 §48.1), and that its five "leans" together read
  −0.1 ± 6.7 (D8 §18);
- its SPSA tool explored with half the intended perturbation until
  2026-10-13 (D8 §48.1), so every 8.0 tuned vector is rough;
- its novelty claims are "to our knowledge" by its own wording (NOV,
  *Method*);
- its search is tuned around an NNUE with experts at 8–40 s per game;
  Basilisk's is an HCE engine gated at `3+0.03`.

## 2. Triumviratus as a donor: what it is for

Stockfish stays the architecture donor and classical Stockfish the
evaluation donor. Triumviratus is read for:

1. **Measured priors on a Stockfish-19-shaped search**, positive and
   negative, before Basilisk spends games on the same idea;
2. **board and infrastructure mechanisms** with identical-tree speed
   measurements (B.15, B.7.2);
3. **time-management rules and their measurement method** (D.1, Phase H);
4. **NNUE architecture, inference and training lessons** for Phase F;
5. **measurement and tuning failure modes** it found on itself.

Its documentation is better than Stockfish's in one respect: it says
*why* and what each change measured, in one narrative, negatives included.
Stockfish's evidence is richer but scattered across fishtest links in
commit messages. Triumviratus's records are less reliable (small samples,
stopped tests, one machine), so they do not replace Stockfish as the
architecture donor.

## 3. Checked on Basilisk: already present or not applicable

Each was read in Basilisk's source on 2026-11-03 (branch
`b3-pruning-extensions`, `bench 13` 11,880,564).

| Triumviratus finding | Its cost there | Basilisk |
|---|---|---|
| Zobrist keys from a 32-bit generator had GF(2) rank 32: false hash hits, a lost CCRL game (commit `24d791c`, +4.57 ± 3.56 for the fix) | blunder | splitmix64 keys (`src/zobrist.cpp`); the 921 keys have GF(2) rank **64** (computed 2026-11-03). A permanent test belongs in B.15.1 |
| Cuckoo repetition test without Stockfish 19's filter (the opponent's key changes must cancel), +5.62 ± 4.47 for the fix (D8 §41) | false draws | present: `Board::upcoming_repetition` folds `other ^= key(i-1) ^ key(i) ^ SideKey` (`src/board.cpp`) |
| Unbuffered stdout and a 15.6 ms `GetTickCount` clock, +29.8 ± 7.4 at 5+0.05 for the fix (D8 §45) | ms per move | `steady_clock`, `sync_with_stdio(false)` (`src/engine.cpp`, `src/engine_entry.cpp`); a wall-clock instrument still belongs in D.1 (TT-2) |
| Threads created per `go`; ponder wait in 1 ms sleeps (D8 §45.5) | ms per move | persistent pool on condition variables (`src/search_thread_pool.h`) |
| `setoption Hash` or `EvalFile` re-sent each game reallocated the table (D8 §29) | 0.22 ply on one side | `apply_table_state` resizes only on a changed size |
| Null move advanced the fifty-move counter (D8 §25) | consistency | `make_null` leaves `halfmove_clock` alone |
| Quiescence TT entries at depth −1 instead of the donor's 0, +3.38 ± 2.58 over 20,162 for the fix (D8 §25) | small | quiescence stores depth 0 (`search_kernel_core.cpp`) |
| Pinned pieces recapturing in SEE (+4.5 ± 6.8); promotion gain missing in SEE (+1.78 ± 1.76) (D8 §25, D81 §2) | small | `see_pins` filters pinned attackers; promotions handled; the residual geometries are BAS-C09's documented approximation |
| Insufficient material after a capture (+1.13 ± 1.56 over 17,162, D8 §42) | small | `is_insufficient_material` in `is_draw` |
| Tuning parameters read as globals in release (+5.0% NPS once frozen, D7 §3) | NPS | release builds fold the parameter struct to constants (`src/search_params.h`) |
| Legal generator with pins and check mask (D81 §1) | NPS | present (`Board::gen_legal`); the per-node sharing of pins and checkers across picker stages is a B.15 question |

## 4. Findings, numbered, with their destination

Each row: the mechanism, Triumviratus's evidence, Basilisk's state where it
was read, and the leaf that owns it. **Small** marks a measured gain of
about 5 Elo or less on Triumviratus, which the maintainer's rule sends to
Phase H.

### 4.1 Search (TS)

| ID | Mechanism | Evidence | Basilisk / destination |
|---|---|---|---|
| TS-1 | **Cap on extensions from a negative reduction, counted per line** (`NegExtMax=1`). A reduced search allowed past the nominal depth chains along a line; capping the count at one per line | −6.6% nodes and −0.55 plies of selective depth at fixed depth on 60 positions; +3.88 ± 3.26 over 11,384 at 12+0.12, baked (D8 §33, §42). Stockfish addressed the same growth after version 19 with a ply limit relative to root depth ("Fix deep recursion") | Basilisk allows `new_depth + 2` then `+ is_pv` (`search_kernel_core.cpp`, `CoreLmrNegativeAllowed`), and B.3's packet §5.10 keeps negative reductions outside the extension budget; B.5 owns this line growth already (Rarog's B-80, BAS-R07's 5–30× won-ending trees, BAS-S21's ceiling-off arm −5.6 ± 9.8). A chain-length read first. **B.5** |
| TS-2 | Low-depth hash-move extension (LDSE, from Reckless and Stormphrax) and its per-line cap (`LdseMax`) | +4.5 ± 3.3 at 10+0.1; the cap +6.25 ± 7.89; rook endgames ×5–×16 nodes without the cap; off in the release after −9.4 ± 10.9 at 40+0.4 (D8 §21–23, §48.4) | Basilisk has no LDSE. Small; the per-line cap is the transferable lesson (TS-1). **H.2** |
| TS-3 | Causal reduction: quiet siblings that touch none of a sibling's refutation squares reduced half a ply more | +4.27 ± 4.13 over 6,594 at 25+0.25 (D8 §38); its variants `CausalAgree`, `CausalNull` lost (D8 §46) | Small. **H.2** |
| TS-4 | TT cutoff value damped toward beta (from Coda) | +2.2 ± 3.2 over 11,774 in three runs (D8 §27) | Small. **H.2** |
| TS-5 | HistGrad: quiet-history update scaled by a softmax gradient over the ordering scores (surprising best move learns more) | +0.90 ± 3.41 over 11,166 (NOV; commit `59003c8`) | Small. **H.2** |
| TS-6 | Discovered-check ordering bonus (`ChkDisc`) | +0.8 ± 3.7 over 9,178, paused (NOV) | Small. **H.2** |
| TS-7 | Chess-knowledge ordering terms: king shield, outposts, safe attacks, passed push, queen promotion, bishop-pair capture | flat at hand values (D8 §34); baked through an SPSA vector (+3.4 ± 3.9); on the next network **removing them gained +4.21 ± 2.79 over 15,194** and 1.5–2.4% cycles (commit `9910b23`). An ordering bonus counted again in reductions lost (`KnowRed`, −13.6, D8 §46) | Coupled with histories and the good/bad quiet thresholds; worth nothing alone. **H.2**, with this history as the prior |
| TS-8 | Continuous improvement in the late-move count (`ImpSlopeLmp`, from Reckless): the move count scales with the two-ply eval change instead of the improving flag | baked 2026-11-03 (commit `0c18907`), no result in the message | Small or unknown. **H.2** |
| TS-9 | No null move with a minor piece or less (`NmpNpmFloor 3`) | +1.32 ± 2.29 over 6,844, endgame book (commit `af49b20`) | Basilisk's null-move gate is B.3's; small. **H.2** |
| TS-10 | Passed-pawn pushes to the 6th/7th reduced less and never pruned with little material | +4.4 ± 6.8 on an endgame book (D8 §17) | Basilisk's network-free evaluation may differ; small. **H.2** |
| TS-11 | Continuation correction six plies back with every correction weight rebalanced; correction weight after a null move as a lever | adopted in 8.1 (commit `2c33b92`); alone −0.8 ± 5.6 (D8 §26) | Basilisk's correction set is B.2/B.3's. Small. **H.2** |
| TS-12 | Corrections and margins per network expert (`CorrPhase` +3.2 ± 3.8; per-expert RFP/futility/LMR offsets +1.6 ± 4.3; reduction slope per expert +1.1 ± 3.7; history per expert −1.7; piece values per expert unbaked after +10.7 against +20.8 at 30+0.3; experts 5–7 margins unbaked after −6.49) (D8 §25, §45.4, §46, §47; commits `c8704f4`, `e1a884c`) | needs a network with experts, or a phase key for the HCE | **H.2**, only after F.7 decides the network's routing |
| TS-13 | Quiet checks removed from quiescence | +9.71 ± 5.99 over 3,294 at 20+0.2, after a one-move-per-node quiescence cap was lifted; under the cap the sign was reversed (D6, notes 9–10); Stockfish removed them in 2024 (PR #5498) | B.4.0's card 2 owns quiet checks. **B.4** |
| TS-14 | Quiescence ideas that lost: passed-pawn push to the 7th searched (−0.77 to +0.23 in four runs), quiet hash move searched (−1.05 ± 3.92), stand on a material estimate (`QsMatStand`, off) (D8 §21, §25, §41; commit `11bd148`) | negative priors | **B.4** |
| TS-15 | Root and aspiration ideas that lost or were neutral: decisive aspiration fail-high (`AspDecisive` +0.7 ± 4.2), root depth recovery after a fail-high (−1.82 ± 4.54), cap on root-move reductions (−5.80 ± 5.93); separate window growth after fail low and fail high kept as levers (D8 §27, §31, §42; S5 §5.4) | priors | **B.5** |
| TS-16 | Post-19 Stockfish ports that did not pay on this base: linear razoring at all-nodes (−2.32 ± 2.52 over 20,032), null threshold after sibling null fail-highs (+0.46 ± 2.66 over 17,308) (D8 §31) | negative priors | **B.6.1** (any later donor re-pin reads them first) |
| TS-17 | Edge cases from other engines, each flat over about 7,000 games: clean singular-verification nodes, root twofold as draw, move count restored after the singular search, no null move under a hash upper bound (D8 §42) | negative priors | **B.6.1** |
| TS-18 | Draw value in defence (`DrawDefend`), contempt as a rating-list lever | +0.3 ± 4.0 (D8 §48.1); contempt needs a gauntlet, not self-play (D8 §18) | **H.2**; E.2's contempt lever is already registered there |
| TS-19 | Long-time-control tree shape: Stockfish's reductions are deeper in endgames (mean LMR 6.5 against 3.6 plies at depth ≥ 12), more TT bounds fit the window (66% against 53%), more null moves succeed (72% against 60%); no single transplant shortened the tree; a 40+0.4 SPSA reduced less, reached 0.6 ply less and lost (D8 §10, §19) | method prior: depth-dependent levers need a long-TC fit, and short-TC tunes never see depth > 15 | **G.3** (search fit at LTC) |

### 4.2 Board and infrastructure (TB)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TB-1 | **`tdperft`**: perft through the search's own generator, make and unmake; at every node every key (hash, pawn, non-pawn, minor/major), the occupancy and the piece table are recomputed from scratch and compared, and every parent move is re-checked against the pseudo-legality test used for TT and killer moves | 482M nodes, 171 positions, 0 errors; validated the Chess960 work and a board rewrite (D8 §12, §14) | **B.15.1** |
| TB-2 | Zobrist GF(2) rank test (§3, first row) | rank 32 caused false hits | **B.15.1** (Basilisk reads 64) |
| TB-3 | FEN and state robustness: an en-passant square inconsistent with the side to move ignored; castling rights that do not match the board dropped; a position whose side not to move is in check rejected; a phantom en-passant square gave two keys to one position (D6 note 8, D8 §12, commit `67538a5`) | crashes, two keys | **B.15.1** |
| TB-4 | Per-node state computed once: pins and checkers shared by the picker's stages; one enemy attack map shared by the generator and quiet scoring (−0.35% cycles); the child's in-check status from the parent's gives-check; the illegal-position guard only at the root (D8 §4, D81 §1, commit `d6a79a8`) | identical-tree speed | **B.15.2** (Stockfish's `StateInfo` check info is the donor form) |
| TB-5 | Make and unmake: unmake restores keys, the fifty-move counter and occupancies from a 64-byte copy taken at make (−0.61%); make/unmake without branches on captures; incremental minor/major keys (−7.5% instructions together with two neighbours) (D8 §4, §40) | identical tree | **B.15.2** |
| TB-6 | SEE kernel: next attacker found by index (−0.38%); both bounds of the resolved list tested before any attacker lookup; the SEE of a quiet move skipped when the enemy attack map already decides it (28% of calls) (D8 §27, §36; commit `e876fa3`) | identical tree | **B.15.2** |
| TB-7 | Slider attacks: bishop and rook computed together where both are needed (slider computations per node 9.8 → 5.1); empty-board attacks from a small table; an AVX-512 obstruction-difference kernel on the four lines of a square (−0.98%); hyperbola quintessence on AVX2 (D8 §27, §40; NOV §8) | identical tree | **B.15.2**; Basilisk's BAS-P26 read compressed slider tables slower, so flat tables stay the default |
| TB-8 | Move generation and ordering mechanics: generator templated on the side to move; branch-free selection (the best-quiet selection caused 11% of branch misses); rank of each move by the count of larger keys, 16 at a time on AVX-512 (−0.70%); captures not regenerated by the quiet stage; victim through the mailbox (+0.7–1.0%) (D7 §3, D8 §4, §20) | identical tree | **B.15.2** for the generator, **B.7.2** for the picker |
| TB-9 | Chess960 representation: occupancy updated by XOR, both origins cleared before both destinations filled, rook origin from one table, X-FEN and Shredder-FEN, castling as king takes rook in UCI; the FRC perft suite at depth 4–5 (D8 §14) | no speed cost (+0.57%) | **B.15.2** designs for it; **G.2** decides the feature |
| TB-10 | One board for search and network: make records only the moved piece; before an evaluation the pending moves are replayed from the current board (variant c, +0.6–1.1% instructions against a lazy second board); a branch-free refresh-cache comparison (D8 §28) | identical tree | **B.15.2** sets the dirty-piece contract F.1 builds on |
| TB-11 | Stack-probe calls (`__chkstk`) in hot functions with frames over 4 KB on Windows: moving list fills out of line and evaluation lists off the stack, −1.55% cycles (D8 §22) | identical tree | **B.7.2** (a disassembly read beside B.7.1's) |

### 4.3 Throughput and builds (TB, continued)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TB-12 | Early prefetch of the child's TT entry from a predicted key at the start of make (+2.21% NPS); prefetch of the child's correction entries (+2.04%) and of the continuation-correction cells at make (−0.3%, which also removed a memory-bandwidth asymmetry between sockets) (D8 §4, §48.2) | Basilisk's B.7.2 lists Rarog's D-5/D-6 child prefetch | **B.7.2** |
| TB-13 | Search state on large pages: histories, correction tables and per-thread data (−0.93% cycles), beyond the TT (D8 §30) | Basilisk's B.7.2 has large pages for the TT (D-8) | **B.7.2** |
| TB-14 | Link-time flags: ext-tsp block layout (−0.6% with a smaller eval cache entry), no cmov-to-branch conversion (−0.15% / −0.29%) (D8 §36, §48.3); deterministic PGO training at fixed nodes so two builds of one source are identical (D8 §31) | identical tree | **B.7.2** |
| TB-15 | Per-thread eval cache removed when the TT already stores the evaluation (12 hits in 2.5M lookups; −0.19% / −0.32%) (D81 §1) | identical tree | **B.7.2**, only if Basilisk has such a cache |
| TB-16 | Measured priors on what does not pay: on that Xeon only removing work paid; prefetching and reordering loads mostly did not (D8 §23); AVX-512 gathers cost as much as sixteen scalar loads (D8 §27); the next-move TT prefetch cost on AMD and on the dual Xeon (D81 §1) | priors | **B.7.2** |

### 4.4 Time management (TT)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TT-1 | **Loss analysis by cell**: every move of lost and spoiled games scored by a judge engine, tabulated by position class (defending, level, ahead) × clock left × material. 33.8% of the loss sat in "defending with under 10% of the clock"; half the real errors vanished with more nodes (D8 §32, §43); defence held 36.7% against Stockfish's 50.6% (D8 §47.4) | an instrument that found the targets of every later time rule | **B.16.3**, read by D.1 |
| TT-2 | **Wall-clock overhead instrument**: `go nodes 1000` wall time against the engine's own report and against Stockfish (6.15 ms against 1.24 ms found the brake); `go movetime` overshoot; ponderhit-to-bestmove latency (D8 §45) | found +29.8 Elo | **B.16.1**, read by D.1 (Basilisk passed the code reading in §3; the read verifies it) |
| TT-3 | Time levers depend on the time control: measure each at its own control; tune them against an external engine with release-speed builds, because self-play rewards the side that spends more (D8 §19, §21, §23; commit `878b574`) | method | **D.1** |
| TT-4 | Game ply from a FEN's move number (`TmFenPly`: after a book the first move got about 30% less time); `go movetime` not scaled by time factors; a hard timeout that fired before any root move existed (D8 §20, D6 note 8) | defects | **D.1** |
| TT-5 | A lower cap on the longest thinks lost −27.1 ± 17.0 at 40+0.4; an expected-reply depth floor lost (−5.5 ± 13.4) (D8 §23, §27) | negative priors | **D.1** |
| TT-6 | Moves-left horizon fitted on the engine's own games, at parity (`TmOwn`); SPSA'd time levers ramped by game time between fitted values (≤ 21 s) and SPSA values (≥ 35 s) because the vector lost at 8+0.08 and gained at 25+0.25 (commits `5a1a1b2`, `67538a5`) | method | **D.1** |
| TT-7 | **Surprise rule**: ×1.2 time when the opponent does not play the predicted reply. +6.23 ± 6.75 at 40+0.4, +0.7 ± 7.1 at 16+0.16; silenced while behind on the clock (D8 §23; commit `ce03c53`) | small, long TC only | **H.3** |
| TT-8 | **Less time when behind on the clock**: optimum × (1 + 0.6·min(relative clock advantage, 0)). +5.23 ± 4.72 at 6+0.06, +0.36 ± 5.13 at 30+0.3 (D8 §31) | small | **H.3** |
| TT-9 | **Time in defence and early saving**: ×1.5 while the root score lies in about [−3.6, −0.45] pawns; optimum −20% with 24 or more pieces. +7.27 ± 7.08 at 40+0.4 together, not separated (D8 §43) | small | **H.3** |
| TT-10 | Guard on sudden score jumps (no time saving on an unconfirmed jump of two printed pawns), −2.52 ± 7.33, kept as a guard; graded surprise; more time when the PV trades into another expert (`TmTrans` +2.35 ± 4.21); time share per expert (+1.9 ± 5.9) (D8 §44, §47; FD §6; commit `5a1a1b2`) | small | **H.3** |

### 4.5 Threads, TT and SMP (TP)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TP-1 | **16-byte TT entries, four per 64-byte bucket, XOR-verified** (lockless: a 48-bit key tag and the static eval in one word XORed with the data word, so a torn write fails the check), bucket index by multiply-high; +6.30 ± 5.06 at Hash 16 against its 24-byte layout (D8 §6; `source/tt.h`) | coherent by construction | **D.2**: a candidate for BAS-C05's retry trigger ("a TT redesign that widens the slot for an independent reason" or a coherent layout within ±0.5%). Basilisk's 10-byte entries are a third denser; the trade is entries for coherence |
| TP-2 | TT store race: reusing the old move and eval after another thread rewrote the way; fixed by re-checking the tag (D81 §2, Bastiii's review) | SMP defect | **D.2** |
| TP-3 | Per-thread state aligned to 128 bytes against false sharing at every make and unmake (commit `f7e1489`) | identical tree | **D.2** |
| TP-4 | SMP levers: helper LMR bias in fine units +4.91 ± 7.97 at 8 threads (from Reckless; D6 row 5); thread vote −3.5 ± 8.7, optimism per thread +0.9 ± 8.6 (D8 §12); staggered helpers and depth in the vote +1.72 ± 4.61 (S5 §5.3); a helper one iteration ahead at ply 2: time to depth 0.908 at 8T, no trend with depth, −2.23 ± 15.48 (D8 §47.2) | priors | **D.2** |
| TP-5 | SMP contracts: a proven mate beats depth in the thread vote; `go nodes` counts every thread; thread 0's PV reset before helpers start (D8 §12) | defects | **D.2** |
| TP-6 | NUMA: threads spread over nodes, one network copy and one set of shared histories per node, each thread's handle on its node (commits `f7e1489`, `36c4e9f`, `f7a83be`) | 4-CPU lists | **G.1** |

### 4.6 Protocol and robustness (TR)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TR-1 | `go`, `d` or `eval` before any `position` searched an empty board and crashed: the loop now starts from the start position (commit `51a3ca6`) | crash | **D.3.2** |
| TR-2 | Input: repeated spaces and a trailing `\r` accepted; a 64 KB input buffer; a clock of exactly −1 not read as "no clock"; `bestmove` and `readyok` under the output mutex; `go infinite` waits for `stop` (D8 §12) | defects | **D.3.2** |
| TR-3 | Output after a stop: the final line printed after a stop mid-iteration (the GUI warned that bestmove did not start the last PV about once in three games); with MultiPV the lines printed again at the stop; `currmove` restored after 3 s (D8 §30, §44, §49) | protocol | **D.3.2** (beside D-73's MultiPV tests) |
| TR-4 | Stale per-ply state in a child: the ProbCut child initialised one of five per-ply stacks, so a sibling's `captured`, history bucket and double-extension count leaked into it (D6 note 8); killers of the child ply cleared at every node, +12.15 ± 5.97 there (D6 row 8) | defect class | **D.3.2** (a test that a child frame starts clean on every entry path) |
| TR-5 | An 8 MB thread stack on Windows; a correction-history decay that overflowed 32 bits in tablebase endgames (D8 §12) | defects | **D.3.2** |

### 4.7 Measurement and tuning method (TM)

| ID | Lesson | Evidence | Destination |
|---|---|---|---|
| TM-1 | One engine per physical core: two engines on one core cost about 1.6 plies at 5 s, and SPSA vectors tuned under hyperthreading gained only where the machine was slow (D8 §45.3, §45.5) | Basilisk already pins physical cores (PLAN rule 7) | reference; no leaf |
| TM-2 | The SPSA perturbation schedule: `c` must reach its final value at the last iteration (fishtest's form); the iteration count of the schedule must equal the iterations played; ranges chosen so the start ± 2.25c never crosses a point where the formula changes nature (D8 §48.1; D81 *Plan*, item 4) | a tool defect that halved every 8.0 tune | **B.3.3**: checked 2026-11-03 on block 1, Colosseum reaches `c_end` at the last iteration (§6) |
| TM-3 | A lever enters a bake only when its net move is at least 2.5 times the random-walk expectation: random-walk segments look monotone (rank correlation ≥ 0.85) 30% of the time, so with 37 levers about 11 pass a monotonicity test by chance (S2 §2.4) | Basilisk's B-101 journal z-read is the same rule | reference (B.6) |
| TM-4 | Stopped "leans" add up to nothing; a bundle of single +1 to +2 estimates read +0.51 ± 6.49 (D8 §18, §48.1) | Basilisk's registered stop rules already forbid this | reference |
| TM-5 | A measurement is valid only in the regime it was taken: quiet checks in quiescence flipped sign once a one-move cap was lifted (D6 notes 9–10); a correction weight went up at 15 s and down at 30 s (D7 §2) | method | C.0, B.4 (cited there) |
| TM-6 | An idea that should not lose but does is analysed before closing: `CorrPhase` lost −5.1 until a prefetch of the wrong (unsalted) keys was found, then gained +3.2 (D8 §25) | method | reference |
| TM-7 | Speed by hardware counters at an identical tree: cycles, instructions and branch misses per node, the engine pinned to one core, only search threads counted (the start-up was 5% of the cycles at 400k nodes per position), and a byte-identical copy of the base as the noise control; paired simultaneous runs on two hyperthread siblings with a null test of ±0.1% (D8 §2, §36) | Basilisk's pooled-PGO read (A.9.4) and profiler (A.9.5) are its instruments | **B.16.4**, then B.15.4 and B.7.2 |
| TM-8 | Tree-size centring for any lever that changes the tree: a fit on search data plus a node-count sweep on fixed positions finds in minutes the value an SPSA looks for over hours; the bench is no use for it because a few positions dominate its sum (S2 §2.6) | method | reference |
| TM-9 | **Pricing what pruning and reductions hide.** Data builds record which rule pruned or reduced a move and search a sample of those moves again at the denied depth: `prunelog` for every pruning rule (S3 §3.3), `redlog` for reduced searches, a quarter of the reduced fail-lows probed at full depth (S2 §2.6). The useful label is "failed low reduced, beats alpha at full depth" (1.7% of probed moves); the first label, "beats alpha", spent nodes on moves already re-searched (−9.65 ± 10.36) | Basilisk's `TraceEvent` records each decision but never prices it | **B.16.2** |
| TM-10 | Shortcut builds: every identical-tree shortcut ships with a build switch that runs the original beside it at every node and aborts on the first disagreement (`TRIUMV_VERIFY_*`: SEE, TT store, rank, quiet scores, attack maps, accumulator sync) (D8 §27, §40) | method | **B.15.3**, **B.7.2** |

### 4.8 Evaluation analysis (TE)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TE-1 | **Residual regression**: for each position, the score of a depth-1 search with quiescence against a 300k-node search; about thirty concepts regressed on that residual by least squares, holding the starting score, material and piece count fixed; |t| > 3 counts as a signal. On Consilium it found passed pawns (+11.1 cp per rank in endgames, t 10.3), unstoppable passers (+44.8), space (+12.0 per square), open files by the king (−15.4) and king-zone attackers (−11.1) underestimated, and drawing factors handled (FD §1) | for an NNUE the residual terms all lost when added outside the network (D8 §44), because the network and the search already held them | **C.0**: on Basilisk's HCE a residual is a direct pointer to mis-valued terms, the question C.0 asks |
| TE-2 | Unsupervised regimes: the first-layer outputs clustered and compared with candidate keys by normalised mutual information: material × queens explained the network's own division best (0.43), material bands 0.34, closed structure 0.07 (FD §2) | method | **F.7** (bucket keys), and C.7 (phase key) as a cheap check |

### 4.9 NNUE architecture and inference (TN)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TN-1 | **Input-layer experts by material**: the king-relative block in four weight sets by total piece count (cuts at equal-frequency bins 9, 15, 23), trained as a shared base plus zero-started deltas (the factoriser applied to bands); a capture across a band forces a refresh; refresh cache per band. −7.5% NPS against −11.2% for L1 1536 and −20% for 2048; about +15 Elo of 8.0's +27.3 with its retune (NET, NOV §1) | measured | **F.7** |
| TN-2 | **Per-side experts by the opponent's non-pawn material** (eight per perspective, a routing table over material signatures, learnable but moved under 1% of its cells), int8 king-relative weights clamped during training, a pawn-king subnetwork (256 hidden, cached per thread, added to the first layer of each stack): +10.3 ± 4.3 over 8,332 at fixed nodes (NET; R07) | measured | **F.7** |
| TN-3 | Input blocks: `PassedPawns` (96 inputs, one per passed pawn by square) +6.96 ± 6.56 grafted; `PawnPair` (Pawnocchio's, Stockfish's `PP_3Wide`) +18.27 ± 9.94 with own self-play data; threat inputs (SFNNv13 onward); a mobility block costs −16.2% NPS untrained (NET; D8 §5; D6) | measured | **F.7** |
| TN-4 | **Zero-initialised graft** of a new block on a finished network, the rest frozen, as a cheap screen; its limit: the frozen later layers may not use new information (a passed-pawn state block gained only after the layers after the accumulator were unfrozen), and a block started from existing rows needs a lower rate (NOV §7, D8 §38) | method | **F.7** |
| TN-5 | Ideas that lost: averaging two experts near a band edge (+5.27 at fixed nodes, +15–18% cycles, −24.23 at time); a split accumulator (+12% cycles from doubled memory traffic); 8-bit king-relative weights quantised after training (0.2–0.5 ply slower; needs clipped training); fixing the root's expert for the search (−27.7) (D8 §44, §46; commit `878b574`) | negative priors | **F.7**, **F.5** |
| TN-6 | **A policy network inside alpha-beta loses**: −85, −30.56, −46.89, −22.83 in four forms; ordering already reached 79.2% first-move cutoffs without a hash move, and a static prior computed at the parent loses to histories that adapt to the subtree (NOV §9) | refusal prior | **F.0** |
| TN-7 | Inference speed: refresh cache (Finny table) per band; a pawn-block refresh cache keyed by the pawn bitboards (+2.8%); the hybrid update on king moves (Stockfish `db98633b`, +3.4%); both perspectives in one dirty-list pass (+1.5%); first-layer neurons permuted at export so zero activations fall in skipped blocks (+2.5%, bit-identical); no threat tuples opened and closed inside one capture; the System V calling convention for inner functions on Win64 (no xmm6–15 saves); the network on large pages (+6.7% at 250 MB) (D7 §3; NET; commits `b34547f`, `8568809`, `42e68f5`) | measured | **F.5** |
| TN-8 | Memory is the cost: 47% of last-level misses in the incremental accumulator update, 11% in refreshes, under 5% for corrections, ordering and the TT together (D8 §46.3) | measured | **F.5** |
| TN-9 | Conformance gates before a training run: an integer evaluator matching the trainer on exported networks; loader indices matched against python-chess (0 of 7,025); a random network with perturbed per-band deltas, since zero deltas hide a wrong band index; engine against evaluator move by move across bucket changes; incremental against refresh at every node (`nnperft`); a golden set of 2,000,124 positions bit-identical after a clean-room rewrite (NET; R07; commit `c35fcca`) | method | **F.4** |

### 4.10 NNUE data and training (TD)

| ID | Lesson | Evidence | Destination |
|---|---|---|---|
| TD-1 | A lambda cycle that climbs back toward 1.0 makes the network imitate the labels: on a fixed holdout the λ=1 loss kept falling while the λ=0 (game result) loss stopped at epoch 200, exactly where Elo flattened; fixed λ 0.75 restored the gain (NET, *The plateau and the lambda fix*) | method: watch holdout loss at fixed λ=0 and λ=1, never the moving training loss | **F.3** |
| TD-2 | Registered checkpoints with Elo targets during training; a logarithmic fit of early points mispredicted the plateau (−32 predicted, −70 measured); late gains arrive while the rate decays (NET) | method | **F.3** |
| TD-3 | A restart cycle at a fifth of the peak rate moved a plateaued pretraining (Consilium's F3, whose loss went below the plateau); the final network is the average of the last few epochs' weights; a second cycle after that made it worse (NET; R07) | method | **F.3** |
| TD-4 | The data loader, not the GPU, was the limit (one feature thread per GPU: 381k → 1.9M positions/s with a larger share); the loader drew files uniformly, so small files were over-read; piece-count sampling shifted toward endgames to feed endgame experts; soft early skipping instead of hard (NET; R07) | method | **F.3**, **F.2.1** |
| TD-5 | Fine-tuning a converged network on more data of the same family moved it by 1% in scale and 0 Elo; per-expert fine-tunes +0.7 ± 6.1 (D8 §29) | negative prior | **F.8** |
| TD-6 | Hard examples: train more on positions the engine loses or where a deep search contradicts the static evaluation, which needs a dedicated generation at scale; overlapping material bands in training so neighbouring experts agree at the edge (FD §4–5) | proposals, unmeasured | **F.8** |
| TD-7 | Network gains shrink at longer time controls and are hidden by shared cores: the same network read +0.8 ± 10.1 at 30+0.3 with 75 concurrent games and +23.4 ± 12.0 at 12+0.12 with 38 (D8 §9; D6) | method | **F.6** |
| TD-8 | Eval scale per piece-count bucket recalibrated for each network; matching the raw ratio to the previous network lost (−27 ± 24) where the search's preferred scale differed; the scale-dependent margins (25 parameters) retuned by SPSA on the final network (+8.6 ± 11.5); the blend constants inherited from Stockfish tuned on its own network (+3.75 ± 3.49 at 20+0.2) (NET; D7 §2) | method | **F.6** |
| TD-9 | One label scale per run, and label depth over volume: every source re-labelled by one evaluator before mixing, because mixed targets train toward neither; a net trained on Stockfish's 5,000-node self-play alone ended −84 against the reference, and a second stage on data with stronger labels (Leela's) closed half the gap; for self-play, stronger means deeper (`removed/dfa5a6b/docs/networks/NETWORKS_4.2-7.0.md`; recipe 02) | method, for self-play data | **F.2.1** |
| TD-10 | The stream's shape is measured: files drawn uniformly over-read small ones; piece-count sampling toward endgames; soft early skipping; the positions the search evaluates against the training stream by bucket (the search held 2–3.5× more unbalanced material pairs than the data; game positions at every ply are the right source) (recipe 07 `docs/C2_SEARCH_DISTRIBUTION_2026-10-21.md`) | method | **F.2.1** |
| TD-11 | Its first network's failures, each visible in the command line: no factoriser, λ 1.0 only, a stop on a flat validation loss while the rate was still 38% of its start: −102 against the reference; fixed, −39. Check a recipe against a reference before paying for a run (recipe 01) | method | **F.3** |
| TD-12 | Judge checkpoints at 20+0.2 or longer (−27 at 8+0.08, −84 at 20+0.2 for one net) and against one fixed anchor (checkpoint-to-checkpoint deltas summed to +71 where the direct gap closed by +28); the first epochs after a restart at a higher rate are recovery, not progress (−27 at epoch 19, parity by 119); a validation loss at a moving λ cannot rank checkpoints (recipes 02, 05) | method | **F.3**, **F.8** |
| TD-13 | Saturation calls for capacity or information, not epochs: on the 1024-wide net no gain between epochs 249 and 416, an annealing tail at −0.28 ± 5.21 over 5,024, fine-tunes at zero, while a 99k-parameter passed-pawn block bought +7 in four epochs and material buckets about +15 (recipe 05; NETWORKS_4.2-7.0) | method | **F.7** |
| TD-14 | Own self-play in a refine: 67M positions (6.5% of the mix, listed ten times) rode with new Leela data and the pawn-pair block, +18 together, so its share is unknown; a dataloader filter that streamed only positions with passed pawns added nothing to a converged block (recipe 03; NETWORKS_4.2-7.0) | prior | **F.8** |
| TD-15 | Finishing: weight average of the last few epochs (+3.8 ± 7.7 over a single checkpoint, kept for lower variance); feature-transformer neurons permuted over all input blocks, the evaluation bit-identical (+1.6 to +2.5% NPS; permuting only some blocks makes the net wrong, not slow) (NETWORKS_4.2-7.0; NET) | method | **F.3**, **F.5** |
| TD-16 | Its next-network design (recipe 07 `docs/DESIGN_NEXT_NETWORK_2026-10-21.md`): CPU-only gates before any GPU hour (the shape's speed with a random net, bucket changes per node in the tree, occupancy per bucket on the data, engine against trainer), proxy trainings of about 40G positions each judged by an SPRT, one full run, close calls to the conservative choice; and a survey of Stockfish's networks in October 2026: PSQT removed (SFNNv17), int8 first-layer weights by clamping, king × opponent-queen buckets with 32 layer stacks (SFNNv18, about +4 at STC and LTC, every single change failing alone), row pairing (+5 to +7% NPS), learned routers for the layer stacks | method and survey | **F.0**, **F.7** |

### 4.11 Analysis build and product (TA)

| ID | Mechanism | Evidence | Destination |
|---|---|---|---|
| TA-1 | An analysis build: the second-best root move always searched with a full window (Arasan 157 → 170 at 1M nodes, two to three plies of depth), paused in quiet positions; the more direct of two clear wins played (search tree unchanged); the analysis search compiled as a second copy so games pay nothing (D8 §36, §49) | product | **G.2** |
| TA-2 | Printed scale calibrated on self-play so +1.00 means a 50% chance to win, refitted for each network; tablebase wins printed as `cp 20000` minus the distance (D8 §24; commits `28c44db`, `9e74ae8`) | product | **G.2** |
| TA-3 | Playing strength levels (`UCI_LimitStrength`, `UCI_Elo`) by limiting the search and choosing among the best root lines, calibrated in games (FD §13) | proposal | **G.2** |

### 4.12 Triumviratus's NNUE path, network by network

Read for Basilisk's Phase F, which trains only on Basilisk's own self-play:
the method and the failures transfer, the public-data recipe does not.

| Net (release) | What was done | Result | Lesson |
|---|---|---|---|
| before 4.2 | Stockfish's own network in the engine; a policy network tried inside alpha-beta | the policy net lost in four forms | TN-6 |
| `rubicon-v1` (4.2) | `HalfKAv2_hm^`, L1 2560, Leela `test80`, nnue-pytorch; first run without the factoriser, λ 1.0, stopped at epoch 124 on a flat validation loss | first run **−102** against the Stockfish net; the fixed run (factoriser, about 400 epochs, λ squeezed to 0.75 on the best checkpoint) **−39** | TD-11 |
| `rubicon-alea-v1` (5.0) | threat inputs (SFNNv13), L1 1024; stage 1 on Stockfish 5,000-node self-play, stage 2 on Leela data at λ 0.74 | stage 1 **−84** at 20+0.2 (−27 at 8+0.08); after stage 2 **−40**; the engine +50 over 4.2 | TD-9, TD-12 |
| `rubicon-alea-v2` (6.0) | fine-tune of v1 with a pawn-pair block grafted at zero, two learning rates (base 1e-4, new block 1e-3), λ 0.75 fixed, 6.5% own self-play; validation flat from epoch 80, stopped at 470; anneal and average of the last 3 | **+18.3** over v1, network isolated | TN-4, TD-14, TD-15 |
| `rubicon-alea-v3` (6.0) | a 96-input passed-pawn block trained alone on the frozen base | **+7.0** in four epochs; later epochs no better | TD-13 |
| `legio-septima` (7.0) | SFNNv16, everything from scratch, both stages re-labelled to one scale (BT4); λ fixed early in stage 2 | **+23.4**; then saturated (annealing tail 0, no gain between epochs 249 and 416) | TD-9, TD-13 |
| Consilium (8.0) | four input-weight sets by material at the same L1; one data mix; a λ cycle that climbed back to 1.0 caused a plateau, fixed mid-run; restart cycles F3 and F4; average of the last 5 epochs | about **+15** of 8.0's +27.3 | TN-1, TD-1, TD-3 |
| Consilium Plebis (8.1) | eight per-side sets by the opponent's material, int8 king-relative weights, a cached pawn-king subnetwork; CPU gates and a staged plan before training | **+10.3** at fixed nodes | TN-2, TD-16 |

What Basilisk can skip, in the order Phase F meets it: a recipe checked
against a reference before the first paid hour (TD-11); conformance gates
before training (TN-9); one label scale and labels deep enough (TD-9);
the stream's shape measured against what the search evaluates (TD-10);
checkpoints judged at long controls against one anchor (TD-12) on
holdout loss at fixed λ (TD-1); λ fixed early; capacity, not epochs, once
saturated (TD-13); and a network programme shaped as CPU gates, proxies,
then one full run (TD-16).

## 5. Interactions (what each placed item touches in Basilisk)

- **TS-1** feeds the depth every LMR child searches at: it interacts with
  B.3's extension budget (`ext_plies`, `ext_budget()`), the singular
  extension's double/triple extension, the `research_deeper` re-search and
  IIR. The cap and the budget count different things. BAS-R07 already
  found 10.9% of LMR-path searches going deeper than full depth; the read
  that decides whether a game is worth it is the distribution of chain
  lengths per line at fixed depth (`suite_v1.epd` and the endgame floors'
  positions).
- **TS-7** (ordering knowledge) is coupled to the quiet histories and to the
  good/bad quiet thresholds that LMR and LMP read; a bonus counted again in
  reductions lost (`KnowRed`). It is never a standalone term.
- **TS-12** needs a routing key that does not exist until F.7; with an HCE
  the nearest analogue is Basilisk's phase, and C.7 owns phase.
- **TT-7 to TT-10** depend on the time control and interact with each other
  and with the clock's stability rule; Triumviratus never separated TT-9's two
  parts.
- **TP-1** trades about a third of Basilisk's entries per MB for a coherent
  record; the capacity loss must be priced at Basilisk's Hash and time
  controls, and BAS-C05's two earlier repairs priced at −3.91% and −1.22% NPS
  are the bar.
- **TB-4/TB-5/TB-10** change what make stores per ply, which is also what
  F.1's accumulator scaffolding needs: B.15.2 fixes that contract once.

## 6. Open checks this review leaves

1. Basilisk's SPSA schedule against TM-2: **resolved 2026-11-03.** The
   generated tune files set `c_end = step · N^-0.102` for N = 2,000, and
   block 1's resolved schedule (`tools/results/b33-spsa-block1/resolved-config.json`)
   sets `c0 = c_end · 2000^0.102` (1.9999 for `c_end` 0.9211) with
   `gamma` 0.102; its log shows c falling from c0 each iteration. `c_end`
   is reached at the last iteration, fishtest's form.
2. Basilisk's hot-function frame sizes against TB-11 (a disassembly read).
3. Whether Basilisk has a per-thread eval cache that the TT duplicates
   (TB-15).
4. Triumviratus 8.1's later results (its SPSA blocks I and P2, the R19
   options of 2026-11-03) were not yet recorded in any document at the pin;
   a later re-pin reads them.

## 7. Negative priors, collected

Not to be re-proposed for Basilisk without a reason Triumviratus's base
lacked (their evidence is in §4 and the cited sections):
razoring linear at all-nodes; null threshold after sibling null fail-highs;
fail-high depth recovery at the root; root reduction cap; decisive
aspiration fail-high; clean singular-verification nodes; root twofold as
draw; restored move count after the singular search; no null move under a
hash upper bound; no null move or no RFP improving discount against an easy
capture; null-move threat extension; transition extension; quiet hash move
in quiescence; passed-pawn push in quiescence; hash-move refresh on a kept
entry; near-miss TT cutoffs; the refutation of a failed null move as the
node's threat (ordering and reduction); reduced reductions for double
threats; reductions learned from search logs (60,000 games, H0); a quiet
history in separate tables by threat context (33,510 games, −3.32, H0);
ordering knowledge counted again in reductions; pawn breaks in closed
structures reduced less; a cycle counted as a draw only beyond a
fifty-move threshold; Stockfish 19's fifty-move bands in the hash key on an
evaluation that already fades; correction faded with the fifty-move
counter; extra fade with the counter; correction tables keyed on concepts;
constant evaluation terms added on top of a network; an expected-reply
depth floor; a lower cap on the longest thinks; a helper thread one
iteration ahead; a policy network inside alpha-beta; expert blending at
evaluation time; a split accumulator; post-training 8-bit weights.

## 8. One rebuild or clusters: what Triumviratus's record says

The maintainer asked (2026-11-03) whether rebuilding the search and the
rest in one step would have been faster. Triumviratus did both, and its
record separates the cases:

- **One step, statistical acceptance.** On 2026-10-04 it rebuilt its whole
  search on Stockfish 19's base in a day and fitted it with one 51-parameter
  SPSA: +85.8 ± 12.8 at 20+0.2 (D8 §17). It then spent the next two weeks
  finding what the big diff had hidden, each a separate SPRT: the cycle
  filter from an older Stockfish (+5.62), quiescence TT depth (+3.38), pins
  in SEE (+4.5), the null move and fifty-move counter (+1.6), a lost
  prefetch, seven adapter divergences and wrongly ported options (D8 §20,
  §25, §41, §48.2). Its first one-step rebuild of the reductions in 8.1,
  accepted on short-control parity, lost at long controls and was taken
  back out (D81 §3).
- **One step, exact acceptance.** Its 8.1 rewrites at an identical tree
  (inference, infrastructure, time manager, each block of the search) went
  fast because the bench, perft and bit-identical evaluations decide each
  one with no games (D81 §3).

For Basilisk: a component whose correctness has an exact oracle
(identical tree, perft, bit-identical output) can be rebuilt in one step,
which is B.15's shape. A component accepted by games is cheaper in
clusters, because a failed one-step change cannot say which part failed,
and Basilisk's evaluation sits on a different score scale from the donor's,
so its constants need fitting cluster by cluster (Rarog's first cluster
read +65 unfitted and +138 fitted; Basilisk's +136 and +58.7 on top).
Much of Phase B's calendar time so far went to correctness repairs and
instruments (A.9–A.17), which a one-step rebuild would also have needed.
