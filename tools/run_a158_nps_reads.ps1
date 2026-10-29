<# Frozen launcher for BAS-P22's pooled-build NPS read (PLAN A.15.8).

   Pools, reused when they exist, else built at HEAD, four builds each:
     a158-native      build_test.ps1 -Native: release as a user builds it here
     a158-pext        build_test.ps1 Release: the pext asset's recipe
     a158-universal   build_test.ps1 -Universal: the four-tier universal binary
     a158-msvc        build_test.ps1 -Msvc, in the Visual Studio x64 environment

   One read, step 1 and step 2 only when a contrast says EXTEND:
     native     a158-native over a158-universal (gain)
     nativecg   a158-native over a158-pext (gain)
     hot        a158-universal against a158-pext (no-regression)
     msvc       a158-msvc over a158-pext (gain)
   A REPEAT repeats the read once into a fresh directory; a second stops for
   the maintainer. -BuildOnly builds the pools and stops. #>
param([switch]$BuildOnly)

$ErrorActionPreference = 'Stop'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/a158-nps'
$buildTest = Join-Path $PSScriptRoot 'build_test.ps1'
$bench = 19470325
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'

$arms = [ordered]@{
    native    = @{ Switch = '-Native'; Staged = 'native-pgo' }
    pext      = @{ Switch = ''; Staged = 'pext-pgo' }
    universal = @{ Switch = '-Universal'; Staged = 'universal-pgo' }
    msvc      = @{ Switch = '-Msvc'; Staged = 'msvc-pgo' }
}
$letters = @{ native = 'n'; pext = 'p'; universal = 'u'; msvc = 'm' }
$pools = [ordered]@{}
foreach ($arm in $arms.Keys) {
    $pool = Join-Path $engines "a158-$arm.pool.json"
    $files = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a158-$($letters[$arm])$_-$($arms[$arm].Staged).exe" })
    if (-not (Test-Path -LiteralPath $pool)) {
        foreach ($i in 1..4) {
            if (Test-Path -LiteralPath $files[$i - 1]) { continue }
            $suffix = "a158-$($letters[$arm])$i"
            if ($arm -eq 'msvc') {
                & cmd /c "call `"$vcvars`" >nul && pwsh -NoProfile -File `"$buildTest`" -Suffix $suffix -Flavor Release -Msvc" | Out-Host
                if ($LASTEXITCODE -ne 0) { throw "MSVC build $suffix failed (exit $LASTEXITCODE)." }
            } elseif ($arms[$arm].Switch) {
                $switch = @{ ($arms[$arm].Switch.TrimStart('-')) = $true }
                & $buildTest -Suffix $suffix -Flavor Release @switch | Out-Host
            } else {
                & $buildTest -Suffix $suffix -Flavor Release | Out-Host
            }
        }
        & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name "a158-$arm" -ExpectBench $bench -Engines $files | Out-Host
    }
    $pools[$arm] = $pool
}
if ($BuildOnly) { Write-Host 'Pools built; no read run.'; return }

$contrasts = @(
    'native=a158-universal:a158-native:gain',
    'nativecg=a158-pext:a158-native:gain',
    'hot=a158-pext:a158-universal:no-regression',
    'msvc=a158-pext:a158-msvc:gain'
)
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($name in @('p22', 'p22-repeat')) {
    $out = Join-Path $results $name
    if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-2.json'))) {
        if (Test-Path -LiteralPath $out) { throw "$out holds a read stopped part-way; move it aside and rerun." }
        & $read -Pools @($pools.Values) -Contrasts $contrasts -Out $out -Label "BAS-P22 $name"
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
throw 'BAS-P22 repeated twice; stop for the maintainer.'
