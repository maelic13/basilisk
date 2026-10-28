<# Frozen launcher for BAS-P20's five pooled-build NPS reads (PLAN A.15.4).

   Pools, reused when they exist, else built at HEAD:
     a15-pext           four build_test.ps1 Release builds (the pext asset)
     a15-universal      the universal binaries of four build_test.ps1
                        -Universal builds
     a15-tier-<tier>    the portable, avx2 and pext tier builds those four
                        universal builds were linked from

   Reads, in the registered order, each step 1 and step 2 only on EXTEND:
     hot            a15-pext against a15-universal, no-regression (the gate)
     tier           a15-pext against a15-tier-pext
     avx2-portable  a15-tier-portable against a15-tier-avx2
     pext-avx2      a15-tier-avx2 against a15-tier-pext
     pext-portable  a15-tier-portable against a15-tier-pext
   A read whose archive exists is skipped, so a stopped run resumes. #>

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/a15-nps'
$bench = 19470325

function Use-Pool([string]$Name, [hashtable]$Arguments) {
    $path = Join-Path $engines "$Name.pool.json"
    if (-not (Test-Path -LiteralPath $path)) { & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name $Name @Arguments | Out-Host }
    $path
}

$tiers = @{ universal = 'universal-pgo'; 'tier-pext' = 'pext-pgo'; 'tier-avx2' = 'avx2-pgo'; 'tier-portable' = 'portable-pgo' }
$staged = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a15-u$_-universal-pgo.exe" })
if (-not (Test-Path -LiteralPath (Join-Path $engines 'a15-universal.pool.json'))) {
    foreach ($i in 1..4) {
        if (-not (Test-Path -LiteralPath $staged[$i - 1])) {
            & (Join-Path $PSScriptRoot 'build_test.ps1') -Suffix "a15-u$i" -Flavor Release -Universal | Out-Host
        }
    }
}
$pools = @{ pext = Use-Pool 'a15-pext' @{ Builds = 4; ExpectBench = $bench } }
foreach ($kind in $tiers.Keys) {
    $files = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a15-u$_-$($tiers[$kind]).exe" })
    $pools[$kind] = Use-Pool "a15-$kind" @{ ExpectBench = $bench; Engines = $files }
}

$reads = @(
    @('hot', 'pext', 'universal', $true),
    @('tier', 'pext', 'tier-pext', $false),
    @('avx2-portable', 'tier-portable', 'tier-avx2', $false),
    @('pext-avx2', 'tier-avx2', 'tier-pext', $false),
    @('pext-portable', 'tier-portable', 'tier-pext', $false)
)
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($r in $reads) {
    $out = Join-Path $results $r[0]
    if (Test-Path -LiteralPath (Join-Path $out 'runs.jsonl')) { Write-Host "$($r[0]): archived, skipped"; continue }
    $noRegression = [bool]$r[3]
    & $read -Base $pools[$r[1]] -Cand $pools[$r[2]] -Out $out -Label "BAS-P20 $($r[0])" -NoRegression:$noRegression -QuietSeconds 120
    $step1 = Get-Content -LiteralPath (Join-Path $out 'verdict-2.json') -Raw | ConvertFrom-Json
    if ($step1.verdict -eq 'EXTEND') { & $read -Out $out -Extend -QuietSeconds 120 }
    & python (Join-Path $PSScriptRoot 'diag/nps_read.py') --report $out
}
