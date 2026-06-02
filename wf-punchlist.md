# Watchface Punchlist

This punchlist consolidates the current handoff backlog, active platform
planning, and useful cleanup/refactoring ideas from the old Cursor export.
Some items are already done or may be irrelevant, but they are captured here
so they can be reviewed deliberately.

## Active Work

1. Plan platform-agnostic layout expansion before further code changes.
   - Preserve the inspect, identify, audit, confirm, execute, validate cycle.
   - Keep rectangular, monochrome, and round support as separate slices.

2. Add monochrome/BW display-mode support plan.
   - Start with a hidden Emery preview option.
   - Decide how dark/light mode maps onto B/W displays.
   - Replace color-only BPM and battery semantics with readable monochrome
     fallbacks before enabling real B/W targets.

3. Expand rectangular platform support after bounds-derived layout validation.
   - Rectangular content widths are the same for date, time, health, and
     environment rows.
   - Validate on smaller rectangular bounds before changing package targets.

4. Plan round-display layout separately.
   - Keep `content_width_date`, `content_width_time`,
     `content_width_health`, and `content_width_environment` separate so
     round layouts can shrink widths by row later.
   - Do not assume a scaled rectangular layout is enough for round displays.

5. Expand `package.json` target platforms only after rendering and config
   semantics are validated.
   - Keep `sdkVersion: "3"` unless Pebble tooling requirements change.
   - Update package metadata, Clay config, JS, C, docs, and defaults together
     for any new AppMessage keys.

## Cleanup And Refactoring Candidates

1. Clean up stale rail constants if no vertical rail is planned soon.
   - Review `RAIL_X`, `RAIL_TOP`, and `RAIL_BOTTOM`.
   - Keep or remove them based on an explicit layout decision.

2. Review `settings_load()` persisted-struct tolerance.
   - Current code reads the persisted settings struct directly.
   - Before platform expansion, decide whether to handle older struct shapes
     or version persisted settings.

3. Split state into explicit structs when it improves clarity.
   - Candidate buckets: settings, runtime metrics, layout, and render state.
   - Keep the first split small and behavior-preserving.

4. Preserve fixed static text-buffer discipline.
   - Keep `MAX_STR_LEN` and `TextBufferId` centralized.
   - Re-check buffer cleanup if state or lifecycle code is split.

5. Review phone weather fallback configurability.
   - Current fallback behavior should be verified in `src/pkjs/index.js`.
   - Decide whether SFO/OAK should be configurable or simply documented.

6. Audit GitHub-facing naming and artifacts before initial publish prep.
   - Check app/package names, generated bundle name, README, screenshots,
     `.gitignore`, and private-data hygiene.

7. Keep system font cleanup simple.
   - System fonts from `fonts_get_system_font()` do not need unloading.
   - If custom fonts are added later, unload them after layers are destroyed.

## Completed Or Recently Addressed

1. Docs drift was reconciled against the current implementation.
2. `refresh_watchface_display()` was audited and made responsible for palette
   refresh on display-mode updates.
3. `WatchfaceLayout` was extracted to group current layer frames.
4. `update_step_count()` was renamed to `update_steps()`.
5. Background layer frame now derives from root bounds.
6. Rule right edge and rule y-position now derive from bounds.
7. Time and health row placement now uses a derived row gap.
8. README and AGENTS geometry were updated for the resulting Emery layout.

## Validation Reminders

1. Run `pebble build` after each committed slice.
2. For visual changes, install to Emery and toggle:
   - `10005=0` dark mode
   - `10005=1` light mode
   - `10004=0` normal icon mode
   - `10004=1` fallback icon mode
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
