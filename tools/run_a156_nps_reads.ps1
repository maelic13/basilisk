<# Frozen launcher for BAS-P21's three pooled-build NPS reads (PLAN A.15.6).

   Pools, reused when they exist, else built at HEAD:
     a156-pext        four build_test.ps1 Release builds: today's pext asset
     a156-universal   the universal binaries (functions aligned to 64 bytes)
                      of four build_test.ps1 -Universal builds
     a156-tier-pext   the pext tier builds those four were linked from

   Reads, in the registered order, each step 1 and step 2 only on EXTEND:
     hot     a156-pext against a156-universal, no-regression (the gate)
     place   a156-tier-pext against a156-universal: the relink, with the
             tier's own alignment kept
     align   a156-pext against a156-tier-pext: 64-byte alignment itself
   -BuildOnly builds the pools and stops. A read whose archive exists is
   skipped, so a stopped run resumes. #>
param([switch]$BuildOnly)

$ErrorActionPreference = 'Stop'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/a156-nps'
$bench = 19470325

function Use-Pool([string]$Name, [hashtable]$Arguments) {
    $path = Join-Path $engines "$Name.pool.json"
    if (-not (Test-Path -LiteralPath $path)) { & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name $Name @Arguments | Out-Host }
    $path
}

if (-not (Test-Path -LiteralPath (Join-Path $engines 'a156-universal.pool.json'))) {
    foreach ($i in 1..4) {
        if (-not (Test-Path -LiteralPath (Join-Path $engines "basilisk-a156-u$i-universal-pgo.exe"))) {
            & (Join-Path $PSScriptRoot 'build_test.ps1') -Suffix "a156-u$i" -Flavor Release -Universal | Out-Host
        }
    }
}
$pools = @{ pext = Use-Pool 'a156-pext' @{ Builds = 4; ExpectBench = $bench } }
foreach ($kind in @(@('universal', 'universal-pgo'), @('tier-pext', 'pext-pgo'))) {
    $files = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a156-u$_-$($kind[1]).exe" })
    $pools[$kind[0]] = Use-Pool "a156-$($kind[0])" @{ ExpectBench = $bench; Engines = $files }
}
if ($BuildOnly) { Write-Host 'Pools built; no read run.'; return }

$reads = @(
    @('hot', 'pext', 'universal', $true),
    @('place', 'tier-pext', 'universal', $false),
    @('align', 'pext', 'tier-pext', $false)
)
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($r in $reads) {
    $out = Join-Path $results $r[0]
    if (Test-Path -LiteralPath (Join-Path $out 'runs.jsonl')) { Write-Host "$($r[0]): archived, skipped"; continue }
    $noRegression = [bool]$r[3]
    & $read -Base $pools[$r[1]] -Cand $pools[$r[2]] -Out $out -Label "BAS-P21 $($r[0])" -NoRegression:$noRegression -QuietSeconds 120
    $step1 = Get-Content -LiteralPath (Join-Path $out 'verdict-2.json') -Raw | ConvertFrom-Json
    if ($step1.verdict -eq 'EXTEND') { & $read -Out $out -Extend -QuietSeconds 120 }
    & python (Join-Path $PSScriptRoot 'diag/nps_read.py') --report $out
}
