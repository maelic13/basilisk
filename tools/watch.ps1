<#
.SYNOPSIS
    Console-noise filter for long fastchess / weather-factory (SPSA) runs.

.DESCRIPTION
    Pipe a noisy match stream through this to (a) TEE the full output to a log
    file — nothing is lost, grep/scroll it for per-game detail — and (b) print
    to the CONSOLE only the meaningful lines: the periodic report / parameter
    blocks, errors, and any anomalous game finishes (time loss / disconnect /
    illegal / crash / forfeit). The per-game 'Started game …', normal
    'Finished game … {Draw / wins by adjudication}', and running 'Score of …'
    lines are dropped from the console so you can actually scroll back through
    how the result / the parameters progressed.

    Keep-by-default is deliberate: anything not recognised as per-game noise is
    shown, so SPSA parameter snapshots, weather-factory's 'Elo difference'
    lines, and any error all reach the console.

.EXAMPLE
    # SPSA — filter weather-factory's console, keep a full log:
    python main.py 2>&1 | pwsh ../watch.ps1 -LogFile ..\results\spsa_see.log

.PARAMETER LogFile
    Path for the full (unfiltered) log. Created if its directory is missing.
    Omit to filter without logging.

.PARAMETER Append
    Append to -LogFile instead of truncating it. Required for anything that can
    be RESUMED: an SPSA run's parameter trajectory is what the tail-mean bake
    reads, and truncating on resume silently deletes the earlier part of it
    (Rarog lost 1,086 of 3,670 iterations exactly this way). spsa.ps1 always
    passes this and rotates the old log into tuner\archive_* on a fresh setup.
    Single-shot runs (SPRT) keep the default truncate.
#>
param(
    [string]$LogFile = "",
    [switch]$Append
)

begin {
    $writer = $null
    if ($LogFile) {
        $dir = Split-Path -Parent $LogFile
        if ($dir -and -not (Test-Path $dir)) {
            New-Item -ItemType Directory -Force -Path $dir | Out-Null
        }
        $writer = [System.IO.StreamWriter]::new($LogFile, [bool]$Append)
        $writer.AutoFlush = $true
        if ($Append) {
            $writer.WriteLine("=== session start: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ===")
        }
    }
}
process {
    $l = "$_"
    if ($writer) { $writer.WriteLine($l) }

    $isNoise =
        ($l -match '^\s*Started game \d+ of') -or
        ($l -match '^\s*Score of .+ vs .+:\s*\d+ - \d+ - \d+') -or
        (($l -match '^\s*Finished game \d') -and
         ($l -notmatch '(?i)(on time|timeout|disconnect|illegal|crash|forfeit|stall)'))

    if (-not $isNoise) { Write-Host $l }
}
end {
    if ($writer) { $writer.Close() }
}
