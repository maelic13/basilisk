<#
.SYNOPSIS
    Set up AND run a repo-local weather-factory SPSA tune — one command, from
    the repo root, no manual cd. (Unified port of Rarog's tools/spsa.ps1.)

.DESCRIPTION
    Replaces the old setup_spsa.ps1 (setup only). By default it does BOTH:
      1. Setup — populate tools\weather-factory\tuner\ (engine, book, fastchess)
         and write the three config files (cutechess.json, spsa.json,
         config.json). Old tuner state is archived first (unless -Resume).
      2. Launch — run `python main.py` with tools\weather-factory as the working
         dir, piped through watch.ps1 (clean console: per-game lines go to the
         log, only param/report blocks show). Returns you to the repo root on
         Ctrl-C. fastchess is resolved via PATH (weather-factory calls a bare
         "fastchess", which Python 3.11's Popen won't find in the CWD on Windows).

    Prerequisites:
      - ./tools/setup_tools.ps1 once if tools\bin\fastchess.exe or
        tools\weather-factory\main.py is missing (this script also auto-clones
        weather-factory if absent).
      - Build the tune engine: ./tools/build_test.ps1 -Suffix <s>
        (the pext-PGO binary is compiled with BASILISK_TUNE, so it already
        exposes the SPSA UCI options — there is no separate -tune binary).

.PARAMETER ConfigGroup
    Which parameter group to tune (selects tools\spsa_configs\config_<g>.json):
    pruning · lmr · combined · tm · wave2 · histshape · hcefinal.

.PARAMETER EngineSuffix
    Suffix of the tune binary in tools\test_engines. Required (no per-group
    default — Basilisk engine names are per-head, e.g. phase8512-instabtm).
    Accepts a bare suffix (basilisk-<s>-pext-pgo.exe), a "-pext-pgo" suffix, or
    a full "*.exe".

.PARAMETER Iterations
    Planned total iterations (sets A = Iterations / 10 in spsa.json).
    Default 5000. State is saved every 10 iterations to tuner\state.json.

.PARAMETER Concurrency
    Parallel games per SPSA mini-match. Default 0 auto-detects physical cores
    and leaves two free. This is weather-factory's `threads` config field; the
    chess engines remain Threads=1.

.PARAMETER Resume
    Preserve the existing tuner state (state.json/games/graph) instead of
    archiving it — continues an interrupted run rather than starting fresh.

.PARAMETER SetupOnly
    Do the setup and stop (do not launch). Prints the launch command.

.PARAMETER LaunchOnly
    Skip setup and just launch (tuner must already be populated). Ignores
    -Iterations / -EngineSuffix / -Resume.

.PARAMETER LogFile
    Override the full-log path. Default tools\results\spsa_<ConfigGroup>.log.

.EXAMPLE
    # Fresh setup + run, one command:
    ./tools/build_test.ps1 -Suffix phase8512-instabtm
    ./tools/spsa.ps1 -ConfigGroup tm -EngineSuffix phase8512-instabtm -Iterations 2500

.EXAMPLE
    # Continue an interrupted run:
    ./tools/spsa.ps1 -ConfigGroup tm -Resume

.EXAMPLE
    # Set up now, launch later:
    ./tools/spsa.ps1 -ConfigGroup tm -EngineSuffix phase8512-instabtm -SetupOnly
    ./tools/spsa.ps1 -ConfigGroup tm -LaunchOnly
#>
param(
    [ValidateSet("pruning","lmr","combined","tm","wave2","histshape","hcefinal")]
    [string]$ConfigGroup = "lmr",
    [string]$EngineSuffix = "",
    [int]$Iterations = 5000,
    [int]$Concurrency = 0,
    [switch]$Resume,
    [switch]$SetupOnly,
    [switch]$LaunchOnly,
    [string]$LogFile = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "..\harness_common.ps1")

$concurrencyInfo = Resolve-HarnessConcurrency -Requested $Concurrency
$Concurrency = $concurrencyInfo.Concurrency

if ($SetupOnly -and $LaunchOnly) { throw "-SetupOnly and -LaunchOnly are mutually exclusive." }

$wfRoot    = Join-Path $PSScriptRoot "weather-factory"
$configs   = Join-Path $PSScriptRoot "spsa_configs"
$fastchess = Join-Path $PSScriptRoot "bin\fastchess.exe"
$watch     = Join-Path $PSScriptRoot "watch.ps1"
# UHO EPD (2026-07-17): weather-factory auto-detects the book format from the
# extension (cutechess.py: format={book.split('.')[-1]}), so the EPD works
# unmodified — and keeps SPSA on the same book as sprt.ps1 (PLAN principle #7).
$book      = Join-Path $PSScriptRoot "books\UHO_Lichess_4852_v1.epd"
if ($LogFile -eq "") { $LogFile = Join-Path $PSScriptRoot "results\spsa_$ConfigGroup.log" }

# ─── Setup ────────────────────────────────────────────────────────────────
if (-not $LaunchOnly) {
    if ($EngineSuffix -eq "") {
        throw "Pass -EngineSuffix (the tune engine's suffix under tools\test_engines, " +
              "e.g. -EngineSuffix phase8512-instabtm)."
    }

    if ($EngineSuffix.EndsWith(".exe")) {
        $engineFile = $EngineSuffix
    } elseif ($EngineSuffix.EndsWith("-pext-pgo")) {
        $engineFile = "basilisk-$EngineSuffix.exe"
    } else {
        $engineFile = "basilisk-$EngineSuffix-pext-pgo.exe"
    }
    $engine = Join-Path $PSScriptRoot "test_engines\$engineFile"

    if (-not (Test-Path (Join-Path $wfRoot "main.py"))) {
        Write-Host "weather-factory missing; running the pinned toolchain setup..."
        & (Join-Path $PSScriptRoot "setup_tools.ps1")
    }

    foreach ($f in @($fastchess, $engine, $book)) {
        if (-not (Test-Path $f)) { throw "Required file not found: $f" }
    }
    $fcInfo = Assert-AffinityFastchess -Path $fastchess

    $wfCute = Join-Path $wfRoot "cutechess.py"
    $expectedAffinityCpus = (Get-HarnessPhysicalCpus).Cpu -join ','
    if (-not (Test-Path $wfCute) -or
        (Get-Content $wfCute -Raw) -notmatch 'BASILISK_AFFINITY_PATCH_V2' -or
        (Get-Content $wfCute -Raw) -notmatch [regex]::Escape("-use-affinity $expectedAffinityCpus ")) {
        throw "weather-factory is not carrying the verified affinity patch; run tools/setup_tools.ps1."
    }
    python -m py_compile $wfCute
    if ($LASTEXITCODE -ne 0) { throw "weather-factory Python syntax validation failed: $wfCute" }

    Write-Host "Installing matplotlib (weather-factory dependency)..."
    pip install matplotlib --quiet
    if ($LASTEXITCODE -ne 0) { Write-Warning "pip install matplotlib failed; run it manually if needed." }

    $tuner = Join-Path $wfRoot "tuner"
    New-Item -ItemType Directory -Force -Path $tuner | Out-Null

    if (-not $Resume) {
        $stateFiles = @("state.json", "games.pgn", "graph.png", "fastchess_config.json")
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
    } else {
        Write-Host "Resume: keeping existing tuner state (state.json preserved)."
    }

    $engineName = Split-Path $engine -Leaf
    Write-Host "Copying engine    -> $tuner\$engineName"
    Copy-Item $engine (Join-Path $tuner $engineName) -Force
    Write-Host "Copying book      -> $tuner\$(Split-Path $book -Leaf)"
    Copy-Item $book (Join-Path $tuner (Split-Path $book -Leaf)) -Force

    Write-Host "Copying fastchess -> $wfRoot\fastchess.exe"
    try {
        Copy-Item $fastchess (Join-Path $wfRoot "fastchess.exe") -Force
    } catch {
        Write-Host "  skipped; fastchess.exe appears to be in use, existing copy will be used"
    }

    $cutechessJson = @{
        engine        = $engineName
        book          = (Split-Path $book -Leaf)  # weather-factory derives format from the extension
        games         = 32
        tc            = 3      # 3+0.03 (weather-factory auto inc = tc/100); UNIFIED
                               # with sprt.ps1's default so SPSA optima transfer to
                               # the confirming SPRT (PLAN.md guiding principle #7).
        hash          = 64
        threads       = $Concurrency
        save_rate     = 10
        pgnout        = "file=tuner/games.pgn"
        use_fastchess = $true
    } | ConvertTo-Json
    $cutechessJson | Out-File (Join-Path $wfRoot "cutechess.json") -Encoding utf8 -NoNewline
    Write-Host "Wrote cutechess.json"

    # ConvertTo-Json can't hold both "a" and "A" (case-insensitive keys), so emit directly.
    $A = [int]([Math]::Floor($Iterations / 10))
    $spsaJson = "{`n    ""a"": 1.0,`n    ""c"": 1.0,`n    ""A"": $A,`n    ""alpha"": 0.601,`n    ""gamma"": 0.102`n}"
    $spsaJson | Out-File (Join-Path $wfRoot "spsa.json") -Encoding utf8 -NoNewline
    Write-Host "Wrote spsa.json (A=$A for $Iterations planned iterations)"

    $srcConfig = Join-Path $configs "config_$ConfigGroup.json"
    if (-not (Test-Path $srcConfig)) { throw "Config not found: $srcConfig" }
    Copy-Item $srcConfig (Join-Path $wfRoot "config.json") -Force
    Write-Host "Wrote config.json (group: $ConfigGroup)"
    Write-Host ""

    if ($SetupOnly) {
        Write-Host "============================================================"
        Write-Host "  Setup complete (SetupOnly). Launch when ready:"
        Write-Host "    ./tools/spsa.ps1 -ConfigGroup $ConfigGroup -LaunchOnly"
        Write-Host "============================================================"
        return
    }
}

# ─── Launch ───────────────────────────────────────────────────────────────
foreach ($p in @((Join-Path $wfRoot "main.py"), (Join-Path $wfRoot "cutechess.py"),
        (Join-Path $wfRoot "fastchess.exe"), $watch, (Join-Path $wfRoot "config.json"),
        (Join-Path $wfRoot "cutechess.json"))) {
    if (-not (Test-Path $p)) {
        throw "Not found: $p — run ./tools/spsa.ps1 -ConfigGroup $ConfigGroup -EngineSuffix <s> (setup) first."
    }
}
$launchFastchess = Join-Path $wfRoot "fastchess.exe"
Assert-AffinityFastchess -Path $launchFastchess | Out-Null
$launchCute = Join-Path $wfRoot "cutechess.py"
$expectedAffinityCpus = (Get-HarnessPhysicalCpus).Cpu -join ','
$launchCuteContent = Get-Content $launchCute -Raw
if ($launchCuteContent -notmatch 'BASILISK_AFFINITY_PATCH_V2' -or
    $launchCuteContent -notmatch [regex]::Escape("-use-affinity $expectedAffinityCpus ")) {
    throw "weather-factory is not carrying the verified affinity patch; run tools/setup_tools.ps1."
}
python -m py_compile $launchCute
if ($LASTEXITCODE -ne 0) { throw "weather-factory Python syntax validation failed: $launchCute" }

$launchConfigPath = Join-Path $wfRoot "cutechess.json"
$launchConfig = Get-Content $launchConfigPath -Raw | ConvertFrom-Json
if ([int]$launchConfig.threads -ne $Concurrency) {
    throw "cutechess.json concurrency is $($launchConfig.threads), but this launch resolved to $Concurrency. " +
          "Run setup again, or pass -Concurrency $($launchConfig.threads) explicitly to resume that run."
}

Write-Host "SPSA ($ConfigGroup): python main.py | watch.ps1  (Ctrl-C to stop when stable)"
Write-Host "  Log: $LogFile"
Write-Host "  State saved every 10 iterations -> tuner\state.json (resume with -Resume)"
Write-Host ""

# weather-factory launches fastchess as a bare "fastchess" command, but on
# Windows + Python 3.11 subprocess.Popen does NOT search the current directory,
# so it fails with FileNotFoundError even though fastchess.exe is in $wfRoot.
# Prepending $wfRoot to PATH lets Popen resolve it (CreateProcess searches PATH).
$savedPath = $env:PATH
$env:PATH = "$wfRoot;$env:PATH"
Push-Location $wfRoot
try {
    # 2>&1 folds Python's stderr into the stream so watch.ps1 can tee/filter it.
    # Use `& $watch` (an in-session pipeline stage), NOT `pwsh $watch` (a child
    # process): a script's process{} block only receives piped input when it runs
    # in THIS session — a separate pwsh silently drops the stream.
    python main.py 2>&1 | & $watch -LogFile $LogFile
} finally {
    Pop-Location
    $env:PATH = $savedPath
}
