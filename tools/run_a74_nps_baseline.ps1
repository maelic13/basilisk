<# Frozen launcher for PLAN A.7.4's pooled-PGO NPS baseline of 1.10.1.

   Four final-PGO Release builds of the A.4.2 revision, each verified at bench
   14,978,465 by build_test.ps1, then two nps_ab.ps1 runs on an idle host:
     1. a self pair over all four builds, which must read ~0.00%;
     2. builds 1-2 against builds 3-4, which prices PGO build-to-build luck
        and gives the pooled baseline median.
   -Smoke proves the wiring in a few minutes with one build and a token
   bench; its output is never a measurement. #>
param([switch]$Smoke)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$revision = '3e5294be5db8e0bbe0eb8a1b18476de58c33845b'
$repo = (Resolve-Path "$PSScriptRoot\..").Path
$label = if ($Smoke) { 'a74-nps-smoke' } else { 'a74-nps' }
$outDir = Join-Path $PSScriptRoot "results\$label"
if (Test-Path -LiteralPath $outDir) { throw "$outDir exists; move it away rather than overwrite a run." }
New-Item -ItemType Directory -Path $outDir | Out-Null
$builds = if ($Smoke) { 1 } else { 4 }
$bench = if ($Smoke) { @{ Depth = 6; Repeats = 1; Rounds = 1 } } else { @{ Depth = 13; Repeats = 3; Rounds = 16 } }

Start-Transcript -LiteralPath (Join-Path $outDir 'transcript.txt') | Out-Null
try {
    Write-Host "A.7.4 NPS baseline: revision $revision, $builds build(s), $(Get-Date -Format s)"
    Assert-HarnessHostIdle | Out-Null

    $work = Join-Path ([IO.Path]::GetTempPath()) ("basilisk-a74-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
    git -C $repo worktree add --detach $work $revision 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "git worktree add failed (exit $LASTEXITCODE)." }
    $engines = @()
    try {
        for ($i = 1; $i -le $builds; $i++) {
            $suffix = "$label-pgo$i"
            & pwsh -NoProfile -File (Join-Path $work 'tools\build_test.ps1') -Suffix $suffix -Flavor Release `
                -TestEnginesDir (Join-Path $PSScriptRoot 'test_engines') | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "build_test.ps1 failed for $suffix (exit $LASTEXITCODE)." }
            $exe = Join-Path $PSScriptRoot "test_engines\basilisk-$suffix-pext-pgo.exe"
            $manifest = Read-BasiliskBuildManifest -Path ([IO.Path]::ChangeExtension($exe, '.manifest.txt'))
            if ($manifest.GitSha -ne $revision -or $manifest.Bench -ne '14978465' -or $manifest.DirtyDiff -ne 'clean') {
                throw "Build $suffix is not a clean $revision at bench 14978465."
            }
            Write-Host "Build ${i}: $exe  SHA-256 $($manifest.BinarySha256)"
            $engines += $exe
        }
    } finally {
        git -C $repo worktree remove --force $work 2>&1 | Out-Null
    }

    Assert-HarnessHostIdle | Out-Null
    $nps = Join-Path $PSScriptRoot 'nps_ab.ps1'
    $selfLog = Join-Path $outDir 'selfpair.txt'
    & $nps -EnginesA $engines -SelfPair -Label "$label self pair" @bench *>&1 |
        Tee-Object -FilePath $selfLog | Out-Host
    if (-not (Select-String -LiteralPath $selfLog -Pattern 'SELF-PAIR OK' -Quiet)) {
        throw "Self pair did not read OK; see $selfLog."
    }
    if (-not $Smoke) {
        $poolLog = Join-Path $outDir 'build-pools.txt'
        & $nps -EnginesA $engines[0..1] -EnginesB $engines[2..3] -Label "$label build pools" @bench *>&1 |
            Tee-Object -FilePath $poolLog | Out-Host
        if (-not (Select-String -LiteralPath $poolLog -Pattern 'delta \(median\)' -Quiet)) {
            throw "Build-pool comparison produced no result; see $poolLog."
        }
    }
    Write-Host "A.7.4 finished $(Get-Date -Format s); results in $outDir"
} finally {
    Stop-Transcript | Out-Null
}
