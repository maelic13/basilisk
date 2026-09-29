# Basilisk Colosseum policy

`tools/colosseum.ps1` is the guarded entry point for the committed run files
in this directory. The run files own shared conditions; a command line owns
the experiment's engines, finite budget, seed and evidence directory.

The wrapper refuses a busy host, an unpinned runner, a missing or stale engine
manifest, dirty or wrong-revision arms, compiler/flavor mismatches, unknown UCI
options, adjudication, policy drift in the resolved configuration, runner
faults, and a PGN recount that disagrees with the runner. `-DryRun` exercises
all preflight checks without playing.

| File | Use |
|---|---|
| `sprt-default.toml` | Ordinary `[0,3]` nElo gate |
| `sprt-removal.toml` | Removal/simplification `[-5,0]` |
| `sprt-repair.toml` | Unknown-sign repair `[-5,5]` |
| `sprt-wide.toml` | Registered large prior `[0,10]` |
| `match-fixed.toml` | Fixed measurement at `3+0.03` |
| `match-fixed-ltc.toml` | Fixed measurement at `10+0.1` |
| `calibrate-null.toml` | Triggered null pair |
| `gauntlet.toml` | Rating gauntlet |
| `spsa-tune.toml` | SPSA block policy: 15 slots × 30 games/iteration |

Example:

```powershell
./tools/colosseum.ps1 -Mode sprt `
  -EngineA tools/test_engines/<candidate>.exe `
  -EngineB tools/test_engines/<baseline>.exe `
  -MaxPairs <registered-cap> -Seed <seed> `
  -ExpectRevision <sha> -ExpectBench <candidate-bench>,<baseline-bench> `
  -Dir tools/results/<experiment>
```

`tools/spsa_colosseum.ps1` runs the generated primary tune path and
`tools/spsa.ps1` remains the weather-factory backup. `PROCESS.md`'s *Harness*
section owns the operator procedure and cross-check triggers.
