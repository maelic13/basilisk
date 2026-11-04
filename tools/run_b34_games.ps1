<# Frozen launcher for PLAN B.3.4 (BAS-S45): the categoricals re-read on
   the fitted ON arm. Each is a 2,000-game match (1,000 paired UHO
   openings) at 3+0.03, one thread, Hash 64, no adjudication, tablebases
   off, on the Colosseum match-fixed.toml policy, on one ON Tune build
   pinned by SHA-256: arm A plays the alternative by UCI option and arm B
   the defaults, so a positive result favours the alternative (adopted at
   >= +5 with the 95% interval excluding 0).

   -Run NmpEvalGate   the null move's eval gate (B3NmpEvalGate=1)
   BAS-S45's amendment 1 withdrew the other eight runs: on an arm fitted
   with Basilisk's forms, a donor form plays unfitted, so their read was
   tilted. A dry run plays nothing. #>
param(
    [Parameter(Mandatory)]
    [ValidateSet('NmpEvalGate')]
    [string]$Run,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$results = Join-Path $PSScriptRoot 'results'
$revision = '220f36fcb37c51d73a54574391342d143d6cdbcf'
$tune = @{ Path = Join-Path $engines 'basilisk-b34-on-tune-pext-tune-pgo.exe'; Sha = 'D47013B95B903F533B786144F70FF5FA243011E4921DB83E9E7DD24779983520'; Bench = 5369042 }

# Alternative (arm A) against the defaults (arm B), with BAS-S45's seeds.
$runs = [ordered]@{
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
