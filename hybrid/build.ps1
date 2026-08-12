<#
.SYNOPSIS
    Build the Basilisk/Stockfish HCE oracle (PLAN 5.1).

.DESCRIPTION
    Produces one executable containing Stockfish 9587eeeb's search driving
    Basilisk 1.9.3's unmodified HCE, with a UCI switch that falls back to
    Stockfish's own evaluator as an exact-revision control.

    The two evaluators live in one binary on purpose: compiler, revision, UCI
    setup, book handling and time management are then identical across the
    control and the treatment, so the only difference the experiment measures
    is which evaluation function is called.

    Nothing under src/ is compiled differently from a normal Basilisk build and
    nothing under src/ is modified. The oracle links Basilisk's real evaluator.

    Requires MSYS2 clang64 (clang++, make not needed — this script drives the
    compiler directly rather than using Stockfish's Makefile, because the link
    set spans both projects).

.PARAMETER Output
    Output directory. Default: hybrid\dist

.PARAMETER Jobs
    Parallel compile jobs. Default: physical cores - 2, minimum 1.

.PARAMETER NoPgo
    Present for symmetry with Basilisk's build_test.ps1. The oracle is built
    -O3 without PGO; the experiment's throughput is reported, not optimized.
#>
param(
    [string]$Output = "$PSScriptRoot\dist",
    [int]$Jobs = 0,
    [switch]$NoPgo
)

$ErrorActionPreference = "Stop"

$hybridRoot = $PSScriptRoot
$repoRoot   = Split-Path -Parent $hybridRoot
$sfSrc      = Join-Path $hybridRoot "stockfish\src"
$objDir     = Join-Path $hybridRoot "build\obj"

if ($Jobs -le 0) {
    $cores = (Get-CimInstance Win32_Processor | Measure-Object -Property NumberOfCores -Sum).Sum
    $Jobs  = [Math]::Max(1, $cores - 2)
}

# Basilisk sources the evaluator needs: the board library plus eval.cpp.
# search/history/syzygy are deliberately NOT linked — the oracle uses
# Stockfish's search, and pulling in Basilisk's would only risk confusion
# about which search is under test.
$basiliskSources = @(
    "src\board.cpp", "src\bitboard.cpp", "src\move.cpp",
    "src\attacks.cpp", "src\zobrist.cpp", "src\eval.cpp"
) | ForEach-Object { Join-Path $repoRoot $_ }

$bridgeSource = Join-Path $hybridRoot "basilisk_bridge.cpp"

$sfSources = @(
    "benchmark.cpp", "bitbase.cpp", "bitboard.cpp", "endgame.cpp",
    "evaluate.cpp", "main.cpp", "material.cpp", "misc.cpp", "movegen.cpp",
    "movepick.cpp", "pawns.cpp", "position.cpp", "psqt.cpp", "search.cpp",
    "thread.cpp", "timeman.cpp", "tt.cpp", "tune.cpp", "uci.cpp",
    "ucioption.cpp", "hybrid_eval.cpp", "syzygy\tbprobe.cpp"
) | ForEach-Object { Join-Path $sfSrc $_ }

# Stockfish 9587eeeb is C++17 and does not compile as C++23; Basilisk requires
# C++23. They are compiled separately and linked, which the type firewall in
# basilisk_bridge.h already requires anyway.
$commonFlags   = @("-O3", "-DNDEBUG", "-DIS_64BIT", "-DUSE_POPCNT", "-DUSE_PEXT",
                   "-mbmi2", "-mpopcnt", "-flto")
# Both projects define a global `Bitboard PawnAttacks[2][64]`. They are the
# only genuine strong-symbol collision between the two link sets (the other 53
# shared symbols are weak COMDAT standard-library instantiations, which merge
# correctly by definition).
#
# This MUST be renamed rather than merged. -Wl,--allow-multiple-definition
# would link silently and leave Basilisk's evaluator reading Stockfish's table
# — and because both tables plausibly hold identical contents under identical
# colour numbering, the resulting corruption could easily produce sane-looking
# evaluations and never be noticed. The whole experiment would then measure an
# evaluator that is not Basilisk's.
#
# The rename is a preprocessor definition applied only to the Basilisk group,
# so declaration, definition and every reference move together, and no file
# under src/ is modified.
$basiliskFlags = @("-std=c++23", "-I$repoRoot\src",
                   "-DPawnAttacks=basilisk_PawnAttacks") + $commonFlags
$sfFlags       = @("-std=c++17", "-I$sfSrc", "-DNO_PRETTY_PRINT") + $commonFlags

New-Item -ItemType Directory -Force -Path $objDir, $Output | Out-Null

function Build-Group {
    param([string[]]$Sources, [string[]]$Flags, [string]$Tag)

    Write-Host "Compiling $Tag ($($Sources.Count) files, $Jobs jobs) ..."
    $jobsList = @()
    foreach ($src in $Sources) {
        $obj = Join-Path $objDir "$Tag-$([IO.Path]::GetFileNameWithoutExtension($src)).o"
        $jobsList += [pscustomobject]@{ Src = $src; Obj = $obj }
    }

    $jobsList | ForEach-Object -ThrottleLimit $Jobs -Parallel {
        $args = $using:Flags + @("-c", $_.Src, "-o", $_.Obj)
        & clang++ @args 2>&1 | ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) { throw "compile failed: $($_.Src)" }
    }

    return $jobsList.Obj
}

$objects  = @()
$objects += Build-Group -Sources ($basiliskSources + $bridgeSource) -Flags $basiliskFlags -Tag "bas"
$objects += Build-Group -Sources $sfSources -Flags $sfFlags -Tag "sf"

$exe = Join-Path $Output "basilisk-stockfish-hce-oracle.exe"
Write-Host "Linking $exe ..."
$linkArgs = @("-O3", "-flto", "-static", "-o", $exe) + $objects + @("-lpthread")
& clang++ @linkArgs
if ($LASTEXITCODE -ne 0) { throw "link failed (exit $LASTEXITCODE)" }

# Provenance. A binary without its manifest is not a reproducible test input
# (PLAN gate 8), and this one has two source revisions to record, not one.
$basiliskSha = (git -C $repoRoot rev-parse HEAD).Trim()
$dirty       = (git -C $repoRoot status --porcelain -- src) -join "`n"
$sfCommit    = (Get-Content (Join-Path $hybridRoot "stockfish\SOURCE_COMMIT") -Raw).Trim()
$binSha      = (Get-FileHash $exe -Algorithm SHA256).Hash
$clangV      = (& clang++ --version 2>&1 | Select-Object -First 1)

$manifest = Join-Path $Output "basilisk-stockfish-hce-oracle.manifest.txt"
@(
    "artifact:        $exe"
    "binary_sha256:   $binSha"
    "basilisk_rev:    $basiliskSha"
    "basilisk_src_dirty: $(if ($dirty) { 'YES - NOT A VALID ORACLE' } else { 'clean' })"
    "stockfish_rev:   $sfCommit"
    "compiler:        $clangV"
    "flags:           $($commonFlags -join ' ')"
    "built_utc:       $((Get-Date).ToUniversalTime().ToString('u'))"
) | Set-Content -Path $manifest -Encoding utf8

if ($dirty) {
    Write-Warning "src/ is dirty: this binary does not measure released 1.9.3 evaluation."
}

Write-Host ""
Write-Host "Done: $exe"
Write-Host "Manifest: $manifest"
Write-Host ""
Write-Host "Register in Colosseum TWICE, as the same executable:"
Write-Host "  Basilisk-SF-Oracle      Use Basilisk HCE = true   (Stockfish search + Basilisk HCE)"
Write-Host "  SF-9587eeeb-HCE-control Use Basilisk HCE = false  (stock Stockfish, same revision)"
