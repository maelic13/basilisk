# Rarog import and roadmap review, 2026-11-11

- State / class: delivered; the source of the PLAN changes of 2026-11-11, `R3`
- Owner / date: agent, 2026-11-11, at the maintainer's request (roadmap
  cohesion, a second Rarog import, a bug scan; analysis only)
- Decision needed: the maintainer's sign-off on the PLAN changes this record
  lists; nothing in the engine changed

**Recommendation.** Take the changes as written: the search fit (the joint
SPSA) now runs after an engine audit and its repairs instead of before any
of them, every correctness item sits before the 1.11.0 cut, and Phase F no
longer trains before it can read a network. Rarog's D.6 turned out to be
mostly Basilisk's own findings travelling back, so the import adds few
defects and more order.

Every claim is **evidence** (a line read, a program run) unless marked
otherwise. "Fetch" marks a claim read by one of two read-only Sonnet agents
and not re-read by the session; "read" marks one the session read itself.
A fetch claim is an input to its leaf, which confirms it with a failing
test before any repair.

## 1. Scope and method

- **Rarog**, the live repository at `8c0e4cb` (2026-11-11), under A.12's
  exception, at the maintainer's request: `docs/PLAN.md` whole (Phases D to
  H; A to C are summaries there), the sub-step list and D.5.17 to D.5.23 of
  `docs/archive/PLAN-phaseD-done-2026-11-04.md`, `GUIDE.md`. The last sweep
  read `75f1c9b` (`analysis/rarog_sweep_2026-11-04.md`); what is new since is
  D.5's closure, D.6.1 to D.6.4's results, D.6.16 to D.6.27, D.8.20, H.4 and
  the reorder of Phase F. Two uncommitted ledger files in Rarog's tree were
  not read.
- **Basilisk** at `b75ffea` (branch `b5-root-aspiration`, B.5.4 open, its
  SPSA paused): PLAN's open leaves read whole, the closed ones by their
  heads.
- **Two Sonnet fetches**, read-only, no build and no run: the kernel against
  fifteen questions from Rarog's D.6, and lifecycle, protocol, endgame
  verdicts and CI against nineteen from Rarog's D.3, D.5 and D.9.
- **Read by the session**: the correction-bonus arithmetic
  (`search_kernel_core.cpp`, the node-exit and multi-cut sites), the
  repetition scan's bound (`board.cpp`), the table's construction
  (`tt.h`), `bench`'s and `wac`'s stop flags, the capture generator's
  promotion pushes (`board.cpp`), the root probe's buffers (`syzygy.cpp`),
  the frame fields a re-entry rewrites, the stack options in
  `CMakeLists.txt`.
- **One run**: Rarog's promotion race
  (`8/2p2P1P/2n2P2/1k6/p7/6N1/P5K1/8 b - - 0 44`, where Rarog reaches
  seldepth 127 by depth 10 and does not finish depth 11 in 60 s) on
  `build/b5-off/basilisk.exe`, Hash 64: depth 14 in 0.77 s, 2,361,508
  nodes, seldepth 43. Not reproduced here.

## 2. Rarog's steps against Basilisk

| Rarog | What it found there | Basilisk today | Goes to |
|---|---|---|---|
| D.6.1 (NODE-3) | Castling neither quiet nor noisy; kept at nElo −4.9 ± 15.2 | Quiet in every predicate (fetch); no test says so | B.6.3, a pin |
| D.6.2 (BOARD-2) | Quiescence blind to a quiet queen promotion; +12.2 ± 6.5 nElo over 11,108 games (RAR-S108) | The capture generator emits every promotion push, all four pieces (read), so quiescence and the capture stage see them. The departure runs the other way: three quiet under-promotions a push are searched in quiescence and ordered with the captures, where the donor leaves them to the quiets | B.6.4, then a B.6.6 row or a B.6.5 repair |
| D.6.3 (NODE-1) | Null and ProbCut children typed cut nodes; retyping lost −3.7 ± 6.9 over 9,662 (RAR-S114), closed `NO_CHANGE` | The donor's typing at every call (fetch); no test pins it | B.6.3, a pin |
| D.6.4 (NODE-2) | A fail low stored its best-scoring move; storing none lost −14.7 ± 13.8 (RAR-S115), because IIR then reduced revisited fail-low nodes | No move stored at a fail low and the table keeps the old one (fetch); IIR is the donor's, PV and cut nodes from depth 6, so Rarog's failure has no path here | none |
| D.6.5 (NODE-4) | The exclusion search prunes on the table-refined estimate; the re-entry rewrites the frame | The pruning block is guarded by the exclusion; IIR, ProbCut and LMR's eval term still read the refined estimate there. A re-entry (exclusion, null verification) resets `move_count`, `stat_score` and the grandchildren's cutoff count, and the exclusion restores none, as the donor (read) | B.6.4, a counter each |
| D.6.6 (NODE-5) | Captures generated before the table move | Lazy in the main search; quiescence generates and scores every capture before it tries the table move (fetch) | B.8.5 |
| D.6.7 (NODE-8) | The ply cap returns a static score in check | Repaired in B.5.2 (`2757cfa`): the draw test first, a draw in check, the cap kept at 128. PLAN's defect table still listed it | table corrected |
| D.6.8 (DRIVER-11) | A depth-0 entry counted twice | Counted once (fetch) | none |
| D.6.9 (EVAL-5) | The scale multiplies the whole tapered score | The same, tempo included (fetch) | C.7.3, already there (E3) |
| D.6.10, D.6.26 | Pawnless endings a minor ahead | `pawnless_scale` and its truth table | none |
| D.6.13, D.6.20 | Aspiration fail-low form; the fail-high stop's hole | B.5's arm and D.1 | already there |
| D.6.15, D.6.16, D.6.17, D.6.21 | Draw verdicts, gain pruning against mating material, the root promotion rule, won KPK at or below 0 | Each is Basilisk's own repair, which Rarog imported | none |
| D.6.22 | `krpkr_scale`'s zeros on 1.7% wins | Five draw sites, each floored at 24 of 64 (fetch); BAS-E54's 18 of 3,000 | B.9.3 |
| D.6.23 | Wrong-bishop draws misread through leaves beside the recogniser | The positions are Basilisk's; B.14's guard is on the verdict leaves only | B.9.3 |
| D.6.24 | A KQ-KP fortress rule zeroing 26.7% wins | No such rule | C.7.6, a prior |
| D.6.25 | Two same-coloured bishops outscore a queen | The queen scores higher, but same-coloured KBB-K reads as a material win and is not in the dead-position rule (fetch) | B.9.3 |
| D.6.27 | IIR at every node type | The donor's condition (fetch) | none |
| D.3.1 | A zero or unreadable clock searched without limit | All clock fields zero, negative or unreadable fall to the bare-`go` depth 7 (fetch) | B.9.1 |
| D.3.2 to D.3.5 | `setoption` in a search, end of input, closed stdout, bad UTF-8 | Queued, handled, unchecked writes, rejected; none tested end to end (fetch) | B.9.1, tests |
| D.3.6 | `stop` during `bench` or `wac` | `stop` and `quit` are not seen until the run ends: each run has its own flag (read) | B.9.1 |
| D.3.8 | The forced-move shortcut under `searchmoves` | No shortcut at all: a one-move root runs its full time (fetch) | D.1 |
| D.4.3 | A search-path perft | Absent; planned in B.7.1, after the fit | B.6.3 |
| D.5.2 (TT-2) | `Hash` fills the table twice | A resize constructs every cluster with sequentially consistent stores on one thread, and `ucinewgame` clears it again on one thread (read) | B.8.4 |
| D.5.3, D.5.5, D.9 | The fixed-depth vote, `Hash` floored, Fathom's `exit`, a non-ASCII path | As the 2026-11-04 sweep found | B.9.1, already there |
| D.5.10 (RUST-8) | Tables built on the stack | Fathom's root-move buffer, about 100 KB, twice on the engine thread's stack (read) | B.9.1 |
| D.5.17 (TT-12) | A torn-pair and false-hit test for the shared table | Absent | D.2 |
| D.5.18 (BOARD-3) | A repetition scan crossing a null move | Bounded by `plies_from_null` (read) | none |
| D.5.19 | Every hard draw verdict against Syzygy | KPK only (`test_kpk_matches_syzygy`) | B.9.3 |
| D.5.20 | CI hygiene | Timeouts, pinned runners and one aggregate check exist; an untrained profile fails only the universal build; no ThreadSanitizer cell (fetch) | B.9.1 |
| D.5.23 | The correction bonus overflows 32 bits | §3 F4 | B.6.4 |
| D.8.1 | The architecture census as a tool | `tools/diag/source_census.py` exists | E.1 re-measures with it |
| D.9 (PROTO-9, 10, 12, 14, 15; DRIVER-15) | Tablebase probe limits, thread safety, a join blocked in a read | Not read against Basilisk | B.9.1 (threads, clock), D.3 (limits) |
| E.3.0 | Originality disclosure before a release cut | Scheduled only at E.3, two releases later | B.11.3, release rules |
| Phase F order (2026-11-05) | Integration before training, the trainer's curve before the corpus, nets judged once inference is fast | Corpus first, baseline nets before the engine can read one | F.2's sub-steps |
| F.6 | The printed score fitted per network | In G.2, after the first NNUE release | F.6 |
| H.4 | Throughput re-read on the network's profile | Absent | H.4 |

Not imported, with the reason: D.8's Rust-specific items (in-file tests,
`unsafe` sites, lints, `Compiled<T>`); D.5.14's compile-time KPK bitbase
(Basilisk builds it before `readyok`, cost unmeasured); D.6.14, D.6.18 and
D.6.19 (Basilisk has the mechanisms or already holds the rows in B.6.6 and
H.2); the replacement of Fathom by a Rust prober.

## 3. Findings of the scan

| # | Finding | Evidence | Goes to |
|---|---|---|---|
| F1 | `stop` and `quit` are ignored during `bench` and `wac` | `bench.cpp:83`, `wac.cpp:262`: a local flag the engine's stop never reaches (read) | B.9.1 |
| F2 | No search thread has a stated stack size. Windows gives a thread the executable's reserve, Linux 8 MB, macOS 512 KB for any thread but the first. The donor sets 8 MB for this reason | `CMakeLists.txt`: the only stack option is on Windows sanitizer builds (read). The deepest frame use at ply 128 is unmeasured | B.9.1 |
| F3 | The table's construction is slow by design: six sequentially consistent stores a cluster on one thread, then a second single-thread pass at `ucinewgame` | `tt.h` `TTCluster()`; `resize`, `clear` (read). Not on the clock, but about 15 s a pass at Hash 16 GB by arithmetic (inference) | B.8.4 |
| F4 | The multi-cut correction bonus can overflow `int`: `(s_val − static_eval) × 1000 / 282 × s_depth × B3SingMcCorr` passes 2³¹ from `s_depth` 54 at the default 177 and a residual of 63,700, sooner at the range's 400. The node-exit bonus cannot (3.6e8 at its ceiling) | `search_kernel_core.cpp`, the multi-cut site (read); unreachable in play at the default, signed overflow all the same | B.6.4 |
| F5 | Quiet under-promotions are captures to the picker and to quiescence: generated with them, exempt from quiescence's gain rules, tied with the queen's push in the capture score | `board.cpp`, the promotion pushes under `!quiets_only` (read); the scoring (fetch) | B.6.4 |
| F6 | A dead position scored as a win: any number of bishops on one colour against a bare king | `eval.cpp`, the KXK gate skips it and the mate drive fires; `board.cpp`'s dead-position rule covers KB-KB and a lone minor only (fetch) | B.9.3 |
| F7 | `go wtime 0 btime 0`, or clocks that do not parse, is a depth-7 search, not a move on the clock | `parameters.cpp`, `has_limit` (fetch) | B.9.1 |
| F8 | A root with one legal move is searched for its whole budget | no single-move exit in the driver (fetch) | D.1 |
| F9 | Write failures are never checked; a closed stdout is handled on POSIX only | `uci_output.h`, `engine_entry.cpp` (fetch) | B.9.1 |
| F10 | No test pins: the type each recursive call passes, quiet against capture over legal moves, the table keeping its move when stored none, IIR's condition, the rule-50 cutoff guard | greps of `tests/` (fetch) | B.6.3 |
| F11 | Quiescence's table cutoff has no rule-50 guard where the main search refuses from 96 | `search_kernel_core.cpp` (fetch); the donor guards it through the key | B.6.6, the rule-50 row |
| F12 | PLAN's *Known defects* still listed NODE-8 as open | `2757cfa`, `tests/test_b5_root.cpp` | corrected |

Read and found sound: the repetition scan across null moves; the node
count; the node-exit correction bonus; IIR's and the fail-low store's
forms; the won-KPK floor; `Hash` above physical memory; a helper's
exception; a terminal root under `go infinite`.

## 4. Order: what moved and why

- **The fit was ahead of its inputs.** B.6's joint SPSA ran before the
  search-path perft (B.7.1), before any whole-engine read of the code four
  clusters wrote, and before the repairs either would find. A repair that
  moves the tree after the fit sends the fit back. B.6 now runs perft and
  pins, the audit, its repairs, the reconciliation, then the fit.
- **One repair pass before the release.** B.9 had one research leaf and
  one repair leaf. Two more questions belong before the cut: whether the
  evaluator's hard verdicts are true, and which known defects owned by later
  leaves can be repaired now. All three now feed one repair leaf and one
  repair gate.
- **The release had no originality step.** 1.11.0 is the first release
  with a donor-shaped search and donor literals in the kernel; the
  disclosure sat at E.3. It is now B.11.3, and a release rule.
- **Vetoes before the gates that use them.** PLAN §3 gives every evaluation
  gate its endgame truth vetoes, and the leaf that audits those instruments
  was C.7.1, after four family gates. It is now C.2.7, before the first
  evaluation gate.
- **Phase F trained before it could read.** The corpus was sized by a
  learning curve the trainer had not yet produced, and baseline nets were
  due before the engine could load one. The order is now Rarog's of
  2026-11-05: integration, trainer and curve, corpus, speed, then nets in
  games.
- **Duplicates removed.** The search-board perft was planned in B.7.1 and
  needed in B.6; the drawn-overclaim instrument was planned in C.7.1 and
  needed in B.9.3; the table's fill and the large-page allocation are one
  change (B.8.4); the kernel's literals, listed by B.6.7's screen, are the
  constants census B.11.3 needs.

HISTORY's *Number map* holds the renumbering. No retired number is reused.
