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

1. Batch analysis. (Planned — see F9 in Pending Features, low priority.)
2. Embedded video playback.
3. ffmpeg SDK linking.

## Pending Features

The following features are identified, prioritized, and pending implementation. They are not yet in scope for the current release.

### F1 — Frame Size and Bitrate Visualization (Priority: High)

**Motivation**: `pkt_size` is not currently captured. Displaying per-frame size and estimated bitrate is a core capability for a frame analysis tool.

**Requirements**:
1. Add `qint64 pktSize = 0` field to `FrameInfo`.
2. `ShowInfoParser` extracts `pkt_size=` from ffprobe output and populates `FrameInfo::pktSize`.
3. `FrameTableView` adds a "Size (B)" column.
4. Add a `BitrateChartWidget` (embedded below the timeline or in a collapsible panel): X-axis = frame index, Y-axis = frame size in bytes; I/P/B bars use the same color scheme as the timeline; selected frame is highlighted in sync with the timeline.
5. `AnalysisSummary` gains two fields: `avgFrameSizeBytes` (double) and `estimatedBitrateKbps` (double, derived from total size / duration).

### F2 — Frame Data Export (CSV) (Priority: High)

**Motivation**: No data export capability exists; analysts need to post-process results in spreadsheet tools.

**Requirements**:
1. Add an "Export CSV..." button to the toolbar.
2. Export per-frame rows: `index, pts_time, type, isKey, gopIndex, indexInGop, durationTime, pktSize`.
3. An optional checkbox in the export dialog exports a second sheet/file with GOP summary rows: `gopIndex, startFrame, endFrame, size, startTime, endTime`.
4. Default filename is `<video_stem>_frames.csv`; a save-file dialog is presented.

### F3 — Analysis Report Export (Text / HTML) (Priority: Medium-High)

**Motivation**: Users need to share analysis conclusions with colleagues or archive results.

**Requirements**:
1. Add an "Export Report..." button to the toolbar or File menu.
2. Report content: video metadata + I/P/B frame counts and ratios + GOP statistics (min/max/avg/distribution summary) + generation timestamp.
3. Plain text (`.txt`) format is the baseline. HTML format (with minimal inline CSS) is a stretch goal.
4. Default filename is `<video_stem>_report.txt`.

### F4 — Drag-and-Drop File Open (Priority: Medium)

**Motivation**: Dragging a file onto the window is the standard desktop interaction; the current browse-dialog-only workflow is friction-heavy.

**Requirements**:
1. `MainWindow` sets `setAcceptDrops(true)` and overrides `dragEnterEvent` / `dropEvent`.
2. Dropping a video file (`.mp4 .mov .mkv .avi .mxf`) fills the path input and triggers analysis.
3. Dropping a log file (`.log .txt`) loads it directly via the existing `loadAnalysisLog` path.

### F5 — Recent Files List (Priority: Medium)

**Motivation**: Analysts repeatedly re-open the same files; the tool should remember past sessions.

**Requirements**:
1. On every successful analysis, the resolved video path is prepended to a `QSettings` list capped at 10 entries.
2. A drop-down button next to the video path input exposes the recent-file list, showing the filename as the label and the full path as a tooltip.
3. Selecting an entry fills the path input (analysis is not auto-started).

### F6 — Frame / Time Range Filter (Priority: Medium)

**Motivation**: Long videos require narrowing focus to a specific segment; the current filter only covers frame type.

**Requirements**:
1. Above the frame table, add a "Range" filter row: start-frame `QSpinBox`, end-frame `QSpinBox`, start-time (seconds) `QDoubleSpinBox`, end-time `QDoubleSpinBox`. Empty or zero-range fields mean no constraint.
2. Filtered-out frames are grayed out in the timeline (still drawn but at reduced opacity) rather than hidden, to preserve positional context.
3. A "Clear Filter" button resets all range controls to unconstrained.

### F7 — GOP Size Distribution Histogram (Priority: Medium)

**Motivation**: min/max/avg values in `SummaryPanel` do not reveal distribution shape or outliers.

**Requirements**:
1. Add a GOP size histogram widget below `SummaryPanel` (collapsible, default collapsed).
2. X-axis = GOP size buckets (auto-bucketed); Y-axis = GOP count per bucket.
3. Hovering a bucket displays a tooltip listing the GOP indices in that bucket.

### F8 — Enhanced Video Metadata (Priority: Low-Medium)

**Motivation**: `AnalysisSummary` currently only exposes codec, resolution, and fps, omitting fields engineers routinely need.

**Requirements**:
1. `AnalysisSummary` gains fields: `durationSeconds` (double), `estimatedBitrateKbps` (double), `colorSpace` (QString), `bitDepth` (int), `pixFmt` (QString).
2. `ShowInfoParser`/`FfmpegRunner` extracts these from ffprobe output where available.
3. The Source panel displays these in a collapsible "More Info" section (collapsed by default).

### F9 — Batch Analysis Mode (Priority: Low)

**Motivation**: Engineers commonly need to compare encoding quality across a set of source files.

**Requirements**:
1. A "Batch Analyze..." entry in the File menu opens a multi-file selection dialog.
2. Files are analyzed serially in the background; a progress bar shows `current / total` file count.
3. On completion, a summary table is shown: one row per file with columns `filename, totalFrames, iRatio%, pRatio%, bRatio%, avgGopSize`.
4. The summary table can be exported to CSV.

## Error Handling

1. Missing analysis executables.
2. Invalid video path.
3. ffprobe and ffmpeg execution failure.
4. Log parse failure.
5. Frame capture failure for preview image extraction.
