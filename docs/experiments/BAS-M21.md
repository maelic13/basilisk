# BAS-M21

**BAS-M21 — the HCE frontier field on the MacBook Air M4: BAS-M18's field rebuilt natively, a 12-engine round robin, stopped at 7,896 games**

- Date / owner / calibration category: run 2026-11-18, maintainer-run in
  the Colosseum GUI on the MacBook Air, agent-read; measurement,
  observation. Not registered in the ledger before exposure; the only prior
  is the provisional rating each new engine carried into the run (below),
  written to the GUI library before any game.
- Why: to rate Basilisk 1.11.0 and Rarog 2.7.0 on the Mac, against BAS-M18's
  HCE frontier field built natively for Apple silicon, and to see whether
  BAS-M18's distances, measured on the Ryzen 9 5950X, hold on a second host.
- Host: MacBook Air, Apple M4 (4 performance and 6 efficiency cores,
  fanless), 24 GB, macOS 27.2.1. At concurrency 8, games ran on both kinds
  of core, so per-game speed varied; the variation falls on every engine
  alike in expectation and adds only noise.
- **Field**: BAS-M18's engines where a native arm64 build exists, plus the
  Mac library's rated engines. 1 thread, Hash 128, move overhead 10 ms
  where the engine has the option (Basilisk, Rarog and Koivisto play their
  defaults).

  | Engine | Build | Provisional rating | SHA-256 |
  |---|---|---|---|
  | Stockfish 16, `Use NNUE=false` | LLVM 23 PGO | 3558 | `271C43C8…` |
  | Stockfish 11 | native PGO, earlier Mac runs | **3497, fixed** | `B81F133B…` |
  | Berserk 4.7.0, no network | LLVM 23 PGO, shim | 3498 | `0B30DD4F…` |
  | Fire 05192023 | native PGO, earlier Mac runs | 3397, Mac-measured | `442993B1…` |
  | Houdini 6.03 | native PGO, earlier Mac runs | 3420, Mac-measured | `97B1777B…` |
  | Ethereal 14.40, no network | LLVM 23 PGO, shim | 3421 | `22E4AC42…` |
  | Rarog 2.7.0 | maintainer's arm64 build | 3399 | `BB9BFC39…` |
  | Dragon 3.3, `Use Regular Eval=true` | vendor `dragon-3.3-macos-m1` | 3378 | `CEBB809B…` |
  | Ethereal 13.07 | LLVM 23 PGO, shim | 3356 | `B40C2C46…` |
  | Basilisk 1.11.0 | tag `v1.11.0` (`f1e31f4`), LLVM 23 PGO | 3358 | `771112D2…` |
  | Rarog 2.5.0 | native PGO, earlier Mac runs | 3245, Mac-measured | `A3625C84…` |
  | Koivisto 4.83 | LLVM 23 PGO, shim | 3151 | `F1D2E605…` |

  - A provisional rating is BAS-M18's "only Stockfish 11 fixed" rating
    less 35, the Mac library's Stockfish 11 (3497) less the Ryzen's (3532).
    Houdini 6.03 and Rarog 2.5.0, rated on the Mac by earlier runs, sit 3
    and 10 from that rule.
  - Not in the field: Komodo 14.1 (no native build; it runs under Rosetta),
    Booot 6.5 (dropped after BAS-M18) and rofChade 2.3 (dropped by the
    maintainer). Fire 05192023 stands in for BAS-M18's Fire 8.3, which has
    no Mac build; it is a different version.
  - Builds, all generic arm64 with Homebrew LLVM 23.1.6 and instrumented
    PGO trained on each engine's own `bench`. LLVM 23 was 0.5–9.6% faster
    than Apple clang 21, and `-mcpu=apple-m4` gained nothing. Basilisk:
    `cmake -DCOMP=llvm`, `pgo` target, `bench 13` 4,777,412 reproduced on
    the installed binary. Stockfish 16: `make profile-build
    ARCH=apple-silicon COMP=clang` with LLVM 23 first on `PATH`; its bench
    of 3,684,832 equals the `sf_16` classical bench that BAS-O09 records.
    The other sources are BAS-M18's (Ethereal 14.40 without a network,
    Berserk 4.7.0 static).
  - **The shim.** macOS gives a secondary thread a 512 KB stack where Linux
    gives 8 MB. Ethereal 14.40's first build crashed (SIGBUS, stack guard)
    on its first search, because LTO folds about 512 KB of locals into
    `getBestMove`. `bench` runs on the main thread and cannot show this. The
    Ethereal, Berserk and Koivisto builds are compiled with a forced-include
    header (`-include`) that wraps `pthread_create` to request 8 MB unless the
    caller sets a size. No engine source changed. Benches are identical with
    and without it, NPS within 0.3%. Stockfish 16 sets its own 8 MB stack.
    Every build was then qualified by 40 game-like UCI searches per engine,
    not by bench alone.
  - The first attempt crashed on Ethereal 14.40's unshimmed build after 4
    games and was deleted. The three ratings its write-back moved were reset
    to their provisional values before this run (the run's start ratings
    confirm it).
- Research questions: where Basilisk 1.11.0 and Rarog 2.7.0 rate on the
  Mac; whether BAS-M18's order and gaps transfer to a second host.

#### RESULT — appended after exposure

- Run 2026-11-18 13:50 to 16:27 UTC (2 h 37 min), about 3,020 games an
  hour. **Stopped by the maintainer at 7,896 of the registered 13,200
  games** (100 cycles; round 658 of 1,100), for lack of time, not on
  the result. Every pairing played 118 or 120 games in complete
  colour-reversed pairs (3,948 pairs, no unpaired game), so the partial
  run is balanced.
- Conditions: Colosseum 1.2.0 GUI round robin, 2 games per opening,
  `3+0.03`, 1 thread, Hash 128, concurrency 8, no ponder, no tablebases,
  no adjudication, no move cap. UHO `UHO_Lichess_4852_v1.epd`, random
  order, seed `6748654`. Write-back "Chosen", with every engine but
  Stockfish 11 ticked.
- Every game ended naturally: 5,428 mates, 1,525 repetitions, 546
  insufficient material, 342 fifty-move draws and 55 stalemates. There were
  no crashes, illegal moves or time losses. Clocks rebuilt from the PGN's
  move times put Rarog 2.5.0 6 ms below zero once. That is inside the GUI's
  2,000 ms margin, and no other engine went below zero.
- Draws were 31.3%; White scored 52.9%.
- Ratings come from a joint maximum-likelihood logistic fit with Stockfish
  11 fixed at 3497, with 95% intervals from 200 pentanomial resamples per
  pairing. The GUI's own table reads the same to within 1.

  | # | Engine | Mac, SF11 fixed | W-D-L | Provisional | BAS-M18, SF11 fixed | Mac − Ryzen − mean |
  |---|---|---|---|---|---|---|
  | 1 | Stockfish 16 HCE | **3544** [3527, 3565] | 746-410-160 | 3558 | 3593 | −0.5 |
  | 2 | Stockfish 11 | 3497 fixed | 666-411-239 | — | 3532 fixed | — |
  | 3 | Berserk 4.7.0 HCE | **3491** [3472, 3509] | 673-376-267 | 3498 | 3533 | +6.8 |
  | 4 | Fire 05192023 | **3415** [3396, 3432] | 494-452-370 | 3397 | (Fire 8.3) | — |
  | 5 | Houdini 6.03 | **3406** [3385, 3425] | 479-448-389 | 3420 | 3452 | +2.8 |
  | 6 | Ethereal 14.40 HCE | **3403** [3382, 3419] | 468-458-390 | 3421 | 3456 | −4.6 |
  | 7 | Rarog 2.7.0 | **3387** [3363, 3408] | 448-436-432 | 3399 | 3434 | +1.6 |
  | 8 | Dragon 3.3 HCE | **3349** [3330, 3367] | 389-410-517 | 3378 | 3413 | −15.7 |
  | 9 | Ethereal 13.07 | **3343** [3320, 3360] | 363-442-511 | 3356 | 3391 | +1.2 |
  | 10 | **Basilisk 1.11.0** | **3340** [3323, 3357] | 375-407-534 | 3358 | 3393 | −3.9 |
  | 11 | Rarog 2.5.0 | **3254** [3232, 3271] | 222-404-690 | 3245 | 3290 | +13.1 |
  | 12 | Koivisto 4.83 | **3136** [3109, 3161] | 105-282-929 | 3151 | 3186 | −1.0 |

  The last column is each engine's Mac rating less its BAS-M18 rating,
  less the mean of that difference over the ten engines both fields share
  (−48.8). That mean is 13.8 more negative than the −35 rule. Relative to
  this field, Stockfish 11 plays about 14 stronger on the M4 than on the
  5950X.
- The head's matches, 118 or 120 games each, with Elo and 95% intervals
  from the pentanomial:

  | Opponent | Head's W-D-L | Mac | BAS-M18 (600 games) |
  |---|---|---|---|
  | Stockfish 16 HCE | 14-37-69 | −172.0 [−221.1, −128.7] | −193.2 |
  | Stockfish 11 | 12-42-66 | −168.4 [−218.7, −124.0] | −127.0 |
  | Berserk 4.7.0 HCE | 20-29-71 | −157.7 [−215.4, −107.1] | −145.1 |
  | Fire 05192023 | 20-40-60 | −120.4 [−165.5, −78.9] | (Fire 8.3 +16.2) |
  | Rarog 2.7.0 | 29-34-55 | −77.8 [−124.2, −34.0] | −37.8 |
  | Ethereal 14.40 HCE | 32-37-51 | −55.5 [−98.1, −14.5] | −71.0 |
  | Houdini 6.03 | 33-38-49 | −46.6 [−97.5, +2.4] | −44.8 |
  | Ethereal 13.07 | 41-37-42 | −2.9 [−45.2, +39.4] | −9.3 |
  | Dragon 3.3 HCE | 41-40-37 | +11.8 [−35.1, +59.1] | +7.5 |
  | Rarog 2.5.0 | 55-42-23 | +94.9 [+45.1, +148.9] | +97.4 |
  | Koivisto 4.83 | 78-31-11 | +219.0 [+170.5, +276.6] | +209.9 |

- Readings:
  - **BAS-M18's field transfers to the M4.** After the one common offset,
    every shared engine lands within 16 of its Ryzen rating, inside this
    run's ±20 intervals. Dragon 3.3 (−15.7) and Rarog 2.5.0 (+13.1) are
    the largest. The order is the same except that Basilisk, Ethereal
    13.07 and Dragon 3.3, level on both hosts, swap places within 9.
  - **The head is 203 below Stockfish 16 HCE** (Ryzen, same fit: 200). It
    is 10th of 12, level with Ethereal 13.07 and Dragon 3.3, and 46 below
    Rarog 2.7.0 on rating (Ryzen 41).
  - The head-to-head reads against Stockfish 11 (−168) and Rarog 2.7.0
    (−78) are 41 and 40 worse than on the Ryzen. Their intervals are about
    ±45 at 120 games, the joint fit moves the head by only 4, and nothing
    is read into the two matches.
  - **The Mac scale holds better than the Ryzen's did.** The three
    engines with Mac-measured library ratings land within 18: Houdini
    −14, Fire +18, Rarog 2.5.0 +9. BAS-M18's scale-transfer falsifier
    fired at about ±50.
  - Ethereal 13.07 at 3343 [3320, 3360] agrees with Ethereal 13.00's Mac
    rating of 3356; the two versions played level on the Ryzen.
  - The provisional ratings were 14 too high on average, the Stockfish 11
    offset above. Rarog 2.7.0's estimate of 3399 missed by 12, Basilisk's
    3358 by 18.
- Evidence (not committed): `tools/results/bas-m21/` holds `games.pgn`
  (7,896 games, SHA-256 `94A56003…`, exported from the GUI database's
  stored PGN), `fit.py`, `fit.txt` (every pairing) and `fit.json`. The
  database was read from a `.backup` snapshot taken with the GUI running.
- Disposition: OBSERVATION, partial run (60% of the planned games). The
  Mac reproduces BAS-M18's distances to within 16 at about 3,000 games an
  hour. Basilisk 1.11.0 stands 203 below Stockfish 16 HCE on both hosts.
