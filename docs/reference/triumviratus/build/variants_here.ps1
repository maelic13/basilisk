# =====================================================================================================================
#  variants_here.ps1 -- phase 1 of the release chain on a machine with nothing set up (29/10/2026).
#
#  For a laptop without git or a GitHub login: no account is needed, the repository is public. The script
#    1. installs git with winget if it is missing (the installer may ask for an administrator confirmation);
#    2. checks that Visual Studio 2022 with clang is there, otherwise downloads setup_toolchain.ps1 and says how to run it;
#    3. downloads build\release.ps1 and runs phase 1 for the chosen variants (default: avxvnni, the Intel build without
#       AVX-512 that only CPUs like the Core Ultra run);
#    4. zips the phase 1 folder on the Desktop, ready to copy to the machine of phase 2 (release.ps1 -Pack -From).
#  ONE COMMAND (PowerShell, not as administrator):
#    Set-ExecutionPolicy -Scope Process Bypass -Force; irm https://raw.githubusercontent.com/Tors3/Triumviratus/main/build/variants_here.ps1 -OutFile $env:TEMP\variants_here.ps1; & $env:TEMP\variants_here.ps1
#  Options: -Day 20261029 (the same day on every machine), -Variants avxvnni,avx2 (empty = every variant this CPU runs),
#  -Ref <commit or tag> (the SAME source as the other machines: phase 2 refuses folders from different commits).
# =====================================================================================================================
param([string]$Day = "20261029", [string[]]$Variants = @("avxvnni"), [string]$Ref = "main")
$ErrorActionPreference = "Stop"
$raw = "https://raw.githubusercontent.com/Tors3/Triumviratus/main/build"
$dir = "$env:USERPROFILE\tr_build"
New-Item -ItemType Directory -Force $dir | Out-Null
Set-Location $dir

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "git not found: installing it with winget (no GitHub account needed)..." -ForegroundColor Yellow
    winget install --id Git.Git -e --silent --accept-package-agreements --accept-source-agreements
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw "git still not found: install it from https://git-scm.com and run this again" }
}

$clang = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-cl.exe"
if (-not (Test-Path $clang)) {
    Invoke-WebRequest "$raw/setup_toolchain.ps1" -OutFile "$dir\setup_toolchain.ps1"
    Write-Host "Visual Studio 2022 with clang is missing. Open PowerShell AS ADMINISTRATOR and run:" -ForegroundColor Red
    Write-Host "    Set-ExecutionPolicy -Scope Process Bypass -Force; & '$dir\setup_toolchain.ps1'"
    Write-Host "then run this command again in a normal PowerShell."
    exit 1
}

Invoke-WebRequest "$raw/release.ps1" -OutFile "$dir\release.ps1"
if ($Variants.Count) { & "$dir\release.ps1" -Ref $Ref -Day $Day -Variants $Variants } else { & "$dir\release.ps1" -Ref $Ref -Day $Day }

$out = Get-ChildItem "$env:USERPROFILE\Triumviratus_release\variants" -Directory -ErrorAction SilentlyContinue |
       Where-Object Name -like "*-$Day-*" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $out) { throw "no phase 1 folder for day $($Day): see the messages above" }
$zip = "$([Environment]::GetFolderPath('Desktop'))\$($out.Name).zip"
Compress-Archive -Path $out.FullName -DestinationPath $zip -Force
Write-Host "DONE: $zip -- copy it to the machine of phase 2, unzip it and give the folder to release.ps1 -Pack -From." -ForegroundColor Green
