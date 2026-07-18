<#
.SYNOPSIS
    One-shot setup: download fastchess and clone weather-factory into tools/.

.DESCRIPTION
    Makes the Basilisk tuning toolchain fully self-contained inside the repo.
    Run this once after cloning.  Downloaded/cloned tools and local opening
    books under tools/ are gitignored.

    After this script:
      - tools/bin/fastchess.exe         (downloaded from GitHub)
      - tools/weather-factory/          (cloned from GitHub)
      - matplotlib installed for Python

    SPRT / SPSA / gauntlet default to the UHO opening book at
    tools\books\UHO_Lichess_4852_v1.epd (repo-local, gitignored like all books
    under tools/books/). Get it from github.com/official-stockfish/books; a
    backup copy is also kept in D:\chess\books so an accidental delete of one
    location does not break dev. Pass -Book to override with any .pgn/.epd.
    Datagen is deliberately NOT switched to UHO -- training-data generation wants
    a diverse/representative book (e.g. beast_seed.epd), not a deliberately
    unbalanced one.

    Then build a test binary and start SPSA (one command, sets up AND runs):
      ./tools/build_test.ps1 -Suffix <s>
      ./tools/spsa.ps1 -ConfigGroup <g> -EngineSuffix <s>

.PARAMETER FastchessTag
    GitHub release tag to download.  Default "latest" fetches the newest
    release.  Pin a tag (e.g. "v1.8.0-alpha") for reproducibility.

.EXAMPLE
    ./tools/setup_tools.ps1
#>
param(
    [string]$FastchessTag = "latest"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$binDir   = Join-Path $PSScriptRoot "bin"
$wfDir    = Join-Path $PSScriptRoot "weather-factory"

# ---- 1. fastchess -----------------------------------------------------------
$fastchessExe = Join-Path $binDir "fastchess.exe"
if (Test-Path $fastchessExe) {
    $ver = & $fastchessExe --version 2>&1 | Select-Object -First 1
    Write-Host "fastchess already present: $ver"
    Write-Host "  Delete tools/bin/fastchess.exe to re-download."
} else {
    Write-Host "Downloading fastchess ($FastchessTag)..."

    $apiUrl = if ($FastchessTag -eq "latest") {
        "https://api.github.com/repos/Disservin/fastchess/releases/latest"
    } else {
        "https://api.github.com/repos/Disservin/fastchess/releases/tags/$FastchessTag"
    }

    $release = Invoke-RestMethod -Uri $apiUrl `
        -Headers @{ Accept = "application/vnd.github.v3+json" }

    # Find the Windows x86-64 zip asset
    $asset = $release.assets |
        Where-Object { $_.name -like "*windows-x86-64*" } |
        Select-Object -First 1

    if (-not $asset) {
        throw "No windows-x86-64 asset found in fastchess release $($release.tag_name). " +
              "Download manually to tools/bin/fastchess.exe."
    }

    $zipPath = Join-Path $binDir "fastchess.zip"
    Write-Host "  Downloading $($asset.name) from $($release.tag_name)..."
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath
    Write-Host "  Extracting..."
    Expand-Archive -Path $zipPath -DestinationPath $binDir -Force
    Remove-Item $zipPath

    if (-not (Test-Path $fastchessExe)) {
        throw "fastchess.exe not found in tools/bin/ after extraction. " +
              "Check zip contents and extract manually."
    }
    $ver = & $fastchessExe --version 2>&1 | Select-Object -First 1
    Write-Host "  Done: $ver"
}

# ---- 2. weather-factory -----------------------------------------------------
if (Test-Path (Join-Path $wfDir "main.py")) {
    Write-Host "weather-factory already present at tools/weather-factory/ -- skipping clone."
} else {
    Write-Host "Cloning weather-factory -> tools/weather-factory/ ..."
    git clone https://github.com/jnlt3/weather-factory $wfDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
    Write-Host "  Done."
}

# ---- 3. Python dependency ---------------------------------------------------
Write-Host "Installing matplotlib (weather-factory dependency)..."
pip install matplotlib --quiet
if ($LASTEXITCODE -ne 0) { Write-Warning "pip install matplotlib failed -- run manually if needed." }

# ---- Done -------------------------------------------------------------------
Write-Host ""
Write-Host "============================================================"
Write-Host "  Toolchain setup complete."
Write-Host ""
Write-Host "  Next steps:"
Write-Host "    1. Build a test binary:"
Write-Host "         ./tools/build_test.ps1 -Suffix <s>"
Write-Host "    2. Set up AND run SPSA (one command):"
Write-Host "         ./tools/spsa.ps1 -ConfigGroup <g> -EngineSuffix <s>"
Write-Host "============================================================"
