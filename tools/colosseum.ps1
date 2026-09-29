<#
.SYNOPSIS
    Run a guarded Basilisk match through the pinned Colosseum CLI.

.DESCRIPTION
    The committed TOML files own common conditions. This wrapper proves the
    host, runner, engine provenance, advertised options and resolved policy
    before play, writes a complete manifest, then checks the runner status and
    independently recounts the PGN.
#>
param(
    [ValidateSet('sprt', 'match', 'calibrate', 'gauntlet')][string]$Mode = 'sprt',
    [ValidateSet('default', 'removal', 'repair', 'wide')][string]$Bracket = 'default',
    [string]$EngineA = '',
    [string]$EngineB = '',
    [string]$NameA = 'New',
    [string]$NameB = 'Base',
    [Parameter(Mandatory)][string]$Dir,
    [int]$MaxPairs = 0,
    [int]$Games = 0,
    [int]$Seed = 0,
    [string[]]$OptionsA = @(),
    [string[]]$OptionsB = @(),
    [int]$Hash = 64,
    [int]$Threads = 1,
    [int]$BaseMs = 3000,
    [int]$IncrementMs = 30,
    [int]$MarginMs = 20,
    [int]$Concurrency = 0,
    [string]$ExpectRevision = '',
    [long[]]$ExpectBench = @(),
    [switch]$AllowDirtyTree,
    [switch]$AllowBusyHost,
    [switch]$DryRun,
    [double]$MaxHostBusyPercent = 15,
    [double]$TimeLossRateCeiling = 0.5,
    [string[]]$ExtraArgs = @(),
    [string]$RunFile = '',
    [string]$Book = "$PSScriptRoot\books\UHO_Lichess_4852_v1.epd",
    [string]$CliPath = "$PSScriptRoot\bin\colosseum-cli.exe",
    [string]$PinPath = "$PSScriptRoot\colosseum\colosseum.pin.json"
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$splitOptions = {
    param($items)
    ,@($items | ForEach-Object { $_ -split ',' } |
        ForEach-Object { $_.Trim().Trim('"') } | Where-Object { $_ })
}
$OptionsA = & $splitOptions $OptionsA
$OptionsB = & $splitOptions $OptionsB

$twoArm = $Mode -in @('sprt', 'match', 'calibrate')
if ($twoArm -and (-not $EngineA -or -not $EngineB)) { throw "-Mode $Mode requires -EngineA and -EngineB." }
if ($Mode -eq 'sprt' -and $MaxPairs -le 0) { throw '-MaxPairs is required for an SPRT.' }
if ($Mode -ne 'sprt' -and $PSBoundParameters.ContainsKey('MaxPairs')) { throw "-Mode $Mode ignores -MaxPairs." }
if ($Mode -eq 'sprt' -and $PSBoundParameters.ContainsKey('Games')) { throw '-Mode sprt ignores -Games; use -MaxPairs.' }
if ($Mode -in @('match', 'calibrate') -and $Games -gt 0 -and ($Games -lt 2 -or ($Games % 2))) {
    throw '-Games must be a positive even number.'
}
if ($Mode -eq 'gauntlet' -and $ExtraArgs.Count -eq 0) { throw '-Mode gauntlet requires participants in -ExtraArgs.' }
if ($Mode -ne 'sprt' -and $PSBoundParameters.ContainsKey('Bracket')) { throw "-Mode $Mode ignores -Bracket." }
if ($Hash -lt 1 -or $Threads -lt 1) { throw '-Hash and -Threads must be positive.' }

Write-Host ''
Write-Host 'Preflight:'
$hostState = Assert-HarnessHostIdle -MaxBusyPercent $MaxHostBusyPercent -Allow:$AllowBusyHost
$cli = Assert-ColosseumCli -Path $CliPath -PinPath $PinPath
Write-Host "  Runner pin OK: $($cli.Version), $($cli.Sha256.Substring(0, 8))..."

$defaults = @{
    sprt = Join-Path $PSScriptRoot "colosseum\sprt-$Bracket.toml"
    match = Join-Path $PSScriptRoot 'colosseum\match-fixed.toml'
    calibrate = Join-Path $PSScriptRoot 'colosseum\calibrate-null.toml'
    gauntlet = Join-Path $PSScriptRoot 'colosseum\gauntlet.toml'
}
if (-not $RunFile) { $RunFile = $defaults[$Mode] }
if (-not (Test-Path -LiteralPath $RunFile -PathType Leaf)) { throw "Run file not found: $RunFile" }
$RunFile = (Resolve-Path -LiteralPath $RunFile).Path
if (-not (Test-Path -LiteralPath $Book -PathType Leaf)) { throw "Book not found: $Book" }
$Book = (Resolve-Path -LiteralPath $Book).Path

$engines = @()
if ($twoArm) {
    $engines = @(
        [pscustomobject]@{ Path = $EngineA; Label = $NameA; Options = $OptionsA }
        [pscustomobject]@{ Path = $EngineB; Label = $NameB; Options = $OptionsB }
    )
} else {
    $engineIndexes = for ($i = 0; $i -lt $ExtraArgs.Count; $i++) { if ($ExtraArgs[$i] -eq '--engine') { $i + 1 } }
    foreach ($index in $engineIndexes) {
        if ($index -ge $ExtraArgs.Count) { throw "--engine in -ExtraArgs has no path." }
        $engines += [pscustomobject]@{ Path = $ExtraArgs[$index]; Label = "participant$($engines.Count + 1)"; Options = @() }
    }
    if ($engines.Count -lt 2) { throw 'A gauntlet requires at least two --engine entries.' }
}
foreach ($arm in $engines) {
    if (-not (Test-Path -LiteralPath $arm.Path -PathType Leaf)) { throw "Engine not found: $($arm.Path)" }
    $arm.Path = (Resolve-Path -LiteralPath $arm.Path).Path
}
if ($ExpectBench.Count -ne $engines.Count) {
    throw "-ExpectBench is required once per engine; got $($ExpectBench.Count), expected $($engines.Count)."
}

$manifests = @{}
for ($i = 0; $i -lt $engines.Count; $i++) {
    $arm = $engines[$i]
    $manifests[$arm.Label] = Assert-EngineProvenance -Path $arm.Path -Label $arm.Label `
        -AllowDirtyTree:$AllowDirtyTree -ExpectRevision $ExpectRevision -ExpectBench $ExpectBench[$i]
    $details = @(Get-EngineUciOptions -Path $arm.Path -Detailed)
    Assert-AdvertisedOptions -Advertised $details -Wanted @("Hash=$Hash", "Threads=$Threads") -Label $arm.Label
    Assert-AdvertisedOptions -Advertised $details -Wanted $arm.Options -Label $arm.Label
}
if ($twoArm) {
    Assert-EngineArmEquality -ManifestA $manifests[$NameA] -ManifestB $manifests[$NameB] -LabelA $NameA -LabelB $NameB
    $shaA = Get-HarnessSha256 $engines[0].Path
    $shaB = Get-HarnessSha256 $engines[1].Path
    if ($Mode -eq 'calibrate' -and $shaA -ne $shaB) { throw 'A null pair requires byte-identical binaries.' }
    if ($Mode -ne 'calibrate' -and $shaA -eq $shaB -and (($OptionsA -join '|') -eq ($OptionsB -join '|'))) {
        throw 'Identical binaries and options require -Mode calibrate.'
    }
}
Write-Host "  Engine manifests, fingerprints, compilers, flavors and UCI options verified."

$Dir = [System.IO.Path]::GetFullPath($Dir, (Get-Location).Path)
$storedConfig = Join-Path $Dir 'resolved-config.json'
if (Test-Path -LiteralPath $storedConfig) {
    $storedSeed = [int](Get-Content -LiteralPath $storedConfig -Raw | ConvertFrom-Json).master_seed
    if ($Seed -eq 0) { $Seed = $storedSeed; Write-Host "  Resuming at recorded seed $Seed." }
    elseif ($Seed -ne $storedSeed) { throw "$Dir records seed $storedSeed; -Seed $Seed would change a resumed run." }
}
$Seed = New-HarnessSeed -Requested $Seed
$concurrencyInfo = Resolve-HarnessConcurrency -Requested $Concurrency -ReservePhysicalCores 1 -ThreadsPerGame $Threads
$expectedConcurrency = $concurrencyInfo.Concurrency

$commandArgs = @('--run-file', $RunFile)
if ($Mode -eq 'gauntlet') { $commandArgs += @('tournament', 'run') }
switch ($Mode) {
    'sprt' { $commandArgs += @($engines[0].Path, $engines[1].Path, '--max-pairs', "$MaxPairs") }
    'match' { $commandArgs += @($engines[0].Path, $engines[1].Path); if ($Games) { $commandArgs += @('--games', "$Games") } }
    'calibrate' { $commandArgs += @($engines[0].Path, $engines[1].Path); if ($Games) { $commandArgs += @('--games', "$Games") } }
}
function Get-SideOptions([string[]]$Own) {
    $names = @($Own | ForEach-Object { ($_ -split '=', 2)[0].Trim().ToLowerInvariant() })
    @(@("Hash=$Hash", "Threads=$Threads") | Where-Object { $names -notcontains ($_ -split '=', 2)[0].ToLowerInvariant() }) + @($Own)
}
if ($twoArm) {
    foreach ($option in (Get-SideOptions $OptionsA)) { $commandArgs += @('--a-option', $option) }
    foreach ($option in (Get-SideOptions $OptionsB)) { $commandArgs += @('--b-option', $option) }
} else {
    $commandArgs += $ExtraArgs
}
$commandArgs += @('--concurrency', "$expectedConcurrency")
$commandArgs += @('--seed', "$Seed", '--dir', $Dir)

$resultsDir = Join-Path $PSScriptRoot 'results'
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$label = Split-Path $Dir -Leaf
$dryPath = Join-Path $resultsDir "colosseum_${Mode}_${label}_${stamp}.dry-run.json"
$notesPath = [IO.Path]::ChangeExtension($dryPath, '.notes.txt')
$manifestPath = Join-Path $resultsDir "colosseum_${Mode}_${label}_${stamp}.manifest.txt"
$logPath = Join-Path $resultsDir "colosseum_${Mode}_${label}_${stamp}.log"

& $cli.Path @commandArgs --dry-run --json 2> $notesPath > $dryPath
if ($LASTEXITCODE -ne 0) { throw "colosseum-cli refused the dry run (exit $LASTEXITCODE)." }
$dry = Get-Content -LiteralPath $dryPath -Raw | ConvertFrom-Json
$resolved = $dry.resolved_configuration
$violations = [System.Collections.Generic.List[string]]::new()
function Add-Violation([string]$Text) { $violations.Add($Text) }
$expectedCommand = if ($Mode -eq 'gauntlet') { 'tournament' } else { $Mode }
if ($dry.command -ne $expectedCommand) { Add-Violation "command '$($dry.command)', expected '$expectedCommand'" }
foreach ($rule in @('draw', 'resign', 'max_moves')) {
    if ($null -ne $resolved.adjudication.$rule) { Add-Violation "adjudication.$rule is set; games must terminate naturally" }
}
$controls = @()
foreach ($field in @('engine_a_time_control', 'engine_b_time_control', 'time_control')) {
    if ($resolved.PSObject.Properties.Name -contains $field) { $controls += ,@($field, $resolved.$field) }
}
if ($controls.Count -eq 0) { Add-Violation 'resolved configuration has no time control' }
foreach ($pair in $controls) {
    $name = $pair[0]; $control = $pair[1]
    if ([int]$control.control.Increment.base_ms -ne $BaseMs) { Add-Violation "$name base is not $BaseMs ms" }
    if ([int]$control.control.Increment.inc_ms -ne $IncrementMs) { Add-Violation "$name increment is not $IncrementMs ms" }
    if ([int]$control.margin_ms -ne $MarginMs) { Add-Violation "$name margin is not $MarginMs ms" }
}
foreach ($field in @('engine_a', 'engine_b')) {
    if ($resolved.PSObject.Properties.Name -contains $field) {
        if ([int]$resolved.$field.options.Hash.value -ne $Hash) { Add-Violation "$field Hash is not $Hash" }
        if ([int]$resolved.$field.options.Threads.value -ne $Threads) { Add-Violation "$field Threads is not $Threads" }
    }
}
if ($resolved.openings.path -ne $Book) { Add-Violation "book is '$($resolved.openings.path)', expected '$Book'" }
if ("$($resolved.openings.order)" -ne 'Random') { Add-Violation "opening order is '$($resolved.openings.order)', expected Random" }
if ($resolved.openings.wrap) { Add-Violation 'openings wrap; a run must not replay its book' }
if ([int]$resolved.execution.concurrency -ne $expectedConcurrency) { Add-Violation "concurrency is $($resolved.execution.concurrency), expected $expectedConcurrency" }
if ("$($resolved.execution.placement_policy.mode)" -ne 'auto') { Add-Violation 'placement is not auto' }
if ([int]$resolved.execution.placement_policy.headroom_physical_cores -lt 1) { Add-Violation 'placement leaves no physical-core headroom' }
if ([int]$resolved.master_seed -ne $Seed) { Add-Violation "seed is $($resolved.master_seed), expected $Seed" }
if ($Mode -eq 'sprt') {
    if ("$($resolved.design.parameters.model)" -ne 'normalized') { Add-Violation 'SPRT model is not normalized' }
    if ([int]$resolved.design.max_pairs -ne $MaxPairs) { Add-Violation "cap is $($resolved.design.max_pairs), expected $MaxPairs" }
}
if ($violations.Count) {
    foreach ($violation in $violations) { Write-Host "  POLICY: $violation" -ForegroundColor Red }
    throw "Resolved configuration is not Basilisk policy ($($violations.Count) violation(s)); see $dryPath."
}
Write-Host '  Resolved configuration matches Basilisk policy.'

$repoSha = (git rev-parse HEAD).Trim()
$repoDirty = [bool](git status --porcelain)
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("mode:             $Mode")
$lines.Add("run_file:         $RunFile")
$lines.Add("run_file_sha256:  $(Get-HarnessSha256 $RunFile)")
$lines.Add("run_directory:    $Dir")
for ($i = 0; $i -lt $engines.Count; $i++) {
    $arm = $engines[$i]; $manifest = $manifests[$arm.Label]
    $lines.Add("engine_$($arm.Label): $($arm.Path)")
    $lines.Add("  sha256:         $(Get-HarnessSha256 $arm.Path)")
    $lines.Add("  revision:       $($manifest.GitSha)")
    $lines.Add("  flavor:         $($manifest.Flavor)")
    $lines.Add("  umbrella:       $($manifest.ArmOption)=$($manifest.ArmState)")
    $lines.Add("  compiler:       $($manifest.Compiler)")
    $lines.Add("  bench:          $($manifest.Bench)")
}
$lines.Add("expect_revision:  $(if ($ExpectRevision) { $ExpectRevision } else { '(manifest only)' })")
$lines.Add("expect_bench:     $($ExpectBench -join ', ')")
$lines.Add("repo_revision:    $repoSha")
$lines.Add("repo_dirty:       $repoDirty")
$lines.Add("runner:           $($cli.Path)")
$lines.Add("runner_version:   $($cli.Version)")
$lines.Add("runner_sha256:    $($cli.Sha256)")
$lines.Add("runner_revision:  $($cli.Pin.revision)")
$lines.Add("book:             $Book")
$lines.Add("book_sha256:      $(Get-HarnessSha256 $Book)")
$lines.Add("opening_seed:     $Seed")
$lines.Add("adjudication:     none (natural termination)")
$lines.Add("hash_mb:          $Hash")
$lines.Add("threads:          $Threads")
$lines.Add("concurrency:      $($resolved.execution.concurrency)")
$lines.Add("host_busy_percent: $($hostState.BusyPercent)")
$lines.Add("host_idle_waived: $($hostState.Waived)")
$lines.Add("dry_run_json:     $dryPath")
$lines.Add("dry_run_sha256:   $(Get-HarnessSha256 $dryPath)")
$lines.Add("command:          $($cli.Path) $($commandArgs -join ' ')")
$lines.Add("started_utc:      $((Get-Date).ToUniversalTime().ToString('u'))")
$lines | Set-Content -LiteralPath $manifestPath -Encoding utf8

if ($DryRun) {
    Add-Content -LiteralPath $manifestPath -Value 'dry_run_only:     true' -Encoding utf8
    Write-Host "Dry run accepted. Manifest: $manifestPath"
    return
}

$returned = $false
$process = $null
try {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $cli.Path
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $commandArgs) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $runExit = $process.ExitCode
    $stdoutText = $stdout.GetAwaiter().GetResult()
    $stderrText = $stderr.GetAwaiter().GetResult()
    @($stdoutText, $stderrText) | Where-Object { $_ } | Set-Content -LiteralPath $logPath -Encoding utf8
    if ($stdoutText) { Write-Host $stdoutText.TrimEnd() }
    if ($stderrText) { Write-Host $stderrText.TrimEnd() -ForegroundColor Yellow }
    $returned = $true
} finally {
    if (-not $returned) {
        if ($process -and -not $process.HasExited) { $process.Kill($true) }
        Add-Content -LiteralPath $manifestPath -Value 'interrupted:      wrapper stopped before a runner verdict'
    }
    if ($process) { $process.Dispose() }
}
$exit = Resolve-ColosseumExit -Mode $Mode -ExitCode $runExit
if ($exit.Kind -ne 'outcome') { throw "colosseum-cli exited $runExit ($($exit.Kind)); see $Dir." }

$recordPath = Join-Path $Dir 'run-record.json'
if (-not (Test-Path -LiteralPath $recordPath)) { throw "No run-record.json in $Dir." }
$record = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
$status = Get-ColosseumRunStatus -CliPath $cli.Path -Dir $Dir
if ($status.durable.journal.refused) { throw "Runner journal/checkpoint mismatch: $($status.durable.journal.refused)." }
$scored = [int]$record.official_sample.scored_games
Assert-ColosseumRunFaults -Status $status -ScoredGames $scored -TimeLossRateCeiling $TimeLossRateCeiling -Dir $Dir | Out-Null

$recountJson = & python (Join-Path $PSScriptRoot 'diag\colosseum_recount.py') --json $Dir 2>&1
$recountExit = $LASTEXITCODE
$recount = @(($recountJson -join "`n") | ConvertFrom-Json)[0]
if ($recountExit -ne 0 -or -not $recount.comparable -or -not $recount.agrees) {
    throw "Independent PGN recount did not agree with the runner; see $Dir."
}
Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
    "completed_utc:    $((Get-Date).ToUniversalTime().ToString('u'))"
    "exit_code:        $runExit ($($exit.Verdict))"
    "scored_games:     $scored"
    "pentanomial:      $($recount.pentanomial -join ', ') (independent PGN recount agrees)"
    "run_record_sha256: $(Get-HarnessSha256 $recordPath)"
    "pgn_sha256:       $(Get-HarnessSha256 (Join-Path $Dir 'games.pgn'))"
)
Write-Host "Run finished: $($exit.Verdict), $scored games, zero disallowed faults; PGN recount agrees."
Write-Host "Manifest: $manifestPath"
