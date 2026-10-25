<#
.SYNOPSIS
    Pooled-build NPS read: two pools from tools/nps_pool.ps1, interleaved
    cycles of `bench 13 3`, every run archived, decided by
    tools/diag/nps_read.py.

.DESCRIPTION
    The method is PROCESS *Build, bench and NPS* (Rarog's RAR-P32 read):

      - A cycle reads every build of both arms once. Builds are paired across
        the arms, the pairs rotate from cycle to cycle, and which arm goes
        first alternates, so drift and the first-slot penalty fall on both
        arms alike. A reading is one pinned `bench <Depth> <Repeats> 1`
        process at High priority, and its value is the best of its runs.
      - Step 1 is two cycles. -Extend adds four more to the same archive,
        six in all, when step 1 says EXTEND.
      - One warm-up reading per build precedes step 1 and is archived but not
        counted.
      - Fresh executables are scanned by antivirus, which contaminates a read
        (RAR-M48), so the read first waits until host load has stayed under
        -BusyCeilingPercent for -QuietSeconds. A start-of-read check is not
        enough, so every cycle records the host's load and any engine,
        harness or build process; the decision treats such a cycle as
        disturbed.
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
    ./tools/nps_read.ps1 -Out tools/results/nps-null-head -Extend
#>
param(
    [string]$Base = '',
    [string]$Cand = '',
    [Parameter(Mandatory)][string]$Out,
    [switch]$Extend,
    [switch]$NoRegression,
    [string]$Label = '',
    [ValidateRange(1, 64)][int]$Depth = 13,
    [ValidateRange(1, 16)][int]$Repeats = 3,
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
        pool = [IO.Path]::GetRelativePath($repoRoot, $full).Replace('\', '/')
        pool_sha256 = Get-HarnessSha256 $full
        name = $pool.name; revision = $pool.revision; compiler = $pool.compiler; bench = [long]$pool.bench
        builds = @($pool.builds | ForEach-Object { [ordered]@{ path = $_.path; sha256 = $_.sha256 } })
    }
}

if ($Extend) {
    if (-not (Test-Path -LiteralPath $configPath)) { throw "-Extend needs an existing read in $Out." }
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json -AsHashtable
    $done = @(Get-Content -LiteralPath $runsPath | Where-Object { $_ } | ForEach-Object { $_ | ConvertFrom-Json } |
        Where-Object kind -EQ 'cycle').Count
    if ($done -ne $Step1Cycles) { throw "-Extend continues a $Step1Cycles-cycle step 1; $Out has $done cycles." }
    $firstCycle = $Step1Cycles + 1; $lastCycle = $Step2Cycles
} else {
    if (-not $Base -or -not $Cand) { throw 'A new read needs -Base and -Cand pools.' }
    if (Test-Path -LiteralPath $Out) {
        if (@(Get-ChildItem -LiteralPath $Out -Force).Count) { throw "$Out exists and is not empty; a read never overwrites one." }
    }
    New-Item -ItemType Directory -Force -Path $Out | Out-Null
    $config = [ordered]@{
        schema = 'basilisk-nps-read-v1'
        label = $Label
        mode = if ($NoRegression) { 'no-regression' } else { 'gain' }
        depth = $Depth; repeats = $Repeats
        busy_ceiling_percent = $BusyCeilingPercent
        base = Read-Pool $Base
        cand = Read-Pool $Cand
        repo_revision = (& git -C $repoRoot rev-parse HEAD).Trim()
        created_utc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    }
    $firstCycle = 1; $lastCycle = $Step1Cycles
}

$arms = @('base', 'cand')
$builds = @{}
foreach ($arm in $arms) {
    $builds[$arm] = @(foreach ($b in $config[$arm].builds) {
        $path = Join-Path $repoRoot $b.path
        if (-not (Test-Path -LiteralPath $path)) { throw "$arm build missing: $path" }
        $sha = Get-HarnessSha256 $path
        if ($sha -ne $b.sha256) { throw "PROVENANCE MISMATCH - $path hashes $sha, its pool says $($b.sha256)." }
        [pscustomobject]@{ Path = $path; Sha = $sha }
    })
}
if ($builds.base.Count -ne $builds.cand.Count) {
    throw "Arms have $($builds.base.Count) and $($builds.cand.Count) builds; the interleaving pairs them, so they must match."
}
$pairs = $builds.base.Count

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

function Invoke-Reading([string]$Exe) {
    $psi = [Diagnostics.ProcessStartInfo]::new($Exe)
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.UseShellExecute = $false
    $p = [Diagnostics.Process]::Start($psi)
    try {
        $p.ProcessorAffinity = $mask
        $p.PriorityClass = [Diagnostics.ProcessPriorityClass]::High
        $p.StandardInput.WriteLine("bench $($config.depth) $($config.repeats) 1")
        $p.StandardInput.WriteLine('quit')
        $p.StandardInput.Close()
        $text = $p.StandardOutput.ReadToEnd()
        $p.WaitForExit()
    } finally { $p.Dispose() }
    $runs = [regex]::Matches($text, '(?m)^run\s+\d+/\d+\s+nodes\s+(\d+)\s+time\s+\d+ms\s+nps\s+(\d+)')
    if ($runs.Count -ne $config.repeats) { throw "$Exe printed $($runs.Count) bench runs, expected $($config.repeats)." }
    $nodes = @($runs | ForEach-Object { [long]$_.Groups[1].Value } | Sort-Object -Unique)
    if ($nodes.Count -ne 1) { throw "$Exe searched different node counts across its runs: $($nodes -join ', ')." }
    [pscustomobject]@{ Nodes = $nodes[0]; Nps = @($runs | ForEach-Object { [long]$_.Groups[2].Value }) }
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

Write-Host "NPS read$(if ($config.label) { " [$($config.label)]" }): $($config.mode), cycles $firstCycle-$lastCycle, $pairs builds an arm, CPU $($core[0].Cpu)"
Assert-HarnessHostIdle -MaxBusyPercent 15 -Allow:$AllowBusyHost | Out-Null
Write-Host "  Waiting for $QuietSeconds s of host load under $BusyCeilingPercent% ..."
Wait-HostQuiet

if (-not $Extend) {
    Write-Host '  Warm-up: one reading per build (archived, not counted)'
    foreach ($arm in $arms) {
        for ($i = 0; $i -lt $pairs; $i++) {
            $r = Invoke-Reading $builds[$arm][$i].Path
            Add-Record ([ordered]@{ kind = 'warmup'; arm = $arm; build = $i; sha256 = $builds[$arm][$i].Sha; nodes = $r.Nodes; nps = $r.Nps })
        }
    }
}

for ($cycle = $firstCycle; $cycle -le $lastCycle; $cycle++) {
    $started = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    $busyBefore = Get-HarnessHostBusyPercent
    $procsBefore = @(Get-HarnessBusyProcess)
    $levels = @{ base = @(); cand = @() }
    for ($k = 0; $k -lt $pairs; $k++) {
        $i = ($k + $cycle - 1) % $pairs
        $order = if (($cycle + $k) % 2) { @('cand', 'base') } else { @('base', 'cand') }
        foreach ($arm in $order) {
            $r = Invoke-Reading $builds[$arm][$i].Path
            Add-Record ([ordered]@{ kind = 'reading'; cycle = $cycle; arm = $arm; build = $i; sha256 = $builds[$arm][$i].Sha; nodes = $r.Nodes; nps = $r.Nps })
            $levels[$arm] += ($r.Nps | Measure-Object -Maximum).Maximum
        }
    }
    $busyAfter = Get-HarnessHostBusyPercent
    $procs = @($procsBefore + @(Get-HarnessBusyProcess) | ForEach-Object { $_.ProcessName } | Sort-Object -Unique)
    $busy = @($busyBefore, $busyAfter | Where-Object { $null -ne $_ } | Measure-Object -Maximum).Maximum
    Add-Record ([ordered]@{ kind = 'cycle'; cycle = $cycle; started_utc = $started; busy_percent = $busy; busy_processes = @($procs) })
    Write-Host ("  cycle {0}/{1}: base {2:N0}  cand {3:N0}  host {4:N1}%{5}" -f $cycle, $lastCycle,
        ($levels.base | Measure-Object -Average).Average, ($levels.cand | Measure-Object -Average).Average, $busy,
        $(if ($procs.Count) { "  BUSY: $($procs -join ', ')" } else { '' }))
}

$verdictPath = Join-Path $Out "verdict-$lastCycle.json"
& python (Join-Path $PSScriptRoot 'diag\nps_read.py') --report $Out --json $verdictPath
$code = $LASTEXITCODE
if ($code -ne 0) { throw "nps_read.py could not read $Out (exit $code)." }
Write-Host "Archive: $Out"
