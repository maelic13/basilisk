<# Frozen launcher for BAS-S34: the root underpromotion rule re-tested
   where it acts. SPRT on the no-worse bracket [-5,0] (sprt-removal.toml)
   from the 3,159 positions of tools/diag/promotion_choices_v1.epd, where
   the two engines choose differently at 100,000 nodes; each opening once
   with colours reversed, capped at the book's size so it never wraps.
   3+0.03, one thread, Hash 64, no adjudication, tablebases off. Engine A
   is the rule (BAS-S33's candidate), engine B the A.11.9 candidate; both
   pinned by SHA-256. #>
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
    Bracket = 'removal'
    MaxPairs = 3159
    EngineA = $fix.Path;  NameA = 'RootPromo'
    EngineB = $base.Path; NameB = 'TierFix'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 34001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a119-up-retest'
    Book = Join-Path $PSScriptRoot 'diag\promotion_choices_v1.epd'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
