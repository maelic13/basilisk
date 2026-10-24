<# Frozen launcher for PLAN B.2.2 (BAS-S21): Cluster 1's unfitted paired run
   and its six categorical runs. Each is a 2,000-game match (1,000 paired UHO
   openings) at 3+0.03, one thread, Hash 64, no adjudication, tablebases off,
   on the Colosseum match-fixed.toml policy; every binary is pinned by SHA-256.

   -Run paired      the B2_CORE=ON release build against the B2_CORE=OFF build
                    of the same revision (both final-PGO): the cluster's
                    unfitted Elo, read by BAS-S21's rule.
   -Run <switch>    one ON Tune build on both arms: A plays the switch's other
                    value, B its default, so a positive result favours the
                    other value (adopted at >= +5 Elo, rejected at <= -5,
                    default kept between; PROCESS, Cluster delivery shape). #>
param(
    [Parameter(Mandatory)]
    [ValidateSet('paired', 'TtCutoffNodeTyped', 'PawnHistBuckets', 'CorrAdmission',
                 'RazorDepthCap', 'LmpExemptChecks', 'LmrNegativeAllowed')]
    [string]$Run,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$on   = @{ Path = Join-Path $engines 'basilisk-b22-on-pgo1-pext-pgo.exe';  Sha = '9FED724F6010990C9FD6A6984A52022F124DC53FFFD7218E148EB952E5F35377';  Bench = 9132715 }
$off  = @{ Path = Join-Path $engines 'basilisk-b22-off-pgo1-pext-pgo.exe'; Sha = 'EF503FC7C3E9A7CC8F8E26A2CE7D82E544600A8D687F8E79C7655009B47D4A58'; Bench = 14978465 }
$tune = @{ Path = Join-Path $engines 'basilisk-b22-on-tune-pext-tune-pgo.exe'; Sha = 'EB3EA56076FBF24E1E17787600465E670E18057C7272FFF4F607F77886802C0B'; Bench = 9132715 }

# Switch: other value (arm A) against the default (arm B), and the run's seed.
$switches = @{
    TtCutoffNodeTyped  = @{ Name = 'CoreTtCutoffNodeTyped';   Other = 0;  Default = 1;  Seed = 22201 }
    PawnHistBuckets    = @{ Name = 'CorePawnHistBucketsLog2'; Other = 11; Default = 13; Seed = 22202 }
    CorrAdmission      = @{ Name = 'CoreCorrAdmission';       Other = 0;  Default = 1;  Seed = 22203 }
    RazorDepthCap      = @{ Name = 'CoreRazorDepthCap';       Other = 3;  Default = 1;  Seed = 22204 }
    LmpExemptChecks    = @{ Name = 'CoreLmpExemptChecks';     Other = 0;  Default = 1;  Seed = 22205 }
    LmrNegativeAllowed = @{ Name = 'CoreLmrNegativeAllowed';  Other = 0;  Default = 1;  Seed = 22206 }
}

# The wrapper checks manifests, revision and bench for a two-arm match; the
# registered binaries' hashes are checked here.
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
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    DryRun = $DryRun
}
if ($Run -eq 'paired') {
    $arguments += @{
        EngineA = $on.Path; NameA = 'B2Core'
        EngineB = $off.Path; NameB = 'Legacy'
        Seed = 22200
        ExpectRevision = '84da91f078f72d62fdf83594cba21fdd2d923993'
        ExpectBench = @($on.Bench, $off.Bench)
        Dir = Join-Path $PSScriptRoot 'results\b22-paired'
    }
} else {
    $s = $switches[$Run]
    $arguments += @{
        EngineA = $tune.Path; NameA = "$($s.Name)$($s.Other)"
        EngineB = $tune.Path; NameB = "$($s.Name)$($s.Default)"
        OptionsA = @("$($s.Name)=$($s.Other)")
        OptionsB = @("$($s.Name)=$($s.Default)")
        Seed = $s.Seed
        ExpectRevision = '84fda5832e77a73f40cc160ec4adeafcff40a585'
        ExpectBench = @($tune.Bench, $tune.Bench)
        Dir = Join-Path $PSScriptRoot "results\b22-cat-$Run"
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
