<#
.SYNOPSIS
    Build or adopt a pool of independent PGO builds of one source for a
    pooled NPS read (tools/nps_read.ps1).

.DESCRIPTION
    Builds, not cycles, set the limit of an NPS read: two PGO builds of one
    source differ by a few tenths of a percent, and a single binary is not an
    arm (PROCESS *Build, bench and NPS*). A pool is therefore at least four
    independent final-PGO Release builds of one clean revision, each from a
    fresh build_test.ps1 configuration.

    Build mode runs `build_test.ps1` -Builds times. A revision other than this
    checkout's HEAD is built from a temporary detached worktree with that
    revision's own build_test.ps1, so the pool is that revision's recipe.
    Adopt mode (-Engines) pools builds already staged in tools/test_engines.

    Either way the pool is refused unless every build has a clean manifest,
    one revision, source tree, compiler, flavor and arm, the expected bench
    fingerprint, and a distinct SHA-256: two equal hashes mean the profile did
    not vary, and pooling them would measure one binary twice.

    Output: tools/test_engines/<Name>.pool.json.

.EXAMPLE
    ./tools/nps_pool.ps1 -Name head-a -Builds 4 -ExpectBench 8437148

.EXAMPLE
    ./tools/nps_pool.ps1 -Name a820-clang23 -Revision 9c93d15 -ExpectBench 14978465

.EXAMPLE
    ./tools/nps_pool.ps1 -Name a820-clang22 -ExpectBench 14978465 -Engines `
        tools/test_engines/basilisk-a820-nps-pgo1-pext-pgo.exe, ...
#>
param(
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][long]$ExpectBench,
    [ValidateRange(1, 16)][int]$Builds = 4,
    [string]$Revision = '',
    [string[]]$Engines = @(),
    [string]$ArmOption = '',
    [ValidateSet('', 'On', 'Off')][string]$Arm = '',
    [switch]$AllowSmallPool,
    [string]$TestEnginesDir = "$PSScriptRoot\test_engines"
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\harness_common.ps1')

$MinBuilds = 4
if ($Name -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$') { throw '-Name must use only letters, digits, dot, underscore, and hyphen.' }
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$TestEnginesDir = [IO.Path]::GetFullPath($TestEnginesDir)
$poolPath = Join-Path $TestEnginesDir "$Name.pool.json"
if (Test-Path -LiteralPath $poolPath) { throw "Pool already exists: $poolPath. Choose another -Name." }

if ($Engines.Count) {
    if ($Revision -or $ArmOption) { throw '-Engines adopts staged builds; -Revision and -ArmOption apply to build mode only.' }
    $paths = @($Engines | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })
} else {
    $head = (& git -C $repoRoot rev-parse HEAD).Trim()
    $target = if ($Revision) { (& git -C $repoRoot rev-parse --verify "$Revision^{commit}").Trim() } else { $head }
    if ($LASTEXITCODE -ne 0 -or -not $target) { throw "Unknown revision '$Revision'." }
    $source = $repoRoot
    $worktree = $null
    if ($target -ne $head) {
        $worktree = Join-Path ([IO.Path]::GetTempPath()) "basilisk-pool-$Name"
        if (Test-Path -LiteralPath $worktree) { throw "Worktree path already exists: $worktree" }
        & git -C $repoRoot worktree add --detach $worktree $target | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "git worktree add failed for $target." }
        $source = $worktree
    } elseif (& git -C $repoRoot status --porcelain) {
        throw 'Working tree is dirty; a pool must come from an identifiable revision.'
    }
    $paths = @()
    try {
        for ($i = 1; $i -le $Builds; $i++) {
            Write-Host "== $Name build $i/$Builds at $($target.Substring(0, 10)) =="
            $buildArgs = @{ Suffix = "$Name-pgo$i"; Flavor = 'Release'; TestEnginesDir = $TestEnginesDir }
            if ($ArmOption) { $buildArgs.ArmOption = $ArmOption; $buildArgs.Arm = $Arm }
            & (Join-Path $source 'tools\build_test.ps1') @buildArgs
            $paths += Join-Path $TestEnginesDir "basilisk-$Name-pgo$i-pext-pgo.exe"
        }
    } finally {
        if ($worktree) {
            & git -C $repoRoot worktree remove --force $worktree | Out-Null
            if ($LASTEXITCODE -ne 0) { Write-Warning "Could not remove worktree $worktree; remove it with git worktree remove." }
        }
    }
}

if ($paths.Count -lt $MinBuilds -and -not $AllowSmallPool) {
    throw "A pool needs at least $MinBuilds builds; got $($paths.Count). A single binary is not an arm."
}
$records = @()
foreach ($path in $paths) {
    $manifestPath = [IO.Path]::ChangeExtension($path, '.manifest.txt')
    if (-not (Test-Path -LiteralPath $manifestPath)) { throw "No build manifest beside $path." }
    $raw = @{}
    foreach ($line in Get-Content -LiteralPath $manifestPath) {
        if ($line -match '^\s*([A-Za-z0-9_]+):\s*(.*)$') { $raw[$Matches[1]] = $Matches[2].Trim() }
    }
    $sha = Get-HarnessSha256 $path
    if ($raw['binary_sha256'] -ne $sha) { throw "PROVENANCE MISMATCH - $path hashes $sha, its manifest says $($raw['binary_sha256'])." }
    if ($raw['dirty_diff'] -ne 'clean') { throw "DIRTY TREE - $path was built from uncommitted changes." }
    if ([long]$raw['bench'] -ne $ExpectBench) { throw "WRONG FINGERPRINT - $path benched $($raw['bench']), not $ExpectBench." }
    $records += [pscustomobject]@{
        path = [IO.Path]::GetRelativePath($repoRoot, $path).Replace('\', '/')
        sha256 = $sha
        revision = $raw['revision']; source_tree = $raw['source_tree']; compiler = $raw['compiler']
        flavor = $raw['flavor']; arm = "$($raw['arm_option'])=$($raw['arm_state'])"; built_utc = $raw['built_utc']
    }
}
foreach ($field in @('revision', 'source_tree', 'compiler', 'flavor', 'arm')) {
    $values = @($records | ForEach-Object { $_.$field } | Sort-Object -Unique)
    if ($values.Count -ne 1) { throw "Pool mixes $($field)s: $($values -join '; ')." }
}
$duplicates = @($records | Group-Object sha256 | Where-Object Count -gt 1)
if ($duplicates.Count) {
    throw "Pool has identical builds ($($duplicates[0].Name.Substring(0, 12))...): the PGO profile did not vary."
}

$pool = [ordered]@{
    schema = 'basilisk-nps-pool-v1'
    name = $Name
    revision = $records[0].revision
    source_tree = $records[0].source_tree
    compiler = $records[0].compiler
    flavor = $records[0].flavor
    arm = $records[0].arm
    bench = $ExpectBench
    small_pool = [bool]($paths.Count -lt $MinBuilds)
    created_utc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    builds = @($records | ForEach-Object { [ordered]@{ path = $_.path; sha256 = $_.sha256; built_utc = $_.built_utc } })
}
$pool | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $poolPath -Encoding utf8
Write-Host "Pool $Name ($($records.Count) builds, $($pool.revision.Substring(0, 10)), $($pool.compiler)): $poolPath"
