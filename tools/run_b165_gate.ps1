<# Frozen launcher for PLAN B.16.5 (BAS-R09): helpers read the stop flag at
   every node, so the main thread no longer waits up to 2,048 helper nodes
   before `bestmove`. SPRT on the repair bracket [-5,5] (sprt-repair.toml)
   at 3+0.03, FOUR threads per engine, Hash 64, UHO, no adjudication,
   tablebases off, capped at 1,500 pairs. At one thread the change cannot
   alter a move (the stop flag is set only by a `stop` command there), so
   the gate runs where it acts. Engine A is the repair (fa2b866), engine B
   the head before it (2f91a78); both are Release PEXT final-PGO builds
   with bench 5,361,058, pinned by SHA-256. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-b165-stop-pext-pgo.exe';   Sha = '4AF28C7E03C70B9D59AC4985CCCC5AAF04083240AACEA10EEDE82D14710E8997'; Bench = 5361058 }
$base = @{ Path = Join-Path $engines 'basilisk-b161-master-pext-pgo.exe'; Sha = 'D5E67D0A59EC7DD50630EE81754155A36CDC17647B31C47DC61BC87E6CBDA07A'; Bench = 5361058 }

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 1500
    EngineA = $fix.Path;  NameA = 'PromptStop'
    EngineB = $base.Path; NameB = 'Head'
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 47001
    Hash = 64
    Threads = 4
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\b165-gate'
    DryRun = $DryRun
    # A dry run plays no games, so it may check the configuration beside
    # other work; the real run keeps the idle-host guard.
    AllowBusyHost = [bool]$DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
