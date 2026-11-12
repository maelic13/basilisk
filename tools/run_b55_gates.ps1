<# Frozen launcher for PLAN B.5.5's gates (BAS-S55). 1T, Hash 64, UHO paired, no
   adjudication, tablebases off, every binary pinned by SHA-256.

   -Gate stop  (c)'s stop bracket, played before B.5.4's fit (maintainer 2026-11-10):
               SPRT [0,3] nElo at 3+0.03, cap 6,000 pairs, seed 55003, on the arm's
               Tune build at the seeds: arm A at B5FailHighStop=200, arm B at its
               default, 100. One binary, so no build offset between the arms. Closed,
               H1; its binary pin is that gate's.
   -Gate a     SPRT [0,3] nElo at 3+0.03, cap 4,500 pairs, seed 55001: the fitted
               arm against the OFF arm, final-PGO Release builds of one revision.
   -Gate d     a direction read, not a gate: 2,000 games of (a)'s pair at 10+0.1
               (match-fixed-ltc.toml), seed 55004. Its census reads the flip's
               long-clock condition (L1) and (e)'s second trigger.
   -Gate e     400 games of (a)'s pair at 60+0.6 (match-fixed-long.toml), seed 55005,
               played only on a trigger and with the maintainer's agreement; it
               refuses until $eAgreed is set in a commit that records both.
   -Gate dcap  L1's registered first remedy, (d) run again (2,000 games at 10+0.1,
               seed 55004, the same openings) with the arm at B5NegExtMax 1, built in;
               both arms rebuilt at one revision, the OFF arm's bench unchanged.
   -Gate acap  (a) on the adopted candidate, the arm at B5NegExtMax 1 (L1 waived by the
               maintainer, 2026-11-12): SPRT [0,3] nElo at 3+0.03, cap 4,500 pairs, seed
               55006, dcap's pinned pair.

   (b) is retired, answered by BAS-S56. -DryRun plays nothing and checks the run. A
   game run refuses a directory that already holds a finished one: a stopped SPRT is
   never extended. #>
param(
    [Parameter(Mandatory)][ValidateSet('stop', 'a', 'd', 'e', 'dcap', 'acap')][string]$Gate,
    [switch]$DryRun,
    [string]$ResultsRoot = '',
    [string]$EnginesDir = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$results = if ($ResultsRoot) { $ResultsRoot } else { Join-Path $PSScriptRoot 'results' }
$engines = if ($EnginesDir) { $EnginesDir } else { Join-Path $PSScriptRoot 'test_engines' }
$eAgreed = $false

if ($Gate -eq 'stop') {
    $revision = 'd07380daa5d707c237e3e6865022f370566b39eb'
    $tune = @{ Path = Join-Path $engines 'basilisk-b53-on-tune-pext-tune-pgo.exe'; Sha = 'D8D9BC6765E19A1645A95763572CD2E8E97099CCEB3C2928346A722EBBE90328'; Bench = 3920652 }
    $pair = @($tune, $tune)
    $dir = Join-Path $results 'b55-gate-c-stop'
    $finished = 'result.json'
} elseif ($Gate -in 'dcap', 'acap') {
    $revision = '833659fb346af4b9ecab0956f81dc3ec1849a6bf'
    $fitted = @{ Path = Join-Path $engines 'basilisk-b55-cap1-on-pext-pgo.exe';  Sha = '122BE50F37E318955F43B01493F85F69A9CE8FCC5900BD970A3C49DE8D51C5AF'; Bench = 3783135 }
    $off    = @{ Path = Join-Path $engines 'basilisk-b55-cap1-off-pext-pgo.exe'; Sha = '17978E36DD29F7FB1FB85CFAAD835EE86F7209222DE9D4A0FDB0234416DC5E85'; Bench = 4144790 }
    $pair = @($fitted, $off)
    $dir = Join-Path $results @{ dcap = 'b55-direction-ltc-cap1'; acap = 'b55-gate-a-cap1' }[$Gate]
    $finished = if ($Gate -eq 'acap') { 'result.json' } else { 'results.txt' }
} else {
    $revision = '2c248bbca73f590909cb0964d1c4843236f95d76'
    $fitted = @{ Path = Join-Path $engines 'basilisk-b55-on-pext-pgo.exe';  Sha = '148104E9BFEF5656A13040699CB2E2EB401D37E1B8346526BA120FC083A43D1A'; Bench = 4585613 }
    $off    = @{ Path = Join-Path $engines 'basilisk-b55-off-pext-pgo.exe'; Sha = '77221608108FF8180D2E0C5EA25889196A1E3B94DFF98E60D234297233CA8B97'; Bench = 4144790 }
    $pair = @($fitted, $off)
    $dir = Join-Path $results @{ a = 'b55-gate-a'; d = 'b55-direction-ltc'; e = 'b55-long-clock' }[$Gate]
    $finished = if ($Gate -eq 'a') { 'result.json' } else { 'results.txt' }
}

if ($Gate -eq 'e' -and -not $eAgreed -and -not $DryRun) {
    throw "ORDER - (e) is played only on a trigger from (d)'s census or the battery, with the maintainer's agreement (BAS-S55)."
}
foreach ($engine in $pair) {
    if (-not (Test-Path -LiteralPath $engine.Path)) { throw "PROVENANCE MISMATCH - no binary at $($engine.Path)." }
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}
# An SPRT writes result.json when it ends and a fixed match results.txt; an interrupted
# run has only its checkpoint, and the same command resumes it.
if (-not $DryRun -and (Test-Path -LiteralPath (Join-Path $dir $finished))) {
    throw "STALE RESULT - $dir holds a finished gate; it is not extended or replaced."
}

$arguments = @{
    EngineA = $pair[0].Path
    EngineB = $pair[1].Path
    ExpectBench = @($pair[0].Bench, $pair[1].Bench)
    ExpectRevision = $revision
    Hash = 64
    Threads = 1
    MarginMs = 20
    DryRun = $DryRun
    Dir = $dir
}
switch ($Gate) {
    'stop' {
        $arguments += @{
            Mode = 'sprt'; MaxPairs = 6000
            NameA = 'Stop200'; NameB = 'Stop100'; Seed = 55003
            OptionsA = @('B5FailHighStop=200')
            BaseMs = 3000; IncrementMs = 30
        }
    }
    'a' {
        $arguments += @{
            Mode = 'sprt'; MaxPairs = 4500
            NameA = 'B5Fitted'; NameB = 'B5Off'; Seed = 55001
            BaseMs = 3000; IncrementMs = 30
        }
    }
    'd' {
        $arguments += @{
            Mode = 'match'; Games = 2000
            NameA = 'B5Fitted'; NameB = 'B5Off'; Seed = 55004
            BaseMs = 10000; IncrementMs = 100
            RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-ltc.toml'
        }
    }
    'acap' {
        $arguments += @{
            Mode = 'sprt'; MaxPairs = 4500
            NameA = 'B5FittedCap1'; NameB = 'B5Off'; Seed = 55006
            BaseMs = 3000; IncrementMs = 30
        }
    }
    'dcap' {
        $arguments += @{
            Mode = 'match'; Games = 2000
            NameA = 'B5FittedCap1'; NameB = 'B5Off'; Seed = 55004
            BaseMs = 10000; IncrementMs = 100
            RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-ltc.toml'
        }
    }
    'e' {
        $arguments += @{
            Mode = 'match'; Games = 400
            NameA = 'B5Fitted'; NameB = 'B5Off'; Seed = 55005
            BaseMs = 60000; IncrementMs = 600
            RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-long.toml'
        }
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
