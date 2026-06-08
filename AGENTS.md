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

- `src/c/main.c`: window lifecycle, service subscriptions,
  settings/palette selection, AppMessage receive flow, and event routing.
- `src/modules/watchface_composer.c`: screen assembly, root layer
  discovery, shared system fonts, module creation/destruction order,
  full-display refresh coordination, and tick-driven date/time refresh.
- `src/modules/layout.c`: rectangular frame calculation through
  `layout_calculate()`, filling caller-owned `WatchfaceLayout` storage.
- `src/modules/display.c`: palette selection, unavailable token, and common
  text-layer display updates.
- `src/modules/settings.c`: persisted settings defaults, validation, load,
  save, and HR sampling interval mapping.
- `src/modules/helper.h`: shared static inline icon-scaling helpers.
- `src/modules/date.c`: date text layer, date buffer, uppercase date
  formatting, and date refresh.
- `src/modules/time.c`: time text layer, time buffer,
  time-format rendering, and time refresh.
- `src/modules/weather.c`: raw Open-Meteo weather-code mapping and
  procedural weather glyph rendering, plus temperature state, text
  layer, formatting, and weather availability.
- `src/modules/health.c`: BPM and steps layers, procedural health icons,
  health text buffers, colors, and health-service update handling.
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

Code style:

- Keep programming source lines at or below 80 characters where practical.

Review and pushback:

- Push back when a change deviates from established platform lifecycle,
  module ownership, product semantics, or repository hygiene. Name the
  deviation plainly and explain the safer local pattern.

Prevent drift:

- Reuse existing sentinels, enum names, macros, message keys, and product
  vocabulary. Do not introduce adjacent names for an existing concept.
- When changing a platform-bound header or API, audit every include and every
  caller for required platform guards. In particular, `health.h` and any
  `Health*` symbols must remain behind `PBL_HEALTH` guardrails.
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

Module ownership:

- `watchface_composer` is the owner of screen composition state. Do not force
  `main.c` to shuttle window, settings, palette, or layout state through every
  module call when composer can own the composition boundary cleanly.
- Keep weather state and procedural weather glyph drawing separate when the
  drawing logic becomes too large to audit inside `weather.c`. The weather
  module should pass explicit render inputs such as condition, frame, and
  palette into glyph rendering rather than exposing weather-owned globals.
- Optional modules should use product semantics for creation success. For
  health, at least one created text layer is meaningfully available; do not
  force arbitrary all-or-nothing success when the product accepts partial text.

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

Rectangular layout is calculated in `layout_calculate()`.
Future layout work must derive frames from root or unobstructed bounds and
known platform invariants. Do not introduce new hard-coded screen positions
when bounds-derived calculations can express the layout.

For Emery (`200x228`):

- derived spacing: `PBL_DISPLAY_HEIGHT / 28`, currently `8`
- date: `GRect(8, 8, 184, 20)`, `FONT_KEY_GOTHIC_18_BOLD`
- time: `GRect(8, 58, 184, 48)`, `FONT_KEY_BITHAM_42_BOLD`
- rule: line from `(8, 114)` to `(192, 114)`
- heart icon: `GRect(8, 118, 28, 28)`, procedural heart
- BPM text: `GRect(38, 122, 28, 20)`, `FONT_KEY_GOTHIC_18`
- steps icon: `GRect(122, 118, 28, 28)`, procedural paw glyph
- steps text: `GRect(152, 122, 40, 20)`, `FONT_KEY_GOTHIC_18`
- weather icon: `GRect(8, 196, 28, 28)`, procedural weather glyph
- temperature text: `GRect(38, 200, 40, 20)`, `FONT_KEY_GOTHIC_18`
- battery icon: `GRect(128, 196, 28, 28)`, procedural horizontal battery
- battery text: `GRect(158, 200, 34, 20)`,
  `FONT_KEY_GOTHIC_18_BOLD`

Text alignment is intentionally left-aligned across all columns.

Current implementation does not draw a vertical rail. It uses a horizontal
rule at `y=114`.

---

## Visual Semantics

- unavailable text token is `---`
- dark mode uses black background, light gray primary text, Windsor Tan
  unavailable text on color displays, Rich Brilliant Lavender date,
  Sunset Orange time, Light Gray rule, and Chrome Yellow steps icon
- light mode uses white background, black primary text, Light Gray
  unavailable text on color displays, Imperial Purple date, Sunset Orange
  time, Light Gray rule, and Chrome Yellow steps icon
- black-and-white displays use inverted unavailable backgrounds so missing
  values remain legible without color
- unavailable icon backgrounds should match unavailable text backgrounds
  unless an explicit product decision says otherwise

BPM color zones:

- `<=0` or unavailable: current mode unavailable color
- `1-99`: Jaeger Green
- `100-120`: Magenta
- `>120`: Red

Battery text/icon colors:

- charging: Jaeger Green
- not charging and `>50`: Cobalt Blue
- not charging and `21-50`: Yellow
- not charging and `<=20`: Red

Weather state:

- PebbleKit JS sends raw Open-Meteo `weather_code`
- PebbleKit JS uses phone geolocation when available and falls back to OAK,
  the product home location, at `37.85626, -122.21383`
- C maps codes into private weather glyph buckets in
  `src/modules/weather.c`
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
