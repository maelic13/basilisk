<#
.SYNOPSIS
    Passive one-second host sampler for a multi-thread read (Windows).

.DESCRIPTION
    Runs typeperf beside a match and writes CSVs to -OutDir; it changes
    nothing the engines see (about 0.1% of the machine in Rarog's smoke).
    Read the result with `python tools/diag/smp_read.py placement`.

      topology.json    each physical core's logical CPUs, from the kernel
      cpu-NNN.csv      \Processor Information(*)\ % Processor Time, Parking
                       Status, % of Maximum Frequency, Processor Frequency,
                       for the whole run (a new segment only if typeperf dies)
      threads-NNN.csv  \Thread(<pattern>)\ % Processor Time, ID Process and
                       ID Thread for every -EnginePatterns entry

    typeperf expands an instance wildcard once, when it starts, and silently
    drops a pattern with no instance, so the thread collector restarts into a
    new segment whenever the set of engine threads changes; Colosseum keeps
    its engine processes per slot, so that happens at a run's start and end,
    not every game. Every start, stop and change is logged in sampler.log.
    The sampler exits when -StopFile appears or -ParentPid is gone.

    pwsh -NoProfile -File tools/host_sampler.ps1 -OutDir <dir> [-EnginePatterns basilisk*,stockfish*] [-ParentPid <pid>] [-StopFile <path>]
#>
param(
    [Parameter(Mandatory)][string]$OutDir,
    [string[]]$EnginePatterns = @('basilisk*'),
    [int]$ParentPid = 0,
    [string]$StopFile = '',
    [int]$PollSeconds = 2
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$EnginePatterns = @($EnginePatterns | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$log = Join-Path $OutDir 'sampler.log'
function Write-Note([string]$Text) { Add-Content -LiteralPath $log -Value "$(Get-Date -Format o)  $Text" }

$cores = @(Get-HarnessPhysicalCpus | Sort-Object Cpu | ForEach-Object { , @($_.Logical) })
@{ cores = $cores } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutDir 'topology.json') -Encoding utf8

$cpuCounters = @(
    '\Processor Information(*)\% Processor Time',
    '\Processor Information(*)\Parking Status',
    '\Processor Information(*)\% of Maximum Frequency',
    '\Processor Information(*)\Processor Frequency'
)
$threadCounters = foreach ($pattern in $EnginePatterns) {
    foreach ($counter in '% Processor Time', 'ID Process', 'ID Thread') { "\Thread($pattern)\$counter" }
}
$cpuList = Join-Path $OutDir 'cpu.counters.txt'
$threadList = Join-Path $OutDir 'threads.counters.txt'
Set-Content -LiteralPath $cpuList -Value $cpuCounters
Set-Content -LiteralPath $threadList -Value $threadCounters

function Start-Collector([string]$CounterFile, [string]$Csv) {
    # -cf reads the counter paths from a file, so no path is re-quoted.
    Start-Process -FilePath 'typeperf.exe' -WindowStyle Hidden -PassThru -ArgumentList @(
        '-cf', "`"$CounterFile`"", '-si', '1', '-f', 'CSV', '-o', "`"$Csv`"", '-y')
}
function Stop-Collector($Process) {
    if ($Process -and -not $Process.HasExited) { Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue }
}
function Get-EngineThreadKey {
    $parts = foreach ($p in @(Get-Process -Name $EnginePatterns -ErrorAction SilentlyContinue)) {
        try { foreach ($t in $p.Threads) { "$($p.Id):$($t.Id)" } } catch { "$($p.Id):?" }
    }
    (@($parts) | Sort-Object) -join ','
}

$cpuSegment = 0; $threadSegment = 0
$cpu = $null; $threads = $null; $threadKey = ''
Write-Note "sampler started, pid $PID, parent $ParentPid, patterns $($EnginePatterns -join ' '), stop file '$StopFile', poll $PollSeconds s"
try {
    while ($true) {
        if ($StopFile -and (Test-Path -LiteralPath $StopFile)) { Write-Note 'stop file seen'; break }
        if ($ParentPid -gt 0 -and -not (Get-Process -Id $ParentPid -ErrorAction SilentlyContinue)) {
            Write-Note "parent $ParentPid gone"; break
        }
        if (-not $cpu -or $cpu.HasExited) {
            if ($cpu) { Write-Note "cpu collector $($cpu.Id) exited $($cpu.ExitCode); restarting" }
            $cpuSegment++
            $csv = Join-Path $OutDir ('cpu-{0:D3}.csv' -f $cpuSegment)
            $cpu = Start-Collector $cpuList $csv
            Write-Note "cpu collector $($cpu.Id) -> $csv"
        }
        $key = Get-EngineThreadKey
        if ($key -ne $threadKey -or ($threads -and $threads.HasExited -and $key)) {
            Stop-Collector $threads
            $threads = $null
            $processes = @($key -split ',' | Where-Object { $_ } | ForEach-Object { ($_ -split ':')[0] } | Sort-Object -Unique)
            $count = @($key -split ',' | Where-Object { $_ }).Count
            if ($key) {
                $threadSegment++
                $csv = Join-Path $OutDir ('threads-{0:D3}.csv' -f $threadSegment)
                $threads = Start-Collector $threadList $csv
                Write-Note "engine threads changed: $count threads in processes $($processes -join ' '); thread collector $($threads.Id) -> $csv"
            } else {
                Write-Note 'engine threads changed: none running; thread collector stopped'
            }
            $threadKey = $key
        }
        Start-Sleep -Seconds $PollSeconds
    }
} finally {
    Stop-Collector $threads
    Stop-Collector $cpu
    Write-Note "sampler stopped ($cpuSegment cpu segments, $threadSegment thread segments)"
}
