<#
.SYNOPSIS
    Run the PLAN 6.4.a probe: does rule-50 damping erode the mate drive's
    gradient below the pruning margin?

.DESCRIPTION
    `damp_rule50` multiplies the evaluation by (199 - clock) / 199 AFTER
    apply_endgame, so it scales the mate-drive override band too. The weights
    were chosen to clear the 243 razoring margin; after damping the kxk king
    step is 224 and its corner step 187 by clock 50, while the kbnk diagonal
    step is still 1422. See analysis/endgame_magnitude_audit_v1.md.

    Three arms differing ONLY in the starting fifty-move counter: 0, 25, 50.
    Clock 50 is chosen deliberately: it is where the kxk king step first falls
    below the margin (224) and where KBN-K still has 14 of 24 clean wins inside
    the remaining 50-halfmove budget. A tighter clock would leave KBN-K with one
    eligible position and no control to compare against.
    Same binary, same frozen cohort, same node budget. Syzygy ignores the
    halfmove clock, so every theory label stays valid; what changes is how much
    damping the engine's own evaluation carries.

    The discriminator is the family contrast, not the absolute numbers. KQ-K,
    KR-K and KBB-K run through kxk_score, whose steps cross the margin by clock
    50. KBN-K runs through kbnk_score, whose diagonal never does. If damping is
    the cause, the first three degrade materially more than the fourth on the
    same eligible positions. If the loss is only the shorter horizon, both
    degrade alike and nothing needs changing.

    REQUIRES pwsh 7 and an idle machine. About 350 short games; a few minutes.
#>
param(
    [string]$Syzygy = "D:\chess\tablebases\syzygy3456",
    [string]$OutputDir = "$PSScriptRoot\..\results\damping-6.4.a",
    [int[]]$Clocks = @(0, 25, 50),
    [int]$Nodes = 60000,
    [int]$MaxPlies = 100,
    [int]$Workers = 30,
    [int]$BuildJobs = 8,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
$cohort = Join-Path $PSScriptRoot "endgame_cohort_v1.manifest.json"
$runner = Join-Path $PSScriptRoot "endgame_truth.py"
$summarizer = Join-Path $PSScriptRoot "damping_resolution_summary.py"
$buildDir = Join-Path $root "build\tune"
$engine = Join-Path $buildDir "basilisk.exe"

foreach ($path in @($cohort, $runner, $summarizer)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
if (-not (Test-Path -LiteralPath $Syzygy -PathType Container)) { throw "Syzygy directory not found: $Syzygy" }
foreach ($c in $Clocks) {
    if ($c -lt 0 -or $c -gt 99) { throw "clock $c is outside 0..99" }
}

& cmake -S $root -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release -DCOMP=clang -DTUNE=ON -DUSE_PEXT=ON
if ($LASTEXITCODE -ne 0) { throw "6.4.a configure failed" }
& cmake --build $buildDir --target basilisk test_eval --parallel $BuildJobs
if ($LASTEXITCODE -ne 0) { throw "6.4.a build failed" }
& (Join-Path $buildDir "test_eval.exe")
if ($LASTEXITCODE -ne 0) { throw "6.4.a evaluator test failed" }

$resolved = [IO.Path]::GetFullPath($OutputDir)
if (Test-Path -LiteralPath $resolved) {
    if ((Get-ChildItem -LiteralPath $resolved -Force).Count -ne 0) {
        throw "Refusing to overwrite non-empty result directory: $resolved"
    }
}
New-Item -ItemType Directory -Path $resolved -Force | Out-Null

# Bare-king mate families only. Their conversion depends on the drive gradient
# rather than on material, which is what this probe is about.
$families = "KQ-K,KR-K,KBB-K,KBN-K"

foreach ($clock in $Clocks) {
    $common = @(
        $runner,
        "--engine", $engine,
        "--syzygy", (Resolve-Path -LiteralPath $Syzygy).Path,
        "--cohort", $cohort,
        "--families", $families,
        "--nodes", $Nodes,
        "--max-plies", $MaxPlies,
        "--hash", 16,
        "--workers", $Workers,
        "--start-halfmove-clock", $clock,
        "--per-position"
    )
    if ($ValidateOnly) {
        Write-Host "Validating starting clock $clock"
        & python @common --validate-only
    } else {
        Write-Host "Running starting clock $clock over $families"
        & python @common --output (Join-Path $resolved "clock$clock.json")
    }
    if ($LASTEXITCODE -ne 0) { throw "6.4.a arm failed at clock $clock" }
}

if ($ValidateOnly) {
    Write-Host "6.4.a arms validated; no searches were run."
    exit 0
}

& python $summarizer $resolved --clocks @Clocks --output (Join-Path $resolved "summary.json")
if ($LASTEXITCODE -ne 0) { throw "6.4.a summary failed" }
Write-Host ""
Write-Host "6.4.a damping probe complete: $resolved"
