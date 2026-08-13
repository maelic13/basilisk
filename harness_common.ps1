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
        [int]$ThreadsPerGame = 1
    )

    if ($ThreadsPerGame -lt 1) { throw "ThreadsPerGame must be >= 1." }

    $physical = Get-PhysicalCoreCount
    $recommended = [Math]::Max(1, [Math]::Floor(($physical - $ReservePhysicalCores) / $ThreadsPerGame))
    $resolved = if ($Requested -gt 0) { $Requested } else { $recommended }

    $coresNeeded = $resolved * $ThreadsPerGame
    if ($coresNeeded -gt $physical) {
        throw "Concurrency $resolved x Threads $ThreadsPerGame = $coresNeeded engine threads " +
              "exceeds the detected $physical physical cores. Oversubscription halves NPS and " +
              "changes the depth reached, which invalidates the match."
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
    param([Parameter(Mandatory)][int]$Concurrency)

    $cores = @(Get-HarnessPhysicalCpus)
    if ($Concurrency -gt $cores.Count) {
        throw "Concurrency $Concurrency exceeds the detected $($cores.Count) physical cores."
    }
    (($cores | Select-Object -First $Concurrency).Cpu -join ',')
}

function New-HarnessSeed {
    param([int]$Requested)

    if ($Requested -ne 0) { return $Requested }
    Get-Random -Minimum 1 -Maximum ([int]::MaxValue)
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
