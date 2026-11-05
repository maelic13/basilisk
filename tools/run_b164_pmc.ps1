<# Frozen launcher for PLAN B.16.4 (BAS-P28): speed at an identical tree from
   hardware counters, validated on BAS-P25's patch (+2.35% NPS, the tree
   unchanged at bench 19,470,325).
   Arms: BAS-P25's base and candidate revisions as four-build PGO pools
   (tools/nps_pool.ps1), and a byte-identical copy of the base pool's first
   build as the null. Each run traces `bench 13` pinned to the first game core
   with xperf's PMC-on-CSWITCH (retired instructions, cycles, branch
   mispredictions per thread); tools/diag/pmc_read.py keeps the search
   thread's counts and the trace is deleted. Three cycles, the order of the
   nine binaries reversed every other cycle, after one untraced `bench 8` of
   each. Needs an elevated shell and an idle host (it refuses a busy one);
   about ten minutes.
   -DryRun needs neither: it checks the inputs, makes the copy, runs every
   binary's `bench 13` untraced (its node count must be 19,470,325) and
   parses the B.16.4 probe trace through the same calls. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$xperf = 'C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit\xperf.exe'
$reader = Join-Path $PSScriptRoot 'diag\pmc_read.py'
$engines = Join-Path $PSScriptRoot 'test_engines'
$expectNodes = 19470325
$pools = @(
    @{ Arm = 'p25-base'; Pool = Join-Path $engines 'b164-p25-base.pool.json' }
    @{ Arm = 'p25-cand'; Pool = Join-Path $engines 'b164-p25-cand.pool.json' }
)

if (-not $DryRun) {
    $principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run this from an elevated PowerShell (Run as administrator): the kernel logger needs it.'
    }
}

# The binaries, each checked against its pool's hash.
$items = @()
foreach ($p in $pools) {
    $pool = Get-Content -Raw $p.Pool | ConvertFrom-Json
    foreach ($b in $pool.builds) {
        $path = if ([IO.Path]::IsPathRooted($b.path)) { $b.path } else { Join-Path (Join-Path $PSScriptRoot '..') $b.path }
        if ((Get-HarnessSha256 $path) -ne $b.sha256) { throw "PROVENANCE MISMATCH - $path" }
        $items += @{ Arm = $p.Arm; Build = [IO.Path]::GetFileNameWithoutExtension($path); Path = (Resolve-Path $path).Path; Sha = $b.sha256 }
    }
}
$source = $items[0]
$copyPath = Join-Path $engines ("$($source.Build)-copy.exe")
Copy-Item $source.Path $copyPath -Force
if ((Get-HarnessSha256 $copyPath) -ne $source.Sha) { throw 'the null copy is not byte-identical' }
$items += @{ Arm = 'copy'; Build = "$($source.Build)-copy"; Path = (Resolve-Path $copyPath).Path; Sha = $source.Sha }

$out = Join-Path $PSScriptRoot $(if ($DryRun) { 'results\b164-pmc-dryrun' } else { 'results\b164-pmc' })
if (Test-Path (Join-Path $out 'runs.jsonl')) {
    if ($DryRun) { Remove-Item -Recurse -Force $out } else { throw "$out already holds a read; move it aside first." }
}
New-Item -ItemType Directory -Force $out | Out-Null
@{ label = 'BAS-P28 counters'; contrasts = @(
        @{ name = 'p25'; base = 'p25-base'; cand = 'p25-cand' }
        @{ name = 'null'; base = 'p25-base'; base_build = $source.Build; cand = 'copy' }) } |
    ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $out 'config.json')

$before = Assert-HarnessHostIdle -Allow:$DryRun
$cpu = @(Get-HarnessGameCpus)[0].Cpu
Write-Host "  $($items.Count) binaries, pinned to CPU $cpu"

function Invoke-Bench([string]$Path, [string]$Depth) {
    $info = New-Object System.Diagnostics.ProcessStartInfo $Path, "bench $Depth"
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $proc = [System.Diagnostics.Process]::Start($info)
    $proc.ProcessorAffinity = [IntPtr](1L -shl $cpu)
    $text = $proc.StandardOutput.ReadToEnd()
    $proc.WaitForExit()
    if ($proc.ExitCode) { throw "$Path bench $Depth exited $($proc.ExitCode)" }
    return @{ Text = $text; Pid = $proc.Id }
}

function Add-Run($Item, [int]$Cycle, [string]$Dumper, [int]$EnginePid, [string]$BenchText) {
    $benchFile = Join-Path $out 'bench.txt'
    $BenchText | Set-Content -Encoding utf8 $benchFile
    $json = & python $reader parse $Dumper --pid $EnginePid --bench $benchFile
    if ($LASTEXITCODE) { throw "parsing $($Item.Build) cycle $Cycle failed" }
    $run = $json | ConvertFrom-Json
    $run | Add-Member -NotePropertyName arm -NotePropertyValue $Item.Arm
    $run | Add-Member -NotePropertyName build -NotePropertyValue $Item.Build
    $run | Add-Member -NotePropertyName cycle -NotePropertyValue $Cycle
    $run | Add-Member -NotePropertyName sha256 -NotePropertyValue $Item.Sha
    ($run | ConvertTo-Json -Compress -Depth 4) | Add-Content -Encoding utf8 (Join-Path $out 'runs.jsonl')
}

if ($DryRun) {
    foreach ($item in $items) {
        $r = Invoke-Bench $item.Path '13'
        if ($r.Text -notmatch "Nodes searched\s*:\s*$expectNodes\b") { throw "$($item.Build) does not search $expectNodes nodes" }
        Write-Host "  $($item.Build): $expectNodes nodes"
    }
    $probe = Join-Path $PSScriptRoot 'results\b164-pmc-probe'
    $probePid = (Get-Content -Raw (Join-Path $probe 'run.json') | ConvertFrom-Json).pid
    Add-Run $items[0] 0 (Join-Path $probe 'dumper.csv') $probePid (Get-Content -Raw (Join-Path $probe 'bench.txt'))
    Write-Host "Dry run accepted: inputs, copy, fingerprints and the parse path; $out"
    exit 0
}

foreach ($item in $items) { Invoke-Bench $item.Path '8' | Out-Null }
$order = @()
for ($i = 0; $i -lt 4; $i++) { $order += $items[$i]; $order += $items[$i + 4] }
$order += $items[8]
for ($cycle = 1; $cycle -le 3; $cycle++) {
    $sequence = $order.Clone()
    if ($cycle % 2 -eq 0) { [array]::Reverse($sequence) }
    foreach ($item in $sequence) {
        $etl = Join-Path $out 'run.etl'
        $dumper = Join-Path $out 'dumper.csv'
        & $xperf -stop *> $null
        & $xperf -on PROC_THREAD+CSWITCH -pmc InstructionRetired,TotalCycles,BranchMispredictions CSWITCH -f (Join-Path $out 'raw.etl')
        if ($LASTEXITCODE) { throw "xperf -on failed with exit $LASTEXITCODE" }
        try { $r = Invoke-Bench $item.Path '13' } finally { & $xperf -d $etl *> $null }
        & $xperf -i $etl -o $dumper -a dumper *> $null
        Add-Run $item $cycle $dumper $r.Pid $r.Text
        Remove-Item -Force $etl, $dumper, (Join-Path $out 'raw.etl') -ErrorAction SilentlyContinue
        Write-Host "  cycle $cycle $($item.Build)"
    }
}
$after = Assert-HarnessHostIdle -Allow -Quiet
@{ before_busy_percent = $before.BusyPercent; before_reasons = $before.Reasons
   after_busy_percent = $after.BusyPercent; after_reasons = $after.Reasons; cpu = $cpu } |
    ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $out 'host.json')
& python $reader analyse $out
