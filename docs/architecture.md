# Architecture

## Modules

1. MainWindow
   - Owns UI composition and workflow orchestration.
2. FfmpegRunner
   - Resolves bundled ffprobe/ffmpeg paths.
   - Prefers ffprobe frame extraction and falls back to ffmpeg showinfo.
   - Streams process output to log file.
   - Provides in-app benchmark comparison execution for ffprobe vs ffmpeg showinfo.
   - Appends benchmark reports and lightweight metrics under logs directory.
3. ShowInfoParser
   - Parses frame rows from ffprobe or showinfo logs.
   - Builds GOP segments.
   - Builds summary metrics.
4. TimelineView
   - Custom paint widget for frame distribution and GOP boundaries.
5. SummaryPanel
   - Displays summary and selected frame/GOP details.
6. FrameTableView
   - Displays frame rows and filter controls.

## Data Flow

1. User selects video.
2. MainWindow calls FfmpegRunner.
3. FfmpegRunner outputs log path.
4. MainWindow calls ShowInfoParser.
5. Parser returns frames + gops + summary.
6. MainWindow updates TimelineView, SummaryPanel, FrameTableView.

## Benchmark Flow

1. User selects video.
2. MainWindow triggers benchmark action.
3. FfmpegRunner runs ffprobe extraction and ffmpeg showinfo extraction independently.
4. Runner writes per-run logs under logs/benchmark.
5. Runner appends comparison row to logs/benchmark/benchmark_comparison.tsv.
6. MainWindow shows one-line benchmark summary and report location in completion dialog.

## Directory Conventions

1. src/models: data structures.
2. src/services: external process integration.
3. src/parsers: text parsing and metrics derivation.
4. src/widgets: reusable UI widgets.
5. docs: design and change records.
