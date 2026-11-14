# B.7.1 — board contract audit and instruments (2026-11-14)

B.7.1 is the defect hunt before the board review (B.7.2), and the safety
net the rebuild (B.7.3) works behind. It builds an external oracle corpus
for the board, turns the Rarog audit's and the Triumviratus review's board
items into permanent tests, and audits the draw policies one at a time. It
changes no playing behaviour. `bench 13` is unchanged at **4,278,307**.

## 1. Instruments

| Instrument | What it checks | Independence |
|---|---|---|
| `tests/data/board_v2.tsv` | 26 positions with coverage tags: single and double check, evasions, legal, pinned and phantom en passant, en passant as an evasion and as a direct and a discovered check, every castle, castling refused through an attacked square, castling that checks, discovered checks, a pin, quiet and capture promotions, all underpromotions, an underpromotion that checks where the queen does not, stalemate, mate, a sparse long-clock ending, and the six published perft positions | the tags are checked by python-chess, so a FEN that stops showing its tag fails the generator |
| `tools/diag/board_v2_oracle.py` → `tests/data/board_v2_oracle.tsv` | for each position: canonical FEN (en passant only when a legal capture exists), checkers, sorted legal moves, captures, checking moves, perft and divide | python-chess 1.11.2, pinned; `--check` (default) fails on a stale file; CRLF copies compare equal |
| `tools/diag/see_oracle.py` → `tests/data/see_v1.tsv` | 192 SEE cases: every fixture `test_board.cpp` asserts (32 with a value), the moves its sweeps visit, Rarog's contract and repair cases re-valued with Basilisk's values (100/300/300/500/900) and their colour mirrors. Each has the legal same-square exchange tree (any legal recapture, every promotion, either side may stop), the least-valuable-legal-attacker chain, and the immediate gain | python-chess; no engine code; the tree and the chain agree on all 192 |
| `tests/test_board_v2.cpp` (CTest `test_board_v2`) | below | reads only the two oracle files |

`test_board_v2` checks the following.

1. **At the root of each position:** FEN, checkers, legal set, captures,
   checking moves, perft and divide all equal the oracle. The input FEN
   gives the canonical position's key, so a phantom en-passant token adds
   no key. The staged generators partition the legal moves: tactical
   (captures and every promotion), quiet, and quiet checks a subset of
   the quiet moves.
2. **At every node of a walk to each position's perft depth**, 12,451
   interior nodes:
   - `assert_ok` holds after every make and null move (it recomputes
     every key from scratch);
   - unmake restores every field, and so does a null move followed by its
     unmake;
   - `gives_check` equals playing the move: 0 mismatches over 9,498
     checks;
   - the FEN round trip gives the same keys, checkers and en-passant
     square: one position, one key (TB-3).
3. **Clones, history and unwinds:** a clone has the same perft and leaves
   the original untouched. A clone taken mid-line unwinds through the
   copied history. A deterministic walk of 160 plies, with a null move
   every seventh ply, unwinds state by state to the root.
4. **Zobrist (TB-2):** the 793 keys (768 piece, side, 16 castling, 8 en-passant) have GF(2) rank 64, and no two
   non-zero keys are equal.
5. **FEN robustness (TB-3):**
   - an en-passant square on the wrong side's rank, with no pushed pawn,
     or on an occupied square is refused, and the board is unchanged;
   - castling rights the board cannot hold are dropped, giving the same
     key as their twin without them;
   - a position whose side not to move is in check is refused.
6. **Position identity (A-14):** the halfmove clock is outside the key,
   and a repetition is found across a clock that moved on.
7. **History capacity (archived 7.7):** a game history of any length is
   kept. Every copy reserves `SEARCH_HISTORY_HEADROOM` (2 × `MAX_PLY`)
   past it. A full search line on a copy does not reallocate.
8. **SEE corpus (A-5):** `see_ge`'s boundary equals the legal exchange
   tree on all 187 non-castling moves. `see()` equals it except for the
   two named cases in §3 F4.
9. **Size guards (A-24):** `sizeof(Move) == 2` and `MoveList` holds at
   least 218 moves.

**Negative controls.** Each comparison is shown to fail:

- an oracle row with the en-passant capture removed, a perft total off
  by one, or the canonical FEN without its en-passant square is rejected;
- a make or a null move that is not undone is seen;
- `gives_check` without its discovered-check clause disagrees 161 times
  over the same walk (Rarog's D.5.17 differential, shown to have power);
- keys cut to 32 bits read rank 32, the Triumviratus defect.

**A-13**, in `test_search`: a stored bound never answers a position that
repeats the game history. A PV node takes no table cutoff, and a fail-high
at the root is re-searched as PV, so a stored *win* cannot mislead the
root. The live hazard is a stored claim that makes a saving repetition fail
low under a null window, which is never re-searched. The fixture is
`r5k1/8/8/8/8/2N5/4R3/6K1 w`, with two root moves, Re8+ (the root's TT
move, searched first) and Kh1. Kh1 reaches a position seen twice before the
root, and the table holds a Black win for that position.

| History | Poisoned | Best move | Score |
|---|---|---|---|
| no | no | Kh1 | +117 |
| no | yes | Re8+ | −37 |
| yes | yes | Kh1 | 0 |

The control row, without history, shows the poisoned entry answering. So
the test fails if the draw check ever moves after the probe.

The search-board perft (TB-1, `tdperft`) is B.6.3's `test_search_path`,
reused as the safety net. No second one was built.

## 2. Policy audit: one policy at a time, against `sf_19` (`edb0d9db`)

| Policy | Basilisk | Stockfish 19 | Verdict |
|---|---|---|---|
| Rule 50 | `rule50_draw`: clock ≥ 100 is a draw unless in check with no legal move, which is mate | `rule50 > 99 && (!checkers \|\| legal moves)` | same |
| Null move and the clock | the clock does not advance; `plies_from_null` resets; the side key toggles; en passant cleared | `memcpy` keeps `rule50`; `pliesFromNull = 0` | same |
| Repetition window | `min(rule50, plies_from_null)`, stepping 2 | the same `end` | same (Basilisk also compares distance 2, which can never match: a wasted compare) |
| Threefold before the root | two earlier matches in the window | `stp->repetition` of the earlier occurrence | equivalent within one window |
| Twofold in the tree | `distance <= search_ply`: a single return to the **root** counts | `repetition < ply`: strictly after the root | **departure, F1** |
| Upcoming repetition (cuckoo) | `search_ply > i`, else the cycle must already have repeated | `ply > i`, the same | same as the donor, so it disagrees with Basilisk's own `is_repetition` at the root cycle (F1) |
| Mate precedence at the draw checks | the draw check runs before the cap, the TT probe and mate-distance pruning, in negamax (`!is_root`) and quiescence | the same order | same; A-13's test now pins it |
| Insufficient material | K v K, K+minor v K, KB v KB with same-coloured bishops (dead positions) | none in `is_draw` | Basilisk-only, sound: each is a dead position |
| TT key and the clock | `position_key` has no clock; `tt_key50` (the donor's rule-50 bucket) exists, default off | `adjust_key50` from 14 | the repetition key is raw in both (A-14); the bucket is B.3.0's open row |

## 3. Findings

- **F1 (semantic, not repaired; correctness-repair path).** At the root
  cycle Basilisk's two repetition predicates disagree. `is_repetition`
  scores a single return to the root position as a draw (`distance <=
  search_ply`, pinned by `test_board`'s "search repetition distinguishes
  root and in-tree repeat"). The cuckoo test, B.3's import in the donor's
  form, does not raise alpha for the same cycle. Stockfish does neither:
  "repeats once earlier but strictly after the root, or repeats twice
  before or at the root".
  - So the search can score a move as a draw once it is played, while the
    pre-emptive bound one ply up does not see it.
  - Each form is defensible; the inconsistency is not.
  - No record of why Basilisk chose the root form was found (it predates
    1.9.1).
  - The repair is a choice of policy: the donor's strict form, which
    matches the cuckoo, or Basilisk's form carried into the cuckoo. It
    changes the tree, so it needs a gate.
  - Proposed owner: B.9.4 reads it and B.9.5 repairs and gates it with
    the release's other repairs.
  - **Repaired 2026-11-14 at the maintainer's word, in B.7.1** (`83648bb`):
    the donor's strict form, which the cuckoo test already assumed. A
    `test_board` section holds the two predicates to one answer before,
    at and after the root; it fails on the head at the root cycle.
    `bench 13` 4,278,307 → **4,247,965**.
    - The fixture `test_b5_root` "a proven mate stands" moves with the
      tree (`d75bc21`): the head proves it in 8 from depth 17, the repair
      in 9 from depth 19 and in 8 from depth 24.
    - Exposure by `tools/diag/decision_diff.py`: 12.1% of 3,000 game
      positions choose another move at 50k nodes (a self pair reads 0).
    - Gate BAS-S63, `[-5,5]`, cap 4,000 pairs.
- **F2 (defect, repaired here, behaviour-neutral).** `Board::operator=`
  copied the history with `history = other.history`. A game history
  longer than about 2,048 − `MAX_PLY` plies left a copy with capacity
  equal to its size, so the search grew it at its first node.
  - That is an allocation in per-node code (`agents/implementation.md`),
    in long games only.
  - The copy now reserves `max(HISTORY_RESERVE, size + 2 × MAX_PLY)`
    first.
  - The new test fails on the old code (two of its three checks) and
    passes on the new.
- **F3 (test defect, repaired).** `test_board`'s SEE fixture "pinner stays:
  pin holds during exchange" (`4k3/8/8/4n2B/6p1/7P/4Q3/K7 w`) was not a
  legal position: the h5 bishop checked the black king, with White to
  move. Its bishop moves to f3 (`4k3/8/8/4n3/6p1/5B1P/4Q3/K7 w`, Bxg4).
  The e2 queen still pins the knight and the value is still +100. The
  python-chess oracle found it.
- **F4 (observation, B.8.2's).** `Board::see` has no caller in `src/`; the
  search uses `see_ge`.
  - Its comment justifies the inexact gain-array prune by "see() only
    feeds move ordering", which no longer holds.
  - Two kiwipete quiet moves (Qf3-f5, Qf3-h5) read −800 against the
    tree's −900. After the bishop's X-ray recapture, the prune stops
    before Black's knight or rook re-recaptures.
  - The corpus test names both, so a change in either direction shows.
  - Deleting or demoting `see()` to a test oracle is B.8.2's dead-code
    sweep.
- **F5 (scope note).** Basilisk has no hinted make, so the archived
  leaf's "hinted" restoration path has nothing to test. The staged path
  is the generators' partition here, and the move picker through B.6.3's
  search-path perft.
- **F6 (no defect).** Legal, capture and check sets, perft, keys,
  restoration, FEN canonicalisation, castling sanitising and the Zobrist
  rank all agree with the oracle. Rarog's D.5.17 `gives_check` result is
  reproduced: 0 mismatches over 9,498 checks.

## 4. Planned instruments this research could use

| Instrument | Would it inform B.7.1? | Verdict |
|---|---|---|
| B.6.3's search-path perft (TB-1) | the picker's staged delivery and the TT-move validator | used as built |
| A.9.5's profiler, per-component microbenchmarks | component shares | B.7.2's, not B.7.1's |
| TM-10's old-beside-new check | per-node equivalence of a rebuilt component | B.7.3's; this corpus is its fixed-input half |
| B.8.3's allocation guard | would have caught F2 mechanically in a long-history search | lower value now that F2 has a direct test; B.8.3 stays where it is |

## 5. Reproduce

```bash
python tools/diag/board_v2_oracle.py
python tools/diag/see_oracle.py
cmake --build build/release --target all
ctest --test-dir build/release --output-on-failure
```
