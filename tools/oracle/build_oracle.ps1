<#
.SYNOPSIS
    Rebuild the classical-Stockfish search oracle around a Basilisk revision's HCE.

.DESCRIPTION
    The oracle package is frozen at tag `oracle/hybrid`: Stockfish 9587eeeb's
    search calling Basilisk's evaluator through a bridge that never edits
    src/. This script rebuilds it for the evaluation of -BasiliskRevision:

      1. a detached worktree at `oracle/hybrid`;
      2. src/ replaced, byte for byte, by -BasiliskRevision's src/;
      3. bridge-board-access.patch applied (Basilisk 1.10 made Board's state
         private; the bridge reaches the same fields without editing src/);
      4. hybrid/build.ps1, then the conformance test, which must report zero
         mismatches between the bridge and a board parsed by try_set_fen();
      5. the oracle's own bench as its node fingerprint.

    The binary and a manifest with every input's identity are copied to
    tools/test_engines/oracle-<Label>.exe. The worktree is removed afterwards.
    The package's own manifest calls the build "dirty" because it does not know
    about step 2; this script's manifest is the authority.

.EXAMPLE
    ./tools/oracle/build_oracle.ps1 -BasiliskRevision v1.10.1 -Label 1.10.1
#>
param(
    [Parameter(Mandatory)][string]$BasiliskRevision,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9._-]+$')][string]$Label,
    [string]$OracleTag = 'oracle/hybrid',
    [string]$TestEnginesDir = "$PSScriptRoot\..\test_engines"
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Checked([string]$What, [scriptblock]$Command) {
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "$What failed (exit $LASTEXITCODE)." }
}

$repo = (Resolve-Path "$PSScriptRoot\..\..").Path
$patch = Join-Path $PSScriptRoot 'bridge-board-access.patch'
$oracleCommit = (git -C $repo rev-parse "$OracleTag^{commit}").Trim()
$srcCommit = (git -C $repo rev-parse "$BasiliskRevision^{commit}").Trim()
if ($LASTEXITCODE -ne 0) { throw "Unknown revision $BasiliskRevision." }

$work = Join-Path ([IO.Path]::GetTempPath()) ("basilisk-oracle-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
Invoke-Checked 'git worktree add' { git -C $repo worktree add --detach $work $oracleCommit 2>&1 | Out-Null }
try {
    Invoke-Checked 'checkout of src/' { git -C $work checkout $srcCommit -- src }
    Invoke-Checked 'bridge patch' { git -C $work apply --ignore-whitespace $patch }
    Invoke-Checked 'src/ identity check' { git -C $work diff --quiet $srcCommit -- src }

    Invoke-Checked 'hybrid/build.ps1' { pwsh -NoProfile -File (Join-Path $work 'hybrid\build.ps1') | Out-Host }
    $exe = Join-Path $work 'hybrid\dist\basilisk-stockfish-hce-oracle.exe'

    $conformanceExe = Join-Path $work 'hybrid\build\conformance_test.exe'
    Push-Location $work
    try {
        Invoke-Checked 'conformance build' {
            clang++ -std=c++23 -O2 -DNDEBUG -DUSE_PEXT -DUSE_POPCNT -mbmi2 -mpopcnt -Isrc -Ihybrid `
                hybrid/conformance_test.cpp hybrid/basilisk_bridge.cpp src/board.cpp src/bitboard.cpp `
                src/move.cpp src/attacks.cpp src/zobrist.cpp src/eval.cpp -o $conformanceExe
        }
    } finally { Pop-Location }
    $conformance = & $conformanceExe
    if ($LASTEXITCODE -ne 0) { throw "Conformance test failed:`n$($conformance -join "`n")" }
    $compared = ($conformance | Select-String 'positions compared\s*:\s*(\d+)').Matches[0].Groups[1].Value
    $mismatches = ($conformance | Select-String 'mismatches\s*:\s*(\d+)').Matches[0].Groups[1].Value
    if ($mismatches -ne '0') { throw "Conformance reported $mismatches mismatches." }

    $bench = & $exe bench 16 1 10 default depth 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Oracle bench failed (exit $LASTEXITCODE)." }
    $nodes = ($bench | Select-String 'Nodes searched\s*:\s*(\d+)').Matches[0].Groups[1].Value

    New-Item -ItemType Directory -Force -Path $TestEnginesDir | Out-Null
    $target = Join-Path (Resolve-Path $TestEnginesDir).Path "oracle-$Label.exe"
    Copy-Item -LiteralPath $exe -Destination $target -Force
    $compiler = (& clang++ --version | Select-Object -First 1)
    @(
        "artifact:            $target"
        "binary_sha256:       $((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash)"
        "oracle_tag:          $OracleTag ($oracleCommit)"
        "basilisk_src:        $BasiliskRevision ($srcCommit), byte-identical"
        "bridge_patch_sha256: $((Get-FileHash -LiteralPath $patch -Algorithm SHA256).Hash)"
        "stockfish:           9587eeeb"
        "compiler:            $compiler"
        "conformance:         $compared positions, $mismatches mismatches"
        "oracle_bench:        bench 16 1 10 default depth = $nodes nodes"
        "default_option:      Use Basilisk HCE = true; Hash default 16, set it explicitly"
        "built_utc:           $((Get-Date).ToUniversalTime().ToString('yyyy-MM-dd HH:mm:ssZ'))"
    ) | Set-Content -LiteralPath ([IO.Path]::ChangeExtension($target, '.manifest.txt')) -Encoding utf8
    Write-Host "Oracle: $target"
    Write-Host "Conformance: $compared positions, $mismatches mismatches; bench $nodes nodes."
} finally {
    git -C $repo worktree remove --force $work 2>&1 | Out-Null
}
