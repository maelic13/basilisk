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

- Registered, not yet run.
