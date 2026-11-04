<# Frozen launcher for B.12 (BAS-S40): quiescence's gain pruning stands
   down in a known loss. SPRT on the repair bracket [-5,5] (sprt-repair.toml)
   at 3+0.03, one thread, Hash 64, UHO, no adjudication, tablebases off,
   capped at 4,000 pairs. Engine A is the repair, engine B the head before
   it; both are Release PEXT final-PGO builds, pinned by SHA-256. They come
   from different commits, so the wrapper checks each manifest's clean tree
   and fingerprint and this script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-b12-fix-pext-pgo.exe';  Sha = 'F1C3D0BC7BAD8377B2287B50ADBCE3786F0876B44A0455F60961DC48914ED6E3'; Bench = 16615757 }
$base = @{ Path = Join-Path $engines 'basilisk-b12-base-pext-pgo.exe'; Sha = '37B2F3E48B43289CFE94A40DE9F49C9AB4ECAFBDC4376BE2158AE2BE3A193E5B'; Bench = 19470325 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'QsKnownLoss'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 40001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b12-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
