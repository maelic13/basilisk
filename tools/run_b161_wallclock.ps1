<# Frozen launcher for PLAN B.16.1 (BAS-R08): the wall-clock overhead read.
   Basilisk's Release PEXT final-PGO build of the current master source
   against Stockfish 19 on the same host, timed from outside the engine by
   tools/diag/wallclock.py: `go nodes 1000` wall time, `go movetime` 50, 100
   and 300 overshoot, and ponderhit-to-bestmove latency on an expired clock,
   at Threads 1 and 4, Hash 64, both engines' default options. Each engine is
   pinned to the first N game cores (physical cores except CPU 0) at high
   priority. Needs an idle host; it refuses a busy one, and records the
   host's load before and after. About seven minutes. #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$engines = Join-Path $PSScriptRoot 'test_engines'
$basilisk  = @{ Path = Join-Path $engines 'basilisk-b161-master-pext-pgo.exe'; Sha = 'D5E67D0A59EC7DD50630EE81754155A36CDC17647B31C47DC61BC87E6CBDA07A' }
$stockfish = @{ Path = 'D:\chess\engines\stockfish\stockfish-19-windows-x86-64-universal.exe'; Sha = '45BC8E4969147DB9C2EB533810637994619BFF0EACC81CCFD9854394901BCBD0' }

foreach ($engine in @($basilisk, $stockfish)) {
    $actual = Get-HarnessSha256 $engine.Path
    if ($actual -ne $engine.Sha) { throw "PROVENANCE MISMATCH - $($engine.Path) is $actual, not $($engine.Sha)." }
}

$out = Join-Path $PSScriptRoot 'results\b161-wallclock'
$rounds = @('--positions', '50', '--nodes-rounds', '500', '--movetimes', '50,100,300',
            '--movetime-rounds', '100', '--ponder-rounds', '100',
            '--ponder-clock', '1000+10', '--ponder-wait', '500')
if ($DryRun) {
    $out = Join-Path $PSScriptRoot 'results\b161-wallclock-dryrun'
    $rounds = @('--positions', '6', '--nodes-rounds', '6', '--movetimes', '50,100,300',
                '--movetime-rounds', '2', '--ponder-rounds', '2',
                '--ponder-clock', '1000+10', '--ponder-wait', '500')
    if (Test-Path $out) { Remove-Item -Recurse -Force $out }
}
if (Test-Path (Join-Path $out 'runs.jsonl')) { throw "$out already holds a read; move it aside first." }

$before = Assert-HarnessHostIdle -Allow:$DryRun
$cpus = (@(Get-HarnessGameCpus) | Select-Object -First 4).Cpu -join ','
Write-Host "  Pinning to game CPUs $cpus"

$tool = Join-Path $PSScriptRoot 'diag\wallclock.py'
& python $tool --engine "basilisk=$($basilisk.Path)" --engine "sf19=$($stockfish.Path)" `
    --reference sf19 --threads 1,4 --hash 64 --cpus $cpus --out $out @rounds
$code = $LASTEXITCODE
$after = Assert-HarnessHostIdle -Allow -Quiet
@{ before_busy_percent = $before.BusyPercent; before_reasons = $before.Reasons
   after_busy_percent = $after.BusyPercent; after_reasons = $after.Reasons
   basilisk_sha256 = $basilisk.Sha; stockfish_sha256 = $stockfish.Sha; dry_run = [bool]$DryRun } |
    ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $out 'host.json')
if ($code) { exit $code }
Write-Host "Archive: $out (python tools/diag/wallclock.py --report $out)"
