<# Frozen launcher for B.14 (BAS-S44): the evaluator's draw verdicts take no
   correction. SPRT on the repair bracket [-5,5] (sprt-repair.toml) at
   3+0.03, one thread, Hash 64, UHO, no adjudication, tablebases off, capped
   at 4,000 pairs. Engine A is the repair's OFF arm, engine B the head before
   it (c15d097, OFF arm); both are Release PEXT final-PGO builds, pinned by
   SHA-256. They come from different commits, so the wrapper checks each
   manifest's clean tree and fingerprint and this script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-b14-fix-pext-pgo.exe';  Sha = '77BB764C7483B22E35DF156A7572E6AE5733B7969292CB11EF1A744D50F86D45'; Bench = 11880564 }
$base = @{ Path = Join-Path $engines 'basilisk-b14-base-pext-pgo.exe'; Sha = '3CC41280666C93AAD6D986A1B44B7E2D84710A91C500A125D872DBD2D2A86C61'; Bench = 16615757 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'VerdictNoCorr'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 44001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b14-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
