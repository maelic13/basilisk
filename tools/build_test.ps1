<#
.SYNOPSIS
    Build and manifest a final-PGO PEXT test binary.

.DESCRIPTION
    Builds one explicit flavor from a fresh configuration of the release
    preset with the release's PEXT tier (USE_PEXT, PORTABLE_BUILD), in
    build/release-pext:

      Release  production UCI surface, for strength and release-equivalent arms
      Tune     tunable search/eval options, for SPSA and parameter probes
      Diag     diagnostics only, without the tunable parameter surface
      Ablate  matched search-family removal mask, without other instruments
      Profile  the Release surface plus line tables with inline records, for
               tools/profile.ps1; code generation is unchanged

    An optional Boolean CMake umbrella option is forwarded through both PGO
    phases and recorded separately from the flavor. This lets an off/on pair
    remain comparable while proving which arm each binary contains.

    -Universal builds the universal x86-64 binary instead (the universal
    preset, Release only) and stages five executables from that one build:
    the universal binary (<suffix>-universal-pgo) and the four tier builds it
    was linked from (<suffix>-base-pgo, -sse41_popcnt-pgo, -avx2-pgo, -pext-pgo), each the
    ordinary PGO build of its tier.

    -Native builds the release preset with NATIVE=ON (-march=native, PEXT as
    the configure step detects it; Release only), in
    build/release-native, staged as <suffix>-native-pgo.

    -Gcc builds any of them with GCC (COMP=gcc) instead of Clang, in a build
    directory ending -gcc, each executable staged with -gcc after its name
    (<suffix>-pext-gcc-pgo, <suffix>-universal-gcc-pgo).

    Every output is copied to tools/test_engines with a hash-bound manifest.
    Bench and UCI-surface verification execute the copied binary and check the
    process exit code directly.

.EXAMPLE
    ./tools/build_test.ps1 -Suffix a1110-fix -Flavor Release
    ./tools/build_test.ps1 -Suffix a15-u1 -Flavor Release -Universal
    ./tools/build_test.ps1 -Suffix a158-n1 -Flavor Release -Native
    ./tools/build_test.ps1 -Suffix c3-off -Flavor Release -ArmOption C3_EVAL -Arm Off
    ./tools/build_test.ps1 -Suffix c3-on  -Flavor Release -ArmOption C3_EVAL -Arm On
#>
param(
    [Parameter(Mandatory)][string]$Suffix,
    [Parameter(Mandatory)][ValidateSet('Release', 'Tune', 'Diag', 'Ablate', 'Profile')][string]$Flavor,
    [string]$ArmOption = '',
    [ValidateSet('', 'On', 'Off')][string]$Arm = '',
    [ValidateRange(1, 128)][int]$BenchDepth = 13,
    [switch]$Universal,
    [switch]$Native,
    [switch]$Gcc,
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
if (($Universal -or $Native) -and ($Flavor -ne 'Release' -or $ArmOption)) {
    throw '-Universal and -Native build the Release flavor without an umbrella arm.'
}
if ($Universal -and $Native) { throw '-Universal and -Native are separate builds.' }

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
    # A tune build exposes the search coordinates.
    $hasTune = $Output -match '(?m)^option name CoreRfpPerPly '
    $hasAblate = $Output -match '(?m)^option name AblationMask '
    switch ($ExpectedFlavor) {
        { $_ -in 'Release', 'Profile' } {
            if ($hasDiag -or $hasTune -or $hasAblate) {
                throw "$ExpectedFlavor flavor exposes diagnostic, tuning or ablation options."
            }
        }
        'Tune' {
            if (-not ($hasDiag -and $hasTune) -or $hasAblate) {
                throw 'Tune flavor does not expose its complete UCI surface.'
            }
        }
        'Diag' {
            if (-not $hasDiag -or $hasTune -or $hasAblate) {
                throw 'Diag flavor must expose diagnostics but not tuning options.'
            }
        }
        'Ablate' {
            if ($hasDiag -or $hasTune -or -not $hasAblate) {
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
$profileSymbols = if ($Flavor -eq 'Profile') { 'ON' } else { 'OFF' }
$armState = if ($Arm) { $Arm.ToUpperInvariant() } else { 'OFF' }
$armLabel = if ($ArmOption) { "$ArmOption=$armState" } else { 'none' }
$isWindowsHost = [Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [Runtime.InteropServices.OSPlatform]::Windows)
$exeSuffix = if ($isWindowsHost) { '.exe' } else { '' }

$comp = if ($Gcc) { 'gcc' } else { 'clang' }
$gccSuffix = if ($Gcc) { '-gcc' } else { '' }
$buildDir = if ($Universal) { "universal$gccSuffix" } elseif ($Native) { "release-native$gccSuffix" } else { "release-pext$gccSuffix" }
# GCC's PGO rebuilds in the instrumented build's tree.
$pgoDirSuffix = if ($Gcc) { '-pgo-generate' } else { '-pgo' }
Push-Location $repoRoot
try {
    if ($Universal) {
        Write-Host "Building the universal x86-64 binary with $comp`: four final-PGO tiers ..."
        $configureArgs = @('--fresh', '--preset', 'universal', '-B', "build/$buildDir", "-DCOMP=$comp", '-DTEXEL=OFF')
    } elseif ($Native) {
        Write-Host "Building the native release with $comp`: -march=native, PEXT as detected ..."
        $configureArgs = @('--fresh', '--preset', 'release', '-B', "build/$buildDir", "-DCOMP=$comp", '-DNATIVE=ON', '-DTEXEL=OFF')
    } else {
        Write-Host "Building PEXT final-PGO $flavorLower flavor with $comp (arm: $armLabel) ..."
        $configureArgs = @(
            '--fresh', '--preset', 'release', '-B', "build/$buildDir",
            "-DCOMP=$comp", '-DPORTABLE_BUILD=ON', '-DUSE_PEXT=ON', '-DTEXEL=OFF',
            "-DTUNE=$tune", "-DDIAGNOSTIC=$diagnostic", "-DABLATION=$ablation", "-DPROFILE_SYMBOLS=$profileSymbols",
            "-DBASILISK_PGO_ARM_OPTION=$ArmOption", "-DBASILISK_PGO_ARM_STATE=$armState"
        )
        if ($ArmOption) { $configureArgs += "-D$ArmOption=$armState" }
    }

    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed (exit $LASTEXITCODE)" }

    if ($ArmOption) {
        $cache = Get-Content -LiteralPath "build/$buildDir/CMakeCache.txt"
        $expected = "${ArmOption}:BOOL=$armState"
        if ($cache -cnotcontains $expected) {
            throw "Umbrella option was not configured as a BOOL: expected '$expected'."
        }
    }

    $buildStarted = Get-Date
    $target = if ($Universal) { 'universal' } else { 'pgo' }
    cmake --build "build/$buildDir" --target $target
    if ($LASTEXITCODE -ne 0) { throw "cmake $target build failed (exit $LASTEXITCODE)" }

    # Each staged executable: (built path, staged name, ISA, preset, build command).
    $outputs = @()
    if ($Universal) {
        $command = "cmake --preset universal -B build/$buildDir -DCOMP=$comp; cmake --build build/$buildDir --target universal"
        $outputs += , @("build/$buildDir/basilisk-universal$exeSuffix", "universal$gccSuffix-pgo", 'universal',
            "universal (COMP=$comp, PORTABLE_BUILD=ON, BASILISK_UNIVERSAL=ON, BASILISK_ALIGN_FUNCTIONS=64; tiers from release with USE_SSE41_POPCNT, USE_AVX2 or USE_PEXT and BASILISK_UNIVERSAL_TIER=ON, PGO=USE)",
            $command)
        foreach ($tier in 'base', 'sse41_popcnt', 'avx2', 'pext') {
            $outputs += , @("build/$buildDir-$tier$pgoDirSuffix/basilisk$exeSuffix", "$tier$gccSuffix-pgo", $tier,
                "the universal binary's $tier tier (COMP=$comp, PORTABLE_BUILD=ON, BASILISK_UNIVERSAL_TIER=ON, BASILISK_ALIGN_FUNCTIONS=64, PGO=USE)",
                $command)
        }
    } elseif ($Native) {
        $cache = Get-Content -LiteralPath "build/$buildDir/CMakeCache.txt"
        $pext = if ($cache -ccontains 'USE_PEXT:BOOL=ON') { 'ON' } else { 'OFF' }
        $outputs += , @("build/$buildDir$pgoDirSuffix/basilisk$exeSuffix", "native$gccSuffix-pgo", 'native',
            "release (COMP=$comp, NATIVE=ON, USE_PEXT=$pext as detected, PGO=USE)",
            "cmake --preset release -B build/$buildDir -DCOMP=$comp -DNATIVE=ON; cmake --build build/$buildDir --target pgo")
    } else {
        $nameFlavor = if ($Flavor -eq 'Release') { "pext$gccSuffix-pgo" } else { "pext-$flavorLower$gccSuffix-pgo" }
        $command = "cmake --preset release -B build/$buildDir -DCOMP=$comp -DUSE_PEXT=ON -DPORTABLE_BUILD=ON -DTUNE=$tune -DDIAGNOSTIC=$diagnostic -DABLATION=$ablation -DPROFILE_SYMBOLS=$profileSymbols" +
            $(if ($ArmOption) { " -D$ArmOption=$armState" } else { '' }) +
            "; cmake --build build/$buildDir --target pgo"
        $outputs += , @("build/$buildDir$pgoDirSuffix/basilisk$exeSuffix", $nameFlavor, 'pext',
            "release (COMP=$comp, USE_PEXT=ON, PORTABLE_BUILD=ON, TUNE=$tune, DIAGNOSTIC=$diagnostic, ABLATION=$ablation, PROFILE_SYMBOLS=$profileSymbols, PGO=USE)",
            $command)
    }

    foreach ($output in $outputs) {
        $built = Join-Path $repoRoot $output[0]
        $nameFlavor = $output[1]
        $isa = $output[2]
        if (-not (Test-Path -LiteralPath $built -PathType Leaf)) {
            throw "Final PGO binary not found: $built"
        }
        if ((Get-Item -LiteralPath $built).LastWriteTime -lt $buildStarted) {
            throw "Final PGO binary predates this build: $built"
        }

        New-Item -ItemType Directory -Path $TestEnginesDir -Force | Out-Null
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

        $compilerMatch = Get-Content "build/$buildDir/CMakeCache.txt" |
            Select-String '^CMAKE_CXX_COMPILER:(?:FILEPATH|STRING)=(.+)$'
        $compilerPath = if ($compilerMatch) { $compilerMatch.Matches[0].Groups[1].Value } else { '' }
        if (-not $compilerPath) { throw 'Configured C++ compiler is missing from CMakeCache.txt.' }
        $compiler = (& $compilerPath --version 2>&1 | Select-Object -First 1).Trim()
        # Releases build with the LLVM major in the pin file; a local build on
        # another is not a measurement of the compiler that ships.
        $pinFile = Join-Path $repoRoot '.github\windows-clang-version'
        if ($isWindowsHost -and -not $Gcc -and (Test-Path -LiteralPath $pinFile)) {
            $pinnedMajor = (Get-Content -LiteralPath $pinFile -Raw).Trim()
            if ($compiler -notmatch "clang version $([regex]::Escape($pinnedMajor))\.") {
                Write-Warning ("LOCAL COMPILER DRIFT - this build used '$compiler', but releases build with LLVM " +
                    "$pinnedMajor (.github/windows-clang-version). Measure on the pinned major, or move the pin " +
                    'with a pooled NPS read (agents/measurement.md).')
            }
        }
        $binary = Get-Item -LiteralPath $dest
        $binarySha = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash
        $manifest = [IO.Path]::ChangeExtension($dest, '.manifest.txt')

        @(
            'schema_version: 2'
            "suffix: $Suffix"
            "engine: $dest"
            "revision: $revision"
            "source_tree: $sourceTree"
            "branch: $branch"
            "dirty_diff: $dirtyHash"
            "flavor: $isa-pgo-$flavorLower"
            "arm_option: $(if ($ArmOption) { $ArmOption } else { 'none' })"
            "arm_state: $(if ($Arm) { $armState } else { 'none' })"
            "isa: $isa"
            "portable_codegen: $(if ($Native) { 'false' } else { 'true' })"
            "preset: $($output[3])"
            "build_command: $($output[4])"
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
    }
} finally {
    Pop-Location
}
