# Repeatedly replays a ghost and reports the lowest race time when stopped with Ctrl+C.
# Run from the out\ directory, e.g. `..\tools\replay_min_time.ps1`.
# Each replay runs at realtime priority, which requires an elevated shell; otherwise Windows
# silently falls back to high priority.
param(
    [string]$Exe = '.\kinokoPGO.exe',
    [string]$Ghost = '..\..\..\AllSyncedGhosts\29-rmc3-tas\1110.rkg',
    [int]$Count = 0 # Number of runs, or 0 to run until Ctrl+C
)

$culture = [Globalization.CultureInfo]::InvariantCulture
$best = [double]::MaxValue
$bestLine = $null
$runs = 0
$priorityWarned = $false

$psi = New-Object Diagnostics.ProcessStartInfo
$psi.FileName = (Get-Command $Exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$psi.Arguments = "replay -g `"$Ghost`""
$psi.WorkingDirectory = (Get-Location).ProviderPath
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true

try {
    while ($Count -eq 0 -or $runs -lt $Count) {
        $proc = [Diagnostics.Process]::Start($psi)
        try {
            try {
                $proc.PriorityClass = [Diagnostics.ProcessPriorityClass]::RealTime
                $proc.Refresh()
                if (-not $priorityWarned -and $proc.PriorityClass -ne [Diagnostics.ProcessPriorityClass]::RealTime) {
                    Write-Warning "Realtime priority unavailable (got $($proc.PriorityClass)); run from an elevated shell"
                    $priorityWarned = $true
                }
            } catch {
                if (-not $priorityWarned) {
                    Write-Warning "Failed to set realtime priority: $($_.Exception.Message)"
                    $priorityWarned = $true
                }
            }

            $output = $proc.StandardOutput.ReadToEnd()
            $proc.WaitForExit()
        } finally {
            $proc.Dispose()
        }

        $line = $output -split "`r?`n" |
            Where-Object { $_ -match 'race ([0-9.]+) s' } | Select-Object -Last 1

        if (-not $line) {
            Write-Warning 'No timing line found in the output'
            continue
        }

        $null = $line -match 'race ([0-9.]+) s'
        $race = [double]::Parse($Matches[1], $culture)
        ++$runs

        if ($race -lt $best) {
            $best = $race
            $bestLine = $line
            Write-Host ('[{0}] New best race time: {1:F10} s' -f $runs, $best)
        }
    }
} finally {
    Write-Host ''
    if ($runs -eq 0) {
        Write-Host 'No completed runs.'
    } else {
        Write-Host "Runs: $runs"
        Write-Host ('Lowest race time: {0:F10} s' -f $best)
        Write-Host $bestLine
    }
}
