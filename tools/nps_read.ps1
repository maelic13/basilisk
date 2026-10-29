<#
.SYNOPSIS
    Pooled-build NPS read: two or more pools from tools/nps_pool.ps1,
    interleaved cycles of `bench 13 3`, every run archived, each contrast
    between two pools decided by tools/diag/nps_read.py.

.DESCRIPTION
    The method is PROCESS *Build, bench and NPS* (Rarog's RAR-P32 read):

      - The arms are pools, named by their pool names. A contrast is a
        named pair of arms, read in the gain or the no-regression form;
        -Base and -Cand are the two-arm read with one contrast, `read`.
      - A cycle reads every build of every arm once. Builds are grouped by
        their index across the arms, the groups rotate from cycle to cycle,
        and the arm order rotates within each group, so drift and the
        first-slot penalty fall on every arm alike. A reading is one pinned
        `bench <Depth> <Repeats> 1` process at High priority, and its value
        is the best of its runs. Every contrast is as precise as a two-arm
        read of its pair, and the arms share one warm-up and one quiet wait.
      - Step 1 is two cycles. -Extend adds four more to the same archive,
        six in all, when step 1 says the read extends (any contrast EXTEND).
      - Fresh executables are scanned by antivirus, which contaminates a read
        (RAR-M48). So every build first runs one short warm-up bench
        (`bench <WarmupDepth> 1 1`, archived, not counted), which sets off
        the scan, and the read then waits until host load has stayed under
        -BusyCeilingPercent for -QuietSeconds; a scan still running holds
        the wait. A start-of-read check is not enough, so every cycle records
        the host's load and any engine, harness or build process; the
        decision treats such a cycle as disturbed.
      - Every binary is hashed against its pool before the first reading, and
        every reading records the hash it ran.

    The archive is <Out>/read.json plus <Out>/runs.jsonl; the decision is
    recomputed from it at any time with
    `python tools/diag/nps_read.py --report <Out>`.

.EXAMPLE
    ./tools/nps_read.ps1 -Base tools/test_engines/head-a.pool.json `
        -Cand tools/test_engines/head-b.pool.json `
        -Out tools/results/nps-null-head -Label "head null"

.EXAMPLE
    ./tools/nps_read.ps1 -Pools tools/test_engines/old.pool.json, `
        tools/test_engines/new.pool.json, tools/test_engines/alt.pool.json `
        -Contrasts 'new=old:new:no-regression', 'alt=old:alt:gain' `
        -Out tools/results/nps-three -Label "three arms"

.EXAMPLE
    ./tools/nps_read.ps1 -Out tools/results/nps-null-head -Extend
#>
param(
    [string]$Base = '',
    [string]$Cand = '',
    [string[]]$Pools = @(),
    [string[]]$Contrasts = @(),
    [Parameter(Mandatory)][string]$Out,
    [switch]$Extend,
    [switch]$NoRegression,
    [string]$Label = '',
    [ValidateRange(1, 64)][int]$Depth = 13,
    [ValidateRange(1, 16)][int]$Repeats = 3,
    [ValidateRange(1, 64)][int]$WarmupDepth = 8,
    [int]$Cpu = -1,
    [double]$BusyCeilingPercent = 10,
    [ValidateRange(0, 3600)][int]$QuietSeconds = 60,
    [ValidateRange(1, 240)][int]$MaxQuietMinutes = 30,
    [switch]$AllowBusyHost,
    [switch]$AllowSmallPool
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Out = [IO.Path]::GetFullPath($Out, (Get-Location).Path)
$configPath = Join-Path $Out 'read.json'
$runsPath = Join-Path $Out 'runs.jsonl'
$Step1Cycles = 2
$Step2Cycles = 6

function Read-Pool([string]$Path) {
    $full = (Resolve-Path -LiteralPath $Path).Path
    $pool = Get-Content -LiteralPath $full -Raw | ConvertFrom-Json
    if ($pool.schema -ne 'basilisk-nps-pool-v1') { throw "$full is not an nps_pool.ps1 pool." }
    if ($pool.small_pool -and -not $AllowSmallPool) { throw "$full is a small pool; it reads only with -AllowSmallPool, as an instrument smoke." }
    [ordered]@{
        name = $pool.name
        pool = [IO.Path]::GetRelativePath($repoRoot, $full).Replace('\', '/')
        pool_sha256 = Get-HarnessSha256 $full
        revision = $pool.revision; compiler = $pool.compiler; bench = [long]$pool.bench
        builds = @($pool.builds | ForEach-Object { [ordered]@{ path = $_.path; sha256 = $_.sha256 } })
    }
}

function Read-Contrast([string]$Spec, [string[]]$ArmNames) {
    if ($Spec -notmatch '^([A-Za-z0-9_.-]+)=([^:]+):([^:]+)(?::(gain|no-regression))?$') {
        throw "Contrast '$Spec' is not name=base:cand[:gain|:no-regression]."
    }
    $c = [ordered]@{ name = $Matches[1]; base = $Matches[2]; cand = $Matches[3]; mode = if ($Matches[4]) { $Matches[4] } else { 'gain' } }
    foreach ($arm in @($c.base, $c.cand)) {
        if ($arm -notin $ArmNames) { throw "Contrast $($c.name) names $arm, which is not one of the arms: $($ArmNames -join ', ')." }
    }
    if ($c.base -eq $c.cand) { throw "Contrast $($c.name) reads $($c.base) against itself." }
    $c
}

if ($Extend) {
    if (-not (Test-Path -LiteralPath $configPath)) { throw "-Extend needs an existing read in $Out." }
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json -AsHashtable
    if ($config.schema -ne 'basilisk-nps-read-v2') { throw "$Out is a $($config.schema) read; extend it with the nps_read.ps1 that started it." }
    $done = @(Get-Content -LiteralPath $runsPath | Where-Object { $_ } | ForEach-Object { $_ | ConvertFrom-Json } |
        Where-Object kind -EQ 'cycle').Count
    if ($done -ne $Step1Cycles) { throw "-Extend continues a $Step1Cycles-cycle step 1; $Out has $done cycles." }
    $step1 = Get-Content -LiteralPath (Join-Path $Out "verdict-$Step1Cycles.json") -Raw | ConvertFrom-Json
    if ($step1.next -ne 'EXTEND') { throw "Step 1 says $($step1.next), not EXTEND; this read does not extend." }
    $firstCycle = $Step1Cycles + 1; $lastCycle = $Step2Cycles
} else {
    if ($Base -or $Cand) {
        if (-not $Base -or -not $Cand -or $Pools.Count -or $Contrasts.Count) { throw 'Give -Base and -Cand together, or -Pools and -Contrasts.' }
        $Pools = @($Base, $Cand)
    } elseif ($Pools.Count -lt 2 -or -not $Contrasts.Count) {
        throw 'A new read needs -Base and -Cand pools, or two or more -Pools and one or more -Contrasts.'
    } elseif ($NoRegression) {
        throw '-NoRegression is for a -Base/-Cand read; a contrast states its own mode.'
    }
    if (Test-Path -LiteralPath $Out) {
        if (@(Get-ChildItem -LiteralPath $Out -Force).Count) { throw "$Out exists and is not empty; a read never overwrites one." }
    }
    $armSpecs = @($Pools | ForEach-Object { Read-Pool $_ })
    $names = @($armSpecs | ForEach-Object { $_.name })
    if (@($names | Sort-Object -Unique).Count -ne $names.Count) { throw "Two pools share a name: $($names -join ', ')." }
    $contrastSpecs = @(if ($Base) {
        [ordered]@{ name = 'read'; base = $names[0]; cand = $names[1]; mode = if ($NoRegression) { 'no-regression' } else { 'gain' } }
    } else {
        $Contrasts | ForEach-Object { Read-Contrast $_ $names }
    })
    $contrastNames = @($contrastSpecs | ForEach-Object { $_.name })
    if (@($contrastNames | Sort-Object -Unique).Count -ne $contrastNames.Count) { throw "Two contrasts share a name: $($contrastNames -join ', ')." }
    New-Item -ItemType Directory -Force -Path $Out | Out-Null
    $config = [ordered]@{
        schema = 'basilisk-nps-read-v2'
        label = $Label
        depth = $Depth; repeats = $Repeats; warmup_depth = $WarmupDepth
        busy_ceiling_percent = $BusyCeilingPercent; quiet_seconds = $QuietSeconds
        arms = @($armSpecs)
        contrasts = @($contrastSpecs)
        repo_revision = (& git -C $repoRoot rev-parse HEAD).Trim()
        created_utc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    }
    $firstCycle = 1; $lastCycle = $Step1Cycles
}

$arms = @($config.arms | ForEach-Object { $_.name })
$builds = @{}
foreach ($spec in $config.arms) {
    $builds[$spec.name] = @(foreach ($b in $spec.builds) {
        $path = Join-Path $repoRoot $b.path
        if (-not (Test-Path -LiteralPath $path)) { throw "$($spec.name) build missing: $path" }
        $sha = Get-HarnessSha256 $path
        if ($sha -ne $b.sha256) { throw "PROVENANCE MISMATCH - $path hashes $sha, its pool says $($b.sha256)." }
        [pscustomobject]@{ Path = $path; Sha = $sha }
    })
}
$pairs = $builds[$arms[0]].Count
foreach ($arm in $arms) {
    if ($builds[$arm].Count -ne $pairs) {
        throw "Arms have $(($arms | ForEach-Object { $builds[$_].Count }) -join ', ') builds; the interleaving groups them, so they must match."
    }
}

# One physical core, the last by default: Windows services interrupts on CPU 0,
# and pinning concentrates whatever interference the chosen core sees.
$cores = @(Get-HarnessPhysicalCpus)
if ($Extend) {
    if ($Cpu -ge 0 -and $Cpu -ne [int]$config.cpu) { throw "Step 1 read on CPU $($config.cpu); -Extend stays on it." }
    $Cpu = [int]$config.cpu
}
$core = if ($Cpu -ge 0) { @($cores | Where-Object Cpu -EQ $Cpu) } else { @($cores[-1]) }
if ($core.Count -ne 1) { throw "-Cpu $Cpu is not one of the physical CPUs: $($cores.Cpu -join ', ')." }
if ($core[0].Cpu -ge 63) { throw 'Processor-group affinity above CPU 62 is not supported by this harness.' }
$mask = [IntPtr]([int64]1 -shl $core[0].Cpu)
if (-not $Extend) { $config.cpu = $core[0].Cpu; $config | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $configPath -Encoding utf8 }

function Invoke-Reading([string]$Exe, [int]$BenchDepth, [int]$Runs) {
    $psi = [Diagnostics.ProcessStartInfo]::new($Exe)
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.UseShellExecute = $false
    $p = [Diagnostics.Process]::Start($psi)
    try {
        $p.ProcessorAffinity = $mask
        $p.PriorityClass = [Diagnostics.ProcessPriorityClass]::High
        $p.StandardInput.WriteLine("bench $BenchDepth $Runs 1")
        $p.StandardInput.WriteLine('quit')
        $p.StandardInput.Close()
        $text = $p.StandardOutput.ReadToEnd()
        $p.WaitForExit()
    } finally { $p.Dispose() }
    # bench prints a line per run only when it makes more than one.
    $pattern = if ($Runs -eq 1) { '(?ms)^Nodes searched\s*:\s*(\d+).*?^Nodes/second\s*:\s*(\d+)' } else { '(?m)^run\s+\d+/\d+\s+nodes\s+(\d+)\s+time\s+\d+ms\s+nps\s+(\d+)' }
    $found = [regex]::Matches($text, $pattern)
    if ($found.Count -ne $Runs) { throw "$Exe printed $($found.Count) bench runs, expected $Runs." }
    $nodes = @($found | ForEach-Object { [long]$_.Groups[1].Value } | Sort-Object -Unique)
    if ($nodes.Count -ne 1) { throw "$Exe searched different node counts across its runs: $($nodes -join ', ')." }
    [pscustomobject]@{ Nodes = $nodes[0]; Nps = @($found | ForEach-Object { [long]$_.Groups[2].Value }) }
}

function Add-Record($Record) {
    Add-Content -LiteralPath $runsPath -Value ($Record | ConvertTo-Json -Compress -Depth 4) -Encoding utf8
}

function Wait-HostQuiet {
    $deadline = (Get-Date).AddMinutes($MaxQuietMinutes)
    $quietSince = $null
    while ($true) {
        $busy = Get-HarnessHostBusyPercent
        $procs = @(Get-HarnessBusyProcess)
        $quiet = $procs.Count -eq 0 -and ($null -eq $busy -or $busy -le $BusyCeilingPercent)
        if ($quiet) {
            if (-not $quietSince) { $quietSince = Get-Date }
            if (((Get-Date) - $quietSince).TotalSeconds -ge $QuietSeconds) { return }
        } else {
            $quietSince = $null
        }
        if ((Get-Date) -gt $deadline) {
            $why = if ($procs.Count) { "processes running: $(($procs.ProcessName | Sort-Object -Unique) -join ', ')" } else { ('host CPU {0:N1}%' -f $busy) }
            if ($AllowBusyHost) { Write-Warning "Host never quiet for $QuietSeconds s ($why); continuing as waived."; return }
            throw "HOST NOT QUIET - $why for $MaxQuietMinutes min; a read needs $QuietSeconds s under $BusyCeilingPercent%."
        }
    }
}

$contrastText = ($config.contrasts | ForEach-Object { "$($_.name) ($($_.mode))" }) -join ', '
Write-Host "NPS read$(if ($config.label) { " [$($config.label)]" }): $($arms.Count) arms of $pairs builds, $contrastText, cycles $firstCycle-$lastCycle, CPU $($core[0].Cpu)"
Assert-HarnessHostIdle -MaxBusyPercent 15 -Allow:$AllowBusyHost | Out-Null

if (-not $Extend) {
    Write-Host "  Warm-up: one bench $($config.warmup_depth) per build (archived, not counted)"
    foreach ($arm in $arms) {
        for ($i = 0; $i -lt $pairs; $i++) {
            $r = Invoke-Reading $builds[$arm][$i].Path $config.warmup_depth 1
            Add-Record ([ordered]@{ kind = 'warmup'; arm = $arm; build = $i; sha256 = $builds[$arm][$i].Sha; depth = $config.warmup_depth; nodes = $r.Nodes; nps = $r.Nps })
        }
    }
}
Write-Host "  Waiting for $QuietSeconds s of host load under $BusyCeilingPercent% ..."
Wait-HostQuiet

for ($cycle = $firstCycle; $cycle -le $lastCycle; $cycle++) {
    $started = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    $busyBefore = Get-HarnessHostBusyPercent
    $procsBefore = @(Get-HarnessBusyProcess)
    $levels = @{}
    foreach ($arm in $arms) { $levels[$arm] = @() }
    for ($k = 0; $k -lt $pairs; $k++) {
        $i = ($k + $cycle - 1) % $pairs
        $first = ($cycle + $k) % $arms.Count
        $order = @(for ($j = 0; $j -lt $arms.Count; $j++) { $arms[($first + $j) % $arms.Count] })
        foreach ($arm in $order) {
            $r = Invoke-Reading $builds[$arm][$i].Path $config.depth $config.repeats
            Add-Record ([ordered]@{ kind = 'reading'; cycle = $cycle; arm = $arm; build = $i; sha256 = $builds[$arm][$i].Sha; nodes = $r.Nodes; nps = $r.Nps })
            $levels[$arm] += ($r.Nps | Measure-Object -Maximum).Maximum
        }
    }
    $busyAfter = Get-HarnessHostBusyPercent
    $procs = @($procsBefore + @(Get-HarnessBusyProcess) | ForEach-Object { $_.ProcessName } | Sort-Object -Unique)
    $busy = @($busyBefore, $busyAfter | Where-Object { $null -ne $_ } | Measure-Object -Maximum).Maximum
    Add-Record ([ordered]@{ kind = 'cycle'; cycle = $cycle; started_utc = $started; busy_percent = $busy; busy_processes = @($procs) })
    $levelText = ($arms | ForEach-Object { '{0} {1:N0}' -f $_, ($levels[$_] | Measure-Object -Average).Average }) -join '  '
    Write-Host ("  cycle {0}/{1}: {2}  host {3:N1}%{4}" -f $cycle, $lastCycle, $levelText, $busy,
        $(if ($procs.Count) { "  BUSY: $($procs -join ', ')" } else { '' }))
}

$verdictPath = Join-Path $Out "verdict-$lastCycle.json"
& python (Join-Path $PSScriptRoot 'diag\nps_read.py') --report $Out --json $verdictPath
$code = $LASTEXITCODE
if ($code -ne 0) { throw "nps_read.py could not read $Out (exit $code)." }
Write-Host "Archive: $Out"
