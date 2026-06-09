# Watchface Punchlist

This punchlist consolidates the current handoff backlog, active platform
planning, and useful cleanup/refactoring ideas from the old Cursor export.
Some items are already done or may be irrelevant, but they are captured here
so they can be reviewed deliberately.

## Active Work

1. Plan round-display layout separately before adding `chalk` or `gabbro`.
   - Preserve the inspect, identify, audit, confirm, execute, validate cycle.
   - Research Pebble/Rebble round-display guidance and community examples.
   - Keep `content_width_date`, `content_width_time`,
     `content_width_health`, and `content_width_bottom` separate so
     round layouts can shrink widths by row later.
   - Do not assume a scaled rectangular layout is enough for round displays.

2. Continue `main.c` modularization only where a real boundary remains.
   - Settings, layout/surface, helper, BPM, steps, battery, and climate
     have been split out.
   - Remaining candidates are narrower: messaging/weather receive flow and
     future round-surface decisions.
   - Extract helpers only when they clarify a real seam or reduce meaningful
     duplication.

3. Keep rectangular support validation visible.
   - Current rectangular targets are `aplite`, `basalt`, `diorite`, `emery`,
     and `flint`.
   - Revalidate smaller rectangular bounds after future layout or typography
     changes.
   - Keep `sdkVersion: "3"` unless Pebble tooling requirements change.

## Cleanup And Refactoring Candidates

1. Review `settings_load()` persisted-struct tolerance.
   - Current code reads the persisted settings struct directly.
   - Before platform expansion, decide whether to handle older struct shapes
     or version persisted settings.

2. Split state into explicit structs when it improves clarity.
   - Candidate buckets: settings, runtime metrics, layout, and render state.
   - Keep the first split small and behavior-preserving.

3. Preserve fixed static text-buffer discipline.
   - Keep `MAX_STR_LEN` and `TextBufferId` centralized.
   - Re-check buffer cleanup if state or lifecycle code is split.

4. Review phone weather fallback configurability.
   - Current fallback behavior should be verified in `src/pkjs/index.js`.
   - Decide whether fixed coordinates should become configurable,
     documented, or replaced by a localStorage cache-first fallback.

5. Audit GitHub-facing naming and artifacts before initial publish prep.
   - Check app/package names, generated bundle name, README, screenshots,
     `.gitignore`, and private-data hygiene.

6. Keep system font cleanup simple.
   - System fonts from `fonts_get_system_font()` do not need unloading.
   - If custom fonts are added later, unload them after layers are destroyed.

## Completed Or Recently Addressed

1. Docs drift was reconciled against the current implementation.
2. `refresh_watchface_display()` was audited and made responsible for palette
   refresh on display-mode updates.
3. `WatchfaceSurface` was extracted to group current layer frames,
   palette, typography, and substrata.
4. `update_step_count()` was renamed to `update_steps()`.
5. Background layer frame now derives from root bounds.
6. Rule left edge, right edge, and y-position now derive from layout state.
7. Time and health row placement now uses a derived row gap.
8. README and AGENTS geometry were updated for the resulting Emery layout.
9. Stale rail constants were removed after the horizontal-rule layout
   decision.
10. Text layer updates were centralized through `update_text_layer_display()`.
11. `CONTENT_X` was removed from the layout code.
12. Rectangular spacing now derives from display height in
    `layout_calculate_surface()`.
13. `layout_calculate_surface()` now fills caller-owned
    `WatchfaceSurface` storage.
14. Battery icon drawing is horizontal and bounds-derived.
15. Fallback heart icon drawing uses the named icon drawing grid.
16. Rebble Clay is wired through `@rebble/clay`.
17. Rectangular target platforms now include `aplite`, `basalt`, `diorite`,
    `emery`, and `flint`.
18. Color and black-and-white display palettes use `PBL_IF_COLOR_ELSE()`
    fallbacks where appropriate.
19. Climate state and weather condition rendering are owned by
    `src/modules/climate.c`.
20. PebbleKit JS sends raw Open-Meteo `weather_code` values to C.
21. Weather codes are mapped into private procedural glyph buckets in C.
22. Weather glyphs are lightweight file-local renderers.
23. Snow grains are folded into snow, freezing drizzle and freezing rain
    share one glyph, and unknown conditions render as a question mark.
24. BPM, steps, and temperature use `FONT_KEY_GOTHIC_18`; battery keeps
    `FONT_KEY_GOTHIC_18_BOLD`.
25. Temperature text width was expanded to 40px for 3-digit Fahrenheit values.
26. Shared icon scaling moved to `src/modules/helper.h`.
27. `WatchfaceSurface` moved to `src/modules/layout.c`.
28. Palette and text-layer display helpers moved to
    `src/modules/layout.c`.
29. Persisted settings defaults, validation, load, and save moved to
    `src/modules/settings.c`.
30. BPM and steps moved to separate `src/modules/bpm.c` and
    `src/modules/steps.c` modules, and BPM now uses only a procedural
    heart icon.
31. The BPM PDC resource and hidden icon fallback AppMessage setting were
    removed.
32. Battery state, rendering, and text updates moved to
    `src/modules/battery.c`.

## Validation Reminders

1. Run `pebble build` after each committed slice.
2. For visual changes, install to Emery and toggle:
   - `10006=0` dark mode
   - `10006=1` light mode
3. For display refresh changes, explicitly audit:
   - every `layer_set_update_proc`
   - every `layer_mark_dirty`
   - every `text_layer_set_text_color`
4. For platform work, maintain a small platform matrix:
   - bounds
   - shape
   - color capability
   - health sensors
   - resource variants
   - SDK constraints
