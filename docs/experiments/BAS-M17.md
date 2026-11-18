# BAS-M17

**BAS-M17 — B.11.1 head-to-head against Rarog 2.7.0, Rarog's latest release**

- Date / owner / calibration category: registered 2026-11-17, before any
  game; maintainer-run, agent-read; measurement.
- Why: BAS-M16 named Rarog 2.6.0 Rarog's latest release, but 2.7.0 had been
  published four days earlier (2026-11-13). The maintainer's rule
  (2026-11-17) is that every Rarog comparison includes the latest release,
  beside any older version kept for continuity. The 2.6.0 match stands; this
  one is added beside it.
- Candidate: the head (`4c0bc0e`, bench 4,777,412, `31BC229A…`), the binary
  BAS-M16 played.
- Opponent: Rarog 2.7.0, the official asset `rarog-2.7.0-windows-x64.exe`
  from `maelic13/rarog` release `v2.7.0` (tag `3c57a899`), SHA-256
  `C3BB0371…`, as GitHub publishes it.
  - It is played at its defaults, as a user gets it. It is one universal x64
    file that picks its code path at start, where 2.6.0 was a `pext-pgo`
    flavour.
  - Its `LargePages` option defaults on where Windows allows it, and it is
    left at that default.
  - Its changelog promises search, clock and protocol fixes at about
    2.6.0's strength, "slightly stronger head to head".
- Research question: where the search programme's head stands against
  Rarog's current release.
- **Build check before any game** (2026-11-17, maintainer's request, idle
  host at 1.1% load, `tools/results/b111-rarog27-speed/`):
  - The maintainer built `rarog-2.7.0-windows-x64-pext.exe` (`9DAD2C99…`)
    locally from `v2.7.0`, to check that the universal download is as fast
    as the best local build.
  - Both report `id name Rarog 2.7.0` and the same `bench 13` fingerprint,
    9,077,016 nodes.
  - Twelve interleaved cycles of `bench 13` were run, one process each,
    pinned to CPU 30 from a suspended start, at High priority, with large
    pages on for both. The local pext build read **−0.17%**, interval
    [−0.27, −0.08], against the universal asset: 2.337M against 2.343M
    medians.
  - With one build each, that is within build-to-build variation; the
    universal asset gives away nothing. So the match keeps the official
    asset as registered, and the prediction is unchanged.
  - Basilisk also takes large pages, so Rarog's default `LargePages=true`
    stays.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- The head against Rarog 2.7.0: **−40**, 80% band **−80 to 0**. Basis:
  BAS-M16 read −35.7 ± 23.2 against 2.6.0, and 2.7.0's changelog claims a
  little more head-to-head strength. Most of the band is this match's own
  noise (400 games, about ±23 at 95%).
- No Basilisk crash, illegal move or time forfeit: probability 0.95, after
  BAS-M16's 2,000 clean games.
- Most likely failure mode: 2.7.0's reworked time management uses
  `3+0.03` differently from 2.6.0, which moves the result by more than the
  changelog's "slightly".
- Falsifiers: beyond the band in either direction, compare the two Rarog
  matches' clock use per move before reading a strength change. A Rarog
  fault or time loss is listed and read as the opponent's.
- Registered design and stop rule: one fixed run, no extension, no re-run on
  the same binaries. It is reported as W-D-L, score and Elo with a 95%
  interval and the pentanomial, beside BAS-M16's 2.6.0 match. It is outside
  the pool verdict.
- Full conditions: `python tools/run_b111_reads.py --read rarog27`;
  Colosseum CLI 0.5.0, fixed match, **400 games** in 200 colour-reversed
  pairs, UHO `UHO_Lichess_4852_v1.epd` in random order, seed `111105`,
  `3+0.03`, margin 20 ms, 1T, Hash 64, no adjudication, no tablebases,
  Rarog as `-ExternalB`, pinned by its SHA-256.
  - The conditions are BAS-M16's Rarog match with only the opponent and
    the seed changed.
  - The launcher refuses until the asset is at
    `D:\chess\engines\rarog\rarog-2.7.0-windows-x64.exe` with that hash.
    Once the asset is there, a dry run comes first.

#### RESULT — append after exposure

- Run 2026-11-17, 15:50–15:55, by the maintainer
  (`tools/results/b111-rarog27/`). 400 games, no faults or time losses on
  either side; the pentanomial recount agrees.
- The head **111-143-146**, 45.6%, Elo **−30.5 ± 25.5**; pentanomial
  [26, 45, 75, 46, 8], nElo −40.9 ± 34.0.
- Calibration: inside the 80% band (−80 to 0), 9.5 above the point.
  Against 2.6.0 the head read −35.7 ± 23.2 (BAS-M16); the two Rarog
  releases are indistinguishable here, as 2.7.0's changelog says.
- Clock use: 2.7.0 used 67.5 ms a move, where 2.6.0 used 70.7 ms in
  BAS-M16, so its reworked time management spends about 4.5% less of
  `3+0.03`. It reads 1.92M nodes per second in play, where 2.6.0 read
  2.43M; the field did not fire, and no strength change is read from it.
- B.9's criterion holds over these 400 games too.
- Disposition: COMPLETE. This is the checkpoint's comparison with Rarog's
  latest release.
