# Requirements

## Goal

Build a desktop tool with C++11 and Qt 5.12.12 to visualize I/B/P frame layout and GOP metrics for one video file.

## Functional Scope

1. Select one video file and run frame analysis (ffprobe preferred, ffmpeg showinfo fallback).
2. Read analysis log and parse frame rows.
3. Visualize frame timeline with I/P/B colors and GOP boundaries.
4. Show GOP metrics: count, avg/min/max size, average GOP interval by frames and seconds.
5. Show frame detail table and basic filters.
6. Open generated analysis log.
7. Trigger in-app benchmark comparison for ffprobe and ffmpeg showinfo on selected video.
8. Show one-line benchmark summary in completion dialog and persist comparison report.
9. Provide an optional frame preview toggle after a video is loaded; when enabled, display a still frame image for the currently selected I/P/B frame position.
10. Keep preview toggle disabled when only analysis log is loaded without a valid source video path.

## Non-Functional Scope

1. Use fixed relative bundled tools under workspace/app folder: ffmpeg/ffprobe.exe and ffmpeg/ffmpeg.exe.
2. Keep implementation in Qt Widgets.
3. Keep first version single-file workflow.
4. Benchmark comparison and analysis metrics are recorded as local TSV files under logs.

## Out Of Scope

1. Batch analysis.
2. Embedded video playback.
3. ffmpeg SDK linking.

## Error Handling

1. Missing analysis executables.
2. Invalid video path.
3. ffprobe and ffmpeg execution failure.
4. Log parse failure.
5. Frame capture failure for preview image extraction.
