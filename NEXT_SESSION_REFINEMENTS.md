# Next Session Refinements

## Priority Candidates

1. Audit and fix docs drift.
   - `README.md` and `AGENTS.md` still contain some older geometry and font
     values.
   - Reconcile them against `src/c/main.c`, `src/c/ataglance.h`,
     `src/pkjs/config.json`, and `package.json`.
   - Acceptance: docs match current frames, fonts, colors, message keys,
     defaults, and hidden settings.

2. Inspect the full display refresh path.
   - Current entry point: `refresh_watchface_display()`.
   - Audit every draw/update path that reads `s_palette` or any derived visual
     state.
   - Confirm every palette-dependent layer is repainted on display-mode change.
   - Current known dirty marks: background/rule and steps icon.
   - Current data refreshes: date, time, steps, BPM, battery, temp.

3. Start the form-factor layout extraction.
   - Current layout is still Emery-pinned despite using `content_width`.
   - Extract a small `LayoutState` or `WatchfaceLayout` struct from
     `main_window_load()`.
   - Keep the first version behavior-equivalent for Emery.
   - Acceptance: no visible layout change on Emery, but frames are grouped in
     one derived layout object.

## Smaller Code Fixes

- Rename `update_step_count()` to `update_steps()` and fix nearby formatting.
- Revisit `init_background_layer()` using root bounds instead of
  `GRect(0, 0, 200, 228)`.
- Decide whether remaining column anchors should be derived from bounds before
  adding non-Emery rectangular targets.
- Clean up stale rail constants if no vertical rail is planned soon.
- Review whether `settings_load()` should tolerate persisted struct shape
  changes more defensively before platform expansion.
- Consider splitting settings, runtime metrics, layout, and render state into
  explicit structs after the layout extraction begins.

## Verification Ideas

- Run `pebble build` after each committed slice.
- For visual changes, install to Emery and toggle:
  - `10005=0` dark mode
  - `10005=1` light mode
  - `10004=0` normal icon mode
  - `10004=1` fallback icon mode
- For display refresh changes, explicitly audit every `layer_set_update_proc`,
  every `layer_mark_dirty`, and every `text_layer_set_text_color` call.
