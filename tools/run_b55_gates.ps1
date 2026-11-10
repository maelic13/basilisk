<# Frozen launcher for PLAN B.5.5's gates (BAS-S55). 1T, Hash 64, UHO paired, no
   adjudication, tablebases off, every binary pinned by SHA-256.

   -Gate stop  (c)'s stop bracket, played before B.5.4's fit (maintainer 2026-11-10):
               SPRT [0,3] nElo at 3+0.03, cap 6,000 pairs, seed 55003, on the arm's
               Tune build at the seeds: arm A at B5FailHighStop=200, arm B at its
               default, 100. One binary, so no build offset between the arms.

   The other gates, (a), (b), (d) and what else (c) holds, are added at B.5.5's
   opening, on the fitted binaries.

   -DryRun plays nothing and checks the run. A game run refuses a directory that
   already holds a finished one: a stopped SPRT is never extended. #>
param(
    [Parameter(Mandatory)][ValidateSet('stop')][string]$Gate,
    [switch]$DryRun,
    [string]$ResultsRoot = '',
    [string]$EnginesDir = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$results = if ($ResultsRoot) { $ResultsRoot } else { Join-Path $PSScriptRoot 'results' }
$engines = if ($EnginesDir) { $EnginesDir } else { Join-Path $PSScriptRoot 'test_engines' }
$revision = 'd07380daa5d707c237e3e6865022f370566b39eb'
$tune = @{ Path = Join-Path $engines 'basilisk-b53-on-tune-pext-tune-pgo.exe'; Sha = 'D8D9BC6765E19A1645A95763572CD2E8E97099CCEB3C2928346A722EBBE90328'; Bench = 3920652 }
$dir = Join-Path $results 'b55-gate-c-stop'

if (-not (Test-Path -LiteralPath $tune.Path)) { throw "PROVENANCE MISMATCH - no binary at $($tune.Path)." }
$actual = Get-HarnessSha256 $tune.Path
if ($actual -ne $tune.Sha) { throw "PROVENANCE MISMATCH - $($tune.Path) is $actual, not $($tune.Sha)." }
# The runner writes result.json when an SPRT ends (h1, h0, or inconclusive at the cap);
# an interrupted run has only its checkpoint, and the same command resumes it.
if (-not $DryRun -and (Test-Path -LiteralPath (Join-Path $dir 'result.json'))) {
    throw "STALE RESULT - $dir holds a finished gate; it is not extended or replaced."
}

$arguments = @{
    Mode = 'sprt'; MaxPairs = 6000
    EngineA = $tune.Path; NameA = 'Stop200'
    EngineB = $tune.Path; NameB = 'Stop100'
    OptionsA = @('B5FailHighStop=200')
    Seed = 55003
    Hash = 64
    Threads = 1
    MarginMs = 20
    BaseMs = 3000
    IncrementMs = 30
    ExpectRevision = $revision
    ExpectBench = @($tune.Bench, $tune.Bench)
    DryRun = $DryRun
    Dir = $dir
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
