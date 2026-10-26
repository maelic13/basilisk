<# Frozen launcher for the A.11.9 follow-up (BAS-S33): root underpromotions
   judged against the queen promotion. SPRT on the repair bracket [-5,5]
   (sprt-repair.toml) at 3+0.03, one thread, Hash 64, UHO, no adjudication,
   tablebases off, capped at 4,000 pairs. Engine A is the repair, engine B
   the A.11.9 candidate BAS-S32 kept; both are Release PEXT final-PGO builds
   with B2_CORE on, pinned by SHA-256. They come from different commits, so
   the wrapper checks each manifest's clean tree and fingerprint and this
   script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a119-up-pext-pgo.exe';   Sha = '0D80B72FB5B2196283F70CA2C8E25F97F7CA4B603460AA5C579860D587DBBCEF'; Bench = 7857713 }
$base = @{ Path = Join-Path $engines 'basilisk-a119-fix-pext-pgo.exe';  Sha = 'FE85A3EF8AA2BD784D87861601AAD0DDF122A219E321FA3B4807B6F3EBD10CE0'; Bench = 7857713 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'RootPromo'
    EngineB = $base.Path; NameB = 'TierFix'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 33001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a119-up-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
