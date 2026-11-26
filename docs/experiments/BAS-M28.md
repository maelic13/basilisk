# BAS-M28

**BAS-M28 — "Frontier HCE" at `3+0.03`: the head after C.3's rebuild with Rarog's head, Stockfish 16 HCE, Stockfish 11, Rarog 2.7.0 and Basilisk 1.11.0; a 6-engine round robin stopped at 9,331 of 15,000 games**

- Date / owner / calibration category: created 2026-11-26 14:40, 1 h 43 min
  of play, maintainer-run in the Colosseum GUI (1.2.1) on the Ryzen 9
  5950X, stopped by the maintainer at 9,331 of 15,000 games, agent-noted
  2026-11-26; measurement, observation. Not registered before exposure.
- Why: the maintainer's dev test of the head after C.3.5's flip in a
  short frontier field, with Rarog's head of the same day, the two
  projects' latest releases and the two classical Stockfish anchors.
- Engines:
  - Basilisk `dev-20261126-41d952f2`: `master` at `41d952f`.
    `D:\chess\engines\basilisk\basilisk-dev-20261126-41d952f2-windows-x64-pext.exe`,
    SHA-256
    `C65EE8CE469F83411E6782FC71F39BE3457155B6F2AF523769BEF68322A4E813`,
    `bench 13` = `3,880,333`, run on that file when this was noted:
    GUIDE's fingerprint.
  - Rarog `dev-20261126-0231c3ed`: Rarog's own build, as registered in
    the GUI. Not inspected.
  - Stockfish 16 HCE, Stockfish 11, Rarog 2.7.0 and Basilisk 1.11.0: the
    GUI's library entries, as in BAS-M23.
- Conditions, as the GUI shows them:
  - a round robin ×500, 2 games per opening, 1,000 games per pairing
    planned and 622 played (623 in two pairings);
  - `3+0.03`, 1 thread, concurrency 14, no ponder, no tablebases, no
    adjudication, no move cap;
  - UHO `UHO_Lichess_4852_v1.epd`;
  - ratings followed for the Basilisk head and Rarog's head, the other
    four fixed at their library ratings.
- **Source of the numbers: the GUI's standings and head-to-head screen**,
  read from the maintainer's screenshot. No PGN and no database snapshot
  were read. The transcription was checked by script: every engine's
  head-to-head row sums to its W-D-L, each pairing mirrors, and the
  points and the terminations each sum to 9,331. The head-to-head Elo
  below is the logistic estimate from those counts with a trinomial 95%
  interval; the games are paired, so the interval is approximate.

#### RESULT — appended after exposure

| # | Engine | Elo | Perf | Points | Games | W-D-L | NPS | Depth |
|---|---|---|---|---|---|---|---|---|
| 1 | Stockfish 16 HCE | 3605 | 3597 | 1,989.5 | 3,110 | 1427-1125-558 | 2.0M | 24.4 |
| 2 | **Basilisk `dev-20261126-41d952f2`** | **3578** | 3578 | 1,889.5 | 3,110 | 1327-1125-658 | 1.4M | 18.3 |
| 3 | Stockfish 11 | 3532 | 3553 | 1,744.0 | 3,110 | 1213-1062-835 | 2.0M | 21.0 |
| 4 | Rarog `dev-20261126-0231c3ed` | 3502 | 3502 | 1,499.0 | 3,111 | 942-1114-1055 | 1.4M | 14.7 |
| 5 | Rarog 2.7.0 | 3445 | 3437 | 1,174.0 | 3,110 | 623-1102-1385 | 1.4M | 12.6 |
| 6 | **Basilisk 1.11.0** | **3415** | 3409 | 1,035.0 | 3,111 | 553-964-1594 | 1.5M | 18.6 |

- Terminations, all natural: 6,085 checkmates, 2,134 threefold
  repetitions, 791 insufficient material, 233 fifty-move draws and 88
  stalemates. No forfeits.
- Head to head, from the Basilisk head's side, 622 games each:

  | Opponent | W-D-L | Score | Elo |
  |---|---|---|---|
  | Stockfish 16 HCE | 163-251-208 | 46.4% | −25.2 [−46.4, −4.1] |
  | Stockfish 11 | 224-224-174 | 54.0% | +28.0 [+6.2, +50.0] |
  | Rarog `dev-20261126-0231c3ed` | 268-222-132 | 60.9% | +77.2 [+55.4, +99.7] |
  | Rarog 2.7.0 | 321-224-77 | 69.6% | +144.0 [+122.0, +167.1] |
  | Basilisk 1.11.0 | 351-204-67 | 72.8% | +171.3 [+148.5, +195.6] |

- Stockfish 16 HCE against Basilisk 1.11.0: 369-188-65, +185.7
  [+162.1, +210.9]. Against Rarog's head: 292-214-116, +101.1.
- Rarog's head against Rarog 2.7.0: 235-269-118, +66.1.

#### Reading

- **The field agrees with the frontier pair.** The head rates 27 below
  Stockfish 16 HCE and scores −25.2 [−46.4, −4.1] against it head to
  head. BAS-M27's registered −39.8 [−48.0, −31.6] over 3,000 games lies
  inside that interval and stays the meter's value.
- **The evaluation's gates carried to the field at about their size.**
  The head is 163 above 1.11.0 in the field and +171.3 head to head.
  Against Stockfish 16 HCE, 1.11.0 reads −185.7 here and the head −25.2:
  about 160 closed in one field, beside the 153 the pairs read (−193 in
  BAS-M18, −120.0 in BAS-M24, −39.8 in BAS-M27).
- **The order changed.** The head is past Stockfish 11 (+28.0
  [+6.2, +50.0]) and past Rarog's head (+77.2). In BAS-M23 Rarog's head
  of that day was 94 above Basilisk's.
- **Same depth, lower speed.** The head searches to 18.3 at 1.4M nps
  where 1.11.0 reaches 18.6 at 1.5M: the gain is the evaluation's, with
  the search frozen, as Phase C intends. Stockfish 16 HCE still reaches
  24.4 at 2.0M. No Elo is attributed to either figure.
- Limits:
  - stopped at 62% of its games, and the GUI's ratings carry no
    interval;
  - a field of six with two engines from each of two families, so a
    third of the head's games are against its own release or Rarog's;
  - the anchors do not sit on their fixed ratings here: Stockfish 11
    performs at 3553 against 3532 and Stockfish 16 HCE at 3597 against
    3605, so the field gap of 27 is uncertain by about that much.
- Nothing here changes a PLAN step. C.8 still owes the registered
  re-read of BAS-M18's full field; this short field does not stand in
  for it.

- Evidence (not committed): the maintainer's screenshot of the stopped
  tournament's standings; the games are in the GUI's database.
- Disposition: OBSERVATION, stopped at 9,331 of 15,000 games. The head
  rates 3578, second of six, 27 below Stockfish 16 HCE (−25.2 head to
  head, 622 games) and 163 above 1.11.0; it is past Stockfish 11 and
  Rarog's head; no forfeits.
