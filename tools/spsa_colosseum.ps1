<#
.SYNOPSIS
    Run one guarded Colosseum SPSA block from the generated search surface.

.DESCRIPTION
    Production blocks are fixed at 2,000 iterations, 30 games per iteration,
    15 slots, and r_end 0.0031. -SeedFrom starts a fresh schedule from the
    previous completed block's rounded centres. -AllowShortRun exists only for
    instrument smoke tests on a separately supplied temporary tune surface.
#>
param(
    [Parameter(Mandatory)][string]$Engine,
    [Parameter(Mandatory)][long]$ExpectBench,
    [Parameter(Mandatory)][string]$Dir,
    [int]$Seed = 0,
    [string]$SeedFrom = '',
    [int]$Iterations = 2000,
    [int]$Concurrency = 15,
    [string]$Tune = '',
    [switch]$AllowShortRun,
    [switch]$AllowDirtyTree,
    [switch]$AllowBusyHost,
    [double]$MaxHostBusyPercent = 15,
    [string]$ExpectRevision = '',
    [string]$CliPath = "$PSScriptRoot\bin\colosseum-cli.exe",
    [string]$PinPath = "$PSScriptRoot\colosseum\colosseum.pin.json"
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$generatedTune = Join-Path $PSScriptRoot 'spsa_configs\colosseum\search.tune.toml'
$runFile = Join-Path $PSScriptRoot 'spsa_configs\colosseum\search.run.toml'
if (-not $Tune) { $Tune = $generatedTune }
if ($Iterations -ne 2000 -and -not $AllowShortRun) {
    throw 'PLAN rule 7c fixes a production block at 2,000 iterations; -AllowShortRun is for instrument smoke only.'
}
if ($Iterations -ne 2000 -and ([IO.Path]::GetFullPath($Tune) -eq [IO.Path]::GetFullPath($generatedTune))) {
    throw 'A short run requires a temporary tune surface generated for that horizon; do not reuse the 2,000-iteration c_end values.'
}
if ($Concurrency -ne 15 -and -not $AllowShortRun) {
    throw 'The registered production shape is 15 slots; a different concurrency is allowed only for instrument smoke.'
}
foreach ($path in @($Engine, $Tune, $runFile)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}

python (Join-Path $PSScriptRoot 'generate_spsa_surface.py') --check
if ($LASTEXITCODE -ne 0) { throw 'Generated SPSA files are stale; regenerate them before a tune.' }

$hostState = Assert-HarnessHostIdle -MaxBusyPercent $MaxHostBusyPercent -Allow:$AllowBusyHost
$cli = Assert-ColosseumCli -Path $CliPath -PinPath $PinPath
$enginePath = (Resolve-Path -LiteralPath $Engine).Path
$tunePath = (Resolve-Path -LiteralPath $Tune).Path
$manifest = Assert-EngineProvenance -Path $enginePath -Label (Split-Path $enginePath -Leaf) `
    -AllowDirtyTree:$AllowDirtyTree -ExpectRevision $ExpectRevision -ExpectBench $ExpectBench
$advertised = @(Get-EngineUciOptions -Path $enginePath -Detailed)
$surfaceNames = @(
    Get-Content -LiteralPath $tunePath | ForEach-Object {
        if ($_ -match '^name\s*=\s*"([^"]+)"$') { $Matches[1] }
    }
)
if (-not $surfaceNames.Count) { throw "No parameter names found in $tunePath" }
Assert-AdvertisedOptions -Advertised $advertised -Wanted @('Hash=64', 'Threads=1') -Label $enginePath
Assert-AdvertisedOptions -Advertised $advertised -Wanted $surfaceNames -Label $enginePath

$Dir = [IO.Path]::GetFullPath($Dir, (Get-Location).Path)
if ($SeedFrom) { $SeedFrom = [IO.Path]::GetFullPath($SeedFrom, (Get-Location).Path) }
$Seed = New-HarnessSeed -Requested $Seed
$args = @(
    'spsa',
    '--run-file', $runFile,
    $enginePath,
    '--tune', $tunePath,
    '--iterations', "$Iterations",
    '--games-per-iteration', '30',
    '--concurrency', "$Concurrency",
    '--seed', "$Seed",
    '--dir', $Dir
)
if ($SeedFrom) { $args += @('--seed-from', $SeedFrom) }

$resultsDir = Join-Path $PSScriptRoot 'results'
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$label = Split-Path $Dir -Leaf
$dryPath = Join-Path $resultsDir "colosseum_spsa_${label}_${stamp}.dry-run.json"
$notesPath = [IO.Path]::ChangeExtension($dryPath, '.notes.txt')
$manifestPath = Join-Path $resultsDir "colosseum_spsa_${label}_${stamp}.manifest.txt"
$logPath = Join-Path $resultsDir "colosseum_spsa_${label}_${stamp}.log"

& $cli.Path @args --dry-run --json 2> $notesPath > $dryPath
if ($LASTEXITCODE -ne 0) { throw "Colosseum refused the tune dry run (exit $LASTEXITCODE); see $notesPath." }
$dry = Get-Content -LiteralPath $dryPath -Raw | ConvertFrom-Json
$resolved = $dry.resolved_configuration
$violations = [Collections.Generic.List[string]]::new()
if ($dry.command -ne 'spsa') { $violations.Add("command is '$($dry.command)', expected spsa") }
if ([int]$resolved.execution.concurrency -ne $Concurrency) { $violations.Add("concurrency is $($resolved.execution.concurrency), expected $Concurrency") }
if ("$($resolved.execution.placement_policy.mode)" -ne 'auto') { $violations.Add('placement is not auto') }
if ([int]$resolved.settings.iterations -ne $Iterations) { $violations.Add("iterations is $($resolved.settings.iterations), expected $Iterations") }
if ([int]$resolved.settings.games_per_iteration -ne 30) { $violations.Add("games per iteration is $($resolved.settings.games_per_iteration), expected 30") }
foreach ($rule in @('draw', 'resign', 'max_moves')) {
    if ($null -ne $resolved.adjudication.$rule) { $violations.Add("adjudication.$rule is set") }
}
if ($violations.Count) { throw "Resolved tune violates policy: $($violations -join '; ')" }

@(
    "mode:             spsa"
    "engine:           $enginePath"
    "engine_sha256:    $(Get-HarnessSha256 $enginePath)"
    "revision:         $($manifest.GitSha)"
    "bench:            $($manifest.Bench)"
    "tune:             $tunePath"
    "tune_sha256:      $(Get-HarnessSha256 $tunePath)"
    "iterations:       $Iterations"
    "games_per_iter:   30"
    "concurrency:      $Concurrency"
    "seed_from:        $(if ($SeedFrom) { $SeedFrom } else { '(seed defaults)' })"
    "seed:             $Seed"
    "runner:           $($cli.Path)"
    "runner_version:   $($cli.Version)"
    "runner_sha256:    $($cli.Sha256)"
    "host_busy_percent: $($hostState.BusyPercent)"
    "dry_run_sha256:   $(Get-HarnessSha256 $dryPath)"
    "command:          $($cli.Path) $($args -join ' ')"
    "started_utc:      $((Get-Date).ToUniversalTime().ToString('u'))"
) | Set-Content -LiteralPath $manifestPath -Encoding utf8

$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $cli.Path
$start.UseShellExecute = $false
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
foreach ($argument in $args) { $start.ArgumentList.Add($argument) }
$process = [Diagnostics.Process]::Start($start)
try {
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $exit = $process.ExitCode
    $stdoutText = $stdout.GetAwaiter().GetResult()
    $stderrText = $stderr.GetAwaiter().GetResult()
    @($stdoutText, $stderrText) | Where-Object { $_ } | Set-Content -LiteralPath $logPath -Encoding utf8
} finally {
    if (-not $process.HasExited) { $process.Kill($true) }
    $process.Dispose()
}
if ($exit -ne 0) { throw "Colosseum SPSA exited $exit; see $logPath." }
$resultPath = Join-Path $Dir 'result.json'
if (-not (Test-Path -LiteralPath $resultPath)) { throw "Completed tune has no result.json in $Dir." }
$result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
if ($result.driver.status -ne 'completed' -or [int]$result.tuned_result.completed_iterations -ne $Iterations) {
    throw "Tune did not complete the registered block; see $resultPath."
}
$faults = @($result.driver.completed_iterations | ForEach-Object { $_.faults })
$engineFaults = [int](($faults | Measure-Object engine_a -Sum).Sum) + [int](($faults | Measure-Object engine_b -Sum).Sum)
$timeLosses = [int](($faults | Measure-Object time_losses_a -Sum).Sum) + [int](($faults | Measure-Object time_losses_b -Sum).Sum)
$infraFaults = [int](($faults | Measure-Object infrastructure -Sum).Sum)
if ($engineFaults -or $timeLosses -or $infraFaults) {
    throw "Tune completed with faults (engine $engineFaults, time $timeLosses, infrastructure $infraFaults); see $resultPath."
}
$lastGame = [int]$result.driver.completed_iterations[-1].games.last
if ($lastGame -ne 30 * $Iterations) {
    throw "Tune recorded $lastGame games, expected $(30 * $Iterations)."
}
Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
    "completed_utc:    $((Get-Date).ToUniversalTime().ToString('u'))"
    "exit_code:        0"
    "games:            $lastGame"
    "faults:           engine 0, time 0, infrastructure 0"
    "result_sha256:    $(Get-HarnessSha256 $resultPath)"
)
Write-Host "SPSA block complete: $Iterations iterations; result $resultPath"
Write-Host "Apply the movement rule: python tools/spsa_block_rule.py $Dir"
