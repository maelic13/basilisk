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
    [string[]]$ExpectSha256 = @(),
    [string[]]$ThreadOptionNames = @(),
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
    $labels = @(for ($i = 0; $i -lt $ExtraArgs.Count; $i++) {
        if ($ExtraArgs[$i] -eq '--label' -and $i + 1 -lt $ExtraArgs.Count) { $ExtraArgs[$i + 1] }
    })
    if ($labels.Count -ne $engines.Count) {
        throw "A gauntlet requires one --label per engine; got $($labels.Count), expected $($engines.Count)."
    }
    for ($i = 0; $i -lt $engines.Count; $i++) { $engines[$i].Label = $labels[$i] }
    for ($i = 0; $i -lt $ExtraArgs.Count; $i++) {
        if ($ExtraArgs[$i] -ne '--engine-option') { continue }
        if ($i + 1 -ge $ExtraArgs.Count -or $ExtraArgs[$i + 1] -notmatch '^(?<index>[1-9][0-9]*):(?<option>.+)$') {
            throw '--engine-option must be followed by INDEX:NAME=VALUE.'
        }
        $participant = [int]$Matches.index
        if ($participant -gt $engines.Count) { throw "--engine-option index $participant exceeds the $($engines.Count)-engine field." }
        $engines[$participant - 1].Options = @($engines[$participant - 1].Options) + $Matches.option
    }
}
foreach ($arm in $engines) {
    if (-not (Test-Path -LiteralPath $arm.Path -PathType Leaf)) { throw "Engine not found: $($arm.Path)" }
    $arm.Path = (Resolve-Path -LiteralPath $arm.Path).Path
}
if ($ExpectBench.Count -ne $engines.Count) {
    throw "-ExpectBench is required once per engine; got $($ExpectBench.Count), expected $($engines.Count)."
}
if ($Mode -eq 'gauntlet') {
    if ($ExpectSha256.Count -ne $engines.Count) {
        throw "-ExpectSha256 is required once per gauntlet engine; got $($ExpectSha256.Count), expected $($engines.Count)."
    }
    if ($ThreadOptionNames.Count -ne $engines.Count) {
        throw "-ThreadOptionNames is required once per gauntlet engine; got $($ThreadOptionNames.Count), expected $($engines.Count)."
    }
}

$manifests = @{}
for ($i = 0; $i -lt $engines.Count; $i++) {
    $arm = $engines[$i]
    if ($Mode -eq 'gauntlet') {
        if ($ExpectSha256[$i] -notmatch '^[0-9a-fA-F]{64}$') { throw "Malformed registered SHA-256 for $($arm.Label)." }
        $actualSha = Get-HarnessSha256 $arm.Path
        if ($actualSha -ne $ExpectSha256[$i]) {
            throw "PROVENANCE MISMATCH - $($arm.Label) SHA-256 is $actualSha, not registered $($ExpectSha256[$i])."
        }
        if ($ExpectBench[$i] -ge 0) {
            $manifests[$arm.Label] = Assert-EngineProvenance -Path $arm.Path -Label $arm.Label `
                -AllowDirtyTree:$AllowDirtyTree -ExpectRevision $ExpectRevision -ExpectBench $ExpectBench[$i]
        } else {
            $manifests[$arm.Label] = $null
        }
    } else {
        $manifests[$arm.Label] = Assert-EngineProvenance -Path $arm.Path -Label $arm.Label `
            -AllowDirtyTree:$AllowDirtyTree -ExpectRevision $ExpectRevision -ExpectBench $ExpectBench[$i]
    }
    $details = @(Get-EngineUciOptions -Path $arm.Path -Detailed)
    $threadOption = if ($Mode -eq 'gauntlet') { $ThreadOptionNames[$i] } else { 'Threads' }
    Assert-AdvertisedOptions -Advertised $details -Wanted @("Hash=$Hash", "${threadOption}=$Threads") -Label $arm.Label
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

if (-not [System.IO.Path]::IsPathRooted($Dir)) {
    $Dir = Join-Path (Get-Location).Path $Dir
}
$Dir = [System.IO.Path]::GetFullPath($Dir)
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
    $commandArgs += @('--option', "Hash=$Hash")
    for ($i = 0; $i -lt $engines.Count; $i++) {
        $commandArgs += @('--engine-option', "$($i + 1):$($ThreadOptionNames[$i])=$Threads")
    }
    $commandArgs += $ExtraArgs
}
$commandArgs += @('--concurrency', "$expectedConcurrency")
$commandArgs += @('--cores-per-game', "$Threads")
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
if ($Mode -eq 'gauntlet') {
    $participants = @($resolved.plan.participants)
    if ($participants.Count -ne $engines.Count) { Add-Violation "gauntlet resolved $($participants.Count) participants, expected $($engines.Count)" }
    $design = $resolved.plan.design
    $gauntletDesign = $design.format.Gauntlet
    if (-not $gauntletDesign) { Add-Violation 'tournament format is not gauntlet' }
    $seeds = [int]$gauntletDesign.seeds
    $cycles = [int]$gauntletDesign.cycles
    $gamesPerPair = [int]$design.games_per_pair
    if ($seeds -ne 1) { Add-Violation "gauntlet has $seeds seeds, expected 1" }
    if ($gamesPerPair -ne 2) { Add-Violation "gauntlet games_per_pair is $gamesPerPair, expected 2 for paired openings" }
    $expectedGames = $seeds * ($participants.Count - $seeds) * $cycles * $gamesPerPair
    if (@($resolved.plan.schedule).Count -ne $expectedGames) {
        Add-Violation "gauntlet schedule has $(@($resolved.plan.schedule).Count) games, expected $expectedGames"
    }
    $expectedOpenings = $seeds * ($participants.Count - $seeds) * $cycles
    if ([int]$resolved.openings.scheduled_openings -ne $expectedOpenings) {
        Add-Violation "gauntlet schedules $($resolved.openings.scheduled_openings) openings, expected $expectedOpenings paired openings"
    }
    if ([int]$resolved.max_engine_faults -ne 0) { Add-Violation "gauntlet permits $($resolved.max_engine_faults) engine faults, expected 0" }
    $fixed = @($resolved.fixed_ratings)
    if ($fixed.Count -ne $participants.Count - $seeds) {
        Add-Violation "gauntlet fixes $($fixed.Count) opponent ratings, expected $($participants.Count - $seeds)"
    }
    for ($i = $seeds; $i -lt $participants.Count; $i++) {
        $id = "$($participants[$i].participant.id)"
        $rating = [double]$participants[$i].initial_rating
        $frozen = @($fixed | Where-Object { "$($_.participant)" -eq $id })
        if ($frozen.Count -ne 1 -or [double]$frozen[0].rating -ne $rating) {
            Add-Violation "participant '$($engines[$i].Label)' is not fixed at its initial rating $rating"
        }
    }
    for ($i = 0; $i -lt [Math]::Min($participants.Count, $engines.Count); $i++) {
        $launch = $participants[$i].participant.launch
        $hashProperty = $launch.options.PSObject.Properties | Where-Object Name -EQ 'Hash' | Select-Object -First 1
        if (-not $hashProperty -or [int]$hashProperty.Value.value -ne $Hash) {
            Add-Violation "participant '$($engines[$i].Label)' Hash is not $Hash"
        }
        $threadName = $ThreadOptionNames[$i]
        $threadProperty = $launch.options.PSObject.Properties | Where-Object Name -EQ $threadName | Select-Object -First 1
        if (-not $threadProperty -or [int]$threadProperty.Value.value -ne $Threads) {
            Add-Violation "participant '$($engines[$i].Label)' $threadName is not $Threads"
        }
    }
}
if ($resolved.openings.path -ne $Book) { Add-Violation "book is '$($resolved.openings.path)', expected '$Book'" }
if ("$($resolved.openings.order)" -ne 'Random') { Add-Violation "opening order is '$($resolved.openings.order)', expected Random" }
if ($resolved.openings.wrap) { Add-Violation 'openings wrap; a run must not replay its book' }
try {
    Assert-HarnessResolvedPlacement -Resolved $resolved `
        -ExpectedConcurrency $expectedConcurrency -ThreadsPerGame $Threads
} catch {
    Add-Violation $_.Exception.Message
}
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
    $lines.Add("  revision:       $(if ($manifest) { $manifest.GitSha } else { 'external binary; SHA-256 frozen' })")
    $lines.Add("  flavor:         $(if ($manifest) { $manifest.Flavor } else { 'external' })")
    $lines.Add("  umbrella:       $(if ($manifest) { "$($manifest.ArmOption)=$($manifest.ArmState)" } else { 'n/a' })")
    $lines.Add("  compiler:       $(if ($manifest) { $manifest.Compiler } else { 'unknown' })")
    $lines.Add("  bench:          $(if ($manifest) { $manifest.Bench } else { 'not applicable' })")
    if ($Mode -eq 'gauntlet') { $lines.Add("  thread_option:  $($ThreadOptionNames[$i])=$Threads") }
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
$lines.Add("cores_per_game:   $($resolved.execution.allocation.cores_per_game)")
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
$scored = [int]$record.official_sample.scored_games
if ($Mode -eq 'gauntlet') {
    $resultPath = Join-Path $Dir 'result.json'
    if (-not (Test-Path -LiteralPath $resultPath)) { throw "No result.json in $Dir." }
    $result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
    if ("$($result.status)" -ne 'completed') { throw "Tournament status is '$($result.status)', not completed; see $Dir." }
    if ([int]$result.engine_faults -ne 0 -or [int]$result.infrastructure_faults -ne 0) {
        throw "Tournament has $($result.engine_faults) engine and $($result.infrastructure_faults) infrastructure faults; see $Dir."
    }
    if ([int]$result.results.games_scheduled -ne $expectedGames -or
        [int]$result.results.games_scored -ne $expectedGames -or
        @($result.games).Count -ne $expectedGames -or $scored -ne $expectedGames) {
        throw "Tournament did not produce the registered $expectedGames-game sample; see $Dir."
    }
    $censusPath = Join-Path $Dir 'pgn-census.json'
    $censusArgs = @(
        (Join-Path $PSScriptRoot 'diag\pgn_census.py'), '--pgn', (Join-Path $Dir 'games.pgn'),
        '--engine', $engines[0].Label, '--opponents'
    ) + @($engines | Select-Object -Skip 1 | ForEach-Object { $_.Label }) + @('--json', $censusPath)
    $censusOutput = & python @censusArgs 2>&1
    $censusExit = $LASTEXITCODE
    if ($censusExit -ne 0) { throw "Independent tournament PGN census failed: $($censusOutput -join ' ')" }
    $census = Get-Content -LiteralPath $censusPath -Raw | ConvertFrom-Json
    if ([int]$census.games_in_pgn -ne $expectedGames -or [int]$census.engine_games -ne $expectedGames) {
        throw "Tournament PGN census found $($census.engine_games) of $($census.games_in_pgn) registered games, expected $expectedGames."
    }
    $expectedPairGames = $cycles * $gamesPerPair
    foreach ($opponent in @($engines | Select-Object -Skip 1)) {
        $row = $census.records.PSObject.Properties | Where-Object Name -EQ $opponent.Label | Select-Object -First 1
        if (-not $row -or [int]$row.Value.games -ne $expectedPairGames -or
            [int]$row.Value.white_games -ne $expectedPairGames / 2 -or
            [int]$row.Value.black_games -ne $expectedPairGames / 2 -or
            [int]$row.Value.unfinished -ne 0) {
            throw "Tournament PGN census does not contain a balanced, finished $expectedPairGames-game row for $($opponent.Label)."
        }
    }
    Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
        "completed_utc:    $((Get-Date).ToUniversalTime().ToString('u'))"
        "exit_code:        $runExit ($($exit.Verdict))"
        "scored_games:     $scored (independent PGN census agrees)"
        "run_record_sha256: $(Get-HarnessSha256 $recordPath)"
        "result_sha256:    $(Get-HarnessSha256 $resultPath)"
        "pgn_sha256:       $(Get-HarnessSha256 (Join-Path $Dir 'games.pgn'))"
        "pgn_census_sha256: $(Get-HarnessSha256 $censusPath)"
    )
    Write-Host "Tournament finished: $scored games, zero faults; PGN census agrees."
    Write-Host "Manifest: $manifestPath"
    return
}
$status = Get-ColosseumRunStatus -CliPath $cli.Path -Dir $Dir
if ($status.durable.journal.refused) { throw "Runner journal/checkpoint mismatch: $($status.durable.journal.refused)." }
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
