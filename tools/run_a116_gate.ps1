<# Frozen launcher for PLAN A.11.6 (BAS-S30): each node counted once, with
   the root kept out of the tie rule. SPRT on the repair bracket [-5,5]
   (sprt-repair.toml) at 3+0.03, one thread, Hash 64, UHO, no adjudication,
   tablebases off, capped at 4,000 pairs. Engine A is the branch's engine
   head, engine B master; both are Release PEXT final-PGO builds with
   B2_CORE on, pinned by SHA-256. They come from different commits, so the
   wrapper checks each manifest's clean tree and fingerprint and this script
   pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a116-fix-pext-pgo.exe';  Sha = 'EA3114242C434FA70E13E6113021C7F550DB6E0FC681137B98C765010D1DA09B'; Bench = 7233823 }
$base = @{ Path = Join-Path $engines 'basilisk-a116-base-pext-pgo.exe'; Sha = 'DE7412DE3B362ED62967AF7949EE4704084B96F4CC74D80BCEF965E2BF3F159D'; Bench = 8753339 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'CountFix'
    EngineB = $base.Path; NameB = 'Master'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 30001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a116-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
