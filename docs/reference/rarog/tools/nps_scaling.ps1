. "$PSScriptRoot\uci_probe.ps1"

# Two middlegame positions. The 2026-07-22 run recorded "two middlegame
# positions" without FENs, so exact reproduction is impossible; these are
# pinned here so future runs ARE reproducible. Kiwipete (heavy tactical
# middlegame) + a quieter closed middlegame.
$POSITIONS = @(
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r1bq1rk1/pp2ppbp/2np1np1/8/2BNP3/2N1BP2/PPPQ2PP/R3K2R w KQ - 0 1"
)
$THREADS = @(1, 2, 4, 8, 16)
$REPS = 2
$MOVETIME = 5000
$HASHMB = 256
$CSV = "$PSScriptRoot\results\scaling.csv"

function Measure-Engine([string]$name, [string]$exe, [string]$threadOpt, [string]$hashOpt) {
    if (-not (Test-Path $CSV)) {
        "engine,threads,position,rep,nps,depth" | Out-File $CSV -Encoding utf8
    }
    foreach ($t in $THREADS) {
        $readings = @()
        for ($pi = 0; $pi -lt $POSITIONS.Count; $pi++) {
            for ($r = 1; $r -le $REPS; $r++) {
                $m = Measure-Search $exe $POSITIONS[$pi] $t $HASHMB $MOVETIME $threadOpt $hashOpt
                if ($null -eq $m -or $null -eq $m.Nps) {
                    "$name,$t,$pi,$r,,0" | Out-File $CSV -Append -Encoding utf8
                    continue
                }
                $readings += $m.Nps
                "$name,$t,$pi,$r,$($m.Nps),$($m.Depth)" | Out-File $CSV -Append -Encoding utf8
            }
        }
        if ($readings.Count -gt 0) {
            $s = $readings | Sort-Object
            $med = if ($s.Count % 2) { $s[[int]($s.Count / 2)] } else { ($s[$s.Count / 2 - 1] + $s[$s.Count / 2]) / 2 }
            "  {0,-16} T={1,-2}  median NPS {2,12:N0}" -f $name, $t, $med
        }
        else { "  {0,-16} T={1,-2}  NO READING" -f $name, $t }
    }
}

