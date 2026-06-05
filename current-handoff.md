# Current Handoff

## Project State

- Workspace: `/Users/manomeht/Code/life-at-a-glance-emery-wf`
- Product: Pebble SDK watchface, "Life at a Glance".
- Date: June 5, 2026.
- Current target platforms in `package.json`: `aplite`, `basalt`,
  `diorite`, `emery`, and `flint`.
- Last known build status: `pebble build` passed after cleanup commits #9
  and #10. Linker RWX warnings are known non-fatal Pebble toolchain noise.
- Working tree at handoff: clean after `38028bd`.

Latest cleanup-series commits:

- `38028bd` `Code clean-up #10: Parse settings tuples strictly`
- `f051b30` `Code clean-up #9: Improve AppMessage diagnostics`
- `1e6d3ca` `Code clean-up #8: Collapse health platform guards`
- `ae2f6e0` `Code clean-up #7: Document OAK weather fallback`
- `43589e3` `Code clean-up #6: Show zero steps`
- `15a53fe` `Code clean-up #5: Send unavailable weather explicitly`
- `749a86e` `Code clean-up #4: Stop persisting temperature`
- `ab7ced1` `Code clean-up #3: Clarify product and module contracts`
- `0e48214` `Code clean-up #2: Move helper bodies out of headers`
- `140bc59` `Code clean-up #1: Use macro for text buffer size`

## Collaboration Directives

Use the strict loop before writing code:

1. Inspect current symbols and call paths.
2. Identify existing vocabulary, ownership, and platform guards.
3. State the intended patch shape before editing.
4. Edit only after the terms and branches line up.
5. Build and review the diff against the stated intent.

Hard lessons from the latest cleanup thread:

- Prevent drift before adding code. Reuse existing sentinels, enum names,
  macros, and product vocabulary instead of creating adjacent names.
- Audit every include of a platform-bound header when changing its API or
  guard shape. This is especially important for `PBL_HEALTH` and
  `health.h`, because unguarded `HealthEventType` references break
  non-health platforms.
- Store source state, not redundant derived render state, unless the derived
  value is expensive or impossible to recompute. Derived text/icon colors
  should come from value, availability, and palette at render/update time.
- Do not add state variables or helper functions just because they are
  convenient. First check whether an existing variable, macro, enum, or
  module-owned contract already expresses the concept.
- Text is the primary glance surface. Icons are secondary support. Missing
  icon controls must not suppress text updates.
- If a required text control for a metric cannot be created, do not create
  or depend on that metric's icon. A text-only metric is acceptable; an
  icon-only metric is not a useful product state.
- Log creation failures once where they happen. Avoid refresh-time log spam
  for missing layers that were already known to be absent.

## Current Runtime Model

Settings and AppMessage keys:

- `TIME_FORMAT`: `0` 24-hour, `1` 12-hour
- `TEMP_UNIT`: `0` Fahrenheit, `1` Celsius
- `TEMPERATURE`: runtime-only Celsius tenths from PebbleKit JS
- `WEATHER_CONDITION`: raw Open-Meteo `weather_code`
- `HR_SAMPLE_MINUTES`: enum-backed sampling interval
- `DISPLAY_MODE`: `0` dark, `1` light

Persisted settings:

- `WatchfaceSettings` owns user preferences only: time format, temperature
  unit, HR sample interval, and display mode.
- Temperature is no longer persisted.
- Weather condition is not persisted.
- There is no offline weather cache and no weather timestamp model.

PebbleKit JS weather behavior:

- `src/pkjs/index.js` fetches `temperature_2m` and `weather_code` from
  Open-Meteo every 30 minutes.
- JS sends raw numeric weather codes to C. Do not convert them to strings.
- If weather cannot be fetched or parsed, JS sends:
  - `TEMPERATURE = WEATHER_TEMP_INVALID`
  - `WEATHER_CONDITION = WEATHER_CONDITION_UNKNOWN`
- If phone geolocation is unavailable, JS falls back to OAK, the product
  home location: `37.85626, -122.21383`.

AppMessage parsing:

- Clay currently sends select values as numeric strings.
- C accepts integer tuples and all-digit string tuples.
- C rejects empty, malformed, signed, partially numeric, and overflowing
  setting strings before range validation.

## Main Boundary Direction

Current reality:

- `main.c` still owns window lifecycle, service subscriptions, top date/time
  layers, temperature text, AppMessage receive flow, and high-level refresh
  order.
- `battery.c` owns battery state, text, icon, color, callback-state updates,
  and rendering.
- `health.c` owns BPM/steps state, text buffers, icons, colors, and health
  service update handling.
- `weather.c` owns weather condition glyph rendering and unavailable glyph
  rendering. Temperature text is still in `main.c`.
- `settings.c` owns persisted settings defaults, validation, load, save, and
  HR sampling interval mapping.

Target direction through initial GitHub release:

- Hold `main.c` to lifecycle and dispatch only.
- Move date/time buffers and formatting into a clock/time-display module.
- Move temperature text, formatting, and weather AppMessage handling into
  the weather module.
- Keep settings tuple parsing/application narrow and explicit.
- Do not move behavior across modules unless the new boundary is cleaner and
  behavior-preserving.

## Visual Contract

Text-first hierarchy:

- Date/time text are core watchface controls.
- Battery percentage text is more important than the battery icon.
- Temperature text is more important than the weather icon.
- BPM and steps text are more important than their icons on `PBL_HEALTH`
  builds.
- Icons are helpful affordances, not required for glanceability.

Must-initialize controls for a useful launched watchface:

- window and root layer
- date text layer
- time text layer
- battery text layer
- temperature text layer
- on `PBL_HEALTH` builds: BPM and steps text layers

Optional controls:

- battery, weather, BPM, and steps icons
- color-specific palette richness beyond black/white fallback
- live weather data
- live health values
- phone/AppMessage availability

Palette fallback:

- `display_get_palette()` should be treated as the palette owner.
- If palette resolution ever fails, the product should degrade to a simple
  black-and-white display rather than blocking data display.

Icon/render-state simplification:

- Prefer source state over derived render globals.
- Battery icon/text color should derive from `BatteryChargeState`.
- BPM text/icon color should derive from BPM value plus availability.
- Steps icon color should derive from steps availability and palette.
- Avoid separate stored icon color globals when render-time calculation is
  clear and cheap.

## Current Layout Snapshot

Rectangular layout is calculated in `layout_calculate()` from display
dimensions and At A Glance product constants in `src/c/ataglance.h`.

Current implementation no longer draws the horizontal rule. Some README and
AGENTS layout text still needs a full drift cleanup after code behavior
stabilizes.

For Emery (`200x228`), the current rectangular model is still organized as:

- date row
- hero time row
- health metrics row
- bottom weather/battery row

Text alignment is intentionally left-aligned across all columns.

## Remaining Cleanup Tracker

1. Battery/health partial refresh policy.
   - Apply the text-first contract.
   - If text creation fails for a metric, do not create/depend on that
     metric's icon.
   - If icon creation fails, continue updating text.
   - Log creation failures once.

2. Main ownership reduction.
   - Move temperature display and weather tuple application out of `main.c`.
   - Move date/time display out of `main.c`.
   - Keep main to lifecycle, subscriptions, and dispatch.

3. Docs drift cleanup.
   - README and AGENTS still contain stale layout/rule/glyph details.
   - Reconcile against `src/c/main.c`, `src/c/ataglance.h`,
     `src/modules/*.h`, `src/pkjs/config.json`, and `package.json`.

4. Publish-prep audit.
   - build check
   - secret/private-data audit
   - `.gitignore`
   - SDK/platform metadata
   - README review
   - screenshots only if useful and approved
   - clear initial private GitHub commit

## Validation Checklist For Next Agent

Before any commit:

1. Run `git status --short`.
2. Inspect diffs, including docs and generated files.
3. Run `pebble build` for code changes.
4. Confirm no untracked screenshots, build artifacts, scratch buffers, or
   private data are staged.
5. Use a detailed commit body for non-trivial changes.
