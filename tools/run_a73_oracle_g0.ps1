<# Frozen launcher for PLAN A.7.3's oracle deficit meter G(0).

   A two-engine gauntlet is a paired match: each cycle plays one opening twice
   with colours reversed. Gauntlet mode is used because it pins an external
   engine (the oracle) by SHA-256, where match mode requires Basilisk build
   manifests on both sides. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = @(
    (Join-Path $PSScriptRoot 'test_engines\basilisk-a42-release-pext-pgo.exe')
    (Join-Path $PSScriptRoot 'test_engines\oracle-1.10.1.exe')
)
$labels = @('Basilisk 1.10.1', 'Oracle 1.10.1 HCE')
$hashes = @(
    '751F6DFF8CB2188F180BC81001E945C237ECEC968B93CA7AF359461726DCF443'
    '5F77850AE1A7102FCB0E4AA88D99AA73E97C22E11FC1AB3C8F258ED578E10725'
)

$extra = @(
    '--format', 'gauntlet', '--seeds', '1', '--cycles', '1500', '--games-per-pair', '2',
    '--max-engine-faults', '0'
)
foreach ($engine in $engines) { $extra += @('--engine', $engine) }
foreach ($label in $labels) { $extra += @('--label', $label) }
# The oracle is the fixed anchor at 1500, so Basilisk's rating minus 1500 is -G(0).
$extra += @('--rating', '1500', '--rating', '1500', '--fixed', '2:1500')
$extra += @('--engine-option', '2:Use Basilisk HCE=true')

$arguments = @{
    Mode = 'gauntlet'
    Dir = (Join-Path $PSScriptRoot 'results\a73-oracle-g0')
    Seed = 7103003
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
