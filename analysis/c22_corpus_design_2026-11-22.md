# C.2.2 corpus design (2026-11-22)

- State / class: research in progress, `R2`. The design below is complete
  except the two maintainer decisions in §7 and BAS-O10's result, which
  §5's off-line share waits on.
- Owner / date: agent, 2026-11-22, PLAN C.2.2.
- Decision needed: the maintainer's, on §7's two items. Corpus A (C.2.3) is
  generated from this packet's registration, not before.

**Recommendation.** Size corpus A in games, not rows: about 1,000,000
self-play games from pool starts, two rows per phase per game. Rows from
one game share their result and correlate (ICC 0.69), so past a handful of
rows a game the information stops growing. About 1.4 effective independent
rows a game is the ceiling, and 1,000,000 games at 8,000 nodes are about
five hours of datagen. The main counter-argument is cost per fit: about 6M
rows instead of 3.5M, which sparse rows now make cheap.

Every claim is evidence (a file read, a program run) unless marked
inference.

## 1. The question

What corpus does C.2.3 generate, so that C.2.8's whole-surface refit is
fitted on the right positions, enough of them, and comparable across arms?
PLAN's items: the source store located and hashed; buckets by evaluator
phase; one extractor contract for every arm; the size from effective
independent rows and a learning curve; the composition and budget
registered before launch; the diverse book. The Rarog audit adds C-8, C-26,
C-27, C-74, C-75 and C-77; C.0 adds H-distribution, the gate games as
validation, and where the deficit lies by phase.

## 2. The source store

The Beast pool was read once, sequentially and read-only, with the
maintainer's leave on 2026-11-22, by `tools/texel/audit_store.py`. It took
749 s. The report is `tools/results/c22/beast_pool_audit.json`.

| Quantity | Value |
|---|---|
| Path | `A:/Chess/Beast/data/txt/positions.txt` |
| Bytes | 7,121,976,716 |
| SHA-256 | `B48E1EC1696B46FF0079C1E77A9F4CC7D8EF246397701877146FB52976AF9E88` |
| Lines, all of them positions | 122,656,978 (none malformed) |
| Distinct on the four position fields | 120,084,027 (2.10% duplicates) |
| Opening (phase 20–24) | 45,101,668 (36.8%) |
| Early middlegame (14–19) | 26,223,808 (21.4%) |
| Middlegame (8–13) | 25,171,622 (20.5%) |
| Endgame (3–7) | 21,357,503 (17.4%) |
| Deep endgame (0–2) | 4,802,377 (3.9%) |

A million-game book needs under 2.5% of the pool's distinct opening
positions. The scarcest bucket a book draws on, the deep endgame, still has
4.8M positions. The pool does not bind any design here. Its FENs carry no
source-game ids, so related starts cannot be proved independent (archived
8.2.a); `audit_starts.py` reports what can be measured.

## 3. Repairs (`040dbea`)

- **C-8.** The de-duplication key now holds the rule-50 clock: the played
  score reads it through the damping, so two rows that differ only in the
  clock are two different labelled scores. The split still keys on the
  position alone. Two tests pin both.
- **C-26.** `tools/datagen.ps1` has no default book and refuses to run
  without `-Book`. The old default, `SuperGM_4mvs.pgn`, once collapsed 200k
  games into 31,880 positions.
- **C-27.** The book sampler takes the extractor's phase rule
  (`extract.placement_phase`), one definition, and a test checks that both
  agree on 200 random positions. Its start-phase mix is now
  `--phase-weights`, which the corpus registers; equal stays the default.
- **A store audit**, `tools/texel/audit_store.py`: one sequential pass with
  the hash, the phase mix and the exact duplicates.

## 4. Measured: yield and dependence

**Yield per start phase (C-75).** `tools/results/c22/yield.py` runs the row
contract's per-game sampling (`extract.game_rows`) over
`armA_basilisk8k.pgn`: 125,000 games, 1.9.3 at 8,000 nodes, from
`beast_seed_2m.epd`. Rows per game, by the start's phase and the row's:

At 8 rows per phase per game (the old cap):

| Start | Games | Plies | Opening | Early mid | Middlegame | Endgame | Deep | All |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Opening | 48,344 | 119 | 5.67 | 5.05 | 4.87 | 5.17 | 2.65 | 23.4 |
| Early mid | 26,624 | 101 | 0 | 5.32 | 4.78 | 5.13 | 2.73 | 18.0 |
| Middlegame | 24,738 | 99 | 0 | 0.02 | 6.07 | 6.02 | 3.24 | 15.3 |
| Endgame | 20,109 | 92 | 0 | 0 | 0.33 | 7.47 | 3.79 | 11.6 |
| Deep endgame | 4,682 | 80 | 0 | 0 | 0.39 | 3.06 | 7.67 | 11.1 |

At 2 rows per phase per game:

| Start | Opening | Early mid | Middlegame | Endgame | Deep | All |
|---|---:|---:|---:|---:|---:|---:|
| Opening | 1.74 | 1.51 | 1.43 | 1.39 | 0.73 | 6.80 |
| Early mid | 0 | 1.66 | 1.41 | 1.39 | 0.75 | 5.21 |
| Middlegame | 0 | 0.01 | 1.78 | 1.59 | 0.89 | 4.27 |
| Endgame | 0 | 0 | 0.14 | 1.95 | 1.04 | 3.13 |
| Deep endgame | 0 | 0 | 0.15 | 0.91 | 1.98 | 3.04 |

- Opening rows come from opening starts alone, and an opening start gives
  rows in every later phase. A deep-endgame row is the scarcest from an
  opening start.
- No game of the 125,000 failed the rules-ending check; 503 gave no
  candidate position.

**Dependence within a game.** On the 100,000-row `armA` corpus of C.2.1.2,
the residuals of the seed's full evaluation at K 1.714, grouped by game,
have an intraclass correlation of **0.69**. Every row of a game shares its
result. The design effect `1 + (m − 1) × 0.69` for `m` rows a game:

| Rows a game | Design effect | Effective independent rows a game |
|---:|---:|---:|
| 1 | 1.0 | 1.00 |
| 3 | 2.4 | 1.26 |
| 6.8 (cap 2) | 5.0 | 1.36 |
| 23.4 (cap 8) | 16.4 | 1.43 |
| unlimited | — | 1.45 (= 1 / 0.69) |

- Inference: a corpus's information is set by its games. A 3.5M-row
  corpus at the old cap, about 150,000 games, holds about 210,000
  effective rows; 1,000,000 games at cap 2 hold about 1.36M.
- Inference: the ICC is read at the seed's evaluation. A refit lowers the
  residuals' common part only if it explains game-level outcome, so the
  ceiling should hold to the first digit.

## 5. The design

- **Source.** The Beast pool (§2), sampled by `sample_fens.py` into a book
  of distinct starts, disjoint from every earlier book's (`--exclude-pgn`).
- **Book mix**, by the start's phase: 70% opening, 10% early middlegame,
  10% middlegame, 5% endgame, 5% deep endgame. Mostly opening starts, so
  the games are whole games; a fifth later starts, for independent endings
  (archived 8.2.b).
- **Off-line starts.** If BAS-O10 reads SUPPORTED, 20% of the book's starts
  are one random legal move off a sampled pool position, and the game
  played from there labels the position a search would visit. If not,
  none.
- **Games.** About 1,000,000 at 8,000 nodes, one thread, Hash 16, natural
  terminations only, one game an opening. The engine is the head
  (C.2.3: the B.11.1 head; the C.1 head plays as 1.11.0 does, BAS-M23).
  At the 1.9.3 manifest's 3,450 games a minute, that is about five hours;
  C.2.3's pilot re-measures it on the head.
- **Extraction**, `basilisk-texel-rows-v1`, one contract for every arm:
  - 2 rows per phase per game, quiet filter on, `--skip-end 6`;
  - validation and test 5% each, by the start's hash;
  - output phase weights per §7.
- **Expected rows.** With the book mix above, cap 2 gives per game about
  1.22 opening, 1.22 early-middlegame, 1.32 middlegame, 1.41 endgame and
  0.83 deep-endgame rows. That is about 6M rows a million games, before
  the quotas.
- **Validation sets.**
  - The corpus's own validation split.
  - Inference: the gate games (C.0: 25,970 games, 254,486 rows, at the
    deployment clock) as a second held-out set, extracted under the same
    contract. They are played positions at the clock that decides, not at
    fixed nodes. Every fit report adds the loss on them by band.
- **Size: a learning curve, registered (inference: the rule).** C.2.8
  fits nested subsets of corpus A's train split by game: an eighth, a
  quarter, a half and all of it, each from the same seed, K and schedule.
  It reads validation loss by band.
  - The size used is the smallest whose next doubling improves validation
    loss by under 5% of the seed-to-full improvement.
  - If the full corpus's last doubling still improves by more, corpus A is
    extended before the fit is accepted.
- **Transient material (C-77).** Corpus A keeps the quiet filter's rows.
  Its residual on rows with a capture of equal or greater value pending,
  which the SEE proxy lets through, is read in C.2.8's fit report as its
  own line. A quiescence-settled arm is a later question, not this
  corpus's.
- **Extractor priors (C-74)** stand: `--skip-end 6`, the quiet filter, and
  no ply-based skip at the start.

## 6. What the design does not decide

- The labels: corpus B, tablebase-relabelled, is C.2.4's.
- The start of the PST seeds: C.2.5's.
- The pilot, its fault checks and the realised opening sequence: C.2.3's
  (C-32, C-76).

## 7. Decisions for the maintainer

1. **Output phase weights.**
   - **Equal (1,1,1,1,1), my recommendation**: every phase's terms keep
     their support, and the eg terms need the endings.
   - **Tilted to the deficit (3,3,2,1,1).** C.0 put 68–69% of the
     same-search deficit at phase 18 to 24, and 48% of quiet game rows at
     phase 9 or below, where 16% of it is. The case for equal is that the
     loss already weights each row's error by how far its phase's terms
     are off; the tilt would buy middlegame fit with endgame support.
2. **The game budget.** About 1,000,000 games, five hours overnight, before
   the learning curve says whether more is needed.

## 8. Open: BAS-O10

Registered and instrumented (`7e3bfe3`), maintainer-run. Its verdict sets
§5's off-line share, and then C.2.2 closes.
