<#
.SYNOPSIS
    Run the final PLAN 6.1.c upper-range KBNK coefficient screen.

.DESCRIPTION
    The corrected sweep improved through its highest diagonal weight, but that
    boundary also exposed a live Syzygy truth loss. This registered follow-up
    explores the remaining safe diagonal/king region. It rejects any new truth
    discard before ply 80 and keeps the same 60 positions, fixed nodes, natural
    termination, and 30 independent one-thread engine processes.
#>
param(
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$OutputDir = "$PSScriptRoot\..\results\kbnk-upper-6.1.c",
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
$summarizer = Join-Path $PSScriptRoot "kbnk_upper_summary.py"
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
if ($LASTEXITCODE -ne 0) { throw "6.1.c upper sweep configure failed" }
& cmake --build $buildDir --target basilisk test_eval --parallel $BuildJobs
if ($LASTEXITCODE -ne 0) { throw "6.1.c upper sweep build failed" }
& (Join-Path $buildDir "test_eval.exe")
if ($LASTEXITCODE -ne 0) { throw "6.1.c upper sweep evaluator test failed" }

$variants = @(
    @{ Name = "legacy-baseline";  Weights = "15600,800,900,220,220" },
    @{ Name = "boundary-control"; Weights = "15600,1450,0,300,0" },
    @{ Name = "d1350-k340";       Weights = "15600,1350,0,340,0" },
    @{ Name = "d1350-k460";       Weights = "15600,1350,0,460,0" },
    @{ Name = "d1450-k340";       Weights = "15600,1450,0,340,0" },
    @{ Name = "d1450-k460";       Weights = "15600,1450,0,460,0" },
    @{ Name = "d1550-k340";       Weights = "15600,1550,0,340,0" },
    @{ Name = "d1550-k460";       Weights = "15600,1550,0,460,0" },
    @{ Name = "d1650-k460";       Weights = "15600,1650,0,460,0" },
    @{ Name = "d1750-k340";       Weights = "15600,1750,0,340,0" },
    @{ Name = "d1750-k460";       Weights = "15600,1750,0,460,0" },
    @{ Name = "d1850-k340";       Weights = "15600,1850,0,340,0" },
    @{ Name = "d1850-k460";       Weights = "15600,1850,0,460,0" },
    @{ Name = "d1900-k340";       Weights = "15600,1900,0,340,0" },
    @{ Name = "d1900-k460";       Weights = "15600,1900,0,460,0" }
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
    if ($LASTEXITCODE -ne 0) { throw "6.1.c upper variant failed: $($variant.Name)" }
}

if ($ValidateOnly) {
    Write-Host "6.1.c upper-range vectors validated; no searches were run."
    exit 0
}

& python $summarizer $resolvedOutput --output (Join-Path $resolvedOutput "summary.json")
if ($LASTEXITCODE -ne 0) { throw "6.1.c upper summary failed" }
Write-Host "6.1.c upper-range measurements complete: $resolvedOutput"
