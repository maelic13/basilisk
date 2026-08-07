<#
.SYNOPSIS
    Build a pext-PGO Basilisk binary and copy it to the test-engines folder.

.DESCRIPTION
    Configures the release-pext CMake preset (if not already configured), then
    builds the `pgo` target, which: compiles an instrumented binary, trains it
    on the 40-position `bench` suite (depth 13), merges the profile, and builds
    the final optimised binary.  The result is copied to
    D:\chess\engines\test_engines\ (kept SEPARATE from released engines in
    D:\chess\engines\) with a human-readable name so it can be passed to the
    independent Colosseum CLI.

    Always use this script (not a plain cmake --build) when building binaries
    for SPRT or gauntlet testing.  PGO shifts hot-path timing enough to affect
    measured NPS and therefore Elo comparisons.

    Prerequisites:
      - MSYS2 clang64 on PATH (or equivalent Clang + Ninja toolchain).
      - CMake >= 3.24 and Ninja available.
      - llvm-profdata on PATH (part of the MSYS2 clang64 toolchain:
        `pacman -S mingw-w64-clang-x86_64-llvm` if missing).

.PARAMETER Suffix
    Short label for the output file, e.g. "phase867-nocheckext" or "1.9.0-base".
    Output: tools\test_engines\basilisk-<Suffix>-pext-pgo.exe

.PARAMETER TestEnginesDir
    Directory where the binary is copied.  Default: tools\test_engines (repo-relative)

.EXAMPLE
    ./tools/build_test.ps1 -Suffix phase867-nocheckext
    # -> tools\test_engines\basilisk-phase867-nocheckext-pext-pgo.exe
#>
param(
    [Parameter(Mandatory)][string]$Suffix,
    [string]$TestEnginesDir = "$PSScriptRoot\test_engines"
)

$ErrorActionPreference = "Stop"

# Must run from repo root (where CMakeLists.txt lives).
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    Write-Host ""
    Write-Host "Building pext+PGO binary (suffix: $Suffix) ..."
    Write-Host ""

    # Configure release-pext preset with TUNE=ON (always required for test binaries —
    # Colosseum SPSA needs the UCI tuning options exposed).
    cmake --preset release-pext -DCOMP=clang -DTUNE=ON
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }

    # Build the pgo target: generate → train → merge → optimised binary → dist copy.
    cmake --build --preset release-pext --target pgo
    if ($LASTEXITCODE -ne 0) { throw "cmake pgo build failed (exit $LASTEXITCODE)" }

    # The pgo target copies the final binary to build/dist/ as
    # basilisk-v<version>-windows-x86_64-pext-pgo.exe
    $dist = Get-ChildItem "build\dist\basilisk-*-pext-pgo.exe" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $dist) {
        throw "No pext-pgo binary found in build\dist\ — check cmake output above."
    }

    if (-not (Test-Path $TestEnginesDir)) {
        New-Item -ItemType Directory -Path $TestEnginesDir | Out-Null
    }

    $dest = Join-Path $TestEnginesDir "basilisk-$Suffix-pext-pgo.exe"
    Copy-Item $dist.FullName $dest -Force

    # Reproducibility manifest (PLAN §1 gate 8): everything needed to rebuild
    # and identify this exact test artifact. A binary without its manifest is
    # not a reproducible test input.
    $sha       = (git rev-parse HEAD).Trim()
    $dirtyDiff = (git diff HEAD | Out-String) + (git diff --cached HEAD | Out-String)
    $dirtyHash = if ($dirtyDiff.Trim()) {
        $ms = [IO.MemoryStream]::new([Text.Encoding]::UTF8.GetBytes($dirtyDiff))
        (Get-FileHash -Algorithm SHA256 -InputStream $ms).Hash
    } else { "clean" }
    if ($dirtyHash -ne "clean") {
        # 8.6.5a (Rarog 9.7 pattern): a binary built from uncommitted changes is
        # untraceable from its revision SHA alone -- say so loudly at build time.
        Write-Warning "DIRTY TREE: this binary includes uncommitted changes (dirty_diff $($dirtyHash.Substring(0,12))...). Commit first for a traceable test artifact."
    }
    $binSha  = (Get-FileHash $dest -Algorithm SHA256).Hash
    $clangV  = (& clang --version 2>&1 | Select-Object -First 1)
    $benchOut = ("bench`nquit" | & $dest 2>&1)
    $bench   = ($benchOut | Select-String -Pattern 'Nodes searched\s*:\s*(\d+)').Matches.Groups[1].Value
    $manifest = Join-Path $TestEnginesDir "basilisk-$Suffix-pext-pgo.manifest.txt"
    @(
        "suffix:       $Suffix"
        "engine:       $dest"
        "revision:     $sha"
        "dirty_diff:   $dirtyHash"
        "preset:       release-pext (USE_PEXT=ON, TUNE=ON, PGO=USE)"
        "compiler:     $clangV"
        "bench:        $bench"
        "binary_sha256: $binSha"
        "built_utc:    $((Get-Date).ToUniversalTime().ToString('u'))"
    ) | Set-Content -Path $manifest -Encoding utf8

    Write-Host ""
    Write-Host "Done: $dest"
    Write-Host "Manifest: $manifest (revision $($sha.Substring(0,10)), bench $bench)"
    Write-Host ""
} finally {
    Pop-Location
}
