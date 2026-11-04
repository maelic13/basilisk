<# Frozen launcher for PLAN B.3.6 step 2 (BAS-S46): the piece-keyed histories
   indexed by coloured piece (COLOUR_HISTORY ON) against the colourless form,
   both with Cluster 2 on. SPRT [0,3] nElo (sprt-default.toml) at 3+0.03,
   capped at 16,000 pairs, one thread, Hash 64, UHO, no adjudication,
   tablebases off. Both arms are Release PEXT final-PGO builds of one
   revision, pinned by SHA-256. The first run (seed 46001, results\b36-gate)
   is void: a Rarog workload shared the host. This re-run starts from zero
   with seed 46002 in results\b36-gate-r2. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$revision = '70e978b72a39b78b8239bd7a863e3b0d16dc34da'
$colour     = @{ Path = Join-Path $engines 'basilisk-b36-colour-on-pext-pgo.exe';  Sha = '0E2C93E28C2AB65C567C18A4175743168E85F0F95E56979C4C54C574EDC092D0'; Bench = 5361058 }
$colourless = @{ Path = Join-Path $engines 'basilisk-b36-colour-off-pext-pgo.exe'; Sha = '21CF9C560ED407F59EA910B69AB9DBDC8D9D5F54E59FFCD1D587E7AE3472C479'; Bench = 5369042 }

foreach ($engine in @($colour, $colourless)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    MaxPairs = 16000
    EngineA = $colour.Path;     NameA = 'ColourHistory'
    EngineB = $colourless.Path; NameB = 'Colourless'
    ExpectBench = @($colour.Bench, $colourless.Bench)
    ExpectRevision = $revision
    Seed = 46002
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b36-gate-r2'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
