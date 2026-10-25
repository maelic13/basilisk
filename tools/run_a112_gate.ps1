<# Frozen launcher for PLAN A.11.2 (BAS-S29): a lone excluded move fails the
   singular search by one. SPRT on the repair bracket [-5,5]
   (sprt-repair.toml) at 3+0.03, one thread, Hash 64, UHO, no adjudication,
   tablebases off, capped at 4,000 pairs. Engine A is the repair, engine B
   its parent on master; both are Release PEXT final-PGO builds with B2_CORE
   on, pinned by SHA-256. They come from different commits, so the wrapper
   checks each manifest's clean tree and fingerprint and this script pins
   both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a112-fix-pext-pgo.exe';  Sha = '777BA298BCFE12E65B46EE3858D28B6A658F6C2C12832D42B64684AD8D179F44'; Bench = 8753339 }
$base = @{ Path = Join-Path $engines 'basilisk-a112-base-pext-pgo.exe'; Sha = '0CB79CCD71D770F3429ED0974BD386E9C82E1E11D4E4C7500770D3776D75BE60'; Bench = 9260429 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'LoneFix'
    EngineB = $base.Path; NameB = 'Master'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 29001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a112-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
