# B.9.4 Deferred defects: repair now or justify

Leaf: PLAN B.9.4 `[R2]`, 2026-11-16. Head: `2b2b98e` plus the routing
commit `9afb2d2` (branch `b9-robustness`), `bench 13` 4,688,537. No engine
source changed.

## 1. The rule and the inputs

Each defect that PLAN §2's *Known defects* gives to a leaf after 1.11.0 is
read once against one rule: it is repaired in B.9.5 when it is in the
shipped engine, has a test that fails, and needs neither a refit nor a
redesign that a later leaf owns. Otherwise it stays with its owner, and the
table and the release notes say why.

The inputs:
- PLAN's list: A-46, A-45, A-59, C-10 to C-13 and BAS-C05.
- What B.9.1 sent here (`analysis/b91_lifecycle_audit_2026-11-16.md` §5):
  A-57, and DRIVER-15, which joins A-46.
- What the maintainer routed on 2026-11-16: B.9.3's V3 to V6
  (`analysis/b93_hard_verdicts_2026-11-16.md` §5). These are listed here,
  not read again.

## 2. Reading

| Defect | In the head | Reading | Decision |
|---|---|---|---|
| A-45 no sub-second horizon | `search_worker.cpp:78-79`, `mtg = 50` in sudden death | A time formula. Changing it changes the time every move gets, a playing change D.1 makes in Stockfish's whole shape (`mtg` from the clock, the scale factors), refitted there. No defect test: the engine does not lose on time from it. Rarog's forfeits were OS holds that no horizon covers. | D.1 keeps it |
| A-46 reserve and hold | `search_worker.cpp:121-125`; `search_thread_pool.cpp:236`, `bestmove` after every helper joins | The reserve is a fitted safety margin (`2·overhead + 30` above one thread), D.1's to refit. The join before `bestmove` is Stockfish's form: its main thread waits for every helper before it prints. Answering before the join would let a helper still write the shared table and counters into the next search, a design change D.2 owns. B.16.5 already shortened the wait. | D.1 and D.2 keep it |
| DRIVER-15 a join behind a table read | as A-46 | A thread stalled in a page fault cannot be cancelled, by Basilisk or by Stockfish. The bound is the disk's, not the engine's. Same owner and reason as A-46. | D.2 keeps it, a known limitation |
| A-57 cold table reads | `syzygy.cpp`, the root probes after `go` | Measured in B.9.1: 4,006 ms on the first search after loading the 3-6-man tables. No engine can avoid the page-cache miss: the probes must follow `go`. Move Overhead is the remedy. | D.3 keeps it, a known limitation |
| A-59 DTZ ranking | `syzygy.cpp:199`, Fathom's `tbRank` | Stockfish's own form: every clean win within the fifty-move rule shares the best rank, and the search chooses among them, its evaluation driving the mate. B.9.5's D13 repair ranks by DTZ once the game has repeated, which is the case that cost game 4965. No game is known lost to the rest. | D.3 keeps it |
| C-10 to C-13 | `eval.cpp`, outposts, the rear doubled passer, "opposed", king blockers | Definitional evaluation repairs. Each moves fitted neighbours, so each needs a refit that Phase C owns (C.3, C.5, C.6). By the maintainer's 2026-11-16 rule, evaluation errors that do not stop play go to Phase C. | Phase C keeps them |
| BAS-C05 TT publication | `tt.h` | Both repairs were correct and both lost Elo on measured cost (−3.91% and −1.22% NPS). The retry trigger is a TT redesign, D.2's. Already a known limitation since 1.10.0. | D.2 keeps it, a known limitation |
| B.9.3 V3, V4, V6 | `eval.cpp`, the KRP-KR rules, the KRPP-KRP table, the pawnless scales | Routed to C.7.5 by the maintainer, 2026-11-16 | C.7.5 keeps them, known limitations |
| B.9.3 V5 | `eval.cpp`, KBN-K's known win | Kept with a retry trigger, 2026-11-16 | C.7.6 holds the trigger |

No defect meets the rule. Each one either changes play through a formula a
later leaf refits (A-45, A-46, C-10 to C-13, V3, V4, V6), is the donor's
form (A-46's join, A-59), cannot be repaired in any engine (A-57,
DRIVER-15), or was repaired and refused on its measured cost (BAS-C05).
B.9.5 takes nothing from this leaf.

## 3. Known limitations for 1.11.0

These go into the release notes (`CHANGELOG.md`, *Known limitations* under
`[Unreleased]`), in plain language:

- The first search after tablebases load can take seconds while the
  system reads the files (A-57). Raise Move Overhead on a slow disk.
- The engine waits for every search thread before it answers, so a thread
  stalled reading a table delays the move (DRIVER-15, A-46).
- Transposition-table coherence (BAS-C05), as in 1.10.0.
- Some endgames the evaluation reads as draws are wins, and the reverse
  (B.9.3's V3 to V6): rook-and-pawn endings, some pawnless endings, the
  bishop and knight mate against a bare king. The search corrects most of
  these within a few moves, and tablebases correct all of them.

## 4. Decision

B.9.4 closes with no repair for B.9.5. PLAN's *Known defects* table names
the owners as above, and the release notes list the limitations of §3.
