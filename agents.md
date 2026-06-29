# Agent Guidelines: At a Glance Watchface

Inherits from:

- [Universal development principles](../AGENTS.md) (`~/Code/AGENTS.md`)
- [Pebble platform guidelines](../agent-templates/AGENTS-PEBBLE.md)
  (`~/Code/agent-templates/AGENTS-PEBBLE.md`)

Read inherited files first.

Use this file for:

- repo facts
- durable product decisions
- project-specific guardrails
- repo-specific validation

Primary architecture source of truth:

- `ARCHITECTURE_LEDGER.md`

For non-trivial work:

1. Read the inherited files.
2. Read `ARCHITECTURE_LEDGER.md`.
3. Audit live code and the dirty tree.
4. Keep the patch narrow.

---

## Context

- Mature Pebble watchface in native C with PebbleKit JS and Clay
- Treat as embedded firmware
- Design first
- Validate before staging or committing

Core product values:

1. One-second glance
2. Time first
3. Text primary, icons secondary
4. Swiss-Rail influence: alignment, restraint, spacing, no clutter
5. Layout constants are product decisions
6. Reuse architecture across shapes, not rectangular coordinates
7. Prefer direct, auditable code

Engineering constraints:

1. Audit source, docs, generated files, and dirty tree before editing
2. Keep one coherent slice
3. Do not mix cleanup, redesign, AppMessage changes, platform work, and glyph tuning
4. Preserve existing user work
5. Do not pass or return large watchface/layout structs by value
6. Keep source lines at or below 100 chars where practical
7. Use fixed-size buffers and `snprintf`
8. Match every Pebble resource acquisition with a destroy path
9. Use integer layout/drawing math only
10. Review API, lifecycle, layout, and platform changes before coding

---

## Repo Facts

- Native C entrypoint: `src/c`
- Watchface modules: `src/modules`
- PebbleKit JS / Clay: `src/pkjs`
- Manifest, resources, platforms, message keys: `package.json`
- Build entry: `wscript`
- Design reference: `DESIGN.md`
- Architecture contract: `ARCHITECTURE_LEDGER.md`
- Round reference: `RoundWatchfacePlan.md`

Required capabilities:

- `configurable`
- `location`
- `health`

Required AppMessage keys:

- `TIME_FORMAT`
- `TEMP_UNIT`
- `TEMPERATURE`
- `WEATHER_CONDITION`
- `IS_DAY`
- `HR_SAMPLE_MINUTES`
- `DISPLAY_MODE`
- `STEPS_GOAL`

Persisted defaults:

- time format: `24h`
- temperature unit: `°F`
- HR sampling: `10` minutes
- display mode: light
- steps goal: `10000`

PebbleKit JS weather fallback:

- OAK: `37.85626, -122.21383`

---

## Architecture Guardrails

- `watchface` owns live `WatchfaceSurface` storage and lifetime
- `layout_watchface_initialize(width, height, &surface)` is the only public layout init contract
- Layout init clears and prepares caller-owned surface
- `layout_watchface_update_palette(&surface.style, display_mode)` restyles without recalculating geometry
- The active architect owns geometry and compact/full classification
- `WatchfaceSurfaceStyle.is_compact` is resolved once by the architect
- The stylist consumes `is_compact`; it does not rederive it
- Blueprints are private and immutable
- Layout-private metrics must not leak through `WatchfaceSurface`
- Feature modules own their Pebble layers and lifecycle
- Shared render helpers may help create/update layers, but do not own them
- `create(root, surface)` may retain prepared surface data once
- `refresh()` APIs must not accept `WatchfaceSurface`
- Pass narrow runtime inputs to `refresh()`, e.g. time format or temp unit
- `ataglance.c` owns Pebble lifecycle, subscriptions, settings, AppMessage parsing, and dispatch
- `ataglance.c` includes `watchface.h`, not feature-module headers
- `watchface_components.h` is shared type vocabulary only
- `helper.h` is shared utility vocabulary
- Dynamic text/icon colors remain module-owned when tied to live source state

---

## Module Map

- `src/c/ataglance.c`: lifecycle, services, settings, AppMessage, watchface dispatch
- `src/modules/watchface.c`: runtime clearing house, surface owner, create/destroy order, refresh routing
- `src/modules/watchface_components.h`: shared palettes, roles, strata, substrata, surface types
- `src/modules/layout.h`: public layout API
- `src/modules/layout_architect.c`: geometry provider
- `src/modules/layout_design.h`: private design constants, blueprints, calculated layout
- `src/modules/layout_stylist.c`: palette, font-role, custom-font, compact/full style consumption
- `src/modules/substratum_renderer.c/.h`: shared text/icon setup, color lookup, coordinate scaling, glyph primitives
- `src/modules/settings.c/.h`: defaults, persistence, validation, HR interval mapping
- `src/modules/date.c/.h`: date text lifecycle and formatting
- `src/modules/time.c/.h`: time text lifecycle, formatting, custom font
- `src/modules/climate.c/.h`: climate source state, temperature text, availability, icon lifecycle
- `src/modules/climate_glyphs.c/.h`: Open-Meteo mapping and procedural weather glyphs
- `src/modules/battery.c/.h`: battery source state, track/bolt layers, colors, refresh
- `src/modules/bpm.c/.h`: BPM source state, text/icon layers, health reads, colors
- `src/modules/steps.c/.h`: steps source state, text/icon layers, health reads, refresh

---

## State And Rendering

- Store source state, not redundant derived render state
- Exception: retain derived state only when recomputation is expensive or impractical
- Setters mutate source state only
- Rendering goes through `watchface_refresh()`
- `watchface_refresh(WatchfaceUpdateMask updates)` is the public render dispatcher
- `ataglance.c` knows update categories, not feature internals
- Parse raw AppMessage tuples in `ataglance.c`
- Convert tuples to typed runtime facts
- Call narrow watchface setters / ingress APIs
- Do not introduce shared runtime-update packets without clear need

---

## Text-First Semantics

- Required text-layer creation determines module success
- If metric text cannot be created, do not depend on its icon
- If icon creation fails, continue updating text when product semantics allow it
- Optional icon creation must not gate module success unless the module explicitly requires it
- Assign retained module state only after required layer creation succeeds
- Failure paths must not leave stale pointers, partial ownership, layers, fonts, or bitmaps behind

---

## Product And Visual Semantics

- Full rectangular displays use canonical full rectangular blueprint values
- Compact rectangular displays may use compact rectangular blueprint values
- Round geometry must account for safe width, margins, font extremes, clipping, and Chalk/Gabbro validation
- Unavailable text token: `---`

Dark mode:

- background: black
- rule: white
- primary text: Celeste
- unavailable text: Light Gray
- date: Electric Blue
- time: Sunset Orange
- metric icons: primary text

Light mode:

- background: white
- rule: Oxford Blue
- primary text: Cobalt Blue
- unavailable text: Dark Gray
- date: black
- time: Sunset Orange
- metric icons: primary text

Other visual rules:

- B/W displays use `PBL_IF_COLOR_ELSE` and `gcolor_legible_over()`
- BPM colors:
  - unavailable or `<=0`: unavailable text
  - `1-99`: primary text
  - `100-120`: Chrome Yellow on dark, Windsor Tan on light
  - `>120`: Orange on dark, Bulgarian Rose on light
- Battery colors:
  - charging: Islamic Green on color, primary text on monochrome
  - `>50`: primary text
  - `21-50`: Rajah on dark, Windsor Tan on light
  - `<=20`: Red on dark, Bulgarian Rose on light
- Weather/date share one row
- Weather owns icon + temperature
- Date aligns in the remaining row space

---

## Validation

- Confirm target platforms in `package.json` are intentional
- Confirm AppMessage keys match across `package.json`, Clay, JS, C, docs, and manual test commands
- Recheck manual numeric AppMessage keys after any key add/remove/reorder
- Confirm defaults match across C, Clay, README, docs, and persisted settings
- Confirm resource filenames and generated IDs match
- Guard all `Health*` symbols and health headers with `PBL_HEALTH`
- No feature-module `*_refresh()` API may accept `WatchfaceSurface`
- Check text clipping at:
  - `WED 30 SEP`
  - `12:59`
  - `100`
  - `99999`
  - `---F`
  - `100%`
- Visual QA must cover:
  - dark/light
  - color/BW
  - available/unavailable health
  - weather unavailable
  - long date
  - `99999` steps
  - `100%` battery
- Run `git diff --check`
- Run `pebble build` for code changes, or explicitly report the gap
- Use emulator screenshots for visual/layout changes when practical
- Delete temporary screenshots/buffers unless intentionally kept

---

## Publishing

- Repo is not yet public
- Before publishing:
  - run a build
  - audit secrets
  - verify `.gitignore`
  - verify SDK/platform metadata
  - verify README
  - verify screenshots if included
  - verify initial commit contents
- Do not publish generated build artifacts unless explicitly approved
