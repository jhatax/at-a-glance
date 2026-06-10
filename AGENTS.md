# Agent Guidelines: Life at a Glance Watchface

**Inherits from:**

- [Universal development principles](../AGENTS.md)
- [Pebble platform guidelines](../agent-templates/AGENTS-PEBBLE.md)

Before applying this file, read the inherited files above in order. There is
no agent-wide `--force` flag that guarantees hierarchy loading; this directive
is the project rule. If an agent cannot read an inherited file, it must say so
before making changes.

This repo is for the Pebble SDK 4+ watchface "Life at a Glance", a
Swiss-Rail-inspired, glanceable rectangular watchface. Keep this file focused
on project facts, product decisions, visual baseline, and watchface-specific
validation. Put platform-wide Pebble rules in `../agent-templates/AGENTS-PEBBLE.md`
and universal engagement rules in `../AGENTS.md`.

---

## Project Facts

- Native C: `src/c`
- Modules: `src/modules`
- PebbleKit JS and Clay config: `src/pkjs`
- Metadata, target platforms, resources, capabilities, and message keys:
  `package.json`
- Build entry: `wscript`
- Current design reference: `DESIGN.md`
- Current rectangular targets: `aplite`, `basalt`, `diorite`, `emery`,
  and `flint`
- Round platforms such as `chalk` and `gabbro` require a separate layout
  plan before support is added.

---

## Current Watchface Snapshot

Keep this section aligned with `src/c/main.c`, `src/c/ataglance.h`,
`src/modules/*.h`, `src/pkjs/config.json`, and `package.json`.

Pebble manifest capabilities currently required:

- `configurable` for the Clay settings page
- `location` for weather geolocation
- `health` for BPM and steps

AppMessage keys currently required:

- `TIME_FORMAT`
- `TEMP_UNIT`
- `TEMPERATURE`
- `WEATHER_CONDITION`
- `HR_SAMPLE_MINUTES`
- `DISPLAY_MODE`

Persisted settings defaults:

- time format: `24h`
- temperature unit: `°F`
- HR sampling: `10` minutes
- display mode: dark

Clay configuration UI:

```text
At A Glance: Configuration
  - Time format: 24-hour, 12-hour
  - Temperature unit: Fahrenheit, Celsius
  - Display mode: Dark mode, Light mode
  - HR Sampling Frequency: 10, 15, 30, 60, 120 minutes
  - Submit: Save Settings
```

---

## Module Boundaries

Use these boundaries when reviewing or extending the code:

- `src/c/main.c`: window lifecycle, service subscriptions, AppMessage
  receive flow, settings ownership, and watchface dispatch.
- `src/modules/watchface.c`: active watchface coordination, root layer
  discovery, `WatchfaceSurface` ownership, module creation/destruction
  order, `s_strata_created_mask` ownership, full-display refresh
  coordination, and event routing.
- `src/modules/watchface_components.h`: shared display component model:
  color palette, font/color roles, surface, strata, and substrata types.
- `src/modules/layout.c`: public layout facade over calculated
  `WatchfaceSurface` data and color-role lookup.
- `src/modules/layout_stylist.c`: private style helper for palette
  selection, compact/full classification, and font-role resolution.
- `src/modules/layout_rect.c`: private rectangular architect that assigns
  bounds-derived frames for strata and substrata.
- `src/modules/substratum_renderer.c`: shared Pebble rendering helper for
  calculated substrata: text-layer creation/update and icon-coordinate
  scaling.
- `src/modules/settings.c`: persisted settings defaults, validation, load,
  save, and HR sampling interval mapping.
- `src/modules/helper.c`: shared arithmetic, color, and parsing helpers.
- `src/modules/date.c`: date text layer, date buffer, uppercase date
  formatting, and date refresh.
- `src/modules/time.c`: time text layer, time buffer,
  time-format rendering, and time refresh.
- `src/modules/climate.c`: raw Open-Meteo weather-code state,
  temperature formatting, temperature text layer, weather availability,
  and weather icon layer ownership.
- `src/modules/climate_glyphs.c`: raw Open-Meteo weather-code mapping and
  procedural weather glyph rendering from explicit condition, frame, and
  palette inputs.
- `src/modules/bpm.c`: BPM text/icon layers, procedural BPM icon, BPM
  buffer, colors, and heart-rate update handling.
- `src/modules/steps.c`: steps text/icon layers, procedural steps icon,
  steps buffer, colors, and movement update handling.
- `src/modules/battery.c`: battery icon/text layers, battery state, battery
  colors, procedural battery drawing, and battery callback state updates.

Do not move behavior across these boundaries unless the new boundary is
cleaner and behavior-preserving.

---

## Engineering Directives

Use the stricter loop before writing code:

1. Inspect current symbols and call paths.
2. Identify existing vocabulary, ownership, platform guards, and state.
3. State the intended patch shape before editing.
4. Edit only after terms and branches line up.
5. Build and review the diff against the stated intent.

When an audit or patch reveals a broken pattern, search for sibling
occurrences before declaring the slice complete. Fix the same pattern across
the relevant modules in one coherent patch when the ownership boundary and
risk profile match. Do not patch one instance and leave the rest as a future
cleanup unless there is a concrete reason to split the work.

Code style:

- Keep programming source lines at or below 80 characters where practical.

Review and pushback:

- Push back when a change deviates from established platform lifecycle,
  module ownership, product semantics, or repository hygiene. Name the
  deviation plainly and explain the safer local pattern.

Prevent drift:

- Reuse existing sentinels, enum names, macros, message keys, and product
  vocabulary. Do not introduce adjacent names for an existing concept.
- Keep naming aligned with the domain model. Watchface creation bookkeeping
  is strata-oriented, so use strata/stratum vocabulary rather than layer
  vocabulary when tracking created watchface sections.
- When changing a platform-bound header or API, audit every include and every
  caller for required platform guards. In particular, `bpm.h`, `steps.h`,
  and any `Health*` symbols must remain behind `PBL_HEALTH` guardrails.
- Audit C, PebbleKit JS, Clay config, `package.json`, README, AGENTS, and
  manual AppMessage assumptions together when message shape changes.
- Prefer existing module-owned contracts over new variables or helper
  functions. Add a new variable only when no existing source of truth already
  expresses the state.

State and rendering:

- Store source state, not redundant derived render state, unless the derived
  value is expensive or impossible to recompute.
- Derive text/icon colors from value, availability, and palette at update or
  render time where practical.
- Avoid storing separate icon color globals when the color can be cheaply and
  clearly derived from module-owned source state.
- If palette resolution ever fails, data display should degrade to a simple
  black-and-white presentation instead of blocking the watchface.

Text-first controls:

- Text is the primary glance surface. Icons are secondary visual support.
- If a metric's text layer cannot be created, do not create or depend on that
  metric's icon. A text-only metric is acceptable; an icon-only metric is not.
- If an icon layer cannot be created, continue updating the corresponding text.
- Log layer creation failures once where creation happens. Avoid repeated
  refresh-time log spam for known-missing layers.
- Module `create()` success means the required product surface was created:
  the background layer for background and the text layer for text-bearing
  strata. Optional icon-layer creation does not gate success.
- Assign a module's retained `s_surface` pointer only after the required layer
  has been created successfully. Failure paths must not leave stale surface
  pointers, custom fonts, or partially-owned state behind.
- If a module loads a custom font or other resource during `create()`, the
  same module must release it both on later create failure and in `destroy()`.
  Audit sibling modules for the same lifecycle pattern before finishing.

Module ownership:

- `watchface` is the active watchface coordinator. Do not force
  `main.c` to shuttle window, settings, palette, or layout state through every
  module call when watchface can own the runtime boundary cleanly.
- Feature modules own their Pebble layers, buffers, source state, update
  procs, refresh logic, and destroy paths. Shared render helpers may create or
  update a layer from a calculated substratum, but ownership stays with the
  feature module that called them.
- `layout.h` is the public layout API. Feature modules should include
  `watchface_components.h` for surface/type contracts and
  `substratum_renderer.h` for text-layer setup/update or icon scaling. Include
  `layout.h` only when calling true layout APIs.
- Keep climate state and procedural weather glyph drawing separate when the
  drawing logic becomes too large to audit inside `climate.c`. The climate
  module should pass explicit render inputs such as condition, frame, and
  palette into glyph rendering rather than exposing climate-owned globals.
- Optional modules should use product semantics for creation success. For
  BPM and steps, a created text layer is meaningfully available; do not
  force arbitrary all-or-nothing success when the product accepts text-only
  metrics.

Main boundary:

- Keep `main.c` moving toward lifecycle and dispatch only: window lifecycle,
  root layer discovery, module initialization/deinitialization order, service
  subscription/unsubscription, and AppMessage callback entry points.
- Move buffers, formatting, rendering state, and module-owned data contracts
  out of `main.c` when a clean behavior-preserving boundary exists.
- Temperature, battery, steps, BPM, and date/time display state should be
  owned by their cohesive modules rather than by `main.c`.

Lifecycle and files:

- Do not fight Pebble lifecycle. Verify official Pebble/Rebble patterns before
  inventing lifecycle control flags or synchronous assumptions.
- Failure semantics must be complete across all return paths, including rare
  error branches, not only the common path.
- Track new source files explicitly. Ignore local build/editor artifacts, but
  do not hide new refactor source files in `.gitignore`.

---

## Rectangular Layout Baseline

Rectangular layout is calculated in `layout_calculate_surface()`.
Future layout work must derive frames from root or unobstructed bounds and
known platform invariants. Do not introduce new hard-coded screen positions
when bounds-derived calculations can express the layout.

For Emery (`200x228`):

- content margin: `4`
- row gap: `8`
- icon: `28x28`
- date: `GRect(4, 122, 192, 28)`, centered,
  `FONT_KEY_GOTHIC_24_BOLD`
- time: `GRect(4, 46, 192, 60)`, centered, custom Unbounded 48 when loaded
  on full rectangular displays, with system-font fallback by platform
- rule: 2 px horizontal line from `(4, 114)` to `(196, 114)`
- battery icon: `GRect(4, 4, 28, 28)`, procedural horizontal battery
- battery text: `GRect(36, 8, 40, 20)`, `FONT_KEY_GOTHIC_18_BOLD`
- climate icon: `GRect(124, 4, 28, 28)`, procedural weather glyph
- temperature text: `GRect(156, 8, 40, 20)`, `FONT_KEY_GOTHIC_18`
- steps icon: `GRect(4, 196, 28, 28)`, procedural steps glyph
- steps text: `GRect(36, 200, 40, 20)`, `FONT_KEY_GOTHIC_18`
- BPM icon: `GRect(124, 196, 28, 28)`, procedural BPM glyph
- BPM text: `GRect(156, 200, 40, 20)`, `FONT_KEY_GOTHIC_18`

For compact rectangular displays such as Aplite, Basalt, Diorite, and Flint
(`144x168`):

- content margin: `3`
- row gap: `6`
- icon: `20x21`
- date: `GRect(3, 90, 138, 21)`, centered,
  `FONT_KEY_GOTHIC_14_BOLD`
- time: `GRect(3, 34, 138, 44)`, centered,
  `FONT_KEY_BITHAM_42_MEDIUM_NUMBERS`
- rule: 2 px horizontal line from `(3, 84)` to `(141, 84)`
- battery icon: `GRect(3, 3, 20, 21)`
- battery text: `GRect(26, 6, 29, 15)`, `FONT_KEY_GOTHIC_14_BOLD`
- climate icon: `GRect(89, 3, 20, 21)`
- temperature text: `GRect(112, 6, 29, 15)`, `FONT_KEY_GOTHIC_14`
- steps icon: `GRect(3, 144, 20, 21)`
- steps text: `GRect(26, 147, 29, 15)`, `FONT_KEY_GOTHIC_14`
- BPM icon: `GRect(89, 144, 20, 21)`
- BPM text: `GRect(112, 147, 29, 15)`, `FONT_KEY_GOTHIC_14`

Date and time are centered. Metric text remains left-aligned within its
calculated substratum. Current implementation uses a background stratum with a
horizontal rule; it does not draw a vertical rail.

---

## Visual Semantics

- unavailable text token is `---`
- dark mode uses black background, white rule, Celeste primary text on color,
  Dark Gray unavailable text on color, Electric Blue date, Sunset Orange time,
  and Celeste steps icon
- light mode uses white background, Oxford Blue rule, Cobalt Blue primary text,
  Light Gray unavailable text on color, black date, Sunset Orange time, and
  Cobalt Blue steps icon
- black-and-white displays fall back to legible black-and-white color choices
  via `PBL_IF_COLOR_ELSE` and `gcolor_legible_over()`
- dynamic icon/text colors are owned by the feature module when they depend on
  live state, such as BPM or battery

BPM color zones:

- `<=0` or unavailable: current mode unavailable color
- `1-99`: current mode primary text color
- `100-120`: Magenta
- `>120`: Red

Battery text/icon colors:

- charging: Jaeger Green
- not charging and `>50`: current mode primary text color
- not charging and `21-50`: Rajah on color, legible fallback otherwise
- not charging and `<=20`: Red

Weather state:

- PebbleKit JS sends raw Open-Meteo `weather_code`
- PebbleKit JS uses phone geolocation when available and falls back to OAK,
  the product home location, at `37.85626, -122.21383`
- C maps codes into private weather glyph buckets in
  `src/modules/climate_glyphs.c`
- procedural glyphs are used instead of Carbon/IcoMoon or PDC weather
  assets for now

---

## Watchface Visual Quality

- Optimize for a one-second glance. The user should read time first, then
  supporting facts without searching.
- Preserve the visual hierarchy: date, hero time, rule, health metrics, and
  bottom metrics.
- Keep the Swiss-Rail influence quiet and precise: strong alignment, restrained
  color, deliberate spacing, and no decorative clutter.
- Prefer fewer, clearer visual elements over dense information.
- Keep the time visually dominant. Secondary metrics must not compete with it.
- Use iconography only when it improves recognition at watch scale.
- Keep icons simple enough to remain legible at 28px and on black-and-white
  displays.
- Use stable frame sizes for icon layers and text layers so values such as
  `---`, `100`, `99999`, `90°F`, and `100%` do not shift layout.
- Validate optical alignment, not only mathematical alignment. Small glyphs,
  icons, and baselines may need visual review against screenshots.
- Preserve negative space around the rule and between rows; do not fill every
  available pixel.
- Validate contrast in dark mode, light mode, monochrome, direct sunlight, and
  low light.
- Avoid thin decorative strokes that disappear on monochrome or low-contrast
  screens.
- Avoid hidden assumptions from desktop previews. Use emulator captures for
  visual changes.

---

## Round Display Planning

Do not add `chalk` or `gabbro` until there is a separate layout plan.

The plan must account for:

- circular clipping by vertical row position
- each row's available text width at its own y-position, not only global
  screen width
- row-specific content widths
- shape-specific margins
- date and time text extremes
- health-row and bottom-row differences
- icon legibility near curved edges
- color and monochrome variants
- platform capabilities and missing sensors

Do not scale the rectangular layout wholesale and call it round support.

---

## Validation Checklist

- Current target platform set is explicit.
- SDK 4+ API availability is verified or uncertainty is stated.
- Platform-specific behavior has a fallback.
- Resource filenames and generated IDs match.
- Resource references were audited after any `package.json` resource or media
  change.
- AppMessage keys match across package metadata, Clay, JS, C, docs, and
  manual test commands.
- Manual AppMessage numeric keys were checked after any `package.json`
  message-key removal or reordering.
- Defaults match across C, Clay, README, docs, and persisted settings.
- Text does not clip at likely extremes: `WED · 30 SEP`, `12:59`, `100`,
  `99999`, `---F`, and `100%`.
- Visual QA sample set was considered: dark and light modes, color and
  black-and-white displays, available and unavailable health values, weather
  unavailable, long date, `99999` steps, and `100%` battery.
- When a bug or lifecycle problem is fixed as a pattern, sibling modules and
  analogous call sites were searched and either fixed in the same coherent
  slice or explicitly called out as intentionally out of scope.
- Ephemeral files such as screenshots and temporary buffers are deleted after
  validation, or when a chat-generated buffer has served its purpose. Confirm
  deletion before acting; this overrides any prior blanket permission to
  delete files or folders.
- Inherited Pebble build/runtime validation was run, or the gap is reported.

---

## Publishing Notes

This folder is not yet on GitHub. The next repo task should prep the initial
private GitHub commit:

- build check
- secret audit
- `.gitignore`
- SDK/platform metadata
- README review
- screenshots if useful and approved
- clear initial commit

Do not publish generated build artifacts unless intentionally releasing them
and the user agrees.
