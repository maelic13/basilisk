# =====================================================================================================================
#  verify_universal.ps1 -- bench of every variant of a universal executable that THIS CPU runs (29/10/2026). No
#  compiler needed: run it on each machine with the executable that pack_universal.ps1 wrote (copied over with its
#  manifest_universal.json), so every variant is checked on a CPU that has its instructions.
#
#  USE:
#    .\verify_universal.ps1 -Exe D:\rel\dist\Triumviratus_8.1_20261030_universal.exe
#  Writes verifica_<COMPUTERNAME>.txt and check_<COMPUTERNAME>.json next to the executable; copy both back into the
#  packing folder before publishing (build\release.ps1 -Publish reads every check_*.json there).
# =====================================================================================================================
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$ExpectedBench = ""          # empty = manifest_universal.json next to the executable, else release.json
)
$ErrorActionPreference = "Stop"
$U = $PSScriptRoot
. "$U\ubuild_lib.ps1"
$Exe = (Resolve-Path $Exe).Path
$dir = Split-Path $Exe -Parent
$LOG = ""
$mf = "$dir\manifest_universal.json"
$m = if (Test-Path $mf) { Get-Content -Raw $mf | ConvertFrom-Json } else { $null }
if ($m -and $m.exe -eq (Split-Path $Exe -Leaf) -and (Get-Sha256 $Exe) -ne $m.exe_sha256) { Fail "$Exe does not match manifest_universal.json (damaged copy?)" }
if (-not $ExpectedBench) {
    $ExpectedBench = if ($m) { "$($m.expected_bench)" } else { "$((Get-Content -Raw "$U\release.json" | ConvertFrom-Json).bench)" }
}
$cpuName = "$((Get-CimInstance Win32_Processor -ErrorAction SilentlyContinue | Select-Object -First 1).Name)".Trim()
Say "$Exe on $env:COMPUTERNAME ($cpuName), expected bench $ExpectedBench"
$tmp = Join-Path $env:TEMP "triumv_verify"
New-Item -ItemType Directory -Force $tmp | Out-Null
$t = Test-Universal $Exe $ExpectedBench $tmp
$ver = @("$(Split-Path $Exe -Leaf) checked on $env:COMPUTERNAME, CPU $cpuName, $(Get-Date -Format 'yyyy-MM-dd HH:mm')",
         "id name $($t.IdName); expected bench $ExpectedBench; sha256 $(Get-Sha256 $Exe)", "")
$ok = $true
foreach ($r in $t.Rows) {
    if ($r.runnable) { if (-not $r.ok) { $ok = $false }; $ver += "{0,-12} bench {1,-9} {2}" -f $r.name, $r.bench, $r.note }
    else { $ver += "{0,-12} not runnable on this CPU" -f $r.name }
}
Set-Content -Encoding utf8 "$dir\verifica_$env:COMPUTERNAME.txt" $ver
# One file per machine, so that checks made on different copies never overwrite each other; build\release.ps1 -Publish
# reads every check_*.json next to the executable whose exe_sha256 matches.
[ordered]@{ host = $env:COMPUTERNAME; cpu = $cpuName; created = (Get-Date -Format s); exe = (Split-Path $Exe -Leaf)
            exe_sha256 = (Get-Sha256 $Exe); expected_bench = $ExpectedBench; rows = $t.Rows } |
    ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 "$dir\check_$env:COMPUTERNAME.json"
$ver | ForEach-Object { Write-Host $_ }
if (-not $ok) { Fail "a variant does not give the expected bench" }
Write-Host "OK: verifica_$env:COMPUTERNAME.txt" -ForegroundColor Green
