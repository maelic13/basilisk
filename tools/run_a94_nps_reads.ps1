<# Frozen launcher for the pooled-build NPS read's first two reads.

   -Read null   BAS-P18: two independent four-build pools of the head's engine
                source. Pool A is built in this checkout at HEAD and pool B
                from a temporary worktree of 4fa68b4 (identical engine
                source), so a zero also clears the worktree path that BAS-P19's
                clang-23 pool is built in. Gain mode; the read must not accept.
   -Read clang  BAS-P19: A.8.20's head 9c93d15, its four clang 22.1.8 builds
                (BAS-P15's pool) against four clang 23.1.3 builds of the same
                revision. No-regression mode.

   Each read runs step 1 and, only when step 1 says EXTEND, step 2. Pools that
   already exist are reused, never rebuilt. #>
param([Parameter(Mandatory)][ValidateSet('null', 'clang')][string]$Read)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results'
$headSource = '4fa68b4'

function Use-Pool([string]$Name, [hashtable]$Arguments) {
    # A build writes cmake's output to the pipeline; it goes to the console so
    # that the pool's path is the only thing this returns.
    $path = Join-Path $engines "$Name.pool.json"
    if (-not (Test-Path -LiteralPath $path)) { & (Join-Path $PSScriptRoot 'nps_pool.ps1') -Name $Name @Arguments | Out-Host }
    $path
}

function Invoke-Read([string]$Base, [string]$Cand, [string]$Out, [string]$Label, [switch]$NoRegression) {
    $read = Join-Path $PSScriptRoot 'nps_read.ps1'
    & $read -Base $Base -Cand $Cand -Out $Out -Label $Label -NoRegression:$NoRegression -QuietSeconds 120
    $step1 = Get-Content -LiteralPath (Join-Path $Out 'verdict-2.json') -Raw | ConvertFrom-Json
    if ($step1.verdict -eq 'EXTEND') { & $read -Out $Out -Extend -QuietSeconds 120 }
}

if ($Read -eq 'null') {
    & git -C $root diff --quiet $headSource HEAD -- src tests CMakeLists.txt CMakePresets.json cmake external
    if ($LASTEXITCODE -ne 0) { throw "HEAD's engine source differs from $headSource; BAS-P18 needs them identical." }
    $a = Use-Pool 'a94-head-a' @{ Builds = 4; ExpectBench = 8437148 }
    $b = Use-Pool 'a94-head-b' @{ Builds = 4; ExpectBench = 8437148; Revision = $headSource }
    Invoke-Read $a $b (Join-Path $results 'a94-nps-null') 'BAS-P18 null: two pools of the head'
} else {
    $clang22 = @(1..4 | ForEach-Object { Join-Path $engines "basilisk-a820-nps-pgo$_-pext-pgo.exe" })
    $old = Use-Pool 'a94-a820-clang22' @{ ExpectBench = 14978465; Engines = $clang22 }
    $new = Use-Pool 'a94-a820-clang23' @{ Builds = 4; ExpectBench = 14978465; Revision = '9c93d15' }
    Invoke-Read $old $new (Join-Path $results 'a94-nps-clang') 'BAS-P19 clang 22.1.8 to 23.1.3' -NoRegression
}
