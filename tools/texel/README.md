# Texel tuning data pipeline

```
Beast FEN pool ──sample_fens.py──▶ <book>.epd ──datagen.ps1──▶ selfplay.pgn ──extract[_parallel].py──▶ train.csv + holdout.csv ──tuner──▶ baked weights
```

The labels are **self-play game results** (WDL, white-perspective 1.0/0.5/0.0),
not external engine scores. The Beast pool is used **only as diverse start
positions**; Basilisk plays them out to produce the labels.

## ⚠️ The diversity rule (read before generating data)

Self-play between two **identical** engines at a fixed node limit is
**deterministic** — a given start position always produces the same game. So
the number of *distinct* games is capped by the number of *distinct openings in
the book*, **not** by `-Rounds`. Running many rounds over a small book just
replays the same games.

- ❌ `SuperGM_4mvs.pgn` (~2.7k openings): 300k rounds → **1.5k distinct games**,
  ~31k unique positions. Useless for tuning. (`datagen.ps1` now warns on this.)
- ✅ `beast_seed.epd` (100k+ sampled FENs): each opening → a distinct game →
  millions of diverse positions.

To add variety beyond the book size, run extra passes at a **different
`-Nodes`** value (same opening + different node budget → a different
deterministic game). `extract_parallel.py` dedups by FEN across all passes.

## Commands

```powershell
# 0. Build the current head as the datagen engine.
./tools/build_test.ps1 -Suffix <head>

# 1. Sample a diverse EPD opening book from the Beast pool (read-only source).
#    ~17 train positions per opening at --max-per-game 24, so size the book to
#    target/17. Example for ~3M: ~200k openings.
python tools/texel/sample_fens.py "A:/Chess/Beast/data/txt/positions.txt" `
    --out tools/texel/data/<book>.epd --count 200000

# 2. Self-play from the book (DELETE/!rename the output first — datagen APPENDS).
./tools/datagen.ps1 -Suffix <head> -Rounds 200000 -Nodes 8000 `
    -Book tools/texel/data/<book>.epd -BookFormat epd `
    -OutputPgn tools/texel/data/<set>.pgn

# 3. Extract (parallel; reuses extract.py's per-game logic, dedups, splits
#    holdout by game). --max-per-game 24 ≈ doubles yield vs the default 12.
python tools/texel/extract_parallel.py tools/texel/data/<set>.pgn `
    --train train_<set>.csv --holdout holdout_<set>.csv --jobs 16 --max-per-game 24
```

## Tools

| Tool | Role |
|------|------|
| `sample_fens.py` | Reservoir-samples N diverse FENs from the Beast pool (or any FEN/CSV) into an EPD **opening book**. Streams (7 GB-safe), validates with python-chess, dedups, filters by piece count / check / optional quietness. |
| `datagen.ps1` (in `tools/`) | Fixed-node self-play from the book → appends to a PGN. Warns if `-Rounds` exceeds the book's opening count (the diversity rule). |
| `extract.py` | Single-threaded PGN → `FEN;result`. Skips opening/endgame/in-check/capture-or-promo plies, caps plies per game, dedups by FEN, splits holdout **by game** (no leakage). |
| `extract_parallel.py` | Multi-process drop-in for `extract.py` on large PGNs. Byte-range split aligned to game boundaries; per-game content-seeded RNG → output is identical regardless of `--jobs`. |
| `bake.py` | Writes a tuner weight dump back into `src/EvalParams.h`, rewriting only the members that moved (`--allow-pst` for the 2-D PST blocks). |
| `tuner.cpp` → `basilisk-texel` | `--tune <group>`, `--tune-kingsafety`, `--verify`, `--feature-support`, `--l2`. |
```
