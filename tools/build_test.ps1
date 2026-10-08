<#
.SYNOPSIS
    Build and manifest a final-PGO PEXT test binary.

.DESCRIPTION
    Builds one explicit flavor from a fresh release-pext configuration:

      Release  production UCI surface, for strength and release-equivalent arms
      Tune     tunable search/eval options, for SPSA and parameter probes
      Diag     diagnostics only, without the tunable parameter surface
      Ablate  matched search-family removal mask, without other instruments

    An optional Boolean CMake umbrella option is forwarded through both PGO
    phases and recorded separately from the flavor. This lets an off/on pair
    remain comparable while proving which arm each binary contains.

    Every output is copied to tools/test_engines with a hash-bound manifest.
    Bench and UCI-surface verification execute the copied binary and check the
    process exit code directly.

.EXAMPLE
    ./tools/build_test.ps1 -Suffix b21-off -Flavor Release -ArmOption B2_CORE -Arm Off
    ./tools/build_test.ps1 -Suffix b21-on  -Flavor Release -ArmOption B2_CORE -Arm On
    ./tools/build_test.ps1 -Suffix b23-tune -Flavor Tune -ArmOption B2_CORE -Arm On
#>
param(
    [Parameter(Mandatory)][string]$Suffix,
    [Parameter(Mandatory)][ValidateSet('Release', 'Tune', 'Diag', 'Ablate')][string]$Flavor,
    [string]$ArmOption = '',
    [ValidateSet('', 'On', 'Off')][string]$Arm = '',
    [ValidateRange(1, 128)][int]$BenchDepth = 13,
    [string]$TestEnginesDir = "$PSScriptRoot\test_engines"
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (($ArmOption -and -not $Arm) -or ($Arm -and -not $ArmOption)) {
    throw '-ArmOption and -Arm must be supplied together.'
}
if ($ArmOption -and $ArmOption -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
    throw "-ArmOption '$ArmOption' is not a CMake identifier."
}
if ($Suffix -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$') {
    throw '-Suffix must use only letters, digits, dot, underscore, and hyphen.'
}

function Invoke-CapturedProcess {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string]$InputText
    )

    $stdinPath = [IO.Path]::GetTempFileName()
    $stdoutPath = [IO.Path]::GetTempFileName()
    $stderrPath = [IO.Path]::GetTempFileName()
    try {
        Set-Content -LiteralPath $stdinPath -Value $InputText -Encoding ascii -NoNewline
        $process = Start-Process -FilePath $FilePath -WindowStyle Hidden -Wait -PassThru `
            -RedirectStandardInput $stdinPath -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath
        $output = (Get-Content -LiteralPath $stdoutPath -Raw) +
            (Get-Content -LiteralPath $stderrPath -Raw)
        if ($process.ExitCode -ne 0) {
            throw "$([IO.Path]::GetFileName($FilePath)) exited with code $($process.ExitCode).`n$output"
        }
        $output
    } finally {
        Remove-Item -LiteralPath $stdinPath, $stdoutPath, $stderrPath `
            -Force -ErrorAction SilentlyContinue
    }
}

function Assert-UciSurface {
    param([string]$Output, [string]$ExpectedFlavor)

    $hasDiag = $Output -match '(?m)^option name Diag '
    $hasTune = $Output -match '(?m)^option name RfpCoeff '
    $hasKbnk = $Output -match '(?m)^option name KBNK Drive '
    $hasAblate = $Output -match '(?m)^option name AblationMask '
    switch ($ExpectedFlavor) {
        'Release' {
            if ($hasDiag -or $hasTune -or $hasKbnk -or $hasAblate) {
                throw 'Release flavor exposes diagnostic, tuning or ablation options.'
            }
        }
        'Tune' {
            if (-not ($hasDiag -and $hasTune -and $hasKbnk) -or $hasAblate) {
                throw 'Tune flavor does not expose its complete UCI surface.'
            }
        }
        'Diag' {
            if (-not $hasDiag -or $hasTune -or $hasKbnk -or $hasAblate) {
                throw 'Diag flavor must expose diagnostics but not tuning options.'
            }
        }
        'Ablate' {
            if ($hasDiag -or $hasTune -or $hasKbnk -or -not $hasAblate) {
                throw 'Ablate flavor must expose only the ablation instrument.'
            }
        }
    }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$TestEnginesDir = [IO.Path]::GetFullPath($TestEnginesDir)
$flavorLower = $Flavor.ToLowerInvariant()
$tune = if ($Flavor -eq 'Tune') { 'ON' } else { 'OFF' }
$diagnostic = if ($Flavor -eq 'Diag') { 'ON' } else { 'OFF' }
$ablation = if ($Flavor -eq 'Ablate') { 'ON' } else { 'OFF' }
$armState = if ($Arm) { $Arm.ToUpperInvariant() } else { 'OFF' }
$armLabel = if ($ArmOption) { "$ArmOption=$armState" } else { 'none' }
$isWindowsHost = [Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [Runtime.InteropServices.OSPlatform]::Windows)
$exeSuffix = if ($isWindowsHost) { '.exe' } else { '' }

Push-Location $repoRoot
try {
    Write-Host "Building PEXT final-PGO $flavorLower flavor (arm: $armLabel) ..."

    $configureArgs = @(
        '--fresh', '--preset', 'release-pext',
        '-DCOMP=clang', '-DPORTABLE_BUILD=ON', '-DTEXEL=OFF',
        "-DTUNE=$tune", "-DDIAGNOSTIC=$diagnostic", "-DABLATION=$ablation",
        "-DBASILISK_PGO_ARM_OPTION=$ArmOption", "-DBASILISK_PGO_ARM_STATE=$armState"
    )
    if ($ArmOption) { $configureArgs += "-D$ArmOption=$armState" }

    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }

    if ($ArmOption) {
        $cache = Get-Content -LiteralPath 'build/release-pext/CMakeCache.txt'
        $expected = "${ArmOption}:BOOL=$armState"
        if ($cache -cnotcontains $expected) {
            throw "Umbrella option was not configured as a BOOL: expected '$expected'."
        }
    }

    $buildStarted = Get-Date
    cmake --build --preset release-pext --target pgo
    if ($LASTEXITCODE -ne 0) { throw "cmake pgo build failed (exit $LASTEXITCODE)" }

    $built = Join-Path $repoRoot "build/release-pext-pgo/basilisk$exeSuffix"
    if (-not (Test-Path -LiteralPath $built -PathType Leaf)) {
        throw "Final PGO binary not found: $built"
    }
    if ((Get-Item -LiteralPath $built).LastWriteTime -lt $buildStarted) {
        throw "Final PGO binary predates this build: $built"
    }

    New-Item -ItemType Directory -Path $TestEnginesDir -Force | Out-Null
    $nameFlavor = if ($Flavor -eq 'Release') { 'pext-pgo' } else { "pext-$flavorLower-pgo" }
    $dest = Join-Path $TestEnginesDir "basilisk-$Suffix-$nameFlavor$exeSuffix"
    Copy-Item -LiteralPath $built -Destination $dest -Force

    $benchOutput = Invoke-CapturedProcess -FilePath $dest -InputText "bench $BenchDepth`nquit`n"
    $benchMatch = [regex]::Matches($benchOutput, 'Nodes searched\s*:\s*([0-9]+)')
    if ($benchMatch.Count -eq 0) { throw 'Could not parse bench nodes from the copied binary.' }
    $bench = [int64]$benchMatch[$benchMatch.Count - 1].Groups[1].Value
    if ($bench -le 0) { throw "Bench reported invalid node count $bench." }

    $uciOutput = Invoke-CapturedProcess -FilePath $dest -InputText "uci`nquit`n"
    if ($uciOutput -notmatch '(?m)^uciok\s*$') { throw 'Copied binary did not complete UCI identification.' }
    Assert-UciSurface -Output $uciOutput -ExpectedFlavor $Flavor

    $revision = (& git rev-parse HEAD).Trim()
    $sourceTree = (& git rev-parse 'HEAD^{tree}').Trim()
    $branch = (& git rev-parse --abbrev-ref HEAD).Trim()
    $status = (& git status --porcelain=v1 --untracked-files=all | Out-String)
    $dirtyHash = if ($status.Trim()) {
        $dirtyEvidence = $status + (& git diff HEAD --binary | Out-String)
        $stream = [IO.MemoryStream]::new([Text.Encoding]::UTF8.GetBytes($dirtyEvidence))
        (Get-FileHash -Algorithm SHA256 -InputStream $stream).Hash
    } else { 'clean' }
    if ($dirtyHash -ne 'clean') {
        Write-Warning "DIRTY TREE: manifest records dirty_diff $($dirtyHash.Substring(0,12))..."
    }

    $compilerMatch = Get-Content 'build/release-pext/CMakeCache.txt' |
        Select-String '^CMAKE_CXX_COMPILER:(?:FILEPATH|STRING)=(.+)$'
    $compilerPath = if ($compilerMatch) { $compilerMatch.Matches[0].Groups[1].Value } else { '' }
    if (-not $compilerPath) { throw 'Configured C++ compiler is missing from CMakeCache.txt.' }
    $compiler = (& $compilerPath --version 2>&1 | Select-Object -First 1).Trim()
    $binary = Get-Item -LiteralPath $dest
    $binarySha = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash
    $manifest = [IO.Path]::ChangeExtension($dest, '.manifest.txt')
    $buildCommand = "cmake --preset release-pext -DPORTABLE_BUILD=ON -DTUNE=$tune -DDIAGNOSTIC=$diagnostic -DABLATION=$ablation" +
        $(if ($ArmOption) { " -D$ArmOption=$armState" } else { '' }) +
        '; cmake --build --preset release-pext --target pgo'

    @(
        'schema_version: 2'
        "suffix: $Suffix"
        "engine: $dest"
        "revision: $revision"
        "source_tree: $sourceTree"
        "branch: $branch"
        "dirty_diff: $dirtyHash"
        "flavor: pext-pgo-$flavorLower"
        "arm_option: $(if ($ArmOption) { $ArmOption } else { 'none' })"
        "arm_state: $(if ($Arm) { $armState } else { 'none' })"
        'isa: pext'
        'portable_codegen: true'
        "preset: release-pext (USE_PEXT=ON, PORTABLE_BUILD=ON, TUNE=$tune, DIAGNOSTIC=$diagnostic, ABLATION=$ablation, PGO=USE)"
        "build_command: $buildCommand"
        "compiler: $compiler"
        'verification: bench'
        "bench_depth: $BenchDepth"
        "bench: $bench"
        "binary_size_bytes: $($binary.Length)"
        "binary_sha256: $binarySha"
        "built_utc: $((Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ'))"
    ) | Set-Content -LiteralPath $manifest -Encoding utf8

    Write-Host "Done: $dest"
    Write-Host "Manifest: $manifest (revision $($revision.Substring(0,10)), bench $bench, flavor $flavorLower)"
} finally {
    Pop-Location
}
