#!/usr/bin/env pwsh
# Build script for Basilisk chess engine.
# Usage: build.ps1 [-Config <gcc-release|gcc-static|clang-release>] [-Clean] [-Rebuild]
param(
    [ValidateSet("gcc-release", "gcc-static", "clang-release")]
    [string]$Config = "gcc-release",
    [switch]$Clean,
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"

$Root    = Split-Path -Parent $PSScriptRoot
$Cmake   = "D:/msys64/mingw64/bin/cmake.exe"
$Ninja   = "D:/msys64/mingw64/bin/ninja.exe"
$BuildDir = "$Root/build/$Config"

$env:Path = "D:/msys64/mingw64/bin;D:/msys64/clang64/bin;" + $env:Path

function Configure {
    Write-Host "Configuring $Config..." -ForegroundColor Cyan
    $cmakeArgs = @(
        "-S", $Root,
        "-B", $BuildDir,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_MAKE_PROGRAM=$Ninja"
    )
    switch ($Config) {
        "gcc-release" {
            $cmakeArgs += "-DCMAKE_CXX_COMPILER=D:/msys64/mingw64/bin/g++.exe"
        }
        "gcc-static" {
            $cmakeArgs += "-DCMAKE_CXX_COMPILER=D:/msys64/mingw64/bin/g++.exe"
            $cmakeArgs += '-DCMAKE_EXE_LINKER_FLAGS=-static'
        }
        "clang-release" {
            $cmakeArgs += "-DCMAKE_CXX_COMPILER=D:/msys64/clang64/bin/clang++.exe"
        }
    }
    & $Cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
}

# Clean build directory
if ($Clean -or $Rebuild) {
    if (Test-Path $BuildDir) {
        Write-Host "Cleaning $Config..." -ForegroundColor Yellow
        & $Ninja -C $BuildDir -t clean
        if ($LASTEXITCODE -ne 0) { throw "Clean failed" }
    }
    if ($Clean) { exit 0 }
}

# Configure if cache is missing (first run or after deleting build dir)
if (-not (Test-Path "$BuildDir/CMakeCache.txt")) {
    New-Item -ItemType Directory -Force $BuildDir | Out-Null
    Configure
}

# Build
Write-Host "Building $Config..." -ForegroundColor Cyan
& $Ninja -C $BuildDir -j4
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
Write-Host "Done: $BuildDir/basilisk.exe" -ForegroundColor Green
