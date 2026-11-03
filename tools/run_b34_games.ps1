<# Frozen launcher for PLAN B.3.4 (BAS-S45): the categoricals re-read on
   the fitted ON arm. Each is a 2,000-game match (1,000 paired UHO
   openings) at 3+0.03, one thread, Hash 64, no adjudication, tablebases
   off, on the Colosseum match-fixed.toml policy, on one ON Tune build
   pinned by SHA-256: arm A plays the alternative by UCI option and arm B
   the defaults, so a positive result favours the alternative (adopted at
   >= +5 with the 95% interval excluding 0).

   -Run FailLowStore, HistoryFills, ProbCutTt, NmpAnyNode, MultiCut,
        SingNegative, SingTtDepth, IirStale, NmpEvalGate
   The runs are independent of one another, so they may be queued in any
   order. A dry run plays nothing. #>
param(
    [Parameter(Mandatory)]
    [ValidateSet('FailLowStore', 'HistoryFills', 'ProbCutTt', 'NmpAnyNode', 'MultiCut',
                 'SingNegative', 'SingTtDepth', 'IirStale', 'NmpEvalGate')]
    [string]$Run,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results'
$revision = '__REVISION__'
$tune = @{ Path = Join-Path $engines 'basilisk-b34-on-tune-pext-tune-pgo.exe'; Sha = '__SHA__'; Bench = 5369042 }

# Alternative (arm A) against the defaults (arm B), with BAS-S45's seeds.
$runs = [ordered]@{
    FailLowStore = @{ Name = 'FailLowStore1'; Options = @('B3FailLowStore=1');   Seed = 45001 }
    HistoryFills = @{ Name = 'HistoryFills1'; Options = @('CoreHistoryFills=1'); Seed = 45002 }
    ProbCutTt    = @{ Name = 'ProbCutTt1';    Options = @('B3ProbCutTt=1');      Seed = 45003 }
    NmpAnyNode   = @{ Name = 'NmpCutOnly0';   Options = @('B3NmpCutOnly=0');     Seed = 45004 }
    MultiCut     = @{ Name = 'MultiCut1';     Options = @('B3MultiCut=1');       Seed = 45005 }
    SingNegative = @{ Name = 'SingNegative1'; Options = @('B3SingNegative=1');   Seed = 45006 }
    SingTtDepth  = @{ Name = 'SingTtDepth1';  Options = @('B3SingTtDepth=1');    Seed = 45007 }
    IirStale     = @{ Name = 'IirStale1';     Options = @('B3IirStale=1');       Seed = 45008 }
    NmpEvalGate  = @{ Name = 'NmpEvalGate1';  Options = @('B3NmpEvalGate=1');    Seed = 45009 }
}

# The wrapper checks manifests, revision and bench; the registered binary's
# hash is checked here.
$actual = Get-HarnessSha256 $tune.Path
if ($actual -ne $tune.Sha) { throw "PROVENANCE MISMATCH - $($tune.Path) is $actual, not $($tune.Sha)." }

$r = $runs[$Run]
$arguments = @{
    Mode = 'match'
    Games = 2000
    Hash = 64
    Threads = 1
    MarginMs = 20
    ExpectRevision = $revision
    EngineA = $tune.Path; NameA = $r.Name
    EngineB = $tune.Path; NameB = 'Defaults'
    OptionsA = $r.Options
    BaseMs = 3000; IncrementMs = 30
    Seed = $r.Seed
    ExpectBench = @($tune.Bench, $tune.Bench)
    Dir = Join-Path $results "b34-cat-$Run"
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
