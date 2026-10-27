<# Frozen launcher for A.13.1 (BAS-S37): a won KPK scores as a known win.
   SPRT on the repair bracket [-5,5] (sprt-repair.toml) at 3+0.03, one
   thread, Hash 64, UHO, no adjudication, tablebases off, capped at 2,000
   pairs. Engine A is the repair, engine B the head before it (BAS-S36's
   candidate, whose engine master reproduces); both are Release PEXT
   final-PGO builds, pinned by SHA-256. They come from different commits, so
   the wrapper checks each manifest's clean tree and fingerprint and this
   script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a1131-fix-pext-pgo.exe'; Sha = 'BD45E2DD69ABE25D52B17CE8CD23DCCC904D47CB2E18E3DE3F58DD9054837A16'; Bench = 19108490 }
$base = @{ Path = Join-Path $engines 'basilisk-a1111-fs-pext-pgo.exe';  Sha = '1F8A2058322ABC25514B1B14FE12573BF0FE35A99876B916F15554AEE8C6A43D'; Bench = 16092165 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 2000
    EngineA = $fix.Path;  NameA = 'KpkWin'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 37001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a1131-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
