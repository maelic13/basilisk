# BAS-M22

**BAS-M22 — the HCE frontier field on Windows arm64 (Snapdragon X Plus): a 9-engine round robin, seven engines native and two under x64 emulation, stopped at 9,495 games**

- Date / owner / calibration category: run 2026-11-20, maintainer-run in
  the Colosseum GUI on the Surface Laptop, agent-read; measurement,
  observation. Not registered in the ledger before exposure; the only prior
  is each engine's library rating at the start of the run (below).
- Why: to rate Basilisk 1.11.0 and Rarog 2.7.0 on a third host and a
  third platform, **Windows on arm64**, against the HCE frontier field of
  BAS-M18 (Ryzen 9 5950X, x64) and BAS-M21 (MacBook Air M4, macOS arm64).
- Host: Microsoft Surface Laptop, 7th Edition. Qualcomm Snapdragon X Plus
  X1P64100, 10 Oryon cores (one core type) at 3.4 GHz, 16 GB, **Windows 11
  Pro 10.0.26300 on arm64**. Concurrency 6 on 10 cores.
- **Field: seven engines run natively as Windows arm64 binaries, and two
  run as x64 binaries under Windows' Prism x64 emulation.** The
  architecture is read from each binary's PE header (machine `0xAA64`
  arm64, `0x8664` x64). 1 thread, Hash 128, move overhead 10 ms in every
  engine (each engine's own option name).

  | Engine | Runs | Build | Start rating | SHA-256 |
  |---|---|---|---|---|
  | Stockfish 16, `Use NNUE=false` | **native arm64** | `sf_16`, clang `armv8-dotprod`, `-mcpu=native`, PGO on the classical bench | 3543 | `689E4EEE…` |
  | Stockfish 11 | **native arm64** | `stockfish_11-arm64-oryon-pgo.exe`, earlier library build | **3481, fixed** | `E260F82E…` |
  | Berserk 4.7.0, no network | **native arm64** | tag `4.7.0`, clang `-march=native` LTO, PGO | 3482 | `D2E63D39…` |
  | Ethereal 14.40, no network | **native arm64** | `0e47e9b`, clang `-march=native` LTO, `USE_NNUE=0`, PGO | 3406 | `CA1F3F12…` |
  | Houdini 6.03 | **native arm64** | `Houdini_6.03_Pro_arm64-pgo.exe`, earlier library build | 3414 | `3C434109…` |
  | Rarog 2.7.0 | **native arm64** | maintainer's `rarog-2.7.0-windows-arm64.exe` | 3384 | `EC785099…` |
  | Basilisk 1.11.0 | **native arm64** | `basilisk-1.11.0-windows-arm64.exe`; `bench 13` 4,777,412 reproduces tag `v1.11.0` (`f1e31f4`) | 3345 | `6F2009D0…` |
  | Komodo 14.1 | **x64, emulated** | vendor `komodo-14.1-64bit.exe` | 3349 | `9320B52F…` |
  | Dragon 3.3, `Use Regular Eval=true` | **x64, emulated** | vendor `dragon-3.3-64bit.exe` | 3362 | `77D50425…` |

  - Stockfish 16, Berserk 4.7.0 and Ethereal 14.40 were built with
    `compile/frontier_arm64.sh` (MSYS2 clangarm64, instrumented PGO trained
    on each engine's own bench, statically linked). Stockfish 11 and
    Houdini 6.03 are earlier arm64 PGO builds already in the library; their
    recipe is not recorded here.
  - Komodo 14.1 and Dragon 3.3 have no Windows arm64 build. Their ratings
    here include whatever emulation costs them (Readings).
  - Not in the field from BAS-M21: Fire 05192023, Ethereal 13.07, Rarog
    2.5.0, Koivisto 4.83. Komodo 14.1, absent on the Mac, plays here.
- Research questions: where Basilisk 1.11.0 and Rarog 2.7.0 rate on
  Windows arm64; whether BAS-M18's and BAS-M21's gaps transfer to a third
  host; what x64 emulation does to the two emulated engines.

#### RESULT — appended after exposure

- Run 2026-11-20 09:40 to 15:15 UTC. It was paused from 10:56 to 12:54
  while a 30-game run (“HCE Blitz”) used the machine, so it played for
  3 h 37 min, about 2,620 games an hour. **Stopped by the maintainer at
  9,495 of the registered 14,400 games** (200 cycles; round 1,187 of
  1,800), for lack of time, not on the result. Every pairing played 131 or
  132 colour-reversed pairs (4,747 pairs). One game, a draw with Komodo
  14.1 as White against Basilisk in round 1,187, is unpaired because its
  colour-reversed twin was still pending at the stop. The fit and
  the pairwise reads below use only complete pairs.
- Conditions: Colosseum 1.2.0 GUI round robin, 2 games per opening,
  `3+0.03`, 1 thread, Hash 128, concurrency 6, no ponder, no tablebases,
  no adjudication, no move cap. UHO `UHO_Lichess_4852_v1.epd`, random
  order, seed `20261117`. Write-back "Chosen", with every engine but
  Stockfish 11 ticked.
- Every game ended naturally: 6,436 mates, 1,859 repetitions, 715
  insufficient material, 439 fifty-move draws and 46 stalemates. There were
  no crashes, illegal moves or time losses. Clocks rebuilt from the PGN's
  move times went below zero for Ethereal 14.40 (−648 ms at worst), Berserk
  (−214), Basilisk (−177), Stockfish 11 (−58) and Stockfish 16 (−30). All
  of these are inside the GUI's 2,000 ms margin. The two emulated engines
  never went below 23 ms.
- Draws were 32.2%; White scored 53.8%.
- Ratings come from a joint maximum-likelihood logistic fit with Stockfish
  11 fixed at 3481, with 95% intervals from 200 pentanomial resamples per
  pairing. The GUI's own table reads the same to within 1. W-D-L counts
  every game, the unpaired one included. Mean NPS is the GUI's per-game
  average.

  | # | Engine | Runs | Win arm64, SF11 fixed | W-D-L | Mean NPS | Start | BAS-M21 Mac | BAS-M18 Ryzen | Win − Mac − mean | Win − Ryzen − mean |
  |---|---|---|---|---|---|---|---|---|---|---|
  | 1 | Stockfish 16 HCE | native | **3536** [3522, 3550] | 1140-650-320 | 2.09M | 3543 | 3544 | 3593 | −0.4 | −1.1 |
  | 2 | Stockfish 11 | native | 3481 fixed | 944-712-454 | 2.13M | — | 3497 fixed | 3532 fixed | — | — |
  | 3 | Berserk 4.7.0 HCE | native | **3463** [3449, 3478] | 920-644-546 | 2.23M | 3482 | 3491 | 3533 | −20.0 | −13.7 |
  | 4 | Rarog 2.7.0 | native | **3398** [3385, 3411] | 671-719-720 | 1.69M | 3384 | 3387 | 3434 | +19.1 | +20.4 |
  | 5 | Ethereal 14.40 HCE | native | **3395** [3379, 3410] | 686-671-755 | 2.72M | 3406 | 3403 | 3456 | 0.0 | −4.7 |
  | 6 | Houdini 6.03 | native | **3393** [3379, 3408] | 652-724-734 | 2.19M | 3414 | 3406 | 3452 | −5.0 | −2.7 |
  | 7 | Komodo 14.1 | **emulated** | **3344** [3330, 3360] | 487-735-887 | 1.48M | 3349 | — | — | — | — |
  | 8 | **Basilisk 1.11.0** | native | **3338** [3324, 3354] | 520-633-956 | 1.74M | 3345 | 3340 | 3393 | +6.4 | +1.7 |
  | 9 | Dragon 3.3 HCE | **emulated** | **3304** [3288, 3321] | 416-630-1064 | 1.47M | 3362 | 3349 | 3413 | **−36.5** | **−52.2** |

  The last two columns are each engine's rating here less its rating on
  the other host, less the mean of that difference over the six engines
  all three fields share that run natively here (−8.1 against the Mac,
  −56.4 against the Ryzen). Dragon 3.3, the seventh shared engine, is
  left out of the mean because it runs emulated here; including it would
  pull the offset down by 5–8 and shift every native residual up by as
  much.
  Dragon 3.3 played natively on both other hosts: the vendor
  `dragon-3.3-macos-m1` build on the Mac and the x64 build on the Ryzen.
- The head's matches, 131 or 132 pairs each, with Elo and 95% intervals
  from the pentanomial:

  | Opponent | Runs | Head's W-D-L | Win arm64 | BAS-M21 Mac (120 games) | BAS-M18 Ryzen (600 games) |
  |---|---|---|---|---|---|
  | Stockfish 16 HCE | native | 31-75-158 | −182.2 [−218.8, −149.0] | −172.0 | −193.2 |
  | Stockfish 11 | native | 36-77-151 | −162.2 [−196.6, −130.5] | −168.4 | −127.0 |
  | Berserk 4.7.0 HCE | native | 57-72-135 | −105.8 [−140.1, −73.4] | −157.7 | −145.1 |
  | Rarog 2.7.0 | native | 59-89-114 | −74.0 [−103.6, −45.5] | −77.8 | −37.8 |
  | Houdini 6.03 | native | 69-93-102 | −43.7 [−72.8, −15.1] | −46.6 | −44.8 |
  | Ethereal 14.40 HCE | native | 80-74-110 | −39.7 [−71.1, −8.8] | −55.5 | −71.0 |
  | Komodo 14.1 | emulated | 84-91-87 | −4.0 [−37.3, +29.3] | — | — |
  | Dragon 3.3 HCE | emulated | 104-61-99 | +6.6 [−27.8, +41.1] | +11.8 | +7.5 |

- Readings:
  - **The head is 197 below Stockfish 16 HCE** (Mac 203, Ryzen 200, each
    in its own fit). It is 8th of 9. Komodo 14.1 is level with it (+6) and
    Dragon 3.3 is 34 below, but both of those run emulated. Among the
    native engines the head is last, 55 below the Houdini 6.03 and
    Ethereal 14.40 pair.
  - **The head is 60 below Rarog 2.7.0** on rating (Mac 46, Ryzen 41). The
    head-to-head reads −74 [−104, −46], the same as the Mac's −78. Rarog
    2.7.0 carries this host's largest native residual (+19 against the
    Mac, +20 against the Ryzen), so the wider gap is Rarog reading higher
    here, not the head reading lower (+6 and +2).
  - **The native engines transfer.** After the one common offset, every
    native engine shared with BAS-M21 lands within 20 of its Mac rating
    and within 21 of its Ryzen rating. Four of the six are within 7 on
    both; the exceptions are Rarog 2.7.0 (above) and Berserk 4.7.0 (−20,
    −14). Among the native engines the order is unchanged except that
    Rarog 2.7.0 moves above Ethereal 14.40 and Houdini 6.03, which also
    swap; all three are within 5 of each other.
  - **Inference: emulation costs Dragon 3.3 roughly 35–50.** It reads
    37 below its native Mac rating and 52 below its native Ryzen rating
    after the offset, the largest residual in the field. No native engine
    misses by more than 21 on either comparison. It also starts 58 above
    where it finishes, the largest move of the run. Komodo 14.1 has no
    native read on these hosts to compare with, but it runs under the same
    emulation at the same mean NPS (1.48M against Dragon's 1.47M), so its
    3344 is taken as an emulated rating, not a native one. **The head's
    level matches with Komodo 14.1 (−4) and Dragon 3.3 (+7) are against
    handicapped opponents and should not be read as parity with those
    engines' native builds.**
  - The head-to-head reads against Berserk 4.7.0 (−106) and Ethereal
    14.40 (−40) are better than on the Mac and Ryzen, by 39–52 and 16–31
    respectively. Their
    intervals are about ±33 at 132 pairs, the joint fit leaves the head
    within 7 of its other ratings, and nothing is read into the two
    matches.
  - The scale holds. Stockfish 11 fixed at the Windows library's 3481
    gives a native mean offset of −8.1 to the Mac and −56.4 to the Ryzen,
    against −16 and −51 between the anchors. Relative to the native
    field, Stockfish 11 plays about 8 weaker here than on the Mac and 5
    stronger than on the Ryzen.
  - The start ratings were 14 too high on average. Most of that is Dragon
    3.3 (58), Houdini 6.03 (21) and Berserk 4.7.0 (19). Basilisk's 3345
    was 7 too high; Rarog 2.7.0's 3384 was 14 too low.
- Evidence (not committed): `tools/results/bas-m22/` holds `games.pgn`
  (9,495 games, SHA-256 `132210C7…`, exported from the GUI database's
  stored PGN), `fit.py`, `fit.txt` (every pairing), `fit.json` and
  `snapshot.sqlite`. The database was read from a `.backup` snapshot taken
  with the GUI running, tournament `4f25bc8a-4d69-459c-8262-29a9f2510923`.
- Disposition: OBSERVATION, partial run (66% of the planned games). On
  Windows arm64 the native engines reproduce BAS-M21's distances to within
  20 at about 2,600 games an hour. Basilisk 1.11.0 stands 197–203 below
  Stockfish 16 HCE on all three hosts. The two x64 engines running under
  emulation read low: Dragon 3.3 by about 35–50, and Komodo 14.1 by an
  unmeasured amount. A rating from this host for either is not a native
  rating.
