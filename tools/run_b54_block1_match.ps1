<# Frozen launcher for BAS-S56: B.5.4 SPSA block 1's centres against the seeds.

   One ON Tune build on both arms (no build offset), the binary SPSA block 1
   played: arm A plays block 1's eight rounded centres from
   tools/results/b54-spsa-block1/tuned-options.txt, arm B the seeds (the
   binary's defaults). 6,000 games (3,000 paired UHO openings) at 3+0.03, one
   thread, Hash 64, no adjudication, tablebases off, on the Colosseum
   match-fixed.toml policy. A diagnostic of what the fit has bought so far,
   not a gate: B.5.4's blocks and B.5.5's gates are unchanged. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engine = Join-Path $PSScriptRoot 'test_engines\basilisk-b54-on-tune-pext-tune-pgo.exe'
$engineSha = 'D3E1B9E9BF8FC87FA291DCB1C0B5BE1BE0CFED8213490722BB827E9C72F659D7'
$theta = Join-Path $PSScriptRoot 'results\b54-spsa-block1\tuned-options.txt'
$thetaSha = '09B3A0552AB1D9FAD7A7FF0773BDDE49367987F3C7E1F0CDE7F938AF318A8DCA'
$surface = Join-Path $PSScriptRoot 'spsa_configs\config_b54.json'

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
    Games = 6000
    EngineA = $engine; NameA = 'Block1'
    EngineB = $engine; NameB = 'Seeds'
    OptionsA = $optionsA
    Seed = 56001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    ExpectRevision = 'a4637b6b4c91fefa0e7909a9120d64afef895cf5'
    ExpectBench = @(3920652, 3920652)
    Dir = Join-Path $PSScriptRoot 'results\b54-block1-match'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
