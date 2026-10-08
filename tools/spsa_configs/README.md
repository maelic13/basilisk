# Generated search SPSA surface

`config_search.json`, `colosseum/search.tune.toml`, and
`colosseum/search.run.toml` are generated from `src/search_params.h`:

```powershell
python tools/generate_spsa_surface.py
python tools/generate_spsa_surface.py --check
```

The generator preserves X-macro order and derives the integer perturbation as
`max(2, round((max - min) / 16))`. The Colosseum `c_end` values are calculated
for one PLAN rule 7c block: 2,000 iterations. Never edit a generated surface.

Colosseum is the primary tune path:

```powershell
./tools/spsa_colosseum.ps1 -Engine <engine.exe> -ExpectBench <bench> `
  -Seed <seed> -Dir tools/results/<block-1>

python tools/spsa_block_rule.py tools/results/<block-1>

./tools/spsa_colosseum.ps1 -Engine <engine.exe> -ExpectBench <bench> `
  -Seed <fresh-seed> -SeedFrom tools/results/<block-1> `
  -Dir tools/results/<block-2>
```

Production blocks are 15 slots × 30 games per iteration. A later block gets a
fresh schedule and seed but starts from the prior block's rounded centres.
Weather-factory remains the backup path through `tools/spsa.ps1 -ConfigGroup
search`; it consumes the same generated JSON and uses the same 30-game shape.
