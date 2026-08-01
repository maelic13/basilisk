<#
.SYNOPSIS
    A/B NPS comparison for Phase 8.7 — the speed counterpart of sprt.ps1.

.DESCRIPTION
    sprt.ps1 has a null-pair calibration for ELO (identical binaries must read
    ~0). Nothing equivalent existed for NPS, and Phase 8.7 judges every item on
    sub-1% speed deltas — so this script IS the instrument, and 8.7.1 gates the
    whole phase on it.

    Imported rules (Rarog 10.3's rewritten NPS protocol; PLAN Phase 8.7):

      - VALIDATE ON A SELF PAIR FIRST (-SelfPair). Two of Rarog's estimators
        were biased -0.2..-0.4% and produced two confident FALSE REJECTIONS
        before a self pair caught them. Bench NPS is left-skewed (the slow tail
        is interference, not signal), so any design that weights the arms
        unequally against that tail manufactures bias. This script therefore
        compares arm MEDIANS (tail-robust) and reports best-of alongside.
      - STRICTLY ALTERNATE ARMS (A,B,A,B,...) so thermal/background drift hits
        both arms equally instead of whichever ran first.
      - POOL >=2 PGO BUILDS PER ARM. Identical-source PGO builds differ by
        ~0.36%, so one build per arm cannot resolve a sub-1% effect: pass
        several -EnginesA / -EnginesB and per-build medians are printed so
        non-overlap between builds is visible.
      - Non-PGO builds are a cheap deterministic SCREEN that OVERSTATES the
        shipped gain (Rarog 8d: +6.35% non-PGO -> +1.18% PGO). Screen non-PGO,
        confirm under PGO.
      - Idle, pinned machine only. Each bench runs pinned to ONE physical core
        at High priority; a busy box invalidates the run (the 2026-07-21
        incident is the standing counterexample).

    Node counts are deterministic, so a bench-identical pair is a PURE speed
    comparison. The script checks the fingerprints and says so explicitly —
    if they differ, the arms are doing different work and the delta is not
    pure speed.

.PARAMETER EnginesA
    One or more engine .exe paths forming arm A (the candidate). Multiple
    paths = independent builds pooled per the PGO-luck rule.

.PARAMETER EnginesB
    Same for arm B (the baseline). Ignored with -SelfPair.

.PARAMETER SelfPair
    Validation mode: both arms are EnginesA. The result MUST read ~0.00%
    (this script flags anything beyond -SelfPairTolerance) before any real
    comparison is trusted.

.PARAMETER OptionsA / OptionsB
    Per-arm UCI options as "Name=Value" strings. This permits a same-binary
    feature A/B without independent-build layout luck, e.g.
    `-OptionsA HelperHistoryBlend=false -OptionsB HelperHistoryBlend=true`.

.PARAMETER Threads
    Worker count passed as the third `bench` argument. Default 1 preserves the
    deterministic fingerprint workload; use 4 for an explicitly MT-only cost.

.PARAMETER Depth
    Bench depth. Default 13 = the standard fingerprint workload (11,941,440).

.PARAMETER Repeats
    Bench repeats per invocation; each repeat yields one NPS sample.

.PARAMETER Rounds
    Alternating A/B rounds. Total samples per arm = Rounds * Repeats.

.PARAMETER Cpu
    Highest-numbered physical CPU in the pinned set. The set contains one
    physical CPU per requested thread and ends at this CPU. Default selects
    the highest available physical CPUs; SMT siblings are excluded.

.EXAMPLE
    # REQUIRED first: validate the estimator reads zero on a self pair
    ./tools/nps_ab.ps1 -EnginesA tools\test_engines\basilisk-v1.9.1-pext-pgo.exe -SelfPair

.EXAMPLE
    # Real comparison, two builds pooled per arm
    ./tools/nps_ab.ps1 `
        -EnginesA cand1.exe,cand2.exe -EnginesB base1.exe,base2.exe
#>
param(
    [Parameter(Mandatory)][string[]]$EnginesA,
    [string[]]$EnginesB = @(),
    [switch]$SelfPair,
    [string[]]$OptionsA = @(),
    [string[]]$OptionsB = @(),
    [int]$Threads = 1,
    [int]$Depth = 13,
    [int]$Repeats = 3,
    [int]$Rounds = 16,
    [int]$Cpu = -1,
    [double]$SelfPairTolerance = 0.30,
    [int]$Bootstrap = 2000,
    [string]$Label = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "..\harness_common.ps1")

if ($SelfPair) { $EnginesB = $EnginesA; $OptionsB = $OptionsA }
if (-not $EnginesB -or $EnginesB.Count -eq 0) {
    throw "-EnginesB is required unless -SelfPair is given."
}
foreach ($e in ($EnginesA + $EnginesB)) {
    if (-not (Test-Path -LiteralPath $e)) { throw "Not found: $e" }
}
$EnginesA = @($EnginesA | ForEach-Object { (Resolve-Path $_).Path })
$EnginesB = @($EnginesB | ForEach-Object { (Resolve-Path $_).Path })
if ($Threads -lt 1) { throw "-Threads must be >= 1." }

function ConvertTo-SetOptionCommands {
    param([string[]]$Options)
    @($Options | Where-Object { $_ } | ForEach-Object {
        if ($_ -notmatch '^([^=]+)=(.*)$') {
            throw "Invalid UCI option '$_': expected Name=Value."
        }
        "setoption name $($Matches[1]) value $($Matches[2])"
    })
}

$optionCommandsA = @(ConvertTo-SetOptionCommands $OptionsA)
$optionCommandsB = @(ConvertTo-SetOptionCommands $OptionsB)

# Pin to one physical core so the process cannot migrate mid-bench. Topology
# comes from harness_common so SMT siblings are never chosen.
#
# Default to the LAST physical core, not the first: Windows schedules interrupt
# /DPC and general system work preferentially on CPU 0, and pinning CONCENTRATES
# whatever interference exists onto the chosen core instead of letting the OS
# spread it. Measured 2026-07-23 with a video playing and CPU 0 pinned: one
# sample stalled to 2.15M against a 3.2M norm (-33%) and the self-pair CI blew
# out to +/-4%, which cannot resolve the sub-1% effects this phase measures.
$cores = @(Get-HarnessPhysicalCpus)
if ($Threads -gt $cores.Count) {
    throw "-Threads $Threads exceeds the $($cores.Count) detected physical cores."
}
$endCoreIndex = $cores.Count - 1
if ($Cpu -ge 0) {
    $matchingCore = @($cores | ForEach-Object -Begin { $i = -1 } -Process {
        ++$i
        if ($_.Cpu -eq $Cpu) { $i }
    })
    if ($matchingCore.Count -ne 1) {
        throw "-Cpu $Cpu is not one of the detected physical CPUs: $($cores.Cpu -join ', ')."
    }
    $endCoreIndex = $matchingCore[0]
}
$startCoreIndex = $endCoreIndex - $Threads + 1
if ($startCoreIndex -lt 0) {
    throw "Not enough physical CPUs at or below -Cpu $Cpu for -Threads $Threads."
}
$selectedCores = @($cores[$startCoreIndex..$endCoreIndex])
$affinityBits = [int64]0
foreach ($core in $selectedCores) {
    if ($core.Cpu -ge 63) {
        throw "Processor-group affinity above CPU 62 is not supported by this harness."
    }
    $affinityBits = $affinityBits -bor ([int64]1 -shl $core.Cpu)
}
$affinityMask = [IntPtr]$affinityBits
$pinnedCpuLabel = $selectedCores.Cpu -join ','

function Invoke-Bench {
    param([string]$Exe, [int]$D, [int]$R, [int]$T, [string[]]$OptionCommands)

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName               = $Exe
    $psi.RedirectStandardInput  = $true
    $psi.RedirectStandardOutput = $true
    $psi.UseShellExecute        = $false
    $p = [System.Diagnostics.Process]::Start($psi)
    try {
        $p.ProcessorAffinity = $affinityMask
        $p.PriorityClass     = [System.Diagnostics.ProcessPriorityClass]::High
    } catch {
        Write-Warning "Could not pin/prioritise $([IO.Path]::GetFileName($Exe)): $($_.Exception.Message)"
    }
    foreach ($command in $OptionCommands) { $p.StandardInput.WriteLine($command) }
    $p.StandardInput.WriteLine("bench $D $R $T")
    $p.StandardInput.WriteLine("quit")
    $p.StandardInput.Close()
    $text = $p.StandardOutput.ReadToEnd()
    $p.WaitForExit()

    $samples = [regex]::Matches($text, 'run\s+\d+/\d+\s+nodes\s+(\d+)\s+time\s+\d+ms\s+nps\s+(\d+)')
    if ($samples.Count -eq 0) {
        # repeats==1 prints no per-run lines; fall back to the summary.
        $m = [regex]::Match($text, 'Nodes/second\s*:\s*(\d+)')
        $n = [regex]::Match($text, 'Nodes searched\s*:\s*(\d+)')
        if (-not $m.Success) { throw "Could not parse NPS from $Exe." }
        return [pscustomobject]@{ Nps = @([double]$m.Groups[1].Value); Nodes = [int64]$n.Groups[1].Value }
    }
    $nps   = @($samples | ForEach-Object { [double]$_.Groups[2].Value })
    $nodes = [int64]$samples[0].Groups[1].Value
    [pscustomobject]@{ Nps = $nps; Nodes = $nodes }
}

function Get-Median {
    param([double[]]$v)
    if ($v.Count -eq 0) { return 0.0 }
    $s = @($v | Sort-Object)
    if ($s.Count % 2 -eq 1) { return $s[[int](($s.Count - 1) / 2)] }
    return ($s[$s.Count / 2 - 1] + $s[$s.Count / 2]) / 2.0
}

$modeLabel = if ($SelfPair) { "SELF-PAIR VALIDATION" } else { "A/B comparison" }
Write-Host ""
Write-Host "======================================================="
Write-Host "  NPS $modeLabel$(if ($Label) { "  [$Label]" })"
Write-Host "  Arm A builds: $($EnginesA.Count)   Arm B builds: $($EnginesB.Count)"
Write-Host "  bench $Depth x $Repeats x $Rounds alternating rounds  Threads: $Threads"
Write-Host "  Samples/arm: $($Rounds * $Repeats)   Pinned CPUs: $pinnedCpuLabel (High priority)"
if ($OptionsA.Count) { Write-Host "  Options A: $($OptionsA -join ', ')" }
if ($OptionsB.Count) { Write-Host "  Options B: $($OptionsB -join ', ')" }
Write-Host "======================================================="
Write-Host ""

$sampA = @(); $sampB = @()
$roundMedA = @(); $roundMedB = @()
$perBuildA = @{}; $perBuildB = @{}
$nodesA = 0; $nodesB = 0

Write-Host "Warm-up..."
Invoke-Bench -Exe $EnginesA[0] -D $Depth -R 1 -T $Threads -OptionCommands $optionCommandsA | Out-Null

for ($r = 0; $r -lt $Rounds; $r++) {
    # Cycle builds within each arm so every build gets even coverage.
    $ea = $EnginesA[$r % $EnginesA.Count]
    $eb = $EnginesB[$r % $EnginesB.Count]

    # ALTERNATE THE WITHIN-ROUND ORDER (Rarog's fix, re-derived the hard way
    # here 2026-07-23). Running A-then-B every round is NOT symmetric: the arm
    # in the first slot pays a cold-start penalty (cache/turbo warm-up from the
    # gap since the previous process), so it reads systematically slower. The
    # first version of this script did exactly that and its SELF PAIR read
    # -0.29% with B faster in 8/8 rounds — a pure artifact, and the same
    # -0.2..-0.4% signature Rarog's two discarded estimators produced. Swapping
    # the slot each round cancels it.
    if ($r % 2 -eq 0) {
        $ra = Invoke-Bench -Exe $ea -D $Depth -R $Repeats -T $Threads -OptionCommands $optionCommandsA
        $rb = Invoke-Bench -Exe $eb -D $Depth -R $Repeats -T $Threads -OptionCommands $optionCommandsB
    } else {
        $rb = Invoke-Bench -Exe $eb -D $Depth -R $Repeats -T $Threads -OptionCommands $optionCommandsB
        $ra = Invoke-Bench -Exe $ea -D $Depth -R $Repeats -T $Threads -OptionCommands $optionCommandsA
    }

    # SAMPLE UNIT = the best of this invocation's repeats, not every repeat
    # (Rarog's design, adopted 2026-07-23 after measuring why it matters).
    # Slow repeats are interference, never signal, and turbo/thermal state also
    # produces fast outliers — pooling raw repeats feeds BOTH tails into the
    # bootstrap. Measured here: raw-repeat pooling gave a self-pair CI of
    # +/-4..5% while the per-round medians sat inside a 1.9% band, i.e. the CI
    # described the noise, not the resolution. Best-of-N per invocation is the
    # noise-free ceiling, which is what "best-of-N NPS" has always meant in
    # this project's dev guide.
    $sA = ($ra.Nps | Measure-Object -Maximum).Maximum
    $sB = ($rb.Nps | Measure-Object -Maximum).Maximum

    $sampA += $sA; $nodesA = $ra.Nodes
    if (-not $perBuildA.ContainsKey($ea)) { $perBuildA[$ea] = @() }
    $perBuildA[$ea] += $sA

    $sampB += $sB; $nodesB = $rb.Nodes
    if (-not $perBuildB.ContainsKey($eb)) { $perBuildB[$eb] = @() }
    $perBuildB[$eb] += $sB

    $mA = $sA; $mB = $sB
    $roundMedA += $mA; $roundMedB += $mB
    Write-Host ("round {0,2}/{1}  {2}  A {3:N0}   B {4:N0}" -f `
        ($r + 1), $Rounds, $(if ($r % 2 -eq 0) { "A first" } else { "B first" }), $mA, $mB)
}

$medA = Get-Median $sampA; $medB = Get-Median $sampB
$bestA = ($sampA | Measure-Object -Maximum).Maximum
$bestB = ($sampB | Measure-Object -Maximum).Maximum
$dMed  = 100.0 * ($medA - $medB) / $medB
$dBest = 100.0 * ($bestA - $bestB) / $bestB

# Bootstrap CI on the median delta: resample each arm's pooled samples with
# replacement, recompute the median delta, take the 2.5/97.5 percentiles.
$rand = [System.Random]::new(20260723)
$boot = New-Object System.Collections.Generic.List[double]
for ($i = 0; $i -lt $Bootstrap; $i++) {
    $ba = @(1..$sampA.Count | ForEach-Object { $sampA[$rand.Next($sampA.Count)] })
    $bb = @(1..$sampB.Count | ForEach-Object { $sampB[$rand.Next($sampB.Count)] })
    # NOT $mb: PowerShell variable names are case-insensitive, so $mb and the
    # per-round $mB above are ONE variable. Harmless as written (the two loops
    # never overlap), but it is the exact shape of the bug that clobbered a
    # Rarog schedule constant, so the two meanings get two names.
    $bootMedB = Get-Median $bb
    if ($bootMedB -gt 0) { $boot.Add(100.0 * ((Get-Median $ba) - $bootMedB) / $bootMedB) }
}
$bootSorted = @($boot | Sort-Object)
$lo = $bootSorted[[int][Math]::Floor(0.025 * $bootSorted.Count)]
$hi = $bootSorted[[int][Math]::Floor(0.975 * $bootSorted.Count)]

Write-Host ""
Write-Host "---- per-build medians (PGO-luck spread; ~0.36% between identical-source builds) ----"
foreach ($k in $perBuildA.Keys) { Write-Host ("  A  {0,-46} {1,12:N0}" -f [IO.Path]::GetFileName($k), (Get-Median $perBuildA[$k])) }
foreach ($k in $perBuildB.Keys) { Write-Host ("  B  {0,-46} {1,12:N0}" -f [IO.Path]::GetFileName($k), (Get-Median $perBuildB[$k])) }

Write-Host ""
Write-Host "======================================================="
Write-Host ("  arm A   median {0,12:N0}   best {1,12:N0}" -f $medA, $bestA)
Write-Host ("  arm B   median {0,12:N0}   best {1,12:N0}" -f $medB, $bestB)
Write-Host ""
Write-Host ("  delta (median) : {0,7:F2}%   95% CI [{1:F2}%, {2:F2}%]" -f $dMed, $lo, $hi)
Write-Host ("  delta (best-of): {0,7:F2}%" -f $dBest)
# Rarog's non-parametric cross-check: with no real effect this should sit near
# half. A lopsided count with a small delta is the fingerprint of a slot/order
# artifact rather than a speed difference.
$aWins = 0
for ($j = 0; $j -lt $roundMedA.Count; $j++) { if ($roundMedA[$j] -gt $roundMedB[$j]) { $aWins++ } }
Write-Host ("  A faster in    : {0}/{1} rounds" -f $aWins, $roundMedA.Count)
if ($nodesA -eq $nodesB) {
    Write-Host "  fingerprints   : IDENTICAL ($nodesA) — pure speed comparison"
} elseif ($Threads -gt 1) {
    Write-Host "  fingerprints   : A=$nodesA B=$nodesB — expected Lazy-SMP variation; NPS includes tree/scheduling effects"
} else {
    Write-Warning "  fingerprints DIFFER (A=$nodesA B=$nodesB) — the arms do different work; this delta is NOT pure speed."
}
Write-Host "======================================================="

if ($SelfPair) {
    Write-Host ""
    if ([Math]::Abs($dMed) -le $SelfPairTolerance -and $lo -le 0 -and $hi -ge 0) {
        Write-Host ("SELF-PAIR OK: |{0:F2}%| <= {1:F2}% and the CI covers 0 — estimator is unbiased." -f $dMed, $SelfPairTolerance) -ForegroundColor Green
    } else {
        Write-Warning ("SELF-PAIR FAILED: median delta {0:F2}% (CI [{1:F2}%, {2:F2}%]) — the estimator is BIASED. " -f $dMed, $lo, $hi)
        Write-Warning "Do NOT use it to judge any item until this reads ~0. Check: box idle? pinning applied? one build per arm (PGO luck)?"
    }
} else {
    Write-Host ""
    Write-Host "Reminder: use >=2 PGO builds per arm, or one byte-identical binary with differing options,"
    Write-Host "and an idle box. Non-PGO readings OVERSTATE the shipped gain — confirm under PGO."
}
