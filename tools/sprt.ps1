<#
.SYNOPSIS
    Run an SPRT self-play match between two Basilisk binaries using fastchess.

.DESCRIPTION
    Starts a fastchess match with the built-in SPRT stopping rule.  The match
    runs until the test accepts H0 (no meaningful improvement) or H1
    (improvement).  Real-time output is printed to the console.

    Tooling:
      - fastchess (NOT cutechess-cli): faster, no Qt dependency, built-in SPRT.
        Download a release from https://github.com/Disservin/fastchess/releases
        and place it at $FastchessPath (default D:\chess\fastchess\fastchess.exe),
        or pass -FastchessPath. The cutechess GUI is still handy for *viewing*
        the resulting PGNs, but is not used to run matches.

    Conditions (unified with SPSA as of 2026-06-17):
      - tc=3+0.03 -> 3 s + 30 ms/move increment, CLOCK-based (default $TC).
                   This is the same TC the SPSA tuner uses, so there is no
                   tune->confirm transfer gap, and it exercises the real
                   clock time-management code. 1% increment follows the
                   Stockfish convention and reaches a representative depth.
      - Pass -MoveTime 0.1 for the optional fixed 100 ms/move sanity gauntlet
        under the old Little Blitzer-style condition. This disables the clock.
      - LTC confirmation runs at tc=10+0.1 (pass -TC "10+0.1") at phase
        boundaries and for TC-suspect features.
      - Hash 64 MB, Threads 1, UHO_Lichess_4852_v1.epd opening book (random order),
        each opening played from both colours (-games 2 -repeat).
      - model=normalized (nElo) — fastchess default, more time-control-robust
        than logistic Elo.

    IMPORTANT — concurrency:
      In a self-play game only the side to move computes, so ~16 concurrent
      games already saturate 16 physical cores. Oversubscribing halves NPS and
      changes the depth reached, distorting results. Keep -Concurrency <=
      PHYSICAL cores - 1. This machine has 16 physical cores, so the default is
      15. Do NOT raise it to the 30 logical processors.

    CALIBRATION CHECK — run this FIRST, before testing any feature:
        ./tools/sprt.ps1 `
            -EngineA "tools\test_engines\basilisk-v1.4.9-copy.exe" `
            -EngineB "tools\test_engines\basilisk-v1.4.9.exe" `
            -NameA "Self" -NameB "Self2" -Elo0 -3 -Elo1 3
        Expected: H0 accepted (~0 Elo). If the harness returns H1, something
        is wrong — fix it before trusting any result.

.PARAMETER EngineA
    Path to the new/candidate engine (usually in D:\chess\engines\test_engines).

.PARAMETER EngineB
    Path to the baseline engine (the current integration head, or a released
    reference in D:\chess\engines).

.PARAMETER NameA / NameB
    Display names. Defaults: "New" / "Base".

.PARAMETER Mode
    "gainer"       -> H0: elo<=0,  H1: elo>=Elo1  (default; test a real gain).
    "simplify"     -> H0: elo<=-5, H1: elo>=0     (non-regression / cleanup).
    The explicit -Elo0/-Elo1 parameters override the mode if supplied.

.PARAMETER Elo1
    Upper SPRT bound for "gainer" mode. Default 5 (nElo). Use 3 for small,
    incremental features (e.g. a single tuned search constant) to demand a
    cleaner signal.

.PARAMETER Hash
    Hash MB per engine. Default 64 (matches deployment).

.PARAMETER Concurrency
    Parallel games. Default 15 (physical cores - 1 on this machine).

.PARAMETER TC
    Clock time control "base+inc" in seconds. Default "3+0.03" (the unified
    SPSA/SPRT TC). Use "10+0.1" for an LTC phase gate. Ignored if -MoveTime is
    supplied.

.PARAMETER MoveTime
    Fixed seconds-per-move. Default 0 (use clock TC instead). Set 0.1 for the
    optional fixed 100 ms/move sanity gauntlet; this disables clock time
    management.

.PARAMETER TimeMargin
    fastchess timeout margin in milliseconds. Default 20. This absorbs small
    Windows scheduler / process IO jitter without changing the engine budget.

.PARAMETER Book
    Opening book. Default tools\books\UHO_Lichess_4852_v1.epd (repo-local,
    gitignored; a backup copy lives in D:\chess\books). The Stockfish/
    OpenBench standard Unbalanced Human Openings set: ~2.6M positions curated to
    a built-in ~+0.5..+1.0 imbalance, so games are decisive and each carries far
    more SPRT signal than a balanced book). Format (.epd/.pgn) is auto-detected
    from the extension. Pass a .pgn (e.g. the old SuperGM/IM books) to override.

.PARAMETER FastchessPath
    Path to fastchess.exe. Default D:\chess\fastchess\fastchess.exe (or found on PATH).

.EXAMPLE
    ./tools/sprt.ps1 `
        -EngineA "tools\test_engines\basilisk-phase1-pext-pgo.exe" `
        -EngineB "tools\test_engines\basilisk-head-pext-pgo.exe" `
        -NameA "Phase1" -NameB "Head" -Elo1 3
#>
param(
    [Parameter(Mandatory)][string]$EngineA,
    [Parameter(Mandatory)][string]$EngineB,
    [string]$NameA = "New",
    [string]$NameB = "Base",
    [ValidateSet("gainer", "simplify")][string]$Mode = "gainer",
    [Nullable[int]]$Elo0 = $null,
    [Nullable[int]]$Elo1 = $null,
    [double]$Alpha = 0.05,
    [double]$Beta  = 0.05,
    [int]$Hash = 64,
    [int]$Concurrency = 15,
    [string]$TC = "3+0.03",
    [double]$MoveTime = 0,
    [int]$TimeMargin = 20,
    [string]$Book = "$PSScriptRoot\books\UHO_Lichess_4852_v1.epd",
    [string]$FastchessPath = "$PSScriptRoot\bin\fastchess.exe"
)

$ErrorActionPreference = "Stop"

# Resolve SPRT bounds from mode unless explicitly overridden.
if ($null -eq $Elo0) { $Elo0 = if ($Mode -eq "simplify") { -5 } else { 0 } }
if ($null -eq $Elo1) { $Elo1 = if ($Mode -eq "simplify") {  0 } else { 5 } }

# Resolve the time control: clock (default) unless a fixed movetime is given.
if ($MoveTime -gt 0) {
    $tcArg   = "st=$MoveTime"
    $tcLabel = "st=$MoveTime (fixed ${MoveTime}s/move)"
} else {
    $tcArg   = "tc=$TC"
    $tcLabel = "tc=$TC (clock)"
}

# Locate fastchess.
$fastchess = $FastchessPath
if (-not (Test-Path $fastchess)) {
    $onPath = Get-Command fastchess -ErrorAction SilentlyContinue
    if ($onPath) { $fastchess = $onPath.Source }
    else {
        throw "fastchess not found at '$FastchessPath' or on PATH. Download from " +
              "https://github.com/Disservin/fastchess/releases and place it there."
    }
}
foreach ($p in @($EngineA, $EngineB, $Book)) {
    if (-not (Test-Path $p)) { throw "Not found: $p" }
}

$EngineA = (Resolve-Path $EngineA).Path
$EngineB = (Resolve-Path $EngineB).Path
$Book    = (Resolve-Path $Book).Path

# fastchess needs the opening format told explicitly; derive it from the file
# extension so .epd (UHO and friends) and .pgn books both just work.
$bookFormat = if ([IO.Path]::GetExtension($Book) -ieq ".epd") { "epd" } else { "pgn" }

$resultsDir = Join-Path $PSScriptRoot "results"
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$pgnOut    = Join-Path $resultsDir "sprt_${NameA}_vs_${NameB}_${timestamp}.pgn"
$logOut    = Join-Path $resultsDir "sprt_${NameA}_vs_${NameB}_${timestamp}.log"

# Reproducibility manifest (PLAN §1 gate 8): a PGN without this manifest is not
# a reproducible test. Emitted next to the PGN before the match starts.
$manifestPath = [IO.Path]::ChangeExtension($pgnOut, ".manifest.txt")
function Get-Sha256($path) { if (Test-Path $path) { (Get-FileHash $path -Algorithm SHA256).Hash } else { "missing" } }
function Get-EngineBench($exe) {
    $out = ("bench`nquit" | & $exe 2>&1)
    ($out | Select-String -Pattern 'Nodes searched\s*:\s*(\d+)').Matches.Groups[1].Value
}
$fcVersion = (& $fastchess --version 2>&1 | Select-Object -First 1)
$repoSha   = (git rev-parse HEAD 2>$null); if (-not $repoSha) { $repoSha = "n/a" } else { $repoSha = $repoSha.Trim() }
@(
    "sprt_mode:     $Mode"
    "engineA:       $NameA = $EngineA"
    "engineA_sha256: $(Get-Sha256 $EngineA)"
    "engineA_bench: $(Get-EngineBench $EngineA)"
    "engineB:       $NameB = $EngineB"
    "engineB_sha256: $(Get-Sha256 $EngineB)"
    "engineB_bench: $(Get-EngineBench $EngineB)"
    "repo_revision: $repoSha"
    "sprt_bounds:   elo0=$Elo0 elo1=$Elo1 alpha=$Alpha beta=$Beta model=normalized"
    "time_control:  $tcLabel  timemargin=${TimeMargin}ms"
    "hash_mb:       $Hash"
    "threads:       1"
    "concurrency:   $Concurrency"
    "book:          $Book"
    "book_sha256:   $(Get-Sha256 $Book)"
    "opening_order: random"
    "adjudication:  draw(mn=40,mc=8,score=10) resign(mc=3,score=600,twosided)"
    "fastchess:     $fcVersion"
    "pgn:           $pgnOut"
    "started_utc:   $((Get-Date).ToUniversalTime().ToString('u'))"
) | Set-Content -Path $manifestPath -Encoding utf8

Write-Host ""
Write-Host "======================================================="
Write-Host "  SPRT ($Mode): $NameA  vs  $NameB"
Write-Host "  Manifest: $manifestPath"
Write-Host "  H0: elo<=$Elo0   H1: elo>=$Elo1   alpha=$Alpha  beta=$Beta  (nElo)"
Write-Host "  TC: $tcLabel   Margin: ${TimeMargin} ms   Hash: ${Hash} MB   Conc: $Concurrency"
Write-Host "  Book: $(Split-Path $Book -Leaf)"
Write-Host "  Runner: $fastchess"
Write-Host "  PGN:  $pgnOut"
Write-Host "  Log:  $logOut  (full output; console shows report blocks only)"
Write-Host "======================================================="
Write-Host ""

# Console-noise filter (2026-07-17, ported from Rarog): the per-game 'Started
# game …' / normal 'Finished game … {Draw/wins by adjudication}' / 'Score of …'
# lines bury the periodic Elo/LLR report blocks and make it impossible to scroll
# back through how the result progressed. So: TEE the FULL stream to $logOut
# (nothing lost — grep/scroll it for detail), and on the CONSOLE keep everything
# EXCEPT that per-game noise. Keep-by-default is deliberate — report blocks,
# errors, and any time-loss / disconnect / illegal 'Finished game' lines (the
# SPRT canaries) all still print. `-ratinginterval 20` reports state every 20
# games so the console shows a clean progression of report blocks.
$dropNoise = {
    param($l)
    $l = "$l"
    if ($l -match '^\s*Started game \d+ of') { return $true }
    if ($l -match '^\s*Score of .+ vs .+:\s*\d+ - \d+ - \d+') { return $true }
    # Drop only NORMAL game finishes; keep anomalies (time loss / disconnect /
    # illegal / crash / forfeit / stall) so a poisoned SPRT is still visible.
    if (($l -match '^\s*Finished game \d') -and
        ($l -notmatch '(?i)(on time|timeout|disconnect|illegal|crash|forfeit|stall)')) { return $true }
    return $false
}

& $fastchess `
    -engine "cmd=$EngineA" "name=$NameA" "option.Hash=$Hash" "option.Threads=1" `
    -engine "cmd=$EngineB" "name=$NameB" "option.Hash=$Hash" "option.Threads=1" `
    -each $tcArg "timemargin=$TimeMargin" `
    -openings "file=$Book" "format=$bookFormat" order=random `
    -rounds 50000 -games 2 -repeat `
    -concurrency $Concurrency `
    -ratinginterval 20 `
    -sprt "elo0=$Elo0" "elo1=$Elo1" "alpha=$Alpha" "beta=$Beta" model=normalized `
    -draw movenumber=40 movecount=8 score=10 `
    -resign movecount=3 score=600 twosided=true `
    -pgnout "file=$pgnOut" `
    -output format=fastchess 2>&1 |    # console ticker format (not the PGN path)
    Tee-Object -FilePath $logOut |
    Where-Object { -not (& $dropNoise $_) }
# $LASTEXITCODE reflects fastchess (Tee-Object/Where-Object are cmdlets and do
# not touch it), so the exit check below stays valid.

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Error "fastchess exited with code $LASTEXITCODE — no games were played."
} else {
    Write-Host ""
    Write-Host "Match finished. PGN saved to: $pgnOut"
    Write-Host "Full console log (all per-game lines): $logOut"
}
