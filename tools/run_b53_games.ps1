<# Frozen launcher for PLAN B.5.3's games (BAS-S53). Every run: 2,000 games (1,000 paired UHO
   openings) at 3+0.03, one thread, Hash 64, no adjudication, tablebases off, on the Colosseum
   match-fixed.toml policy, at one revision, every binary pinned by SHA-256.

   -Run paired  the whole-arm run: the B5_ROOT=ON release build against the OFF build, both final-PGO,
                each pool's build 1, seed 53000. It does not wait on M1.
   -Run width   the width run (§A1.5): the arm's Tune build at B5AspBase=19000 (arm A) against the
                same binary at its defaults (arm B), seed 53001. After the whole-arm run.
   -Run negext  §9 item 1: the Tune build at B5NegExtMax=1 (arm A) against the same binary at its
                defaults (arm B), seed 53002, at the seed's width (the width run left 1410). After
                the width run.
   -Run fhr     §9 item 2: the Tune build at B5FailHighReduce=1 (arm A) against the same binary at
                its defaults (arm B), seed 53003, at 1410 and B5NegExtMax 0 (item 1 left both).
                After item 1. Its WAC veto was read before the games and holds (269 against 263).

   -Smoke plays four games (the harness plays pairs, so four is the least above three) to
   <results>\b53-<run>-smoke, replaced each time. -DryRun plays nothing and checks the run. A game run
   refuses a directory that already holds one, and a run before its predecessor. #>
param(
    [ValidateSet('paired', 'width', 'negext', 'fhr')][string]$Run = 'paired',
    [switch]$DryRun,
    [switch]$Smoke,
    [switch]$CheckOnly,
    [string]$ResultsRoot = '',
    [string]$EnginesDir = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$results = if ($ResultsRoot) { $ResultsRoot } else { Join-Path $PSScriptRoot 'results' }
$engines = if ($EnginesDir) { $EnginesDir } else { Join-Path $PSScriptRoot 'test_engines' }
$revision = 'd07380daa5d707c237e3e6865022f370566b39eb'
$on   = @{ Path = Join-Path $engines 'basilisk-b53-on-pgo1-pext-pgo.exe';       Sha = 'A1D8CE882DD467952BBCCC920D73EE669BEF26535BB71163CAFDAB86F422329F'; Bench = 3920652 }
$off  = @{ Path = Join-Path $engines 'basilisk-b53-off-pgo1-pext-pgo.exe';      Sha = '64E4ECC76528C7C6406879B5F09BA0E5CADD0AA1767B0FA7E64D5E6B38F3FB85'; Bench = 4144790 }
$tune = @{ Path = Join-Path $engines 'basilisk-b53-on-tune-pext-tune-pgo.exe'; Sha = 'D8D9BC6765E19A1645A95763572CD2E8E97099CCEB3C2928346A722EBBE90328'; Bench = 3920652 }
$name = @{ paired = 'b53-paired'; width = 'b53-width'; negext = 'b53-negext'; fhr = 'b53-fhr' }[$Run]
$dir = Join-Path $results $(if ($Smoke) { "$name-smoke" } else { $name })

$used = if ($Run -eq 'paired') { @($on, $off) } else { @($tune) }
foreach ($engine in $used) {
    if (-not (Test-Path -LiteralPath $engine.Path)) { throw "PROVENANCE MISMATCH - no binary at $($engine.Path)." }
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}
# Each run follows its predecessor's completed 2,000 games. Whatever the whole-arm run reads, the
# width run is next: P1's falsifier also sends a read below -20 to the width run before any ablation.
# Item 1 follows the width run, at the width it left, and item 2 follows item 1.
$predecessor = @{ width = 'b53-paired'; negext = 'b53-width'; fhr = 'b53-negext' }[$Run]
if ($predecessor -and -not $Smoke) {
    $previous = Join-Path $results "$predecessor\result.json"
    if (-not (Test-Path -LiteralPath $previous)) { throw "ORDER - $predecessor has not been played." }
    $done = Get-Content -LiteralPath $previous -Raw | ConvertFrom-Json
    if ($done.status -ne 'completed' -or [int]$done.games_completed -ne 2000) { throw "ORDER - $predecessor is not a completed 2,000-game run." }
}
if (-not $Smoke -and -not $DryRun -and (Test-Path -LiteralPath (Join-Path $dir 'result.json'))) {
    throw "STALE RESULT - $dir holds a run; it is not extended or replaced. Move it first."
}
if ($CheckOnly) { Write-Host "GUARDS OK - $Run may run."; exit 0 }
if ($Smoke -and (Test-Path -LiteralPath $dir)) { Remove-Item -LiteralPath $dir -Recurse -Force }

$arguments = @{
    Mode = 'match'
    Games = if ($Smoke) { 4 } else { 2000 }
    Hash = 64
    Threads = 1
    MarginMs = 20
    BaseMs = 3000
    IncrementMs = 30
    ExpectRevision = $revision
    DryRun = $DryRun
    Dir = $dir
}
if ($Run -eq 'paired') {
    $arguments += @{
        EngineA = $on.Path; NameA = 'B5Root'
        EngineB = $off.Path; NameB = 'B4Head'
        Seed = 53000
        ExpectBench = @($on.Bench, $off.Bench)
    }
} elseif ($Run -eq 'width') {
    $arguments += @{
        EngineA = $tune.Path; NameA = 'AspBase19000'
        EngineB = $tune.Path; NameB = 'Defaults'
        OptionsA = @('B5AspBase=19000')
        Seed = 53001
        ExpectBench = @($tune.Bench, $tune.Bench)
    }
} elseif ($Run -eq 'negext') {
    $arguments += @{
        EngineA = $tune.Path; NameA = 'NegExtMax1'
        EngineB = $tune.Path; NameB = 'Defaults'
        OptionsA = @('B5NegExtMax=1')
        Seed = 53002
        ExpectBench = @($tune.Bench, $tune.Bench)
    }
} else {
    $arguments += @{
        EngineA = $tune.Path; NameA = 'FailHighReduce1'
        EngineB = $tune.Path; NameB = 'Defaults'
        OptionsA = @('B5FailHighReduce=1')
        Seed = 53003
        ExpectBench = @($tune.Bench, $tune.Bench)
    }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
