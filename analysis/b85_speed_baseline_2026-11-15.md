# B.8.5 — the speed pass's baseline reads (2026-11-15)

The reads PLAN B.8.5 names, on the head at `bench 13` **4,688,537** with large
pages on (B.8.4's default), on the idle 5950X. B.8.8's packet ranks the
candidates from here; B.8.9 implements them. No engine behaviour changed: the
only engine commit is three Diag counters (`ee6312d`), which release builds do not
compile.

## 1. The profile, two cohorts

`tools/profile.ps1` on `basilisk-b85-prof-pext-profile-pgo.exe` (`-Flavor
Profile`, PGO, line tables with inline records, code byte-identical to Release;
`28006a8`), CPU 30, a sample a millisecond, each sample charged to the source
file of its innermost engine frame (PROCESS *Profiling*). Self share with its 95%
interval, then the share including inlined frames.

| Region | `bench 13` ×5 (10,207 samples) | Timed-game cohort (63,779 samples) |
|---|---|---|
| ordering | 24.4% [23.6, 25.2], 27.8% | 25.2% [24.9, 25.6], 28.2% |
| board | 23.3% [22.5, 24.1], 27.0% | 22.2% [21.9, 22.6], 25.4% |
| eval | 22.6% [21.8, 23.4], 26.4% | 18.1% [17.8, 18.4], 21.6% |
| search | 17.2% [16.5, 18.0], 40.7% | 15.7% [15.5, 16.0], 39.8% |
| tt | 6.1% [5.6, 6.6] | 13.0% [12.8, 13.3] |
| attacks | 6.0% [5.6, 6.5] | 5.1% [4.9, 5.2] |

- **The timed-game cohort** is `tools/diag/game_cohort_v1.epd` (`28006a8`; sha256
  `127bad01…`): one position from every eighth game of BAS-S64's 8,000, at a point
  that moves through the game with its index (`tools/diag/pgn_cohort.py`), 1,000
  positions, ply 8 to 323 (median 58), 3 to 32 men (median 17; 267 at ten men or
  fewer). Each is searched to 150,000 nodes from `ucinewgame`, about one move at
  `3+0.03` (BAS-S64: 129,933 nodes a move), at the profiler's Hash 64.
- **The table's share doubles in the game cohort** (13.0% against 6.1%):
  `probe_copy` 6.78% of the samples against 2.38% on `bench`. The bench's table is
  16 MiB, the cohort's 64 MiB, and the cohort's positions are not bench's 40.
  Of the cohort's 13.0%, 3.96% is `TTCluster`, the table's construction: the
  profiler sends `ucinewgame` before each of the 1,000 positions, which clears the
  whole table each time, where a game clears it once. That share is the harness's;
  the probe's is the game's.
- **Top inner frames, cohort (bench):** `evaluate_full` 14.10% (17.34%),
  `score_quiets` 7.01% (6.88%), `probe_copy` 6.78% (2.38%), the root lambda's
  `operator()` 6.30% (6.84%), `negamax<NonPV>` 4.89% (5.12%),
  `CoreMovePicker::next` 4.27% (4.34%), `TTCluster` 3.96% (—),
  `partial_insertion_sort` 3.72% (3.16%), the history `update` 2.30% (2.16%),
  `attackers_to` 2.05% (2.03%), `gives_check` 1.94% (2.21%), `is_attacked_by`
  1.76% (1.45%), `correction_value` 1.74% (2.36%), `fill_entries` 1.44%,
  `bishop_attacks` 1.43% (1.56%).
- Archives: `tools/results/b85-profile-bench`, `tools/results/b85-profile-games`.

## 2. The game's speed against the bench's (B.3.5's P9)

BAS-S64's record has the game's NPS per move: 1.85M for the head in games, 15 at a
time, against 2.36M for `bench 13` alone (BAS-P35's `b84-on`). The game cohort,
200 positions at 150,000 nodes, Hash 64 unless named, each engine pinned to its
own physical core, the release head (`basilisk-b84-on-pgo1-pext-pgo.exe`):

| Condition | NPS per engine |
|---|---:|
| one engine, Hash 64 | 2.502M, 2.500M (twice) |
| one engine, Hash 1 | 2.608M |
| 15 engines at once, Hash 1 | 2.207M (2.053 to 2.261) |
| 15 engines at once, Hash 64 | 1.866M, 1.868M (1.741 to 1.962) |

- **The gap is the shared host, not the workload.** Alone the cohort runs faster
  than `bench` (a quarter of it is endings); 15 at once it runs at the games'
  1.85M.
- **Half the gap is the table's memory traffic.** With a 1 MiB table the 15-way
  load costs 15.4% (the cores' all-core clock and the shared caches); the 64 MiB
  table costs 4.1% alone and a further 15.4% under the load. So the table's
  memory traffic is about a twentieth of a node alone and about a sixth of one in
  the gates' conditions.
- **For the method:** BAS-P35's and every pooled read so far is one engine alone
  with `bench`'s 16 MiB table, which under-weights a memory item (a prefetch, the
  entry's size, a probe saved) by three to four times against play. B.8.8 reads
  memory items also under load.
- The counting of these runs: a node-limited search stops at exactly its limit
  unless it ends earlier, which here only a mate in one does (the driver searches
  on after a longer mate); such a search counts its last line's nodes.

## 3. Frames and stack probes (TB-11, NODE-5's frame)

From the disassembly of the Profile build (code identical to Release): the hot
functions' frames are `negamax<NonPV>` 1,416 bytes, `negamax<PV>` 728,
`negamax<Root>` 856, `quiescence<PV>` 1,256, `quiescence<NonPV>` 1,240,
`evaluate_full` 1,352, `CoreMovePicker::next` 792. None reaches 4 KiB, and no
hot function calls `__chkstk`: TB-11 does not apply. B.9.1 sizes the thread stacks
from these.

## 4. Quiescence's table move (NODE-5)

Three Diag counters (`ee6312d`), on the game cohort at 150,000 nodes (Diag build,
summed by `run_suite.parse_diag`): of about 27.7M quiescence nodes that reach the
capture loop, 501,864 (1.8%) have the table move among their captures and 344,851
(1.2%) are cut by it searched first; those nodes generated and scored 946,108
captures (2.74 a cut), 1.8% of the 51.2M quiescence generated. Staged delivery of
the table move would save at most that: NODE-5 closes, not worth building.

## 5. The four-thread instrument (BAS-P36)

`nps_read.ps1 -Threads 4` (`586e3bf`): `bench 13 3 4` per reading, pinned to the
last four physical cores. Its null pair, `b84-on` against a second pool of the
same revision (`b84-on-null`), **failed**: both step-1 reads were disturbed (an
arm's two cycles 2.1% and then 1.05% apart, as the cores' clock settles under a
four-core load), and the builds of one source spread about 2.7% at four threads
against about 0.4% at one. B.8.9 reads one thread; a second four-thread design
needs the clock held (a heat soak, or the boost off, which is the maintainer's
host setting) and a smaller build spread. B.8.4's pinned scaling check (four
threads 4.1 to 4.25 times one) stays the D-9 guard against a collapse, which it
can see; it cannot see a 1% change.

## 6. Not re-read

B.7's frontier counts (SEE asked twice, captures generated twice, the
evaluation's `check_squares`) stand as sized there; this profile does not move
their shares (board 23%, `see_ge` about 1.5%, `gives_check` about 2%). B.7.2's and
A.17's profiles are superseded.
