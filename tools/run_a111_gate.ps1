<# Frozen launcher for PLAN A.11.1 (BAS-S28): rule-50 damping out of the TT
   static eval. SPRT on the repair bracket [-5,5] (sprt-repair.toml) at
   3+0.03, one thread, Hash 64, UHO, no adjudication, tablebases off, capped
   at 4,000 pairs. Engine A is the repair, engine B its parent on master;
   both are Release PEXT final-PGO builds with B2_CORE on, pinned by SHA-256.
   They come from different commits, so the wrapper checks each manifest's
   clean tree and fingerprint and this script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a111-fix-pext-pgo.exe';  Sha = 'ED147BAA0840D76F2A3612BCC4E707C653B13204CBEAC6C4712ADD2C03AC9B18'; Bench = 9260429 }
$base = @{ Path = Join-Path $engines 'basilisk-a111-base-pext-pgo.exe'; Sha = '261C155ABA32EAC6E28E759D310F76E792DE395E48571E0B3D0FA7C237254898'; Bench = 8437148 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'R50Fix'
    EngineB = $base.Path; NameB = 'Master'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 28001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a111-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
