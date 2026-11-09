<# Frozen launcher for PLAN B.5.3's games (BAS-S53). The whole-arm run: 2,000 games (1,000 paired UHO
   openings) at 3+0.03, one thread, Hash 64, no adjudication, tablebases off, on the Colosseum
   match-fixed.toml policy; the B5_ROOT=ON release build against the OFF build of the same revision, both
   final-PGO, each pool's build 1, pinned by SHA-256. It does not wait on M1.

   The width run and the two categoricals follow it in §A1.5's order, in later commits of this launcher.

   -Smoke plays four games (the harness plays pairs, so four is the least above three) to
   <results>\b53-paired-smoke, replaced each time. -DryRun plays nothing and checks the run. A game run
   refuses a directory that already holds one. #>
param(
    [ValidateSet('paired')][string]$Run = 'paired',
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
$on  = @{ Path = Join-Path $engines 'basilisk-b53-on-pgo1-pext-pgo.exe';  Sha = 'A1D8CE882DD467952BBCCC920D73EE669BEF26535BB71163CAFDAB86F422329F'; Bench = 3920652 }
$off = @{ Path = Join-Path $engines 'basilisk-b53-off-pgo1-pext-pgo.exe'; Sha = '64E4ECC76528C7C6406879B5F09BA0E5CADD0AA1767B0FA7E64D5E6B38F3FB85'; Bench = 4144790 }
$dir = Join-Path $results $(if ($Smoke) { 'b53-paired-smoke' } else { 'b53-paired' })

foreach ($engine in @($on, $off)) {
    if (-not (Test-Path -LiteralPath $engine.Path)) { throw "PROVENANCE MISMATCH - no binary at $($engine.Path)." }
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
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
    EngineA = $on.Path; NameA = 'B5Root'
    EngineB = $off.Path; NameB = 'B4Head'
    Seed = 53000
    ExpectBench = @($on.Bench, $off.Bench)
    Dir = $dir
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
