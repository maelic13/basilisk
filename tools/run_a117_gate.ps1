<# Frozen launcher for PLAN A.11.7 (BAS-S31): the pawnless scaler's winning
   side taken from the score. SPRT on the repair bracket [-5,5]
   (sprt-repair.toml) at 3+0.03, one thread, Hash 64, UHO, no adjudication,
   tablebases off, capped at 4,000 pairs. Engine A is the repair, engine B
   the A.11.6 candidate BAS-S30 accepted; both are Release PEXT final-PGO
   builds with B2_CORE on, pinned by SHA-256. They come from different
   commits, so the wrapper checks each manifest's clean tree and
   fingerprint and this script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a117-fix-pext-pgo.exe';  Sha = '24C1629C5B5700A65102E2A842E69DAB3F45D662285C91D379B60A7C6AC030CF'; Bench = 7384101 }
$base = @{ Path = Join-Path $engines 'basilisk-a116-fix-pext-pgo.exe';  Sha = 'EA3114242C434FA70E13E6113021C7F550DB6E0FC681137B98C765010D1DA09B'; Bench = 7233823 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'ScaleFix'
    EngineB = $base.Path; NameB = 'CountFix'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 31001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a117-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
