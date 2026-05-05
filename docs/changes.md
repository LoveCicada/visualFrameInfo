# Feature Change Log

## 2026-04-26

### F-001 Initial project scaffold and workflow

1. Created Qt Widgets C++11 project scaffold.
2. Added ffmpeg showinfo execution service with fixed relative path policy.
3. Added showinfo parser and GOP summary calculations.
4. Implemented three-part analysis UI: source panel, timeline panel, summary panel.
5. Added frame detail table with type/key filtering.
6. Added design docs: requirements, ui spec, architecture, diagrams.

### Impact

1. First runnable baseline for single-file GOP visualization is available.

### F-002 Source metadata and navigation closure

1. Added codec/resolution/fps extraction from ffmpeg showinfo log into analysis summary.
2. Bound Source panel fields to parsed metadata values after each analysis run.
3. Added timeline toolbar controls for GOP jump and frame jump.
4. Added selection-driven timeline auto-scroll so jumps land visually in viewport.
5. Verified full qmake+nmake compilation in VS Developer Command Prompt context.

### Impact (F-002)

1. UI display loop is now complete for source metadata and direct GOP/frame navigation.

### F-003 Build system migration to CMake

1. Added CMake project entry with Qt5 Core/Gui/Widgets linkage.
2. Added CMake preset for MSVC x64 and Qt 5.12.12 prefix path.
3. Removed qmake/pro project file and generated qmake build artifacts.
4. Built successfully with CMake configure and build presets.

### Impact (F-003)

1. Project now uses CMake as the only build system.

### F-004 VS Code CMake Tools one-click workflow

1. Added workspace CMake Tools settings for preset-based configure/build defaults.
2. Added CMakeUserPresets aliases (`default`, `default-build`) for stable workspace selection.
3. Executed CMake Tools configure command and validated Build button path succeeds.

### Impact (F-004)

1. VS Code CMake Tools build flow is now directly usable after opening workspace.

### F-005 VS Code F5 debug integration

1. Added workspace launch configuration for CMake launch target path.
2. Bound debugger to `${command:cmake.launchTargetPath}` so executable path follows selected CMake target.

### Impact (F-005)

1. Pressing F5 now directly launches the current CMake target in VS Code.

### F-006 Debug and Release one-click launch split

1. Added dedicated Release launch configuration alongside existing Debug launch configuration.
2. Added VS Code build tasks for `default-debug-build` and `default-release-build` presets.
3. Added explicit Debug/Release build presets in CMakeUserPresets for deterministic mode switching.

### Impact (F-006)

1. Users can switch run mode in one click by selecting Debug or Release launch profile.

### F-007 Development documentation for startup modes

1. Added development guide documenting VS Code Debug/Release startup profiles and build presets.
2. Added command-line build instructions for Debug/Release modes.

### Impact (F-007)

1. Team members can follow a single documented startup workflow for both Debug and Release.

### F-008 Troubleshooting section for startup workflow

1. Added common troubleshooting section to development guide.
2. Documented check order for configure preset activation, Qt path mismatch, and F5 launch failures.
3. Added safe recovery sequence for inconsistent CMake/launch state.

### Impact (F-008)

1. Startup and build issues can be diagnosed with a deterministic step-by-step checklist.

### F-009 Runtime robustness for ffmpeg path discovery

1. Improved ffmpeg path lookup to support running executable from nested CMake output directories.
2. Added parent-directory search strategy for `ffmpeg/ffmpeg.exe` from app and current working directories.

### Impact (F-009)

1. Application no longer fails with `ffmpeg.exe not found under ./ffmpeg` when started from build output folders.

### F-010 Timeline visual and zoom interaction polish

1. Redesigned timeline paint style with layered background, GOP banding, frame ruler, and selection crosshair.
2. Added frame hover tooltip for quick readout (frame index, time, type, GOP id).
3. Added zoom slider and percentage label to make zoom state explicit.
4. Added Ctrl + mouse wheel zoom support with anchor-based zoom behavior.
5. Improved zoom button behavior to preserve visual focus around viewport center.

### Impact (F-010)

1. Timeline area is more readable and zoom interactions are smoother and easier to control.

### F-011 High-density long-video timeline mode

1. Added density mode switch with `Frame Mode` and `GOP Compact` options.
2. Added mini overview bar for full-range timeline preview and quick jump.
3. Added viewport-range sync between scroll position and overview highlight.
4. Added frame/GOP-aware coordinate mapping to keep zoom/selection centering accurate across modes.

### Impact (F-011)

1. Long videos are easier to navigate with compact GOP visualization and overview-assisted positioning.

### F-012 Extended navigation and imported-log workflow

1. Added `Open Analysis Log` workflow to parse previously generated showinfo logs directly.
2. Added Prev/Next frame buttons and Prev/Next I-frame buttons in the timeline toolbar.
3. Added left/right keyboard navigation for frame stepping.
4. Improved frame detail table to support horizontal scrolling for long raw log lines.

### Impact (F-012)

1. Users can reopen historical analysis results and navigate frame structure more efficiently.

### F-013 Adaptive compact labels and variable frame bar heights

1. Added adaptive GOP Compact labels that reveal GOP number and size only at useful zoom levels.
2. Changed frame timeline bars to use different heights for I, P, and B frames.
3. Added GOP-specific tooltip content in compact mode.

### Impact (F-013)

1. Dense timelines remain readable at distance while frame-type differences are visually stronger.

### F-014 Timeline toolbar frame-type legend

1. Added inline I/P/B color legend chips directly to the timeline toolbar.
2. Kept legend colors consistent with frame bars and compact-mode composition colors.

### Impact (F-014)

1. Users can identify I/P/B colors without shifting attention to another panel.

### F-015 B-frame visual separation in dense runs

1. Added dashed outline/separator rendering for each B-frame bar in Frame Mode.
2. Added narrow-width fallback separator so adjacent B frames remain distinguishable at high zoom-out levels.

### Impact (F-015)

1. Consecutive B-frame bars are visually separated, improving frame-index distinction during inspection.

### F-016 Optional B-frame separator toggle

1. Added timeline toolbar checkbox `B separator` to enable or disable dashed B-frame separators.
2. Default value is ON for dense inspection; users can turn it OFF for cleaner visual style.

### Impact (F-016)

1. Users can switch between high-density readability and cleaner timeline appearance without code changes.

### F-017 Analysis log naming prefixed by source video name

1. Updated generated analysis log naming from `showinfo_yyyyMMdd_HHmmss.log` to `<videoBaseName>_showinfo_yyyyMMdd_HHmmss.log`.
2. Kept existing timestamp-based uniqueness and output directory policy unchanged.

### Impact (F-017)

1. Generated logs are easier to trace back to their source video file when analyzing multiple materials.

### F-018 Non-blocking analysis with progress indicator

1. Moved `Start Analyze` workflow to background thread using `QtConcurrent::run`.
2. Added status bar progress indicator (busy progress bar) during ffmpeg run and parsing.
3. Disabled related actions while analysis is running, and restored controls after completion.

### Impact (F-018)

1. Main UI remains responsive during long analysis tasks and clearly communicates in-progress state.

## 2026-04-28

### F-019 In-app benchmark comparison workflow

1. Added `Run Benchmark` action in MainWindow top toolbar.
2. Implemented asynchronous benchmark execution in UI to avoid blocking.
3. Added service-level benchmark comparison runner for ffprobe and ffmpeg showinfo.
4. Added per-run benchmark logs under logs/benchmark.
5. Added benchmark comparison report append file logs/benchmark/benchmark_comparison.tsv.

### Impact (F-019)

1. Backend comparison can now be executed fully inside the application without terminal scripts.

### F-020 Benchmark completion summary dialog

1. Added one-line benchmark summary generation in service layer based on elapsed time and log size ratios.
2. Updated benchmark completion dialog to display summary line and report path.

### Impact (F-020)

1. Users can read the comparison conclusion directly from the UI without opening raw reports first.

### F-021 Release deploy pipeline with windeployqt

1. Added deploy script `scripts/deploy_release.ps1` to create `install/Release` output folder.
2. Script now copies `build/cmake-msvc-release/Release/visualFrameInfo.exe` to deploy folder.
3. Script auto-detects `windeployqt.exe` from `CMAKE_PREFIX_PATH` (with PATH fallback) and deploys Qt runtime dependencies.
4. Script copies workspace-level `ffmpeg` folder to `install/Release/ffmpeg` for runtime tool availability.
5. Added VS Code task chain `Build And Deploy Release` to run build and deploy steps sequentially.
6. Updated development guide with release deploy workflow and troubleshooting notes.

### Impact (F-021)

1. Team can generate a runnable Release package in one step with consistent Qt and ffmpeg runtime dependencies.

## 2026-04-29

### F-022 Inno Setup installer packaging workflow

1. Added Inno Setup script `scripts/inno/visualFrameInfo.iss` to package deployed folder content.
2. Added packaging helper script `scripts/package_inno.ps1` with ISCC auto-detection from `PATH`, `INNO_SETUP_HOME`, and default install locations.
3. Added VS Code tasks `Package Inno Setup Installer` and `Build Deploy And Package Installer`.
4. Updated development guide with Inno Setup prerequisites, command-line usage, and troubleshooting.

### Impact (F-022)

1. Project can generate a standard Windows installer executable from `install/Release` using a repeatable workflow.

### F-023 Auto-versioned installer package and upgrade validation checklist

1. Updated `scripts/package_inno.ps1` to parse project version from `CMakeLists.txt` `project(... VERSION ...)`.
2. Updated Inno compile command to pass `/DMyAppVersion=<parsed-version>` so installer version and output filename are no longer hardcoded.
3. Updated `scripts/inno/visualFrameInfo.iss` to allow command-line override for `MyAppVersion`.
4. Added installer upgrade validation checklist in development guide, covering overwrite upgrade, shortcut checks, and uninstall residual checks.

### Impact (F-023)

1. Installer package version now stays aligned with CMake project version automatically and release verification process is more standardized.

### F-024 Installer filename transition log for version changes

1. Enhanced `scripts/package_inno.ps1` to detect previous versioned installer artifact in `install/installer`.
2. Packaging log now prints installer filename transition in format `old-installer.exe -> new-installer.exe`.

### Impact (F-024)

1. Release verification can quickly confirm installer version progression directly from packaging logs.

### F-025 Optional frame preview on I/P/B frame positioning

1. Added timeline-area `Frame Preview` toggle, default OFF.
2. Added preview panel below timeline area to show still frame image for selected frame position.
3. Bound preview update to unified frame selection path so timeline/table/jump/navigation all trigger the same preview flow.
4. Added guard behavior for log-only mode: preview toggle stays disabled without valid source video.
5. Added ffmpeg-based frame capture API with per-video+frame cache under `logs/preview_cache`.
6. Added async preview request handling with stale-result suppression to keep UI responsive during rapid frame navigation.

### Impact (F-025)

1. Users can optionally inspect actual video image context while navigating GOP/frame structure, without introducing embedded playback dependency.

## 2026-05-05

### F-026 Frame size parsing and average bitrate display

1. Extended ffprobe frame field collection to include `pkt_size`.
2. Added parser mapping for per-frame packet size and derived per-frame bitrate.
3. Added summary-level average bitrate calculation based on total frame bytes and total duration.
4. Added Source panel field `Average Bitrate` in main UI.
5. Kept fallback behavior as `-` when imported historical logs do not contain frame size data.

### Impact (F-026)

1. Source panel now shows end-to-end estimated average bitrate for newly analyzed videos.
2. Bitrate calculation is now performed in parser/service data pipeline rather than UI-layer placeholders.

### F-027 Frame and GOP data CSV export

1. Added toolbar action `Export CSV...` in the main window.
2. Implemented frame-level CSV export fields: `index, pts_time, type, isKey, gopIndex, indexInGop, durationTime, pktSize`.
3. Added optional second export for GOP summary CSV fields: `gopIndex, startFrame, endFrame, size, startTime, endTime`.
4. Added default output naming `<video_stem>_frames.csv` and sibling GOP file suffix `_gops.csv`.

### Impact (F-027)

1. Users can now post-process frame and GOP data in spreadsheet or scripting workflows.

### F-028 Analysis report export (txt / html)

1. Added toolbar action `Export Report...` in the main window.
2. Users can choose `.txt` (plain text) or `.html` (styled HTML) format via file save dialog.
3. Report includes: video source path, total frames, I/P/B frame counts and ratios, average bitrate, total GOP count, average/min/max GOP size, and export timestamp.
4. Default output filename: `<video_stem>_report.txt` or `<video_stem>_report.html`.

### Impact (F-028)

1. Users can generate standalone analysis reports for archiving or sharing without extra tooling.

### F-029 Frame / time range filter

1. Added a dedicated `Range` filter row above the frame table.
2. Added start/end frame `QSpinBox` filters and start/end time `QDoubleSpinBox` filters.
3. Added `Clear Filter` action to reset all range constraints back to unconstrained.
4. Frames outside the active range remain visible in the timeline with reduced opacity instead of being removed.

### Impact (F-029)

1. Long videos can now be narrowed to a target segment without losing positional context in the timeline.

### F-030 Drag-and-drop video file loading

1. Added `dragEnterEvent` and `dropEvent` handlers to `MainWindow` for accepting video file drops.
2. Accepted formats: any single file path ending in a recognised video extension.
3. Drop sets `m_videoPath`, updates path edit and source info in the same code path as manual file selection.

### Impact (F-030)

1. Users can open a video for analysis by dragging it directly from Explorer onto the application window.

### F-031 Recent files quick-access combo

1. Added persistent recent-files list stored in `QSettings` (key `recentFiles`, max 10 entries).
2. Added `QComboBox` in the top toolbar row for one-click re-open of previously analyzed files.
3. Added inline `Clear` entry at the bottom of the combo to wipe the history.
4. Fixed combo not firing selection when the same single item is chosen by switching from `currentIndexChanged` to `activated` signal.

### Impact (F-031)

1. Re-opening a previously analyzed video no longer requires re-navigating the file system.

### F-032 GOP size histogram widget

1. Added `GopHistogramWidget` custom QWidget with auto-bucketed bar chart of GOP sizes.
2. Added live mouse-hover tooltip showing bucket range and count.
3. Integrated into a collapsible section in the right summary panel, default collapsed.

### Impact (F-032)

1. GOP size distribution is now visible as an in-app histogram without external tooling.

### F-033 Enhanced video metadata (color space, bit depth, pixel format, duration)

1. Switched ffprobe stream output format from CSV to key-value (`-of default=noprint_wrappers=0:nokey=0`) to prevent column-offset misparse.
2. Added parsing of `color_space`, `bits_per_raw_sample`, `pix_fmt`, and `duration` from stream block.
3. Added `inferBitDepthFromPixFmt()` fallback so bit depth is derived from pixel format string when raw sample depth is absent.
4. Extended `AnalysisSummary` model and `SummaryPanel` UI with a collapsible "More Info" section showing the new fields.

### Impact (F-033)

1. Source panel now reports color space, bit depth, pixel format, and duration for newly analyzed videos.

### F-034 Batch analysis with summary table and CSV export

1. Added `Batch Analyze…` menu action to select multiple video files in one dialog.
2. Implemented serial background analysis via `QtConcurrent::run` with per-file progress reporting.
3. Added batch results dialog showing a sortable table (file, frames, I/P/B counts, avg bitrate, resolution, fps).
4. Added `Export CSV…` action in the batch results dialog to save the table as a UTF-8 CSV file.

### Impact (F-034)

1. Multiple video files can be profiled in a single batch run and results compared or exported without switching files manually.

## 2026-05-05

### UI-001 Rounded widget theme (Round 1 – baseline rounded style)

1. Applied global `QStyleSheet` via `buildRoundedUiStyleSheet()` in `MainWindow::setupUi()`.
2. Rounded all major controls: buttons (8 px), inputs (8 px), group-box cards (12 px), table (10 px), progress bar (8 px), scroll handles (6 px), check-box indicator (5 px).
3. Replaced default flat palette with a cool-blue-grey surface palette for window, card, and input backgrounds.

### Impact (UI-001)

1. Application visual style is consistent and modern without requiring a third-party style library.

### UI-002 Accent button and card surface quality (Round 2 – refinement)

1. Applied accent colour (`#5a8ddf`) to the primary `Start Analyze` button via `setProperty("accent", true")` and a dedicated `QPushButton[accent="true"]` CSS selector.
2. Increased GroupBox card surface contrast (`#f7faff` background, `#ccd6e6` border).
3. Refined hover / pressed states for all button and input variants.
4. Polished alternate table row colour, header section, and scroll-bar handle appearance.

### Impact (UI-002)

1. Primary action button is clearly visually distinguished from secondary actions.
2. GroupBox cards have a card-like surface that separates them from the window background.

### UI-003 Theme tokens, capsule toolbar groups, and aligned side cards (Round 3 – brand polish)

1. Refactored `buildRoundedUiStyleSheet()` to declare six named theme-token `const QString` variables (`bg`, `card`, `border`, `accent`, `accentDk`, `text`) plus derived tokens; entire style string uses `.arg()` substitution so future retheming requires only editing the six top-level values.
2. Added `QPushButton[capsule="first/mid/last"]` CSS rules that selectively remove inner border-radii and borders, creating a seamless capsule grouping effect. Applied to the navigation cluster (`Prev Frame / Next Frame / Prev I / Next I`) and zoom cluster (`Zoom - / Reset / Zoom +`) in the timeline toolbar via `setProperty("capsule", …)`.
3. Set `rightPanel->setObjectName("summaryCard")` and added matching `QWidget#summaryCard` CSS rule (same border, radius, and background as `QGroupBox`). Set matching `minimumWidth(220)` / `maximumWidth(340)` so both side panels form a consistent visual grid.

### Impact (UI-003)

1. Application colour scheme can be changed globally by editing six token values in one function.
2. Navigation and zoom buttons are visually grouped as capsules for a modern toolbar appearance.
3. Source card (left) and Summary card (right) are visually identical in border, radius, background, and width range, forming a balanced three-column layout.
