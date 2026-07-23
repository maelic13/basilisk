<#
.SYNOPSIS
    Run an SPRT self-play match between two Basilisk binaries using fastchess.

.DESCRIPTION
    Starts a fastchess match with the built-in SPRT stopping rule.  The match
    runs until the test accepts H0 (no meaningful improvement) or H1
    (improvement).  Real-time output is printed to the console.

    Tooling:
      - fastchess (NOT cutechess-cli): faster, no Qt dependency, built-in SPRT.
        Download a release from https://github.com/Disservin/fastchess/releases
        and place it at $FastchessPath (default tools\bin\fastchess.exe), or
        pass -FastchessPath. The cutechess GUI is still handy for *viewing*
        the resulting PGNs, but is not used to run matches.

    Conditions (unified with SPSA as of 2026-06-17):
      - tc=3+0.03 -> 3 s + 30 ms/move increment, CLOCK-based (default $TC).
                   This is the same TC the SPSA tuner uses, so there is no
                   tune->confirm transfer gap, and it exercises the real
                   clock time-management code. 1% increment follows the
                   Stockfish convention and reaches a representative depth.
      - Pass -MoveTime 0.1 for the optional fixed 100 ms/move sanity gauntlet
        under the old Little Blitzer-style condition. This disables the clock.
      - LTC confirmation runs at tc=10+0.1 (pass -TC "10+0.1") at phase
        boundaries and for TC-suspect features.
      - Hash 64 MB, Threads 1, UHO_Lichess_4852_v1.epd opening book (random order),
        each opening played from both colours (-games 2 -repeat).
      - model=normalized (nElo) — fastchess default, more time-control-robust
        than logistic Elo.

    IMPORTANT — affinity and calibration (2026-07-21 incident): a null pair
    measured +9.34 +/- 8.20 over only 2,566 games. That single 95% interval is
    evidence worth investigating, but it does NOT establish a persistent
    +9.34 Elo harness bias; nonzero null estimates and occasional 95% misses
    are expected. The old follow-up used SPRT [-3,+3], whose true value 0 lies
    exactly between the hypotheses, so expecting it to accept H0 was also a
    statistical error.

    Two real implementation hazards were found instead: fastchess versions
    before 1.7.0 did not apply process affinity correctly on Windows, and the
    1.8.0 Windows auto-topology code groups a whole processor package and then
    guesses SMT siblings from alternating CPU IDs. The harness now requires
    >=1.7.0, discovers physical cores through the OS, passes an explicit CPU
    list, and hard-fails affinity warnings. Re-validate harness changes with
    -Mode calibrate: PASS requires the entire 95% nElo CI inside the tolerance.

    IMPORTANT — concurrency:
      In a self-play game only the side to move computes, so ~16 concurrent
      games already saturate 16 physical cores. Oversubscribing halves NPS and
      changes the depth reached, distorting results. Keep -Concurrency <=
      physical cores. The default is detected physical cores minus two, leaving
      capacity for the OS; do not size this from logical processors.

    CALIBRATION CHECK — run this FIRST, before testing any feature:
        ./tools/sprt.ps1 `
            -EngineA "tools\test_engines\basilisk-v1.4.9-copy.exe" `
            -EngineB "tools\test_engines\basilisk-v1.4.9.exe" `
            -NameA "Self" -NameB "Self2" -Mode calibrate
        Calibration is fixed-size, not SPRT: truth=0 lies midway between
        [-3,+3], so that old SPRT had no preferred hypothesis and was not a
        valid harness test.

.PARAMETER EngineA
    Path to the new/candidate engine (usually in D:\chess\engines\test_engines).

.PARAMETER EngineB
    Path to the baseline engine (the current integration head, or a released
    reference in D:\chess\engines).

.PARAMETER NameA / NameB
    Display names. Defaults: "New" / "Base".

.PARAMETER Mode
    "gainer"       -> H0: elo<=0,  H1: elo>=Elo1  (default; test a real gain).
    "simplify"     -> H0: elo<=-5, H1: elo>=0     (non-regression / cleanup).
    "calibrate"    -> fixed-size identical-binary null match; no SPRT. PASS =
                      whole 95% nElo CI inside +/-CalibrationTolerance.
    "fixed"        -> fixed-size match (no SPRT stopping rule), reports the
                      final Elo/nElo CI and STOPS — the harness makes no
                      accept/reject decision. This is the 8.6.8A accept-audit
                      instrument: an equivalence / feature-value question has
                      its truth at or near a bound, where SPRT stalls, so it is
                      judged by the ESTIMATE at a fixed N (calibration doctrine).
                      Pass -Games (e.g. 10000, Elo CI ~ +/-4.2) and, for a
                      knob probe, differing -OptionsA/-OptionsB on one binary.
    The explicit -Elo0/-Elo1 parameters override the mode if supplied
    (ignored by calibrate/fixed, which run no SPRT).

.PARAMETER OptionsA
    Extra UCI options for engine A as "Name=Value" strings (each becomes an
    fastchess `option.Name=Value`), recorded in the manifest. Lets a single
    binary be A/B-tested on a UCI knob without a rebuild -- e.g.
    `-OptionsA TmInstability=0` to measure a feature's off-switch. When A and B
    are the same binary, the option sets MUST differ (else it is a degenerate
    null -> use -Mode calibrate).

.PARAMETER OptionsB
    Extra UCI options for engine B, same form as -OptionsA (default: none).

.PARAMETER Elo1
    Upper SPRT bound for "gainer" mode. Default 5 (nElo). Use 3 for small,
    incremental features (e.g. a single tuned search constant) to demand a
    cleaner signal.

.PARAMETER Hash
    Hash MB per engine. Default 64 (matches deployment).

.PARAMETER Concurrency
    Parallel games. Default 0 auto-detects physical cores and leaves two free.

.PARAMETER Games
    Fixed game count for -Mode calibrate and -Mode fixed. Default 30000; pass
    e.g. 10000 for an 8.6.8A probe. Must be even. In calibrate, too wide a
    final interval is an explicit fail; in fixed, the CI is simply reported.

.PARAMETER CalibrationTolerance
    Absolute nElo tolerance for calibration. Default 5. PASS requires the full
    reported 95% confidence interval to fit inside [-tolerance,+tolerance].

.PARAMETER Seed
    Opening randomization seed. Default 0 generates a seed and records it in
    the manifest; pass the recorded value to reproduce the opening order.

.PARAMETER TC
    Clock time control "base+inc" in seconds. Default "3+0.03" (the unified
    SPSA/SPRT TC). Use "10+0.1" for an LTC phase gate. Ignored if -MoveTime is
    supplied.

.PARAMETER MoveTime
    Fixed seconds-per-move. Default 0 (use clock TC instead). Set 0.1 for the
    optional fixed 100 ms/move sanity gauntlet; this disables clock time
    management.

.PARAMETER TimeMargin
    fastchess timeout margin in milliseconds. Default 20. This absorbs small
    Windows scheduler / process IO jitter without changing the engine budget.

.PARAMETER Book
    Opening book. Default tools\books\UHO_Lichess_4852_v1.epd (repo-local,
    gitignored; a backup copy lives in D:\chess\books). The Stockfish/
    OpenBench standard Unbalanced Human Openings set: ~2.6M positions curated to
    a built-in ~+0.5..+1.0 imbalance, so games are decisive and each carries far
    more SPRT signal than a balanced book). Format (.epd/.pgn) is auto-detected
    from the extension. Pass a .pgn (e.g. the old SuperGM/IM books) to override.

.PARAMETER FastchessPath
    Path to fastchess.exe. Default D:\chess\fastchess\fastchess.exe (or found on PATH).

.EXAMPLE
    ./tools/sprt.ps1 `
        -EngineA "tools\test_engines\basilisk-phase867-nocheckext-pext-pgo.exe" `
        -EngineB "tools\test_engines\basilisk-head-pext-pgo.exe" `
        -NameA "Phase1" -NameB "Head" -Elo1 3
#>
param(
    [Parameter(Mandatory)][string]$EngineA,
    [Parameter(Mandatory)][string]$EngineB,
    [string]$NameA = "New",
    [string]$NameB = "Base",
    [ValidateSet("gainer", "simplify", "calibrate", "fixed")][string]$Mode = "gainer",
    [string[]]$OptionsA = @(),
    [string[]]$OptionsB = @(),
    [Nullable[int]]$Elo0 = $null,
    [Nullable[int]]$Elo1 = $null,
    [double]$Alpha = 0.05,
    [double]$Beta  = 0.05,
    [int]$Hash = 64,
    [int]$Concurrency = 0,
    [int]$Games = 30000,
    [double]$CalibrationTolerance = 5,
    [int]$Seed = 0,
    [string]$TC = "3+0.03",
    [double]$MoveTime = 0,
    [int]$TimeMargin = 20,
    [string]$Book = "$PSScriptRoot\books\UHO_Lichess_4852_v1.epd",
    [string]$FastchessPath = "$PSScriptRoot\bin\fastchess.exe"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "..\harness_common.ps1")

function Get-Sha256($path) { if (Test-Path $path) { (Get-FileHash $path -Algorithm SHA256).Hash } else { "missing" } }

$concurrencyInfo = Resolve-HarnessConcurrency -Requested $Concurrency
$Concurrency = $concurrencyInfo.Concurrency
$AffinityCpus = Get-HarnessAffinityCpuList -Concurrency $Concurrency
$Seed = New-HarnessSeed -Requested $Seed

if ($Mode -eq "calibrate" -or $Mode -eq "fixed") {
    if ($Games -lt 2 -or ($Games % 2) -ne 0) { throw "-Games must be a positive even number." }
    if ($Mode -eq "calibrate" -and $CalibrationTolerance -le 0) { throw "-CalibrationTolerance must be positive." }
}

# Normalize option lists into fastchess `option.Name=Value` args, and a stable
# string for the identical-binary guard below.
$optArgsA = @($OptionsA | Where-Object { $_ } | ForEach-Object { "option.$_" })
$optArgsB = @($OptionsB | Where-Object { $_ } | ForEach-Object { "option.$_" })
$sameOptions = (($optArgsA -join ';') -eq ($optArgsB -join ';'))

# Resolve SPRT bounds from mode unless explicitly overridden.
if ($null -eq $Elo0) { $Elo0 = if ($Mode -eq "simplify") { -5 } else { 0 } }
if ($null -eq $Elo1) { $Elo1 = if ($Mode -eq "simplify") {  0 } else { 5 } }

# Resolve the time control: clock (default) unless a fixed movetime is given.
if ($MoveTime -gt 0) {
    $tcArg   = "st=$MoveTime"
    $tcLabel = "st=$MoveTime (fixed ${MoveTime}s/move)"
} else {
    $tcArg   = "tc=$TC"
    $tcLabel = "tc=$TC (clock)"
}

# Locate fastchess.
$fastchess = $FastchessPath
if (-not (Test-Path $fastchess)) {
    $onPath = Get-Command fastchess -ErrorAction SilentlyContinue
    if ($onPath) { $fastchess = $onPath.Source }
    else {
        throw "fastchess not found at '$FastchessPath' or on PATH. Download from " +
              "https://github.com/Disservin/fastchess/releases and place it there."
    }
}
foreach ($p in @($EngineA, $EngineB, $Book)) {
    if (-not (Test-Path $p)) { throw "Not found: $p" }
}

$EngineA = (Resolve-Path $EngineA).Path
$EngineB = (Resolve-Path $EngineB).Path
$Book    = (Resolve-Path $Book).Path

$shaA = Get-Sha256 $EngineA
$shaB = Get-Sha256 $EngineB
if ($Mode -eq "calibrate") {
    if ($shaA -ne $shaB) {
        throw "Calibration requires byte-identical engine binaries (SHA-256 differs)."
    }
    if (-not $sameOptions) {
        throw "Calibration requires identical UCI options on both sides (a true null). Use -Mode fixed to A/B-test a knob."
    }
} elseif ($shaA -eq $shaB -and $sameOptions) {
    # Same binary AND same options => truth is exactly 0, a degenerate null an
    # SPRT can never resolve. A knob probe (-Mode fixed with differing options)
    # is the legitimate same-binary case and is allowed.
    throw "Identical binaries with identical options is a degenerate null: use -Mode calibrate, or pass differing -OptionsA/-OptionsB (with -Mode fixed) to A/B-test a UCI knob."
}

# fastchess needs the opening format told explicitly; derive it from the file
# extension so .epd (UHO and friends) and .pgn books both just work.
$bookFormat = if ([IO.Path]::GetExtension($Book) -ieq ".epd") { "epd" } else { "pgn" }

$resultsDir = Join-Path $PSScriptRoot "results"
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$pgnOut    = Join-Path $resultsDir "sprt_${NameA}_vs_${NameB}_${timestamp}.pgn"
$logOut    = Join-Path $resultsDir "sprt_${NameA}_vs_${NameB}_${timestamp}.log"

# Reproducibility manifest (PLAN §1 gate 8): a PGN without this manifest is not
# a reproducible test. Emitted next to the PGN before the match starts.
$manifestPath = [IO.Path]::ChangeExtension($pgnOut, ".manifest.txt")
function Get-EngineBench($exe) {
    $out = ("bench`nquit" | & $exe 2>&1)
    ($out | Select-String -Pattern 'Nodes searched\s*:\s*(\d+)').Matches.Groups[1].Value
}
# 8.6.5a: A/B compiler equality -- the C++ analog of a toolchain pin. A silent
# compiler upgrade between building engine A and engine B folds compiler delta
# into a +/-3 Elo measurement, and the bench fingerprint cannot see it (node
# counts are compiler-independent; only NPS shifts). Each build_test.ps1 binary
# carries its compiler line in a sidecar manifest: copy both beside the result
# so every run is self-describing (Rarog 9.7), and HARD-FAIL on a mismatch.
# Warn-not-fail when a manifest is missing (pre-8.6 binaries).
function Get-BuildManifest($enginePath) {
    $m = $enginePath -replace '\.exe$', '.manifest.txt'
    if (Test-Path $m) { $m } else { $null }
}
$manA = Get-BuildManifest $EngineA
$manB = Get-BuildManifest $EngineB
foreach ($pair in @(@($manA, $NameA), @($manB, $NameB))) {
    if ($pair[0]) {
        Copy-Item $pair[0] (Join-Path $resultsDir ("sprt_${NameA}_vs_${NameB}_${timestamp}." + $pair[1] + ".manifest.txt")) -Force
    } else {
        Write-Warning "No build manifest for $($pair[1]) (pre-8.6 binary?) -- compiler equality not checkable."
    }
}
if ($manA -and $manB) {
    $compA = (Select-String -Path $manA -Pattern '^compiler:\s*(.+)$').Matches.Groups[1].Value.Trim()
    $compB = (Select-String -Path $manB -Pattern '^compiler:\s*(.+)$').Matches.Groups[1].Value.Trim()
    if ($compA -and $compB -and ($compA -ne $compB)) {
        throw "COMPILER MISMATCH between A and B:`n  $NameA -> $compA`n  $NameB -> $compB`nRebuild one side so the pair is compiler-identical (8.6.5a)."
    }
    Write-Host "Compiler equality OK: $compA"
}

$fcInfo    = Assert-AffinityFastchess -Path $fastchess
$fcVersion = $fcInfo.Text
$repoSha   = (git rev-parse HEAD 2>$null); if (-not $repoSha) { $repoSha = "n/a" } else { $repoSha = $repoSha.Trim() }
@(
    "sprt_mode:     $Mode"
    "engineA:       $NameA = $EngineA"
    "engineA_sha256: $shaA"
    "engineA_bench: $(Get-EngineBench $EngineA)"
    "engineB:       $NameB = $EngineB"
    "engineB_sha256: $shaB"
    "engineB_bench: $(Get-EngineBench $EngineB)"
    "repo_revision: $repoSha"
    "test_design:   $(switch ($Mode) { 'calibrate' { "fixed ${Games}-game null; tolerance +/-${CalibrationTolerance} nElo" } 'fixed' { "fixed ${Games}-game probe; estimate-judged, no SPRT (8.6.8A)" } default { "SPRT elo0=$Elo0 elo1=$Elo1 alpha=$Alpha beta=$Beta model=normalized" } })"
    "optionsA:      $(if ($optArgsA.Count) { $optArgsA -join ' ' } else { '(none)' })"
    "optionsB:      $(if ($optArgsB.Count) { $optArgsB -join ' ' } else { '(none)' })"
    "time_control:  $tcLabel  timemargin=${TimeMargin}ms"
    "hash_mb:       $Hash"
    "threads:       1"
    "concurrency:   $Concurrency"
    "physical_cores: $($concurrencyInfo.PhysicalCores)"
    "affinity_cpus: $AffinityCpus (one logical CPU per physical core)"
    "book:          $Book"
    "book_sha256:   $(Get-Sha256 $Book)"
    "opening_order: random"
    "opening_seed:  $Seed"
    "adjudication:  draw(mn=40,mc=8,score=10) resign(mc=3,score=600,twosided)"
    "fastchess:     $fcVersion"
    "fastchess_sha256: $(Get-Sha256 $fastchess)"
    "pgn:           $pgnOut"
    "started_utc:   $((Get-Date).ToUniversalTime().ToString('u'))"
) | Set-Content -Path $manifestPath -Encoding utf8

Write-Host ""
Write-Host "======================================================="
Write-Host "  SPRT ($Mode): $NameA  vs  $NameB"
Write-Host "  Manifest: $manifestPath"
if ($Mode -eq "calibrate") {
    Write-Host "  Fixed null calibration: $Games games; 95% nElo CI must fit inside +/-$CalibrationTolerance"
} elseif ($Mode -eq "fixed") {
    Write-Host "  Fixed probe: $Games games; estimate-judged (no SPRT stopping rule) — 8.6.8A"
} else {
    Write-Host "  H0: elo<=$Elo0   H1: elo>=$Elo1   alpha=$Alpha  beta=$Beta  (nElo)"
}
if ($optArgsA.Count) { Write-Host "  OptionsA: $($optArgsA -join ' ')" }
if ($optArgsB.Count) { Write-Host "  OptionsB: $($optArgsB -join ' ')" }
Write-Host "  TC: $tcLabel   Margin: ${TimeMargin} ms   Hash: ${Hash} MB   Conc: $Concurrency"
Write-Host "  CPUs: $AffinityCpus"
Write-Host "  Book: $(Split-Path $Book -Leaf)"
Write-Host "  Runner: $fastchess"
Write-Host "  PGN:  $pgnOut"
Write-Host "  Log:  $logOut  (full output; console shows report blocks only)"
Write-Host "======================================================="
Write-Host ""

# calibrate and fixed run a fixed number of rounds with NO SPRT stopping rule.
$fixedSize = ($Mode -eq "calibrate" -or $Mode -eq "fixed")
$rounds = if ($fixedSize) { [int]($Games / 2) } else { 50000 }
$sprtArgs = if ($fixedSize) {
    @()
} else {
    @('-sprt', "elo0=$Elo0", "elo1=$Elo1", "alpha=$Alpha", "beta=$Beta", 'model=normalized')
}

# Per-engine argument arrays so a variable number of UCI options can be
# appended cleanly (splatted into the native call below).
$engineArgsA = @("-engine", "cmd=$EngineA", "name=$NameA", "option.Hash=$Hash", "option.Threads=1") + $optArgsA
$engineArgsB = @("-engine", "cmd=$EngineB", "name=$NameB", "option.Hash=$Hash", "option.Threads=1") + $optArgsB

# Console-noise filter (2026-07-17, ported from Rarog): the per-game 'Started
# game …' / normal 'Finished game … {Draw/wins by adjudication}' / 'Score of …'
# lines bury the periodic Elo/LLR report blocks and make it impossible to scroll
# back through how the result progressed. So: TEE the FULL stream to $logOut
# (nothing lost — grep/scroll it for detail), and on the CONSOLE keep everything
# EXCEPT that per-game noise. Keep-by-default is deliberate — report blocks,
# errors, and any time-loss / disconnect / illegal 'Finished game' lines (the
# SPRT canaries) all still print. `-ratinginterval 20` reports state every 20
# games so the console shows a clean progression of report blocks.
$dropNoise = {
    param($l)
    $l = "$l"
    if ($l -match '^\s*Started game \d+ of') { return $true }
    if ($l -match '^\s*Score of .+ vs .+:\s*\d+ - \d+ - \d+') { return $true }
    # Drop only NORMAL game finishes; keep anomalies (time loss / disconnect /
    # illegal / crash / forfeit / stall) so a poisoned SPRT is still visible.
    if (($l -match '^\s*Finished game \d') -and
        ($l -notmatch '(?i)(on time|timeout|disconnect|illegal|crash|forfeit|stall)')) { return $true }
    return $false
}

& $fastchess `
    @engineArgsA `
    @engineArgsB `
    -each $tcArg "timemargin=$TimeMargin" `
    -openings "file=$Book" "format=$bookFormat" order=random `
    -rounds $rounds -games 2 -repeat `
    -concurrency $Concurrency `
    -use-affinity $AffinityCpus `
    -srand $Seed `
    -ratinginterval 20 `
    @sprtArgs `
    -draw movenumber=40 movecount=8 score=10 `
    -resign movecount=3 score=600 twosided=true `
    -pgnout "file=$pgnOut" `
    -output format=fastchess 2>&1 |    # console ticker format (not the PGN path)
    Tee-Object -FilePath $logOut |
    Where-Object { -not (& $dropNoise $_) }
# $LASTEXITCODE reflects fastchess (Tee-Object/Where-Object are cmdlets and do
# not touch it), so the exit check below stays valid.

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Error "fastchess exited with code $LASTEXITCODE — no games were played."
} else {
    Assert-NoAffinityFailure -LogPath $logOut
    Write-Host ""
    Write-Host "Match finished. PGN saved to: $pgnOut"
    Write-Host "Full console log (all per-game lines): $logOut"

    # A timeout/crash/protocol anomaly biases any fixed-size result (the slower
    # or dying side loses games it should not). Both calibrate and fixed care.
    $anomaly = Select-String -LiteralPath $logOut `
        -Pattern '(?i)(loses on time|timeouts:\s*[1-9]|crashed:\s*[1-9]|disconnect|illegal move)' `
        -ErrorAction SilentlyContinue

    if ($Mode -eq "calibrate") {
        if ($anomaly) {
            throw "Calibration contained a timeout/crash/protocol anomaly and is invalid. See '$logOut'."
        }

        $eloLine = Select-String -LiteralPath $logOut `
            -Pattern '\bnElo:\s*(?<estimate>[+-]?\d+(?:\.\d+)?)\s*\+/-\s*(?<error>\d+(?:\.\d+)?)' |
            Select-Object -Last 1
        if (-not $eloLine) { throw "Could not parse the final nElo confidence interval from '$logOut'." }

        $estimate = [double]$eloLine.Matches[0].Groups['estimate'].Value
        $error = [double]$eloLine.Matches[0].Groups['error'].Value
        $lower = $estimate - $error
        $upper = $estimate + $error
        $passes = $lower -ge -$CalibrationTolerance -and $upper -le $CalibrationTolerance

        Write-Host ""
        Write-Host ("Calibration 95% nElo CI: [{0:F2}, {1:F2}]; required inside [-{2:F2}, +{2:F2}]" -f $lower, $upper, $CalibrationTolerance)
        if ($passes) {
            Write-Host "CALIBRATION PASS" -ForegroundColor Green
        } else {
            throw "CALIBRATION INCONCLUSIVE/FAIL: the confidence interval does not establish the requested bias bound. Increase -Games only after resolving anomalies."
        }
    }
    elseif ($Mode -eq "fixed") {
        # Report-only: an equivalence / feature-value probe is judged by the
        # ESTIMATE at fixed N, never a pass/fail throw. Print the Elo CI (the
        # feature-value unit) with the nElo CI alongside, plus a NON-binding
        # classification against the 8.6.8A -3 Elo removal threshold. The
        # accept/keep/split decision (bundle-splitting, extend-to-2x) is the
        # operator's per PLAN 8.6.8A(d). Convention: engine A is the removal
        # side, so a NEGATIVE estimate means removal lost => the feature is real.
        $eloLine = Select-String -LiteralPath $logOut `
            -Pattern '^\s*Elo:\s*(?<est>[+-]?\d+(?:\.\d+)?)\s*\+/-\s*(?<err>\d+(?:\.\d+)?).*?nElo:\s*(?<nest>[+-]?\d+(?:\.\d+)?)\s*\+/-\s*(?<nerr>\d+(?:\.\d+)?)' |
            Select-Object -Last 1
        if (-not $eloLine) { throw "Could not parse the final Elo/nElo confidence interval from '$logOut'." }

        $est  = [double]$eloLine.Matches[0].Groups['est'].Value
        $err  = [double]$eloLine.Matches[0].Groups['err'].Value
        $nest = [double]$eloLine.Matches[0].Groups['nest'].Value
        $nerr = [double]$eloLine.Matches[0].Groups['nerr'].Value
        $lo   = $est - $err
        $hi   = $est + $err

        Write-Host ""
        if ($anomaly) {
            Write-Warning "This probe contains a timeout/crash/protocol anomaly (see '$logOut') — the estimate is SUSPECT. Resolve the anomaly and rerun before trusting it."
        }
        Write-Host ("Fixed probe ($Games games):  Elo {0:F2} +/- {1:F2}  [{2:F2}, {3:F2}]   (nElo {4:F2} +/- {5:F2})" -f $est, $err, $lo, $hi, $nest, $nerr)
        Write-Host "  A = $NameA (removal side); a NEGATIVE estimate means removing the feature lost => the feature is REAL."
        $hint = if ($lo -ge -3) {
            "NOISE — 95% CI entirely above -3 Elo: removal costs < 3 Elo => remove the feature (8.6.8A(d))."
        } elseif ($hi -le -3) {
            "REAL  — 95% CI entirely below -3 Elo: removal costs >= 3 Elo => KEEP the feature (8.6.8A(d))."
        } else {
            "STRADDLE -3 Elo: inconclusive => extend the SAME run to 2x games, then split a bundle / default a single to KEEP (8.6.8A(d))."
        }
        Write-Host "  Classification (non-binding): $hint"
    }
}
