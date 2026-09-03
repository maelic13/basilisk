<#
.SYNOPSIS
    Does the 6.1.e ranking survive a game-representative node budget?

.DESCRIPTION
    Every coefficient in 6.1 was selected at 60,000 nodes per move. A 3+0.03
    game spends roughly 250,000-350,000 nodes per move at this machine's ~3.6M
    nps, and far more once the board simplifies, so the whole of 6.1 was tuned
    at a budget the engine essentially never plays at.

    That matters concretely: the failure that decided 6.1.e -- 2.Nc2 allowing
    2...Kd1 to fork bishop and knight on KBNK0061 -- is a two-ply tactic that a
    real game budget very likely sees. If so, the rejection of
    `15600,1750,0,340,0` is sound at 60k and unproven at game depth.

    Rather than guess the single "right" budget, this brackets it. The 6.1.e
    result at 60,000 already exists; this adds 200,000 and 600,000, a 10x span
    around the game condition. If the ranking is stable across that span the
    concern is closed; if it flips, we learn exactly where and why.

    Frozen 60,000-node control fingerprints cannot apply at a different budget,
    so the summarizer is called with --allow-control-drift. Every other
    condition -- cohort, positions, plies, hash, workers, tablebases off, no
    adjudication -- is identical to 6.1.e.

    REQUIRES pwsh 7 and a free machine: three arms per budget at 198 positions.
    Expect roughly 4 minutes per arm at 200k and 12 at 600k, so about 50
    minutes total. Do not start this while an SPRT is running.
#>
param(
    [int[]]$NodeBudgets = @(200000, 600000),
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [int]$MaxPlies = 100,
    [int]$Workers = 30,
    [int]$BuildJobs = 8,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
$cohort = Join-Path $PSScriptRoot "kbnk_cohort_v1.manifest.json"
$runner = Join-Path $PSScriptRoot "endgame_truth.py"
$summarizer = Join-Path $PSScriptRoot "kbnk_holdout_summary.py"
$buildDir = Join-Path $root "build\tune"
$engine = Join-Path $buildDir "basilisk.exe"

foreach ($path in @($cohort, $runner, $summarizer)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
if (-not (Test-Path -LiteralPath $Syzygy -PathType Container)) { throw "Syzygy directory not found: $Syzygy" }
foreach ($n in $NodeBudgets) { if ($n -le 0) { throw "node budgets must be positive" } }

& cmake -S $root -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release -DCOMP=clang -DTUNE=ON -DUSE_PEXT=ON
if ($LASTEXITCODE -ne 0) { throw "budget-transfer configure failed" }
& cmake --build $buildDir --target basilisk test_eval --parallel $BuildJobs
if ($LASTEXITCODE -ne 0) { throw "budget-transfer build failed" }
& (Join-Path $buildDir "test_eval.exe")
if ($LASTEXITCODE -ne 0) { throw "budget-transfer evaluator test failed" }

# Same three arms as 6.1.e so the two runs are directly comparable.
$variants = @(
    @{ Name = "legacy-baseline"; Weights = "15600,800,900,220,220" },
    @{ Name = "d1750-k340";      Weights = "15600,1750,0,340,0" },
    @{ Name = "d1900-k460";      Weights = "15600,1900,0,460,0" }
)

foreach ($nodes in $NodeBudgets) {
    $outputDir = Join-Path $PSScriptRoot ("..\results\kbnk-budget-{0}k" -f [int]($nodes / 1000))
    $resolved = [IO.Path]::GetFullPath($outputDir)
    if (Test-Path -LiteralPath $resolved) {
        if ((Get-ChildItem -LiteralPath $resolved -Force).Count -ne 0) {
            throw "Refusing to overwrite non-empty result directory: $resolved"
        }
    }
    New-Item -ItemType Directory -Path $resolved -Force | Out-Null

    $common = @(
        $runner,
        "--engine", $engine,
        "--syzygy", (Resolve-Path -LiteralPath $Syzygy).Path,
        "--cohort", $cohort,
        "--families", "KBN-K",
        "--cohort-limit-per-family", 0,
        "--nodes", $nodes,
        "--max-plies", $MaxPlies,
        "--hash", 16,
        "--workers", $Workers,
        "--per-position"
    )

    foreach ($variant in $variants) {
        $output = Join-Path $resolved ($variant.Name + ".json")
        if ($ValidateOnly) {
            Write-Host "Validating $($variant.Name) at $nodes nodes"
            & python @common --engine-option "KBNK Drive=$($variant.Weights)" --validate-only
        } else {
            Write-Host "Running $($variant.Name) at $nodes nodes/move over all 198 positions"
            & python @common --engine-option "KBNK Drive=$($variant.Weights)" --output $output
        }
        if ($LASTEXITCODE -ne 0) { throw "budget-transfer variant failed: $($variant.Name) at $nodes" }
    }

    if (-not $ValidateOnly) {
        Write-Host ""
        Write-Host "=== $nodes nodes/move ==="
        & python $summarizer $resolved --allow-control-drift `
            --output (Join-Path $resolved "summary.json")
        if ($LASTEXITCODE -ne 0) { throw "budget-transfer summary failed at $nodes" }
    }
}

if ($ValidateOnly) {
    Write-Host "Budget-transfer vectors validated; no searches were run."
    exit 0
}
Write-Host ""
Write-Host "Compare each budget's head-to-head line and verdicts against the"
Write-Host "60,000-node result in tools\results\kbnk-holdout-6.1.e\summary.json."
