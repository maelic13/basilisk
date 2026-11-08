<# Frozen launcher for PLAN B.4.5 (BAS-S50): §9's categoricals and the whole-arm
   paired run. Each is a 2,000-game match (1,000 paired UHO openings) at 3+0.03,
   one thread, Hash 64, no adjudication, tablebases off, on the Colosseum
   match-fixed.toml policy; every binary is pinned by SHA-256.

   Items 1 and 2, which decide the arm the paired run measures:
   -Run EvasionPrune  item 1: B4EvasionPrune=0
   -Run FailLowStore  item 2: B3FailLowStore=0
   The whole-arm run:
   -Run paired        the B4_QSEARCH=ON release build against the OFF build of
                      the same revision (both final-PGO, each pool's build 1)
   Items 3 to 5, in order:
   -Run GainValues    item 3: B4GainValues=1
   -Run SeeValues     item 4: B4SeeValues=1
   -Run QuietChecks   item 5: B4QuietChecks=6, only past its gate

   BAS-S50's order is enforced, so the runs may be queued:
   - `paired` refuses until items 1 and 2 are complete and neither moves a
     default (a moved default means a rebuilt, re-pinned ON arm first);
   - items 3 to 5 refuse the same way, and also until the paired run is
     complete and at or above P1's -30 defect stop;
   - item 4 refuses until item 3 is complete, and item 5 until item 4 is, and
     item 5 also until its gate is met ($itemFiveGateMet, from the recorded read).
   A refusal throws, and nothing is played. A dry run plays nothing and is not
   held to the order.

   For the categoricals arm A plays the alternative and arm B the defaults on
   one ON Tune build, so a positive result favours the alternative (BAS-S50
   CONDITIONS: adopted at >= +5 and rejected at <= -5 only when the 95%
   interval also excludes 0). #>
param(
    [Parameter(Mandatory)]
    [ValidateSet('EvasionPrune', 'FailLowStore', 'paired', 'GainValues', 'SeeValues', 'QuietChecks')]
    [string]$Run,
    [switch]$DryRun,
    # Runs the order's guards and stops, so they can be tested on synthetic results.
    [switch]$CheckOrder,
    [string]$ResultsRoot = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = if ($ResultsRoot) { $ResultsRoot } else { Join-Path $PSScriptRoot 'results' }
$revision = '516d42224038ee28ebd840f11cfbcc8188a2e480'
$on   = @{ Path = Join-Path $engines 'basilisk-b45s-on-pgo1-pext-pgo.exe';      Sha = '9C9E463E9AC691860BCF51FFA6D15A72AEDD7375E4615D171FF1A15785915F27';   Bench = 4003560 }
$off  = @{ Path = Join-Path $engines 'basilisk-b45s-off-pgo1-pext-pgo.exe';     Sha = '84F059B4DD71B9CDAEB073A57D57866483C1FBCC925CDE2DEA7EE8E17F2AC711';  Bench = 6090091 }
$tune = @{ Path = Join-Path $engines 'basilisk-b45s-on-tune-pext-tune-pgo.exe'; Sha = 'FDB8D28C0F0E377615C7D27E17425775754800294B2957D7DDCBEE8C0116A24D'; Bench = 4003560 }
# Item 5's gate (§9): razoring's beat share at expected cut nodes falls by a
# quarter of itself or more with six quiet checks. Read 17.31% to 6.44%, -62.8%.
$itemFiveGateMet = $true

$runs = [ordered]@{
    EvasionPrune = @{ Name = 'EvasionPrune0'; Options = @('B4EvasionPrune=0'); Seed = 50001 }
    FailLowStore = @{ Name = 'FailLowStore0'; Options = @('B3FailLowStore=0'); Seed = 50002 }
    GainValues   = @{ Name = 'GainValues1';   Options = @('B4GainValues=1');   Seed = 50003 }
    SeeValues    = @{ Name = 'SeeValues1';    Options = @('B4SeeValues=1');    Seed = 50004 }
    QuietChecks  = @{ Name = 'QuietChecks6';  Options = @('B4QuietChecks=6');  Seed = 50005 }
}
$first = @('EvasionPrune', 'FailLowStore')

# A completed run's final estimate: Colosseum's last progress record, which
# carries the pentanomial Elo and its 95% half-width.
function Get-B45Read([string]$Name) {
    $dir = Join-Path $results $Name
    $resultFile = Join-Path $dir 'result.json'
    if (-not (Test-Path -LiteralPath $resultFile)) { throw "ORDER - $Name has not been played." }
    $result = Get-Content -LiteralPath $resultFile -Raw | ConvertFrom-Json
    if ($result.status -ne 'completed' -or [int]$result.games_completed -ne 2000) {
        throw "ORDER - $Name is not a completed 2,000-game run."
    }
    $last = Get-Content -LiteralPath (Join-Path $dir 'run.log') | Where-Object { $_ -match '"event":"progress"' } | Select-Object -Last 1
    $progress = ($last | ConvertFrom-Json).progress
    if ([int]$progress.done -ne 2000) { throw "ORDER - $Name's last progress record is not its final one." }
    $elo = ($progress.fields | Where-Object { $_.label -eq 'Elo' }).value
    if ($elo -notmatch '^(?<e>[+-]?[0-9.]+) \+/- (?<h>[0-9.]+)$') { throw "ORDER - cannot read $Name's Elo '$elo'." }
    [pscustomobject]@{ Elo = [double]$Matches.e; Half = [double]$Matches.h }
}
function Test-Adopts($read) { $read.Elo -ge 5 -and ($read.Elo - $read.Half) -gt 0 }

function Assert-B45Order([string]$Target) {
    if ($Target -in $first) { return }
    $moved = @()
    foreach ($name in $first) {
        if (Test-Adopts (Get-B45Read "b45-cat-$name")) { $moved += $name }
    }
    if ($moved.Count) { throw "ORDER - items 1 and 2 move a default ($($moved -join ', ')): rebuild and re-pin the ON arm before -Run $Target." }
    if ($Target -eq 'paired') { return }
    $paired = Get-B45Read 'b45-paired'
    if ($paired.Elo -lt -30) { throw "ORDER - the paired run reads $($paired.Elo), below P1's -30 defect stop: the switch ablation comes first." }
    if ($Target -in @('SeeValues', 'QuietChecks')) { $null = Get-B45Read 'b45-cat-GainValues' }
    if ($Target -eq 'QuietChecks') {
        $null = Get-B45Read 'b45-cat-SeeValues'
        if (-not $itemFiveGateMet) { throw 'ORDER - item 5 is past no gate: its card closes at 0 on the recorded read.' }
    }
}

if ($CheckOrder) {
    Assert-B45Order $Run
    Write-Host "ORDER OK - $Run may run."
    exit 0
}
if (-not $DryRun) { Assert-B45Order $Run }

# The wrapper checks manifests, revision and bench; the registered binaries'
# hashes are checked here.
$used = if ($Run -eq 'paired') { @($on, $off) } else { @($tune) }
foreach ($engine in $used) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'match'
    Games = 2000
    Hash = 64
    Threads = 1
    MarginMs = 20
    BaseMs = 3000
    IncrementMs = 30
    ExpectRevision = $revision
    DryRun = $DryRun
}
if ($Run -eq 'paired') {
    $arguments += @{
        EngineA = $on.Path; NameA = 'B4Qsearch'
        EngineB = $off.Path; NameB = 'B3Head'
        Seed = 50000
        ExpectBench = @($on.Bench, $off.Bench)
        Dir = Join-Path $results 'b45-paired'
    }
} else {
    $r = $runs[$Run]
    $arguments += @{
        EngineA = $tune.Path; NameA = $r.Name
        EngineB = $tune.Path; NameB = 'Defaults'
        OptionsA = $r.Options
        Seed = $r.Seed
        ExpectBench = @($tune.Bench, $tune.Bench)
        Dir = Join-Path $results "b45-cat-$Run"
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
