<# Frozen launcher for BAS-P23: BAS-P22's read repeated on an Intel host.

   Run from a clean checkout of the registered revision, on an idle machine,
   in PowerShell 7 with MSYS2's clang64 (Clang 23) on PATH, and mingw64's
   g++ for the GCC pool. Pools, built here when missing, four builds each:
     p23-native      build_test.ps1 -Native: release with NATIVE=ON
     p23-pext        build_test.ps1 Release: the pext asset's recipe
     p23-universal   build_test.ps1 -Universal: the four-tier universal binary
     p23-gcc         build_test.ps1 -Native -Gcc (skipped with -NoGcc)

   One read, step 1 and step 2 only when a contrast says EXTEND:
     native     p23-native over p23-universal (gain)
     nativecg   p23-native over p23-pext (gain)
     hot        p23-universal against p23-pext (no-regression)
     gcc        p23-gcc over p23-native (gain)
   -Cpu pins the read to one logical CPU: on a hybrid Intel CPU, a
   performance core. The CPU and host are written to host.txt beside the
   archive in tools/results/p23-intel/. #>
param([switch]$BuildOnly, [switch]$NoGcc, [int]$Cpu = -1)

$ErrorActionPreference = 'Stop'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/p23-intel'
$buildTest = Join-Path $PSScriptRoot 'build_test.ps1'
$bench = 19470325

$arms = [ordered]@{
    native    = @{ Args = @{ Native = $true }; Staged = 'native-pgo'; Letter = 'n' }
    pext      = @{ Args = @{}; Staged = 'pext-pgo'; Letter = 'p' }
    universal = @{ Args = @{ Universal = $true }; Staged = 'universal-pgo'; Letter = 'u' }
    gcc       = @{ Args = @{ Native = $true; Gcc = $true }; Staged = 'native-gcc-pgo'; Letter = 'g' }
}
if ($NoGcc) { $arms.Remove('gcc') }

$pools = [ordered]@{}
foreach ($arm in $arms.Keys) {
    $pool = Join-Path $engines "p23-$arm.pool.json"
    $files = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-p23-$($arms[$arm].Letter)$_-$($arms[$arm].Staged).exe" })
    if (-not (Test-Path -LiteralPath $pool)) {
        foreach ($i in 1..4) {
            $file = $files[$i - 1]
            $manifest = [IO.Path]::ChangeExtension($file, '.manifest.txt')
            if ((Test-Path -LiteralPath $file) -and (Test-Path -LiteralPath $manifest)) { continue }
            $buildArgs = $arms[$arm].Args
            & $buildTest -Suffix "p23-$($arms[$arm].Letter)$i" -Flavor Release @buildArgs | Out-Host
        }
        & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name "p23-$arm" -ExpectBench $bench -Engines $files | Out-Host
    }
    $pools[$arm] = $pool
}
if ($BuildOnly) { Write-Host 'Pools built; no read run.'; return }

New-Item -ItemType Directory -Force -Path $results | Out-Null
$cpuInfo = Get-CimInstance Win32_Processor | Select-Object -First 1
@(
    "cpu: $($cpuInfo.Name)"
    "cores: $($cpuInfo.NumberOfCores); logical: $($cpuInfo.NumberOfLogicalProcessors)"
    "os: $((Get-CimInstance Win32_OperatingSystem).Caption)"
    "pinned_cpu: $(if ($Cpu -ge 0) { $Cpu } else { 'default (the last physical core)' })"
    "revision: $((& git rev-parse HEAD).Trim())"
) | Set-Content -LiteralPath (Join-Path $results 'host.txt') -Encoding utf8

$contrasts = @(
    'native=p23-universal:p23-native:gain',
    'nativecg=p23-pext:p23-native:gain',
    'hot=p23-pext:p23-universal:no-regression'
)
if (-not $NoGcc) { $contrasts += 'gcc=p23-native:p23-gcc:gain' }
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
$cpuArgs = if ($Cpu -ge 0) { @{ Cpu = $Cpu } } else { @{} }
foreach ($name in @('p23', 'p23-repeat')) {
    $out = Join-Path $results $name
    if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-2.json'))) {
        if (Test-Path -LiteralPath $out) { throw "$out holds a read stopped part-way; move it aside and rerun." }
        & $read -Pools @($pools.Values) -Contrasts $contrasts -Out $out -Label "BAS-P23 $name" @cpuArgs
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
throw 'BAS-P23 repeated twice; stop for the maintainer.'
