<#
.SYNOPSIS
    Recompute a match result (Elo, LOS, pentanomial) directly from a PGN.

.DESCRIPTION
    THE PGN IS THE DURABLE RECORD; THE CONSOLE LOG IS NOT. Learned 2026-07-25:
    `sprt.ps1` pipes fastchess through `Tee-Object`, which BUFFERS. A run whose
    log has not been flushed for hours can be perfectly healthy — during the
    (k)+(m) gate the log stopped at game 3,136 while fastchess went on to play
    28,362, and the buffered tail was lost when the machine was power-cycled.
    A stalled log is NOT evidence of a stalled run; check the PGN's game count
    before concluding anything died.

    fastchess writes the PGN itself, per game, independently of the PowerShell
    pipeline — so it survives a lost console, a killed shell, or a hard reset.
    This script turns that file back into the verdict.

    Pairs are grouped by the `[Round]` tag rather than by file order, because
    with `-concurrency N` games finish out of order and adjacent entries are
    NOT a colour-reversed pair.

.PARAMETER Pgn
    Path to the PGN.

.PARAMETER Engine
    Name (as it appears in the White/Black tags) to score FROM the perspective
    of. Defaults to the first engine seen.

.EXAMPLE
    ./tools/pgn_result.ps1 -Pgn tools\results\sprt_a_vs_b_20260725_153910.pgn
#>
param(
    [Parameter(Mandatory)][string]$Pgn,
    [string]$Engine = ""
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $Pgn)) { throw "PGN not found: $Pgn" }

$rounds = @{}
$curRound = $null
$curWhite = $null
$seen = @{}

foreach ($m in (Select-String -Path $Pgn -Pattern '^\[(Round|White|Black|Result) "([^"]*)"\]' -AllMatches).Matches) {
    $tag = $m.Groups[1].Value
    $val = $m.Groups[2].Value
    switch ($tag) {
        'Round' { $curRound = $val }
        'White' { $curWhite = $val; $seen[$val] = $true }
        'Black' { $seen[$val] = $true }
        'Result' {
            if ($Engine -eq "") { $Engine = $curWhite }
            $pts = switch ($val) {
                '1-0' { if ($curWhite -eq $Engine) { 1.0 } else { 0.0 } }
                '0-1' { if ($curWhite -eq $Engine) { 0.0 } else { 1.0 } }
                '1/2-1/2' { 0.5 }
                default { $null }   # '*' = unfinished, excluded
            }
            if ($null -ne $pts) {
                if (-not $rounds.ContainsKey($curRound)) { $rounds[$curRound] = @() }
                $rounds[$curRound] += $pts
            }
        }
    }
}

$pairs = @($rounds.Values | Where-Object { $_.Count -eq 2 } | ForEach-Object { $_[0] + $_[1] })
$partial = @($rounds.Values | Where-Object { $_.Count -ne 2 }).Count
if ($pairs.Count -lt 2) { throw "Only $($pairs.Count) complete pair(s) found — nothing to report." }

$penta = @(0, 0, 0, 0, 0)
foreach ($p in $pairs) { $penta[[int]($p * 2)]++ }

$n = $pairs.Count
$mean = ($pairs | Measure-Object -Average).Average
$acc = 0.0
foreach ($p in $pairs) { $acc += ($p - $mean) * ($p - $mean) }
$sd = [Math]::Sqrt($acc / ($n - 1))
$mu = $mean / 2.0
$seMu = ($sd / [Math]::Sqrt($n)) / 2.0

# Pentanomial SE, then propagate through the logistic Elo curve.
$elo = -400.0 * [Math]::Log10(1.0 / $mu - 1.0)
$deriv = 400.0 / ([Math]::Log(10) * $mu * (1 - $mu))
$ci = 1.96 * $seMu * $deriv
$z = ($mu - 0.5) / $seMu
$los = 0.5 * (1.0 + [Math]::Sign($z) * [Math]::Sqrt(1.0 - [Math]::Exp(-2.0 * $z * $z / [Math]::PI)))

"== $(Split-Path $Pgn -Leaf) =="
"perspective    : $Engine    (engines seen: $(($seen.Keys | Sort-Object) -join ', '))"
"complete pairs : {0:N0}   ({1:N0} games)" -f $n, ($n * 2)
if ($partial -gt 0) { "partial rounds : $partial (excluded)" }
"score          : {0:N1} / {1:N0} = {2:P3}" -f ($pairs | Measure-Object -Sum).Sum, ($n * 2), $mu
"Elo            : {0:N2} +/- {1:N2}   (95%)" -f $elo, $ci
"LOS            : {0:P2}" -f $los
"Ptnml(0-2)     : [{0}]" -f ($penta -join ', ')
"draw pairs     : {0:P1}" -f ($penta[2] / $n)
