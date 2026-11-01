<# Frozen launcher for PLAN B.3.2 (BAS-S41): §9's categoricals and the
   whole-arm paired run. Each is a 2,000-game match (1,000 paired UHO
   openings), one thread, Hash 64, no adjudication, tablebases off, on the
   Colosseum match-fixed.toml policy; every binary is pinned by SHA-256.

   Items 1-3, which decide the arm the paired run measures:
   -Run CheckExt      item 1 (and item 2's read B): B3CheckExt=1, 3+0.03
   -Run CheckExtLtc   item 1 at 10+0.1
   -Run CheckLegacy   item 2's read A: the legacy check policy (B3CheckExt=1,
                      B3CheckPrune=0), 3+0.03
   -Run NmpAnyNode    item 3: B3NmpCutOnly=0, 3+0.03
   -Run NmpAnyNodeLtc item 3 at 10+0.1
   The whole-arm run:
   -Run paired        the B3_PROOF=ON release build against the OFF build of
                      the same revision (both final-PGO)
   Items 4-16 in BAS-S41's order, each at 3+0.03:
   -Run MultiCut, NmpVerify, ProbCutTt, NmpEvalGate, SingNegative,
        PvQuietPrune, SingTtDepth, IirStale, Cuckoo, HistoryFills, Iir,
        FailLowStore
   Items 8 (B3ProbCutPv) and 11 (B3CorrExcluded) are not here: the depth-14
   counters left their mechanisms within 10% (§5.12's rule), so their
   defaults stand without games.

   BAS-S41's order is enforced, so the runs may be queued unattended:
   - `paired` refuses until items 1-3 are complete and none of them moves a
     default (a moved default means a rebuilt, re-pinned ON arm first);
   - items 4-16 refuse the same way, and also until the paired run is
     complete and above P1's -40 defect stop.
   A refusal throws, and nothing is played. A dry run plays nothing and is
   not held to the order.

   For the categoricals arm A plays the alternative and arm B the defaults on
   one ON Tune build, so a positive result favours the alternative (BAS-S41
   CONDITIONS: adopted at >= +5 and rejected at <= -5 only when the 95%
   interval also excludes 0). #>
param(
    [Parameter(Mandatory)]
    [ValidateSet('CheckExt', 'CheckExtLtc', 'CheckLegacy', 'NmpAnyNode', 'NmpAnyNodeLtc', 'paired',
                 'MultiCut', 'NmpVerify', 'ProbCutTt', 'NmpEvalGate', 'SingNegative', 'PvQuietPrune',
                 'SingTtDepth', 'IirStale', 'Cuckoo', 'HistoryFills', 'Iir', 'FailLowStore')]
    [string]$Run,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results'
$revision = 'c6f7151a9da91a7990d01ed5b4217164b5466c61'
# The paired arms are build 1 of each NPS pool (b32-on, b32-off), which
# replaced the first pgo1 builds under the same names before any game used them.
$on   = @{ Path = Join-Path $engines 'basilisk-b32-on-pgo1-pext-pgo.exe';       Sha = '0B5D9DC605D765ECAC802775C83067A6A5454EB8AF21D73DEFEE0E29F77EA0D5'; Bench = 4923507 }
$off  = @{ Path = Join-Path $engines 'basilisk-b32-off-pgo1-pext-pgo.exe';      Sha = '8E5B98742393E76DD196534A945BFF5A09B7E96A3AEE7A2F17D06298CE9646E5'; Bench = 11880564 }
$tune = @{ Path = Join-Path $engines 'basilisk-b32-on-tune-pext-tune-pgo.exe';  Sha = 'B757C9369EEACCA541DA7457D2C892F7EDD7F6CE9814F245D9535F24F2052F70'; Bench = 4923507 }

# Alternative (arm A) against the defaults (arm B), the control and the seed
# BAS-S41 registered.
$runs = [ordered]@{
    CheckExt      = @{ Name = 'CheckExt1';      Options = @('B3CheckExt=1');                   Ms = 3000;  Inc = 30;  Seed = 41001 }
    CheckExtLtc   = @{ Name = 'CheckExt1';      Options = @('B3CheckExt=1');                   Ms = 10000; Inc = 100; Seed = 41011 }
    CheckLegacy   = @{ Name = 'CheckLegacy';    Options = @('B3CheckExt=1', 'B3CheckPrune=0'); Ms = 3000;  Inc = 30;  Seed = 41002 }
    NmpAnyNode    = @{ Name = 'NmpCutOnly0';    Options = @('B3NmpCutOnly=0');                 Ms = 3000;  Inc = 30;  Seed = 41003 }
    NmpAnyNodeLtc = @{ Name = 'NmpCutOnly0';    Options = @('B3NmpCutOnly=0');                 Ms = 10000; Inc = 100; Seed = 41013 }
    MultiCut      = @{ Name = 'MultiCut1';      Options = @('B3MultiCut=1');                   Ms = 3000;  Inc = 30;  Seed = 41004 }
    NmpVerify     = @{ Name = 'NmpVerify0';     Options = @('B3NmpVerify=0');                  Ms = 3000;  Inc = 30;  Seed = 41005 }
    ProbCutTt     = @{ Name = 'ProbCutTt1';     Options = @('B3ProbCutTt=1');                  Ms = 3000;  Inc = 30;  Seed = 41051 }
    NmpEvalGate   = @{ Name = 'NmpEvalGate1';   Options = @('B3NmpEvalGate=1');                Ms = 3000;  Inc = 30;  Seed = 41052 }
    SingNegative  = @{ Name = 'SingNegative1';  Options = @('B3SingNegative=1');               Ms = 3000;  Inc = 30;  Seed = 41014 }
    PvQuietPrune  = @{ Name = 'PvQuietPrune0';  Options = @('B3PvQuietPrune=0');               Ms = 3000;  Inc = 30;  Seed = 41015 }
    SingTtDepth   = @{ Name = 'SingTtDepth1';   Options = @('B3SingTtDepth=1');                Ms = 3000;  Inc = 30;  Seed = 41006 }
    IirStale      = @{ Name = 'IirStale1';      Options = @('B3IirStale=1');                   Ms = 3000;  Inc = 30;  Seed = 41007 }
    Cuckoo        = @{ Name = 'Cuckoo0';        Options = @('B3Cuckoo=0');                     Ms = 3000;  Inc = 30;  Seed = 41009 }
    HistoryFills  = @{ Name = 'HistoryFills1';  Options = @('CoreHistoryFills=1');             Ms = 3000;  Inc = 30;  Seed = 41010 }
    Iir           = @{ Name = 'Iir0';           Options = @('B3Iir=0');                        Ms = 3000;  Inc = 30;  Seed = 41020 }
    FailLowStore  = @{ Name = 'FailLowStore1';  Options = @('B3FailLowStore=1');               Ms = 3000;  Inc = 30;  Seed = 41016 }
}
$first = @('CheckExt', 'CheckExtLtc', 'CheckLegacy', 'NmpAnyNode', 'NmpAnyNodeLtc')

# A completed run's final estimate: Colosseum's last progress record, which
# carries the pentanomial Elo and its 95% half-width.
function Get-B32Read([string]$Name) {
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
    if ($elo -notmatch '^(?<e>-?[0-9.]+) \+/- (?<h>[0-9.]+)$') { throw "ORDER - cannot read $Name's Elo '$elo'." }
    [pscustomobject]@{ Elo = [double]$Matches.e; Half = [double]$Matches.h }
}
function Test-Adopts($read) { $read.Elo -ge 5 -and ($read.Elo - $read.Half) -gt 0 }

if (-not $DryRun -and $Run -notin $first) {
    $reads = @{}
    foreach ($name in $first) { $reads[$name] = Get-B32Read "b32-cat-$name" }
    $moved = @()
    # Item 1 adopts 1 only if neither read is <= -5 and one is >= +5.
    $itemOneDown = @($reads.CheckExt, $reads.CheckExtLtc | Where-Object { $_.Elo -le -5 }).Count
    if ($itemOneDown -eq 0 -and ((Test-Adopts $reads.CheckExt) -or (Test-Adopts $reads.CheckExtLtc))) { $moved += 'B3CheckExt (item 1)' }
    # Item 2's read A at <= -5 for the default is the legacy policy at >= +5.
    if (Test-Adopts $reads.CheckLegacy) { $moved += 'the check policy (item 2, read A)' }
    # Item 3 keeps 1 unless the 10+0.1 read is <= -5 for it.
    if (Test-Adopts $reads.NmpAnyNodeLtc) { $moved += 'B3NmpCutOnly (item 3, 10+0.1)' }
    if ($moved.Count) { throw "ORDER - items 1-3 move a default ($($moved -join '; ')): rebuild and re-pin the ON arm before -Run $Run." }
    if ($Run -ne 'paired') {
        $paired = Get-B32Read 'b32-paired'
        if ($paired.Elo -lt -40) { throw "ORDER - the paired run reads $($paired.Elo), below P1's -40 defect stop: the switch ablation comes first." }
    }
}

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
    ExpectRevision = $revision
    DryRun = $DryRun
}
if ($Run -eq 'paired') {
    $arguments += @{
        EngineA = $on.Path; NameA = 'B3Proof'
        EngineB = $off.Path; NameB = 'B2Head'
        BaseMs = 3000; IncrementMs = 30
        Seed = 41000
        ExpectBench = @($on.Bench, $off.Bench)
        Dir = Join-Path $results 'b32-paired'
    }
} else {
    $r = $runs[$Run]
    $arguments += @{
        EngineA = $tune.Path; NameA = $r.Name
        EngineB = $tune.Path; NameB = 'Defaults'
        OptionsA = $r.Options
        BaseMs = $r.Ms; IncrementMs = $r.Inc
        Seed = $r.Seed
        ExpectBench = @($tune.Bench, $tune.Bench)
        Dir = Join-Path $results "b32-cat-$Run"
    }
    # 10+0.1 needs its own run file (the policy check reads the resolved
    # control); -Games 2000 overrides the file's 1,000.
    if ($r.Ms -eq 10000) { $arguments.RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-ltc.toml' }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
