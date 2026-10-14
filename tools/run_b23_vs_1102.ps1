<# Frozen launcher for BAS-S25: the current state against Basilisk 1.10.2.

   Engine 1 is the ON Tune build (engine 3eea3c5, Cluster 1 with Amendment 5)
   playing SPSA block 1's 49 rounded centres; engine 2 is the 1.10.2 release
   binary (built from 2ec445bb, whose engine is v1.10.2's), anchored at 1500,
   so engine 1's rating minus 1500 is its Elo over 1.10.2. A gauntlet, because
   the two binaries differ in flavour (Tune against Release) and the
   two-arm modes pair only equal flavours. 1,000 paired cycles (2,000 games)
   at 3+0.03, one thread, Hash 64, UHO, no adjudication, tablebases off. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$current = Join-Path $PSScriptRoot 'test_engines\basilisk-b23b-on-tune-pext-tune-pgo.exe'
$release = Join-Path $PSScriptRoot 'test_engines\basilisk-1.10.2-rc-pext-pgo.exe'
$hashes = @(
    'A900EFD2AC77CD3F4178A1161A4FA5091A313DA5F4CCCE4EA799043C99CA2AEB'
    '1234F3F68A6C9C55578DA3CE18086B2FB499C97986A24022CCD8C745626972F1'
)
$theta = Join-Path $PSScriptRoot 'results\b23-spsa-block1\tuned-options.txt'
$thetaSha = '5DB77308154B088BE3E7D641F38776DEAA7390DDFB30D5E666A58C1E231282A1'
$surface = Join-Path $PSScriptRoot 'spsa_configs\config_core.json'

$actual = Get-HarnessSha256 $theta
if ($actual -ne $thetaSha) { throw "PROVENANCE MISMATCH - $theta is $actual, not $thetaSha." }
$names = @((Get-Content -LiteralPath $surface -Raw | ConvertFrom-Json).PSObject.Properties.Name)
$options = @(
    Get-Content -LiteralPath $theta | ForEach-Object {
        if ($_ -notmatch '^setoption name (\S+) value (-?\d+)$') { throw "Unexpected line in ${theta}: $_" }
        if ($names -notcontains $Matches[1]) { throw "$($Matches[1]) is not on the tuned surface." }
        "$($Matches[1])=$($Matches[2])"
    }
)
if ($options.Count -ne $names.Count) { throw "Block 1 sets $($options.Count) options; the surface has $($names.Count)." }

$extra = @(
    '--format', 'gauntlet', '--seeds', '1', '--cycles', '1000', '--games-per-pair', '2',
    '--max-engine-faults', '0',
    '--engine', $current, '--engine', $release,
    '--label', 'Basilisk B23 block 1', '--label', 'Basilisk 1.10.2',
    '--rating', '1500', '--rating', '1500', '--fixed', '2:1500'
)
foreach ($option in $options) { $extra += @('--engine-option', "1:$option") }

$arguments = @{
    Mode = 'gauntlet'
    Dir = (Join-Path $PSScriptRoot 'results\b23-vs-1102')
    Seed = 25001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    ExpectBench = @(6096792, 14978465)
    ExpectSha256 = $hashes
    ThreadOptionNames = @('Threads', 'Threads')
    ExtraArgs = $extra
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
