<#
.SYNOPSIS
    Re-measure the PLAN 6.0.b endgame baseline after the material-abort fix.

.DESCRIPTION
    6.0.b was scored with a termination rule that ended a game the moment the
    strong side's piece count dropped. That is sound for bare-king families and
    false for pawn technique, where giving a pawn to promote another is the
    winning method. In the original artifacts 178 of the Basilisk arm's 193
    `material_lost` aborts, and 173 of the reference arm's 174, happened
    without the engine having played a single non-win-preserving move.

    Both arms are therefore contaminated and both must be re-run; comparing a
    fixed Basilisk arm against the old reference arm would be worse than
    leaving the number alone.

    Conditions are exactly those recorded for 6.0.b -- 60,000 nodes/move, one
    engine thread, 16 MB hash, a 100-ply diagnostic limit, engine tablebases
    disabled, no score adjudication, the frozen 770-position cohort, and both
    original binaries verified by SHA-256. Only the termination rule differs,
    which is the point.

    Workers are independent one-thread engine processes: 30 of 32 logical CPUs.
    Worker count changes wall time, never the deterministic per-position result.

    REQUIRES pwsh 7 and an idle machine.
#>
param(
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$Reference = "D:\chess\engines\stockfish.exe",
    [string]$OutputDir = "$PSScriptRoot\..\results\endgame-truth-6.0.b-refixed",
    [int]$Nodes = 60000,
    [int]$MaxPlies = 100,
    [int]$Workers = 30,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"
$cohort = Join-Path $PSScriptRoot "endgame_cohort_v1.manifest.json"
$runner = Join-Path $PSScriptRoot "endgame_truth.py"
$basilisk = Join-Path $PSScriptRoot "..\test_engines\basilisk-6.0.b-accepted-head-pext-pgo.exe"

# The original artifacts name both binaries by hash. Re-measuring with anything
# else would not be a re-measurement.
$expected = @{
    $basilisk   = "D0E558F8A113CD9D17905B6EF701040CF5B0FB450FC058EFB9C49DF2329F8430"
    $Reference  = "91AE61DFCAEF1A5FDFEE9722EDE0591DA1FCB124D1DE7FBD44DD8786BD6531E3"
}
foreach ($path in @($cohort, $runner)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
foreach ($exe in $expected.Keys) {
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Engine not found: $exe" }
    $actual = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    if ($actual -ne $expected[$exe]) {
        throw "$exe : SHA-256 $actual does not match the 6.0.b record $($expected[$exe])"
    }
    Write-Host ("verified {0}" -f (Split-Path $exe -Leaf))
}
if (-not (Test-Path -LiteralPath $Syzygy -PathType Container)) { throw "Syzygy directory not found: $Syzygy" }

$resolved = [IO.Path]::GetFullPath($OutputDir)
if (Test-Path -LiteralPath $resolved) {
    if ((Get-ChildItem -LiteralPath $resolved -Force).Count -ne 0) {
        throw "Refusing to overwrite non-empty result directory: $resolved"
    }
}
New-Item -ItemType Directory -Path $resolved -Force | Out-Null

$arms = @(
    @{ Name = "basilisk";  Engine = $basilisk },
    @{ Name = "reference"; Engine = $Reference }
)
foreach ($arm in $arms) {
    $common = @(
        $runner,
        "--engine", $arm.Engine,
        "--syzygy", (Resolve-Path -LiteralPath $Syzygy).Path,
        "--cohort", $cohort,
        "--nodes", $Nodes,
        "--max-plies", $MaxPlies,
        "--hash", 16,
        "--workers", $Workers,
        "--per-position"
    )
    if ($ValidateOnly) {
        Write-Host "Validating $($arm.Name)"
        & python @common --validate-only
    } else {
        Write-Host "Running $($arm.Name) over all 770 frozen positions at $Nodes nodes"
        & python @common --output (Join-Path $resolved ($arm.Name + ".json"))
    }
    if ($LASTEXITCODE -ne 0) { throw "6.0.b re-measure failed on arm: $($arm.Name)" }
}

if ($ValidateOnly) {
    Write-Host "6.0.b re-measure validated; no searches were run."
    exit 0
}
Write-Host ""
Write-Host "Original (contaminated) artifacts remain at tools\results\endgame-truth-6.0.b\"
Write-Host "for comparison. They are superseded, not deleted."
