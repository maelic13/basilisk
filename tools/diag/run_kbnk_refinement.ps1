<#
.SYNOPSIS
    Run the confound-corrected PLAN 6.1.c KBNK coefficient refinement.

.DESCRIPTION
    Separates the total KBNK base from the diagonal slope, then screens a
    registered diagonal/king grid and an independent base axis. All variants
    use the same first 60 frozen wins, fixed nodes, natural termination, and
    30 independent one-thread engine processes.
#>
param(
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$OutputDir = "$PSScriptRoot\..\results\kbnk-refinement-6.1.c",
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
$summarizer = Join-Path $PSScriptRoot "kbnk_refinement_summary.py"
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
if ($LASTEXITCODE -ne 0) { throw "6.1.c refinement configure failed" }
& cmake --build $buildDir --target basilisk test_eval --parallel $BuildJobs
if ($LASTEXITCODE -ne 0) { throw "6.1.c refinement build failed" }
& (Join-Path $buildDir "test_eval.exe")
if ($LASTEXITCODE -ne 0) { throw "6.1.c refinement evaluator test failed" }

# Explicit five-field form: total-base, diagonal, edge, strong-king, knight.
# The first two controls exactly reproduce the original accepted vector and
# the provisional first-pass winner despite the new independent coordinates.
$variants = @(
    @{ Name = "legacy-baseline"; Weights = "15600,800,900,220,220" },
    @{ Name = "pass1-winner";    Weights = "17000,1000,0,220,0" },
    @{ Name = "d0900-k220";      Weights = "15600,900,0,220,0" },
    @{ Name = "d1000-k140";      Weights = "15600,1000,0,140,0" },
    @{ Name = "d1000-k220";      Weights = "15600,1000,0,220,0" },
    @{ Name = "d1000-k300";      Weights = "15600,1000,0,300,0" },
    @{ Name = "d1250-k140";      Weights = "15600,1250,0,140,0" },
    @{ Name = "d1250-k220";      Weights = "15600,1250,0,220,0" },
    @{ Name = "d1250-k300";      Weights = "15600,1250,0,300,0" },
    @{ Name = "d1450-k140";      Weights = "15600,1450,0,140,0" },
    @{ Name = "d1450-k220";      Weights = "15600,1450,0,220,0" },
    @{ Name = "d1450-k300";      Weights = "15600,1450,0,300,0" },
    @{ Name = "base14200";       Weights = "14200,1250,0,220,0" },
    @{ Name = "base17000";       Weights = "17000,1250,0,220,0" },
    @{ Name = "base18400";       Weights = "18400,1250,0,220,0" }
)
$common = @(
    $runner,
    "--engine", $engine,
    "--syzygy", (Resolve-Path -LiteralPath $Syzygy).Path,
    "--cohort", $cohort,
    "--families", "KBN-K",
    "--cohort-limit-per-family", 60,
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
        Write-Host "Running $($variant.Name): KBNK Drive=$($variant.Weights)"
        & python @common --engine-option "KBNK Drive=$($variant.Weights)" --output $output
    }
    if ($LASTEXITCODE -ne 0) { throw "6.1.c refinement variant failed: $($variant.Name)" }
}

if ($ValidateOnly) {
    Write-Host "6.1.c corrected parameterization, cohort and all registered vectors validated; no searches were run."
    exit 0
}

& python $summarizer $resolvedOutput --output (Join-Path $resolvedOutput "summary.json")
if ($LASTEXITCODE -ne 0) { throw "6.1.c refinement summary failed" }
Write-Host "6.1.c refinement measurements complete: $resolvedOutput"
