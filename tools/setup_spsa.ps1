<#
.SYNOPSIS
    One-shot setup for a weather-factory SPSA run.

.DESCRIPTION
    Populates the weather-factory tuner\ folder and writes the three config
    files (cutechess.json, spsa.json, config.json).  After this script
    completes, run:

        cd tools/weather-factory
        python main.py

    Stop it with Ctrl-C whenever parameter values look stable (typically
    after 2000-5000 iterations).  State is saved every 10 iterations
    to tuner\state.json so you can resume at any time.

    Prerequisites:
      - Run ./tools/setup_tools.ps1 once (downloads fastchess, clones weather-factory).
      - Build the test binary you want to tune from:
        ./tools/build_test.ps1 -Suffix phase1-defaults

.PARAMETER ConfigGroup
    Which parameter group to tune.
    "pruning"  (default) - 13 pruning / margin constants.
    "lmr"                - LMR formula + adjustment constants.
    "combined"           - narrowed Phase 1 polish around accepted pruning+LMR.

.PARAMETER EngineSuffix
    Suffix of the already-built engine under tools\test_engines.
    The script expects:
        tools\test_engines\basilisk-<EngineSuffix>-pext-pgo.exe

.PARAMETER Resume
    Keep existing weather-factory tuner state. Use this only when resuming the
    same SPSA run. By default, old state/games/graphs are archived before a new
    run is configured.

.PARAMETER Iterations
    Planned total iterations (used to set A = Iterations / 10 in spsa.json).
    Default: 5000.  At ~12 s/iteration this is roughly 17 hours; stop
    earlier with Ctrl-C if values stabilise sooner.

.EXAMPLE
    # Standard first run -- pruning group, 5000 iterations
    ./tools/setup_spsa.ps1

.EXAMPLE
    # Shorter overnight run (~7 hours)
    ./tools/setup_spsa.ps1 -Iterations 2000

.EXAMPLE
    # Resume an already-configured run without archiving its state
    ./tools/setup_spsa.ps1 -ConfigGroup lmr -EngineSuffix phase1-lmr-baseline -Resume

.EXAMPLE
    # LMR group
    ./tools/setup_spsa.ps1 -ConfigGroup lmr -EngineSuffix phase1-lmr-baseline

.EXAMPLE
    # Narrow combined Phase 1 polish from the accepted LMR head
    ./tools/setup_spsa.ps1 -ConfigGroup combined -EngineSuffix phase1-lmr -Iterations 2000
#>
param(
    [ValidateSet("pruning","lmr","combined")][string]$ConfigGroup = "pruning",
    [string]$EngineSuffix = "phase1-defaults",
    [switch]$Resume,
    [int]$Iterations = 5000
)

$ErrorActionPreference = "Stop"

$wfRoot    = Join-Path $PSScriptRoot "weather-factory"
$configs   = Join-Path $PSScriptRoot "spsa_configs"
$fastchess = Join-Path $PSScriptRoot "bin\fastchess.exe"
$engine    = Join-Path $PSScriptRoot "test_engines\basilisk-$EngineSuffix-pext-pgo.exe"
$book      = Join-Path $PSScriptRoot "books\SuperGM_4mvs.pgn"

# 1. Validate prerequisites
foreach ($f in @($fastchess, $engine, $book)) {
    if (-not (Test-Path $f)) { throw "Required file not found: $f" }
}

# 2. Populate tuner\ folder
$tuner = Join-Path $wfRoot "tuner"
New-Item -ItemType Directory -Force -Path $tuner | Out-Null

if (-not $Resume) {
    $stateFiles = @(
        "state.json",
        "games.pgn",
        "graph.png",
        "fastchess_config.json"
    )

    $existingState = $stateFiles |
        ForEach-Object { Join-Path $tuner $_ } |
        Where-Object { Test-Path $_ }

    if ($existingState) {
        $archive = Join-Path $tuner ("archive_" + (Get-Date -Format "yyyyMMdd_HHmmss"))
        New-Item -ItemType Directory -Force -Path $archive | Out-Null
        foreach ($f in $existingState) {
            Move-Item $f (Join-Path $archive (Split-Path $f -Leaf)) -Force
        }
        Write-Host "Archived previous tuner state -> $archive"
    }
}

$engineName = Split-Path $engine -Leaf
Write-Host "Copying engine  -> $tuner\$engineName"
Copy-Item $engine  (Join-Path $tuner $engineName) -Force
Write-Host "Copying book    -> $tuner\$(Split-Path $book -Leaf)"
Copy-Item $book    (Join-Path $tuner (Split-Path $book -Leaf)) -Force

# weather-factory calls fastchess as just "fastchess" (no path), so it
# must be findable via the CWD when running from the weather-factory root.
# Skip if fastchess is locked by another process (e.g. a concurrent SPSA run).
Write-Host "Copying fastchess -> $wfRoot\fastchess.exe"
try {
    Copy-Item $fastchess (Join-Path $wfRoot "fastchess.exe") -Force
} catch {
    Write-Host "  (skipped -- fastchess.exe is in use; existing copy will be used)"
}

# 5. Write cutechess.json
$cutechessJson = @{
    engine        = $engineName
    book          = "SuperGM_4mvs.pgn"
    games         = 32
    tc            = 1
    hash          = 64
    threads       = 15
    save_rate     = 10
    pgnout        = "file=tuner/games.pgn"
    use_fastchess = $true
} | ConvertTo-Json
$cutechessJson | Out-File (Join-Path $wfRoot "cutechess.json") -Encoding utf8 -NoNewline
Write-Host "Wrote cutechess.json"

# 6. Write spsa.json (A = Iterations / 10)
# ConvertTo-Json can't have both "a" and "A" as keys (case-insensitive in PS),
# so write the JSON directly as a string.
$A = [int]([Math]::Floor($Iterations / 10))
$spsaJson = "{`n    ""a"": 1.0,`n    ""c"": 1.0,`n    ""A"": $A,`n    ""alpha"": 0.601,`n    ""gamma"": 0.102`n}"
$spsaJson | Out-File (Join-Path $wfRoot "spsa.json") -Encoding utf8 -NoNewline
Write-Host "Wrote spsa.json  (A=$A for $Iterations planned iterations)"

# 7. Write config.json from the chosen parameter group
$srcConfig = Join-Path $configs "config_$ConfigGroup.json"
Copy-Item $srcConfig (Join-Path $wfRoot "config.json") -Force
Write-Host "Wrote config.json (group: $ConfigGroup)"

Write-Host ""
Write-Host "============================================================"
Write-Host "  Setup complete."
Write-Host ""
Write-Host "  Run SPSA:"
Write-Host "    cd tools/weather-factory"
Write-Host "    python main.py"
Write-Host ""
Write-Host "  Stop with Ctrl-C when values stabilise."
Write-Host "  State saved every 10 iterations -> tuner\state.json"
Write-Host "  Resume after a stop: just run python main.py again"
Write-Host "============================================================"
