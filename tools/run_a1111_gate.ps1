<# Frozen launcher for A.11.11 (BAS-S36): fail-soft quiescence in
   Stockfish's form. SPRT on the repair bracket [-5,5] (sprt-repair.toml) at
   3+0.03, one thread, Hash 64, UHO, no adjudication, tablebases off, capped
   at 4,000 pairs. Engine A is fail-soft quiescence on the A.11.10 repair,
   engine B the A.11.10 repair (BAS-S35's candidate). Run it only after
   BAS-S35 keeps the repair. Both are Release PEXT final-PGO builds with
   B2_CORE on, pinned by SHA-256; they come from different commits, so the
   wrapper checks each manifest's clean tree and fingerprint and this
   script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$soft = @{ Path = Join-Path $engines 'basilisk-a1111-fs-pext-pgo.exe';  Sha = '1F8A2058322ABC25514B1B14FE12573BF0FE35A99876B916F15554AEE8C6A43D'; Bench = 16092165 }
$base = @{ Path = Join-Path $engines 'basilisk-a1110-fix-pext-pgo.exe'; Sha = 'C8F6296C378A6F6FB2B388C01BDA63DC0C2EA3747FED3BA3FD31318A8BA5A511'; Bench = 8913394 }

foreach ($engine in @($soft, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$arguments = @{
    Mode = 'sprt'
    Bracket = 'repair'
    MaxPairs = 4000
    EngineA = $soft.Path; NameA = 'FailSoft'
    EngineB = $base.Path; NameB = 'TierBase'
    ExpectBench = @($soft.Bench, $base.Bench)
    Seed = 36001
    Hash = 64
    Threads = 1
    BaseMs = 3000
    IncrementMs = 30
    MarginMs = 20
    Dir = Join-Path $PSScriptRoot 'results\a1111-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
