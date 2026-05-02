# UI Spec

## Layout

1. Top operation bar: select video, start analyze, run benchmark, reanalyze, open log, status text.
2. Top operation bar also supports opening an existing analysis log file for direct parsing.
   - During `Start Analyze`, status bar shows busy progress indicator and keeps UI responsive.
   - During `Run Benchmark`, status bar shows indeterminate progress and disables analyze-related actions.
3. Middle horizontal split:
   - Left source panel: file and source metadata.
   - Center timeline panel: frame timeline canvas in scroll area, GOP/frame jump controls, zoom controls, and optional frame preview area below timeline.
   - Right summary panel: GOP metrics and selected frame details.
4. Bottom detail panel: frame table with type and key-frame filters.

## Display Items

1. Timeline
   - I/P/B frame blocks with colors.
   - I/P/B frame bars use different heights.
   - B-frame bars include dashed per-frame separators for adjacent-frame distinction.
   - I frame boundary marker line.
   - GOP alternating background bands for structure readability.
   - Frame ruler labels for navigation context.
   - Selected frame highlight.
   - Hover tooltip with frame time/type/GOP details.
   - Density switch: Frame Mode / GOP Compact.
   - Mini overview bar with viewport range highlight and click-to-jump.
   - Inline toolbar legend chips for I/P/B colors.
   - Prev/Next Frame and Prev/Next I-frame navigation buttons.
   - Optional `B separator` toggle for dashed B-frame per-frame boundaries.
   - Optional `Frame Preview` toggle (default OFF, enabled only when a valid video file is selected).
   - Frame preview area displays a still image at selected frame position when toggle is ON.
2. Summary
   - Total frames.
   - I/P/B counts.
   - GOP count.
   - Avg/min/max GOP size.
   - Average GOP interval in frames and seconds.
3. Table columns
   - Frame index.
   - pts_time.
   - Frame type.
   - Key frame flag.
   - GOP index.
   - Index in GOP.
   - Duration.
   - Raw line excerpt.

## Interaction Rules

1. Selecting a frame in timeline selects corresponding row in table.
2. Selecting a row in table highlights corresponding frame in timeline.
3. Type filter in table updates timeline visibility.
4. Zoom controls resize timeline width.
5. GOP jump selects GOP start frame and centers it in timeline viewport.
6. Frame jump selects target frame and centers it in timeline viewport.
7. Ctrl + mouse wheel triggers anchored timeline zoom.
8. In GOP Compact mode, timeline width scales by GOP count for high-density navigation.
9. Overview bar drag/click updates selected frame and centers timeline accordingly.
10. Left/Right keyboard keys move to previous/next frame.
11. Bottom raw data column supports horizontal scrolling for full text inspection.
12. Benchmark completion dialog shows one-line summary and comparison report path.
13. Benchmark report appends to logs/benchmark/benchmark_comparison.tsv.
14. Frame Preview toggle is disabled when only log file is loaded and no valid source video exists.
15. Any frame selection path (timeline click, table click, jump controls, prev/next navigation) updates preview image when Frame Preview is enabled.
