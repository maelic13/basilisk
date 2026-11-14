# B.7.2 board speed inputs: evidence inventory (2026-11-14)

Input file for PLAN leaf B.7.2 (branch `b7-board-audit`, after B.7.1). It decides nothing: it lists
techniques, who has them, what a source measured, and what Basilisk does today. Nothing was built or run
for it.

**Conventions.** SF = Stockfish 19 at `edb0d9db` (`git -C D:/code/stockfish show edb0d9db:src/<file>`); no
source measured SF here, so every SF row is design, "unmeasured". TRI = Triumviratus (a prior from another
engine): `TRI-R` is `analysis/triumviratus_review_2026-11-03.md` (row id), `TRI-D8` is
`docs/reference/triumviratus/docs/development/archive/DEVELOPMENT_8.0.md` (section), `TRI-N` is
`.../docs/project/NOVELTIES.md`, `TRI-src` is `docs/reference/triumviratus/source/`. RAR = Rarog (a prior from
another engine): files under `docs/reference/rarog/analysis/`, `RAR-D` is `analysis/rarog_audit_2026-10-24_D.md`
(item D-n). BAS = Basilisk's own record. **Sign convention differs by source:** TRI's cycles/instructions per
node are negative when faster; RAR's and BAS's NPS figures are positive when faster; each cell says which.
Intervals are as printed by the source; "median" figures are never converted. A board microbenchmark is not a
search-speed measure (see the calibration in *Methods*).

Basilisk's own profile (BAS A.17 survey, `bench 13` x5, 32,862 samples, Profile build of `e7bf352`; local
artifact `tools/results/a17-profile/head/report.json`, summarised in `docs/experiments/BAS-P26.md`): regions board 26.7%
(30.1% with inlined callees), ordering 24.6%, search 19.8%, eval 17.2%, attacks 5.4%, TT 4.7%. Exclusive
functions in the board and neighbouring regions: `make_move` 2.64%, `attackers_to` 2.47%, `move_piece` 2.21%,
`is_attacked_by` 1.93%, picker `fill` 1.75%, `bishop_attacks` 1.58%. B.7.2's own A.9.5 profile at the current
head is not yet taken; these are the only Basilisk board shares on record. `gives_check` is called 1.43 per
node and `see_ge` 0.57 per node (`analysis/search_programme_2026-10-06.md` section 2.5, diag counters).

## 1. Check information and gives_check

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Check squares, blockers, pinners computed at every make and stored in the state | SF `StateInfo::checkSquares/blockersForKing/pinners` (position.h:45), `Position::set_check_info`, `update_slider_blockers` (position.cpp:467, 613); called at the end of `do_move` | SF: unmeasured. TRI-R TB-4: one enemy attack map shared by generator and quiet scoring -0.35% cycles; check info is Stockfish's form, not separately timed | `Board::check_squares(pt, us)` (board.cpp:1407) recomputes per call; `CoreMovePicker::checks_for` (move_picker_core.cpp:42) caches per picker, lazily per piece type | Eager per-make form is exactly what BAS-P04 measured as a loss (row below); the lazy per-picker form is Basilisk's |
| Same, as Basilisk's own experiment | BAS-P04 (`docs/experiments/BAS-P04.md`; CHANGELOG 1.9.1 line 786) | **BAS: per-node `CheckInfo` + check-hinted `make_move` read -1.8% to -2.7% (NPS), "a repeat of the 8.5.1 result, closed permanently"**; pin sharing across generation stages -0.16% | none kept | Strongest local prior against; retry only on a profile showing changed reuse (BAS-P04's own trigger). Pool, build count and interval are not in the record |
| `gives_check` from the stored check info: two bit tests for the direct and discovered cases, slider work only for promotion and en passant | SF `Position::gives_check` (position.cpp:766) | unmeasured | `Board::gives_check` (board.cpp:1419): direct case by one lookup on `new_occ`, discovered case always two slider lookups on the king | RAR cost shares on its own check queries: `gives_check` 1.654%, `check_info` 1.026%, `compute_pinned` 0.979%, whole check-query region 5.179% (board_search_profile_2026-09-08, RAR-M36). Basilisk's share is unmeasured; B.7.1 verified `gives_check` equals play on 9,498 checks (a differential exists for any rewrite) |
| Child's checkers from the parent's gives-check (`do_move(givesCheck)`) | SF `do_move`: `st->checkersBB = givesCheck ? attackers_to(ksq) & pieces(us) : 0` (position.cpp:~1050); TRI-R TB-4 (with root-only illegal-position guard, -2% instructions, TRI-D8 s4) | RAR: 89.02% of real makes take the hinted form; 99.38% of `gives_check` calls take the fast path, 755k full of 121M (board_search_profile_2026-09-07, counters). TRI: -2% instructions as part of a bundle | `make_move` always runs `attackers_to(king, all_occ, ~stm)` (board.cpp:741), two slider lookups per make; null make sets 0 | Basilisk's hint attempt is the BAS-P04 loss. Only a consumer that already computed `gives_check` could pass it, and the search computes it lazily (1.43/node) |
| Cheaper in-check test at make | none in donors (they use the hint) | unmeasured | as above | Candidate only if a profile shows the make-time `attackers_to` is material: `attackers_to` is 2.47% exclusive in A.17 but shared with SEE and generation |

## 2. Make, unmake and state copy

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| State record copied up to a `key` marker at make, previous pointer, unmake restores nothing but the pointer | SF `StateInfo`, `do_move` memcpy to `offsetof(StateInfo, key)` (position.cpp:~830), `undo_move` (1086) | unmeasured | `UndoInfo` (board.h:16): five keys, checkers, ep, castling, halfmove, plies_from_null, captured, pushed on a `std::vector` (reserved: `Board` copy keeps `HISTORY_RESERVE`+headroom); unmake copies it out and restores keys by assignment | Basilisk's record is larger than TRI's 64 bytes; RAR's `UnmakeInfo` is 24 bytes (representation_2026-09-08, `size_of` guard) |
| Unmake restores keys, rule-50 and occupancies from a copy taken at make; no capture branches | TRI-R TB-5; TRI-D8 s40 package AA3 | **TRI cycles/node: unmake alone -0.61% (middlegame, 8 builds, rounds spread to 0.9%); the whole search package -1.33% mid / -1.30% endgames; the infrastructure variant alone -0.29%** | Unmake already restores keys by assignment but first calls `put_piece/move_piece` (board.cpp:17-66), which also XOR the hash and the pawn, minor and non-pawn keys, then overwrites them (board.cpp:793-800) | Removable duplicate key work in unmake; B.7.1's restore-every-field test covers it. Prior from another engine |
| Make/unmake without branches on captures | TRI-D8 s20 table | TRI avx512 -0.13% cycles, -5.2% mispredictions: **noise, not kept** | `make_move` branches on EN_PASSANT, CASTLING, capture, PROMOTION (board.cpp:679-730) | Neutral prior; do not chase |
| Incremental pawn, minor, non-pawn keys | SF `do_move` (pawnKey, minorPieceKey, nonPawnKey); TRI-R TB-5 | TRI -7.5% instructions with two neighbours (TRI-D8 s4); table-driven partial keys as part of a -1.05% cycles package (TRI-D8 s17) | Incremental already, with three type tests per `put/remove/move_piece` | HAS; only the branches are open |
| Fused ordinary relocation: one from/to mask and one paired key | RAR relocation_2026-09-07 (RAR-M32 void, RAR-M33 accepted) | **RAR NPS +0.876% [+0.050%, +2.055%], 32 pairs, 23 faster; isolated make/unmake +16.3 to +19.3%; interval half-width 1.003%, 200-seed sweep excludes 0 in 198** | `Board::move_piece` is already fused (mask, paired keys, board.cpp:46) | HAS for quiets and the moved piece in captures; capture path uses `remove_piece` + `move_piece` |
| Predicted-key estimate of the child key | SF `Position::prefetch_key` (position.cpp:1293) | see section 3 | none | see section 3 |
| En passant square kept only when a legal capture exists, from stored blockers | SF `do_move` pawn branch (uses `st->previous->blockersForKing`) | unmeasured | `ep_capture_legal` full simulation per candidate, only on a double push with an attacking pawn (board.cpp:723, 1520); identity fixed by B.7.1 | Different, exact form; cost unmeasured, rare path |
| Plain king-square field | RAR king_square_cache_2026-09-08 (RAR-M37) | RAR: lookup share 0.502% (0.544% earlier), 2x ceiling 0.25%, closed NO_CHANGE: cannot be resolved by a 1.003% instrument | `king_sq[2]` cached, kept by `put_piece` | Rarog extracts by `lsb`; Basilisk's cache already exists; nothing to gain by changing either way |

## 3. Keys and TT prefetch

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| TT prefetch from the new key inside `do_move`, before the pieces move; correction-history entries prefetched there too | SF `do_move` (position.cpp:1008-1018), `prefetch_key` (1293) | SF unmeasured | `TT::prefetch(key)` (tt.h:201) called after make: `search_kernel_core.cpp:1156, 1254, 1737` (RAR-D D-5 read 1058-1059 in an older head); no predicted-key helper; no history or correction prefetch | The only cross-engine speed result on this item is positive |
| Early prefetch of the child's TT entry from an estimated key at the start of make | TRI-R TB-12; TRI-D8 s4 | **TRI NPS +2.21%** (paired runs); correction-entry prefetch +2.04%; cont-correction prefetch -0.3% cycles; **issuing the child hash prefetch earlier still: +0.03%, withdrawn (TRI-D8 s17)** | as above | Prior; TB-16 warns that on that Xeon prefetching mostly did not pay except these; "next-move TT prefetch cost on AMD" |
| Same from `key_after_hint`, before the make | RAR-D D-5 (RAR-P30/P31 candidate A) | **RAR NPS +1.48% screen, +1.69% [+1.56%, +1.89%], +1.78%**; hint test fails under three planted key defects | none | Prior; Rarog's TT first-load was 6.89% of samples there; Basilisk's TT region is 4.7% (A.17) |
| Pick-time prefetch (when the move is picked, before pruning tests) | RAR-D D-6 (`c12_speed_pass_2026-10-17.md`) | RAR +0.5% and +0.3% under PGO; -0.04% without; moving it later or dropping it lost 1.7% | none | Prior; PGO-dependent, so read under PGO (D-14) |
| Most other prefetches lose under 4 KB pages | RAR-D D-7, D-15 | RAR: history prefetch -3.1%, correction -0.4%/-0.6%, store-side -0.26% (+0.32% on large pages), RAR-P30 D -1.87%; branch-free TT probe -2.0% | no history prefetch | Loss prior; B.8 adds large pages, which changes this |
| Rule-50 band in the key | SF `adjust_key50` (position.h:~319) | unmeasured | clock deliberately outside the key (B.7.1, A-14) | Policy, not speed |
| Zobrist structure | TRI-R TB-2 (GF(2) rank 64) | not a speed item | rank 64 tested by B.7.1 | Closed |

## 4. Legality and move validation

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Legal test from stored blockers: `!(blockers & from) || line_bb(from,to) & king`; king moves by `attackers_to_exist` | SF `Position::legal` (position.cpp:662) | unmeasured | `Board::is_legal` (board.cpp:903) does the pseudo-legality and legality in one function, with two slider lookups plus knight and pawn tests for every non-king move; no pin information used | Used for the TT move (`table_move_usable`, move_picker_core.h:56), root and ponder checks; not among A.17's top 15 functions (smallest listed 1.58%); frequency unmeasured |
| Separate cheap pseudo-legal validator for TT and killer moves | SF `Position::pseudo_legal` (position.cpp:705); TRI-R TB-1 re-checks every parent move against it in `tdperft` | unmeasured | merged into `is_legal`; picker has a TT stage only (no killer stage) | Ordering contract is B.7.3's; a split changes nothing visible if the fingerprint holds |
| Pseudo-legal generation, legality at delivery | SF `generate<LEGAL>` filters only pinned, king and en passant moves (movegen.cpp end of file) | unmeasured | strictly legal generation with pin masks and check masks (`gen_legal_impl`, board.cpp:1066) | RAR representation_2026-09-08 rejected "selectively checked" generation on counters (gen+legal share 6.556%, pin work already shared); BAS's form is the donor-neutral choice |
| Evasion generator specialised on checkers | SF `generate<EVASIONS>` | unmeasured | `check_mask` inside the same function, double check returns after king moves | keep |

## 5. Move generation and staged delivery

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Side as template parameter, compile-time generation type | SF `generate_all<Us, Type>` (movegen.cpp); TRI-R TB-8 | TRI: part of a -10% cycles / -20% branch-miss bundle (TRI-D8 s4); RAR const-generic colour +7.38% on the legal-moves bench column (movelist_delivery_2026-09-09, RAR-M44(c)) | `gen_legal_impl<Us>` templated on colour; capture/quiet selection is a **runtime** pair of bools (`caps_only`, `quiets_only`) tested per piece | The bundle that carried const-generic colour measured **-0.55% [-0.76%, -0.30%] pooled-PGO**; do not bank the bench column |
| Quiet-only and capture-only passes without regeneration | SF `generate<CAPTURES>/<QUIETS>`; TB-8 | unmeasured | `gen_legal_captures` then `gen_legal_quiets`: two passes, each recomputes king moves, pins and slider lookups | Pin sharing between the passes measured -0.16% (BAS-P04); RAR already shares and counts 422,246 quiet passes at zero extra `compute_pinned` (pin_check_sharing_2026-09-08, NO_CHANGE) |
| Generate into the delivery buffer; no intermediate list or copy | RAR movelist_delivery_2026-09-09; RAR-D D-10 (RAR-P27/P28) | **RAR pooled-PGO +2.48% [+2.29%, +2.65%] (list returned by value, 520-byte memcpy at four sites, null pair -0.21%); RAR-P28 +6.93% [+6.38%, +7.68%] (six 4,104-byte copies per node, 5.08% of samples)** | `CaptureInit`/`QuietInit` generate into a stack `MoveList` (512 B, `Move` = `uint16_t`) passed by reference, then `fill()` rewrites every move into `moves_` as a scored entry (move_picker_core.cpp:56, 153-188); `fill` = 1.75% exclusive (A.17) | Not Rarog's memcpy (Basilisk passes by reference), but the same shape: the second pass over the list. Share is on record; gain is a prior. Needs a disassembly or profile read first (RAR-D D-10 "profile first") |
| Quiet-stage early exit when quiets are skipped | RAR-D D-11 (RAR-P27/28 candidate 1) | RAR +8.85% [+8.17%, +9.48%] (60% of scanned elements and 64% of picks produced nothing under skip-quiets) | with `keep_direct_checks_` the picker walks all remaining quiets calling `direct_check` (move_picker_core.cpp:194-224); without it exits at once | Partial; RAR-D marks "profile decides" |
| AVX-512 compaction in generation | TRI-D8 s20 | TRI avx512 -1.51% cycles; AVX2 form +0.29% (slower), not kept | none | Not for the AVX2 / PEXT tiers |
| Branch-free best-quiet selection; rank by count of larger keys | TRI-R TB-8 (selecting the best quiet caused ~11% of branch misses) | TRI -20% branch misses, -10% cycles with two neighbours (TRI-D8 s4); AVX-512 rank -0.70% | `partial_insertion_sort` already selects with a branch-free scan (move_picker_core.cpp:6) | Partly HAS; ordering, not board |
| Victim from the mailbox in capture scoring | TRI-R TB-8 (sign of its "+0.7-1.0%" not stated in the review row) | unmeasured here | `capture_order_score` reads `piece_on(to)` (move_picker_core.cpp:65) | HAS |
| Enemy attack map shared by generator and quiet scoring | TRI-R TB-4; SF `movepick.cpp` `threatByLesser` | TRI -0.35% cycles | `threats_against(board, us)` built lazily at the first non-king, non-pawn quiet (move_picker_core.cpp:121) | Partly HAS (lazy, per picker) |
| Quiet-check generation | none: SF19's `movegen.cpp` has no quiet-check type | n/a | `gen_quiet_checks` generates all legal moves and filters by `gives_check` (board.cpp:1482), used at `search_kernel_core.cpp:582` | Unmeasured; called in quiescence only; frequency not on record |
| Stack frames over 4 KB (`__chkstk`) | TRI-R TB-11; TRI-D8 s22 | TRI: -1.55% cycles (queue fill out of line -0.81%, with the evaluation frame) | `MoveList` is 512 B; frames in the picker are small by inspection; no disassembly read on record | Review section 6 item 2 still open; cheap to close |

## 6. SEE

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Threshold SEE with both bounds before any lookup, pins from stored pinners and blockers, non-normal moves pass | SF `Position::see_ge` (position.cpp:1389) | unmeasured | `Board::see_ge` (board.cpp:1857): promotion, en passant and castling handled; exits `swap < 0`, `swap <= 0 && !promo`, and "no enemy attacker" before pins; pin filtering through `see_pins` on the exchange occupancy | Basilisk's pin model is exact (B.7.1: `see_ge` exact on all 187 moves) where SF's is the stored original-position pins; both departures are intended (BAS-C08/C09) |
| Pins at the exchange occupancy | Basilisk-only; RAR 4.11b.5 selected-king legality | RAR per-function (RAR-M36): `see_recapturer` (king legality) 4.35% exclusive vs `see_ge_impl` 0.87%; SEE region 5.239% | `see_pins` = 4 slider lookups per colour (8 per call) whenever an enemy attacker exists, recomputed when a departure opens a king line (board.cpp:1663, 1982) | The costly part of Rarog's SEE is the legality test; Basilisk's analogue is `see_pins` (cost unmeasured here). RAR concluded the target is the per-candidate legality test, not the attacker set |
| Incrementally carried attacker set; only the revealed ray added | SF `see_ge`; RAR-M35 | **RAR bench threshold-SEE only -2.92% median, 3 rounds, 0 up (-0.69/-10.42/-2.92%); RAR-P33 incremental SEE -2.09% (as quoted in RAR-D D-20, unit not stated there)**; stage 2 not run | `see_ge` already adds only the revealed bishop/rook ray (board.cpp:1987-1995) | HAS; Rarog's loss came from computing both colours at entry and from the per-candidate king test, which Basilisk's form does not have. Do not retry on donor similarity |
| Next attacker found by index | TRI-R TB-6; TRI-D8 s36 | **TRI cycles/node -0.38% mid, -0.14% endgames** (adopted after a first rejection) | scans `pt = PAWN..KING` with a pin filter per type (board.cpp:1958-1965) | Prior; small; B.7.1's SEE corpus guards it |
| SEE of a quiet move skipped when the enemy attack map decides it | TRI-R TB-6; TRI-D8 s27 (28% of SEE calls); TB-6 | TRI: the eight-patch package -0.72% mid / -0.64% endgames (TRI-D8 s27, s36); **RAR-D D-16: the same shortcut was a -0.58% member inside a +0.15% bundle** | quiet moves go through `see_ge`'s loop; the "no enemy attacker" exit handles the unattacked case, no attack-map exit; the picker calls `see_ge(m, -75)` for checking quiets only | Conflicting priors; B.7.2 must read it alone |
| Full `see()` (gain array) for ordering | BAS-C09/BAS-D | n/a | no caller left in `src/` (B.7.1 F4) | B.8.2 deletes it |
| Both slider attacks from one lookup (queen branch) | SF `see_ge` queen branch uses `both_attacks_bb` | see section 7 | `bishop_attacks` and `rook_attacks` separately | PEXT gives no sharing |

## 7. Attack tables

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Bishop and rook together (dual hyperbola quintessence on AVX2, plain magic otherwise) | SF `attacks.h` `DualMagic::both_attacks_bb`, `both_attacks_bb`, used by `set_check_info`, `attackers_to`, `see_ge`, EP in `gives_check` | SF unmeasured. TRI: slider computations per node 9.8 -> 5.1 mid, 11.3 -> 5.7 endgames (TRI-D8 s40 AA1); **TRI cycles/node: removing DualMagic +0.33% mid / +0.28% endgames (so it was worth that, TRI-D8 s36); AVX2 `pair_hqs` -0.23% / -0.05% (TRI-N); AVX-512 obstruction difference -0.98% / -1.20%** | PEXT or baked magics, flat tables, `bishop_attacks`/`rook_attacks` out of line in `attacks.cpp` (178-190); `queen_attacks` is two calls | TRI deliberately left PEXT tables (2.25 MB, one random read per attack) which is a bigger table than Basilisk's 840 KB flat PEXT (BAS-P26); AMD PEXT is slow on some families (TRI-D8 s35) |
| Empty-board attack tables for snipers and pin detection | SF `update_slider_blockers`; TRI-R TB-7 | **RAR M31 (movegen_2026-09-07): isolated legal generation +8.54%, captures +11.43%, staged +7.41%; whole search +0.57% [-0.51%, +1.00%] generic, +1.45% [-1.57%, +4.26%] PEXT, neither excludes zero; withdrawn** | pin detection in `gen_legal_impl` is Rarog's baseline form: four occupied-board slider lookups (board.cpp:1120-1141) | Basilisk's pin code is the form Rarog withdrew the candidate from; a retry has to state a floor first (RAR-M31 retry trigger) |
| Compressed slider tables | BAS-P26 | **BAS: -0.98% [-1.19%, -0.78%], six cycles; `-mtune=znver3` -0.95%; native -1.04%; v3 flags +0.02%** | flat tables stay (PLAN B.7.2 text) | **Measured loss**; slider lookups 2.7-3.2% of samples bound the prize |
| Per-line slider tables | TRI-D8 s17 | TRI cycles/node **+0.55%, withdrawn** | n/a | Loss prior |
| Compile-time tables, no lazy check, no heap pointer | RAR-D D-12 | RAR +2.36% screen, +1.46% read (binary 1.01 -> 1.85 MB); part of that was a `LazyLock` check (0.6-0.71% of samples) Basilisk does not have | plain global arrays filled at start-up; lookups load `MagicEntry::attacks` and are out of line, so inlining depends on LTO; A.17 shows `bishop_attacks` as its own function at 1.58% | PARTIAL; low priority per RAR-D; a PGO disassembly read says whether callers inline it |
| NEON / hyperbola paths | SF `USE_HYPERBOLA_QUINT` (aarch64) | unmeasured | PEXT or magic | G.2 platform item |

## 8. Repetition and draw detection

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Repetition distance stored in the state at make (one scan over the reversible window); `is_repetition` is a field test | SF `do_move` tail loop, `is_repetition` (position.cpp:1506), `has_repeated` | SF unmeasured (a scan at every make) | no field; `Board::is_repetition` (board.cpp:1562) scans the key history by stride 2 at each call, after `halfmove_clock >= 4` | Basilisk pays only where asked. The one measured share: `upcoming_repetition` is 0.30% of samples, Wilson [0.21%, 0.42%] (PLAN B.3.5 P9, "keep the donor form" decided 2026-11-04) |
| Cuckoo upcoming-repetition test | SF `upcoming_repetition` (position.cpp:1527) | as above | `Board::upcoming_repetition` (board.cpp:1589) is the donor form over the key history | HAS |
| Rule-50 with mate precedence | SF `is_draw` | unmeasured | `rule50_draw` generates legal moves only at clock >= 100 in check (board.cpp:1532) | HAS |
| One return to the root scored as a draw | SF `is_repetition(ply)` requires strictly after the root | policy, not speed | Basilisk scores one return to the root as a draw (B.7.1 F1, for the maintainer) | Not a B.7.2 speed item; any storage change must carry the F1 outcome |
| Allocation guard for the history | RAR-D D-22/D-23 | RAR: history growth 0 events in 25,718,154 pushes (board_search_profile_2026-09-07) | HAS (`SEARCH_HISTORY_HEADROOM` repaired by B.7.1) | Closed |

## 9. Representation and caches

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Six type boards plus colours (one load plus AND per `pieces(c,pt)`) | SF `byTypeBB/byColorBB`; RAR representation_2026-09-08 (RAR-M39) | RAR: argued, **unmeasured**: Board 264 B -> 216 B; 208 call sites, 102 in eval; NO_CHANGE | 12 colour-piece bitboards + 2 occupancies + `all_occ` + mailbox + `king_sq` + `checkers` (board.h:51-70) | Reopen only for a single region above 12% (RAR-M39 trigger); Basilisk's board region is 26.7% by A.17's own mapping |
| Per-ply full-state copy instead of compact undo | Reckless `InternalState` (cited in RAR board_audit) | RAR: 128 x 264 B = 33 KiB vs 3 KiB for a 24 B record (arithmetic, not a timing) | compact record on a vector | Keep |
| Chess960 representable: rook-origin and path tables, castling as king takes rook, XOR occupancy update | SF `castlingRookSquare/castlingPath/castlingRightsMask` (position.h members); TRI-R TB-9, TRI-D8 s14 | TRI-R TB-9 prints "no speed cost (+0.57%)" without a sign convention; FRC perft suite at depth 4-5 | castling hard-coded: king E1/E8, rooks H/A, `CASTLING_MASK` constant (board.h:278), in `make_move`, `unmake_move`, `gives_check`, `is_legal`, `can_castle_*` | B.7.2 designs for it, G.2 decides the feature (PLAN) |
| One board for search and network, with the network reading make's record | TRI-D8 s28 | TRI cycles/node -0.18% mid / +0.43% endgames: **neutral, kept for simplicity** | one `Board`; NNUE not yet built | See section 10 |
| Material and PST sums incremental in make | BAS A.17 note (PLAN B.8 lead) | BAS: sums over every piece are 1.37% of a node (A.17, via PLAN B.8.5 text) | recomputed per evaluation | Evaluation item, B.8.5's, not the board's |

## 10. NNUE-facing dirty-piece record

| Technique | Donor and location | Measured effect | Basilisk today | Relevance |
|---|---|---|---|---|
| Make writes a per-move record: moved piece, from, to, captured piece and square (en passant pawn square), added piece/square (promotion, castling rook) | SF `DirtyPiece` filled in `do_move` (dp.pc/from/to/remove_pc/remove_sq/add_pc/add_sq, position.cpp:~845-960), plus `DirtyThreats` and `DirtyPawnPairs` in `Dirties` | SF unmeasured | `UndoInfo::captured` only; `search_worker.h:337` comment says per-ply bookkeeping, "later an NNUE accumulator push/pop", attaches there | B.7.2 fixes the contract (TB-10); F.1 builds on it |
| Record only the moved piece at make, compute threats and pawn pairs lazily before an evaluation by replaying from the current board | TRI-D8 s28 variant (c); `TRI-src/nn_dirty.h` `NnDirtyPiece` on `NnStack` (247 states, null moves add none), `search/06_nndirty.inc` `nn_dirty_catch_up` | **TRI instructions/node against the old code: first version +4.19% mid / +4.64% endgames; (a) eager +2.68 / +3.40; (b) lazy copy +2.71 / +3.19; (c) moved piece only +0.60 / +1.13; (c) with branch-free refresh +1.21 / +1.66; final cycles -0.18% mid / +0.43% endgames** | none | Eager threats in make cost more than lazy because Basilisk's make, like TRI's, already existed and does its own work; the evidence favours (c) for a make that serves a search |
| Branch-free accumulator-refresh comparison | TRI-D8 s28 | TRI mispredictions +6.2% -> +1.9% | none | F.1's, not B.7.2's |
| Dirty state outside the board, per worker per ply; no full accumulators in Board/undo | RAR board_audit_2026-09-05 (5.2, 5.3) | design; "do not put full accumulators inside Board/undo" | `UndoInfo` has none | Consistent with TRI's stack |
| Verification build comparing the network stack to the board | TRI `TRIUMV_VERIFY_NNSYNC` (`07_makemove.inc:494`, `06_nndirty.inc:549`) | caught a deliberately broken make at the first bench (TRI-D8 s28) | B.7.3's TM-10 switch rule | Reuse as the shape of B.7.3's per-component check |

## Methods the sources used for board speed

| Method | Source | Shape | Note |
|---|---|---|---|
| Cross-engine microbenchmark `cross-engine-board-v1` | `tests/board_performance.cpp`; RAR board_benchmark_recipe_2026-09-05 | 5 positions; 150 ms warm-up, 11 x 150 ms samples, median +/- MAD; work frozen: 128 legal moves, 10 captures, 128 make/unmake, 10 SEE, perft(4) 197,281, two-ply 4,597; preflight refuses other work; three cyclic engine orders, host-busy gate | RAR-M20/M44(d) read Basilisk 642.6 / 120.0 / 58.2 / 60.4 / 402.6 / 536.7 M ops/s (legal, captures, make/unmake, SEE, perft(4), two-ply), non-PGO, Ryzen 9 5950X; Rarog trailed by 33.2% legal, 12.3% make/unmake, 35.0% SEE, led captures by 5.8% after removing its own list copy (board_comparison_411b19_2026-09-09). BAS-X16: Basilisk 22-46% ahead of the pre-fix Rarog; it "rejects a broad board-throughput deficit" |
| Isolated primitives `board_v2` | RAR board_v2_instrument_2026-09-06; RAR movegen_2026-09-07 | five separate columns: legal generation, capture generation, staged generation, make/unmake only, threshold SEE only; unchanged columns as noise controls; allocation test | Basilisk has none of the five isolated; its make/unmake column includes `gen_legal`, its SEE column includes capture generation (Rarog's finding 3 applies as is) |
| Counter profile at fixed nodes | RAR board_search_profile_2026-09-07 | diag build counters: 30,604,224 nodes over 20 roots; per-node call rates | Basilisk has `diag_gives_check_calls` and `diag_see_ge_calls` only (board.h:199); no counters for check info, pins, `is_legal`, make variants |
| Sampling profiler with inline attribution | RAR RAR-M30/M36; BAS A.9.5 `tools/diag/profile_regions.py`, A.17 report | per-address xperf samples, llvm-symbolizer inline chains, region map; RAR's trap: a per-function report silently charges inlined board work to `negamax` | A.9.5 exists; regions are file-mapped (`board` includes `move_piece`, `make_move`, `attackers_to`) |
| Pooled-PGO NPS reads | BAS `tools/nps_read.ps1` (BAS-P26, resolution about +/-0.5%); RAR-M33/M44 | 3-4 PGO builds per arm, alternating, null pair of same revision (RAR -0.21%, +0.06%) | RAR-M33's 32 pairs at 1.2M nodes gave half-width 1.003%, twice its prediction; RAR-D D-17: eight builds an arm resolve +/-0.2-0.25% |
| Hardware counters at an identical tree | TRI-R TM-7; TRI-D8 s2, s36 | cycles, instructions, mispredictions per node, pinned core, search threads only, deterministic PGO at fixed nodes, byte-identical copy as null (0.02-0.19% apart) | Basilisk's B.16.4 / BAS-P28 counter read is the same instrument (maintainer-run) |
| Shortcut verification builds | TRI-R TM-10 (`TRIUMV_VERIFY_SEEQ/RANK/SLIDERS/NNSYNC`); B.7.3 requires one per component | old form beside the new at every node, abort on first difference | Needed per replaced component |
| Perft through the search's own path | TRI-R TB-1 `tdperft` (482M nodes, 171 positions, 0 errors) | B.6.3's search-path perft | built |

**What `tests/board_performance.cpp` measures:** six workloads on the five FENs. **What it lacks** (to price
each component separately): staged quiet generation (the picker's `gen_legal_quiets`); make/unmake only on a
fixed list; SEE only on a fixed list; `gives_check` and `check_squares` per move; `is_legal` (TT-move validation);
`is_repetition` / `upcoming_repetition` on a long history; `see_pins`; board copy; unmake alone; in-check,
en passant, promotion and castling roots (Rarog's audit: the same five roots hold no checked side to move, no
legal en passant, no promotion; B.7.1's corpus of 26 tagged positions has them but no timing consumer);
Chess960 roots; a counter for per-node call rates. Rarog's calibration: the (c) bundle moved legal moves +12.7%
on the bench and measured -0.55% of search; (b) moved legal moves +11% and captures +41% and measured +2.48%
(board_comparison_411b19). A per-primitive column is a screen for a falsifier (RAR-D D-20), not a gate.

## Techniques the evidence ranks most promising for Basilisk

1. **Predicted-key TT prefetch before make.** TRI +2.21% NPS (TB-12), RAR +1.69% [+1.56%, +1.89%] (D-5); both
   positive, from other engines; Basilisk prefetches after make today; TRI's earlier issue of the same prefetch
   read +0.03% and the next-move variant cost on AMD (TB-16), so read it as one change under PGO.
2. **Remove the picker's second pass over the generated list.** `fill` is 1.75% exclusive in BAS A.17; RAR's
   same-shape removal read +2.48% [+2.29%, +2.65%] pooled-PGO and +6.93% [+6.38%, +7.68%] (RAR-D D-10); needs a
   profile or disassembly read at the current head first.
3. **Unmake: stop XORing keys that are then overwritten.** Basilisk code evidence (board.cpp:793-800) plus TRI's
   unmake series -0.61% cycles/node (TB-5, TRI-D8 s40); `make_move` 2.64% + `move_piece` 2.21% exclusive (A.17).
4. **A one-record dirty-piece contract written by make (moved piece only), with lazy replay.** TRI variant (c)
   +0.60% / +1.13% instructions against +2.68% / +3.40% eager (TRI-D8 s28); this is required for F.1 whatever
   its speed, so the evidence picks its shape.
5. **Slider lookups inlined or table-resident for the hot callers.** RAR +1.46% read for compile-time static
   tables (D-12); BAS A.17 lists `bishop_attacks` as its own 1.58% function and the attacks region at 5.4%; read
   the PGO build's disassembly first, since PGO may already inline. Compressed or per-line tables are not
   candidates (below).

Also keep in the packet, as design inputs without a speed claim: Chess960-ready castling tables (TB-9),
`see()` deletion (F4), the F1 repetition policy, and the `__chkstk` disassembly read (TB-11).

## Measured as loss or neutral (equally decisive)

| Technique | Result | Source |
|---|---|---|
| Per-node `CheckInfo` + check-hinted make (Basilisk) | **-1.8% to -2.7% NPS**; rejected, retried only on a new profile showing changed reuse (EXPERIMENTS §9, owner B.7.2) | BAS-P04, CHANGELOG 1.9.1 |
| Pin sharing across generation stages (Basilisk) | -0.16% NPS | BAS-P04 |
| Compressed PEXT slider tables (Basilisk) | **-0.98% [-1.19%, -0.78%]** | BAS-P26 |
| Native / `-mtune=znver3` builds (Basilisk, Zen 3) | -1.04% / -0.95%; the loss is the tuning model, not the instructions (v3 flags +0.02%) | BAS-P26 |
| Bundle: const-generic colour + forced inlining in generation (Rarog) | bench legal moves +12.7%, **pooled-PGO -0.55% [-0.76%, -0.30%]**, reverted | movelist_delivery_2026-09-09 (RAR-M44(c)) |
| Unchecked `MoveList::push`; six `LazyLock` checks hoisted (Rarog) | bench -9.57% and -4.25%, reverted | same, (c) screen; RAR-D D-15 |
| Empty-board pin detection (Rarog) | bench +8.5% to +11.4%; whole search +0.57% and +1.45%, both intervals include 0, withdrawn | movegen_2026-09-07 (RAR-M31) |
| Incremental attacker set in SEE (Rarog) | bench -2.92% median, 0 of 3 rounds up | see_kernel_2026-09-08 (RAR-M35) |
| Quiet-SEE skip inside a bundle (Rarog) | -0.58% as a member of a +0.15% bundle | RAR-D D-16 |
| King-square cache (Rarog) | 0.502% share, 2x ceiling 0.25%, closed | king_square_cache_2026-09-08 |
| Six type boards, per-ply state copy, selective legality (Rarog) | rejected on footprint and counters, **not timed** | representation_2026-09-08 |
| Fused relocation first run (Rarog) | void: host 50.2-53.4% busy; rerun on an idle host accepted +0.876% | relocation_2026-09-07 |
| History, correction, store-side prefetches under 4 KB pages (Rarog) | -3.1%, -0.4%/-0.6%, -0.26% | RAR-D D-7 |
| Slider tables per line; branch-free capture in make/unmake; slot-remembering TT probe; AVX2 compaction; child-hash prefetch issued earlier (Triumviratus) | cycles/node +0.55% (loss); -0.13% (noise); +0.42% (loss); +0.29% and +2.29% (loss); +0.03% (neutral) | TRI-D8 s17, s20 |
| AVX-512 gathers; threat-PSQT prefetch loops; "one board" for search and network (Triumviratus) | as costly as sixteen scalar loads; switched off (-1.32% cycles to remove); -0.18% / +0.43% neutral | TRI-D8 s20, s17, s36; TRI-R TB-16 |

## Open items this inventory could not close

- No Basilisk share exists for `gives_check`, `is_legal`, `see_pins`, `fill`-versus-generation split, `is_repetition`
  or the make-time `attackers_to` at the current head; A.9.5's profile at B.7.2 supplies them.
- BAS-P04 does not record its pools or intervals; only the CHANGELOG range is on record.
- RAR-P33's "-2.09%" and TRI-R TB-8's "+0.7-1.0%" do not state the unit or sign convention in the text read.
- Rarog's counts (hinted make 89.02%, fast `gives_check` 99.38%) come from a different search; Basilisk's
  frequencies are 1.43 `gives_check` and 0.57 `see_ge` per node.
