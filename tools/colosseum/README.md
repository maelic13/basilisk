# Engine testing with Colosseum CLI

Basilisk delegates generic UCI orchestration, statistics, SPSA scheduling,
affinity, persistence and result analysis to the independent `colosseum-cli`.
The engine remains an ordinary UCI executable; Colosseum requires no manifest,
compiler convention or custom benchmark protocol.

Install `colosseum-cli` separately and put it on `PATH`. Run examples from the
Basilisk repository root. Committed TOML files contain engine-project policy;
executable paths, books, run directories and host concurrency are arguments.

## Strength tests and calibration

The profiles preserve Basilisk's `3+0.03`, draw 40/8/10, two-sided resign
600/3, Hash 64 per thread and normalized-Elo conventions. No opening book is
bundled. Pass one explicitly when available and choose concurrency for the
current host:

```powershell
colosseum-cli --run-file tools/colosseum/profiles/sprt-gainer.toml `
  sprt <candidate> <baseline> --book <book.epd> --concurrency 14

colosseum-cli --run-file tools/colosseum/profiles/sprt-simplify.toml `
  sprt <candidate> <baseline> --book <book.epd> --concurrency 14

colosseum-cli --run-file tools/colosseum/profiles/calibrate.toml `
  calibrate <engine> <byte-identical-copy> --book <book.epd> --concurrency 14

colosseum-cli --run-file tools/colosseum/profiles/calibrate-4t.toml `
  calibrate <engine> <byte-identical-copy> --book <book.epd> --concurrency 3
```

Calibration is optional. The 4-thread profile exists because Basilisk ships
and tests parallel search; it does not make calibration a prerequisite for an
ordinary match or release.

## SPSA

Build a tune-enabled engine with Basilisk's build script, then select a
committed Colosseum parameter vector:

```powershell
.\tools\build_test.ps1 -Suffix search-tune -Tune
colosseum-cli --run-file tools/colosseum/profiles/spsa.toml `
  spsa <tune-engine> --tune tools/colosseum/tunes/pruning.toml `
  --book <book.epd> --concurrency 14 --dir <run-directory>
```

The vectors were converted from the former weather-factory 5,000-iteration
schedule. Their terminal perturbation is
`max(0.5, legacy_step * 5000^-0.102)`; two legacy step-1 knobs are intentionally
floored so their integer plus/minus arms remain distinct. Use `spsa plan` before
a long run, `spsa status` for read-only progress, and `sprt --apply` to gate the
completed vector.

## Other workflows

```powershell
# UCI schema and compliance
colosseum-cli engine inspect <engine> --json
colosseum-cli engine check <engine> --json

# Fixed-node speed, pooled independent builds and thread scaling
colosseum-cli nps <candidate> --against <baseline> --nodes 10000000 --repetitions 12
colosseum-cli nps <build-a1> --a-build <build-a2> --against <build-b1> `
  --b-build <build-b2> --nodes 10000000 --repetitions 12
colosseum-cli nps <engine> --nodes 10000000 --scale-threads 1,2,4,8 `
  --threads-option Threads --hash-policy per-thread --hash-mb 64

# Round-robin or gauntlet
colosseum-cli --run-file tools/colosseum/profiles/tournament.toml `
  tournament run --format gauntlet --seeds 1 --engine <basilisk> `
  --engine <opponent> --book <book.epd> --concurrency 14 --dir <run-directory>

# Fixed-node self-play PGN for the engine-owned Texel extractor
colosseum-cli --run-file tools/colosseum/profiles/datagen.toml `
  match <engine> <engine> --book <seed.epd> --book-order random `
  --games 30000 --concurrency 14 --dir <run-directory>
```

Use `stats`, `stats telemetry`, `suite` and `book` for result recomputation,
PGN diagnostics, generic EPD suites and opening inspection. Colosseum's
`games.pgn` feeds Basilisk's Texel extraction tools; sampling, labels, fitting
and source-value baking remain engine-specific.

## Responsibility boundary

| Colosseum CLI owns | Basilisk owns |
|---|---|
| UCI launch/probe, matches, SPRT, calibration and tournaments | Source, UCI option semantics and engine correctness |
| SPSA schedule, audit, persistence, resume and result artifacts | Tune-enabled builds and baking accepted values into source |
| CPU topology/affinity, concurrent execution, seeds and hashing | Compiler, flags, PGO/ISA builds and build comparability |
| NPS experiments, result statistics, PGN telemetry and EPD suites | Profiling and engine-specific diagnostic readouts |
| Opening parsing, ordering and non-reuse policy | Book choice and generated Texel data/labels |

The retired PowerShell/Python harness implementations remain in Git history.
Historical experiment records may cite them as evidence; those citations are
not current commands.
