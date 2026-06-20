# Retrospective: Rectangular Watchface Work

Generated on June 3, 2026.

This document inventories the work completed so far for the rectangular
version of "Life at a Glance". It is factual by design. Reflection,
assessment, and opinions are intentionally left for a later phase.

## Inputs Reviewed

Current Markdown files reviewed:

- `AGENTS.md`
- `README.md`
- `DESIGN.md`
- `initial-creation.md`
- `NEXT_SESSION_REFINEMENTS.md`
- `current-handoff.md`
- `wf-goal.md`
- `wf-punchlist.md`

Repository history reviewed:

- Full reverse Git history from `e228390` through `57d619d`.
- Commit subjects, commit bodies, and changed-file lists.
- Current tracked source, module, resource, build, package, and docs files.

Memory inputs reviewed:

- `MEMORY.md` task group for this repo.
- Rollout summary for the early Pebble agent/code-review/handoff work.
- Rollout summary for the palette/layout/review-gated refactor cycle.
- The memory summary available in this session.

Live implementation files inspected for current inventory:

- `package.json`
- `wscript`
- `src/c/ataglance.h`
- `src/c/main.c`
- `src/modules/layout.[ch]`
- `src/modules/display.[ch]`
- `src/modules/settings.[ch]`
- `src/modules/weather.[ch]`
- `src/modules/health.[ch]`
- `src/modules/battery.[ch]`
- `src/modules/helper.h`
- `src/pkjs/config.json`
- `src/pkjs/index.js`

## Current Rectangular Inventory

The current product is a Pebble watchface named "Life at a Glance" with a
Swiss-Rail-inspired rectangular layout.

Current rectangular target platforms in `package.json`:

- `aplite`
- `basalt`
- `diorite`
- `emery`
- `flint`

Current watchface capabilities:

- `configurable`
- `location`
- `health`

Current AppMessage keys:

- `TIME_FORMAT`
- `TEMP_UNIT`
- `TEMPERATURE`
- `WEATHER_CONDITION`
- `HR_SAMPLE_MINUTES`
- `DISPLAY_MODE`

Current resource inventory:

- `resources/images/icon.png` as `MENU_ICON`.
- The earlier BPM PDC resource path has been removed.
- Health, weather, and battery icons are procedural in C.

Current native source shape:

- `src/c/main.c`
  - window lifecycle
  - service subscriptions
  - AppMessage inbox handling
  - date/time layers
  - temperature text layer
  - background and rule drawing
  - high-level display refresh ordering
- `src/c/ataglance.h`
  - shared text-buffer ids and buffer length
- `src/modules/layout.c`
  - rectangular layout calculation through `layout_calculate()`
  - fills caller-owned `WatchfaceLayout` storage
- `src/modules/display.c`
  - visual palette selection
  - unavailable text token
  - shared text-layer update helper
- `src/modules/settings.c`
  - persisted settings defaults
  - settings validation
  - settings load/save
  - heart-rate sampling interval mapping
- `src/modules/helper.h`
  - shared `static inline` icon-scaling helpers
- `src/modules/weather.c`
  - Open-Meteo weather-code mapping
  - procedural weather glyph drawing
  - weather icon layer ownership
- `src/modules/health.c`
  - BPM and steps layers
  - procedural heart and step icons
  - health text buffers
  - health color handling
  - health event refresh handling
- `src/modules/battery.c`
  - battery state
  - procedural horizontal battery icon
  - battery text refresh
  - battery callback state updates

Current JavaScript and configuration shape:

- `src/pkjs/config.json` defines the Rebble Clay settings page.
- `src/pkjs/index.js` initializes Clay from the JSON config.
- The phone companion fetches Open-Meteo temperature and weather codes.
- Temperature is sent as Celsius tenths.
- Weather condition is sent as a raw numeric Open-Meteo `weather_code`.

Current rectangular layout model:

- Layout is calculated from display dimensions in `layout_calculate()`.
- Vertical spacing derives from `PBL_DISPLAY_HEIGHT / 28` for rectangular
  builds.
- Current horizontal rectangular content margin is 2 px.
- Emery baseline is `200x228`.
- Emery spacing is currently 8 px.
- The layout uses a horizontal rule, not the earlier vertical rail.
- Health row and bottom row are separate layout concepts.
- The bottom row contains weather/temperature on the left and battery on
  the right.

Current Emery frame inventory from `src/modules/layout.c`:

- Date: `GRect(2, 8, 196, 20)`
- Time: `GRect(2, 58, 196, 48)`
- Rule: from `(2, 114)` to `(198, 114)`
- BPM icon: `GRect(2, 118, 28, 28)`
- BPM text: `GRect(32, 122, 28, 20)`
- Steps icon: `GRect(128, 118, 28, 28)`
- Steps text: `GRect(158, 122, 40, 20)`
- Weather icon: `GRect(2, 196, 28, 28)`
- Temperature text: `GRect(32, 200, 40, 20)`
- Battery icon: `GRect(126, 196, 28, 28)`
- Battery text: `GRect(156, 200, 34, 20)`

Current documentation drift observed during this inventory:

- `AGENTS.md` and `current-handoff.md` still list the older 8 px Emery
  horizontal content margin and corresponding `x=8` frames.
- `src/modules/layout.c` currently uses a 2 px horizontal content margin
  after `a66af3f`.

Current visual semantics:

- Unavailable text token is `---`.
- Dark mode has black background.
- Light mode has white background.
- Color displays use distinct semantic colors for date, time, steps, BPM
  zones, battery zones, and unavailable values.
- Black-and-white displays fall back to primary text colors for available
  BPM and battery values.
- Unavailable BPM, steps, and weather states use procedural visual marks.
- Weather unknown state renders separately from unavailable weather.

Current settings defaults:

- Time format: 24-hour.
- Temperature unit: Fahrenheit.
- Heart-rate sampling: 10 minutes.
- Display mode: dark.
- Temperature starts at invalid/unavailable until phone data arrives.

Current build shape:

- `wscript` builds `src/c/**/*.c` and `src/modules/**/*.c`.
- `wscript` bundles JS and JSON from `src/pkjs`.
- `wscript` still includes a `node_modules/pebble-clay/**/*.js` bundle glob.
- `package.json` keeps `sdkVersion` at `"3"` for Pebble tooling
  compatibility while the project targets SDK 4+ APIs where available.

## Markdown Inventory

`AGENTS.md` currently documents:

- inherited root and Pebble template guidance
- repo facts and module boundaries
- current target platforms
- current AppMessage keys
- rectangular layout baseline
- visual semantics
- round-display planning constraints
- validation checklist
- publishing notes
- ephemeral-file deletion confirmation rule

`README.md` currently documents:

- product description
- rectangular platform support
- feature list
- layout model and hierarchy
- fonts
- procedural icon behavior
- color and monochrome modes
- Clay configuration
- Open-Meteo weather behavior
- build and emulator commands
- publish README target

`DESIGN.md` currently preserves the early Swiss-Rail B2 design reference:

- date row
- hero time
- horizontal rule
- health complications row
- bottom-left temperature
- bottom-right battery
- original left-rail design notes
- original configurable settings plan
- intended data sources

`initial-creation.md` records the early design and setup transcript:

- initial project creation request
- SDK/dependency verification
- target correction from `basalt` to `emery`
- UUID explanation
- attempted "Ahead of Time" source download
- visual concept exploration
- Swiss-Rail B2 selection
- final layout edits
- first implementation summary
- initial Clay/time/temp/weather/health/battery implementation
- later addition of configurable heart-rate sampling

`NEXT_SESSION_REFINEMENTS.md` records earlier next-session planning:

- docs drift audit
- display refresh path audit
- form-factor layout extraction
- root-bounds background layer cleanup
- stale rail constant cleanup
- settings tolerance follow-up
- state-splitting follow-up
- emulator/manual message validation ideas using older numeric keys

`current-handoff.md` records the current handoff state:

- rectangular targets
- current settings and AppMessage model
- current layout snapshot
- palette and unavailable-state model
- weather module state
- current module boundaries
- pending round, main-module, state, settings, and weather-fallback
  planning topics
- validation checklist for the next agent

`wf-goal.md` records the active work goals:

- finish/refine weather-condition icons
- plan round watchface support
- plan remaining `main.c` module boundaries
- decide on screenshots and obsolete resources
- plan state, settings tolerance, and weather fallback changes

`wf-punchlist.md` records:

- round-display planning as active work
- remaining main-module modularization candidates
- rectangular validation reminders
- cleanup and refactoring candidates
- completed rectangular, palette, module, weather, and icon work
- validation reminders for builds, AppMessage display toggles, and platform
  checks

## Memory Inventory

The current memory set captures work in two main historical groups.

Early Pebble agent/code-review/handoff memory:

- Created and refined repo-level `AGENTS.md`.
- Preserved the SDK 4+ direction while keeping `sdkVersion: "3"`.
- Added GitHub publishing and no-secret expectations.
- Added inspect-first, review-gated, one-diff-at-a-time working style.
- Reviewed runtime/lifecycle code.
- Landed lifecycle, health guard, and settings validation hardening.
- Standardized unavailable placeholders.
- Kept `src/pkjs/config.json` as JSON.
- Ignored `.lock-waf_darwin_build`.
- Added handoff/resume guidance.

Palette/layout/review-gated memory:

- Audited docs drift against live code and package metadata.
- Centralized display refresh and palette-driven text updates.
- Began and continued rectangular layout derivation.
- Derived row spacing from `PBL_DISPLAY_HEIGHT / 28`.
- Investigated Pebble CLI behavior around `pebble sdk`.
- Captured monochrome palette direction.
- Consolidated the punchlist.
- Preserved the review-gated operating rule:
  inspect, identify, audit, confirm, execute, validate.

Ad-hoc memory preferences reflected in this repo:

- Keep substantive engineering review-gated.
- Split runtime, layout, palette, cleanup, and docs work into coherent
  slices.
- Treat pause/stop requests as hard stops.
- Use detailed commit bodies for non-trivial Pebble changes.
- Avoid large struct pass-by-value in embedded C paths.
- Keep 72-character source-line guidance for programming files.

Memory noted items that later changed:

- Earlier memory described inverse unavailable text backgrounds after
  `0f194da`; later commit `7122064` removed unavailable text/background
  fields from the shared palette.
- Earlier memory described `ICON_FALLBACK_MODE`; later commit `57d619d`
  removed the obsolete fallback AppMessage key from `package.json`.
- Earlier memory described `CONTENT_X`; later layout work removed that
  old constant path and then tightened the rectangular content margin.

## Commit Timeline

### May 30, 2026

`e228390` - Add agent collaboration guide

- Added the first `AGENTS.md`.

`e4412b4` - Establishing norms, working style, engagement model,
expectations, and flow before starting work.

- Added initial repo scaffold and project assets.
- Added `.cursorrules`, `.gitignore`, `.zed/tasks.json`.
- Added `DESIGN.md`, `README.md`, `initial-creation.md`, `LICENSE`,
  `Makefile`, `package.json`, `package-lock.json`, `wscript`.
- Added initial resources including BPM PDC/image experiments.
- Added initial native C and PebbleKit JS sources.

### June 1, 2026

`aac5fb5` - Harden lifecycle, health guards, and settings validation

- Hardened C lifecycle paths.
- Added health subscription guards.
- Added settings validation.

`59e9c35` - Standardize unavailable placeholders in display formatting

- Standardized unavailable display text.

`ab2098d` - Update config UI defaults and ignore waf lock file

- Updated Clay config defaults.
- Added Waf lock-file ignore behavior.
- Removed the tracked Waf lock file.

`891734f` - docs: add watchface-specific README and AGENTS implementation
details

- Updated `README.md`.
- Updated `AGENTS.md`.

`92a0492` - feat(layout): retune emery typography and row geometry

- Retuned Emery typography and layer geometry in C.

`bba9828` - fix(heart): add non-allocating fallback render path and clear
TODOs

- Added non-allocating heart fallback rendering.
- Updated design documentation.

`e5a13f0` - refactor(bpm): normalize icon naming from heart to bpm

- Renamed heart icon terminology toward BPM icon terminology.

`46454f7` - docs(agents): enforce 72-character source line width

- Added source-line width guidance to `AGENTS.md`.

`90e4876` - fix(lifecycle): guard window layer pointers in load and unload

- Added window layer pointer guards.

`aef7fb6` - Refactor window-load setup into inline layer helpers

- Extracted window-load setup helpers in `main.c`.

`69eecf3` - Add compile-time BPM icon fallback mode

- Added compile-time BPM fallback mode.

`9bf685e` - docs: add current project handoff

- Added `current-handoff.md`.

`8383f96` - Add runtime fallback icon mode

- Added runtime fallback icon mode.
- Updated `.gitignore`, `package.json`, `ataglance.h`, `main.c`, and Clay
  config.
- Removed several exploratory BPM/heart resources.
- Renamed one BPM PDC resource into the images path.

`51f260f` - Refactor config values into enum ranges

- Refactored config values into enum-backed ranges.
- Updated C and Clay config.

`9b48641` - Clean up fallback mode visual handling

- Cleaned fallback mode visual behavior.

`271e2e5` - Add display mode configuration setting

- Added display mode configuration.
- Updated package message keys, settings, C, and Clay config.

`6d894b7` - Wire visual cue to display mode

- Connected visual cue behavior to display mode.

`2d9e015` - docs: record commit message preference

- Recorded preference for bulleted bodies on non-trivial commits.

`6d0fa4e` - Make icon fallback mode global

- Renamed fallback config from BPM-specific to global icon fallback.
- Preserved hidden fallback key slot at that point.
- Updated README and AGENTS snapshots.

`c63ce78` - Define display color palettes

- Added `VisualPalette`.
- Documented dark and light mode palette values.
- Kept rendering behavior unchanged for later wiring.

`57de6f9` - Wire rendering to display palettes

- Routed text, rule, and steps icon colors through the active palette.
- Repainted layers after display mode changes.
- Fixed day tick handling so date updates were not skipped at midnight.

`f6cb94b` - Clarify visual palette application flow

- Made palette selection explicit after settings load and display mode
  changes.
- Renamed the repaint path to `apply_visual_palette_to_watchface`.

`9fa6850` - Rename watchface display refresh path

- Replaced `apply_visual_palette_to_watchface` with
  `refresh_watchface_display`.
- Folded the old update sequence into the display refresh path.
- Added missing-window guard before repaint.
- Marked palette-driven steps icon dirty during refresh.
- Commit body records `pebble build` verification.

### June 2, 2026

`6c0df49` - docs: add next-session handoff and runbook

- Refreshed `current-handoff.md`.
- Added `NEXT_SESSION_REFINEMENTS.md`.
- Documented inspect, identify, audit, confirm, execute, validate in
  `AGENTS.md`.

`a02e6d1` - docs: refresh implementation snapshot

- Refreshed `AGENTS.md` and `README.md`.

`c0f8125` - Extract watchface layout frames

- Added `WatchfaceLayout` in `main.c`.
- Grouped current layer frames.
- Passed layout frames to initializer helpers.
- Preserved existing Emery geometry at that stage.

`dc248c4` - Rename steps display updater

- Renamed `update_step_count()` to `update_steps()`.
- Updated prototype, refresh path, and health event call site.

`16db6f4` - Use root bounds for background layer

- Derived background layer frame from root bounds in `WatchfaceLayout`.

`6c870ec` - Display Refresh using Palette to support Light/Dark Modes

- Moved palette selection into `refresh_watchface_display()`.
- Routed display-mode AppMessage changes through display refresh.
- Removed initial text color assignment from text-layer creation.
- Kept `update_*()` functions responsible for text colors.

`81d8323` - Derive rule layout from bounds

- Added derived `rule_y` and `rule_right`.
- Added row-specific content width fields.
- Derived rectangular content widths.
- Derived center rule from bounds height.
- Derived row spacing from bounds height divided by 28.
- Updated README and AGENTS with resulting Emery geometry.

`4cf634d` - Add watchface punchlist

- Replaced `cursor_refactoring_cleanup.md` with `wf-punchlist.md`.
- Captured active platform, monochrome, rectangular, and round planning
  items.
- Captured completed work and validation reminders.

`0f194da` - Centralize text layer palette backgrounds

- Added available and unavailable text background colors to the palette.
- Routed text layer updates through a shared helper.
- Used inverse unavailable backgrounds at that point.
- Made `format_temp()` return availability.
- Cleaned some PebbleKit JS logging/timing behavior.

### June 3, 2026

`f5fc4b5` - Continue rectangular layout derivation

- Replaced fixed offsets with display-height-derived spacing for
  rectangular layouts.
- Refined Emery rows, icon frames, and text frames.
- Redrew fallback and battery icons from layer bounds.
- Made the battery icon horizontal.
- Filled `WatchfaceLayout` through caller-owned storage.
- Updated README, AGENTS, and planning notes.

`6846d99` - Remove stale layout constants

- Removed stale rail constants.
- Moved rectangular spacing math into layout calculation.
- Updated punchlist after rail cleanup.

`c8de014` - Expanding compatibility to include Monochrome Displays (except
Flint)

- Added `aplite`, `basalt`, and `diorite` targets alongside `emery`.
- Added explicit color and black-and-white palettes.
- Used Pebble compile-time color cues.
- Collapsed BPM and battery semantic colors to readable primary text on
  black-and-white platforms.
- Guarded health-only functions and heart-rate updates with `PBL_HEALTH`.
- Did not yet include Flint because the then-current Clay dependency did
  not support it.

`8d55de4` - Continue layout refactoring to extend device compatibility

- Used compact text fonts and reduced metric text frames.
- Collapsed dark/light palettes with `PBL_IF_COLOR_ELSE`.
- Drew unavailable BPM and steps icon backgrounds before glyphs.
- Refreshed health icon layers when metric state changed.
- Forced the BPM fallback icon on black-and-white platforms at that time.
- Thinned the horizontal rule to 1 px.
- Fixed horizontal battery drawing inset behavior.

`6e2b9c0` - Tighten embedded geometry helper boundaries

- Moved shared watchface data shapes into `ataglance.h`.
- Replaced header macros for temp and icon constants with file-local typed
  constants.
- Refactored icon geometry helpers to use pointers and caller-owned
  outputs.
- Passed layout frames by pointer to initialization helpers.

`c34396c` - Add Flint support through Rebble Clay

- Replaced `pebble-clay` with `@rebble/clay`.
- Updated PebbleKit JS import.
- Added `flint` to the rectangular target platform list.
- Refreshed README for rectangular device UI, Rebble Clay, and publish
  structure.

`043cca4` - Add weather condition icon support

- Fetched Open-Meteo `weather_code` alongside temperature.
- Added `WEATHER_CONDITION` AppMessage handling.
- Added a weather icon frame in rectangular layout.
- Moved weather icon state, drawing, palette updates, and layer ownership
  into `src/modules/weather.c`.
- Updated `wscript` to compile module sources.
- Replaced the BPM PDC resource used for menu/icon path at that point.

`db7de61` - Expand weather condition glyph rendering

- Mapped raw Open-Meteo weather codes to private `WeatherIconKind`
  buckets.
- Added file-local `static inline` glyph renderers for clear, cloudy, fog,
  drizzle, rain, frozen rain, snow, showers, snow showers, thunderstorm,
  and unknown states.
- Folded snow grains into snow.
- Rendered unknown conditions as a question mark.

`f82682c` - Tune secondary metric typography

- Split secondary and battery value fonts.
- Rendered BPM, steps, and temperature with `FONT_KEY_GOTHIC_18`.
- Kept battery bold.
- Increased bottom-left text width for three-digit Fahrenheit values.
- Updated BPM, steps, temperature, and battery initializers.

`fee1079` - Refresh project handoff context

- Updated `AGENTS.md` with deliberate planning, review-first coding,
  small-slice commit, and embedded struct-passing directives.
- Replaced `current-handoff.md` with current platform, layout, weather,
  typography, and pending planning context.
- Recorded validation expectations and emulator/screenshot caveats.

`328b7fa` - Update watchface punchlist

- Moved completed rectangular, monochrome, weather, and typography work into
  completed items.
- Kept round-display planning, main modularization, state hygiene, settings
  tolerance, and weather fallback visible.

`4ec1427` - Record current watchface goal

- Added `wf-goal.md`.
- Recorded active weather, round-layout, modularization, cleanup, state,
  settings, and weather-fallback planning directives.

`f427809` - Refine weather icon visual vocabulary

- Added shared icon-scaling helpers.
- Refined clear, cloud, rain, drizzle, frozen rain, snow, fog, and
  thunderstorm glyphs.
- Kept weather glyph rendering procedural and grid-derived.

`5ac72c7` - Modularize watchface support modules

- Extracted layout calculation into `src/modules/layout.c`.
- Moved `WatchfaceLayout` into the layout module.
- Extracted palette selection and text-layer display helpers into
  `src/modules/display.c`.
- Extracted persisted settings and HR sampling mapping into
  `src/modules/settings.c`.
- Extracted BPM, steps, and related procedural icon rendering into
  `src/modules/health.c`.
- Extracted battery rendering and state into `src/modules/battery.c`.
- Removed the unused BPM PDC resource.
- Preserved the legacy AppMessage slot at that moment so `DISPLAY_MODE`
  kept its wire id.

`18e1afe` - Gate health module on health platforms

- Included and called health module APIs only when `PBL_HEALTH` is
  available.
- Compiled health layer state and render callbacks only on health-capable
  targets.
- Left non-health builds without partial BPM and steps layer state.

`9148748` - Add unavailable weather glyph

- Drew a slashed cloud glyph when temperature or weather data is
  unavailable.
- Kept unknown-condition question mark for unsupported weather codes.
- Preserved existing unavailable palette behavior at that point.

`a66af3f` - Tighten rectangular content margin

- Changed rectangular horizontal content margin to 2 px.
- Kept vertical row spacing derived from display height.
- Recovered bottom-row space on 144 px black-and-white targets.

`7122064` - Simplify unavailable visual state

- Removed unavailable text/background fields from shared palette.
- Kept text layers clear.
- Rendered unavailable text in mode-appropriate colors.
- Added data-gap slashes to unavailable BPM and steps glyphs.
- Kept unavailable weather glyph on the active mode background.
- Cleared the battery glyph background before redraw.

`58de501` - docs: update watchface agent guidance

- Pointed repo guidance at inherited universal and Pebble layers.
- Captured current module, layout, visual, round-display, and validation
  rules.
- Added explicit ephemeral-file cleanup confirmation requirements.

`57d619d` - Remove obsolete icon fallback AppMessage key

- Removed `ICON_FALLBACK_MODE` from `package.json` message keys.
- Allowed `DISPLAY_MODE` to use the compact generated key sequence.

## Activity Timeline

### Project Setup And Tooling

1. A Pebble C watchface project was scaffolded.
2. Pebble SDK and tooling were checked.
3. The build system was corrected to match Pebble multi-platform build
   expectations.
4. The target was corrected from `basalt` to `emery` for Pebble Time 2.
5. The project UUID was explained and retained as app identity metadata.

### Visual Design

1. Several 200x228 layout concepts were explored.
2. The Swiss-Rail/typographic direction was selected.
3. The design converged on the B2 layout.
4. Final design choices were recorded:
   - short date
   - large time
   - horizontal rule
   - BPM left and steps right on the health row
   - full step count
   - temperature bottom-left
   - battery percentage bottom-right
   - configurable F/C and 12h/24h
5. `DESIGN.md` and `design-preview.html` captured the early design target.

### Initial Watchface Implementation

1. Native C watchface UI was implemented in `main.c`.
2. Time/date display was added.
3. Battery state display was added.
4. Pebble Health BPM and steps were added.
5. Clay configuration was added.
6. Time-format and temperature-unit settings were persisted.
7. PebbleKit JS weather fetch was added through Open-Meteo.
8. The phone sent temperature to the watch through AppMessage.
9. Configurable HR sampling was added.

### Runtime Hardening

1. Window creation and teardown were hardened.
2. Layer/resource creation paths were guarded.
3. Health-service subscription and HR sampling paths were guarded.
4. Settings validation was added.
5. Unavailable display placeholders were standardized.
6. Generated Waf lock-file churn was removed from version control.

### Documentation And Working-Style Capture

1. `AGENTS.md` was created and iteratively refined.
2. Repo-specific collaboration rules were captured.
3. The inspect, identify, audit, confirm, execute, validate cycle was
   documented.
4. Commit-message preferences were documented.
5. `current-handoff.md` was added.
6. `NEXT_SESSION_REFINEMENTS.md` was added.
7. `wf-punchlist.md` replaced the older Cursor cleanup export.
8. `wf-goal.md` captured the active goal state.

### Palette And Display Refresh

1. Display mode was added as a configuration setting.
2. Dark and light palettes were defined.
3. Rendering was routed through active palette state.
4. `refresh_watchface_display()` became the high-level refresh path.
5. Text-layer update behavior was centralized.
6. Display-mode AppMessage updates were routed through refresh.
7. Date/time/metric updates retained explicit ownership of their text
   updates.

### Rectangular Layout Derivation

1. `WatchfaceLayout` was introduced to group layer frames.
2. Background frame calculation moved to root bounds.
3. Rule position and width were derived from bounds.
4. Row-specific content widths were introduced.
5. Row spacing started deriving from display height.
6. Rectangular spacing was standardized around `PBL_DISPLAY_HEIGHT / 28`.
7. Layer frames were tuned for Emery.
8. Metric text frames were tightened for smaller rectangular displays.
9. The horizontal rule design replaced stale vertical rail constants.
10. Layout calculation was changed to fill caller-owned storage.
11. Layout was later extracted into `src/modules/layout.c`.
12. The rectangular content margin was later tightened to 2 px.

### Monochrome And Multi-Rectangular Platform Support

1. Rectangular targets expanded from Emery to include `aplite`, `basalt`,
   and `diorite`.
2. Color and black-and-white palette behavior was added.
3. `PBL_IF_COLOR_ELSE()` was used to keep color/monochrome choices in one
   palette path.
4. Health functionality was guarded for platforms without health support.
5. `@rebble/clay` replaced `pebble-clay`.
6. `flint` was added to the rectangular target list after the Clay change.

### Procedural Icon Evolution

1. BPM fallback icon rendering was added without heap allocation.
2. Steps used a procedural glyph.
3. Battery drawing became a horizontal procedural icon.
4. Weather icon support was added.
5. Open-Meteo weather codes were mapped to private C glyph buckets.
6. Weather glyphs were implemented as lightweight file-local static inline
   renderers.
7. Shared icon-scaling helpers moved into `src/modules/helper.h`.
8. The BPM PDC resource was removed.
9. The hidden icon fallback setting was removed.
10. Unavailable BPM, steps, and weather states became procedural marks.

### Module Extraction

1. Weather became the first module-style extraction.
2. Shared helper code moved into `src/modules/helper.h`.
3. Layout moved into `src/modules/layout.c`.
4. Display palette/text helpers moved into `src/modules/display.c`.
5. Settings persistence and HR sampling mapping moved into
   `src/modules/settings.c`.
6. BPM and steps moved into `src/modules/health.c`.
7. Battery state and rendering moved into `src/modules/battery.c`.
8. Health module compilation was gated on `PBL_HEALTH`.
9. `main.c` remained owner of lifecycle, subscriptions, top date/time,
   temperature text, AppMessage flow, and high-level refresh ordering.

### Current Forward Work Captured But Not Completed

1. Round-display layout planning.
2. Remaining `main.c` modularization candidates:
   - messaging/weather receive flow
   - temperature formatting/display
   - top-row time/date
3. State hygiene planning.
4. Persisted settings tolerance/versioning.
5. Phone weather fallback configurability.
6. Visual QA screenshot set for color, monochrome, dark, light, unavailable
   health, unavailable weather, long date, 99999 steps, and 100% battery.
7. Initial private GitHub publishing preparation.

## Large Work Chunks In Order

1. Scaffold and SDK readiness.
2. Swiss-Rail B2 design exploration and lock.
3. First full Emery implementation with time, date, health, battery,
   temperature, Clay settings, persistence, and Open-Meteo.
4. Runtime/lifecycle hardening and placeholder normalization.
5. Repo guidance, handoff, and review-gated working-style documentation.
6. Display mode, palette definition, and display-refresh consolidation.
7. Early layout grouping through `WatchfaceLayout`.
8. Bounds-derived rectangular layout and row-spacing derivation.
9. Punchlist consolidation for active and future work.
10. Monochrome support and rectangular platform expansion.
11. Embedded helper-boundary cleanup to reduce struct pass-by-value.
12. Rebble Clay migration and Flint support.
13. Weather condition AppMessage support and procedural weather glyphs.
14. Secondary metric typography tuning.
15. Weather glyph visual refinement and shared icon-scaling helpers.
16. Main support-module extraction:
    - layout
    - display
    - settings
    - health
    - battery
17. Health platform gating.
18. Unavailable weather and unavailable visual-state simplification.
19. Rectangular margin tightening.
20. Agent-guidance hierarchy cleanup.
21. Obsolete fallback AppMessage key removal.
