# Texel tuning data pipeline

```
Beast FEN pool ──sample_fens.py──▶ <book>.epd ──datagen.ps1──▶ selfplay.pgn ──extract[_parallel].py──▶ train.csv + holdout.csv ──tuner──▶ baked weights
```

The labels are **Basilisk self-play game results** (WDL, white-perspective
1.0/0.5/0.0), not external engine scores. The Beast pool is a compilation of
positions from human, ICCF and computer-game databases; it supplies starts,
then Basilisk plays each position out to produce the label. Exact start FENs
are deduplicated, but Beast does not retain source-game IDs, so related opening
positions cannot be proven independent. `audit_starts.py` reports measurable
exact and pawn-structure-family concentration without pretending otherwise.

## ⚠️ The diversity rule (read before generating data)

Self-play between two **identical** engines at a fixed node limit is
**deterministic** — a given start position always produces the same game. So
the number of *distinct* games is capped by the number of *distinct openings in
the book*, **not** by `-Rounds`. Running many rounds over a small book just
replays the same games. `datagen.ps1` deliberately plays **one game per
opening**: swapping the A/B identities of identical engines would reproduce
the same moves and label exactly, so the extractor would discard the repeat.

- ❌ `SuperGM_4mvs.pgn` (~2.7k openings): 300k rounds → **1.5k distinct games**,
  ~31k unique positions. Useless for tuning. (`datagen.ps1` now warns on this.)
- ✅ `beast_seed.epd` (100k+ sampled FENs): each opening → a distinct game →
  millions of diverse positions.

To add variety beyond the book size, run extra passes at a **different
`-Nodes`** value (same opening + different node budget → a different
deterministic game). `extract_parallel.py` dedups by FEN across all passes.
To extend at the same node budget, use `sample_fens.py --exclude-pgn <old.pgn>`
to build a disjoint opening book, then make an intentional registered append.

## Commands

```powershell
# 0. Build the current head as the datagen engine.
./tools/build_test.ps1 -Suffix <head>

# 1. Sample a diverse EPD start book from the Beast pool (read-only source).
#    Five equal material-phase reservoirs are the default. Use
#    --natural-phase-mix only for an intentional distribution experiment.
python tools/texel/sample_fens.py "A:/Chess/Beast/data/txt/positions.txt" `
    --out tools/texel/data/<book>.epd --count <games>

# To reuse an existing 200k-game corpus and reach 600k unique starts instead:
python tools/texel/sample_fens.py tools/texel/data/beast_seed_2m.epd `
    --out tools/texel/data/<extension>.epd --count 400000 --seed 43 `
    --no-validate --exclude-pgn tools/texel/data/<existing>.pgn

# 2. Self-play from the book. datagen refuses an existing output by default,
#    records the random seed, and writes <set>.pgn.manifest.txt.
./tools/datagen.ps1 -Suffix <head> -Rounds 600000 -Nodes 8000 `
    -Book tools/texel/data/<book>.epd -BookFormat epd `
    -OutputPgn tools/texel/data/<set>.pgn -Seed 42

# 3. Extract. Beast starts use --skip-start 0: the supplied FEN itself is a
#    valid labelled position, not a conventional played book line. Sampling is
#    capped inside each of five phases per game, then fed to five equal bounded
#    reservoirs. Quiet filtering, global FEN dedup, exact balanced holdout and
#    atomic output are on by default. 3.5M means exactly 700k per phase.
python tools/texel/extract_parallel.py tools/texel/data/<set>.pgn `
    --train train_<set>.csv --holdout holdout_<set>.csv --jobs 16 `
    --skip-start 0 --max-per-phase-per-game 8 --target-train 3500000

# Fast header-only start-source/correlation proxy audit.
python tools/texel/audit_starts.py tools/texel/data/<set>.pgn
```

## Tools

| Tool | Role |
|------|------|
| `sample_fens.py` | Streams the Beast pool into five equal phase reservoirs, validates and dedups the selected EPDs; `--exclude-pgn` makes extension books disjoint from prior starts. |
| `datagen.ps1` (in `tools/`) | Fixed-node, seeded self-play from the book → fresh PGN + provenance manifest. Refuses accidental append and warns if `-Rounds` exceeds the book's opening count. |
| `extract.py` | Reference/sequential extractor plus `--preflight-games`: five exact reservoirs, per-phase-per-game sampling, quiet filtering, global dedup, deterministic game split, and atomic output. |
| `extract_parallel.py` | Production multi-process implementation of the same contract for large PGNs; `--audit-only` measures exact capacity without publishing CSVs. |
| `audit_starts.py` | Header-only audit of exact start duplication, five-phase coverage and pawn-family concentration; explicitly cannot reconstruct missing Beast source-game provenance. |
| `phase911.ps1` | Registered one-dataset/one-fit driver: deterministic extraction, then `all` bake → rebuild → king-safety bake → rebuild → reconstruction verification. |
| `bake.py` | Writes a tuner weight dump back into `src/eval_params.h`, rewriting only the members that moved (`--allow-pst` for the 2-D PST blocks) without formatting drift across repeated bakes. |
| `tuner.cpp` → `basilisk-texel` | `--tune <group>`, `--tune-kingsafety`, `--verify`, `--feature-support`, `--l2`; holdout diagnostics use the extractor's same five phases, and both joint and king-safety fits restore their best holdout checkpoint. |
