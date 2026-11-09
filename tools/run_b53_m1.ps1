<# Frozen launcher for PLAN B.5.3's M1 (BAS-R10): the fail-high stop's firing read on the arm.
   Each game of each corpus is replayed by tools/diag/replay_games.py at the game's own clocks, one
   thread, Hash 64, for each column below, then tools/diag/replay_read.py turns the rows into M1's
   outputs and the gate corpus into the stop packet's rule. Every binary and PGN is pinned by SHA-256.

   Corpus gate       the first 300 games of the B.4.7 gate (a) PGN, 3+0.03
   Corpus direction  the first 100 of its 10+0.1 direction run

   Columns           arm-1x, arm-2x, arm-0  the arm's Tune build, B5FailHighStop 100, 200, 0
                     off-1x                 the OFF arm's Tune build, B5FailHighStop 100
                     diag-arm, diag-off     each arm's Diag build at the default stop
   -OffStop0 adds    off-0                  the OFF arm's Tune build, B5FailHighStop 0: A6's OFF-arm
                                            time removed needs it, and the registration does not list it

   The columns of a corpus run together on an otherwise idle host (-Parallel), which is lighter than the
   14 game slots that played these clocks, and each arm meets the same load. About an hour in all.

   -DryRun replays three games a column to <results>\b53-m1-dry, replaced each time.
   -CheckOnly runs the guards and stops, so they can be tested. A refusal throws, and nothing is played. #>
param(
    [ValidateSet('all', 'gate', 'direction')][string]$Corpus = 'all',
    [switch]$OffStop0,
    [switch]$DryRun,
    [switch]$CheckOnly,
    [ValidateRange(1, 14)][int]$Parallel = 7,
    [string]$ResultsRoot = '',
    [string]$EnginesDir = '',
    [string]$ReplayScript = '',
    [switch]$AllowBusyHost
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$results = if ($ResultsRoot) { $ResultsRoot } else { Join-Path $PSScriptRoot 'results' }
$engines = if ($EnginesDir) { $EnginesDir } else { Join-Path $PSScriptRoot 'test_engines' }
$replay = if ($ReplayScript) { $ReplayScript } else { Join-Path $PSScriptRoot 'diag\replay_games.py' }
$reader = Join-Path $PSScriptRoot 'diag\replay_read.py'
$side = 'basilisk-b47-on'

$binaries = @{
    OnTune  = @{ Name = 'basilisk-b53-on-tune-pext-tune-pgo.exe';  Sha = 'D8D9BC6765E19A1645A95763572CD2E8E97099CCEB3C2928346A722EBBE90328' }
    OffTune = @{ Name = 'basilisk-b53-off-tune-pext-tune-pgo.exe'; Sha = '6DD0B156B692A642E717FACEA0B55B4677D428DF57CB2D3CB6B14BF9AA3B2A4B' }
    OnDiag  = @{ Name = 'basilisk-b53-on-diag-pext-diag-pgo.exe';  Sha = 'DD0258A8D891D49BF9CC43ECACAB6C4E7A2FF97BEBABEF1DD57C82BCA88129D2' }
    OffDiag = @{ Name = 'basilisk-b53-off-diag-pext-diag-pgo.exe'; Sha = '63C9C26FAAA5D84685D4244B5238805A2C2D9DD75E4A307A17905BF0E6413EA7' }
}
$corpora = [ordered]@{
    gate      = @{ Pgn = Join-Path $PSScriptRoot 'results\b47-gate-a\games.pgn';        Sha = '18AEA0CD94AF2119'; Games = 300 }
    direction = @{ Pgn = Join-Path $PSScriptRoot 'results\b47-direction-ltc\games.pgn'; Sha = '6931997C5EE5B5CF'; Games = 100 }
}
$columns = [ordered]@{
    'arm-1x'   = @{ Build = 'OnTune';  Options = @('B5FailHighStop=100') }
    'arm-2x'   = @{ Build = 'OnTune';  Options = @('B5FailHighStop=200') }
    'arm-0'    = @{ Build = 'OnTune';  Options = @('B5FailHighStop=0') }
    'off-1x'   = @{ Build = 'OffTune'; Options = @('B5FailHighStop=100') }
    'diag-arm' = @{ Build = 'OnDiag';  Options = @() }
    'diag-off' = @{ Build = 'OffDiag'; Options = @() }
}
if ($OffStop0) { $columns['off-0'] = @{ Build = 'OffTune'; Options = @('B5FailHighStop=0') } }
$chosen = if ($Corpus -eq 'all') { @($corpora.Keys) } else { @($Corpus) }
$outRoot = Join-Path $results $(if ($DryRun) { 'b53-m1-dry' } else { 'b53-m1' })

# The guards.
foreach ($key in $binaries.Keys) {
    $path = Join-Path $engines $binaries[$key].Name
    if (-not (Test-Path -LiteralPath $path)) { throw "PROVENANCE MISMATCH - no binary at $path." }
    $actual = Get-HarnessSha256 $path
    if ($actual -ne $binaries[$key].Sha) { throw "PROVENANCE MISMATCH - $path is $actual, not $($binaries[$key].Sha)." }
}
foreach ($name in $chosen) {
    $actual = Get-HarnessSha256 $corpora[$name].Pgn
    if (-not $actual.StartsWith($corpora[$name].Sha)) { throw "PROVENANCE MISMATCH - $($corpora[$name].Pgn) is $actual, not $($corpora[$name].Sha)..." }
}
$null = Assert-HarnessHostIdle -Allow:$AllowBusyHost
if (-not $DryRun) {
    foreach ($name in $chosen) {
        foreach ($column in $columns.Keys) {
            $rows = Join-Path $outRoot "$name\$column.jsonl"
            if (Test-Path -LiteralPath $rows) { throw "STALE RESULT - $rows exists; a replay is not resumed or overwritten. Move it first." }
        }
    }
}
if ($CheckOnly) { Write-Host 'GUARDS OK - M1 may run.'; exit 0 }
if ($DryRun -and (Test-Path -LiteralPath $outRoot)) { Remove-Item -LiteralPath $outRoot -Recurse -Force }

foreach ($name in $chosen) {
    $set = $corpora[$name]
    $games = if ($DryRun) { 3 } else { $set.Games }
    $dir = Join-Path $outRoot $name
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Write-Host "=== $name, $games games a column, $($columns.Count) columns $(Get-Date -Format s)"
    $jobs = foreach ($column in $columns.Keys) {
        [pscustomobject]@{
            Column = $column
            Engine = Join-Path $engines $binaries[$columns[$column].Build].Name
            Options = $columns[$column].Options
            Rows = Join-Path $dir "$column.jsonl"
            Log = Join-Path $dir "$column.log"
        }
    }
    $outcomes = $jobs | ForEach-Object -ThrottleLimit $Parallel -Parallel {
        $job = $_
        $arguments = @($using:replay, '--engine', $job.Engine, '--pgn', ($using:set).Pgn, '--side', $using:side,
            '--games', "$($using:games)", '--out', $job.Rows)
        foreach ($option in $job.Options) { $arguments += @('--option', $option) }
        & python @arguments *> $job.Log
        [pscustomobject]@{ Column = $job.Column; Exit = $LASTEXITCODE }
    }
    $failed = @($outcomes | Where-Object { $_.Exit -ne 0 })
    foreach ($outcome in $outcomes) { Write-Host "  $($outcome.Column): exit $($outcome.Exit)" }
    if ($failed.Count) { throw "REPLAY FAILED - $($failed.Column -join ', ') on ${name}; see $dir. The reader did not run." }

    $read = @($reader)
    foreach ($column in $columns.Keys) { $read += @('--column', "$column=$(Join-Path $dir "$column.jsonl")") }
    $read += @('--removed-against', 'arm-1x=arm-0', '--removed-against', 'arm-2x=arm-0')
    if ($OffStop0) { $read += @('--removed-against', 'off-1x=off-0') }
    if ($name -eq 'gate') { $read += @('--rule', 'arm-1x') }
    if ($name -eq 'gate' -and -not $DryRun) { $read += @('--split', '100') }
    $read += @('--out', (Join-Path $dir 'm1.json'))
    & python @read *> (Join-Path $dir 'm1.txt')
    if ($LASTEXITCODE) { throw "READER FAILED - exit $LASTEXITCODE on ${name}; see $dir\m1.txt." }
    Get-Content -LiteralPath (Join-Path $dir 'm1.txt') | Select-String -Pattern '^(arm-|off-|the rule)' | ForEach-Object { Write-Host "  $($_.Line)" }
}
Write-Host "M1 DONE $(Get-Date -Format s): $outRoot"
