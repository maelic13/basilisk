<# Frozen launcher for PLAN B.4.7 (BAS-S52): the Cluster 3 gates. Every arm is a
   PEXT final-PGO build pinned by SHA-256; 1T, Hash 64, UHO paired, no
   adjudication, tablebases off.

   -Gate b  SPRT [0,3] nElo at 3+0.03, cap 12,000 pairs: the fitted ON arm
            against the unfitted ON arm (516d422). The arms come from different
            commits, so each manifest's clean tree and fingerprint are checked
            by the wrapper and both hashes here.
   -Gate c  SPRT [-5,0] at 3+0.03, cap 18,000 pairs, on one ON Tune build: arm A
            the deletion (B4QsDelta 0, the default), arm B the rule (B4QsDelta
            1), so H1 says removal is not worse.
   -Gate a  SPRT [0,3] at 3+0.03, cap 4,500 pairs: the candidate ON arm against
            the OFF arm of the same revision.
   -Gate d  a direction read, not a gate: 2,000 games of (a)'s pair at 10+0.1
            (match-fixed-ltc.toml).

   BAS-S52's order: (b), (c), the maintainer's P9 choice, then (a) and (d). (a)
   and (d) refuse until $candidateFixed is set, in a commit that records (c)'s
   verdict and that choice; (c)'s H0 changes the candidate and re-pins it. A dry
   run plays nothing and is not held to the order. #>
param(
    [Parameter(Mandatory)][ValidateSet('a', 'b', 'c', 'd')][string]$Gate,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$revision = '507fc7ba10487ea9945727965344ea4767f1715e'
$candidate = @{ Path = Join-Path $engines 'basilisk-b47-on-pext-pgo.exe';            Sha = 'AB97234E104F38EB4D2B8C5D9F7A4D708CBDDAF8A6544851F59DCB4B78BE8EF5';   Bench = 4144790 }
$off       = @{ Path = Join-Path $engines 'basilisk-b47-off-pext-pgo.exe';           Sha = 'DB15A497012E23CB31E923F529349AE83A411B0EFA415CE37EABDE1B6BAF2CEA';  Bench = 6090091 }
$tune      = @{ Path = Join-Path $engines 'basilisk-b47-on-tune-pext-tune-pgo.exe';  Sha = 'AA404AB333E4938E60D5A33E73C084D9A4F4C3DDB827714D90DB12B749A151B7'; Bench = 4144790 }
$unfitted  = @{ Path = Join-Path $engines 'basilisk-b45s-on-pgo1-pext-pgo.exe';      Sha = '9C9E463E9AC691860BCF51FFA6D15A72AEDD7375E4615D171FF1A15785915F27'; Bench = 4003560 }
$candidateFixed = $true

if ($Gate -in @('a', 'd') -and -not $candidateFixed -and -not $DryRun) {
    throw "ORDER - (a) and (d) wait for (c)'s verdict and the maintainer's P9 choice (BAS-S52)."
}

$pair = switch ($Gate) {
    'b' { @($candidate, $unfitted) }
    'c' { @($tune, $tune) }
    default { @($candidate, $off) }
}
foreach ($engine in $pair) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    EngineA = $pair[0].Path
    EngineB = $pair[1].Path
    ExpectBench = @($pair[0].Bench, $pair[1].Bench)
    Hash = 64
    Threads = 1
    MarginMs = 20
    DryRun = $DryRun
}
switch ($Gate) {
    'a' {
        $arguments += @{
            Mode = 'sprt'; MaxPairs = 4500
            NameA = 'B4Fitted'; NameB = 'B3Head'; Seed = 52001
            BaseMs = 3000; IncrementMs = 30
            ExpectRevision = $revision
            Dir = Join-Path $PSScriptRoot 'results\b47-gate-a'
        }
    }
    'b' {
        $arguments += @{
            Mode = 'sprt'; MaxPairs = 12000
            NameA = 'Fitted'; NameB = 'Unfitted'; Seed = 52002
            BaseMs = 3000; IncrementMs = 30
            Dir = Join-Path $PSScriptRoot 'results\b47-gate-b'
        }
    }
    'c' {
        $arguments += @{
            Mode = 'sprt'; Bracket = 'removal'; MaxPairs = 18000
            NameA = 'NoDelta'; NameB = 'Delta'; Seed = 52003
            OptionsB = @('B4QsDelta=1')
            BaseMs = 3000; IncrementMs = 30
            ExpectRevision = $revision
            Dir = Join-Path $PSScriptRoot 'results\b47-gate-c'
        }
    }
    'd' {
        $arguments += @{
            Mode = 'match'; Games = 2000
            NameA = 'B4Fitted'; NameB = 'B3Head'; Seed = 52004
            BaseMs = 10000; IncrementMs = 100
            ExpectRevision = $revision
            RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-ltc.toml'
            Dir = Join-Path $PSScriptRoot 'results\b47-direction-ltc'
        }
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
