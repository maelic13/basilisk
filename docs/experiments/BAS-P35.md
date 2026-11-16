# BAS-P35

**BAS-P35 — B.8.4's large pages in the whole engine, on against off (B.8.4)**

- **Registration.** 2026-11-15, B.8.4, before any read. Agent-built pools; the read
  runs on the idle host. Behaviour-neutral: `bench 13` 4,688,537 on both arms (the
  option changes where memory lives, not what is searched; the Linux build reads the
  same).
- **Question.** Does the `LargePages` option's default (on) make the engine faster
  than off?
- **Arms.** Four-build final-PGO pools of `60db617`, clang 23.1.5, PEXT
  (`tools/nps_pool.ps1 -ArmOption LARGE_PAGES`): `b84-on` (`-Arm On`, the default)
  and `b84-off` (`-Arm Off`). The account holds "Lock pages in memory", so the on arm
  gets large pages (`bench` reports `on`).
- **What it covers.** The table (`bench`'s 16 MiB), the one search thread's
  `Searcher` and its three heap history tables.
- **Prediction.** Rarog's Windows read, D-8: +2.48% bench (`bench`'s table at its
  size); RAR-M77 +2.7% at four threads. Basilisk's `bench` table is 16 MiB, so the
  gain is mostly the histories' and the table's TLB reach: +1% to +3%.
- **Method.** `tools/nps_read.ps1 -Base b84-off -Cand b84-on`, the gain form (PROCESS
  *NPS*): step 1, two cycles, accepts at +0.9% or more and is NO_CHANGE at +0.1% or
  less; otherwise step 2, six cycles, accepts at +0.5% or more with the lower bound
  above 0.
- **Verdict rule** (frozen):
  - ACCEPT: the default stays on.
  - NO_CHANGE: the default stays on, the donor's form at no measured cost here; the
    record says no gain was measured at `bench`'s table size.
  - An estimate below zero with its upper bound under 0: the default goes off and
    B.8.4 returns to research.
- **Beside it (not a gate), D-9:** `bench 13` at one and four threads, the four on
  one logical CPU per physical core (affinity 0x55), both arms' first build.

#### RESULT

Idle host, 2026-11-15. Pooled read (`tools/nps_read.ps1`,
`tools/results/b84-nps-20261115`), two cycles, host at 1.9% or less, no cycle
disturbed; per-build medians `b84-off` 2.339M, 2.341M, 2.333M, 2.341M and `b84-on`
2.354M, 2.360M, 2.361M, 2.363M.

| Contrast | Delta | 95% interval | Verdict |
|---|---:|---|---|
| `b84-on` over `b84-off` | +0.91% | [+0.62%, +1.20%] | ACCEPT at step 1 |

- **Disposition.** ACCEPTED: the default stays on.
- **Calibration.** At the lower edge of the predicted +1% to +3%, under Rarog's
  +2.48%: `bench`'s table is 16 MiB, so the gain here is the histories' and a small
  table's TLB reach; a game's table is larger.
- **D-9, beside it:** `bench 13 3` pinned, one thread on CPU 0 and four on CPUs 0, 2,
  4 and 6: four threads read 4.11 to 4.25 times one on both arms (off 4.18, 4.21,
  4.24, 4.25; on 3.91, 4.11, 4.17, 4.23), so no placement or code collapse. Four-thread
  `bench` trees differ from run to run and the runs last about a second, so these
  readings do not resolve an on-against-off difference at four threads.
- **Also measured:** Hash 16384 allocated until `readyok` in 1,358 ms at Threads 8
  (7,431 ms before, 7,919 at Threads 1; 4,511 ms now at Threads 1), `ucinewgame` in
  848 ms (1,222 before; the clear is memory-bound near 19 GB/s). On Linux (WSL2
  Ubuntu 24.04, transparent huge pages in `madvise` mode) a running engine at Hash
  1024 held 1,075,200 kB of its 1,081,188 kB resident set in huge pages with the
  option on and none with it off; `bench 13` 4,688,537 there as on Windows.
- **Artifacts.** `tools/results/b84-nps-20261115` (`read.json`, `runs.jsonl`,
  `verdict-2.json`); pools `tools/test_engines/b84-on.pool.json` and
  `b84-off.pool.json`.
