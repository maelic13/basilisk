<#
.SYNOPSIS
    Run the calibrated 4-thread gate for the TT publication-coherence repair.

.DESCRIPTION
    The candidate binds each TT partial key to the payload observed by the
    reader, rejecting an incoherent key/payload pair before search consumes its
    score or bound. This changes search only under SMP races, so its registered
    playing gate is the deployed 4-thread configuration.

    Multi-thread fastchess runs are intentionally unpinned. The harness must
    therefore prove its placement bias at this thread count before the verdict:
    this wrapper first runs a 30,000-game byte-identical null calibration whose
    complete 95% nElo interval must lie inside +/-5, then runs the candidate
    against the frozen pre-repair binary with an SPRT at [-5,0]. Score-based
    adjudication is off for both runs. Any time forfeit invalidates either run.

    H1 accepts the implementation. H0 does not license restoring incoherent TT
    reads; it requires redesigning the publication mechanism and gating that
    replacement separately.

    REQUIRES PowerShell 7 because tools/sprt.ps1 is UTF-8 without a BOM.
#>
param(
    [int]$CalibrationGames = 30000,
    [int]$Threads = 4,
    [int]$Hash = 256,
    [string]$TC = "3+0.03",
    [switch]$WhatIfOnly
)

$ErrorActionPreference = "Stop"
if ($PSVersionTable.PSVersion.Major -lt 7) {
    throw "PowerShell 7 or newer is required; run this wrapper with pwsh."
}
if ($Threads -ne 4) {
    throw "This gate is registered at Threads=4; do not change -Threads."
}
if ($Hash -ne 256) {
    throw "This gate is registered at Hash=256 MB per engine; do not change -Hash."
}
if ($CalibrationGames -ne 30000) {
    throw "This gate is registered at 30,000 calibration games; do not change -CalibrationGames."
}

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$candidate = Join-Path $PSScriptRoot "test_engines\basilisk-tt-coherence-candidate-pext-pgo.exe"
$baseline = Join-Path $PSScriptRoot "test_engines\basilisk-tt-coherence-base-pext-pgo.exe"
$candidateManifest = $candidate -replace '\.exe$', '.manifest.txt'
$baselineManifest = $baseline -replace '\.exe$', '.manifest.txt'

foreach ($path in @($candidate, $baseline, $candidateManifest, $baselineManifest)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing gate artifact: $path" }
}

$head = (git -C $root rev-parse HEAD).Trim()
if ((git -C $root status --porcelain).Count -ne 0) {
    throw "The working tree is dirty; the frozen candidate must be run from its clean prepared commit."
}

function Read-BuildManifest([string]$path) {
    $contents = Get-Content -LiteralPath $path -Raw
    if ($contents -notmatch '(?m)^revision:\s+(\S+)') { throw "$path has no revision" }
    $revision = $Matches[1]
    if ($contents -notmatch '(?m)^dirty_diff:\s+clean\s*$') { throw "$path is not a clean build" }
    if ($contents -notmatch '(?m)^bench:\s+(\d+)') { throw "$path has no bench fingerprint" }
    [pscustomobject]@{ Revision = $revision; Bench = $Matches[1] }
}

$candidateBuild = Read-BuildManifest $candidateManifest
$baselineBuild = Read-BuildManifest $baselineManifest
$expectedBaseline = "9e52ff4f70159af79c20b686040f1f1f199b3f8f"
if ($candidateBuild.Revision -ne $head) {
    throw "Candidate revision $($candidateBuild.Revision) does not match HEAD $head."
}
if ($baselineBuild.Revision -ne $expectedBaseline) {
    throw "Baseline revision $($baselineBuild.Revision) is not the frozen pre-repair revision $expectedBaseline."
}
if ($candidateBuild.Bench -ne $baselineBuild.Bench) {
    throw "Candidate/base 1T benches differ ($($candidateBuild.Bench) vs $($baselineBuild.Bench))."
}

$sprt = Join-Path $PSScriptRoot "sprt.ps1"
$calibrationArgs = @(
    "-EngineA", $baseline,
    "-EngineB", $baseline,
    "-NameA", "tt-4t-null-a",
    "-NameB", "tt-4t-null-b",
    "-Mode", "calibrate",
    "-Games", $CalibrationGames,
    "-CalibrationTolerance", 5,
    "-Threads", $Threads,
    "-Hash", $Hash,
    "-TC", $TC
)
$gateArgs = @(
    "-EngineA", $candidate,
    "-EngineB", $baseline,
    "-NameA", "tt-coherent",
    "-NameB", "tt-plain-key-base",
    "-Mode", "simplify",
    "-Threads", $Threads,
    "-Hash", $Hash,
    "-TC", $TC
)

Write-Host "Candidate revision: $head"
Write-Host "Exact 1T bench:     $($candidateBuild.Bench) (candidate = baseline)"
Write-Host "Registered design:  $CalibrationGames-game 4T null calibration, then 4T SPRT [-5,0]"
Write-Host "Adjudication:       OFF"

if ($WhatIfOnly) {
    Write-Host ""
    Write-Host "Validated. Would run these sequentially:"
    Write-Host ("  {0} {1}" -f $sprt, ($calibrationArgs -join " "))
    Write-Host ("  {0} {1}" -f $sprt, ($gateArgs -join " "))
    exit 0
}

Write-Host ""
Write-Host "Phase 1/2: calibrating the unpinned 4-thread harness"
& $sprt @calibrationArgs
if ($LASTEXITCODE -ne 0) { throw "4T null calibration failed with exit code $LASTEXITCODE" }

Write-Host ""
Write-Host "Phase 2/2: TT coherence non-regression SPRT [-5,0]"
& $sprt @gateArgs
if ($LASTEXITCODE -ne 0) { throw "TT coherence SPRT failed with exit code $LASTEXITCODE" }

Write-Host "TT coherence gate finished; logs, PGNs, and manifests are in tools\results\."
