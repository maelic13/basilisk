<# Frozen launcher for PLAN B.5.7's direction reads (BAS-S58): whether a gate of
   28 or 30 one-thread games, two per physical core on its SMT siblings, reads
   like today's 15 games on 15 cores at 3+0.03. Every binary is pinned by SHA-256.

   -Read speed  the zero-game sibling-load speed read (tools/smt_read.py), about
                40 minutes of an idle PC: pinned bench processes on 15 CPUs (one
                per game core), 28 and 30 (both siblings of 14 or 15 game cores),
                three rounds. Classical phase: the prefetch pair (an identical
                tree, a memory-latency cost) and BAS-P25's pools (an identical
                tree, a compute cost). NNUE phase: Stockfish with its embedded
                network, the stand-in for a network engine. B.5.5's pair is read
                from the games' own telemetry instead. Read once (2026-11-12),
                VOID: one NNUE window over the line (BAS-S58 Amendment 1).
   -Read nnue   Amendment 1: the NNUE phase alone, again, about 13 minutes of an
                idle PC, with one Stockfish per CPU for each window, every process
                pinned before it runs and the disturbance read as foreign work;
                reported with the first read's classical phase.
   -Read g28   B.5.5's capped pair, 5,006 games at 3+0.03, 28 at once on cores
                1-14's siblings, the prototype runner (--slot-per-logical-cpu);
                master seed 55006, so it replays gate (a)'s 2,503 openings in
                order and reads against it opening by opening.
   -Read g30    the same at 30 at once (cores 1-15), seed 55006; refused until
                g28 has finished with no time loss and no fault.

   A game read runs tools/host_sampler.ps1 beside it. -DryRun plays nothing and
   checks the run. A finished read is never extended or replaced. #>
param(
    [Parameter(Mandatory)][ValidateSet('speed', 'nnue', 'g28', 'g30')][string]$Read,
    [switch]$DryRun,
    [string]$ResultsRoot = '',
    [string]$EnginesDir = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$results = if ($ResultsRoot) { $ResultsRoot } else { Join-Path $PSScriptRoot 'results' }
$engines = if ($EnginesDir) { $EnginesDir } else { Join-Path $PSScriptRoot 'test_engines' }

function Get-Pinned([string]$Name, [string]$Sha, [string]$Arm = '', [int]$Depth = 0) {
    $path = Join-Path $engines $Name
    if (-not (Test-Path -LiteralPath $path)) { throw "PROVENANCE MISMATCH - no binary at $path." }
    $actual = Get-HarnessSha256 $path
    if ($actual -ne $Sha) { throw "PROVENANCE MISMATCH - $path is $actual, not $Sha." }
    $entry = [ordered]@{ name = [IO.Path]::GetFileNameWithoutExtension($Name); path = $path; sha256 = $Sha; arm = $Arm }
    if ($Depth) { $entry.depth = $Depth }
    $entry
}

$capRevision = '833659fb346af4b9ecab0956f81dc3ec1849a6bf'
$capOn = @{ Name = 'basilisk-b55-cap1-on-pext-pgo.exe'; Sha = '122BE50F37E318955F43B01493F85F69A9CE8FCC5900BD970A3C49DE8D51C5AF'; Bench = 3783135 }
$capOff = @{ Name = 'basilisk-b55-cap1-off-pext-pgo.exe'; Sha = '17978E36DD29F7FB1FB85CFAAD835EE86F7209222DE9D4A0FDB0234416DC5E85'; Bench = 4144790 }

if ($Read -in 'speed', 'nnue') {
    $first = Join-Path $results 'b57-speed'
    $dir = if ($Read -eq 'nnue') { Join-Path $results 'b57-speed-nnue' } else { $first }
    if (Test-Path -LiteralPath (Join-Path $dir 'runs.jsonl')) {
        throw "STALE RESULT - $dir holds a speed read; it is not extended or replaced."
    }
    if ($Read -eq 'nnue' -and -not (Test-Path -LiteralPath (Join-Path $first 'runs.jsonl'))) {
        throw "ORDER - the NNUE re-run is reported with the first read's classical phase, and $first holds none (BAS-S58)."
    }
    $classical = @(
        (Get-Pinned 'basilisk-b57-pfon1-pext-pgo.exe' '2DA8401D034E234F0131710F89D7EBB659CB69F5E54471B2EB6BCDCBEB609CEC' 'pf-on'),
        (Get-Pinned 'basilisk-b57-pfon2-pext-pgo.exe' '76776566C71C75E81E11254659277B51201885D124F21DFE4134615AC715EA16' 'pf-on'),
        (Get-Pinned 'basilisk-b57-pfoff1-pext-pgo.exe' 'A4E82F0B45DF2A4D13BD0FFBC050D9FD3925C44BEAB621BD860CEB346EF4AFA0' 'pf-off'),
        (Get-Pinned 'basilisk-b57-pfoff2-pext-pgo.exe' '0AB244D2A203D4B32A33A48125030BBF1E6D90C6D1CF6F3924D1FD45437EDCD0' 'pf-off'),
        (Get-Pinned 'basilisk-b164-p25-base-pgo1-pext-pgo.exe' 'F25C5532A334FC18C7CCD4CA1F0CBD8352AB32A904153D11922F68DDB147127C' 'p25-base' 10),
        (Get-Pinned 'basilisk-b164-p25-base-pgo2-pext-pgo.exe' '908B39B350477DBBD6FBC0A0341B847F80C9F474550A82ED0C956D0A0665DE3A' 'p25-base' 10),
        (Get-Pinned 'basilisk-b164-p25-base-pgo3-pext-pgo.exe' 'BE9A2686B7253FCBB988C49BFB0A8F2B69A634F2E94682CBFB07356AE89D7413' 'p25-base' 10),
        (Get-Pinned 'basilisk-b164-p25-base-pgo4-pext-pgo.exe' '76F06F061B820C8E1EDEE483580DF8F79DB65BCD2DA4C6BBA1E0C04EEA706287' 'p25-base' 10),
        (Get-Pinned 'basilisk-b164-p25-cand-pgo1-pext-pgo.exe' '33406E1804CFC4352866877939AD6387D7FB88D8F95608E8106284B3E292E2CA' 'p25-cand' 10),
        (Get-Pinned 'basilisk-b164-p25-cand-pgo2-pext-pgo.exe' '5DCC57B80A20D59C6BE2E174875B9C018BEEEB60FA17696F5964BBE1A5CF62C6' 'p25-cand' 10),
        (Get-Pinned 'basilisk-b164-p25-cand-pgo3-pext-pgo.exe' 'E89524E8D25F5BE75C491417E1F34A0BF769973198DEE71A2BEBBAE48B0E21DF' 'p25-cand' 10),
        (Get-Pinned 'basilisk-b164-p25-cand-pgo4-pext-pgo.exe' 'CE47F62528E6866FE204EADD9D74926759ED9E1D45F44518331C243F94D67149' 'p25-cand' 10)
    )
    $nnue = @((Get-Pinned 'stockfish-b57-nnue.exe' 'F7C3D427012ADE479574F0A58E4D5371A54DF13A2A9892E1D311A7039181F2B2' 'sf'))
    $gameCores = @(Get-HarnessGameCpus | Sort-Object Cpu)
    if ($gameCores.Count -ne 15) { throw "TOPOLOGY - the registered read needs 15 game cores; this host has $($gameCores.Count)." }
    $layouts = [ordered]@{
        L15 = @($gameCores | ForEach-Object { [int](@($_.Logical) | Sort-Object | Select-Object -First 1) })
        L28 = @(Get-HarnessSiblingPairCpuList -Concurrency 28 -GameCores $gameCores)
        L30 = @(Get-HarnessSiblingPairCpuList -Concurrency 30 -GameCores $gameCores)
    }
    $config = [ordered]@{
        experiment = 'BAS-S58'; read = 'speed'; repo_revision = "$(git -C $repo rev-parse HEAD)"
        layouts = $layouts
        # Warm-ups run one at a time before any window, on CPU 0's sibling, never a game CPU.
        warmup_cpu = [int](@(Get-HarnessPhysicalCpus | Sort-Object Cpu | Select-Object -First 1).Logical | Sort-Object | Select-Object -Last 1)
        settle_seconds = 20; gap_seconds = 5; rounds = 3
        phases = @(
            [ordered]@{
                name = 'classical'; kind = 'basilisk'; binaries = $classical
                workload = [ordered]@{ hash = 64; depth = 13; repeats = 3 }
                layouts = @('L15', 'L28', 'L30'); seconds = 150; warmup_seconds = 10
                contrasts = @(
                    [ordered]@{ name = 'prefetch'; kind = 'memory latency, identical tree'; slow = 'pf-off'; fast = 'pf-on'; judged = $true },
                    [ordered]@{ name = 'p25'; kind = 'compute, identical tree'; slow = 'p25-base'; fast = 'p25-cand'; judged = $true }
                )
            },
            [ordered]@{
                name = 'nnue'; kind = 'stockfish'; binaries = $nnue
                workload = [ordered]@{ hash = 64; depth = 15 }
                layouts = @('L15', 'L28', 'L30'); seconds = 80; warmup_seconds = 10
                contrasts = @()
            }
        )
        rules = [ordered]@{
            reference_layout = 'L15'; judged_layouts = @('L28', 'L30')
            classical_phase = 'classical'; nnue_phase = 'nnue'; yield_arm = 'pf-on'
            pricing_floor_pp = 0.75; pricing_share = 0.25
            nnue_gap_pass = 0.05; nnue_gap_fail = 0.10
            min_processes_per_cell = 30
        }
    }
    if ($Read -eq 'nnue') {
        # Amendment 1: the NNUE phase alone, one persistent Stockfish per CPU.
        $config.read = 'nnue (Amendment 1)'
        $nnuePhase = $config.phases[1]
        $nnuePhase.persistent = $true
        $config.phases = @($nnuePhase)
    }
    New-Item -ItemType Directory -Force -Path $results | Out-Null
    $configPath = Join-Path $results "b57-$(if ($Read -eq 'nnue') { 'speed-nnue' } else { 'speed' }).read.json"
    $config | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $configPath -Encoding utf8
    Write-Host "Speed read configured: $configPath"
    foreach ($name in $layouts.Keys) { Write-Host "  $name : $($layouts[$name] -join ',')" }
    Write-Host "  phases: $(($config.phases | ForEach-Object { "$($_.name) ($(@($_.binaries).Count) binaries)" }) -join ', '); about $(if ($Read -eq 'nnue') { 13 } else { 40 }) minutes."
    if ($DryRun) {
        $state = Get-HarnessHostBusyPercent
        Write-Host "Dry run accepted; host busy $([Math]::Round($state, 1))% now (the read itself requires an idle host)."
        exit 0
    }
    Assert-HarnessHostIdle -MaxBusyPercent 10 | Out-Null
    & python (Join-Path $PSScriptRoot 'smt_read.py') run --config $configPath --out $dir
    $status = $LASTEXITCODE
    if ($Read -eq 'nnue') {
        & python (Join-Path $PSScriptRoot 'smt_read.py') report $first --phase-from "nnue=$dir"
    } else {
        & python (Join-Path $PSScriptRoot 'smt_read.py') report $dir
    }
    if ($status) { exit $status }
    exit $LASTEXITCODE
}

# Game reads.
$concurrency = @{ g28 = 28; g30 = 30 }[$Read]
$dir = Join-Path $results "b57-$Read"
if (-not $DryRun -and (Test-Path -LiteralPath (Join-Path $dir 'results.txt'))) {
    throw "STALE RESULT - $dir holds a finished read; it is not extended or replaced."
}
if ($Read -eq 'g30' -and -not $DryRun) {
    $g28 = Join-Path $results 'b57-g28\results.txt'
    if (-not (Test-Path -LiteralPath $g28)) { throw 'ORDER - g30 runs only after g28 has finished (BAS-S58).' }
    $text = Get-Content -LiteralPath $g28 -Raw
    if ($text -notmatch '(?m)^faults: none' -or $text -notmatch '(?m)^time losses: none') {
        throw 'ORDER - g28 recorded a time loss or a fault; g30 does not run (BAS-S58).'
    }
}
$on = Get-Pinned $capOn.Name $capOn.Sha
$off = Get-Pinned $capOff.Name $capOff.Sha
$arguments = @{
    Mode = 'match'; Games = 5006
    EngineA = $on.path; EngineB = $off.path
    NameA = 'B5FittedCap1'; NameB = 'B5Off'
    ExpectBench = @($capOn.Bench, $capOff.Bench)
    ExpectRevision = $capRevision
    Hash = 64; Threads = 1; MarginMs = 20
    BaseMs = 3000; IncrementMs = 30
    Seed = 55006
    Placement = 'siblings'; Concurrency = $concurrency
    CliPath = Join-Path $PSScriptRoot 'bin\colosseum-cli-smt-proto.exe'
    PinPath = Join-Path $PSScriptRoot 'colosseum\colosseum-smt-proto.pin.json'
    Dir = $dir
    DryRun = $DryRun
}
$sampler = $null
$stopFile = Join-Path $results "b57-$Read-sampler.stop"
if (-not $DryRun) {
    Remove-Item -LiteralPath $stopFile -ErrorAction SilentlyContinue
    $sampler = Start-Process -FilePath 'pwsh' -PassThru -WindowStyle Hidden -ArgumentList @(
        '-NoProfile', '-File', (Join-Path $PSScriptRoot 'host_sampler.ps1'),
        '-OutDir', (Join-Path $results "b57-$Read-sampler"), '-ParentPid', "$PID", '-StopFile', $stopFile)
}
try {
    & (Join-Path $PSScriptRoot 'colosseum.ps1') @arguments
    $status = $LASTEXITCODE
} finally {
    if ($sampler) { New-Item -ItemType File -Force -Path $stopFile | Out-Null }
}
if ($status) { exit $status }
if (-not $DryRun) {
    & python (Join-Path $PSScriptRoot 'diag\replay_compare.py') (Join-Path $results 'b55-gate-a-cap1') $dir
}
