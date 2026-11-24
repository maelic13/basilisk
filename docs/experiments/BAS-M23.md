# BAS-M23

**BAS-M23 — the HCE frontier field at `3+0.03` with Rarog's compact-kernel head, the C.1 head and Basilisk 1.11.0: a 12-engine round robin, 52,800 games**

- Date / owner / calibration category: run 2026-11-21 22:07 to 2026-11-22
  (9 h 45 min), maintainer-run in the Colosseum GUI on the Ryzen 9 5950X,
  agent-noted 2026-11-22; measurement, observation. Not registered before
  exposure.
- Why: the maintainer's dev test of Rarog's new head
  (`dev-20261121-94d7f72d`, the compact search kernel of Rarog's D.7) in
  the HCE frontier field, with Basilisk's head from that night
  (`dev-20261121-f0ed0612`; its engine source is C.1's `c0f0209`) and
  Basilisk 1.11.0 beside it.
- Conditions, as the GUI shows them:
  - a round robin ×400, 2 games per opening, 8,800 games per engine;
  - `3+0.03`, 1 thread, concurrency 14, no ponder, no tablebases, no
    adjudication, no move cap;
  - UHO `UHO_Lichess_4852_v1.epd`;
  - ratings followed for the two Basilisk heads and Rarog's head, the
    rest of the field fixed at its library ratings.
- **Source of the numbers: the GUI's standings and head-to-head screen**,
  read from the maintainer's screenshot. No PGN and no database snapshot
  were read, so there are no intervals, and nothing here is a fit.

#### RESULT — appended after exposure

| # | Engine | Elo | Perf | Points | W-D-L | NPS |
|---|---|---|---|---|---|---|
| 1 | Stockfish 16 HCE | 3605 | 3609 | 6,069.5 | 4588-2963-1249 | 2.2M |
| 2 | Stockfish 11 | 3532 | 3560 | 5,436.0 | 3928-3016-1856 | 2.2M |
| 3 | Berserk 4.7.0 HCE | 3545 | 3544 | 5,258.0 | 3886-2744-2170 | 2.4M |
| 4 | Rarog `dev-20261121-94d7f72d` | 3509 | 3509 | 4,799.5 | 3291-3017-2492 | 1.6M |
| 5 | Houdini 6.03 | 3499 | 3476 | 4,386.5 | 2766-3241-2793 | 2.4M |
| 6 | Komodo 14.1 | 3476 | 3478 | 4,374.5 | 2783-3183-2834 | 2.1M |
| 7 | Ethereal 14.40 HCE | 3467 | 3474 | 4,325.5 | 2880-2891-3029 | 3.0M |
| 8 | Rarog 2.7.0 | 3445 | 3442 | 3,899.5 | 2381-3037-3382 | 1.5M |
| 9 | Dragon 3.3 HCE | 3425 | 3433 | 3,774.0 | 2378-2792-3630 | 2.1M |
| 10 | **Basilisk `dev-20261121-f0ed0612`** | **3415** | 3415 | 3,548.0 | 2080-2936-3784 | 1.7M |
| 11 | **Basilisk 1.11.0** | **3415** | 3415 | 3,541.5 | 2145-2793-3862 | 1.7M |
| 12 | Fire 8.3 | 3423 | 3401 | 3,387.5 | 1840-3095-3865 | 2.3M |

- Terminations, all natural: 34,946 checkmates, 11,017 threefold
  repetitions, 4,098 insufficient material, 2,388 fifty-move draws and
  351 stalemates. No forfeits.
- Head to head, from the row engine's side:
  - Rarog's head against the Basilisk head: 367-271-162, about +90;
  - Rarog's head against Rarog 2.7.0: 311-357-132, about +79;
  - Stockfish 16 HCE against the Basilisk head: 502-214-84, about +201.

  The two Basilisks' own pairing is not on the screen.

#### Reading

- **The C.1 head plays as 1.11.0 does.** The two Basilisks rate the same,
  3415, and their points differ by 6.5 of 8,800 games. C.1 is
  behaviour-neutral (`bench 13` 4,777,412 on both), and its measured
  −0.71% NPS (`analysis/c0_evaluation_programme_2026-11-18.md` §10.8)
  shows no cost in play at this resolution. That agrees with the
  accepted-cost reasoning of §10.10, under about 1 Elo.
- **Rarog's kernel gains in the field what its own gate read.** Rarog's
  head rates 64 above Rarog 2.7.0 here and 94 above Basilisk; with 2.7.0
  the gap was 30. Rarog's own gauntlet read its adopted kernel at
  3,508.1, 96.6 below Stockfish 16 HCE (RAR-M90). This field agrees, at
  96 below.
- **Basilisk's distance to Stockfish 16 HCE is unchanged:** 190 in the
  field and about 201 head to head, against BAS-M22's 197–203 on three
  hosts.
- Nothing here changes a PLAN step. The gap to Rarog is in the search and
  its fit (Rarog's D.7). Basilisk's lever now is the evaluation programme.
  RAR-O10 read Rarog's evaluation as the more accurate one, and C.2.10
  holds that prior.

- Evidence (not committed): the maintainer's screenshot of the finished
  tournament's standings; the games are in the GUI's database.
- Disposition: OBSERVATION. The C.1 head and 1.11.0 are level in the
  field (3415 each, 8,800 games), Rarog's compact-kernel head is 94
  above them, and Stockfish 16 HCE is 190 above.
