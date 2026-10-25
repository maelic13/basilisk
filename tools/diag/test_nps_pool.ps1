<# Prove each nps_pool.ps1 refusal on one deliberately broken input. #>
param(
    [string]$PoolGlob = "tools/test_engines/basilisk-a820-nps-pgo*-pext-pgo.exe",
    [string]$Foreign = "tools/test_engines/basilisk-a74-nps-pgo1-pext-pgo.exe",
    [long]$Bench = 14978465
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$tool = Join-Path $repo 'tools\nps_pool.ps1'
$scratch = Join-Path ([IO.Path]::GetTempPath()) ("basilisk-nps-pool-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $scratch | Out-Null
$results = [System.Collections.Generic.List[object]]::new()

function Invoke-Case([string]$Name, [string]$Expect, [scriptblock]$Check) {
    $passed = $false; $detail = ''
    try {
        & $Check
        if (-not $Expect) { $passed = $true; $detail = 'accepted' } else { $detail = 'accepted broken input' }
    } catch {
        $message = $_.Exception.Message
        if ($Expect -and $message -like "*$Expect*") { $passed = $true; $detail = "refused: $message" }
        else { $detail = "wrong refusal: $message" }
    }
    $results.Add([pscustomobject]@{ Name = $Name; Passed = $passed; Detail = $detail })
    Write-Host ("  {0}  {1,-30} {2}" -f $(if ($passed) { 'PASS' } else { 'FAIL' }), $Name, $detail) `
        -ForegroundColor $(if ($passed) { 'Green' } else { 'Red' })
}

function Copy-Build([string]$Source, [string]$Name) {
    $target = Join-Path $scratch "basilisk-$Name-pext-pgo.exe"
    Copy-Item -LiteralPath $Source -Destination $target
    Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($Source, '.manifest.txt')) `
        -Destination ([IO.Path]::ChangeExtension($target, '.manifest.txt'))
    $target
}

function Invoke-Pool([string]$Name, [string[]]$Engines, [long]$Expect = $Bench, [switch]$Small) {
    & $tool -Name $Name -ExpectBench $Expect -TestEnginesDir $scratch -Engines $Engines -AllowSmallPool:$Small | Out-Null
}

try {
    $good = @(Get-ChildItem (Join-Path $repo $PoolGlob) | Sort-Object Name | ForEach-Object { Copy-Build $_.FullName $_.BaseName.Replace('basilisk-', '').Replace('-pext-pgo', '') })
    if ($good.Count -lt 4) { throw "The control needs four staged builds matching $PoolGlob." }
    Write-Host 'Pool guard cases:'
    Invoke-Case 'control: four clean builds' '' { Invoke-Pool 'control' $good[0..3] }
    Invoke-Case 'pool name reused' 'Pool already exists' { Invoke-Pool 'control' $good[0..3] }
    Invoke-Case 'wrong fingerprint' 'WRONG FINGERPRINT' { Invoke-Pool 'fingerprint' $good[0..3] -Expect ($Bench + 1) }
    Invoke-Case 'three builds' 'at least 4 builds' { Invoke-Pool 'three' $good[0..2] }
    Invoke-Case 'control: small pool when allowed' '' { Invoke-Pool 'small' $good[0..1] -Small }
    $twin = Copy-Build $good[0] 'twin'
    Invoke-Case 'identical builds' 'identical builds' { Invoke-Pool 'twins' @($good[0..2] + $twin) }
    $foreign = Copy-Build (Join-Path $repo $Foreign) 'foreign'
    Invoke-Case 'mixed revisions' 'Pool mixes revisions' { Invoke-Pool 'mixed' @($good[0..2] + $foreign) -Expect 14978465 }
    $dirty = Copy-Build $good[3] 'dirty'
    $manifest = [IO.Path]::ChangeExtension($dirty, '.manifest.txt')
    (Get-Content -LiteralPath $manifest) -replace '^dirty_diff:.*$', 'dirty_diff: DEADBEEF' | Set-Content -LiteralPath $manifest
    Invoke-Case 'dirty build' 'DIRTY TREE' { Invoke-Pool 'dirty' @($good[0..2] + $dirty) }
    $stale = Copy-Build $good[3] 'stale'
    $manifest = [IO.Path]::ChangeExtension($stale, '.manifest.txt')
    (Get-Content -LiteralPath $manifest) -replace '^binary_sha256:.*$', ('binary_sha256: ' + ('0' * 64)) | Set-Content -LiteralPath $manifest
    Invoke-Case 'stale binary hash' 'PROVENANCE MISMATCH' { Invoke-Pool 'stale' @($good[0..2] + $stale) }
} finally {
    Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
}

$failed = @($results | Where-Object { -not $_.Passed })
Write-Host "`n$($results.Count - $failed.Count)/$($results.Count) pool guard cases behaved."
if ($failed.Count) { exit 1 }
