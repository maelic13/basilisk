# BAS-M30

**BAS-M30 — "Frontier HCE" on the Snapdragon X Plus at `3+0.03`: the head after C.3's rebuild with Rarog's head, Stockfish 16 HCE and Stockfish 11, all native Windows ARM64; a 4-engine round robin, 2,400 games**

- Date / owner / calibration category: run 2026-11-26 18:49 to 19:39 UTC
  (50 min, about 2,880 games an hour), maintainer-run in the Colosseum
  GUI (1.2.1), finished, agent-noted 2026-11-26; measurement,
  observation. Not registered before exposure.
- Why: the third host for the field of BAS-M28 and BAS-M29, played at
  the same time as BAS-M29 and meant as its twin.
- Host: Microsoft Surface Laptop, 7th Edition, Snapdragon X Plus
  X1P-64-100 (10 Oryon cores of one kind), 16 GB, Windows 11 Pro build
  26300. At concurrency 8 every game ran on the same kind of core,
  unlike BAS-M29's M4.
- **Engines, all native Windows ARM64** (the PE header's machine field is
  `0xAA64` in each file: no x64 under emulation and no ARM64EC):

  | Engine | Build | Rating | SHA-256 |
  |---|---|---|---|
  | Basilisk `dev-20261126-41d952f2` | the maintainer's native build of `master` at `41d952f` | start 3578, followed | `6CDC6186…` |
  | Rarog `dev-20261126-0ffb85bd` | the maintainer's native build of Rarog's head | start 3502, followed | `A07DCE00…` |
  | Stockfish 16, `Use NNUE=false` | `stockfish-16-arm64-oryon-classical-pgo.exe` | 3536, fixed | `689E4EEE…` |
  | Stockfish 11 | `stockfish_11-arm64-oryon-pgo.exe` | 3481, fixed | `E260F82E…` |

  - Basilisk: `bench 13` = `3,880,333` on the installed file when this
    was noted, GUIDE's fingerprint. Its build recipe is not recorded with
    the binary.
  - Rarog: `bench` = `3,694,298` on the installed file, the count
    BAS-M29's Mac build gives: the same Rarog head.
  - The two Stockfish ratings are this host's library ratings, last
    written by its own earlier runs; the Stockfish build recipes are not
    recorded here.
- Conditions, from the tournament's stored configuration:
  - a round robin ×200, 2 games per opening, colour-reversed: 400 games
    per pairing, 1,200 per engine;
  - `3+0.03`, 1 thread, Hash 128, concurrency 8, no ponder, no
    tablebases, no adjudication, no move cap; Move Overhead 10 for the
    Stockfishes, the default (10) for Basilisk and Rarog;
  - **`SuperGM_4mvs.pgn`**, 8 plies, random order, seed `202611261947`.
    The seed is BAS-M29's; the book is not. BAS-M29 records UHO
    `UHO_Lichess_4852_v1.epd`, read from the Mac's database, so the two
    runs did not play the same openings;
  - ratings followed for the Basilisk head and Rarog's head, both
    starting from BAS-M28's Ryzen ratings; the two Stockfishes fixed.
- **Source of the numbers: the GUI's database, read in place
  (read-only).** The counts reproduce the maintainer's screenshot of the
  standings and head-to-head screen exactly. Head-to-head Elo is
  logistic with a 95% interval from the 200 opening pairs of each
  pairing (pentanomial); every game sits in a complete pair.

#### RESULT — appended after exposure

| # | Engine | Elo | Perf | Points | W-D-L | NPS | Depth |
|---|---|---|---|---|---|---|---|
| 1 | Stockfish 16 HCE | 3536 fixed | 3538 | 680.0 | 381-598-221 | 2.34M | 23.3 |
| 2 | **Basilisk `dev-20261126-41d952f2`** | **3534** | 3533 | 671.0 | 385-572-243 | 1.90M | 19.4 |
| 3 | Stockfish 11 | 3481 fixed | 3480 | 549.0 | 279-540-381 | 2.37M | 20.0 |
| 4 | Rarog `dev-20261126-0ffb85bd` | 3458 | 3458 | 500.0 | 228-544-428 | 1.94M | 15.9 |

- Every game ended naturally: 1,273 checkmates, 686 threefold
  repetitions, 313 insufficient material, 112 fifty-move draws and 16
  stalemates. No forfeits. Draws 47.0% (BAS-M29: 36.4%); White scored
  55.0%. Every engine used 71–72 ms a move.
- Head to head, from the Basilisk head's side, 400 games each:

  | Opponent | W-D-L | Score | Snapdragon | BAS-M29, Mac | BAS-M28, Ryzen |
  |---|---|---|---|---|---|
  | Stockfish 16 HCE | 92-213-95 | 49.6% | −2.6 [−24.8, +19.5] | −13.9 [−36.1, +8.2] | −25.2 [−46.4, −4.1] |
  | Stockfish 11 | 156-160-84 | 59.0% | +63.2 [+37.1, +90.1] | +43.7 [+20.4, +67.3] | +28.0 [+6.2, +50.0] |
  | Rarog's head | 137-199-64 | 59.1% | +64.1 [+41.9, +86.9] | +75.0 [+51.8, +98.8] | +77.2 (`dev-20261126-0231c3ed`) |

- Among the others: Stockfish 16 HCE against Stockfish 11 +50.7
  [+28.8, +73.0] (Mac +44.5); against Rarog's head +87.8 [+61.8, +114.8]
  (Mac +104.7). Stockfish 11 against Rarog's head +24.4 [−0.2, +49.2]
  (Mac +31.4).

#### Reading

- **The order holds on a third host.** Stockfish 16 HCE, the head,
  Stockfish 11, Rarog's head, as on the Mac; the head is 2 below
  Stockfish 16 HCE here and level with it head to head (−2.6
  [−24.8, +19.5]).
- **The −44 is a change of scale again.** Both followed engines started
  at their Ryzen ratings; this host's library sits 69 lower at Stockfish
  16 HCE and 51 lower at Stockfish 11 than BAS-M28's. Both anchors
  perform within 2 of their fixed ratings, so the scale held. The head's
  3534 equals its Mac rating by coincidence: the two scales differ by 9
  at Stockfish 16 HCE.
- **The gain carried to this host.** In this host's "HCE Frontier
  (ARM)" run of 2026-11-20 (UHO, stopped), Basilisk 1.11.0 scored −182.2
  against Stockfish 16 HCE and −162.2 against Stockfish 11 over 264 games
  each; the head reads −2.6 and +63.2. Different fields and books, so
  this is a size, not a measurement.
- **The three hosts rank the head against the Stockfishes in the order
  of its relative speed.** The head's NPS as a share of Stockfish 16
  HCE's is 0.70 on the Ryzen, 0.77 on the Mac and 0.81 here, and its
  score against it −25.2, −13.9, −2.6; against Stockfish 11 the shares
  are 0.70, 0.73, 0.80 and the scores +28.0, +43.7, +63.2. Rarog's head
  moves the same way between the Mac and here (0.75 to 0.83 of Stockfish
  16 HCE's NPS, −104.7 to −87.8). Not measured as the cause: no step is
  significant on its own (Ryzen to Snapdragon against Stockfish 16 HCE
  is 23 Elo at about 1.4 standard errors), the fields differ, and this
  host played another book.
- **The pair against Stockfish 16 HCE is not portable.** BAS-M27's
  −39.8 [−48.0, −31.6] (Ryzen, UHO, 3,000 games) lies about three
  standard errors from this run's −2.6. Host and book changed together,
  so the two cannot be separated here; BAS-M27 stays the meter's value
  for its own conditions.
- Limits:
  - the book differs from BAS-M28's and BAS-M29's, so the comparison
    across hosts carries a book confound as well as a host one;
  - four engines and 400 games a pairing: every head to head is about
    ±23;
  - the GUI's ratings carry no interval, and the two followed ratings
    are fitted against only two fixed anchors.
- Nothing here changes a PLAN step. C.8 still owes the registered
  re-read of BAS-M18's full field.

- Evidence (not committed): `tools/results/bas-m30/` holds `games.pgn`
  (2,400 games exported from the GUI database's stored PGN, SHA-256
  `5C81C083…`), `h2h.py` and its output `h2h.txt`. The games stay in the
  GUI's database as the tournament created 2026-11-26 18:49 UTC.
- Disposition: OBSERVATION, complete at 2,400 games. All four engines
  native Windows ARM64. The head rates 3534, second of four, 2 below
  Stockfish 16 HCE (−2.6 [−24.8, +19.5] head to head) and above
  Stockfish 11 (+63.2) and Rarog's head (+64.1); the order of BAS-M28 and
  BAS-M29 holds; played on `SuperGM_4mvs.pgn`, not UHO; no forfeits.
