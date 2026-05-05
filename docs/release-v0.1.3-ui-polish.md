# v0.1.3-ui-polish

Release date: 2026-05-02 → updated 2026-05-05

This release focuses on adaptive UI behavior, readability polish, and a complete rounded-widget visual theme with brand colour tokens.

## Highlights

1. Improved adaptive toolbar layout and control grouping to better fit different window sizes.
2. Reduced text truncation risk for action buttons in maximized and wide-window scenarios.
3. Polished spacing and alignment for top-level controls to improve visual consistency.
4. Refined interactive details for smoother day-to-day operation in the main workflow.
5. Applied a global rounded-widget theme (cool-blue-grey palette, 8–12 px radii throughout).
6. Introduced accent colour for the primary `Start Analyze` button to distinguish it from secondary actions.
7. Refactored stylesheet to use six named colour-token variables — one-line retheming going forward.
8. Navigation and zoom toolbar buttons are now rendered as seamless capsule groups.
9. Source card (left) and Summary card (right) use identical border/radius/background, forming a balanced three-column grid.

## Scope

1. UI/UX refinement in main window layout, control arrangement, and visual style.
2. No breaking data format changes.
3. Existing analysis workflow remains compatible.

## Commits Included

1. `a8a1c33` - ui: improve adaptive toolbar layout and control grouping
2. `2fb7f34` - ui: final polish for adaptive controls and readability
3. *(pending)* - style: rounded UI theme with colour tokens, capsule toolbar, and aligned side cards
