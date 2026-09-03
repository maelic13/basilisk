<#
.SYNOPSIS
    Run the PLAN 6.2.a Group A gate: a fresh no-adjudication SPRT that isolates
    the 6.1 KBNK change.

.DESCRIPTION
    Single-variable A/B. The baseline is commit 6accfe6 (Complete 6.1.b), the
    last revision carrying the pre-6.1 KBNK scoring; 6.1.b was proven score-
    and bench-identical, so everything outside kbnk_score matches the candidate
    exactly. That deliberately differs from the superseded Group A run's
    5.9.18-base, which 6.2.b already declares preliminary.

    Adjudication is OFF, as the project default requires.

    BAS-E43 measured KBNK occurrence at zero across 30.5M nodes from 86
    non-endgame roots, so 6.2 is a NON-REGRESSION gate. The registered [0,3]
    gainer test is the default here because changing registered bounds is a
    plan-structure decision for the maintainer; -NonRegression switches to the
    [-5,0] simplify test that the revised purpose actually calls for.

    REQUIRES pwsh 7. tools/sprt.ps1 is UTF-8 without a BOM and contains
    non-ASCII em-dashes that Windows PowerShell 5.1 mis-decodes into
    unterminated string literals, so it does not parse under 5.1.
#>
param(
    [switch]$NonRegression,
    [int]$Hash = 64,
    [int]$Threads = 1,
    [string]$TC = "3+0.03",
    [switch]$WhatIfOnly
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path

# Frozen provenance for this gate. A binary built from a different revision, or
# from a dirty tree, is not the artifact this step registered.
$expected = @{
    "6.2a-cand" = "af6420c7d3ddf073f30fdf2e71d43b95e456413d"
    "6.2a-base" = "6accfe6c9885977475d8dab48e219e8d4eb840e7"
}
$engines = @{}
foreach ($suffix in $expected.Keys) {
    $exe = Join-Path $PSScriptRoot "test_engines\basilisk-$suffix-pext-pgo.exe"
    $manifest = Join-Path $PSScriptRoot "test_engines\basilisk-$suffix-pext-pgo.manifest.txt"
    if (-not (Test-Path -LiteralPath $exe))      { throw "missing engine: $exe" }
    if (-not (Test-Path -LiteralPath $manifest)) { throw "missing manifest: $manifest" }
    $text = Get-Content -LiteralPath $manifest -Raw
    if ($text -notmatch '(?m)^revision:\s+(\S+)')   { throw "$suffix : manifest has no revision" }
    $revision = $Matches[1]
    if ($revision -ne $expected[$suffix]) {
        throw "$suffix : built from $revision, expected $($expected[$suffix])"
    }
    if ($text -notmatch '(?m)^dirty_diff:\s+clean') {
        throw "$suffix : built from a dirty tree; rebuild from a clean checkout"
    }
    if ($text -match '(?m)^bench:\s+(\d+)') { $bench = $Matches[1] } else { $bench = "?" }
    Write-Host ("{0,-10} revision {1}  bench {2}" -f $suffix, $revision.Substring(0, 10), $bench)
    $engines[$suffix] = $exe
}

# The two binaries differ only in kbnk_score's coefficients, which the bench
# suite never reaches, so identical bench is expected here and is NOT evidence
# of behavioural identity -- it is the provenance check that nothing else moved.
$benches = foreach ($suffix in $expected.Keys) {
    (Get-Content -LiteralPath (Join-Path $PSScriptRoot "test_engines\basilisk-$suffix-pext-pgo.manifest.txt") -Raw |
        Select-String -Pattern '(?m)^bench:\s+(\d+)').Matches.Groups[1].Value
}
if (($benches | Select-Object -Unique).Count -ne 1) {
    throw "candidate and baseline benches differ; something outside kbnk_score changed"
}

$sprtArgs = @(
    "-EngineA", $engines["6.2a-cand"],
    "-EngineB", $engines["6.2a-base"],
    "-NameA", "6.2a-cand-kbnk",
    "-NameB", "6.2a-base-6.1.b",
    "-Threads", $Threads,
    "-Hash", $Hash,
    "-TC", $TC
)
if ($NonRegression) {
    $sprtArgs += @("-Mode", "simplify")
    Write-Host "Test design: SPRT [-5,0] nElo non-regression, no adjudication"
} else {
    $sprtArgs += @("-Mode", "gainer", "-Elo1", 3)
    Write-Host "Test design: SPRT [0,3] nElo gainer (as registered), no adjudication"
}
Write-Host "Adjudication: OFF (no -Adjudicate switch is passed)"

if ($WhatIfOnly) {
    Write-Host ""
    Write-Host "Validated. Would run:"
    Write-Host ("  {0} {1}" -f (Join-Path $PSScriptRoot "sprt.ps1"), ($sprtArgs -join " "))
    exit 0
}

& (Join-Path $PSScriptRoot "sprt.ps1") @sprtArgs
if ($LASTEXITCODE -ne 0) { throw "6.2.a SPRT failed with exit code $LASTEXITCODE" }
Write-Host "6.2.a complete; the log and manifest are in tools\results\."
