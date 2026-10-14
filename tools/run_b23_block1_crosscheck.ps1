<# Frozen launcher for BAS-S24's backup-path cross-check (PROCESS, Harness).

   The same two arms as tools/run_b23_block1_match.ps1 (block 1's 49 rounded
   centres against the seeds, one ON Tune build) on fastchess through
   tools/sprt.ps1 -Mode fixed: 2,000 games at 3+0.03, 1T, Hash 64, UHO,
   no adjudication, a fresh seed. Read as "do the two instruments agree
   inside their intervals", never as a second chance at a verdict. #>
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$sprt = Join-Path $PSScriptRoot 'sprt.ps1'
$engine = Join-Path $PSScriptRoot 'test_engines\basilisk-b23b-on-tune-pext-tune-pgo.exe'
$engineSha = 'A900EFD2AC77CD3F4178A1161A4FA5091A313DA5F4CCCE4EA799043C99CA2AEB'
$theta = Join-Path $PSScriptRoot 'results\b23-spsa-block1\tuned-options.txt'
$thetaSha = '5DB77308154B088BE3E7D641F38776DEAA7390DDFB30D5E666A58C1E231282A1'
$surface = Join-Path $PSScriptRoot 'spsa_configs\config_core.json'

foreach ($pin in @(@($engine, $engineSha), @($theta, $thetaSha))) {
    $actual = Get-HarnessSha256 $pin[0]
    if ($actual -ne $pin[1]) { throw "PROVENANCE MISMATCH - $($pin[0]) is $actual, not $($pin[1])." }
}
$names = @((Get-Content -LiteralPath $surface -Raw | ConvertFrom-Json).PSObject.Properties.Name)
$optionsA = @(
    Get-Content -LiteralPath $theta | ForEach-Object {
        if ($_ -notmatch '^setoption name (\S+) value (-?\d+)$') { throw "Unexpected line in ${theta}: $_" }
        if ($names -notcontains $Matches[1]) { throw "$($Matches[1]) is not on the tuned surface." }
        "$($Matches[1])=$($Matches[2])"
    }
)
if ($optionsA.Count -ne $names.Count) { throw "Block 1 sets $($optionsA.Count) options; the surface has $($names.Count)." }

& $sprt -EngineA $engine -EngineB $engine -NameA Block1 -NameB Seeds -Mode fixed -Games 2000 `
    -OptionsA $optionsA -Seed 24002 `
    -ExpectRevision 'b5983c6dfa08c136231d636d9bcbd5eb7cecfc5f' -ExpectBench 6096792, 6096792
if ($LASTEXITCODE) { exit $LASTEXITCODE }
