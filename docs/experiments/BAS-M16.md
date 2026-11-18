# BAS-M16

**BAS-M16 — B.11.1 1T pool gauntlet: the head against the four targets, Rarog 2.6.0 beside it**

- Date / owner / calibration category: registered 2026-11-17, before any
  game; maintainer-run, agent-read; measurement.
- Candidate: the head (`4c0bc0e`, bench 4,777,412, `31BC229A…`).
- Field, pinned by SHA-256, priced at their Super Rating Tournament 1T
  ratings (BAS-M08):
  - Houdini 3 (`B0E7A631…`) 3287;
  - Critter 1.6a (`A8988AC9…`) 3192, `OwnBook=false`,
    `Tablebase Usage=Disable`;
  - Fritz 16 (`264F2EED…`) 3173 and Rybka 4.1 (`15901A12…`) 3111, both
    `Max CPUs=1`; both now under `D:\chess\engines\rybka\`, the same binaries
    as BAS-M11.
  - Rarog 2.6.0 (`rarog-v2.6.0-windows-pext-pgo.exe`, `274EAB97…`), Rarog's
    latest release, has no rating on this scale. It plays a separate
    400-game match beside the gauntlet and stays outside the verdict, as
    BAS-M11 kept Rarog 2.5.0.
- Research question: the head's 1T performance against the targets at the
  end of the search programme (1.10.0 rates 2994 on this scale; BAS-M15 read
  the head after Clusters 1 to 3 at +314 over 1.10.2 in a different pool),
  and where it stands against Rarog's release.
- Also read here: B.9's closing criterion, no crash, illegal move or forfeit
  by Basilisk (one reopens B.9.1), and the conversion audit
  (`tools/diag/conversion_audit.py`) on these games.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Performance against the four fixed ratings: **3380**, 80% band
  **3320 to 3440**. Basis: BAS-M15 implies about 3340 after Clusters 1 to 3
  (+148 against Critter 1.6a's 3192); the clusters since add about +55 to
  +65 in self-play, less against a field.
- Rarog 2.6.0 head-to-head: **−30**, 80% band −70 to +10 (BAS-M15: −73.2
  after Clusters 1 to 3).
- No Basilisk crash, illegal move or time forfeit: probability 0.9.
- Most likely failure mode: the old engines' draw rate compresses the scale
  above 3300, so the performance reads low against its self-play sum.
- Falsifiers: a performance under 3250 says the programme's gains did not
  reach the field and is checked against BAS-M15 before it is read; over
  3500 points at the anchors' pricing first.
- Registered design and stop rule: fixed runs, no extension, no re-run on
  the same binaries. The verdict is the maximum-likelihood performance
  against the four fixed ratings with a 95% interval; each head-to-head
  W-D-L, score and Elo is reported beside it. Opponent faults are capped at
  the wrapper's 1% of foreign games (16) and listed; a Basilisk fault voids
  nothing but opens a repair leaf.
- Full conditions:
  - Gauntlet: `python tools/run_b111_reads.py --read pool`; Colosseum CLI
    0.5.0, gauntlet, 200 cycles × 2 colour-reversed games, **400 games per
    opponent, 1,600 in all**, UHO random order, seed `111103`, `3+0.03`,
    margin 20 ms, 1T, Hash 64, concurrency 15 (auto, a core of headroom), no
    adjudication, no tablebases.
  - Rarog: `python tools/run_b111_reads.py --read rarog`; fixed match,
    **400 games** in 200 pairs, seed `111104`, the same conditions, Rarog as
    `-ExternalB`.
  - Dry runs clean at `62e7643`.

#### RESULT — append after exposure

- **Gauntlet.** Run 2026-11-17, 15:00–15:23, by the maintainer on an idle
  host, at `769ade8` (`tools/results/b111-pool/`). 1,600 games, no faults
  and no time losses on either side; the census agrees.
  - Performance **3416.3**, 95% interval [3395.5, 3437.1]; the head scored
    1249.5/1600 (1104-291-205).
  - Inside the 80% band (3320 to 3440), 36 above the point.

  | Opponent (rating) | W-D-L | Score | Elo, 95% interval | Implied |
  |---|---|---:|---|---:|
  | Houdini 3 (3287) | 253-68-79 | 71.8% | +161.9 [+131.4, +194.9] | 3449 |
  | Critter 1.6a (3192) | 269-76-55 | 76.8% | +207.5 [+177.8, +240.1] | 3400 |
  | Fritz 16 (3173) | 281-76-43 | 79.8% | +238.1 [+208.4, +271.3] | 3411 |
  | Rybka 4.1 (3111) | 301-71-28 | 84.1% | +289.7 [+255.1, +329.7] | 3401 |

  Against Critter 1.6a the head was −198 on 1.10.0 and +148 after
  Clusters 1 to 3 (BAS-M15); it is +207.5 now. All four rating gaps that
  PLAN §1's budget sized are closed, Houdini 3's (293) included.
- **Rarog 2.6.0.** Run 15:23–15:27 (`tools/results/b111-rarog/`), 400 games,
  no faults, no time losses; the pentanomial recount agrees.
  - The head 103-153-144, 44.9%, Elo **−35.7 ± 23.2**; pentanomial
    [15, 63, 78, 36, 8], nElo −52.7 ± 34.0.
  - Inside the band (−70 to +10), 5.7 below the point. BAS-M15 read −73.2
    after Clusters 1 to 3.
- **B.9's criterion.** No Basilisk crash, illegal move or forfeit, given
  0.9. Held: zero Basilisk faults and zero time losses in this read's 2,000
  games, and none in BAS-O06's or O07's 6,000.
- **Deviation.** The registration named Rarog 2.6.0 "Rarog's latest
  release". It was not: 2.7.0 was published 2026-11-13, four days before
  this entry, and the preparation did not check. The 2.6.0 match stands as
  run, and it is the one comparable with BAS-M15. The maintainer added
  2.7.0 beside it (2026-11-17), registered separately as BAS-M17.
- **Conversion audit** (B.11.1's close, 2026-11-18;
  `tools/diag/conversion_audit.py`, BAS-M08's settings: 12 persistent plies,
  3 material units, lone minors excluded; `tools/results/b111-checkpoint/conversion/`):
  - The gauntlet, 1,600 games: **17 draws and 1 loss** after a persistent
    piece-up advantage, **10.6 / 0.6 per 1,000** (11.3 thrown). 1.10.0
    read 16.67 / 6.67 (23.33) in BAS-M08 against six HCE-era anchors.
    - Against Critter 1.6a and Fritz 16, the two anchors the reads share,
      the head threw 6 draws and no loss in 800 games.
  - Against Rarog 2.6.0, 400 games: 4 draws and 4 losses, 10.0 / 10.0 per
    1,000.
  - The head also saved 20 games and won 13 from a persistent deficit in
    the gauntlet.
- Disposition: COMPLETE for the gauntlet and the 2.6.0 match.
