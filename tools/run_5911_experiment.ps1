<#
.SYNOPSIS
    Run the whole 5.9.11 three-arm label-source experiment unattended.

.DESCRIPTION
    One-off convenience runner for BAS-E17. Generates all three corpora and
    extracts all three to Texel CSVs, then prints a summary. Roughly 5-6 hours.

    The single variable across arms is which engine's games produce the WDL
    labels; starts, adjudication and extraction parameters are identical:

      A  Basilisk 1.9.3       8,000 nodes   ~43 min
      B  Stockfish dev        8,000 nodes   ~2h17m
      C  Basilisk 1.9.3      25,000 nodes   ~1h51m

    Arms are INDEPENDENT. A failure in one does not stop the others; the run
    continues and the final summary reports what succeeded. Nothing is retried
    automatically -- a partial corpus is a diagnosis, not something to paper
    over by rerunning silently.

    Already-finished stages are skipped, so re-running after an interruption
    resumes rather than redoing hours of work.

    Full engine output goes to tools\results\5911_<arm>.log; only progress
    markers are printed here.

.PARAMETER Rounds
    Games per arm. Default 125,000, which the BAS-E17 pilots showed satisfies a
    1,000,000-row target in every arm (binding phase is `opening` at ~2.04-2.10
    positions/game).

.PARAMETER SkipExtract
    Generate the PGNs but do not extract.
#>
param(
    [int]$Rounds     = 125000,
    [int]$TargetTrain = 1000000,
    [string]$Stockfish = "D:\chess\engines\stockfish.exe",
    [switch]$SkipExtract
)

$ErrorActionPreference = "Continue"
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot

$dataDir    = Join-Path $repoRoot "tools\texel\data"
$resultsDir = Join-Path $repoRoot "tools\results"
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

$arms = @(
    @{ Name="A"; Tag="armA_basilisk8k";   Suffix="5.9.11-datagen"; Engine="";         Nodes=8000;  Seed=101; Desc="Basilisk 1.9.3 @ 8k" }
    @{ Name="B"; Tag="armB_stockfish8k";  Suffix="sf-oracle";      Engine=$Stockfish; Nodes=8000;  Seed=102; Desc="Stockfish dev @ 8k" }
    @{ Name="C"; Tag="armC_basilisk25k";  Suffix="5.9.11-datagen"; Engine="";         Nodes=25000; Seed=103; Desc="Basilisk 1.9.3 @ 25k" }
)

$started = Get-Date
$status  = @{}

function Say($msg) {
    $ts = (Get-Date).ToString("HH:mm:ss")
    Write-Host "[$ts] $msg"
}

Say "5.9.11 label-source experiment - $($arms.Count) arms x $Rounds rounds"
Say "Expect roughly 5-6 hours. Logs: tools\results\5911_<arm>.log"
Write-Host ""

# ---- Phase 1: generate ------------------------------------------------------
foreach ($arm in $arms) {
    $pgn = Join-Path $dataDir "$($arm.Tag).pgn"
    $log = Join-Path $resultsDir "5911_$($arm.Tag).log"

    if (Test-Path -LiteralPath $pgn) {
        Say "arm $($arm.Name): PGN already present, skipping generation"
        $status["gen_$($arm.Name)"] = "skipped (already present)"
        continue
    }

    Say "arm $($arm.Name) [$($arm.Desc)]: generating $Rounds games ..."
    $t0 = Get-Date

    $params = @{
        Suffix       = $arm.Suffix
        Rounds       = $Rounds
        Nodes        = $arm.Nodes
        Seed         = $arm.Seed
        Book         = "tools\texel\data\beast_seed_2m.epd"
        BookFormat   = "epd"
        Adjudication = "none"
        OutputPgn    = $pgn
    }
    if ($arm.Engine) { $params.EnginePath = $arm.Engine }

    & "$PSScriptRoot\datagen.ps1" @params *>&1 | Tee-Object -FilePath $log | Out-Null

    $mins = [math]::Round(((Get-Date) - $t0).TotalMinutes, 1)
    if (Test-Path -LiteralPath $pgn) {
        $mb = [math]::Round((Get-Item $pgn).Length / 1MB, 0)
        Say "arm $($arm.Name): done in $mins min ($mb MB)"
        $status["gen_$($arm.Name)"] = "ok ($mins min, $mb MB)"
    } else {
        Say "arm $($arm.Name): FAILED after $mins min - see $log"
        $status["gen_$($arm.Name)"] = "FAILED (see $log)"
    }
}

# ---- Phase 2: extract -------------------------------------------------------
if (-not $SkipExtract) {
    Write-Host ""
    foreach ($arm in $arms) {
        $pgn   = Join-Path $dataDir "$($arm.Tag).pgn"
        $train = "$($arm.Tag)_train.csv"
        $hold  = "$($arm.Tag)_holdout.csv"
        $log   = Join-Path $resultsDir "5911_$($arm.Tag)_extract.log"

        if (-not (Test-Path -LiteralPath $pgn)) {
            Say "arm $($arm.Name): no PGN, skipping extraction"
            $status["ext_$($arm.Name)"] = "skipped (no PGN)"
            continue
        }
        if (Test-Path -LiteralPath (Join-Path $dataDir $train)) {
            Say "arm $($arm.Name): CSV already present, skipping extraction"
            $status["ext_$($arm.Name)"] = "skipped (already present)"
            continue
        }

        Say "arm $($arm.Name): extracting to $train ..."
        $t0 = Get-Date

        # Run from tools\texel: extract_parallel imports `extract` as a sibling.
        Push-Location (Join-Path $repoRoot "tools\texel")
        & python extract_parallel.py $pgn `
            --out-dir data --train $train --holdout $hold `
            --target-train $TargetTrain --holdout-pct 5 *>&1 |
            Tee-Object -FilePath $log | Out-Null
        Pop-Location

        $mins = [math]::Round(((Get-Date) - $t0).TotalMinutes, 1)
        $trainPath = Join-Path $dataDir $train
        if (Test-Path -LiteralPath $trainPath) {
            $rows = (Get-Content -LiteralPath $trainPath -ReadCount 0).Count
            Say "arm $($arm.Name): extracted $rows rows in $mins min"
            $status["ext_$($arm.Name)"] = "ok ($rows rows, $mins min)"
        } else {
            # extract publishes atomically: a missing CSV means a quota was
            # unreachable, which is information, not a transient error.
            Say "arm $($arm.Name): EXTRACTION FAILED - see $log"
            $status["ext_$($arm.Name)"] = "FAILED (see $log)"
        }
    }
}

# ---- Summary ----------------------------------------------------------------
$elapsed = [math]::Round(((Get-Date) - $started).TotalHours, 2)
Write-Host ""
Write-Host ("=" * 64)
Write-Host "  5.9.11 experiment finished - $elapsed hours"
Write-Host ("=" * 64)
foreach ($arm in $arms) {
    Write-Host ("  arm {0} [{1}]" -f $arm.Name, $arm.Desc)
    Write-Host ("      generate : {0}" -f $status["gen_$($arm.Name)"])
    if (-not $SkipExtract) {
        Write-Host ("      extract  : {0}" -f $status["ext_$($arm.Name)"])
    }
}
Write-Host ("=" * 64)
$bad = ($status.Values | Where-Object { $_ -like "FAILED*" }).Count
if ($bad -gt 0) {
    Write-Host "  $bad stage(s) failed - do NOT treat partial corpora as comparable."
} else {
    Write-Host "  All arms ready. Next: fits, bakes, builds, then the three SPRTs."
}
Write-Host ""

Pop-Location
