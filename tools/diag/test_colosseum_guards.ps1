<# Prove each A.3.1 guard refuses one deliberately broken input. #>
param(
    [string]$EngineA = "tools/test_engines/basilisk-b12-fix-pext-pgo.exe",
    [string]$EngineB = "tools/test_engines/basilisk-b12-base-pext-pgo.exe",
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
    # Values against the advertised range, on a synthetic option table.
    $ranged = @(
        [pscustomobject]@{ Name = 'Hash'; Type = 'spin'; Min = 1; Max = 1024; Raw = 'option name Hash type spin default 16 min 1 max 1024' }
        [pscustomobject]@{ Name = 'Ponder'; Type = 'check'; Min = $null; Max = $null; Raw = 'option name Ponder type check default false' }
        [pscustomobject]@{ Name = 'Mode'; Type = 'combo'; Min = $null; Max = $null; Raw = 'option name Mode type combo default a var a var b' }
    )
    Invoke-Case 'control: option values in range' '' {
        Assert-AdvertisedOptions $ranged @('Hash=1024', 'hash=1', 'Ponder=false', 'Mode=b', 'Hash') 'synthetic'
        Assert-AdvertisedOptions $options @('Hash=64', 'Threads=1') 'armA'
    }
    Invoke-Case 'spin value above its range' 'Hash=1025 (outside' {
        Assert-AdvertisedOptions $ranged @('Hash=1025') 'synthetic'
    }
    Invoke-Case 'spin value below its range' 'Hash=0 (outside' {
        Assert-AdvertisedOptions $ranged @('Hash=0') 'synthetic'
    }
    Invoke-Case 'check value not boolean' 'not true or false' {
        Assert-AdvertisedOptions $ranged @('Ponder=1') 'synthetic'
    }
    Invoke-Case 'combo value not listed' 'not one of a, b' {
        Assert-AdvertisedOptions $ranged @('Mode=c') 'synthetic'
    }
    Invoke-Case 'busy host threshold' 'HOST NOT IDLE' {
        Assert-HarnessHostIdle -MaxBusyPercent -1 | Out-Null
    }

    # Resolved placements in the shape Colosseum 0.5.0 gives, on a synthetic
    # host whose game cores are the even logical CPUs 2..30 (the 5950X's).
    $hostGameCpus = @(1..15 | ForEach-Object { 2 * $_ })
    function New-Cpus([int[]]$Numbers) { @($Numbers | ForEach-Object { [pscustomobject]@{ group = 0; number = $_ } }) }
    function New-Placement([string]$Mode, [int[][]]$SlotCpus, [int[]]$Listed = @()) {
        $policy = if ($Mode -eq 'auto') { [pscustomobject]@{ mode = 'auto'; headroom_physical_cores = 1 } }
                  else { [pscustomobject]@{ mode = 'explicit'; cpus = (New-Cpus $Listed) } }
        $index = 0
        $slots = foreach ($cpus in $SlotCpus) {
            $side = { [pscustomobject]@{ physical_core_count = 4
                allocation = [pscustomobject]@{ mode = 'enforced'; cpus = (New-Cpus $cpus) } } }
            [pscustomobject]@{ slot_index = $index++; asymmetries = @(); engine_a = (& $side); engine_b = (& $side) }
        }
        [pscustomobject]@{ execution = [pscustomobject]@{
            concurrency = $SlotCpus.Count; placement_policy = $policy
            allocation = [pscustomobject]@{ mode = 'shared'; cores_per_game = 4 }; slots = @($slots) } }
    }
    $autoPlacement = New-Placement 'auto' @(, @(2..9))
    Invoke-Case '4T in an automatic slot' 'SMT siblings' {
        Assert-HarnessResolvedPlacement $autoPlacement 1 4
    }
    Invoke-Case 'control: sibling-free list from the auto plan' '' {
        $plan = New-Placement 'auto' @(@(2..9), @(16..23))
        $list = @(Get-HarnessSiblingFreeCpuList -AutoResolved $plan -GameCpus $hostGameCpus)
        if (($list -join ',') -ne '2,4,6,8,16,18,20,22') { throw "derived list is '$($list -join ',')'" }
    }
    $goodPlacement = New-Placement 'explicit' @(, @(2, 4, 6, 8)) @(2, 4, 6, 8)
    Invoke-Case 'control: resolved 4T sibling-free placement' '' {
        Assert-HarnessResolvedPlacement $goodPlacement 1 4 -Placement 'sibling-free' -GameCpus $hostGameCpus
    }
    $sibling = New-Placement 'explicit' @(, @(2, 3, 4, 6, 8)) @(2, 3, 4, 6, 8)
    Invoke-Case 'SMT sibling in the explicit list' 'outside the game-core set (3)' {
        Assert-HarnessResolvedPlacement $sibling 1 4 -Placement 'sibling-free' -GameCpus $hostGameCpus
    }
    Invoke-Case 'two logical CPUs on one core' 'logical CPUs on 4 physical cores' {
        Assert-HarnessResolvedPlacement $sibling 1 4 -Placement 'sibling-free' -GameCpus @(2, 3, 4, 6, 8, 10)
    }
    Invoke-Case 'explicit list with no free game core' 'no game core free' {
        Assert-HarnessResolvedPlacement $goodPlacement 1 4 -Placement 'sibling-free' -GameCpus @(2, 4, 6, 8)
    }
    Invoke-Case 'auto plan passed as sibling-free' "expected an explicit sibling-free list" {
        Assert-HarnessResolvedPlacement $autoPlacement 1 4 -Placement 'sibling-free' -GameCpus $hostGameCpus
    }
    $badPlacement = $goodPlacement | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $badPlacement.execution.allocation.cores_per_game = 1
    $badPlacement.execution.slots[0].engine_a.physical_core_count = 1
    $badPlacement.execution.slots[0].engine_b.physical_core_count = 1
    Invoke-Case 'underallocated 4T placement' 'cores per game is 1, expected 4' {
        Assert-HarnessResolvedPlacement $badPlacement 1 4 -Placement 'sibling-free' -GameCpus $hostGameCpus
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
    # Gauntlet faults, in the shape 0.4.0's `stats --json` gave for BAS-M10's
    # void run: two setup crashes by Houdini 3 in its 400 games.
    function New-FaultReport([object[]]$Groups, [int]$Infrastructure = 0, [int]$BasiliskTimeLosses = 0) {
        [pscustomobject]@{
            faults = @($Groups)
            time_losses = @(
                [pscustomobject]@{ engine = 'Basilisk'; games = $BasiliskTimeLosses },
                [pscustomobject]@{ engine = 'Houdini 3'; games = 0 })
            infrastructure_faults = $Infrastructure
        }
    }
    $crashes = [pscustomobject]@{ engine = 'Houdini 3'; termination = 'EngineCrash'; kind = 'disconnect'; phase = 'setup'; scored_as = 'loss'; games = 2 }
    $field = @{ 'Houdini 3' = 400 }
    Invoke-Case 'control: opponent crashes under rule' '' {
        $lines = @(Assert-ColosseumGauntletFaults (New-FaultReport @($crashes)) @('Basilisk') $field 1.0 2 0)
        $expected = 'Houdini 3 disconnect in setup, EngineCrash, scored loss: 2'
        if ($lines.Count -ne 1 -or $lines[0] -isnot [string] -or $lines[0] -ne $expected) { throw "accepted lines are '$lines'" }
        $none = @(Assert-ColosseumGauntletFaults (New-FaultReport @()) @('Basilisk') $field 1.0 0 0)
        if ($none.Count -ne 0) { throw "a clean report returned $($none.Count) line(s)" }
    }
    Invoke-Case 'Basilisk crash in a gauntlet' 'BASILISK FAULT' {
        Assert-ColosseumGauntletFaults (New-FaultReport @($crashes)) @('Basilisk', 'Houdini 3') @{} 1.0 2 0 | Out-Null
    }
    Invoke-Case 'Basilisk time loss in a gauntlet' 'lost 1 game(s) on time' {
        Assert-ColosseumGauntletFaults (New-FaultReport @() -BasiliskTimeLosses 1) @('Basilisk') $field 1.0 0 0 | Out-Null
    }
    Invoke-Case 'opponent over the fault rate' 'OPPONENT FAULT RATE' {
        Assert-ColosseumGauntletFaults (New-FaultReport @($crashes)) @('Basilisk') @{ 'Houdini 3' = 100 } 1.0 2 0 | Out-Null
    }
    Invoke-Case 'gauntlet infrastructure fault' 'infrastructure fault' {
        $harness = [pscustomobject]@{ engine = 'harness'; termination = 'Aborted'; kind = 'spawn'; phase = 'setup'; scored_as = 'not scored'; games = 1 }
        Assert-ColosseumGauntletFaults (New-FaultReport @($harness) -Infrastructure 1) @('Basilisk') $field 1.0 0 1 | Out-Null
    }
    Invoke-Case 'fault counters disagree' 'FAULT COUNTERS DISAGREE' {
        Assert-ColosseumGauntletFaults (New-FaultReport @($crashes)) @('Basilisk') $field 1.0 3 0 | Out-Null
    }

    $base = @{
        Mode = 'match'; EngineA = $armA; EngineB = $armB; NameA = 'A'; NameB = 'B'
        Games = 2; Seed = 7; ExpectBench = @($benchA, $benchB)
        Dir = (Join-Path $scratch 'dry-run'); DryRun = $true; AllowBusyHost = $true
    }
    Invoke-Case 'control: wrapper dry run' '' { & $wrapper @base | Out-Null }
    # Self-relative scaling: one binary at four threads against itself at
    # one, and at twice the clock; both resolve sibling-free.
    $selfThreads = $base.Clone()
    $selfThreads.EngineB = $armA; $selfThreads.ExpectBench = @($benchA, $benchA)
    $selfThreads.ThreadsB = 4
    $selfThreads.Dir = Join-Path $scratch 'self-threads'
    Invoke-Case 'control: 4T against 1T, sibling-free' '' {
        & $wrapper @selfThreads | Out-Null
        $manifest = Get-ChildItem (Join-Path $repo 'tools\results') -Filter 'colosseum_match_self-threads_*.manifest.txt' |
            Sort-Object LastWriteTime | Select-Object -Last 1
        $text = Get-Content -LiteralPath $manifest.FullName -Raw
        if ($text -notmatch 'threads:\s+A 1, B 4' -or $text -notmatch 'placement:\s+sibling-free') { throw "manifest does not record it: $($manifest.Name)" }
    }
    $selfClock = $base.Clone()
    $selfClock.EngineB = $armA; $selfClock.ExpectBench = @($benchA, $benchA)
    $selfClock.BaseMsB = 6000; $selfClock.IncrementMsB = 60
    $selfClock.Dir = Join-Path $scratch 'self-clock'
    Invoke-Case 'control: time odds' '' { & $wrapper @selfClock | Out-Null }
    $autoThreads = $selfThreads.Clone()
    $autoThreads.Placement = 'auto'; $autoThreads.Dir = Join-Path $scratch 'auto-threads'
    Invoke-Case 'auto placement at four threads' 'packs them on SMT siblings' { & $wrapper @autoThreads | Out-Null }
    $nullOdds = $base.Clone()
    $nullOdds.Mode = 'calibrate'; $nullOdds.EngineB = $armA; $nullOdds.ThreadsB = 4
    $nullOdds.Dir = Join-Path $scratch 'null-odds'
    Invoke-Case 'null pair with thread odds' 'refuses -ThreadsB' { & $wrapper @nullOdds | Out-Null }
    $sameArms = $base.Clone()
    $sameArms.EngineB = $armA; $sameArms.ExpectBench = @($benchA, $benchA)
    $sameArms.Dir = Join-Path $scratch 'same-arms'
    Invoke-Case 'identical arms outside calibrate' 'require -Mode calibrate' { & $wrapper @sameArms | Out-Null }
    $badPolicy = $base.Clone()
    $badPolicy.RunFile = Join-Path $repo 'tools\colosseum\match-fixed-ltc.toml'
    $badPolicy.Dir = Join-Path $scratch 'bad-policy'
    Invoke-Case 'resolved policy mismatch' 'not Basilisk policy' { & $wrapper @badPolicy | Out-Null }
    # Every run keeps its engine processes; a command's default changed between
    # runner versions (a tournament started fresh ones before 0.3.0).
    $freshRunFile = Join-Path $scratch 'match-fresh-processes.toml'
    $fixedRunFile = (Join-Path $repo 'tools\colosseum\match-fixed.toml').Replace('\', '/')
    @("extend = ""$fixedRunFile""", '', '[options]', 'engine-processes = "per-game"') |
        Set-Content -LiteralPath $freshRunFile -Encoding utf8
    $freshMatch = $base.Clone()
    $freshMatch.RunFile = $freshRunFile
    $freshMatch.Dir = Join-Path $scratch 'fresh-match'
    Invoke-Case 'match with fresh engine processes' "engine processes are 'per-game'" { & $wrapper @freshMatch | Out-Null }

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
    $gauntlet4 = $gauntlet.Clone()
    $gauntlet4.Threads = 4; $gauntlet4.Dir = Join-Path $scratch 'gauntlet-4t'
    Invoke-Case 'control: four-thread gauntlet, sibling-free' '' { & $wrapper @gauntlet4 | Out-Null }
    $wrongGauntletHash = $gauntlet.Clone()
    $wrongGauntletHash.ExpectSha256 = @($gauntlet.ExpectSha256[0], ('0' * 64))
    $wrongGauntletHash.Dir = Join-Path $scratch 'gauntlet-bad-hash'
    Invoke-Case 'external gauntlet hash mismatch' 'PROVENANCE MISMATCH' { & $wrapper @wrongGauntletHash | Out-Null }
    $freshGauntlet = $gauntlet.Clone()
    $freshGauntlet.ExtraArgs = @($gauntletExtra + @('--engine-processes', 'per-game'))
    $freshGauntlet.Dir = Join-Path $scratch 'fresh-gauntlet'
    Invoke-Case 'gauntlet with fresh engine processes' "engine processes are 'per-game'" { & $wrapper @freshGauntlet | Out-Null }
    # 100 cycles give 200 games against the external engine: a cap of 2 at 1%.
    function Set-GauntletArgs([string]$Cycles, [string]$MaxFaults) {
        $copy = @($gauntletExtra)
        $copy[[array]::IndexOf($copy, '--cycles') + 1] = $Cycles
        $copy[[array]::IndexOf($copy, '--max-engine-faults') + 1] = $MaxFaults
        $copy
    }
    $capped = $gauntlet.Clone()
    $capped.ExtraArgs = Set-GauntletArgs '100' '2'
    $capped.Dir = Join-Path $scratch 'gauntlet-capped'
    Invoke-Case 'control: foreign fault cap at the rate' '' { & $wrapper @capped | Out-Null }
    $strict = $gauntlet.Clone()
    $strict.ExtraArgs = Set-GauntletArgs '100' '0'
    $strict.Dir = Join-Path $scratch 'gauntlet-strict'
    Invoke-Case 'foreign field with no fault cap' 'engine faults, expected 2' { & $wrapper @strict | Out-Null }
    $loose = $gauntlet.Clone()
    $loose.ExtraArgs = Set-GauntletArgs '1' '1'
    $loose.Dir = Join-Path $scratch 'gauntlet-loose'
    Invoke-Case 'fault cap beyond the rate' 'engine faults, expected 0' { & $wrapper @loose | Out-Null }
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
