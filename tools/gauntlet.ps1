<#
.SYNOPSIS
    Run fixed-game validation matches against one or more reference binaries.

.DESCRIPTION
    This is the phase-boundary validation runner. Unlike sprt.ps1, it does not use
    SPRT stopping. It runs a fixed number of paired games per opponent so the
    accepted head can be sanity-checked against older Basilisk builds.

.PARAMETER Engine
    Candidate engine to test.

.PARAMETER Opponents
    One or more opponent engine paths.

.PARAMETER Name
    Display name for the candidate.

.PARAMETER Games
    Games per opponent. Rounded up to an even number because openings are
    repeated with colors swapped. Default: 1000.

.PARAMETER Hash
    Hash MB per engine. Default 64.

.PARAMETER Concurrency
    Parallel games. Default 15 for this 16-physical-core machine.

.PARAMETER TC
    Clock time control "base+inc" in seconds. Default "10+0.1" — the
    phase-boundary/LTC clock condition (PLAN.md §10), which exercises the real
    clock time-management code (forfeits only happen on this path, never on
    fixed movetime). Ignored if -MoveTime is supplied.

.PARAMETER MoveTime
    Fixed seconds-per-move. Default 0 (use clock TC instead). Set 0.1 for the
    optional old fixed-movetime sanity gauntlet; this disables clock time
    management and forfeits, so it cannot validate a time-safety fix.

.PARAMETER TimeMargin
    fastchess timeout margin in milliseconds. Default 20. Absorbs small
    Windows scheduler / process IO jitter without changing the engine budget.

.PARAMETER Book
    Opening book PGN. Default: tools\books\SuperGM_4mvs.pgn.

.PARAMETER FastchessPath
    Path to fastchess.exe. Default: tools\bin\fastchess.exe.

.EXAMPLE
    .\tools\gauntlet.ps1 `
        -Engine tools\test_engines\basilisk-phase1-final-pext-pgo.exe `
        -Opponents tools\test_engines\basilisk-phase1-defaults-pext-pgo.exe `
        -Name Phase1Final `
        -Games 1000

.EXAMPLE
    # Optional old fixed-movetime sanity gauntlet (no forfeits possible).
    .\tools\gauntlet.ps1 `
        -Engine tools\test_engines\basilisk-phase1-final-pext-pgo.exe `
        -Opponents tools\test_engines\basilisk-phase1-defaults-pext-pgo.exe `
        -Name Phase1Final -Games 1000 -MoveTime 0.1
#>
param(
    [Parameter(Mandatory)][string]$Engine,
    [Parameter(Mandatory)][string[]]$Opponents,
    [string]$Name = "Candidate",
    [int]$Games = 1000,
    [int]$Hash = 64,
    [int]$Concurrency = 15,
    [string]$TC = "10+0.1",
    [double]$MoveTime = 0,
    [int]$TimeMargin = 20,
    [string]$Book = "$PSScriptRoot\books\SuperGM_4mvs.pgn",
    [string]$FastchessPath = "$PSScriptRoot\bin\fastchess.exe"
)

$ErrorActionPreference = "Stop"

if ($Games -lt 2) { throw "-Games must be at least 2." }
if ($Games % 2 -ne 0) { $Games += 1 }
$rounds = [int]($Games / 2)

# Resolve the time control: clock (default) unless a fixed movetime is given.
if ($MoveTime -gt 0) {
    $tcArg   = "st=$MoveTime"
    $tcLabel = "st=$MoveTime (fixed ${MoveTime}s/move)"
} else {
    $tcArg   = "tc=$TC"
    $tcLabel = "tc=$TC (clock)"
}

$fastchess = $FastchessPath
if (-not (Test-Path $fastchess)) {
    $onPath = Get-Command fastchess -ErrorAction SilentlyContinue
    if ($onPath) { $fastchess = $onPath.Source }
    else { throw "fastchess not found at '$FastchessPath' or on PATH." }
}

foreach ($p in @($Engine, $Book) + $Opponents) {
    if (-not (Test-Path $p)) { throw "Not found: $p" }
}

$Engine = (Resolve-Path $Engine).Path
$Book = (Resolve-Path $Book).Path

$resultsDir = Join-Path $PSScriptRoot "results"
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

foreach ($opponent in $Opponents) {
    $opponentPath = (Resolve-Path $opponent).Path
    $opponentName = [IO.Path]::GetFileNameWithoutExtension($opponentPath)
    $safeOpponent = $opponentName -replace '[^A-Za-z0-9_.-]', '_'
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $pgnOut = Join-Path $resultsDir "gauntlet_${Name}_vs_${safeOpponent}_${timestamp}.pgn"

    Write-Host ""
    Write-Host "======================================================="
    Write-Host "  Gauntlet: $Name vs $opponentName"
    Write-Host "  Games: $Games   TC: $tcLabel   Margin: ${TimeMargin} ms   Hash: ${Hash} MB   Conc: $Concurrency"
    Write-Host "  Book: $(Split-Path $Book -Leaf)"
    Write-Host "  PGN:  $pgnOut"
    Write-Host "======================================================="
    Write-Host ""

    & $fastchess `
        -engine "cmd=$Engine" "name=$Name" "option.Hash=$Hash" "option.Threads=1" `
        -engine "cmd=$opponentPath" "name=$opponentName" "option.Hash=$Hash" "option.Threads=1" `
        -each $tcArg "timemargin=$TimeMargin" `
        -openings "file=$Book" format=pgn order=random `
        -rounds $rounds -games 2 -repeat `
        -concurrency $Concurrency `
        -draw movenumber=40 movecount=8 score=10 `
        -resign movecount=3 score=600 twosided=true `
        -pgnout "file=$pgnOut" `
        -output format=fastchess

    if ($LASTEXITCODE -ne 0) {
        throw "fastchess exited with code $LASTEXITCODE for opponent '$opponentName'."
    }
}
