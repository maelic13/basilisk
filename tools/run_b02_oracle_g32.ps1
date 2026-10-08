<# Frozen launcher for PLAN B.0.2 (BAS-S18): the oracle deficit meter G(32).

   The BAS-O05 recipe (tools/run_a73_oracle_g0.ps1) with the oracle's shallow
   move-loop pruning off: the ablate oracle at AblationMask=32 against the same
   Basilisk 1.10.1 arm, 1,000 paired cycles (2,000 games). #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = @(
    (Join-Path $PSScriptRoot 'test_engines\basilisk-a42-release-pext-pgo.exe')
    (Join-Path $PSScriptRoot 'test_engines\oracle-1.10.1-ablate.exe')
)
$labels = @('Basilisk 1.10.1', 'Oracle 1.10.1 HCE mask 32')
$hashes = @(
    '751F6DFF8CB2188F180BC81001E945C237ECEC968B93CA7AF359461726DCF443'
    '0E5155CC226B8089F5332080DA2BED5FBCDBA052002C8C2C16E5C49300BE8644'
)

$extra = @(
    '--format', 'gauntlet', '--seeds', '1', '--cycles', '1000', '--games-per-pair', '2',
    '--max-engine-faults', '0'
)
foreach ($engine in $engines) { $extra += @('--engine', $engine) }
foreach ($label in $labels) { $extra += @('--label', $label) }
# The oracle is the fixed anchor at 1500, so Basilisk's rating minus 1500 is -G(32).
$extra += @('--rating', '1500', '--rating', '1500', '--fixed', '2:1500')
$extra += @('--engine-option', '2:Use Basilisk HCE=true', '--engine-option', '2:AblationMask=32')

$arguments = @{
    Mode = 'gauntlet'
    Dir = (Join-Path $PSScriptRoot 'results\b02-oracle-g32')
    Seed = 20102
    Hash = 64
    Threads = 1
    Concurrency = 14
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    ExpectRevision = '3e5294be5db8e0bbe0eb8a1b18476de58c33845b'
    ExpectBench = @(14978465, -1)
    ExpectSha256 = $hashes
    ThreadOptionNames = @('Threads', 'Threads')
    ExtraArgs = $extra
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
