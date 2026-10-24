<# Frozen launcher for BAS-S24: SPSA block 1's centres against the seeds.

   One ON Tune build on both arms (no build offset): arm A plays block 1's 49
   rounded centres from tools/results/b23-spsa-block1/tuned-options.txt, arm B
   the seeds (the binary's defaults). 2,000 games (1,000 paired UHO openings)
   at 3+0.03, one thread, Hash 64, no adjudication, tablebases off, on the
   Colosseum match-fixed.toml policy. A diagnostic of where the fit stands,
   not a gate: B.2.3's blocks and B.2.4(b) are unchanged. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engine = Join-Path $PSScriptRoot 'test_engines\basilisk-b23b-on-tune-pext-tune-pgo.exe'
$engineSha = 'A900EFD2AC77CD3F4178A1161A4FA5091A313DA5F4CCCE4EA799043C99CA2AEB'
$theta = Join-Path $PSScriptRoot 'results\b23-spsa-block1\tuned-options.txt'
$thetaSha = '5DB77308154B088BE3E7D641F38776DEAA7390DDFB30D5E666A58C1E231282A1'
$surface = Join-Path $PSScriptRoot 'spsa_configs\config_core.json'

foreach ($pin in @(@($engine, $engineSha), @($theta, $thetaSha))) {
    $actual = Get-HarnessSha256 $pin[0]
    if ($actual -ne $pin[1]) { throw "PROVENANCE MISMATCH - $($pin[0]) is $actual, not $($pin[1])." }
}

# Block 1's centres as Name=Value, every one a coordinate of the tuned surface.
$names = @((Get-Content -LiteralPath $surface -Raw | ConvertFrom-Json).PSObject.Properties.Name)
$optionsA = @(
    Get-Content -LiteralPath $theta | ForEach-Object {
        if ($_ -notmatch '^setoption name (\S+) value (-?\d+)$') { throw "Unexpected line in ${theta}: $_" }
        if ($names -notcontains $Matches[1]) { throw "$($Matches[1]) is not on the tuned surface." }
        "$($Matches[1])=$($Matches[2])"
    }
)
if ($optionsA.Count -ne $names.Count) { throw "Block 1 sets $($optionsA.Count) options; the surface has $($names.Count)." }

$arguments = @{
    Mode = 'match'
    Games = 2000
    EngineA = $engine; NameA = 'Block1'
    EngineB = $engine; NameB = 'Seeds'
    OptionsA = $optionsA
    Seed = 24001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    ExpectRevision = 'b5983c6dfa08c136231d636d9bcbd5eb7cecfc5f'
    ExpectBench = @(6096792, 6096792)
    Dir = Join-Path $PSScriptRoot 'results\b23-block1-match'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
