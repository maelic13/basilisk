<# Frozen launcher for PLAN B.16.3 (BAS-M14): where Basilisk loses, by cell.
   Stockfish 19 at 150,000 nodes judges every move of three stored sources,
   one judge per game core (physical cores except CPU 0), normal priority:
     m11  the 4T gauntlet of Basilisk 1.10.1 (BAS-M11), 2,400 games, exported
          read-only from the Colosseum app's database;
     b36  the first 2,000 games of B.3.6's 1T gate (both arms the current
          engine but for the coloured histories);
     srt  Basilisk 1.10.0's 8,200 games of the Super Rating Tournament
          (BAS-M08), which carry no clock or node annotations.
   Then the 100 worst open errors of m11 and of b36 are searched again by the
   binaries that played them at 1, 4 and 16 times their game nodes. Every
   stage resumes where it stopped: run the same command again. About five
   hours on 15 cores; the srt stage is the last and the longest (about three).
   It needs a host free of other engine work (it refuses a busy one). #>
param([switch]$DryRun)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')
$tool = Join-Path $PSScriptRoot 'diag\loss_by_cell.py'
$engines = Join-Path $PSScriptRoot 'test_engines'
$inputs = @(
    @{ Path = 'D:\chess\engines\stockfish\stockfish-19-windows-x86-64-universal.exe'; Sha = '45BC8E4969147DB9C2EB533810637994619BFF0EACC81CCFD9854394901BCBD0' }
    @{ Path = Join-Path $PSScriptRoot 'results\b163-loss\sources\m11-gauntlet-4t.pgn'; Sha = 'E4E3379A4C179F85CC807522376CCB24CC6A278EAF361C3B7AD318861BBC89FB' }
    @{ Path = Join-Path $PSScriptRoot 'results\b36-gate\games.pgn'; Sha = 'A38F1459DB00219C2754921CF30B8F1027C5526F743BEE22EFEA31879349E96F' }
    @{ Path = 'D:\chess\results\super_rating_tournament.pgn'; Sha = '4E87A36A030DFC696C9328F9F34F60B94784A5303AF4C8BE92D2DB6B2C05103C' }
    @{ Path = Join-Path $engines 'basilisk-1.10.1-rc4-pext-pgo.exe'; Sha = 'C1F9DC2DBC3E15554C4A8EE783A7520ED6EA263A5F8A82315590FCC11797890A' }
    @{ Path = Join-Path $engines 'basilisk-b36-colour-on-pext-pgo.exe'; Sha = '0E2C93E28C2AB65C567C18A4175743168E85F0F95E56979C4C54C574EDC092D0' }
    @{ Path = Join-Path $engines 'basilisk-b36-colour-off-pext-pgo.exe'; Sha = '21CF9C560ED407F59EA910B69AB9DBDC8D9D5F54E59FFCD1D587E7AE3472C479' }
)
foreach ($item in $inputs) {
    $actual = Get-HarnessSha256 $item.Path
    if ($actual -ne $item.Sha) { throw "PROVENANCE MISMATCH - $($item.Path) is $actual, not $($item.Sha)." }
}
$judge = $inputs[0].Path

$out = Join-Path $PSScriptRoot 'results\b163-loss\judged'
$nodes = '150000'
$first = @{ m11 = @(); b36 = @('--first', '2000'); srt = @('--only', '^Basilisk 1\.10\.0$') }
$top = '100'
if ($DryRun) {
    $out = Join-Path $PSScriptRoot 'results\b163-loss-dryrun'
    if (Test-Path $out) { Remove-Item -Recurse -Force $out }
    $nodes = '20000'
    $first = @{ m11 = @('--first', '2'); b36 = @('--first', '2'); srt = @('--only', '^Basilisk 1\.10\.0$', '--first', '2') }
    $top = '2'
}

$before = Assert-HarnessHostIdle -Allow:$DryRun
$cpus = @(Get-HarnessGameCpus | ForEach-Object { $_.Cpu })
$cpuList = $cpus -join ','
$workers = "$($cpus.Count)"
Write-Host "  $workers judges on CPUs $cpuList, $nodes nodes"

function Invoke-Stage([string]$Name, [string[]]$Arguments) {
    Write-Host "== $Name"
    & python $tool @Arguments
    if ($LASTEXITCODE) { throw "stage $Name failed with exit $LASTEXITCODE" }
}

$sources = @{
    m11 = $inputs[1].Path
    b36 = $inputs[2].Path
    srt = $inputs[3].Path
}
$common = @('--judge', $judge, '--nodes', $nodes, '--workers', $workers, '--cpus', $cpuList, '--out', $out)
foreach ($source in 'm11', 'b36') {
    Invoke-Stage "judge $source" (@('judge', '--pgn', $sources[$source], '--source', $source) + $first[$source] + $common)
}
Invoke-Stage 'research m11' (@('research', '--source', 'm11', '--top', $top,
    '--engine', "Basilisk 1.10.1=$($inputs[4].Path)") + $common)
Invoke-Stage 'research b36' (@('research', '--source', 'b36', '--top', $top,
    '--engine', "basilisk-b36-colour-on-pext-pgo=$($inputs[5].Path)",
    '--engine', "basilisk-b36-colour-off-pext-pgo=$($inputs[6].Path)") + $common)
Invoke-Stage 'tables' @('tables', '--out', $out)
Invoke-Stage 'judge srt' (@('judge', '--pgn', $sources['srt'], '--source', 'srt') + $first['srt'] + $common)
Invoke-Stage 'tables' @('tables', '--out', $out)

$after = Assert-HarnessHostIdle -Allow -Quiet
@{ before_busy_percent = $before.BusyPercent; before_reasons = $before.Reasons
   after_busy_percent = $after.BusyPercent; after_reasons = $after.Reasons
   judge_nodes = $nodes; workers = $workers; cpus = $cpuList; dry_run = [bool]$DryRun } |
    ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $out 'host.json')
Write-Host "Archive: $out (python tools/diag/loss_by_cell.py tables --out $out)"
