<# Frozen launcher for PLAN B.2.0's pooled-PGO NPS pool (BAS-P17).

   Two final-PGO Release builds of the B.2.0 head (9b51b46) against BAS-P16's
   two builds of the B.1 head, every binary pinned by SHA-256, then two
   nps_ab.ps1 runs on an idle host:
     1. a self pair over the two B.2.0 builds, which must read ~0.00%;
     2. the B.2.0 pair against the B.1 pair: the pool's reading. #>

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$outDir = Join-Path $PSScriptRoot 'results\b20-nps'
if (Test-Path -LiteralPath $outDir) { throw "$outDir exists; move it away rather than overwrite a run." }
$engines = Join-Path $PSScriptRoot 'test_engines'
$b20 = @(
    @{ Path = Join-Path $engines 'basilisk-b20-pgo1-pext-pgo.exe'; Sha = 'FD7384FC6102ED86C8CD943C1A009B263D9AA2AC5A51BE9980FE03E81387A134' }
    @{ Path = Join-Path $engines 'basilisk-b20-pgo2-pext-pgo.exe'; Sha = '18AED349459021C0EFADBF411D3B47192AD93096CA15F2F8110FD1DAC8F37811' }
)
$b1 = @(
    @{ Path = Join-Path $engines 'basilisk-b1-pgo1-pext-pgo.exe'; Sha = 'FC7973810A54D0B93717CCA2028BC93635844EE016ED07CDFB9C02320E840C96' }
    @{ Path = Join-Path $engines 'basilisk-b1-pgo2-pext-pgo.exe'; Sha = '6A7BB43878FB832DF075A80F8DC061AFE6B1E2686DF9D9446DF762BAF70EB56D' }
)
foreach ($engine in $b20 + $b1) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

New-Item -ItemType Directory -Path $outDir | Out-Null
Start-Transcript -LiteralPath (Join-Path $outDir 'transcript.txt') | Out-Null
try {
    Write-Host "B.2.0 NPS pool (BAS-P17), $(Get-Date -Format s)"
    Assert-HarnessHostIdle | Out-Null
    $nps = Join-Path $PSScriptRoot 'nps_ab.ps1'
    $bench = @{ Depth = 13; Repeats = 3; Rounds = 16 }

    $selfLog = Join-Path $outDir 'selfpair.txt'
    & $nps -EnginesA ($b20 | ForEach-Object Path) -SelfPair -Label 'b20-nps self pair' @bench *>&1 |
        Tee-Object -FilePath $selfLog | Out-Host
    if (-not (Select-String -LiteralPath $selfLog -Pattern 'SELF-PAIR OK' -Quiet)) {
        throw "Self pair did not read OK; see $selfLog."
    }

    $gateLog = Join-Path $outDir 'b20-vs-b1.txt'
    & $nps -EnginesA ($b20 | ForEach-Object Path) -EnginesB ($b1 | ForEach-Object Path) `
        -Label 'b20-nps B.2.0 vs B.1 head' @bench *>&1 | Tee-Object -FilePath $gateLog | Out-Host
    if (-not (Select-String -LiteralPath $gateLog -Pattern 'delta \(median\)' -Quiet)) {
        throw "Pool comparison produced no result; see $gateLog."
    }
    Write-Host "B.2.0 NPS pool finished $(Get-Date -Format s); results in $outDir"
} finally {
    Stop-Transcript | Out-Null
}
