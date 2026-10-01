<# Frozen launcher for PLAN A.7.2's 4-thread baseline gauntlet. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = @(
    (Join-Path $PSScriptRoot 'test_engines\basilisk-a42-release-pext-pgo.exe')
    'D:\chess\engines\Houdini_3_Pro_x64.exe'
    'D:\chess\engines\Critter_1.6a_64bit.exe'
    'D:\chess\engines\Fritz 16.exe'
    'D:\chess\engines\Deep Rybka 4.1 SSE42 x64.exe'
    'D:\chess\engines\rarog\rarog-v2.4.0-windows-pext-native-pgo.exe'
)
$labels = @('Basilisk 1.10.1', 'Houdini 3', 'Critter 1.6a', 'Fritz 16', 'Rybka 4.1', 'Rarog 2.4.0')
$ratings = @(2994, 3287, 3192, 3173, 3111, 3001)
$hashes = @(
    '18F7D28EF6D5646E718FA8386C1DBB6E2C821A73BDB0CBB4E937BC08A3084FC3'
    'B0E7A631083B9F5B9FF9D5976133FD1C183219C8BB8C70E9CFB58FF7761FF026'
    'A8988AC94B0513B29A4F91DB9618F412B01DACCBAC0B32C2E98205D05B3E63D2'
    '264F2EEDE4C737F938C726E3C91E5CE07460E998110CF197ABFC463DFA4622AE'
    '15901A12179EBD773871AE556CCFC05CAB85C30D014345F6EA64B77F44DC9923'
    'D9C01EE71C742F2BB8EE6BDCD0356FE0B5A4F68043C0DD57CCF7374B6BFD0926'
)
$threadOptions = @('Threads', 'Threads', 'Threads', 'Max CPUs', 'Max CPUs', 'Threads')

$extra = @(
    '--format', 'gauntlet', '--seeds', '1', '--cycles', '200', '--games-per-pair', '2',
    '--max-engine-faults', '0'
)
foreach ($engine in $engines) { $extra += @('--engine', $engine) }
foreach ($label in $labels) { $extra += @('--label', $label) }
foreach ($rating in $ratings) { $extra += @('--rating', "$rating") }
for ($i = 1; $i -lt $ratings.Count; $i++) { $extra += @('--fixed', "$($i + 1):$($ratings[$i])") }
$extra += @('--engine-option', '3:OwnBook=false', '--engine-option', '3:Tablebase Usage=Disable')

$arguments = @{
    Mode = 'gauntlet'
    Dir = (Join-Path $PSScriptRoot 'results\a72-gauntlet-4t')
    Seed = 7102002
    Hash = 64
    Threads = 4
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    ExpectRevision = '3e5294be5db8e0bbe0eb8a1b18476de58c33845b'
    ExpectBench = @(14978465, -1, -1, -1, -1, -1)
    ExpectSha256 = $hashes
    ThreadOptionNames = $threadOptions
    ExtraArgs = $extra
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
