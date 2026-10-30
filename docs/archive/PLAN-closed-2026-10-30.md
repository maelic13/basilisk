# Basilisk plan — closed steps A.8–A.17, B.0–B.2 and B.10

Moved verbatim out of `docs/PLAN.md` on 2026-10-30, when A.17 closed Phase
A, as A.8.4 moved A.1–A.7 (PLAN keeps open work). PLAN citations of these
steps resolve here. This is a record: never take a next step from it.

## Phase A — Reset: documents, harness, instruments, baselines (closed)

- [x] **A.8** Rarog-parity repairs, repository and C++23
  Added 2026-10-06 by maintainer decision, before B.0's remaining leaves:
  everything the 2026-10-06 review of Rarog 2.4.0→2.5.0 and of
  `maelic13/manta#2` and `#4` found applicable to Basilisk
  (`analysis/rarog_parity_review_2026-10-06.md`, probes by
  `tools/diag/uci_probe.py`). Unlike A.1–A.7, some leaves change behaviour;
  each of those has its own research leaf and game gate. SEE recapture
  promotions are excluded: BAS-C09 closed them and its retry trigger has not
  fired. After A.8.1, Basilisk reads no more of Rarog.
    - [x] **A.8.1** `[M]` Final Rarog import
      Snapshot Rarog `dev` at `dcf15c51` (2026-10-06) as
      `docs/reference/rarog-2026-10-06/` with its own manifest, which the
      checker verifies. Include only what A.8's leaves cite:
      `CHANGELOG.md`; `docs/PLAN.md`; `docs/PROCESS.md`; `AGENTS.md`;
      `.github/workflows/`; `xtask/` (`release-check`); `tools/diag/
      check_guide.py` and `guide_board.py`; `analysis/
      uci_info_review_2026-09-16.md`, `tb_root_pv_2026-09-27.md` and
      `b52_research_2026-10-01.md`; and `tests/multipv.rs` and
      `multipv_syzygy.rs`. Back up a full-history bundle beside the existing
      ones in ignored storage. `docs/reference/README.md` and
      `agents/research.md` record that this is the last import (maintainer
      decision 2026-10-06): Rarog's later findings no longer enter.
      — CLOSED 2026-10-07: 19 files, each checked against its git blob id;
      manifest `rarog-2026-10-06.sha256`, 295 reference files verified.
      `LICENSE`, `GUIDE.md` and `docs/EXPERIMENTS.md` were added to the list
      for the licence, A.8.6's fingerprint rows and A.8.5's index. Bundle
      `rarog-all-refs-2026-10-07.bundle` (`dev` had moved to `1647dd28`,
      which contains `dcf15c51`). Import row BAS-X35.
    - [x] **A.8.2** `[I1]` CI on pull requests to master only
      `ci.yml` today runs on pushes to `master` and to `development` (no such
      branch; ours is `dev`) and on every pull request. It runs on pull
      requests into `master` and on `workflow_dispatch` only, and a newer
      commit cancels a superseded run. The local loop covers `dev` commits.
      Verified by one PR run and one manual dispatch.
      — CLOSED 2026-10-07 on local qualification: `ci.yml` parses with
      triggers `pull_request` (branches `master`) and `workflow_dispatch`,
      `concurrency` `ci-${{ github.ref }}` with cancel-in-progress, and its
      four jobs unchanged; no job or document referred to the push
      triggers. The PR run and the dispatch need a push, which is the
      maintainer's, so they are owed at A.8.21's release PR; a failure
      there reopens this leaf.
    - [x] **A.8.3** `[M]` Merge commits from dev to master
      `dev` reaches `master` by a merge commit, never a squash: AGENTS
      *Commits and reporting*, PROCESS's release procedure and PLAN §5.
      `master` is already an ancestor of `dev` (checked 2026-10-06), so no
      joining merge is needed. Review the non-release tags (`archive/
      nnue-local`, `oracle/hybrid`, `oracle/hybrid-diag`): keep each one a
      document cites, with its retirement condition. The maintainer changes
      the repository settings (merge commits allowed; squash off for
      `master`).
      — CLOSED 2026-10-07: the rule is in AGENTS *Commits and reporting*
      and §5. PROCESS has no release procedure yet; A.8.6 writes one with
      the merge commit in it. Tag review: `oracle/hybrid`,
      `oracle/hybrid-diag` and `archive/nnue-local` each hold commits `dev`
      does not reach, so merge commits retire none; all three keep
      HISTORY's conditions. Repository settings remain the maintainer's.
      — SUPERSEDED 2026-10-08 by maintainer decision: releases are squash
      merges again, with an `archive/dev-X.Y.Z` tag per release (§5).
    - [x] **A.8.4** `[M]` Documents into docs/, closed Phase A archived
      `PLAN.md`, `PROCESS.md`, `HISTORY.md`, `EXPERIMENTS.md` and `DESIGN.md`
      move to `docs/`. The root keeps `GUIDE.md`, `AGENTS.md`, `CLAUDE.md`,
      `README.md`, `CHANGELOG.md` and `LICENSE`. `check_roadmap.py`, every
      tool and link that names a moved file, and AGENTS' task table follow;
      historical records keep their wording. A.1–A.7's records move verbatim
      to `docs/archive/PLAN-closed-<date>.md` (checked byte for byte before
      writing), and PLAN keeps a one-paragraph summary and a pointer. A.8
      stays in PLAN until it closes. Checker passes; no new broken link.
      — CLOSED 2026-10-07: the five documents moved with `git mv`; A.1–A.7
      archived in `docs/archive/PLAN-closed-2026-10-07.md`, asserted
      byte-identical to the cut text. `check_roadmap.py` reads `docs/` and
      now fails on a missing document instead of skipping it, which would
      have let a wrong path pass silently; its self-test passes. 15 links
      repointed by target only. The 13 broken links in tracked Markdown are
      the same 13 before and after, all in historical records. Sibling
      names inside `docs/` stay as written.
    - [x] **A.8.5** `[I1]` Experiment ledger split into entries
      `docs/EXPERIMENTS.md` (306 KB) keeps its prose, structure and retry
      map, with an index table of ID (linked), short title and disposition;
      each entry moves to `docs/experiments/<ID>.md`. Before writing, every
      entry is rebuilt byte for byte from its file. `check_roadmap.py` fails
      when the index and the files disagree (a row without a file, a file
      without a row, a duplicate, a wrong heading); its self-test plants each
      disagreement. AGENTS', PROCESS's and `agents/records.md`'s registration
      rules describe entry-plus-index registration.
      — CLOSED 2026-10-07: 179 entry files (185 parts) and a 51 KB index,
      down from 304 KB. Before writing, the former file was rebuilt byte for
      byte from the generated index and the entry files parsed back from
      their own text. Basilisk's ledger was not table-only like Rarog's:
      ID-table rows become field/value tables under their column names;
      bold-paragraph entries, and the BAS-S16 field table, move verbatim;
      two rows a blank line had cut off from their table (BAS-O04, O05)
      became entries. Six IDs hold two parts each: the collisions BAS-E08,
      X08 and X11, and the continuations D03, E39 and E55. `check_roadmap.py`'s
      `validate_ledger` replaces the heading-collision check, and its
      self-test plants each disagreement. Removing `BAS-X35.md` from the
      real tree fails the check (exit 1). The 13 broken links are unchanged.
    - [x] **A.8.6** `[I1]` Tag-driven release flow
      Moved from E.3.1. Today `release.yml` fires on `release: published`, so
      a release exists before any asset is built, and nothing checks that the
      tag equals the version. In the new flow, pushing a `vX.Y.Z` tag on
      `master` runs a workflow that validates first: the tag equals both
      version sources (`CMakeLists.txt` and `src/constants.h`), the commit
      is on `master`, and CHANGELOG has a dated section. It then builds every
      asset read-only, asserts one `bench 13` fingerprint across the matrix
      equal to the one GUIDE declares, and only then publishes, with notes
      taken from CHANGELOG. Taken from Rarog's first release through its
      flow (A.8.1's snapshot):
      - a pull request into `master` runs the same build as a candidate,
        so a green PR is a releasable one; a plain `X.Y.Z` version also runs
        the release check there, a `-dev` version skips it;
      - the declared fingerprint is read by one parser from one named GUIDE
        row (Rarog's first tag run read the wrong row and failed every
        cell);
      - after a release, the first commit on `dev` bumps to the next `-dev`
        version, and CHANGELOG's `[Unreleased]` grows as work lands.
      A `workflow_dispatch` rehearsal runs without a tag. Tag, push and
      publish stay the maintainer's. Verified by a rehearsal and a PR
      candidate run.
      — CLOSED 2026-10-07 on local qualification. `release.yml` is rewritten:
      prepare, nine build cells, collect (nine assets, one fingerprint
      equal to GUIDE's, SHA256SUMS) and publish (tag push only, write
      permission there alone). The build cells and toolchain steps are
      unchanged, except that each smoke test now asserts the exact `id name`
      and the declared `bench 13` count instead of any count. The checks are
      `tools/diag/release_check.py`, which reads the fingerprint with the
      roadmap checker's own parser; `test_release_check.py` refuses each
      defect (4 tests, 11 cases). Locally the prepare step passes as a
      candidate on `dev` at 1.10.1 and refuses a tag run, `dev` not being
      on `master`. Both workflows parse, and all 31 `run:` blocks pass
      `bash -n`. PROCESS *Release* holds the procedure, and CHANGELOG has
      an `[Unreleased]` section. `dev` stays at 1.10.1 until A.8.21: a
      version-string change is engine source and waits for a build. The
      rehearsal, PR candidate and tag runs need GitHub and are owed at
      A.8.21; a failure there reopens this leaf.
    - [x] **A.8.7** `[I1]` SMP: the chosen thread's line before bestmove
      Measured 20 of 120 searches at Threads 8 (review item 1). A result
      carries its thread's PV, depth, score and seldepth. When the merged
      result is not what thread 0 last printed, the pool prints that
      result's line before `bestmove`, as Stockfish's
      `output_pv(*bestThread)` does. The move chosen is unchanged. Tests: the
      decision as a unit test, and a Threads 8 end-to-end test that fails
      with the print disabled; `uci_probe.py smp` reads 0 of 120. Exact
      bench; no SPRT.
      — CLOSED 2026-10-07 (`56918ed`): the merged result carries its
      thread's line and seldepth. The shared root table keeps no line, so
      the line comes from the thread whose result matches move and depth,
      with that thread's reported score. The pool prints it when
      `needs_pool_line` holds. `format_info_line` is now the single
      formatter. `uci_probe.py smp` reads 0 of 120 (20 of 120 before). The
      engine test fails 5 of 40 with the print disabled and passes with it.
      Release CTest 13/13; bench 14,978,465.
    - [x] **A.8.8** `[I1]` UCI info conformance
      Output only (review item 6), each with a protocol test:
      - Stockfish's field order: `depth seldepth multipv score [bound]
        nodes nps hashfull tbhits time pv`, with `multipv 1` on every line;
      - `seldepth` resets each iteration;
      - a single-PV aspiration fail prints a `lowerbound`/`upperbound` line,
        on Stockfish's condition (more than 3 s elapsed), so fast games print
        the same lines as now;
      - a mated or stalemated root prints one `info depth 0 score mate 0` or
        `cp 0` line, then `bestmove 0000` (after `stop` under `infinite` or
        `ponder`);
      - the final extended tablebase line carries the full field set;
      - a tablebase root's cursed win or blessed loss shows ±1–49 cp by its
        distance to the rule-50 border, as Stockfish, not `cp 0`, if the
        value feeds nothing but the display; otherwise the leaf returns;
      - `bench` floors elapsed time at 1 ms for `nps`.
      Exact bench.
      — CLOSED 2026-10-07: every item done, each with a test (release CTest
      13/13; bench 14,978,465). The cursed-win value feeds root ordering, so
      it is shown through a separate display field carrying Fathom's own
      1–49 cp, and ordering is untouched. The seldepth test relies on a fact:
      a running maximum cannot fall, and on the bench positions at depth 10
      it does. Live: a 12 s search printed an `upperbound` line at 8.3 s, and
      KQvK at rule-50 clock 92 shows `cp 49` with the 3-6-man tables.
    - [x] **A.8.9** `[I2]` MultiPV
      `MultiPV` (default 1, up to 256; capped by the legal or `searchmoves`
      count): each depth reports the best N lines, each tagged `multipv k`.
      Lines cut short by a stop carry `lowerbound`/`upperbound`, and
      `bestmove` is always line 1. At a tablebase root the lines stay within
      the best-ranked group, as Stockfish. Helper threads keep their present
      role. At `MultiPV 1` the search and its output are byte-identical:
      exact bench, the A.8.7 and A.8.8 tests unchanged, and pooled-PGO NPS
      within noise of the A.7.4 method. Tests on Rarog's `multipv.rs` and
      `multipv_syzygy.rs` shapes (A.8.1); stop sessions; Threads 4.
      — CLOSED 2026-10-07 (`d1ad178`). Lines 2..N search the root at full
      width, without the moves already reported, and store no root
      hash-table entry. Deviations, each with its reason:
      - the lines are sorted by score, so `bestmove` is the top line and may
        differ from the first line's move (line 2 scored above line 1 in
        testing; Stockfish sorts the same way);
      - a line a stop cuts short is not reported, and its previous depth
        stands, rather than a bound-marked partial line;
      - with MultiPV > 1 the main thread answers and the helpers do not
        vote, as in Stockfish.
      Identity at MultiPV 1: bench 14,978,465, and info lines identical
      line for line (a test). NPS by BAS-P13: +1.13% [+0.91, +1.91] against
      1.10.1, the condition met; the gain is unexplained and not claimed.
      Release CTest 13/13.
    - [x] **A.8.10** `[I1]` Command-line commands, fatal errors on stdout
      Arguments run as engine commands in order, then the process exits:
      `basilisk bench 13` benches. An unknown argument prints a message and
      exits 2; with no arguments the UCI loop starts as now. `help` prints
      what the engine is, how to drive it and where its source is. A fatal
      exception is reported on stdout as `info string` (and stderr), where
      harnesses record it. Process-level CTest cases; exact bench.
      — CLOSED 2026-10-07 (`db6b845`). The arguments form one command; the
      quit that follows does not raise stop, so `go depth N` runs to its end.
      The interactive loop still ignores unknown commands, as UCI asks. Four
      CLI CTest cases (bench, `go depth 4`, help, unknown exits 2); the check
      script fails on a wrong status or text. `basilisk bench 13` reads
      14,978,465. Release CTest 17/17. The fatal path has no test: nothing in
      a release build throws on demand.
    - [x] **A.8.11** `[I1]` Tablebase PV extension start rule
      Under a clock, `publish_tablebase_pv` starts only when at least ten
      move overheads remain before the hard ceiling once the search has
      ended (Rarog's rule, after a 54 ms cold DTZ read lost a game at 58 ms;
      review item 5). Without a clock it is unchanged. A unit test holds
      the rule at those numbers. Exact bench; ponder and tablebase engine
      tests unchanged.
      — CLOSED 2026-10-07 (`14dd3c3`). The hard ceiling is the clock less
      twice the overhead (30 ms more with helpers), as the search computes
      it; elapsed runs from `go` receipt. Unit test at the forfeit's numbers
      (58 ms left at 10 ms overhead refuses; exactly 100 ms allows). Engine
      test: at `wtime 100` the line is not extended, and it fails with the
      rule disabled. Bench 14,978,465; release CTest 17/17.
    - [x] **A.8.12** `[I2]` C++23 idiom pass, behaviour-neutral
      Modern idiom across `src/` without moving code between files (B.1 and
      C.1 own the moves): `std::format`/`std::print` for UCI and bench
      output in place of concatenation and the printf family;
      `std::expected` for FEN, `position` and `setoption` parsing;
      `std::span`, ranges, `constexpr` tables, `std::to_underlying`,
      `std::unreachable` and `[[nodiscard]]` where they make code clearer.
      Per-node code keeps `agents/implementation.md`'s rules. Each commit
      reproduces the fingerprint; release and sanitizer CTest; pooled-PGO
      NPS within noise, by A.7.4's method, against the head before the pass.
      — CLOSED 2026-10-07 (`0e80d1a`). `std::format` replaces the 25
      `snprintf` Diag printers (whose fixed buffers once truncated fields), the
      info formatter, and the bench and WAC reports. The remaining board
      queries and new helpers are `[[nodiscard]]`; the exhaustive WDL switch
      ends in `std::unreachable()`. Not used, with reasons:
      - `std::print`: the Ubuntu runners' libstdc++ 13 lacks it;
      - `std::expected` for FEN: already there;
      - ranges and constexpr rewrites of hot code: no clarity gain worth an
        NPS risk.
      Byte-identical output with times masked: the TUNE build's Diag at
      depth 11 on three positions (75 lines), the bench report, and WAC in
      both modes. Bench 14,978,465; release and ASan/UBSan CTest 17/17 (ASan
      shown live on a planted overflow). NPS by BAS-P14: −0.11% [−0.20,
      +0.15].
    - [x] **A.8.13** `[R2]` Score bands and in-search tablebase probes
      Two defects (review items 3 and 4). (a) Known-win evaluations up to
      27,420 (KBNK prints `cp 22048`) share the band `is_tablebase_decisive`
      reads as tablebase results. (b) The in-search probe returns a flat
      ±20,000 as an exact score, at every node from the probe depth and at
      ply 1, with no zeroing-move condition, no distance and no bound
      handling. Research the Stockfish shape: a tablebase band of its own
      between evaluations and mates, holding `win - ply`; evaluations clamped
      below it; probes only after a zeroing move within the probe limits; a
      win as a lower bound and a loss as an upper bound, returning on a
      cutoff and storing at depth + 6; display `cp ±(20000 - plies)`.
      Required content:
      - which consumers read the band (pruning and mate guards, the hash
        table's ply adjustment, the root display, the PV extension);
      - Rarog's refuted part, moving band values by ply in the table, which
        changed its bench without tablebases;
      - the KBNK drive's range under the clamp (BAS-E28/E29);
      - a prediction and falsifier;
      - the gate for A.8.15. No current run file sets `SyzygyPath`, so the
        gate is designed here: an endgame-start cohort with tables on both
        sides, conversion plus an SPRT, and a faults rule.
      — CLOSED 2026-10-07: `READY_FOR_IMPLEMENTATION`, packet
      `analysis/a813_tb_bands_2026-10-07.md`. Clerical correction: the probe
      has the zeroing-move condition under `Syzygy50MoveRule`; (b) above
      overstated it. New finding: with tables, the flat 20,000 passes every
      "not a mate" guard, so correction history trains on it and the
      reverse-futility refinement uses it. The handoff moves the band
      directly below mates, clamps evaluations under it, and widens six
      guards to `is_decisive`. Bench exactness is the deciding check for
      ply-adjusting the band in the hash table, with Rarog's unadjusted form
      as the fallback. The gate is changed from the endgame cohort: its
      roots are all in the tables, where in-search probes are off. It is a
      `[-5,5]` repair bracket at `10+0.1` on the standard book with tables
      configured, plus conversion counts and an activation read.
    - [x] **A.8.14** `[I2]` Score bands and probes, implementation
      Implements A.8.13's handoff. Qualification by that handoff: a
      fingerprint change if the clamp reaches the bench, the tablebase tests
      on the committed fixture, and protocol tests for the display.
      — CLOSED 2026-10-07: the handoff implemented in full, with the hash
      table ply-adjusting the band. H2 is refuted: bench 14,978,465 exact
      without tables, so Rarog's unadjusted fallback was not needed.
      `test_tt`'s boundary moves with the contract. New tests:
      - the band constants;
      - the display mapping;
      - KBNK without tables below the band;
      - a fixture search capturing into KQvK, scoring `tablebaseValue − 1`
        (`cp 19999`), where the old flat value gave 20,000.
      Release and ASan/UBSan CTest 17/17. With the 3–6-man tables, a 7-man
      root probes in search (365k hits) and reaches depth 29 in 2.3 s, where
      the head before reached depth 21 in 3.8 s.
    - [x] **A.8.15** `[V]` Tablebase-enabled gate
      A.8.13's registered gate, maintainer-run.
      Prepared 2026-10-07 as BAS-S19: candidate PGO build of `b0a078a`
      against the pre-A.8.14 PGO build. `sprt-repair-ltc.toml` (`[-5,5]`,
      `10+0.1`), concurrency 7, tables on both sides, cap 4,000 pairs, seed
      815; dry run accepted. Deviation from the packet: the existing wrapper
      flags carry `SyzygyPath`, so only the clock needed a new run file.
      — CLOSED 2026-10-07 (BAS-S19, maintainer-run): H1 at 280 pairs, **+31.7
      ± 13.4 Elo**, zero faults or time losses. All 181 clean tablebase wins
      reached were converted (candidate 115, baseline 66). Both binaries
      probe in search in 200/200 sampled 7–9-man positions. The prediction
      (+3) missed: the old flat value acted at every probe in half the games.
      The three parts are not separated; no ablation was run.
    - [x] **A.8.16** `[R2]` Won-endgame time sink
      A won ending without tablebases spent the whole hard maximum on one move
      (review item 2: 32,305 ms of a 60 s clock, last `info` at 1,020 ms;
      Stockfish about 6 s). Establish why the iteration never completes
      (Rarog's diagnosis: an aspiration cascade of fail-highs on a rising
      score). Also check whether the maximum itself, more than half the clock
      at move 86, is a defect. Design a bounded repair, its interaction with
      the aspiration loop (B.5's cluster) and the stability terms, a frozen
      prediction and a falsifier. D.1 keeps the general clock audit.
      — CLOSED 2026-10-07: `READY_FOR_IMPLEMENTATION`, packet
      `analysis/a816_time_sink_2026-10-07.md`. Confirmed: depth 28 fails high
      six times on the move depth 27 chose (`f7d7`), with no iteration
      completing before the maximum. The maximum is Stockfish's formula
      exactly, and stays. The donor's shallower re-search is refused
      (BAS-D17). Repair: past the optimum, a root fail-high on the last
      iteration's move ends the search; no move can change. The gate is
      amended to a `[-5,5]` repair bracket at `3+0.03`.
    - [x] **A.8.17** `[I1]` Won-endgame time sink, implementation
      Implements A.8.16's handoff. `uci_probe.py clock` on the review's
      positions shows the repair. Exact bench unless the handoff says
      otherwise.
      — CLOSED 2026-10-07: the handoff as written. At `60000+600`:
      - rec1: 32,313 → 4,750 ms (predicted about 4.7 s);
      - rec2: 3,817 → 3,875 ms;
      - rec3: 6,943 → 5,156 ms;
      - KQvK: 1,056 → 891 ms;
      - every move unchanged.
      KRvK reads about 1.7 s, the same on the pre-fix A.8.14 build, so that
      change predates this leaf. Engine test: 2.36 s at `30000+300`, 15.72 s
      with the stop disabled. Bench 14,978,465; release and ASan/UBSan CTest
      17/17.
    - [x] **A.8.18** `[V]` Won-endgame time sink gate
      SPRT `[0,3]` at `3+0.03` 1T and a `10+0.1` direction check, as A.8.16
      registers them; zero time forfeits. Maintainer-run.
      Amended by A.8.16 to a `[-5,5]` repair bracket at `3+0.03`. Prepared
      2026-10-07 as BAS-S20: candidate PGO build of `9c93d15` against the
      BAS-S19 candidate, cap 4,000 pairs, seed 818; dry run accepted.
      Played 2026-10-07: cap reached, **−0.7 ± 4.5 Elo** (nElo [−8.71, +6.51]),
      zero time losses. The registered acceptance (interval above −5) is not
      met, and neither is the return condition (H0). — CLOSED 2026-10-08:
      A.8.17 kept by maintainer decision, with the registered acceptance
      not met (BAS-S20).
    - [x] **A.8.19** `[R2]` Displayed-score normalisation
      Moved from D.3.3. Displayed-score research card: fit a win-rate model
      on Basilisk's own games, decide the `cp` mapping as Stockfish
      normalises, then implement. It works on A.8.14's tablebase band, and
      decides how known-win evaluations display (KBNK `cp 22048` beside KRvK
      `cp 715`). Display only, gated by identity: exact bench, no SPRT.
      — CLOSED 2026-10-08, NO_CHANGE by maintainer decision (BAS-C14). The fit
      puts the 50% point at 135 internal units in the middlegame and 334 in
      the endgame. `cp` stays internal: 29 tools, B.0's scale-ratio
      instrument among them, read it as internal units. `UCI_ShowWDL` was
      deferred by the maintainer. Known-win evaluations display as they are;
      since A.8.14 they sit below the tablebase band.
    - [x] **A.8.20** `[M]` B-programme anchors on the A.8 head
      Record A.8's fingerprint changes. Re-anchor the B.0 packet's §12
      handoffs and A.6's move table, which cite line ranges at `2e7914e`, on
      the A.8 head. State which A.7 baselines stand: G(0) and the gauntlets
      are 1.10.1's release baselines. The pooled-PGO NPS baseline is re-read
      if any A.8 leaf moved NPS (maintainer-run). Confirm B.0.1 and B.0.2's
      registered binaries are pinned and unaffected.
      Records done 2026-10-08:
      - fingerprint unchanged through A.8 (14,978,465);
      - A.6's map re-anchored in its packet (`search.cpp` 3,125 → 3,298
        lines; `eval.cpp` unchanged), with what A.8 added to each section;
      - B.0 packet §12.4: §10's fixed-node baselines unaffected, the
        instruments' parsers checked on A.8.8's line shape, `cp` unchanged
        (BAS-C14), and B.1's NPS reference re-based on BAS-P15;
      - B.0.1 and B.0.2's binaries re-hashed and unchanged;
      - G(0) and the gauntlets stand as 1.10.1's release baselines.
      — CLOSED 2026-10-08: BAS-P15 (maintainer-run) puts the A.8 head at
      **4.189M NPS** pooled, +1.5% over BAS-P12, with every prediction met.
      It is B.1's NPS reference.
    - [x] **A.8.21** `[M]` Patch release 1.10.2 (ANY TIME) — after A.8.20
      Maintainer decisions 2026-10-07: cut **1.10.2** through A.8.6's flow
      once the rest of Phase A is finished (amended the same day from "after
      A.8.11"), so it ships the gated A.8.14 and A.8.17 changes. Version sources, CHANGELOG's
      `[Unreleased]` dated as `[1.10.2]`, release and sanitizer CTest,
      the bench A.8.20 records, and a maintainer-run smoke run with ponder
      on and at Threads 4 against 1.10.1 with zero faults. A.8.15 and A.8.18
      are maintainer-run gates (decision 2026-10-07). The PR into
      `master` merges with a merge commit (A.8.3). The GitHub checks the
      earlier leaves could not run locally are owed here: A.8.2's PR run
      and manual dispatch, and A.8.6's candidate run on this PR and its
      tag run. A failure reopens the leaf it proves. Tag, push and
      publish stay the maintainer's.
      — CLOSED 2026-10-08 with the release commit:
      - version 1.10.2 in both sources;
      - CHANGELOG's user-facing `[1.10.2]` section;
      - README, GUIDE, DESIGN and HISTORY marked released.
      Maintainer decisions 2026-10-08:
      - the verification match against 1.10.1 (BAS-M13) is withdrawn
        unplayed, and the CHANGELOG quotes BAS-S19;
      - the ponder-on Threads 4 smoke run (BAS-C15) was stopped at 121 of 200
        games and accepted on judgement, with 1.10.2 at zero failures, as a
        check that 1.10.1's ponder fixes still hold.
      The PR's CI and Release runs and the tag run complete A.8.2's and
      A.8.6's owed checks.

- [x] **A.9** Harness and measurement instruments
  Added 2026-10-24. Evidence: `analysis/rarog_audit_2026-10-24.md`, appendix
  D.
    - [x] **A.9.1** `[I1]` Colosseum re-pinned to 0.4.0
      (D-36, D-38, C-31) Basilisk pins `colosseum-cli` 0.2.0. Rarog moved to
      0.3.0 (engine processes kept per slot, the setup position counted for
      threefold, faster datagen; cross-checked against fastchess, +6.9 ± 9.4
      against +6.78 ± 9.07) and 0.4.0 (per-engine depth, NPS and hold
      percentiles, pair Elo with the pentanomial, terminations and faults by
      engine). Re-pin with a registered cross-check, as Rarog's RAR-M70 was.
      The wrapper keeps zero tolerance for Basilisk's own faults and reads
      opponent faults under a rate rule instead of voiding a run (BAS-M10).
      Before B.3's gate, so B.3 is read on the new runner.
      — CLOSED 2026-10-24. Maintainer decisions: no cross-check and no null
      pair are owed (2026-10-14, and 2026-10-24: 0.4.0 is proven in Rarog).
      - The pin is `cli-v0.4.0` (`9c72539`), Rarog's executable and archive
        digests. Gauntlets no longer unset `engine-processes`, and the
        wrapper refuses any resolved value but `per-slot`.
      - The guard test had been failing on 0.2.0 too: its default arms
        were no longer staged. It now uses BAS-S27's pair.
      - Gauntlet faults are read per engine from `colosseum-cli stats`
        (D-38). A Basilisk build's fault or time loss voids the run, and so
        does an infrastructure fault. A foreign opponent's faults are read
        up to 1% of its games, and the runner's cap is 1% of the foreign
        games. BAS-M10's two Houdini 3 crashes would have been read at the
        registered 400 games a row.
      - Guard cases 31/31. A strengthened control fails on a return bug
        the smoke run found.
      - An 8-game smoke gauntlet streamed live progress to the console,
        and a dry run on 0.4.0 accepted the SPSA command and its run file.
      - The three BAS-S27 dry runs resolve as on 0.2.0, except
        `engine_processes`.
    - [x] **A.9.2** `[V]` Direction reads of the head against 1.10.2
      Added 2026-10-24 by maintainer decision. Every gate since 1.10.2 read
      `3+0.03` on one thread; the head has never been read at `10+0.1`, at
      four threads, or with tablebases, and B.2's kernel has its own tablebase
      probes. Three 1,000-game gauntlets against 1.10.2 rebuilt with the same
      compiler, registered as BAS-S27 with frozen predictions and a reading
      rule: each read passes when its 95% interval excludes a loss; a read
      below its band, or any Basilisk fault, opens a diagnosis before B.3.
      Maintainer-run, `tools/run_head_reads.ps1 -Read ltc|4t|tb`; B.11's
      release reads repeat them on the B.9 head.
      — CLOSED 2026-10-25 (BAS-S27, maintainer-run 2026-10-24). All three
      reads pass, with zero faults or time losses:
      - `ltc` +241.4 [+221.9, +262.3];
      - `4t` +237.6 [+217.4, +259.2];
      - `tb` +221.9 [+202.4, +242.7], with tablebases live.
      None is below its band, so no diagnosis opens before B.3. All three
      came in above their predictions: there was no LTC compression, and
      thread scaling is intact. B.11's release reads predict accordingly.
    - [x] **A.9.3** `[M]` Measurement rules from the audit
      (D-39, D-40, D-41, D-28, D-47) Into `agents/measurement.md` and PROCESS:
      a stopped SPRT's estimate is a first-crossing estimate, never the effect
      size, and is never extended or re-scored; about 2 Elo per 1% of speed
      held to +22% at `3+0.03` (RAR-S98, RAR-S99), replacing PROCESS's "small
      deltas only"; opponents play their smallest safe overhead (10 ms); a
      compiler bump owes a pooled NPS read and never falls between an SPRT's
      arms (rustc 1.98 to 1.99 cost Rarog 1.6% at the same fingerprint); a
      SQLite database is copied with its `-wal` file.
      — CLOSED 2026-10-25.
      - `agents/measurement.md` gained three rules: the first-crossing
        label and its no-extension rule (D-39), the compiler bump as a
        pooled speed read never between an SPRT's arms (D-28), and the
        SQLite copy (D-47).
      - D-47 was unverified in the audit. `export_tournament_pgn.py` opens
        the database read-only in place, so it already sees the WAL.
      - PROCESS gained the speed-to-Elo prior to +22% (D-40) and the 10 ms
        opponent overhead, which is dormant until E.2's field (D-41).
      - Under D-28, MSYS2's clang 22.1.8 to 23.1.3 step of 2026-10-23 owes
        a pooled NPS read, which no record yet carries. A.9.4 registers it
        as BAS-P19.
    - [x] **A.9.4** `[I1]` NPS read rebuilt on pooled builds
      (D-1 to D-4) Rarog's method study (RAR-P32): builds, not cycles, set the
      limit. Best of three per cycle; four builds an arm; a t-interval on
      per-build medians; a two-step read (two cycles, accept at +0.9% and
      close at +0.1%; between them six cycles, +0.5% with the lower bound
      above 0); a disturbance rule; two independent pools as the null; a pause
      after fresh builds, which antivirus scanning slows. `nps_ab.ps1`
      bootstraps readings, which Rarog showed is not the uncertainty.
      Implemented 2026-10-25, LOCAL_QUALIFIED:
      - `tools/nps_pool.ps1`, `tools/nps_read.ps1` and
        `tools/diag/nps_read.py`, as PROCESS *Build, bench and NPS* now
        states.
      - `test_nps_read.py` passes 18/18, and each of five mutations of the
        decision rules fails it. `test_nps_pool.ps1` passes 9/9.
      - A smoke read of two adopted pools ran step 1 and `-Extend` end to
        end, and a worktree build of `9c93d15` reproduced its fingerprint.
      - The leaf closes when BAS-P18, the null, reads zero. BAS-P19, the
        clang 22 to 23 read A.9.3 owed, runs after it. Both are
        maintainer-run with `tools/run_a94_nps_reads.ps1 -Read null|clang`.
      — CLOSED 2026-10-25.
      - BAS-P18, the null, read −0.19% [−0.66%, +0.27%], NO_CHANGE at step 1.
        That is inside its prediction, so the instrument is validated and
        the worktree build path costs nothing visible.
      - BAS-P19 read clang 23.1.3 at +0.89% [+0.23%, +1.54%] against 22.1.8
        at identical source: no regression. Rarog's −1.6% prior did not
        transfer, so A.9.3's owed compiler read is done.
      - The launcher's first run stopped before any reading on a pipeline
        bug, fixed in `5b6ea69`. Its pools were reused, and no reading was
        lost.
    - [x] **A.9.5** `[I1]` Profiler with inline attribution
      (D-13, D-75) B.7.2 and C.12 both require a profile, and Basilisk has no
      profiler. Rarog's reads per-region shares from ETW or a user-mode
      sampler with full inline chains over frozen cohorts, and validates its
      schema from the data: xperf with `-symbols` once silently charged
      inlined board work to `negamax`. The Clang/MinGW toolchain needs its own
      symbol path.
      — CLOSED 2026-10-25.
      - The research question was the symbol path, and it settled on two
        facts measured here:
        - `-gline-tables-only` leaves `.text`, `.rdata` and `.data`
          byte-identical (non-PGO, bench 8,437,148 both ways);
        - llvm-symbolizer reads full inline chains from MinGW's DWARF.
      - `PROFILE_SYMBOLS` (off by default; byte-identical when off) and
        `build_test.ps1 -Flavor Profile` build the binary. Its PGO build
        benches 8,437,148 and carries 23,871 inline records.
      - `tools/profile.ps1` is a user-mode sampler that needs no
        administrator rights. `tools/diag/profile_regions.py` charges the
        innermost engine frame by source file. Its tests pass 10/10.
      - On the head's `bench 13` × 5 (about 12,000 samples): board 23.5%,
        eval 21.4%, ordering 20.8%, search 16.7%, TT 6.8%, attacks 6.7%,
        system 4.1%. A repeat agrees in every region within its interval.
      - Charging by function instead (the trap) reads search 38.4%,
        board 12.8%, attacks 0.0% and TT 0.3%.
      - Refusals proven on real inputs: a symbolization without inline
        chains (exit 2), and a Release binary without line tables.
      - The sampler's first form read at 19 Hz, because Windows ignores a
        background process's timer request. Pollers' wake-ups also
        inflated `system` to 67%. A spin clock and a busy-thread filter
        fixed both before any recorded profile.
      - Cohorts beyond `bench 13` belong to B.7.2 and C.12, which define
        them. An EPD cohort run shows the path works.
- [x] **A.10** Delivery and repository repairs
  Added 2026-10-24. The delivery machinery matches Rarog's (CI on pull
  requests, tag-driven release, one fingerprint across the assets); these
  steps bring the practice and four checks up to it. Evidence:
  `analysis/rarog_audit_2026-10-24.md`, appendix D Part 2.
    - [x] **A.10.1** `[M]` Land dev and B.10 on master by squash PRs (ANY TIME) — before B.3
      (D-55) PROCESS *Delivery*'s transition said `dev` lands as one squash PR
      once B.2 closed; it has not (`origin/master` is still 1.10.2), and
      B.10's branch was cut from `dev`. Land B.2 (`dev`), then B.10, as one
      squash PR each with the fixed message shape and their results;
      afterwards new branches start from `master`. PRs, pushes and merges are
      the maintainer's.
      — CLOSED 2026-10-24 by the maintainer's merges: B.2 landed as `71a612b`
      (PR #5) and B.10 as `f4ebe79` (PR #6), one squash commit each, and
      branches now start from `master`. It closed out of turn, so it keeps
      its `(ANY TIME)` marker.
    - [x] **A.10.2** `[M]` CHANGELOG Unreleased caught up and kept
      (D-66) `[Unreleased]` is empty although B.2's cluster and B.10's repair
      were accepted. Write their user-facing entries, and add Rarog's rule to
      `agents/records.md`: a user-visible change gets its entry in the commit
      that records it.
      — CLOSED 2026-10-25.
      - B.10's entry had already landed with its fix, so only B.2's was owed.
        It is now written: the rebuilt selectivity core, at +136 ± 34 and
        +59 ± 21 Elo at `3+0.03`, and the head's BAS-S27 reads against
        1.10.2.
      - Nothing else user-visible has landed since 1.10.2. The release
        binaries' UCI option lists are identical apart from the version
        string, and PR #7 and PR #8 were roadmap and tooling only.
      - The rule, with what counts as user-visible, is in
        `agents/records.md`.
    - [x] **A.10.3** `[M]` PR refspec and required checks on master
      (D-59, D-58) Configure `remote.origin.fetch` with
      `+refs/pull/*/head:refs/remotes/origin/pr/*`, so a pruning fetch keeps
      the PR refs citations need (they vanished on 2026-10-23), and record it
      in `agents/records.md`. Make CI and Release required checks on `master`
      (repository settings, the maintainer's).
      Implemented 2026-10-25; the maintainer's setting is pending.
      - This clone maps the PR refs. A dry run showed that a pruning fetch
        without the mapping deletes all 8; with it, a real pruning fetch
        keeps all 8. The likely pruner is GitHub Desktop.
      - `agents/records.md`, PROCESS and `check_citations.py`'s hint now
        give the once-per-clone mapping, not a one-off fetch.
      - The required checks needed aggregators that cannot pass by being
        skipped. CI gains `CI complete` over its six jobs; Release's
        `Verify the complete matrix` now always runs and fails unless
        every build succeeded.
      - The step closes once the maintainer requires both checks on
        `master`, verified through the API. This branch's PR is their
        first live run, and A.10's closing check.
      — CLOSED 2026-10-25. Verified through the API: `master` requires
        `CI complete` and `Verify the complete matrix`, and branches up to
        date with it. Administrators included, force pushes and deletions
        blocked, squash merges only, as before.
    - [x] **A.10.4** `[I1]` Release and CI checks tightened
      (D-65, D-29, D-27, D-57) `release_check.py` also compares the released
      baseline's fingerprint. The Windows release cells pin and print the
      MSYS2 clang version: they install whatever MSYS2 ships, which is how
      clang moved from 22.1.8 to 23.1.3 on 2026-10-23. An ARM64 ISA contract
      with a required `prfm` class (`verify_isa.py` checks x86 tiers only).
      `ctest -R` filters in CI fail when they match nothing
      (`--no-tests=error`).
      — CLOSED 2026-10-25. Each check was proven live on known-bad input.
      - **Fingerprint (D-65).** GUIDE's *Released baseline* row states its
        `bench 13` count, and `release_check.py check` refuses a release
        whose row differs from the declared fingerprint. The two new test
        cases fail on the old checker.
      - **Compiler pin (D-29).** CI and Release had built Windows assets with
        clang 22.1.8, from the runner image's frozen MSYS2 database, while
        every measurement since 2026-10-23 used 23.1.3. By maintainer
        decision the pin is 23.1.3, in `.github/windows-clang-version`.
        Every Windows cell refreshes MSYS2, prints its compiler and fails
        on any other clang. The check passed on 23.1.3 and failed on a 22.1.8
        pin with this machine's clang.
      - **ARM64 contract (D-27).** `verify_isa.py --tier arm64` requires
        `prfm`, the TT prefetch, and forbids SVE, dot-product, AES and SHA.
        It reads AArch64's one-word objdump format, which the old parser
        skipped entirely; x86 counts are unchanged on the head's PEXT
        asset.
        - Freestanding objects for Linux and Windows ARM64 pass.
        - No-prefetch, SVE, dot-product and AES objects fail.
        - The contract runs on every ARM64 cell of CI and Release.
        - PR #9's first runs failed Windows ARM64 on one SVE instruction that
          Linux and macOS lack. The check now names every forbidden
          instruction's function.
          - The second run named it: `cntd x0` in LLVM libunwind's
            `Registers_arm64::getRegister`, not compiler-rt's
            `__arm_get_current_vg`, which I had guessed. libunwind reads the
            vector-granule register only when unwinding an SVE frame, and the
            engine has none.
          - The check allows it in that one function alone. Windows links
            LLVM libunwind, while Linux uses libgcc's unwinder.
        - `test_verify_isa.py` runs in the documents job.
      - **Empty filters (D-57).** The three `-R` filters and the
        sanitizer's `-LE strength` run pass `--no-tests=error`. Locally, a
        filter matching nothing exits 8 with the flag and 0 without it.
      - **Not yet run live.** The workflow steps first run on this branch's
        PR.
      - **Amended 2026-10-25 (maintainer decision).**
        - The pin is the LLVM major, `23`. Patch releases pass, because MSYS2
          ships several a month and an exact pin would stall CI on each.
        - `build_test.ps1` warns when the local clang is on another major. It
          warned on a `22` pin and stayed silent on `23`.
        - The workflow check, run verbatim on this machine, passes on `23`
          and fails on `22` and `2`.
    - [x] **A.10.5** `[I1]` Ledger index dispositions kept in step
      (D-61) `docs/EXPERIMENTS.md` lists BAS-M10 and BAS-M11 as not run while
      their entries read VOID and COMPLETE; the checker compares IDs, not
      dispositions. Fix the rows and make `check_roadmap.py` flag an index
      disposition that disagrees with its entry's.
      — CLOSED 2026-10-25.
      - Free-text dispositions are compared by state, open or closed. A
        verdict word anywhere means closed. A text that opens with
        registered, prepared, not (yet) run, maintainer-run or pending, and
        holds no verdict, is open.
      - The entry's state comes from its last `Disposition:` line, or from
        its `Disposition`, `Result / disposition` or `Result` field.
      - Entries with no such field, multi-part entries, and texts that state
        neither are not compared. The 52 entries with a state on both sides
        are compared.
      - On the real ledger the check refused exactly BAS-M10 and BAS-M11.
        Their rows now read VOID and COMPLETE. The self-test covers both
        directions, both entry forms, and prose that merely says "running".
    - [x] **A.10.6** `[M]` Tools index and retired numbers in comments
      (D-62, D-63) A `tools/README.md` with one line per tool and the
      procedure or leaf that uses it; retired step numbers removed from source
      comments (for example `// 8.7.6(c)` in `search_kernel_core.cpp`,
      "8.7.10" in `attacks.h`), behaviour-neutral.
      — CLOSED 2026-10-25.
      - `tools/README.md` indexes every tracked tool, one line each, with
        the procedure, step or ledger entry that uses it. A coverage check
        found none missing.
      - The retired numbering (Phases 5–15) is gone from engine comments:
        185 lines in 27 files. Each number became plain words, and
        history-only fragments were deleted. Two rewordings were corrected
        against the code: the search's clock does start at `recv_time`, and
        the bishop-scaler scope is deferred, not settled.
      - Behaviour-neutral, proven: against a non-PGO build of the code before
        the edit, `.text`, `.rdata`, `.data`, `.pdata`, `.tls` and `.reloc` are
        byte-identical; bench 8,437,148.
      - Left for the maintainer: 32 current-roadmap or ledger IDs in
        comments, and stale bench counts quoted in a few comments (for
        example 11,941,440 in `eval_params.h`).
    - [x] **A.10.7** `[I1]` Dev builds name their commit
      Added 2026-10-24 by maintainer decision. The exact shape is decided when
      this leaf opens. The proposal follows Reckless: a dev build's id name
      appends the short commit, as in `Basilisk 1.11.0-dev-<sha>`. That needs
      a build change, and `release.yml`'s id-name check must accept the suffix
      on candidate builds (next to A.10.4's release checks).
      — CLOSED 2026-10-25. Shape by maintainer decision: Stockfish's exact form.
      - A development build reads `Basilisk dev-<commit date>-<8 hash
        characters>`, with `-m` after the date when the working tree differs
        from the commit. Without git it reads `dev-<compile date>-nogit`. A
        release still reads `Basilisk X.Y.Z`. The release line was dropped:
        every dev build shares it, so it tells nothing the date and hash
        don't. `engineVersion` keeps `X.Y.Z-dev` for the release tooling.
      - One deliberate difference from Stockfish: the modified check uses
        the clone's own line endings, because Stockfish's
        `core.autocrlf=false` can read a CRLF working copy as modified.
      - A header regenerated at every build carries the commit, so a commit
        made without reconfiguring never leaves a stale name.
        `BASILISK_DEV_ID` fixes the name for byte-identity comparisons.
      - Release's smoke test now requires a candidate build to name exactly
        the commit it was built from, and a release to read `Basilisk X.Y.Z`.
        Run verbatim here, it passed both and refused a wrong commit, a
        suffixed name and a dev name on a release.
      - Verified: a clean tree names its commit; an uncommitted edit adds
        `-m`; the override works; a PGO build names its commit at bench
        8,437,148. CTest 21/21, and `test_uci_protocol` 85/85 with four new
        sections.
      - The first live run (PR #9) found every Windows build reading
        `dev-<date>-nogit`: the MSYS2 build shell has no git on its PATH, and
        MSYS2's own git reads this CRLF clone as modified. Configure now looks
        for Git for Windows first and hands the build that git. Under MSYS2
        it picks Git for Windows even with MSYS2's git present.
- [x] **A.11** Correctness repairs from the Rarog audit
  Added 2026-10-24 by maintainer decision. Each repair is its own engine
  commit with a test that fails on the old code, bench accounting for any
  fingerprint move and CTest on both arms; a repair that changes play is gated
  by a registered `[-5,5]` repair SPRT unless its leaf argues none is owed.
  Evidence: `analysis/rarog_audit_2026-10-24.md`. The repairs change the
  default search only: the legacy kernel (`-DB2_CORE=OFF`) was neither fixed
  nor maintained (maintainer decision 2026-10-24), and B.8.1 deleted it on
  2026-10-27, after which there is one arm.
    - [x] **A.11.1** `[I1]` Rule-50 damping out of the TT static eval
      (B-1, C-7) `evaluate()` damps the score by the halfmove clock
      (`eval.cpp:1210`, `:1985`); the kernel stores that value as the TT
      static eval and reuses it at any clock
      (`search_kernel_core.cpp:488-495`, qsearch `:184-187`), though the TT
      key has no clock. Rarog found the same defect in its own selectivity
      core (RAR-S73, `db92790`) and moved the damping into the search, applied
      once after the TT read. Test: an eval stored at clock 0 and read at
      clock 80 is served as a fresh evaluation would serve it. Moves the
      fingerprint; it precedes any fit and optimism (B.5).
      - Implemented 2026-10-25: `Evaluator::evaluate_undamped` is the eval
        without the clock, `evaluate()` damps it for every other caller,
        and the default kernel stores the undamped value and damps it after
        the read in negamax and qsearch. The legacy kernel is untouched.
      - The new `test_search` sections plant a clock-0 entry for a
        capture-free child at clock 81: the old kernel serves it undamped
        (root -748 against a fresh -443), the repair serves -443.
      - Qualified 2026-10-25 on `2c0cc76`: `bench 13` 9,260,429 (from
        8,437,148, +9.8%); the legacy arm 15,887,213, unchanged; CTest ON
        23/23 and OFF 20/20 in Release.
      - It changes play, so it owed the repair gate: BAS-S28, `[-5,5]`,
        launcher `tools/run_a111_gate.ps1`, maintainer-run.
      - Closed 2026-10-25: BAS-S28 reached its 4,000-pair cap at
        +1.5 ± 4.8 Elo (8,000 games, LLR +1.58, no faults) with equal NPS,
        and the repair stays by the registered rule on the maintainer's
        confirmation. The development fingerprint is `bench 13` 9,260,429;
        the legacy arm stays 15,887,213.
    - [x] **A.11.2** `[I1]` Singular search: lone excluded move returns alpha
      (B-4) When the singular search excludes a node's only legal move,
      `searched == 0` returns `in_check ? -(MATE_SCORE - ply) : 0`
      (`search_kernel_core.cpp:1297`). That scores far below singular beta, so
      the move extends, up to two plies off the PV, and the extensions chain:
      doubles are capped only by `DoubleExtMax`, and the check extension
      stacks on top. Stockfish returns `alpha` when the excluded move is the
      only one. In Rarog the chain left bench unfinished (seldepth 127 at 331M
      nodes, one search spinning 39 minutes) once IIR changed; Basilisk's IIR,
      at its rail, plausibly hides it today, and B.3 plans an IIR-off arm.
      Test: a forced position whose only move is the TT move does not extend;
      a counter for the lone-move return.
      - Implemented 2026-10-25 (`8802a85`): an exclusion search with no
        move left returns `alpha` (`no_move_score`, `src/search_types.h`),
        counted as `singular_lone_move` in diagnostic builds.
      - The leaf's "does not extend" is corrected to its mechanism: `alpha`
        is one below the singular beta, so a forced move still extends by
        one ply, as in Stockfish and Rarog, and never doubles at the
        default `SingularDoubleMargin` 4. The test checks exactly that on
        Rarog's forced `Kxg2`. `negamax` has no test seam, so the test
        holds the return's value and the Diag counter shows the kernel
        takes it: 2,719 returns over `suite_v1.epd` at depth 10.
      - Qualified: `bench 13` 8,753,339 (from 9,260,429, -5.5%); the legacy
        arm 15,887,213, unchanged; CTest ON 23/23 and OFF 20/20.
      - It changes play, so it owed the repair gate: BAS-S29, `[-5,5]`,
        launcher `tools/run_a112_gate.ps1`, maintainer-run.
      - Closed 2026-10-25: BAS-S29 reached its 4,000-pair cap at
        +1.7 ± 4.8 Elo (8,000 games, LLR +1.84, no faults) with equal NPS,
        and the repair stays by the registered rule on the maintainer's
        confirmation. The development fingerprint is `bench 13` 8,753,339;
        the legacy arm stays 15,887,213.
    - [x] **A.11.3** `[I1]` Own clock of zero without increment is bounded
      (A-55) `go wtime 0 btime 5000` with no increments: a negative clock
      clamps to 0 (`parameters.cpp:291-293`), `compute_time_limit` sets no
      soft or hard limit when `time <= 0 && inc <= 0`
      (`search_worker.cpp:74`), and `infinite` stays false because the other
      clock is set (`engine.cpp:92-98`), so the search runs to
      `MAX_SEARCH_DEPTH`. Stockfish budgets from `max(1, ...)`. A zero own
      clock gets a minimal budget. Test: the command answers `bestmove` within
      a bound.
      - Closed 2026-10-26 (`6de81be`). Time management is on when either
        side has a clock or an increment, as in Stockfish, so the budget
        formula runs for an empty own clock and its floors give one
        millisecond. A search with no clock at all keeps its depth, node or
        mate limit: the early return could not simply go, since it is also
        what leaves `go depth N` unbounded in time.
      - New `test_search` sections: `go wtime 0 btime 5000` answers a legal
        `bestmove` within 2 s, and a clock-free `go depth 6` still completes
        depth 6. On the old code the first is stopped only by the test's
        10 s watchdog. Through UCI, `wtime 0` and `wtime -50` answer at
        once at depth 4; `go depth 9` and a 2 s clock are unchanged.
      - `bench 13` 8,753,339 and the legacy arm 15,887,213, both unchanged;
        CTest ON 23/23 and OFF 20/20. The driver is shared, so the legacy
        arm carries the fix too.
      - No gate is owed: the change acts only when the side to move has no
        time and no increment, which no registered gate can reach (every
        one has an increment) and which in a game means the flag has
        already fallen.
    - [x] **A.11.4** `[I1]` Upper-case UCI move text accepted
      (A-9) `apply_uci_move` matches `move_to_uci(move) == token` exactly
      (`parameters.cpp:67`), so `E2E4` or `e7e8Q` is rejected, and a rejected
      `position` ends the process by design (BAS-C12). Rarog accepts
      upper-case move text (its tests come from Little Blitzer's output).
      Compare case-insensitively before the legality match; the fatal policy
      for a genuinely illegal move stays.
      - Closed 2026-10-26 (`d81efc7`): `fold_move_text` lower-cases the token
        before both legality matches, `position` and `searchmoves`
        (`src/parameters.cpp`); a folded token must still equal a legal
        move's spelling, and the diagnostics print the token as sent.
      - New `test_uci_protocol` sections: `E2E4` plays e2e4, `e7e8Q` and
        `E7E8N` promote to the piece named, `searchmoves E2E4 D2D4` is
        accepted, and `E2E5` is still rejected with the board unchanged.
        The first three fail on the old parser. Through UCI,
        `position startpos moves E2E4 E7E5 G1F3` now searches instead of
        ending the process.
      - `bench 13` 8,753,339 and the legacy arm 15,887,213, unchanged;
        CTest ON 23/23 and OFF 20/20. No gate is owed: the change is
        protocol only, and every harness sends lower-case moves.
    - [x] **A.11.5** `[I1]` Fatal errors from every thread reach stdout
      (A-35) Only the UCI thread catches (`main.cpp:86-93`); the engine thread
      (`engine_entry.cpp:49`) and the helper threads have no handler and no
      `std::set_terminate`, so an exception there aborts with nothing on
      stdout, where harnesses look. A concrete source: `setoption Hash` at the
      advertised maximum throws `bad_alloc` in `tt.h:130-131` after
      `clusters_.reset()`. Print `info string CRITICAL ERROR ...` on stdout
      from every thread, and keep a usable table when a Hash allocation fails.
      - Closed 2026-10-26 (`8289ceb`). `run_engine` installs a terminate handler
        (`report_terminate`, `src/uci_output.h`): an exception escaping any
        thread prints `info string CRITICAL ERROR: unhandled exception: ...;
        exiting.` on stdout and exits with status 1. It does not wait on
        the output mutex, which the dying thread may hold.
      - `TranspositionTable::resize` keeps the previous size, cleared, when
        the new table cannot be allocated, and returns false; the engine
        prints `info string Hash N MB could not be allocated; keeping M MB`.
        It does not halve toward a smaller table: zero-filling the largest
        one the system would commit could exhaust memory.
      - Tests: `fatal_report_from_a_thread` (a helper-thread exception
        yields exit 1 and the line on stdout; without the handler the
        process aborts with stdout empty) and `test_tt`'s failed-resize
        sections (the old `resize` threw `bad_alloc` and left no table
        behind its stale size; skipped under ASan, whose allocator aborts
        on an oversized request). Through UCI, `setoption name Hash value
        33554432` now keeps 64 MB and the engine searches.
      - `bench 13` 8,753,339 and the legacy arm 15,887,213, unchanged;
        CTest ON 24/24 and OFF 21/21. No gate is owed: nothing changes
        unless a thread fails or a Hash cannot be allocated.
    - [x] **A.11.6** `[I1]` Frontier nodes counted once
      (B-111) `negamax` counts a node at entry (`record_node()`,
      `search_kernel_core.cpp:335`) and at depth 0 or less hands off to
      `quiescence`, which counts it again (`:117`); `interior_nodes` is also
      counted before the hand-off (`:391`). `nodes`, NPS, bench and `go nodes`
      budgets are inflated, and the move loop's `(int(state_.nodes) & 14)`
      reads the count, so search behaviour depends on it. Rarog's counter
      specification holds `main_tt_probes == nodes` as an exact-mode
      invariant. Count each node once and re-declare the fingerprint; whether
      a gate is owed is decided with the maintainer (timed play changes only
      through that rule and the stop poll).
      - A precondition defect, found 2026-10-26: the tie rule also ran at
        the root. A later root move is searched with a null window at
        alpha, and the fail-hard quiescence search returns alpha exactly,
        so an equal score is usually a fail-low bound. With the count
        fixed, the free-queen test played Qa4 (worth −6) at Qxd5's
        `cp 1018`. Fixed first in its own commit (`0eadf03`, maintainer
        decision): the root is kept out of the rule, as Stockfish's root
        plays only a move that exceeded alpha. `bench 13` 8,753,339 →
        8,101,423; all 40 positions move, 39 in score. The legacy kernel
        has no such rule.
      - Implemented (`07a357f`): the depth-0 hand-off and razoring, which
        also re-entered quiescence on the same node, pass the node as
        counted, so quiescence skips the count and the stop, ply and draw
        checks negamax already ran. `interior_nodes` moved below the
        hand-off; `in_check_nodes` stays above it, keeping run_suite's
        `check_exts <= in_check_nodes`.
      - A new `test_search` section: a depth-1 search of a single quiet
        move counts 2 nodes (the old count, on the A.11.2 binary, is 3).
        Free-queen now plays Qxd5 at depths 1–3.
      - `bench 13` 7,233,823 with both commits (from 8,753,339); the
        legacy arm 15,887,213, unchanged; CTest ON 24/24, OFF 21/21.
      - Gate: one `[-5,5]` repair SPRT for both commits against `master`
        (maintainer decision 2026-10-26), since the root fix changes play
        and the count reshuffles every remaining tie: BAS-S30, launcher
        `tools/run_a116_gate.ps1`, maintainer-run.
      - The leaf's "timed play changes only through that rule and the stop
        poll" missed a third reader: the time manager's root-effort ratio
        (`search_worker.cpp`, `root_best_nodes / root_depth_nodes`). The
        gate's games show it did not move the clock (69.6 ms per move on
        both arms).
      - Closed 2026-10-26: BAS-S30 accepted H1, +4.3 ± 5.9 Elo (5,312
        games, LLR +2.95, no faults); both repairs stay by the registered
        rule. The development fingerprint is `bench 13` 7,233,823; the
        legacy arm stays 15,887,213.
    - [x] **A.11.7** `[I1]` Evaluation colour symmetry: tie-break and sweep
      (C-3, A-22, C-103) The pawnless "ahead by at most a minor" scaler picks
      `strong = (w_tot >= b_tot) ? WHITE : BLACK` (`eval.cpp:800`), so at
      equal totals White is always the strong side: a colour asymmetry, rare
      because the totals must tie exactly. Pick the side by the score's sign,
      as Stockfish does. `tests/test_eval.cpp` mirrors only five positions;
      Rarog's corpus-wide sweep with promoted material and the lazy gate open
      found twelve asymmetric evaluations (its C.13.1, among them a
      connected-rooks bug). Add a sweep over a versioned sample with promoted
      and odd material.
      - Implemented 2026-10-26 (`d5442f8`): the scaler takes the winning
        side from the score's sign (`score > 0`), as Stockfish's
        `eg > VALUE_DRAW`. Besides the tie, the totals could disagree with
        the score, and then the richer side's lack of pawns scaled the other
        side's advantage.
      - Tests in `test_eval`: an equal-totals position (R+N+B against Q and
        two pawns) equals its mirror, where the old scaler gave −9 against
        −150; a sweep over 3,149 positions (the bench and WAC positions, ten
        odd- and promoted-material positions, each with a seeded eight-ply
        walk, `std::mt19937_64`) finds none asymmetric, where the old scaler
        gave nine. No other asymmetry was found.
      - `bench 13` 7,384,101 (from 7,233,823); the legacy arm 17,606,287
        (from 15,887,213), since the evaluation is shared; CTest ON 24/24,
        OFF 21/21.
      - It changes the evaluation, so it owed the repair gate: BAS-S31,
        launcher `tools/run_a117_gate.ps1`, maintainer-run.
      - Closed 2026-10-26: BAS-S31 accepted H1, +5.9 ± 6.3 Elo (3,204
        games, LLR +2.96, no faults); the repair stays by the registered
        rule. The development fingerprint is `bench 13` 7,384,101; the
        legacy arm 17,606,287.
      - Found beside it, not changed: the same scaler's near-draw factor
        reads `npm(weak) > 0 ? 4 : 14` where the donor has
        `npm(weak) <= BishopValueMg ? 4 : 14`
        (`sf9587-ks/src/material.cpp`). Ours scales a weaker side holding a
        rook (KRBKR, KRNKR) to 4/64 where the donor gives 14/64, and our 14
        is unreachable.
    - [x] **A.11.8** `[I1]` Info lines: bestmove after a stop, mate scores as cp
      (A-26, A-31) After 3 s the aspiration loop prints a `lowerbound` line
      for a new move that is failing high (`search_worker.cpp:450-459`); a
      hard-limit or GUI stop then discards the iteration (`:515`), `bestmove`
      is the previous depth's move, and at Threads 1 nothing is printed after
      it (`search_thread_pool.cpp:193-194`; `needs_pool_line` compares
      results, not the last printed line). Rarog's fix (`97ea52d`) records the
      move its last line named and prints the result's line when they differ.
      At a tablebase root the bound line also prints the raw bound, not the
      tablebase score. Output only; exact bench.
      Mate scores printed as `cp` (BAS-S27, maintainer decision 2026-10-25):
      - Without tablebases, both 1.10.2 and the head sometimes print a
        mate-range score as `cp`, for example `cp -31742`, which is beyond
        the 128-ply mate window.
      - Both also print `cp` values in the tablebase band (`cp 20000 -
        plies`) in won endings: 111 move scores in 64 of the `ltc` read's
        games, all decisive.
      - The cause is not located yet. Find which values reach the UCI score
        unconverted.
      - Test: a score is `mate n` exactly when it is a mate, and a `cp` value
        is in the tablebase band only when a tablebase or tablebase-backed
        entry produced it.
      - If the cause is in the search rather than the display, the leaf
        owes bench accounting and A.11's repair SPRT.
      - Closed 2026-10-26 (`accaac0`), after A.11.7, output only, `bench 13`
        exact:
        - The search records the first move of its last line-1 `info` and,
          when `bestmove` names another, repeats the result's line (Rarog's
          rule). At a tablebase root a failed window prints the move's
          tablebase score without a bound. The bound lines' 3 s delay is a
          `SearchLimits` field so a test can open it.
        - The "mate scores as cp" premise was false. `cp -31742` is the
          rule-50 downgrade: `score_from_tt` turns a mate or tablebase
          result the clock may spoil into the strongest non-decisive score,
          as Stockfish's `value_from_tt` does. The `cp 20000 - plies`
          values without tablebases were evaluations, not tablebase results:
          won-ending evaluations print raw and run through the display band
          (the KBNK drive reaches 31,660). In the `ltc` read's games the
          first such score came from 1.10.2 in 62 games and from the head in
          2, both bishop endings at `cp -19873` and `-19883` that fresh
          searches show as ordinary evaluations. The repair is in the
          display: an evaluation prints at most ±19,871, below the band.
        - Tests in `test_search`: a stopped fail-high search ends on the
          line of the move played (the old code ended on `e2e3` while
          playing `h2h3`); a tablebase root prints no bound (old: 7
          bounded lines); evaluations of 25,000 and the rule-50 value print
          as ±19,871. CTest ON 24/24, OFF 21/21; `bench 13` 7,384,101 and
          the legacy arm 17,606,287, both exact. No gate is owed.
    - [x] **A.11.9** `[R2]` Override boundaries never reward shedding material
      (C-9, C-131) The KBNK and KXK overrides (`kbnk_score` base 15,600;
      `kxk_score = KNOWN_WIN + ...`, `eval.cpp:306`, `:520`) apply only
      without the winner's pawns, so KBNP-K to KBN-K, and KBP-K promoting to a
      knight or an opposite-colour bishop, score far above the generic score
      of the position before; BAS-E49 saw KBP-K convert "via knight
      promotion". Rarog's rule (RAR-E53, RAR-E54): a replacement score stays
      above every neighbour one capture or one promotion away, held by a unit
      test with a wire check. Research first (how often play meets these
      boundaries, and the cost in games), then a design. It changes the
      evaluation during the freeze, so a repair needs the maintainer's
      B.10-style admission.
      - Research 2026-10-26: over 26,574 gate and read games, about 1.3% of
        `3+0.03` games and 0.3% at `10+0.1` entered an override from a
        position with a pawn, almost always by underpromotion; the strong
        side then drew 17 of 352 (4.8%) positions a queen wins. The cause:
        queen and rook mates scored ordinarily (about 2,400 for KQB-K)
        under KBNK's 15,600 and KXK's 10,650. RAR-E53/E54 are not in the
        pinned snapshots, so the design rests on Basilisk's own rule that
        more material never scores lower against a bare king.
      - Admitted into the freeze and implemented (`fdf4277`, maintainer
        decision 2026-10-26): tiers by the winner's strongest piece, queen
        (28,700 to 31,742) above rook (26,100 to 28,699) above the
        minor-piece mates. The queen and rook tiers carry the ordinary
        score; KBNK keeps its weights with its base at `KNOWN_WIN`; pawns
        add 3 each within a minor tier.
      - Screens: tiers with only a small drive lost KR-K (2/4) and left a
        wandering queen in a tablebase analysis line, so the queen and rook
        tiers carry the ordinary score; then KQ-K 4/4, KR-K 4/4, KBB-K 6/6,
        KBN-K 6/6 and the WAC floor hold. On the 342 positions where games
        underpromoted, `master` still underpromotes in 337; the candidate
        in none (289 queen promotions). `bench 13` 7,857,713 (+6.4%); the
        legacy arm 23,696,970; CTest ON 24/24, OFF 21/21; a `test_eval`
        section on promotion order and pawn shedding fails on the old
        evaluation.
      - Gate BAS-S32, `[-5,5]`, launcher `tools/run_a119_gate.ps1`,
        maintainer-run.
      - Closed 2026-10-26: BAS-S32 reached its 4,000-pair cap at
        −0.3 ± 4.0 Elo (8,000 games, LLR −0.33, no faults); the repair stays
        by the registered rule on the maintainer's confirmation. The
        development fingerprint is `bench 13` 7,857,713; the legacy arm
        23,696,970.
      - On the same branch, A.11.8's repeated final line goes through
        `send_info` (`28b7018`), so in analysis it gets the tablebase
        extension other lines get; output only, `bench 13` exact.
      - Also on the branch (maintainer decision 2026-10-26), root
        underpromotions are judged against the queen (`1641e64`): a rook or
        bishop promotion beats the queen promotion on the same squares only
        by more than 200 of exact score, and the queen is searched first.
        A transposing rook line had displaced the queen in 59 of 228 gate
        positions (7 of them errors by Stockfish 18); now 1, with the
        stalemate escapes and Saavedra's `c8=R` kept. Pruning
        underpromotions below the root instead lost Saavedra. `bench 13`
        exact, NPS unchanged. Gate BAS-S33, `[-5,5]`, launcher
        `tools/run_a119_up_gate.ps1`, maintainer-run: H0 at 3,285 pairs,
        −2.7 ± 4.1, so the rule is reverted (`c1791b3`), keeping the
        stalemate and Saavedra tests. A replay found its choices no worse
        (Stockfish 18: 21 better, 148 equal, 8 worse), and the rule acts in
        only about 6% of games, so BAS-S34 re-tests it from the 3,159
        positions where the arms differ (`[-5,0]`, launcher
        `tools/run_a119_up_retest.ps1`, maintainer-run). It reached its
        3,159-pair cap at −0.3 ± 1.1 Elo on that book (about 0.02 Elo in
        ordinary play), so the rule is re-applied (`d7d2c34`) by its cap
        rule on the maintainer's confirmation; `bench 13` 7,857,713, the
        legacy arm 23,696,970; CTest ON 24/24, OFF 21/21.
    - [x] **A.11.10** `[R2]` KR-K conversion: the clock keeps a tier's base
      Added 2026-10-27 by maintainer decision, ahead of fail-soft
      quiescence, whose gate runs against the repaired head. A.11.9 put
      bare-king wins against a queen or rook in tiers (rook 26,100 and up,
      queen 28,700 and up), and the rule-50 damping scales the whole score,
      so the rook tier loses about 131 points a ply: more than the king
      drive KR-K is mated with. Found by A.11.11's research: over the first
      24 positions of the conversion-floor generator (60k cold nodes a
      move) `5524bf7` converts 24/24 and `00809cc` 20/24, the losses
      drifting into fifty-move draws; the floor's first four still
      converted.
      - Mechanism confirmed 2026-10-27 on master: tier scores left undamped
        convert 24/24; the tier's cap on the ordinary score lifted converts
        20/24. The damping, not the cap.
      - Repair (`3d7559a`): `damp_rule50` keeps a tier's base and damps only
        the score above it, so the clock's pressure on KR-K and KQ-K is the
        ordinary score's, as before A.11.9, and the tier order holds at any
        clock. The tier bases move to `eval.h`, their one definition. Minor
        tiers and every other score are damped as before (with the base at
        0 the curve reproduces `bench 13` 7,857,713). The Texel tuner's
        linear curve does not mirror apply_endgame's overrides and is
        unchanged.
      - Tests failing on the old curve: `test_eval`'s tier order at clock
        99 and the bound on the rook tier's loss to the clock; the KR-K
        conversion floor over 24 positions with floor 24 (old code 21/24).
        The legacy kernel keeps the four-position KR-K floor, and its
        transposing and Saavedra promotion checks are held only on the core
        (`d0e0991`): master's legacy kernel already plays the rook in the
        transposing position at 800,000 nodes, and the budget at which it
        proves Saavedra's mate moves with the evaluation.
      - Qualified 2026-10-27: `bench 13` 8,913,394 (from 7,857,713,
        +13.4%; positions 1 and 2 unchanged, tier scores in deep capture
        lines from position 3 on); the legacy arm 22,652,055 (from
        23,696,970); CTest ON 24/24, OFF 21/21. KR-K over 40 positions
        40/40 at a mean 22.1 plies to mate (master 35/40, 33.5); KQ-K 40/40
        at 10.7 (master 40/40, 13.3).
      - Exposure: 3,414 of 11,234 A.11.7 and A.11.9 gate games (30.4%)
        reach a settled bare king against a queen or rook, and one of them
        was drawn (insufficient material), so the outcome almost never
        rests on it. It changes play, so it owes the repair gate: BAS-S35,
        `[-5,5]`, capped at 1,500 pairs, launcher `tools/run_a1110_gate.ps1`,
        maintainer-run.
      - 2026-10-27, by maintainer decision: one PR carries this repair,
        A.11.11 and B.8.1, and the first BAS-S35 run was stopped at 107
        pairs (void) so that both gates run after the legacy kernel's
        deletion and on hardened tests. B.8.1 made the core-only guards of
        `d0e0991` unconditional. The hardened tests (`bb38444`): every
        conversion family plays the same 24 positions in parallel (8.8 s in
        CTest, from about 53 s serially, identical results), with a floor,
        a ceiling on the mean plies to mate a quarter above the measured
        mean, and a staleness bound that fails a floor two below the
        family's rate or a ceiling a third above its mean; the WAC floor
        moves from 130 (57 below the count) to 175, with a staleness bound
        at +20. Read on the repair and with A.11.11: KQ-K 24/24, KR-K
        24/24, KBB-K 24/24, KBN-K 24/24 and 23/24; WAC 188 and 182. On
        master the new KR-K bounds fail (21/24, mean 38.0 against the 29.5
        ceiling) and every other bound holds. The re-run: seed 35002,
        results `tools/results/a1110-gate-r2/`, the same pinned binaries,
        since neither engine changes.
      - Closed 2026-10-27: BAS-S35 reached its 1,500-pair cap at −1.3 ±
        6.7 Elo (3,000 games, LLR −0.59, no faults), and the repair stays
        by the registered rule on the maintainer's confirmation. It is
        A.11.11's baseline, so the development fingerprint moves past its
        8,913,394 to A.11.11's.
    - [x] **A.11.11** `[R2]` Fail-soft quiescence
      (B-19) Pulled forward from B.4 by maintainer decision 2026-10-26, so
      that B.3 is not fitted around a known defect. Quiescence fails hard:
      a fail-low returns `alpha` (delta pruning and the final return in
      `quiescence`, `search_kernel_core.cpp`). The exact `alpha` echoes up
      the tree. Root fail-highs then equal beta and drive aspiration
      cascades (93% of Rarog's), and inside the equal-score tie rule's
      window (`ply + 2 >= root_depth_`) an "equal" score is usually a
      bound, so below the root a refuted move becomes the node's best move
      one time in eight (A.11.6 kept the root out). Stockfish's quiescence
      is fail-soft throughout.
      - Research first. The only record of the earlier attempts is two
        comments in `quiescence`: a fail-soft delta-pruning return broke the
        KBNK fixed-depth conversion canary, and seeding the final store from
        the TT-tightened stand-pat turned a KQK mate in 5 into `mate 63`,
        because a stand-pat raised by a lower bound is not an upper bound.
        Each changed one return alone. Re-measure both, then specify every
        return, the stand-pat tightening and the stored bounds together,
        against Stockfish's.
      - Obligations: the KBNK and KQK fixtures and their mate distances, the
        conversion floors, WAC, bench accounting, and a diagnostic count of
        tie-rule promotions of a bound, read before and after.
      - Gate: a `[-5,5]` repair SPRT. RFP, razoring and aspiration were
        fitted on fail-hard values; B.3 to B.5 refit them on the repaired
        search.
      - Audit input (2026-10-27, `analysis/code_audit_2026-10-27.md` §2
        F2): the returns and stores that must change together — the
        node-type-free TT cutoff, which lets a PV quiescence node take a
        depth-0 bound (B-27); the stand-pat tightened by decisive TT
        bounds, which negamax's refinement excludes; the delta-pruning
        return that stores nothing; the final exact-or-upper store of
        `alpha`, where the donor stores only lower or upper bounds of the
        fail-soft best. H1: the KQK `mate 63` came from the PV-node bound
        cutoff consuming the upper bounds a fail-soft store seeds from a
        lower-bound-tightened stand-pat, not from fail-soft itself;
        falsified if the donor's form with the PV exclusion still breaks
        the KQK and KBNK fixtures. The tie rule's `(nodes & 14) == 0`
        fires one node in eight; the audit read the donor's as `& 15`,
        which is wrong (corrected below). The counter it needs: tie-rule
        promotions of a bound echo (`score == alpha`) against exact ties.
        Verified 2026-10-27: `quiescence` has no node-type parameter, so
        the PV exclusion needs a flag threaded through the call at
        `:414` and the recursive calls at `:169` and `:300`. Today a
        null-window qnode can store only bounds (`:213`, `:312`, or
        `TT_ALPHA` at the unchanged alpha), so a bound cutoff at a PV
        qnode returns a value outside the window and reaches the line
        only through a re-search; a mate distance enters the PV
        verbatim only through an EXACT entry from a wide window. H1
        describes the fail-soft store that does not exist yet, so the
        attempt tests it, not reading.
      - Research (2026-10-27; release-pext builds, `bench 13`, CTest,
        fixed-depth-18 mate probes; Diag counters from `run_suite.py` at
        depth 12 over `suite_v2`; raw runs in `tools/results/a1110/`).
        Neither recorded failure reproduces on the current kernel. Each
        old attempt alone (the fail-soft delta return; the final store
        seeded from the tightened stand-pat) keeps the KQK shortest mate
        at mate 4, KBNK at mate 5 and the KBNK canary passing, and so
        does the donor form with or without the PV exclusion. H1 is
        therefore moot: its effect is gone, not explained. Bench and WAC
        (depth 6) by arm: master 7,857,713 and 187/300; delta alone
        8,175,334 and 182; seeded store alone 10,732,207 and 163; donor
        form 15,201,413 and 182; without the decisive exclusion
        12,978,636; without the PV exclusion 18,599,768. Every arm fails
        the four-position KR-K floor at 3/4.
      - The candidate (`3d31b50`) is the donor form: no table cutoff at
        a PV qnode (the node type is a template parameter, set at the
        depth-0 drop by the caller's type and NonPV for razoring and
        ProbCut); the stand-pat tightened by any bound that proves a
        better estimate, exact included, never by a decisive score; the
        delta prune returns and stores its upper bound `stand_pat +
        queen + 200`; a futility-pruned capture raises the best to its
        optimistic value; the final store is the fail-soft best as an
        upper bound, never exact. The stand-pat cutoff, the in-check
        branch and the depth-cap return keep their existing fail-soft
        form; the donor's fail-high averaging toward beta is a separate
        mechanism and is not taken.
      - Tie-rule promotions (`48f1d5c`): of an alpha echo 79,408 to
        6,993, below alpha 3,653 to 14,353. The rule's `& 14` is left
        unchanged so the gate tests one variable; the counters say it
        now acts on genuine fail-low ties, not on the clamp.
      - Cost: the depth-13 tree doubles (+93% nodes, bench time 3.0 s to
        5.5-6.4 s). The growth is spread over every family, most in the
        donor rules that compare fail-soft values with margins: LMR
        deeper re-searches x3.3, singular double extensions x3.0,
        triple x2.3. Fail-hard leaves had held them near dormant; their
        margins are B.3-B.5's to refit, not this leaf's.
      - KR-K conversion, 60k cold nodes over the floor generator: master
        35/40 (mean 33.5 plies to mate), candidate 33/40 (43.6); KQ-K
        40/40 in both. Master's 20/24 on the first 24 against the 24/24
        recorded 2026-10-08 is a regression bisected to `00809cc`
        (A.11.9's bare-king tiers; its parent `5524bf7` converts 24/24).
        Suspected mechanism, untested: at the rook tier (26,100 and up)
        the rule-50 damping removes about 131 cp a ply, more than the
        king-drive gradient the search converts with. The floor's resize
        waits on the maintainer's decision about that regression.
      - The regression became A.11.10 (maintainer decision 2026-10-27),
        and the candidate is gated on its repair: `69ecf42` merges it.
        Qualified there: `bench 13` 16,092,165 (+80.5% on the repair's
        8,913,394; time to depth 13 +74% at +3.9% nps, PGO, four
        interleaved runs), the legacy arm 22,652,055; CTest ON 24/24 with
        the 24-position KR-K floor, OFF 21/21; KR-K 40/40 at a mean 23.1
        plies. Gate BAS-S36, `[-5,5]`, capped at 4,000 pairs (quiescence is
        in every game), launcher `tools/run_a1111_gate.ps1`,
        maintainer-run after BAS-S35 keeps the repair.
      - Closed 2026-10-27: BAS-S36 accepted H1 after BAS-S35 kept its
        baseline, +12.1 ± 9.9 Elo (1,834 games, LLR +2.95, no faults; a
        first-crossing estimate), without a refit. The repair stays by the
        registered rule. The development fingerprint is `bench 13`
        16,092,165. B.3 to B.5 refit RFP, razoring, aspiration and the
        margins fail-soft woke (LMR's deeper re-search, the singular double
        and triple extensions) on this search.
      - Correction, 2026-10-27: the tie rule has no divergence. Stockfish's
        is `(int(nodes) & 14) == 0` as well, one node in eight: at the
        pinned `sf_19` (`edb0d9db`, `src/search.cpp:1511`) and on master
        since. The audit's F2.4 misread it, and this leaf repeated it
        unchecked. A scratch `& 15` build read `bench 13`
        13,430,303 and WAC 180 against 182; it is not the donor's form and
        is not gated.
- [x] **A.12** `[M]` Roadmap rules from the audit
  Added 2026-10-24, for the maintainer to decide one by one: Rarog rule 7c's
  amendment of 2026-10-20 (a near-optimum surface may be tuned as one longer
  run with a larger perturbation and a smaller gain ratio); the standing
  contracts Rarog keeps that Basilisk's table lacks (the allocation guard once
  B.7.1 lands, diagnostic counter units, the Texel data contract once C.2.1
  writes it, board footprint assertions, cross-engine board benchmark parity);
  and whether `agents/research.md`'s snapshot-only rule for reading Rarog gets
  a standing exception for audits the maintainer asks for (waived for the
  2026-10-24 audit).
  - Decided 2026-10-27 by the maintainer:
    - Rule 7c gains the single longer run as an opt-in for a surface read
      as near its optimum; blocks stay the default. Rarog's evidence,
      read from its live repository under the exception below (Rarog
      `ec6f5b5`), is BAS-X36.
    - *Standing contracts* gains the diagnostic counters' units and the
      cross-engine board benchmark's profile, both already true in code.
      The allocation guard enters with B.7.1 and the Texel data contract
      with C.2.1; both leaves now say so. Board footprint assertions are
      not adopted.
    - `agents/research.md`'s snapshot-only rule has a standing exception:
      an audit or import the maintainer asks for may read Rarog's live
      repository, citing its commit.
- [x] **A.13** Correctness repairs from the code audit
  Added 2026-10-27 from the audit of the engine's crucial paths
  (`analysis/code_audit_2026-10-27.md`; its items are F1 and F3, and
  §6's D1 and S11 from the second pass). The two evaluation repairs,
  A.13.1 and A.13.2, are admitted into the evaluation freeze (maintainer
  decision 2026-10-27, after §6.7's read found no cost in games and no
  dependency of the search on them: known theory-truth defects are not
  carried through Phase B). The same rules as A.11: each repair is its
  own engine commit with a test that fails on the old code, bench
  accounting for any fingerprint move, CTest on both arms, and a
  registered `[-5,5]` repair SPRT when play changes unless the leaf
  argues none is owed. The legacy kernel is deleted (B.8.1), so CTest has
  one arm.
    - [x] **A.13.1** `[I1]` Won KPK scores as a win
      (F1) `apply_endgame` keeps the tapered score of a KPK the bitbase
      calls won (`eval.cpp:753`); when that score favours the bare king,
      the pawnless scaler takes the bare king as the strong side (`:833`,
      A.11.7's sign rule) and scales a won ending to 0. Measured against
      Syzygy on all 662,704 legal KPK positions: the bitbase agrees on
      every one, the engine serves 0 for 2,022 wins (0.31%, mostly a
      second-rank pawn with the lone king adjacent;
      `8/8/8/8/8/8/Pk6/3K4 w - - 0 1`, pre-scaler −28; its leaf
      `8/8/8/8/8/2k5/P7/3K4 b` after `...Kb2` is served 0 at depth 1,
      verified 2026-10-27). Repair, in the donor's form
      (`sf9587-ks/src/endgame.cpp:153-171`): in the KPK branch a won
      position returns `KNOWN_WIN` plus the donor's `PawnValueEg` 206
      plus the pawn's rank, from the pawn side's view and signed to
      White's as `apply_endgame` works, in place of the comment at
      `:753` — before every tier, the wrong-bishop rule and the scalers;
      one site covers the lazy path, since both paths call
      `apply_endgame` and no evaluation cache sits between. The value
      lands at 10,207–10,212: above `KNOWN_WIN`, below the rook tier
      (26,100) and the queen tier (28,700), far below `is_decisive`
      (31,743), so pruning treats it as an ordinary evaluation. The
      scaler stays as it is: the donor also picks the strong side by
      sign and also scales a bare king to a draw (`evaluate.cpp:829`,
      `material.cpp:205-207`); the audit's proposed invariant that a
      lone king is never the strong side is withdrawn, because in KPPK,
      KNPK or KBPK with a score favouring the bare king it would turn a
      served 0 into a wrong-signed score, untested and
      fingerprint-moving. Promotion closure: KPPK, KNPK and KBPK reach
      a won KPK when the lone king captures, and their ordinary scores
      sit far below 10,2xx; only the defender makes that capture, so
      minimax resolves it, as in the donor. Rule 50: the donor returns
      the KPK value before its damping; Basilisk damps every score
      (`eval.h:137`), a deliberate departure the leaf records. Test: a
      `test_endgames` section over every legal KPK position against a
      bundled `KPvK.rtbw` fixture (7,824 bytes; the WDL file alone
      serves WDL probes): the undamped evaluation's sign equals the WDL
      verdict and no won position scores below `KNOWN_WIN`; on the old
      code it fails at 2,022 positions. Wiring the test needs:
      `test_endgames` has no `BASILISK_TEST_SYZYGY_FIXTURE_DIR`
      (`CMakeLists.txt:366-367`; `test_search` and
      `test_engine_threading` have it), `tests/syzygy_fixture.h:92-93`
      hard-codes the two KQvK files and their sizes, and
      `test_search.cpp:954` counts the shared directory's files, so
      KPvK is materialised into its own directory and the fixture
      README's decoder list and SHA table are updated. Invariants:
      white perspective; the KBP wrong-bishop rule and the bare-king
      tiers unchanged (KPK has no pieces). Admitted into the evaluation
      freeze (maintainer decision 2026-10-27, after §6.7's games read);
      bench accounting (the bench suite's pawn ending can reach KPK),
      and the repair SPRT if the fingerprint moves.
      - Implemented 2026-10-27 (`6e39087`), as specified. The test
        `test_endgames`' KPK section, over the bundled `KPvK.rtbw` in its own
        temporary directory, counts two failures apart: the wrong sign
        (2,022 on the old code, the audit's number) and a decided position
        below a known win (443,106 on the old code: the tapered score of
        about 100 to 900). Both are 0 on the repair, in 1.1 s.
      - Qualified: `bench 13` 19,108,490 (from 16,092,165, +18.7%; 38 of
        the 40 positions move through KPK in deep lines; position 39's
        lost score for Black moves from −30,837 to −8,165); CTest 24/24;
        the conversion floors and WAC 182 unchanged.
      - Exposure: 189 of 4,864 recent gate games (3.9%) reach KPK. It
        changes play, so it owes the repair gate: BAS-S37, `[-5,5]`,
        capped at 2,000 pairs, launcher `tools/run_a1131_gate.ps1`,
        maintainer-run.
      - Closed 2026-10-27: BAS-S37 reached its 2,000-pair cap at +2.3 ± 5.7
        Elo (4,000 games, LLR +1.42, no faults), and the repair stays by the
        registered rule on the maintainer's confirmation. The development
        fingerprint is `bench 13` 19,108,490.
    - [x] **A.13.2** `[I1]` A won queen against two minors is not scaled to a draw
      (§6 D1) The pawnless scaler's classifier compares non-pawn material
      in `EG_MAT` units (`eval.cpp:833-835`: Q 950, B 330, N 320), so a
      queen against bishop and knight is "ahead by at most a minor"
      (300 ≤ 330) and the ending is scaled by 4/64; the donor compares in
      middlegame values (`material.cpp:201`: 2538 − 1606 = 932 > 825) and
      does not scale it. Syzygy over 4,000 random positions a class,
      from the queen's side: KQ-KBN 82.5% won, KQ-KBB 71.0% (6% cursed),
      KQ-KNN 73.5%. Served at depth 1 from White's view (verified
      2026-10-27): KQ-KBN `8/8/3nb3/4k3/8/8/8/K6Q b - - 1 1` +21, KQ-KNN
      +27, against KQ-KB +697 — one minor fewer and the defender scores
      thirty times worse. The scaler also fires when the defender has
      pawns. Repair, in the donor's form: the classifier compares in the
      donor's middlegame values (a local table N 781, B 825, R 1276,
      Q 2538 for this comparison alone, with the dead-draw branch at
      `npm(strong) < RookValueMg`); every other `EG_MAT` use is
      unchanged. The near-draw factor in the same `if` (4 whenever the
      defender has material, where the donor gives 4 only against at
      most a minor and 14 otherwise) is the Known-defects row C.3–C.7
      owns; this leaf takes it only if the maintainer admits both, since
      the same test decides both. Test: a `test_endgames` truth table
      over material signatures through the evaluation trace's scale
      factor (added to the trace if it lacks one) — KQ-KBN, KQ-KBB,
      KQ-KNN and KQ-KR unscaled; KR-KB, KB-KN and KRB-KR scaled — plus
      the three FENs served positive for the queen's side; on the old
      code it fails at the first three. Invariants: A.13.1's KPK return
      sits before this scaler; A.11.9's material-shed boundaries are
      unaffected (no override row changes). Admitted into the evaluation
      freeze with A.13.1 (maintainer decision 2026-10-27); bench
      accounting (expect no move unless a bench position reaches a
      pawnless ending), and the repair SPRT if the fingerprint moves;
      the occurrence that matters is search without tablebases (C-147).
      - The maintainer admitted the near-draw factor with it (2026-10-27),
        so the Known-defects row C.3–C.7 held for it closes here.
      - Implemented 2026-10-27 (`1375dff`) in the donor's form: the
        predicate `pawnless_scale` in `eval.h` (the shape of
        `is_knnk_draw`, so the tuner can mirror it at C.2.1) counts
        material in the donor's middlegame values for this rule alone and
        returns 0, 4 or 14, or 64 where the rule does not apply; the
        evaluator calls it. The evaluation trace has no scale factor, so
        the truth table reads the predicate the evaluator calls rather
        than a new trace field: KQ-KBN, KQ-KBB, KQ-KNN and KQ-KR unscaled,
        KR-KB 4, KB-KN 0, KRB-KR, KRN-KR and KQ-KRB 14. The three
        queen-against-two-minors FENs are served above +150 for the queen
        at depth 1 (+354, +458, +272); the old scaler fails all three
        (about +20).
      - Qualified: `bench 13` 19,470,325 (from 19,108,490, +1.9%); CTest
        24/24; the conversion floors and WAC 182 unchanged.
      - Exposure: up to 606 of 8,864 recent gate games (6.8%, transient
        positions included). It changes play, so it owes the repair gate:
        BAS-S38, `[-5,5]`, capped at 4,000 pairs, launcher
        `tools/run_a1132_gate.ps1`, maintainer-run.
      - Closed 2026-10-27: BAS-S38 reached its 4,000-pair cap at +0.3 ± 4.0
        Elo (8,000 games, LLR +0.33, no faults), and the repair stays by the
        registered rule on the maintainer's confirmation. The development
        fingerprint is `bench 13` 19,470,325.
    - [x] **A.13.3** `[I1]` PV copy reads past the per-ply length array
      (F3) At `ply == MAX_PLY - 1` a move that raises alpha reads
      `state_.pv_len[ply + 1]` (`search_kernel_core.cpp:1272`), one past
      `int pv_len[MAX_PLY]` (`search_worker.h:135`); the value is clamped
      to `MAX_PLY`, so play is unaffected and the read is undefined
      behaviour only. Size the array `MAX_PLY + 1` or guard the copy;
      exact fingerprint on both arms; no gate. No fixed-depth test
      reaches ply 127 without exposing `negamax`, so the leaf records
      that exemption and the sanitizer CTest job stands for the proof.
      - Closed 2026-10-27 (`c23eefa`): the copy is guarded rather than the
        array widened, since a child at `MAX_PLY` returns before it sets a
        length and so has no entry to read; at `ply == MAX_PLY - 1` the
        guard yields `MAX_PLY`, the value the clamp gave, so the copy
        loop is empty as before. Release PEXT `bench 13` 19,470,325,
        exact (one arm since B.8.1); CTest 24/24. Exempt from a focused
        test as registered: no fixed-depth search reaches ply 127; the
        sanitizer CTest job runs on the PR.
    - [x] **A.13.4** `[I1]` The tablebase store carries a stale PV bit
      (§6 S11) The in-search WDL probe (`search_kernel_core.cpp:356-375`)
      runs before the TT probe, and its store at `:371` passes
      `ss->tt_pv`, which the node assigns only at `:455-457`: the stored
      bit is whatever the previous visitor of that ply record left
      (verified 2026-10-27). The donor probes after the TT probe and
      cutoff, where `ttPv` is already set. Repair in the donor's order:
      move the probe block after the TT cutoff, so the store takes this
      node's flag and a TT hit no longer pays a tablebase read; D.4
      keeps probe depth and limits. Behaviour changes only with
      tablebases: the fingerprint is exact (bench runs without tables);
      the proof is `test_search`'s tablebase fixtures, a counter of
      stores whose bit differs from the node's `tt_pv` (zero after), and
      a re-read of A.8.15's tablebase-enabled gate at its registered
      size, since the probe population changes.
      - Implemented 2026-10-27 (`0c0571a`): the probe block, unchanged,
        runs after the early TT cutoff and before the eval-difference
        training, the donor's Step 7. It now reads the node's depth after
        the check extension and the hindsight adjustment, as the donor's
        does after its Step 5, so an in-check node entered at depth 0 is
        probed at depth 1; the static eval runs before a tablebase return.
      - The counter `tb_store_stale_pv` (diagnostic builds; counter 83):
        on six 7–8-man endgames at depth 18 with 3–6-man tables, 2,901 of
        54,443 stores on the old order (5.3%), 0 of 16,291 on the
        candidate; the tablebase reads fall by 70% because a cut node no
        longer probes.
      - Qualified: `bench 13` 19,470,325, exact; CTest 24/24, the
        tablebase fixtures included.
      - Gate: BAS-S39, A.8.15's conditions (`[-5,5]` at `10+0.1`, tables on
        both sides, concurrency 7, cap 4,000 pairs), launcher
        `tools/run_a1134_tb_gate.ps1`, maintainer-run.
      - Closed 2026-10-28: BAS-S39 reached its 4,000-pair cap at +0.3 ± 3.2
        Elo (8,000 games, LLR +0.48, no faults), and the repair stays by the
        registered rule on the maintainer's confirmation. 2,285 of 2,286
        clean tablebase wins were converted; the miss is the baseline's,
        A-58 (D.4). The fingerprint is unchanged at 19,470,325.
- [x] **A.14** Simplifications from the code audit
  Added 2026-10-27 (`analysis/code_audit_2026-10-27.md` §3, from a
  mechanical census of `00809cc`). Behaviour-neutral: exact fingerprint,
  CTest, and a pooled NPS read only where a hot path is touched (the
  leaf touches none). The audit found engine duplication negligible, the
  language work done by A.8.12, and the long kernel and evaluator
  functions best left whole until B.3–B.5 and C.1 rewrite them; the
  smaller cleanups it found are folded into B.8, D.3.1 and D.3.2. Its
  case for deleting the legacy kernel now was declined the same morning
  and then taken (maintainer decision 2026-10-27): B.8.1 deleted it.
    - [x] **A.14.1** `[I1]` One definition of the score bands
      `MATE_SCORE` and `MAX_PLY` are defined in `tt.h:106-107` and
      `search_types.h:18-19`, and `KBNK_STATIC_MATE_FLOOR` is a literal in
      `eval.h`, held equal by static asserts in `search_worker.cpp:28` and
      `tests/test_endgames.cpp:33`; DESIGN §3 calls the pair a silent
      corruption waiting for a change to one. `constants.h` already holds
      the tablebase band for the same reason and depends on nothing.
      Move `MATE_SCORE`, `MAX_PLY`, `INF_SCORE`/`INF_EVAL` and
      `VALUE_NONE`/`NO_SCORE` there, derive the KBNK floor from them,
      delete the duplicates and the asserts that only pinned them, and
      update DESIGN §3's sentence. Exact fingerprint on both arms; CTest.
      - Closed 2026-10-28: `constants.h` defines `MAX_PLY`, `MATE_SCORE`,
        `INF_SCORE` and `VALUE_NONE`, and the tablebase band is derived
        from them; the table's `INF_EVAL`/`NO_SCORE` became `INF_SCORE`/
        `VALUE_NONE`, its `DECISIVE` and `TB_VALUE` are the band's names,
        `KBNK_STATIC_MATE_FLOOR` is `MATE_SCORE - MAX_PLY`, and the three
        pinning asserts are gone. `bench 13` 19,470,325 on the release,
        tune, diagnostic and ablation builds; CTest 24/24 release, 22/22
        diagnostic and tune.
- [x] **A.15** Universal binaries and the release runners
  Added 2026-10-27 at the maintainer's request
  (`analysis/code_audit_2026-10-27.md` §4). Stockfish 19 (2026-09-05)
  ships one fat binary per OS and architecture — every ISA level a full
  PGO build in its own namespace, dispatched once at `main` — in place of
  the 27 per-ISA assets of Stockfish 18. Basilisk ships nine per-ISA
  assets. Its GitHub runner labels were checked on 2026-10-27 and are all
  current (`ubuntu-26.04` and `-arm` for CI, `ubuntu-24.04` for the Linux
  release cells, `windows-2025`, `windows-11-arm`, `macos-26`; none
  deprecated), so the runners need no label change; the leaves are the
  design, the measurement that decides it, and the hygiene the check
  found. Restructured 2026-10-28 when the maintainer chose to build the
  binary now: the leaves run in their numbered order, and the ladder read
  that was A.15.2 is A.15.4, read from the universal builds.
    - [x] **A.15.1** `[R2]` Universal x86-64 binary: design packet
      Write `analysis/universal_binary_2026-10.md` from the audit's §4:
      the Stockfish 19 mechanism (`src/universal/`, the
      `universal-object-pgo` target, renamed init sections, the
      baseline-first link, the SDE selection matrix) and Basilisk's form
      of it — one `BASILISK_NS` namespace wrap (behaviour-neutral, exact
      fingerprint), Fathom built once at baseline, constant-initialised
      globals in `syzygy.cpp`, per-tier clang profiles with one LTO link,
      `src/main.cpp` as the baseline dispatcher with a `BASILISK_ISA`
      override, `verify_isa.py`'s contract for shared code, the tier
      namespaces and the whole image, the SDE test matrix, PGO liveness
      per tier — with the traps (first-wins symbols, early initialisers,
      profiles matched by mangled name; D-72) and the costs (three builds
      per x86 cell, 5–6 MB assets, nine assets to five). It rejects
      function multiversioning (no IFUNC on COFF; it forfeits the
      whole-engine codegen the tier flags give), macOS universal2 (Intel
      macOS ends with macOS 26) and a launcher process, each with its
      reason. The decision rule for building it is frozen here, before
      A.15.2 reads (proposed; the maintainer may amend it before the
      read): pulled into Phase A if the pext-over-portable ladder is at
      least 3% pooled NPS (about 6 Elo for a user on the wrong asset);
      otherwise G.2 builds it when Phase F's SIMD tiers arrive. No engine
      source changes.
      - Closed 2026-10-28: `analysis/universal_binary_2026-10.md`. The
        design is Stockfish's fat binary with two departures: Fathom
        built once at baseline outside every tier, and tier objects that
        carry no initialiser (only `syzygy.cpp` and Fathom have one
        today), so no section rewriting. Each tier is a full PGO+LTO
        executable under its `BASILISK_NS` namespace whose LTO object is
        extracted; one LTO link over every tier's bitcode is rejected
        (a prevailing `linkonce_odr` copy carries one tier's target
        attributes to every caller). The dispatcher adds the donor's
        slow-`pext` rule (AMD family 15h/17h) and a `BASILISK_ISA`
        override; `verify_isa.py` attributes the image by the linker
        map. Size about 3.5-5 MB per x86 asset; nine assets become five.
        The rule above is frozen as proposed, with the packet's
        prediction (`pext` over `portable` +7% [+4%, +12%], about 90%
        that it clears +3%). Because the rule therefore nearly always
        fires, the packet (§9) recommends an amendment for the
        maintainer to choose before A.15.2 reads: pull the build forward
        only on evidence of harm from asset choice, otherwise build it
        at G.2. A.15.3's advisory must also cover `pext` on a slow-`pext`
        CPU (A-36).
      - 2026-10-28: the maintainer chose to build it now, the packet's
        counter-argument; no rule is left for the ladder to decide.
    - [x] **A.15.2** `[I2]` Universal x86-64 binary: tiers, dispatch, link
      Added 2026-10-28. The packet's §4 in the form of its amendment 1:
      each tier is the ordinary PGO build of its preset with the engine
      link's LTO objects kept (`BASILISK_UNIVERSAL_TIER`), and
      `cmake/universal-tiers.cmake` renames every symbol a tier defines
      under the tier's name, so no 57-file namespace wrap. Only type
      identities (`_ZTI`, `_ZTS`) and `.refptr` data stay shared; the
      script fails on anything else left global and on any initialiser in
      a tier object. The baseline dispatcher (`src/main.cpp` under
      `BASILISK_UNIVERSAL`) links the three tiers with one Fathom and picks
      pext unless the CPU is AMD family 15h or 17h, then avx2, then
      portable; `BASILISK_ISA` forces a tier. `verify_isa.py --tier
      universal` attributes each function by its tier's prefix. Exact
      fingerprint on every tier, on Windows and on Linux (WSL, Clang 19).
      - Closed 2026-10-28 (`0c1c0f4`, `3508756`; the packet's amendment 1).
        Every tier and `BASILISK_ISA` reproduce 19,470,325 on Windows
        (clang 23.1.4) and Linux (clang 19.1.1, static); the universal ISA
        contract passes on both, and planted instructions and synthetic
        images fail it. The assets are 3,513,344 bytes (Windows) and
        5,140,440 (Linux). CTest 22/22. Two findings changed the plan: ThinLTO
        and the static libc++ leave tier symbols global, so every symbol is
        renamed instead of wrapping a namespace; and `tzcnt` (LLVM's `bsf`)
        and SSE3 (MSYS2's runtime floor) stay allowed in portable code.
    - [x] **A.15.3** `[I1]` Tier advisory, ISA contract and toolchain pins
      (D-25, A-36, taken over from D.3.2) `src/main.cpp` prints an
      `info string` on stdout when a lower tier runs on a CPU that could
      run a higher one, naming the asset, and the refusal of an
      incompatible tier goes to stdout too (today stderr only), with a
      unit truth table beside `test_cpu_features`; `tools/diag/
      verify_isa.py`'s portable set forbids SSE4.1, AVX1 and BMI1
      mnemonics (`andn`, `tzcnt`) as it forbids `popcnt`, checked once
      with a planted instruction; `release.yml` and `ci.yml` pin the
      macOS Xcode major as `.github/windows-clang-version` pins clang,
      and drop the stale `brew install ninja` step and comment (the
      `macos-26` image ships Ninja); the Linux release cells record
      their plan for a baseline `libstdc++.a` after `ubuntu-24.04`
      retires. Exact fingerprint; a release dispatch run proves the
      matrix.
      - Implemented 2026-10-28 with A.15.2 and A.15.5 (`0c1c0f4`,
        `3508756`, `c81e220`): the advisory and the stdout refusal in
        `src/main.cpp`, the dispatch truth table in `test_cpu_features`,
        the portable set (corrected: `tzcnt` and SSE3 stay allowed, the
        packet's amendment 1), `.github/xcode-version` checked in CI and
        Release, the Ninja step gone, the libstdc++ plan (an `ubuntu:24.04`
        container) in `release.yml`. Closed 2026-10-29 on local evidence:
        the truth table in CTest, the planted-instruction check, the
        stricter sets passing on all three tier builds, both workflows
        valid YAML. The Xcode pin's first run is this branch's pull
        request (CI and the Release candidate run on it).
    - [x] **A.15.4** `[V]` Universal binary speed and the ISA tier ladder
      (D-24; was A.15.2, the ladder alone) BAS-P20, registered before any
      reading: four universal builds and four pext builds, each hop read
      directly (`tools/nps_pool.ps1`, `tools/nps_read.ps1`). The universal
      binary against the pext asset under the no-regression form is the
      gate that keeps it; the pext tier build against the pext asset
      locates a loss; the ladder avx2/portable, pext/avx2 and pext/portable
      is read from the universal builds' tier builds, which are the old
      per-tier assets. Priors: Rarog read avx2 +4.59% over base and pext
      +2.45% over avx2; the packet's §9 prediction is frozen.
      - Closed 2026-10-29 (BAS-P20). The host-load measure was broken and
        refused quiet hosts (WMI's raw counters, repaired in `7996e4d`);
        after the repair every cycle was undisturbed. The universal binary
        reads **−0.58%** [−0.82%, −0.33%] against the pext asset, FAIL,
        so by the registered rule the release keeps its per-tier assets.
        The pext tier build reads +0.04% [−0.12, +0.19]: the loss is in the
        relink, whose hot code is byte-identical and whose functions sit
        16 bytes off their 64-byte placement in all four builds (A.15.6).
        The ladder: avx2 over portable **+5.04%**, pext over avx2
        **+2.82%**, pext over portable **+8.49%**, each inside the
        packet's frozen interval.
    - [x] **A.15.5** `[I1]` Release: one x86-64 asset per OS
      The release's six x86 cells become two universal cells, Linux and
      Windows, each building the `universal` preset and checking
      `verify_isa.py`'s universal contract and every forced tier's
      `bench 13`; an SDE job runs the packet's selection matrix (§4.7).
      Nine assets become five. README, CHANGELOG and
      `docs/release_tiers.md` say which tier a CPU gets and how to force
      one. A release dispatch run proves the matrix (maintainer).
      - Implemented 2026-10-28 (`c81e220`, `089fe7d`): two universal cells
        with the universal contract and every forced tier's bench, the
        `select` job running `tools/diag/check_universal.py` under SDE
        9.58.0 (the donor's pinned action), five assets in `collect`.
        The checker was exercised with a native stand-in for SDE; SDE
        itself first runs in the dispatch run.
      - Closed 2026-10-29 NOT SHIPPED: BAS-P20's gate failed, so the
        release's universal cells and the SDE job are reverted to the nine
        per-tier cells, keeping A.15.3's Xcode pin, Ninja step and
        libstdc++ note. The `universal` preset, the dispatcher,
        `verify_isa.py --tier universal` and `tools/diag/check_universal.py`
        stay in the tree for A.15.6.
      - Shipped after all by A.15.6 (2026-10-29), with the tiers aligned
        to 64 bytes, once BAS-P21 passed.
    - [x] **A.15.6** `[R2]` Universal binary: the relink's code placement
      Added 2026-10-29 from BAS-P20's diagnosis. The universal binary's
      pext code is byte-identical to its tier build, but the relink moves
      the hot functions by 16 bytes modulo 64, and the read was 0.58%
      slower. Four builds of one source share that placement, so a pool
      does not average it out.
      - First, whether alignment is the cause: a padding sweep, 0, 16, 32
        and 48 bytes of padding before the pext tier's objects in the
        final link, each against the pext asset. If the sweep moves NPS by
        about the loss, placement is the cause; if not, the hypothesis is
        refuted and the cause is open.
      - Then a repair that makes the tier's placement independent of the
        link: `-falign-functions=64` on every build (a speed change of its
        own, read against today's pext asset first), or 64-byte alignment
        of each tier's code at the start of its objects, matched to its
        tier build.
      - Gate: a new registered no-regression read of the repaired
        universal binary against the pext asset, with the padding control
        beside it. The release adopts the universal binary only if it
        passes; the cells of A.15.5 come back from `c81e220`.
      - Research amendment 1 (2026-10-29): the images answered the cause
        without a sweep. lld's call-graph sort puts `operator delete`
        (`_ZdlPv`, a 16-byte runtime stub) inside the pext tier build's
        hot cluster; in the universal link the tiers share it and it lands
        elsewhere, moving every later hot function 16 bytes. `/order`
        cannot place runtime symbols and GNU ld does not sort, so the
        candidate is `-falign-functions=64` on the tiers (`4488e13`): 554
        of 559 pext functions on a cache line in both images, identical
        code, every tier exact. BAS-P21 replaces the sweep: the universal
        binary against the pext asset (the gate), against its own tier
        build (the relink alone) and the tier build against the pext asset
        (the alignment alone). Pools built; the reads need the host idle
        (`pwsh tools/run_a156_nps_reads.ps1`).
      - BAS-P21 registration amendment 1 (2026-10-29, before exposure): the
        three reads run as one read of the three pools with the same three
        contrasts and rules, about 9 minutes at step 1 instead of about 30.
      - BAS-P21 result (2026-10-29): the universal binary PASSES against
        the pext asset (+0.00% at step 1; −0.29% [−0.73, +0.16] at six
        cycles); the relink alone reads −0.13% and the alignment alone
        −0.16%, both NO_CHANGE, so placement was BAS-P20's whole loss. By
        the registered rule the release takes the universal cells back
        from `c81e220`, with the alignment.
      - Closed 2026-10-29 SHIPPED. `release.yml` is `c81e220`'s again:
        two universal x86-64 cells (the `universal` preset, which sets the
        alignment), every forced tier benched, the SDE `select` job, five
        assets in `collect`. A universal configure without
        `BASILISK_ALIGN_FUNCTIONS=64` now fails, so the unaligned layout
        cannot ship by accident. README, PROCESS, `docs/release_tiers.md`
        and CHANGELOG describe one x86-64 asset again. Built locally from
        the final tree with the release cell's commands: every forced tier
        exact and the universal ISA contract passing. As for A.15.3 and
        A.15.5, the release workflow itself is proved by its first run on
        GitHub, the candidate run of the PR that carries it.
      - Trap audit (2026-10-29, after closing, at the maintainer's request
        with Rarog's notes): each universal-binary trap now has a check
        that was watched failing on a planted fault. New: the stale-tier
        guard (one source, one compiler), the image's alignment and
        single-Fathom checks, unprofiled files as errors in tier builds,
        and the tier object checks runnable on their own
        (`analysis/universal_binary_2026-10.md` §14).
    - [x] **A.15.7** `[I1]` Universal binary: an sse41-popcnt tier
      Added 2026-10-29 at the maintainer's request, after comparing
      Stockfish's universal binary (eight x86 builds: `x86-64`,
      `sse41-popcnt`, `avx2`, `bmi2`, `avxvnni`, `avx512`, `vnni512`,
      `avx512icl`). Most of what the avx2 tier gains over the base is the
      hardware `popcnt`, which Nehalem to Ivy Bridge and AMD Bulldozer to
      Steamroller have but the base build cannot use. A fourth tier,
      `-msse4.1 -mpopcnt` (`USE_SSE41_POPCNT`, preset
      `release-sse41-popcnt`), is selected on a CPU with SSE4.1 and POPCNT
      and no AVX2; AMD K10 (POPCNT, no SSE4.1) stays on the base.
      - Names: Basilisk keeps its own (`base`, `sse41-popcnt`, `avx2`,
        `pext`), not Stockfish's `x86-64` and `bmi2`: the download is
        already named `x86_64`, and `pext` names the one instruction
        Basilisk's hand-written code uses, as its preset and slow-PEXT rule
        do.
      - Checks: the tier's ISA contract (popcnt, nothing of SSE4.2 or
        later), the SDE selection (Nehalem, Sandy Bridge and Ivy Bridge),
        the truth table, the forced-tier bench, and the universal build's
        guards over four tiers.
      - Stockfish's AVX-512 and VNNI builds serve its network's arithmetic;
        A.17 and G.2 hold them as candidates to try.
      - Closed 2026-10-29. Built from the final tree with the release
        cells' commands on Windows (clang 23.1.4) and Linux (WSL, Clang
        19.1.1, static): every guard passes over four tiers, the ISA
        contract passes, 558 or 484 of each tier's functions on a cache
        line, every forced tier 19,470,325 with its banner, the CPU truth
        table 49/49. Indicative speed on the 5950X, each tier forced in one
        binary: `sse41-popcnt` +5.6% over `base` (`avx2` +6.0%, `pext`
        +9.2%). Size 4.2 MB on Windows, 5.8 MB on Linux. The SDE job and
        the release workflow are proved by the PR's candidate run.
    - [x] **A.15.8** `[I1]` Three presets: release, universal, debug
      Added 2026-10-29 at the maintainer's request. Nobody building for their
      own machine should pick a tier: `release` builds with `-march=native`
      and turns PEXT on where the building CPU runs it fast, by the
      universal binary's own rule (`src/cpu_detect.h`, probed at configure
      time by `cmake/probe_cpu.cpp`; `-DUSE_PEXT` overrides it). The
      `release-pext`, `release-avx2`, `release-sse41-popcnt` and
      `relwithdebinfo` presets go: the universal build, `build_test.ps1` and
      CI build a tier from `release` with its option, the PGO step passes
      the options to its child builds, and the profiler never used
      `relwithdebinfo` (`build_test.ps1 -Flavor Profile`). The README leads
      with `cmake --build --preset release --target pgo`; PGO stays a
      target, not the default.
      - BAS-P22, registered before any reading: the native release against
        the universal binary and the pext asset, the four-tier universal
        binary against the pext asset, and, by amendment 1, all three
        compilers at their best (GCC and Clang native; MSVC, which has no
        native target, at its pext build), with MSVC also against Clang on
        the same instruction set. MSVC still builds, `bench 13` exact, in no
        CI job. MSVC stays only if the read
        keeps it within reach and it then gets a CI job like GCC's; the
        maintainer decides.
      - BAS-P22 result (2026-10-29, read twice, the repeat on a quieter host
        with intervals ten times narrower, every verdict agreeing): the
        universal binary matches the pext build (−0.14%, PASS); GCC native
        reads 2.7% behind Clang native; MSVC 14.4% behind Clang on the same
        instruction set, so the registered recommendation is to drop it
        (maintainer's decision).
      - **Returned to research (2026-10-29):** the leaf's premise is false
        on this host. Clang's `-march=native` build reads **1.0% slower**
        than the fixed pext instruction set ([−1.14%, −0.86%]; the first,
        noisier read −1.5%), so `release`
        is not the fastest build for this computer. Options:
        1. `release` builds the detected tier with the release's fixed
           flags (the universal binary's tier for this CPU) and drops
           `-march=native` by default: the measured fastest here, the same
           code users download, and one less variable in measurement.
        2. Keep `-march=native` and find what costs the 1.5% (the Zen 3
           tuning, or an extension), then decide.
        3. Keep native as it is and say in the README that the download is
           as fast.
        The recommendation is 1: it is what this read measured fastest, and
        option 2's question can still be read later as a contrast.
      - Closed 2026-10-29 with the maintainer's choices: option 1, and MSVC
        dropped. `release` probes the CPU and builds its tier with the
        release's fixed flags (`-march=native` only with `-DNATIVE=ON`); a
        named tier or an explicit `PORTABLE_BUILD`, as CI, `build_test.ps1`
        and the universal build pass, skips the probe. MSVC's preset,
        `-DCOMP=msvc` (now refused), its flags and PGO branches and
        `build_test.ps1 -Msvc` are gone; the sources keep their small
        `_MSC_VER` guards (CPUID, prefetch). Verified: a plain `release`
        PGO build detects `pext`, compiles the pext tier's flags with no
        `-march=native` and benches 19,470,325; `build_test.ps1` Release,
        `-Native` and `-Universal` build and bench exact; CTest 24/24. A.17
        candidate 5 owns why native is slower.
      - Names and defaults (2026-10-29, before the PR merged): `release` and
        `universal` build PGO by default (build presets), `--target nopgo`
        builds the engine alone and `--target all` the development loop;
        every build lands in `build/dist` as
        `basilisk-<version>-<os>-<x64|arm64>[-<tier>][-<kind>]`, the version
        the engine reports, read at build time (`cmake/dist-copy.cmake`);
        release assets are `basilisk-<X.Y.Z>-<os>-<x64|arm64>`.
    - [x] **A.15.9** `[I2]` GCC as a full equal: Stockfish's universal route
      Added 2026-10-29 at the maintainer's request, to run on its own branch
      after the universal binary's PR merges. GCC builds and passes CI for
      normal and PGO builds, but the universal binary is Clang-only: it
      renames the post-LTO objects that only LLVM's linkers keep (lld
      `-lldsavetemps`, gold's LLVM plugin `save-temps`). Clang stays the
      default and the release compiler (BAS-P22: GCC 2.7% slower); the aim
      is that every build works and is tested with both.
      - The universal route becomes Stockfish's, for both compilers, so there
        is one route: each tier compiled with link-time optimisation to an
        assembly file, its symbols renamed in that text, then assembled and
        linked (Stockfish's `universal-object-pgo` and
        `rewrite_asm_sections.awk`, read at `edb0d9db`). Every guard of
        `analysis/universal_binary_2026-10.md` §14 is carried over and seen
        to fail again on the new route: still-global symbols, initialisers,
        a tier without its profile (GCC's own missing-profile check), stale
        tiers, ISA attribution and alignment.
      - CI: GCC test cells beside Clang's (Linux and Windows), a GCC
        universal build in the release candidate run with the forced-tier
        bench and the ISA contract.
      - Re-measured, registered before reading: the new route's Clang
        universal against today's (placement, BAS-P20/P21's lesson) and
        against the pext build; GCC's universal against GCC's pext build.
        The release keeps Clang's universal binary unless the new route
        reads slower, in which case Clang keeps today's route and only GCC
        uses the assembly one.
      - Interactions: A.17 candidate 5 (code generation) reads on the same
        builds; G.2 extends the tier list on whichever route ships.
      - Implemented 2026-10-29 (`analysis/universal_binary_2026-10.md` §15):
        GCC's `-save-temps` keeps each LTO partition's assembly; Clang links
        the tier's objects a second time with the linker writing assembly
        (`basilisk_tier_asm`); `cmake/universal-asm.py` renames every symbol
        the tier defines in that text, with the COFF section names built
        from them, and the result is assembled. The earlier Clang route
        stays selectable (`BASILISK_UNIVERSAL_ROUTE=objects`) for BAS-P24.
        The first builds found four things, each fixed and recorded: GCC's
        PE COMDATs are the `.rdata$.refptr.<symbol>` pointers, matched by
        section name, 90 a tier to the tier's own symbols (the evaluation
        tables among them), which a symbol-only rename would have shared
        silently; GCC's LTO output defines libstdc++'s type identities as
        ordinary data (238 multiple definitions), so GCC tiers rename them
        too and exceptions still match by name; Clang's `$` lambda names
        escaped the first tokenizer, and the tier check now covers local
        symbols as well; GCC does not align never-run functions
        (207 of 596), fixed by `-fmin-function-alignment` (GCC 14).
        Linux added two: the gold plugin's extensionless assembly files
        (assembled with `-x assembler`), and GCC's `.cold` fragments, which
        the alignment rule now leaves out.
        Verified on Windows (clang 23.1.4, MSYS2 GCC 16.2.0) and Linux (WSL
        Ubuntu 24.04, Clang 19.1.1 with gold and GCC 14.2, static): all four
        universal binaries bench 19,470,325 on every forced tier with its
        banner and pass the universal ISA contract, every function on a
        cache line but Clang's stubs (558 of 562 on Windows, 483 of 484 on
        Linux); every guard of §14 plus the two new ones seen to fail on a
        planted fault on the new route. GCC's portable
        build passes CTest 24/24 and the single-tier contract, `bench 10`
        3,684,650 as Clang's. CI: GCC cells for every x86 tier on Linux
        and Windows in the bench agreement; the release builds and checks
        GCC's universal binary on both, unpublished.
      - BAS-P24, registered before any pool was built, read on the 5950X
        (four reads, amendments 1 to 3): the assembly route against Clang's
        objects route +0.33% [−0.19%, +0.86%] and against the pext build
        +0.04% [−0.21%, +0.29%], both PASS; GCC's universal binary against
        GCC's pext build +0.41% [+0.06%, +0.77%], PASS; GCC against Clang
        −3.47% universal and −3.70% pext.
      - Closed 2026-10-29 by the registered rule: the objects route is
        removed, so Clang and GCC build the universal binary one way. Before
        closing, every trap of Rarog's notes (`analysis/universal_binary_2026-10.md`
        §15) was re-shown failing on both compilers, and the hot pext-tier
        code of the universal binary was found identical to the tier's own
        PGO build (GCC byte for byte; Clang every instruction and its line
        offset, the padding encoded differently). Rebuilt after the removal
        on Windows and Linux with both compilers: every forced tier
        19,470,325, the universal ISA contract passing. The PR's first CI
        run found that ubuntu-26.04's GCC 15 targets AVX, BMI and POPCNT by
        default, so its "portable" builds were not; a portable build now
        pins `-march=x86-64` where the compiler's default is above the
        baseline (a GCC emulating that default then passes the portable
        contract), and every other compiler builds as before.
- [x] **A.16** `[I1]` Zero-weight evaluation terms off the hot path
  Added 2026-10-28 (simplification read of the kernel and the evaluator
  against the donor, after A.14.1; the lines are cited here). Terms whose
  every weight is 0 are still computed on every full evaluation:
  - the winnable block (`eval.cpp:1975-2009`): all seven `win_*` weights
    are 0, yet it reads both kings' files and ranks, the flanks, the
    piece census and the passer count each call;
  - the hanging-piece loop (`eval.cpp:1706-1722`): `hang_pen` is all
    zeros, absorbed by the threats package in Phase 4.2 (C-37);
  - five king-danger inputs (`eval.cpp:1552-1592`): `ks_ring_pressure`,
    `ks_flank_attack`, `ks_flank_defense`, `ks_pawnless_flank` and
    `ks_open_file` are 0, but the flank zone's three popcounts and the
    ring popcount run for both colours.
  Deleting them is behaviour-neutral: each adds its weight times a
  feature, and the winnable step adds `sgn * max(0, -|eg|)`, which is 0.
  The space loop stays (`space_blocked_mg` is live).
  - Gate: exact `bench 13`; CTest; a pooled-PGO NPS read registered
    before it runs (maintainer-run, PROCESS's method). The deletion is
    kept if the read is faster than BAS-P18's null interval
    (−0.19% [−0.66, +0.27]); otherwise it is reverted and the slots wait
    for their Phase C owners.
  - Interactions: the slots' owners rebuild them in the donor's form
    rather than refit them — king danger in C.3 (C-111: flank attack and
    defence and the pawnless flank were nil in Rarog), the threats in
    C.4 (C-37), the complexity term in C.5.3 (C-47: never fitted, the
    tuner has no working winnable instrument). If the deletion is kept,
    C.5.3 builds the donor's winnable term instead of refitting this
    one, and the texel trace loses the `HangPen` sites with the loop.
  - Implemented 2026-10-29 (`ba67ecc`): the three computations are
    deleted, their parameters kept (a revert is the one commit), and
    `load_eval_params` refuses a nonzero value for any of the thirteen
    groups, so a tune cannot move a weight nothing reads. Verified:
    `bench 13` 19,470,325; CTest 24/24; the TUNE and TEXEL flavors
    build; a dumped parameter file loads and benches exact, and a
    planted `WinBias 7` or `HangPen 25` is refused.
  - BAS-P25 registered before any pool was built; both pools are built
    (`a16-base` at `0680393`, `a16-cand` at `fcb3819`, every build
    19,470,325).
  - Closed 2026-10-29: BAS-P25 read the deletion **+2.35%** NPS
    [+2.01%, +2.70%] (ACCEPT at step 1, undisturbed), above the null
    interval, so it is kept. The slots' Phase C owners build the donor's
    forms (C.3, C.4, C.5.3); until then their parameters stay at 0 and an
    eval file cannot set them.
  - Amended 2026-10-30 at the maintainer's review: the thirteen slots are
    removed too, with the loader's guard and the tuner's `hanging` and
    `winnable` groups. They were kept only so a failed read could be
    reverted in one commit, and BAS-P25 kept the deletion. Verified:
    `bench 13` 19,470,325; CTest 24/24; the TUNE and TEXEL flavors build;
    a dumped parameter file (1,159 slots) loads and benches exact, and one
    naming `HangPen` is refused as unknown; `--audit-coverage` reads texel
    1,093, kingsafety 52, material 10.
- [x] **A.17** `[R2]` Instruction-specific speed for the avx2 and pext tiers
  Added 2026-10-29 at the maintainer's request, after A.15 shipped the
  universal binary. The tier ladder (BAS-P20) reads `avx2` +5.04% over the
  base build and `pext` +2.82% over `avx2`. Almost all of the first is the
  hardware `popcnt` the compiler may then use; the second is the one piece
  of hand-written instruction-specific code, the rook and bishop lookups by
  `_pext_u64` in `attacks.cpp`. Everything else the two tiers gain is
  compiler code generation. The question: can code written for these
  instruction sets buy more speed, without changing a single move?
  - Candidates, each a hypothesis to measure, not a change to make:
    1. **Compressed slider tables (pext tier).** The PEXT tables hold
       107,648 attack sets of 8 bytes, about 860 KB, beyond the L2 cache.
       Storing each set as a 16-bit index of its ray squares and
       expanding it with `pdep` cuts them to about 215 KB, at the cost
       of one `pdep` per lookup. First measure how many slider lookups
       miss the cache (A.9.5's profiler, or hardware counters), which
       bounds the gain.
    2. **PEXT and PDEP in the evaluation (pext tier).** Bit-pattern
       lookups such as king shelter and storm, or pawn-structure
       patterns, indexed straight from a bitboard. Survey first which
       evaluation terms are table lookups by pattern at all.
    3. **AVX2 in the evaluation (avx2 and pext tiers).** The hand-written
       evaluation is scalar. Survey what the compiler already vectorises,
       then whether a feature or PST accumulation vectorises by hand.
       Likely small before a network evaluation; Phase F's NNUE is where
       AVX2 pays, so this must not build what Phase F replaces.
    4. **AVX-512 and VNNI tiers.** Stockfish's universal binary adds
       `avxvnni`, `avx512`, `vnni512` and `avx512icl` builds. For the
       hand-written evaluation they are worth trying only if candidate 3
       finds vector code that pays; G.2 owns them for the network. The
       5950X has no AVX-512: SDE checks such a tier's selection and
       fingerprint, but a speed read needs hardware that has it (Intel
       Ice Lake or later, AMD Zen 4 or later), which may not be at hand.
    5. **Code generation: why `-march=native` is slower.** BAS-P22 read
       Clang's native build 1.0% slower than the fixed pext instruction set
       on this Zen 3 ([−1.14%, −0.86%], twice). Native differs in three
       separable ways, each one pool in one read: the pext build with
       `-mtune=znver3` (the tuning model alone), native with
       `-mtune=generic` (the extra instruction families alone: FMA, MOVBE,
       ADX and the like), and native with 64-byte function alignment
       (layout luck, as in BAS-P20). If the extra families help and the
       tuning hurts, a tier with those families and generic tuning may beat
       today's pext build, for local builds and, where every CPU of the tier
       has them, for the shipped tiers. Only this Zen 3 can be read here:
       a CPU-specific choice (tuning) applies to local builds only, and the
       shipped tiers keep generic tuning unless other hardware is read.
       BAS-P23 repeated BAS-P22 on the maintainer's Intel laptop:
       inconclusive (a host that never idles, intervals about ±2%),
       native again no faster in both reads.
  - Rules: every candidate is behaviour-neutral (`bench 13` identical in
    every tier, the base build untouched), keeps the ISA contract and the
    universal binary's checks, and respects the slow-`pdep` rule (AMD
    family 15h and 17h already run the `avx2` tier).
  - Method: a survey that ranks the candidates by measured headroom, then
    one registered pooled NPS read per candidate kept, on its tier's
    single-tier build (one read with named contrasts when they share
    pools), deciding by PROCESS's gain form. The universal binary picks
    up every kept change through its tier builds.
  - Interactions: A.16 removes evaluation compute first, so the survey
    measures the evaluation that remains; B.7's speed work and Phase F's
    network evaluation are the other owners of NPS, and the survey names
    any candidate that belongs to them instead.
  - Closed 2026-10-30, `NO_CHANGE` (BAS-P26). The survey, a sampling
    profile of `bench 13` after A.16 (`tools/results/a17-profile/head/`):
    board 26.7%, move ordering 24.6%, search 19.8%, evaluation 17.2%,
    attacks 5.4%, TT 4.7%.
    1. Compressed tables: built behind an umbrella option and read
       **−0.98%** [−1.19%, −0.78%]. The flat tables' hot lines stay in
       cache, and `pdep` lengthens every lookup; the arm is removed. An
       exhaustive lookup test (every square and occupancy against a ray
       walk, in `test_magics`) stays.
    2. PEXT in the evaluation: king shelter and storm, the only
       pattern-like loops, are 0.75% of a node, below what a read resolves.
       Not built (`NOT_WORTH_PURSUING`).
    3. AVX2 in the evaluation: the scalar hotspot is the material imbalance
       (2.89%), which depends on material alone, so it belongs to B.7 as a
       material-keyed cache, not to hand vectorisation. Not built.
    4. AVX-512 and VNNI: nothing in the evaluation to build them on, and no
       such hardware here. Left to G.2.
    5. Code generation: `-mtune=znver3` **−0.95%**; FMA, F16C and MOVBE
       (the rest of x86-64-v3) +0.02%; native −1.04%, of which its tuning
       −1.01% and its instruction families −0.04%; native's layout +0.22%,
       within noise. The pext tier and the release flags stay as they are.
  - For B.7, measured here: the material imbalance recomputes 48 products
    of piece counts on every evaluation (2.89% of a node), and material and
    PST are summed from scratch over every piece (1.37%); both could be
    cached or kept incrementally. The comments calling the imbalance and
    the small positional terms "seeded inert" are stale: both are fitted.

## Phase B — Search programme (closed leaves)

- [x] **B.0** Search programme investigation
  Investigation (`R3`): the Basilisk-versus-Stockfish mechanism map, cluster
  contents, scale ratio and seed rule, survivors, SPSA surfaces, B.2.2's
  registered screen numbers, frozen B.2 predictions and handoffs for B.1–B.3.
  No engine source changes.
  **Research delivered 2026-10-06:** `analysis/search_programme_2026-10-06.md`
  ends `READY_FOR_IMPLEMENTATION` for B.1 and B.2, B.3 contingent on the
  accepted B.2 head, with the donor pinned at Stockfish `sf_19` (`edb0d9db`).
  Measured on the 1.10.1 head (BAS-D20, BAS-D21): per-ply growth equals the
  oracle's (1.767 against 1.757) but the tree is a constant ×4 from depth 4
  on; with the shallow move-loop pruning family removed on both matched
  ablation builds the trees are equal at depth 4, so that family carries the
  multiplier (oracle 4.5× of selectivity against Basilisk's 2.2×); Basilisk's
  razoring costs 51 WAC positions at 100k nodes and its LMR 15, where the
  oracle's cost none; the evaluation scale is 0.282 of Stockfish 19's and
  0.557 of the oracle's with a residual over half its own magnitude, so the
  oracle column seeds every evaluation-unit constant. Phase 5's "width is a
  symptom" premise stays refuted; the check policy is not the differential
  (both extension families cost 1.9× nodes for no fixed-node tactics) and
  stays B.3's. Retry triggers recorded: BAS-S07/S10 fire at B.2, BAS-S08/S09/
  S11 at B.3, BAS-S12 not fired. Two cheap maintainer-run game tests were
  registered and spawned below; neither blocks B.1, and B.0 closes when both
  have been read into the packet's calibration.
  — CLOSED 2026-10-08: both read into packet §11, inside their 80%
  intervals; no falsifier fired, the cluster order and P2 stand.
  The deliverable covers:
  1. The donor revisions pinned.
  2. Zero-game measurements on the 1.10.1 head, with the A.5 instruments:
     - the branching profile and the depth reached at 300k nodes;
     - WAC at fixed nodes;
     - stride-1 counters, including the share of LMR reductions that land in
       qsearch, the re-search rate after reduced fail-highs, NMP conversion,
       the qsearch share, the check-extension population (BAS-S16 measured
       15.84% of interior nodes) and history-pruning activity (last measured
       near-dead).
  3. The scale ratios and the three-column seed rule.
  4. The mechanism map, per area: node entry (draw value, upcoming
     repetition as BAS-S12's retry, mate distance); TT; static eval and
     correction; node-level pruning; singular and extensions, including
     Basilisk's check extension; the picker and histories, including whether
     killers go; move-loop pruning and LMR; plus qsearch, root and time as
     boundaries.
  5. The interaction map and shared signals.
  6. Cluster contents confirmed or changed, and the board producers the
     clusters need.
  7. Survivors with local evidence (operating rule 4).
  8. SPSA surfaces, with categorical switches kept apart.
  9. Counter re-keying and the ablation-mask disposition.
  10. B.2.2's screen numbers: a branching window, WAC floors and targets,
      the agreement floor, the time-to-depth floor, and the paired-run floor
      and target.
  11. Frozen predictions for B.2.
  12. Handoffs for B.1, B.2 and, contingent on B.2's head, B.3.

  A.2.3's `analysis/parameter_inventory_v1.md` is an input: the live list,
  and no SPSA surface moves `LmrBase` while capture futility is in the tree.
  It also answers the four `DESIGN.md` questions for the programme. It
  tests Phase 5's "width is a symptom" premise, and it records which retry
  triggers the programme fires: BAS-S07, BAS-S10 and BAS-S12 name a history
  ownership change; BAS-S08 and BAS-S09 name a joint architecture and fit.
    - [x] **B.0.1** `[V]` Razoring depth reach in Elo (maintainer)
      BAS-S17, registered 2026-10-06: `RazorCoeff=500` against 243 on the
      B.0 Tune binary, 2,000 paired games at `3+0.03`, no adjudication,
      through the A.3.1 run-file path; a Tune-build diagnostic, never
      acceptance. Frozen: +8 Elo, 80% interval [−4, +20]. The reading rule
      in BAS-S17 decides whether B.2 seeds razoring from the oracle column
      (depth 1, 256 cp) without a categorical re-test. Prepare the run file
      and the one runnable command; append the calibration to BAS-S17 and
      to the packet's §11.
      Prepared 2026-10-08: `tools/colosseum.ps1 -Mode match`
      (`match-fixed.toml`), one binary two option sets, 1,000 paired
      openings, seed 20101, 14 slots, revision and bench pinned; dry run
      accepted.
      — CLOSED 2026-10-08: **+2.4 Elo**, 95% interval [−7.5, +12.3]
      (pentanomial [50, 224, 431, 252, 43], 0 faults, every game ended
      naturally). The interval holds 0, so neither branch of the reading rule
      fires and B.2 decides razoring's depth cap with the cluster.
      Calibration: inside the 80% interval [−4, +20], 5.6 below centre.
    - [x] **B.0.2** `[V]` Oracle move-loop pruning family in Elo (maintainer)
      BAS-S18, registered 2026-10-06: `oracle-1.10.1-ablate.exe` with
      `AblationMask=32` (its shallow move-loop pruning off) against Basilisk
      1.10.1 at equal time, the BAS-O05 recipe at 1,000 cycles (2,000
      games); G(32) = 1500 − Basilisk's rating. Frozen: G(32) = +110, 80%
      interval [+40, +180], so the family explains about 200 of the 312.6.
      G(32) ≥ +250 lowers B.2's prediction P2 and re-opens the cluster order
      before B.2.1. Prepare from `tools/run_a73_oracle_g0.ps1` with the
      ablate binary and option; append the calibration to BAS-S18 and §11.
      Prepared 2026-10-08: `tools/run_b02_oracle_g32.ps1`, BAS-O05's
      launcher with the ablate binary, `AblationMask=32`, 1,000 cycles and
      seed 20102; dry run accepted, and the mask is live (startpos depth 14:
      445,620 nodes against 131,089 at mask 0).
      — CLOSED 2026-10-08: **G(32) = +102.3 ± 15.9** (Basilisk 1397.7;
      oracle 868-838-294, 64.35%; 0 faults, every game ended naturally).
      G(0) − G(32) = **210.3 ± 23.9** of 312.6: the oracle's move-loop
      pruning family is worth about two thirds of its lead. At mask 0 the
      ablate oracle searches the same tree as G(0)'s oracle (967,078
      nodes) at the same NPS within 1%. No falsifier fired, so P2 and the
      cluster order stand. Calibration: inside [+40, +180], 7.7 below
      centre.
- [x] **B.1** `[I1]` Search restructure, behaviour-neutral
  `src/search.cpp` split into modules, node types as template parameters, a
  per-thread worker and stack separated from per-search configuration and
  engine-owned shared resources, inert parameters removed per A.2.3; exact
  fingerprint and pooled-PGO NPS within noise.
  The move table comes from A.6 and B.0. Release and sanitizer CTest, and
  exact `bench 13` at every commit.
  Handoff frozen by B.0 (packet §12.1, 2026-10-06): A.6's tickets 1–8 with
  `NodeType`, the A.2.3 removals and the two comment corrections; killers,
  countermove, low-ply history and every live mechanism stay byte-identical
  in behaviour; the B.0 zero-game baselines (packet §10) are reproduced on
  the B.1 binary exactly except NPS. Does not wait on B.0.1 or B.0.2.
  Implemented 2026-10-08, `456cf6a`..`bf44834`, one commit per ticket:
  - the A.2.3 removals: `time_limit_`, `RootMoveStat`, capture futility,
    quiet SEE pruning, qsearch quiet checks and the post-LMR nudge with
    their 7 coordinates, `KBNK Drive`; the `DoubleExtMax` note corrected
    (the capture-futility comment went with its code);
  - `search_types.h`, `search_root`, `search_diagnostics`,
    `search_history.cpp`, `move_picker`, `search_worker` with
    `SearchConfig` / `SearchShared` / `SearchState`, `NodeType`,
    `search_thread_pool`, and `search.h` as the facade.
  Bench 14,978,465 at every commit on PEXT, non-PEXT, Tune, Diag and
  Ablate; release and Tune CTest 19/19, sanitizer CTest 17/17 (`-LE
  strength`), the Texel build reconstructs exactly; Diag, decision trace,
  MultiPV, tablebase, pool and time-budget output match the pre-B.1 Tune
  build line for line. Packet §10 reproduced on the B.1 PGO builds
  (`tools/results/b1-qual/`): branching 1.767 and every position at every
  depth; WAC at 100k / 400k nodes 204 / 242, identical to the A.8 head in
  every field; LMR-off WAC 219 (+15); the mask 0 / 32 profiles cell for
  cell; counters and the oracle differential (66/105, median depth 14
  against 18) identical except the mated root `Fool's mate`, which the B.0
  binaries searched once per iteration and A.8.8 reports without
  searching; canaries 77/77.
  — CLOSED 2026-10-09: the NPS gate BAS-P16 (maintainer-run) reads B.1
  **+1.21%** against the A.8 head, 95% CI [+0.57%, +1.44%], with the self
  pair at −0.04%. It passes, and the gain is recorded unexplained. B.1's
  pooled median, 4.216M NPS, is B.2's NPS reference.
- [x] **B.2** Cluster 1 — the selectivity core
  Cluster 1 — the selectivity core; final contents fixed by B.0.
  Expected contents:
  - TT use: the TT-adjusted eval, raw eval stored on a miss, and the
    graph-history check on cutoffs;
  - the correction histories and the corrected eval, with the correction
    magnitude as an input to margins and reductions;
  - the history set and its update policy (tables, TT-cutoff bonus,
    eval-swing training, fail-low parent bonus, post-LMR update, malus
    decay; killers dropped if B.0 confirms);
  - the move picker;
  - move-loop pruning;
  - the LMR formula and re-search rules;
  - RFP and razoring on the corrected eval;
  - the hindsight depth adjustment and cutoff counting.

  NMP, ProbCut, singular and extensions keep Basilisk's forms in this
  cluster so that B.3 can measure them separately. Gated at `[0,10]`,
  because the prior is large.
  Contents fixed by B.0 (packet §5, §12.2, 2026-10-06), changing the list
  above in five places: razoring in the donor's return shape seeded from
  the oracle column (depth 1, 256 cp; the cap categorical); RFP in
  Stockfish 19's shape with the TT-move condition and the blended return;
  a persisted `tt_pv` bit taken from the age field; killers and countermove
  dropped, low-ply history kept; and the move-loop family (count pruning
  as a picker skip, history-adjusted `lmrDepth`, continuation-history
  pruning, quiet SEE pruning, capture futility, fail-soft futility) named
  as the primary target, since it carries the ×4 node multiplier
  (BAS-D21). Seeds, surfaces, screens and the frozen predictions P1–P6 are
  the packet's §7, §8, §10 and §11.
    - [x] **B.2.0** `[R2]` Architecture review and neutral upgrades
      Architecture review of the B.1 head: the seams the cluster needs, and
      behaviour-neutral upgrades landed at the exact fingerprint before the
      first mechanism ticket.
      Rarog's review landed twelve such upgrades.
      Reviewed 2026-10-10 on the B.1 head `8c706a8`:
      `analysis/b20_architecture_review_2026-10-10.md`. The head needs no
      structural rewrite; the weight sat where the cluster presses. Landed at
      the exact fingerprint (`c434ee4`..`9b51b46`): the node kernel split out
      of the driver behind three entry points (`root_search`,
      `kernel_begin_search`, `kernel_new_game`) with Searcher's kernel members
      in one fenced section; the `B2_CORE` umbrella option, inert, both arms
      at 14,978,465; the counter, parameter (root, legacy kernel, core kernel)
      and TT-layout tables made one-source; the driver's comments without
      retired numbers (391 → 288 references in `src/`); the SPSA surface
      regenerated; a source census tool. Release PEXT and plain, Tune, Diag,
      Ablate and the sanitizer build at the fingerprint; release CTest 19/19,
      sanitizer 17/17; the Tune option set, `Diag` and trace output
      byte-identical to the B.1 Tune build. The review's §5 refines B.2.1's
      contract to the code: the fence, the per-ply record, the tables and
      their 9.2 MiB footprint, the TT encoding with the miss-store's
      occupancy rule, the picker and threat-producer contracts, 51 seeded
      coordinates and 6 categorical switches, the tests. Remaining gate:
      BAS-P17, the pooled-PGO NPS pool against the B.1 builds (maintainer-run
      on an idle host, `tools/run_b20_nps_gate.ps1`), passing above −0.5%.
      — CLOSED 2026-10-10: BAS-P17 (maintainer-run) reads the B.2.0 pair
      **0.00%** against the B.1 pair, 95% CI [−0.22%, +0.28%], self pair
      −0.03% OK, fingerprints identical; the frozen 0.0% held at its centre.
      B.1's 4.216M pooled median stays B.2's NPS reference.
    - [x] **B.2.1** `[I2]` Implement behind the umbrella switch
      Implement to the B.0 handoff behind one umbrella CMake option, OFF by
      default, as ordered tickets; ticket 0 is the decision trace hook.
      Each ticket keeps the umbrella-off arm at the exact fingerprint. Unit
      tests cover every table's bounds and gravity, picker exhaustiveness, TT
      store and probe, and stack unwind. CTest passes on both arms: the
      mate-drive floors bind the on arm too (the WAC floor from ticket 7,
      Amendment 2), and a failure there returns the premise to research.
      Reviewer acceptance (a separate `R2` session) is
      recorded before B.2.2.
      Tickets 0–2 landed 2026-10-10 (`1dbf0d9`..`f179404`): the counters and
      trace version 2, the threat producer and the core's stack record, the
      TT encoding with the miss-store and the cutoff rules. The off arm stays
      at 14,978,465 at every commit; the ON arm reads 13,488,940; CTest
      passes on both arms.
      — RETURNED 2026-10-10 (Return 1, packet §16): Basilisk's `see_ge`
      answers `0 ≥ threshold` for every quiet move, so the picker's
      `see_ge(−75)` check gate and quiet SEE pruning are inert as written.
      Research amendment 1 decides before ticket 3; the implementation
      recommends the donor's quiet-move semantics for `see_ge`.
      — AMENDED 2026-10-10 (Amendment 1, packet §17): `see_ge` takes the
      donor's quiet-move semantics (option a); decisions 2–13 confirmed except
      razoring above depth 1, now linear `margin·d`; P5's sources re-registered.
      Implementation resumes at ticket 3.
      Ticket 3's first commit landed 2026-10-10 (`ea5c49f`, `60a610e`):
      `see_ge` evaluates quiet moves, tested against the legality oracle;
      neither arm's fingerprint moved.
      — RETURNED 2026-10-10 (Return 2, packet §18): the rest of ticket 3 (the
      core tables, the picker, the update routing; held as a patch) fails
      the ON arm's WAC floor, 119/300 at depth 6 against 130. The picker's
      SEE-gated check bonus costs 13 of the 19 positions under the legacy
      pruning and LMR still in place. Research amendment 2 decides; the
      implementation recommends a categorical switch for the gate.
      — AMENDED 2026-10-10 (Amendment 2, packet §19): the check gate stays
      as contracted, no switch; the depth-6 WAC floor binds the ON arm from
      ticket 7, recorded as a count per ticket before that with §10's
      100k-node reading beside it. Implementation resumes at ticket 3's
      remainder from the held patch.
      Ticket 3 landed 2026-10-10 (`9a632e6` count mode, `bcf63e8` tables,
      picker and routing; record in packet §20): ON arm 12,815,895
      (transient), off arm 14,978,465; CTest passes on both arms; the ON
      arm's WAC reads 119 at depth 6 and 200 at 100,000 nodes.
      Ticket 4 landed 2026-10-10 (`221afdf`; packet §20): the cluster's
      correction form and admission, `improving`, opponent worsening,
      hindsight and eval-difference training. ON arm 13,061,601, off arm
      14,978,465; CTest passes on both arms; WAC 127 at depth 6 and 193 at
      100,000 nodes.
      Ticket 5 landed 2026-10-11 (`bb73fb5`; packet §20): razoring at
      depth 1 with the quiescence value returned, and RFP in the cluster's
      shape. ON arm 25,182,694 (one bench position found a forced mate),
      off arm 14,978,465; CTest passes on both arms; WAC 167 at depth 6
      (+40: razoring +43, RFP +5, −8 interaction) and 237 at 100,000 nodes.
      Ticket 6 landed 2026-10-11 (`573add0`, `8337944`; packet §20): the
      move-loop pruning family on the history-adjusted reduced depth. ON
      arm 11,922,648, off arm 14,978,465; CTest passes on both arms; every
      member fires at `bench 13`; WAC 153 at depth 6 (−14: the legacy family
      removed +14, the new family −28, capture/check SEE −23 of it) and 235
      at 100,000 nodes.
      Ticket 7 landed 2026-10-11 (`b02fcb7`, `802ed5c`; packet §20): the
      cluster's LMR, its floor and re-search rules, `depth −= 3`, the
      fail-high blend. ON arm 10,739,013, off arm 14,978,465; CTest passes
      on both arms with the ON arm's depth-6 WAC floor binding again (176
      against 130; +23: LMR +29, `depth −= 3` −8); 241 at 100,000 nodes.
      Windows sanitizer builds now reserve an 8 MB stack, as Linux does.
      Ticket 8 landed 2026-10-11 (`4112db0`; packet §20): the cluster's
      update policy (exit-time training of every alpha-raising node, the
      fail-low parent bonus, the post-LMR update, the TT-move history) and
      the per-search decay. ON arm 6,337,226, off arm 14,978,465; CTest
      passes on both arms; WAC 185 at depth 6 and 249 at 100,000 nodes.
      The ablation bits are live (8 of 8), and the four decision traces are
      recorded.
      — IMPLEMENTED 2026-10-11: implementation record in packet §20, with
      the reviewer's checklist. Reviewer acceptance (a separate `R2`
      session) comes before B.2.2.
      — RETURNED 2026-10-11 (Return 3, review, packet §21): every
      fingerprint reproduced from clean sources (149 of 149), CTest on both
      arms, WAC 185 at depth 6 and 249 at 100k nodes; not accepted. Two
      defects: `stat_score` never reset at node entry, and the PV
      TT-move depth rule inert on the first move. One gap: the
      stack-unwind test. Two contract questions: the first child's
      node type, and two donor constants on evaluation units left
      unconverted. Research amendment 3 rules on the questions; then
      implementation lands all five and a delta review accepts them.
      — AMENDED 2026-10-11 (Amendment 3, packet §22): the first child at a
      non-PV node typed `!cut_node` with the donor's first-move step-down;
      three evaluation-unit conversions (`|corr|/7419`, the LMR alpha term
      and the eval-difference training on scaled differences); F1, F2 and
      F5 confirmed; two more defects (the per-ply move count surviving
      same-slot re-entries; the absent step-down). Implementation lands
      eleven items in order, then the delta review. B.2.2–B.2.4 registered
      as BAS-S21–S23.
      — RETURNED 2026-10-11 (Return 4, packet §23): items 1–5 landed
      (`31c80db`..`90d7db7`); ON arm 5,941,186, WAC 172 at depth 6 and 250
      at 100k nodes; the off arm stays at 14,978,465. Item 6's `|corr|/7419`
      never finishes `bench 13`: where the correction saturates it removes
      up to 9.3 plies of reduction, against the donor's 2.6. Research
      amendment 4 decides the divisor; items 6–11 resume after it.
      — AMENDED 2026-10-12 (Amendment 4, packet §24): item 6 is the converted
      slope under the donor's bound, `min(|corr|/7419, 2695)`; rule 4 gains a
      saturation clause; the item-2 deviation is accepted; the correction
      bound's own conversion is a registered B.2.3 question. Items 6–11
      resume in order, then the delta review.
      Return 4 resume landed 2026-10-12 (items 6–10: `45f4827`..`7f1b5bb`;
      items 1–5: `31c80db`..`90d7db7`; record in the packet's §20 addendum):
      ON arm 9,132,715 at `bench 13` on every flavour, `bench 10`
      1,678,409 release and sanitizer; off arm 14,978,465 on every flavour,
      sanitizer 3,190,673. CTest: ON 23/23 release and 21/21 sanitizer, off
      20/20 and 18/18. ON WAC 185 at depth 6 and 249 at 100k nodes.
      `test_stack_unwind` and the node-entry checks were proven live with
      two planted stale fields. The R2 delta review (§21.4) accepts before
      B.2.2.
      — ACCEPTED 2026-10-12 (delta review, packet §25): every fingerprint of
      items 1–10 reproduced from clean sources on both arms, CTest on both
      arms, WAC 249 / 204 at 100k nodes, the changed lines exact against
      §22.6 and §24.1, the planted proofs re-run; two follow-ups bind
      nothing here. CLOSED.
    - [x] **B.2.2** `[V]` Diagnostics and the unfitted paired run
      Screens at B.0's registered numbers; the ablation sweep if a floor
      fails; the unfitted 2,000-game paired run against the accepted head,
      maintainer-run, which governs. Categorical switches the review asks for
      are settled by 2,000-game paired runs on one tune build and never
      become SPSA coordinates.
      Screens read 2026-10-12 on the accepted head (packet §26, BAS-S21):
      two floors flagged. The canaries read 68/77; eight of the nine
      regressions come back with the move-loop pruning family removed, and
      there are 30 new passes, WAC.001 among them. Branching 4–14 reads
      1.6465 against the window's 1.65, carried by the LMR at depths 12–14.
      Nodes at depth 12 relative to the oracle read 1.86 (median 1.04); WAC
      249 at 100k and 276 at 400k nodes; the LMR-off delta +8; the
      shallow-pruning factor 3.46; agreement 66/105; time to depth 0.59×;
      pooled NPS 1.038× the B.1 head. P6 held; BAS-C09's trigger was not
      met. Prepared, not started: the paired run and the six categorical
      runs (`tools/run_b22_games.ps1`, `526a68d`; dry runs accepted). The
      paired run governs, and the review decides on the flagged floors
      with its result.
      Games 2026-10-13 (maintainer-run, packet §26.5, BAS-S21): the paired
      run reads **+130.9 ± 11.4 Elo** (ON against OFF), which proceeds to
      B.2.3; P1's −15 ± 30 missed high. `CoreLmpExemptChecks` is adopted at
      0 (+13.6 ± 9.7); the other five switches keep their defaults
      (`CoreCorrAdmission` = 0 costs −96.9). The fastchess cross-check
      agrees (+136.2 ± 11.0, 2026-10-13).
      — CLOSED 2026-10-13: `CoreLmpExemptChecks` defaults to 0 (`cd2e397`).
      The ON arm's `bench 13` is now 6,199,439 (`bench 10` 1,598,399
      release and sanitizer) and the off arm 14,978,465 on every flavour.
      CTest passes on both arms (ON 23/23 and 21/21, off 20/20 and 18/18);
      ON WAC reads 173 at depth 6 and 249 at 100k nodes. B.2.3 starts from
      this head.
    - [x] **B.2.3** `[V]` Curvature sweep, then SPSA in blocks
      Curvature sweep, then SPSA in blocks (rule 7c) over the cluster's live
      coordinates, maintainer-run; theta baked in one engine commit.
      Sweep read 2026-10-13 (`analysis/b23_sweep_2026-10-13.md`, classification
      frozen before the first point): `CoreRfpPerPly`, `CoreLmpSquareScale`,
      `CoreLmrOffset` and `CoreCorrUpdateScale` curved, `CoreQuietFutBase`
      monotone, so the tune runs. Every curve peaks at the seed (WAC 249 at
      100k), so the curvature is shallow. Block 1 is registered in BAS-S22
      and prepared, not started: the core surface's 50 coordinates (now
      generated, `e24914f`) on the ON Tune build at `526dd49`; runner
      preflight and dry run accepted.
      — AMENDED 2026-10-13 (Amendment 5, packet §27): before block 1, one `I1`
      commit gives the picker's capture score, its threat term and LMR's
      capture `statScore` the donor's `PieceValue` (history-unit sums),
      converts the good-capture split to `−score/47`, seeds
      `CoreRfpCorrMult` at 676 and fixes `CoreQuietSeeCoeff` off the surface
      (49 coordinates); then the sweep re-runs on the new binary and block 1
      is re-registered. The eight legacy coordinates stay B.3's.
      Amendment 5 landed 2026-10-13 (packet §28: `3eea3c5` engine; `e24114a`,
      `b5983c6` tooling). The ON `bench 13` is now 6,096,792 and the off arm
      14,978,465; CTest passes on both arms. ON WAC reads 177 at depth 6 and
      237 at 100k (from 249); first-move cutoffs rise 88.5% → 89.1% and
      good-capture cutoffs 21.0% → 24.9%. The sweep re-run on the new Tune
      binary finds three coordinates curved and the seed now at or near the
      lowest WAC point; block 1 is re-registered (49 coordinates) and
      prepared, not started.
      Owed before the bake (maintainer, 2026-10-14): a written analysis of
      the fitted constants, not only their gate. Every coordinate that
      moved a full step, ended at a range edge or moved against its seed
      column (§7.3) is read for what it says about implementation,
      design, unit or feature cohesion. Examples: a mechanism tuned toward
      off, two terms moving to cancel, a margin compensating for a
      mis-scaled input, a rail that hides a wanted value outside the range.
      Each finding gets a proposed fix and an owner leaf. Block 1 already
      shows two rails at iteration 1,530: `CoreIirMinDepth` 4 → 8 (the
      kept IIR barely wanted) and `CoreResearchShallower` 2 → 0.
      Block 1 complete 2026-10-14 (60,000 games, 0 faults): six coordinates
      moved a full step, so the movement rule says CONTINUE. The interim
      analysis is `analysis/b23_constants_2026-10-14.md`: IIR tuned off
      (B.3), the shallower re-search firing only on fail-hard bound echoes
      (B.4, B.8), RFP's seed column conservative (C, B.9), the `cutoffCnt`
      terms (B.3), and the correction rate (the registered `CoreCorrLimit`
      question). Block 2 is prepared (seed 23002, from block 1's centres),
      maintainer-run. After the last block: the final analysis, the bake,
      then B.2.4's gates whatever the analysis finds.
      Block 2 complete 2026-10-16 (60,000 games, 0 faults): two full-step
      movers, so the rule says STOP; theta is block 2's rounded centres.
      The final analysis (`analysis/b23_constants_2026-10-14.md`) carries
      seven findings with owners (IIR tuned off and the shallower re-search
      inside its dead zone, B.3 and B.4/B.8; RFP's seed, C and B.9; the
      correction wanting more weight, which argues against running the
      `CoreCorrLimit` categorical; histories weighing more, the `CoreHistoryFills`
      residual); none holds the bake.
      — CLOSED 2026-10-16: theta baked in one engine commit (`b777ba8`; 46
      of 49 defaults change), surface regenerated (`2ab17c7`). The ON
      `bench 13` is now 11,099,210 (`bench 10` 2,034,942) and the off arm
      14,978,465 on every flavour. CTest passes on both arms (ON 23/23 and
      21/21, off 20/20 and 18/18); ON WAC reads 181 at depth 6 and 241 at
      100k. The endgame cohort at 600k nodes holds every floor (KBN-K 23/24
      inside its slack; BAS-S23). B.2.4's gates are registered and
      dry-run.
    - [x] **B.2.4** `[V]` Cluster 1 gates
      Gate: (a) the unfitted arm against the umbrella-off arm at the same
      revision, `[0,10]`, once its PGO binaries exist; (b) the fitted arm
      against the unfitted one, `[0,10]`, after the bake.
      Each gate accepts one thing on its own evidence, so a weak tune cannot
      ride on the cluster's margin. The default flips to the accepted arm;
      the legacy search stays compilable until B.8.
      — CLOSED 2026-10-16 (BAS-S23): both gates accept H1. (a) the unfitted
      arm against the off arm, **+136.2 ± 33.6 Elo** (252 games); (b) the
      fitted arm against the unfitted, **+58.7 ± 20.9 Elo** (430 games).
      Chained, B.2 reads about +195 Elo over the B.1 head; P2's +40 missed
      high. `B2_CORE` now defaults ON (`8a10d14`): the development
      fingerprint is `bench 13` 11,099,210, and the legacy kernel builds
      with `-DB2_CORE=OFF` (14,978,465), tested in CI (`2853699`), until B.8.
- [x] **B.10** `[I1]` PST orientation repair (ANY TIME) — before B.3
  Correctness repair, admitted into the evaluation freeze by maintainer
  decision (2026-10-23): fix it before B.3 starts fitting to the eval. Seven
  piece-square tables have been stored rank-mirrored since v1.0.0: mg and eg
  knight, bishop and queen, plus eg king. They hold PeSTO's rank-8-first rows
  on a1-first squares, so, for example, a White knight on f3 gets f6's +110.
  Pawn, rook and mg king are correct. Evidence is in
  `analysis/pst_orientation_2026-10-23.md`, all of it static:
  - the fix moves the eval by 31.7 cp mean;
  - holdout loss is lower with no refit, on two corpora;
  - an unanchored refit pulls only the seven mirrored tables toward their flip;
  - the L2 anchor froze the error in place.
  Change: rank-flip exactly those seven tables in `src/eval_params.h` and
  nothing else. The candidate header is recorded by sha256 in the analysis.
  Add an orientation test that fails on the old tables. Example: the White
  and Black PST contributions of mirrored piece placements must agree with
  the declared a1 = 0 orientation for every table, checked against a
  committed PeSTO-orientation fixture.
  Qualification:
  - CTest on both arms;
  - WAC;
  - the endgame cohort floors at 600k nodes, with KBN-K and the KXK drives
    as the fragile ones;
  - bench accounting for the new fingerprint.
  Gate: register a BAS-S entry first, then Colosseum `sprt-repair.toml`
  (`[-5,5]` nElo, 3+0.03, 1 thread, Hash 64, UHO, no adjudication). Clean
  PGO builds; the candidate plays the B.2.4 head (`bench 13` 11,099,210).
  Frozen prediction: H1, +3 to +10 Elo, about 70% confidence. The main risk
  is that terms fitted around the mirrored tables compensate (outposts,
  mobility, Cluster 1's margins).
  Verdict:
  - H1: the flip becomes the default and the fingerprint record updates;
  - H0: the old tables stay, and C.2.5 fits from the flipped seeds instead.
  Scope (maintainer decision 2026-10-23): the flip alone, gated; no refit
  inside Phase B. Phase C fits the tables onward from the flipped values:
  no neutral start is required, and PeSTO-derived values may stay. The
  follow-ups are routed to C.0, C.2.1, C.2.5, C.7 and C.10.
  Implemented 2026-10-23 (`75381dc`): the seven tables flipped, reproducing
  the analysis's candidate header, with the orientation test in
  `tests/test_eval.cpp`. ON `bench 13` 11,099,210 → 8,437,148; CTest
  passes on both arms; WAC at 100k 241 → 248. BAS-S26 is registered and
  dry-run; the baseline is rebuilt with clang 23.1.3 at the B.2.4 head's
  engine (still 11,099,210). The endgame cohort at 600k holds every floor
  (conversions 460/480 against the head's 452); the SPRT is maintainer-run.
  — CLOSED 2026-10-24 (BAS-S26): H1, **+18.3 ± 12.9 Elo** (1,388 games),
  above the frozen +3 to +10. The flipped tables are the default: the
  development fingerprint is `bench 13` 8,437,148, and the legacy
  `-DB2_CORE=OFF` arm reads 15,887,213.
