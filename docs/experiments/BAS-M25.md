# BAS-M25

**BAS-M25 — C.3.4: the rebuilt evaluation at seed values against the head**

- Date / owner / category: registered 2026-11-25, before the port
  exists and before any game; maintainer-run, agent-read; measurement.
- Arms: clean final PGO builds of one commit after C.3.3's acceptance:
  the new arm (the evaluation rebuilt in classical Stockfish's form, at
  the reference's values converted by the packet's §7) against the off
  arm, which is the head (`bench 13` 3,359,329).
- Research question: does the donor's evaluation carry its lead into
  Basilisk's own search and endgame layer? In `9587eeeb`'s search it
  leads the head's by +146.2 (BAS-O14); this is the first read of it in
  Basilisk's.
- Registration, mechanism and the rule:
  `analysis/c31_rebuild_2026-11-25.md` §3 and §11.
- Registered design and stop rule: no gate; one fixed match of 3,000
  games, never extended or replaced.
- Full conditions: Colosseum CLI, two-engine match, 1,500 cycles, UHO
  `UHO_Lichess_4852_v1.epd` in random order, seed `210004`, `3+0.03`,
  margin 20 ms, 1T, Hash 64, concurrency 14, no adjudication, no
  tablebases, zero permitted faults. The launcher and the builds are
  C.3.4's; this entry is completed with their hashes before the run.
- Beside it, deciding nothing alone: pooled NPS of the two arms and the
  fixed-depth tree read, by PROCESS.
- Completed 2026-11-25, before any game (C.3.4's preparation):
  - Both arms are built from `7d68e25`: C.3.3's accepted rebuild with
    the new arm's diagnostics (`c669b1f`) and its clamp test. Both stay
    behaviour-neutral, at `bench 13` 3,717,775 and 3,359,329.
  - New arm: `tools/test_engines/basilisk-c34-on-pext-pgo.exe`, SHA-256
    325B660438FF44F4B8EB650114CA4D774A7AB6A0E11E817BB081BAEAC264AAF9,
    `EVAL_REBUILD=ON`.
  - Head: `tools/test_engines/basilisk-c34-off-pext-pgo.exe`, SHA-256
    DF4D302194AE277E7D0C8455F6E2082CD52EB49A10E5F6CF1E3BD735FF24B8A1,
    `EVAL_REBUILD=OFF`.
  - Both are final PGO PEXT Release builds from
    `tools/build_test.ps1 -ArmOption EVAL_REBUILD`, Clang 23.1.5, with
    clean manifests at that commit, which the launcher checks.
  - The runner: Colosseum's gauntlet command with the two arms, the head
    fixed at 1500 and `--max-engine-faults 0`. The new arm's rating less
    1500 is the read. Each arm carries Hash 64 and Threads 1 and nothing
    else; the dry run's launch record shows this.
  - The NPS pools beside it: `tools/test_engines/c34-nps-on.pool.json`
    and `c34-nps-off.pool.json`, four builds each of the same commit.
    They are read only on an idle host, when the maintainer agrees.
  - Command: `python tools/run_c34_reads.py --read m25`. The table:
    `python tools/diag/c34_table.py`.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-25

- The new arm minus the head: +85 Elo, 80% band +40 to +130.
- Return rule: below +30 the leaf returns to research before any fit.
  The premise would be false.
- The seed arm never ships: its values are the reference's.

#### RESULT — append after exposure

- 2026-11-25. Complete: 3,000 games (`tools/results/c34-m25/`, PGN
  SHA-256 `A54C0C53…D1F77282`), zero engine faults, zero time losses,
  recount agrees. Summary table: `tools/results/c34-table.md`.
- **Basilisk rebuilt seed minus Basilisk head: +99.8 Elo [+91.5, +108.3]**
  (pentanomial 95% CI; trinomial [+90.0, +109.8]), score 63.98%
  (1,919.5 / 3,000), 1,351 wins, 1,137 draws, 512 losses, pentanomial
  [22, 140, 510, 633, 195].
- In-game speed and depth: rebuilt seed 1.88M nodes/s (134,376
  nodes/move, 71.5 ms/move, mean non-mate depth 16.58, median 15) against
  the head's 1.92M nodes/s (139,437 nodes/move, 72.7 ms/move, mean
  non-mate depth 16.29, median 15), ratio 1.020 (unflagged).
  Terminations: 1,863 checkmates, 929 threefold, 184 insufficient
  material, 18 fifty-move, 6 stalemate. Clamped bare-king draws: 0 on
  rebuilt seed, 3 on head.
- Pooled NPS (`tools/results/c34-nps/verdict-2.json`, four PGO builds per
  arm, 2 cycles × 3 repeats of `bench 13` on pinned CPU 30, idle host
  0.17% and 0.41% busy, 0 disturbed cycles): **−4.47% [−4.79%, −4.16%]**
  (2.354M against 2.464M mean nodes/s; `bench 13` 3,717,775 against
  3,359,329, +10.7% bench nodes and +15.8% bench time-to-depth).
- Fixed-depth tree read (`tools/diag/run_suite.py`, `suite_v2.epd`, depth
  12, Hash 64, stride 1, `tools/results/c34-tree/`):
  - rebuilt seed: 5,703,964 total nodes (3,374,638 interior + 2,329,326
    qsearch), first-move cutoffs 86.808%, LMR applied 59.797% (mean
    reduction 3.190, research 5.338%), pawn cache 2,785,028 / 3,050,059
    (91.31% hit), `eval_clamps` 0;
  - head: 5,824,526 total nodes (3,511,978 interior + 2,312,548
    qsearch), first-move cutoffs 87.639%, LMR applied 59.453% (mean
    reduction 3.110, research 5.181%), pawn cache 2,708,764 / 2,949,008
    (91.85% hit).
- Rule applied: +99.8 clears the +30 return line by 70 Elo, so the
  donor's evaluation carries its lead into Basilisk's search and C.3.4
  proceeds to the static fit.

#### PREDICTION CALIBRATION

- Inside the 80% band: +99.8 for +85 (+40 to +130).
- In `9587eeeb`'s search the rebuild closed 103.8 Elo of BAS-O14's
  +146.2 gap (BAS-O16), matching the +99.8 gained in Basilisk's search.
  A gain against the head is not a read of the gap to Stockfish 16 HCE
  (BAS-M24, −120.0); the frontier pair in C.3.5 measures that.


