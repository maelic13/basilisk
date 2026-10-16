<# Frozen launcher for PLAN B.2.4 (BAS-S23): Cluster 1's two gates, each an
   SPRT [0,10] (the wide bracket, justified in BAS-S23) at 3+0.03, one
   thread, Hash 64, UHO, no adjudication, tablebases off, capped at 2,500
   pairs. Every binary is a Release PEXT final-PGO build pinned by SHA-256.

   -Gate a   the unfitted B2_CORE=ON build against the B2_CORE=OFF build of
             the same revision (399c7ce): the cluster on its own evidence.
   -Gate b   the fitted ON build (SPSA block 2's centres baked) against the
             unfitted ON build: the fit on its own evidence. The two arms
             come from different commits, so the wrapper checks each
             manifest and fingerprint and this script pins both hashes. #>
param(
    [Parameter(Mandatory)][ValidateSet('a', 'b')][string]$Gate,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$unfit = @{ Path = Join-Path $engines 'basilisk-b24-on-unfit-pext-pgo.exe'; Sha = 'BF64DE180BDD2ABC169B47F5110E651E26D37807C526C90E9B3FB1F9B9D5C06C'; Bench = 6096792 }
$off   = @{ Path = Join-Path $engines 'basilisk-b24-off-pext-pgo.exe';      Sha = 'C9FD51631E9A50701B8A7B83400EABC5AFCDFD709B33F41D871784FE57FB0D1C'; Bench = 14978465 }
$fit   = @{ Path = Join-Path $engines 'basilisk-b24-on-fit-pext-pgo.exe';   Sha = '5D04A916445C41A744633C7026632B8AAD351B734B4AD0F54E38E4FE10EE322A'; Bench = 11099210 }

$pair = if ($Gate -eq 'a') { @($unfit, $off) } else { @($fit, $unfit) }
foreach ($engine in $pair) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'wide'
    MaxPairs = 2500
    EngineA = $pair[0].Path
    EngineB = $pair[1].Path
    ExpectBench = @($pair[0].Bench, $pair[1].Bench)
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    DryRun = $DryRun
}
if ($Gate -eq 'a') {
    $arguments += @{
        NameA = 'B2Core'; NameB = 'Legacy'; Seed = 24001
        ExpectRevision = '399c7ceab90bc884e59ee8c90c3de63796101078'
        Dir = Join-Path $PSScriptRoot 'results\b24-gate-a'
    }
} else {
    $arguments += @{
        NameA = 'Fitted'; NameB = 'Unfitted'; Seed = 24002
        Dir = Join-Path $PSScriptRoot 'results\b24-gate-b'
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
