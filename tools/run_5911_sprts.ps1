<#
.SYNOPSIS
    Run the three 5.9.11 label-source gates unattended (BAS-E17).

.DESCRIPTION
    One-off overnight runner. Each arm gets ONE SPRT against the current
    accepted head, per the decision rule registered in BAS-E17 before any of
    this data existed:

      winner = the arm that PASSES with the highest nElo point estimate;
      if NO arm passes, the label-source hypothesis is not supported and the
      next suspect is the extraction contract or the fit, not another corpus.

    Pairwise arm-vs-arm matches are deliberately NOT run here. The registered
    rule makes them conditional on the head gates being ambiguous, and running
    them pre-emptively would invite picking a winner after the fact.

    Binaries are built in advance and merely referenced here, so an overnight
    run has no compile step to fail on. The script verifies every binary exists
    and reports its bench before starting; a missing or unexpected binary aborts
    immediately rather than three hours in.

    Arm order is A, C, B -- decision value first. A is the conservative choice
    (our own labels), C is the predicted winner, B carries the independence
    question and is the one most acceptable to lose if the night runs out.

    THERE IS NO GAME CAP, and an earlier version of this file wrongly claimed
    otherwise. sprt.ps1 honours -Games only in its fixed/calibrate modes; a
    gainer-mode SPRT runs until the LLR crosses a boundary or it hits the
    50,000-round (100,000-game) hard stop.

    Budget accordingly. A decisive arm is quick -- arm A resolved in 20,096
    games (3h45m). An arm sitting near zero does NOT stop: arm C reached 44,800
    games in 8h20m at LLR +0.03 and was still ~10 hours from the hard stop when
    it was halted by decision. Plan for one long arm per night, not three.

.PARAMETER PreflightOnly
    Verify binaries and print their benches, then exit without running anything.

.PARAMETER Elo1
    Upper SPRT bound in nElo. Default 3 -- PLAN's registered gate. Note the
    sprt.ps1 default is 5; this script does not inherit it.
#>
param(
    [int]$Elo1  = 3,
    [switch]$PreflightOnly
)

$ErrorActionPreference = "Continue"
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot

$engines    = Join-Path $repoRoot "tools\test_engines"
$resultsDir = Join-Path $repoRoot "tools\results"
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

$baseName = "5911-base"
$basePath = Join-Path $engines "basilisk-$baseName-pext-pgo.exe"

# Decision value first: see .DESCRIPTION.
$arms = @(
    @{ Name="A"; Tag="5911-armA"; Desc="Basilisk-labelled @ 8k" }
    @{ Name="C"; Tag="5911-armC"; Desc="Basilisk-labelled @ 25k" }
    @{ Name="B"; Tag="5911-armB"; Desc="Stockfish-labelled @ 8k" }
)

function Say($msg) {
    Write-Host ("[{0}] {1}" -f (Get-Date).ToString("HH:mm:ss"), $msg)
}

function Get-Bench($exe) {
    $out = "bench`nquit`n" | & $exe 2>$null
    $m = $out | Select-String -Pattern '^Nodes searched\s*:\s*(\d+)' | Select-Object -First 1
    if ($m) { return [int64]$m.Matches.Groups[1].Value }
    return -1
}

# ---- Preflight: fail now, not in three hours --------------------------------
Say "Preflight: verifying binaries exist and identifying them by bench"
$missing = @()
if (-not (Test-Path -LiteralPath $basePath)) { $missing += $basePath }
foreach ($arm in $arms) {
    $p = Join-Path $engines "basilisk-$($arm.Tag)-pext-pgo.exe"
    if (-not (Test-Path -LiteralPath $p)) { $missing += $p }
}
if ($missing.Count -gt 0) {
    Write-Host ""
    Write-Host "ABORT: missing binaries -- build them before running this script:"
    $missing | ForEach-Object { Write-Host "  $_" }
    Pop-Location
    exit 1
}

$baseBench = Get-Bench $basePath
Say ("  base   $baseName  bench $baseBench")
foreach ($arm in $arms) {
    $p = Join-Path $engines "basilisk-$($arm.Tag)-pext-pgo.exe"
    $b = Get-Bench $p
    Say ("  arm {0}  {1}  bench {2}" -f $arm.Name, $arm.Tag, $b)
    if ($b -eq $baseBench) {
        Write-Host ""
        Write-Host "ABORT: arm $($arm.Name) has the SAME bench as the baseline ($b)."
        Write-Host "That means its fitted values were not baked -- the arms would be"
        Write-Host "identical engines and the SPRT would measure nothing."
        Pop-Location
        exit 1
    }
}

if ($PreflightOnly) {
    Write-Host ""
    Say "Preflight only: all binaries present and distinct. Nothing was run."
    Pop-Location
    exit 0
}

Write-Host ""
Say "Running $($arms.Count) SPRTs, nElo [0, $Elo1], no game cap"
Say "NOTE: a NEUTRAL arm does not stop early. Measured: 8h20m and still running."

Write-Host ""

$started = Get-Date
$verdicts = @{}

foreach ($arm in $arms) {
    $tag  = $arm.Tag
    $exe  = Join-Path $engines "basilisk-$tag-pext-pgo.exe"
    $done = Join-Path $resultsDir "5911_sprt_$($arm.Name).done.txt"

    if (Test-Path -LiteralPath $done) {
        Say "arm $($arm.Name): already completed, skipping"
        $verdicts[$arm.Name] = (Get-Content -LiteralPath $done -Raw).Trim()
        continue
    }

    Say "arm $($arm.Name) [$($arm.Desc)] vs $baseName ..."
    $t0 = Get-Date

    # No -Games: gainer mode cannot honour it (see sprt.ps1). Each arm runs to
    # an SPRT boundary or the 50,000-round hard stop.
    $out = & "$PSScriptRoot\sprt.ps1" `
        -EngineA $exe -EngineB $basePath `
        -NameA $tag -NameB $baseName `
        -Elo1 $Elo1 *>&1

    $mins = [math]::Round(((Get-Date) - $t0).TotalMinutes, 1)

    # Keep the last Elo/LLR/verdict lines; that is what a verdict is made of.
    $elo  = ($out | Select-String -Pattern '^Elo:'   | Select-Object -Last 1).ToString()
    $llr  = ($out | Select-String -Pattern '^LLR:'   | Select-Object -Last 1).ToString()
    # NB: do NOT name this $games -- PowerShell is case-insensitive and it would
    # alias the [int]$Games parameter, throwing on assignment.
    $gameLine = ($out | Select-String -Pattern '^Games:' | Select-Object -Last 1).ToString()
    $sprt = ($out | Select-String -Pattern 'SPRT .*completed' | Select-Object -Last 1)
    $verdict = if ($sprt) { $sprt.ToString().Trim() } else { "cap reached - no SPRT verdict" }

    $summary = @(
        "arm $($arm.Name) [$($arm.Desc)]  vs $baseName"
        "  $elo"
        "  $gameLine"
        "  $llr"
        "  verdict: $verdict"
        "  wall: $mins min"
    ) -join "`n"

    $summary | Set-Content -LiteralPath $done -Encoding utf8
    $verdicts[$arm.Name] = $summary

    Say "arm $($arm.Name): $verdict  ($mins min)"
    Write-Host ""
}

# ---- Summary ----------------------------------------------------------------
$hrs = [math]::Round(((Get-Date) - $started).TotalHours, 2)
Write-Host ("=" * 68)
Write-Host "  5.9.11 label-source gates finished - $hrs hours"
Write-Host ("=" * 68)
foreach ($arm in $arms) {
    Write-Host ""
    Write-Host $verdicts[$arm.Name]
}
Write-Host ""
Write-Host ("=" * 68)
Write-Host "  Decision rule (BAS-E17, registered before the data existed):"
Write-Host "    winner = the arm that PASSES with the highest nElo estimate."
Write-Host "    If NO arm passes, the label-source hypothesis is NOT supported;"
Write-Host "    next suspect is the extraction contract or the fit -- not more data."
Write-Host "    Do not re-fit or re-extract an arm after seeing its result."
Write-Host ("=" * 68)

Pop-Location
