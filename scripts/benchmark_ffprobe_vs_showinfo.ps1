$ErrorActionPreference = 'Stop'

$root = 'e:\ownCode\visualFrameInfo'
$benchDir = Join-Path $root 'logs\benchmark'
New-Item -ItemType Directory -Path $benchDir -Force | Out-Null

$ffprobe = Join-Path $root 'ffmpeg\ffprobe.exe'
$ffmpeg = Join-Path $root 'ffmpeg\ffmpeg.exe'

$videos = @(
    'E:\file\clip\video\同步测试视频\car.mp4',
    'E:\file\clip\video\timer\时钟素材\h264时钟测试素材\10min_noAudio_60fps.mp4'
)

function Run-Probe {
    param([string]$VideoPath, [string]$OutFile)

    $elapsed = Measure-Command {
        & $ffprobe -v error -select_streams v:0 -show_streams -show_frames `
            -show_entries 'stream=codec_name,width,height,avg_frame_rate,r_frame_rate:frame=key_frame,pict_type,best_effort_timestamp,best_effort_timestamp_time,pkt_duration,pkt_duration_time' `
            -of default=noprint_wrappers=0 "$VideoPath" *> "$OutFile"
    }

    return [PSCustomObject]@{
        seconds = [Math]::Round($elapsed.TotalSeconds, 3)
        exitCode = $LASTEXITCODE
        logBytes = (Get-Item $OutFile).Length
    }
}

function Run-ShowInfo {
    param([string]$VideoPath, [string]$OutFile)

    $elapsed = Measure-Command {
        & $ffmpeg -hide_banner -nostats -i "$VideoPath" -map 0:v:0 -an -sn -dn -vf showinfo -f null - *> "$OutFile"
    }

    return [PSCustomObject]@{
        seconds = [Math]::Round($elapsed.TotalSeconds, 3)
        exitCode = $LASTEXITCODE
        logBytes = (Get-Item $OutFile).Length
    }
}

$rows = @()

foreach ($video in $videos) {
    $name = [IO.Path]::GetFileNameWithoutExtension($video)

    $probeR1 = Join-Path $benchDir ($name + '_r1_ffprobe.log')
    $showR1 = Join-Path $benchDir ($name + '_r1_showinfo.log')
    if (Test-Path $probeR1) { Remove-Item $probeR1 -Force }
    if (Test-Path $showR1) { Remove-Item $showR1 -Force }

    $r1Probe = Run-Probe -VideoPath $video -OutFile $probeR1
    $r1Show = Run-ShowInfo -VideoPath $video -OutFile $showR1

    $rows += [PSCustomObject]@{
        video = $video
        round = 'r1_probe_then_show'
        ffprobe_seconds = $r1Probe.seconds
        ffprobe_exit = $r1Probe.exitCode
        ffprobe_log_bytes = $r1Probe.logBytes
        showinfo_seconds = $r1Show.seconds
        showinfo_exit = $r1Show.exitCode
        showinfo_log_bytes = $r1Show.logBytes
    }

    $probeR2 = Join-Path $benchDir ($name + '_r2_ffprobe.log')
    $showR2 = Join-Path $benchDir ($name + '_r2_showinfo.log')
    if (Test-Path $probeR2) { Remove-Item $probeR2 -Force }
    if (Test-Path $showR2) { Remove-Item $showR2 -Force }

    $r2Show = Run-ShowInfo -VideoPath $video -OutFile $showR2
    $r2Probe = Run-Probe -VideoPath $video -OutFile $probeR2

    $rows += [PSCustomObject]@{
        video = $video
        round = 'r2_show_then_probe'
        ffprobe_seconds = $r2Probe.seconds
        ffprobe_exit = $r2Probe.exitCode
        ffprobe_log_bytes = $r2Probe.logBytes
        showinfo_seconds = $r2Show.seconds
        showinfo_exit = $r2Show.exitCode
        showinfo_log_bytes = $r2Show.logBytes
    }
}

$summary = $rows | Group-Object video | ForEach-Object {
    $g = $_.Group
    $ffprobeAvg = [Math]::Round((($g | Measure-Object ffprobe_seconds -Average).Average), 3)
    $showAvg = [Math]::Round((($g | Measure-Object showinfo_seconds -Average).Average), 3)
    $ffprobeBytes = [Math]::Round((($g | Measure-Object ffprobe_log_bytes -Average).Average), 0)
    $showBytes = [Math]::Round((($g | Measure-Object showinfo_log_bytes -Average).Average), 0)

    [PSCustomObject]@{
        video = $_.Name
        ffprobe_avg_seconds = $ffprobeAvg
        showinfo_avg_seconds = $showAvg
        speedup_showinfo_over_ffprobe = [Math]::Round(($showAvg / [Math]::Max($ffprobeAvg, 0.0001)), 3)
        ffprobe_avg_log_bytes = $ffprobeBytes
        showinfo_avg_log_bytes = $showBytes
        showinfo_over_ffprobe_log_ratio = [Math]::Round(($showBytes / [Math]::Max($ffprobeBytes, 1)), 3)
    }
}

$roundsCsv = Join-Path $benchDir 'benchmark_rounds.csv'
$summaryCsv = Join-Path $benchDir 'benchmark_summary.csv'
$rows | Export-Csv -Path $roundsCsv -NoTypeInformation -Encoding UTF8
$summary | Export-Csv -Path $summaryCsv -NoTypeInformation -Encoding UTF8

Write-Output 'Benchmark completed.'
Write-Output "rounds_csv=$roundsCsv"
Write-Output "summary_csv=$summaryCsv"
