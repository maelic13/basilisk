<#
.SYNOPSIS
    Generate a self-play PGN dataset for Texel tuning (Step 2.3).

.DESCRIPTION
    Runs fastchess self-play between two copies of the given engine at a fixed
    node limit, collecting a large PGN file suitable for tools\texel\extract.py.

    The output PGN is written to tools\texel\data\selfplay.pgn (or -OutputPgn).
    Existing output is refused by default: fixed-node self-play is deterministic,
    so an accidental append mostly adds duplicates. Use -AllowAppend only for an
    intentionally registered multi-pass dataset.

    A provenance manifest is written beside the PGN before fastchess starts and
    completed with the output hash after success.

    Adjudication: draw after movenumber 40 with 8 move window at score < 10 cp,
    resign after 3 moves at score > 600 cp (both sides). These defaults match the
    SPRT/gauntlet scripts.

.PARAMETER Suffix
    Engine binary suffix. Looks for
    tools\test_engines\basilisk-<Suffix>-pext-pgo.exe.
    Build with:  .\tools\build_test.ps1 -Suffix <Suffix>

.PARAMETER Rounds
    Number of distinct openings/games. Datagen uses one game per opening:
    both engine processes run the same binary, so a color-swapped repeat is
    bit-identical and contributes no positions after extraction deduplication.

.PARAMETER Nodes
    Node limit per move. Default 8000 (fast, diverse). Values 5000-12000 add
    variety; combine multiple runs with different nodes for the train split.

.PARAMETER Hash
    Hash table size per engine in MB. Default 16 (small enough to keep per-game
    state mostly cache-hot at this node count).

.PARAMETER Concurrency
    Parallel games. Default: logical CPU count minus 1 (leave one core free).

.PARAMETER OutputPgn
    Path for the output PGN file. Existing output is refused unless
    -AllowAppend is explicit.
    Default: tools\texel\data\selfplay.pgn

.PARAMETER Book
    Opening book PGN/EPD. Default: tools\books\SuperGM_4mvs.pgn

.PARAMETER BookFormat
    Opening book format passed to fastchess: pgn or epd. Default: pgn.

.PARAMETER FastchessPath
    Path to fastchess.exe. Default: tools\bin\fastchess.exe

.PARAMETER Seed
    Opening randomization seed passed to fastchess -srand. Default: 42.

.PARAMETER AllowAppend
    Explicitly allow appending to an existing PGN. Off by default.

.PARAMETER PreflightOnly
    Validate inputs and print the registered run without starting fastchess.

.EXAMPLE
    # Build the base binary first, then generate data
    .\tools\build_test.ps1 -Suffix phase2-base
    .\tools\datagen.ps1 -Suffix phase2-base -Rounds 30000

.EXAMPLE
    # Second pass with a different node count (more variety)
    .\tools\datagen.ps1 -Suffix phase2-base -Rounds 15000 -Nodes 5000
#>
param(
    [Parameter(Mandatory)][string]$Suffix,
    [int]   $Rounds      = 30000,
    [int]   $Nodes       = 8000,
    [int]   $Hash        = 16,
    [int]   $Concurrency = 0,        # 0 = auto (logical CPUs - 1)
    [string]$OutputPgn   = "",
    [string]$Book        = "",
    [ValidateSet("pgn", "epd")]
    [string]$BookFormat  = "pgn",
    [string]$FastchessPath = "",
    [int]   $Seed        = 42,
    [switch]$AllowAppend,
    [switch]$PreflightOnly
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot

try {
    # ---- Defaults resolved relative to repo root ----
    if (-not $Book)          { $Book          = "$PSScriptRoot\books\SuperGM_4mvs.pgn" }
    if (-not $FastchessPath) { $FastchessPath = "$PSScriptRoot\bin\fastchess.exe" }
    if (-not $OutputPgn)     { $OutputPgn     = "$PSScriptRoot\texel\data\selfplay.pgn" }

    $enginePath = "$PSScriptRoot\test_engines\basilisk-$Suffix-pext-pgo.exe"

    foreach ($p in @($Book, $FastchessPath, $enginePath)) {
        if (-not (Test-Path $p)) { throw "Not found: $p" }
    }
    $enginePath    = (Resolve-Path $enginePath).Path
    $Book          = (Resolve-Path $Book).Path
    $FastchessPath = (Resolve-Path $FastchessPath).Path
    # System.IO resolves relative paths against the process working directory,
    # which PowerShell's Push-Location does not update. Anchor explicitly to
    # the repository so `tools\...` can never escape to the parent directory.
    $OutputPgn = if ([IO.Path]::IsPathFullyQualified($OutputPgn)) {
        [IO.Path]::GetFullPath($OutputPgn)
    } else {
        [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputPgn))
    }
    $manifestPath  = "$OutputPgn.manifest.txt"
    $engineManifest = [IO.Path]::ChangeExtension($enginePath, ".manifest.txt")

    if (-not (Test-Path -LiteralPath $engineManifest)) {
        throw "Engine manifest not found: $engineManifest. Build the datagen engine with tools\build_test.ps1."
    }

    if ((Test-Path -LiteralPath $OutputPgn) -and -not $AllowAppend) {
        throw "Output already exists: $OutputPgn. Move/remove it for a fresh dataset, or pass -AllowAppend for an intentional registered append."
    }
    if ((Test-Path -LiteralPath $manifestPath) -and -not $AllowAppend) {
        throw "Manifest already exists: $manifestPath. Preserve or move it with its PGN before starting a fresh dataset."
    }
    if ($AllowAppend -and ((Test-Path -LiteralPath $OutputPgn) -xor
                           (Test-Path -LiteralPath $manifestPath))) {
        throw "Registered append requires both the existing PGN and its manifest (or neither)."
    }
    if ($AllowAppend -and (Test-Path -LiteralPath $manifestPath)) {
        $priorStatus = Get-Content -LiteralPath $manifestPath |
            Where-Object { $_ -match '^status:\s+' } | Select-Object -Last 1
        if (($priorStatus -replace '^status:\s+', '').Trim() -ne 'complete') {
            throw "Cannot append to a dataset whose prior manifest is not complete: $manifestPath"
        }
    }
    $existingGames = 0
    if ($AllowAppend -and (Test-Path -LiteralPath $OutputPgn)) {
        $rg = Get-Command rg -ErrorAction SilentlyContinue
        if ($rg) {
            $countText = (& $rg.Source --count '^\[Event ' -- $OutputPgn 2>$null | Select-Object -First 1)
            if ($LASTEXITCODE -le 1 -and $countText) {
                $existingGames = [int64]($countText.Trim())
            }
        }
        if ($existingGames -eq 0) {
            $existingGames = (Select-String -LiteralPath $OutputPgn -Pattern '^\[Event ').Count
        }
    }

    # ---- Diversity guard ----------------------------------------------------
    # Self-play between two identical engines at a fixed node limit is
    # DETERMINISTIC: a given opening always yields the same game. So the number
    # of DISTINCT games is capped by the number of distinct openings in the
    # book, NOT by -Rounds. Running -Rounds far above the opening count just
    # replays the same handful of games (e.g. 300k rounds over a 6.5k-opening
    # book produced only ~1.5k distinct games -> a near-useless tuning set).
    # Use a large, diverse book (the sampled beast_seed.epd, ~100k+ positions);
    # add variety with extra passes at a DIFFERENT -Nodes value if needed.
    $openingCount = 0
    $rg = Get-Command rg -ErrorAction SilentlyContinue
    if ($rg) {
        $pattern = if ($BookFormat -eq "epd") { '\S' } else { '^\[Event ' }
        $countText = (& $rg.Source --count $pattern -- $Book 2>$null | Select-Object -First 1)
        if ($LASTEXITCODE -le 1 -and $countText) {
            $openingCount = [int64]($countText.Trim())
        }
    }
    if ($openingCount -eq 0) {
        if ($BookFormat -eq "epd") {
            $openingCount = (Get-Content -LiteralPath $Book | Where-Object { $_.Trim() }).Count
        } else {
            $openingCount = (Select-String -LiteralPath $Book -Pattern '^\[Event ').Count
        }
    }
    if ($openingCount -gt 0 -and $Rounds -gt $openingCount) {
        Write-Host ""
        Write-Host "  !! DIVERSITY WARNING ----------------------------------------" -ForegroundColor Yellow
        Write-Host ("  !! Book has only {0:N0} distinct openings but -Rounds is {1:N0}." -f $openingCount, $Rounds) -ForegroundColor Yellow
        Write-Host "  !! Deterministic self-play will REPLAY openings -> duplicate" -ForegroundColor Yellow
        Write-Host "  !! games and a low-diversity dataset. Use a bigger/diverse" -ForegroundColor Yellow
        Write-Host "  !! book (sample_fens.py -> *.epd) or lower -Rounds to <= the" -ForegroundColor Yellow
        Write-Host "  !! opening count (and add passes at a different -Nodes)." -ForegroundColor Yellow
        Write-Host "  !! -------------------------------------------------------------" -ForegroundColor Yellow
        Write-Host ""
    }

    # NOTE (2026-07-21): datagen deliberately has NO -use-affinity and keeps
    # oversubscribed concurrency: games are NODE-limited (tc=inf), so search
    # decisions are placement/speed-independent by construction - the
    # scheduler lottery that biased clock-TC SPRTs (see sprt.ps1 header)
    # cannot change a single move or label here, and throughput is all that
    # matters. Do not "fix" this.
    # Auto concurrency: logical CPUs - 1, minimum 1
    if ($Concurrency -le 0) {
        $logical = [int]$env:NUMBER_OF_PROCESSORS
        if (-not $logical -or $logical -lt 1) { $logical = 1 }
        $Concurrency = [Math]::Max(1, $logical - 1)
    }

    # Ensure output directory exists
    $outDir = Split-Path -Parent $OutputPgn
    if ($outDir -and -not (Test-Path $outDir)) {
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    }

    # One game per opening is sufficient. A -repeat color swap changes only
    # the A/B process names; both processes run the same deterministic binary,
    # so the moves, WDL label and extracted FENs are identical.
    $games = $Rounds
    Write-Host ""
    Write-Host "============================================================"
    Write-Host "  Basilisk Texel datagen — self-play"
    Write-Host "  Engine  : $enginePath"
    Write-Host "  Rounds  : $Rounds  ($games games)"
    Write-Host "  Nodes   : $Nodes per move"
    Write-Host "  Hash    : $Hash MB"
    Write-Host "  Conc.   : $Concurrency"
    Write-Host "  Book    : $(Split-Path $Book -Leaf) ($BookFormat)"
    Write-Host "  Seed    : $Seed"
    Write-Host "  Append  : $($AllowAppend.IsPresent)"
    Write-Host "  Output  : $OutputPgn"
    Write-Host "  Manifest: $manifestPath"
    Write-Host "============================================================"
    Write-Host ""

    if ($PreflightOnly) {
        Write-Host "Preflight only: inputs are valid; fastchess was not started."
        return
    }

    $engineSha = (Get-FileHash -LiteralPath $enginePath -Algorithm SHA256).Hash
    $bookSha = (Get-FileHash -LiteralPath $Book -Algorithm SHA256).Hash
    $engineManifestSha = (Get-FileHash -LiteralPath $engineManifest -Algorithm SHA256).Hash
    $revision = "unknown"
    $revisionLine = Get-Content -LiteralPath $engineManifest |
        Where-Object { $_ -match '^revision:\s+' } | Select-Object -First 1
    if ($revisionLine) { $revision = ($revisionLine -replace '^revision:\s+', '').Trim() }
    $fastchessVersion = (& $FastchessPath --version 2>&1 | Select-Object -First 1)
    $startedUtc = (Get-Date).ToUniversalTime().ToString('u')
    $appendMode = $AllowAppend.IsPresent.ToString().ToLowerInvariant()
    $commandLine = ".\tools\datagen.ps1 -Suffix $Suffix -Rounds $Rounds -Nodes $Nodes -Hash $Hash -Concurrency $Concurrency -Book `"$Book`" -BookFormat $BookFormat -OutputPgn `"$OutputPgn`" -Seed $Seed"
    if ($AllowAppend) { $commandLine += " -AllowAppend" }
    $manifestLines = @(
        "schema: datagen-v1"
        "status: running"
        "started_utc: $startedUtc"
        "command: $commandLine"
        "revision: $revision"
        "engine: $enginePath"
        "engine_sha256: $engineSha"
        "engine_manifest: $engineManifest"
        "engine_manifest_sha256: $engineManifestSha"
        "bench_contract: fixed-node labels; Threads=1"
        "book: $Book"
        "book_format: $BookFormat"
        "book_openings: $openingCount"
        "book_sha256: $bookSha"
        "rounds: $Rounds"
        "games_planned: $games"
        "games_existing: $existingGames"
        "games_cumulative_planned: $($existingGames + $games)"
        "games_per_opening: 1 (identical-engine color swap is redundant)"
        "nodes_per_move: $Nodes"
        "threads: 1"
        "hash_mb: $Hash"
        "concurrency: $Concurrency"
        "opening_order: random"
        "opening_seed: $Seed"
        "pgn_append: $appendMode"
        "adjudication: draw movenumber=40 movecount=8 score=10; resign movecount=3 score=600 twosided=true"
        "fastchess: $fastchessVersion"
        "output_pgn: $OutputPgn"
    )
    if ($AllowAppend -and (Test-Path -LiteralPath $manifestPath)) {
        Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
            ""
            "append_run: $startedUtc"
        )
        Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value $manifestLines
    } else {
        $manifestLines | Set-Content -LiteralPath $manifestPath -Encoding utf8
    }

    & $FastchessPath `
        -engine "cmd=$enginePath" "name=A" "option.Hash=$Hash" "option.Threads=1" `
        -engine "cmd=$enginePath" "name=B" "option.Hash=$Hash" "option.Threads=1" `
        -each "tc=inf" "nodes=$Nodes" `
        -openings "file=$Book" "format=$BookFormat" order=random `
        -srand $Seed `
        -rounds $Rounds -games 1 `
        -concurrency $Concurrency `
        -draw movenumber=40 movecount=8 score=10 `
        -resign movecount=3 score=600 twosided=true `
        -pgnout "file=$OutputPgn" "append=$appendMode" `
        -output format=fastchess

    if ($LASTEXITCODE -ne 0) {
        Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
            "status: failed"
            "fastchess_exit: $LASTEXITCODE"
            "finished_utc: $((Get-Date).ToUniversalTime().ToString('u'))"
        )
        throw "fastchess exited with code $LASTEXITCODE."
    }

    $pgnSha = (Get-FileHash -LiteralPath $OutputPgn -Algorithm SHA256).Hash
    $pgnBytes = (Get-Item -LiteralPath $OutputPgn).Length
    Add-Content -LiteralPath $manifestPath -Encoding utf8 -Value @(
        "status: complete"
        "fastchess_exit: 0"
        "games_total: $($existingGames + $games)"
        "pgn_bytes: $pgnBytes"
        "pgn_sha256: $pgnSha"
        "finished_utc: $((Get-Date).ToUniversalTime().ToString('u'))"
    )

    Write-Host ""
    Write-Host "Done. PGN: $OutputPgn"
    Write-Host "Manifest: $manifestPath"

    # Print rough position estimate
    try {
        $lineCount = (Get-Content $OutputPgn -Encoding utf8 | Measure-Object -Line).Lines
        # Very rough: ~35-40 qualifying positions per game after filtering
        $estimatedPositions = [int]($games * 35)
        Write-Host ("Lines in PGN : {0:N0}" -f $lineCount)
        Write-Host ("Estimated qualifying positions after extract.py : ~{0:N0}" -f $estimatedPositions)
    } catch { }

} finally {
    Pop-Location
}
