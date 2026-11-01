<# Frozen launcher for PLAN B.3.2 (BAS-S41): §9 items 1-3 and the whole-arm
   paired run. Each is a 2,000-game match (1,000 paired UHO openings), one
   thread, Hash 64, no adjudication, tablebases off, on the Colosseum
   match-fixed.toml policy; every binary is pinned by SHA-256. Run them in
   BAS-S41's order, one at a time, each on the maintainer's go-ahead.

   -Run CheckExt     item 1 (and item 2's read B): B3CheckExt=1 against the
                     defaults, 3+0.03
   -Run CheckExtLtc  item 1 at 10+0.1
   -Run CheckLegacy  item 2's read A: the legacy check policy (B3CheckExt=1,
                     B3CheckPrune=0) against the defaults, 3+0.03
   -Run NmpAnyNode   item 3: B3NmpCutOnly=0 against the defaults, 3+0.03
   -Run NmpAnyNodeLtc item 3 at 10+0.1
   -Run paired       the B3_PROOF=ON release build against the OFF build of
                     the same revision (both final-PGO), on the defaults items
                     1-3 leave; if they flip a default, this launcher is
                     re-pinned to the rebuilt ON binary first.

   For the categoricals arm A plays the alternative and arm B the defaults on
   one ON Tune build, so a positive result favours the alternative (BAS-S41
   CONDITIONS: adopted at >= +5 and rejected at <= -5 only when the 95%
   interval also excludes 0). #>
param(
    [Parameter(Mandatory)]
    [ValidateSet('CheckExt', 'CheckExtLtc', 'CheckLegacy', 'NmpAnyNode', 'NmpAnyNodeLtc', 'paired')]
    [string]$Run,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$revision = 'c6f7151a9da91a7990d01ed5b4217164b5466c61'
$on   = @{ Path = Join-Path $engines 'basilisk-b32-on-pgo1-pext-pgo.exe';       Sha = '4CE2E74867ECE9A394188F110C937423B1EC93598D2314ABAB93E6745152640D'; Bench = 4923507 }
$off  = @{ Path = Join-Path $engines 'basilisk-b32-off-pgo1-pext-pgo.exe';      Sha = 'A919936F9199F7778BA77CAB7ACDA574E2767BEDC87F7CE6ED29CAFB7DDB8CAD'; Bench = 11880564 }
$tune = @{ Path = Join-Path $engines 'basilisk-b32-on-tune-pext-tune-pgo.exe';  Sha = 'B757C9369EEACCA541DA7457D2C892F7EDD7F6CE9814F245D9535F24F2052F70'; Bench = 4923507 }

# Alternative (arm A) against the defaults (arm B), the control and the seed
# BAS-S41 registered.
$runs = @{
    CheckExt      = @{ Name = 'CheckExt1';     Options = @('B3CheckExt=1');                   Ms = 3000;  Inc = 30;  Seed = 41001 }
    CheckExtLtc   = @{ Name = 'CheckExt1';     Options = @('B3CheckExt=1');                   Ms = 10000; Inc = 100; Seed = 41011 }
    CheckLegacy   = @{ Name = 'CheckLegacy';   Options = @('B3CheckExt=1', 'B3CheckPrune=0'); Ms = 3000;  Inc = 30;  Seed = 41002 }
    NmpAnyNode    = @{ Name = 'NmpCutOnly0';   Options = @('B3NmpCutOnly=0');                 Ms = 3000;  Inc = 30;  Seed = 41003 }
    NmpAnyNodeLtc = @{ Name = 'NmpCutOnly0';   Options = @('B3NmpCutOnly=0');                 Ms = 10000; Inc = 100; Seed = 41013 }
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
        Dir = Join-Path $PSScriptRoot 'results\b32-paired'
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
        Dir = Join-Path $PSScriptRoot "results\b32-cat-$Run"
    }
    # 10+0.1 needs its own run file (the policy check reads the resolved
    # control); -Games 2000 overrides the file's 1,000.
    if ($r.Ms -eq 10000) { $arguments.RunFile = Join-Path $PSScriptRoot 'colosseum\match-fixed-ltc.toml' }
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
