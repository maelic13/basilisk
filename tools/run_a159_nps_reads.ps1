<# Frozen launcher for BAS-P24's pooled-build NPS read (PLAN A.15.9).

   Pools, reused when they exist, else built at HEAD, four builds each:
     a159-casm    build_test.ps1 -Universal: Clang's universal binary, assembly route
     a159-cobj    build_test.ps1 -Universal -UniversalRoute objects: today's Clang route
     a159-cpext   build_test.ps1 Release: Clang's pext build, the per-tier recipe
     a159-gasm    build_test.ps1 -Universal -Gcc: GCC's universal binary (assembly)
     a159-gpext   build_test.ps1 Release -Gcc: GCC's pext build

   One read, step 1 and step 2 only when a contrast says EXTEND:
     route     a159-casm against a159-cobj (no-regression)
     hot       a159-casm against a159-cpext (no-regression)
     gcchot    a159-gasm against a159-gpext (no-regression)
   A REPEAT repeats the read once into a fresh directory; a second stops for
   the maintainer. -BuildOnly builds the pools and stops. #>
param([switch]$BuildOnly)

$ErrorActionPreference = 'Stop'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/a159-nps'
$buildTest = Join-Path $PSScriptRoot 'build_test.ps1'
$bench = 19470325

$arms = [ordered]@{
    casm  = @{ Args = @{ Universal = $true }; Staged = 'universal-pgo' }
    cobj  = @{ Args = @{ Universal = $true; UniversalRoute = 'objects' }; Staged = 'objects-universal-pgo' }
    cpext = @{ Args = @{}; Staged = 'pext-pgo' }
    gasm  = @{ Args = @{ Universal = $true; Gcc = $true }; Staged = 'universal-gcc-pgo' }
    gpext = @{ Args = @{ Gcc = $true }; Staged = 'pext-gcc-pgo' }
}
$letters = @{ casm = 'a'; cobj = 'o'; cpext = 'p'; gasm = 'g'; gpext = 'q' }
$pools = [ordered]@{}
foreach ($arm in $arms.Keys) {
    $pool = Join-Path $engines "a159-$arm.pool.json"
    $files = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a159-$($letters[$arm])$_-$($arms[$arm].Staged).exe" })
    if (-not (Test-Path -LiteralPath $pool)) {
        foreach ($i in 1..4) {
            if (Test-Path -LiteralPath $files[$i - 1]) { continue }
            $buildArgs = $arms[$arm].Args
            & $buildTest -Suffix "a159-$($letters[$arm])$i" -Flavor Release @buildArgs | Out-Host
        }
        & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name "a159-$arm" -ExpectBench $bench -Engines $files | Out-Host
    }
    $pools[$arm] = $pool
}
if ($BuildOnly) { Write-Host 'Pools built; no read run.'; return }

$contrasts = @(
    'route=a159-cobj:a159-casm:no-regression',
    'hot=a159-cpext:a159-casm:no-regression',
    'gcchot=a159-gpext:a159-gasm:no-regression'
)
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($name in @('p24', 'p24-repeat')) {
    $out = Join-Path $results $name
    if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-2.json'))) {
        if (Test-Path -LiteralPath $out) { throw "$out holds a read stopped part-way; move it aside and rerun." }
        & $read -Pools @($pools.Values) -Contrasts $contrasts -Out $out -Label "BAS-P24 $name"
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
throw 'BAS-P24 repeated twice; stop for the maintainer.'
