<# One-off capture for PLAN B.16.4: what an ETW hardware-counter trace of a
   pinned Basilisk bench looks like on this host, so the counter-read parser is
   written against real output. Needs an elevated shell (the kernel logger),
   takes about a minute, plays no games and needs no idle host.

   It records, at every context switch, the retired instructions, cycles and
   branch mispredictions of the thread switched out (xperf's PMC-on-CSWITCH),
   while `bench 13` of the current master build runs pinned to one game core,
   and saves: the counter sources, the trace, xperf's `pmc` and `dumper`
   outputs, the bench output and the engine's process id. #>
param([int]$Cpu = 0)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\..\harness_common.ps1')
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Run this from an elevated PowerShell (Run as administrator): the kernel logger needs it.'
}
$xperf = 'C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit\xperf.exe'
$engine = Join-Path $PSScriptRoot '..\test_engines\basilisk-b161-master-pext-pgo.exe'
$sha = 'D5E67D0A59EC7DD50630EE81754155A36CDC17647B31C47DC61BC87E6CBDA07A'
if ((Get-HarnessSha256 $engine) -ne $sha) { throw "PROVENANCE MISMATCH - $engine" }
if ($Cpu -le 0) { $Cpu = @(Get-HarnessGameCpus)[0].Cpu }

$out = Join-Path $PSScriptRoot '..\results\b164-pmc-probe'
New-Item -ItemType Directory -Force $out | Out-Null
& $xperf -pmcsources *> (Join-Path $out 'pmcsources.txt')
$etl = Join-Path $out 'kernel.etl'
& $xperf -stop *> $null   # a kernel logger left running from an earlier attempt
& $xperf -on PROC_THREAD+LOADER+CSWITCH -pmc InstructionRetired,TotalCycles,BranchMispredictions CSWITCH -f (Join-Path $out 'raw.etl')
if ($LASTEXITCODE) { throw "xperf -on failed with exit $LASTEXITCODE" }
try {
    $info = New-Object System.Diagnostics.ProcessStartInfo $engine, 'bench 13'
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $proc = [System.Diagnostics.Process]::Start($info)
    $proc.ProcessorAffinity = [IntPtr](1L -shl $Cpu)
    $bench = $proc.StandardOutput.ReadToEnd()
    $proc.WaitForExit()
} finally {
    & $xperf -d $etl
}
$bench | Set-Content -Encoding utf8 (Join-Path $out 'bench.txt')
@{ pid = $proc.Id; cpu = $Cpu; engine_sha256 = $sha; exit_code = $proc.ExitCode } |
    ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $out 'run.json')
& $xperf -i $etl -o (Join-Path $out 'pmc.txt') -a pmc
& $xperf -i $etl -o (Join-Path $out 'dumper.csv') -a dumper
Write-Host "Saved to $out; tell the agent it is there."
