# BAS-M18

**BAS-M18 — B.11.5 HCE frontier field: a rated round robin of the strongest hand-crafted-evaluation engines**

- Date / owner / calibration category: registered 2026-11-17, before any
  game; maintainer-run overnight in the Colosseum GUI, agent-read;
  measurement.
- Why: §1's aim is to become the strongest hand-crafted-evaluation engine
  before NNUE. This run is that aim's meter: what each contender really
  rates on the pool's scale, and how far the search programme's head is from
  the top.
- Timing: the maintainer runs it before the 1.11.0 release (2026-11-17),
  on master. Master's engine source is the release candidate's: `src/` is
  identical between master `4d2742f` and the head `4c0bc0e` that BAS-M16
  and BAS-M17 played. The maintainer built it from the branch at
  `e907e60`, whose `src/` is master's. It reads `id name Basilisk
  dev-20261117-e907e603` and `bench 13` 4,777,412, checked 2026-11-17.
- **Field**: one version per engine family, plus five already-rated anchors
  that are themselves contenders. Every engine plays 1 thread, Hash 128, with
  move overhead 10 ms where it has the option.

  | Engine | Role | Library rating | SHA-256 |
  |---|---|---|---|
  | Basilisk `dev-20261117-e907e603` | contender | 3416, BAS-M16's performance | `685D0E06…` |
  | Stockfish 16, `Use NNUE=false` | contender | 3555 est. | `041396A4…` |
  | Dragon 3.3, `Use Regular Eval=true` | contender | 3510 est. | `90CA3D6A…` |
  | Berserk 4.7.0 | contender | 3500 est. | `2F1D85FA…` |
  | Ethereal 14.40, no network | contender | 3449, measured against 13.07 | `317A21DB…` |
  | Rarog 2.7.0, local pext build | contender | 3415 est. | `9DAD2C99…` |
  | Booot 6.5 | contender | 3365 est. | `12BF463B…` |
  | rofChade 2.3, BMI2 | contender | 3345 est. | `D68E2AC8…` |
  | Koivisto 4.83 | contender | 3250 est. | `A42384F3…` |
  | Stockfish 11 | anchor | 3532 | `271B62DC…` |
  | Houdini 6.03 | anchor | 3499 | `E5C0FDC5…` |
  | Fire 8.3 | anchor | 3423 | `C2967A3D…` |
  | Ethereal 13.07 | anchor | 3356 | `61F0270F…` |
  | Rarog 2.5.0 | anchor | 3281 | `1F584523…` |

  - Rarog 2.5.0 was added by the maintainer before the run. It is Rarog's
    release at the end of its search programme, the same place in its plan
    as this head, and it extends the anchors down to 3281.

  - The anchors span 3281–3532, the field's range bar its two ends.
  - Stockfish 15.1 HCE (3543) and Komodo 14.1 (3476) sit out. Their
    ratings are already on the scale, so Stockfish 16 and Dragon 3.3 are
    compared with them without playing them.
  - Rarog 2.7.0 joins by the rule that every Rarog comparison includes its
    latest release. It is the maintainer's local build, which BAS-M17's
    build check found as fast as the release asset.
  - Option fixes made before the run: Houdini 6.03 `Move Overhead=10`
    (default 0), and Komodo 14.1 `OwnBook=false`.
- Research questions: who leads hand-crafted evaluation today, and by how
  much; where the head stands in that field; whether the field's estimated
  ratings hold.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Basilisk master: **3416**, 80% band **3385–3450**.
- The leader is Stockfish 16 HCE, probability 0.55, at **3555**, 80% band
  3500–3610. Otherwise it is Stockfish 11, Dragon 3.3 HCE or Berserk 4.7.0.
- The head's gap to the leader: **140**, 80% band **90–190**.
- The head places 7th to 9th of 14, probability 0.8.
- No Basilisk crash, illegal move or time loss: probability 0.95. Booot
  6.5 has no overhead option, so it is the most likely engine to lose on
  time.
- Falsifiers:
  - **Scale transfer.** In a fit with only Stockfish 11 fixed, each other
    anchor should land within 30 of its library rating. If one does not,
    the pool's scale does not carry over to this field, and both fits are
    reported.
  - **Time losses.** An engine that loses more than 1% of its games on
    time is listed. Its rating is reported with and without those games.
- Registered design and stop rule:
  - One fixed run, no extension and no re-run on the same binaries.
  - If the host restarts mid-run, the run is resumed if the GUI can resume
    it. Otherwise the partial games are read and the shortfall recorded.
  - Reported as each engine's rating with its 95% interval and its
    W-D-L, the cross-table, faults and time losses.
- Rating method:
  - Colosseum GUI write-back is set to **Chosen engines**, with the nine
    contenders ticked. The anchors' library ratings stay untouched.
  - The agent re-reads the PGN with a joint maximum-likelihood fit with
    all five anchors fixed. That re-read is the reported rating.
- Full conditions:
  - Colosseum GUI preset `HCE frontier RR 3+0.03`
    (`%APPDATA%\Colosseum\config\presets\hce_frontier_rr.json`).
  - Round robin of 14 engines, 300 cycles × 2 games: 600 games per pair in
    colour-reversed pairs, 91 pairs, **54,600 games**.
  - `3+0.03`, 1 thread, Hash 128, concurrency 14, no ponder.
  - No adjudication, no move cap, no tablebases.
  - UHO `UHO_Lichess_4852_v1.epd`, random order, seed `20261117`.
  - About 11.3 hours at the 4,824 games an hour of the Ethereal round robin.
  - Maintainer's decisions before any game (2026-11-17): Hash 128 in place
    of the preset's 64, and 300 cycles in place of 200.
  - PGN written to `tools/results/bas-m18/games.pgn` (not committed).
- Afterwards, at the maintainer's choice: one pairing is replayed under
  fastchess 1.8.0 at the same conditions, as PLAN B.11.5's harness
  cross-check and fastchess-GUI trial.

#### RESULT — append after exposure

- Run 2026-11-17 20:38 to 2026-11-18 07:37 (9 h 58 min), by the maintainer
  (PGN `tools/results/bas-m18/games.pgn`; fit `fit.txt` beside it). All
  54,600 games were played and paired, 27,300 colour-reversed pairs over
  91 pairings, with no unpaired game.
- Every game ended naturally: 38,677 mates, 9,595 repetitions, 3,857
  insufficient material, 2,192 fifty-move draws and 279 stalemates. There
  were no time losses, crashes or illegal moves by any engine.
- Draws were 29.2%; White scored 53.3%.
- Ratings come from a joint maximum-likelihood logistic fit with the five
  anchors fixed, with 95% intervals from 200 pentanomial resamples per
  pairing. The intervals omit the anchors' own error. The GUI's Chosen
  write-back reads the same values to within 0.5.

  | # | Engine | Five anchors fixed | Only Stockfish 11 fixed | Estimate before |
  |---|---|---|---|---|
  | 1 | Stockfish 16 HCE | **3605** [3599, 3612] | 3593 | 3555 |
  | 2 | Berserk 4.7.0 HCE | **3545** [3538, 3550] | 3533 | 3500 |
  | 3 | Stockfish 11 | 3532 fixed | 3532 fixed | — |
  | 4 | Houdini 6.03 | 3499 fixed | 3452 | — |
  | 5 | Ethereal 14.40 HCE | **3467** [3461, 3473] | 3456 | 3449 |
  | 6 | Rarog 2.7.0 | **3445** [3438, 3451] | 3434 | 3415 |
  | 7 | Dragon 3.3 HCE | **3425** [3418, 3430] | 3413 | 3510 |
  | 8 | Fire 8.3 | 3423 fixed | 3374 | — |
  | 9 | **Basilisk `e907e603`** | **3403** [3397, 3409] | 3393 | 3416 |
  | 10 | Ethereal 13.07 | 3356 fixed | 3391 | — |
  | 11 | rofChade 2.3 | **3281** [3275, 3287] | 3271 | 3345 |
  | 12 | Rarog 2.5.0 | 3281 fixed | 3290 | — |
  | 13 | Koivisto 4.83 | **3196** [3188, 3203] | 3186 | 3250 |
  | 14 | Booot 6.5 | **3138** [3130, 3146] | 3128 | 3365 |

- The head's matches, 600 games each, with Elo and 95% intervals from the
  pentanomial:

  | Opponent | Head's W-D-L | Elo |
  |---|---|---|
  | Stockfish 16 HCE | 67-163-370 | −193.2 [−217.7, −170.3] |
  | Berserk 4.7.0 HCE | 105-153-342 | −145.1 [−169.2, −122.3] |
  | Stockfish 11 | 107-176-317 | −127.0 [−149.5, −105.4] |
  | Ethereal 14.40 HCE | 146-187-267 | −71.0 [−92.3, −50.3] |
  | Houdini 6.03 | 163-197-240 | −44.8 [−64.9, −25.1] |
  | Rarog 2.7.0 | 169-197-234 | −37.8 [−59.3, −16.5] |
  | Ethereal 13.07 | 194-196-210 | −9.3 [−31.0, +12.4] |
  | Dragon 3.3 HCE | 198-191-211 | −7.5 [−28.2, +13.1] |
  | Fire 8.3 | 204-220-176 | +16.2 [−2.9, +35.4] |
  | Rarog 2.5.0 | 275-214-111 | +97.4 [+77.9, +117.6] |
  | rofChade 2.3 | 315-185-100 | +130.3 [+109.2, +152.3] |
  | Koivisto 4.83 | 386-152-62 | +209.9 [+185.4, +236.4] |
  | Booot 6.5 | 397-143-60 | +220.7 [+195.0, +248.7] |

- Calibration against the registered predictions:
  - Head at 3403: inside the band (3385–3450), 13 below the point.
  - The leader is Stockfish 16 HCE, as predicted (probability 0.55), at
    3605, at the top edge of its band (3500–3610).
  - **Missed**: the head's gap to the leader is **202** (−193 head to
    head), above the band of 90–190.
  - The head placed 9th of 14 by rating and 8th by points; the band was
    7th to 9th.
  - No Basilisk fault, as predicted at 0.95. Booot 6.5 lost nothing on
    time.
- **The scale-transfer falsifier fired.** With only Stockfish 11 fixed:
  - Houdini 6.03 lands 47.5 below its library rating, and Fire 8.3 49.3
    below;
  - Ethereal 13.07 lands 34.8 above;
  - only Rarog 2.5.0 lands within 30 (+8.7).

  So the pool's ratings do not carry over to this field to better than
  about ±50. The five-anchor fit averages that out, but its ratings carry
  that much uncertainty on the pool's scale. **Distances inside this field
  are the robust reading**: the two fits agree on every gap to within 12.
- Readings:
  - **Stockfish 16 HCE is the strongest hand-crafted-evaluation engine in
    this field.** It leads Berserk 4.7.0 by 60 and Stockfish 11 by 73
    (+69 head to head).
    - Its library rating stands about 60 above Stockfish 15.1 HCE's
      (3543), across the scale caveat.
    - C.0's donor choice expects SF16's evaluation. This field cannot
      separate its evaluation from its search.
  - **The head is 202 below the top and 142 below second.** It is level
    with Dragon 3.3 HCE and Ethereal 13.07, and 38 below Rarog 2.7.0.
  - **Against Rarog 2.5.0**, Rarog's release at the end of its own search
    programme, the head reads **+97.4**. BAS-M15 read +74.6 after Clusters
    1 to 3.
  - **Dragon 3.3's regular evaluation** reads 85 below its estimate. On
    library ratings Komodo 14.1 (3476) stands above it, so 14.1 is the
    Komodo family's stronger hand-crafted engine on this evidence.
  - **Booot 6.5** read 227 below its estimate, and rofChade and Koivisto
    about 60 below. These are older engines at `3+0.03` 1T.
    - Booot exposes no UCI options at all, Hash and Threads included, so
      its setup was never checked.
    - Its rating here is not read as its strength until that is checked.
- Disposition: COMPLETE. It is §1's aim's first meter reading: about 200
  below Stockfish 16 HCE. PLAN B.11.5's fastchess replay of one pairing is
  still open.
