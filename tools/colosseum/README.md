# Basilisk Colosseum policy

`tools/colosseum.ps1` is the guarded entry point for the committed run files
in this directory. The run files own shared conditions; a command line owns
the experiment's engines, finite budget, seed and evidence directory.

The wrapper refuses a busy host, an unpinned runner, a missing or stale engine
manifest, dirty or wrong-revision arms, compiler/flavor mismatches, unknown UCI
options, adjudication, policy drift in the resolved configuration (fresh
engine processes per game included), runner faults, and a PGN recount that
disagrees with the runner. In a gauntlet a Basilisk build's fault voids the
run; a foreign opponent's faults are read up to 1% of its games. `-DryRun` exercises all preflight checks without
playing. The runner is `colosseum-cli` 0.5.0, pinned by revision and SHA-256
in `colosseum.pin.json`; `tools/setup_tools.ps1` stages it from the release
asset and checks both digests.

| File | Use |
|---|---|
| `sprt-default.toml` | Ordinary `[0,3]` nElo gate |
| `sprt-removal.toml` | Removal/simplification `[-5,0]` |
| `sprt-repair.toml` | Unknown-sign repair `[-5,5]` |
| `sprt-wide.toml` | Registered large prior `[0,10]` |
| `match-fixed.toml` | Fixed measurement at `3+0.03` |
| `match-fixed-ltc.toml` | Fixed measurement at `10+0.1` |
| `match-fixed-long.toml` | Fixed measurement at `60+0.6` |
| `calibrate-null.toml` | Triggered null pair |
| `gauntlet.toml` | Rating gauntlet |
| `gauntlet-ltc.toml` | Rating gauntlet at `10+0.1` |
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

A run where either side has more than one thread is sibling-free: the
wrapper cuts Colosseum's automatic slots to one logical CPU per physical core
and passes them as `--placement`, and refuses an automatic multi-thread slot
(PROCESS *Harness*). A self-relative scaling leg plays one binary against
itself, arm B at its own thread count or clock:

```powershell
./tools/colosseum.ps1 -Mode match -Games 1000 -Seed <seed> `
  -EngineA tools/test_engines/<build>.exe -EngineB tools/test_engines/<build>.exe `
  -NameA b-1t -NameB b-4t -Threads 1 -ThreadsB 4 -Hash 256 `
  -RunFile tools/colosseum/match-fixed-ltc.toml -BaseMs 10000 -IncrementMs 100 `
  -ExpectBench <bench>,<bench> -Dir tools/results/<leg>
```

`-BaseMsB`/`-IncrementMsB` give the time-odds leg (arm B at twice the clock).
Start `tools/host_sampler.ps1 -OutDir tools/results/<leg>-sampler` beside a
multi-thread leg and read it with `tools/diag/smp_read.py`.

`tools/spsa_colosseum.ps1` runs the generated primary tune path and
`tools/spsa.ps1` remains the weather-factory backup. `docs/PROCESS.md`'s *Harness*
section owns the operator procedure and cross-check triggers.
