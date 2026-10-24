<# Frozen launcher for BAS-S27: three direction reads of the head against
   Basilisk 1.10.2, at the conditions no gate has read yet.

   Engine 1 is the head (master 4fa68b4: the selectivity core, fitted, with the
   piece-square repair); engine 2 is 1.10.2 rebuilt from its tag with the same
   compiler, anchored at 1500, so engine 1's rating minus 1500 is its Elo over
   1.10.2. Each read is a gauntlet of 500 paired cycles (1,000 games), UHO, no
   adjudication, both binaries Release PEXT final PGO and pinned by SHA-256.

   -Read ltc   10+0.1, one thread, Hash 64, no tablebases
   -Read 4t    3+0.03, four threads, Hash 512, no tablebases
   -Read tb    10+0.1, one thread, Hash 64, Syzygy 3-6 men on both engines #>
param(
    [Parameter(Mandatory)][ValidateSet('ltc', '4t', 'tb')][string]$Read,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$wrapper = Join-Path $PSScriptRoot 'colosseum.ps1'
$engines = Join-Path $PSScriptRoot 'test_engines'
$head = @{ Path = Join-Path $engines 'basilisk-head-4fa68b4-pext-pgo.exe';  Sha = 'FCC39B1E1F66F943046D44EDA7962B8291C156ADC5BD22A7FA3B5E5E3B992669'; Bench = 8437148 }
$base = @{ Path = Join-Path $engines 'basilisk-v1102-clang23-pext-pgo.exe'; Sha = '2282DC135A80192EE25EC2565C10061269989103DFDF5DFF48FFD0D3DA5C3E0E'; Bench = 14978465 }
$syzygy = 'D:/chess/tablebases/syzygy3456'

$reads = @{
    ltc = @{ Seed = 27001; BaseMs = 10000; IncrementMs = 100; Threads = 1; Hash = 64;  Tb = $false }
    '4t' = @{ Seed = 27002; BaseMs = 3000;  IncrementMs = 30;  Threads = 4; Hash = 512; Tb = $false }
    tb  = @{ Seed = 27003; BaseMs = 10000; IncrementMs = 100; Threads = 1; Hash = 64;  Tb = $true }
}
$r = $reads[$Read]

$extra = @(
    '--format', 'gauntlet', '--seeds', '1', '--cycles', '500', '--games-per-pair', '2',
    '--max-engine-faults', '0',
    '--engine', $head.Path, '--engine', $base.Path,
    '--label', 'Basilisk head', '--label', 'Basilisk 1.10.2',
    '--rating', '1500', '--rating', '1500', '--fixed', '2:1500'
)
if ($r.Tb) {
    if (-not (Test-Path -LiteralPath $syzygy)) { throw "Syzygy tables not found at $syzygy." }
    $extra += @('--engine-option', "1:SyzygyPath=$syzygy", '--engine-option', "2:SyzygyPath=$syzygy")
}

$arguments = @{
    Mode = 'gauntlet'
    Dir = (Join-Path $PSScriptRoot "results\head-vs-1102-$Read")
    RunFile = (Join-Path $PSScriptRoot $(if ($r.BaseMs -eq 10000) { 'colosseum\gauntlet-ltc.toml' } else { 'colosseum\gauntlet.toml' }))
    Seed = $r.Seed
    Hash = $r.Hash
    Threads = $r.Threads
    BaseMs = $r.BaseMs
    IncrementMs = $r.IncrementMs
    MarginMs = 20
    ExpectBench = @($head.Bench, $base.Bench)
    ExpectSha256 = @($head.Sha, $base.Sha)
    ThreadOptionNames = @('Threads', 'Threads')
    ExtraArgs = $extra
    DryRun = $DryRun
}
& $wrapper @arguments
if ($LASTEXITCODE) { exit $LASTEXITCODE }
