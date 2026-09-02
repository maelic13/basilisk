<#
.SYNOPSIS
    Run PLAN 6.0.b on the frozen cohort with identical engine limits.

.DESCRIPTION
    Measures the accepted Basilisk binary and a strong reference sequentially
    using independent position workers. Every worker runs a one-thread engine
    at the same nodes per move, with engine tablebase probing disabled. The
    diagnostic itself uses Syzygy only to verify and grade play. Games
    terminate by chess rules or the registered diagnostic ply limit; there is
    no score adjudication.
#>
param(
    [Parameter(Mandatory)][string]$Basilisk,
    [string]$Reference = "D:\chess\engines\stockfish.exe",
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$Cohort = "$PSScriptRoot\endgame_cohort_v1.manifest.json",
    [string]$OutputDir = "$PSScriptRoot\..\results\endgame-truth-6.0.b",
    [int]$Nodes = 60000,
    [int]$MaxPlies = 100,
    [int]$Hash = 16,
    [int]$Workers = 30,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"
$runner = Join-Path $PSScriptRoot "endgame_truth.py"

foreach ($path in @($Basilisk, $Reference, $Cohort, $runner)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required file not found: $path"
    }
}
if (-not (Test-Path -LiteralPath $Syzygy -PathType Container)) {
    throw "Syzygy directory not found: $Syzygy"
}
if ($Nodes -le 0 -or $MaxPlies -le 0 -or $Hash -le 0 -or $Workers -le 0) {
    throw "Nodes, MaxPlies, Hash and Workers must be positive"
}

$common = @(
    $runner,
    "--syzygy", (Resolve-Path -LiteralPath $Syzygy).Path,
    "--cohort", (Resolve-Path -LiteralPath $Cohort).Path,
    "--nodes", $Nodes,
    "--max-plies", $MaxPlies,
    "--hash", $Hash,
    "--workers", $Workers
)

if ($ValidateOnly) {
    foreach ($engine in @($Basilisk, $Reference)) {
        & python @common --engine (Resolve-Path -LiteralPath $engine).Path --validate-only
        if ($LASTEXITCODE -ne 0) { throw "Validation failed for $engine" }
    }
    Write-Host "6.0.b configuration validated; no searches were run."
    exit 0
}

$resolvedOutput = [IO.Path]::GetFullPath($OutputDir)
$basiliskOutput = Join-Path $resolvedOutput "basilisk.json"
$referenceOutput = Join-Path $resolvedOutput "reference.json"
if ((Test-Path -LiteralPath $basiliskOutput) -or
    (Test-Path -LiteralPath $referenceOutput)) {
    throw "Refusing to overwrite an existing 6.0.b result in $resolvedOutput"
}
New-Item -ItemType Directory -Path $resolvedOutput -Force | Out-Null

Write-Host "Running accepted Basilisk on the frozen cohort..."
& python @common --engine (Resolve-Path -LiteralPath $Basilisk).Path --output $basiliskOutput
if ($LASTEXITCODE -ne 0) { throw "Basilisk measurement failed" }

Write-Host "Running strong reference on the same frozen cohort..."
& python @common --engine (Resolve-Path -LiteralPath $Reference).Path --output $referenceOutput
if ($LASTEXITCODE -ne 0) { throw "Reference measurement failed" }

Write-Host "6.0.b measurements complete: $resolvedOutput"
