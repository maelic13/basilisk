<# Frozen launcher for BAS-P21's pooled-build NPS read (PLAN A.15.6).

   Pools, reused when they exist, else built at HEAD:
     a156-pext        four build_test.ps1 Release builds: today's pext asset
     a156-universal   the universal binaries (functions aligned to 64 bytes)
                      of four build_test.ps1 -Universal builds
     a156-tier-pext   the pext tier builds those four were linked from

   One read of the three pools (registration amendment 1), step 1 and step 2
   only when a contrast says EXTEND, with three contrasts:
     hot     a156-pext against a156-universal, no-regression (the gate)
     place   a156-tier-pext against a156-universal: the relink, with the
             tier's own alignment kept
     align   a156-pext against a156-tier-pext: 64-byte alignment itself
   A REPEAT repeats the read once into a fresh directory; a second stops for
   the maintainer. -BuildOnly builds the pools and stops. A finished read is
   kept, so a stopped run resumes; a read stopped part-way must be moved
   aside first. #>
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

$contrasts = @(
    'hot=a156-pext:a156-universal:no-regression',
    'place=a156-tier-pext:a156-universal:gain',
    'align=a156-pext:a156-tier-pext:gain'
)
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($name in @('p21', 'p21-repeat')) {
    $out = Join-Path $results $name
    if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-2.json'))) {
        if (Test-Path -LiteralPath $out) { throw "$out holds a read stopped part-way; move it aside and rerun." }
        & $read -Pools $pools.pext, $pools.universal, $pools['tier-pext'] -Contrasts $contrasts -Out $out -Label "BAS-P21 $name"
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
throw 'BAS-P21 repeated twice; stop for the maintainer.'
