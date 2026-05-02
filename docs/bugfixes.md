# Bugfix Log

## 2026-04-26

### B-001 Timeline filter signal linkage hardening

1. Replaced string-based signal/slot mapping with type-safe lambda bridge in MainWindow.
2. Avoided potential runtime mismatch between FrameTableView filter signal and TimelineView method.

### B-002 Zoom behavior consistency

1. Changed zoom actions from fixed value assignment to incremental zoom state updates.
2. Added reset behavior consistency when clearing analysis views.

### B-003 Metadata parsing fallback and build validation path

1. Added fps fallback extraction from stream lines containing `xx fps` when `frame_rate` line is unavailable.
2. Switched compile validation to direct VS developer command initialization call to avoid shell quoting failures.

### B-004 ffmpeg executable lookup under CMake build output

1. Reworked ffmpeg path resolution to search `ffmpeg/ffmpeg.exe` upwards from both application directory and current working directory.
2. Fixed runtime error where executable launched from `build/.../Debug` could not find workspace-level `ffmpeg` folder.

### B-005 timeline scrollbar sync and compact mode colors

1. Fixed timeline canvas sizing so horizontal scrollbar range matches actual drawn content width.
2. Switched overview highlight sync to viewport-ratio mapping for exact scrollbar alignment.
3. Changed GOP Compact mode to render I/P/B composition colors instead of alternating single-color blocks.
4. Further changed GOP Compact mode to use vertically stacked I/P/B color bands so P and B remain visually distinct even when GOP columns are very narrow.
