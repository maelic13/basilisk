<# Frozen launcher for BAS-P26's pooled-build NPS read (PLAN A.17).

   Pools, reused when they exist, else built at HEAD, four build_test.ps1
   Release builds each (Clang):
     a17-pext     the pext asset's recipe: the control
     a17-comp     pext with PEXT_COMPRESSED=ON (candidate 1)
     a17-tune     pext with -mtune=znver3 (candidate 5: the tuning model)
     a17-v3       pext with -mfma -mf16c -mmovbe (candidate 5: x86-64-v3's rest)
     a17-native   -Native (candidate 5: BAS-P22's native build)
     a17-natgen   -Native with -mtune=generic (the native families alone)
     a17-natalign -Native with -falign-functions=64 (native's layout)

   One read, step 1 and step 2 only when a contrast says EXTEND, every
   contrast in the gain form:
     comp       a17-comp over a17-pext
     tune       a17-tune over a17-pext
     v3         a17-v3 over a17-pext
     native     a17-native over a17-pext
     natgen     a17-natgen over a17-pext
     nattune    a17-native over a17-natgen  (znver3 tuning inside native)
     natalign   a17-natalign over a17-native
   A REPEAT repeats the read once into a fresh directory; a second stops for
   the maintainer. -BuildOnly builds the pools and stops. #>
param([switch]$BuildOnly)

$ErrorActionPreference = 'Stop'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results/a17-nps'
$buildTest = Join-Path $PSScriptRoot 'build_test.ps1'
$bench = 19470325

$arms = [ordered]@{
    pext     = @{ Args = @{}; Staged = 'pext-pgo' }
    comp     = @{ Args = @{ ArmOption = 'PEXT_COMPRESSED'; Arm = 'On' }; Staged = 'pext-pgo' }
    tune     = @{ Args = @{ CxxFlags = '-mtune=znver3' }; Staged = 'pext-pgo' }
    v3       = @{ Args = @{ CxxFlags = '-mfma -mf16c -mmovbe' }; Staged = 'pext-pgo' }
    native   = @{ Args = @{ Native = $true }; Staged = 'native-pgo' }
    natgen   = @{ Args = @{ Native = $true; CxxFlags = '-mtune=generic' }; Staged = 'native-pgo' }
    natalign = @{ Args = @{ Native = $true; CxxFlags = '-falign-functions=64' }; Staged = 'native-pgo' }
}
$pools = [ordered]@{}
foreach ($arm in $arms.Keys) {
    $pool = Join-Path $engines "a17-$arm.pool.json"
    $files = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a17-$arm$_-$($arms[$arm].Staged).exe" })
    if (-not (Test-Path -LiteralPath $pool)) {
        foreach ($i in 1..4) {
            if (Test-Path -LiteralPath $files[$i - 1]) { continue }
            $buildArgs = $arms[$arm].Args
            & $buildTest -Suffix "a17-$arm$i" -Flavor Release @buildArgs | Out-Host
        }
        & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name "a17-$arm" -ExpectBench $bench -Engines $files | Out-Host
    }
    $pools[$arm] = $pool
}
if ($BuildOnly) { Write-Host 'Pools built; no read run.'; return }

$contrasts = @(
    'comp=a17-pext:a17-comp:gain',
    'tune=a17-pext:a17-tune:gain',
    'v3=a17-pext:a17-v3:gain',
    'native=a17-pext:a17-native:gain',
    'natgen=a17-pext:a17-natgen:gain',
    'nattune=a17-natgen:a17-native:gain',
    'natalign=a17-native:a17-natalign:gain'
)
$read = Join-Path $PSScriptRoot 'nps_read.ps1'
foreach ($name in @('p26', 'p26-repeat')) {
    $out = Join-Path $results $name
    if (-not (Test-Path -LiteralPath (Join-Path $out 'verdict-2.json'))) {
        if (Test-Path -LiteralPath $out) { throw "$out holds a read stopped part-way; move it aside and rerun." }
        & $read -Pools @($pools.Values) -Contrasts $contrasts -Out $out -Label "BAS-P26 $name"
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
throw 'BAS-P26 repeated twice; stop for the maintainer.'
