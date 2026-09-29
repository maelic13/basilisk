# Shared preflight for clock-based fastchess harnesses.

$script:MinimumAffinityFastchessVersion = [version]"1.7.0"
$script:HarnessIsWindows = [Environment]::OSVersion.Platform -eq [PlatformID]::Win32NT
# This file lives at the repo root and is dot-sourced from tools\*.ps1, so
# $PSScriptRoot here is always the repo root regardless of the caller.
$script:HarnessRepoRoot = $PSScriptRoot

function Get-HarnessPhysicalCpus {
    if ($script:HarnessIsWindows) {
        if (-not ('BasiliskHarness.CpuTopology' -as [type])) {
            Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.InteropServices;

namespace BasiliskHarness {
    public sealed class CpuCore {
        public int Cpu { get; set; }
        public int EfficiencyClass { get; set; }
    }

    public static class CpuTopology {
        private const int RelationProcessorCore = 0;

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetLogicalProcessorInformationEx(
            int relationship, IntPtr buffer, ref uint returnedLength);

        public static CpuCore[] PhysicalCpus() {
            uint length = 0;
            GetLogicalProcessorInformationEx(RelationProcessorCore, IntPtr.Zero, ref length);
            if (length == 0) throw new Win32Exception(Marshal.GetLastWin32Error());

            IntPtr buffer = Marshal.AllocHGlobal((int)length);
            try {
                if (!GetLogicalProcessorInformationEx(RelationProcessorCore, buffer, ref length))
                    throw new Win32Exception(Marshal.GetLastWin32Error());

                var result = new List<CpuCore>();
                int offset = 0;
                int groupAffinitySize = IntPtr.Size + 8;
                while (offset < length) {
                    IntPtr entry = IntPtr.Add(buffer, offset);
                    int relationship = Marshal.ReadInt32(entry, 0);
                    int size = Marshal.ReadInt32(entry, 4);
                    if (size <= 0 || offset + size > length)
                        throw new InvalidOperationException("Invalid Windows CPU-topology record.");

                    if (relationship == RelationProcessorCore) {
                        int efficiencyClass = Marshal.ReadByte(entry, 9);
                        int groupCount = (ushort)Marshal.ReadInt16(entry, 30);
                        var logical = new List<int>();
                        for (int groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
                            int gaOffset = 32 + groupIndex * groupAffinitySize;
                            ulong mask = IntPtr.Size == 8
                                ? unchecked((ulong)Marshal.ReadInt64(entry, gaOffset))
                                : unchecked((uint)Marshal.ReadInt32(entry, gaOffset));
                            int group = (ushort)Marshal.ReadInt16(entry, gaOffset + IntPtr.Size);
                            for (int bit = 0; bit < IntPtr.Size * 8; ++bit)
                                if ((mask & (1UL << bit)) != 0) logical.Add(group * 64 + bit);
                        }
                        if (logical.Count == 0)
                            throw new InvalidOperationException("A physical core has no logical processors.");
                        result.Add(new CpuCore {
                            Cpu = logical.Min(),
                            EfficiencyClass = efficiencyClass
                        });
                    }
                    offset += size;
                }

                return result
                    .OrderByDescending(c => c.EfficiencyClass)
                    .ThenBy(c => c.Cpu)
                    .ToArray();
            } finally {
                Marshal.FreeHGlobal(buffer);
            }
        }
    }
}
'@
        }
        return [BasiliskHarness.CpuTopology]::PhysicalCpus()
    }

    if (Get-Command lscpu -ErrorAction SilentlyContinue) {
        $seen = @{}
        $cores = foreach ($line in (& lscpu '-p=CPU,CORE,SOCKET' 2>$null)) {
            if (-not $line -or $line.StartsWith('#')) { continue }
            $cpu, $core, $socket = $line.Split(',')
            $key = "$socket,$core"
            if (-not $seen.ContainsKey($key)) {
                $seen[$key] = $true
                [pscustomobject]@{ Cpu = [int]$cpu; EfficiencyClass = 0 }
            }
        }
        return @($cores | Sort-Object Cpu)
    }

    return @(0..([Environment]::ProcessorCount - 1) |
        ForEach-Object { [pscustomobject]@{ Cpu = $_; EfficiencyClass = 0 } })
}

function Get-FastchessVersion {
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "fastchess not found: $Path"
    }

    $line = (& $Path --version 2>&1 | Select-Object -First 1)
    if (-not $line) {
        throw "Could not query fastchess version at '$Path'."
    }

    $match = [regex]::Match("$line", '(?<major>\d+)\.(?<minor>\d+)\.(?<patch>\d+)')
    if (-not $match.Success) {
        throw "Unrecognized fastchess version string: '$line'."
    }

    [pscustomobject]@{
        Text    = "$line".Trim()
        Version = [version]::new(
            [int]$match.Groups['major'].Value,
            [int]$match.Groups['minor'].Value,
            [int]$match.Groups['patch'].Value)
    }
}

function Assert-AffinityFastchess {
    param([Parameter(Mandatory)][string]$Path)

    $info = Get-FastchessVersion -Path $Path
    if ($script:HarnessIsWindows -and $info.Version -lt $script:MinimumAffinityFastchessVersion) {
        throw "fastchess $($info.Version) is too old for reliable Windows affinity. " +
              "Version 1.7.0 contains the process-affinity fix; run tools/setup_tools.ps1 " +
              "to install the pinned runner. Found: $($info.Text)"
    }
    $info
}

function Get-PhysicalCoreCount {
    $count = @(Get-HarnessPhysicalCpus).Count
    if (-not $count -or $count -lt 1) { $count = 1 }
    [int]$count
}

function Get-HarnessGameCpus {
    # Timed games never use CPU 0. Windows services most device interrupts on
    # it, so including it creates a placement-dependent clock offset. On a
    # hybrid CPU, use the highest efficiency class only: Colosseum's automatic
    # placement does the same, and mixing P- and E-cores is not a matched clock.
    $cores = @(Get-HarnessPhysicalCpus)
    if ($cores.Count -le 1) { return $cores }
    $highest = ($cores | Measure-Object EfficiencyClass -Maximum).Maximum
    $preferred = if ($highest -gt 0) { @($cores | Where-Object EfficiencyClass -eq $highest) } else { $cores }
    if ($preferred.Count -le 1) { return $preferred }
    @($preferred | Sort-Object Cpu | Select-Object -Skip 1)
}

function Resolve-HarnessConcurrency {
    <#
        Games in flight, sized so the box is not oversubscribed.

        -ThreadsPerGame (9.2) is the engine `Threads` value each game runs at:
        a game with Threads=4 occupies four cores, not one, so the count that
        must fit in the machine is concurrency x threads. At the default of 1
        this is byte-identical to the pre-9.2 behaviour (physical - 2).

        On 16 physical cores: T1 -> 14, T2 -> 7, T4 -> 3, T8 -> 1.
    #>
    param(
        [int]$Requested,
        [int]$ReservePhysicalCores = 2,
        [int]$ThreadsPerGame = 1,
        [switch]$AllowOversubscribe
    )

    if ($ThreadsPerGame -lt 1) { throw "ThreadsPerGame must be >= 1." }

    $physical = Get-PhysicalCoreCount
    $ceiling = if ($AllowOversubscribe) {
        [Environment]::ProcessorCount
    } else {
        @(Get-HarnessGameCpus).Count
    }
    $budgetBase = if ($AllowOversubscribe) { $ceiling } else { $ceiling }
    # CPU 0 is already absent from the timed ceiling, so a reserve of two means
    # one additional free game core; a reserve of one means only CPU 0.
    $reserveFromCeiling = if ($AllowOversubscribe) { $ReservePhysicalCores } else { [Math]::Max(0, $ReservePhysicalCores - 1) }
    $budget = [Math]::Max(1, $budgetBase - $reserveFromCeiling)
    $recommended = [Math]::Max(1, [Math]::Floor($budget / $ThreadsPerGame))
    $resolved = if ($Requested -gt 0) { $Requested } else { $recommended }

    $coresNeeded = $resolved * $ThreadsPerGame
    if ($coresNeeded -gt $ceiling) {
        $kind = if ($AllowOversubscribe) { "logical processors" } else { "game cores (physical cores except CPU 0)" }
        throw "Concurrency $resolved x Threads $ThreadsPerGame = $coresNeeded exceeds the detected $ceiling $kind."
    }

    [pscustomobject]@{
        Concurrency    = [int]$resolved
        PhysicalCores  = [int]$physical
        ThreadsPerGame = [int]$ThreadsPerGame
        CoresUsed      = [int]$coresNeeded
        AutoSelected   = ($Requested -le 0)
    }
}

function Get-HarnessAffinityCpuList {
    param([Parameter(Mandatory)][int]$Concurrency, [int]$ThreadsPerGame = 1)

    $cores = @(Get-HarnessGameCpus)
    $needed = $Concurrency * $ThreadsPerGame
    if ($needed -gt $cores.Count) {
        throw "Concurrency $Concurrency x Threads $ThreadsPerGame = $needed exceeds $($cores.Count) game cores (physical cores except CPU 0)."
    }
    (($cores | Select-Object -First $needed).Cpu -join ',')
}

function New-HarnessSeed {
    param([int]$Requested)

    if ($Requested -ne 0) { return $Requested }
    Get-Random -Minimum 1 -Maximum ([int]::MaxValue)
}

function Get-HarnessSha256 {
    param([Parameter(Mandatory)][string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-EngineUciOptions {
    param(
        [Parameter(Mandatory)][string]$Path,
        [int]$TimeoutMs = 15000,
        [switch]$Detailed
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Engine not found: $Path" }
    $full = (Resolve-Path -LiteralPath $Path).Path
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $full
    $psi.WorkingDirectory = Split-Path -Parent $full
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $proc = [System.Diagnostics.Process]::Start($psi)
    try {
        $stdout = $proc.StandardOutput.ReadToEndAsync()
        $stderr = $proc.StandardError.ReadToEndAsync()
        $proc.StandardInput.WriteLine("uci")
        $proc.StandardInput.WriteLine("quit")
        $proc.StandardInput.Close()
        if (-not $proc.WaitForExit($TimeoutMs)) {
            throw "Engine '$Path' did not answer 'uci' within ${TimeoutMs} ms."
        }
        $text = $stdout.Result
        $errorText = $stderr.Result
        if ($proc.ExitCode -ne 0) {
            throw "Engine '$Path' exited $($proc.ExitCode) during UCI discovery: $errorText"
        }
    } finally {
        if (-not $proc.HasExited) { $proc.Kill($true) }
        $proc.Dispose()
    }
    if ($text -notmatch '(?m)^\s*uciok\s*$') {
        throw "Engine '$Path' did not emit 'uciok'; it is not a working UCI engine."
    }

    $options = [System.Collections.Generic.List[object]]::new()
    foreach ($line in ($text -split "`r?`n")) {
        $match = [regex]::Match($line, '^\s*option\s+name\s+(?<name>.+?)\s+type\s+(?<type>\S+)(?<tail>.*)$')
        if (-not $match.Success) { continue }
        $tail = $match.Groups['tail'].Value
        $defaultMatch = [regex]::Match($tail, '(?:^|\s)default\s+(?<value>\S+)')
        $minMatch = [regex]::Match($tail, '(?:^|\s)min\s+(?<value>-?\d+)')
        $maxMatch = [regex]::Match($tail, '(?:^|\s)max\s+(?<value>-?\d+)')
        $options.Add([pscustomobject]@{
            Name = $match.Groups['name'].Value.Trim()
            Type = $match.Groups['type'].Value
            Default = if ($defaultMatch.Success) { $defaultMatch.Groups['value'].Value } else { $null }
            Min = if ($minMatch.Success) { [int64]$minMatch.Groups['value'].Value } else { $null }
            Max = if ($maxMatch.Success) { [int64]$maxMatch.Groups['value'].Value } else { $null }
            Raw = $line.Trim()
        })
    }
    if ($Detailed) { $options.ToArray(); return }
    $options.Name
}

function Assert-AdvertisedOptions {
    param([object[]]$Advertised, [string[]]$Wanted, [string]$Label)
    if (-not $Wanted -or @($Wanted).Count -eq 0) { return }
    $normalize = { param($value) ($value -replace '\s+', ' ').Trim().ToLowerInvariant() }
    $have = @($Advertised | ForEach-Object { & $normalize $_.Name })
    $missing = @($Wanted | Where-Object { $_ } |
        ForEach-Object { ($_ -split '=', 2)[0] } |
        Where-Object { $have -notcontains (& $normalize $_) })
    if ($missing.Count -gt 0) {
        throw "$Label does not advertise: $($missing -join ', '). Rebuild it before measuring; the runner would otherwise use defaults."
    }
}

function Read-BasiliskBuildManifest {
    param([Parameter(Mandatory)][string]$Path)
    $fields = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^\s*(?<key>[A-Za-z0-9_]+):\s*(?<value>.*)$') {
            $fields[$Matches.key] = $Matches.value.Trim()
        }
    }
    [pscustomobject]@{
        Path = $Path
        Engine = $fields.engine
        GitSha = $fields.revision
        DirtyDiff = $fields.dirty_diff
        Preset = $fields.preset
        Flavor = $fields.flavor
        ArmOption = $fields.arm_option
        ArmState = $fields.arm_state
        Compiler = $fields.compiler
        Verification = $fields.verification
        Bench = $fields.bench
        BinarySha256 = $fields.binary_sha256
    }
}

function Assert-EngineProvenance {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Label,
        [switch]$AllowDirtyTree,
        [string]$ExpectRevision = "",
        [long]$ExpectBench = -1
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Engine not found: $Path" }
    $manifestPath = [System.IO.Path]::ChangeExtension($Path, ".manifest.txt")
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw "Missing engine manifest: $manifestPath. Rebuild with tools/build_test.ps1."
    }
    $manifest = Read-BasiliskBuildManifest -Path $manifestPath
    foreach ($field in @('GitSha', 'DirtyDiff', 'Flavor', 'Compiler', 'Verification', 'Bench', 'BinarySha256')) {
        if (-not $manifest.$field) { throw "Manifest for $Label has no required '$field' field; rebuild with tools/build_test.ps1." }
    }
    $actual = Get-HarnessSha256 $Path
    if ($actual -ne $manifest.BinarySha256) {
        throw "PROVENANCE MISMATCH - $Label sidecar SHA-256 is $($manifest.BinarySha256), selected binary is $actual."
    }
    if ($manifest.Verification -ne 'bench') {
        throw "Manifest for $Label records '$($manifest.Verification)', not bench verification."
    }
    if ($manifest.DirtyDiff -ne 'clean' -and -not $AllowDirtyTree) {
        throw "DIRTY TREE - $Label was built from uncommitted changes ($($manifest.DirtyDiff)); commit and rebuild."
    }
    if ($manifest.DirtyDiff -ne 'clean') {
        Write-Warning "$Label was built from a dirty tree and -AllowDirtyTree was passed."
    }
    if ($ExpectRevision -and $manifest.GitSha -notlike "$ExpectRevision*") {
        throw "WRONG REVISION - $Label was built at $($manifest.GitSha), not $ExpectRevision."
    }
    if ($ExpectBench -ge 0 -and [long]$manifest.Bench -ne $ExpectBench) {
        throw "WRONG FINGERPRINT - $Label benched $($manifest.Bench), not $ExpectBench."
    }
    $manifest
}

function Assert-EngineArmEquality {
    param(
        [Parameter(Mandatory)][object]$ManifestA,
        [Parameter(Mandatory)][object]$ManifestB,
        [Parameter(Mandatory)][string]$LabelA,
        [Parameter(Mandatory)][string]$LabelB
    )
    if ($ManifestA.Flavor -ne $ManifestB.Flavor) {
        throw "BUILD FLAVOR MISMATCH - ${LabelA}: $($ManifestA.Flavor); ${LabelB}: $($ManifestB.Flavor)."
    }
    if ($ManifestA.ArmOption -ne $ManifestB.ArmOption) {
        throw "UMBRELLA OPTION MISMATCH - ${LabelA}: $($ManifestA.ArmOption); ${LabelB}: $($ManifestB.ArmOption)."
    }
    if ($ManifestA.Compiler -ne $ManifestB.Compiler) {
        throw "COMPILER MISMATCH - ${LabelA}: $($ManifestA.Compiler); ${LabelB}: $($ManifestB.Compiler)."
    }
}

function Get-HarnessBusyProcess {
    $patterns = @('basilisk*', 'rarog*', 'stockfish*', 'fastchess*', 'colosseum-cli*', 'cutechess*', 'clang*', 'cmake*', 'ninja*')
    $self = $PID
    @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
        if ($_.Id -eq $self) { return $false }
        $name = $_.ProcessName.ToLowerInvariant()
        foreach ($pattern in $patterns) { if ($name -like $pattern) { return $true } }
        $false
    })
}

function Get-HarnessHostBusyPercent {
    param([double]$WindowSeconds = 2.0, [double]$SettleSeconds = 1.0)
    if (-not $script:HarnessIsWindows) { return $null }
    $cpus = [Environment]::ProcessorCount
    $read = {
        $raw = Get-CimInstance Win32_PerfRawData_PerfOS_Processor -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -eq '_Total' } | Select-Object -First 1
        if (-not $raw) { return $null }
        [pscustomobject]@{
            Idle = [double]$raw.PercentProcessorTime
            Time = [double]$raw.Timestamp_Sys100NS
            Self = (Get-Process -Id $PID).TotalProcessorTime.TotalSeconds
        }
    }
    Start-Sleep -Milliseconds ([int](1000 * $SettleSeconds))
    $first = & $read
    Start-Sleep -Milliseconds ([int](1000 * $WindowSeconds))
    $second = & $read
    if (-not $first -or -not $second) { return $null }
    $elapsed = $second.Time - $first.Time
    if ($elapsed -le 0) { return $null }
    $busy = 100.0 * (1.0 - ($second.Idle - $first.Idle) / $elapsed)
    $self = 100.0 * ($second.Self - $first.Self) / ($elapsed / 1e7) / $cpus
    [Math]::Max(0.0, $busy - $self)
}

function Assert-HarnessHostIdle {
    param([double]$MaxBusyPercent = 15, [switch]$Allow, [switch]$Quiet)
    $busy = @(Get-HarnessBusyProcess)
    $percent = Get-HarnessHostBusyPercent
    $reasons = @()
    if ($busy.Count -gt 0) {
        $reasons += "engine, harness or build processes are running: " + (($busy | ForEach-Object { "$($_.ProcessName) ($($_.Id))" }) -join ', ')
    }
    if ($null -ne $percent -and $percent -gt $MaxBusyPercent) {
        $reasons += ("host CPU is {0:N0}%, over the {1:N0}% ceiling" -f $percent, $MaxBusyPercent)
    }
    $state = [pscustomobject]@{ BusyPercent = $percent; Reasons = $reasons; Waived = ([bool]$Allow -and $reasons.Count -gt 0) }
    if ($reasons.Count -eq 0) {
        if (-not $Quiet) { Write-Host "  Host idle; no engine, harness or build process." }
        return $state
    }
    if ($Allow) { Write-Warning ("HOST NOT IDLE and waived: " + ($reasons -join '; ')); return $state }
    throw "HOST NOT IDLE - $($reasons -join '; '). Stop the other work before measuring."
}

function Get-ColosseumPin {
    param([Parameter(Mandatory)][string]$PinPath)
    if (-not (Test-Path -LiteralPath $PinPath -PathType Leaf)) { throw "Colosseum pin not found: $PinPath" }
    $pin = Get-Content -LiteralPath $PinPath -Raw | ConvertFrom-Json
    if (-not $pin.revision -or $pin.sha256 -notmatch '^[0-9a-fA-F]{64}$') { throw "Malformed Colosseum pin: $PinPath" }
    $pin
}

function Assert-ColosseumCli {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$PinPath)
    $pin = Get-ColosseumPin -PinPath $PinPath
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Colosseum CLI not found at '$Path'; run tools/setup_tools.ps1."
    }
    $full = (Resolve-Path -LiteralPath $Path).Path
    $sha = Get-HarnessSha256 $full
    if ($sha -ne $pin.sha256) {
        throw "COLOSSEUM PIN MISMATCH - staged $sha; pinned $($pin.sha256) at $($pin.revision)."
    }
    $version = "$(& $full --version 2>&1 | Select-Object -First 1)".Trim()
    if ($pin.version -and $version -ne $pin.version) { throw "Colosseum CLI reports '$version', pin requires '$($pin.version)'." }
    [pscustomobject]@{ Path = $full; Sha256 = $sha; Version = $version; Pin = $pin }
}

function Get-ColosseumRunStatus {
    param([Parameter(Mandatory)][string]$CliPath, [Parameter(Mandatory)][string]$Dir)
    $text = & $CliPath status $Dir --json 2>$null
    if ($LASTEXITCODE -ne 0) { throw "colosseum-cli status failed on $Dir." }
    try { ($text -join "`n") | ConvertFrom-Json } catch { throw "colosseum-cli status did not return JSON for $Dir." }
}

function Resolve-ColosseumExit {
    param([Parameter(Mandatory)][string]$Mode, [Parameter(Mandatory)][int]$ExitCode)
    $outcomes = switch ($Mode) {
        'sprt' { @{ 0 = 'H1 accepted'; 1 = 'H0 accepted'; 4 = 'cap reached, inconclusive' } }
        'calibrate' { @{ 0 = 'pass'; 1 = 'fail'; 4 = 'inconclusive' } }
        default { @{ 0 = 'completed' } }
    }
    $invalid = if ($Mode -in @('sprt', 'calibrate')) { 5 } else { 1 }
    if ($outcomes.ContainsKey($ExitCode)) { return [pscustomobject]@{ Kind = 'outcome'; Verdict = $outcomes[$ExitCode] } }
    $kind = switch ($ExitCode) { $invalid { 'invalid' }; 6 { 'cancelled' }; 2 { 'refused' }; default { 'error' } }
    [pscustomobject]@{ Kind = $kind; Verdict = $null }
}

function Get-ColosseumRunFaults {
    param([Parameter(Mandatory)][object]$Status)
    $checkpoint = $Status.durable.checkpoint
    if ($null -eq $checkpoint) { throw 'FAULT COUNTERS UNREADABLE - run status has no checkpoint.' }
    $faults = $checkpoint.faults
    if ($null -ne $faults) {
        foreach ($field in @('engine_a', 'engine_b', 'time_losses_a', 'time_losses_b', 'infrastructure')) {
            if ($null -eq $faults.$field) { throw "FAULT COUNTERS UNREADABLE - missing '$field'." }
        }
        $time = [int]$faults.time_losses_a + [int]$faults.time_losses_b
        $other = ([int]$faults.engine_a - [int]$faults.time_losses_a) +
                 ([int]$faults.engine_b - [int]$faults.time_losses_b) + [int]$faults.infrastructure
        return [pscustomobject]@{ Split = $true; TimeLosses = $time; Other = $other }
    }
    if ($null -ne $checkpoint.engine_faults) {
        return [pscustomobject]@{ Split = $false; TimeLosses = $null; Other = [int]$checkpoint.engine_faults }
    }
    throw 'FAULT COUNTERS UNREADABLE - no supported fault count is present.'
}

function Assert-ColosseumRunFaults {
    param(
        [Parameter(Mandatory)][object]$Status,
        [Parameter(Mandatory)][int]$ScoredGames,
        [Parameter(Mandatory)][double]$TimeLossRateCeiling,
        [string]$Dir = ''
    )
    $faults = Get-ColosseumRunFaults -Status $Status
    if (-not $faults.Split) {
        if ($faults.Other -gt 0) { throw "Run has $($faults.Other) engine fault(s); see $Dir." }
        return $faults
    }
    if ($faults.Other -gt 0) { throw "Run has $($faults.Other) non-time engine fault(s); see $Dir." }
    if ($faults.TimeLosses -gt 0) {
        if ($ScoredGames -le 0) { throw "Run has time losses but no scored game; see $Dir." }
        $rate = 100.0 * $faults.TimeLosses / $ScoredGames
        if ($rate -gt $TimeLossRateCeiling) {
            throw ("Time-loss rate {0:N3}% exceeds the {1}% ceiling; see {2}." -f $rate, $TimeLossRateCeiling, $Dir)
        }
    }
    $faults
}

# ── weather-factory overlay (Phase 9.1) ──────────────────────────────────────
# tools/weather-factory/ is a gitignored clone, so every Basilisk change to it
# has to live in the repo and be re-applied. cutechess.py is patched in place by
# setup_tools.ps1 (a one-line anchored insert); spsa.py / main.py are rewritten
# far too heavily for that, so they are kept whole under
# tools/weather-factory-overlay/ and COPIED over the clone. Both the setup and
# the launch path assert the copy is byte-identical to the tracked source — a
# stale clone silently reintroduces the games-vs-iterations schedule bug, and
# there is no way to see that in the run output.
$script:HarnessWfOverlayFiles = @("spsa.py", "main.py", "write_spsa_json.py", "describe_state.py")

function Get-HarnessWfOverlayDir {
    Join-Path $script:HarnessRepoRoot "tools\weather-factory-overlay"
}

function Install-WfOverlay {
    param([Parameter(Mandatory)][string]$WeatherFactoryDir)

    $overlayDir = Get-HarnessWfOverlayDir
    foreach ($name in $script:HarnessWfOverlayFiles) {
        $src = Join-Path $overlayDir $name
        if (-not (Test-Path $src)) { throw "Overlay file missing from the repo: $src" }
        $dst = Join-Path $WeatherFactoryDir $name
        Copy-Item $src $dst -Force
        python -m py_compile $dst
        if ($LASTEXITCODE -ne 0) { throw "Overlay file failed Python syntax validation: $dst" }
    }
    Write-Host "  weather-factory overlay installed and syntax-verified ($($script:HarnessWfOverlayFiles -join ', '))."
}

function Assert-WfOverlay {
    param([Parameter(Mandatory)][string]$WeatherFactoryDir)

    $overlayDir = Get-HarnessWfOverlayDir
    foreach ($name in $script:HarnessWfOverlayFiles) {
        $src = Join-Path $overlayDir $name
        $dst = Join-Path $WeatherFactoryDir $name
        if (-not (Test-Path $dst)) {
            throw "weather-factory is missing the Basilisk overlay file '$name'; run tools/setup_tools.ps1."
        }
        $srcHash = (Get-FileHash -LiteralPath $src -Algorithm SHA256).Hash
        $dstHash = (Get-FileHash -LiteralPath $dst -Algorithm SHA256).Hash
        if ($srcHash -ne $dstHash) {
            throw "weather-factory's '$name' does not match tools/weather-factory-overlay/$name " +
                  "(the clone is stale or was edited in place). Run tools/setup_tools.ps1. " +
                  "Without the overlay the SPSA schedule reverts to the pre-9.1 " +
                  "games-vs-iterations bug and every tune anneals ~8x too fast."
        }
    }
}

function Get-WfTunerState {
    <#
        Read tuner/state.json via the overlay's describe_state.py and return it
        as a hashtable, or $null when there is no usable state.

        PowerShell cannot parse this file at all: ConvertFrom-Json rejects the
        SPSA schema because `a` and `A` collide under its case-insensitive key
        handling, which is the same reason spsa.json is written from Python.
    #>
    param([Parameter(Mandatory)][string]$WeatherFactoryDir)

    $statePath = Join-Path $WeatherFactoryDir "tuner\state.json"
    if (-not (Test-Path $statePath)) { return $null }

    $describe = Join-Path $WeatherFactoryDir "describe_state.py"
    if (-not (Test-Path $describe)) { return $null }

    $lines = & python $describe $statePath 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $lines) { return $null }

    # Ordinal comparer, belt and braces: describe_state.py already avoids
    # emitting keys that differ only by case (`a` vs `A`), and a default
    # PowerShell hashtable would silently merge them if it ever did.
    $state = [System.Collections.Hashtable]::new(0, [System.StringComparer]::Ordinal)
    foreach ($line in $lines) {
        $kv = "$line".Split("=", 2)
        if ($kv.Count -eq 2) { $state[$kv[0]] = $kv[1] }
    }
    $state
}

function Test-HarnessFiniteNumber {
    <#
        True only for a plain finite decimal. fastchess prints 'inf' / 'nan' for
        an estimate or an error term when the sample is too small or degenerate
        (a clean sweep reports "nElo: inf +/- nan"), and casting those to
        [double] yields values that silently poison any comparison they enter.
    #>
    param([string]$Value)
    return ("$Value".Trim() -match '^[+-]?\d+(\.\d+)?$')
}

function Assert-NoAffinityFailure {
    param([Parameter(Mandatory)][string]$LogPath)

    $failure = Select-String -LiteralPath $LogPath `
        -Pattern '(?i)(failed to set cpu affinity|no cores available)' `
        -ErrorAction SilentlyContinue
    if ($failure) {
        throw "fastchess reported an affinity failure; the match is invalid. See '$LogPath'."
    }
}
