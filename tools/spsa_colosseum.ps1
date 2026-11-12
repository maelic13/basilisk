<#
.SYNOPSIS
    Run one guarded Colosseum SPSA tune under PLAN rule 7c: one run to a
    registered ceiling, read at checkpoints, stopped when its gradients are gone.

.DESCRIPTION
    The tune file is the rule 7c surface from
    `tools/generate_spsa_surface.py --rule7c <surface> --ceiling <N> --output <file>`,
    and its header must name the same ceiling. Production shape: ceiling 6,000
    iterations, 30 games an iteration on 15 slots, r_end 0.00155, the first
    checkpoint at 2,000 and one every 1,000 after it.

    The run is played a segment at a time with Colosseum's --stop-after-iteration.
    At each checkpoint the wrapper proves the pause (the history's completed
    count equals the checkpoint), refuses any engine, time or infrastructure
    fault, then applies `tools/spsa_read.py --record`:
      CONTINUE      the next segment runs;
      STOP_QUIET    the tune stops; theta is the checkpoint's rounded centres;
      STOP_CEILING  the ceiling is reached; theta likewise;
      STOP_RAIL     the tune stops for a new registration of the range;
      FAULT         the tune stops for the maintainer.
    After a FAULT the maintainer may accept the recorded faults, once they are
    analysed, with `python tools/spsa_read.py <dir> --accept-faults "<reason>"`.
    The same command then reads that checkpoint again for its real verdict and
    resumes or stops by it; a later fault stops the tune again.
    A file named STOP in the run directory is the maintainer's stop: at the
    next checkpoint the tune stops there and that checkpoint's centres are
    theta. Running the same command again resumes an interrupted run from its
    directory at its recorded seed; a stopped tune is never extended.

    -AllowShortRun exists only for instrument smoke on a separately generated
    temporary surface: it lifts the production shape (ceiling, checkpoints,
    games per iteration, concurrency). -DryRun plays nothing.
#>
param(
    [Parameter(Mandatory)][string]$Engine,
    [Parameter(Mandatory)][long]$ExpectBench,
    [Parameter(Mandatory)][string]$Dir,
    [Parameter(Mandatory)][string]$Tune,
    [int]$Seed = 0,
    [int]$Ceiling = 6000,
    [double]$REnd = 0.00155,
    [int]$FirstCheckpoint = 2000,
    [int]$CheckpointEvery = 1000,
    [int]$Window = 1000,
    [int]$GamesPerIteration = 30,
    [int]$Concurrency = 15,
    [switch]$AllowShortRun,
    [switch]$AllowDirtyTree,
    [switch]$AllowBusyHost,
    [switch]$DryRun,
    [double]$MaxHostBusyPercent = 15,
    [string]$ExpectRevision = '',
    [string]$CliPath = "$PSScriptRoot\bin\colosseum-cli.exe",
    [string]$PinPath = "$PSScriptRoot\colosseum\colosseum.pin.json",
    [string]$Python = 'python'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$runFile = Join-Path $PSScriptRoot 'spsa_configs\colosseum\search.run.toml'
$reader = Join-Path $PSScriptRoot 'spsa_read.py'
$production = @{ Ceiling = 6000; REnd = 0.00155; FirstCheckpoint = 2000; CheckpointEvery = 1000; Window = 1000; GamesPerIteration = 30; Concurrency = 15 }
if (-not $AllowShortRun) {
    foreach ($name in $production.Keys) {
        $value = Get-Variable -Name $name -ValueOnly
        if ($value -ne $production[$name]) {
            throw "PLAN rule 7c's production shape has $name $($production[$name]), not $value; -AllowShortRun is for instrument smoke only."
        }
    }
}
if ($Ceiling -lt $FirstCheckpoint -or $FirstCheckpoint -lt $Window -or $CheckpointEvery -lt 1 -or $Window -lt 1) {
    throw "Checkpoints must satisfy window <= first checkpoint <= ceiling, with a positive spacing."
}
foreach ($path in @($Engine, $Tune, $runFile, $reader)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
$tuneText = Get-Content -LiteralPath $Tune -Raw
if ($tuneText -notmatch 'rule 7c ceiling N = (\d+);' -or [int]$Matches[1] -ne $Ceiling) {
    throw "The tune file is not a rule 7c surface for a ceiling of $Ceiling; generate it with generate_spsa_surface.py --rule7c <surface> --ceiling $Ceiling."
}
python (Join-Path $PSScriptRoot 'generate_spsa_surface.py') --check | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Generated SPSA files are stale; regenerate them before a tune.' }

# The checkpoints, then the ceiling if it is not on the grid.
$checkpoints = [System.Collections.Generic.List[int]]::new()
for ($k = $FirstCheckpoint; $k -lt $Ceiling; $k += $CheckpointEvery) { $checkpoints.Add($k) }
$checkpoints.Add($Ceiling)

$hostState = Assert-HarnessHostIdle -MaxBusyPercent $MaxHostBusyPercent -Allow:($AllowBusyHost -or $DryRun)
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
$storedConfig = Join-Path $Dir 'resolved-config.json'
if (Test-Path -LiteralPath $storedConfig) {
    $storedSeed = [int](Get-Content -LiteralPath $storedConfig -Raw | ConvertFrom-Json).master_seed
    if ($Seed -eq 0) { $Seed = $storedSeed; Write-Host "  Resuming at recorded seed $Seed." }
    elseif ($Seed -ne $storedSeed) { throw "$Dir records seed $storedSeed; -Seed $Seed would change a resumed run." }
}
$Seed = New-HarnessSeed -Requested $Seed
$baseArgs = @(
    'spsa',
    '--run-file', $runFile,
    $enginePath,
    '--tune', $tunePath,
    '--iterations', "$Ceiling",
    '--r-end', ([string]::Format([Globalization.CultureInfo]::InvariantCulture, '{0}', $REnd)),
    '--games-per-iteration', "$GamesPerIteration",
    '--concurrency', "$Concurrency",
    '--seed', "$Seed",
    '--dir', $Dir
)

$resultsDir = Join-Path $PSScriptRoot 'results'
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$label = Split-Path $Dir -Leaf
$dryPath = Join-Path $resultsDir "colosseum_spsa_${label}_${stamp}.dry-run.json"
$notesPath = [IO.Path]::ChangeExtension($dryPath, '.notes.txt')
$manifestPath = Join-Path $resultsDir "colosseum_spsa_${label}_${stamp}.manifest.txt"

& $cli.Path @baseArgs --stop-after-iteration "$($checkpoints[0])" --dry-run --json 2> $notesPath > $dryPath
if ($LASTEXITCODE -ne 0) { throw "Colosseum refused the tune dry run (exit $LASTEXITCODE); see $notesPath." }
$dry = Get-Content -LiteralPath $dryPath -Raw | ConvertFrom-Json
$resolved = $dry.resolved_configuration
$violations = [Collections.Generic.List[string]]::new()
if ($dry.command -ne 'spsa') { $violations.Add("command is '$($dry.command)', expected spsa") }
if ([int]$resolved.execution.concurrency -ne $Concurrency) { $violations.Add("concurrency is $($resolved.execution.concurrency), expected $Concurrency") }
if ("$($resolved.execution.placement_policy.mode)" -ne 'auto') { $violations.Add('placement is not auto') }
if ([int]$resolved.settings.iterations -ne $Ceiling) { $violations.Add("iterations is $($resolved.settings.iterations), expected the ceiling $Ceiling") }
if ([int]$resolved.settings.games_per_iteration -ne $GamesPerIteration) { $violations.Add("games per iteration is $($resolved.settings.games_per_iteration), expected $GamesPerIteration") }
if ([Math]::Abs([double]$resolved.r_end - $REnd) -gt 1e-12) { $violations.Add("r_end is $($resolved.r_end), expected $REnd") }
if ($null -ne $resolved.final_window_percent) { $violations.Add('the estimator is a window average; rule 7c takes the checkpoint centres') }
if ("$($resolved.engine_processes)" -ne 'per-slot') { $violations.Add("engine processes are '$($resolved.engine_processes)', expected per-slot") }
foreach ($rule in @('draw', 'resign', 'max_moves')) {
    if ($null -ne $resolved.adjudication.$rule) { $violations.Add("adjudication.$rule is set") }
}
if ($violations.Count) { throw "Resolved tune violates policy: $($violations -join '; ')" }

$lines = [Collections.Generic.List[string]]::new()
@(
    "mode:             spsa (rule 7c: one run, read at checkpoints)"
    "engine:           $enginePath"
    "engine_sha256:    $(Get-HarnessSha256 $enginePath)"
    "revision:         $($manifest.GitSha)"
    "bench:            $($manifest.Bench)"
    "tune:             $tunePath"
    "tune_sha256:      $(Get-HarnessSha256 $tunePath)"
    "ceiling:          $Ceiling iterations"
    "r_end:            $REnd"
    "checkpoints:      $($checkpoints -join ', ') (window $Window)"
    "games_per_iter:   $GamesPerIteration"
    "concurrency:      $Concurrency"
    "seed:             $Seed"
    "runner:           $($cli.Path)"
    "runner_version:   $($cli.Version)"
    "runner_sha256:    $($cli.Sha256)"
    "host_busy_percent: $($hostState.BusyPercent)"
    "dry_run_sha256:   $(Get-HarnessSha256 $dryPath)"
    "command:          $($cli.Path) $($baseArgs -join ' ') --stop-after-iteration <checkpoint>"
) | ForEach-Object { $lines.Add($_) }
if ($DryRun) {
    $lines.Add('dry_run:          accepted; nothing played')
    $lines | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Write-Host "Dry run accepted. Checkpoints: $($checkpoints -join ', '). Manifest: $manifestPath"
    exit 0
}

function Get-Completed {
    if (-not (Test-Path -LiteralPath (Join-Path $Dir 'checkpoint.json'))) { return 0 }
    $text = & $cli.Path spsa history $Dir --json
    if ($LASTEXITCODE -ne 0) { throw "spsa history failed on $Dir (exit $LASTEXITCODE)." }
    [int](($text -join "`n") | ConvertFrom-Json).report.completed_iterations
}

function Test-Recorded([int]$At) {
    if (-not (Test-Path -LiteralPath $readsPath)) { return $false }
    [bool](@(Get-Content -LiteralPath $readsPath | Where-Object { $_.Trim() } |
        ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { [int]$_.at -eq $At }).Count)
}

function Read-Checkpoint([int]$At, [switch]$WriteTheta) {
    $readArgs = @($reader, $Dir, '--at', "$At", '--window', "$Window", '--first', "$FirstCheckpoint",
        '--every', "$CheckpointEvery", '--cli', $cli.Path, '--record', '--json')
    if ($WriteTheta) { $readArgs += '--write-theta' }
    $text = & $Python @readArgs
    if ($LASTEXITCODE -ne 0) { throw "spsa_read.py failed at iteration $At (exit $LASTEXITCODE)." }
    ($text -join "`n") | ConvertFrom-Json
}

$readsPath = Join-Path $Dir 'spsa-read.jsonl'
$acceptHint = "After the analysis the maintainer may accept the recorded faults: python tools/spsa_read.py `"$Dir`" --accept-faults `"<reason>`"; then run this command again."
if (Test-Path -LiteralPath $readsPath) {
    $last = Get-Content -LiteralPath $readsPath | Where-Object { $_.Trim() } | Select-Object -Last 1 | ConvertFrom-Json
    # A FAULT read stops the tune for the maintainer. Once its faults are accepted the
    # reader gives that checkpoint its real verdict, so the run goes on from that read;
    # whether the acceptance covers the run's faults is the reader's judgement.
    $faultAccepted = "$($last.verdict)" -eq 'FAULT' -and (Test-Path -LiteralPath (Join-Path $Dir 'faults-accepted.json'))
    if (-not $faultAccepted) {
        if ("$($last.verdict)" -eq 'FAULT') { throw "STOPPED - $Dir read FAULT at iteration $($last.at). $acceptHint" }
        if ("$($last.verdict)" -ne 'CONTINUE' -or (Test-Path -LiteralPath (Join-Path $Dir "theta-$($last.at).txt"))) {
            throw "STOPPED - $Dir ended at iteration $($last.at) with $($last.verdict); a stopped tune is never extended."
        }
    }
}
$lines.Add("started_utc:      $((Get-Date).ToUniversalTime().ToString('u'))")
$lines | Set-Content -LiteralPath $manifestPath -Encoding utf8

$verdict = $null
foreach ($checkpoint in $checkpoints) {
    $completed = Get-Completed
    if ($completed -gt $checkpoint) {
        if (-not (Test-Recorded $checkpoint)) { throw "The run is past the checkpoint $checkpoint, which was never read." }
        continue
    }
    if ($completed -lt $checkpoint) {
        $logPath = Join-Path $resultsDir "colosseum_spsa_${label}_${stamp}_to$checkpoint.log"
        $run = @{}
        try {
            # The runner's progress is passed through live; the log keeps both streams.
            $streamed = Invoke-ColosseumStreamed -Path $cli.Path -Arguments ($baseArgs + @('--stop-after-iteration', "$checkpoint")) -State $run
            @($streamed.Stdout, $streamed.Stderr) | Where-Object { $_ } | Set-Content -LiteralPath $logPath -Encoding utf8
        } finally {
            if ($run.Process) {
                if (-not $run.Process.HasExited) { $run.Process.Kill($true) }
                $run.Process.Dispose()
            }
        }
        # 6 is the runner's clean pause at --stop-after-iteration; 0 its completed horizon.
        $expectedExit = if ($checkpoint -eq $Ceiling) { 0 } else { 6 }
        if ($streamed.ExitCode -ne $expectedExit) { throw "Colosseum SPSA exited $($streamed.ExitCode) before iteration $checkpoint (expected $expectedExit); see $logPath." }
        $completed = Get-Completed
        if ($completed -ne $checkpoint) { throw "The run paused at iteration $completed, not at the checkpoint $checkpoint." }
    }
    $stopFile = Join-Path $Dir 'STOP'
    $read = Read-Checkpoint -At $checkpoint
    $verdict = "$($read.verdict)"
    if ($verdict -eq 'CONTINUE' -and (Test-Path -LiteralPath $stopFile)) {
        $read = Read-Checkpoint -At $checkpoint -WriteTheta
        $verdict = 'STOP_MAINTAINER'
    }
    Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value ("checkpoint {0,5}: {1}; max |z| {2} (line {3}); excess {4} SD; gain {5} (null SD {6}); forecast {7} +/- {8}" -f
        $checkpoint, $verdict, $read.max_abs_z, $read.familywise_line, $read.excess_sd, $read.gain.elo, $read.gain.null_sd, $read.forecast.elo, $read.forecast.sd)
    Write-Host "Checkpoint ${checkpoint}: $verdict (max |z| $($read.max_abs_z) against $($read.familywise_line); excess $($read.excess_sd) SD)"
    if ($verdict -ne 'CONTINUE') { break }
}

Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
    "completed_utc:    $((Get-Date).ToUniversalTime().ToString('u'))"
    "verdict:          $verdict"
)
switch ($verdict) {
    { $_ -in 'STOP_QUIET', 'STOP_CEILING', 'STOP_MAINTAINER' } {
        Write-Host "Tune stopped ($verdict) at iteration $($read.at). Theta: $(Join-Path $Dir "theta-$($read.at).txt"); the read: $(Join-Path $Dir 'spsa-read.txt')"
        exit 0
    }
    'STOP_RAIL' { throw "RAIL - $($read.rails -join ', ') at a bound with z pointing outward past the familywise line at iteration $($read.at): register the range again; no theta." }
    'FAULT' { throw "FAULT - the run recorded faults by iteration $($read.at): $(($read.faults.PSObject.Properties | Where-Object Value | ForEach-Object { "$($_.Name) $($_.Value)" }) -join ', '). The maintainer decides. $acceptHint" }
    default { throw "Unexpected verdict '$verdict' at a registered checkpoint." }
}
