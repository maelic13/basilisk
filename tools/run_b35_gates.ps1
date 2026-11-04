<# Frozen launcher for PLAN B.3.5 (BAS-S43): the Cluster 2 gates. Every
   arm is a Release PEXT final-PGO build pinned by SHA-256; 1T, Hash 64,
   UHO paired, no adjudication, tablebases off.

   -Gate a  SPRT [0,3] nElo (sprt-default.toml) at 3+0.03, cap 2,000 pairs:
            the candidate ON arm (B.3.4's adoptions) against the OFF arm of
            the same revision.
   -Gate b  SPRT [0,3] at 3+0.03, cap 12,000 pairs: the fitted ON arm at
            B.3.3's bake (the candidate: B.3.4 adopted nothing) against the
            unfitted ON arm (c6f7151). The arms
            come from different commits, so each manifest's clean tree and
            fingerprint are checked by the wrapper and both hashes here.
   -Gate c  a direction read, not a gate: 2,000 games of (a)'s pair at
            10+0.1 (match-fixed-ltc.toml). #>
param(
    [Parameter(Mandatory)][ValidateSet('a', 'b', 'c')][string]$Gate,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$candidateRevision = '5f1cdf3eddeda54c5b2f0dab59ac9e6f9140d69c'
$candidate = @{ Path = Join-Path $engines 'basilisk-b35-on-pext-pgo.exe';      Sha = '3DFF7B64533B92FB352A6E0E0B6CD61B03C8E06DFD2DF18FDB5140357C8EA810'; Bench = 5369042 }
$off       = @{ Path = Join-Path $engines 'basilisk-b35-off-pext-pgo.exe';     Sha = '4127849DB8ACD77D5715BADDFECBF81011DE28B333989C20BD054C4A8C05CCE9'; Bench = 11880564 }
# B.3.4 adopted nothing, so (b)'s fitted arm at the bake is the candidate itself.
$fitted    = $candidate
$unfitted  = @{ Path = Join-Path $engines 'basilisk-b32-on-pgo1-pext-pgo.exe'; Sha = '0B5D9DC605D765ECAC802775C83067A6A5454EB8AF21D73DEFEE0E29F77EA0D5'; Bench = 4923507 }

$pair = if ($Gate -eq 'b') { @($fitted, $unfitted) } else { @($candidate, $off) }
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
            Mode = 'sprt'; MaxPairs = 2000
            NameA = 'B3Fitted'; NameB = 'B2Head'; Seed = 43001
            BaseMs = 3000; IncrementMs = 30
            ExpectRevision = $candidateRevision
            Dir = Join-Path $PSScriptRoot 'results\b35-gate-a'
        }
    }
    'b' {
        $arguments += @{
            Mode = 'sprt'; MaxPairs = 12000
            NameA = 'Fitted'; NameB = 'Unfitted'; Seed = 43002
            BaseMs = 3000; IncrementMs = 30
            Dir = Join-Path $PSScriptRoot 'results\b35-gate-b'
        }
    }
    'c' {
        $arguments += @{
            Mode = 'match'; Games = 2000
            NameA = 'B3Fitted'; NameB = 'B2Head'; Seed = 43003
            BaseMs = 10000; IncrementMs = 100
            ExpectRevision = $candidateRevision
            RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-ltc.toml'
            Dir = Join-Path $PSScriptRoot 'results\b35-direction-ltc'
        }
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
