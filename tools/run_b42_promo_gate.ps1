<# Frozen launcher for PLAN B.4.2 (BAS-S48): SEE scores promoting
   recaptures. SPRT on the repair bracket [-5,5] (sprt-repair.toml) at
   3+0.03, one thread, Hash 64, UHO, no adjudication, tablebases off,
   capped at 4,000 pairs. Engine A is the repair (9aaa528), engine B the
   head before it (9f29f49); both are Release PEXT final-PGO builds from
   clean trees, pinned by SHA-256. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-b42-promo-pext-pgo.exe'; Sha = '416A153C725C59FAE884708BBE5E7AA8763093B27DA52B28F74811A658BB3828'; Bench = 8532389 }
$base = @{ Path = Join-Path $engines 'basilisk-b42-base-pext-pgo.exe'; Sha = 'EA12EE3A1934D22945E44C0228717BD7426534CF11A6EC1CF7A57E34B1F2EC96'; Bench = 5361058 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'PromoSee'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 48001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b42-promo-gate'
    DryRun = $DryRun
    # A dry run plays no games, so it may check the configuration beside
    # other work; the real run keeps the idle-host guard.
    AllowBusyHost = [bool]$DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
