<#
.SYNOPSIS
    Run the registered Phase 9.11 Texel extraction or sequential joint fit.

.DESCRIPTION
    Phase 9.11 is deliberately ONE dataset and ONE fit. Generate `games.pgn`
    with the Colosseum CLI recipe in tools\colosseum\README.md. This script then:

      extract: PGN -> deterministic, game-split train/holdout CSVs
      fit:     verify -> tune all -> bake -> rebuild -> tune KS -> bake ->
               rebuild -> verify

    The fit stage modifies only src\eval_params.h and does not commit or build
    the SPRT binary. Review and commit the baked weights first; build_test.ps1
    then produces the candidate artifact.

.EXAMPLE
    .\tools\texel\phase911.ps1 -Stage extract `
        -Pgn tools\texel\data\selfplay_phase911.pgn

    .\tools\texel\phase911.ps1 -Stage fit `
        -Pgn tools\texel\data\selfplay_phase911.pgn
#>
param(
    [Parameter(Mandatory)]
    [ValidateSet("extract", "fit")]
    [string]$Stage,

    [Parameter(Mandatory)][string]$Pgn,
    [string]$Train = "",
    [string]$Holdout = "",
    [string]$ExtractionManifest = "",
    [string]$BuildDir = "build\texel-verify",
    [int]$Jobs = 16,
    [int]$Seed = 42,
    [int]$MaxPerPhasePerGame = 8,
    [int]$SkipStart = 0,
    [string]$PhaseWeights = "1,1,1,1,1",
    [switch]$NoQuietFilter,
    [int]$TargetTrain = 3500000,
    [int]$TuneEpochs = 100,
    [int]$TuneMaxPositions = 5000000,
    [int]$KingSafetyMaxPositions = 100000
)

$ErrorActionPreference = "Stop"

if ($TargetTrain -lt 1) { throw "-TargetTrain must be >= 1 for the registered Phase 9.11 fit." }
if ($MaxPerPhasePerGame -lt 1) { throw "-MaxPerPhasePerGame must be >= 1." }
if ($SkipStart -lt 0) { throw "-SkipStart must be >= 0." }

function Assert-NativeSuccess([string]$Label) {
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed (exit $LASTEXITCODE)."
    }
}

function Get-LastManifestValue([string]$Path, [string]$Key) {
    $prefix = "${Key}:"
    $line = Get-Content -LiteralPath $Path |
        Where-Object { $_.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) } |
        Select-Object -Last 1
    if (-not $line) { return "" }
    return $line.Substring($prefix.Length).Trim()
}

function Assert-FreshPath([string]$Path) {
    if (Test-Path -LiteralPath $Path) {
        throw "Refusing to overwrite registered Phase 9.11 artifact: $Path"
    }
}

function Get-TextLineCount([string]$Path) {
    $rg = Get-Command rg -ErrorAction SilentlyContinue
    if ($rg) {
        $countText = (& $rg.Source --count '^' -- $Path 2>$null | Select-Object -First 1)
        if ($LASTEXITCODE -le 1 -and $countText) { return [int64]($countText.Trim()) }
    }
    return [int64]((Get-Content -LiteralPath $Path -ReadCount 10000 |
        ForEach-Object { $_.Count } | Measure-Object -Sum).Sum)
}

function Assert-RegisteredHash([string]$Path, [string]$Manifest, [string]$Key) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Dataset not found: $Path" }
    $expected = Get-LastManifestValue $Manifest $Key
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if (-not $expected -or $expected -ne $actual) {
        throw "$Key mismatch for $Path"
    }
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
function Resolve-RepoPath([string]$Path) {
    if ([IO.Path]::IsPathFullyQualified($Path)) {
        return [IO.Path]::GetFullPath($Path)
    }
    return [IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

Push-Location $repoRoot
try {
    $Pgn = Resolve-RepoPath $Pgn
    if (-not (Test-Path -LiteralPath $Pgn)) { throw "PGN not found: $Pgn" }

    $dataDir = Split-Path -Parent $Pgn
    if (-not $Train) { $Train = Join-Path $dataDir "train_phase911.csv" }
    if (-not $Holdout) { $Holdout = Join-Path $dataDir "holdout_phase911.csv" }
    if (-not $ExtractionManifest) {
        $ExtractionManifest = Join-Path $dataDir "phase911_extract.manifest.txt"
    }
    $Train = Resolve-RepoPath $Train
    $Holdout = Resolve-RepoPath $Holdout
    $ExtractionManifest = Resolve-RepoPath $ExtractionManifest
    if ((Split-Path -Parent $Train) -ne (Split-Path -Parent $Holdout)) {
        throw "Train and holdout outputs must share one directory."
    }
    $datagenManifest = "$Pgn.manifest.txt"
    if (-not (Test-Path -LiteralPath $datagenManifest)) {
        throw "Datagen manifest not found: $datagenManifest"
    }
    if ((Get-LastManifestValue $datagenManifest "status") -ne "complete") {
        throw "Datagen manifest is not complete: $datagenManifest"
    }
    $registeredPgnSha = Get-LastManifestValue $datagenManifest "pgn_sha256"
    $actualPgnSha = (Get-FileHash -LiteralPath $Pgn -Algorithm SHA256).Hash
    if (-not $registeredPgnSha -or $registeredPgnSha -ne $actualPgnSha) {
        throw "PGN SHA-256 does not match its datagen manifest."
    }

    if ($Stage -eq "extract") {
        Assert-FreshPath $Train
        Assert-FreshPath $Holdout
        Assert-FreshPath $ExtractionManifest

        $started = (Get-Date).ToUniversalTime().ToString('u')
        @(
            "schema: phase911-extract-v2"
            "status: running"
            "started_utc: $started"
            "revision: $((git rev-parse HEAD).Trim())"
            "pgn: $Pgn"
            "pgn_sha256: $actualPgnSha"
            "datagen_manifest: $datagenManifest"
            "datagen_manifest_sha256: $((Get-FileHash -LiteralPath $datagenManifest -Algorithm SHA256).Hash)"
            "seed: $Seed"
            "jobs: $Jobs"
            "phase_buckets: opening=20-24, early_mid=14-19, middlegame=8-13, endgame=3-7, deep_endgame=0-2"
            "phase_weights: $PhaseWeights"
            "max_per_phase_per_game: $MaxPerPhasePerGame"
            "skip_start: $SkipStart"
            "skip_end: 6"
            "quiet_filter: $(if ($NoQuietFilter) { 'off' } else { 'winning-capture proxy' })"
            "target_train: $TargetTrain"
            "holdout_split: stable SHA-256(start FEN + movetext), 5%, exact phase quotas"
            "train: $Train"
            "holdout: $Holdout"
        ) | Set-Content -LiteralPath $ExtractionManifest -Encoding utf8

        $extractArgs = @(
            "tools\texel\extract_parallel.py", $Pgn,
            "--out-dir", (Split-Path -Parent $Train),
            "--train", (Split-Path -Leaf $Train),
            "--holdout", (Split-Path -Leaf $Holdout),
            "--jobs", "$Jobs",
            "--seed", "$Seed",
            "--skip-start", "$SkipStart",
            "--max-per-phase-per-game", "$MaxPerPhasePerGame",
            "--phase-weights", "$PhaseWeights",
            "--target-train", "$TargetTrain"
        )
        if ($NoQuietFilter) { $extractArgs += "--no-quiet-filter" }
        & python @extractArgs
        Assert-NativeSuccess "Phase 9.11 extraction"

        $trainRows = Get-TextLineCount $Train
        $holdoutRows = Get-TextLineCount $Holdout
        if ($TargetTrain -gt 0 -and $trainRows -ne $TargetTrain) {
            throw "Extractor wrote $trainRows train rows; registered target is $TargetTrain."
        }

        Add-Content -LiteralPath $ExtractionManifest -Encoding utf8 -Value @(
            "status: complete"
            "train_rows: $trainRows"
            "train_bytes: $((Get-Item -LiteralPath $Train).Length)"
            "train_sha256: $((Get-FileHash -LiteralPath $Train -Algorithm SHA256).Hash)"
            "holdout_rows: $holdoutRows"
            "holdout_bytes: $((Get-Item -LiteralPath $Holdout).Length)"
            "holdout_sha256: $((Get-FileHash -LiteralPath $Holdout -Algorithm SHA256).Hash)"
            "finished_utc: $((Get-Date).ToUniversalTime().ToString('u'))"
        )
        Write-Host "Phase 9.11 extraction complete: $ExtractionManifest"
        return
    }

    if (-not (Test-Path -LiteralPath $ExtractionManifest)) {
        throw "Extraction manifest not found: $ExtractionManifest"
    }
    if ((Get-LastManifestValue $ExtractionManifest "status") -ne "complete") {
        throw "Extraction manifest is not complete: $ExtractionManifest"
    }
    Assert-RegisteredHash $Train $ExtractionManifest "train_sha256"
    Assert-RegisteredHash $Holdout $ExtractionManifest "holdout_sha256"

    $dirty = git status --short
    if ($dirty) {
        throw "The fit must start from a clean accepted head; working tree is dirty.`n$dirty"
    }
    $datasetRevision = Get-LastManifestValue $datagenManifest "revision"
    $headRevision = (git rev-parse HEAD).Trim()
    if ($datasetRevision -ne $headRevision) {
        git diff --quiet $datasetRevision $headRevision -- src CMakeLists.txt CMakePresets.json cmake external
        if ($LASTEXITCODE -ne 0) {
            throw "Dataset engine revision $datasetRevision differs in engine source from fit head $headRevision."
        }
        Write-Host "Dataset revision differs only in non-engine workflow/docs; engine source is identical."
    }

    $outDir = Resolve-RepoPath "tools\texel\out"
    if (-not (Test-Path -LiteralPath $outDir)) {
        New-Item -ItemType Directory -Path $outDir | Out-Null
    }
    $allDump = Join-Path $outDir "phase911_all.txt"
    $allLog = Join-Path $outDir "phase911_all.log"
    $ksDump = Join-Path $outDir "phase911_kingsafety.txt"
    $ksLog = Join-Path $outDir "phase911_kingsafety.log"
    $fitManifest = Join-Path $outDir "phase911_fit.manifest.txt"
    foreach ($path in @($allDump, $allLog, $ksDump, $ksLog, $fitManifest)) {
        Assert-FreshPath $path
    }

    $header = Resolve-RepoPath "src\eval_params.h"
    $baseHeaderSha = (Get-FileHash -LiteralPath $header -Algorithm SHA256).Hash
    @(
        "schema: phase911-fit-v1"
        "status: running"
        "started_utc: $((Get-Date).ToUniversalTime().ToString('u'))"
        "base_revision: $headRevision"
        "base_eval_params_sha256: $baseHeaderSha"
        "extraction_manifest: $ExtractionManifest"
        "extraction_manifest_sha256: $((Get-FileHash -LiteralPath $ExtractionManifest -Algorithm SHA256).Hash)"
        "train: $Train"
        "train_sha256: $((Get-FileHash -LiteralPath $Train -Algorithm SHA256).Hash)"
        "holdout: $Holdout"
        "holdout_sha256: $((Get-FileHash -LiteralPath $Holdout -Algorithm SHA256).Hash)"
        "protocol: tune all -> restore best holdout -> bake -> rebuild -> tune-kingsafety -> restore best holdout -> bake -> rebuild -> verify"
        "all_epochs: $TuneEpochs"
        "all_max_positions: $TuneMaxPositions"
        "all_l2: 1e-6"
        "kingsafety_max_positions: $KingSafetyMaxPositions"
    ) | Set-Content -LiteralPath $fitManifest -Encoding utf8

    $BuildDir = Resolve-RepoPath $BuildDir
    cmake -S . -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release `
        -DTEXEL=ON -DUSE_PEXT=ON -DTUNE=ON -DCOMP=clang
    Assert-NativeSuccess "Texel configure"
    cmake --build $BuildDir --target basilisk-texel
    Assert-NativeSuccess "Texel baseline build"
    $texel = Join-Path $BuildDir "basilisk-texel.exe"

    & $texel --verify $Holdout
    Assert-NativeSuccess "Baseline reconstruction verify"

    & $texel --tune all $Train $Holdout $allDump --l2 1e-6 `
        --epochs $TuneEpochs --max-positions $TuneMaxPositions 2>&1 |
        Tee-Object -FilePath $allLog
    Assert-NativeSuccess "All-parameter Texel fit"
    & python tools\texel\bake.py $allDump --allow-pst
    Assert-NativeSuccess "All-parameter bake"

    cmake --build $BuildDir --target basilisk-texel
    Assert-NativeSuccess "Texel rebuild after all-parameter bake"
    & $texel --tune-kingsafety $Train $Holdout $ksDump `
        --max-positions $KingSafetyMaxPositions 2>&1 |
        Tee-Object -FilePath $ksLog
    Assert-NativeSuccess "King-safety Texel fit"
    & python tools\texel\bake.py $ksDump
    Assert-NativeSuccess "King-safety bake"

    cmake --build $BuildDir --target basilisk-texel
    Assert-NativeSuccess "Final Texel rebuild"
    & $texel --verify $Train
    Assert-NativeSuccess "Final train reconstruction verify"
    & $texel --verify $Holdout
    Assert-NativeSuccess "Final holdout reconstruction verify"

    $changed = @(git diff --name-only)
    if ($changed.Count -gt 1 -or ($changed.Count -eq 1 -and $changed[0] -ne "src/eval_params.h")) {
        throw "Phase 9.11 fit changed files outside src/eval_params.h: $($changed -join ', ')"
    }
    Add-Content -LiteralPath $fitManifest -Encoding utf8 -Value @(
        "status: complete"
        "all_dump_sha256: $((Get-FileHash -LiteralPath $allDump -Algorithm SHA256).Hash)"
        "kingsafety_dump_sha256: $((Get-FileHash -LiteralPath $ksDump -Algorithm SHA256).Hash)"
        "final_eval_params_sha256: $((Get-FileHash -LiteralPath $header -Algorithm SHA256).Hash)"
        "tracked_change: $(if ($changed.Count) { $changed -join ',' } else { 'none' })"
        "finished_utc: $((Get-Date).ToUniversalTime().ToString('u'))"
    )
    Write-Host "Phase 9.11 fit complete. Review src\eval_params.h and $fitManifest."
} finally {
    Pop-Location
}
