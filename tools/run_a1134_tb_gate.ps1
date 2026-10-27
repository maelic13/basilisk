<# Frozen launcher for A.13.4 (BAS-S39): the in-search tablebase probe moved
   after the TT cutoff, in Stockfish's order, so its store takes the node's
   own PV flag. The change acts only with tablebases, so this re-reads
   A.8.15's tablebase-enabled gate (BAS-S19) at its registered size: SPRT on
   the repair bracket [-5,5] at 10+0.1 (sprt-repair-ltc.toml), one thread,
   Hash 64, UHO, no adjudication, Syzygy 3-6 men on both engines,
   concurrency 7, capped at 4,000 pairs. Engine A is the repair, engine B the
   head after A.13.3; both are Release PEXT final-PGO builds, pinned by
   SHA-256, and play the bench identically, since bench runs without tables.
   They come from different commits, so the wrapper checks each manifest's
   clean tree and fingerprint and this script pins both hashes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$fix  = @{ Path = Join-Path $engines 'basilisk-a1134-fix-pext-pgo.exe';  Sha = 'F34EE4F9FA063A5C3EA1E58506EFE67B0FB3D3867670B61B418D8520D083C3F8'; Bench = 19470325 }
$base = @{ Path = Join-Path $engines 'basilisk-a1134-base-pext-pgo.exe'; Sha = '4440B8526987606D15745E64F85DB5FD7D60606448B10309D7FAE053E406B4DA'; Bench = 19470325 }
$syzygy = 'D:/chess/tablebases/syzygy3456'

foreach ($engine in @($fix, $base)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}
if (-not (Test-Path -LiteralPath $syzygy)) { throw "Syzygy tables not found at $syzygy." }

$arguments = @{
    Mode = 'sprt'
    RunFile = Join-Path $PSScriptRoot 'colosseum\sprt-repair-ltc.toml'
    MaxPairs = 4000
    EngineA = $fix.Path;  NameA = 'TbAfterCutoff'
    EngineB = $base.Path; NameB = 'Head'
    OptionsA = @("SyzygyPath=$syzygy")
    OptionsB = @("SyzygyPath=$syzygy")
    ExpectBench = @($fix.Bench, $base.Bench)
    Seed = 39001
    Hash = 64
    Threads = 1
    BaseMs = 10000
    IncrementMs = 100
    MarginMs = 20
    Concurrency = 7
    Dir = Join-Path $PSScriptRoot 'results\a1134-tb-gate'
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
