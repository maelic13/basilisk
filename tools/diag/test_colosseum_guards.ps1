<# Prove each A.3.1 guard refuses one deliberately broken input. #>
param(
    [string]$EngineA = "tools/test_engines/basilisk-tt-coherence-base-pext-pgo.exe",
    [string]$EngineB = "tools/test_engines/basilisk-tt-coherence-candidate-pext-pgo.exe",
    [switch]$KeepScratch
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
. (Join-Path $repo 'harness_common.ps1')
$wrapper = Join-Path $repo 'tools\colosseum.ps1'
$scratch = Join-Path ([IO.Path]::GetTempPath()) ("basilisk-a31-guards-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $scratch | Out-Null
$results = [System.Collections.Generic.List[object]]::new()

function Invoke-Case([string]$Name, [string]$Expect, [scriptblock]$Check) {
    $passed = $false; $detail = ''
    try {
        & $Check
        if (-not $Expect) { $passed = $true; $detail = 'accepted' }
        else { $detail = 'accepted broken input' }
    } catch {
        $message = $_.Exception.Message
        if ($Expect -and $message -like "*$Expect*") { $passed = $true; $detail = "refused: $($message.Split([char]10)[0])" }
        else { $detail = "wrong refusal: $message" }
    }
    $results.Add([pscustomobject]@{ Name = $Name; Passed = $passed; Detail = $detail })
    Write-Host ("  {0}  {1,-34} {2}" -f $(if ($passed) { 'PASS' } else { 'FAIL' }), $Name, $detail) `
        -ForegroundColor $(if ($passed) { 'Green' } else { 'Red' })
}

function Copy-Arm([string]$Source, [string]$Name) {
    $sourcePath = Join-Path $repo $Source
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Required staged engine missing: $sourcePath" }
    $target = Join-Path $scratch "$Name.exe"
    Copy-Item -LiteralPath $sourcePath -Destination $target
    $sourceManifest = [IO.Path]::ChangeExtension($sourcePath, '.manifest.txt')
    $lines = @(Get-Content -LiteralPath $sourceManifest)
    if ($lines -notmatch '^flavor:') { $lines += 'flavor:       pext-pgo' }
    if ($lines -notmatch '^verification:') { $lines += 'verification: bench' }
    $lines | Set-Content -LiteralPath ([IO.Path]::ChangeExtension($target, '.manifest.txt')) -Encoding utf8
    $target
}

function Set-ManifestField([string]$Engine, [string]$Field, [string]$Value) {
    $path = [IO.Path]::ChangeExtension($Engine, '.manifest.txt')
    $lines = @(Get-Content -LiteralPath $path)
    $pattern = "^$([regex]::Escape($Field)):\s*"
    if ($lines -match $pattern) { $lines = @($lines | ForEach-Object { if ($_ -match $pattern) { "${Field}: $Value" } else { $_ } }) }
    else { $lines += "${Field}: $Value" }
    $lines | Set-Content -LiteralPath $path -Encoding utf8
}

try {
    $armA = Copy-Arm $EngineA 'armA'
    $armB = Copy-Arm $EngineB 'armB'
    $benchA = [long](Read-BasiliskBuildManifest ([IO.Path]::ChangeExtension($armA, '.manifest.txt'))).Bench
    $benchB = [long](Read-BasiliskBuildManifest ([IO.Path]::ChangeExtension($armB, '.manifest.txt'))).Bench

    Write-Host 'Guard cases:'
    Invoke-Case 'control: clean provenance' '' {
        Assert-EngineProvenance $armA 'armA' -ExpectBench $benchA | Out-Null
    }
    $stale = Copy-Arm $EngineA 'stale'
    Set-ManifestField $stale 'binary_sha256' ('0' * 64)
    Invoke-Case 'stale binary hash' 'PROVENANCE MISMATCH' {
        Assert-EngineProvenance $stale 'stale' -ExpectBench $benchA | Out-Null
    }
    $dirty = Copy-Arm $EngineA 'dirty'
    Set-ManifestField $dirty 'dirty_diff' 'DEADBEEF'
    Invoke-Case 'dirty build' 'DIRTY TREE' {
        Assert-EngineProvenance $dirty 'dirty' -ExpectBench $benchA | Out-Null
    }
    Invoke-Case 'wrong revision' 'WRONG REVISION' {
        Assert-EngineProvenance $armA 'armA' -ExpectRevision 'deadbee' -ExpectBench $benchA | Out-Null
    }
    Invoke-Case 'wrong bench' 'WRONG FINGERPRINT' {
        Assert-EngineProvenance $armA 'armA' -ExpectBench ($benchA + 1) | Out-Null
    }
    $noFlavor = Copy-Arm $EngineA 'noFlavor'
    Set-ManifestField $noFlavor 'flavor' ''
    Invoke-Case 'missing flavor' "no required 'Flavor'" {
        Assert-EngineProvenance $noFlavor 'noFlavor' -ExpectBench $benchA | Out-Null
    }
    $compiler = Copy-Arm $EngineB 'compiler'
    Set-ManifestField $compiler 'compiler' 'different compiler'
    Invoke-Case 'compiler mismatch' 'COMPILER MISMATCH' {
        Assert-EngineArmEquality (Assert-EngineProvenance $armA 'a' -ExpectBench $benchA) `
            (Assert-EngineProvenance $compiler 'b' -ExpectBench $benchB) 'a' 'b'
    }
    $flavor = Copy-Arm $EngineB 'flavor'
    Set-ManifestField $flavor 'flavor' 'portable-pgo'
    Invoke-Case 'flavor mismatch' 'BUILD FLAVOR MISMATCH' {
        Assert-EngineArmEquality (Assert-EngineProvenance $armA 'a' -ExpectBench $benchA) `
            (Assert-EngineProvenance $flavor 'b' -ExpectBench $benchB) 'a' 'b'
    }
    $umbrella = Copy-Arm $EngineB 'umbrella'
    Set-ManifestField $umbrella 'arm_option' 'DIFFERENT_CLUSTER'
    Invoke-Case 'umbrella option mismatch' 'UMBRELLA OPTION MISMATCH' {
        Assert-EngineArmEquality (Assert-EngineProvenance $armA 'a' -ExpectBench $benchA) `
            (Assert-EngineProvenance $umbrella 'b' -ExpectBench $benchB) 'a' 'b'
    }
    $options = @(Get-EngineUciOptions $armA -Detailed)
    Invoke-Case 'unknown UCI option' 'does not advertise' {
        Assert-AdvertisedOptions $options @('NoSuchOption=1') 'armA'
    }
    Invoke-Case 'busy host threshold' 'HOST NOT IDLE' {
        Assert-HarnessHostIdle -MaxBusyPercent -1 | Out-Null
    }

    $goodPlacement = [pscustomobject]@{
        execution = [pscustomobject]@{
            concurrency = 1
            placement_policy = [pscustomobject]@{ mode = 'auto'; headroom_physical_cores = 1 }
            allocation = [pscustomobject]@{ mode = 'shared'; cores_per_game = 4 }
            slots = @([pscustomobject]@{
                slot_index = 0
                asymmetries = @()
                engine_a = [pscustomobject]@{
                    physical_core_count = 4
                    allocation = [pscustomobject]@{
                        mode = 'enforced'
                        cpus = @(0..7 | ForEach-Object { [pscustomobject]@{ group = 0; number = $_ } })
                    }
                }
                engine_b = [pscustomobject]@{
                    physical_core_count = 4
                    allocation = [pscustomobject]@{
                        mode = 'enforced'
                        cpus = @(0..7 | ForEach-Object { [pscustomobject]@{ group = 0; number = $_ } })
                    }
                }
            })
        }
    }
    Invoke-Case 'control: resolved 4T placement' '' {
        Assert-HarnessResolvedPlacement $goodPlacement 1 4
    }
    $badPlacement = $goodPlacement | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $badPlacement.execution.allocation.cores_per_game = 1
    $badPlacement.execution.slots[0].engine_a.physical_core_count = 1
    $badPlacement.execution.slots[0].engine_b.physical_core_count = 1
    Invoke-Case 'underallocated 4T placement' 'cores per game is 1, expected 4' {
        Assert-HarnessResolvedPlacement $badPlacement 1 4
    }

    $wrongPin = Join-Path $scratch 'wrong-pin.json'
    $pin = Get-Content (Join-Path $repo 'tools\colosseum\colosseum.pin.json') -Raw | ConvertFrom-Json
    $pin.sha256 = 'A' * 64
    $pin | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $wrongPin
    Invoke-Case 'runner pin mismatch' 'COLOSSEUM PIN MISMATCH' {
        Assert-ColosseumCli (Join-Path $repo 'tools\bin\colosseum-cli.exe') $wrongPin | Out-Null
    }

    Invoke-Case 'fault counter unreadable' 'FAULT COUNTERS UNREADABLE' {
        Assert-ColosseumRunFaults ([pscustomobject]@{ durable = [pscustomobject]@{} }) 2 0.5 | Out-Null
    }
    Invoke-Case 'non-time engine fault' 'non-time engine fault' {
        $faults = [pscustomobject]@{ engine_a = 1; engine_b = 0; time_losses_a = 0; time_losses_b = 0; infrastructure = 0 }
        $status = [pscustomobject]@{ durable = [pscustomobject]@{ checkpoint = [pscustomobject]@{ faults = $faults } } }
        Assert-ColosseumRunFaults $status 2 0.5 | Out-Null
    }

    $base = @{
        Mode = 'match'; EngineA = $armA; EngineB = $armB; NameA = 'A'; NameB = 'B'
        Games = 2; Seed = 7; ExpectBench = @($benchA, $benchB)
        Dir = (Join-Path $scratch 'dry-run'); DryRun = $true; AllowBusyHost = $true
    }
    Invoke-Case 'control: wrapper dry run' '' { & $wrapper @base | Out-Null }
    $badPolicy = $base.Clone()
    $badPolicy.RunFile = Join-Path $repo 'tools\colosseum\match-fixed-ltc.toml'
    $badPolicy.Dir = Join-Path $scratch 'bad-policy'
    Invoke-Case 'resolved policy mismatch' 'not Basilisk policy' { & $wrapper @badPolicy | Out-Null }

    $gauntletExtra = @(
        '--format', 'gauntlet', '--seeds', '1', '--cycles', '1', '--games-per-pair', '2', '--max-engine-faults', '0',
        '--engine', $armA, '--engine', $armB,
        '--label', 'Seed', '--label', 'External',
        '--rating', '1500', '--rating', '1500', '--fixed', '2:1500'
    )
    $gauntlet = @{
        Mode = 'gauntlet'; Seed = 8; Hash = 64; Threads = 1
        ExpectBench = @($benchA, -1)
        ExpectSha256 = @((Get-FileHash $armA -Algorithm SHA256).Hash, (Get-FileHash $armB -Algorithm SHA256).Hash)
        ThreadOptionNames = @('Threads', 'Threads'); ExtraArgs = $gauntletExtra
        Dir = (Join-Path $scratch 'gauntlet'); DryRun = $true; AllowBusyHost = $true
    }
    Invoke-Case 'control: external gauntlet provenance' '' { & $wrapper @gauntlet | Out-Null }
    $wrongGauntletHash = $gauntlet.Clone()
    $wrongGauntletHash.ExpectSha256 = @($gauntlet.ExpectSha256[0], ('0' * 64))
    $wrongGauntletHash.Dir = Join-Path $scratch 'gauntlet-bad-hash'
    Invoke-Case 'external gauntlet hash mismatch' 'PROVENANCE MISMATCH' { & $wrapper @wrongGauntletHash | Out-Null }
} finally {
    if (-not $KeepScratch -and (Test-Path -LiteralPath $scratch)) {
        Remove-Item -LiteralPath $scratch -Recurse -Force
    }
}

$failed = @($results | Where-Object { -not $_.Passed })
Write-Host "`n$($results.Count - $failed.Count)/$($results.Count) guard cases behaved."
if ($failed.Count) {
    $failed | ForEach-Object { Write-Host "  FAILED: $($_.Name) - $($_.Detail)" -ForegroundColor Red }
    exit 1
}
