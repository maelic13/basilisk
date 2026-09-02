<#
.SYNOPSIS
    One-shot setup: download fastchess and clone weather-factory into tools/.

.DESCRIPTION
    Makes the Basilisk tuning toolchain fully self-contained inside the repo.
    Run this once after cloning.  Downloaded/cloned tools and local opening
    books under tools/ are gitignored.

    After this script:
      - tools/bin/fastchess.exe         (downloaded from GitHub)
      - tools/weather-factory/          (cloned from GitHub, then patched:
                                         the -use-affinity insert into
                                         cutechess.py plus the tracked overlay
                                         from tools/weather-factory-overlay/)
      - matplotlib installed for Python

    Re-run it after pulling: the clone is gitignored, so a repo update that
    changes the overlay only reaches the tuner through this script.

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
    GitHub release tag to download. Default "v1.8.0-alpha", a pinned
    Basilisk harness runner after the Windows process-affinity fix in v1.7.0.

.EXAMPLE
    ./tools/setup_tools.ps1
#>
param(
    [string]$FastchessTag = "v1.8.0-alpha"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "..\harness_common.ps1")
$repoRoot = Split-Path -Parent $PSScriptRoot
$binDir   = Join-Path $PSScriptRoot "bin"
$wfDir    = Join-Path $PSScriptRoot "weather-factory"
New-Item -ItemType Directory -Force -Path $binDir | Out-Null

# ---- 1. fastchess -----------------------------------------------------------
$fastchessExe = Join-Path $binDir "fastchess.exe"
$downloadFastchess = -not (Test-Path $fastchessExe)
if (Test-Path $fastchessExe) {
    try {
        $info = Assert-AffinityFastchess -Path $fastchessExe
        Write-Host "fastchess already present: $($info.Text)"
        Write-Host "  Existing compatible runner retained (version is recorded per match)."
    } catch {
        Write-Warning $_.Exception.Message
        Write-Host "  Replacing incompatible runner with $FastchessTag."
        $downloadFastchess = $true
    }
}
if ($downloadFastchess) {
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
    if ($asset.digest -match '^sha256:(?<hash>[0-9a-fA-F]{64})$') {
        $actualHash = (Get-FileHash $zipPath -Algorithm SHA256).Hash
        if ($actualHash -ne $Matches['hash']) {
            throw "fastchess archive SHA-256 mismatch: expected $($Matches['hash']), got $actualHash"
        }
        Write-Host "  Archive SHA-256 verified."
    }
    Write-Host "  Extracting..."
    $extractDir = Join-Path ([System.IO.Path]::GetTempPath()) ("basilisk-fastchess-" + [guid]::NewGuid())
    New-Item -ItemType Directory -Path $extractDir | Out-Null
    try {
        Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force
        $extracted = @(Get-ChildItem -LiteralPath $extractDir -Recurse -Filter "fastchess.exe" -File)
        if ($extracted.Count -ne 1) {
            throw "Expected one fastchess.exe in $($asset.name), found $($extracted.Count)."
        }
        Copy-Item -LiteralPath $extracted[0].FullName -Destination $fastchessExe -Force
    } finally {
        Remove-Item -LiteralPath $extractDir -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $zipPath -Force -ErrorAction SilentlyContinue
    }

    if (-not (Test-Path $fastchessExe)) {
        throw "fastchess.exe not found in tools/bin/ after extraction. " +
              "Check zip contents and extract manually."
    }
    $ver = & $fastchessExe --version 2>&1 | Select-Object -First 1
    Write-Host "  Done: $ver"
    Assert-AffinityFastchess -Path $fastchessExe | Out-Null
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

# ---- 2b. Local patches: affinity + no-adjudication default ------------------
# Weather-factory's fastchess mini-matches run at clock TC and are subject to
# the same scheduler placement lottery documented in sprt.ps1's header. The
# clone is gitignored, so the patch is applied HERE to survive re-clones.
$wfCute = Join-Path $wfDir "cutechess.py"
if (Test-Path $wfCute) {
    $c = Get-Content $wfCute -Raw
    $allPhysicalCpus = (Get-HarnessPhysicalCpus).Cpu -join ','
    # Repair the original 2026-07-21 patch, which inserted a Python `+`
    # expression between implicitly concatenated f-strings and caused a
    # SyntaxError at startup.
    $c = $c -replace '(?m)^\s*\+ \("-use-affinity " if self\.use_fastchess else ""\).*\r?\n?', ''
    # Rebuild V2 every time so a clone moved to different hardware cannot keep
    # a stale machine-specific CPU list.
    $c = $c -replace '(?m)^.*BASILISK_AFFINITY_PATCH_V2.*\r?\n?', ''

    # Score-based draw/resign truncates precisely the endgame positions needed
    # by evaluation and Texel work. Keep it behind explicit config opt-in.
    $c = $c -replace '(?m)^\s*"-resign movecount=3 score=400 "\r?\n?', ''
    $c = $c -replace '(?m)^\s*"-draw movenumber=40 movecount=8 score=10 "\r?\n?', ''
    if ($c -notmatch 'BASILISK_NO_ADJUDICATION_DEFAULT_V1') {
        $signatureAnchor = '        use_fastchess: bool = True'
        $signaturePatch = $signatureAnchor + ",`n" + '        adjudicate: bool = False'
        $fieldAnchor = '        self.use_fastchess = use_fastchess'
        $fieldPatch = $fieldAnchor + "`n" + '        self.adjudicate = adjudicate'
        $commandAnchor = '            "-recover "'
        $commandPatch = ('            f"{''-resign movecount=3 score=400 -draw movenumber=40 movecount=8 score=10 '' if self.adjudicate else ''''}"  # BASILISK_NO_ADJUDICATION_DEFAULT_V1') + "`n" + $commandAnchor
        foreach ($requiredAnchor in @($signatureAnchor, $fieldAnchor, $commandAnchor)) {
            if (-not $c.Contains($requiredAnchor)) {
                throw "weather-factory/cutechess.py no-adjudication patch anchor not found; upstream changed."
            }
        }
        $c = $c.Replace($signatureAnchor, $signaturePatch)
        $c = $c.Replace($fieldAnchor, $fieldPatch)
        $c = $c.Replace($commandAnchor, $commandPatch)
    }

    $anchor = 'f"-concurrency {self.threads} "'
    $patch  = $anchor + "`n" + ('            f"{''-use-affinity ' + $allPhysicalCpus + ' '' if self.use_fastchess else ''''}"  # BASILISK_AFFINITY_PATCH_V2')
    if (-not $c.Contains($anchor)) {
        throw "weather-factory/cutechess.py affinity patch anchor not found; upstream changed."
    }
    $c = $c.Replace($anchor, $patch)
    Set-Content -Path $wfCute -Value $c -Encoding utf8

    python -m py_compile $wfCute
    if ($LASTEXITCODE -ne 0) {
        throw "weather-factory affinity patch failed Python syntax validation: $wfCute"
    }
    Write-Host "  weather-factory affinity/no-adjudication patches and Python syntax verified."
}

# ---- 2c. Local overlay: SPSA schedule + runner (Phase 9.1) -------------------
# spsa.py / main.py / write_spsa_json.py are replaced wholesale from
# tools/weather-factory-overlay/ (too much changed for an anchored patch). This
# carries the games-vs-iterations schedule repair, the fishtest end-state
# parameterization and the multi-session hardening. spsa.ps1 refuses to set up
# or launch if the clone's copies drift from the tracked originals.
if (Test-Path (Join-Path $wfDir "main.py")) {
    Install-WfOverlay -WeatherFactoryDir $wfDir
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
