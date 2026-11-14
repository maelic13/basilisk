# Generated search SPSA surface

`config_search.json`, `colosseum/search.tune.toml`, and
`colosseum/search.run.toml` are generated from `src/search_params.h`:

```powershell
python tools/generate_spsa_surface.py
python tools/generate_spsa_surface.py --check
```

The generator preserves X-macro order and derives the integer step as
`max(2, round((max - min) / 16))`. The checked-in surfaces carry one step at
2,000 iterations, the shape of the blocks already played; they are kept so
those registrations name an exact surface. Never edit a generated surface.

A tune under PLAN rule 7c is one run to a registered ceiling. Its tune file is
generated for that ceiling, the perturbation twice the step where the seed has
the room and graded down to one step where it has not:

```powershell
python tools/generate_spsa_surface.py --rule7c <surface> --ceiling 6000 `
  --output tools/spsa_configs/colosseum/<tune>.rule7c.tune.toml
./tools/spsa_colosseum.ps1 -Engine <engine.exe> -ExpectBench <bench> `
  -Tune tools/spsa_configs/colosseum/<tune>.rule7c.tune.toml `
  -Seed <seed> -Dir tools/results/<tune>
```

The production shape is 15 slots × 30 games per iteration at `r_end` 0.00155,
read at 2,000 iterations and every 1,000 after it by `tools/spsa_read.py`.
Weather-factory remains the backup path through `tools/spsa.ps1 -ConfigGroup
search`; it consumes the same generated JSON and uses the same 30-game shape.
