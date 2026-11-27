# BAS-M29

**BAS-M29 — "Frontier HCE" on the MacBook Air M4 at `3+0.03`: the head after C.3's rebuild with Rarog's head, Stockfish 16 HCE and Stockfish 11, all native arm64; a 4-engine round robin, 2,400 games**

- Date / owner / calibration category: run 2026-11-26 18:49 to 19:37 UTC
  (48 min, about 2,970 games an hour), maintainer-run in the Colosseum
  GUI (1.2.1), finished, agent-noted 2026-11-26; measurement,
  observation. Not registered before exposure.
- Why: the Mac counterpart of BAS-M28: where the head after C.3.5's flip
  stands against the two classical Stockfish anchors and Rarog's head on
  the second host.
- Host: MacBook Air, Apple M4 (4 performance and 6 efficiency cores,
  fanless), macOS 27.2. At concurrency 8, games ran on both kinds of
  core, as in BAS-M21; the per-game speed variation falls on every engine
  alike in expectation and adds only noise.
- **Engines, all native arm64** (`file`: Mach-O 64-bit executable
  arm64; none under Rosetta):

  | Engine | Build | Rating | SHA-256 |
  |---|---|---|---|
  | Basilisk `dev-20261126-41d952f2` | the maintainer's native build of `master` at `41d952f` | start 3578, followed | `1E2A9C5A…` |
  | Rarog `dev-20261126-0ffb85bd` | the maintainer's native build of Rarog's head | start 3502, followed | `0FD70795…` |
  | Stockfish 16, `Use NNUE=false` | LLVM 23 PGO, BAS-M21's binary | 3545, fixed | `271C43C8…` |
  | Stockfish 11 | native PGO, BAS-M21's binary | 3497, fixed | `B81F133B…` |

  - Basilisk: `bench 13` = `3,880,333` on the installed file when this
    was noted, GUIDE's fingerprint. Its build recipe is not recorded with
    the binary.
  - Rarog: `bench` = `3,694,298` on the installed file. This is a later
    Rarog head than BAS-M28's `dev-20261126-0231c3ed`. Not inspected
    further.
  - The two Stockfish hashes equal BAS-M21's: the same binaries, and the
    Mac library's ratings for them are BAS-M21's fit (3544 and 3497,
    fixed).
- Conditions, from the tournament's stored configuration:
  - a round robin ×200, 2 games per opening, colour-reversed: 400 games
    per pairing, 1,200 per engine;
  - `3+0.03`, 1 thread, Hash 128, concurrency 8, no ponder, no
    tablebases, no adjudication, no move cap; Move Overhead 10 for the
    Stockfishes, the default (10) for Basilisk and Rarog;
  - UHO `UHO_Lichess_4852_v1.epd`, random order, seed `202611261947`;
  - ratings followed for the Basilisk head and Rarog's head, both
    starting from BAS-M28's Ryzen ratings; the two Stockfishes fixed at
    their Mac library ratings.
- **Source of the numbers: the GUI's database, read in place
  (read-only).** The counts reproduce the maintainer's screenshot of the
  standings exactly. Head-to-head Elo is logistic with a 95% interval from
  the 200 opening pairs of each pairing (pentanomial); every game sits in
  a complete pair.

#### RESULT — appended after exposure

| # | Engine | Elo | Perf | Points | W-D-L | NPS | Depth |
|---|---|---|---|---|---|---|---|
| 1 | Stockfish 16 HCE | 3545 fixed | 3550 | 692.0 | 470-444-286 | 2.30M | 24.5 |
| 2 | **Basilisk `dev-20261126-41d952f2`** | **3534** | 3534 | 659.5 | 430-459-311 | 1.76M | 19.3 |
| 3 | Stockfish 11 | 3497 fixed | 3493 | 567.5 | 353-429-418 | 2.41M | 21.0 |
| 4 | Rarog `dev-20261126-0ffb85bd` | 3456 | 3455 | 481.0 | 274-414-512 | 1.73M | 15.4 |

- Every game ended naturally: 1,527 checkmates, 513 threefold
  repetitions, 244 insufficient material, 97 fifty-move draws and 19
  stalemates. No forfeits. Draws 36.4%; White scored 54.6%. Every engine
  used 72–74 ms a move.
- Head to head, from the Basilisk head's side, 400 games each:

  | Opponent | W-D-L | Score | Mac | BAS-M28, Ryzen |
  |---|---|---|---|---|
  | Stockfish 16 HCE | 114-156-130 | 48.0% | −13.9 [−36.1, +8.2] | −25.2 [−46.4, −4.1] |
  | Stockfish 11 | 147-156-97 | 56.2% | +43.7 [+20.4, +67.3] | +28.0 [+6.2, +50.0] |
  | Rarog's head | 169-147-84 | 60.6% | +75.0 [+51.8, +98.8] | +77.2 (`dev-20261126-0231c3ed`) |

- Among the others: Stockfish 16 HCE against Stockfish 11 +44.5
  [+22.9, +66.5]; against Rarog's head +104.7 [+80.5, +129.8]. Stockfish
  11 against Rarog's head +31.4 [+7.3, +55.7].

#### Reading

- **The Ryzen result reproduces on the Mac.** The head is second, 11
  below Stockfish 16 HCE and −13.9 [−36.1, +8.2] against it head to
  head, past Stockfish 11 (+43.7) and Rarog's head (+75.0). Every head
  to head agrees with BAS-M28 inside its interval.
- **The −44 is a change of scale, not of strength.** The head started at
  its Ryzen rating, 3578; the Mac library sits 60 lower at Stockfish 16
  HCE (3605 against 3545) and 35 lower at Stockfish 11. Against the
  anchors the head is 11 below Stockfish 16 HCE here and 27 below on the
  Ryzen, 37 above Stockfish 11 here and 46 on the Ryzen. Rarog's −46 is
  the same offset. Both anchors perform within 5 of their fixed ratings,
  so the Mac scale held.
- **The gain carried to the second host.** On the Mac, Basilisk 1.11.0
  stood 203 below Stockfish 16 HCE (BAS-M21, −172.0 head to head over
  120 games); the head stands 11 below. Different fields, so this is a
  size, not a measurement.
- **The pair against Stockfish 16 HCE reads closer than the meter.**
  BAS-M27's −39.8 [−48.0, −31.6] (Ryzen, 3,000 games) lies just outside
  this interval: the difference is 26 Elo at about two standard errors.
  One candidate is speed: the head's NPS is 0.77 of Stockfish 16 HCE's
  here against 0.70 in BAS-M28 on the Ryzen. That is not measured as
  the cause, and BAS-M27 stays the meter's value.
- **Same shape as the Ryzen: lower speed, middling depth.** The head
  searches to 19.3 at 1.76M nps; Stockfish 16 HCE to 24.5 at 2.30M,
  Stockfish 11 to 21.0 at 2.41M. Rarog's head reaches 15.4 at 1.73M. No
  Elo is attributed to either figure.
- Limits:
  - four engines and 400 games a pairing: every head to head is about
    ±23;
  - the GUI's ratings carry no interval, and the two followed ratings
    are fitted against only two fixed anchors;
  - concurrency 8 on four performance cores of a fanless machine, as in
    BAS-M21.
- Nothing here changes a PLAN step. C.8 still owes the registered
  re-read of BAS-M18's full field.

- Evidence (not committed): `tools/results/bas-m29/` holds `games.pgn`
  (2,400 games exported from the GUI database's stored PGN, SHA-256
  `8664C30E…`), `h2h.py` and its output `h2h.txt`. The games stay in the
  GUI's database as the tournament created 2026-11-26 18:49 UTC.
- Disposition: OBSERVATION, complete at 2,400 games. All four engines
  native arm64. The head rates 3534, second of four, 11 below Stockfish
  16 HCE (−13.9 [−36.1, +8.2] head to head) and above Stockfish 11 (+43.7)
  and Rarog's head (+75.0); BAS-M28's Ryzen reading reproduces; no
  forfeits.
