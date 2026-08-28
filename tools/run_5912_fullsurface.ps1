<#
.SYNOPSIS
    Run the 5.9.12 full-surface fit: 1,116 Texel params + 57 king-safety, iterated.

.DESCRIPTION
    The 768 piece-square tables have been frozen since Phase 4.7 and are the last
    large unexplored surface in the evaluation. This fits them jointly with the
    348 scalars, then re-runs king safety on the changed surface, then repeats --
    the two sets interact, so one pass of each is not a converged fit.

    Parameter taxonomy (1,190 fittable in total):

      768  PSTs        gradient   -- frozen since Phase 4.7
      348  scalars     gradient
     ----
     1116  --tune texel                <- stages 1 and 3

       57  king safety  coordinate descent (capped, non-linear -- BAS-X14)
                                        <- stages 2 and 4
        7  winnable     EXCLUDED, see below
       10  material     PINNED: exactly collinear with PST, zero expressiveness lost

    Winnable is excluded deliberately. It is capped and non-linear like king
    safety, but unlike king safety it has no finite-difference instrument -- the
    only way to reach it is `--tune winnable`, which is the gradient path that
    BAS-X14 says corrupts capped coefficients. Seven parameters are not worth
    fitting through a cap. Revisit if a finite-diff instrument is written.

    BAS-E21 is the evidence this split is real, not bookkeeping: the king-safety
    funnel yielded +2.64 Elo through the coordinate-descent fitter after a linear
    fit had found nothing there.

    Every stage bakes and rebuilds, so each fit sees the previous stage's result.
    Holdout loss is reported after every stage so convergence is visible rather
    than assumed.

.PARAMETER Corpus
    Corpus stem under tools\texel\data. Default armC_basilisk25k -- the best
    label source measured in BAS-E18 (our own engine at 25k nodes; the Stockfish
    -labelled arm was the worst of the three).

.PARAMETER Iterations
    Texel+kingsafety rounds. Default 2.

.PARAMETER Epochs
    Gradient epochs per Texel stage. Default 200.

.PARAMETER Smoke
    Validate the whole pipeline in ~2 minutes instead of ~3 hours: 3 gradient
    epochs, a single short king-safety pass on 50k positions, one iteration, and
    the tree restored to HEAD at the end. Proves the bake/rebuild/parse chain
    works before committing real time to it. Results are meaningless.
#>
param(
    [string]$Corpus     = "armC_basilisk25k",
    [int]   $Iterations = 2,
    [int]   $Epochs     = 200,
    [switch]$Smoke
)

if ($Smoke) { $Iterations = 1; $Epochs = 3 }

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot

$train = "tools\texel\data\${Corpus}_train.csv"
$hold  = "tools\texel\data\${Corpus}_holdout.csv"
$outDir = "tools\texel\out"
$texel  = ".\build\texel\basilisk-texel.exe"

foreach ($f in @($train, $hold)) {
    if (-not (Test-Path -LiteralPath $f)) { throw "Corpus file not found: $f" }
}

function Say($m) { Write-Host ("[{0}] {1}" -f (Get-Date).ToString("HH:mm:ss"), $m) }

function Invoke-Bake {
    param([string]$Vector, [switch]$AllowPst)

    # --allow-pst is REQUIRED for the texel stage. bake.py refuses to rewrite
    # 2-D arrays without it, so omitting it silently bakes the 348 scalars and
    # DISCARDS all 768 PST changes -- which are the whole point of 5.9.12.
    $bakeArgs = @('tools\texel\bake.py', $Vector)
    if ($AllowPst) { $bakeArgs += '--allow-pst' }

    $out = & python @bakeArgs 2>&1
    # $ErrorActionPreference does not cover native commands; check explicitly.
    if ($LASTEXITCODE -ne 0) {
        $out | ForEach-Object { Write-Host "      $_" }
        throw "bake.py FAILED for $Vector (exit $LASTEXITCODE). Nothing was written."
    }
    $summary = ($out | Select-String -Pattern 'changed' | Select-Object -First 1)
    if (-not $summary) { throw "bake.py wrote nothing recognisable for $Vector" }
    Write-Host "      $summary"
}

function Rebuild-Tuner {
    # The tuner's starting values come from the COMPILED eval_params.h, so it
    # must be rebuilt after every bake or the next stage refits from stale
    # weights. Retry once: a just-exited tuner can still hold the .exe briefly.
    for ($i = 0; $i -lt 2; $i++) {
        cmake --build build/texel --config Release --target basilisk-texel -j 16 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) { return }
        Start-Sleep -Seconds 5
    }
    throw "basilisk-texel rebuild failed"
}

Say "5.9.12 full-surface fit"
Say "  corpus     : $Corpus"
Say "  iterations : $Iterations  (texel 1116 -> kingsafety 57, per round)"
Say "  baseline   : $(git rev-parse --short HEAD) -- includes 5.9.14's accepted king safety"
Write-Host ""

$log = @()
$started = Get-Date

for ($it = 1; $it -le $Iterations; $it++) {

    # ---- stage A: gradient over PSTs + scalars ------------------------------
    Say "iter $it/$Iterations  stage A: --tune texel (1,116 params, $Epochs epochs) ..."
    $vec = "$outDir\5912_texel_it$it.txt"
    $o = & $texel --tune texel $train $hold $vec --l2 1e-6 --epochs $Epochs 2>&1
    $o | Select-String -Pattern '^K =|^Initial holdout|^Restored best' |
        ForEach-Object { Write-Host "      $_" }
    $m = ($o | Select-String -Pattern 'holdout=([\d.]+)' | Select-Object -Last 1)
    $hl = if ($m) { $m.Matches.Groups[1].Value } else { "n/a" }
    $log += "iter $it texel      holdout $hl"

    Invoke-Bake -Vector $vec -AllowPst
    Rebuild-Tuner

    # ---- stage B: coordinate descent over the king-danger funnel ------------
    Say "iter $it/$Iterations  stage B: --tune-kingsafety (57 knobs) ... ~90 min"
    $ksv = "$outDir\5912_ks_it$it.txt"
    $ksArgs = @('--tune-kingsafety', $train, $hold, $ksv, '--step', '8')
    if ($Smoke) { $ksArgs += @('--epochs', '1', '--max-positions', '50000') }
    $o = & $texel @ksArgs 2>&1
    $o | Select-String -Pattern '^K =|^Initial train|^Restored best|^Final train' |
        ForEach-Object { Write-Host "      $_" }
    $mk = ($o | Select-String -Pattern 'holdout MSE = ([\d.]+)' | Select-Object -Last 1)
    $ksl = if ($mk) { $mk.Matches.Groups[1].Value } else { "n/a" }
    $log += "iter $it kingsafety holdout $ksl"

    # King safety is all scalars and 1-D tables, so no -AllowPst here.
    Invoke-Bake -Vector $ksv
    Rebuild-Tuner

    Write-Host ""
}

# ---- verification -----------------------------------------------------------
Say "Rebuilding engine and verifying ..."
cmake --build build/release -j 16 2>&1 | Out-Null

$bench = ("bench`nquit`n" | & .\build\release\basilisk.exe 2>$null |
    Select-String '^Nodes searched\s*:\s*(\d+)').Matches.Groups[1].Value
$ct = (& ctest --test-dir build/release 2>&1 | Select-String 'tests passed' | Select-Object -First 1)
$canary = (& .\build\release\test_eval.exe 2>&1 | Select-String 'pawn gate|mate-drive drives')

if ($bench -eq '12844350' -and -not $Smoke) {
    Write-Host ""
    Write-Host "WARNING: bench is UNCHANGED from the 5.9.14 head. The fit produced"
    Write-Host "no behavioural change, which for a 1,116-parameter refit means"
    Write-Host "something did not take. Do NOT gate this; investigate the bakes."
}

$hrs = [math]::Round(((Get-Date) - $started).TotalHours, 2)
Write-Host ""
Write-Host ("=" * 70)
Write-Host "  5.9.12 full-surface fit complete - $hrs hours"
Write-Host ("=" * 70)
$log | ForEach-Object { Write-Host "  $_" }
Write-Host ""
Write-Host "  bench  : $bench   (5.9.14 head was 12,844,350)"
Write-Host "  ctest  : $ct"
$canary | ForEach-Object { Write-Host "  canary : $($_.ToString().Trim())" }
Write-Host ""
Write-Host "  Mandatory checks still owed before any gate (PLAN 5.9.12):"
Write-Host "    1. score-scale audit -- compare the fitted K against baseline 1.94497;"
Write-Host "       a moved K silently rescales every centipawn search margin."
Write-Host "    2. repeat the BAS-E08 ablation -- are the 5.9.1/5.9.2 terms STILL"
Write-Host "       inert now that the 768 PSTs were free to move? If yes, remove them."
Write-Host ("=" * 70)

if ($Smoke) {
    Write-Host ""
    Say "SMOKE: restoring eval_params.h to HEAD and rebuilding"
    & git checkout HEAD -- src/eval_params.h
    Rebuild-Tuner
    cmake --build build/release -j 16 2>&1 | Out-Null
    Say "SMOKE complete -- pipeline works, results discarded."
}

Pop-Location
