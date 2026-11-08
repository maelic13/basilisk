<# Frozen launcher for PLAN B.4.2 (BAS-S49): SEE follows pins opened
   during the exchange. SPRT on the repair bracket [-5,5]
   (sprt-repair.toml) at 3+0.03, one thread, Hash 64, UHO, no
   adjudication, tablebases off, capped at 1,500 pairs. Engine A is the
   repair (d3add9c), engine B the promotion repair before it (9aaa528);
   both are Release PEXT final-PGO builds from clean trees, pinned by
   SHA-256. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-b42-pins-pext-pgo.exe'; Sha = 'ABF97BE1390040943E8331995178D98CB61182BBC65874A558DEC8DC119982B1'; Bench = 6090091 }
$base = @{ Path = Join-Path $engines 'basilisk-b42-promo-pext-pgo.exe'; Sha = '416A153C725C59FAE884708BBE5E7AA8763093B27DA52B28F74811A658BB3828'; Bench = 8532389 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 1500
    EngineA = $fix.Path;  NameA = 'PinSee'
    EngineB = $base.Path; NameB = 'PromoSee'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 49001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b42-pins-gate'
    DryRun = $DryRun
    # A dry run plays no games, so it may check the configuration beside
    # other work; the real run keeps the idle-host guard.
    AllowBusyHost = [bool]$DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
