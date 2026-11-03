# =====================================================================================================================
#  release.ps1 -- Windows build chain of Triumviratus releases (29/10/2026, from 8.1). One script for every machine.
#
#  The universal executable holds seven variants (sse41, avx2-nopext, avx2, avxvnni, avx512, vnni512, avx512icl).
#  Each variant's PGO profile must be recorded on a CPU that runs that variant, with its own instructions, and no
#  machine of the author runs all seven:
#    laptop Zen 4          sse41, avx2-nopext, avx2, avx512, vnni512, avx512icl
#    laptop Core Ultra 7   sse41, avx2-nopext, avx2, avxvnni
#    Xeon Gold 6138 (x2)   sse41, avx2-nopext, avx2, avx512 (Skylake-SP: AVX-512 without VNNI)
#  The two laptops together cover all seven. So the chain has two phases: every machine builds and verifies the variants it runs (phase 1), then one machine
#  joins the profiles into the universal executable (phase 2), which the other machines verify.
#
#  PHASE 1, on each machine (PowerShell, no administrator if the toolchain is there):
#    Set-ExecutionPolicy -Scope Process Bypass -Force
#    .\release.ps1                                 # branch main, dated pre-release of today, every variant this CPU runs
#    .\release.ps1 -Ref v8.1 -Official             # the official release from tag v8.1 (id name without date)
#    .\release.ps1 -Day 20261030 -LowPriority      # same day on every machine; below-normal priority (tests running)
#    .\release.ps1 -Variants avxvnni               # only some variants
#    .\release.ps1 -NoBuild                        # only download and checks
#  -> <Work>\variants\<version>-<flavour>-<day|official>-<COMPUTERNAME>\ : one verified executable per variant, their
#     profiles, manifest.json, verifica.txt, SHA256SUMS.txt. Copy that folder to the machine of phase 2.
#
#  PHASE 2, on one machine (the same -Ref; the source must be identical, the manifests check it):
#    .\release.ps1 -Pack -From D:\in\8.1-game-20261030-ZEN4, D:\in\8.1-game-20261030-ULTRA7
#    (without -From: every folder in <Work>\variants with the same source and day as this machine's)
#  -> <Work>\dist\<version>-<flavour>-<day|official>\ : Triumviratus_<ver>_..._universal.exe, SHA256SUMS.txt,
#     verifica.txt, manifest_universal.json
#
#  VERIFY, on the other machines (bench of the variants each CPU runs):
#    .\release.ps1 -Verify -Exe D:\in\dist\Triumviratus_8.1_20261030_universal.exe
#    then copy verifica_<COMPUTERNAME>.txt and check_<COMPUTERNAME>.json back into the dist folder.
#
#  PUBLISH, only with this flag and only after typing the tag at the prompt (nothing is uploaded otherwise):
#    .\release.ps1 -Publish -Tag v8.1-prerelease-20261030 -Dist <Work>\dist\8.1-game-20261030 [-IncludeNet] [-Clobber]
#  The release must exist (created by the author with its notes); the script prints the files, their SHA-256, the
#  checks of every variant, and asks for confirmation before `gh release upload`.
#
#  Needs: git (phase 1 downloads the source with it), Visual Studio 2022 with the clang components, Windows SDK,
#  Python 3; build\setup_toolchain.ps1 installs them (-InstallToolchain runs it as administrator). Publishing needs gh.
#  The expected bench, the version and the network (URL, size, SHA-256, MD5) come from source\universal\release.json
#  of the chosen ref; -ExpectedBench overrides the bench.
# =====================================================================================================================
param(
    [string]$Ref = "main",
    [string]$Repo = "https://github.com/Tors3/Triumviratus",
    # DEDICATED folder: the script resets the clone in <Work>\repo at every run. It refuses a non-empty folder it did
    # not create (marker file .triumv_release), so it can never wipe a working tree.
    [string]$Work = "$env:USERPROFILE\Triumviratus_release",
    # An existing source folder instead of the download (a checkout of the public repo, e.g. to test the chain).
    [string]$Source = "",
    [string]$Day = "",
    [switch]$Official,
    [switch]$PosSolver,
    [string]$ExpectedBench = "",
    [string[]]$Variants = @(),
    [int]$Workers = [Environment]::ProcessorCount,
    [int]$Jobs = [Environment]::ProcessorCount,
    [int]$NnPerft = 0,
    [int]$TrainPositions = 200,
    [switch]$NoPgo,
    [switch]$LowPriority,
    [switch]$InstallToolchain,
    [switch]$NoBuild,
    # phase 2
    [switch]$Pack,
    [string[]]$From = @(),
    [switch]$AllowMissing,
    [switch]$AllowTestProfiles,
    # verification on another machine
    [switch]$Verify,
    [string]$Exe = "",
    # publication
    [switch]$Publish,
    [string]$Tag = "",
    [string]$Dist = "",
    [string]$GhRepo = "Tors3/Triumviratus",
    [switch]$IncludeNet,
    [switch]$Clobber
)
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"   # Invoke-WebRequest is ten times slower with the progress bar

function Step($m) { Write-Host "`n==== $m ====" -ForegroundColor Cyan }
function Ok($m)   { Write-Host "  $m" -ForegroundColor Green }
function Warn($m) { Write-Host "  $m" -ForegroundColor Yellow }
function Die($m)  { Write-Host "  $m" -ForegroundColor Red; throw $m }
# git writes progress and notes on stderr: with "Stop" PowerShell 5.1 would turn them into errors.
function Invoke-Git { $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
                      $o = & git.exe @args 2>&1; $code = $LASTEXITCODE; $ErrorActionPreference = $old
                      if ($code -ne 0) { Die "git $($args -join ' ') failed: $($o -join ' ')" }; return ($o | Where-Object { $_ -is [string] }) }

if ($LowPriority) {
    # Children (compiler, training, engine) inherit below-normal priority: the build yields to running tests. The PGO
    # training counts nodes, not time, so the profile does not change.
    (Get-Process -Id $PID).PriorityClass = 'BelowNormal'
}

# --- publication: no source and no compiler needed -----------------------------------------------------------------------
if ($Publish) {
    Step "Publication to $GhRepo"
    if (-not $Tag -or -not $Dist) { Die "-Publish needs -Tag and -Dist" }
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { Die "gh (GitHub CLI) not found" }
    $Dist = (Resolve-Path $Dist).Path
    $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    $relInfo = & gh release view $Tag -R $GhRepo --json name,isDraft,isPrerelease,assets 2>&1
    $code = $LASTEXITCODE; $ErrorActionPreference = $old
    if ($code -ne 0) {
        Die "release $Tag not found in $GhRepo. Create it first with its notes, e.g.: gh release create $Tag -R $GhRepo --prerelease --draft --title `"...`" --notes-file notes.md"
    }
    $relInfo = ($relInfo | Where-Object { $_ -is [string] }) -join "`n" | ConvertFrom-Json
    $mf = "$Dist\manifest_universal.json"
    if (-not (Test-Path $mf)) { Die "$Dist has no manifest_universal.json (not a -Pack folder)" }
    $m = Get-Content -Raw $mf | ConvertFrom-Json
    $exePath = "$Dist\$($m.exe)"
    if ((Get-FileHash -Algorithm SHA256 $exePath).Hash.ToLower() -ne $m.exe_sha256) { Die "$exePath does not match its manifest" }
    # Checks of every variant: the packing machine (manifest) and every check_<PC>.json of -Verify for this executable.
    $checks = @($m.checked) + @(Get-ChildItem "$Dist\check_*.json" -ErrorAction SilentlyContinue |
                                ForEach-Object { Get-Content -Raw $_.FullName | ConvertFrom-Json } | Where-Object { $_.exe_sha256 -eq $m.exe_sha256 })
    $status = @{}
    foreach ($c in $checks) { foreach ($r in $c.rows) {
        if ($r.runnable) { if (-not $status.ContainsKey($r.name)) { $status[$r.name] = @() }; $status[$r.name] += "$(if ($r.ok) { 'ok' } else { 'FAILED' }) on $($c.host) ($($r.bench))" } } }
    Write-Host "  $($m.exe): version $($m.version), $($m.flavour), expected bench $($m.expected_bench), source $($m.source.commit)"
    if ("$($m.source.commit)" -match '\+modified$|^$') { Warn "the source was not a clean commit: a release should be built from a pushed tag or branch" }
    $failed = $false
    foreach ($v in 'sse41', 'avx2-nopext', 'avx2', 'avxvnni', 'avx512', 'vnni512', 'avx512icl') {
        $pf = @($m.profiles | Where-Object { $_.name -eq $v })[0].from
        $s = if ($status.ContainsKey($v)) { @($status[$v] | Select-Object -Unique) -join '; ' } else { 'NOT CHECKED on any CPU' }
        if ($s -match 'FAILED') { $failed = $true }
        $line = "    {0,-12} profile {1,-16} {2}" -f $v, $(if ($pf) { $pf } else { 'none (no PGO)' }), $s
        if ($s -match 'FAILED|NOT CHECKED') { Warn $line.Trim() } else { Write-Host $line }
    }
    if ($failed) { Die "a variant failed its bench: nothing to publish" }
    # Staging folder: the files to upload and their SHA256SUMS.txt.
    $stage = "$Dist\upload"
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    New-Item -ItemType Directory $stage | Out-Null
    Copy-Item $exePath $stage
    if ($IncludeNet) {
        $netFile = "$Work\nets\$($m.network.file)"
        if (-not (Test-Path $netFile) -or (Get-FileHash -Algorithm SHA256 $netFile).Hash.ToLower() -ne $m.network.sha256) {
            Die "-IncludeNet: $netFile missing or not the network of the build (run phase 1 on this machine first)"
        }
        Copy-Item $netFile $stage
    }
    $files = @(Get-ChildItem $stage -File | Sort-Object Name)
    $files | ForEach-Object { (Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLower() + "  " + $_.Name } |
        Set-Content -Encoding ascii "$stage\SHA256SUMS.txt"
    $files = @(Get-ChildItem $stage -File | Sort-Object Name)
    Write-Host "`n  Release: $($relInfo.name) (tag $Tag, $(if ($relInfo.isDraft) { 'draft' } elseif ($relInfo.isPrerelease) { 'pre-release' } else { 'RELEASE' }))"
    Write-Host "  Would upload:"
    foreach ($f in $files) { Write-Host ("    {0,-48} {1,12:N0} bytes  {2}" -f $f.Name, $f.Length, (Get-FileHash -Algorithm SHA256 $f.FullName).Hash.ToLower()) }
    $existing = @($relInfo.assets | ForEach-Object { $_.name } | Where-Object { $_ -in @($files | ForEach-Object Name) })
    if ($existing.Count -and -not $Clobber) { Die "already on the release: $($existing -join ', ') (add -Clobber to replace them)" }
    if ($existing.Count) { Warn "will REPLACE: $($existing -join ', ')" }
    $ans = Read-Host "`n  Type the tag ($Tag) to upload these files, anything else cancels"
    if ($ans -ne $Tag) { Write-Host "  Cancelled: nothing uploaded."; exit 0 }
    $ghArgs = @('release', 'upload', $Tag, '-R', $GhRepo) + @($files | ForEach-Object FullName)
    if ($Clobber) { $ghArgs += '--clobber' }
    & gh @ghArgs
    if ($LASTEXITCODE -ne 0) { Die "gh release upload failed" }
    Ok "uploaded to https://github.com/$GhRepo/releases/tag/$Tag"
    exit 0
}

# --- work folder ---------------------------------------------------------------------------------------------------------
$marker = "$Work\.triumv_release"
if ((Test-Path $Work) -and -not (Test-Path $marker) -and @(Get-ChildItem $Work -Force).Count) {
    Die "$Work exists, is not empty and was not created by this script: choose another -Work"
}
New-Item -ItemType Directory -Force $Work | Out-Null
Set-Content -Encoding ascii $marker "Triumviratus release work folder (build\release.ps1): <Work>\repo is reset at every run."
$Work = (Resolve-Path $Work).Path

# --- source -------------------------------------------------------------------------------------------------------------
# For -Pack the commit comes from the phase 1 manifests.
$wantCommit = ""
if ($Pack) {
    if (-not $From.Count) {
        $From = @(Get-ChildItem "$Work\variants" -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path "$($_.FullName)\manifest.json" } |
                  ForEach-Object FullName)
        if (-not $From.Count) { Die "no phase 1 folder in $Work\variants: give -From" }
    }
    $mans = @($From | ForEach-Object { Get-Content -Raw "$_\manifest.json" | ConvertFrom-Json })
    $commits = @($mans | ForEach-Object { $_.source.commit } | Sort-Object -Unique)
    if ($commits.Count -ne 1) { Die "the phase 1 folders come from different commits: $($commits -join ', ') (give -From explicitly)" }
    $wantCommit = $commits[0]
    if (-not $Source -and $wantCommit -match '\+modified$') { Die "the profiles come from a modified source ($wantCommit): give the same tree with -Source" }
}
if ($Source) {
    $SRC = (Resolve-Path $Source).Path
    if (Test-Path "$SRC\source\universal") { $SRC = "$SRC\source" }
    if (-not (Test-Path "$SRC\universal")) { Die "$Source is not an engine source folder (no universal\)" }
    $commit = "$(& git.exe -C $SRC rev-parse HEAD 2>$null)".Trim()
    if ($commit -and "$(& git.exe -C $SRC status --porcelain --untracked-files=no -- . 2>$null)".Trim()) { $commit += "+modified" }
    Step "Source: $SRC ($commit)"
} else {
    Step "Source: $Repo, $(if ($wantCommit) { "commit $wantCommit (of the phase 1 folders)" } else { "ref $Ref" })"
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Die "git not found: install it (build\setup_toolchain.ps1, or winget install Git.Git)" }
    $repoDir = "$Work\repo"
    if (-not (Test-Path "$repoDir\.git")) { Invoke-Git clone --quiet --filter=blob:none --no-checkout $Repo $repoDir | Out-Null }
    Invoke-Git -C $repoDir fetch --quiet --force --tags origin '+refs/heads/*:refs/remotes/origin/*' | Out-Null
    $commit = ""
    $cands = if ($wantCommit) { @($wantCommit) } else { @("origin/$Ref", "refs/tags/$Ref", $Ref) }
    foreach ($c in $cands) {
        $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
        $r = & git.exe -C $repoDir rev-parse --verify --quiet "$c^{commit}" 2>$null
        $ErrorActionPreference = $old
        if ($LASTEXITCODE -eq 0 -and $r) { $commit = "$r".Trim(); break }
    }
    if (-not $commit) { Die "ref '$Ref' not found in $Repo (branch, tag or commit)" }
    Invoke-Git -C $repoDir checkout --quiet --force --detach $commit | Out-Null
    Invoke-Git -C $repoDir clean -xfdq | Out-Null
    $SRC = "$repoDir\source"
    Ok "commit $commit"
}
$U = "$SRC\universal"
if (-not (Test-Path "$U\release.json") -or -not (Test-Path "$U\build_variants.ps1")) {
    Die "this source has no release chain (universal\release.json, build_variants.ps1): use universal\build_universal.ps1 of that ref"
}
$rel = Get-Content -Raw "$U\release.json" | ConvertFrom-Json
$VER = $rel.version
. "$U\ubuild_lib.ps1"
$LOG = ""

# --- verification of a universal executable (no compiler needed) ---------------------------------------------------------
if ($Verify) {
    if (-not $Exe) { Die "-Verify needs -Exe" }
    & "$U\verify_universal.ps1" -Exe $Exe -ExpectedBench $ExpectedBench
    exit 0
}

# --- network --------------------------------------------------------------------------------------------------------------
# The one file where a silent error would cost days: a wrong network compiles, starts and plays, worse, without a word.
# Checked by size, SHA-256 and MD5 (release.json, the same values as the Networks release and NETWORKS.md).
Step "Network $($rel.network.file)"
function NetOk($p) {
    (Test-Path $p) -and ((Get-Item $p).Length -eq [int64]$rel.network.bytes) -and
    ((Get-FileHash $p -Algorithm SHA256).Hash.ToLower() -eq $rel.network.sha256) -and
    ((Get-FileHash $p -Algorithm MD5).Hash.ToLower() -eq $rel.network.md5)
}
$cache = "$Work\nets\$($rel.network.file)"
New-Item -ItemType Directory -Force "$Work\nets" | Out-Null
if (-not (NetOk $cache)) {
    # First next to this script (USB stick, shared folder), by content.
    $here = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
    $local = Get-ChildItem $here -Filter *.nnue -File -ErrorAction SilentlyContinue | Where-Object { NetOk $_.FullName } | Select-Object -First 1
    if ($local) { Copy-Item $local.FullName $cache -Force; Ok "taken from $($local.FullName)" }
    else {
        Write-Host "  downloading $([math]::Round($rel.network.bytes / 1MB)) MB from $($rel.network.url)"
        $part = "$cache.part"
        if (Get-Command curl.exe -ErrorAction SilentlyContinue) { & curl.exe -L --fail --silent --show-error -o $part $rel.network.url }
        else { Invoke-WebRequest $rel.network.url -OutFile $part }
        if (-not (NetOk $part)) { Remove-Item $part -ErrorAction SilentlyContinue; Die "the downloaded network does not match release.json (sha256 $($rel.network.sha256))" }
        Move-Item -Force $part $cache
    }
}
if (-not (NetOk $cache)) { Die "network check failed" }
$embed = Get-EmbeddedNetPath
Copy-Item $cache $embed -Force
Ok "verified (size, SHA-256, MD5) -> $embed (the file triumv.rc embeds)"

# --- toolchain ------------------------------------------------------------------------------------------------------------
Step "Toolchain"
try { $tc = Find-Toolchain -NeedPgo:(-not $NoPgo); Ok "$($tc.clang); MSVC $($tc.msvc.Name); SDK $($tc.sdk.Name); Python $($tc.py)" }
catch {
    $setup = Join-Path (Split-Path $SRC -Parent) "build\setup_toolchain.ps1"
    if (-not (Test-Path $setup)) { $setup = "$PSScriptRoot\setup_toolchain.ps1" }
    if ($InstallToolchain -and (Test-Path $setup)) {
        Warn "installing with $setup (administrator prompt)"
        Start-Process powershell -Verb RunAs -Wait -ArgumentList "-ExecutionPolicy Bypass -File `"$setup`""
        Die "toolchain installed: open a NEW PowerShell (for the PATH) and run this script again"
    }
    Warn "$_"
    Warn "Install it from an administrator PowerShell:  Set-ExecutionPolicy -Scope Process Bypass -Force; & '$setup'"
    Warn "or run this script again with -InstallToolchain."
    Die "toolchain missing"
}
$clangMajor = [int]([regex]::Match($tc.clang, 'version (\d+)').Groups[1].Value)
if ($clangMajor -lt 19) { Die "clang ${clangMajor}: the engine needs clang 19 or later (static_assert)" }

if ($NoBuild) { Step "Ready (-NoBuild)"; exit 0 }
if (-not $Day) {
    $Day = if ($Pack) { "" } else { Get-Date -Format yyyyMMdd }
}

# --- phase 2 ---------------------------------------------------------------------------------------------------------------
if ($Pack) {
    $m0 = $mans[0]
    $tag = if ($m0.official) { 'official' } else { if ($Day) { $Day } else { $m0.day } }
    $dist = "$Work\dist\$VER-$($m0.flavour)-$tag"
    Step "Phase 2: universal executable -> $dist"
    $p = @{ From = $From; Out = $dist; Jobs = $Jobs; ObjDir = "$Work\obj\dist-$VER-$($m0.flavour)-$tag" }
    if ($Day) { $p.Day = $Day }
    if ($AllowMissing) { $p.AllowMissing = $true }
    if ($AllowTestProfiles) { $p.AllowTestProfiles = $true }
    & "$U\pack_universal.ps1" @p
    Step "Done"
    Get-Content "$dist\verifica.txt" | ForEach-Object { Write-Host "  $_" }
    Write-Host "`n  Next: on every other machine  .\release.ps1 -Verify -Exe <copy of $dist>\$((Get-Content -Raw "$dist\manifest_universal.json" | ConvertFrom-Json).exe)"
    Write-Host "  then copy back verifica_<PC>.txt and check_<PC>.json, and publish with -Publish -Tag <tag> -Dist $dist"
    exit 0
}

# --- phase 1 ---------------------------------------------------------------------------------------------------------------
$flavour = if ($PosSolver) { 'possolver' } else { 'game' }
$tag = if ($Official) { 'official' } else { $Day }
$out = "$Work\variants\$VER-$flavour-$tag-$env:COMPUTERNAME"
Step "Phase 1: variants of this CPU -> $out"
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
$p = @{ Out = $out; Day = $Day; Workers = $Workers; Jobs = $Jobs; TrainPositions = $TrainPositions; Commit = $commit
        ObjDir = "$Work\obj\$(Split-Path $out -Leaf)" }
if ($Official) { $p.Official = $true }
if ($PosSolver) { $p.PosSolver = $true }
if ($ExpectedBench) { $p.ExpectedBench = $ExpectedBench }
if ($Variants.Count) { $p.Variants = $Variants }
if ($NnPerft) { $p.NnPerft = $NnPerft }
if ($NoPgo) { $p.NoPgo = $true }
& "$U\build_variants.ps1" @p
Step "Done"
Get-Content "$out\verifica.txt" | ForEach-Object { Write-Host "  $_" }
Write-Host "`n  Next: copy $out to the machine that packs (or keep it here), then there:"
Write-Host "    .\release.ps1 -Ref $Ref -Pack -From <folders of every machine>$(if ($Source) { " -Source <same tree>" })"
