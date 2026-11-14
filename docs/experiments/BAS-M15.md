# BAS-M15

**BAS-M15 — "Dev Test": the head after Clusters 1 to 3 in a twelve-engine 1T
round robin, read against Basilisk 1.10.2 and the Rarog releases**

- Date / owner / calibration category: played 2026-11-08 21:00 to 2026-11-09
  06:44 UTC, read 2026-11-09; maintainer-run, agent-read; measurement.
- **Not registered.** The maintainer ran it as a look at the head, and no leaf
  ordered it. It carries no prediction, gates nothing and accepts nothing.
  Disposition: **observation**.
- Baseline / candidate identity: Basilisk `dev-20261108-9c9c2335`, the head of
  `b5-root-aspiration` at `9c9c233`, whose engine source equals master's
  `d76a98c`: Clusters 1 to 3 and the deleted old arms, with **B.5 to B.8
  unplayed**. PGO pext build
  `D:\chess\engines\basilisk\basilisk-dev-20261108-9c9c2335-windows-x64-pext.exe`,
  SHA-256 `05DA63208B4B6010BD463E34834EC29A8D960DA4C356CB79BAE780E99D2DA3D6`;
  re-run on 2026-11-09 it reproduces `bench 13` **4,144,790**, the current
  fingerprint.
- Comparison binaries: Basilisk 1.10.2 (`FA874A89C0FC…`); Rarog 2.5.0
  (`rarog-v2.5.0-windows-pext-pgo.exe`, `1F58452349D0…`, another build of that
  release than BAS-M11's native `AF529F9E…`). The twelve paths, full hashes and
  options are in the results' `engines.json`.
- Conditions: Colosseum desktop app, tournament
  `73a47714-e34c-4604-8ad1-1828baaaeb05`; round robin, 400 cycles × 2
  colour-reversed games, **800 games per pair, 52,800 in all**; `3+0.03`, 1T,
  Hash 128, concurrency 14; UHO `UHO_Lichess_4852_v1.epd` (`7A7F6470…`), random
  order, seed 15533351315; no adjudication, no tablebases, no ponder. The
  config states no affinity. The field left at its defaults: HIARCS 15.4's
  `OwnBook` and `Book Learning` were not turned off (Critter's `OwnBook` was).
- Reading the app's columns: only Basilisk dev and Rarog dev were written back,
  so the **Elo** column sets their new ratings beside everyone else's stored
  ones and its gaps are not measurements (3339 against Rarog 2.5.0's stored
  3281). The **Δ +93** is against a stored 3246, not a measured gain. The
  **Perf** column is this tournament's: Basilisk dev 3339, Rarog 2.5.0 3262,
  Basilisk 1.10.2 3025.

#### RESULT

Integrity, from the app's database read in place, not from the standings: all
52,800 games finished, every pair 400/400 by colour, all 26,400 game pairs on
one opening with colours reversed; terminations checkmate 39,672, threefold
8,676, insufficient material 2,689, fifty-move 1,524, stalemate 239. **No
forfeit, crash or time loss in any game.** The app's crosstable and standings
equal the recomputed ones.

Basilisk dev's head-to-heads; intervals are 95% over the 400 game pairs:

| Opponent | W-D-L | Score | Elo [95%] |
|---|---:|---:|---:|
| Rarog dev-20261108-32199adf | 185-250-365 | 38.75% | −79.5 [−98.5, −61.0] |
| Rarog 2.6.0 | 195-244-361 | 39.62% | −73.2 [−91.7, −55.0] |
| Ethereal 13.07 | 239-248-313 | 45.38% | −32.2 [−50.2, −14.4] |
| Houdini 4 | 405-196-199 | 62.88% | +91.5 [+71.9, +111.8] |
| **Rarog 2.5.0** | **347-268-185** | **60.12%** | **+71.3 [+53.5, +89.5]** |
| HIARCS 15.4 | 413-193-194 | 63.69% | +97.6 [+78.1, +117.7] |
| Stockfish 5 | 415-217-168 | 65.44% | +110.9 [+91.4, +131.1] |
| Shredder 13 | 419-228-153 | 66.62% | +120.1 [+101.0, +139.9] |
| Critter 1.6a | 478-165-157 | 70.06% | +147.7 [+127.4, +169.0] |
| Rarog 2.4.0 | 626-134-40 | 86.62% | +324.5 [+299.4, +352.6] |
| **Basilisk 1.10.2** | **611-157-32** | **86.19%** | **+318.1 [+294.8, +343.9]** |

Joint maximum likelihood over all 66 pairs, 400 bootstrap resamples of each
pair's game pairs; the scale is set by the mean of Critter 1.6a, Houdini 4 and
Rarog 2.4.0's Super Rating Tournament ratings (BAS-X34), the three engines here
with a rating on record:

| Engine | Rating [95%] |
|---|---:|
| Rarog 2.6.0 | 3425 [3419, 3432] |
| Rarog dev-20261108-32199adf | 3413 [3406, 3420] |
| Ethereal 13.07 | 3380 [3373, 3387] |
| **Basilisk dev** | **3344 [3338, 3351]** |
| Houdini 4 | 3281 [3276, 3286] |
| Rarog 2.5.0 | 3270 [3263, 3277] |
| HIARCS 15.4 | 3234 [3227, 3240] |
| Stockfish 5 | 3225 [3219, 3232] |
| Shredder 13 | 3214 [3207, 3221] |
| Critter 1.6a | 3198 [3194, 3203] |
| Basilisk 1.10.2 | 3030 [3023, 3038] |
| Rarog 2.4.0 | 3025 [3019, 3030] |

Pool differences from Basilisk dev, with their own bootstrap intervals:
Basilisk 1.10.2 **+314.2 [+305.1, +323.2]**; Rarog 2.5.0 **+74.6 [+67.2,
+83.3]**; Critter 1.6a +146.2 [+138.7, +154.9]; Houdini 4 +63.8 [+56.1, +72.0];
Ethereal 13.07 −35.4 [−42.7, −27.2]; Rarog dev −68.3 [−76.9, −59.4]; Rarog 2.6.0
−80.6 [−89.1, −72.7].

Readings, under these conditions:

- **Past Rarog's whole search programme with B.5 to B.8 still to come.** Rarog
  2.5.0 is Rarog's release at the end of its Phase B; the head beats it by about
  +71 to +75. 1.10.2 still reads −238.8 against it. The pairing and the pool
  difference agree within 3.3 Elo, so PLAN's sibling matchup warning (35 to 45
  Elo in Rarog's legacy 10.0) does not show here.
- **Above B's budget before B closes.** B's planned recovery is 150 to 250; the
  pool gap to 1.10.2 is +314. The two are different meters: that band prices
  G(0)'s deficit, which is unread on this head, and B.11.1 corrects the table.
- **Evaluation still separates it from the top.** Rarog 2.6.0 is Rarog's release
  after its evaluation programme and stays about +73 to +81 ahead. Ethereal
  13.07 is about +32 to +35 ahead.
- **E.2 is not read.** Of its four targets only Critter 1.6a was played, at 1T.
  Houdini 3, Rybka 4.1 and Fritz 16 were not in the field, and nothing ran at
  4T. Houdini 4, not a target, rated 23 above Houdini 3 on the Super Rating
  Tournament's scale, and it scored 37.12% here; PLAN §1's "Houdini 3 falls, if
  at all, only with C" now looks too cautious, which only the E.2 field can say.
- **Speed.** Median in-game NPS as the engines reported it: Basilisk dev 1.71M,
  Basilisk 1.10.2 2.53M (−32%), Rarog 2.5.0 1.65M, Rarog 2.6.0 2.07M; mean depth
  17.9 against 1.10.2's 15.1. This is not an NPS read (14 games at once, each
  engine's own count); it says only that B.8.5's speed pass has room to look.

#### Conditional lesson and retry trigger

A pool read of the head is cheap beside its information: a night's 52,800 games
priced the whole of Clusters 1 to 3 against fixed outside engines, which paired
gates between neighbours cannot do. It does not replace B.11.1's pool read,
which runs on the final search head with the four targets and the deficit
meters. BAS-M08's conversion read was not repeated here; it stays with B.11.1.

- Source: maintainer's Colosseum tournament; PLAN §1, B.11.1.
- Artifacts: `tools/results/m15-dev-test/` (ignored): `games.tsv` (one row a
  game, SHA-256
  `63A1C7018F52AEA9637838C650A6DB10A861295FD84E34CC5F5BC77DC3E694B5`),
  `engines.json`, `tournament-config.json`, `export.py`, `analyse.py`,
  `analyse.log`. PGNs stay in the app's database under the tournament ID above;
  `tools/diag/export_tournament_pgn.py` exports them.
