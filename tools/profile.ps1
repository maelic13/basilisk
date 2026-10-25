<#
.SYNOPSIS
    Sampling profile of a Profile-flavor engine over a frozen cohort,
    attributed to engine regions through full inline chains.

.DESCRIPTION
    No administrator rights are needed: a user-mode sampler suspends each
    engine thread that has run since the last tick, reads its instruction
    pointer, and resumes it, about once a millisecond. Samples are counted
    per address in the engine's image; tools/diag/profile_regions.py
    symbolizes them with llvm-symbolizer's inline chains and charges each to
    the region of its innermost engine frame (tools/diag/profile_regions.json).
    It refuses a profile whose attribution cannot be trusted.

    The engine must be a `build_test.ps1 -Flavor Profile` build: the Release
    surface plus line tables with inline records, whose code is
    byte-identical to Release. Its manifest must be clean, match its hash and
    bench at -ExpectBench, and its engine source must be the checkout's, so a
    stale binary is refused rather than profiled.

    Cohorts:
      bench13   `bench 13 <Repeats> 1`, the fingerprint workload (default)
      -Epd      every line of a frozen EPD file searched to -Nodes, one
                position at a time from `ucinewgame`

.EXAMPLE
    ./tools/build_test.ps1 -Suffix head-prof -Flavor Profile
    ./tools/profile.ps1 -Engine tools/test_engines/basilisk-head-prof-pext-profile-pgo.exe `
        -ExpectBench 8437148 -Out tools/results/profile-head
#>
param(
    [Parameter(Mandatory)][string]$Engine,
    [Parameter(Mandatory)][long]$ExpectBench,
    [Parameter(Mandatory)][string]$Out,
    [string]$Epd = '',
    [long]$Nodes = 1000000,
    [ValidateRange(1, 50)][int]$Repeats = 5,
    [ValidateRange(1, 100)][int]$IntervalMs = 1,
    [int]$Cpu = -1,
    [string]$Label = '',
    [switch]$NoInlines,
    [switch]$AllowOtherSource,
    [switch]$AllowBusyHost
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Engine = (Resolve-Path -LiteralPath $Engine).Path
$Out = [IO.Path]::GetFullPath($Out, (Get-Location).Path)

$manifestPath = [IO.Path]::ChangeExtension($Engine, '.manifest.txt')
if (-not (Test-Path -LiteralPath $manifestPath)) { throw "No build manifest beside $Engine." }
$m = @{}
foreach ($line in Get-Content -LiteralPath $manifestPath) { if ($line -match '^\s*([A-Za-z0-9_]+):\s*(.*)$') { $m[$Matches[1]] = $Matches[2].Trim() } }
$sha = Get-HarnessSha256 $Engine
if ($m['binary_sha256'] -ne $sha) { throw "PROVENANCE MISMATCH - $Engine hashes $sha, its manifest says $($m['binary_sha256'])." }
if ($m['flavor'] -notlike '*-profile') { throw "$Engine is a '$($m['flavor'])' build; profile a build_test.ps1 -Flavor Profile build." }
if ($m['dirty_diff'] -ne 'clean') { throw "DIRTY TREE - $Engine was built from uncommitted changes." }
if ([long]$m['bench'] -ne $ExpectBench) { throw "WRONG FINGERPRINT - $Engine benched $($m['bench']), not $ExpectBench." }
& git -C $repoRoot diff --quiet $m['revision'] HEAD -- src external CMakeLists.txt CMakePresets.json cmake
if ($LASTEXITCODE -ne 0 -and -not $AllowOtherSource) {
    throw "STALE BINARY - $Engine was built at $($m['revision'].Substring(0, 10)), whose engine source differs from HEAD; rebuild it or pass -AllowOtherSource."
}
if ($Epd -and -not (Test-Path -LiteralPath $Epd)) { throw "Cohort file not found: $Epd" }
if (Test-Path -LiteralPath $Out) {
    if (@(Get-ChildItem -LiteralPath $Out -Force).Count) { throw "$Out exists and is not empty." }
}
New-Item -ItemType Directory -Force -Path $Out | Out-Null

if (-not ('BasiliskProfile.Sampler' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;

namespace BasiliskProfile {
    public sealed class Sampler {
        const uint Access = 0x0002 | 0x0008 | 0x0040;  // suspend/resume, get context, query information
        const int ContextSize = 1232, ContextFlagsOffset = 0x30, RipOffset = 0xF8;
        const int ContextControl = 0x00100001;          // CONTEXT_AMD64 | CONTEXT_CONTROL
        const uint StillActive = 259;

        [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr OpenThread(uint access, bool inherit, uint id);
        [DllImport("kernel32.dll", SetLastError = true)] static extern uint SuspendThread(IntPtr h);
        [DllImport("kernel32.dll", SetLastError = true)] static extern uint ResumeThread(IntPtr h);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetThreadContext(IntPtr h, IntPtr context);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool QueryThreadCycleTime(IntPtr h, out ulong cycles);
        [DllImport("kernel32.dll", SetLastError = true)] static extern bool GetExitCodeThread(IntPtr h, out uint code);
        [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);

        readonly int pid, intervalMs;
        readonly long low, high;
        readonly Dictionary<long, int> inModule = new Dictionary<long, int>();
        Thread worker;
        volatile bool stop;

        public long Outside, Ticks, Failed, Idle;
        public int ThreadsSeen;
        public readonly Dictionary<int, long> PerThread = new Dictionary<int, long>();
        public Dictionary<long, int> InModule { get { return inModule; } }

        public Sampler(int pid, long moduleBase, long moduleSize, int intervalMs) {
            this.pid = pid; low = moduleBase; high = moduleBase + moduleSize; this.intervalMs = intervalMs;
        }

        public void Start() {
            worker = new Thread(Loop) { IsBackground = true, Priority = ThreadPriority.Highest };
            worker.Start();
        }

        public void Stop() { stop = true; worker.Join(); }

        void Loop() {
            IntPtr raw = Marshal.AllocHGlobal(ContextSize + 16);
            IntPtr context = new IntPtr((raw.ToInt64() + 15) & ~15L);
            var handles = new Dictionary<int, IntPtr>();
            var lastCycles = new Dictionary<int, ulong>();
            try {
                Process process = Process.GetProcessById(pid);
                var clock = Stopwatch.StartNew();
                long refreshAt = 0;
                long ticksPerInterval = Stopwatch.Frequency * intervalMs / 1000;
                long nextTick = 0;
                var deltas = new Dictionary<int, ulong>();
                while (!stop) {
                    // A spin, not Sleep: Windows ignores a background process's
                    // timer-resolution request, so Sleep(1) lasts a 15.6 ms tick.
                    while (clock.ElapsedTicks < nextTick && !stop) Thread.SpinWait(50);
                    nextTick = clock.ElapsedTicks + ticksPerInterval;
                    if (clock.ElapsedMilliseconds >= refreshAt) {
                        process.Refresh();
                        if (process.HasExited) break;
                        foreach (ProcessThread t in process.Threads) {
                            if (handles.ContainsKey(t.Id)) continue;
                            IntPtr h = OpenThread(Access, false, (uint)t.Id);
                            if (h != IntPtr.Zero) { handles[t.Id] = h; ThreadsSeen++; }
                        }
                        refreshAt = clock.ElapsedMilliseconds + 500;
                    }
                    Ticks++;
                    // Only threads that ran for at least half as long as the busiest
                    // one since the last tick are sampled: a thread that merely woke
                    // up (a poller, a timer) would otherwise be read in ntdll.
                    deltas.Clear();
                    ulong busiest = 0;
                    foreach (var entry in handles) {
                        uint code;
                        if (!GetExitCodeThread(entry.Value, out code) || code != StillActive) continue;
                        ulong cycles;
                        if (!QueryThreadCycleTime(entry.Value, out cycles)) continue;
                        ulong previous;
                        bool seen = lastCycles.TryGetValue(entry.Key, out previous);
                        lastCycles[entry.Key] = cycles;
                        if (!seen) continue;
                        ulong delta = cycles - previous;
                        deltas[entry.Key] = delta;
                        if (delta > busiest) busiest = delta;
                    }
                    if (busiest == 0) { Idle++; continue; }
                    foreach (var d in deltas) {
                        if (d.Value * 2 < busiest) continue;
                        IntPtr thread = handles[d.Key];
                        if (SuspendThread(thread) == 0xFFFFFFFF) { Failed++; continue; }
                        Marshal.WriteInt32(context, ContextFlagsOffset, ContextControl);
                        bool ok = GetThreadContext(thread, context);
                        long rip = ok ? Marshal.ReadInt64(context, RipOffset) : 0;
                        ResumeThread(thread);
                        if (!ok) { Failed++; continue; }
                        long threadCount;
                        PerThread.TryGetValue(d.Key, out threadCount);
                        PerThread[d.Key] = threadCount + 1;
                        if (rip >= low && rip < high) {
                            int n;
                            inModule.TryGetValue(rip - low, out n);
                            inModule[rip - low] = n + 1;
                        } else {
                            Outside++;
                        }
                    }
                }
            } finally {
                foreach (IntPtr h in handles.Values) CloseHandle(h);
                Marshal.FreeHGlobal(raw);
            }
        }
    }
}
'@
}

function Get-PreferredImageBase([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3C)
    if ([BitConverter]::ToUInt16($bytes, $pe + 24) -ne 0x20B) { throw "$Path is not a PE32+ image." }
    [BitConverter]::ToUInt64($bytes, $pe + 24 + 24)
}

Assert-HarnessHostIdle -Allow:$AllowBusyHost | Out-Null
$cores = @(Get-HarnessPhysicalCpus)
$core = if ($Cpu -ge 0) { @($cores | Where-Object Cpu -EQ $Cpu) } else { @($cores[-1]) }
if ($core.Count -ne 1) { throw "-Cpu $Cpu is not one of the physical CPUs: $($cores.Cpu -join ', ')." }

$cohort = if ($Epd) { "epd:$([IO.Path]::GetFileName($Epd)) nodes $Nodes" } else { "bench13 x$Repeats" }
Write-Host "Profile$(if ($Label) { " [$Label]" }): $([IO.Path]::GetFileName($Engine)), cohort $cohort, CPU $($core[0].Cpu), every $IntervalMs ms"

$psi = [Diagnostics.ProcessStartInfo]::new($Engine)
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.UseShellExecute = $false
$proc = [Diagnostics.Process]::Start($psi)
$sampler = $null
try {
    $proc.ProcessorAffinity = [IntPtr]([int64]1 -shl $core[0].Cpu)
    $proc.PriorityClass = [Diagnostics.ProcessPriorityClass]::High
    $module = $null
    for ($try = 0; $try -lt 50 -and -not $module; $try++) {
        try { $proc.Refresh(); $module = $proc.MainModule } catch { Start-Sleep -Milliseconds 20 }
    }
    if (-not $module) { throw 'Could not read the engine process''s main module.' }
    $sampler = [BasiliskProfile.Sampler]::new($proc.Id, $module.BaseAddress.ToInt64(), $module.ModuleMemorySize, $IntervalMs)
    $started = Get-Date
    $sampler.Start()
    $searched = 0L
    if ($Epd) {
        $fens = @(Get-Content -LiteralPath $Epd | Where-Object { $_.Trim() -and -not $_.StartsWith('#') } |
            ForEach-Object { ($_ -split '\s+')[0..3] -join ' ' })
        foreach ($fen in $fens) {
            $proc.StandardInput.WriteLine('ucinewgame')
            $proc.StandardInput.WriteLine("position fen $fen 0 1")
            $proc.StandardInput.WriteLine("go nodes $Nodes")
            while ($true) {
                $line = $proc.StandardOutput.ReadLine()
                if ($null -eq $line) { throw 'The engine exited during the cohort.' }
                if ($line -match '\bnodes (\d+)') { $last = [long]$Matches[1] }
                if ($line.StartsWith('bestmove')) { $searched += $last; break }
            }
        }
        $proc.StandardInput.WriteLine('quit')
        $proc.StandardInput.Close()
        $null = $proc.StandardOutput.ReadToEnd()
    } else {
        $proc.StandardInput.WriteLine("bench 13 $Repeats 1")
        $proc.StandardInput.WriteLine('quit')
        $proc.StandardInput.Close()
        $text = $proc.StandardOutput.ReadToEnd()
        $runs = @([regex]::Matches($text, '(?m)^run\s+\d+/\d+\s+nodes\s+(\d+)') | ForEach-Object { [long]$_.Groups[1].Value })
        if ($Repeats -gt 1 -and ($runs.Count -ne $Repeats -or @($runs | Sort-Object -Unique) -ne $ExpectBench)) {
            throw "bench reported nodes $($runs -join ', '), expected $Repeats runs of $ExpectBench."
        }
        $searched = $ExpectBench * $Repeats
    }
    $proc.WaitForExit()
    $sampler.Stop()
    $elapsed = ((Get-Date) - $started).TotalSeconds
} finally {
    if ($sampler -and -not $proc.HasExited) { $proc.Kill() }
    $proc.Dispose()
}

$inModule = [ordered]@{}
foreach ($entry in ($sampler.InModule.GetEnumerator() | Sort-Object Key)) { $inModule[('0x{0:x}' -f $entry.Key)] = $entry.Value }
$perThread = [ordered]@{}
foreach ($entry in ($sampler.PerThread.GetEnumerator() | Sort-Object Value -Descending)) { $perThread["$($entry.Key)"] = $entry.Value }
$samplesPath = Join-Path $Out 'samples.json'
[ordered]@{
    schema = 'basilisk-profile-samples-v1'
    label = $Label
    cohort = $cohort
    exe = $Engine
    sha256 = $sha
    revision = $m['revision']
    image_base = ('0x{0:x}' -f (Get-PreferredImageBase $Engine))
    interval_ms = $IntervalMs
    cpu = $core[0].Cpu
    duration_seconds = [Math]::Round($elapsed, 2)
    nodes = $searched
    ticks = $sampler.Ticks
    threads_seen = $sampler.ThreadsSeen
    failed = $sampler.Failed
    idle_ticks = $sampler.Idle
    per_thread = $perThread
    outside_module = $sampler.Outside
    in_module = $inModule
    repo_revision = (& git -C $repoRoot rev-parse HEAD).Trim()
    created_utc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $samplesPath -Encoding utf8
$total = ($sampler.InModule.Values | Measure-Object -Sum).Sum + $sampler.Outside
Write-Host ("  {0:N0} samples in {1:N1} s ({2:N0} ticks, {3:N0} idle; {4} threads seen, {5} sampled; {6} failed reads)" -f `
    $total, $elapsed, $sampler.Ticks, $sampler.Idle, $sampler.ThreadsSeen, $perThread.Count, $sampler.Failed)

$pyArgs = @((Join-Path $PSScriptRoot 'diag\profile_regions.py'), '--samples', $samplesPath, '--json', (Join-Path $Out 'report.json'))
if ($NoInlines) { $pyArgs += '--no-inlines' }
& python @pyArgs
$code = $LASTEXITCODE
if ($code -ne 0) { throw "The profile was refused or could not be read (exit $code); samples kept in $samplesPath." }
Write-Host "Profile: $Out"
