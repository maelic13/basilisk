<#
.SYNOPSIS
    Run the PLAN 6.1.e held-out KBNK confirmation.

.DESCRIPTION
    6.1.c chose `15600,1750,0,340,0` on cohort positions 1-60 after three
    rounds and 42 arms, and that winner was not separated from its own
    plateau. This step re-runs the complete frozen 198-position cohort for the
    legacy control, the selected vector and the plateau probe
    `15600,1900,0,460,0`, then decides on positions 61-198 only.

    Conditions are identical to the 6.1.c screens: 60,000 nodes/move, 100-ply
    limit, 16 MB hash, 30 independent one-thread engine processes, engine
    tablebases disabled, natural termination and no score adjudication. The
    legacy control must reproduce both frozen fingerprints or the summarizer
    voids the run.
#>
param(
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$OutputDir = "$PSScriptRoot\..\results\kbnk-holdout-6.1.e",
    [int]$Nodes = 60000,
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
if ($Nodes -le 0 -or $MaxPlies -le 0 -or $Workers -le 0 -or $BuildJobs -le 0) {
    throw "Nodes, MaxPlies, Workers and BuildJobs must be positive"
}

$resolvedOutput = [IO.Path]::GetFullPath($OutputDir)
if (Test-Path -LiteralPath $resolvedOutput) {
    $existing = Get-ChildItem -LiteralPath $resolvedOutput -Force
    if ($existing.Count -ne 0) { throw "Refusing to overwrite non-empty result directory: $resolvedOutput" }
}
New-Item -ItemType Directory -Path $resolvedOutput -Force | Out-Null

& cmake -S $root -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release -DCOMP=clang -DTUNE=ON -DUSE_PEXT=ON
if ($LASTEXITCODE -ne 0) { throw "6.1.e configure failed" }
& cmake --build $buildDir --target basilisk test_eval --parallel $BuildJobs
if ($LASTEXITCODE -ne 0) { throw "6.1.e build failed" }
& (Join-Path $buildDir "test_eval.exe")
if ($LASTEXITCODE -ne 0) { throw "6.1.e evaluator test failed" }

# The control runs first: if it drifts, nothing later is worth reading.
$variants = @(
    @{ Name = "legacy-baseline"; Weights = "15600,800,900,220,220" },
    @{ Name = "d1750-k340";      Weights = "15600,1750,0,340,0" },
    @{ Name = "d1900-k460";      Weights = "15600,1900,0,460,0" }
)
$common = @(
    $runner,
    "--engine", $engine,
    "--syzygy", (Resolve-Path -LiteralPath $Syzygy).Path,
    "--cohort", $cohort,
    "--families", "KBN-K",
    "--cohort-limit-per-family", 0,
    "--nodes", $Nodes,
    "--max-plies", $MaxPlies,
    "--hash", 16,
    "--workers", $Workers,
    "--per-position"
)

foreach ($variant in $variants) {
    $output = Join-Path $resolvedOutput ($variant.Name + ".json")
    if ($ValidateOnly) {
        Write-Host "Validating $($variant.Name): KBNK Drive=$($variant.Weights)"
        & python @common --engine-option "KBNK Drive=$($variant.Weights)" --validate-only
    } else {
        Write-Host "Running $($variant.Name) over all 198 positions: KBNK Drive=$($variant.Weights)"
        & python @common --engine-option "KBNK Drive=$($variant.Weights)" --output $output
    }
    if ($LASTEXITCODE -ne 0) { throw "6.1.e variant failed: $($variant.Name)" }
}

if ($ValidateOnly) {
    Write-Host "6.1.e vectors validated; no searches were run."
    exit 0
}

& python $summarizer $resolvedOutput --output (Join-Path $resolvedOutput "summary.json")
if ($LASTEXITCODE -ne 0) { throw "6.1.e held-out summary failed" }
Write-Host "6.1.e held-out confirmation complete: $resolvedOutput"
