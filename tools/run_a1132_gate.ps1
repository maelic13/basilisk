<# Frozen launcher for A.13.2 (BAS-S38): the pawnless scaler in Stockfish's
   form, its classifier in the donor's middlegame values and its near-draw
   factor. SPRT on the repair bracket [-5,5] (sprt-repair.toml) at 3+0.03,
   one thread, Hash 64, UHO, no adjudication, tablebases off, capped at
   4,000 pairs. Engine A is the repair, engine B the head after A.13.1; both
   are Release PEXT final-PGO builds, pinned by SHA-256. They come from
   different commits, so the wrapper checks each manifest's clean tree and
   fingerprint and this script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a1132-fix-pext-pgo.exe';  Sha = 'B7C3277E8EC0337A2E6AD46A00E0DB4A97DFACAD9D6438E3CF02C91DC0F43A8F'; Bench = 19470325 }
$base = @{ Path = Join-Path $engines 'basilisk-a1131-head-pext-pgo.exe'; Sha = '8BC14CB4A9B3E78546E814E41E33AA804EE498092A59E71395DAC811FB260EF0'; Bench = 19108490 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'PawnlessDonor'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 38001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a1132-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
