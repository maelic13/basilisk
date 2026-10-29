<# Frozen launcher for BAS-P25's pooled-build NPS read (PLAN A.16).

   Pools, reused when they exist, four build_test.ps1 Release builds each:
     a16-base   master before A.16 (0680393), from a temporary worktree
     a16-cand   this checkout's HEAD, the zero-weight terms no longer computed

   One read, step 1 and step 2 only when the contrast says EXTEND:
     zero   a16-cand over a16-base (gain)
   A REPEAT repeats the read once into a fresh directory; a second stops for
   the maintainer. -BuildOnly builds the pools and stops. #>
param([switch]$BuildOnly)

$ErrorActionPreference = 'Stop'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/a16-nps'
$bench = 19470325

function Use-Pool([string]$Name, [hashtable]$Arguments) {
    $path = Join-Path $engines "$Name.pool.json"
    if (-not (Test-Path -LiteralPath $path)) { & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name $Name @Arguments | Out-Host }
    $path
}

$base = Use-Pool 'a16-base' @{ Builds = 4; ExpectBench = $bench; Revision = '0680393' }
$cand = Use-Pool 'a16-cand' @{ Builds = 4; ExpectBench = $bench }
if ($BuildOnly) { Write-Host 'Pools built; no read run.'; return }

$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($name in @('p25', 'p25-repeat')) {
    $out = Join-Path $results $name
    if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-2.json'))) {
        if (Test-Path -LiteralPath $out) { throw "$out holds a read stopped part-way; move it aside and rerun." }
        & $read -Pools @($base, $cand) -Contrasts @('zero=a16-base:a16-cand:gain') -Out $out -Label "BAS-P25 $name"
    }
    $step1 = Get-Content -LiteralPath (Join-Path $out 'verdict-2.json') -Raw | ConvertFrom-Json
    $final = $step1
    if ($step1.next -eq 'EXTEND') {
        if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-6.json'))) { & $read -Out $out -Extend }
        $final = Get-Content -LiteralPath (Join-Path $out 'verdict-6.json') -Raw | ConvertFrom-Json
    }
    & python (Join-Path $PSScriptRoot 'diag/nps_read.py') --report $out
    if ($final.next -ne 'REPEAT') { return }
}
throw 'BAS-P25 repeated twice; stop for the maintainer.'
