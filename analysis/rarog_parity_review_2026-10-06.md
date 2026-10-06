# A.8 — What Rarog 2.4.0→2.5.0 and Manta's reports found, checked on Basilisk

- State / class: delivered; the source of PLAN A.8, `R2`
- Owner / date: agent, 2026-10-06, at maintainer request
- Decision taken: maintainer, 2026-10-06. Every applicable item becomes a
  sub-step of A.8, done before B.0's remaining leaves. After A.8.1's final
  import, Basilisk no longer reads Rarog.

## Sources

- Rarog `dev` at `dcf15c512f7c871bb11faaa70ccbb479953ddea9` (2026-10-06):
  `CHANGELOG.md` 2.5.0, the commits from `4a738d2` (the merge base with
  `v2.4.0`) onward, `analysis/uci_info_review_2026-09-16.md`,
  `analysis/tb_root_pv_2026-09-27.md`, `analysis/b52_research_2026-10-01.md`,
  `docs/PLAN.md` D.3 and E.3, `.github/workflows/`. This was read from the
  live repository at the maintainer's direction; A.8.1 pins what the leaves
  cite.
- Manta issues `maelic13/manta#2` (three Threads > 1 defects) and
  `maelic13/manta#4` (repetition from a hash-table replay, instant single
  move, KBNK). Rarog issue `maelic13/rarog#1` (bestmove not in the PV).
- Basilisk `dev` at `98ac050`; binary `build/release-pext-pgo/basilisk.exe`,
  `bench 13` = **14,978,465** (reproduced before probing).

## Evidence on Basilisk

Probes use `tools/diag/uci_probe.py` (one fresh process per search).

1. **SMP: `bestmove` is not the move the last `info` line reports.** 20 of
   120 searches at Threads 8: the 40 bench FENs at `go movetime` 40, 150 and
   400 ms (`smp` mode). An earlier, smaller run was 0 of 24, so the defect
   is intermittent. Mechanism (source): `SearchThreadPool::merge_results`
   picks the deepest result of any thread; only searcher 0 has an info
   callback, and nothing is printed after the merge. Threads 1 is correct: a
   stopped iteration is discarded, so the last line describes the move.
2. **Won-endgame clock sink.** `7r/5R2/8/2k1PB2/8/4K3/8/8 w - - 0 86` at
   `wtime 60000 winc 600`, no tablebases: **32,305 ms** on one move, with
   the last `info` at 1,020 ms (depth 27). That equals the computed maximum
   and Rarog's figure to the millisecond, because Basilisk's clock formula is
   a port of Rarog's. Rarog measured Stockfish 19 at 5.6–6.4 s on it. Other
   won endings took 1.0–6.9 s (KQvK, rec2 `8/P4k2/8/1N6/1P2B1K1/8/8/8 w - - 7
   81`, rec3 `1r6/R7/6k1/8/8/5PP1/6K1/8 w - - 6 72`).
3. **The known-win scale overlaps the tablebase band.** Without tablebases,
   KBNK `8/8/8/2K5/8/3k4/8/N1B5 w - - 0 1` prints `cp 22048` at depth 1 and
   18,424 at depth 12; KRvK prints 708–731, KQvK 1,027–1,066. KBNK's
   override reaches 27,420 (BAS-E28/E29), and `is_tablebase_decisive` reads
   anything from 19,872 up to the mate band as a tablebase result, so
   evaluations and tablebase values share one band.
4. **In-search tablebase probing is not Stockfish's.** Source:
   `Searcher::negamax` probes WDL at ply 1 and at every node with depth >=
   `SyzygyProbeDepth`, without the zeroing-move condition. It returns a flat
   ±20,000 at once, stored as an exact score, with no distance from the root
   and no bound handling. Rarog's adoption of Stockfish's form measured
   +11.4 ± 8.5 with tables (Rarog RAR-S94).
5. **The tablebase PV extension has no start rule.** `Engine::
   publish_tablebase_pv` checks `2 x elapsed >= Move Overhead` only between
   probes. Rarog lost a game on time when one DTZ root probe that missed the
   page cache took 54 ms at 58 ms of clock; the tables (151 GB) and RAM
   (128 GB) are the same here. Not reproduced on Basilisk.
6. **`info` conformance.** No aspiration bound lines (Stockfish prints them
   after 3 s); `seldepth` resets per search, not per iteration; no
   `multipv` token; Basilisk's field order is not Stockfish's; a mated or
   stalemated root prints one `info depth N … mate 0` line per depth up to
   the limit instead of one `depth 0` line; the final extended tablebase
   line omits `seldepth`, `nps` and `hashfull`; a tablebase root's cursed
   win shows `cp 0`. No `MultiPV` option.
7. **Process interface.** `int main()` ignores its arguments, so
   `basilisk.exe bench 13` waits on stdin; a fatal exception is reported only
   on stderr, which harnesses do not record; `bench` prints `nps` equal to
   the node count at 0 ms (bench 7/40: `nodes 283 … nps 283`).
8. **Repository.** `ci.yml` triggers on pushes to `master` and to
   `development`, a branch that does not exist (the branch is `dev`), and on
   every pull request. `release.yml` fires on `release: published`.
   `EXPERIMENTS.md` is 306 KB, `PLAN.md` 97 KB. `master` is already an
   ancestor of `dev`, so moving to merge commits needs no joining merge.
9. **C++23.** `CMAKE_CXX_STANDARD 23` is set, but the source uses about 11
   modern library facilities and 27 printf-family calls.

## Checked and not applicable

- Ponder race, tablebase work on the clock, hash on the clock, KPK at
  start-up, triple check, one-move tablebase PV: fixed in 1.10.1
  (BAS-C10–C13).
- `nps` inside the first millisecond: Basilisk divides by fractional
  seconds; `SyzygyPath <empty>` is handled; Threads resize at `setoption`.
- `maelic13/manta#2`: no unchecked replacement seed in `tt.h`; stdout is
  written synchronously (a stalled GUI blocks, it does not end the
  process); every completed depth prints a full line.
- `maelic13/manta#4`: the root is a PV node, so it never takes a hash cutoff;
  a single legal move is answered in 3 ms; KBNK converts 90.2% (BAS-E39).
- **Refused: SEE recapture promotions.** BAS-C09 closed this as a documented
  approximation: 0 of 339,607 production `see_ge` calls reach it. Its retry
  trigger has not fired. Not in A.8.

## Decision

A.8's sub-steps, in PLAN order. Behaviour-changing leaves (A.8.13–A.8.18)
carry a research leaf and a game gate each; everything else is output,
tooling or documents and is qualified by exact bench plus focused tests.
