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
    search is generated from src/search_params.h. Hand-maintained parameter
    vectors are retired because their defaults and ranges drifted.

.PARAMETER EngineSuffix
    Suffix of the tune binary in tools\test_engines. Required (no per-group
    default — Basilisk engine names are per-head, e.g. phase8512-instabtm).
    Accepts a bare suffix (basilisk-<s>-pext-pgo.exe), a "-pext-pgo" suffix, or
    a full "*.exe".

.PARAMETER Iterations
    Planned horizon N, in iterations. Default and production floor 2000: one
    PLAN rule 7c block. Pass -AllowShortRun only for instrument tests.

    The whole schedule is back-solved from N (see -REnd), and the run STOPS
    ITSELF there. State is saved every 10 iterations to tuner\state.json.

    !! N and the schedule are FROZEN at first launch. main.py restores them from
    state.json, so re-passing -Iterations with -Resume does NOT change the run —
    it only rewrites a spsa.json that a resumed run never reads. Both this
    script and main.py say so loudly when they see it.

.PARAMETER REnd
    End-of-run step ratio (fishtest's parameterization): how far one unit of
    match signal moves a parameter, in units of that parameter's own `step`, at
    iteration N. Default 0.0031.

    This replaces hand-picking `a`. Phase 9.1 fixed a units bug that made every
    historical tune anneal ~8x too fast; the fix alone would have multiplied
    every step by ~8, so `a` and `c` are now derived from (N, r_end) instead:
        A = 0.1*N,  c = N^gamma,  a = r_end * (A + N)^alpha
    Because both constants derive from the planned horizon, changing the horizon
    can no longer silently change end behaviour, and `a` can never go stale.
    Fishtest's own default is ~0.002; our historical a=1.0 corresponds to
    r_end ~ 0.031 at a 1,000-iteration horizon, i.e. ~15x hotter than fishtest
    has ever defaulted to. Verify any change with tools/verify_spsa_schedule.py.

.PARAMETER AllowShortRun
    Permit -Iterations below one 2,000-iteration rule-7c block. For instrument
    tests only, never for a tune whose result may be baked.

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
    -Iterations / -EngineSuffix / -Resume / -Adjudicate; the existing
    cutechess.json remains authoritative.

.PARAMETER LogFile
    Override the full-log path. Default tools\results\spsa_<ConfigGroup>.log.

.PARAMETER Adjudicate
    Opt in to legacy score-based draw 40/8/10 and resign 400/3. Omitted by
    default: weather-factory mini-matches end only by chess rules.

.EXAMPLE
    # Fresh setup + run, one command:
    ./tools/build_test.ps1 -Suffix phase8512-instabtm
    ./tools/spsa.ps1 -ConfigGroup search -EngineSuffix b23core

.EXAMPLE
    # Continue an interrupted run:
    ./tools/spsa.ps1 -ConfigGroup search -Resume

.EXAMPLE
    # Set up now, launch later:
    ./tools/spsa.ps1 -ConfigGroup search -EngineSuffix b23core -SetupOnly
    ./tools/spsa.ps1 -ConfigGroup search -LaunchOnly
#>
param(
    [ValidateSet("search")]
    [string]$ConfigGroup = "search",
    [string]$EngineSuffix = "",
    [int]$Iterations = 2000,
    [double]$REnd = 0.0031,
    [int]$Concurrency = 0,
    [switch]$AllowShortRun,
    [switch]$Resume,
    [switch]$SetupOnly,
    [switch]$LaunchOnly,
    [switch]$Adjudicate,
    [string]$LogFile = "",
    [switch]$AllowDirtyTree,
    [string]$ExpectRevision = "",
    [long]$ExpectBench = 0,
    [switch]$AllowBusyHost,
    [double]$MaxHostBusyPercent = 15
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "..\harness_common.ps1")

$concurrencyInfo = Resolve-HarnessConcurrency -Requested $Concurrency
$Concurrency = $concurrencyInfo.Concurrency

if ($SetupOnly -and $LaunchOnly) { throw "-SetupOnly and -LaunchOnly are mutually exclusive." }

# PLAN rule 7c registers tunes as 2,000-iteration blocks. A later block starts
# from the previous block's rounded centres on a fresh schedule.
if (-not $LaunchOnly -and $Iterations -lt 2000) {
    if (-not $AllowShortRun) {
        throw "-Iterations $Iterations is below PLAN rule 7c's 2,000-iteration block. " +
              "Pass -AllowShortRun if you are testing the instrument rather than tuning."
    }
    Write-Warning "SHORT RUN: $Iterations iterations is below a rule-7c block; instrument test only."
}
if ($REnd -le 0) { throw "-REnd must be > 0 (default 0.0031; fishtest's own default is ~0.002)." }

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
    if ($ExpectBench -le 0) { throw "-ExpectBench is required for the tune binary." }
    $engineManifest = Assert-EngineProvenance -Path $engine -Label $engineFile `
        -AllowDirtyTree:$AllowDirtyTree -ExpectRevision $ExpectRevision -ExpectBench $ExpectBench
    $fcInfo = Assert-AffinityFastchess -Path $fastchess

    $wfCute = Join-Path $wfRoot "cutechess.py"
    $expectedAffinityCpus = (Get-HarnessPhysicalCpus).Cpu -join ','
    if (-not (Test-Path $wfCute) -or
        (Get-Content $wfCute -Raw) -notmatch 'BASILISK_AFFINITY_PATCH_V2' -or
        (Get-Content $wfCute -Raw) -notmatch 'BASILISK_NO_ADJUDICATION_DEFAULT_V1' -or
        (Get-Content $wfCute -Raw) -notmatch [regex]::Escape("-use-affinity $expectedAffinityCpus ")) {
        throw "weather-factory is not carrying the verified affinity/no-adjudication patches; run tools/setup_tools.ps1."
    }
    python -m py_compile $wfCute
    if ($LASTEXITCODE -ne 0) { throw "weather-factory Python syntax validation failed: $wfCute" }

    # Phase 9.1: the clone must be carrying the tracked spsa.py / main.py /
    # write_spsa_json.py, or the schedule silently reverts to the pre-9.1 bug.
    Assert-WfOverlay -WeatherFactoryDir $wfRoot

    Write-Host "Installing matplotlib (weather-factory dependency)..."
    pip install matplotlib --quiet
    if ($LASTEXITCODE -ne 0) { Write-Warning "pip install matplotlib failed; run it manually if needed." }

    $tuner = Join-Path $wfRoot "tuner"
    New-Item -ItemType Directory -Force -Path $tuner | Out-Null

    if (-not $Resume) {
        # trajectory.csv joins the archive set (9.1): it is appended across
        # resumes on purpose, so a FRESH run must not inherit the old one.
        $stateFiles = @("state.json", "games.pgn", "graph.png", "fastchess_config.json", "trajectory.csv")
        $existingState = $stateFiles |
            ForEach-Object { Join-Path $tuner $_ } |
            Where-Object { Test-Path $_ }

        if ($existingState) {
            $archive = Join-Path $tuner ("archive_" + (Get-Date -Format "yyyyMMdd_HHmmss"))
            New-Item -ItemType Directory -Force -Path $archive | Out-Null
            foreach ($f in $existingState) {
                Move-Item $f (Join-Path $archive (Split-Path $f -Leaf)) -Force
            }
            # The console log carries the same trajectory and is now appended
            # rather than truncated, so a fresh run rotates it into the archive
            # instead of overwriting the previous run's record.
            if (Test-Path $LogFile) {
                Move-Item $LogFile (Join-Path $archive (Split-Path $LogFile -Leaf)) -Force
            }
            Write-Host "Archived previous tuner state -> $archive"
        }
    } else {
        Write-Host "Resume: keeping existing tuner state (state.json preserved)."
        $state = Get-WfTunerState -WeatherFactoryDir $wfRoot
        if ($state) {
            $stateN = [int]$state["N"]
            Write-Host ""
            Write-Warning ("The schedule is FROZEN at what the first launch created: " +
                "a=$($state['gain_a']) c=$($state['probe_c']) A=$($state['damp_A'])" +
                $(if ($stateN) { " N=$stateN" } else { " (pre-9.1 state: no horizon recorded)" }) + ". " +
                "main.py restores it from state.json, so -Iterations/-REnd passed now are " +
                "IGNORED by the resumed run. Start a fresh run to change the schedule.")
            if ($stateN -and $stateN -ne $Iterations) {
                Write-Warning ("You passed -Iterations $Iterations but the run's horizon is $stateN. " +
                    "The run will stop at $stateN.")
            }
            Write-Host ""
        }
    }

    $engineName = Split-Path $engine -Leaf
    Write-Host "Copying engine    -> $tuner\$engineName"
    Copy-Item $engine (Join-Path $tuner $engineName) -Force
    Copy-Item $engineManifest.Path ([IO.Path]::ChangeExtension((Join-Path $tuner $engineName), '.manifest.txt')) -Force
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
        games         = 30
        tc            = 3      # 3+0.03 (weather-factory auto inc = tc/100); UNIFIED
                               # with sprt.ps1's default so SPSA optima transfer to
                               # the confirming SPRT (PLAN.md guiding principle #7).
        hash          = 64
        threads       = $Concurrency
        save_rate     = 10
        pgnout        = "file=tuner/games.pgn"
        use_fastchess = $true
        adjudicate    = [bool]$Adjudicate
    } | ConvertTo-Json
    $cutechessJson | Out-File (Join-Path $wfRoot "cutechess.json") -Encoding utf8 -NoNewline
    Write-Host "Wrote cutechess.json"

    # spsa.json is emitted by the overlay's write_spsa_json.py, not here: the
    # a/c/A back-solve must exist exactly once (in SpsaParams.from_end_state),
    # and PowerShell cannot write this schema anyway — ConvertTo-Json can't hold
    # both "a" and "A" because hashtable keys are case-insensitive.
    Push-Location $wfRoot
    try {
        python write_spsa_json.py --iterations $Iterations --r-end $REnd --out spsa.json
        if ($LASTEXITCODE -ne 0) { throw "write_spsa_json.py failed." }
    } finally {
        Pop-Location
    }
    if ($Resume) {
        Write-Host "  (a resumed run reads state.json, NOT this file — see the warning above)"
    }

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
    $launchCuteContent -notmatch 'BASILISK_NO_ADJUDICATION_DEFAULT_V1' -or
    $launchCuteContent -notmatch [regex]::Escape("-use-affinity $expectedAffinityCpus ")) {
    throw "weather-factory is not carrying the verified affinity/no-adjudication patches; run tools/setup_tools.ps1."
}
python -m py_compile $launchCute
if ($LASTEXITCODE -ne 0) { throw "weather-factory Python syntax validation failed: $launchCute" }

# The launch path re-checks the overlay too: -LaunchOnly skips setup entirely,
# and a clone re-created between setup and launch would otherwise run the
# upstream schedule without a word.
Assert-WfOverlay -WeatherFactoryDir $wfRoot

$launchConfigPath = Join-Path $wfRoot "cutechess.json"
$launchConfig = Get-Content $launchConfigPath -Raw | ConvertFrom-Json
if ([int]$launchConfig.threads -ne $Concurrency) {
    throw "cutechess.json concurrency is $($launchConfig.threads), but this launch resolved to $Concurrency. " +
          "Run setup again, or pass -Concurrency $($launchConfig.threads) explicitly to resume that run."
}
if ($ExpectBench -le 0) { throw "-ExpectBench is required for the tune binary." }
Assert-HarnessHostIdle -MaxBusyPercent $MaxHostBusyPercent -Allow:$AllowBusyHost | Out-Null
$launchEngine = Join-Path $wfRoot ("tuner\" + $launchConfig.engine)
Assert-EngineProvenance -Path $launchEngine -Label $launchConfig.engine `
    -AllowDirtyTree:$AllowDirtyTree -ExpectRevision $ExpectRevision -ExpectBench $ExpectBench | Out-Null
$advertised = @(Get-EngineUciOptions -Path $launchEngine -Detailed)
$surface = Get-Content -LiteralPath (Join-Path $wfRoot 'config.json') -Raw | ConvertFrom-Json
Assert-AdvertisedOptions -Advertised $advertised -Wanted @($surface.PSObject.Properties.Name) -Label $launchConfig.engine

# The horizon the run stops itself at. A resumed run's schedule comes from
# state.json (frozen at first launch), so the target comes from there too when
# it exists — passing a different -Iterations must not silently extend a run
# onto a schedule that was solved for a different N. main.py enforces the same
# precedence independently; this is only what gets reported before launch.
$launchTarget = $Iterations
$launchState = Get-WfTunerState -WeatherFactoryDir $wfRoot
if ($launchState -and [int]$launchState["N"] -gt 0) {
    $launchTarget = [int]$launchState["N"]
}
$env:WF_TARGET_ITERATIONS = "$launchTarget"

# Read spsa.json back off disk and assert the schedule it actually holds, right
# before the tuner consumes it (Rarog 2026-07-30: their spsa.json came out with
# A ~ 0.0965 instead of 500 — no damping at all — because PowerShell folded $a
# and $A into one variable. Our back-solve has always been in Python so we were
# never exposed, but a silently-wrong schedule costs a 40-hour run that looks
# completely normal while it burns, so it is now checked rather than assumed.)
# Skipped on a resume: main.py reads state.json, not this file.
if (-not $launchState) {
    Push-Location $wfRoot
    try {
        python write_spsa_json.py --verify-only --out spsa.json
        if ($LASTEXITCODE -ne 0) {
            throw "spsa.json failed verification — do NOT start this tune. Re-run setup."
        }
    } finally {
        Pop-Location
    }
}

Write-Host "SPSA ($ConfigGroup): python main.py | watch.ps1  (Ctrl-C to stop early)"
Write-Host "  Log: $LogFile (appended; a fresh setup rotates the old one into tuner\archive_*)"
Write-Host "  Target: $launchTarget iterations — the run STOPS ITSELF there."
Write-Host "  State saved every 10 iterations -> tuner\state.json (resume with -Resume)"
Write-Host "  Trajectory appended per iteration -> tuner\trajectory.csv (bake its tail mean)"
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
    # -Append (9.1): the parameter trajectory the tail-mean bake reads used to
    # live only in this log, and watch.ps1 reopened it in truncate mode — so a
    # resume destroyed the earlier part of the very record the bake needs.
    python main.py 2>&1 | & $watch -LogFile $LogFile -Append
} finally {
    Pop-Location
    $env:PATH = $savedPath
}
