<# Frozen launcher for A.11.10 (BAS-S35): the rule-50 damping keeps a
   bare-king tier's base. SPRT on the repair bracket [-5,5]
   (sprt-repair.toml) at 3+0.03, one thread, Hash 64, UHO, no adjudication,
   tablebases off, capped at 1,500 pairs. This is the second run, from zero:
   the maintainer stopped the first at 107 pairs. Engine A is the repair,
   engine B the head before it; both are Release PEXT final-PGO builds with B2_CORE
   on, pinned by SHA-256. They come from different commits, so the wrapper
   checks each manifest's clean tree and fingerprint and this script pins
   both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a1110-fix-pext-pgo.exe';  Sha = 'C8F6296C378A6F6FB2B388C01BDA63DC0C2EA3747FED3BA3FD31318A8BA5A511'; Bench = 8913394 }
$base = @{ Path = Join-Path $engines 'basilisk-a1110-base-pext-pgo.exe'; Sha = '6BB7B1CBE29EABC059A7C6092512C7A789C8D1872DE5FF03F5E080139F2B2AF6'; Bench = 7857713 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 1500
    EngineA = $fix.Path;  NameA = 'TierBase'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 35002
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a1110-gate-r2'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
