$ErrorActionPreference = 'Stop'
$ffprobe = 'E:\ownCode\visualFrameInfo\ffmpeg\ffprobe.exe'
$ffmpeg  = 'E:\ownCode\visualFrameInfo\ffmpeg\ffmpeg.exe'
$tmpDir  = 'E:\ownCode\visualFrameInfo\logs\benchmark'
New-Item -ItemType Directory -Path $tmpDir -Force | Out-Null

$videos = @(
    'E:\file\clip\video\同步测试视频\car.mp4',
    'E:\file\clip\video\timer\时钟素材\h264时钟测试素材\10min_noAudio_60fps.mp4'
)

function Invoke-Timed {
    param([string]$Label, [string[]]$Cmd, [string]$OutFile)
    Write-Host "  Running $Label ..."
    $t = Measure-Command { & $Cmd[0] $Cmd[1..($Cmd.Count-1)] *> $OutFile }
    [PSCustomObject]@{
        label  = $Label
        sec    = [Math]::Round($t.TotalSeconds, 2)
        bytes  = (Get-Item $OutFile).Length
        exit   = $LASTEXITCODE
    }
}

$allRows = @()
foreach ($v in $videos) {
    $name = [IO.Path]::GetFileNameWithoutExtension($v)
    Write-Host "`n=== $name ==="

    $showEntries = 'stream=codec_name,width,height,avg_frame_rate,r_frame_rate:frame=key_frame,pict_type,best_effort_timestamp,best_effort_timestamp_time,pkt_duration,pkt_duration_time'

    $probOld = Invoke-Timed 'ffprobe-OLD' @($ffprobe,'-v','error','-select_streams','v:0','-show_streams','-show_frames','-show_entries',$showEntries,'-of','default=noprint_wrappers=0',$v) "$tmpDir\${name}_old_ffprobe.log"
    $probNew = Invoke-Timed 'ffprobe-NEW' @($ffprobe,'-v','error','-threads','0','-select_streams','v:0','-show_streams','-show_frames','-show_entries',$showEntries,'-of','default=noprint_wrappers=0',$v) "$tmpDir\${name}_new_ffprobe.log"
    $showOld = Invoke-Timed 'showinfo-OLD' @($ffmpeg,'-hide_banner','-nostats','-i',$v,'-map','0:v:0','-an','-sn','-dn','-vf','showinfo','-f','null','-') "$tmpDir\${name}_old_ffmpeg.log"
    $showNew = Invoke-Timed 'showinfo-NEW' @($ffmpeg,'-hide_banner','-nostats','-threads','0','-i',$v,'-map','0:v:0','-an','-sn','-dn','-vf','showinfo=checksum=0','-f','null','-') "$tmpDir\${name}_new_ffmpeg.log"

    $fOldProbe = (Select-String -Pattern '\[FRAME\]' -Path "$tmpDir\${name}_old_ffprobe.log").Count
    $fNewProbe = (Select-String -Pattern '\[FRAME\]' -Path "$tmpDir\${name}_new_ffprobe.log").Count
    $fOldShow  = (Select-String -Pattern 'Parsed_showinfo.*type:' -Path "$tmpDir\${name}_old_ffmpeg.log").Count
    $fNewShow  = (Select-String -Pattern 'Parsed_showinfo.*type:' -Path "$tmpDir\${name}_new_ffmpeg.log").Count

    Write-Host ""
    Write-Host "  [ffprobe]  OLD $($probOld.sec)s / $($probOld.bytes) bytes  ->  NEW $($probNew.sec)s / $($probNew.bytes) bytes   frame-match=$($fOldProbe -eq $fNewProbe) ($fOldProbe vs $fNewProbe)"
    Write-Host "  [showinfo] OLD $($showOld.sec)s / $($showOld.bytes) bytes  ->  NEW $($showNew.sec)s / $($showNew.bytes) bytes   frame-match=$($fOldShow -eq $fNewShow) ($fOldShow vs $fNewShow)"

    $allRows += [PSCustomObject]@{
        video              = $name
        probe_old_sec      = $probOld.sec
        probe_new_sec      = $probNew.sec
        probe_delta_pct    = [Math]::Round(($probNew.sec - $probOld.sec) / [Math]::Max($probOld.sec,0.001) * 100, 1)
        probe_bytes_old    = $probOld.bytes
        probe_bytes_new    = $probNew.bytes
        probe_frames_ok    = ($fOldProbe -eq $fNewProbe)
        show_old_sec       = $showOld.sec
        show_new_sec       = $showNew.sec
        show_delta_pct     = [Math]::Round(($showNew.sec - $showOld.sec) / [Math]::Max($showOld.sec,0.001) * 100, 1)
        show_bytes_old     = $showOld.bytes
        show_bytes_new     = $showNew.bytes
        show_frames_ok     = ($fOldShow -eq $fNewShow)
    }
}

Write-Host "`n====== SUMMARY ======"
$allRows | Format-Table -AutoSize
$csvPath = "$tmpDir\bench_compare_$(Get-Date -Format 'yyyyMMdd_HHmmss').csv"
$allRows | Export-Csv -Path $csvPath -NoTypeInformation -Encoding UTF8
Write-Host "CSV saved: $csvPath"
