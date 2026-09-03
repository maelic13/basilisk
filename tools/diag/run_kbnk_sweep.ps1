<#
.SYNOPSIS
    Build the tune-only engine and run PLAN 6.1.c with 30 one-thread workers.

.DESCRIPTION
    Screens registered KBNK drive vectors on the first 60 entries of the exact
    historical 198-position cohort. Limits are fixed nodes, no engine Syzygy
    probing is allowed, and games have no score adjudication. Variants run
    sequentially; positions within each variant run in parallel.
#>
param(
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$OutputDir = "$PSScriptRoot\..\results\kbnk-sweep-6.1.c",
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
$summarizer = Join-Path $PSScriptRoot "kbnk_sweep_summary.py"
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
if ($LASTEXITCODE -ne 0) { throw "6.1.c tune configure failed" }
& cmake --build $buildDir --target basilisk test_eval --parallel $BuildJobs
if ($LASTEXITCODE -ne 0) { throw "6.1.c tune build failed" }
& (Join-Path $buildDir "test_eval.exe")
if ($LASTEXITCODE -ne 0) { throw "6.1.c focused evaluator test failed" }

$variants = @(
    @{ Name = "baseline";          Weights = "800,900,220,220" },
    @{ Name = "diagonal-600";      Weights = "600,900,220,220" },
    @{ Name = "diagonal-1000";     Weights = "1000,900,220,220" },
    @{ Name = "no-edge";           Weights = "800,0,220,220" },
    @{ Name = "no-king";           Weights = "800,900,0,220" },
    @{ Name = "no-knight";         Weights = "800,900,220,0" },
    @{ Name = "no-edge-knight";    Weights = "800,0,220,0" },
    @{ Name = "dominant-diagonal"; Weights = "1000,0,220,0" },
    @{ Name = "rarog-shape";       Weights = "1000,0,100,0" },
    @{ Name = "diagonal-only";     Weights = "1000,0,0,0" }
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
    if ($LASTEXITCODE -ne 0) { throw "6.1.c variant failed: $($variant.Name)" }
}

if ($ValidateOnly) {
    Write-Host "6.1.c build, cohort and all registered coefficient vectors validated; no searches were run."
    exit 0
}

& python $summarizer $resolvedOutput --output (Join-Path $resolvedOutput "summary.json")
if ($LASTEXITCODE -ne 0) { throw "6.1.c summary failed" }
Write-Host "6.1.c measurements complete: $resolvedOutput"
