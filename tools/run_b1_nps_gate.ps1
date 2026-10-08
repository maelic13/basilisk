<# Frozen launcher for PLAN B.1's pooled-PGO NPS gate (BAS-P16).

   Two final-PGO Release builds of the B.1 head against BAS-P15's builds 1-2
   of the A.8 head, every binary pinned by SHA-256, then two nps_ab.ps1 runs
   on an idle host:
     1. a self pair over the two B.1 builds, which must read ~0.00%;
     2. the B.1 pair against the A.8 pair: the gate's reading. #>

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$outDir = Join-Path $PSScriptRoot 'results\b1-nps'
if (Test-Path -LiteralPath $outDir) { throw "$outDir exists; move it away rather than overwrite a run." }
$engines = Join-Path $PSScriptRoot 'test_engines'
$b1 = @(
    @{ Path = Join-Path $engines 'basilisk-b1-pgo1-pext-pgo.exe'; Sha = 'FC7973810A54D0B93717CCA2028BC93635844EE016ED07CDFB9C02320E840C96' }
    @{ Path = Join-Path $engines 'basilisk-b1-pgo2-pext-pgo.exe'; Sha = '6A7BB43878FB832DF075A80F8DC061AFE6B1E2686DF9D9446DF762BAF70EB56D' }
)
$a8 = @(
    @{ Path = Join-Path $engines 'basilisk-a820-nps-pgo1-pext-pgo.exe'; Sha = '1932634705AEFBE416199236EBE5F9292D09ED34A636AE0F231050AE249AC546' }
    @{ Path = Join-Path $engines 'basilisk-a820-nps-pgo2-pext-pgo.exe'; Sha = '3169742D4DBCDCA7BC2891911B67BB0D7E6507C51A1C26E35FB650761A151F9D' }
)
foreach ($engine in $b1 + $a8) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

New-Item -ItemType Directory -Path $outDir | Out-Null
Start-Transcript -LiteralPath (Join-Path $outDir 'transcript.txt') | Out-Null
try {
    Write-Host "B.1 NPS gate (BAS-P16), $(Get-Date -Format s)"
    Assert-HarnessHostIdle | Out-Null
    $nps = Join-Path $PSScriptRoot 'nps_ab.ps1'
    $bench = @{ Depth = 13; Repeats = 3; Rounds = 16 }

    $selfLog = Join-Path $outDir 'selfpair.txt'
    & $nps -EnginesA ($b1 | ForEach-Object Path) -SelfPair -Label 'b1-nps self pair' @bench *>&1 |
        Tee-Object -FilePath $selfLog | Out-Host
    if (-not (Select-String -LiteralPath $selfLog -Pattern 'SELF-PAIR OK' -Quiet)) {
        throw "Self pair did not read OK; see $selfLog."
    }

    $gateLog = Join-Path $outDir 'b1-vs-a8.txt'
    & $nps -EnginesA ($b1 | ForEach-Object Path) -EnginesB ($a8 | ForEach-Object Path) `
        -Label 'b1-nps B.1 vs A.8 head' @bench *>&1 | Tee-Object -FilePath $gateLog | Out-Host
    if (-not (Select-String -LiteralPath $gateLog -Pattern 'delta \(median\)' -Quiet)) {
        throw "Gate comparison produced no result; see $gateLog."
    }
    Write-Host "B.1 NPS gate finished $(Get-Date -Format s); results in $outDir"
} finally {
    Stop-Transcript | Out-Null
}
