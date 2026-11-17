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
  that are themselves contenders. Every engine plays 1 thread, Hash 64, with
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

  - The anchors span 3281–3532, the field's range bar its top.
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
  - Round robin of 14 engines, 200 cycles × 2 games: 400 games per pair in
    colour-reversed pairs, 91 pairs, **36,400 games**.
  - `3+0.03`, 1 thread, Hash 64, concurrency 14, no ponder.
  - No adjudication, no move cap, no tablebases.
  - UHO `UHO_Lichess_4852_v1.epd`, random order, seed `20261117`.
  - About 7.5 hours at the 4,824 games an hour of the Ethereal round robin.
  - PGN written to `tools/results/bas-m18/games.pgn` (not committed).
- Afterwards, at the maintainer's choice: one pairing is replayed under
  fastchess 1.8.0 at the same conditions, as PLAN B.11.5's harness
  cross-check and fastchess-GUI trial.

#### RESULT — append after exposure
