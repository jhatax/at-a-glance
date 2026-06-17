# Agent Guidelines: Life at a Glance Watchface

Inherits from:

- [Universal development principles](../AGENTS.md) (`~/Code/AGENTS.md`)
- [Pebble platform guidelines](../agent-templates/AGENTS-PEBBLE.md)
  (`~/Code/agent-templates/AGENTS-PEBBLE.md`)

Read the inherited files first. This file contains only Life at a Glance facts,
durable product decisions, project-specific guardrails, and validation checks.
`ARCHITECTURE_LEDGER.md` is the current architecture source of truth; start
there for non-trivial code work and update it when a decision changes.

---

## Context Summary

You are working on a mature Pebble watchface in native C with PebbleKit JS and
Clay. Treat it as embedded firmware: design first, make narrow changes, preserve
memory/lifecycle safety, and validate with the Pebble build/emulator path.

Core product values:

1. One-second glance: time first, then supporting facts without searching.
2. Text is primary; icons are secondary recognition aids.
3. Preserve the Swiss-Rail influence: strong alignment, restrained color,
   deliberate spacing, and no decorative clutter.
4. Layout constants are product decisions. Do not change text widths, icon
   sizes, fonts, palette, geometry, platforms, or AppMessage keys in unrelated
   cleanup.
5. Reuse architecture across display shapes, not rectangular coordinates.
   Round layout is round-native and must account for circular clipping.
6. Prefer direct, auditable code. Do not add tiny helper wrappers unless they
   clarify a real boundary or remove meaningful complexity.

Engineering constraints:

1. Audit live source, dirty tree, and the ledger before editing.
2. Keep one coherent slice. Do not mix cleanup, redesign, AppMessage changes,
   platform enablement, and glyph tuning without approval.
3. Preserve user work; never revert unrelated changes.
4. Do not pass or return large watchface/layout structs by value.
5. Keep source lines at or below 100 characters where practical.
6. Fixed-size buffers and `snprintf` only for C string formatting.
7. Every acquired Pebble resource must have a matching destroy path, including
   create-failure paths.
8. Use integer layout/drawing math. Do not introduce floating-point math.
9. For API, lifecycle, layout, or platform changes, discuss the flow and
   invariants before coding and commit only after explicit approval.

---

## Project Facts

- Native C entrypoint: `src/c`
- Watchface modules: `src/modules`
- PebbleKit JS and Clay config: `src/pkjs`
- Manifest, platforms, capabilities, resources, and message keys:
  `package.json`
- Build entry: `wscript`
- Design reference: `DESIGN.md`
- Architecture contract: `ARCHITECTURE_LEDGER.md`
- Round reference: `RoundWatchfacePlan.md`

Required capabilities:

- `configurable` for Clay
- `location` for weather geolocation
- `health` for BPM and steps

Required AppMessage keys:

- `TIME_FORMAT`
- `TEMP_UNIT`
- `TEMPERATURE`
- `WEATHER_CONDITION`
- `IS_DAY`
- `HR_SAMPLE_MINUTES`
- `DISPLAY_MODE`

Persisted defaults:

- time format: `24h`
- temperature unit: `°F`
- HR sampling: `10` minutes
- display mode: light

PebbleKit JS uses phone geolocation when available and falls back to OAK at
`37.85626, -122.21383`.

---

## Architecture Guardrails

- `watchface` owns the live `WatchfaceSurface` storage and lifetime.
- `layout_watchface_initialize(width, height, &surface)` is the single public
  layout initialization contract. The active shape implementation clears and
  prepares the caller-owned surface.
- `layout_update_watchface_style(&surface.style, display_mode)` applies style
  without recalculating geometry.
- The active architect owns geometry and compact/full classification.
- Compact/full is resolved once by the architect and stored on
  `WatchfaceSurfaceStyle.is_compact`; the stylist consumes it and does not
  rederive it from dimensions.
- Architects use private immutable blueprints and private calculated metrics.
  Layout-private metrics must not leak through `WatchfaceSurface`.
- Feature modules own Pebble layer lifecycle. Shared render helpers may create
  or update layers from calculated substrata, but feature modules remain the
  owners.
- Module `create(root, surface)` may retain the calculated surface once.
  Module `refresh()` APIs must not accept `WatchfaceSurface`; pass only narrow
  runtime payloads such as time format or temperature unit.
- `ataglance.c` owns Pebble lifecycle, service subscriptions, settings,
  AppMessage parsing, and dispatch. It includes `watchface.h`, not feature
  module headers.
- `watchface_components.h` is type-focused shared display contract; it must not
  gain behavior.
- Dynamic text/icon colors are module-owned when they depend on live source
  state, such as BPM or battery.

---

## Module Map

- `src/c/ataglance.c`: window lifecycle, services, settings, AppMessage,
  watchface dispatch.
- `src/modules/watchface.c`: runtime clearing house, surface owner, module
  create/destroy order, refresh coordination, event routing.
- `src/modules/watchface_components.h`: palette, font/color roles, surface,
  strata, and substrata types.
- `src/modules/layout.h`: public layout initialization and style API.
- `src/modules/layout_architect.c`: active geometry provider.
- `src/modules/layout_design.h`: private design constants, blueprints, and calculated
  layout structs; cleanup candidate.
- `src/modules/layout_stylist.c`: palette, font-role, custom-font, compact/full
  style consumption.
- `src/modules/substratum_renderer.c/.h`: shared TextLayer/Icon setup, color
  role lookup, icon coordinate scaling, and shared glyph primitives.
- `src/modules/settings.c/.h`: defaults, persistence, validation, HR interval
  mapping.
- `src/modules/date.c/.h`: date text lifecycle and formatting.
- `src/modules/time.c/.h`: time text lifecycle, formatting, custom font.
- `src/modules/climate.c/.h`: weather source state, temperature text, weather
  availability, climate icon lifecycle.
- `src/modules/climate_glyphs.c/.h`: Open-Meteo code mapping and procedural
  weather glyphs.
- `src/modules/battery.c/.h`: battery source state, track/bolt layers, battery
  colors, refresh.
- `src/modules/bpm.c/.h`: BPM source state, text/icon layers, health reads,
  BPM colors.
- `src/modules/steps.c/.h`: steps source state, text/icon layers, health reads,
  steps refresh.

---

## State And Rendering

- Store source state, not redundant derived render state, unless the derived
  value is expensive or impossible to recompute.
- Setters mutate source state only. Rendering happens through
  `watchface_refresh()`.
- `watchface_refresh(WatchfaceUpdateMask updates)` is the public render
  dispatcher. `ataglance.c` knows update categories, not feature internals.
- AppMessage handling in `ataglance.c` accumulates a local update mask and
  makes one coalesced `watchface_refresh()` call.
- Parse raw AppMessage tuples in `ataglance.c`, convert them to typed runtime
  facts, then call narrow watchface setters.
- Do not introduce shared runtime-update packages unless traffic volume or
  atomic multi-field updates justify it.

---

## Text-First Semantics

- Required text-layer creation determines text-bearing module success.
- If a metric text layer cannot be created, do not create or depend on that
  metric icon.
- If an icon layer cannot be created, keep updating the corresponding text.
- Optional icon creation does not gate module success unless the module defines
  that icon as required.
- Assign a module's retained `s_surface` only after required layer creation
  succeeds.
- Failure paths must not leave stale surface pointers, custom fonts, layers,
  bitmaps, or partially-owned state behind.

---

## Product And Visual Semantics

- Full rectangular displays use canonical rectangular blueprint values.
- Compact rectangular displays may use compact rectangular blueprint values.
- Round geometry must account for row-specific safe width, shape-specific
  margins, font extremes, icon legibility near curves, and Chalk/Gabbro
  screenshots.
- Unavailable text token: `---`
- Dark mode: black background, white rule, Celeste primary text, Light Gray
  unavailable text, Electric Blue date, Sunset Orange time, primary-text metric
  icons.
- Light mode: white background, Oxford Blue rule, Cobalt Blue primary text,
  Dark Gray unavailable text, black date, Sunset Orange time, primary-text
  metric icons.
- Black-and-white displays fall back to legible choices via `PBL_IF_COLOR_ELSE`
  and `gcolor_legible_over()`.
- BPM colors: `<=0` or unavailable uses unavailable text; `1-99` uses primary
  text; `100-120` uses Chrome Yellow on dark and Windsor Tan on light; `>120`
  uses Orange on dark and Bulgarian Rose on light.
- Battery colors: charging uses Islamic Green on color and primary text on
  monochrome; not charging `>50` uses primary text; `21-50` uses Rajah on dark
  and Windsor Tan on light; `<=20` uses Red on dark and Bulgarian Rose on
  light.
- Weather/date share one row. Weather owns icon plus temperature; date aligns
  in the remaining space.

---

## Validation Checklist

Use the parent Pebble checklist plus these repo checks:

- Target platform set in `package.json` is intentional.
- AppMessage keys match across `package.json`, Clay, PebbleKit JS, C, docs, and
  manual test commands.
- Manual AppMessage numeric keys were checked after key removal or reordering.
- Defaults match across C, Clay, README, docs, and persisted settings.
- Resource filenames and generated IDs match.
- `PBL_HEALTH` guards protect `bpm.h`, `steps.h`, and every `Health*` symbol.
- No feature-module `*_refresh()` declaration accepts `WatchfaceSurface`.
- Text does not clip at likely extremes: `WED 30 SEP`, `12:59`, `100`,
  `99999`, `---F`, and `100%`.
- Visual QA considers dark/light, color/BW, available/unavailable health,
  weather unavailable, long date, `99999` steps, and `100%` battery.
- `git diff --check` passes.
- `pebble build` runs for code changes, or the validation gap is reported.
- Emulator screenshots are used for visual/layout changes when practical.
- Temporary screenshots and buffers are deleted unless the user keeps them.

---

## Publishing Notes

This folder is not yet on GitHub. Before publishing, run a build, audit
secrets, verify `.gitignore`, SDK/platform metadata, README, screenshots if
approved, and the initial commit contents. Do not publish generated build
artifacts unless intentionally releasing them with user approval.
