<#
.SYNOPSIS
    Generate a self-play PGN dataset for Texel tuning (Step 2.3).

.DESCRIPTION
    Runs fastchess self-play between two copies of the given engine at a fixed
    node limit, collecting a large PGN file suitable for tools\texel\extract.py.

    The output PGN is written to tools\texel\data\selfplay.pgn (or -OutputPgn).
    Subsequent runs APPEND to the existing file; delete it first if starting fresh.

    Adjudication: draw after movenumber 40 with 8 move window at score < 10 cp,
    resign after 3 moves at score > 600 cp (both sides). These defaults match the
    SPRT/gauntlet scripts.

.PARAMETER Suffix
    Engine binary suffix. Looks for
    tools\test_engines\basilisk-<Suffix>-pext-pgo.exe.
    Build with:  .\tools\build_test.ps1 -Suffix <Suffix>

.PARAMETER Rounds
    Number of opening pairs (each pair = 2 games, colors swapped). Default 30000
    gives ~60k games, enough for ~1.5M training positions.

.PARAMETER Nodes
    Node limit per move. Default 8000 (fast, diverse). Values 5000-12000 add
    variety; combine multiple runs with different nodes for the train split.

.PARAMETER Hash
    Hash table size per engine in MB. Default 16 (small enough to keep per-game
    state mostly cache-hot at this node count).

.PARAMETER Concurrency
    Parallel games. Default: logical CPU count minus 1 (leave one core free).

.PARAMETER OutputPgn
    Path for the output PGN file (appended to if it exists).
    Default: tools\texel\data\selfplay.pgn

.PARAMETER Book
    Opening book PGN/EPD. Default: tools\books\SuperGM_4mvs.pgn

.PARAMETER BookFormat
    Opening book format passed to fastchess: pgn or epd. Default: pgn.

.PARAMETER FastchessPath
    Path to fastchess.exe. Default: tools\bin\fastchess.exe

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
    [string]$FastchessPath = ""
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
    $enginePath   = (Resolve-Path $enginePath).Path
    $Book         = (Resolve-Path $Book).Path
    $FastchessPath = (Resolve-Path $FastchessPath).Path

    # ---- Diversity guard ----------------------------------------------------
    # Self-play between two identical engines at a fixed node limit is
    # DETERMINISTIC: a given opening always yields the same game. So the number
    # of DISTINCT games is capped by the number of distinct openings in the
    # book, NOT by -Rounds. Running -Rounds far above the opening count just
    # replays the same handful of games (e.g. 300k rounds over a 6.5k-opening
    # book produced only ~1.5k distinct games -> a near-useless tuning set).
    # Use a large, diverse book (the sampled beast_seed.epd, ~100k+ positions);
    # add variety with extra passes at a DIFFERENT -Nodes value if needed.
    if ($BookFormat -eq "epd") {
        $openingCount = (Get-Content -LiteralPath $Book | Where-Object { $_.Trim() }).Count
    } else {
        $openingCount = (Select-String -LiteralPath $Book -Pattern '^\[Event ').Count
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

    $games = $Rounds * 2
    Write-Host ""
    Write-Host "============================================================"
    Write-Host "  Basilisk Texel datagen — self-play"
    Write-Host "  Engine  : $enginePath"
    Write-Host "  Rounds  : $Rounds  ($games games)"
    Write-Host "  Nodes   : $Nodes per move"
    Write-Host "  Hash    : $Hash MB"
    Write-Host "  Conc.   : $Concurrency"
    Write-Host "  Book    : $(Split-Path $Book -Leaf) ($BookFormat)"
    Write-Host "  Output  : $OutputPgn"
    Write-Host "============================================================"
    Write-Host ""

    & $FastchessPath `
        -engine "cmd=$enginePath" "name=A" "option.Hash=$Hash" "option.Threads=1" `
        -engine "cmd=$enginePath" "name=B" "option.Hash=$Hash" "option.Threads=1" `
        -each "tc=inf" "nodes=$Nodes" `
        -openings "file=$Book" "format=$BookFormat" order=random `
        -rounds $Rounds -games 2 -repeat `
        -concurrency $Concurrency `
        -draw movenumber=40 movecount=8 score=10 `
        -resign movecount=3 score=600 twosided=true `
        -pgnout "file=$OutputPgn" `
        -output format=fastchess

    if ($LASTEXITCODE -ne 0) {
        throw "fastchess exited with code $LASTEXITCODE."
    }

    Write-Host ""
    Write-Host "Done. PGN: $OutputPgn"

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
