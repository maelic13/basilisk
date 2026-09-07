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
    The required binaries are gitignored, so this wrapper first checks out the
    two frozen revisions in temporary detached worktrees and builds a fresh
    PEXT+PGO binary from each. Their artifacts are copied to a newly created,
    uniquely named directory; an existing executable is never overwritten.

    It then runs a 30,000-game byte-identical null calibration whose complete
    95% nElo interval must lie inside +/-5, followed by the candidate against
    the frozen pre-repair binary with an SPRT at [-5,0]. Score-based
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
    [switch]$PrepareOnly,
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
$clang = Get-Command clang -CommandType Application -ErrorAction SilentlyContinue
if (-not $clang) {
    $clangBin = @("D:\msys64\clang64\bin", "C:\msys64\clang64\bin") |
        Where-Object { Test-Path -LiteralPath (Join-Path $_ "clang.exe") } |
        Select-Object -First 1
    if ($clangBin) {
        $env:PATH = "$clangBin;$env:PATH"
    }
}
foreach ($tool in @("git", "cmake", "ninja", "clang", "llvm-profdata")) {
    if (-not (Get-Command $tool -CommandType Application -ErrorAction SilentlyContinue)) {
        throw "Required build tool '$tool' is not on PATH. Install/configure the MSYS2 clang64 toolchain first."
    }
}

$candidateRevision = "355aec1b4918427116eff9bcc4863992acb72b3f"
$baselineRevision = "9e52ff4f70159af79c20b686040f1f1f199b3f8f"
foreach ($revision in @($candidateRevision, $baselineRevision)) {
    & git -C $root cat-file -e "$revision^{commit}"
    if ($LASTEXITCODE -ne 0) {
        throw "Required revision $revision is unavailable. Fetch/pull the prepared commits first."
    }
}

if ($WhatIfOnly) {
    Write-Host "Validated frozen revisions. The full run will:"
    Write-Host "  1. create two isolated temporary worktrees"
    Write-Host "  2. build fresh PEXT+PGO baseline and candidate binaries"
    Write-Host "  3. preserve them in a new unique tools\test_engines subdirectory"
    Write-Host "  4. run the 30,000-game 4T null calibration"
    Write-Host "  5. run the 4T SPRT [-5,0] if calibration passes"
    exit 0
}

$artifactParent = Join-Path $PSScriptRoot "test_engines"
New-Item -ItemType Directory -Path $artifactParent -Force | Out-Null
do {
    $runId = "tt-coherence-gate-{0}-{1}" -f (Get-Date -Format "yyyyMMdd-HHmmss"),
        ([guid]::NewGuid().ToString("N").Substring(0, 8))
    $artifactDir = Join-Path $artifactParent $runId
} while (Test-Path -LiteralPath $artifactDir)
New-Item -ItemType Directory -Path $artifactDir | Out-Null

$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ("basilisk-tt-gate-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
$temporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
$createdWorktrees = [Collections.Generic.List[string]]::new()

function Build-FrozenRevision([string]$label, [string]$revision) {
    $worktree = [IO.Path]::GetFullPath((Join-Path $temporaryRoot $label))
    if (-not $worktree.StartsWith($temporaryRoot + [IO.Path]::DirectorySeparatorChar,
                                  [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing unsafe temporary worktree path: $worktree"
    }

    Write-Host ""
    Write-Host "Creating detached $label worktree at revision $revision"
    & git -C $root worktree add --detach $worktree $revision
    if ($LASTEXITCODE -ne 0) { throw "git worktree add failed for $label" }
    $createdWorktrees.Add($worktree)

    $suffix = "tt-coherence-$label-$($revision.Substring(0, 10))"
    & (Join-Path $worktree "tools\build_test.ps1") `
        -Suffix $suffix -TestEnginesDir $artifactDir
    if ($LASTEXITCODE -ne 0) { throw "PEXT+PGO build failed for $label" }

    $exe = Join-Path $artifactDir "basilisk-$suffix-pext-pgo.exe"
    $manifest = $exe -replace '\.exe$', '.manifest.txt'
    if (-not (Test-Path -LiteralPath $exe) -or
        -not (Test-Path -LiteralPath $manifest)) {
        throw "Build for $label did not produce its executable and manifest."
    }
    [pscustomobject]@{ Exe = $exe; Manifest = $manifest }
}

try {
    $baselineArtifact = Build-FrozenRevision "base" $baselineRevision
    $candidateArtifact = Build-FrozenRevision "candidate" $candidateRevision
} finally {
    for ($index = $createdWorktrees.Count - 1; $index -ge 0; --$index) {
        $worktree = [IO.Path]::GetFullPath($createdWorktrees[$index])
        if ($worktree.StartsWith($temporaryRoot + [IO.Path]::DirectorySeparatorChar,
                                 [StringComparison]::OrdinalIgnoreCase)) {
            & git -C $root worktree remove --force $worktree
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Could not remove temporary worktree $worktree; remove it manually."
            }
        }
    }
    if (Test-Path -LiteralPath $temporaryRoot) {
        try { Remove-Item -LiteralPath $temporaryRoot -ErrorAction Stop }
        catch { Write-Warning "Temporary root is not empty: $temporaryRoot" }
    }
}

$candidate = $candidateArtifact.Exe
$baseline = $baselineArtifact.Exe
$candidateManifest = $candidateArtifact.Manifest
$baselineManifest = $baselineArtifact.Manifest

function Read-BuildManifest([string]$path) {
    $contents = Get-Content -LiteralPath $path -Raw
    if ($contents -notmatch '(?m)^revision:\s+(\S+)') { throw "$path has no revision" }
    $revision = $Matches[1]
    if ($contents -notmatch '(?m)^dirty_diff:\s+clean\s*$') { throw "$path is not a clean build" }
    if ($contents -notmatch '(?m)^bench:\s+(\d+)') { throw "$path has no bench fingerprint" }
    $bench = $Matches[1]
    if ($contents -notmatch '(?m)^binary_sha256:\s+([0-9A-Fa-f]{64})') {
        throw "$path has no binary SHA-256"
    }
    $recordedHash = $Matches[1]
    $exe = $path -replace '\.manifest\.txt$', '.exe'
    $actualHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    if ($actualHash -ne $recordedHash) { throw "$exe does not match its manifest SHA-256" }
    [pscustomobject]@{ Revision = $revision; Bench = $bench }
}

$candidateBuild = Read-BuildManifest $candidateManifest
$baselineBuild = Read-BuildManifest $baselineManifest
if ($candidateBuild.Revision -ne $candidateRevision) {
    throw "Candidate revision $($candidateBuild.Revision) is not frozen revision $candidateRevision."
}
if ($baselineBuild.Revision -ne $baselineRevision) {
    throw "Baseline revision $($baselineBuild.Revision) is not frozen revision $baselineRevision."
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

Write-Host "Artifact directory:  $artifactDir"
Write-Host "Candidate revision:  $candidateRevision"
Write-Host "Baseline revision:   $baselineRevision"
Write-Host "Exact 1T bench:     $($candidateBuild.Bench) (candidate = baseline)"
Write-Host "Registered design:  $CalibrationGames-game 4T null calibration, then 4T SPRT [-5,0]"
Write-Host "Adjudication:       OFF"

if ($PrepareOnly) {
    Write-Host ""
    Write-Host "Preparation complete. The following commands were not run:"
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
