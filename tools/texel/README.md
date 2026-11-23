# Texel tuning data pipeline

```
Beast FEN pool ──sample_fens.py──▶ <book>.epd ──datagen.ps1──▶ selfplay.pgn ──extract[_parallel].py──▶ train/validation/test + manifest.json ──fit.py──▶ a run: vectors, the baked header, run.json
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
#    The pool lives outside the repository: HANDBOOK.md names where.
python tools/texel/sample_fens.py "<the Beast pool's positions.txt>" `
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

# 3. Extract, under the row contract basilisk-texel-rows-v1. Only games that
#    ended by the rules are labelled, every rejection is counted, and the run
#    fails past --max-reject-pct (1%). Train, validation and test are split by
#    the start's hash and published together with manifest.json, never over an
#    existing directory's files. Beast starts use --skip-start 0: the supplied
#    FEN itself is a valid labelled position. 3.5M means exactly 700k per phase.
python tools/texel/extract_parallel.py tools/texel/data/<set>.pgn `
    --out-dir tools/texel/data/<set> --jobs 16 `
    --skip-start 0 --max-per-phase-per-game 8 --target-train 3500000

# 4. Fit, from a clean tree: the surface, K once, the stages, the bake checked as
#    built. The tree is left as it was; the run directory holds the baked header.
python tools/texel/fit.py --corpus tools/texel/data/<set> --out tools/results/fit-<name> `
    --expect-bench <the seed's bench 13>

# 5. After model selection, and once: claim the frozen test split.
python tools/texel/claim_test.py claim tools/texel/data/<set> `
    --candidate <vector hash> --purpose "<what the test read decides>"

# Fast header-only start-source/correlation proxy audit.
python tools/texel/audit_starts.py tools/texel/data/<set>.pgn
```

## Tools

| Tool | Role |
|------|------|
| `sample_fens.py` | Streams the Beast pool into five equal phase reservoirs, validates and dedups the selected EPDs; `--exclude-pgn` makes extension books disjoint from prior starts. |
| `datagen.ps1` (in `tools/`) | Fixed-node, seeded self-play from the book → fresh PGN + provenance manifest. Refuses accidental append, book wrap and an unregistered binary; fails a run with any fault or a short game count, and hashes the realised opening sequence. |
| `extract.py` | The row contract, `basilisk-texel-rows-v1`, and its sequential extractor, plus `--preflight-games`. Rows are `fen,result,game_id,start_id,ply,phase` under a schema line. Only rules endings are labelled, with rejections counted by reason. There are three splits by start, five exact phase reservoirs, quiet filtering and global dedup, and the three files and `manifest.json` are published together, never overwritten. |
| `extract_parallel.py` | The production multi-process extractor under the same contract, byte-identical to `extract.py` on the same inputs; `--audit-only` reports the accounting and quotas without publishing. |
| `relabel.py` | Corpus B (PLAN C.2.4): a rows-v1 corpus relabelled row by row by Syzygy under the label contract `basilisk-texel-labels-syzygy-v1`. Each row of six men or fewer with no castling right takes the tablebase verdict under its own rule-50 clock, in Stockfish's form; every other column and row is A's. `build` publishes B or nothing and then runs `check`. `check` proves the identity with A and re-probes a sample. `games` measures whole-game adjudication without building it. |
| `test_relabel.py` | The verdict's clock semantics on a fake tablebase, and, with the local tables, build, refusal to overwrite and `check` catching three kinds of tampering. |
| `claim_test.py` | Claims a corpus's frozen test split once, by exclusive create, after checking the test file against its manifest's hash. |
| `surface_v1.tsv` | The fit's surface, `basilisk-texel-surface-v1`. It holds every one of the tuner's slots, each with:<br>• its class: free, fixed or excluded;<br>• the stage that fits it: linear, or the king-safety descent;<br>• its anchor, where `mean:<set>` holds a set's level where the data cannot see it;<br>• a knob's bounds and first step;<br>• the reason.<br>Generated by `surface.py build`; never edited by hand. |
| `surface.py` | Builds and checks the manifest from the tuner's registry and its rules (`build`, `check`). `rank` reads a corpus's coefficient matrix: it freezes slots under a support threshold into a corpus manifest, and fails if a direction the data cannot see survives the anchors. `check-vector` validates a complete initial vector, with the PST orientation test for a seed. |
| `test_surface.py` | The manifest's tests: the rules and invariants, the rank analysis on matrices with known null directions, and, with `BASILISK_TEXEL` set, the tracked manifest against the tuner. |
| `test_extract.py` | The contract's tests: rules endings, row identity, splits by start, the reject ceiling, publication, extractor parity and the claim. |
| `audit_starts.py` | Header-only audit of exact start duplication, five-phase coverage and pawn-family concentration; explicitly cannot reconstruct missing Beast source-game provenance. |
| `fit.py` | The fit driver, which replaced `phase911.ps1` (retired 2026-11-22). It runs one corpus on one surface:<br>• it requires a clean tree and builds its own traced tuner and release engine;<br>• it checks the seed's bench and verifies the model;<br>• it fits K once, on validation;<br>• it runs the stages king safety, linear, king safety and polish, each from the vector the last one wrote;<br>• it bakes, rebuilds and checks the result as built: the vector held exactly, `--verify`, the new bench, the played hash and `test_eval`.<br>It hashes every input and output in `run.json` and restores `src/eval_params.h` byte for byte, success or failure, leaving the baked header and its diff in the run directory. |
| `test_fit.py` | The fit's convergence rules on data whose optimum is known. A run with room reaches it; a schedule too short to travel there exits 3. |
| `bake.py` | Writes a tuner vector back into `src/eval_params.h`, rewriting only the members that moved (`--allow-pst` for the 2-D PST blocks) without formatting drift across repeated bakes. It refuses a vector that:<br>• has no `basilisk-texel-vector-v1` header;<br>• misses a member or names an unknown one;<br>• has a root that is not this header's defaults: a stale dump. |
| `tuner.cpp` → `basilisk-texel` | `--tune <group>` (`--tune surface` fits the manifest's free linear slots, with its mean anchors), `--tune-kingsafety`, `--verify`, `--played-hash`, `--feature-support`, `--list-params`, `--dump-coeffs`, `--l2`. Every mode takes `--init FILE`, a complete vector to start from that refuses a partial one, and `--surface FILE`. Every fit models the score the engine serves, from the evaluator's own record of it: the lazy checkpoint, the endgame step and the rule-50 damping. Overrides and draw verdicts take no part in K or the loss. `--verify` checks that model against the engine stage by stage on every row. `--played-hash` hashes the played scores as `eval_hash` hashes a release build's, so equal hashes prove the lazy path. Holdout diagnostics use the extractor's five phases, and both fits restore their best holdout checkpoint. |
