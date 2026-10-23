<# Frozen launcher for PLAN B.10 (BAS-S26): the piece-square table orientation
   repair. SPRT on the repair bracket [-5,5] (sprt-repair.toml) at 3+0.03, one
   thread, Hash 64, UHO, no adjudication, tablebases off, capped at 4,000
   pairs. Engine A is the flipped tables, engine B the B.2.4 head's engine
   rebuilt with the same compiler; both are Release PEXT final-PGO builds with
   B2_CORE on, pinned by SHA-256. They come from different commits, so the
   wrapper checks each manifest's clean tree and fingerprint and this script
   pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$flip = @{ Path = Join-Path $engines 'basilisk-b10-flip-pext-pgo.exe';   Sha = 'AF8B1398A4722C4F0411199E09EBEB376809C6156E43BBEF8F9657AD5757BBAE'; Bench = 8437148 }
$head = @{ Path = Join-Path $engines 'basilisk-b10-base-pext-pgo.exe';   Sha = '41C7DA36636D083678310428EF80B95437BE3AF294AA0827803738B8D19E0F23'; Bench = 11099210 }

foreach ($engine in @($flip, $head)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $flip.Path; NameA = 'PstFlip'
    EngineB = $head.Path; NameB = 'B24Head'
    ExpectBench = @($flip.Bench, $head.Bench)
    Seed = 26001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b10-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
